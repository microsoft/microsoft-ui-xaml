// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <RuntimeEnabledFeatureOverride.h>
#include "FeatureFlags.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {
        class SimplePropertiesTests : public WEX::TestClass<SimplePropertiesTests>
        {
        public:
            BEGIN_TEST_CLASS(SimplePropertiesTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

#if WI_IS_FEATURE_PRESENT(Feature_XamlMotionSystemHoldbacks)
            TEST_METHOD(BasicFunctionality)
#endif
        };
    }
} } } }
