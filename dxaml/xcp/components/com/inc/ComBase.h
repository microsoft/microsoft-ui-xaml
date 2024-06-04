// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

enum class KnownTypeIndex: UINT16;

#include <minerror.h>
#include "WeakReferenceImpl.h"
#include "ComPtr.h"
#include <ComMacros.h>
#include <FeatureFlags.h>

#if defined(_X86_) || defined(_AMD64_)

#define UnknownInterlockedCompareExchangePointer InterlockedCompareExchangePointer
#define UnknownInterlockedCompareExchangePointerForIncrement InterlockedCompareExchangePointer

#elif defined(_ARM_) || defined(_ARM64_)

#define UnknownInterlockedCompareExchangePointer InterlockedCompareExchangePointer
#define UnknownInterlockedCompareExchangePointerForIncrement InterlockedCompareExchangePointerNoFence

#else

#error Unsupported architecture.

#endif

namespace ctl
{
    interface INonDelegatingUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE NonDelegatingQueryInterface(
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ __RPC__deref_out void __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        virtual ULONG STDMETHODCALLTYPE NonDelegatingAddRef( void) = 0;
        virtual ULONG STDMETHODCALLTYPE NonDelegatingRelease( void) = 0;
    };

    interface INonDelegatingInspectable: public INonDelegatingUnknown
    {
        virtual HRESULT STDMETHODCALLTYPE NonDelegatingGetIids(
            /* [out] */ __RPC__out ULONG *iidCount,
            /* [size_is][size_is][out] */ __RPC__deref_out_ecount_full_opt(*iidCount) IID **iids) = 0;

        virtual HRESULT STDMETHODCALLTYPE NonDelegatingGetRuntimeClassName(
            /* [out] */ __RPC__deref_out_opt HSTRING *className) = 0;

        virtual HRESULT STDMETHODCALLTYPE NonDelegatingGetTrustLevel(
            /* [out] */ __RPC__out TrustLevel *trustLevel) = 0;
    };

    //
    // This is the CTL equivalent of the WRL::RuntimeClass.
    // It implements reference counting and support weak references for
    // child classes that implements IWeakReferenceSource.
    // ComBase uses the same approach and implementation the WRL uses for ref counting.
    // The strong reference count and the weak reference pointer are multiplexed
    // in the ReferenceCountOrWeakReferencePointer union. After switching to the
    // "weak reference pointer" mode, we never go back to manipulating the strong ref count.
    // We let the WeakReferenceImpl instance do that from that point.
    // The switch and memory access to the union is synchronized using a lock free algorithm
    // we took from the WRL.
    //
    class ComBase: public IInspectable
    {
    protected:
        friend class WeakReferenceSourceNoThreadId;
        friend class ComObjectBase;

        ComBase()
            : m_pUnkFTM(nullptr)
        {
            refCount_.rawValue = 1;
        }

        virtual ~ComBase()
        {
            ReleaseInterface(m_pUnkFTM);
            if (Details::IsValueAPointerToWeakReference(refCount_.rawValue))
            {
                Details::WeakReferenceImpl* weakRef = Details::DecodeWeakReferencePointer(refCount_.rawValue);
                weakRef->Release();
                weakRef = nullptr;
            }
        }

        virtual KnownTypeIndex GetTypeIndex() const;

    public:
        BOOL IsComposed()
        {
            return AsNonDelegatingInspectable() != nullptr;
        }

        virtual INonDelegatingUnknown* AsNonDelegatingInspectable()
        {
            return nullptr;
        }

    public:

        ULONG AddRefDirect()
        {
            // Direct AddRef, even if this is an aggregated object
            return AddRefImpl();
        }

        ULONG ReleaseDirect()
        {
            // Direct Release, even if this is an aggregated object
            return ReleaseImpl();
        }

        #if XCP_MONITOR && DBG
        void DisableLeakCheck()
        {
            // Should be called right after creation.
            ASSERT(refCount_.rawValue == 1 && !m_disableLeakCheck);
            m_disableLeakCheck = true;
        }
        #endif

        bool IsInFinalRelease() const
        { return m_inFinalRelease; }

    protected:

        // IUnknown implementation
        virtual _Check_return_ HRESULT QueryInterfaceImpl(REFIID iid, void** ppObject)
        {
            IUnknown* pInterface = nullptr;

            if (InlineIsEqualGUID(iid, IID_IUnknown))
            {
                pInterface = static_cast<IUnknown *>(this);
            }
            else if (InlineIsEqualGUID(iid, IID_IInspectable))
            {
                pInterface = static_cast<IInspectable *>(this);
            }
            else if (InlineIsEqualGUID(iid, IID_IMarshal))
            {
                IFC_RETURN(EnsureFTM());
                IFC_RETURN(m_pUnkFTM->QueryInterface(iid, ppObject));
                return S_OK;
            }
            else
            {
                return E_NOINTERFACE;
            }

            *ppObject = pInterface;
            pInterface->AddRef();

            return S_OK;
        }

        ctl::ComPtr<IWeakReference> GetWeakReferenceImpl()
        {
            Details::WeakReferenceImpl* weakRef = nullptr;
            auto currentValue = Microsoft::WRL::Details::ReadValueFromPointerNoFence<Details::ReferenceCountOrWeakReferencePointer>(&refCount_);

            if (Details::IsValueAPointerToWeakReference(currentValue.rawValue))
            {
                weakRef = Details::DecodeWeakReferencePointer(currentValue.rawValue);
                return ctl::ComPtr<IWeakReference>(weakRef);
            }

            // WeakReferenceImpl is created with ref count 2 to avoid interlocked increment
            weakRef = Details::CreateWeakReference(this);
            if (weakRef == nullptr)
            {
                FAIL_FAST_USING_EXISTING_ERROR_CONTEXT(E_OUTOFMEMORY);
            }

            INT_PTR encodedWeakRef = Details::EncodeWeakReferencePointer(weakRef);

            for (;;)
            {
                INT_PTR previousValue = 0;

                weakRef->SetStrongReference(static_cast<unsigned long>(currentValue.refCount));

                previousValue = reinterpret_cast<INT_PTR>(UnknownInterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&(this->refCount_.ifHighBitIsSetThenShiftLeftToYieldPointerToWeakReference)), reinterpret_cast<PVOID>(encodedWeakRef), currentValue.ifHighBitIsSetThenShiftLeftToYieldPointerToWeakReference));
                if (previousValue == currentValue.rawValue)
                {
                    // No need to call AddRef in this case, WeakReferenceImpl is created with ref count 2 to avoid interlocked increment
#if DBG
                    if (m_disableLeakCheck)
                    {
                        ::XcpDebugSetLeakDetectionFlag(weakRef, true);
                    }
#endif
                    ctl::ComPtr<IWeakReference> result;
                    result.Attach(weakRef);
                    return result;
                }
                else if (Details::IsValueAPointerToWeakReference(previousValue))
                {
                    // Another thread beat this call to create the weak reference.

                    delete weakRef;

                    weakRef = Details::DecodeWeakReferencePointer(previousValue);
                    return ctl::ComPtr<IWeakReference>(weakRef);
                }

                // Another thread won via an AddRef or Release.
                // Let's try again
                currentValue.rawValue = previousValue;
            }
        }

        virtual void OnFinalRelease()
        {
            delete this;
        }

        ULONG AddRefImpl()
        {
            auto currentValue = Microsoft::WRL::Details::ReadValueFromPointerNoFence<Details::ReferenceCountOrWeakReferencePointer>(&refCount_);

            for (;;)
            {
                if (!Details::IsValueAPointerToWeakReference(currentValue.rawValue))
                {
                    UINT_PTR updateValue = currentValue.refCount + 1;

                    INT_PTR previousValue = reinterpret_cast<INT_PTR>(UnknownInterlockedCompareExchangePointerForIncrement(reinterpret_cast<PVOID*>(&(refCount_.refCount)), reinterpret_cast<PVOID>(updateValue), reinterpret_cast<PVOID>(currentValue.refCount)));
                    if (previousValue == currentValue.rawValue)
                    {
                        return static_cast<unsigned long>(updateValue);
                    }

                    currentValue.rawValue = previousValue;
                }
                else
                {
                    Details::WeakReferenceImpl* weakRef = Details::DecodeWeakReferencePointer(currentValue.rawValue);
                    return weakRef->IncrementStrongReference();
                }
            }
        }

        ULONG ReleaseImpl()
        {
            auto currentValue = Microsoft::WRL::Details::ReadValueFromPointerNoFence<Details::ReferenceCountOrWeakReferencePointer>(&refCount_);
            unsigned long refCount;
            for (;;)
            {
                if (!Details::IsValueAPointerToWeakReference(currentValue.rawValue))
                {
                    UINT_PTR updateValue = currentValue.refCount - 1;

                    INT_PTR previousValue = reinterpret_cast<INT_PTR>(UnknownInterlockedCompareExchangePointerForIncrement(reinterpret_cast<PVOID*>(&(refCount_.refCount)), reinterpret_cast<PVOID>(updateValue), reinterpret_cast<PVOID>(currentValue.refCount)));
                    if (previousValue == currentValue.rawValue)
                    {
                        refCount = static_cast<unsigned long>(updateValue);
                        break;
                    }

                    currentValue.rawValue = previousValue;
                }
                else
                {
                    Details::WeakReferenceImpl* weakRef = Details::DecodeWeakReferencePointer(currentValue.rawValue);
                    refCount = weakRef->DecrementStrongReference();
                    break;
                }
            }

            if (refCount == 0)
            {
                ASSERT(false == m_inFinalRelease);
                m_inFinalRelease = true;

                if (m_ignoreReleases)
                {
                    // This flag was set because the object was added to the UIAffinityReleaseQueue. The flag will
                    // be unset when the queue processes this object. If we're in here, it means someone other than
                    // the UIAffinityReleaseQueue is trying to release this object

                    // Fast fail so we can identify the culprit
                    IFCFAILFAST(E_ILLEGAL_METHOD_CALL);
                }

                // We decremented the ref. count making the assumption that OnFinalRelease
                // will destroy this object. However, it's possible the destruction is delayed
                // to make it happen on the UI thread. As such, we should put our ref. count back
                // to 1, so we can properly retry the Release on the UI thread, and have it go
                // through OnFinalRelease again.
                AddRefImpl();

                OnFinalRelease();
            }

            return refCount;
        }

        ULONG AddRefOuter()
        {
            return static_cast<IUnknown*>(this)->AddRef();
        }

    public:
        ULONG GetRefCount()
        {
            auto currentValue = Microsoft::WRL::Details::ReadValueFromPointerNoFence<Details::ReferenceCountOrWeakReferencePointer>(&refCount_);

            if (Details::IsValueAPointerToWeakReference(currentValue.rawValue))
            {
                Details::WeakReferenceImpl* weakRef = Details::DecodeWeakReferencePointer(currentValue.rawValue);
                return weakRef->GetStrongReferenceCount();
            }
            else
            {
                return static_cast<unsigned long>(currentValue.refCount);
            }
        }

    private:

        IID const * const * GetLocalImplementedIIDs(_Out_ INT *count)
        {
            static IID const * const localIIDs[] =
            {
                &IID_IUnknown,
                &IID_IInspectable
            };

            *count = _countof(localIIDs);

            return localIIDs;
        }

        INT GetLocalImplementedIIDsCount()
        {
            INT count = 0;

            GetLocalImplementedIIDs(&count);

            return count;
        }

        _Check_return_ HRESULT EnsureFTM()
        {
            HRESULT hr = S_OK;
            IUnknown* pUnkFTM = NULL;

            if (m_pUnkFTM)
            {
                goto Cleanup;
            }

            IFC(CoCreateFreeThreadedMarshaler(this, &pUnkFTM));

            if (NULL == InterlockedCompareExchangePointer(reinterpret_cast<void**>(&m_pUnkFTM), pUnkFTM, NULL))
            {
                pUnkFTM = NULL;
            }

        Cleanup:
            ReleaseInterface(pUnkFTM);

            RRETURN(hr);
        }

    protected:

        // The Top of the chain of implemented IIDs
        virtual INT GetImplementedIIDsCount()
        {
            return GetLocalImplementedIIDsCount();
        }

        virtual void CopyIIDsToArray(INT first, IID *pResult)
        {
            INT count = 0;
            IID const * const * pLocalIIDs = GetLocalImplementedIIDs(&count);

            for (INT current = 0; current < count; current++)
            {
                pResult[first + current] = *(pLocalIIDs[current]);
            }
        }

    protected:

        // IInspectable implementation
        virtual _Check_return_ HRESULT GetRuntimeClassNameImpl(_Out_ HSTRING *pClassName)
        {
            IFCPTR_RETURN(pClassName);
            *pClassName = NULL;

            return S_OK;
        }

        _Check_return_ HRESULT GetIidsImpl(_Out_ ULONG *iidCount, _Outptr_ IID **iids)
        {
            IID *pResult = NULL;
            ULONG count = 0;

            count = GetImplementedIIDsCount();

            pResult = reinterpret_cast<IID *>(CoTaskMemAlloc(sizeof(IID) * count));
            if (pResult == NULL)
            {
                return E_OUTOFMEMORY;
            }

            CopyIIDsToArray(0, pResult);
            *iids = pResult;
            *iidCount = count;

            return S_OK;
        }

    protected:
        // Prepares object's state
        virtual _Check_return_ HRESULT Initialize()
        {
            RRETURN(S_OK);
        }

        void ResurrectWeakReference()
        {
            auto currentValue = Microsoft::WRL::Details::ReadValueFromPointerNoFence<Details::ReferenceCountOrWeakReferencePointer>(&refCount_);

            if (Details::IsValueAPointerToWeakReference(currentValue.rawValue))
            {
                Details::WeakReferenceImpl* weakRef = Details::DecodeWeakReferencePointer(currentValue.rawValue);
                return weakRef->Resurrect(this);
            }
        }

        bool m_inFinalRelease = false;
        bool m_ignoreReleases = false;
#if DBG
        bool m_disableLeakCheck = false;
#endif

    public:
        void DisableIgnoreReleases() { m_ignoreReleases = false; }

#if __XAML_UNITTESTS__
    protected:
#else
    private:
#endif
        IUnknown* m_pUnkFTM;
        Details::ReferenceCountOrWeakReferencePointer refCount_;
    };

    class TearoffSupportErrorInfo final :
        public ISupportErrorInfo
    {
    public:
        TearoffSupportErrorInfo(IInspectable* owner) : m_owner(owner), m_cRef(1)
        {
            m_owner->AddRef();
        }

        ~TearoffSupportErrorInfo()
        {
            m_owner->Release();
        }

        IFACEMETHODIMP QueryInterface(_In_ REFIID riid, _Outptr_ void** ppv) override
        {
            if (InlineIsEqualGUID(riid, __uuidof(ISupportErrorInfo)))
            {
                *ppv = static_cast<ISupportErrorInfo*>(this);
                AddRef();
                return S_OK;
            }
            else
            {
                return m_owner->QueryInterface(riid, ppv);
            }
        }

        IFACEMETHODIMP_(ULONG) AddRef() override
        {
            return InterlockedIncrement(&m_cRef);
        }

        IFACEMETHODIMP_(ULONG) Release() override
        {
            ULONG cRef = InterlockedDecrement(&m_cRef);
            if (!cRef)
                delete this;

            return cRef;
        }

        IFACEMETHODIMP InterfaceSupportsErrorInfo(_In_ REFIID iid) override
        {
            UNREFERENCED_PARAMETER(iid);
            return S_OK;
        }

    private:
        xref_ptr<IInspectable> m_owner;
        ULONG m_cRef;
    };

    class SupportErrorInfo:
        public ComBase
    {
    protected:
        SupportErrorInfo() = default;
        ~SupportErrorInfo() override = default;

        BEGIN_INTERFACE_MAP(SupportErrorInfo, ComBase)
            INTERFACE_ENTRY(SupportErrorInfo,ISupportErrorInfo)
        END_INTERFACE_MAP(SupportErrorInfo, ComBase)

    protected:

        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, __uuidof(ISupportErrorInfo)))
            {
                *ppObject = new TearoffSupportErrorInfo(this);
                return S_OK;
            }
            else
            {
                return ComBase::QueryInterfaceImpl(iid, ppObject);
            }
        }
    };

    template <typename T>
    struct IsComObject
    {
        static const bool value = std::is_base_of<ComBase, T>::value;
    };

    inline IUnknown* iunknown_cast(_In_opt_ ComBase* instance)
    {
        return instance;
    }

    inline IInspectable* iinspectable_cast(_In_opt_ ComBase* instance)
    {
        return instance;
    }

    template<typename T>
    inline typename std::enable_if<!IsComObject<T>::value, T*>::type down_cast(_In_opt_ IUnknown* instance)
    {
        return static_cast<T*>(instance);
    }

    template<typename T>
    inline typename std::enable_if<IsComObject<T>::value, T*>::type down_cast(_In_opt_ IUnknown* instance)
    {
        return static_cast<T*>(static_cast<ComBase*>(instance));
    }
}

#undef UnknownInterlockedCompareExchangePointer
#undef UnknownInterlockedCompareExchangePointerForIncrement
