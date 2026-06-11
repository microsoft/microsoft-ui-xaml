// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Animation {

class KeySplineUnitTests : public WEX::TestClass<KeySplineUnitTests>
{
public:
    BEGIN_TEST_CLASS(KeySplineUnitTests)
    END_TEST_CLASS()

    BEGIN_TEST_METHOD(ValidateGetSplineProgress)
        TEST_METHOD_PROPERTY(L"Classification", L"Integration")
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
#if defined(_WIN64) // Disabled on x64 - spline interpolation gives different results.
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
#endif
    END_TEST_METHOD()
};

} } } } } }
