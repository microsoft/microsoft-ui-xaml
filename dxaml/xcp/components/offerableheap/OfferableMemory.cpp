// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <memoryapi.h>
#include <minerror.h>
#include "OfferableMemory.h"

// round allocation size to multiple of page size for VirtualAlloc
#define ROUND_TO_PAGESIZE(size, pagesize)    \
  ((((ULONG_PTR)(size) + (pagesize) - 1) & (~((ULONG_PTR)(pagesize) - 1))))

OfferableMemory::OfferableMemory ( uint32_t size ) : m_size(size)
{
    SYSTEM_INFO sysinfo;
    GetSystemInfo (&sysinfo);
    uint32_t page_size = sysinfo.dwPageSize;
    m_roundedsize = ROUND_TO_PAGESIZE(m_size, page_size); // OfferVirtualMemory requires size rounded to multiple of page size boundary
    m_buffer = VirtualAlloc(NULL, m_roundedsize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    // If the offerable heap allocation failed, more detailed information can be obtained by checking the
    // last NTSTATUS for the active thread (in WinDbg, it's available via !teb or !gle).
    IFCOOMFAILFAST(m_buffer);
}

OfferableMemory::~OfferableMemory()
{
    // Fails fast if any params are wrong, buffer is not from this heap
    BOOL result = VirtualFree(m_buffer, 0, MEM_RELEASE);
    if(result == FALSE)
    {
        IFCFAILFAST(E_FAIL);
    }
}

void OfferableMemory::Offer()
{
    if(!m_offered)
    {
        // Fails fast if any params are wrong, memory not from this heap or memory already offered and not reclaimed
        DWORD result = OfferVirtualMemory(m_buffer, m_roundedsize, VmOfferPriorityNormal);
        if (result == ERROR_SUCCESS)
        {
            m_offered = true;
        }
        else
        {
            IFCFAILFAST(HRESULT_FROM_WIN32(result));
        }
    }
    
}

// Returns true if the resources were discarded, false otherwise.
// NOTE:  The semantic of OfferableMemory::Reclaim is same as public
// IDXGIDevice2::ReclaimResources. 
bool OfferableMemory::Reclaim()
{
    bool isDiscarded = false;

    if(m_offered)
    {
        // Fails if any params are wrong or memory has not been offered.
        DWORD result = ReclaimVirtualMemory(m_buffer, m_roundedsize);
        if (result == ERROR_SUCCESS)
        {
            m_offered = false;
        }
        else if (result == ERROR_BUSY)
        {
            m_offered = false;
            isDiscarded = true;
        }
        else
        {
            IFCFAILFAST(HRESULT_FROM_WIN32(result));
        }
    }

    return isDiscarded;
}
