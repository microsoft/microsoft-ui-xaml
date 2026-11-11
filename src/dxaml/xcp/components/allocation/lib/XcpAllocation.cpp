// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XcpAllocation.h"
#include "XAMLTerminateProcessOnOOM.h"
#include "FrameworkUdk/Containment.h"

#define WINAPPSDK_CHANGEID_64006293 64006293

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

void XcpAllocation::EnableMemoryTracking()
{
    FAIL_FAST_ASSERT(WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_64006293>());

    g_memoryTrackingEnabled = true;
}

void XcpAllocation::UpdateAllocatedMemory(INT64 cSize)
{
    FAIL_FAST_ASSERT(WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_64006293>());

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
    if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_64006293>())
    {
        if (pAddress)
        {
            UpdateAllocatedMemory(cSize);
        }
        else
        {
            // Terminate the process on OOM in a predictable way that gives us
            // clear Watson data.
            XAMLTerminateProcessOnMemoryExhaustion(cSize);
        }
    }
    else
    {
        if (!pAddress)
        {
            // Terminate the process on OOM in a predictable way that gives us
            // clear Watson data.
            XAMLTerminateProcessOnMemoryExhaustion(cSize);
        }
    }

    return pAddress;
}

_Check_return_ void *XcpAllocation::OSMemoryAllocateZeroMemoryFailFast(_In_ size_t cSize)
{
    void *pAddress = NULL;

    EnsureHeap();

    pAddress = HeapAlloc(ghHeap, HEAP_ZERO_MEMORY, cSize);
    if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_64006293>())
    {
        if (pAddress)
        {
            UpdateAllocatedMemory(cSize);
        }
        else
        {
            // Terminate the process on OOM in a predictable way that gives us
            // clear Watson data.
            XAMLTerminateProcessOnMemoryExhaustion(cSize);
        }
    }
    else
    {
        if (!pAddress)
        {
            // Terminate the process on OOM in a predictable way that gives us
            // clear Watson data.
            XAMLTerminateProcessOnMemoryExhaustion(cSize);
        }
    }

    return pAddress;
}

_Check_return_ void *XcpAllocation::OSMemoryAllocateNoFailFast(_In_ size_t cSize)
{
    EnsureHeap();

    if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_64006293>())
    {
        void* pAddress = HeapAlloc(ghHeap, 0, cSize);
        if (pAddress)
        {
            UpdateAllocatedMemory(cSize);
        }

        return pAddress;
    }
    else
    {
        return HeapAlloc(ghHeap, 0, cSize);
    }
}

_Check_return_ void *XcpAllocation::OSMemoryResize(_Frees_ptr_opt_ void *pAddress, _In_ size_t cSize)
{
    EnsureHeap();

    if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_64006293>())
    {
        // Skip the HeapSize call entirely when tracking is disabled which saves a
        // kernel call per realloc.
        const bool trackMemory = g_memoryTrackingEnabled;

        size_t cOldSize = 0;
        if (trackMemory && pAddress)
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

        return newAddress;
    }
    else
    {
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
}

void XcpAllocation::OSMemoryFree(_Frees_ptr_opt_ void *pAddress)
{
    EnsureHeap();

    if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_64006293>())
    {
        // Skip the HeapSize call entirely when tracking is disabled to give
        // unmanaged apps near-zero tracking overhead.
        if (pAddress && g_memoryTrackingEnabled)
        {
            size_t cSize = HeapSize(ghHeap, 0, pAddress);
            ASSERT(cSize != (SIZE_T)-1);
            UpdateAllocatedMemory(-(INT64)cSize);
        }

        HeapFree(ghHeap, 0, pAddress);
    }
    else
    {
        HeapFree(ghHeap, 0, pAddress);
    }
}
