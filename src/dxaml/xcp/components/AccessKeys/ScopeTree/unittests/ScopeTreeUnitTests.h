// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WexTestClass.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { 
    namespace AccessKeys {
        class ScopeTreeUnitTests : public WEX::TestClass<ScopeTreeUnitTests>
        {
        public:          
            BEGIN_TEST_CLASS(ScopeTreeUnitTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            BEGIN_TEST_METHOD(RootScopeIsCreated)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that when we first enter ak mode, we create the root scope")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyScopeWasInvoked)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that the scope is invoked after we enter ak mode and send a character")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ScopeAcceptsMulipleInputBeforeCreatingNewScope)
                TEST_METHOD_PROPERTY(L"Description", L"If we send multiple characters to the scope, we should not build a new scope until the current scope is finished processing")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ScopeIsInvokedThroughHotkey)
                TEST_METHOD_PROPERTY(L"Description", L"If we send in the combination ALT + S, we should create the root scope and invoke the scope")
            END_TEST_METHOD()

            TEST_METHOD(ModeContainerMustBeActiveToUpdateScope) ;
            TEST_METHOD(UpdateScopeExitsAndEnters)

            BEGIN_TEST_METHOD(MnemonicsExitedOnEscapeInRootScope)
                TEST_METHOD_PROPERTY(L"Description", L"If we receive an escape character while in the root scope, and in AccessKey mode then exit the mode.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ExitingWithAltClosesKeytips)
                TEST_METHOD_PROPERTY(L"Description", L"Verify exiting mnemonics with alt calls HideAccessKeys")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(InvokeCanEnterScope)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that we can EnterScope as result of Invocation")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PropertyChangedAddsToTree)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we add to the scope when we get an appropriate PropertyChanged")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PropertyChangedRemovesFromTree)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we remove from the scope when we get an appropriate PropertyChanged")
            END_TEST_METHOD()
        };
    }
}}}}
