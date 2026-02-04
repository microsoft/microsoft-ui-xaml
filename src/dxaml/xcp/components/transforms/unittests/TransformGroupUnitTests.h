// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Transforms {

class TransformGroupUnitTestsWUC : public WEX::TestClass<TransformGroupUnitTestsWUC>
{
public:
    BEGIN_TEST_CLASS(TransformGroupUnitTestsWUC)
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_CLASS()

    TEST_METHOD(ValidateWinRTExpression)

    BEGIN_TEST_METHOD(ValidateDeviceLostCleanupWUC)
    END_TEST_METHOD()
};

} } } } } }
