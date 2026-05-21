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

void EnsureHeap()
{
    if(ghHeap == nullptr)
    {
        ghHeap = GetProcessHeap();
    }
}

#if DBG
#define COUNT_ALLOC 1
#endif

// Uncomment to trace allocations via ETW
//#define TRACE_ALLOC 1

#if TRACE_ALLOC
#define COUNT_ALLOC 1
#endif

#if COUNT_ALLOC
std::atomic<size_t> g_allocCount = 0;
std::atomic<size_t> g_allocSize = 0;
std::atomic<size_t> g_deallocCount = 0;
#endif

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

_Check_return_ void *XcpAllocation::OSMemoryAllocateFailFast(_In_ size_t cSize)
{
     void *pAddress = NULL;

    EnsureHeap();

    pAddress = HeapAlloc(ghHeap, 0, cSize);

    if (!pAddress)
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

    if (!pAddress)
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

    return HeapAlloc(ghHeap, 0, cSize);
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

    return newAddress;
}

void XcpAllocation::OSMemoryFree(_Frees_ptr_opt_ void *pAddress)
{
    EnsureHeap();
    HeapFree(ghHeap, 0, pAddress);

#if COUNT_ALLOC
    g_deallocCount.fetch_add(1, std::memory_order_relaxed);
#endif

#if TRACE_ALLOC
    TraceLoggingProviderWrite(
        XamlTelemetry, "HeapFree_OSMemoryFree",
        TraceLoggingUInt64(GetDeallocationCount(), "DeallocCount"),
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));
#endif

}