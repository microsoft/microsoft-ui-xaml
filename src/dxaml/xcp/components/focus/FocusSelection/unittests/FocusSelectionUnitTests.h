// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WexTestClass.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { 
    namespace Focus {
        class FocusSelectionUnitTests : public WEX::TestClass<FocusSelectionUnitTests>
        {
            BEGIN_TEST_CLASS(FocusSelectionUnitTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            BEGIN_TEST_METHOD(ValidateShouldUpdateFocusWhenAllowFocusOnInteractionDisabled)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the only certain scenarios should cuase the method to return false")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateNavigationDirection)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we get the correct FocusNavigationDirection")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateNavigationDirectionForKeyboardArrow)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we get can correctly tell when an key originated from an arrow key")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateTryDirectionalFocus)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that when we call TryDirectionalFocus on an scope that has Local set, we find an element")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateTryDirectionalFocusMarksUnhandled)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that when we call TryDirectionalFocus on an scope that has Local set, but we don't find a candidate, we return false")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateThatNotHandledWhenModeInherited)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that when we call TryDirectionalFocus on an scope that does not set the mode at all, we do not handle it")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateShouldNotBubbleWhenModeNone)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that when we call TryDirectionalFocus on an scope that has mode set to None, we stop bubbling")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateNotHandledWhenNotUIElement)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that when we process directional focus on something that is not a uielement (ie. hyperlink), we do not handle it")
            END_TEST_METHOD()
        };
    }
}}}}
