// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XcpAllocation.h"
#include "XAMLTerminateProcessOnOOM.h"
#include <cstdlib>
#if DBG
#include <atomic>
#endif

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
std::atomic<size_t> g_allocCount = 0;
std::atomic<size_t> g_allocSize = 0;
std::atomic<size_t> g_deallocCount = 0;
#endif

size_t XcpAllocation::GetAllocationCount()
{
#if DBG
    return g_allocCount.load(std::memory_order_relaxed);
#else
    return 0;
#endif
}

size_t XcpAllocation::GetAllocationSize()
{
#if DBG
    return g_allocSize.load(std::memory_order_relaxed);
#else
    return 0;
#endif
}

size_t XcpAllocation::GetDeallocationCount()
{
#if DBG
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

#if DBG
    g_allocCount.fetch_add(1, std::memory_order_relaxed);
    g_allocSize.fetch_add(cSize, std::memory_order_relaxed);
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

#if DBG
    g_allocCount.fetch_add(1, std::memory_order_relaxed);
    g_allocSize.fetch_add(cSize, std::memory_order_relaxed);
#endif

    return pAddress;
}

_Check_return_ void *XcpAllocation::OSMemoryAllocateNoFailFast(_In_ size_t cSize)
{
    EnsureHeap();

#if DBG
    g_allocCount.fetch_add(1, std::memory_order_relaxed);
    g_allocSize.fetch_add(cSize, std::memory_order_relaxed);
#endif

    return HeapAlloc(ghHeap, 0, cSize);
}

_Check_return_ void *XcpAllocation::OSMemoryResize(_Frees_ptr_opt_ void *pAddress, _In_ size_t cSize)
{
    EnsureHeap();

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
#if DBG
    g_deallocCount.fetch_add(1, std::memory_order_relaxed);
#endif
}