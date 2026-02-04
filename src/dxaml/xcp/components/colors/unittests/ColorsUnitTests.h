// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Colors {

    class ColorsUnitTests
    {
    public:
        BEGIN_TEST_CLASS(ColorsUnitTests)
        END_TEST_CLASS()

        BEGIN_TEST_METHOD(ValidateKnownColorNamesMatchRGBA)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates that known color name strings match the RGBA values we provide via Windows.UI.Colors")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ThrowIfColorNameDoesNotMatchAnyKnownColors)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates that exception is thrown if a color name that does not exist in known colors is used")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateColorMatchesForTrailingSpaces)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a known color can be matched even if we have trailing spaces.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateColorMatchesForLeadingSpaces)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a known color can be matched even if we have leading spaces.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateColorMatchesForTrailingAndLeadingSpaces)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a known color can be matched even if we have trailing and leading spaces.")
        END_TEST_METHOD()
    };

} } } } }
