// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#pragma once

#include "WexTestClass.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { 
    namespace AccessKeys {
        class TreeAnalyzerUnitTests : public WEX::TestClass<TreeAnalyzerUnitTests>
        {
        public:          
            BEGIN_TEST_CLASS(TreeAnalyzerUnitTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                END_TEST_CLASS()

                BEGIN_TEST_METHOD(AllElementsWithAKFound)
                    TEST_METHOD_PROPERTY(L"Description", L"Verifies that we find all elements in the tree with an AK")
                END_TEST_METHOD()

                BEGIN_TEST_METHOD(ElementsWithInvalidAKAreFiltered)
                    TEST_METHOD_PROPERTY(L"Description", L"Verifies that when invalid AKs are found, we do not add them to the list")
                END_TEST_METHOD()

                BEGIN_TEST_METHOD(EmptyVectorReturnedIfNoValidAK)
                    TEST_METHOD_PROPERTY(L"Description", L"If we can't find any AK's, return an empty vector")
                END_TEST_METHOD()

                BEGIN_TEST_METHOD(AllElementsWithMultipleLeaves)
                    TEST_METHOD_PROPERTY(L"Description", L"Makes sure we explore the entire tree, including nodes that branch from other nodes")
                END_TEST_METHOD()

                BEGIN_TEST_METHOD(AllElementsAreFoundEvenWithInvalidNodes)
                    TEST_METHOD_PROPERTY(L"Description", L"Even if some nodes are invalid, we should still search its children")
                END_TEST_METHOD()

                TEST_METHOD(FindScopeRootOnBasicTree)
                TEST_METHOD(FindScopeRootOnTreeWithScopeOwner)
                TEST_METHOD(FindScopeRootOnTreesWithOwner)
                TEST_METHOD(ValidateScopeOwnerMap)
                TEST_METHOD(FindAKsInMultipleTrees)
                TEST_METHOD(ScopeCycle)
                TEST_METHOD(ScopeStopsAtBranch)
                TEST_METHOD(TreesWithMultipleParentPointers)
                TEST_METHOD(ScopeOwnerMustBeScope)


            template<class Element>
            class MockCollection
            {
            public:
                bool IsLeaving() const
                {
                    return false;
                }

                unsigned int GetCount() const
                {
                    return static_cast<unsigned int>(m_collection.size());
                }

                Element* GetItemWithAddRef(int index)
                {
                    return m_collection[index];
                }

                void SetCollection(std::vector<Element*>& collection)
                {
                    m_collection = collection;
                }

                void AddItem(Element* e)
                {
                    m_collection.push_back(e);
                }
            private:
                std::vector<Element*> m_collection;
            };
        };
    }
}}}}
