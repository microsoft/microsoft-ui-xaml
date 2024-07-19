// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <wrl\async.h>

#include <minerror.h>
#include <vector>
#include <unordered_map>

//
// This file contains the tracker primitives we use in MUX extensions.
// They are similar to the one available in MUX. We don't include those
// because it would have required a lot of refactoring (for example, use WRL instead
// of the CTL). Also, specific to MUX Extensions, we need to fallback to using untracked COM pointers
// whe running in the designer.
//

#if !_HAS_EXCEPTIONS
    #define VERIFYSTL(exp) {exp;}
    #define VERIFYSTL_RETURN(exp) {exp;}
#else
    #define IFCSTL(exp) { try { exp; } catch (const std::exception&) { IFC(E_UNEXPECTED); } }
    #define IFCSTL_RETURN(exp) { try { exp; } catch (const std::exception&) { IFC_RETURN(E_UNEXPECTED); } }
    #define VERIFYSTL(exp) IFCSTL(exp)
    #define VERIFYSTL_RETURN(exp) IFCSTL_RETURN(exp)
#endif

namespace Private
{
    //
    // Allows manipulation of TrackerHandle instances.
    //
    struct __declspec(novtable) ITrackerHandleManager
    {
        virtual _Check_return_ HRESULT NewTrackerHandle(_Outptr_ ::TrackerHandle* pValue) = 0;
        virtual _Check_return_ HRESULT DeleteTrackerHandle(_In_ ::TrackerHandle value) = 0;
        virtual _Check_return_ HRESULT SetTrackerValue(_In_ ::TrackerHandle handle, _In_opt_ IUnknown* pValue) = 0;
        virtual _Success_(!!return) _Check_return_ BOOLEAN TryGetSafeTrackerValue(_In_ ::TrackerHandle handle, _COM_Outptr_result_maybenull_ IUnknown** ppValue) = 0;
        virtual bool ShouldFallbackToComPointers() = 0;
    };

    //
    // Base class for all tracker pointer primitives.
    //
    class __declspec(novtable) ReferenceTrackerBase
    {
    protected:
        ReferenceTrackerBase() = default;

        explicit ReferenceTrackerBase(_In_ ITrackerHandleManager* pOwner)
            : m_pOwnerNoRef(pOwner)
        {}

        ReferenceTrackerBase(const ReferenceTrackerBase&) = delete;
        ReferenceTrackerBase& operator=(const ReferenceTrackerBase&) = delete;

        ReferenceTrackerBase(ReferenceTrackerBase&& other)
            : m_pOwnerNoRef(other.m_pOwnerNoRef)
        {
            other.m_pOwnerNoRef = nullptr;
        }

        ReferenceTrackerBase& operator=(ReferenceTrackerBase&& other)
        {
            if (this != std::addressof(other))
            {
                // You really don't want to be swapping around trackers with different owners
                ASSERT(!HasOwner());
                ReferenceTrackerBase(std::move(other)).Swap(*this);
            }
            return *this;
        }

        void Swap(ReferenceTrackerBase& other)
        {
            std::swap(m_pOwnerNoRef, other.m_pOwnerNoRef);
        }

    public:
        void RegisterOwner(_In_ ITrackerHandleManager* pOwner)
        {
            // A tracker should never change owner.
            ASSERT(!m_pOwnerNoRef || m_pOwnerNoRef == pOwner);
            m_pOwnerNoRef = pOwner;
        }

        bool HasOwner() const
        {
            return m_pOwnerNoRef != nullptr;
        }

        ITrackerHandleManager* GetOwner() const
        {
            return m_pOwnerNoRef;
        }

    private:
        ITrackerHandleManager* m_pOwnerNoRef = nullptr;
    };

    //
    // Base class for tracker pointer primitives that own
    // TrackerPtr references (i.e. TrackerPtrVector<T> and TrackerEventSource<T>).
    //
    class TrackerPtrWrapperManager
        : public ReferenceTrackerBase
        , public ITrackerHandleManager
    {
        //
        // ITrackerPtrWrapperManager interface
        //

        _Check_return_ HRESULT NewTrackerHandle(_Outptr_::TrackerHandle* pValue) override
        {
            ASSERT(HasOwner());
            return GetOwner()->NewTrackerHandle(pValue);
        }

        _Check_return_ HRESULT DeleteTrackerHandle(_In_::TrackerHandle value) override
        {
            ASSERT(HasOwner());
            return GetOwner()->DeleteTrackerHandle(value);
        }

        _Check_return_ HRESULT SetTrackerValue(_In_ ::TrackerHandle handle, _In_opt_ IUnknown* pValue) override
        {
            ASSERT(HasOwner());
            return GetOwner()->SetTrackerValue(handle, pValue);
        }

        _Success_(return) _Check_return_ BOOLEAN TryGetSafeTrackerValue(_In_ ::TrackerHandle handle, _COM_Outptr_result_maybenull_ IUnknown** ppValue) override
        {
            ASSERT(HasOwner());
            return GetOwner()->TryGetSafeTrackerValue(handle, ppValue);
        }

        bool ShouldFallbackToComPointers() override
        {
            ASSERT(HasOwner());
            return GetOwner()->ShouldFallbackToComPointers();
        }

    protected:
        explicit TrackerPtrWrapperManager(_In_ ITrackerHandleManager* owner)
            : ReferenceTrackerBase(owner)
        {}

        TrackerPtrWrapperManager() = default;
    };

    // Forward declarations.
    template <typename I0, typename I1, typename I2, typename I3, typename I4,
              typename I5, typename I6, typename I7, typename I8>
    class ReferenceTrackerRuntimeClass;
    template<class T>
    class TrackerPtrVector;

    //
    // Represents a tracked reference when set to a CCW or an IReferenceTrackerInternal object.
    // Otherwise, behaves like a ref counted COM reference.
    //
    template<class T>
    class TrackerPtr final
        : public ReferenceTrackerBase
    {
    public:
        TrackerPtr() = default;

        // explicit to avoid usages such as _tpMyTrackerPtr = nullptr.
        // call Clear instead.
        explicit TrackerPtr(ITrackerHandleManager* pOwner)
            : ReferenceTrackerBase(pOwner)
        { }

        ~TrackerPtr()
        {
            if (m_handle)
            {
                ASSERT(HasOwner());
                VERIFYHR(GetOwner()->DeleteTrackerHandle(m_handle));
            }
        }

        // Move semantics are available for TrackerPtrVector<T> to be used
        // carefully so that we don't end up with two TrackerPtr instances
        // with the same data.
        // Move constructor.
        TrackerPtr(TrackerPtr&& other)
            : ReferenceTrackerBase(static_cast<ReferenceTrackerBase&&>(other))
            , m_handle(std::move(other.m_handle))
            , m_pFallbackPtr(std::move(other.m_pFallbackPtr))
            , m_pValueNoRef(std::move(other.m_pValueNoRef))
        {
            other.m_handle = nullptr;
            other.m_pFallbackPtr = nullptr;
            other.m_pValueNoRef = nullptr;
        }
        // Move assignment operator.
        TrackerPtr& operator=(TrackerPtr&& other)
        {
            if (this != std::addressof(other))
            {
                // Unsupported use case.
                ASSERT(!m_handle && !HasOwner());
                TrackerPtr(std::move(other)).Swap(*this);
            }
            return *this;
        }

        void Swap(TrackerPtr& other)
        {
            ReferenceTrackerBase::Swap(other);
            std::swap(m_handle, other.m_handle);
            m_pFallbackPtr.Swap(other.m_pFallbackPtr);
            std::swap(m_pValueNoRef, other.m_pValueNoRef);
        }

    private:
        // To keep things simple, copy and assignment are not allowed.
        // This is because it would require to re-register the pointers.
        TrackerPtr(const TrackerPtr&) = delete;
        TrackerPtr& operator=(const TrackerPtr&) = delete;

    public:
        T* Get() const
        {
#if DBG
            // The Get implementation in MUX has a bunch of debug tests.
            auto succeeded = false;

            if (m_handle)
            {
                HRESULT hr = S_OK;
                wrl::ComPtr<IUnknown> spUnknown;
                succeeded = GetOwner()->TryGetSafeTrackerValue(m_handle, &spUnknown);
                ASSERT(succeeded);

                wrl::ComPtr<T> spValue;
                if (spUnknown)
                {
                    hr = spUnknown.As(&spValue);
                    ASSERTSUCCEEDED(hr);
                }

                if (spUnknown && m_pValueNoRef)
                {
                    wrl::ComPtr<T> storedValue(m_pValueNoRef);
                    wrl::ComPtr<IUnknown> storedValueAsUnknown;

                    ASSERTSUCCEEDED(storedValue.As(&storedValueAsUnknown));
                    ASSERT(spUnknown.Get() == storedValueAsUnknown.Get());
                }
                else
                {
                    ASSERT(!spUnknown && !m_pValueNoRef);
                }

                return spValue.Get();
            }
#endif
            return m_pValueNoRef;
        }

        bool TryGetSafeReference(_COM_Outptr_result_maybenull_ T** ppValue) const
        {
            bool succeeded = false;
            *ppValue = nullptr;

            if (m_pValueNoRef == nullptr)
            {
                succeeded = true;
            }
            else
            {
                if (m_handle)
                {
                    wrl::ComPtr<IUnknown> spResult;
                    succeeded = GetOwner()->TryGetSafeTrackerValue(m_handle, &spResult);
                    if (spResult && succeeded)
                    {
                        succeeded = SUCCEEDED(spResult.CopyTo(ppValue));
                    }
                }
                else
                {
                    ASSERT(m_pFallbackPtr);
                    // We are holding a strong ref, so it's always safe to use it.
                    VERIFYHR(m_pFallbackPtr.CopyTo(ppValue));
                    succeeded = true;
                }
            }

            return succeeded;
        }

        void Clear()
        {
            m_pFallbackPtr = nullptr;
            m_pValueNoRef = nullptr;
            if (m_handle)
            {
                ASSERT(HasOwner());
                VERIFYHR(GetOwner()->SetTrackerValue(m_handle, static_cast<IUnknown*>(nullptr)));
            }
        }

        //
        // Helpers
        //
        explicit operator bool() const
        {
            return (m_pValueNoRef != nullptr);
        }

        T* operator-> () const
        {
            return Get();
        }

        bool operator==(const TrackerPtr& rhs) const
        {
            return Get() == rhs.Get();
        }

        bool operator!=(const TrackerPtr& rhs) const
        {
            return !(*this == rhs);
        }

        bool operator==(const T* rhs) const
        {
            return Get() == rhs;
        }

        bool operator!=(const T* rhs) const
        {
            return !(*this == rhs);
        }

        template <typename U>
        _Check_return_ HRESULT As(_Inout_ wrl::Details::ComPtrRef<wrl::ComPtr<U>> pOtherRef) const
        {
            T* pValue = Get();
            U** ppValue = pOtherRef.ReleaseAndGetAddressOf();

            if (pValue)
            {
                IFC_RETURN(pValue->QueryInterface(__uuidof(U), reinterpret_cast<void**>(ppValue)));
            }
            return S_OK;
        }

        void CopyTo(_Outptr_ T** ppValue) const
        {
            *ppValue = wrl::ComPtr<T>(Get()).Detach();
        }

        template<typename U>
        _Check_return_ HRESULT CopyTo(_Outptr_result_maybenull_ U** ppValue) const
        {
            T* pValue = Get();
            if (pValue)
            {
                return wrl::ComPtr<T>(pValue).CopyTo(ppValue);
            }
            else
            {
                *ppValue = nullptr;
                return S_OK;
            }
        }

        template <typename U>
        wrl::ComPtr<U> AsOrNull() const
        {
            wrl::ComPtr<U> spResult;
            IGNOREHR(As(&spResult));
            return spResult;
        }

    private:
        _Check_return_ HRESULT Set(_In_ T* pValue)
        {
            ASSERT(HasOwner());

            if (GetOwner()->ShouldFallbackToComPointers())
            {
                m_pFallbackPtr = pValue;
            }
            // Optimization: only create the handle if pValue isn't null or,
            // if it is, we are already set to a non-null value.
            else
            {
                if (!m_handle && pValue)
                {
                    IFC_RETURN(GetOwner()->NewTrackerHandle(&m_handle));
                }
                if (m_handle)
                {
                    IFC_RETURN(GetOwner()->SetTrackerValue(m_handle, pValue));
                }
            }

            m_pValueNoRef = pValue;
            return S_OK;
        }

    private:
        ::TrackerHandle m_handle = nullptr;
        wrl::ComPtr<T> m_pFallbackPtr;
        T* m_pValueNoRef = nullptr;

        template <typename I0, typename I1, typename I2, typename I3, typename I4,
                  typename I5, typename I6, typename I7, typename I8>
        friend class ReferenceTrackerRuntimeClass;
        friend class TrackerPtrVector<T>;
    };

    template<typename T>
    bool operator==(const T* lhs, const TrackerPtr<T>& rhs)
    {
        return rhs == lhs;
    }

    template<typename T>
    bool operator!=(const T* lhs, const TrackerPtr<T>& rhs)
    {
        return !(lhs == rhs);
    }

    template<typename T>
    bool operator==(nullptr_t lhs, const TrackerPtr<T>& rhs)
    {
        return rhs == lhs;
    }

    template<typename T>
    bool operator!=(nullptr_t lhs, const TrackerPtr<T>& rhs)
    {
        return !(lhs == rhs);
    }

    //
    // Represents a vecotr of TrackerPtr<T> pointers.
    //
    template<class T>
    class TrackerPtrVector final
        : public TrackerPtrWrapperManager
    {
    public:
        TrackerPtrVector() = default;

        explicit TrackerPtrVector(_In_ ITrackerHandleManager* owner)
            : TrackerPtrWrapperManager(owner)
        {}

    private:
        // To keep things simple, copy and assignment are not allowed.
        // This is because it would require to re-register the pointers.
        TrackerPtrVector(const TrackerPtrVector&) = delete;
        TrackerPtrVector& operator=(const TrackerPtrVector&) = delete;

    public:
        T* GetAt(_In_ UINT index)
        {
            ASSERT(index < m_registeredTrackerPtrs.size());
            return m_registeredTrackerPtrs[index].Get();
        }

        bool TryGetSafeReferenceAt(_In_ UINT index, _COM_Outptr_result_maybenull_ T** ppValue) const
        {
            ASSERT(index < m_registeredTrackerPtrs.size());
            return m_registeredTrackerPtrs[index].TryGetSafeReference(ppValue);
        }

        _Check_return_ HRESULT SetAt(_In_ UINT index, _In_ T* pValue)
        {
            ASSERT(index < m_registeredTrackerPtrs.size());
            // No need to take the lock since we're not really
            // modifying the collection, only the TrackerPtr stored in it.
            // Set implementation will take the GC lock.
            IFC_RETURN(m_registeredTrackerPtrs[index].Set(pValue));
            return S_OK;
        }

        _Check_return_ HRESULT InsertAt(_In_ UINT index, _In_ T* pValue)
        {
            ASSERT(index <= m_registeredTrackerPtrs.size());

            TrackerPtr<T> ptr(this);
            IFC_RETURN(ptr.Set(pValue));
            m_registeredTrackerPtrs.insert(m_registeredTrackerPtrs.begin() + index, std::move(ptr));
            return S_OK;
        }

        _Check_return_ HRESULT Append(_In_ T* pValue)
        {
            TrackerPtr<T> ptr(this);

            IFC_RETURN(ptr.Set(pValue));
            m_registeredTrackerPtrs.push_back(std::move(ptr));
            return S_OK;
        }

        void RemoveAt(_In_ UINT index)
        {
            ASSERT(index < m_registeredTrackerPtrs.size());

            // We first move the pointer OUT of the vector before deleting it, in case any reentrancy causes us to hit this vector again
            TrackerPtr<T> ptr = std::move(m_registeredTrackerPtrs[index]);
            m_registeredTrackerPtrs.erase(m_registeredTrackerPtrs.begin() + index);
        }

        void Remove(_In_ T* pValue)
        {
            TrackerPtr<T> ptrToRemove;

            auto handlerToRemove = std::find_if(m_registeredTrackerPtrs.begin(), m_registeredTrackerPtrs.end(),
                [pValue](const TrackerPtr<T>& tracker) { return tracker.Get() == pValue; });

            if (handlerToRemove != m_registeredTrackerPtrs.end())
            {
                // Move the TrackerPtr<T> out of the collection
                ptrToRemove = std::move(*handlerToRemove);
                m_registeredTrackerPtrs.erase(handlerToRemove);
            }

            // Let the ptrToRemove be destructed her
        }

        void Clear()
        {
            // First release each tracker within the collection without taking the lock
            // reach individual Clear operation will take it
            std::for_each(m_registeredTrackerPtrs.begin(), m_registeredTrackerPtrs.end(),
                [](TrackerPtr<T>& tracker) { tracker.Clear(); });

            m_registeredTrackerPtrs.clear();
        }

        typename std::vector<TrackerPtr<T>>::const_iterator Begin() const
        {
            return m_registeredTrackerPtrs.cbegin();
        }

        typename std::vector<TrackerPtr<T>>::const_iterator End() const
        {
            return m_registeredTrackerPtrs.cend();
        }

        UINT Size() const
        {
            return static_cast<UINT>(m_registeredTrackerPtrs.size());
        }

        bool Empty() const
        {
            return m_registeredTrackerPtrs.empty();
        }

    private:
        std::vector<TrackerPtr<T>> m_registeredTrackerPtrs;
    };

    //
    // Represents an event source.
    //
    template<typename TDelegateInterface>
    class TrackerEventSource
        : public TrackerPtrWrapperManager
    {
    public:
        TrackerEventSource() = default;

        TrackerEventSource(_In_ ITrackerHandleManager* owner)
            : TrackerPtrWrapperManager(owner)
        {
        }

    private:
        // To keep things simple, copy and assignment are not allowed.
        // This is because it would require to re-register the pointers.
        TrackerEventSource(const TrackerEventSource&) = delete;
        TrackerEventSource& operator=(const TrackerEventSource&) = delete;

    public:
        _Check_return_ HRESULT Add(_In_ TDelegateInterface* pHandler, _Out_ EventRegistrationToken* pToken)
        {
            ASSERT(pHandler);
            pToken->value = reinterpret_cast<__int64>(pHandler);
            RRETURN(m_delegates.Append(pHandler));
        }

        _Check_return_ HRESULT Remove(_In_ EventRegistrationToken token)
        {
            ASSERT(token.value);
            m_delegates.Remove(reinterpret_cast<TDelegateInterface*>(token.value));
            RRETURN(S_OK);
        }

        _Check_return_ size_t GetSize() const
        {
            return static_cast<size_t>(m_delegates.Size());
        }

        _Check_return_ HRESULT InvokeAll()
        {
            return DoInvoke([](TDelegateInterface* pHandler) -> HRESULT
            { return pHandler->Invoke(); });
        }

        template <typename T0>
        _Check_return_ HRESULT InvokeAll(_In_ T0 arg0)
        {
            return DoInvoke([arg0](TDelegateInterface* pHandler) -> HRESULT
            { return pHandler->Invoke(arg0); });
        }

        template <typename T0, typename T1>
        _Check_return_ HRESULT InvokeAll(_In_ T0 arg0, _In_ T1 arg1)
        {
            return DoInvoke([arg0, arg1](TDelegateInterface* pHandler) -> HRESULT
            { return pHandler->Invoke(arg0, arg1); });
        }

    private:
        template <typename TInvokeMethod>
        _Check_return_ HRESULT DoInvoke(_In_ TInvokeMethod invokeMethod)
        {
            HRESULT invokeHR = S_OK;

            switch (m_delegates.Size())
            {
            case 0:
                // No handlers... Short-circuit.
                break;
            case 1:
                {
                    // One handler... No need to copy things to a temporary list.
                    auto itrDelegate = m_delegates.Begin();
                    ASSERT(itrDelegate != m_delegates.End());
                    wrl::ComPtr<TDelegateInterface> spHandler = (*itrDelegate).Get();

                    invokeHR = invokeMethod(spHandler.Get());
                    if (invokeHR == RPC_E_DISCONNECTED || invokeHR == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE))
                    {
                        // If the event has been disconnected, we swallow the error and remove the handler.
                        // This is consistent with the rest of the WinRT event source implementations.
                        ::RoClearError();
                        EventRegistrationToken token = { reinterpret_cast<__int64>(spHandler.Get()) };
                        IFC_RETURN(Remove(token));
                        invokeHR = S_OK;
                    }

                    IFC_RETURN(invokeHR);
                }
                break;
            default:
                {
                    // Multiple handlers... Copy to temporary list to avoid re-entrancy issues.
                    std::vector<wrl::ComPtr<TDelegateInterface>> handlers;

                    // Copy the list first to prevent re-entrancy problems
                    std::for_each(m_delegates.Begin(), m_delegates.End(),
                        [&handlers](const TrackerPtr<TDelegateInterface>& tracker) {
                        handlers.push_back(tracker.Get());
                    });


                    // Invoke the handlers
                    for (auto iterCopy = handlers.cbegin(); iterCopy != handlers.cend(); ++iterCopy)
                    {
                        auto& spHandler = (*iterCopy);

                        invokeHR = invokeMethod(spHandler.Get());
                        if (invokeHR == RPC_E_DISCONNECTED || invokeHR == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE))
                        {
                            // If the event has been disconnected, we swallow the error and remove the handler.
                            // This is consistent with the rest of the WinRT event source implementations.
                            ::RoClearError();
                            EventRegistrationToken token = { reinterpret_cast<__int64>(spHandler.Get()) };
                            IFC_RETURN(Remove(token));
                            invokeHR = S_OK;
                        }

                        IFC_RETURN(invokeHR);
                    }
                }
                break;
            }

            return S_OK;
        }

    private:
        // The listener delegates are wrapped in a TrackerPtr, so that we can
        // incorporate into the IReferenceTracker lifetime logic.
        TrackerPtrVector<TDelegateInterface> m_delegates{ this };
    };
}
