// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WexTestClass.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { 
    namespace AccessKeys {
        class ScopeBuilderUnitTests : public WEX::TestClass<ScopeBuilderUnitTests>
        {
        public:          
            BEGIN_TEST_CLASS(ScopeBuilderUnitTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            BEGIN_TEST_METHOD(VerifyThatScopeIsCreated)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that a scope is built with the mapping of Elements to AKs")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyThatNoScopeIsCreatedWhenNoAccessKeys)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that in the scenario where there are no access keys, the scope is not created")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(InvalidAccessKeysIgnoredFromScope)
                TEST_METHOD_PROPERTY(L"Description", L"If getting the AccessKey fails, we should not add the element/ak pair to the scope")
            END_TEST_METHOD()

        private:
            template<class MockElement, class MockScope>
            void VerifyAccessKeysInScopeAreSame(_In_ std::vector<MockElement>& akList, _In_ const MockScope* const scope)
            {
                for (auto& element : akList)
                {
                    bool found = std::find(scope->accessKeys.begin(), scope->accessKeys.end(), element.accessKey)
                        != scope->accessKeys.end();
                    VERIFY_IS_TRUE(found);
                }
            }
        };
    }
}}}}
