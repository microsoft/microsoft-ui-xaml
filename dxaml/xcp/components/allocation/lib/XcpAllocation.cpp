// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XcpAllocation.h"
#include "XAMLTerminateProcessOnOOM.h"
#include <cstdlib>
#include <atomic>

#include "XamlTelemetry.h"

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

#if DBG
#define COUNT_ALLOC 1
#endif

#if TRACE_ALLOC
#define COUNT_ALLOC 1
#endif

#if COUNT_ALLOC
std::atomic<size_t> g_allocCount = 0;
std::atomic<size_t> g_allocSize = 0;
std::atomic<size_t> g_deallocCount = 0;
std::atomic<size_t> g_outstandingAllocCount = 0;
std::atomic<size_t> g_outstandingAllocSize = 0;
#endif

namespace
{
    void RecordOutstandingAllocation(_In_ const void *pAddress)
    {
#if COUNT_ALLOC
        const size_t cSize = XcpAllocation::OSMemoryGetBlockSize(pAddress);
        if (cSize != 0)
        {
            g_outstandingAllocCount.fetch_add(1, std::memory_order_relaxed);
            g_outstandingAllocSize.fetch_add(cSize, std::memory_order_relaxed);
        }
#endif
    }
}

size_t XcpAllocation::GetAllocationCount()
{
#if COUNT_ALLOC
    return g_allocCount.load(std::memory_order_relaxed);
#else
    return 0;
#endif
}

size_t XcpAllocation::GetAllocationSize()
{
#if COUNT_ALLOC
    return g_allocSize.load(std::memory_order_relaxed);
#else
    return 0;
#endif
}

size_t XcpAllocation::GetDeallocationCount()
{
#if COUNT_ALLOC
    return g_deallocCount.load(std::memory_order_relaxed);
#else
    return 0;
#endif
}

size_t XcpAllocation::GetOutstandingAllocationCount()
{
#if COUNT_ALLOC
    return g_outstandingAllocCount.load(std::memory_order_relaxed);
#else
    return 0;
#endif
}

size_t XcpAllocation::GetOutstandingAllocationSize()
{
#if COUNT_ALLOC
    return g_outstandingAllocSize.load(std::memory_order_relaxed);
#else
    return 0;
#endif
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

    pAddress = HeapAlloc(ghHeap, 0, cSize);
    if (pAddress)
    {
        UpdateAllocatedMemory(cSize);
        RecordOutstandingAllocation(pAddress);
    }
    else
    {
        // Terminate the process on OOM in a predictable way that gives us
        // clear Watson data.
        XAMLTerminateProcessOnMemoryExhaustion(cSize);
    }

#if COUNT_ALLOC
    g_allocCount.fetch_add(1, std::memory_order_relaxed);
    g_allocSize.fetch_add(cSize, std::memory_order_relaxed);
#endif

#if TRACE_ALLOC
    TraceLoggingProviderWrite(
        XamlTelemetry, "HeapAlloc_OSMemoryAllocateFailFast",
        TraceLoggingUInt64(cSize, "cSize"),
        TraceLoggingUInt64(GetAllocationCount(), "AllocCount"),
        TraceLoggingUInt64(GetAllocationSize(), "AllocSize"),
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));
#endif

    return pAddress;
}

_Check_return_ void *XcpAllocation::OSMemoryAllocateZeroMemoryFailFast(_In_ size_t cSize)
{
    void *pAddress = NULL;

    EnsureHeap();

    pAddress = HeapAlloc(ghHeap, HEAP_ZERO_MEMORY, cSize);
    if (pAddress)
    {
        UpdateAllocatedMemory(cSize);
        RecordOutstandingAllocation(pAddress);
    }
    else
    {
        // Terminate the process on OOM in a predictable way that gives us
        // clear Watson data.
        XAMLTerminateProcessOnMemoryExhaustion(cSize);
    }

#if COUNT_ALLOC
    g_allocCount.fetch_add(1, std::memory_order_relaxed);
    g_allocSize.fetch_add(cSize, std::memory_order_relaxed);
#endif

#if TRACE_ALLOC
    TraceLoggingProviderWrite(
        XamlTelemetry, "HeapAlloc_OSMemoryAllocateZeroMemoryFailFast",
        TraceLoggingUInt64(cSize, "cSize"),
        TraceLoggingUInt64(GetAllocationCount(), "AllocCount"),
        TraceLoggingUInt64(GetAllocationSize(), "AllocSize"),
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));
#endif

    return pAddress;
}

_Check_return_ void *XcpAllocation::OSMemoryAllocateNoFailFast(_In_ size_t cSize)
{
    void* pAddress = NULL;

    EnsureHeap();

#if COUNT_ALLOC
    g_allocCount.fetch_add(1, std::memory_order_relaxed);
    g_allocSize.fetch_add(cSize, std::memory_order_relaxed);
#endif

#if TRACE_ALLOC
    TraceLoggingProviderWrite(
        XamlTelemetry, "HeapAlloc_OSMemoryAllocateNoFailFast",
        TraceLoggingUInt64(cSize, "cSize"),
        TraceLoggingUInt64(GetAllocationCount(), "AllocCount"),
        TraceLoggingUInt64(GetAllocationSize(), "AllocSize"),
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));
#endif

    pAddress = HeapAlloc(ghHeap, 0, cSize);
    if (pAddress)
    {
        UpdateAllocatedMemory(cSize);
        RecordOutstandingAllocation(pAddress);
    }

    return pAddress;
}

_Check_return_ void *XcpAllocation::OSMemoryResize(_Frees_ptr_opt_ void *pAddress, _In_ size_t cSize)
{
    EnsureHeap();

#if TRACE_ALLOC
    TraceLoggingProviderWrite(
        XamlTelemetry, "HeapReAlloc_OSMemoryResize",
        TraceLoggingUInt64(cSize, "cSize"),
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));
#endif

    const bool trackMemory = g_memoryTrackingEnabled;
    bool needBlockSize = trackMemory;
#if COUNT_ALLOC
    needBlockSize = true;
#endif

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

#if COUNT_ALLOC
    if (newAddress)
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
#endif

    return newAddress;
}

void XcpAllocation::OSMemoryFree(_Frees_ptr_opt_ void *pAddress)
{
    EnsureHeap();

    const bool trackMemory = g_memoryTrackingEnabled;
    bool needBlockSize = trackMemory;
#if COUNT_ALLOC
    needBlockSize = true;
#endif

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

#if COUNT_ALLOC
    g_deallocCount.fetch_add(1, std::memory_order_relaxed);
    if (cSize != 0)
    {
        g_outstandingAllocCount.fetch_sub(1, std::memory_order_relaxed);
        g_outstandingAllocSize.fetch_sub(cSize, std::memory_order_relaxed);
    }
#endif

#if TRACE_ALLOC
    TraceLoggingProviderWrite(
        XamlTelemetry, "HeapFree_OSMemoryFree",
        TraceLoggingUInt64(GetDeallocationCount(), "DeallocCount"),
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));
#endif

}
