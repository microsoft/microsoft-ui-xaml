// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Imaging {

        class WicServiceUnitTests : public WEX::TestClass<WicServiceUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(WicServiceUnitTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)

            BEGIN_TEST_METHOD(JpegMetadata)
                TEST_METHOD_PROPERTY(L"Description", L"Tests retrieving the image metadata from a JPEG.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(JpegOrientation180)
                TEST_METHOD_PROPERTY(L"Description", L"Tests retrieving the image metadata from a JPEG that has EXIF data indicating a 180 degree rotation.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PngMetadata)
                TEST_METHOD_PROPERTY(L"Description", L"Tests retrieving the image metadata from a PNG.")
            END_TEST_METHOD()
                
            BEGIN_TEST_METHOD(BmpMetadata)
                TEST_METHOD_PROPERTY(L"Description", L"Tests retrieving the image metadata from a BMP.")
            END_TEST_METHOD()
                
            BEGIN_TEST_METHOD(AnimatedGifMetadata)
                TEST_METHOD_PROPERTY(L"Description", L"Tests retrieving the image metadata from an animated GIF.")
            END_TEST_METHOD()
                
            BEGIN_TEST_METHOD(BadImage)
                TEST_METHOD_PROPERTY(L"Description", L"Tests loading a bad image and verifying it fails in an expected way.")
            END_TEST_METHOD()

        private:
            static void ValidateFileMetadata(
                const WEX::Common::String& fileName,
                const GUID& expectedContainerGuid,
                uint32_t expectedWidth,
                uint32_t expectedHeight);
        };
    }}
} } } }
