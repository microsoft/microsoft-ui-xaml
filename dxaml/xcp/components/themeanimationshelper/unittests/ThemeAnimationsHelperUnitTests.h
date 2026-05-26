// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

#include <ThemeAnimationsHelper.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace ThemeAnimationsHelper {

    class ThemeAnimationsHelperUnitTests : public WEX::TestClass<ThemeAnimationsHelperUnitTests>
    {
    public:
        BEGIN_TEST_CLASS(ThemeAnimationsHelperUnitTests)
        END_TEST_CLASS()

        BEGIN_TEST_METHOD(ValidateGetLVIPPointerDownThemeAnimationTransformGroup)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates the static function GetLVIPPointerDownThemeAnimationTransformGroup is functioning properly.")
        END_TEST_METHOD()

    private:
    };

} } } } } } 
