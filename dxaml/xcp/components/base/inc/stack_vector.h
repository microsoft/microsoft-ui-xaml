// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// This is a wrapper to make it easier to wrap a vector around a stack_allocator, all in one simple construct.
#pragma once

#include <vector>
#include "stack_allocator.h"

namespace Jupiter {
    // stack_vector is a struct containing a vector and a stack_allocator's arena bundled together
    // This adds no functionality other than being able to declare a stack_allocator, its arena, and a vector all in one line
    template<typename T, ::std::size_t N>
    struct stack_vector
    {
        static const ::std::size_t arena_size = N * sizeof(T);
        using arena_t = arena<arena_size>;
        using alloc_t = stack_allocator<T, arena_size>;
        using vector_t = ::std::vector<T, alloc_t>;

        arena_t m_arena;
        vector_t m_vector;

        stack_vector() WI_NOEXCEPT
            : m_arena()
            , m_vector(alloc_t(m_arena))
        {
            m_vector.reserve(N);
        }
    };
}