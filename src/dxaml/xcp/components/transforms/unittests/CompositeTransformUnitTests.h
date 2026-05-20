// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Transforms {

class CompositeTransformUnitTestsWUC : public WEX::TestClass<CompositeTransformUnitTestsWUC>
{
public:
    BEGIN_TEST_CLASS(CompositeTransformUnitTestsWUC)
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_CLASS()

    BEGIN_TEST_METHOD(ValidateWinRTExpression_Empty)
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateWinRTExpression_OneProperty)
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateWinRTExpression_PropertyCombinations)
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateDeviceLostCleanupWUC)
    END_TEST_METHOD()
};

} } } } } }
