// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

/////////////////////////////////////
// indexed_list is a sequence container like std::vector, for long lists that change in the middle.
//
// The items are stored in blocks of up to BlockCapacity items, and a directory keeps the position of the first item
// of each block. Access by position costs O(log(number of blocks)), and O(1) for each step of a walk in order.
// Insert and erase at a position cost O(BlockCapacity + number of blocks), where std::vector costs O(size).
//
// With BlockCapacity 4 and the items A to J:
//
//     m_blockStart:   0          4         7
//     m_blocks:      [A B C D]  [E F G]   [H I J]
//
// Item 5 is in the block that starts at 4, at offset 1: F.
//
// With a KeyOf function object, the list also keeps an index from each item's key to the block that holds the item,
// and find_first(key) returns the position of the first item with that key without searching the whole list. Keys may
// repeat, as items may repeat in any list. The index only makes lookups faster; it never limits what the list holds.
// Each change to an item walks the index entries for its key, so it is fast only while few items share a key.
//
// Rules for callers:
// - Items must not change their key while they are in the list, except through replace().
// - A key must not point into its item, like a string_view of a string member: items move between blocks.
// - Not thread safe, not even for reads: lookups update a mutable hint.
// - insert, erase, push_back, pop_back and clear can move any item, so a reference or pointer from operator[], back()
//   or for_each is valid only until the next of these calls. replace() moves nothing: a reference to the replaced
//   position stays valid and refers to the new item.
// - A position out of range is a programming error and fails fast.
// - insert() and replace() leave the list unchanged if an allocation fails. erase() never allocates or throws.
/////////////////////////////////////

#include <algorithm>
#include <cstdint>
#include <functional>
#include <intrin.h>
#include <memory>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace containers {

namespace detail {

    struct indexed_list_no_key {};

    template <typename T, typename KeyOf>
    struct indexed_list_key_traits
    {
        static constexpr bool has_key = true;
        using key_type = ::std::decay_t<decltype(::std::declval<const KeyOf&>()(::std::declval<const T&>()))>;
    };

    template <typename T>
    struct indexed_list_key_traits<T, void>
    {
        static constexpr bool has_key = false;
        using key_type = indexed_list_no_key;
    };

} // namespace detail

template <typename T, typename KeyOf = void, ::std::uint32_t BlockCapacity = 64>
class indexed_list
{
    using key_traits = detail::indexed_list_key_traits<T, KeyOf>;
    static constexpr bool c_hasKey = key_traits::has_key;
    static constexpr ::std::uint32_t c_halfCapacity = BlockCapacity / 2;

    static_assert(BlockCapacity >= 2 && BlockCapacity % 2 == 0, "BlockCapacity must be even and at least 2");
    static_assert(
        ::std::is_nothrow_move_constructible_v<T> && ::std::is_nothrow_move_assignable_v<T> && ::std::is_nothrow_destructible_v<T>,
        "Items must move and destroy without throwing, so that a change is never left half done");

public:
    using value_type = T;
    using key_type = typename key_traits::key_type;

    static_assert(
        ::std::disjunction_v<
            ::std::bool_constant<!c_hasKey>,
            ::std::conjunction<
                ::std::is_nothrow_default_constructible<KeyOf>,
                ::std::is_nothrow_destructible<KeyOf>,
                ::std::is_nothrow_invocable<KeyOf, const T&>,
                ::std::is_nothrow_invocable<::std::hash<key_type>, const key_type&>,
                ::std::is_nothrow_invocable_r<bool, ::std::equal_to<key_type>, const key_type&, const key_type&>>>,
        "Creating KeyOf, calling it, and hashing and comparing its keys must not throw: erase and the end of insert cannot fail");

    indexed_list() = default;
    indexed_list(const indexed_list&) = delete;
    indexed_list& operator=(const indexed_list&) = delete;
    indexed_list(indexed_list&&) = delete;
    indexed_list& operator=(indexed_list&&) = delete;

    // Like clear(): the list is already empty when the items are destroyed.
    ~indexed_list() { clear(); }

    ::std::uint32_t size() const noexcept { return m_size; }
    bool empty() const noexcept { return m_size == 0; }
    ::std::uint32_t block_count() const noexcept { return static_cast<::std::uint32_t>(m_blocks.size()); }

    T& operator[](::std::uint32_t index) noexcept { return item_at(index); }
    const T& operator[](::std::uint32_t index) const noexcept { return item_at(index); }

    T& back() noexcept { return item_at(last_index()); }
    const T& back() const noexcept { return item_at(last_index()); }

    // Inserts the item before position index. index == size() appends.
    void insert(::std::uint32_t index, T item)
    {
        if (index > m_size || m_size == UINT32_MAX)
        {
            fail_fast();
        }

        // Find the block and offset where the item goes.
        ::std::uint32_t blockIndex = 0;
        ::std::uint32_t offset = 0;
        if (!m_blocks.empty())
        {
            if (index == m_size)
            {
                blockIndex = block_count() - 1;
                offset = block_size(blockIndex);
            }
            else
            {
                blockIndex = find_block(index);
                offset = index - m_blockStart[blockIndex];
            }

            // An item inserted before the first item of a block goes to the end of the previous block if it has room.
            // So items inserted one after another, as TreeView does when it expands a node, fill blocks completely.
            if (offset == 0 && blockIndex > 0 && block_size(blockIndex - 1) < BlockCapacity)
            {
                --blockIndex;
                offset = block_size(blockIndex);
            }
        }

        // Prepare everything that can allocate, so that a failure leaves the list unchanged:
        // - An empty list gets its first block, which holds the item.
        // - A full block is split at the insert position. The items from that position on move to a new block after it,
        //   and the item goes to the end of the first part. So the next item inserted after it also goes there, and
        //   items inserted one after another fill blocks completely. At the end of the list nothing moves, and the new
        //   block holds just the item.
        const bool firstBlock = m_blocks.empty();
        const bool split = !firstBlock && block_size(blockIndex) == BlockCapacity;
        const bool intoNewBlock = firstBlock || (split && offset == BlockCapacity);
        ::std::unique_ptr<block> newBlock;
        if (firstBlock || split)
        {
            newBlock = make_block();
            reserve_one_more(m_blocks);
            reserve_one_more(m_blockStart);
        }

        block* target = intoNewBlock ? newBlock.get() : m_blocks[blockIndex].get();
        const ::std::uint32_t targetOffset = intoNewBlock ? 0 : offset;

        if constexpr (c_hasKey)
        {
            m_keyIndex.emplace(KeyOf{}(item), target);
        }

        // Commit. Nothing below allocates or throws.
        block* second = newBlock.get();
        if (firstBlock)
        {
            insert_block(0, ::std::move(newBlock), 0);
        }
        else if (split)
        {
            block& full = *m_blocks[blockIndex];
            for (::std::uint32_t i = offset; i < BlockCapacity; ++i)
            {
                move_key(full.items[i], &full, second);
                second->items.push_back(::std::move(full.items[i]));
            }
            full.items.erase(full.items.begin() + offset, full.items.end());
            insert_block(blockIndex + 1, ::std::move(newBlock), m_blockStart[blockIndex] + offset);
        }

        target->items.insert(target->items.begin() + targetOffset, ::std::move(item));
        for (::std::uint32_t i = target->position + 1; i < block_count(); ++i)
        {
            ++m_blockStart[i];
        }
        ++m_size;

        // A split can leave a small block next to a small neighbor. The 2 parts of the split hold BlockCapacity + 1
        // items together, so they never merge with each other.
        if (split)
        {
            restore_block_sizes(blockIndex);
            restore_block_sizes(second->position);
        }
    }

    void push_back(T item)
    {
        insert(m_size, ::std::move(item));
    }

    // Removes the item at position index and returns it. The caller decides when it is destroyed.
    T erase(::std::uint32_t index) noexcept
    {
        if (index >= m_size)
        {
            fail_fast();
        }

        const ::std::uint32_t blockIndex = find_block(index);
        block& current = *m_blocks[blockIndex];
        const ::std::uint32_t offset = index - m_blockStart[blockIndex];

        T removed = ::std::move(current.items[offset]);
        if constexpr (c_hasKey)
        {
            erase_key(KeyOf{}(removed), &current);
        }
        current.items.erase(current.items.begin() + offset);
        for (::std::uint32_t i = blockIndex + 1; i < block_count(); ++i)
        {
            --m_blockStart[i];
        }
        --m_size;

        if (current.items.empty())
        {
            remove_block(blockIndex);
            if (blockIndex > 0)
            {
                restore_block_sizes(blockIndex - 1);
            }
        }
        else
        {
            restore_block_sizes(blockIndex);
        }

        return removed;
    }

    T pop_back() noexcept
    {
        return erase(last_index());
    }

    // Replaces the item at position index and returns the old item. The caller decides when it is destroyed.
    T replace(::std::uint32_t index, T item)
    {
        if (index >= m_size)
        {
            fail_fast();
        }

        const ::std::uint32_t blockIndex = find_block(index);
        block& current = *m_blocks[blockIndex];
        T& slot = current.items[index - m_blockStart[blockIndex]];

        if constexpr (c_hasKey)
        {
            auto newKey = KeyOf{}(item);
            const auto oldKey = KeyOf{}(slot);
            if (!::std::equal_to<key_type>{}(newKey, oldKey))
            {
                // Add the new entry first: it can allocate. Erasing the old one cannot fail.
                m_keyIndex.emplace(::std::move(newKey), &current);
                erase_key(oldKey, &current);
            }
        }

        T old = ::std::move(slot);
        slot = ::std::move(item);
        return old;
    }

    // Removes all items. The list is already empty when the items are destroyed.
    void clear() noexcept
    {
        auto blocks = ::std::move(m_blocks);
        m_blocks.clear();
        m_blockStart.clear();
        m_size = 0;
        m_lastBlock = 0;
        if constexpr (c_hasKey)
        {
            m_keyIndex.clear();
        }
    }

    // Calls function(item) for up to count items, starting at position first, in order. Returns how many it visited.
    // function must not change the list.
    template <typename Function>
    ::std::uint32_t for_each(::std::uint32_t first, ::std::uint32_t count, Function&& function) const
    {
        if (first >= m_size)
        {
            return 0;
        }

        count = (::std::min)(count, m_size - first);
        ::std::uint32_t blockIndex = find_block(first);
        ::std::uint32_t offset = first - m_blockStart[blockIndex];
        for (::std::uint32_t visited = 0; visited < count; ++blockIndex, offset = 0)
        {
            const block& current = *m_blocks[blockIndex];
            for (; offset < current.items.size() && visited < count; ++offset, ++visited)
            {
                function(current.items[offset]);
            }
        }
        return count;
    }

    // The position of the first item whose key equals key, or nothing.
    ::std::optional<::std::uint32_t> find_first(const key_type& key) const
    {
        static_assert(c_hasKey, "find_first needs a KeyOf");

        // Every entry for the key names a block that holds at least one such item. The first position is the smallest
        // first match over those blocks; the order of entries in the hash table does not matter.
        ::std::optional<::std::uint32_t> result;
        const auto range = m_keyIndex.equal_range(key);
        for (auto entry = range.first; entry != range.second; ++entry)
        {
            const block& candidate = *entry->second;
            const ::std::uint32_t start = m_blockStart[candidate.position];
            if (result && start >= *result)
            {
                continue;
            }

            for (::std::uint32_t offset = 0; offset < candidate.items.size(); ++offset)
            {
                if (::std::equal_to<key_type>{}(KeyOf{}(candidate.items[offset]), key))
                {
                    result = start + offset;
                    break;
                }
            }
        }
        return result;
    }

    // For tests: a description of the first broken internal rule, or nullptr if the list is consistent.
    const char* invariant_violation() const
    {
        if (m_blockStart.size() != m_blocks.size())
        {
            return "m_blockStart and m_blocks have different lengths";
        }

        ::std::uint32_t total = 0;
        for (::std::uint32_t i = 0; i < block_count(); ++i)
        {
            const block* current = m_blocks[i].get();
            if (!current)
            {
                return "a block is null";
            }
            if (current->items.empty())
            {
                return "a block is empty";
            }
            if (current->items.size() > BlockCapacity)
            {
                return "a block holds more than BlockCapacity items";
            }
            if (current->items.capacity() < BlockCapacity)
            {
                return "a block has less than BlockCapacity reserved, so an insert could allocate after the list changed";
            }
            if (current->position != i)
            {
                return "a block's position does not match its index in m_blocks";
            }
            if (m_blockStart[i] != total)
            {
                return "a block's start does not match the number of items before it";
            }
            if (i > 0 && block_size(i - 1) + block_size(i) <= c_halfCapacity)
            {
                return "2 neighboring blocks hold half a block or less together";
            }
            total += block_size(i);
        }
        if (total != m_size)
        {
            return "m_size does not match the number of items in the blocks";
        }

        if constexpr (c_hasKey)
        {
            if (m_keyIndex.size() != m_size)
            {
                return "the key index does not have one entry per item";
            }

            // Count the entries for each block and key, then compare with the items in each block. A block pointer is
            // looked up among the blocks of the list before it is used, because a stale one points to freed memory.
            ::std::unordered_map<const block*, ::std::unordered_map<key_type, ::std::uint32_t>> entries;
            for (const auto& current : m_blocks)
            {
                entries[current.get()];
            }
            for (const auto& entry : m_keyIndex)
            {
                const auto owner = entries.find(entry.second);
                if (owner == entries.end())
                {
                    return "a key index entry names a block that is not in the list";
                }
                ++owner->second[entry.first];
            }
            for (const auto& current : m_blocks)
            {
                ::std::unordered_map<key_type, ::std::uint32_t> keys;
                for (const T& item : current->items)
                {
                    ++keys[KeyOf{}(item)];
                }
                if (keys != entries[current.get()])
                {
                    return "the key index entries of a block do not match its items";
                }
            }
        }

        return nullptr;
    }

private:
    struct block
    {
        ::std::vector<T> items;            // up to BlockCapacity items, in order; capacity reserved when created
        ::std::uint32_t position = 0;      // index of this block in m_blocks
    };

    using key_index = ::std::conditional_t<c_hasKey, ::std::unordered_multimap<key_type, block*>, detail::indexed_list_no_key>;

    [[noreturn]] static void fail_fast() noexcept
    {
        __fastfail(5 /* FAST_FAIL_INVALID_ARG */);
    }

    ::std::uint32_t last_index() const noexcept
    {
        if (m_size == 0)
        {
            fail_fast();
        }
        return m_size - 1;
    }

    ::std::uint32_t block_size(::std::uint32_t blockIndex) const noexcept
    {
        return static_cast<::std::uint32_t>(m_blocks[blockIndex]->items.size());
    }

    // Makes sure that one more element fits without an allocation. Grows by doubling, like push_back.
    template <typename Vector>
    static void reserve_one_more(Vector& vector)
    {
        if (vector.size() == vector.capacity())
        {
            vector.reserve(vector.size() < 4 ? 4 : vector.size() * 2);
        }
    }

    static ::std::unique_ptr<block> make_block()
    {
        auto newBlock = ::std::make_unique<block>();
        newBlock->items.reserve(BlockCapacity);
        return newBlock;
    }

    T& item_at(::std::uint32_t index) const noexcept
    {
        if (index >= m_size)
        {
            fail_fast();
        }
        const ::std::uint32_t blockIndex = find_block(index);
        return m_blocks[blockIndex]->items[index - m_blockStart[blockIndex]];
    }

    // The block that holds position index, which must be less than m_size. Checks the block of the last lookup and the
    // next one first, so walking the list in order costs O(1) per step.
    ::std::uint32_t find_block(::std::uint32_t index) const noexcept
    {
        const ::std::uint32_t count = block_count();
        const ::std::uint32_t hint = m_lastBlock;
        if (hint < count && index >= m_blockStart[hint])
        {
            if (index - m_blockStart[hint] < block_size(hint))
            {
                return hint;
            }
            // index is past this block, so index >= m_blockStart[hint + 1].
            if (hint + 1 < count && index - m_blockStart[hint + 1] < block_size(hint + 1))
            {
                m_lastBlock = hint + 1;
                return hint + 1;
            }
        }

        // Blocks are never empty, so the starts strictly increase: the block is the last one that starts at or before index.
        const auto next = ::std::upper_bound(m_blockStart.begin(), m_blockStart.end(), index);
        m_lastBlock = static_cast<::std::uint32_t>(next - m_blockStart.begin()) - 1;
        return m_lastBlock;
    }

    // Requires capacity reserved in m_blocks and m_blockStart. Does not allocate.
    void insert_block(::std::uint32_t blockIndex, ::std::unique_ptr<block> newBlock, ::std::uint32_t start) noexcept
    {
        m_blocks.insert(m_blocks.begin() + blockIndex, ::std::move(newBlock));
        m_blockStart.insert(m_blockStart.begin() + blockIndex, start);
        for (::std::uint32_t i = blockIndex; i < block_count(); ++i)
        {
            m_blocks[i]->position = i;
        }
    }

    void remove_block(::std::uint32_t blockIndex) noexcept
    {
        m_blocks.erase(m_blocks.begin() + blockIndex);
        m_blockStart.erase(m_blockStart.begin() + blockIndex);
        for (::std::uint32_t i = blockIndex; i < block_count(); ++i)
        {
            m_blocks[i]->position = i;
        }
    }

    // After block blockIndex got smaller, restore the rule that 2 neighboring blocks hold more than half a block
    // together, by merging. The rule keeps the number of blocks below about 4 * size / BlockCapacity + 1.
    void restore_block_sizes(::std::uint32_t blockIndex) noexcept
    {
        while (blockIndex < block_count())
        {
            if (blockIndex > 0 && block_size(blockIndex - 1) + block_size(blockIndex) <= c_halfCapacity)
            {
                merge_into_previous(blockIndex);
                --blockIndex;
            }
            else if (blockIndex + 1 < block_count() && block_size(blockIndex) + block_size(blockIndex + 1) <= c_halfCapacity)
            {
                merge_into_previous(blockIndex + 1);
            }
            else
            {
                break;
            }
        }
    }

    // Moves all items of block blockIndex to the end of the block before it, then removes block blockIndex. The 2 blocks
    // hold at most half a block together, so the previous block has room and nothing allocates.
    void merge_into_previous(::std::uint32_t blockIndex) noexcept
    {
        block& previous = *m_blocks[blockIndex - 1];
        block& current = *m_blocks[blockIndex];
        for (T& item : current.items)
        {
            move_key(item, &current, &previous);
            previous.items.push_back(::std::move(item));
        }
        current.items.clear();
        remove_block(blockIndex);
    }

    // Points one key index entry of item from block from to block to.
    void move_key([[maybe_unused]] const T& item, [[maybe_unused]] const block* from, [[maybe_unused]] block* to) noexcept
    {
        if constexpr (c_hasKey)
        {
            const auto range = m_keyIndex.equal_range(KeyOf{}(item));
            for (auto entry = range.first; entry != range.second; ++entry)
            {
                if (entry->second == from)
                {
                    entry->second = to;
                    return;
                }
            }
            fail_fast();
        }
    }

    // Erases one key index entry for key in block owner. Entries with the same key and block are interchangeable.
    void erase_key([[maybe_unused]] const key_type& key, [[maybe_unused]] const block* owner) noexcept
    {
        if constexpr (c_hasKey)
        {
            const auto range = m_keyIndex.equal_range(key);
            for (auto entry = range.first; entry != range.second; ++entry)
            {
                if (entry->second == owner)
                {
                    m_keyIndex.erase(entry);
                    return;
                }
            }
            fail_fast();
        }
    }

    ::std::vector<::std::unique_ptr<block>> m_blocks;    // the blocks, in list order
    ::std::vector<::std::uint32_t> m_blockStart;         // position of the first item of each block
    key_index m_keyIndex;                                // key -> block that holds an item with that key; one entry per item
    ::std::uint32_t m_size = 0;                          // number of items
    mutable ::std::uint32_t m_lastBlock = 0;             // block of the last lookup; only a hint, checked before use
};

} // namespace containers
