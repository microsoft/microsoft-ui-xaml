// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { 
    namespace Controls { namespace Pivot {

        class PivotStateMachineUnitTests : public WEX::TestClass<PivotStateMachineUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(PivotStateMachineUnitTests)
                END_TEST_CLASS()

                BEGIN_TEST_METHOD(ValidateInitialization)
                    TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                    TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                    TEST_METHOD_PROPERTY(L"Description", L"Validates the correct events are fired when the state machine is initialized.")
                END_TEST_METHOD()

                BEGIN_TEST_METHOD(ValidateCallingMeasureAndArrangeWithUnequalValues)
                    TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                    TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                    TEST_METHOD_PROPERTY(L"Description", L"Validates that the state machine goes to the correct state when Measure and Arrange are called with different values.")
                END_TEST_METHOD()
        };

    } } 
} } } }
