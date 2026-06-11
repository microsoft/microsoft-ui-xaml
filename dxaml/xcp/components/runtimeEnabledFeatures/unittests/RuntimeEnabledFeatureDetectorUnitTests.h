// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {


        class RuntimeEnabledFeatureDetectorUnitTests : public WEX::TestClass<RuntimeEnabledFeatureDetectorUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(RuntimeEnabledFeatureDetectorUnitTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            BEGIN_TEST_METHOD(ValidateDefaultValues)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the default RuntimeFeature true and false values.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateOverride)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully override RuntimeFeatures using the APIs the test hook will call.")
            END_TEST_METHOD()
        };
    }
} } } }
