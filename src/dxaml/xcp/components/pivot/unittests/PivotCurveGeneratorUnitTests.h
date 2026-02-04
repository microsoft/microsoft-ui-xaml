// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { 
    namespace Controls { namespace Pivot {

        class PivotCurveGeneratorUnitTests : public WEX::TestClass<PivotCurveGeneratorUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(PivotCurveGeneratorUnitTests)
            END_TEST_CLASS()

            BEGIN_TEST_METHOD(ValidateZeroItemCurve)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Description", L"Validates the generated curves are correct when the item count is zero.")
            END_TEST_METHOD()
        };

    } } 
} } } }
