// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Primitives { namespace Color {

    class ColorIntegrationTests : public WEX::TestClass<ColorIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(ColorIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"2b4b21d5-e5c0-4fad-bed4-62030fe191a1;eadeac67-1552-4876-a67e-dc1fcb8a6e25;bdc5c68b-03c1-4217-832c-4c09f99946f4")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanRetrieveColorNames)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully retrieve the expected named colors.")
        END_TEST_METHOD()

    private:
        void ValidateColorNameFromHslRange(double hMin, double hMax, double sMin, double sMax, double lMin, double lMax, Platform::String^ expectedName);
        void ValidateColorNameFromHsl(double h, double s, double l, Platform::String^ expectedName);
        ::Windows::UI::Color ColorFromHsl(double h, double s, double l);
    };

} } } } } } }
