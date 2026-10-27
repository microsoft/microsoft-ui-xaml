// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "GraphicsResourceDiagnosticsTests.h"
#include <GraphicsResourceCounters.h>
#include <GraphicsResourceDiagnostics.h>
#include <AllocationDiagnostics.h>
#include <XcpAllocation.h>

namespace GraphicsResourceDiagnosticsTests
{
    void DiagnosticsTests::MemorySnapshotContract()
    {
        static_assert(sizeof(XamlMemorySnapshot) == 5960);
        VERIFY_ARE_EQUAL(E_POINTER, AllocationDiagnostics::GetMemorySnapshot(nullptr));

        const auto trackingEnabled = XcpAllocation::GetAllocationSnapshot().memoryTrackingEnabled;
        XamlMemorySnapshot snapshot{};
        VERIFY_SUCCEEDED(AllocationDiagnostics::GetMemorySnapshot(&snapshot));
        VERIFY_ARE_EQUAL(static_cast<UINT32>(sizeof(snapshot)), snapshot.Size);
        VERIFY_ARE_EQUAL(UINT32{1}, snapshot.Version);
        VERIFY_ARE_EQUAL(GraphicsResourceCounters::ResourceKindCount, snapshot.ResourceCount);
        VERIFY_IS_TRUE((snapshot.Flags & XamlMemorySnapshot_NonTransactional) != 0);
        VERIFY_IS_TRUE((snapshot.Flags & XamlMemorySnapshot_WinUICoreObservedScope) != 0);
        VERIFY_ARE_EQUAL(trackingEnabled, (snapshot.Flags & XamlMemorySnapshot_AllocatedMemoryTrackingEnabled) != 0);
        VERIFY_IS_TRUE(snapshot.ReadStartQpc > 0);
        VERIFY_IS_TRUE(snapshot.ReadEndQpc >= snapshot.ReadStartQpc);
#if DBG || TRACE_ALLOC
        VERIFY_IS_TRUE((snapshot.Flags & XamlMemorySnapshot_AllocationCountsAvailable) != 0);
#else
        VERIFY_IS_FALSE((snapshot.Flags & XamlMemorySnapshot_AllocationCountsAvailable) != 0);
        VERIFY_ARE_EQUAL(UINT64{0}, snapshot.AllocCount);
        VERIFY_ARE_EQUAL(UINT64{0}, snapshot.AllocSize);
#endif
        for (UINT32 index = 0; index < snapshot.ResourceCount; ++index)
        {
            XamlGraphicsResourceCounterSnapshot expected{};
            const auto result = GraphicsResourceDiagnostics::GetCounterSnapshot(index, &expected);
#if GRAPHICS_RESOURCE_COUNTERS_ENABLED
            VERIFY_SUCCEEDED(result);
#else
            VERIFY_ARE_EQUAL(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED), result);
#endif
            const auto& resource = snapshot.Resources[index];
            VERIFY_ARE_EQUAL(0, wcscmp(expected.Name, resource.Name));
            VERIFY_ARE_EQUAL(expected.Flags, resource.Flags);
            VERIFY_ARE_EQUAL(expected.Size, resource.Size);
            VERIFY_ARE_EQUAL(expected.Version, resource.Version);
        }
        for (UINT32 index = snapshot.ResourceCount; index < XamlMemorySnapshotResourceCapacity; ++index)
        {
            VERIFY_ARE_EQUAL(UINT32{0}, snapshot.Resources[index].Flags);
            VERIFY_ARE_EQUAL(UINT64{0}, snapshot.Resources[index].TotalCreated);
        }
    }

    void DiagnosticsTests::MemorySnapshotValues()
    {
        XcpAllocation::EnableMemoryTracking();
        XamlMemorySnapshot before{};
        const auto beforeResult = AllocationDiagnostics::GetMemorySnapshot(&before);

        using GraphicsResourceCounters::ResourceKind;
        GraphicsResourceCounters::RecordCreated(ResourceKind::DCompSurfaceWrapper);
        void* block = XcpAllocation::OSMemoryAllocateNoFailFast(128);
        XamlMemorySnapshot during{};
        const auto duringResult = AllocationDiagnostics::GetMemorySnapshot(&during);
        const auto blockSize = XcpAllocation::OSMemoryGetBlockSize(block);
        XcpAllocation::OSMemoryFree(block);
        GraphicsResourceCounters::RecordDestroyed(ResourceKind::DCompSurfaceWrapper);
        XamlMemorySnapshot after{};
        const auto afterResult = AllocationDiagnostics::GetMemorySnapshot(&after);

        VERIFY_IS_NOT_NULL(block);
        VERIFY_SUCCEEDED(beforeResult);
        VERIFY_SUCCEEDED(duringResult);
        VERIFY_SUCCEEDED(afterResult);
        VERIFY_IS_TRUE((during.Flags & XamlMemorySnapshot_AllocatedMemoryTrackingEnabled) != 0);
        VERIFY_ARE_EQUAL(before.AllocatedMemory + INT64{128}, during.AllocatedMemory);
        const auto expectedMemory = std::max(INT64{0}, during.AllocatedMemory - static_cast<INT64>(blockSize));
        VERIFY_ARE_EQUAL(expectedMemory, after.AllocatedMemory);
        if ((during.Flags & XamlMemorySnapshot_AllocationCountsAvailable) != 0)
        {
            VERIFY_ARE_EQUAL(before.AllocCount + UINT64{1}, during.AllocCount);
            VERIFY_ARE_EQUAL(before.AllocSize + UINT64{128}, during.AllocSize);
            VERIFY_ARE_EQUAL(during.AllocCount, after.AllocCount);
            VERIFY_ARE_EQUAL(during.AllocSize, after.AllocSize);
        }
        if ((during.Flags & XamlMemorySnapshot_OutstandingAllocationCountsAvailable) != 0)
        {
            VERIFY_ARE_EQUAL(before.OutstandingAllocCount + UINT64{1}, during.OutstandingAllocCount);
            VERIFY_ARE_EQUAL(before.OutstandingAllocSize + blockSize, during.OutstandingAllocSize);
            VERIFY_ARE_EQUAL(before.OutstandingAllocCount, after.OutstandingAllocCount);
            VERIFY_ARE_EQUAL(before.OutstandingAllocSize, after.OutstandingAllocSize);
        }
        const auto index = static_cast<UINT32>(ResourceKind::DCompSurfaceWrapper);
#if GRAPHICS_RESOURCE_COUNTERS_ENABLED
        VERIFY_ARE_EQUAL(before.Resources[index].TotalCreated + UINT64{1}, during.Resources[index].TotalCreated);
        VERIFY_ARE_EQUAL(before.Resources[index].LiveCount + UINT64{1}, during.Resources[index].LiveCount);
        VERIFY_ARE_EQUAL(before.Resources[index].TotalDestroyed + UINT64{1}, after.Resources[index].TotalDestroyed);
        VERIFY_ARE_EQUAL(before.Resources[index].LiveCount, after.Resources[index].LiveCount);
#else
        VERIFY_ARE_EQUAL(UINT64{0}, during.Resources[index].TotalCreated);
        VERIFY_ARE_EQUAL(UINT64{0}, during.Resources[index].LiveCount);
#endif
    }

    void DiagnosticsTests::CounterSnapshotContract()
    {
        static_assert(sizeof(XamlGraphicsResourceCounterSnapshot) == 184);

        VERIFY_ARE_EQUAL(E_POINTER, GraphicsResourceDiagnostics::GetCounterCount(nullptr));
        VERIFY_ARE_EQUAL(E_POINTER, GraphicsResourceDiagnostics::GetCounterSnapshot(0, nullptr));
        UINT32 count = 0;
        VERIFY_SUCCEEDED(GraphicsResourceDiagnostics::GetCounterCount(&count));
        VERIFY_ARE_EQUAL(GraphicsResourceCounters::ResourceKindCount, count);

        XamlGraphicsResourceCounterSnapshot snapshot{};
        VERIFY_ARE_EQUAL(E_INVALIDARG, GraphicsResourceDiagnostics::GetCounterSnapshot(count, &snapshot));
        VERIFY_ARE_EQUAL(E_INVALIDARG, GraphicsResourceDiagnostics::GetCounterSnapshot(UINT32_MAX, &snapshot));

        for (UINT32 index = 0; index < count; ++index)
        {
            const auto result = GraphicsResourceDiagnostics::GetCounterSnapshot(index, &snapshot);
#if GRAPHICS_RESOURCE_COUNTERS_ENABLED
            VERIFY_SUCCEEDED(result);
            VERIFY_IS_TRUE((snapshot.Flags & XamlGraphicsResourceCounter_CreationCountAvailable) != 0);
            VERIFY_IS_TRUE((snapshot.Flags & XamlGraphicsResourceCounter_WinUICoreObservedScope) != 0);
            VERIFY_IS_TRUE((snapshot.Flags & XamlGraphicsResourceCounter_NonTransactional) != 0);
            const bool hasLifetime = index >= static_cast<UINT32>(GraphicsResourceCounters::ResourceKind::DCompSurfaceWrapper);
            VERIFY_ARE_EQUAL(hasLifetime, (snapshot.Flags & XamlGraphicsResourceCounter_WrapperLifetimeAvailable) != 0);
            VERIFY_IS_TRUE(snapshot.ReadStartQpc > 0);
            VERIFY_IS_TRUE(snapshot.ReadEndQpc >= snapshot.ReadStartQpc);
            if (!hasLifetime)
            {
                VERIFY_ARE_EQUAL(UINT64{0}, snapshot.LiveCount);
                VERIFY_ARE_EQUAL(UINT64{0}, snapshot.TotalDestroyed);
            }
#else
            VERIFY_ARE_EQUAL(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED), result);
            VERIFY_ARE_EQUAL(UINT32{0}, snapshot.Flags);
#endif
            VERIFY_ARE_EQUAL(UINT32{1}, snapshot.Version);
            VERIFY_ARE_EQUAL(static_cast<UINT32>(sizeof(snapshot)), snapshot.Size);
            VERIFY_IS_TRUE(wcslen(snapshot.Name) > 0);
        }
    }
}
