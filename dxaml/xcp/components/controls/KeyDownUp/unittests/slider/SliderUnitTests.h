// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WexTestClass.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { 
    namespace Controls {
        class SliderUnitTests : public WEX::TestClass<SliderUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(SliderUnitTests)
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

            BEGIN_TEST_METHOD(ValidateKeyDownReversedWithNavigationOrGamepad)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the value changing logic is inverted")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateKeyDownReversed)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the value changing logic is inverted")
            END_TEST_METHOD()
        };
    }
}}}}
