// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls {

class CalendarLayoutStrategyUnitTests : public WEX::TestClass<CalendarLayoutStrategyUnitTests>
{
public:
    BEGIN_TEST_CLASS(CalendarLayoutStrategyUnitTests)
        TEST_METHOD_PROPERTY(L"Classification", L"Integration")
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_CLASS()

    #pragma region ILayoutStrategy.GetElementMeasureSize validation.
    BEGIN_TEST_METHOD(CanCalculateMeasureSizeForContainer)
        TEST_METHOD_PROPERTY(L"Description", L"Validates GetElementMeasureSize behavior for containers.")
    END_TEST_METHOD()
    #pragma endregion

    #pragma region ILayoutStrategy.GetElementBounds validation.
    BEGIN_TEST_METHOD(CanCalculateBoundsForContainer)
        TEST_METHOD_PROPERTY(L"Description", L"Validates GetElementBounds behavior for containers.")
    END_TEST_METHOD()
    #pragma endregion

    #pragma region ILayoutStrategy.GetElementArrangeBounds validation.
    BEGIN_TEST_METHOD(CanCalculateElementArrangeBounds)
        TEST_METHOD_PROPERTY(L"Description", L"Validates GetElementArrangeBounds behavior for containers and headers.")
    END_TEST_METHOD()
    #pragma endregion
  
    #pragma region ILayoutStrategy.HasSnapPointOnElement validation.
    BEGIN_TEST_METHOD(CanHasSnapPointOnElement)
        TEST_METHOD_PROPERTY(L"Description", L"Validates HasSnapPointOnElement behavior.")
    END_TEST_METHOD()
    #pragma endregion

    BEGIN_TEST_METHOD(VerifyIndexCorrection)
        TEST_METHOD_PROPERTY(L"Description", L"Validates Index Correction behavior.")
    END_TEST_METHOD()

};

} } } } }
