// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <vector>
#include <functional> // For std::less
#include <memory>     // For std::allocator
#include "vector_tree.h"

namespace containers
{
    namespace detail
    {
        template <typename Key, typename Pred, typename Alloc, bool multi>
        struct set_traits
        {
            typedef Key   key_type;
            typedef Key   value_type;
            typedef Pred  key_compare;
            typedef Alloc allocator_type;
            static const Key& get_key(const value_type& arg) { return arg; }
        };
    }

    // Replacement for std::set that uses a sorted vector for its underlying storage
    // Most of the implementation lives in vector_tree
    template <
        typename Key,
        typename Pred = ::std::less<>, // Predicate used to compare keys. This is a new container, so we can default to transparent less without perf risk.
        typename Alloc = ::std::allocator < Key > >
    class vector_set :
        public detail::vector_tree <detail::set_traits<Key, Pred, Alloc, false>>
    {
        using base_type = detail::vector_tree<detail::set_traits<Key, Pred, Alloc, false>>;

    public:
        using key_type = typename base_type::key_type;
        using value_type = typename base_type::value_type;
        using key_compare = typename base_type::key_compare;
        using allocator_type = typename base_type::allocator_type;
        using iterator = typename base_type::iterator;

#pragma region constructors
        explicit vector_set(const key_compare& pred, const allocator_type& alloc = allocator_type())
            : base_type(pred, alloc)
        {}

        vector_set() : vector_set(key_compare()) {} // delegating

        explicit vector_set(const allocator_type& alloc)
            : base_type(key_compare(), alloc)
        {}

        template<typename InputIt>
        vector_set(InputIt first, InputIt last)
            : base_type(first, last, key_compare(), allocator_type())
        {}

        template<typename InputIt>
        vector_set(InputIt first, InputIt last, const key_compare& pred, const allocator_type& alloc = allocator_type())
            : base_type(first, last, pred, alloc)
        {}

        template<typename InputIt>
        vector_set(InputIt first, InputIt last, const allocator_type& alloc)
            : base_type(first, last, key_compare(), alloc)
        {}

        vector_set(const vector_set& other) = default;

        vector_set(const vector_set& other, const allocator_type& alloc)
            : base_type(other, alloc)
        {}

        vector_set(vector_set&& other) = default;

        vector_set(vector_set&& other, const allocator_type& alloc)
            : base_type(::std::move(other), alloc)
        {}
#pragma endregion

#pragma region assignment
        vector_set& operator=(const vector_set&) = default;

        vector_set& operator=(vector_set&&) = default;
#pragma endregion

#pragma region modifiers
        void swap(vector_set& other)
        {
            if (this != &other)
            {
                base_type::swap(other);
            }
        }
#pragma endregion
    };

#pragma region nonmember operators
    template <typename Key, typename Pred, typename Alloc>
    void swap(vector_set<Key, Pred, Alloc>& lhs,
        vector_set<Key, Pred, Alloc>& rhs)
    {
        lhs.swap(rhs);
    }
#pragma endregion
}