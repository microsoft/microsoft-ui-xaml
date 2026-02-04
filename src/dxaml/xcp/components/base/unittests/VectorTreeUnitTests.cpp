// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "VectorTreeUnitTests.h"

#include <vector_map.h>
#include <vector_set.h>
#include <map>
#include <set>
#include <algorithm>
#include <vector>
#include <random>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace VectorTree {

    // Make sure the layout is still optimally compact for an empty comparator
    static_assert(sizeof(containers::vector_set<int>) == sizeof(std::vector<int>), "vector_tree has excess padding");

    template<typename T1, typename T2>
    void VerifyElementsEqual(const T1& elem1, const T2& elem2)
    {
        if (elem1 != elem2)
        {
            VERIFY_ARE_EQUAL(elem1, elem2);
        }
    }

    template<typename T1, typename T2, typename T3, typename T4>
    void VerifyElementsEqual(const std::pair<T1, T2>& elem1, const std::pair<T3, T4>& elem2)
    {
        if (elem1.first != elem2.first)
        {
            VERIFY_ARE_EQUAL(elem1.first, elem2.first);
        }
        if (elem1.second != elem2.second)
        {
            VERIFY_ARE_EQUAL(elem1.second, elem2.second);
        }
    }

    template<typename Container1, typename Container2>
    void VerifyContainersEqual(const Container1& cont1, const Container2& cont2)
    {
        if (cont1.size() != cont2.size())
        {
            VERIFY_ARE_EQUAL(cont1.size(), cont2.size());
        }
        auto it1 = cont1.begin();
        auto it2 = cont2.begin();
        for (; it1 != cont1.end(); ++it1, ++it2)
        {
            VerifyElementsEqual(*it1, *it2);
        }
    }

    void VectorTreeUnitTests::MapInsertionTest()
    {
        // Insert lots of stuff into a std::map and a vector_map, and verify they match
        const size_t elementCount = 100;
        std::map<int, int> nMap;
        for (int i = 0; nMap.size() < 100; ++i) {
            nMap.emplace(i, i);
        }
        VERIFY_ARE_EQUAL(nMap.size(), elementCount);

        // Construct with iterator range
        containers::vector_map<int, int> vMap(nMap.begin(), nMap.end());
        VerifyContainersEqual(nMap, vMap);

        // Insertion into empty map
        vMap.clear();
        vMap.insert(nMap.begin(), nMap.end());
        VerifyContainersEqual(nMap, vMap);

        // Re-insertion
        std::map<int, int> nMap2(nMap);
        nMap.insert(nMap.begin(), nMap.end());
        vMap.insert(nMap.begin(), nMap.end());
        VerifyContainersEqual(nMap2, vMap);

        // Insert in random order
        std::vector<int> vec;
        vec.reserve(elementCount);
        for (int i = 0; vec.size() < elementCount; ++i) {
            vec.push_back(i);
        }
        std::mt19937 generator;
        std::shuffle(vec.begin(), vec.end(), generator);

        nMap.clear();
        vMap.clear();
        for (auto i : vec) {
            nMap.emplace(i, i);
            vMap.emplace(i, i);
            VerifyContainersEqual(nMap, vMap);
        }

        // Remove in random order
        std::shuffle(vec.begin(), vec.end(), generator);
        for (auto i : vec) {
            nMap.erase(i);
            vMap.erase(i);
            VerifyContainersEqual(nMap, vMap);
        }
    }

    void VectorTreeUnitTests::SetInsertionTest()
    {
        // Insert lots of stuff into a std::map and a vector_map, and verify they match
        const size_t elementCount = 100;
        std::set<int> nSet;
        for (int i = 0; nSet.size() < 100; ++i) {
            nSet.emplace(i);
        }
        VERIFY_ARE_EQUAL(nSet.size(), elementCount);

        // Construct with iterator range
        containers::vector_set<int> vSet(nSet.begin(), nSet.end());
        VerifyContainersEqual(nSet, vSet);

        // Insertion into empty map
        vSet.clear();
        vSet.insert(nSet.begin(), nSet.end());
        VerifyContainersEqual(nSet, vSet);

        // Re-insertion
        std::set<int> nSet2(nSet);
        nSet.insert(nSet.begin(), nSet.end());
        vSet.insert(nSet.begin(), nSet.end());
        VerifyContainersEqual(nSet, vSet);

        // Insert in random order
        std::vector<int> vec;
        vec.reserve(elementCount);
        for (int i = 0; vec.size() < elementCount; ++i) {
            vec.push_back(i);
        }
        std::mt19937 generator;
        std::shuffle(vec.begin(), vec.end(), generator);

        nSet.clear();
        vSet.clear();
        for (auto i : vec) {
            nSet.emplace(i);
            vSet.emplace(i);
            VerifyContainersEqual(nSet, vSet);
        }

        // Remove in random order
        std::shuffle(vec.begin(), vec.end(), generator);
        for (auto i : vec) {
            nSet.erase(i);
            vSet.erase(i);
            VerifyContainersEqual(nSet, vSet);
        }
    }

} } } } }