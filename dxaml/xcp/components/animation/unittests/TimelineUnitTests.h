// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Animation {

        class TimelineUnitTests : public WEX::TestClass<TimelineUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(TimelineUnitTests)
            END_TEST_CLASS()

            BEGIN_TEST_METHOD(ValidateDurationRepeat)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDurationReverse)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDurationSpeedRatio)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDurationBeginTime)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateGetDurationWithProperties)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
        };
    }}
} } } }
