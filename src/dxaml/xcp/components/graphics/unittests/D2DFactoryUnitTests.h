// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Graphics {

    class D2DFactoryUnitTests : public WEX::TestClass<D2DFactoryUnitTests>
    {
    public:
        BEGIN_TEST_CLASS(D2DFactoryUnitTests)
        END_TEST_CLASS()

        BEGIN_TEST_METHOD(Creation)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()
    };

} } } } }
