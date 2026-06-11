// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ScopeBuilder.h"
#include "ScopeBuilderUnitTests.h"
#include "DependencyLocator.h"
#include <CxxMock.h>

#include <TypeTableStructs.h>
#include <Mocks.h>

using namespace CxxMock;
using namespace AccessKeys;

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
   namespace AccessKeys {


        MOCK_CLASS(MockElement, Base)
            MockElement(std::wstring accessKey) : accessKey(accessKey)
            {
                VERIFY_SUCCEEDED(xstring_ptr::CloneBuffer(accessKey.c_str(), &key));

                Expect(*this, OfTypeByIndex)
                    .ReturnValue(true);
            }

            HRESULT GetValueByIndex(KnownPropertyIndex, CValue* value)
            {
                Expect(*value, AsString)
                    .ReturnValue(key);

                return S_OK;
            }
            
            template <KnownTypeIndex targetTypeIndex>
            bool OfTypeByIndex()
            {
                return OfTypeByIndex(targetTypeIndex);
            }

            STUB_METHOD(bool, OfTypeByIndex, 1(KnownTypeIndex))

            xstring_ptr key;
            std::wstring accessKey;
        END_MOCK

        MOCK_CLASS(MockScope, Base)
            MockScope(MockElement* element, std::vector<std::pair<MockElement*, AKAccessKey>> scopeInitList) : element(element)
            {
                for (const auto& item : scopeInitList)
                {
                    accessKeys.push_back(item.first->accessKey);
                }

                Expect(*this, SetNextScope);
            }

            STUB_METHOD(void, SetNextScope, 1(MockScope))

            MockElement* element;
            std::vector<std::wstring> accessKeys;
        END_MOCK

        void ScopeBuilderUnitTests::VerifyThatScopeIsCreated()
        {
            MOCK_CLASS(MockTreeAnalyzer, Base)
                MockTreeAnalyzer() : I(L"I"), O(L"O"), M(L"M")
                {
                    std::vector<MockElement*> elementList;
                    elementList.push_back(&I);
                    elementList.push_back(&O);
                    elementList.push_back(&M);

                    Expect(*this, FindElementsForAK)
                        .With(eq<MockElement*>(nullptr))
                        .SetOutValue<1>(elementList)
                        .ReturnValue(S_OK);
                }

                STUB_METHOD(HRESULT, FindElementsForAK, 2(MockElement* const, std::vector<MockElement*>&))

                MockElement I;
                MockElement O;
                MockElement M;
            END_MOCK

            MOCK_CLASS(MockParser, Base)
                static bool TryParseAccessKey(_In_ const std::wstring& accessString, _Inout_ AKAccessKey& accessKey)
                {
                    accessKey = accessString.at(0);
                    return true;
                }
            END_MOCK

            MockTreeAnalyzer treeAnalyzer;
            AKScopeBuilder<MockTreeAnalyzer, MockParser, MockElement, MockScope> scopeBuilder(treeAnalyzer);

            std::shared_ptr<MockScope> newScope;
            VERIFY_SUCCEEDED(scopeBuilder.ConstructScope(nullptr, newScope));

            VERIFY_IS_NULL(newScope->element);

            std::vector<MockElement> endList;
            MockElement I(L"I");
            MockElement O(L"O");
            MockElement M(L"M");

            endList.push_back(I);
            endList.push_back(O);
            endList.push_back(M);

            VerifyAccessKeysInScopeAreSame<MockElement, MockScope>(endList, newScope.get());
        }

        void ScopeBuilderUnitTests::VerifyThatNoScopeIsCreatedWhenNoAccessKeys()
        {
            MOCK_CLASS(MockTreeAnalyzer, Base)
                MockTreeAnalyzer()
                {
                    std::vector<MockElement*> elementList;

                    Expect(*this, FindElementsForAK)
                        .With(eq<MockElement*>(nullptr))
                        .SetOutValue<1>(elementList)
                        .ReturnValue(S_OK);

                }

                STUB_METHOD(HRESULT, FindElementsForAK, 2(MockElement* const, std::vector<MockElement*>&))
            END_MOCK

            MOCK_CLASS(MockParser, Base)
                static bool TryParseAccessKey(const std::wstring&, AKAccessKey& accessKey) { return true; }
            END_MOCK 

            MockTreeAnalyzer treeAnalyzer;
            AKScopeBuilder<MockTreeAnalyzer, MockParser, MockElement, MockScope> scopeBuilder(treeAnalyzer);

            std::shared_ptr<MockScope> newScope;
            VERIFY_SUCCEEDED(scopeBuilder.ConstructScope(nullptr, newScope));

            VERIFY_IS_NULL(newScope);
        }

        void ScopeBuilderUnitTests::InvalidAccessKeysIgnoredFromScope()
        {
            MOCK_CLASS(MockTreeAnalyzer, Base)
                MockTreeAnalyzer() : I(L"I"), O(L"O"), M(L"M")
            {
                std::vector<MockElement*> elementList;
                elementList.push_back(&I);
                elementList.push_back(&O);
                elementList.push_back(&M);

                Expect(*this, FindElementsForAK)
                    .With(eq<MockElement*>(nullptr))
                    .SetOutValue<1>(elementList)
                    .ReturnValue(S_OK);

            }

            STUB_METHOD(HRESULT, FindElementsForAK, 2(MockElement* const, std::vector<MockElement*>&))

                MockElement I;
                MockElement O;
                MockElement M;
            END_MOCK

            MOCK_CLASS(MockParser, Base)
                static bool TryParseAccessKey(_In_ const std::wstring& accessString, _Inout_ AKAccessKey& accessKey)
            {
                if (accessString == L"O")
                {
                    return false;
                }

                accessKey = accessString[0];

                return true;
            }
            END_MOCK

            MockTreeAnalyzer treeAnalyzer;
            AKScopeBuilder<MockTreeAnalyzer, MockParser, MockElement, MockScope> scopeBuilder(treeAnalyzer);

            std::shared_ptr<MockScope> newScope;
            VERIFY_SUCCEEDED(scopeBuilder.ConstructScope(nullptr, newScope));

            VERIFY_IS_NULL(newScope->element);

            std::vector<MockElement> endList;
            MockElement I(L"I");
            MockElement O(L"O");
            MockElement M(L"M");

            endList.push_back(I);
            endList.push_back(M);

            VerifyAccessKeysInScopeAreSame<MockElement, MockScope>(endList, newScope.get());

            bool wasOFound = std::find(newScope->accessKeys.begin(), newScope->accessKeys.end(), O.accessKey)
                != newScope->accessKeys.end();

            VERIFY_IS_FALSE(wasOFound);
        }
    }
}}}}
