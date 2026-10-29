// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XcpAllocation.h"
#include "XAMLTerminateProcessOnOOM.h"
#include <cstdlib>
#include <atomic>

#include "XamlProfilerTracing.h"

using namespace XcpAllocation;

XHANDLE ghHeap = nullptr;

// Tracks total native XAML heap allocation size. Used by
// TriggerCollectionForOrphanedObjects in PerFrameCallback to detect
// when orphaned objects need GC-driven cleanup.
INT64 g_allocatedMemory = 0;

// Memory tracking is gated on this flag — apps without a managed runtime
// (pure C++/WinRT) don't need the orphaned-object GC trigger and would
// otherwise pay overhead on every alloc/free for nothing. The flag is set
// once when the managed runtime registers a reference tracker host
// (see ReferenceTrackerManager::SetReferenceTrackerHost) and stays set
// for the lifetime of the process. Reads are done without an atomic op —
// stale reads during the brief transition window are harmless because the
// counter clamps at zero (so frees of pre-tracked allocations don't go
// negative).
static bool g_memoryTrackingEnabled = false;

void EnsureHeap()
{
    if(ghHeap == nullptr)
    {
        ghHeap = GetProcessHeap();
    }
}

_Check_return_ size_t XcpAllocation::OSMemoryGetBlockSize(_In_opt_ const void *pAddress) noexcept
{
    if (pAddress == nullptr)
    {
        return 0;
    }

    EnsureHeap();

    const SIZE_T size = HeapSize(ghHeap, 0, pAddress);
    return size == static_cast<SIZE_T>(-1) ? 0 : size;
}

std::atomic<size_t> g_allocCount = 0;
std::atomic<size_t> g_allocSize = 0;
std::atomic<size_t> g_deallocCount = 0;
std::atomic<size_t> g_outstandingAllocCount = 0;
std::atomic<size_t> g_outstandingAllocSize = 0;

static void RecordOutstandingAllocation(_In_ const void *pAddress)
{
    const size_t cSize = XcpAllocation::OSMemoryGetBlockSize(pAddress);
    g_outstandingAllocCount.fetch_add(1, std::memory_order_relaxed);
    g_outstandingAllocSize.fetch_add(cSize, std::memory_order_relaxed);
}

size_t XcpAllocation::GetAllocationCount()
{
    return g_allocCount.load(std::memory_order_relaxed);
}

size_t XcpAllocation::GetAllocationSize()
{
    return g_allocSize.load(std::memory_order_relaxed);
}

size_t XcpAllocation::GetDeallocationCount()
{
    return g_deallocCount.load(std::memory_order_relaxed);
}

XcpAllocation::AllocationSnapshot XcpAllocation::GetAllocationSnapshot() noexcept
{
    AllocationSnapshot snapshot{};
    snapshot.allocationCountsAvailable = XamlProfilerTracing::IsAllocationProfilingEnabled();
    snapshot.outstandingAllocationCountsAvailable = snapshot.allocationCountsAvailable;
    if (snapshot.allocationCountsAvailable)
    {
        snapshot.allocCount = GetAllocationCount();
        snapshot.allocSize = GetAllocationSize();
        snapshot.outstandingAllocCount = g_outstandingAllocCount.load(std::memory_order_relaxed);
        snapshot.outstandingAllocSize = g_outstandingAllocSize.load(std::memory_order_relaxed);
    }
    snapshot.memoryTrackingEnabled = g_memoryTrackingEnabled;
    snapshot.allocatedMemory = InterlockedCompareExchange64(&g_allocatedMemory, 0, 0);
    return snapshot;
}

uint64_t XcpAllocation::GetHeapHandle()
{
    EnsureHeap();
    return reinterpret_cast<uint64_t>(ghHeap);
}

bool XcpAllocation::IsUsingPrivateHeap()
{
    EnsureHeap();
    return ghHeap != GetProcessHeap();
}

void XcpAllocation::EnableMemoryTracking()
{
    g_memoryTrackingEnabled = true;
}

void XcpAllocation::UpdateAllocatedMemory(INT64 cSize)
{
    if (!g_memoryTrackingEnabled)
    {
        return;
    }

    // CAS loop that clamps at zero. Clamping handles the case where a free
    // is tracked for memory that was allocated before tracking was enabled
    // (and therefore not tracked on alloc). Without clamping, the counter
    // would go negative and the threshold heuristic would break.
    INT64 oldValue;
    INT64 newValue;
    do
    {
        oldValue = g_allocatedMemory;
        newValue = oldValue + cSize;
        if (newValue < 0)
        {
            newValue = 0;
        }
    } while (InterlockedCompareExchange64(&g_allocatedMemory, newValue, oldValue) != oldValue);
}

_Check_return_ void *XcpAllocation::OSMemoryAllocateFailFast(_In_ size_t cSize)
{
     void *pAddress = NULL;

    EnsureHeap();
    const bool profilerEnabled = XamlProfilerTracing::IsAllocationProfilingEnabled();

    pAddress = HeapAlloc(ghHeap, 0, cSize);
    if (pAddress)
    {
        UpdateAllocatedMemory(cSize);
        if (profilerEnabled)
        {
            RecordOutstandingAllocation(pAddress);
        }
    }
    else
    {
        // Terminate the process on OOM in a predictable way that gives us
        // clear Watson data.
        XAMLTerminateProcessOnMemoryExhaustion(cSize);
    }

    if (profilerEnabled)
    {
        g_allocCount.fetch_add(1, std::memory_order_relaxed);
        g_allocSize.fetch_add(cSize, std::memory_order_relaxed);
    }

    return pAddress;
}

_Check_return_ void *XcpAllocation::OSMemoryAllocateZeroMemoryFailFast(_In_ size_t cSize)
{
    void *pAddress = NULL;

    EnsureHeap();
    const bool profilerEnabled = XamlProfilerTracing::IsAllocationProfilingEnabled();

    pAddress = HeapAlloc(ghHeap, HEAP_ZERO_MEMORY, cSize);
    if (pAddress)
    {
        UpdateAllocatedMemory(cSize);
        if (profilerEnabled)
        {
            RecordOutstandingAllocation(pAddress);
        }
    }
    else
    {
        // Terminate the process on OOM in a predictable way that gives us
        // clear Watson data.
        XAMLTerminateProcessOnMemoryExhaustion(cSize);
    }

    if (profilerEnabled)
    {
        g_allocCount.fetch_add(1, std::memory_order_relaxed);
        g_allocSize.fetch_add(cSize, std::memory_order_relaxed);
    }

    return pAddress;
}

_Check_return_ void *XcpAllocation::OSMemoryAllocateNoFailFast(_In_ size_t cSize)
{
    void* pAddress = NULL;

    EnsureHeap();
    const bool profilerEnabled = XamlProfilerTracing::IsAllocationProfilingEnabled();

    if (profilerEnabled)
    {
        g_allocCount.fetch_add(1, std::memory_order_relaxed);
        g_allocSize.fetch_add(cSize, std::memory_order_relaxed);
    }

    pAddress = HeapAlloc(ghHeap, 0, cSize);
    if (pAddress)
    {
        UpdateAllocatedMemory(cSize);
        if (profilerEnabled)
        {
            RecordOutstandingAllocation(pAddress);
        }
    }

    return pAddress;
}

_Check_return_ void *XcpAllocation::OSMemoryResize(_Frees_ptr_opt_ void *pAddress, _In_ size_t cSize)
{
    EnsureHeap();
    const bool profilerEnabled = XamlProfilerTracing::IsAllocationProfilingEnabled();

    const bool trackMemory = g_memoryTrackingEnabled;
    const bool needBlockSize = trackMemory || profilerEnabled;

    size_t cOldSize = 0;
    if (needBlockSize && pAddress)
    {
        cOldSize = HeapSize(ghHeap, 0, pAddress);
        ASSERT(cOldSize != (SIZE_T)-1);
    }

    void* newAddress = HeapReAlloc(ghHeap, 0, pAddress, cSize);

    if (!newAddress
        && cSize < 0x0004000)
    {
        // Terminate the process on OOM in a predictable way that gives us
        // clear Watson data.  In the future we may want to do this even on larger
        // allocations, but for now we only do it for <16K allocations to reduce
        // risk.
        XAMLTerminateProcessOnMemoryExhaustion(cSize);
    }

    if (trackMemory && newAddress)
    {
        // Compute delta in signed 64-bit space to avoid size_t underflow on
        // shrinking reallocations.
        UpdateAllocatedMemory(static_cast<INT64>(cSize) - static_cast<INT64>(cOldSize));
    }

    if (profilerEnabled && newAddress)
    {
        const size_t cNewSize = XcpAllocation::OSMemoryGetBlockSize(newAddress);
        if (cNewSize >= cOldSize)
        {
            g_outstandingAllocSize.fetch_add(cNewSize - cOldSize, std::memory_order_relaxed);
        }
        else
        {
            g_outstandingAllocSize.fetch_sub(cOldSize - cNewSize, std::memory_order_relaxed);
        }
    }

    return newAddress;
}

void XcpAllocation::OSMemoryFree(_Frees_ptr_opt_ void *pAddress)
{
    EnsureHeap();
    const bool profilerEnabled = XamlProfilerTracing::IsAllocationProfilingEnabled();

    const bool trackMemory = g_memoryTrackingEnabled;
    const bool needBlockSize = trackMemory || profilerEnabled;

    size_t cSize = 0;
    if (needBlockSize && pAddress)
    {
        cSize = HeapSize(ghHeap, 0, pAddress);
        ASSERT(cSize != (SIZE_T)-1);

        if (g_memoryTrackingEnabled)
        {
            UpdateAllocatedMemory(-(INT64)cSize);
        }
    }

    HeapFree(ghHeap, 0, pAddress);

    if (profilerEnabled)
    {
        g_deallocCount.fetch_add(1, std::memory_order_relaxed);
        if (pAddress != nullptr)
        {
            g_outstandingAllocCount.fetch_sub(1, std::memory_order_relaxed);
            g_outstandingAllocSize.fetch_sub(cSize, std::memory_order_relaxed);
        }
    }
}
