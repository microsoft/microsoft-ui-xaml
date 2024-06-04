// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

/////////////////////////////////////
// vector_tree is the base class for vector_map and vector_set
// These are implementations of associative containers like std::map and std::set
// They provide log(n) lookup, but benefit from more efficient memory consumption
// and improved cache locality, because the items are stored in a sorted vector
// rather than a binary tree or linked list
/////////////////////////////////////

#include <vector>
#include <utility>
#include <type_traits>
#include <algorithm>
#include <iterator>

namespace containers { namespace detail {

    // Utility class to hold predicate with storage
    template<bool Pred_has_storage, typename tree_traits>
    class tree_comparator
    {
    public:
        typedef typename tree_traits::key_compare key_compare;
        typedef typename tree_traits::value_type  value_type;

#pragma region constructors
        explicit tree_comparator(const key_compare& key_comp)
            : m_key_comp(key_comp)
        {}

        explicit tree_comparator(key_compare&& key_comp)
            : m_key_comp(::std::forward<key_compare>(key_comp))
        {}

        tree_comparator(const tree_comparator&) = default;
        tree_comparator& operator=(const tree_comparator&) = default;

        tree_comparator(tree_comparator&&) = default;
        tree_comparator& operator=(tree_comparator&&) = default;
#pragma endregion

        key_compare get_key_compare() const
        {
            return m_key_comp;
        }

        bool operator()(const value_type& lhs, const value_type& rhs) const
        {
            return m_key_comp(tree_traits::get_key(lhs), tree_traits::get_key(rhs));
        }

    private:
        key_compare m_key_comp;
    };

    // Specialization to hold predicate with no storage
    template<typename tree_traits>
    class tree_comparator < false, tree_traits>
    {
    public:
        typedef typename tree_traits::key_compare key_compare;
        typedef typename tree_traits::value_type  value_type;

        explicit tree_comparator(const key_compare&) {}
        tree_comparator(const tree_comparator&) = default;
        tree_comparator& operator=(const tree_comparator&) = default;

        key_compare get_key_compare() const
        {
            return key_compare();
        }

        bool operator()(const value_type& lhs, const value_type& rhs) const
        {
            return key_compare()(tree_traits::get_key(lhs), tree_traits::get_key(rhs));
        }
    };

    // Sorted vector implementation for vector_map and vector_set
    template <typename tree_traits>
    class vector_tree
    {
    public:
#pragma region public typedefs
        typedef ::std::vector<typename tree_traits::value_type, typename tree_traits::allocator_type> storage_type;
        // These typedefs are largely defined in terms of the underlying vector implementation
        typedef typename tree_traits::key_type                key_type;
        typedef typename storage_type::value_type             value_type;
        typedef typename storage_type::size_type              size_type;
        typedef typename storage_type::difference_type        difference_type;
        typedef typename storage_type::allocator_type         allocator_type;
        typedef typename storage_type::reference              reference;
        typedef typename storage_type::const_reference        const_reference;
        typedef typename storage_type::pointer                pointer;
        typedef typename storage_type::const_pointer          const_pointer;
        typedef typename storage_type::iterator               iterator;
        typedef typename storage_type::const_iterator         const_iterator;
        typedef typename storage_type::reverse_iterator       reverse_iterator;
        typedef typename storage_type::const_reverse_iterator const_reverse_iterator;

        // Comparators
        typedef typename tree_traits::key_compare key_compare;
        typedef tree_comparator<!::std::is_empty<key_compare>::value, tree_traits> value_compare;

#pragma endregion

#pragma region constructors
        // Constructors and assignment are all protected
    protected:
        vector_tree(const key_compare& pred, const allocator_type& alloc)
            : m_data(pred, alloc)
        {}

        template<typename InputIt>
        vector_tree(
            InputIt first,
            InputIt last,
            const key_compare& key_comp,
            const allocator_type& alloc
            )
            : m_data(key_comp, alloc)
        {
            insert(first, last);
        }

        vector_tree(const vector_tree& other) = default;

        vector_tree(const vector_tree& other, const allocator_type& alloc)
            : m_data(other.m_data, alloc)
        {}

        vector_tree(vector_tree&& other) = default;

        vector_tree(vector_tree&& other, const allocator_type& alloc)
            : m_data(::std::move(other.m_data), alloc)
        {}

#pragma endregion

#pragma region assignment
        vector_tree& operator=(const vector_tree& other) = default;

        vector_tree& operator=(vector_tree&& other) = default;
#pragma endregion
    public:
        allocator_type get_allocator() const WI_NOEXCEPT
        {
            return m_data.m_vector.get_allocator();
        }

#pragma region observers
        key_compare key_comp() const
        {
            return m_data.get_key_compare();
        }

        value_compare value_comp() const
        {
            return m_data;
        }
#pragma endregion

#pragma region iterators
        iterator begin() WI_NOEXCEPT { return m_data.m_vector.begin(); }
        const_iterator begin() const WI_NOEXCEPT { return m_data.m_vector.begin(); }
        const_iterator cbegin() const WI_NOEXCEPT { return m_data.m_vector.cbegin(); }

        iterator end() WI_NOEXCEPT { return m_data.m_vector.end(); }
        const_iterator end() const WI_NOEXCEPT { return m_data.m_vector.end(); }
        const_iterator cend() const WI_NOEXCEPT { return m_data.m_vector.cend(); }

        reverse_iterator rbegin() WI_NOEXCEPT { return m_data.m_vector.rbegin(); }
        const_reverse_iterator rbegin() const WI_NOEXCEPT { return m_data.m_vector.rbegin(); }
        const_reverse_iterator crbegin() const WI_NOEXCEPT { return m_data.m_vector.crbegin(); }

        reverse_iterator rend() WI_NOEXCEPT { return m_data.m_vector.rend(); }
        const_reverse_iterator rend() const WI_NOEXCEPT { return m_data.m_vector.rend(); }
        const_reverse_iterator crend() const WI_NOEXCEPT { return m_data.m_vector.crend(); }
#pragma endregion

#pragma region vector capacity
        bool empty() const WI_NOEXCEPT { return m_data.m_vector.empty(); }

        size_type size() const WI_NOEXCEPT { return m_data.m_vector.size(); }

        size_type max_size() const WI_NOEXCEPT { return m_data.m_vector.max_size(); }

        void reserve(size_type new_capacity) { m_data.m_vector.reserve(new_capacity); }

        size_type capacity() const WI_NOEXCEPT { return m_data.m_vector.capacity(); }

        void shrink_to_fit() { m_data.m_vector.shrink_to_fit(); }
#pragma endregion

#pragma region modifiers
    protected:
        void swap(vector_tree& other)
        {
            if (this != &other)
            {
                m_data.swap(other.m_data);
            }
        }

    public:
        void clear() WI_NOEXCEPT
        {
            m_data.m_vector.clear();
        }

        ::std::pair<iterator, bool> insert(const value_type& value)
        {
            return internal_insert_nohint(value);
        }

        // Insert something that is convertible to value_type
        template <typename Val>
        typename ::std::enable_if<::std::is_convertible<Val, value_type>::value,
            ::std::pair<iterator, bool>>::type
        insert(Val&& value)
        {
            return internal_insert_nohint(::std::forward<Val>(value));
        }

        iterator insert(const_iterator hint, const value_type& value)
        {
            return internal_insert_hint(hint, value);
        }

        // Insert, with hint, something that is convertible to value_type
        template <typename Val>
        typename ::std::enable_if<::std::is_convertible<Val, value_type>::value,
            iterator>::type
        insert(const_iterator hint, Val&& value)
        {
            return internal_insert_hint(hint, ::std::forward<Val>(value));
        }

        template <typename InputIt>
        void insert(InputIt first, InputIt last)
        {
            internal_insert_range(first, last, std::iterator_traits<InputIt>::iterator_category());
        }

        template <typename... Args>
        ::std::pair<iterator, bool> emplace(Args&&... args)
        {
            // Optimistically assume the insertion is valid, and take care of all the allocation
            // and construction work up front. Rolling back is cheap.
            m_data.m_vector.emplace_back(::std::forward<Args>(args)...);

            // Figure out if, and where this element can really be inserted
            const auto elem_it = end() - 1;
            auto info = internal_get_insertion_pos(begin(), elem_it, m_data.m_vector.back());
            ::std::pair<iterator, bool> result;
            result.second = info.allowed;
            result.first = extract_iterator(info.pos);

            if (info.allowed) {
                // Rotate the element into place
                ::std::rotate(result.first, elem_it, end());
            }
            else {
                // Revert the emplace_back and return a false
                ASSERT(result.first != elem_it);
                ASSERT(result.first != end());
                m_data.m_vector.pop_back();
            }
            return result;
        }

        template <typename... Args>
        iterator emplace_hint(const_iterator hint, Args&&... args)
        {
            // Inserting may invalidate the original iterator, so "save" it off
            const auto offset = hint - begin();

            // Optimistically assume the insertion is valid, and take care of all the allocation
            // and construction work up front. Rolling back is cheap.
            m_data.m_vector.emplace_back(::std::forward<Args>(args)...);

            const auto newHint = begin() + offset;

            // Figure out if, and where this element can really be inserted
            const auto elem_it = end() - 1;
            auto info = internal_get_insertion_pos(begin(), newHint, elem_it, m_data.m_vector.back());
            iterator result = extract_iterator(info.pos);

            if (info.allowed) {
                // Rotate the element into place
                ::std::rotate(result, elem_it, end());
            }
            else {
                // Revert the emplace_back and return a false
                ASSERT(result != elem_it);
                ASSERT(result != end());
                m_data.m_vector.pop_back();
            }
            return result;
        }

        iterator erase(const_iterator pos)
        {
            return m_data.m_vector.erase(pos);
        }

        iterator erase(const_iterator first, const_iterator last)
        {
            return m_data.m_vector.erase(first, last);
        }

        size_type erase(const key_type& key)
        {
            auto iterPair = this->equal_range(key);
            auto num = iterPair.second - iterPair.first;
            if (num > 0) {
                m_data.m_vector.erase(iterPair.first, iterPair.second);
            }
            return num;
        }
#pragma endregion

#pragma region lookup
        size_type count(const key_type& key) const
        {
            auto iterPair = equal_range(key);
            return ::std::distance(iterPair.first, iterPair.second);
        }

        template<class KeyLike,
            class MyComp = key_compare,
            class = typename MyComp::is_transparent>
        size_type count(const KeyLike& key) const
        {
            auto iterPair = equal_range(key);
            return ::std::distance(iterPair.first, iterPair.second);
        }

        iterator find(const key_type& key)
        {
            iterator pos = lower_bound(key);
            if (pos == end() || key_comp()(key, get_key(*pos))) {
                return end();
            }
            else {
                return pos;
            }
        }

        const_iterator find(const key_type& key) const
        {
            const_iterator pos = lower_bound(key);
            if (pos == end() || key_comp()(key, get_key(*pos))) {
                return end();
            }
            else {
                return pos;
            }
        }

        template<class KeyLike,
            class MyComp = key_compare,
            class = typename MyComp::is_transparent>
        iterator find(const KeyLike& key)
        {
            iterator pos = lower_bound(key);
            if (pos == end() || key_comp()(key, get_key(*pos))) {
                return end();
            }
            else {
                return pos;
            }
        }

        template<class KeyLike,
            class MyComp = key_compare,
            class = typename MyComp::is_transparent>
        const_iterator find(const KeyLike& key) const
        {
            const_iterator pos = lower_bound(key);
            if (pos == end() || key_comp()(key, get_key(*pos))) {
                return end();
            }
            else {
                return pos;
            }
        }

        ::std::pair<iterator, iterator> equal_range(const key_type& key)
        {
            return internal_equal_range(begin(), end(), key);
        }

        ::std::pair<const_iterator, const_iterator> equal_range(const key_type& key) const
        {
            return internal_equal_range(begin(), end(), key);
        }

        template<class KeyLike,
            class MyComp = key_compare,
            class = typename MyComp::is_transparent>
        ::std::pair<iterator, iterator> equal_range(const KeyLike& key)
        {
            return internal_equal_range(begin(), end(), key);
        }

        template<class KeyLike,
            class MyComp = key_compare,
            class = typename MyComp::is_transparent>
        ::std::pair<const_iterator, const_iterator> equal_range(const KeyLike& key) const
        {
            return internal_equal_range(begin(), end(), key);
        }

        iterator lower_bound(const key_type& key)
        {
            return internal_lower_bound(begin(), end(), key);
        }

        const_iterator lower_bound(const key_type& key) const
        {
            return internal_lower_bound(begin(), end(), key);
        }

        template<class KeyLike,
            class MyComp = key_compare,
            class = typename MyComp::is_transparent>
        iterator lower_bound(const KeyLike& key)
        {
            return internal_lower_bound(begin(), end(), key);
        }

        template<class KeyLike,
            class MyComp = key_compare,
            class = typename MyComp::is_transparent>
        const_iterator lower_bound(const KeyLike& key) const
        {
            return internal_lower_bound(begin(), end(), key);
        }

        iterator upper_bound(const key_type& key)
        {
            return internal_upper_bound(begin(), end(), key);
        }

        const_iterator upper_bound(const key_type& key) const
        {
            return internal_upper_bound(begin(), end(), key);
        }

        template<class KeyLike,
            class MyComp = key_compare,
            class = typename MyComp::is_transparent>
        iterator upper_bound(const KeyLike& key)
        {
            return internal_upper_bound(begin(), end(), key);
        }

        template<class KeyLike,
            class MyComp = key_compare,
            class = typename MyComp::is_transparent>
        const_iterator upper_bound(const KeyLike& key) const
        {
            return internal_upper_bound(begin(), end(), key);
        }
#pragma endregion

#pragma region helpers
    protected:
        static const key_type& get_key(const value_type& value)
        {
            return tree_traits::get_key(value);
        }

        template <class... Args>
        iterator internal_emplace(iterator pos, Args&&... args)
        {
            return m_data.m_vector.emplace(pos, ::std::forward<Args>(args)...);
        }

        // Writing my own versions of lower_bound, upper_bound, and equal_range
        // because the versions of <algorithm> expect lookup value and the range to be of the same types.
        // With maps, we're comparing a key against a pair, and trying to reuse <algorithm> makes
        // things a lot more complex/ugly
        template <typename Iter, typename KeyLike>
        Iter internal_lower_bound(Iter first, Iter last, const KeyLike& key) const
        {
            difference_type length = last - first;
            difference_type half;
            Iter middle;
            while (length > 0) {
                half = length >> 1;
                middle = first;
                middle += half;

                if (key_comp()(get_key(*middle), key)) {
                    ++middle;
                    first = middle;
                    length = length - half - 1;
                }
                else {
                    length = half;
                }
            }
            return first;
        }

        template <typename Iter, typename KeyLike>
        Iter internal_upper_bound(Iter first, Iter last, const KeyLike& key) const
        {
            difference_type length = last - first;
            difference_type half;
            Iter middle;

            while (length > 0) {
                half = length >> 1;
                middle = first;
                middle += half;

                if (key_comp()(key, get_key(*middle))) {
                    length = half;
                }
                else {
                    ++middle;
                    first = middle;
                    length = length - half - 1;
                }
            }
            return first;
        }

        template <typename Iter, typename KeyLike>
        ::std::pair<Iter, Iter> internal_equal_range(Iter first, Iter last, const KeyLike& key) const
        {
            difference_type length = last - first;
            difference_type half;
            Iter left, middle, right;

            while (length > 0) {
                half = length >> 1;
                middle = first;
                middle += half;

                if (key_comp()(get_key(*middle), key)) {
                    first = middle;
                    ++first;
                    length = length - half - 1;
                }
                else if (key_comp()(key, get_key(*middle))) {
                    length = half;
                }
                else {
                    left = internal_lower_bound(first, middle, key);
                    first += length;
                    right = internal_upper_bound(++middle, first, key);
                    return ::std::pair<Iter, Iter>(left, right);
                }
            }
            return ::std::pair<Iter, Iter>(first, first);
        }

        template<typename Val>
        ::std::pair<iterator, bool> internal_insert_nohint(Val&& value)
        {
            auto info = internal_get_insertion_pos(begin(), end(), value);
            ::std::pair<iterator, bool> result;
            result.second = info.allowed;
            if (info.allowed) {
                result.first = m_data.m_vector.emplace(info.pos, ::std::forward<Val>(value));
            }
            else {
                result.first = extract_iterator(info.pos);
            }
            return result;
        }

        template<typename Val>
        iterator internal_insert_hint(const_iterator pos, Val&& value)
        {
            // Hint may not be accurate, so we need to see if it's correct, or at least close
            auto info = internal_get_insertion_pos(begin(), pos, end(), value);
            if (info.allowed) {
                return m_data.m_vector.emplace(info.pos, ::std::forward<Val>(value));
            }
            else {
                return extract_iterator(info.pos);
            }
        }

        struct insertion_info
        {
            const_iterator pos;
            bool allowed;
        };

        // insertion_info carries a const_iterator, but
        // some non-const methods need to return a non-const iterator
        iterator extract_iterator(const_iterator pos)
        {
            return begin() + (pos - cbegin());
        }

        insertion_info internal_get_insertion_pos(const value_type& val) const
        {
            return internal_get_insertion_pos(cbegin(), cend(), val);
        }

        insertion_info internal_get_insertion_pos(const_iterator begin_it, const_iterator end_it, const value_type& value) const
        {
            // This assumes uniqueness. Change this up if we add multimap/multiset
            insertion_info result;
            result.pos = internal_lower_bound(begin_it, end_it, get_key(value));
            result.allowed = (result.pos == end_it) || value_comp()(value, *result.pos);
            return result;
        }

        insertion_info internal_get_insertion_pos(const_iterator begin_it, const_iterator hint, const_iterator end_it, const value_type& value) const
        {
            // N1780 insert as close to hint as possible
            // if pos == end || value <= *pos
            //     if pos == begin || value >= *(pos-1)
            //         insert value before pos === #1 ===
            //     else
            //         insert value before upper_bound(value) === #2 ===
            // else
            //     insert value before lower_bound(value) === #3 ===

            const auto& val_comp = value_comp();
            if (hint == end_it || val_comp(value, *hint)) {
                if (hint == begin_it) {
                    // === #1a ===
                    // If container is empty, or we belong in front of the first element, insert here
                    return insertion_info{ hint, true };
                }
                const auto prev = hint - 1;
                if (val_comp(*prev, value)) {
                    // === #1b ===
                    // If previous element was less, then we're either at the end, or sandwiched between two elements in order.
                    // insert between prev and pos (right before pos)
                    return insertion_info{ hint, true };
                }
                else if (!val_comp(value, *prev)) {
                    // === #2a ===
                    // If 1b failed, then value <= *prev. If we get here, value == prev.
                    // For unique values, we can prevent insertion immediately
                    return insertion_info{ prev, false };
                }
                else {
                    // === #2b ===
                    // value < prev, so the hint was useless and we could go just about anywhere before prev
                    // Go for hintless insertion, but search between begin and prev
                    return internal_get_insertion_pos(begin_it, prev, value);
                }
            }
            else {
                // pos is not the end, and *pos <= value
                // The hint is too early, and we can actually insert anywhere between it and the end
                return internal_get_insertion_pos(hint, end_it, value);
            }
        }

        template <typename InputIt>
        void internal_insert_range(InputIt first, InputIt last, ::std::input_iterator_tag)
        {
            // Can't query the distance of input iterators, so just insert and let the reallocations occur
            for (; first != last; ++first) {
                // There's a decent chance the caller of this API is giving us a pre-sorted range
                // Let's try to provide a hint for that case
                internal_insert_hint(cend(), *first);
            }
        }

        template <typename InputIt>
        void internal_insert_range(InputIt first, InputIt last, ::std::forward_iterator_tag)
        {
            // With a forward iterator, we can ask for the distance and preallocate to save work
            auto length = ::std::distance(first, last);
            reserve(size() + length);
            for (; first != last; ++first) {
                // There's a decent chance the caller of this API is giving us a pre-sorted range
                // Let's try to provide a hint for that case
                internal_insert_hint(cend(), *first);
            }
        }
#pragma endregion

    private:
        // Bundle up the vector and the comparator to trigger the Empty Base Class Optimization
        // for when the comparator is empty
        struct CompressedData : public value_compare
        {
            storage_type m_vector;

            CompressedData() = default;
            CompressedData(const CompressedData&) = default;
            CompressedData(CompressedData&&) = default;
            CompressedData& operator=(const CompressedData&) = default;
            CompressedData& operator=(CompressedData&&) = default;

            CompressedData(const CompressedData& d, const allocator_type& alloc)
                : value_compare(static_cast<const value_compare&>(d)), m_vector(d.m_vector, alloc)
            {}

            CompressedData(CompressedData&& d, const allocator_type& alloc)
                : value_compare(::std::move(static_cast<value_compare&>(d))), m_vector(::std::move(d.m_vector), alloc)
            {}

            explicit CompressedData(const key_compare& pred)
                : value_compare(pred)
            {}

            CompressedData(const key_compare& pred, const allocator_type& alloc)
                : value_compare(pred), m_vector(alloc)
            {}

            explicit CompressedData(const allocator_type& alloc)
                : value_compare(), m_vector(alloc)
            {}

            void swap(CompressedData& d)
            {
                value_compare& myPred = *this;
                value_compare& otherPred = d;
                ::std::swap(myPred, otherPred);
                m_vector.swap(d.m_vector);
            }
        };
        CompressedData m_data;
    };

    template <typename tree_traits>
    bool operator==(
        const vector_tree<tree_traits>& lhs,
        const vector_tree<tree_traits>& rhs)
    {
        return lhs.size() == rhs.size() &&
            ::std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }

    template <typename tree_traits>
    bool operator!=(
        const vector_tree<tree_traits>& lhs,
        const vector_tree<tree_traits>& rhs)
    {
        return !(lhs == rhs);
    }

    template <typename tree_traits>
    bool operator<(
        const vector_tree<tree_traits>& lhs,
        const vector_tree<tree_traits>& rhs)
    {
        return ::std::lexicographical_compare(
            lhs.begin(),
            lhs.end(),
            rhs.begin(),
            rhs.end());
    }

    template <typename tree_traits>
    bool operator<=(
        const vector_tree<tree_traits>& lhs,
        const vector_tree<tree_traits>& rhs)
    {
        return !(lhs < rhs);
    }

    template <typename tree_traits>
    bool operator>(
        const vector_tree<tree_traits>& lhs,
        const vector_tree<tree_traits>& rhs)
    {
        return rhs < lhs;
    }

    template <typename tree_traits>
    bool operator>=(
        const vector_tree<tree_traits>& lhs,
        const vector_tree<tree_traits>& rhs)
    {
        return !(lhs < rhs);
    }
} }