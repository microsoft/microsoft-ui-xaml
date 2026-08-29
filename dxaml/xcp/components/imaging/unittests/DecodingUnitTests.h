// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Imaging {

        class DecodingUnitTests : public WEX::TestClass<DecodingUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(DecodingUnitTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)

            BEGIN_TEST_METHOD(Jpeg)
                TEST_METHOD_PROPERTY(L"Description", L"Tests decoding a jpeg image.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(Png)
                TEST_METHOD_PROPERTY(L"Description", L"Tests decoding a png image.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(Bmp)
                TEST_METHOD_PROPERTY(L"Description", L"Tests decoding a bmp image.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(GifOutOfRange)
                TEST_METHOD_PROPERTY(L"Description", L"Out of bounds frames are expected to be clipped.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(GifOutOfRange2)
                TEST_METHOD_PROPERTY(L"Description", L"Entirely out of bounds frames are expected to be skipped.")
            END_TEST_METHOD()

        private:
            static void ValidateImage(
                const WEX::Common::String& fileName,
                const WEX::Common::String& testIdentifier,
                uint32_t expectedCrc32
                );
        };
    }}
} } } }
