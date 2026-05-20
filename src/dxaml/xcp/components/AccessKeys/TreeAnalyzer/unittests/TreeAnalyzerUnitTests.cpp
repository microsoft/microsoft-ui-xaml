// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

int failFastCalls = 0;
#undef XAML_FAIL_FAST
#define XAML_FAIL_FAST() {++failFastCalls;}

#include "TreeAnalyzerUnitTests.h"
#include "TreeAnalyzer.h"
#include <CxxMockTaef.h>
#include <XamlLogging.h>
#include <Indexes.g.h>
#include <Mocks.h>

using namespace CxxMock;
using namespace AccessKeys;

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
   namespace AccessKeys {

        enum ScopeFlags
        {
            ScopeFlags_None,
            ScopeFlags_IsScope
        };

        MOCK_CLASS(MockElement, Base)
            MockElement(std::wstring accessKey, MockElement* p = nullptr, ScopeFlags f = ScopeFlags_None, MockElement* owner = nullptr)
                : accessKey(accessKey)
                , parent(p)
                , scopeOwner(owner)
            {
                VERIFY_SUCCEEDED(xstring_ptr::CloneBuffer(accessKey.c_str(), &key));

                Expect(*this, Release)
                    .ReturnValue(0);
                Expect(*this, OfTypeByIndex)
                    .ReturnValue(true);
                Expect(*this, Release)
                    .ReturnValue(0);
                Expect(*this, OfTypeByIndex)
                    .ReturnValue(true);
                Expect(*this, IsActive)
                    .ReturnValue(true);
                Expect(*this, IsVisible)
                    .ReturnValue(true);
                Expect(*this, IsEnabled)
                    .ReturnValue(true);
                Expect(*this, AreAllAncestorsVisible)
                    .ReturnValue(true);

                if ((f & ScopeFlags_IsScope) != 0)
                {
                    isScope = true;
                }
                if (p)
                {
                    p->AddChild(this);
                }
            }

            static MockElement CreateInvalid()
            {
                MockElement m(L"INVALID");
                m.isValid = false;
                return m;
            }

            HRESULT GetValueByIndex(KnownPropertyIndex, CValue* value) const
            {
                ASSERT(isValid);
                Expect(*value, AsString)
                    .ReturnValue(key);
                return S_OK;
            }

            void SetCollection(TreeAnalyzerUnitTests::MockCollection<MockElement>* collection)
            {
                ASSERT(isValid);
                m_collection = collection;
            }

            TreeAnalyzerUnitTests::MockCollection<MockElement>* GetChildren() const
            {
                ASSERT(isValid);
                return m_collection;
            }

            MockElement* AddChild(MockElement* e)
            {
                ASSERT(isValid);
                if (!m_collection)
                {
                    m_collection = new TreeAnalyzerUnitTests::MockCollection<MockElement>();
                }
                m_collection->AddItem(e);
                e->parent = this;
                return e;
            }

            MockElement* GetParent() const
            {
                ASSERT(isValid);
                return parent;
            }

            MockElement* GetMentor() const
            {
                ASSERT(isValid);
                return nullptr;
            }

            bool IsScope() const
            {
                ASSERT(isValid);
                return isScope;
            }

            MockElement* GetScopeOwner() const
            {
                ASSERT(isValid);
                return scopeOwner;
            }

            STUB_METHOD(int, Release, 0)
            STUB_METHOD(MockElement*, GetRoot, 0)

            // Convenience for debugging
            void Print(int indent = 0)
            {
                wchar_t spaces[32] = {};


                for (int i=0;i<indent;++i)
                {
                    wcscat(spaces, L"  ");
                }

                if (indent > 10)
                {
                    LOG_OUTPUT(L"%s(too deep, truncating)",
                        spaces);
                    return;
                }

                LOG_OUTPUT(L"%s%s %s",
                    spaces,
                    key.GetBuffer(),
                    isScope ? L"IsScope" : L"");

                if (m_collection)
                {
                    for (unsigned int i=0;i<m_collection->GetCount();++i)
                    {
                        m_collection->GetItemWithAddRef(i)->Print(indent+1);
                    }
                }
            }

            bool isValid = true;
            bool isScope = false;
            MockElement* scopeOwner = nullptr;
            MockElement* parent = nullptr;
            xstring_ptr key;
            std::wstring accessKey;
            TreeAnalyzerUnitTests::MockCollection<MockElement>* m_collection = nullptr;

            template <KnownTypeIndex targetTypeIndex>
            bool OfTypeByIndex()
            {
                return OfTypeByIndex(targetTypeIndex);
            }

            STUB_METHOD(bool, OfTypeByIndex, 1(KnownTypeIndex))
            STUB_METHOD(bool, IsActive, 0)
            STUB_METHOD(bool, IsVisible, 0)
            STUB_METHOD(bool, AreAllAncestorsVisible, 0)
            STUB_METHOD(bool, IsEnabled, 0)

        END_MOCK

        MockElement root = MockElement::CreateInvalid();
        MockElement root2 = MockElement::CreateInvalid();

        // Convenience for debugging
        void PrintTree()
        {
            root.Print();
            root2.Print();
        }

        struct TreeInitHelper
        {
            TreeInitHelper()
            {
                LOG_OUTPUT(L"Initializing roots");
                root = MockElement(L"");
                root2 = MockElement(L"");
            }
            ~TreeInitHelper()
            {
                LOG_OUTPUT(L"Destroying roots");
                Verify(root);
                Verify(root2);
                root = MockElement::CreateInvalid();
                root2 = MockElement::CreateInvalid();
            }
        };

        MOCK_CLASS(MockVisualTree, Base)
            static MockElement* GetRootForElement(MockElement* element)
            {
                if (element == nullptr)
                {
                    return &root;
                }
                return element->GetRoot();
            }

            static void GetAllVisibleRootsNoRef(_Out_writes_(3) MockElement** roots)
            {
                roots[0] = &root;
                roots[1] = &root2;
                roots[2] = nullptr;
            }
        END_MOCK

        MOCK_CLASS(MockVisualTreeLibrary, Base)

            TreeAnalyzerUnitTests::MockCollection<MockElement>* GetChildren(_In_ MockElement* const element)
            {
                return element->GetChildren();
            }

            MockElement* GetParent(_In_ MockElement* const element)
            {
                return element->GetParent();
            }

            bool IsScope(_In_ MockElement* element)
            {
                return element->IsScope();
            }

            MockElement* GetScopeOwner(_In_ const MockElement* element)
            {
                return element->GetScopeOwner();
            }

            void GetAllVisibleRootsNoRef(_Out_writes_(3) MockElement** roots)
            {
                MockVisualTree::GetAllVisibleRootsNoRef(roots);
            }
        END_MOCK

        typedef AKTreeAnalyzer<
            TreeAnalyzerUnitTests::MockCollection<MockElement>,
            MockVisualTree,
            MockVisualTreeLibrary,
            MockElement> TreeAnalyzer;

        void TreeAnalyzerUnitTests::AllElementsWithAKFound()
        {
            TreeInitHelper tih;
            MockElement elementA(L"M");
            MockElement elementB(L"A");
            MockElement elementC(L"T");
            MockElement elementD(L"H");

            Expect(elementA, IsActive)
                .ReturnValue(true);
            Expect(elementA, IsVisible)
                .ReturnValue(true);
            Expect(elementB, IsActive)
                .ReturnValue(true);
            Expect(elementB, IsVisible)
                .ReturnValue(true);
            Expect(elementC, IsActive)
                .ReturnValue(true);
            Expect(elementC, IsVisible)
                .ReturnValue(true);
            Expect(elementD, IsActive)
                .ReturnValue(true);
            Expect(elementD, IsVisible)
                .ReturnValue(true);

            std::vector<MockElement*> elementList;
            elementList.push_back(&elementA);
            elementList.push_back(&elementB);
            elementList.push_back(&elementC);
            elementList.push_back(&elementD);

            MockCollection<MockElement> collection;
            collection.SetCollection(elementList);

            root.SetCollection(&collection);

            MockVisualTreeLibrary treeLibrary;

            TreeAnalyzer tree(treeLibrary);

            std::vector<MockElement*> akList;
            VERIFY_SUCCEEDED(tree.FindElementsForAK(nullptr, akList));

            VERIFY_IS_TRUE(std::is_permutation(elementList.begin(), elementList.end(), akList.begin()));
            VERIFY_EXPECTATIONS(treeLibrary);
        }

        void TreeAnalyzerUnitTests::ElementsWithInvalidAKAreFiltered()
        {
            TreeInitHelper tih;
            MockElement elementA(L"M");
            MockElement elementB(L"A");
            MockElement elementC(L"T");
            MockElement elementD(L"");

            Expect(elementA, IsActive)
                .ReturnValue(true);
            Expect(elementA, IsVisible)
                .ReturnValue(true);
            Expect(elementB, IsActive)
                .ReturnValue(true);
            Expect(elementB, IsVisible)
                .ReturnValue(true);
            Expect(elementC, IsActive)
                .ReturnValue(true);
            Expect(elementC, IsVisible)
                .ReturnValue(true);
            Expect(elementD, IsActive)
                .ReturnValue(true);
            Expect(elementD, IsVisible)
                .ReturnValue(true);

            std::vector<MockElement*> elementList;
            elementList.push_back(&elementA);
            elementList.push_back(&elementB);
            elementList.push_back(&elementC);
            elementList.push_back(&elementD);

            MockCollection<MockElement> collection;
            collection.SetCollection(elementList);

            root.SetCollection(&collection);

            MockVisualTreeLibrary treeLibrary;

            TreeAnalyzer tree(treeLibrary);
            std::vector<MockElement*> akList;
            VERIFY_SUCCEEDED(tree.FindElementsForAK(nullptr, akList));

            std::vector<MockElement*> targetList;
            targetList.push_back(&elementA);
            targetList.push_back(&elementB);
            targetList.push_back(&elementC);

            VERIFY_IS_TRUE(std::is_permutation(akList.begin(), akList.end(), targetList.begin()));
        }

        void TreeAnalyzerUnitTests::EmptyVectorReturnedIfNoValidAK()
        {
            TreeInitHelper tih;
            MockElement elementA(L"");
            MockElement elementB(L"");
            MockElement elementC(L"");
            MockElement elementD(L"");

            Expect(elementA, IsActive)
                .ReturnValue(true);
            Expect(elementA, IsVisible)
                .ReturnValue(true);
            Expect(elementB, IsActive)
                .ReturnValue(true);
            Expect(elementB, IsVisible)
                .ReturnValue(true);
            Expect(elementC, IsActive)
                .ReturnValue(true);
            Expect(elementC, IsVisible)
                .ReturnValue(true);
            Expect(elementD, IsActive)
                .ReturnValue(true);
            Expect(elementD, IsVisible)
                .ReturnValue(true);

            std::vector<MockElement*> elementList;
            elementList.push_back(&elementA);
            elementList.push_back(&elementB);
            elementList.push_back(&elementC);
            elementList.push_back(&elementD);

            MockCollection<MockElement> collection;
            collection.SetCollection(elementList);

            root.SetCollection(&collection);

            MockVisualTreeLibrary treeLibrary;

            TreeAnalyzer tree(treeLibrary);

            std::vector<MockElement*> akList;
            VERIFY_SUCCEEDED(tree.FindElementsForAK(nullptr, akList));


            VERIFY_IS_TRUE(akList.empty());
        }

        void TreeAnalyzerUnitTests::AllElementsWithMultipleLeaves()
        {
            TreeInitHelper tih;
            MockElement elementA(L"M");
            MockElement elementB(L"A");
            MockElement elementC(L"T");
            MockElement elementD(L"H");
            MockElement elementE(L"I");

            Expect(elementA, IsActive)
                .ReturnValue(true);
            Expect(elementA, IsVisible)
                .ReturnValue(true);
            Expect(elementB, IsActive)
                .ReturnValue(true);
            Expect(elementB, IsVisible)
                .ReturnValue(true);
            Expect(elementC, IsActive)
                .ReturnValue(true);
            Expect(elementC, IsVisible)
                .ReturnValue(true);
            Expect(elementD, IsActive)
                .ReturnValue(true);
            Expect(elementD, IsVisible)
                .ReturnValue(true);

            std::vector<MockElement*> elementList;
            elementList.push_back(&elementA);
            elementList.push_back(&elementB);
            elementList.push_back(&elementC);
            elementList.push_back(&elementD);

            MockCollection<MockElement> collection;
            collection.SetCollection(elementList);

            root.SetCollection(&collection);

            std::vector<MockElement*> elementListB;
            elementListB.push_back(&elementE);

            MockCollection<MockElement> collectionB;
            collectionB.SetCollection(elementListB);

            elementD.SetCollection(&collectionB);

            MockVisualTreeLibrary treeLibrary;

            TreeAnalyzer tree(treeLibrary);
            std::vector<MockElement*> akList;
            VERIFY_SUCCEEDED(tree.FindElementsForAK(nullptr, akList));


            std::vector<MockElement*> targetList;
            targetList.push_back(&elementA);
            targetList.push_back(&elementB);
            targetList.push_back(&elementC);
            targetList.push_back(&elementD);
            targetList.push_back(&elementE);

            VERIFY_IS_TRUE(std::is_permutation(akList.begin(), akList.end(), targetList.begin()));
        }

        void TreeAnalyzerUnitTests::AllElementsAreFoundEvenWithInvalidNodes()
        {
            TreeInitHelper tih;
            MockElement elementA(L"M");
            MockElement elementB(L"A");
            MockElement elementC(L"T");
            MockElement elementD(L"");
            MockElement elementE(L"I");

            Expect(elementA, IsActive)
                .ReturnValue(true);
            Expect(elementA, IsVisible)
                .ReturnValue(true);
            Expect(elementB, IsActive)
                .ReturnValue(true);
            Expect(elementB, IsVisible)
                .ReturnValue(true);
            Expect(elementC, IsActive)
                .ReturnValue(true);
            Expect(elementC, IsVisible)
                .ReturnValue(true);
            Expect(elementD, IsActive)
                .ReturnValue(true);
            Expect(elementD, IsVisible)
                .ReturnValue(true);

            std::vector<MockElement*> elementList;
            elementList.push_back(&elementA);
            elementList.push_back(&elementB);
            elementList.push_back(&elementC);
            elementList.push_back(&elementD);

            MockCollection<MockElement> collection;
            collection.SetCollection(elementList);

            root.SetCollection(&collection);

            std::vector<MockElement*> elementListB;
            elementListB.push_back(&elementE);

            MockCollection<MockElement> collectionB;
            collectionB.SetCollection(elementListB);

            elementD.SetCollection(&collectionB);

            MockVisualTreeLibrary treeLibrary;

            TreeAnalyzer tree(treeLibrary);
            std::vector<MockElement*> akList;
            VERIFY_SUCCEEDED(tree.FindElementsForAK(nullptr, akList));

            std::vector<MockElement*> targetList;
            targetList.push_back(&elementA);
            targetList.push_back(&elementB);
            targetList.push_back(&elementC);
            targetList.push_back(&elementE);

            VERIFY_IS_TRUE(std::is_permutation(akList.begin(), akList.end(), targetList.begin()));
        }


        void TreeAnalyzerUnitTests::FindScopeRootOnBasicTree()
        {
            TreeInitHelper tih;
            MockElement r(L"R", &root);
            MockElement m   (L"M", &r);
            MockElement a   (L"A", &r);
            MockElement t   (L"T", &r);
            MockElement q       (L"Q", &t);
            MockElement w       (L"W", &t);
            MockElement h   (L"H", &r);

            MockVisualTreeLibrary treeLibrary;

            TreeAnalyzer tree(treeLibrary);

            VERIFY_IS_NULL(tree.GetScopeOwner(&q));
            VERIFY_IS_NULL(tree.GetScopeOwner(&w));
            VERIFY_IS_NULL(tree.GetScopeOwner(&h));
        }

        void TreeAnalyzerUnitTests::FindScopeRootOnTreeWithScopeOwner()
        {
            TreeInitHelper tih;
            MockElement r(L"R", &root, ScopeFlags_IsScope);
            MockElement m   (L"M", &r);
            MockElement a   (L"A", &r);
            MockElement t   (L"T", &r, ScopeFlags_IsScope);
            MockElement q       (L"Q", &t);
            MockElement w       (L"W", &t);
            MockElement h   (L"H", &r);

            MockVisualTreeLibrary treeLibrary;

            TreeAnalyzer tree(treeLibrary);

            VERIFY_ARE_EQUAL(tree.GetScopeOwner(&q), &t);
            VERIFY_ARE_EQUAL(tree.GetScopeOwner(&w), &t);
            VERIFY_ARE_EQUAL(tree.GetScopeOwner(&h), &r);
            VERIFY_ARE_EQUAL(tree.GetScopeOwner(&r), nullptr);
            VERIFY_ARE_EQUAL(tree.GetScopeOwner(&t), &r);
        }


        void TreeAnalyzerUnitTests::FindScopeRootOnTreesWithOwner()
        {
            TreeInitHelper tih;
            MockElement r(L"R", &root);
            MockElement m   (L"M", &r);
            MockElement a   (L"A", &r);
            MockElement t   (L"T", &r, ScopeFlags_IsScope);
            MockElement q       (L"Q", &t);
            MockElement w       (L"W", &t);
            MockElement h   (L"H", &r);

            MockElement z(L"Z", &root2, ScopeFlags_None, &w);
            MockElement x   (L"X", &z);
            MockElement c   (L"C", &z);

            MockVisualTreeLibrary treeLibrary;

            TreeAnalyzer tree(treeLibrary);

            VERIFY_ARE_EQUAL(tree.GetScopeOwner(&z), &t);
            VERIFY_ARE_EQUAL(tree.GetScopeOwner(&x), &t);
        }


        void TreeAnalyzerUnitTests::ValidateScopeOwnerMap()
        {
            TreeInitHelper tih;
            MockElement r(L"R", &root);
            MockElement m   (L"M", &r);
            MockElement a   (L"A", &r);
            MockElement t   (L"T", &r, ScopeFlags_IsScope);
            MockElement q       (L"Q", &t);
            MockElement w       (L"W", &t);
            MockElement h   (L"H", &r);

            MockElement z(L"Z", &root2);
            MockElement x   (L"X", &z);
            MockElement c   (L"C", &z);

            MockVisualTreeLibrary treeLibrary;

            TreeAnalyzer tree(treeLibrary);

            {
                std::vector<std::pair<MockElement*,MockElement*>> map;
                VERIFY_SUCCEEDED(tree.BuildScopeOwnerMap(map));
                VERIFY_ARE_EQUAL(static_cast<size_t>(0), map.size());
            }

            LOG_OUTPUT(L"Setting z's scopeOwner to w, and t as isScope=true");
            z.scopeOwner = &t;
            t.isScope = true;

            {
                std::vector<std::pair<MockElement*,MockElement*>> map;
                VERIFY_SUCCEEDED(tree.BuildScopeOwnerMap(map));
                VERIFY_ARE_EQUAL(static_cast<size_t>(1), map.size());
                VERIFY_ARE_EQUAL(map[0].first, &t);
                VERIFY_ARE_EQUAL(map[0].second, &z);
            }
        }

        void TreeAnalyzerUnitTests::FindAKsInMultipleTrees()
        {
            TreeInitHelper tih;
            MockElement r(L"R", &root);
            MockElement m   (L"M", &r);
            MockElement a   (L"A", &r);
            MockElement t   (L"T", &r);
            MockElement q       (L"Q", &t);
            MockElement w       (L"W", &t);
            MockElement h   (L"H", &r);

            MockElement z(L"Z", &root2);
            MockElement x   (L"X", &z);
            MockElement c   (L"C", &z);

            MockVisualTreeLibrary treeLibrary;

            TreeAnalyzer tree(treeLibrary);

            std::vector<MockElement*> keyList;
            VERIFY_SUCCEEDED(tree.FindElementsForAK(nullptr, keyList));
            VERIFY_ARE_EQUAL(static_cast<size_t>(10), keyList.size());

            LOG_OUTPUT(L"Setting z's scopeOwner to w");
            z.scopeOwner = &t;
            t.isScope = true;

            keyList.clear();
            VERIFY_SUCCEEDED(tree.FindElementsForAK(&t, keyList));
            VERIFY_ARE_EQUAL(static_cast<size_t>(5), keyList.size());

            MockElement* expected[] = {&x, &q, &w, &z, &c};
            VERIFY_IS_TRUE(std::is_permutation(keyList.begin(), keyList.end(), expected));

        }

        void TreeAnalyzerUnitTests::ScopeCycle()
        {
            TreeInitHelper tih;
            MockElement r(L"R", &root);
            MockElement m   (L"M", &r);
            MockElement a   (L"A", &r);
            MockElement t   (L"T", &r);
            MockElement q       (L"Q", &t);
            MockElement w       (L"W", &t);
            MockElement h   (L"H", &r);

            MockElement z(L"Z", &root2);
            MockElement x   (L"X", &z);
            MockElement c   (L"C", &z);

            MockVisualTreeLibrary treeLibrary;

            TreeAnalyzer tree(treeLibrary);

            std::vector<MockElement*> keyList;
            VERIFY_SUCCEEDED(tree.FindElementsForAK(nullptr, keyList));
            VERIFY_ARE_EQUAL(static_cast<size_t>(10), keyList.size());

            LOG_OUTPUT(L"Setting z's scopeOwner to w, r's scopeOwner to c");
            z.scopeOwner = &w;
            r.scopeOwner = &c;

            failFastCalls = 0;
            VERIFY_IS_NULL(tree.GetScopeOwner(&x));
            VERIFY_IS_LESS_THAN(0, failFastCalls);
        }


        void TreeAnalyzerUnitTests::ScopeStopsAtBranch()
        {
            TreeInitHelper tih;
            MockElement z(L"Z", &root2, ScopeFlags_IsScope);
            MockElement x   (L"X", &z);
            MockElement c   (L"C", &z);

            MockElement r(L"R", &root);
            MockElement m   (L"M", &r);
            MockElement a   (L"A", &r);
            MockElement t   (L"T", &r, ScopeFlags_None, &z);
            MockElement q       (L"Q", &t);
            MockElement w       (L"W", &t);
            MockElement h   (L"H", &r);


            MockVisualTreeLibrary treeLibrary;

            TreeAnalyzer tree(treeLibrary);

            std::vector<MockElement*> keyList;
            VERIFY_SUCCEEDED(tree.FindElementsForAK(nullptr, keyList));

            MockElement* expected[] = {&r, &m, &a, &h, &z};
            VERIFY_IS_TRUE(std::is_permutation(keyList.begin(), keyList.end(), expected));
        }


        void TreeAnalyzerUnitTests::TreesWithMultipleParentPointers()
        {
            TreeInitHelper tih;
            MockElement r(L"R", &root);
            MockElement m   (L"M", &r);
            MockElement a   (L"A", &r);
            MockElement t   (L"T", &r, ScopeFlags_IsScope);
            MockElement q       (L"Q", &t);
            MockElement w       (L"W", &t);
            MockElement h   (L"H", &r);

            MockElement z(L"Z", &root2, ScopeFlags_None, &t);
            MockElement x   (L"X", &z, ScopeFlags_None, &t);
            MockElement c   (L"C", &z);

            MockVisualTreeLibrary treeLibrary;

            TreeAnalyzer tree(treeLibrary);

            std::vector<MockElement*> keyList;
            VERIFY_SUCCEEDED(tree.FindElementsForAK(&t, keyList));

            MockElement* expected[] = {&q, &w, &z, &x, &c};
            VERIFY_ARE_EQUAL(_countof(expected), keyList.size());
            VERIFY_IS_TRUE(std::is_permutation(keyList.begin(), keyList.end(), expected));
        }


        void TreeAnalyzerUnitTests::ScopeOwnerMustBeScope()
        {
            TreeInitHelper tih;
            MockElement r(L"R", &root);
            MockElement m   (L"M", &r);
            MockElement a   (L"A", &r);
            MockElement t   (L"T", &r);
            MockElement q       (L"Q", &t);
            MockElement w       (L"W", &t);
            MockElement h   (L"H", &r);

            MockElement z(L"Z", &root2);
            MockElement x   (L"X", &z);
            MockElement c   (L"C", &z);

            MockVisualTreeLibrary treeLibrary;

            TreeAnalyzer tree(treeLibrary);
            std::vector<MockElement*> keyList;

            LOG_OUTPUT(L"Can only call FindElementsForAK for scopes");
            VERIFY_FAILED(tree.FindElementsForAK(&q, keyList));

            LOG_OUTPUT(L"Tree walk should fail, because z's scopeOwner is set to q, which has isScope=false");
            z.scopeOwner = &q;

            std::vector<std::pair<MockElement*,MockElement*>> scopeOwnerMap;
            VERIFY_FAILED(tree.BuildScopeOwnerMap(scopeOwnerMap));

            LOG_OUTPUT(L"Tree walk should succeed now, because q now has isScope=true");
            q.isScope = true;
            VERIFY_SUCCEEDED(tree.FindElementsForAK(&q, keyList));
        }

    }
}}}}
