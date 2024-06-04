// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <algorithm>
#include <Microsoft.UI.Xaml.hosting.referencetracker.h>
#include <windows.ui.core.corewindow-defs.h>

namespace Private
{
    //#define LifetimeTrace(...) TRACE(TraceAlways, __VA_ARGS__)
    #define LifetimeTrace(...)

#if DBG
    extern ULONG g_wuxeDOInstancesCount;
#endif

    //
    // Allows ReferenceTrackerRuntimeClass instances to split their implementation into different
    // classes whose instance has an identical lifetime to the ReferenceTrackerRuntimeClass instance.
    //
    template <typename TReferenceTrackerRuntimeClass>
    class ReferenceTrackerHelper
    {
    public:
        explicit ReferenceTrackerHelper(_In_ TReferenceTrackerRuntimeClass* pRuntimeClass)
            : m_pRuntimeClassNoRef(pRuntimeClass)
        { ASSERT(m_pRuntimeClassNoRef); }

        template <typename T>
        _Check_return_ HRESULT SetPtrValue(_In_ TrackerPtr<T>& ptr, _In_ T* ptrValue)
        {
            RRETURN(m_pRuntimeClassNoRef->SetPtrValue(ptr, ptrValue));
        }

        template <typename T>
        _Check_return_ HRESULT RegisterTrackerPtrVector(_In_ TrackerPtrVector<T>& ptrVector)
        {
            RRETURN(m_pRuntimeClassNoRef->RegisterTrackerPtrVector(ptrVector));
        }

    private:
        TReferenceTrackerRuntimeClass* m_pRuntimeClassNoRef = nullptr;
    };

    //
    // Represents a composable WinRT RuntimeClass instance that integrates with the tracker
    // reference mechanism in MUX and is capable of walking its registered tracker objects.
    // This should be the base class of all MUX extensions that inherits (directly or indirectly)
    // from DependencyObject.
    // Tracker pointers should be used instead of wrl::ComPtr for class members. Use
    // SetPtrValue to set their value.
    //
    template <typename I0,
        typename I1 = wrl::Details::Nil, typename I2 = wrl::Details::Nil, typename I3 = wrl::Details::Nil,
        typename I4 = wrl::Details::Nil, typename I5 = wrl::Details::Nil, typename I6 = wrl::Details::Nil,
        typename I7 = wrl::Details::Nil, typename I8 = wrl::Details::Nil>
    class __declspec(novtable) ReferenceTrackerRuntimeClass
        : public wrl::RuntimeClass<
                    // It's important to inhibit WRL's weak ref implementation so that we use the MUX implementation
                    // instead. The latter disconnects the weak ref source when this object becomes unreachable, thus
                    // avoiding bad resurrection situations where we might end up holding references to neutred CCWs.
                    wrl::InhibitWeakReferencePolicy,
                    I0, I1, I2, I3, I4, I5, I6, I7, I8>
        , public ITrackerHandleManager
        , public ::IReferenceTrackerExtension
    {
    public:
        using RuntimeClassT = typename wrl::RuntimeClass<wrl::InhibitWeakReferencePolicy, I0, I1, I2, I3, I4, I5, I6, I7, I8>::RuntimeClassT;

        ReferenceTrackerRuntimeClass()
        {
            LifetimeTrace(L"WUXExt DO count: %lu \tthis: %p (%s)", InterlockedIncrement(&g_wuxeDOInstancesCount), this, s_typeName);
        }

        virtual ~ReferenceTrackerRuntimeClass()
        {
            LifetimeTrace(L"WUXExt DO count: %lu \tthis: %p (%s)", InterlockedDecrement(&g_wuxeDOInstancesCount), this, s_typeName);
        }

        // Non-delegating QI (the AddRef is delegated though).
        _Check_return_
        STDMETHOD(QueryInterface)(_In_ REFIID riid, _Outptr_ void** ppInterface) override
        {
            // If we had used WinRtClassicComMix, we will limit the number of interfaces we can
            // let derived classes use. To avoid that, we implement the QI for supported
            // COM interfaces here.

            if (RuntimeClassT::GetRefCount() > 0)
            {
                // We only implement IReferenceTrackerExtension if we're not composed (subclassed)
                // by another class; this interface has to be implemented by the controlling unknown,
                // because that's the object that controls the ref count.  Implementing this interface
                // communicates that you return valid ref counts from Release().
                if (InlineIsEqualGUID(riid, __uuidof(::IReferenceTrackerExtension)) && !IsComposed())
                {
                    *ppInterface = static_cast<::IReferenceTrackerExtension*>(this);
                    static_cast<IUnknown*>(*ppInterface)->AddRef();
                }

                else if (!QueryInterfaceOverride(riid, ppInterface))
                {
                    return RuntimeClassT::QueryInterface(riid, ppInterface);
                }
            }
            else
            {
                // After we dropped to a ref count of zero, something is QI-ing us.
                // This means that while we are running our destructor, some code tried to
                // resolve a weak reference on us.
                // Usually, WinRT components disconnect their weak references when they are being
                // destroyed to avoid this situation. But since we inhibited the WRL weakref
                // implementation to use the MUX implementation instead, we lost that control.
                // By returning null, it won't be possible to resolve the weak reference and
                // it will look like our weak references got disconnected.
                // Once we release our reference on the base class (the one and only), the weak
                // references will be truly disconnected. It's only the timeframe between running
                // our destructor and calling release on the base class instance that's problematic.
                *ppInterface = nullptr;
            }

            return S_OK;
        }

        _Check_return_
        virtual bool QueryInterfaceOverride(_In_ REFIID /*riid*/, _Outptr_ void** /*ppInterface*/)
        {
            return false;
        }

        // Non-delegating AddRef
        STDMETHOD_(ULONG, AddRef)() override
        {
            ULONG refCount = RuntimeClassT::AddRef();
            // LifetimeTrace(L"WUXExt DO AddRef \t\tcount: %lu\tthis: %p (%s)", refCount, this, s_typeName);
            return refCount;
        }

        // Non-delegating Release
        STDMETHOD_(ULONG, Release)() override
        {
            ULONG refCount = RuntimeClassT::InternalRelease();
            // LifetimeTrace(L"WUXExt DO Release \tcount: %lu\tthis: %p (%s)", refCount, this, s_typeName);

            if (refCount == 0)
            {
                bool deleteThis = true;

                // Safe guard to prevent re-entrency.
                RuntimeClassT::InternalAddRef();

#if DBG
                ASSERT(m_wasEnsureCalled);
#endif

                if (m_pTrackerOwnerInnerNoRef)
                {
                    // Try to queue the delete over to the UI thread.
                    // (This will return !queued if we're already on the UI thread)

                    auto queued = false;
                    VERIFYHR(TryQueueForFinalRelease(&queued));
                    if (queued)
                    {
                        // We are queued for finalization on the UI thread.
                        // We already AddRef'ed ourselves back to a ref count of 1.
                        deleteThis = false;
                    }
                }

                if (deleteThis)
                {
                    DeleteThis();
                }
            }

            return refCount;
        }

        void DeleteThis()
        {
            RuntimeClassT::InternalRelease();
            delete this;

            auto module = ::Microsoft::WRL::GetModuleBase();
            if (module != nullptr)
            {
                module->DecrementObjectCount();
            }
        }

        //
        // ITrackerPtrWrapperManager interface
        //

        _Check_return_ HRESULT NewTrackerHandle(_Outptr_ ::TrackerHandle* pValue) final
        {
#if DBG
            ASSERT(m_wasEnsureCalled && m_pTrackerOwnerInnerNoRef);
#endif
            return m_pTrackerOwnerInnerNoRef->CreateTrackerHandle(pValue);
        }

        _Check_return_ HRESULT DeleteTrackerHandle(_In_ ::TrackerHandle handle) final
        {
#if DBG
            ASSERT(m_wasEnsureCalled && m_pTrackerOwnerInnerNoRef);
#endif
            return m_pTrackerOwnerInnerNoRef->DeleteTrackerHandle(handle);
        }

        _Check_return_ HRESULT SetTrackerValue(_In_ ::TrackerHandle handle, _In_opt_ IUnknown* pValue) final
        {
#if DBG
            ASSERT(m_wasEnsureCalled && m_pTrackerOwnerInnerNoRef);
#endif
            return m_pTrackerOwnerInnerNoRef->SetTrackerValue(handle, pValue);
        }

        _Success_(!!return) _Check_return_ BOOLEAN TryGetSafeTrackerValue(_In_ ::TrackerHandle handle, _COM_Outptr_result_maybenull_ IUnknown** ppValue) final
        {
#if DBG
            ASSERT(m_wasEnsureCalled && m_pTrackerOwnerInnerNoRef);
#endif
            return m_pTrackerOwnerInnerNoRef->TryGetSafeTrackerValue(handle, ppValue);
        }

        // ComObject<T> leaves this false, AggregableComObject<> overrides this to return true
        // we're composed
        virtual boolean IsComposed()
        {
            return false;
        }

        bool ShouldFallbackToComPointers() final
        {
            // If we are running in an old designer (win 8.1), m_pTrackerOwnerInnerNoRef is going to be null because
            // the MUX binary that shipped with it doesn't expose the ref tracking extension mechanism.
#if DBG
            ASSERT(m_wasEnsureCalled);
#endif
            return (m_pTrackerOwnerInnerNoRef == nullptr);
        }

        //
        // Initialization routines
        //

        // Note: RuntimeClassT::SetComposableBasePointers is not virtual.
        template <typename FactoryInterface>
        _Check_return_ HRESULT
        SetComposableBasePointers(_In_ IInspectable* base, _In_ FactoryInterface* baseFactory)
        {
            ASSERT(base);
            IFC_RETURN(RuntimeClassT::SetComposableBasePointers(base, baseFactory));
            IFC_RETURN(EnsureReferenceTrackerInterfaces());

            return S_OK;
        }

        _Check_return_ HRESULT EnsureReferenceTrackerInterfaces()
        {
#if DBG
            // Should be called once after the object is initialized.
            ASSERT(!m_wasEnsureCalled && GetComposableBase());
#endif
            wrl::ComPtr<::ITrackerOwner> spTrackerOwnerInner;

            if (SUCCEEDED(this->GetComposableBase().As(&spTrackerOwnerInner)))
            {
                m_pTrackerOwnerInnerNoRef = spTrackerOwnerInner.Get();

                // Capture the thread dispatcher so that we can use it for cleanup
                wrl::ComPtr<xaml::IDependencyObject> trackerOwnerDO;

                IFC_RETURN(this->GetComposableBase().As(&trackerOwnerDO));

                IFC_RETURN(trackerOwnerDO->get_Dispatcher(&m_coreDispatcher));
            }
            else
            {
                // In the past, the only situation where we would fail to query IReferenceTrackerInternal
                // is if we are running in an old designer that shipped with a MUX binary that didn't
                // support reference tracker extensions.
                // This is no longer an issue but we are keeping the COM fallback support around.
            }

#if DBG
            m_wasEnsureCalled = true;
#endif
            return S_OK;
        }

    protected:
        //
        // Tracker pointers methods
        //

        template <typename T>
        _Check_return_ HRESULT SetPtrValue(_In_ TrackerPtr<T>& ptr, _In_ T* ptrValue)
        {
            IFC_RETURN(RegisterReferenceTrackerBase(ptr));
            IFC_RETURN(ptr.Set(ptrValue));

            return S_OK;
        }

        template <typename T>
        _Check_return_ HRESULT RegisterTrackerPtrVector(_In_ TrackerPtrVector<T>& ptrVector)
        {
            return RegisterReferenceTrackerBase(ptrVector);
        }

        template <typename T>
        _Check_return_ HRESULT RegisterEventSource(_In_ TrackerEventSource<T>& eventSource)
        {
            return RegisterReferenceTrackerBase(eventSource);
        }

    private:
        _Check_return_ HRESULT RegisterReferenceTrackerBase(
            _In_ ReferenceTrackerBase& tracker)
        {
            if (tracker.HasOwner())
            {
                ASSERT(tracker.GetOwner() == this);
            }
            else
            {
                tracker.RegisterOwner(this /* pOwner */);
            }
            return S_OK;
        }



    private:


        // Post a call to DeleteThis() to the UI thread.  If we're already on the UI thread, then just
        // return false.  If we're off the UI thread but can't get to it, then do the DeleteThis() here (asynchronously).
        _Check_return_ HRESULT TryQueueForFinalRelease( _Out_ bool* queued )
        {
            *queued = false;
            wrl::ComPtr<wuc::ICoreDispatcher2> coreDispatcher2;

            // (We won't have a m_coreDispatcher when running in the designer)
            if(m_coreDispatcher)
            {
                boolean hasThreadAccess;

                // See if we're on the UI thread
                IFC_RETURN(m_coreDispatcher->get_HasThreadAccess(&hasThreadAccess));

                if(!hasThreadAccess)
                {
                    // We're not on the UI thread

                    wrl::ComPtr<wf::IAsyncOperation<bool>> asyncOperation;
                    auto handler = MakeAgileDispatcherCallback([this]() -> HRESULT
                    {
                        // This is the code that will run on the UI thread
                        DeleteThis();
                        return S_OK;
                    });

                    // Post to the UI thread's dispatcher (if it's still pumping)
                    IFC_RETURN(m_coreDispatcher.As(&coreDispatcher2));
                    coreDispatcher2->TryRunAsync(
                        wuc::CoreDispatcherPriority_Normal,
                        handler.Get(), &asyncOperation);

                    *queued = true;

                    // If the post asynchronously fails (because the UI thread is gone), resort to an off-thread cleanup.
                    asyncOperation->put_Completed(
                        wrl::Callback<wf::IAsyncOperationCompletedHandler<bool>>(
                            [this](wf::IAsyncOperation<bool>* asyncInfo, wf::AsyncStatus asyncStatus) -> HRESULT
                            {
                                auto deleteThis = false;

                                if( asyncStatus == wf::AsyncStatus::Completed )
                                {
                                    boolean succeeded = false;
                                    VERIFYHR(asyncInfo->GetResults(&succeeded));
                                    if(!succeeded)
                                    {
                                        deleteThis = true;
                                    }
                                }

                                if(deleteThis)
                                {
                                    DeleteThis();
                                }

                                return S_OK;
                            }).Get());

                }
            }

            return S_OK;
        }


    private:
        ::ITrackerOwner* m_pTrackerOwnerInnerNoRef = nullptr;
        wrl::ComPtr<wuc::ICoreDispatcher> m_coreDispatcher; // The Dispatcher of the thread we were created on

#if DBG
        bool m_wasEnsureCalled = false;
        static const LPCWSTR s_typeName;
        UINT32 m_threadID = ::GetCurrentThreadId();

        _Check_return_ HRESULT CheckThread() const
        {
            if (m_threadID != ::GetCurrentThreadId())
            {
                return RPC_E_WRONG_THREAD;
            }
            return S_OK;
        }
#endif

        template<typename TReferenceTrackerRuntimeClass> friend class ReferenceTrackerHelper;
    };
}
