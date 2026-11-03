// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <ResourceDiagnostics.h>
#include <XamlProfilerTracing.h>
#include <GraphicsResourceCounters.h>

namespace ResourceDiagnostics
{
    HRESULT GetResourceSnapShot(_Out_ XamlResourceSnapshot* snapshot)
    {
        IFCPTR_RETURN(snapshot);
        *snapshot = {};

        (void)XamlProfilerLogging::Provider();
        const auto allocations = XcpAllocation::GetAllocationSnapshot();
        snapshot->AllocCount = allocations.allocCount;
        snapshot->AllocSize = allocations.allocSize;
        snapshot->AllocatedMemory = allocations.allocatedMemory;
        snapshot->OutstandingAllocCount = allocations.outstandingAllocCount;
        snapshot->OutstandingAllocSize = allocations.outstandingAllocSize;

        static_assert(GraphicsResourceCounters::ResourceKindCount <= XamlResourceSnapshotGraphicsCapacity);
        for (UINT32 index = 0; index < GraphicsResourceCounters::ResourceKindCount; ++index)
        {
            GraphicsResourceCounters::Snapshot counters{};
            VERIFY(GraphicsResourceCounters::TryGetSnapshot(index, counters));
            snapshot->Resources[index].TotalCreated = counters.created;
            snapshot->Resources[index].TotalDestroyed = counters.destroyed;
            snapshot->Resources[index].LiveCount = counters.live;
        }

        return S_OK;
    }
}
