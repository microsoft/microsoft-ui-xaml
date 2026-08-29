// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "XcpAllocation.h"

// Right now we define our own memory allocators for Jupiter in debug builds. This
// historically was done to support a feature called XcpLeakDetector. This feature
// tracks internally allocated memory and raises an error in debug builds if Jupiter
// exits with allocated memory.
//
// In fre build we also define our own memory allocator to keep track of current
// total allocation to report to the GC and to FAIL_FAST on failed small allocations.
//
// TODO: Our allocator can return null. A better SAL annotation for the return
// value (and the other new operator) is: _Ret_maybenull_ _Post_writable_byte_size_(_Size)

#if DBG && !defined(NO_XCP_NEW_AND_DELETE) && !defined(_PREFAST_)

#include "XcpAllocationDebug.h"

// In checked builds, memory allocation is done through the memory allocator
// in the PAL debugging service.
__declspec(allocator) void * __cdecl operator new(size_t cSize)
{
    return XcpDebugAllocate(cSize, AllocationNewScalar);
}

__bcount(cSize) __declspec(allocator) void * __cdecl operator new[](size_t cSize)
{
    return XcpDebugAllocate(cSize, AllocationNewArray);
}

__declspec(allocator) void * __cdecl operator new(size_t cSize, NoFailFastAllocationPolicy /*policy*/)
{
    return XcpDebugAllocate(cSize, AllocationNewScalar);
}

__bcount(cSize) XCP_FORCEINLINE __declspec(allocator) void * __cdecl operator new[](size_t cSize, NoFailFastAllocationPolicy /*policy*/)
{
    return XcpDebugAllocate(cSize, AllocationNewArray);
}

__declspec(allocator) void * __cdecl operator new(size_t cSize, ZeroMemAllocationPolicy /*policy*/)
{
    void * pAllocation = XcpDebugAllocate(cSize, AllocationNewScalar);
    ZeroMemory(pAllocation, cSize);
    return pAllocation;
}

__bcount(cSize) XCP_FORCEINLINE __declspec(allocator) void * __cdecl operator new[](size_t cSize, ZeroMemAllocationPolicy /*policy*/)
{
    void * pAllocation = XcpDebugAllocate(cSize, AllocationNewArray);
    ZeroMemory(pAllocation, cSize);
    return pAllocation;
}

#pragma warning(push)
// Some header files have their own definition of this function and don't
// properly annotate it. We disable this warning to avoid inconsistent annotation
// errors for now.
#pragma warning(disable:28301)

void __cdecl operator delete(_Frees_ptr_opt_ void *pAddress) noexcept
{
    XcpDebugFree(pAddress, AllocationNewScalar);
}

void __cdecl operator delete[](_Frees_ptr_opt_ void *pAddress) noexcept
{
    XcpDebugFree(pAddress, AllocationNewArray);
}

#pragma warning(pop)

#elif !defined(NO_XCP_NEW_AND_DELETE)

// Fail fast allocators - these are also default allocators
_Ret_notnull_ _Post_writable_byte_size_(cSize) __declspec(allocator) void * __cdecl operator new(size_t cSize)
{
    return XcpAllocation::OSMemoryAllocateFailFast(cSize);
}

_Ret_notnull_ _Post_writable_byte_size_(cSize) __declspec(allocator) void * __cdecl operator new[](size_t cSize)
{
    return XcpAllocation::OSMemoryAllocateFailFast(cSize);
}

// Zero memory allocators
_Ret_notnull_ _Post_writable_byte_size_(cSize) __declspec(allocator) void * __cdecl operator new(size_t cSize, ZeroMemAllocationPolicy /*policy*/)
{
    return XcpAllocation::OSMemoryAllocateZeroMemoryFailFast(cSize);
}

_Ret_notnull_ _Post_writable_byte_size_(cSize) __declspec(allocator) void * __cdecl operator new[](size_t cSize, ZeroMemAllocationPolicy /*policy*/)
{
    return XcpAllocation::OSMemoryAllocateZeroMemoryFailFast(cSize);
}

// No fail fast allocators
_Ret_notnull_ _Post_writable_byte_size_(cSize) __declspec(allocator) void * __cdecl operator new(size_t cSize, NoFailFastAllocationPolicy /*policy*/)
{
    return XcpAllocation::OSMemoryAllocateNoFailFast(cSize);
}

_Ret_notnull_ _Post_writable_byte_size_(cSize) __declspec(allocator) void * __cdecl operator new[](size_t cSize, NoFailFastAllocationPolicy /*policy*/)
{
    return XcpAllocation::OSMemoryAllocateNoFailFast(cSize);
}

// Some header files have their own definition of this function and don't 
// properly annotate it. We disable this warning to avoid inconsistent annotation
// errors for now.

#pragma warning(suppress: 28301) // warning C28301: No annotations for first declaration
void __cdecl operator delete(_Frees_ptr_opt_ void *pAddress)
{
    if (pAddress)
    {
        XcpAllocation::OSMemoryFree(pAddress);
    }
}

#pragma warning(suppress: 28301) // warning C28301: No annotations for first declaration
void __cdecl operator delete[](_Frees_ptr_opt_ void *pAddress)
{
    if (pAddress)
    {
        XcpAllocation::OSMemoryFree(pAddress);
    }
}

#endif // #if DBG && !defined(NO_XCP_NEW_AND_DELETE)

_Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(cSize) __declspec(allocator) void * __cdecl operator new(size_t cSize, std::nothrow_t const&) noexcept
{
    return operator new(cSize);
}

_Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(cSize) __declspec(allocator) void * __cdecl operator new[](size_t cSize, const std::nothrow_t&) noexcept
{
    return operator new[](cSize);
}

#pragma warning(suppress: 28301) // warning C28301: No annotations for first declaration
void __cdecl operator delete(_Frees_ptr_opt_ void *pAddress, const std::nothrow_t&) noexcept
{
    operator delete(pAddress);
}

#pragma warning(suppress: 28301) // warning C28301: No annotations for first declaration
void __cdecl operator delete[](_Frees_ptr_opt_ void *pAddress, const std::nothrow_t&) noexcept
{
    operator delete[](pAddress);
}

