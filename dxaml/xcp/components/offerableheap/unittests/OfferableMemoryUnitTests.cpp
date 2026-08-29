// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "OfferableMemory.h"
#include "OfferableMemoryUnitTests.h"

extern void SetOfferableHeapFailurePercentage(uint32_t failPercent);

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace OfferableHeap {

void OfferableMemoryUnitTests::MultipleAllocationsWrite()
{
    uint32_t allocationSizes[] =
    {
        1,
        10,
        100,
        1000,
        10000,
        100000,
        1000000
    };

    for (auto &size : allocationSizes)
    {
        OfferableMemory memory(size);
        memset(memory.GetBuffer(), 1, memory.GetSize());
    }
}

void OfferableMemoryUnitTests::OfferReclaim()
{
    uint32_t allocationSizes[] =
    {
        1,
        10,
        100,
        1000,
        10000,
        100000,
        1000000
    };
    
    SYSTEM_INFO sysinfo;
    GetSystemInfo (&sysinfo);
    uint32_t page_size = sysinfo.dwPageSize;

    for (auto &size : allocationSizes)
    {
        OfferableMemory memory(size);
        uint32_t num_pages = memory.GetRoundedSize() / page_size;
        auto statusWrite = WriteOnEachPage(memory.GetBuffer(), num_pages, page_size, 0xf00d);
        if (statusWrite != STATUS_SUCCESS)
        {      
            WEX::Logging::Log::Error(L"Write on each page not successful");
            VERIFY_ARE_EQUAL(statusWrite, STATUS_SUCCESS);
            
        }
        memory.Offer();
        VERIFY_IS_FALSE(memory.Reclaim());
        VERIFY_IS_TRUE(CheckValueOnEachPage(memory.GetBuffer(), num_pages, page_size, 0xf00d));
        memset(memory.GetBuffer(), 1, memory.GetSize());
    }
}

// This function writes Value on each page of the given virtual memory region.
NTSTATUS OfferableMemoryUnitTests::WriteOnEachPage(_In_ LPVOID BaseAddress, _In_ ULONG NumberOfPages, _In_ ULONG PageSize, _In_ ULONG Value)
{
    volatile ULONG_PTR* Ptr;
    SIZE_T Offset = 0;
    NTSTATUS Status = STATUS_SUCCESS;

    for (ULONG i = 0; i < NumberOfPages; i++)
    {
        Ptr = (volatile ULONG_PTR*)((ULONG_PTR)BaseAddress + Offset);
        __try {
            *Ptr = Value;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            Status = GetExceptionCode();
            if (STATUS_ACCESS_VIOLATION != Status)
            {
                WEX::Logging::Log::Error(L"Exception which is not status access violation in writing value. Maybe it is a read/write issue");
            }
            else
            {
                WEX::Logging::Log::Error(L"Exception due to status access violation in writing value");
            }
            return Status;
        }
        Offset += PageSize;
    }

    return Status;
}



// This function compares first ULONG value of each page to ExpectedValue.
// returns true if value of each page is same as ExpectedValue, false otherwise
bool OfferableMemoryUnitTests::CheckValueOnEachPage(_In_ LPVOID BaseAddress, _In_ ULONG NumberOfPages, _In_ ULONG PageSize, _In_ ULONG ExpectedValue)
{
    SIZE_T Offset = 0;
    volatile ULONG_PTR* Ptr;
    NTSTATUS Status = STATUS_SUCCESS;

    for (ULONG i = 0; i < NumberOfPages; i++) {
        Ptr = (volatile ULONG_PTR*)((ULONG_PTR)BaseAddress + Offset);

        __try {
            // because of non stack unwinding constraint imposed by __try, LOG_OUTPUT cannot be called here to print values
            if ((*Ptr) != ExpectedValue)
            {
                WEX::Logging::Log::Error(L"Checked value mismatch : checked value is different from expected value");
                return false;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Status = GetExceptionCode();
            if (STATUS_ACCESS_VIOLATION != Status) {
                WEX::Logging::Log::Error(L"Exception which is not status access violation. Maybe it is a read/write issue");
            }
            else
            {
                WEX::Logging::Log::Error(L"Unable to write due to status access violation");
            }
            return false;
        }
        Offset += PageSize;
    }

    return true;
}

} } } } } }
