// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#if XCP_MONITOR

#include "XcpAllocation.h"
#include "XcpAllocationDebug.h"

// Used to guarantee exclusive access to the pPrevious and pNext pointers
// in memory check blocks. Allows the blocks to be inserted into and
// removed from a doubly-linked list from multiple processes.
wil::critical_section* g_pCheckedMemoryChainLock = nullptr;

// Memory header check blocks are kept in a doubly linked circular
// list. This node is always present, an empty list being represented
// by this node alone.
//
// In this specific instance, pTrailer = NULL and cSize = 0.
DebugMemoryCheckBlock g_RootCheckBlock =
{
    (XUINT64)"XcpMRtB",         // chXcpB
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, // guard
    0,                          // cSize
    BLOCK_SIGNATURE,            // blockSignature
    &g_RootCheckBlock,          // pPrevious
    &g_RootCheckBlock,          // pNext
    NULL,                       // pTrailer
    0,                          // cCallers
    NULL,                       // pCallers
    0,                          // allocation class
    0,                          // fDeallocated
    NULL,                       // pAddRefHead
    NULL,                       // pAddRefTail
    0,                          // section
    0,                          // index
    NULL,                       // taggedPointers
    NULL,                       // inboundLinkList
    LinkStrengthNone,           // maxOutboundLinkStrength
    TopLevelType_NotTopLevel,   // topLevelType
    false,                      // isOnTraversalStack
    false,                      // isConsideredBefore
    false,                      // isHardToTrack
    (XUINT64)"XcpMRtE"          // chXcpE
};

void InitCheckedMemoryChainLock()
{
    ASSERT(g_pCheckedMemoryChainLock == nullptr);
    g_pCheckedMemoryChainLock = new wil::critical_section();
    XcpDebugSetLeakDetectionFlag(g_pCheckedMemoryChainLock, true);
}

void DeleteCheckedMemoryChainLock()
{
    if (g_pCheckedMemoryChainLock != nullptr)
    {
        // This lock is itself a heap allocation.  If we just call delete on it then the allocation tracker will
        // call into EnterCheckedMemoryChainLock and try to enter the lock that we just deleted.  Move it into
        // a local variable and null the global pointer before that happens so that EnterCheckedMemoryChainLock
        // is a NOOP.
        wil::critical_section* localLock = g_pCheckedMemoryChainLock;
        g_pCheckedMemoryChainLock = nullptr;
        delete localLock;
    }
}

wil::cs_leave_scope_exit EnterCheckedMemoryChainLock()
{
    if (g_pCheckedMemoryChainLock)
    {
        return g_pCheckedMemoryChainLock->lock();
    }
    return nullptr;
}

LONG  g_AllocationCount  = 0;
void  *g_pLastFreedMemory = nullptr;
// Marks objects as free to leak.
INT16  g_bMarkLeakable = 0;

wil::details::lambda_call<std::function<void(void)>> XcpDebugStartIgnoringLeaks()
{
    g_bMarkLeakable = static_cast<INT16>(AllocationIgnoreLeak);
    return wil::details::lambda_call<std::function<void(void)>>([] {
        g_bMarkLeakable = 0;
    });
}

// Suspend counting clocks
IThreadMonitor* XcpDebugGetThreadMonitor();

bool XcpDebugCaptureStack(_Out_ XUINT32 *pcCallers, _Outptr_result_buffer_(cMaxCallers) XUINT64  **ppCallers);

void XcpDebugTrace(XUINT32 uClass, DebugAllocationClass allocationClass, void *pNewMemory, size_t cSize);

bool XcpDebugTrace(XUINT32 uClass, const XCHAR *pFilename, XINT32 iLine, XINT32 iValue, const XCHAR *pTestString, const XCHAR *pMessageString, ...);

bool XcpDebugIsStressModeEnabled();

// Creates check blocks at the start and end of the passed memory range,
// and returns the address of the content within the range.
__declspec(allocator) void* AllocateCheckedMemory(
    size_t               cSize,        // Size excluding check blocks
    DebugAllocationClass allocationClass
)
{
    XUINT32 fUseTrailer = TRUE;
    XUINT32 checkedSize = 0;
    DebugMemoryCheckBlock* pHeader = NULL;
    DebugMemoryCheckBlock* pTrailer = NULL;
    XUINT8* pContent = NULL;

    XUINT32 sizeToAllocate = static_cast<XUINT32>(cSize);

    if (sizeToAllocate < cSize)
    {
        // If casting from size_t to XUNIT32 caused us to truncate cSize is too big to allocate.
        return NULL;
    }

    checkedSize = sizeToAllocate + (fUseTrailer ? 2 : 1) * sizeof(DebugMemoryCheckBlock);

    checkedSize = (checkedSize + 15) & 0xfffffff0; // Round up checked size to next multiple of 16 bytes

    if (checkedSize < cSize)
    {
        return NULL;  // Too big to allocate without address arithmetic overflow
    }


    // Allocate the checked memory

    pHeader = (DebugMemoryCheckBlock*)XcpAllocation::OSMemoryAllocateFailFast(checkedSize);

    if (!pHeader)
    {
        return NULL;  // Out of memory
    }

    IThreadMonitor* pThreadMonitor = XcpDebugGetThreadMonitor(); // Suspend counting clocks

    // Determine position of the trailer

    pContent = (XUINT8*)(pHeader + 1);
    if (fUseTrailer)
    {
        pTrailer = (DebugMemoryCheckBlock*)(
            (XUINT8*)(pHeader)
            +checkedSize
            - sizeof(DebugMemoryCheckBlock)
            );
    }

    // Fill the content with JUNK to catch uninitialized memory issues
    memset(pContent, 0xCA, sizeToAllocate);


    // Fill in the header and trailer specific fields

    pHeader->chXcpB = XcpHdrB;
    pHeader->chXcpE = XcpHdrE;


    // Fill in the remaining header fields

    pHeader->cSize = sizeToAllocate;
    pHeader->blockSignature = BLOCK_SIGNATURE;
    pHeader->pTrailer = pTrailer;

    if (pThreadMonitor != NULL)
        pHeader->section = pThreadMonitor->m_sectionStack[pThreadMonitor->m_sectionStackTop];
    else
        pHeader->section = 0;

    pHeader->allocationClass = XUINT16(allocationClass | g_bMarkLeakable);
    pHeader->fDeallocated = FALSE;
    pHeader->index = XUINT32(::InterlockedIncrement(&g_AllocationCount));
    pHeader->pAddRefHead = NULL;
    pHeader->pAddRefTail = NULL;

    pHeader->taggedPointers = NULL;
    pHeader->inboundLinkList = NULL;
    pHeader->maxOutboundLinkStrength = LinkStrengthNone;
    pHeader->topLevelType = TopLevelType_NotTopLevel;
    pHeader->isOnTraversalStack = false;
    pHeader->isConsideredBefore = false;
    pHeader->isHardToTrack = false;

    // If xcpmon is connected, record the allocation call stack
    if (!XcpDebugCaptureStack(&pHeader->cCallers, &pHeader->pCallers))
    {
        pHeader->pCallers = NULL;
        pHeader->cCallers = 0;
    }

    // Fill in guard memory

    for (int i = 0; i < ARRAY_SIZE(pHeader->guard); i++)
    {
        pHeader->guard[i] = (XUINT16)(XUINT64((XUINT16*)&pHeader->guard[i]) & 0xffff);
    }

    // Insert this node into the memory chain
    {
        auto lock = EnterCheckedMemoryChainLock();

        pHeader->pNext = g_RootCheckBlock.pNext;
        g_RootCheckBlock.pNext = pHeader;
        pHeader->pPrevious = pHeader->pNext->pPrevious;
        pHeader->pNext->pPrevious = pHeader;

        if (fUseTrailer)
        {
            *pTrailer = *pHeader;  // Duplicate the header block into the trailer block
        }
    }

    if (fUseTrailer)
    {
        // Fix up those parts of the trailer block that differ from the header

        pTrailer->chXcpB = XcpTlrB;
        pTrailer->chXcpE = XcpTlrE;
        for (int i=0; i<ARRAY_SIZE(pTrailer->guard); i++)
        {
            pTrailer->guard[i] = (XUINT16)(XUINT64((XUINT16*)&pTrailer->guard[i]) & 0xffff);
        }
    }

    // Fill any space between the end of the client memory and the trailer block.

    XUINT8 *pFill = pContent + sizeToAllocate;
    while (pFill < (XUINT8*)pTrailer)
    {
        *pFill = (XUINT8)((XUINT64)pFill & 0xff);
        pFill++;
    }

    // Success. Return the address of the content.
   if (pThreadMonitor != NULL)
        pThreadMonitor->Resume();

    return pContent;
}

// Memory allocator for new / malloc, that in debug builds adds check blocks
// before and after the allocated memory, and sends a message to the monitoring
// process recording the allocation and where in the source it came from.
_Check_return_ __declspec(allocator) void *XcpDebugAllocate(
    size_t               cSize,
    DebugAllocationClass allocationClass
)
{
    void *pNewMemory = AllocateCheckedMemory(cSize, allocationClass);

    XcpDebugTrace(MonitorAllocation, allocationClass, pNewMemory, cSize);

    return pNewMemory;
}

void XcpDebugFreeStackTrace(void* pCallers)
{
    HeapFree( GetProcessHeap(), 0, pCallers );
}

_Check_return_ XUINT32 TestMemoryCheckBlocks(
    _In_  DebugMemoryCheckBlock *pHeader,
    _In_  DebugAllocationClass   allocationClass
);

//  Checks that the check blocks either side of the passed memory address
//  are uncorrupted, and returns both the address of the leading check block
//  and the length of the content.
//
//  If corruption is discovered, there will be an assertion failure, causing
//  a DebugBreak. If the user presses go, E_FAIL is returned.
_Check_return_ XUINT32 TestMemoryCheckBlocks(
    _In_  DebugMemoryCheckBlock *pHeader,
    _In_  DebugAllocationClass   allocationClass
)
{
    XUINT32 fUseTrailer             = TRUE;

    WCHAR                 *pFaultMsg = NULL;

    DebugMemoryCheckBlock *pTrailer    = NULL;
    XUINT64                checkedSize = 0;
    XUINT32                fGood       = TRUE;
    IThreadMonitor *pThreadMonitor = XcpDebugGetThreadMonitor(); // Suspend counting clocks

    // Validate content of header.

    if      (pHeader->fDeallocated)       pFaultMsg = const_cast<WCHAR*>(L"Memory check block header: memory already deallocated"); //Warning! Casting constness away!
    else if (pHeader->chXcpB != XcpHdrB)  pFaultMsg = const_cast<WCHAR*>(L"Memory check block header: first 8 chars corrupt"); //Warning! Casting constness away!
    else if (pHeader->chXcpE != XcpHdrE)  pFaultMsg = const_cast<WCHAR*>(L"Memory check block header: last 8 chars corrupt"); //Warning! Casting constness away!
    else if ((pHeader->allocationClass & AllocationClassMask)
                        > AllocationClassMax) pFaultMsg = const_cast<WCHAR*>(L"Memory check block header: allocation class corrupt"); //Warning! Casting constness away!
    else if (pHeader->fDeallocated > 1)   pFaultMsg = const_cast<WCHAR*>(L"Memory check block header: fDeallocated corrupt"); //Warning! Casting constness away!

    if (pFaultMsg == NULL)
    {
        // Validate guard memory

        for (int i=0; i<ARRAY_SIZE(pHeader->guard); i++)
        {
            if (pHeader->guard[i] != (XUINT16)(XUINT64((XUINT16*)&pHeader->guard[i]) & 0xffff))
            {
                pFaultMsg = const_cast<WCHAR*>(L"Memory check block header: guard block corrupted"); //Warning! Casting constness away!
                break;
            }
        }
    }

    if (pFaultMsg == NULL)
    {
        // Validate cSize vs pTrailer

        checkedSize  = pHeader->cSize + (fUseTrailer?2:1) * sizeof(DebugMemoryCheckBlock);
        checkedSize = (checkedSize + 15) & 0xfffffff0; // Round up checked size to next multiple of 16 bytes

        if (fUseTrailer)
        {
            pTrailer = (DebugMemoryCheckBlock*)(
                (XUINT8*)(pHeader)
                + checkedSize
                - sizeof(DebugMemoryCheckBlock)
            );
        }

        if (pHeader->pTrailer != pTrailer) pFaultMsg = const_cast<WCHAR*>(L"Memory check block header: pTrailer corrupted"); //Warning! Casting constness away!
    }

    if (pFaultMsg == NULL)
    {
        // Validate filler between content and trailer

        XUINT8 *pFill = (XUINT8*)(pHeader+1) + pHeader->cSize;
        while (pFill < (XUINT8*)pTrailer)
        {
            if (*pFill != (XUINT8)((XUINT64)pFill & 0xff))
            {
                pFaultMsg = const_cast<WCHAR*>(L"Memory check block header: filler between content and trailer corrupted"); //Warning! Casting constness away!
                break;
            }
            pFill++;
        }
    }


    if (pFaultMsg == NULL)
    {
        // Simple check that doubly linked list is valid locally

        auto lock = EnterCheckedMemoryChainLock();

        if (pHeader->pPrevious->pNext != pHeader) pFaultMsg = const_cast<WCHAR*>(L"Memory check block header: Previous pointer corrupt"); //Warning! Casting constness away!
        else if(pHeader->pNext->pPrevious != pHeader) pFaultMsg = const_cast<WCHAR*>(L"Memory check block header: Next pointer corrupt"); //Warning! Casting constness away!
    }

    if (pFaultMsg)
    {
        if(XcpDebugTrace(
            MonitorMemoryHeaderCorruption,
            __WFILE__,
            __LINE__,
            (XINT32)(XUINT64) pHeader,
            pFaultMsg,
            NULL))  // Signals no optional arguments
        {
            // Break-in enabled, so assert false to break in.
            if(XcpDebugIsStressModeEnabled())
            {
                DebugBreak();
            }
            else
            {
                ASSERT(false);
            }
        }
        fGood = FALSE;
        goto Cleanup;
    }

    // Test memory trailer
    if (fUseTrailer)
    {
        if      (pTrailer->chXcpB          != XcpTlrB                 ) pFaultMsg = const_cast<WCHAR*>(L"Memory check block trailer: first 8 chars corrupt"); //Warning! Casting constness away!
        else if (pTrailer->chXcpE          != XcpTlrE                 ) pFaultMsg = const_cast<WCHAR*>(L"Memory check block trailer: last 8 chars corrupt"); //Warning! Casting constness away!
        else if (pTrailer->pTrailer        != pHeader->pTrailer       ) pFaultMsg = const_cast<WCHAR*>(L"Memory check block trailer: pTrailer corrupt"); //Warning! Casting constness away!
        else if (pTrailer->section         != pHeader->section        ) pFaultMsg = const_cast<WCHAR*>(L"Memory check block trailer: section corrupt"); //Warning! Casting constness away!
        else if (pTrailer->allocationClass != pHeader->allocationClass) pFaultMsg = const_cast<WCHAR*>(L"Memory check block trailer: allocation class corrupt"); //Warning! Casting constness away!
        else if (pTrailer->fDeallocated    != pHeader->fDeallocated   ) pFaultMsg = const_cast<WCHAR*>(L"Memory check block trailer: fDeallocated corrupt"); //Warning! Casting constness away!

        if (pFaultMsg == NULL)
        {
            // Validate guard memory

            for (int i=0; i<ARRAY_SIZE(pTrailer->guard); i++)
            {
                if (pTrailer->guard[i] != (XUINT16)(XUINT64((XUINT16*)&pTrailer->guard[i]) & 0xffff))
                {
                    pFaultMsg = const_cast<WCHAR*>(L"Memory check block trailer: guard block corrupted"); //Warning! Casting constness away!
                    break;
                }
            }
        }
    }

    if (pFaultMsg)
    {
        if (XcpDebugTrace(
            MonitorMemoryTrailerCorruption,
            __WFILE__,
            __LINE__,
            (XINT32)(XUINT64) pHeader,
            pFaultMsg,
            NULL))    // Signals no optional arguments
        {
            // Break-in enabled, so assert false to break in.
            if(XcpDebugIsStressModeEnabled())
            {
                DebugBreak();
            }
            else
            {
                ASSERT(false);
            }
        }
        fGood = FALSE;
        goto Cleanup;
    }


    // No corruption detected, now just check that deallocation matches allocation
    if ((pHeader->allocationClass & AllocationClassMask) != (allocationClass & AllocationClassMask))
    {
        if(XcpDebugIsStressModeEnabled())
        {
            DebugBreak();
        }
        else
        {
            /*
            BUG: Temporarily disabling assert due to compiler bug 6590841.
            ASSERT(
                false,
                L"Allocated as %s but deallocated as %s",
                AllocationClassName(DebugAllocationClass(pHeader->allocationClass)),
                AllocationClassName(DebugAllocationClass(allocationClass))
            );
            */
        }
    }

Cleanup:
    if (pThreadMonitor != NULL)
    {
        pThreadMonitor->Resume();
    }

    return fGood;
}

void XcpDebugTraceLeakFullRefs(DebugMemoryCheckBlock *pHeader);

// Memory deallocator for delete/free, that in debug builds tests check blocks
// before and after the allocated memory, and sends a message to the monitoring
// process recording the allocation and where in the source it came from.
void XcpDebugFree(
    _In_ void            *pAddress,
    DebugAllocationClass  allocationClass
)
{
    XUINT32 fUseTrailer = TRUE;

    if (pAddress)
    {
        DebugMemoryCheckBlock *pHeader = ((DebugMemoryCheckBlock*)pAddress)-1;

        if (TestMemoryCheckBlocks(pHeader, allocationClass))
        {
            XcpDebugTrace(MonitorDeallocation, allocationClass, pAddress, static_cast<size_t>(pHeader->cSize));

            {
                // Suspend counting clocks while in debug memory management code
                IThreadMonitor *pThreadMonitor = XcpDebugGetThreadMonitor(); // Suspend counting clocks

                // Mark as deallocated

                pHeader->fDeallocated = TRUE;
                if (fUseTrailer)
                {
                    pHeader->pTrailer->fDeallocated = TRUE;
                }

                // Unlink from memory chain
                {
                    auto lock = EnterCheckedMemoryChainLock();

                    pHeader->pPrevious->pNext = pHeader->pNext;
                    pHeader->pNext->pPrevious = pHeader->pPrevious;
                }

                // Free record of allocation callstack if any

                if (pHeader->pCallers)
                {
                    XcpDebugFreeStackTrace(pHeader->pCallers);
                }

                DebugAddRefList *pNode = pHeader->pAddRefHead;
                DebugAddRefList *pNext = NULL;

                if (pNode)
                {
                    XcpDebugTraceLeakFullRefs(pHeader);
                }

                while (pNode != NULL)
                {
                    pNext = pNode->pNext;
                    if (pNode->pCallers)
                    {
                        XcpDebugFreeStackTrace(pNode->pCallers);
                    }
                    XcpAllocation::OSMemoryFree(pNode);
                    pNode = pNext;
                }

                PointerStrengthTag* ptrStrengthIter = pHeader->taggedPointers;
                while (ptrStrengthIter != NULL)
                {
                    PointerStrengthTag* next = ptrStrengthIter->next;
                    XcpAllocation::OSMemoryFree(ptrStrengthIter);
                    ptrStrengthIter = next;
                }

                // Fill the released memory with 0xccccccccc to significantly
                // increase the chance that anything that uses a now obsolete
                // reference to this memory will fail.

                memset(
                    pHeader,
                    0xcc,
                    (XUINT32)pHeader->cSize + (fUseTrailer?2:1) * sizeof(DebugMemoryCheckBlock)
                );

                // Rather than freeing the memory immediately, we hold on to it
                // until the next deallocation request comes in. This way we
                // guarantee that the OS memory allocator cannot immediately
                // reallocate it on the next allocation call, which helps us find
                // stale pointer usage in code.

                {
                    auto lock = EnterCheckedMemoryChainLock();

                    if (g_pLastFreedMemory)
                    {
                        // Free the previously released memory block. Note that the OS
                        // memory free code will usually reuse the first couple of
                        // bytes for its own memory management data.
                        XcpAllocation::OSMemoryFree(g_pLastFreedMemory);
                    }

                    g_pLastFreedMemory = pHeader;
                }

                if (pThreadMonitor != NULL)
                {
                    pThreadMonitor->Resume(); // Resume counting clocks
                }
            }
        }
    }
}

// Mark LeakDetection flag in individual object that was allocated
// through XcpDebugAllocate.
void XcpDebugSetLeakDetectionFlag(_In_ void *pAddress, _In_ bool fDisableLeakDetection)
{
    XUINT32 fUseTrailer = TRUE;

    if (pAddress)
    {
        DebugMemoryCheckBlock *pHeader = NULL;

        pHeader = ((DebugMemoryCheckBlock*)pAddress)-1;

        // Clear the AllocationIgnoreLeak bit.
        pHeader->allocationClass = XUINT16( pHeader->allocationClass & (~AllocationIgnoreLeak) );

        if (fDisableLeakDetection)
        {
            pHeader->allocationClass = XUINT16(pHeader->allocationClass | AllocationIgnoreLeak);
        }

        if (fUseTrailer)
        {
            XUINT64 checkedSize = 0;
            DebugMemoryCheckBlock *pTrailer = NULL;

            // As fUseTrailer is True multiple will be 2.
            checkedSize  = pHeader->cSize + 2 * sizeof(DebugMemoryCheckBlock);
            checkedSize = (checkedSize + 15) & 0xfffffff0; // Round up checked size to next multiple of 16 bytes

            pTrailer = (DebugMemoryCheckBlock*)(
                    (XUINT8*)(pHeader)
                    + checkedSize
                    - sizeof(DebugMemoryCheckBlock)
                    );

            pTrailer->allocationClass = pHeader->allocationClass;
        }
    }

    return;
}

// Memory re-allocator for new / malloc, that in debug builds adds check blocks
// before and after the allocated memory, and sends a message to the monitoring
// process recording the allocation and where in the source it came from.
_Check_return_ __declspec(allocator) void *XcpDebugResize(
    _In_ void            *pAddress,
    size_t                cSize,
    DebugAllocationClass  allocationClass
)
{
    void *pNewMemory = XcpDebugAllocate(cSize, allocationClass);

    if (pAddress)
    {
        DebugMemoryCheckBlock *pHeader = ((DebugMemoryCheckBlock*)pAddress)-1;

        if (pNewMemory != NULL)
        {
             memcpy(pNewMemory, pAddress, (XUINT32)pHeader->cSize);
             XcpDebugFree(pAddress, allocationClass);
        }
    }

    return pNewMemory;
}

#endif
