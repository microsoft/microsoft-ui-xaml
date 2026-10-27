// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <AllocationDiagnostics.h>
#include <GraphicsResourceDiagnostics.h>
#include <GraphicsResourceCounters.h>
#include <XcpAllocation.h>

namespace AllocationDiagnostics
{
    HRESULT GetMemorySnapshot(_Out_ XamlMemorySnapshot* snapshot)
    {
        static_assert(GraphicsResourceCounters::ResourceKindCount <= XamlMemorySnapshotResourceCapacity);
        IFCPTR_RETURN(snapshot);
        *snapshot = {};
        LARGE_INTEGER start{};
        IFCW32_RETURN(QueryPerformanceCounter(&start));

        const auto allocations = XcpAllocation::GetAllocationSnapshot();
        snapshot->Size = sizeof(*snapshot);
        snapshot->Version = 1;
        snapshot->Flags = XamlMemorySnapshot_WinUICoreObservedScope | XamlMemorySnapshot_NonTransactional;
        if (allocations.allocationCountsAvailable)
        {
            snapshot->Flags |= XamlMemorySnapshot_AllocationCountsAvailable;
        }
        if (allocations.outstandingAllocationCountsAvailable)
        {
            snapshot->Flags |= XamlMemorySnapshot_OutstandingAllocationCountsAvailable;
        }
        if (allocations.memoryTrackingEnabled)
        {
            snapshot->Flags |= XamlMemorySnapshot_AllocatedMemoryTrackingEnabled;
        }
        snapshot->AllocCount = allocations.allocCount;
        snapshot->AllocSize = allocations.allocSize;
        snapshot->AllocatedMemory = allocations.allocatedMemory;
        snapshot->OutstandingAllocCount = allocations.outstandingAllocCount;
        snapshot->OutstandingAllocSize = allocations.outstandingAllocSize;

        IFC_RETURN(GraphicsResourceDiagnostics::GetCounterCount(&snapshot->ResourceCount));
        for (UINT32 index = 0; index < snapshot->ResourceCount; ++index)
        {
            const auto result = GraphicsResourceDiagnostics::GetCounterSnapshot(index, &snapshot->Resources[index]);
            if (result != HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED))
            {
                IFC_RETURN(result);
            }
        }

        LARGE_INTEGER end{};
        IFCW32_RETURN(QueryPerformanceCounter(&end));
        snapshot->ReadStartQpc = start.QuadPart;
        snapshot->ReadEndQpc = end.QuadPart;
        return S_OK;
    }
}
