// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { 
    namespace AccessKeys {

        class BasicModeContainerUnitTests : public WEX::TestClass<BasicModeContainerUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(BasicModeContainerUnitTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)

            TEST_METHOD(SmokeTest)
            TEST_METHOD(EnterViaKeyPress)

            BEGIN_TEST_METHOD(ShouldEvaluateProperMessage)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we mark a message as 'evaluate' when the proper scenarios are met")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(EnterViaHotkey)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that if we input the combination ALT + key, we invoke as a hotkey")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyMultipleHotkeyPresses)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we can invoke multiple hotkeys while holding down alt")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(HotkeyShouldNotEnterAKMode)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that when we invoke a hotkey, we do not enter ak mode and invoke other ako's")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(InvalidResultIsReported)
                TEST_METHOD_PROPERTY(L"Description", L"An invalid HRESULT is reported in 'evaluate'")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TabShouldExitAKMode)
                TEST_METHOD_PROPERTY(L"Description", L"Tab should not be processed, as well as exit ak mode")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DirectionKeysShouldExitAKMode)
                TEST_METHOD_PROPERTY(L"Description", L"Direction keys should not be processed, as well as exit ak mode")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(EscapeKeyNotHandledWithModifiers)
                TEST_METHOD_PROPERTY(L"Description", L"Tests that we do not handle escape key + <modifier>")
            END_TEST_METHOD()

        };

    }

} } } }
