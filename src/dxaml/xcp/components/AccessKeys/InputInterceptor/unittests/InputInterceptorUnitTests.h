// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WexTestClass.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { 
    namespace AccessKeys {
        class InputInterceptorUnitTests : public WEX::TestClass<InputInterceptorUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(InputInterceptorUnitTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            BEGIN_TEST_METHOD(VerifyHandledWithCorrectMessage)
                TEST_METHOD_PROPERTY(L"Description", L"When valid input is sent through the interceptor, we should mark the message as being handled")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyNotHandledWithIncorrectMessage)
                TEST_METHOD_PROPERTY(L"Description", L"When invalid input is sent through the interceptor, we should mark the message as not being handled")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(InputIsAcceptedWhenInAKMode)
                TEST_METHOD_PROPERTY(L"Description", L"After we press alt, we should handle a single message input since we are in AK mode")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MultipleInputIsAcceptedWhenInAKMode)
                TEST_METHOD_PROPERTY(L"Description", L"After we press alt, we should handle all input since we are in AK mode")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AccessKeyNotHandledWhenAltPressedTwice)
                TEST_METHOD_PROPERTY(L"Description", L"Pressing Alt once enters AK mode. Pressing it again exits it. One we exit, we should not process the input")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyPressingRightAltDoesNotEnterAKMode)
                TEST_METHOD_PROPERTY(L"Description", L"We should not be able to enter AK mode when the right alt is pressed")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ExceptionalResultIsReported)
                TEST_METHOD_PROPERTY(L"Description", L"An invalid HResult is reported")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(EscapeKeyNotHandled)
                TEST_METHOD_PROPERTY(L"Description", L"Tests that we do not handle escape key when it comes with no modifiers.  Allows UI to handle an escape key prior to accesskey framework.")
            END_TEST_METHOD()
        };
    }
}}}}
