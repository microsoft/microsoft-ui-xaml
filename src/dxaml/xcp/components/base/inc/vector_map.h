// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <vector>
#include <functional> // For std::less
#include <memory>     // For std::allocator
#include <utility>    // For std::pair
#include "vector_tree.h"

namespace containers
{
    namespace detail
    {
        template <typename Key,
            typename T,
            typename Pred,
            typename Alloc,
            bool multi>
        struct map_traits
        {
            typedef Key                 key_type;
            typedef ::std::pair<Key, T> value_type;
            typedef Pred                key_compare;
            typedef Alloc               allocator_type;
            static const Key& get_key(const value_type& arg) { return arg.first; }
        };
    }

    template <typename Key,  // Key type used for lookup
        typename T,          // Mapped type
        typename Pred = ::std::less<>, // Predicate used to compare keys. This is a new container, so we can default to transparent less without perf risk.
        typename Alloc = ::std::allocator<::std::pair<Key, T>>> // Allocator used in the vector
    class vector_map :
        public detail::vector_tree <detail::map_traits<Key, T, Pred, Alloc, false>>
    {
        using base_type = detail::vector_tree<detail::map_traits<Key, T, Pred, Alloc, false>>;

    public:
#pragma region public typedefs

        using mapped_type = T;
        using key_type = typename base_type::key_type;
        using value_type = typename base_type::value_type;
        using key_compare = typename base_type::key_compare;
        using allocator_type = typename base_type::allocator_type;
        using iterator = typename base_type::iterator;

#pragma endregion

#pragma region constructors
        explicit vector_map(const key_compare& pred, const allocator_type& alloc = allocator_type())
            : base_type(pred, alloc)
        {}

        vector_map() : vector_map(key_compare()) {} // delegating

        explicit vector_map(const allocator_type& alloc)
            : base_type(key_compare(), alloc)
        {}

        template<typename InputIt>
        vector_map(InputIt first, InputIt last)
            : base_type(first, last, key_compare(), allocator_type())
        {}

        template<typename InputIt>
        vector_map(InputIt first, InputIt last, const key_compare& pred, const allocator_type& alloc = allocator_type())
            : base_type(first, last, pred, alloc)
        {}

        template<typename InputIt>
        vector_map(InputIt first, InputIt last, const allocator_type& alloc)
            : base_type(first, last, key_compare(), alloc)
        {}

        vector_map(const vector_map&) = default;

        vector_map(const vector_map& other, const allocator_type& alloc)
            : base_type(other, alloc)
        {}

        vector_map(vector_map&&) = default;

        vector_map(vector_map&& other, const allocator_type& alloc)
            : base_type(::std::move(other), alloc)
        {}

#pragma endregion

#pragma region assignment
        vector_map& operator=(const vector_map&) = default;

        vector_map& operator=(vector_map&& other) = default;
#pragma endregion

#pragma region element access

        // If pair matching 'key' exists, return a reference to it
        // Otherwise, emplace a new pair with a default-constructed value and return a reference to that
        mapped_type& operator[](const key_type& key)
        {
            iterator pos = this->lower_bound(key);

            if (pos == this->end() || this->key_comp()(key, this->get_key(*pos)))
            {
                pos = this->internal_emplace(pos, ::std::piecewise_construct, ::std::forward_as_tuple(key), std::tuple<>());
            }
            return (*pos).second;
        }

        mapped_type& operator[](key_type&& key)
        {
            iterator pos = this->lower_bound(key);

            if (pos == this->end() || this->key_comp()(key, this->get_key(*pos)))
            {
                pos = this->internal_emplace(pos, ::std::piecewise_construct, ::std::forward_as_tuple(std::move(key)), std::tuple<>());
            }
            return (*pos).second;
        }
#pragma endregion

#pragma region modifiers
        void swap(vector_map& other)
        {
            if (this != &other)
            {
                base_type::swap(other);
            }
        }
#pragma endregion
    };

#pragma region nonmember operators
    template <typename Key, typename T, typename Pred, typename Alloc>
    void swap(vector_map<Key, T, Pred, Alloc>& lhs,
        vector_map<Key, T, Pred, Alloc>& rhs)
    {
        lhs.swap(rhs);
    }
#pragma endregion
}