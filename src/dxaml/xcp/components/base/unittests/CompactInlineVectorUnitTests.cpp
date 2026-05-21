// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CompactInlineVectorUnitTests.h"

#include <CompactInlineVector.h>
#include <vector>
#include <string>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace CompactInlineVectorTests {

    // Helper to track construction/destruction for non-trivial types
    struct TrackedObject
    {
        static int s_constructCount;
        static int s_destructCount;
        static int s_moveCount;
        static int s_copyCount;

        int value;

        TrackedObject() : value(0) { ++s_constructCount; }
        explicit TrackedObject(int v) : value(v) { ++s_constructCount; }
        TrackedObject(const TrackedObject& other) : value(other.value) { ++s_constructCount; ++s_copyCount; }
        TrackedObject(TrackedObject&& other) noexcept : value(other.value) { other.value = -1; ++s_constructCount; ++s_moveCount; }
        ~TrackedObject() { ++s_destructCount; }

        TrackedObject& operator=(const TrackedObject& other) { value = other.value; ++s_copyCount; return *this; }
        TrackedObject& operator=(TrackedObject&& other) noexcept { value = other.value; other.value = -1; ++s_moveCount; return *this; }

        bool operator==(const TrackedObject& other) const { return value == other.value; }

        static void ResetCounts()
        {
            s_constructCount = 0;
            s_destructCount = 0;
            s_moveCount = 0;
            s_copyCount = 0;
        }
    };

    int TrackedObject::s_constructCount = 0;
    int TrackedObject::s_destructCount = 0;
    int TrackedObject::s_moveCount = 0;
    int TrackedObject::s_copyCount = 0;

    void CompactInlineVectorUnitTests::DefaultConstruction()
    {
        CompactInlineVector<int> vec;

        VERIFY_ARE_EQUAL(static_cast<size_t>(0), vec.size());
        VERIFY_ARE_EQUAL(true, vec.empty());
        VERIFY_ARE_EQUAL(true, vec.is_inline());
        VERIFY_IS_TRUE(vec.capacity() >= vec.inline_capacity());
    }

    void CompactInlineVectorUnitTests::PushBackAndAccess()
    {
        CompactInlineVector<int> vec;

        vec.push_back(10);
        vec.push_back(20);
        vec.push_back(30);

        VERIFY_ARE_EQUAL(static_cast<size_t>(3), vec.size());
        VERIFY_ARE_EQUAL(false, vec.empty());

        VERIFY_ARE_EQUAL(10, vec[0]);
        VERIFY_ARE_EQUAL(20, vec[1]);
        VERIFY_ARE_EQUAL(30, vec[2]);

        VERIFY_ARE_EQUAL(10, vec.front());
        VERIFY_ARE_EQUAL(30, vec.back());

        // Modify through operator[]
        vec[1] = 25;
        VERIFY_ARE_EQUAL(25, vec[1]);

        // Pop back
        vec.pop_back();
        VERIFY_ARE_EQUAL(static_cast<size_t>(2), vec.size());
        VERIFY_ARE_EQUAL(25, vec.back());
    }

    void CompactInlineVectorUnitTests::InlineToHeapTransition()
    {
        // Use a small inline capacity to force heap allocation
        CompactInlineVector<int, 2> vec;

        VERIFY_ARE_EQUAL(static_cast<size_t>(2), vec.inline_capacity());
        VERIFY_ARE_EQUAL(true, vec.is_inline());

        vec.push_back(1);
        vec.push_back(2);
        VERIFY_ARE_EQUAL(true, vec.is_inline());

        // This should trigger heap allocation
        vec.push_back(3);
        VERIFY_ARE_EQUAL(false, vec.is_inline());
        VERIFY_IS_TRUE(vec.capacity() > 2);

        // Verify data is intact
        VERIFY_ARE_EQUAL(1, vec[0]);
        VERIFY_ARE_EQUAL(2, vec[1]);
        VERIFY_ARE_EQUAL(3, vec[2]);
    }

    void CompactInlineVectorUnitTests::MoveConstruction()
    {
        CompactInlineVector<int, 2> vec1;
        vec1.push_back(10);
        vec1.push_back(20);

        // Move from inline storage
        CompactInlineVector<int, 2> vec2(std::move(vec1));

        VERIFY_ARE_EQUAL(static_cast<size_t>(2), vec2.size());
        VERIFY_ARE_EQUAL(10, vec2[0]);
        VERIFY_ARE_EQUAL(20, vec2[1]);
        VERIFY_ARE_EQUAL(static_cast<size_t>(0), vec1.size());

        // Now test move from heap storage
        CompactInlineVector<int, 2> vec3;
        vec3.push_back(1);
        vec3.push_back(2);
        vec3.push_back(3); // Forces heap

        VERIFY_ARE_EQUAL(false, vec3.is_inline());

        CompactInlineVector<int, 2> vec4(std::move(vec3));
        VERIFY_ARE_EQUAL(static_cast<size_t>(3), vec4.size());
        VERIFY_ARE_EQUAL(false, vec4.is_inline());
        VERIFY_ARE_EQUAL(1, vec4[0]);
        VERIFY_ARE_EQUAL(2, vec4[1]);
        VERIFY_ARE_EQUAL(3, vec4[2]);
        VERIFY_ARE_EQUAL(static_cast<size_t>(0), vec3.size());
        VERIFY_ARE_EQUAL(true, vec3.is_inline());
    }

    void CompactInlineVectorUnitTests::MoveAssignment()
    {
        CompactInlineVector<int, 2> vec1;
        vec1.push_back(10);
        vec1.push_back(20);

        CompactInlineVector<int, 2> vec2;
        vec2.push_back(100);

        vec2 = std::move(vec1);

        VERIFY_ARE_EQUAL(static_cast<size_t>(2), vec2.size());
        VERIFY_ARE_EQUAL(10, vec2[0]);
        VERIFY_ARE_EQUAL(20, vec2[1]);
        VERIFY_ARE_EQUAL(static_cast<size_t>(0), vec1.size());
    }

    void CompactInlineVectorUnitTests::InsertAndErase()
    {
        CompactInlineVector<int> vec;
        vec.push_back(1);
        vec.push_back(3);

        // Insert in middle
        vec.insert(vec.begin() + 1, 2);
        VERIFY_ARE_EQUAL(static_cast<size_t>(3), vec.size());
        VERIFY_ARE_EQUAL(1, vec[0]);
        VERIFY_ARE_EQUAL(2, vec[1]);
        VERIFY_ARE_EQUAL(3, vec[2]);

        // Insert at beginning
        vec.insert(vec.begin(), 0);
        VERIFY_ARE_EQUAL(static_cast<size_t>(4), vec.size());
        VERIFY_ARE_EQUAL(0, vec[0]);
        VERIFY_ARE_EQUAL(1, vec[1]);

        // Erase from middle
        vec.erase(vec.begin() + 2);
        VERIFY_ARE_EQUAL(static_cast<size_t>(3), vec.size());
        VERIFY_ARE_EQUAL(0, vec[0]);
        VERIFY_ARE_EQUAL(1, vec[1]);
        VERIFY_ARE_EQUAL(3, vec[2]);

        // Erase range
        vec.erase(vec.begin(), vec.begin() + 2);
        VERIFY_ARE_EQUAL(static_cast<size_t>(1), vec.size());
        VERIFY_ARE_EQUAL(3, vec[0]);
    }

    void CompactInlineVectorUnitTests::EmplaceBack()
    {
        CompactInlineVector<std::pair<int, int>> vec;

        vec.emplace_back(1, 2);
        vec.emplace_back(3, 4);

        VERIFY_ARE_EQUAL(static_cast<size_t>(2), vec.size());
        VERIFY_ARE_EQUAL(1, vec[0].first);
        VERIFY_ARE_EQUAL(2, vec[0].second);
        VERIFY_ARE_EQUAL(3, vec[1].first);
        VERIFY_ARE_EQUAL(4, vec[1].second);
    }

    void CompactInlineVectorUnitTests::Clear()
    {
        TrackedObject::ResetCounts();

        {
            CompactInlineVector<TrackedObject, 4> vec;
            vec.emplace_back(1);
            vec.emplace_back(2);
            vec.emplace_back(3);

            VERIFY_ARE_EQUAL(static_cast<size_t>(3), vec.size());

            vec.clear();

            VERIFY_ARE_EQUAL(static_cast<size_t>(0), vec.size());
            VERIFY_ARE_EQUAL(true, vec.empty());
            // Capacity should remain unchanged
            VERIFY_IS_TRUE(vec.capacity() >= 4);
        }

        // All objects should be destructed
        VERIFY_ARE_EQUAL(TrackedObject::s_constructCount, TrackedObject::s_destructCount);
    }

    void CompactInlineVectorUnitTests::Resize()
    {
        CompactInlineVector<int> vec;
        vec.push_back(1);
        vec.push_back(2);
        vec.push_back(3);

        // Resize larger
        vec.resize(5);
        VERIFY_ARE_EQUAL(static_cast<size_t>(5), vec.size());
        VERIFY_ARE_EQUAL(1, vec[0]);
        VERIFY_ARE_EQUAL(2, vec[1]);
        VERIFY_ARE_EQUAL(3, vec[2]);
        VERIFY_ARE_EQUAL(0, vec[3]); // Default constructed
        VERIFY_ARE_EQUAL(0, vec[4]);

        // Resize smaller
        vec.resize(2);
        VERIFY_ARE_EQUAL(static_cast<size_t>(2), vec.size());
        VERIFY_ARE_EQUAL(1, vec[0]);
        VERIFY_ARE_EQUAL(2, vec[1]);
    }

    void CompactInlineVectorUnitTests::Iterators()
    {
        CompactInlineVector<int> vec;
        vec.push_back(1);
        vec.push_back(2);
        vec.push_back(3);

        // Range-for (uses begin/end)
        int sum = 0;
        for (int val : vec)
        {
            sum += val;
        }
        VERIFY_ARE_EQUAL(6, sum);

        // Verify iterators
        VERIFY_ARE_EQUAL(vec.data(), vec.begin());
        VERIFY_ARE_EQUAL(vec.data() + vec.size(), vec.end());

        // Modify through iterator
        for (auto& val : vec)
        {
            val *= 2;
        }
        VERIFY_ARE_EQUAL(2, vec[0]);
        VERIFY_ARE_EQUAL(4, vec[1]);
        VERIFY_ARE_EQUAL(6, vec[2]);

        // Reverse iterators
        auto rit = vec.rbegin();
        VERIFY_ARE_EQUAL(6, *rit);
        ++rit;
        VERIFY_ARE_EQUAL(4, *rit);
    }

    void CompactInlineVectorUnitTests::DeepCopy()
    {
        CompactInlineVector<int> vec1;
        vec1.push_back(10);
        vec1.push_back(20);
        vec1.push_back(30);

        CompactInlineVector<int> vec2;
        vec1.DeepCopyTo(vec2);

        VERIFY_ARE_EQUAL(vec1.size(), vec2.size());
        VERIFY_ARE_EQUAL(vec1[0], vec2[0]);
        VERIFY_ARE_EQUAL(vec1[1], vec2[1]);
        VERIFY_ARE_EQUAL(vec1[2], vec2[2]);

        // Modify original, verify copy is independent
        vec1[0] = 100;
        VERIFY_ARE_EQUAL(10, vec2[0]);
    }

    void CompactInlineVectorUnitTests::NonTrivialType()
    {
        TrackedObject::ResetCounts();

        {
            CompactInlineVector<TrackedObject, 2> vec;

            vec.emplace_back(1);
            vec.emplace_back(2);
            VERIFY_ARE_EQUAL(true, vec.is_inline());

            // This triggers heap allocation and element moves
            vec.emplace_back(3);
            VERIFY_ARE_EQUAL(false, vec.is_inline());

            VERIFY_ARE_EQUAL(1, vec[0].value);
            VERIFY_ARE_EQUAL(2, vec[1].value);
            VERIFY_ARE_EQUAL(3, vec[2].value);
        }

        // Verify all objects were properly destructed
        VERIFY_ARE_EQUAL(TrackedObject::s_constructCount, TrackedObject::s_destructCount);
    }

} } } } }
