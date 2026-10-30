// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ITreeBuilder.g.h"
#include "LifetimeUtils.h"
#include <atomic>
#include <cstdint>
#include <memory>

namespace DirectUI
{
    class DXamlCore;
    struct ReleaseQueueCleanupTip;

    //+------------------------------------------------------------------------------
    //
    //  UIAffinityReleaseQueue
    //
    //  Objects with UI affinity must be cleaned up on the UI thread.  When reference count reaches 0
    //  on such objects, but the Release call is on the wrong thread, use this queue to ensure cleanup
    //  happens on the right thread.
    //
    //+------------------------------------------------------------------------------
    class UIAffinityReleaseQueue :
        public DirectUI::ITreeBuilder,
        public ctl::WeakReferenceSourceNoThreadId
    {

    private:
        struct ObjectReleaseInfo
        {
            ObjectReleaseInfo(
                ctl::WeakReferenceSourceNoThreadId* ptr,
                const void* vtablePtr)
                : m_ptr(ptr)
                , m_vtablePtr(vtablePtr)
            {}

            ctl::WeakReferenceSourceNoThreadId* m_ptr;

            // Stores pointer to object's v-table separately to provide some debugging information in case m_ptr
            // was overreleased and there is nothing to help in debugging.
            // This can be a RVA instead, which would free half-pointer size for other debugging info we might need.
            const void*                         m_vtablePtr;
        };

        std::vector<ObjectReleaseInfo> m_queuedObjectsForUnreachableCleanup;
        std::vector<ObjectReleaseInfo> m_queuedObjectsForFinalRelease;
        std::atomic<bool> m_hasUnreachableObjects{ false };
        uint64_t m_unreachableQueuedAt{};
        uint64_t m_finalQueuedAt{};
        std::shared_ptr<ReleaseQueueCleanupTip> m_cleanupTip;

        wil::critical_section m_CriticalSection;
        BOOLEAN m_bInCleanup = FALSE;

        // For ITreeBuilder
        BOOLEAN m_bIsRegisteredForCallbacks = FALSE;
        UINT m_cDoCleanupAttempts = 0;
        int m_cleanupCount = 0;

        struct CleanupWakeState
        {
            ctl::WeakRefPtr target;
            std::atomic<bool> pending{ false };
        };
        wrl::ComPtr<msy::IDispatcherQueue> m_wakeDispatcher;
        std::shared_ptr<CleanupWakeState> m_wakeState;

        static _Check_return_ HRESULT RequestCleanupWake(
            _In_opt_ msy::IDispatcherQueue* dispatcher,
            const std::shared_ptr<CleanupWakeState>& state);
        static _Check_return_ HRESULT OnPlatformCleanupWake(const std::shared_ptr<CleanupWakeState>& state);
        static _Check_return_ HRESULT OnCleanupWake(std::shared_ptr<CleanupWakeState> state);

    protected:

        _Check_return_ HRESULT Initialize() override;

        BEGIN_INTERFACE_MAP(UIAffinityReleaseQueue, ctl::WeakReferenceSourceNoThreadId)
            INTERFACE_ENTRY(UIAffinityReleaseQueue, ITreeBuilder)
        END_INTERFACE_MAP(UIAffinityReleaseQueue, ctl::WeakReferenceSourceNoThreadId)

        HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;

    public:

        ~UIAffinityReleaseQueue() override;

        HRESULT QueueUnreachableCleanup( ctl::WeakReferenceSourceNoThreadId *pItem )
        {
            m_queuedObjectsForUnreachableCleanup.emplace_back(pItem, reinterpret_cast<void*>(*reinterpret_cast<uintptr_t*>(pItem)));
            ctl::addref_interface_inner(pItem);
            if (m_queuedObjectsForUnreachableCleanup.size() == 1)
            {
                m_unreachableQueuedAt = GetTickCount64();
            }
            m_hasUnreachableObjects.store(true, std::memory_order_release);
            const auto dispatcher = m_wakeDispatcher;
            const auto wakeState = m_wakeState;
            return RequestCleanupWake(dispatcher.Get(), wakeState);
        }

        HRESULT QueueFinalRelease( ctl::WeakReferenceSourceNoThreadId *pItem )
        {
            wrl::ComPtr<msy::IDispatcherQueue> dispatcher;
            std::shared_ptr<CleanupWakeState> wakeState;
            {
                auto lock = m_CriticalSection.lock();
                m_queuedObjectsForFinalRelease.emplace_back(pItem, reinterpret_cast<void*>(*reinterpret_cast<uintptr_t*>(pItem)));
                if (m_queuedObjectsForFinalRelease.size() == 1)
                {
                    m_finalQueuedAt = GetTickCount64();
                }
                dispatcher = m_wakeDispatcher;
                wakeState = m_wakeState;
            }
            // Shutdown may destroy this queue after unlocking; posting uses only the retained state.
            return RequestCleanupWake(dispatcher.Get(), wakeState);
        }

        BOOLEAN IsEmpty();
        HRESULT GetQueueObjects(wfc::IVector<IInspectable*>* queue);

        HRESULT Cleanup( _In_ BOOLEAN bSync );
        HRESULT DoCleanup( _In_ BOOLEAN bSync, _Out_ BOOLEAN *pbCompleted );

        // ITreeBuilder
        IFACEMETHOD(IsBuildTreeSuspended)(_Out_ BOOLEAN* pReturnValue) override {
            *pReturnValue = FALSE;
            return S_OK;
        }
        IFACEMETHOD(BuildTree)(_Out_ BOOLEAN* returnValue) override;
        IFACEMETHOD(ShutDownDeferredWork)() override;
        _Check_return_ IFACEMETHOD(get_IsRegisteredForCallbacks)(_Out_ BOOLEAN* value) override;
        _Check_return_ IFACEMETHOD(put_IsRegisteredForCallbacks)(_In_ BOOLEAN value) override;



    };
}
