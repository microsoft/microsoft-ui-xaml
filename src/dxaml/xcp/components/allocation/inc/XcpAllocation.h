// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <wil\common.h>

enum NoFailFastAllocationPolicy
{
    NO_FAIL_FAST
};

enum ZeroMemAllocationPolicy
{
    ZERO_MEM_FAIL_FAST
};

_Ret_notnull_ _Post_writable_byte_size_(cSize) __declspec(allocator) void * __cdecl operator new(size_t cSize, NoFailFastAllocationPolicy);
_Ret_notnull_ _Post_writable_byte_size_(cSize) XCP_FORCEINLINE __declspec(allocator) void * __cdecl operator new[](size_t cSize, NoFailFastAllocationPolicy);

_Ret_notnull_ _Post_writable_byte_size_(cSize) __declspec(allocator) void * __cdecl operator new(size_t cSize, ZeroMemAllocationPolicy);
_Ret_notnull_ _Post_writable_byte_size_(cSize) XCP_FORCEINLINE __declspec(allocator) void * __cdecl operator new[](size_t cSize, ZeroMemAllocationPolicy);

namespace XcpAllocation {
    
    _Check_return_ __declspec(allocator) void *OSMemoryAllocateFailFast(_In_ size_t cSize);
    _Check_return_ __declspec(allocator) void *OSMemoryAllocateNoFailFast(_In_ size_t cSize);
    _Check_return_ __declspec(allocator) void *OSMemoryAllocateZeroMemoryFailFast(_In_ size_t cSize);
    _Check_return_ __declspec(allocator) void *OSMemoryResize(_Frees_ptr_opt_ void *pAddress, _In_ size_t cSize);
    void __stdcall OSMemoryFree(_Frees_ptr_opt_ void *pAddress);

    // An allocator that allows STL to hook into the XcpDebugSetLeakDetectionFlag() machinery
    template <typename T>
    struct LeakIgnoringAllocator
    {
        typedef T value_type;

        __declspec(allocator) T* allocate(size_t n) const
        {
            if (n == 0) { return nullptr; }
            if (n > static_cast<size_t>(-1) / sizeof(T))
            {
                // Prevent overflow
                XAML_FAIL_FAST();
            }

            void* result = ::operator new(n * sizeof(T));
            ASSERT(result);
#if XCP_MONITOR
            ::XcpDebugSetLeakDetectionFlag(result, true);
#endif
            return static_cast<T*>(result);
        }

        void deallocate(T* ptr, size_t) const WI_NOEXCEPT
        {
            ::operator delete(ptr);
        }

        // I think this is needed because of our reliance on STL110.
        // We may be able to remove this method when we update.
        template<class Other>
        void destroy(Other* ptr)
        {
            // destroy object at _Ptr
            ptr->~Other();
        }

        // Allocator boilerplate
        LeakIgnoringAllocator() WI_NOEXCEPT{}
        LeakIgnoringAllocator(const LeakIgnoringAllocator&) WI_NOEXCEPT{}
        template <typename U> LeakIgnoringAllocator(const LeakIgnoringAllocator<U>&) WI_NOEXCEPT{}
        template <typename U> bool operator==(
            const LeakIgnoringAllocator<U>&) const WI_NOEXCEPT { return true; }
        template <typename U> bool operator!=(
            const LeakIgnoringAllocator<U>&) const WI_NOEXCEPT { return false; }
        template <typename U> struct rebind {
            typedef LeakIgnoringAllocator<U> other;
        };

    };
}
