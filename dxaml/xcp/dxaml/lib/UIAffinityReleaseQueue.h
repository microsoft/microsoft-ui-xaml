// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ITreeBuilder.g.h"
#include "LifetimeUtils.h"

namespace DirectUI
{
    class DXamlCore;

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

        wil::critical_section m_CriticalSection;
        BOOLEAN m_bInCleanup = FALSE;

        // For ITreeBuilder
        BOOLEAN m_bIsRegisteredForCallbacks = FALSE;
        UINT m_cDoCleanupAttempts = 0;
        int m_cleanupCount = 0;

    protected:

        BEGIN_INTERFACE_MAP(UIAffinityReleaseQueue, ctl::WeakReferenceSourceNoThreadId)
            INTERFACE_ENTRY(UIAffinityReleaseQueue, ITreeBuilder)
        END_INTERFACE_MAP(UIAffinityReleaseQueue, ctl::WeakReferenceSourceNoThreadId)

        HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;

    public:

        ~UIAffinityReleaseQueue() override;

        HRESULT QueueUnreachableCleanup( ctl::WeakReferenceSourceNoThreadId *pItem )
        {
            HRESULT hr = S_OK;
            m_queuedObjectsForUnreachableCleanup.emplace_back(pItem, reinterpret_cast<void*>(*reinterpret_cast<uintptr_t*>(pItem)));
            ctl::addref_interface_inner(pItem);
            RRETURN(hr);//RRETURN_REMOVAL
        }

        HRESULT QueueFinalRelease( ctl::WeakReferenceSourceNoThreadId *pItem )
        {
            HRESULT hr = S_OK;
            auto lock = m_CriticalSection.lock();
            m_queuedObjectsForFinalRelease.emplace_back(pItem, reinterpret_cast<void*>(*reinterpret_cast<uintptr_t*>(pItem)));
            RRETURN(hr);//RRETURN_REMOVAL
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
        IFACEMETHOD(get_IsRegisteredForCallbacks)(_Out_ BOOLEAN* value) override;
        IFACEMETHOD(put_IsRegisteredForCallbacks)(_In_ BOOLEAN value) override;



    };
}

