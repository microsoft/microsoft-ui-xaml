// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Theming {

    class FrameworkThemingUnitTests : public WEX::TestClass<FrameworkThemingUnitTests>
    {
    public:
        BEGIN_TEST_CLASS(FrameworkThemingUnitTests)
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_CLASS()

        BEGIN_TEST_METHOD(ValidateGetTheme)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that GetTheme() returns the correct combination of requested theme,\n"
                                                 L"high-constrast theme, and system theme.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateNotifyThemeChangedHandlerInvoked)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the notify theme changed handler is invoked")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateColorAndBrushResources)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that all the necessary color & brush resources are created on a theme change.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanDisableHighContrast)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that high-contrast can be disabled.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesInitialThemeMatchSystemTheme)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that new instances of FrameworkTheming report a theme that matches the system theme.")
        END_TEST_METHOD()

    }; // class FrameworkThemingUnitTests

} } } } } } // namespace ::Windows::UI::Xaml::Tests::Controls::Theming
