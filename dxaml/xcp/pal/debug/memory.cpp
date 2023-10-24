// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#if XCP_MONITOR

#include "XcpAllocation.h"
#include "XcpAllocationDebug.h"

#include <wil\resource.h>
#include <array>
#include <vector>

void XcpFindAndDumpTopLevelLeaksV2();

bool XcpDebugTrace(XUINT32 uClass, const XCHAR *pFilename, XINT32 iLine, XINT32 iValue, const XCHAR *pTestString, const XCHAR *pMessageString, ...)
{
    va_list vargs;
    va_start(vargs, pMessageString);
    return !!GetPALDebuggingServices()->XcpVTrace(uClass, pFilename, iLine, iValue, pTestString, pMessageString, vargs);
}

bool XcpDebugIsStressModeEnabled()
{
    return !!GetPALDebuggingServices()->IsStressModeEnabled();
}

// Attempts to find the specified address in the specified memory range.
bool AddressInObject(_In_ void *memory, _In_ XUINT64 size, _In_ void *address)
{
    while (size >= sizeof(void *))
    {
        if (address == ((void **) memory)[0])
            return true;

        memory = (void *)(XUINT64(memory) + sizeof(void *));
        size -= sizeof(void*);
    }

    return false;
}

void FindOwners(_In_ DebugMemoryCheckBlock *pBlock)
{
    XUINT8 *pMemoryBlock = (XUINT8*)(pBlock + 1);

    if ((pBlock->allocationClass & AllocationClassMask) == AllocationNewArray )
    {
        pMemoryBlock += sizeof(void*);
    }

    DebugMemoryCheckBlock *pHeader = g_RootCheckBlock.pNext;

    while (pHeader != &g_RootCheckBlock)
    {
    // Skip ourselves
        if (pHeader != pBlock)
        {
            if (AddressInObject(pHeader+1, pHeader->cSize, pMemoryBlock))
            {
                LOG(L"Owner of this block: 0x%08Ix", pHeader+1);
            }
        }
        pHeader = pHeader->pNext;
    }
}

// Returns a string for annotating messages that names an allocation class.
_Check_return_ const WCHAR *AllocationClassName(
    DebugAllocationClass allocationClass
)
{
    switch (allocationClass & AllocationClassMask)
    {
    case AllocationNewScalar:    return L"C++ scalar";
    case AllocationNewArray:     return L"C++ array";
    case AllocationFlat:         return L"XcpFlatAllocate";
    default:                     return L"Unrecognised allocation class";
    }
}

// Give a hex dump of the content of memory.
void Dump(void *pAddress, XUINT64 cSize, XUINT32 lineLimit)
{
    XUINT64 i;
    XUINT32 j;
    WCHAR   line[]  = L"  ----  .. .. .. ..  .. .. .. .. - .. .. .. ..  .. .. .. ..  xxxxxxxxxxxxxxxx ";
    XUINT8 *pBytes  = (XUINT8*)pAddress;
    WCHAR   hex[17] = L"0123456789abcdef";
    XINT32  fLineSame = FALSE;
    XUINT32 cSameLines = 0;
    XUINT32 cLines = 0;

    for (i=0; (i<cSize) && (cLines < lineLimit); i+=16)
    {
        fLineSame = FALSE;
        if ((i > 0) && (i+16 < cSize))
        {
            // Compare this line with the previous one
            fLineSame = TRUE;
            for (j=0; fLineSame && (j<16); j++)
            {
                fLineSame = (pBytes[i+j] == pBytes[i-16+j]);
            }
        }

        if (fLineSame)
        {
            cSameLines++;
        }
        else
        {
            if (cSameLines > 0)
            {
                LOG(L"        --------------- %4d repeated lines ---------------", cSameLines);
                cSameLines = 0;
            }

            line[2] = hex[(i>>12) & 0xf];
            line[3] = hex[(i>>8)  & 0xf];
            line[4] = hex[(i>>4)  & 0xf];
            line[5] = hex[i       & 0xf];

            for (j=0; j<16; j++)
            {
                if (i+j < cSize)
                {
                    line[3*j + j/4 + j/8 + 8] = hex[pBytes[i+j]>>4];
                    line[3*j + j/4 + j/8 + 9] = hex[pBytes[i+j]&15];
                    line[j+61] = (pBytes[i+j] >= 32 && pBytes[i+j] < 127) ? pBytes[i+j] : L'.';
                }
                else
                {
                    line[3*j + j/4 + j/8 + 8] = L' ';
                    line[3*j + j/4 + j/8 + 9] = L' ';
                    line[j+61] = L' ';
                }
            }

            if (i+9 > cSize)
            {
                line[33] = L' '; // Clear '-' separator
            }

            LOG(L"%s", line);

            cLines++;
        }
    }
}

// Does a pretty print of the stack
void PrintStackTrace(
    _In_                         XUINT32          cCallers,
    _In_reads_(cCallers)        XUINT64         *pCallers,
    _In_                         ICallingMethod  *pCallingMethods )
{
    XUINT32 i;

    // Print the allocation stack
    for (i = 0; i<cCallers; i++)
    {
        if (pCallingMethods[i].szFilename[0])
        {
            if ( GetPALDebuggingServices()->GetTraceFlags() & TraceLeakAddresses )
            {
                //
                // Print out the address and the source location: eg.
                //
                //     0x64a0f63f  apiwrapperbase.cpp[438] in CAPIWrapperBase::InvokeOnObject
                //
                RAWTRACE(TraceLeakAddresses, L"    0x%08I64x  %s[%d]%s%s",
                    pCallers[i],
                    pCallingMethods[i].szFilename,
                    pCallingMethods[i].iLine,
                    pCallingMethods[i].szSymbolName[0] ? L" in " : L"",
                    pCallingMethods[i].szSymbolName
                );
            }
            else
            {
                //
                // Print out the source location: eg.
                //
                //     apiwrapperbase.cpp[438] in CAPIWrapperBase::InvokeOnObject
                //
                LOG_INFO_EX(L"    %s[%d]%s%s",
                    pCallingMethods[i].szFilename,
                    pCallingMethods[i].iLine,
                    pCallingMethods[i].szSymbolName[0] ? L" in " : L"",
                    pCallingMethods[i].szSymbolName
                );
            }
        }
        else
        {
            //
            // Print out just address  location: eg.
            //
            //     0x64a0f63f
            //
            RAWTRACE(TraceLeakAddresses, L"    0x%08I64x", pCallers[i] );
        }
    }
}

// Does a pretty print of the allocation stack
void PrintAllocationStack(
    _In_                         XUINT32          cCallers,
    _In_reads_(cCallers)        XUINT64         *pCallers,
    _In_opt_z_             const WCHAR           *pHeaderMsg = NULL,
    _In_opt_                     void            *pAddress = NULL,
    _In_opt_                     ICallingMethod  *pAddressName = NULL,
    _In_opt_                     void            *pOwner = NULL,
    _In_opt_                     ICallingMethod  *pOwnerName = NULL )
{
    // Display allocation call stack, if there is one.
    if (pCallers)
    {
        XUINT32 i;
        ICallingMethod *pCallingMethods = NULL;
        XUINT32 bIgnoreObjectDump = FALSE;

        // Obtain source file locations for caller return addresses
        GetPALDebuggingServices()->GetCallerSourceLocations(
            cCallers,
            pCallers,
           &pCallingMethods
        );

        // See if we want to ignore any allocation made from the Core's create method -
        // these are usually leaked with the core
        if ( ((GetPALDebuggingServices()->GetTraceFlags() & TraceLeakCoreChildren) == 0) && pCallingMethods  )
        {
            WCHAR szCoreCreate[] = L"CCoreServices::Create";
            // Filter out anything that was created with the core
            for (i = 1; i<cCallers; i++) // start at 1 since the core create itself is valid
            {
                if (pCallingMethods[i].szFilename[0] && pCallingMethods[i].szSymbolName[0] )
                {
                    if ( 0 == xstrncmp( szCoreCreate, pCallingMethods[i].szSymbolName, ARRAY_SIZE(szCoreCreate) ) )
                    {
                        bIgnoreObjectDump = TRUE;
                        break;
                    }
                }
            }
        }

        if (!bIgnoreObjectDump)
        {
            // Print a header - usually either Owned or Top Level LEAK
            if ( pHeaderMsg )
            {
                LOG_INFO_EX(L"%s -- 0x%08I64x %s",
                    pHeaderMsg,
                    XUINT64(pAddress),
                    pAddressName ? pAddressName->szSymbolName : L"" );
            }

            if(pCallingMethods)
            {
                // Print the allocation stack
                PrintStackTrace(cCallers, pCallers, pCallingMethods);
            }
        }

        if(pCallingMethods)
        {
            GetPALDebuggingServices()->FreeCallingMethods(pCallingMethods);
        }
        pCallingMethods = NULL;
    }
}

void PrintAddRefReleaseStack(
    _In_ DebugMemoryCheckBlock *pHeader )
{
    ICallingMethod *pCallingMethods = NULL;

    // Display AddRef/Release call stack, if there is one.
    if (pHeader->pAddRefHead)
    {
        DebugAddRefList *pNode = pHeader->pAddRefHead;

        LOG(L"##################################################");
        LOG(L"## Tracking AddRefs and Releases for 0x%08Ix ##", pHeader+1 );
        LOG(L"##################################################");
        LOG(L"");

        while (pNode != NULL)
        {
            // Obtain source file locations for caller return addresses
            GetPALDebuggingServices()->GetCallerSourceLocations(
                pNode->cCallers,
                pNode->pCallers,
               &pCallingMethods
            );

            LOG(L"New reference count after this change: %i", pNode->cRef );

            PrintStackTrace(pNode->cCallers, pNode->pCallers, pCallingMethods);

            GetPALDebuggingServices()->FreeCallingMethods(pCallingMethods);
            pCallingMethods = NULL;

            LOG(L"");

            pNode = pNode->pNext;
        }
    }
}

HRESULT StringCchPrintf(  // StringCchPrintF is in monitor.cpp in this directory
    WCHAR   *pString,
    XUINT32  cString,
    const WCHAR   *pFormat,
    ...
);

bool TryReadErrorLogDirectoryFromRegistry(_Out_writes_(MAX_PATH + 1) wchar_t* logDirectory)
{
    const wchar_t* valueName = L"ErrorLogDirectory";
    const wchar_t* keyName = XAML_ROOT_KEY;

    wil::unique_hkey xamlKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, keyName, 0, KEY_READ, &xamlKey) != ERROR_SUCCESS)
    {
        return false;
    }

    DWORD stringSize = MAX_PATH + 1;
    DWORD keyType = REG_NONE;

    if (RegQueryValueExW(xamlKey.get(), valueName, 0, &keyType,
        reinterpret_cast<LPBYTE>(logDirectory), &stringSize) != ERROR_SUCCESS ||
        keyType != REG_SZ)
    {
        return false;
    }
    return true;
}

_Check_return_ HRESULT XcpLeakDetectionResult(UINT32 leakedMemoryBlocks, UINT64 leakedMemoryBytes, UINT32 leakedControlBlocks)
{
    if (leakedMemoryBlocks > 0)
    {
        LOG(L"");
        LOG_LEAK_EX(L"*** Leaked memory totals: %d blocks, %I64d bytes. ***",
            leakedMemoryBlocks,
            leakedMemoryBytes
        );
        LOG(L"");
    }

    if (leakedControlBlocks > 0)
    {
        LOG_LEAK_EX(L"*** Leaked memory totals: %d shared pointer control blocks. ***", leakedControlBlocks);
    }

    return ((leakedMemoryBlocks == 0) && (leakedControlBlocks == 0)) ? S_OK : E_FAIL;
}

// Walks the allocated heap listing every allocated heap node
// including where it was allocated from.
_Check_return_ HRESULT XcpCheckLeaks(unsigned int leakThresholdForSummary)
{
    DebugMemoryCheckBlock *pHeader = NULL;

    XUINT32 cLeakedMemoryBlocks = 0;
    XUINT32 cIgnoredBlocks = 0;
    XUINT64 cLeakedMemoryBytes  = 0;
    XUINT64 cIgnoredBytes = 0;
    WCHAR message[100];
    WCHAR description[400];
    XUINT32 uLeakedControlBlocks = 0;

    bool runningInModernTestProcess = false;
    // This has to be after we release the control block because it can change.
    pHeader = g_RootCheckBlock.pNext;
    while (pHeader != &g_RootCheckBlock)
    {
        if (static_cast<void*>(pHeader+1) != static_cast<void*>(g_pCheckedMemoryChainLock))
        {
            // Count ignored leaks.  Yes, yes, this sounds contradictory but we want
            // to make sure this number doesn't get out of hand.
            if (pHeader->allocationClass & AllocationIgnoreLeak)
            {
                cIgnoredBlocks++;
                cIgnoredBytes += pHeader->cSize;
            }
            else
            {
                // (We count anything except the memory chain critical section which we
                // expect to leave allocated)
                cLeakedMemoryBlocks++;
                cLeakedMemoryBytes += pHeader->cSize;
            }
        }
        pHeader = pHeader->pNext;
    }

    if (cLeakedMemoryBlocks > 0)
    {
        if (!GetPALDebuggingServices()->XcpTraceMonitorAttached())
        {
            // If we can't detect a debugger then show the dialog
            if (!IsDebuggerPresent())
            {
                // TESTSignalMemLeak is a named event used by the legacy test framework. We can't use such a
                // mechanism due to the ordering of TE.ProcessHost.exe's teardown.
                if (GetPALDebuggingServices()->TESTSignalMemLeak() == false)
                {
                    wchar_t errorLogDirectory[MAX_PATH + 1] = {0};

                    // This is how we can detect if we are running in an our modern test process or not.
                    // If we are, we actually want to display all the data as we can log it to the console
                    // so it will show up in test failure logs.
                    runningInModernTestProcess = TryReadErrorLogDirectoryFromRegistry(errorLogDirectory);

                    if (!runningInModernTestProcess)
                    {
                        StringCchPrintf(
                            message,
                            ARRAY_SIZE(message),
                            L"Native Memory Leaks have been detected. Please consider notifying 'xamlleaks' rather than filing a bug, to find out if this is a known issue. Attach a debugger to see the analysis"
                            );

                        StringCchPrintf(
                            description,
                            ARRAY_SIZE(description),
                            L"%s\n\nPress 'Yes' to call DebugBreak() after attaching the debugger, 'No' to ignore and continue.\n\n",
                            message
                            );

                        if (GetPALDebuggingServices()->YesNoMessageBox(description, L"Native Memory leak in XCP heap. Break?"))
                        {
                            ASSERT(g_RootCheckBlock.pNext == NULL, message);
                        }
                    }
                    else if (cLeakedMemoryBlocks > leakThresholdForSummary)
                    {
                        // If there are too many leaks, it can cause tests to timeout trying to display the ginormous call stacks
                        LOG_WARNING_EX(L"Showing leak summary only. On desktop, run locally to see full output.");
                        return XcpLeakDetectionResult(cLeakedMemoryBlocks, cLeakedMemoryBytes, uLeakedControlBlocks);
                    }
                }
            }
        }
    }

    // Start over and dump all the relevant information about the leaks.
    pHeader = g_RootCheckBlock.pNext;
    while (pHeader != &g_RootCheckBlock)
    {
        if (static_cast<void*>(pHeader+1) != static_cast<void*>(g_pCheckedMemoryChainLock))
        {
            if (!(pHeader->allocationClass & AllocationIgnoreLeak))
            {
                // Lookup the symbol of the VTable to help figure out what the object is
                ICallingMethod* pCallingMethods = NULL;
                if (pHeader->cSize >= sizeof(void*))
                {
                    XUINT64 caller = (XUINT64) (((void**) (pHeader+1))[0]);
                    GetPALDebuggingServices()->GetCallerSourceLocations(1, &caller, &pCallingMethods);
                    if (pCallingMethods)
                    {
                        // Since we are looking for VTables, just end the string at the first Colon.
                        WCHAR *pColon = xstrchr(pCallingMethods[0].szSymbolName, xstrlen(pCallingMethods[0].szSymbolName), L':');
                        if (pColon)
                        {
                            pColon[0] = L'\0';
                        }
                    }
                }

                LOG(L"Leaked memory at 0x%08Ix, length 0x%08I64x, allocation class %s, section %d, index %i, %s%s %s",
                    pHeader+1, // Address of content
                    pHeader->cSize,
                    AllocationClassName(DebugAllocationClass(pHeader->allocationClass)),
                    pHeader->section,
                    pHeader->index,
                    pCallingMethods ? L"Type " : L"",
                    pCallingMethods ? pCallingMethods[0].szSymbolName : L"",
                    pHeader->pCallers ? L", Call stack:" : L""
                );

                if (pCallingMethods)
                {
                    GetPALDebuggingServices()->FreeCallingMethods(pCallingMethods);
                    pCallingMethods = nullptr;
                }

                if (!runningInModernTestProcess)
                {
                    PrintAllocationStack(pHeader->cCallers, pHeader->pCallers);
                }

                // Display the internal content of the leak memory
                if ( GetPALDebuggingServices()->GetTraceFlags() & TraceLeakHexDump )
                {
                    Dump(pHeader+1, pHeader->cSize, 8);
                }

                // Display the owners of the leak memory
                FindOwners(pHeader);

                // Display AddRef/Release call stack, if there is one.
                if (pHeader->pAddRefHead)
                {
                    // Printout any AddRef/Release
                    PrintAddRefReleaseStack(pHeader);
                }
            }
        }
        pHeader = pHeader->pNext;
    }

    if (cIgnoredBlocks > 0)
    {
        LOG(L"Ignoring %d blocks, %d bytes of static media allocations", cIgnoredBlocks, cIgnoredBytes);
    }

    if (cLeakedMemoryBlocks > 0)
    {
        // Allocate a block of objects describing the leaked objects.  We'll use
        // this to attempt to find the root objects by inspecting pointers stored
        // in the objects.
        ObjectOwnerBlock* blocks = new ObjectOwnerBlock[cLeakedMemoryBlocks];
        XUINT32 iblock = 0;
        XUINT32 jblock = 0;

        if (blocks)
        {
            XcpDebugSetLeakDetectionFlag(blocks, true);

            // Walk the list again and fill with all the leaked blocks
            pHeader = g_RootCheckBlock.pNext;
            while (pHeader != &g_RootCheckBlock)
            {
                if ((static_cast<void*>(pHeader+1) != static_cast<void*>(g_pCheckedMemoryChainLock)) &&
                    !(pHeader->allocationClass & AllocationIgnoreLeak))
                {
                    // Don't include the block list itself in the search...
                    if (blocks != static_cast<void*>(pHeader + 1))
                    {
                        blocks[iblock].address = static_cast<void*>(pHeader + 1);
                        blocks[iblock].size = pHeader->cSize;
                        blocks[iblock].index = pHeader->index;
                        blocks[iblock].owned = FALSE;
                        blocks[iblock].owner = NULL;
                        blocks[iblock].pCallers = pHeader->pCallers;
                        blocks[iblock].cCallers = pHeader->cCallers;
                        iblock++;
                    }
                }
                pHeader = pHeader->pNext;
            }

            // Now walk the list of objects and try to establish ownership.
            for (iblock = 0; iblock < cLeakedMemoryBlocks; iblock++)
            {
                for (jblock = 0; jblock < cLeakedMemoryBlocks; jblock++)
                {
                    // If the owner block is newer than the test block then we assume it
                    // can't be a parent.  This assumes a usage pattern of:
                    // Create parent
                    //   Create child
                    //   Assign child to parent
                    // Which is the typical pattern.  This allows parent pointers and
                    // other weak references (such as a self reference) to not exclude
                    // an object as a top-level leaker.
                    if (blocks[jblock].index >= blocks[iblock].index)
                    {
                        continue;
                    }

                    if (AddressInObject(blocks[jblock].address, blocks[jblock].size, blocks[iblock].address))
                    {
                        blocks[iblock].owned = TRUE;
                        blocks[iblock].owner = blocks[jblock].address;
                        break;
                    }
                }
            }


            if (!runningInModernTestProcess)
            {
                // Display all owned objects.
                LOG(L"=====================================" );
                LOG(L"= Owned objects                     =" );
                LOG(L"=====================================" );
                LOG(L"" );
                // First dump owned objects
                for (iblock = 0; iblock < cLeakedMemoryBlocks; iblock++)
                {
                    // Lookup the symbol of the VTable to help figure out what the object is
                    ICallingMethod* pCallingMethods = NULL;
                    ICallingMethod* pCallingMethodsOwner = NULL;

                    if (blocks[iblock].owned)
                    {
                        if (blocks[iblock].size >= sizeof(void*))
                        {
                            XUINT64 caller = (XUINT64) (((void**) (blocks[iblock].address))[0]);
                            GetPALDebuggingServices()->GetCallerSourceLocations(1, &caller, &pCallingMethods);
                        }

                        {
                            XUINT64 ownerCaller = (XUINT64) (((void**) (blocks[iblock].owner))[0]);
                            GetPALDebuggingServices()->GetCallerSourceLocations(1, &ownerCaller, &pCallingMethodsOwner);
                        }

                        LOG(L"Owned  0x%08I64x %s   (Owner: 0x%08I64x %s)",
                            XUINT64(blocks[iblock].address),
                            pCallingMethods? pCallingMethods->szSymbolName : L"",
                            XUINT64(blocks[iblock].owner),
                            pCallingMethodsOwner? pCallingMethodsOwner->szSymbolName : L"" );
                    }

                    if (pCallingMethods)
                    {
                        GetPALDebuggingServices()->FreeCallingMethods(pCallingMethods);
                        pCallingMethods = NULL;
                    }
                    if (pCallingMethodsOwner)
                    {
                        GetPALDebuggingServices()->FreeCallingMethods(pCallingMethodsOwner);
                        pCallingMethodsOwner = NULL;
                    }
                }

                // Display Top Level Leaks.
                LOG(L"");
                LOG(L"=====================================");
                LOG(L"= Top Level objects (V1 algorithm)  =");
                LOG(L"=====================================");
                LOG(L"");
                // Now dump TLL objects with full allocation stack
                for (iblock = 0; iblock < cLeakedMemoryBlocks; iblock++)
                {
                    // Lookup the symbol of the VTable to help figure out what the object is
                    ICallingMethod* pCallingMethods = NULL;

                    if (!blocks[iblock].owned)
                    {
                        if (blocks[iblock].size >= sizeof(void*))
                        {
                            XUINT64 caller = (XUINT64) (((void**) (blocks[iblock].address))[0]);
                            GetPALDebuggingServices()->GetCallerSourceLocations(1, &caller, &pCallingMethods);
                        }

                        PrintAllocationStack(
                            blocks[iblock].cCallers,
                            blocks[iblock].pCallers,
                            L"Top Level LEAK",
                            blocks[iblock].address,
                            pCallingMethods,
                            blocks[iblock].owner,
                            nullptr);

                        // Display the internal content of the leak memory
                        if ( GetPALDebuggingServices()->GetTraceFlags() & TraceLeakHexDump )
                        {
                            Dump(blocks[iblock].address, blocks[iblock].size, 8);
                        }
                    }

                    if (pCallingMethods)
                    {
                        GetPALDebuggingServices()->FreeCallingMethods(pCallingMethods);
                        pCallingMethods = nullptr;
                    }
                }
            }

            // Clean up
            delete[] blocks;
            blocks = nullptr;

            XcpFindAndDumpTopLevelLeaksV2();
        }

        // Always break if stress mode is enabled and we're over the stress leak threshold.
        if (XcpDebugIsStressModeEnabled())
        {
            LOG(L"Stress mode enabled");

            if(cLeakedMemoryBytes >= GetPALDebuggingServices()->GetStressLeakThreshold())
            {
                LOG(L"Leak is higher than stress threshold, breaking...");
                DebugBreak();
            }
            else
            {
                LOG(L"Leak is lower than stress threshold, ignoring");
            }
        }
    }

    if (g_pLastFreedMemory)
    {
        // XcpDebugFree blocks the OS allocator from reallocating the last
        // freed memory immediately by holding on to the most recently
        // freed block. At this point it has already been removed from the
        // debug memory chain, so it won't have been detected as leaked above,
        // but we should still release it here so it doesn't show up if we're
        // running under another leak detection mechanism.
        XcpAllocation::OSMemoryFree(g_pLastFreedMemory);
        g_pLastFreedMemory = NULL;
    }

    return XcpLeakDetectionResult(cLeakedMemoryBlocks, cLeakedMemoryBytes, uLeakedControlBlocks);
}

void SendXcpMonMemoryMessage(
    XUINT32  uClass,      // MonitorAllocationPerf or MonitorDeallocationPerf
    XUINT64  memoryAddress,
    XUINT64  cBytes
);

void XcpDebugTrace(XUINT32 uClass, DebugAllocationClass allocationClass, void *pNewMemory, size_t cSize)
{
    if (GetPALDebuggingServices()->GetTraceFlags() & TraceAlloc)
    {
        XcpDebugTrace(
            uClass,
            __WFILE__,
            __LINE__,
            (XUINT32)(XUINT64)cSize,
            NULL,
            L".", // Signals presence of optional arguments
            L"address: 0x%x, cSize: %d, allocation class %s",
            (XINT32)(XUINT64)pNewMemory,
            cSize,
            AllocationClassName(allocationClass)
        );
    }

    if (GetPALDebuggingServices()->GetTraceFlags() & TraceMemoryPerf)
    {
        XUINT32 perfClass = (uClass == MonitorAllocation) ? MonitorAllocationPerf : MonitorDeallocationPerf;
        SendXcpMonMemoryMessage(
            perfClass,
            (XUINT64)pNewMemory,
            (XUINT64)cSize
        );
    }
}

IThreadMonitor* XcpDebugGetThreadMonitor()
{
    return GetPALDebuggingServices()->GetThreadMonitor(); // Suspend counting clocks
}

bool XcpDebugCaptureStack(_Out_ XUINT32 *pcCallers, _Outptr_result_buffer_(cMaxCallers) XUINT64  **ppCallers)
{
    // If xcpmon is connected, record the allocation call stack
    if (GetPALDebuggingServices()->GetTraceFlags() & TraceAllocationStack)
    {
        GetPALDebuggingServices()->CaptureStack(AllocationStackCaptureDepth, pcCallers, ppCallers, 5);
        return true;
    }

    return false;
}

void XcpDebugTraceLeakFullRefs(DebugMemoryCheckBlock *pHeader)
{
    if ( GetPALDebuggingServices()->GetTraceFlags() & TraceLeakFullRefs )
    {
        // Printout any AddRef/Release
        PrintAddRefReleaseStack(pHeader);
    }
}

void XcpTrackAddRefRelease(
    _In_ void *pAddress,
    XUINT32    cRef
    )
{
    DebugMemoryCheckBlock *pHeader = ((DebugMemoryCheckBlock*) pAddress) - 1;

    DebugAddRefList *pNode = (DebugAddRefList*)XcpAllocation::OSMemoryAllocateFailFast(sizeof(DebugAddRefList));

    if (pNode != NULL)
    {
        memset(pNode, 0, sizeof(DebugAddRefList));

        // Capture the current stack
        pNode->cRef = cRef;
        GetPALDebuggingServices()->CaptureStack(AllocationStackCaptureDepth, &pNode->cCallers, &pNode->pCallers, 3);

        auto lock = EnterCheckedMemoryChainLock();

        if (pHeader->pAddRefTail)
        {
            pHeader->pAddRefTail->pNext = pNode;
            pHeader->pAddRefTail = pNode;
        }
        else
        {
            pHeader->pAddRefHead = pHeader->pAddRefTail = pNode;
        }
    }

    return;
}

DebugMemoryCheckBlock* XcpFindRecentContainingBlock(void* ptrAtBlock)
{
    // First see if the pointer is a pointer to the start of a block.
    DebugMemoryCheckBlock *possibleBlock = ((DebugMemoryCheckBlock*)ptrAtBlock) - 1;
    if (possibleBlock->blockSignature == BLOCK_SIGNATURE)
    {
        return possibleBlock;
    }

    // If not a pointer to the start of a block, then check through all current allocations, from most recently to least recently
    // allocated.  This function is meant to be used shortly after the block is allocated, so normally this will be a short search.

    auto lock = EnterCheckedMemoryChainLock();

    XUINT32 giveUpCount = 32;
    DebugMemoryCheckBlock* retVal = NULL;
    for (DebugMemoryCheckBlock* blockIter = g_RootCheckBlock.pNext; blockIter != &g_RootCheckBlock; blockIter = blockIter->pNext)
    {
        void* start = blockIter + 1;
        void* end = (void*)(((XUINT64)(blockIter + 1)) + blockIter->cSize);

        if (start <= ptrAtBlock && ptrAtBlock < end)
        {
            // Found the allocation that spans over ptrAtBlock.
            retVal = blockIter;
            goto Cleanup;
        }

        giveUpCount -= 1;
        if (giveUpCount == 0)
        {
            goto Cleanup;
        }
    }

Cleanup:;
    // For statically-allocated objects, it's possible for this function to return NULL.

    return retVal;
}

PointerStrengthTag* XcpMarkPointer(
    _In_ void* thisPtr,
    _In_ void** ptrToPtr,
    _In_ LinkStrength strength,
    _In_ XUINT32 initialTrackedCount)
{
    DebugMemoryCheckBlock* containingBlock = XcpFindRecentContainingBlock(thisPtr);
    if (containingBlock == NULL)
    {
        // A given type is allowed to have static instances and dynamic instances.
        // Strong links from static instances are not dealt with by this top-level leak finder, at least not yet.
        // TODO, put static strong links on global list and look for them incoming to leaked objects...
        return NULL;
    }

    PointerStrengthTag* tag = (PointerStrengthTag*)XcpAllocation::OSMemoryAllocateFailFast(sizeof(PointerStrengthTag));

    // Most have ptrToPtr specifying a specific field within the block of the thisPtr.
    // However, for some xvector and similar cases, it is possible to tag an entire block as only having weak outbound links by using
    // -1 as the ptrToPtr value.

    tag->pointerToPointer = (XUINT64)ptrToPtr;
    tag->strength = strength;
    tag->trackedCount = initialTrackedCount;
    tag->next = containingBlock->taggedPointers;
    containingBlock->taggedPointers = tag;

    return tag;
}

extern "C"
void XcpMarkWeakPointer(
    _In_ void* thisPtr,
    _In_ void* ptrToWeakPtr)
{
    // input param is void* to get as much type-safety as possible (is a pointer) without forcing casting all over the place at call sites (by
    // leveraging fact that casting to void* from any pointer type works).
    XcpMarkPointer(thisPtr, (void**)ptrToWeakPtr, LinkStrengthWeak, 0xFFFFFFFF);
}

extern "C"
void XcpMarkStrongPointer(
    _In_ void* thisPtr,
    _In_ void* ptrToStrongPtr)
{
    // input param is void* to get as much type-safety as possible (is a pointer) without forcing casting all over the place at call sites (by
    // leveraging fact that casting to void* from any pointer type works).
    XcpMarkPointer(thisPtr, (void**)ptrToStrongPtr, LinkStrengthStrong, 0xFFFFFFFF);
}

extern "C"
void XcpMarkPartTimeStrongPointer(
    _In_ void* thisPtr,
    _In_ void* ptrToPartTimeStrongPtr,
    _Out_ void** context)
{
    // input param is void* to get as much type-safety as possible (is a pointer) without forcing casting all over the place at call sites (by
    // leveraging fact that casting to void* from any pointer type works).
    PointerStrengthTag* tag = XcpMarkPointer(thisPtr, (void**)ptrToPartTimeStrongPtr, LinkStrengthStrong, 0);
    *context = tag;
}

extern "C"
void XcpIncPartTimeStrongPointer(
    _In_ void* context)
{
    if (context == NULL)
    {
        return;
    }

    PointerStrengthTag* tag = (PointerStrengthTag*)context;
    PAL_InterlockedIncrement((XINT32*)&tag->trackedCount);
}

extern "C"
void XcpDecPartTimeStrongPointer(
    _In_ void* context)
{
    if (context == NULL)
    {
        return;
    }

    PointerStrengthTag* tag = (PointerStrengthTag*)context;
    PAL_InterlockedDecrement((XINT32*)&tag->trackedCount);
}

void XcpMarkHardToTrack(
    _In_ void* hardToTrackBlock)
{
    DebugMemoryCheckBlock* containingBlock = XcpFindRecentContainingBlock(hardToTrackBlock);
    if (containingBlock == NULL)
    {
        // A given type is allowed to have static instances and dynamic instances.
        // Strong links from static instances are not dealt with by this top-level leak finder, at least not yet.
        // TODO, put static strong links on global list and look for them incoming to leaked objects...

        // Don't know if this ever actually happens currently for this particular path:
        return;
    }

    containingBlock->isHardToTrack = true;

    return;
}

// This exists because we want to find the blocks in a loop we just noticed, without unwinding the recursion yet, because the recursion isn't done finding other loops
// via other links.
struct TraversalInStackLink
{
    // a given block will only be actually traversed (beyond just "looked at") once at a time, so the "block" field is unique among all current TraversalInStackLink structs.
    DebugMemoryCheckBlock* block;
    // toward the leaf direction, in terms of block links.
    TraversalInStackLink* leafward;
};

void XcpRecursiveTagTopLevelV2(
    _In_ TraversalInStackLink* leafward,
    _In_ DebugMemoryCheckBlock* block
    )
{
    // Check whether a top-level loop is found.
    if (block->isOnTraversalStack)
    {
        // Already currently traversing this block (up-stack, and leafward of self in terms of block links), so found a loop.  Don't re-traverse, just mark the loop.
        // The up-stack frame is taking care of actually traversing this block.
        // Marking the loop involves walking the in-stack traversal links.  Probably there's a better way but this works.

        TraversalInStackLink* iter = leafward;
        DebugMemoryCheckBlock* leafwardBlock;
        do
        {
            leafwardBlock = iter->block;
            leafwardBlock->topLevelType = TopLevelType_Loop;
            iter = iter->leafward;
        } while (leafwardBlock != block);

        return; // don't goto Cleanup; here.
    }

    // Iterating all possible loops takes too long, so don't.  At least one loop involving any object that's involved in a loop will
    // still be found (80% sure of that statement).
    if (block->isConsideredBefore)
    {
        return;
    }

    // Check whether this block is itself top-level.  We avoided representing any links that are known weak in this list, so if the list is empty it means there are no links of strength >= unknown.
    if (block->inboundLinkList == NULL)
    {
        // no inbound links of sufficient strength, so mark as top-level and return.  caller can keep looking for other reasons for liveness, in case there is more than one reason.
        block->topLevelType = TopLevelType_Single;

        return; // don't goto Cleanup; here.
    }

    // Nothing identified yet, so actually traverse the block (or realize it's top level and mark as such).

    TraversalInStackLink whatThisFrameIsCurrentlyTraversing;
    whatThisFrameIsCurrentlyTraversing.leafward = leafward;
    whatThisFrameIsCurrentlyTraversing.block = block;

    block->isOnTraversalStack = true; // any return from here down must use goto Cleanup;

    for (InboundLink* iter = block->inboundLinkList; iter != NULL; iter = iter->next)
    {
        XcpRecursiveTagTopLevelV2(&whatThisFrameIsCurrentlyTraversing, iter->headerOfLinkSource);
    }

    block->isOnTraversalStack = false;
    block->isConsideredBefore = true;

    return;
}

void XcpDumpTopLevelLeaksV2(
    _In_ bool isHardToTrackFilter,
    _In_ TopLevelType topLevelTypeFilter,
    _In_ LinkStrength incomingMaxFilter,
    _In_ LinkStrength outboundMaxFilter
    )
{
    for (DebugMemoryCheckBlock* blockIter = g_RootCheckBlock.pNext; blockIter != &g_RootCheckBlock; blockIter = blockIter->pNext)
    {
        if (blockIter->isHardToTrack != isHardToTrackFilter)
        {
            continue;
        }

        if (blockIter->topLevelType != topLevelTypeFilter)
        {
            continue;
        }

        LinkStrength outboundMax = blockIter->maxOutboundLinkStrength;

        if (outboundMax != outboundMaxFilter)
        {
            continue;
        }

        LinkStrength incomingMax = LinkStrengthNone;

        for (InboundLink* iter = blockIter->inboundLinkList; iter != NULL; iter = iter->next)
        {
            if (iter->strength > incomingMax)
            {
                incomingMax = iter->strength;
            }
        }

        if (incomingMax != incomingMaxFilter)
        {
            continue;
        }

        // ok, passed the filters.

        if (blockIter->allocationClass & AllocationIgnoreLeak)
        {
            continue;
        }

        // Now dump the block with full allocation stack

        // Lookup the symbol of the VTable to help figure out what the object is
        ICallingMethod* pCallingMethods = NULL;

        if (blockIter->cSize >= sizeof(void*))
        {
            XUINT64 caller = (XUINT64) (((void**) (blockIter+1))[0]);
            GetPALDebuggingServices()->GetCallerSourceLocations(1, &caller, &pCallingMethods);
        }

        PrintAllocationStack(
            blockIter->cCallers,
            blockIter->pCallers,
            L"Top Level LEAK",
            blockIter + 1,
            pCallingMethods,
            NULL,
            NULL);

        // Display the internal content of the leak memory
        if ( GetPALDebuggingServices()->GetTraceFlags() & TraceLeakHexDump )
        {
            Dump(blockIter + 1, blockIter->cSize, 8);
        }

        if (pCallingMethods)
        {
            GetPALDebuggingServices()->FreeCallingMethods(pCallingMethods);
            pCallingMethods = NULL;
        }
    }
}

DebugMemoryCheckBlock** g_SortedArray;

// only call this if not supposed to be silent...
void XcpFindAndDumpTopLevelLeaksV2()
{
    XUINT64 minAddress = 0xFFFFFFFFFFFFFFFF;
    XUINT64 maxAddress = 0;
    XUINT32 count = 0;

    LOG_INFO_EX(L"Running top level leak finder V2 algorithm" );
    LOG_INFO_EX(L"Scanning for min/max/count...");

    for (DebugMemoryCheckBlock* iter = g_RootCheckBlock.pNext; iter != &g_RootCheckBlock; iter = iter->pNext)
    {
        if (iter->allocationClass & AllocationIgnoreLeak)
        {
            continue;
        }

        XUINT64 blockStart = (XUINT64)(iter+1);
        XUINT64 blockEnd = blockStart + iter->cSize;
        if (blockEnd > maxAddress)
        {
            maxAddress = blockEnd;
        }
        if (blockStart < minAddress)
        {
            minAddress = blockStart;
        }
        count += 1;
    }

    LOG_INFO_EX(L"min: 0x%08I64x max: 0x%08I64x diff: 0x%08I64x count: %lu", minAddress, maxAddress, maxAddress - minAddress, count);

    g_SortedArray = (DebugMemoryCheckBlock**)XcpAllocation::OSMemoryAllocateFailFast(count * sizeof(DebugMemoryCheckBlock*));
    // just assume successful allocation...

    XUINT64 index = 0;
    for (DebugMemoryCheckBlock* iter = g_RootCheckBlock.pNext; iter != &g_RootCheckBlock; iter = iter->pNext)
    {
        if (iter->allocationClass & AllocationIgnoreLeak)
        {
            continue;
        }
        // allocation may have failed above if it crashes here - or maybe we should take the chain lock for this whole function.
        g_SortedArray[index++] = iter;
    }
    ASSERT(count == index);

    // shell sort
    if (count >= 2)
    {
        XUINT32 interval = count / 2;
        do {
            bool anySwapped;
            do {
                anySwapped = false;
                for (index = 0; index < count - interval; index++)
                {
                    if (g_SortedArray[index] > g_SortedArray[index + interval])
                    {
                        // swap
                        DebugMemoryCheckBlock* temp = g_SortedArray[index];
                        g_SortedArray[index] = g_SortedArray[index + interval];
                        g_SortedArray[index + interval] = temp;
                        anySwapped = true;
                    }
                }
            } while (anySwapped);
            interval /= 2;
        } while (interval >= 1);
    }

    LOG_INFO_EX(L"sorting done");

    // scan for pointer links between allocated blocks, and build a list of the incoming links in each destination block header.
    for (XUINT32 srcIndex = 0; srcIndex < count; srcIndex += 1)
    {
        DebugMemoryCheckBlock* srcIter = g_SortedArray[srcIndex];
        ASSERT(!(srcIter->allocationClass & AllocationIgnoreLeak));

        XUINT64 remainingSize = srcIter->cSize;
        void** srcPtrScan = (void**)(srcIter+1);
        while (remainingSize >= sizeof(void*))
        {
            XUINT64 targetLoc = (XUINT64)*srcPtrScan;

            if (targetLoc < minAddress)
            {
                goto NextSrcPtr;
            }

            if (targetLoc >= maxAddress)
            {
                goto NextSrcPtr;
            }

            // binary search
            XUINT32 minIndex = 0;
            XUINT32 maxIndex = count - 1;
            XUINT32 curIndex;
            do
            {
                curIndex = (minIndex + maxIndex) / 2;
                DebugMemoryCheckBlock* testBlock = g_SortedArray[curIndex];
                XUINT64 start = (XUINT64)(testBlock + 1);
                XUINT64 end = start + testBlock->cSize;
                if (targetLoc >= end)
                {
                    minIndex = curIndex + 1;
                    continue;
                }
                if (targetLoc < start)
                {
                    maxIndex = curIndex - 1;
                    continue;
                }
                // found it.
                ASSERT(targetLoc >= start && targetLoc < end);
                minIndex = curIndex;
                maxIndex = curIndex;
                goto DoneSearching;
            } while (minIndex <= maxIndex);
        DoneSearching:;

            if (minIndex > maxIndex)
            {
                goto NextSrcPtr;
            }
            ASSERT(minIndex <= maxIndex);

            DebugMemoryCheckBlock* dstIter = g_SortedArray[curIndex];

            if (dstIter == srcIter)
            {
                // assume that links to self are not causing leaks.
                goto NextSrcPtr;
            }

            // Link found from src to dst.  Might be able to determine strength of the link, or use default of unknown...

            PointerStrengthTag* psIter;
            for (psIter = srcIter->taggedPointers; psIter != NULL; psIter = psIter->next)
            {
                if (psIter->pointerToPointer == (XUINT64)(void*)-1)
                {
                    goto FoundMarker;
                }
                if (psIter->pointerToPointer == (XUINT64)srcPtrScan)
                {
                    goto FoundMarker;
                }
            }
        FoundMarker:;

            LinkStrength linkStrength;
            if (psIter != NULL)
            {
                if (psIter->strength == LinkStrengthStrong && psIter->trackedCount == 0)
                {
                    linkStrength = LinkStrengthWeak;
                }
                else
                {
                    linkStrength = psIter->strength;
                }
            }
            else
            {
                linkStrength = LinkStrengthUnknown;
            }

            if (linkStrength <= LinkStrengthWeak)
            {
                goto SkipMakingWeakLink;
            }

            // These are leaked.  Avoid using "new" here to keep the layers separate, though no big reason "new" couldn't work if
            // we marked the block AllocationIgnoreLeak (still, easier to think about this way).
            InboundLink* inboundLink = (InboundLink*)XcpAllocation::OSMemoryAllocateFailFast(sizeof(InboundLink));

            inboundLink->headerOfLinkSource = srcIter;
            inboundLink->strength = linkStrength;

            // push to head of list.
            inboundLink->next = dstIter->inboundLinkList;
            dstIter->inboundLinkList = inboundLink;

        SkipMakingWeakLink:;

            // max outbound link strength is used later to dump in a helpful order (most likely to be relevant listed last)
            if (linkStrength > srcIter->maxOutboundLinkStrength)
            {
                srcIter->maxOutboundLinkStrength = linkStrength;
            }

        NextSrcPtr:;
            srcPtrScan++;
            remainingSize -= sizeof(void*);
        }
    //NextSrcBlock:;
    }
    // ok, now each block has a list of all incoming links.

    LOG(L"done building InboundLink reverse-map");

    // for each block, follow incoming links of sufficient strength to find and tag the top level leaked blocks.
    // Marking already-traversed blocks would potentially miss alternate loop paths - maybe there's a clever faster way but I haven't yet thought of a faster way
    // that finds all possible top-level blocks in all situations.

    for (DebugMemoryCheckBlock* blockIter = g_RootCheckBlock.pNext; blockIter != &g_RootCheckBlock; blockIter = blockIter->pNext)
    {
        if (blockIter->allocationClass & AllocationIgnoreLeak)
        {
            continue;
        }

        XcpRecursiveTagTopLevelV2(NULL, blockIter);
    }
    // ok, now top-level blocks are tagged.

    // Dump the top-level blocks.  Try to dump in an order that puts the most interesting ones last.

    // For dumping, do this order,
    //   * max incoming is strong (loop type)
    //   * max incoming is unknown (loop type)
    //   * max incoming is weak or none (single type)
    // Within each of the above,
    //   * max outbound is weak or none
    //   * max outbound is unknown
    //   * max outbound is strong

    LOG(L"");
    LOG_INFO_EX(L"=====================================");
    LOG_INFO_EX(L"= Top level blocks (V2 scan)        =");
    LOG_INFO_EX(L"=====================================");
    LOG(L"");

    LinkStrength strengthType[4] = {LinkStrengthStrong, LinkStrengthUnknown, LinkStrengthWeak, LinkStrengthNone};
    for (XINT32 hardToTrack = 1; hardToTrack >= 0; hardToTrack--)
    {
        bool hardToTrackFilter = (hardToTrack != 0);

        for (XUINT32 topLevelTypeFilter = TopLevelType_Loop; topLevelTypeFilter >= TopLevelType_Single; topLevelTypeFilter--)
        {

            // incoming LinkStrengthWeak won't print anything because it ends up not getting tracked so always
            //   looks like LinkStrengthNone.
            // Tracking the weak strength despite not tracking the actual link would make sense, if there's a case
            //   where it might remove some ambiguity (but hopefully there will always be later-printed blocks that
            //   are more interesting).
            // outbound LinkStrengthWeak can print stuff, in contrast to incoming LinkStrengthWeak.

            for (int incoming = 0; incoming < 4; incoming++)
            {
                for (int outbound = 3; outbound >= 0; outbound--)
                {
                    LOG(L"hardToTrack: %lu topLevelType: %lu incomingStrength: %lu outboundStrength: %lu", hardToTrack, topLevelTypeFilter, strengthType[incoming], strengthType[outbound]);
                    XcpDumpTopLevelLeaksV2(hardToTrackFilter, (TopLevelType)topLevelTypeFilter, strengthType[incoming], strengthType[outbound]);
                }
            }
        }
    }

    LOG(L"");
    LOG_INFO_EX(L"================================================================================================================");
    LOG_INFO_EX(L"= Top level blocks above are printed in order from least to most likely to be the real problem (V2 scan)       =");
    LOG_INFO_EX(L"= Start at the last block printed above and fix why it's leaking or why its marking is broken, re-run, repeat. =");
    LOG_INFO_EX(L"================================================================================================================");
    LOG(L"");
}

void XcpIgnoreAllOutstandingAllocations()
{
    for (DebugMemoryCheckBlock* blockIter = g_RootCheckBlock.pNext; blockIter != &g_RootCheckBlock; blockIter = blockIter->pNext)
    {
        XcpDebugSetLeakDetectionFlag(static_cast<void*>(blockIter + 1), true);
    }
}
#endif
