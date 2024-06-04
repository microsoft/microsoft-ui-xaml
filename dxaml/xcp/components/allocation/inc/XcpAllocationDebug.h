// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#if XCP_MONITOR
#include <wil\resource.h>

const UINT64 XcpHdrB = 0x42726448706358LL; // "XcpHdrB"
const UINT64 XcpHdrE = 0x45726448706358LL; // "XcpHdrE"
const UINT64 XcpTlrB = 0x42726c54706358LL; // "XcpTlrB"
const UINT64 XcpTlrE = 0x45726c54706358LL; // "XcpTlrE"

// Randomly-generated signature value.
#define BLOCK_SIGNATURE 0xA6B0E1B1D486CDFF

const UINT32 AllocationStackCaptureDepth = 32;

// Allocation classes - correspond to different allocators
enum DebugAllocationClass
{
    AllocationNewScalar = 0,
    AllocationNewArray = 1,
    AllocationFlat = 2,
    AllocationClassMax = 2,
    AllocationClassMask = 0x7FFF,
    AllocationIgnoreLeak = 0x8000
};

struct DebugAddRefList
{
    DebugAddRefList       *pNext;        // Pointer to the next entry
    UINT32                cRef;         // Current Reference count
    UINT64               *pCallers;     // Array of up to [recordedStackDepth] caller return addresses
    UINT32                cCallers;     // Number of useful entries in pCallers
};

enum LinkStrength
{
    // numeric value is used, so order matters.
    LinkStrengthNone,
    LinkStrengthWeak,
    LinkStrengthUnknown,
    LinkStrengthStrong,
    LinkStrength_Count
};

enum TopLevelType
{
    TopLevelType_NotTopLevel = 0,
    TopLevelType_Single, // != 0
    TopLevelType_Loop    // != 0
};

struct PointerStrengthTag
{
    LinkStrength strength;
    UINT64 pointerToPointer;
    // most are not tracked, but part-time strong links are.
    // 0xFFFFFFFF if not tracked.  Outbound ref count if tracked, possibly 0.
    // When 0, considered a weak link instead of strong.
    UINT32 trackedCount;
    PointerStrengthTag* next;
};

struct DebugMemoryCheckBlock;

struct InboundLink
{
    DebugMemoryCheckBlock* headerOfLinkSource;
    LinkStrength strength;
    InboundLink* next;
};

struct DebugMemoryCheckBlock
{
    UINT64                 chXcpB;             // Must be 'XcpHdrB' or 'XcpTlrB'
    UINT16                 guard[32];          // Each word must be low word of address
    UINT64                 cSize;              // Excludes size of check blocks.
    UINT64                 blockSignature;     // BLOCK_SIGNATURE
    DebugMemoryCheckBlock*  pPrevious;          // Allways points to a header check block
    DebugMemoryCheckBlock*  pNext;              // Allways points to a header check block
    DebugMemoryCheckBlock*  pTrailer;           // Allways points to trailer block
    UINT64*                pCallers;           // Array of up to [recordedStackDepth] caller return addresses
    UINT32                 cCallers;           // Number of useful entries in pCallers
    UINT16                 allocationClass;
    UINT16                 fDeallocated;       // Bit 2 set iff deallocated
    DebugAddRefList*        pAddRefHead;        // List of AddRefs and Releases
    DebugAddRefList*        pAddRefTail;        // List of AddRefs and Releases
    UINT32                 section;            // Section of code active at allocation time
    UINT32                 index;              // Allocation index - copied from g_AllocationCount

    // These are used by the top-level-leak finder (V2).  Some of these could be out of band or allocated while finding leaks, but this works.
    PointerStrengthTag*     taggedPointers;     // List of which pointers in this block are known to be weak or strong pointers
    InboundLink*            inboundLinkList;    // List of inbound pointers from other blocks.
    LinkStrength            maxOutboundLinkStrength; // Maximum strength among outbound pointers.
    TopLevelType            topLevelType;       // Whether this is top-level, and if so, whether it's a loop or single block
    bool                    isOnTraversalStack; // Whether this block is currently being traversed by the top-level-leak finder (V2)
    bool                    isConsideredBefore; // Whether this block has been considered before, to save considering again.  This early-out might miss reporting all possible loops, but will still report at least one loop if any loops exist.
    bool                    isHardToTrack;      // If this block ends up top-level, de-emphasize it in the dump unless there's nothing better.

    UINT64                 chXcpE;             // Must be 'XcpHdrE' or 'XcpTlrE'
};

// Marks objects as free to leak.  Should only be used by the media platform
extern INT16  g_bMarkLeakable;

extern DebugMemoryCheckBlock g_RootCheckBlock;

struct  ObjectOwnerBlock
{
    UINT64     size;       // Size of object
    void       *address;    // Base address of object
    INT32      index;      // When the object was allocated
    INT32      owned;      // Has ownership be established for this object?
    UINT64    *pCallers;   // Array of up to [recordedStackDepth] caller return addresses
    UINT32     cCallers;   // Number of useful entries in pCallers
    void       *owner;      // Base address of object's owner
};

extern wil::critical_section* g_pCheckedMemoryChainLock;

void InitCheckedMemoryChainLock();

void DeleteCheckedMemoryChainLock();

wil::cs_leave_scope_exit EnterCheckedMemoryChainLock();

extern LONG  g_AllocationCount;
extern void  *g_pLastFreedMemory;

// Memory allocator for new / malloc, that in debug builds adds check blocks
// before and after the allocated memory, and sends a message to the monitoring
// process recording the allocation and where in the source it came from.
_Check_return_ __declspec(allocator) void *XcpDebugAllocate(
    size_t               cSize,
    DebugAllocationClass allocationClass
);

// Memory deallocator for delete/free, that in debug builds tests check blocks
// before and after the allocated memory, and sends a message to the monitoring
// process recording the allocation and where in the source it came from.
void XcpDebugFree(
    _In_ void            *pAddress,
    DebugAllocationClass  allocationClass
);

// Memory re-allocator for new / malloc, that in debug builds adds check blocks
// before and after the allocated memory, and sends a message to the monitoring
// process recording the allocation and where in the source it came from.
_Check_return_ __declspec(allocator) void *XcpDebugResize(
    _In_ void            *pAddress,
    size_t                cSize,
    DebugAllocationClass  allocationClass
);

// Mark LeakDetection flag in individual object that was allocated
// through XcpDebugAllocate.
void XcpDebugSetLeakDetectionFlag(_In_ void *pAddress, _In_ bool fDisableLeakDetection);

// Ignores allocations for the lifetime of the function that is returned, can be stopped
// if the function is invoked manually
wil::details::lambda_call<std::function<void(void)>> XcpDebugStartIgnoringLeaks();

#endif
