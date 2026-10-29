// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <ResourceDiagnostics.h>
#include <XamlProfilerTracing.h>

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

        // TODO: Populate graphics resource counters in a separate PR.

        return S_OK;
    }
}
