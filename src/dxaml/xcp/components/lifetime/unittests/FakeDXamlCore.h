// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <ReferenceTrackerInterfaces.h>
#include "FakeDependencyObject.h"

namespace ctl
{
    class WeakReferenceSourceNoThreadId;
}

namespace DirectUI
{
    class FakeDXamlCore final
        : public IDXamlCore
    {
    public :
        FakeDXamlCore();
        ~FakeDXamlCore();

        // IPeerTableHost
        PeerTable& GetPeers() override { return m_peers; }
        ReferenceTrackerTable& GetReferenceTrackers() override { return m_referenceTrackers; }

        void AddToReferenceTrackingList(_In_ xaml_hosting::IReferenceTrackerInternal* item) override;
        void RemoveFromReferenceTrackingList(_In_ xaml_hosting::IReferenceTrackerInternal* item) override;
        _Check_return_ HRESULT ShutdownAllPeers() override;

        void QueueObjectForUnreachableCleanup(_In_ ctl::WeakReferenceSourceNoThreadId *item) override { m_unreachableQueue.emplace_back(item); }
        void QueueObjectForFinalRelease(_In_ ctl::WeakReferenceSourceNoThreadId *item) override { m_finalReleaseQueue.emplace_back(item); }

#if DBG
        void SetTrackerPtrsNeedValidation(BOOLEAN value) override { m_fTrackerPtrsNeedValidation = !!value; }
        BOOLEAN GetTrackerPtrsNeedValidation() override { return m_fTrackerPtrsNeedValidation; }
        virtual bool IsInReferenceTrackingList(_In_ xaml_hosting::IReferenceTrackerInternal* pItem) override
        {
            return m_referenceTrackers.find(pItem) != m_referenceTrackers.end();
        }
#endif

        SRWLOCK& GetReferenceSrwLock() override { return m_peerReferenceLock; }
        LONG IncrementReferenceLockEnterCount() override { return ++m_cReferenceLockEnters; }
        LONG DecrementReferenceLockEnterCount() override { return --m_cReferenceLockEnters; }
        LONG GetReferenceLockEnterCount() override { return m_cReferenceLockEnters; }
        void ReferenceTrackerWalkOnCoreGCRoots(_In_ EReferenceTrackerWalkType walkType)override {}
        virtual UINT32 GetThreadId() override { return m_threadId; }

        // IDXamlCore
        bool IsInitializing() const override { return false; }
        bool IsInitialized() override { return true; }
        bool IsShutdown() override { return false; }

        virtual std::uint64_t GetRTWElementCount() const override { return m_rtwElementCount; }
        virtual void SetRTWElementCount(std::uint64_t count) override { m_rtwElementCount = count; }
        virtual std::uint64_t GetRTWTotalCompressedImageSize() const override { return m_rtwCompressedImageSize; }
        virtual void SetRTWTotalCompressedImageSize(std::uint64_t imageSize) override { m_rtwCompressedImageSize = imageSize; }

        BOOLEAN IsFinalReleaseQueueEmpty() { return m_finalReleaseQueue.size() == 0; }
        UINT32 GenerateRawElementProviderRuntimeId() override { ASSERT(false); return 0; }

        static IDXamlCore* GetCurrentDXamlCore() { return s_currentDXamlCore; }

    private:
        PeerTable m_peers;
        ReferenceTrackerTable m_referenceTrackers;

        std::vector<ctl::WeakReferenceSourceNoThreadId*> m_unreachableQueue;
        std::vector<ctl::WeakReferenceSourceNoThreadId*> m_finalReleaseQueue;

        bool m_fTrackerPtrsNeedValidation = false;

        SRWLOCK m_peerReferenceLock;
        LONG m_cReferenceLockEnters = 0;

        static IDXamlCore* s_currentDXamlCore;
        UINT32 m_threadId;

        std::uint64_t m_rtwElementCount = 0;
        std::uint64_t m_rtwCompressedImageSize = 0;
    };
}
