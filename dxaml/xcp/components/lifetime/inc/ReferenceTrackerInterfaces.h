// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <ReferenceTrackerExtension.h>
#include <unordered_set>

class CoreWindowRootScale;

#if XCP_MONITOR
#include "XcpAllocation.h"
#endif
namespace ctl
{
    class WeakReferenceSourceNoThreadId;
}

namespace DirectUI
{
    class TrackerTargetReference;
}

namespace DirectUI
{
    class DependencyObject;

    enum EReferenceTrackerWalkType
    {
        RTW_Unpeg,
        RTW_Find,
        RTW_Peg,
        RTW_Reachable,
        RTW_GetElementCount,
        RTW_TotalCompressedImageSize,
        RTW_None,

#if DBG
        RTW_TrackerPtrTest
#endif
    };

    //------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Subset of IDxamlCore functionality for peer table management.
    //
    //------------------------------------------------------------------------

#if XCP_MONITOR
    // We use the LeakIgnoringAllocator here because the unordered_set stores it's buckets in a vector that it doesn't resize
    // when you use .clear(). The lookup/insert/erase benefits of unordered_set (compared to std::set) outweigh this little nuance.
    // We should verify during post test cleanup that these tables are clear.
    typedef std::unordered_set<DependencyObject*, std::hash<DependencyObject*>, std::equal_to<DependencyObject*>, XcpAllocation::LeakIgnoringAllocator<DependencyObject*>> PeerTable;
    typedef std::unordered_set<xaml_hosting::IReferenceTrackerInternal*,
                            std::hash<xaml_hosting::IReferenceTrackerInternal*>,
                            std::equal_to<xaml_hosting::IReferenceTrackerInternal*>,
                            XcpAllocation::LeakIgnoringAllocator<xaml_hosting::IReferenceTrackerInternal*>> ReferenceTrackerTable;
#else
    typedef std::unordered_set<DependencyObject*> PeerTable;
    typedef std::unordered_set<xaml_hosting::IReferenceTrackerInternal*> ReferenceTrackerTable;
#endif

    interface IPeerTableHost
    {
        virtual PeerTable& GetPeers() = 0;
        virtual ReferenceTrackerTable& GetReferenceTrackers() = 0;

        virtual void AddToReferenceTrackingList(_In_ xaml_hosting::IReferenceTrackerInternal* pItem) = 0;
        virtual void RemoveFromReferenceTrackingList(_In_ xaml_hosting::IReferenceTrackerInternal* pItem) = 0;
        virtual _Check_return_ HRESULT ShutdownAllPeers() = 0;

        virtual void QueueObjectForUnreachableCleanup(_In_ ctl::WeakReferenceSourceNoThreadId *pItem) = 0;
        virtual void QueueObjectForFinalRelease(_In_ ctl::WeakReferenceSourceNoThreadId *pItem) = 0;

#if DBG
        virtual void SetTrackerPtrsNeedValidation(BOOLEAN value) = 0;
        virtual BOOLEAN GetTrackerPtrsNeedValidation() = 0;
        virtual bool IsInReferenceTrackingList(_In_ xaml_hosting::IReferenceTrackerInternal* pItem) = 0;
#endif

        virtual SRWLOCK& GetReferenceSrwLock() = 0;
        virtual LONG IncrementReferenceLockEnterCount() = 0;
        virtual LONG DecrementReferenceLockEnterCount() = 0;
        virtual LONG GetReferenceLockEnterCount() = 0;
        virtual void ReferenceTrackerWalkOnCoreGCRoots(_In_ EReferenceTrackerWalkType walkType) = 0;
        virtual UINT32 GetThreadId() = 0;
    };

    //------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Full interface implemented by DXamlCore.
    //
    //------------------------------------------------------------------------
    interface IDXamlCore : public IPeerTableHost
    {
        virtual bool IsInitializing() const = 0;
        virtual bool IsInitialized() = 0;
        virtual bool IsShutdown() = 0;

        virtual BOOLEAN IsFinalReleaseQueueEmpty() = 0;
        virtual UINT32 GenerateRawElementProviderRuntimeId() = 0;

        virtual std::uint64_t GetRTWElementCount() const = 0;
        virtual void SetRTWElementCount(std::uint64_t count) = 0;
        virtual std::uint64_t GetRTWTotalCompressedImageSize() const = 0;
        virtual void SetRTWTotalCompressedImageSize(std::uint64_t imageSize) = 0;
    private:
        // For use only by CoreWindowRootScale
        friend class ::CoreWindowRootScale;
    };

    //+---------------------------------------------------------------------------
    //
    //  PReferenceTrackerInternal
    //
    //  Internal pattern for processing IReferenceTracker::FindTrackerTargets, and
    //  pegging/unpegging of tracker targets.
    //
    //+---------------------------------------------------------------------------
    interface PReferenceTrackerInternal
    {
        virtual bool ReferenceTrackerWalk(_In_ EReferenceTrackerWalkType walkType, _In_ bool fIsRoot = false) = 0;
    };
}
