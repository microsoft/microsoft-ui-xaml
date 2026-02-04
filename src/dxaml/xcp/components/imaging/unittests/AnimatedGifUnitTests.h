// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Imaging {

        class AnimatedGifUnitTests : public WEX::TestClass<AnimatedGifUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(AnimatedGifUnitTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)

            BEGIN_TEST_METHOD(GlobalMetadata1)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the global metadata of an animated GIF with a non-zero loop count.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(GlobalMetadata2)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the global metadata of an animated GIF with the first frame size not matching the canvas size.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DeltaFramesImages)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the animated GIF delta frame images.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DeltaFramesCrc32)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the animated GIF delta frame crc32 values.")
            END_TEST_METHOD()
                
            BEGIN_TEST_METHOD(_08bSAnim3)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the animated GIF 08bSAnim3.gif with CRC checks.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(c0)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the animated GIF disposalmethod\\c0.gif with CRC checks.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(c1)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the animated GIF disposalmethod\\c1.gif with CRC checks.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(c2)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the animated GIF disposalmethod\\c2.gif with CRC checks.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(c3)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the animated GIF disposalmethod\\c3.gif with CRC checks.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(GifTest)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the animated GIF disposalmethod\\GifTest.gif with CRC checks.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(GifTest2)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the animated GIF disposalmethod\\GifTest2.gif with CRC checks.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(LoopedTransparent)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the animated GIF loop handling looped_transparent.gif with CRC checks.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FrameDelay)
            END_TEST_METHOD()

        private:

            using Crc32List = std::vector<uint32_t>;

            using BitmapProvider = std::function<
                xref_ptr<OfferableSoftwareBitmap>(uint32_t frameIndex)
            >;

            // TODO: Create a lambda version of this
            static void ValidateAllDeltaFrameImages(
                _In_ const WEX::Common::String& fileName,
                _In_ const WEX::Common::String& imageName
                );

            static void ValidateAllBitmaps(
                _In_ const wrl::ComPtr<IWICBitmapDecoder>& spWicBitmapDecoder,
                _In_ BitmapProvider bitmapProvider,
                _In_ const WEX::Common::String& imageName
                );

            static void ValidateAllDeltaFramesCrc32(
                _In_ const WEX::Common::String& fileName,
                _In_ const WEX::Common::String& imageName,
                _In_ const Crc32List& expectedFrameCrcValues
                );

            static void ValidateAllFramesCrc32(
                _In_ const WEX::Common::String& fileName,
                _In_ const WEX::Common::String& imageName,
                _In_ const Crc32List& expectedFrameCrcValues
                );

            static void ValidateAllBitmapsCrc32(
                _In_ const wrl::ComPtr<IWICBitmapDecoder>& spWicBitmapDecoder,
                _In_ BitmapProvider bitmapProvider,
                _In_ const WEX::Common::String& imageName,
                _In_ const Crc32List& expectedFrameCrcValues
                );

            static void FixAlpha(
                _In_ const xref_ptr<OfferableSoftwareBitmap>& spSoftwareBitmap
                );
        };
    }}
} } } }
