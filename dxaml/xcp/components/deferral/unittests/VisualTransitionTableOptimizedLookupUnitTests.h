// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {

        class VisualTransitionTableOptimizedLookupUnitTests : public WEX::TestClass<VisualTransitionTableOptimizedLookupUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(VisualTransitionTableOptimizedLookupUnitTests)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            TEST_METHOD(ValidateEmptyLookup)
            TEST_METHOD(ValidateDefaultGroupLookup)
            TEST_METHOD(ValidateToOnlyLookup)
            TEST_METHOD(ValidateFromOnlyLookup)
            TEST_METHOD(ValidateBothLookup)
            TEST_METHOD(ValidateExplicitExclusionLookup)
            TEST_METHOD(ValidateFallbackOnGroup)
            TEST_METHOD(ValidateScoringSystem)
        
        };
    }
} } } }
