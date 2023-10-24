// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Adaptation of Howard Hinnant's stack allocator

// Example:
// void foo(const unordered_set<SomeType>& s) {
//   typedef arena<32*sizeof(SomeType)> arena_t; // This application figures 32 elements should usually be big enough
//   typedef stack_allocator<SomeType, arena_t> allocator_t;

//   arena_t local_arena; // local stack space resides here
//   vector<SomeType, allocator_t> v(allocator_t(local_arena));
//   v.reserve(s.size()); // If the local buffer is big enough, the vector uses it. Otherwise, it calls out to global heap allocation.
//   v.assign(s.begin(), s.end());
//   // Do something with v
//   // When v is destroyed, it will call allocator.deallocate(), which is a no-op if we stayed within the stack buffer.
// }

// Be sure no instances of the allocator outlive the arena!

#pragma once

#include <cstddef>
#include <minerror.h>

namespace Jupiter {

    template<std::size_t N, std::size_t alignment = alignof(std::max_align_t)>
    class arena
    {
        alignas(alignment) char m_buf[N];
        char* m_ptr;

        bool pointer_in_buffer(char* p) WI_NOEXCEPT
        {
            return m_buf <= p && p <= m_buf + size();
        }

        // Round up to next aligned "slot"
        static std::size_t align_up(std::size_t n) WI_NOEXCEPT
        {
            return n + (alignment - 1) & ~(alignment - 1);
        }

    public:
        arena() WI_NOEXCEPT : m_ptr(m_buf) {}
        ~arena() WI_NOEXCEPT{ m_ptr = nullptr; } // Sanity value for dangling reference checks

        // Noncopyable, nonmovable
        arena(const arena&) = delete;
        arena& operator=(const arena&) = delete;

        static constexpr std::size_t size() WI_NOEXCEPT { return N; }

        std::size_t used() const WI_NOEXCEPT { return static_cast<std::size_t>(m_ptr - m_buf); }

        void reset() WI_NOEXCEPT { m_ptr = m_buf; }

        template<std::size_t ReqAlign>
        __declspec(allocator) void* allocate(::std::size_t num_bytes)
        {
            static_assert(ReqAlign <= alignment, "alignment is too large for this arena");
            ASSERT(pointer_in_buffer(m_ptr), L"allocator outlived arena");
            auto const aligned_n = align_up(num_bytes);

            if (m_ptr + aligned_n <= m_buf + N)
            {
                auto result = m_ptr;
                m_ptr += aligned_n;
                return result;
            }
            // Doesn't fit in the arena, fall back to heap allocation
            return ::operator new(num_bytes);
        }

        void deallocate(void* p, ::std::size_t num_bytes) WI_NOEXCEPT
        {
            ASSERT(pointer_in_buffer(m_ptr), L"stack_allocator outlived arena");
            auto pc = static_cast<char*>(p);
            if (pointer_in_buffer(pc))
            {
                // If deallocating the most recent allocation, rollback the pointer - still contiguous
                auto aligned_n = align_up(num_bytes);
                if (pc + aligned_n == m_ptr)
                {
                    m_ptr = pc;
                }
            }
            else
            {
                ::operator delete(p);
            }
        }
    };

    template<typename T, std::size_t N, std::size_t Align = alignof(std::max_align_t)>
    class stack_allocator
    {
    public:
        using value_type = T;
        static auto constexpr alignment = Align;
        using arena_type = arena<N, alignment>;

    private:
        arena_type& arena_ref;

    public:
        // Allow copy-construct, but not assignment because, you know, references
        stack_allocator(const stack_allocator&) = default;
        stack_allocator& operator=(const stack_allocator&) = delete;

        explicit stack_allocator(arena<N>& a) WI_NOEXCEPT
            : arena_ref(a) {}

        template<class U>
        explicit stack_allocator(const stack_allocator<U, N, alignment>& a) WI_NOEXCEPT
            : arena_ref(a.arena_ref) {}

        template<typename T2>
            struct rebind { using other =  stack_allocator<T2, N, alignment>; };

        __declspec(allocator) T* allocate(std::size_t count)
        {
            return static_cast<T*>(arena_ref.template allocate<alignof(T)>(count * sizeof(T)));
        }
        void deallocate(T* p, std::size_t count) WI_NOEXCEPT
        {
            arena_ref.deallocate(p, count * sizeof(T));
        }

        template<typename T2, std::size_t N2, std::size_t Align2>
        bool operator==(const stack_allocator<T2, N2, Align2>& other) const WI_NOEXCEPT
        {
            return N == N2 && Align == Align2 && (&arena_ref == &other.arena_ref);
        }

        template<typename T2, std::size_t N2, std::size_t Align2> friend class stack_allocator;
    };

    template<typename T, std::size_t N, std::size_t Align, typename T2, std::size_t N2, std::size_t Align2>
    inline bool operator!=(const stack_allocator<T, N, Align>& lhs, const stack_allocator<T2, N2, Align2>& rhs)
    {
        return !(lhs == rhs);
    }
}