// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace DependencyObject {

        class ThemingUnitTests : public WEX::TestClass<ThemingUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(ThemingUnitTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_CLASS_PROPERTY(L"FeatureUnderTest", L"@ClassName = 'CDependencyObject' AND @FunctionName = 'Theming*")
            END_TEST_CLASS()

            TEST_METHOD(DoesNotifyThemeChangeForObjectSetOnSparseProperty)
        };
    }}
} } } }
