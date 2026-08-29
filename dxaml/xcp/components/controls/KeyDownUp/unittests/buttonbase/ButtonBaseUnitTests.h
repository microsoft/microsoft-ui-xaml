// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WexTestClass.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { 
    namespace Controls {

        class ButtonBaseUnitTests : public WEX::TestClass<ButtonBaseUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(ButtonBaseUnitTests)
            END_TEST_CLASS()

            BEGIN_TEST_METHOD(ValidateKeyDownWithNavigationOrGamepad)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the key down processes correctly with gamepad as the trigger")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateKeyDown)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the key down processes correctly with the keyboard as the trigger")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateOnClickCalledKeyDown)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the onlick method is called")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateOnClickCalledKeyDownNavigationOrGamepad)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the onlick method is called")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateKeyUpWithNavigationOrGamepad)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the key up processes correctly with gamepad as the trigger")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateKeyUp)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the key up processes correctly with the keyboard as the trigger")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateOnClickCalledKeyUp)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the onlick method is called")
                END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateOnClickCalledKeyUpNavigationOrGamepad)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the onlick method is called")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidatePressUnsupportedKeyDownUp)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the key does not trigger another press if it is still being pressed")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidatePressUnsupportedKeyDownUpNavigationOrGamepad)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the key does not trigger another press if it is still being pressed")
            END_TEST_METHOD()
        }; 
    }
}}}}
