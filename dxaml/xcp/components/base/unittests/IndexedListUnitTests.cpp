// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "IndexedListUnitTests.h"

#include <indexed_list.h>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <random>
#include <set>
#include <utility>
#include <vector>

using namespace WEX::TestExecution;

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace IndexedList {

    namespace {

        struct Item
        {
            int key;
            int id; // unique, so that items with the same key can be told apart

            bool operator==(const Item& other) const { return key == other.key && id == other.id; }
        };

        struct ItemKey
        {
            int operator()(const Item& item) const noexcept { return item.key; }
        };

        // Blocks of 4 make a few items enough for many splits and merges.
        using SmallList = containers::indexed_list<Item, ItemKey, 4>;
        using DefaultList = containers::indexed_list<Item, ItemKey>;

        std::uint32_t RandomBelow(std::mt19937& random, std::uint32_t limit)
        {
            return std::uniform_int_distribution<std::uint32_t>(0, limit - 1)(random);
        }

        // Checks the internal rules of the list, then compares its items with expected: in order through for_each, and
        // backward through operator[], so that the lookup hint is both used and missed.
        template <typename List>
        void VerifyMatches(const List& list, const std::vector<Item>& expected)
        {
            if (const char* violation = list.invariant_violation())
            {
                VERIFY_FAIL(WEX::Common::String().Format(L"Broken internal rule: %S", violation));
            }
            VERIFY_ARE_EQUAL(static_cast<std::uint32_t>(expected.size()), list.size());

            std::uint32_t position = 0;
            std::optional<std::uint32_t> firstDifference;
            const std::uint32_t visited = list.for_each(0, list.size(), [&](const Item& item)
            {
                if (!firstDifference && !(item == expected[position]))
                {
                    firstDifference = position;
                }
                ++position;
            });
            VERIFY_ARE_EQUAL(list.size(), visited);
            if (firstDifference)
            {
                VERIFY_FAIL(WEX::Common::String().Format(L"for_each differs at position %u", *firstDifference));
            }

            for (std::uint32_t i = list.size(); i-- > 0;)
            {
                if (!(list[i] == expected[i]))
                {
                    VERIFY_FAIL(WEX::Common::String().Format(L"operator[] differs at position %u", i));
                }
            }
        }

        template <typename List>
        void VerifyFindFirst(const List& list, const std::vector<Item>& expected, int key)
        {
            const auto found = std::find_if(expected.begin(), expected.end(), [key](const Item& item) { return item.key == key; });
            const int wanted = found == expected.end() ? -1 : static_cast<int>(found - expected.begin());
            const std::optional<std::uint32_t> actual = list.find_first(key);
            VERIFY_ARE_EQUAL(wanted, actual ? static_cast<int>(*actual) : -1, WEX::Common::String().Format(L"find_first(%d)", key));
        }

        template <typename List>
        void Append(List& list, std::vector<Item>& expected, int count)
        {
            for (int i = 0; i < count; ++i)
            {
                const Item item{ static_cast<int>(expected.size()), static_cast<int>(expected.size()) };
                list.push_back(item);
                expected.push_back(item);
            }
        }

        template <typename List>
        void InsertBoth(List& list, std::vector<Item>& expected, std::uint32_t position, const Item& item)
        {
            list.insert(position, item);
            expected.insert(expected.begin() + position, item);
        }

        template <typename List>
        void EraseBoth(List& list, std::vector<Item>& expected, std::uint32_t position)
        {
            const Item removed = list.erase(position);
            VERIFY_IS_TRUE(removed == expected[position]);
            expected.erase(expected.begin() + position);
        }

        // A move-only item that keeps the set of alive items, to catch items that leak or are destroyed too early. Its key
        // is its id, which a move clears, so that code that reads the key of an item after moving it fails.
        class TrackedItem
        {
        public:
            explicit TrackedItem(int id) : m_id(id) { s_alive.insert(id); }
            TrackedItem(TrackedItem&& other) noexcept : m_id(std::exchange(other.m_id, -1)) {}
            TrackedItem& operator=(TrackedItem&& other) noexcept
            {
                Release();
                m_id = std::exchange(other.m_id, -1);
                return *this;
            }
            ~TrackedItem() { Release(); }

            int Id() const { return m_id; }

            inline static std::set<int> s_alive;

            // When set, each destroyed item records what it can see of this list at that moment.
            inline static const containers::indexed_list<TrackedItem, struct TrackedItemKey, 4>* s_watchedList = nullptr;
            inline static std::uint32_t s_destroyedWhileListNotEmpty = 0;
            inline static std::uint32_t s_destroyedWhileFound = 0;

        private:
            void Release() noexcept;

            int m_id;
        };

        struct TrackedItemKey
        {
            int operator()(const TrackedItem& item) const noexcept { return item.Id(); }
        };

        using TrackedList = containers::indexed_list<TrackedItem, TrackedItemKey, 4>;

        void TrackedItem::Release() noexcept
        {
            if (m_id >= 0)
            {
                s_alive.erase(m_id);
                if (s_watchedList)
                {
                    s_destroyedWhileListNotEmpty += s_watchedList->empty() ? 0 : 1;
                    s_destroyedWhileFound += s_watchedList->find_first(m_id).has_value() ? 1 : 0;
                }
                m_id = -1;
            }
        }

    } // namespace

    // Applies thousands of random edits to a list with blocks of 4 and to a std::vector, and compares them after each
    // edit. The list grows and shrinks in turns, so that blocks split and merge everywhere, and it is sometimes empty.
    // With 10 keys for up to about 250 items, a block often holds the same key more than once.
    template <typename List, bool HasKey>
    static void RunRandomEdits()
    {
        constexpr std::uint32_t keyCount = 10;

        for (std::uint32_t seed = 1; seed <= 5; ++seed)
        {
            std::mt19937 random(seed);
            List list;
            std::vector<Item> expected;
            int nextId = 0;

            for (int step = 0; step < 4000; ++step)
            {
                const bool growing = (step / 500) % 2 == 0;
                const auto size = static_cast<std::uint32_t>(expected.size());
                const std::uint32_t choice = RandomBelow(random, 100);
                const Item item{ static_cast<int>(RandomBelow(random, keyCount)), nextId++ };
                int changedKey = item.key;

                if (step % 2000 == 1250)
                {
                    list.clear();
                    expected.clear();
                }
                else if (size == 0 || choice < (growing ? 55u : 15u))
                {
                    InsertBoth(list, expected, RandomBelow(random, size + 1), item);
                }
                else if (choice < (growing ? 65u : 20u))
                {
                    list.push_back(item);
                    expected.push_back(item);
                }
                else if (choice < (growing ? 80u : 70u))
                {
                    const std::uint32_t position = RandomBelow(random, size);
                    changedKey = expected[position].key;
                    EraseBoth(list, expected, position);
                }
                else if (choice < 85u)
                {
                    VERIFY_IS_TRUE(list.pop_back() == expected.back());
                    changedKey = expected.back().key;
                    expected.pop_back();
                }
                else
                {
                    const std::uint32_t position = RandomBelow(random, size);
                    VERIFY_IS_TRUE(list.replace(position, item) == expected[position]);
                    changedKey = expected[position].key;
                    expected[position] = item;
                }

                VerifyMatches(list, expected);
                if constexpr (HasKey)
                {
                    VerifyFindFirst(list, expected, changedKey);
                    if (item.key != changedKey)
                    {
                        VerifyFindFirst(list, expected, item.key);
                    }
                }
            }
        }
    }

    void IndexedListUnitTests::RandomEditsMatchStdVector()
    {
        SetVerifyOutput verifySettings(VerifyOutputSettings::LogOnlyFailures);
        RunRandomEdits<SmallList, true>();

        // A list without a key, like TreeView's token lists, is compiled without the key index.
        RunRandomEdits<containers::indexed_list<Item, void, 4>, false>();
    }

    // The same comparison with the default block size, at sizes that TreeView reaches: grow to 50,000 items at random
    // positions, shrink to 10,000, then insert and remove 50,000 items one after another in the middle, as when a node
    // with many children expands and collapses. Compares everything every 5,000 edits.
    void IndexedListUnitTests::LargeListMatchesStdVector()
    {
        SetVerifyOutput verifySettings(VerifyOutputSettings::LogOnlyFailures);
        std::mt19937 random(42);
        DefaultList list;
        std::vector<Item> expected;
        int nextId = 0;

        // Some keys repeat.
        const auto nextItem = [&]() { const int id = nextId++; return Item{ id % 40000, id }; };
        const auto verify = [&]()
        {
            VerifyMatches(list, expected);
            for (int i = 0; i < 20; ++i)
            {
                VerifyFindFirst(list, expected, expected[RandomBelow(random, list.size())].key);
            }
            VerifyFindFirst(list, expected, -1);
        };

        while (expected.size() < 50000)
        {
            InsertBoth(list, expected, RandomBelow(random, list.size() + 1), nextItem());
            if (expected.size() % 5000 == 0)
            {
                verify();
            }
        }

        while (expected.size() > 10000)
        {
            EraseBoth(list, expected, RandomBelow(random, list.size()));
            if (expected.size() % 5000 == 0)
            {
                verify();
            }
        }

        constexpr std::uint32_t node = 5000;
        for (std::uint32_t child = 1; child <= 50000; ++child)
        {
            InsertBoth(list, expected, node + child, nextItem());
            if (child % 5000 == 0)
            {
                verify();
            }
        }

        for (std::uint32_t child = 1; child <= 50000; ++child)
        {
            EraseBoth(list, expected, node + 1);
            if (child % 5000 == 0)
            {
                verify();
            }
        }
    }

    // Edits at the first and last position of blocks, with blocks of 4, and the merges that follow when blocks shrink.
    void IndexedListUnitTests::BlockEdges()
    {
        SetVerifyOutput verifySettings(VerifyOutputSettings::LogOnlyFailures);

        // 12 items added at the end fill exactly 3 blocks. An insert at any position then splits exactly one block.
        for (std::uint32_t position = 0; position <= 12; ++position)
        {
            SmallList list;
            std::vector<Item> expected;
            Append(list, expected, 12);
            VERIFY_ARE_EQUAL(3u, list.block_count());

            InsertBoth(list, expected, position, Item{ 100, 100 });
            VerifyMatches(list, expected);
            VERIFY_ARE_EQUAL(4u, list.block_count());
        }

        // A block that loses its last item goes away: first, middle and last block.
        for (std::uint32_t emptiedBlock = 0; emptiedBlock < 3; ++emptiedBlock)
        {
            SmallList list;
            std::vector<Item> expected;
            Append(list, expected, 12);
            for (int i = 0; i < 4; ++i)
            {
                EraseBoth(list, expected, emptiedBlock * 4);
                VerifyMatches(list, expected);
            }
            VERIFY_ARE_EQUAL(2u, list.block_count());
        }

        // When 2 neighboring blocks hold half a block or less together, they merge. Start from [0 1 2 3] [7] [8 9 10 11].
        {
            SmallList list;
            std::vector<Item> expected;
            Append(list, expected, 12);
            for (int i = 0; i < 3; ++i)
            {
                EraseBoth(list, expected, 4);
            }
            VERIFY_ARE_EQUAL(3u, list.block_count());

            // The first block shrinks to [3], next to [7]: it takes in the block after it.
            for (int i = 0; i < 3; ++i)
            {
                EraseBoth(list, expected, 0);
                VerifyMatches(list, expected);
            }
            VERIFY_ARE_EQUAL(2u, list.block_count());
        }
        {
            SmallList list;
            std::vector<Item> expected;
            Append(list, expected, 12);
            for (int i = 0; i < 3; ++i)
            {
                EraseBoth(list, expected, 4);
            }

            // The last block shrinks to [8], next to [7]: the block before it takes it in.
            for (int i = 0; i < 3; ++i)
            {
                VERIFY_IS_TRUE(list.pop_back() == expected.back());
                expected.pop_back();
                VerifyMatches(list, expected);
            }
            VERIFY_ARE_EQUAL(2u, list.block_count());
        }

        // for_each on every part of the list, as IVector::GetMany reads it, including parts that end past the last item.
        {
            SmallList list;
            std::vector<Item> expected;
            Append(list, expected, 10);
            for (std::uint32_t first = 0; first <= 11; ++first)
            {
                for (std::uint32_t count = 0; count <= 11; ++count)
                {
                    const std::uint32_t begin = (std::min)(first, 10u);
                    const std::uint32_t end = (std::min)(first + count, 10u);
                    const std::vector<Item> wanted(expected.begin() + begin, expected.begin() + end);
                    std::vector<Item> visited;
                    VERIFY_ARE_EQUAL(end - begin, list.for_each(first, count, [&](const Item& item) { visited.push_back(item); }));
                    VERIFY_IS_TRUE(visited == wanted);
                }
            }

            // A cleared list works like a new one.
            list.clear();
            expected.clear();
            VerifyMatches(list, expected);
            VERIFY_ARE_EQUAL(0u, list.block_count());
            Append(list, expected, 5);
            VerifyMatches(list, expected);
        }
    }

    // TreeView inserts the children of an expanding node one after another, right after the node. Those inserts must
    // fill blocks completely, or the list uses up to twice as many blocks. Tries every position of the node in its block.
    void IndexedListUnitTests::ConsecutiveInsertsFillBlocks()
    {
        SetVerifyOutput verifySettings(VerifyOutputSettings::LogOnlyFailures);

        for (std::uint32_t node = 0; node < 8; ++node)
        {
            SmallList list;
            std::vector<Item> expected;
            Append(list, expected, 8);
            for (std::uint32_t child = 1; child <= 40; ++child)
            {
                InsertBoth(list, expected, node + child, Item{ static_cast<int>(1000 + child), static_cast<int>(1000 + child) });
            }
            VerifyMatches(list, expected);

            // 48 items fill 12 blocks. Only the block with the last child and the block with the items that followed
            // the node can be partly filled, and 2 partly filled blocks need 13 blocks at most.
            VERIFY_IS_LESS_THAN_OR_EQUAL(list.block_count(), 13u, WEX::Common::String().Format(L"node at position %u", node));
        }
    }

    // A list can hold the same item more than once, so the key index allows repeated keys. find_first returns the first
    // position with the key, like std::find. Erasing one copy leaves the other copies.
    void IndexedListUnitTests::DuplicateKeys()
    {
        SetVerifyOutput verifySettings(VerifyOutputSettings::LogOnlyFailures);
        SmallList list;
        std::vector<Item> expected;

        // Keys 0 1 2 0 1 2 ...: every full block holds each key, and some blocks hold a key twice.
        for (int id = 0; id < 30; ++id)
        {
            list.push_back(Item{ id % 3, id });
            expected.push_back(Item{ id % 3, id });
        }
        VerifyMatches(list, expected);
        VERIFY_ARE_EQUAL(0u, list.find_first(0).value());
        VERIFY_ARE_EQUAL(2u, list.find_first(2).value());
        VERIFY_IS_FALSE(list.find_first(3).has_value());

        // Erasing the first copy of key 0 makes its next copy the first: [1 2 0 1 2 0 ...].
        EraseBoth(list, expected, 0);
        VERIFY_ARE_EQUAL(2u, list.find_first(0).value());

        // A replace that changes the key moves the item from one key to the other: [3 2 0 1 2 0 ...].
        VERIFY_IS_TRUE((list.replace(0, Item{ 3, 100 }) == Item{ 1, 1 }));
        expected[0] = Item{ 3, 100 };
        VERIFY_ARE_EQUAL(3u, list.find_first(1).value());
        VERIFY_ARE_EQUAL(0u, list.find_first(3).value());
        VerifyMatches(list, expected);

        // Erase the rest in random order, and check every key after each erase.
        std::mt19937 random(3);
        while (!expected.empty())
        {
            EraseBoth(list, expected, RandomBelow(random, list.size()));
            VerifyMatches(list, expected);
            for (int key = 0; key <= 3; ++key)
            {
                VerifyFindFirst(list, expected, key);
            }
        }
    }

    // The list must not leak or destroy items on its own, except in clear() and its destructor. erase, pop_back and
    // replace hand the item back, so the caller decides when it is destroyed. clear() and the destructor empty the
    // list before they destroy the items, so code that runs when an item is destroyed sees an empty list.
    void IndexedListUnitTests::ItemLifetime()
    {
        SetVerifyOutput verifySettings(VerifyOutputSettings::LogOnlyFailures);
        TrackedItem::s_alive.clear();
        const auto verifyAllFound = [](const TrackedList& list)
        {
            if (const char* violation = list.invariant_violation())
            {
                VERIFY_FAIL(WEX::Common::String().Format(L"Broken internal rule: %S", violation));
            }
            for (std::uint32_t i = 0; i < list.size(); ++i)
            {
                VERIFY_ARE_EQUAL(i, list.find_first(list[i].Id()).value_or(UINT32_MAX));
            }
        };

        {
            auto list = std::make_unique<TrackedList>();
            for (int id = 0; id < 40; ++id)
            {
                list->push_back(TrackedItem(id));
            }
            for (int id = 40; id < 60; ++id)
            {
                list->insert(id % 7, TrackedItem(id));
            }
            VERIFY_ARE_EQUAL(60u, static_cast<std::uint32_t>(TrackedItem::s_alive.size()));
            verifyAllFound(*list);

            {
                const int id = (*list)[10].Id();
                TrackedItem removed = list->erase(10);
                VERIFY_ARE_EQUAL(id, removed.Id());
                VERIFY_ARE_EQUAL(1u, static_cast<std::uint32_t>(TrackedItem::s_alive.count(id)));
                VERIFY_IS_FALSE(list->find_first(id).has_value());

                const int lastId = (*list)[list->size() - 1].Id();
                TrackedItem popped = list->pop_back();
                VERIFY_ARE_EQUAL(lastId, popped.Id());

                const int replacedId = (*list)[0].Id();
                TrackedItem old = list->replace(0, TrackedItem(1000));
                VERIFY_ARE_EQUAL(replacedId, old.Id());
                VERIFY_ARE_EQUAL(1000, (*list)[0].Id());
                VERIFY_IS_FALSE(list->find_first(replacedId).has_value());
                VERIFY_ARE_EQUAL(61u, static_cast<std::uint32_t>(TrackedItem::s_alive.size()));
            }
            VERIFY_ARE_EQUAL(58u, static_cast<std::uint32_t>(TrackedItem::s_alive.size()));
            verifyAllFound(*list);

            // Erases that merge blocks keep every other item alive.
            std::mt19937 random(5);
            while (list->size() > 10)
            {
                list->erase(RandomBelow(random, list->size()));
                verifyAllFound(*list);
            }
            VERIFY_ARE_EQUAL(10u, static_cast<std::uint32_t>(TrackedItem::s_alive.size()));

            TrackedItem::s_watchedList = list.get();
            TrackedItem::s_destroyedWhileListNotEmpty = 0;
            TrackedItem::s_destroyedWhileFound = 0;
            list->clear();
            VERIFY_ARE_EQUAL(0u, static_cast<std::uint32_t>(TrackedItem::s_alive.size()));
            VERIFY_ARE_EQUAL(0u, TrackedItem::s_destroyedWhileListNotEmpty);
            VERIFY_ARE_EQUAL(0u, TrackedItem::s_destroyedWhileFound);

            for (int id = 0; id < 20; ++id)
            {
                list->insert(0, TrackedItem(id));
            }
            VERIFY_ARE_EQUAL(20u, static_cast<std::uint32_t>(TrackedItem::s_alive.size()));
            verifyAllFound(*list);

            // The destructor of the list destroys the rest, the same way.
            list.reset();
            TrackedItem::s_watchedList = nullptr;
            VERIFY_ARE_EQUAL(0u, TrackedItem::s_destroyedWhileListNotEmpty);
            VERIFY_ARE_EQUAL(0u, TrackedItem::s_destroyedWhileFound);
        }
        VERIFY_ARE_EQUAL(0u, static_cast<std::uint32_t>(TrackedItem::s_alive.size()));
    }

} } } } }
