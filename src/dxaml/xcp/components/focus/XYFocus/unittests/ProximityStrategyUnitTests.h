// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WexTestClass.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { 
    namespace Xaml { namespace Focus { namespace XYFocus {
        class ProximityStrategyUnitTests : public WEX::TestClass<ProximityStrategyUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(ProximityStrategyUnitTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            BEGIN_TEST_METHOD(VerifyProximityStrategyClosestToAxis)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that when we use ClosestToAxis mode, the closest element in the specified direction is chosen")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyProximityStrategyClosestToAxisWithExtremeDistance)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that when we use ClosestToAxis mode, the closest element in the specified direction is chosen, even when the secondary axis distance is much larger")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyProximityStrategyNearness)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that when we use Nearness mode, we use both the primary and secondary axis to score the element")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyNearnessMeasuresShadow)
                TEST_METHOD_PROPERTY(L"Description", L"As long as elements are within the shadow of the element, we do not use the secondary axis when comparing elements")
            END_TEST_METHOD()
        };
    }}}
}}}}
