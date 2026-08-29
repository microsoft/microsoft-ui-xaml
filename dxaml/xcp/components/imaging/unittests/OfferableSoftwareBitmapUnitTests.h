// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Imaging {

        class OfferableSoftwareBitmapUnitTests : public WEX::TestClass<OfferableSoftwareBitmapUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(OfferableSoftwareBitmapUnitTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            BEGIN_TEST_METHOD(AllocateWrite)
                TEST_METHOD_PROPERTY(L"Description", L"Tests allocating an OfferableSoftwareBitmap and writing to the whole surface.")
            END_TEST_METHOD()
        };
    }}
} } } }
