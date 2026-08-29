// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>
#include "palgfx.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Brushes {

class SolidColorBrushUnitTests : public WEX::TestClass<SolidColorBrushUnitTests>
{
public:
    BEGIN_TEST_CLASS(SolidColorBrushUnitTests)
    END_TEST_CLASS()

    BEGIN_TEST_METHOD(ValidateSetUpSolidColorBrushTransition)
        TEST_METHOD_PROPERTY(L"Classification", L"Integration")
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_METHOD()
};

} } } } } }
