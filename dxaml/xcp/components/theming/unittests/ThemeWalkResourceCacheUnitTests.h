// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Theming {

    class ThemeWalkResourceCacheUnitTests : public WEX::TestClass<ThemeWalkResourceCacheUnitTests>
    {
    public:
        BEGIN_TEST_CLASS(ThemeWalkResourceCacheUnitTests)
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_CLASS()

        TEST_METHOD(DoesCacheResourceInThemeWalk)

        TEST_METHOD(DoesNotCacheResourceOutsideThemeWalk)

        TEST_METHOD(DoesCacheResourcePerDictionary)

        TEST_METHOD(DoesCacheResourcePerSubTreeTheme)

        TEST_METHOD(DoesClearCacheAfterThemeWalk)

    }; // class ThemeWalkResourceCacheUnitTests

} } } } } } // namespace ::Windows::UI::Xaml::Tests::Controls::Theming
