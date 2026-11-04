// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <wincodec.h>
#include "ImageProviderInterfaces.h"
#include "PixelFormat.h"
#include "palgfx.h"
#include "RawData.h"
#include "ImageMetadata.h"
#include "SurfaceDecodeParams.h"
#include "ImageDecodeParams.h"
#include "EncodedImageData.h"
#include "OfferableSoftwareBitmap.h"
#include "WicService.h"
#include "WicSingleImageDecoder.h"
#include "ImageTestHelper.h"
#include "DecodingUnitTests.h"
#include "ImagingUtility.h"
#include <CoInitHelper.h>

#define LOG_OUTPUT(fmt, ...) WEX::Logging::Log::Comment(WEX::Common::String().Format(fmt, __VA_ARGS__))

using namespace ImageTestHelper;

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Imaging {

bool DecodingUnitTests::ClassSetup()
{
    CoInitHelper::EnsureCoInitialized();
    return true;
}

// TODO: Add tests for different file types, different input decoding sizes, different output decoding sizes
//                 Add tests that use color transform and a 90 or 270 rotation
//                 Add a test to validate that different pixel formats fail
//                 Add tests that test for decoding failure (Ideally at different stages in the WIC pipeline)

void DecodingUnitTests::Jpeg()
{
    // JPEG decoding is lossy and the WIC decoder may produce slightly different output
    // across OS versions, so we validate decode succeeds and produces a valid bitmap
    // rather than checking a specific CRC.
    auto spEncodedImageData = GetFileEncodedData(GetImageResourcesPath() + L"rainier_444_2048x1536.jpg");
    auto spWicImageDecoder = WicService::GetInstance().CreateDefaultDecoder(spEncodedImageData->GetMetadata());
    auto spDecodeParams = make_xref<ImageDecodeParams>(pixelColor32bpp_A8R8G8B8, 0, 0, false, 0 /* imageId */, xstring_ptr::EmptyString());

    wrl::ComPtr<IWICBitmapSource> bitmapSource;
    std::chrono::milliseconds delay;
    VERIFY_SUCCEEDED(spWicImageDecoder->DecodeFrame(
        *spEncodedImageData,
        *spDecodeParams,
        0 /* frameIndex */,
        bitmapSource,
        delay));

    xref_ptr<OfferableSoftwareBitmap> softwareBitmap;
    VERIFY_SUCCEEDED(ImagingUtility::RealizeBitmapSource(
        spEncodedImageData->GetMetadata(),
        bitmapSource.Get(),
        *spDecodeParams,
        softwareBitmap));

    VERIFY_IS_NOT_NULL(softwareBitmap.get());
}

void DecodingUnitTests::Png()
{
    ValidateImage(
        GetImageResourcesPath() + L"windows.png",
        L"DecodingUnitTests_Png",
        0x1214b40e);
}

void DecodingUnitTests::Bmp()
{
    ValidateImage(
        GetImageResourcesPath() + L"Smiley.bmp",
        L"DecodingUnitTests_Bmp",
        0x92ad67b6);
}

void DecodingUnitTests::GifOutOfRange()
{
    ValidateImage(
        GetImageResourcesPath() + L"OutOfRange.gif",
        L"DecodingUnitTests_GifOutOfRange",
        0x604f8340);
}

void DecodingUnitTests::GifOutOfRange2()
{
    ValidateImage(
        GetImageResourcesPath() + L"OutOfRange2.gif",
        L"DecodingUnitTests_GifOutOfRange2",
        0xc71c0011);
}

void DecodingUnitTests::IcoDecodeSize()
{
    // MultiSizeIcon.ico holds solid 16x16 green, 32x32 blue and 48x48 red images. WIC does not expose them
    // in size order, so frame 0 is not the image that fits the requested size.
    VERIFY_ARE_EQUAL(0xff00ff00, DecodeSolidIco(L"MultiSizeIcon.ico", 16, 16)); // the 16x16 image
    VERIFY_ARE_EQUAL(0xff00ff00, DecodeSolidIco(L"MultiSizeIcon.ico", 17, 16)); // DecodeToRenderSize can round one side up
    VERIFY_ARE_EQUAL(0xff0000ff, DecodeSolidIco(L"MultiSizeIcon.ico", 24, 24)); // shrinking 32x32 beats stretching 16x16
    VERIFY_ARE_EQUAL(0xffff0000, DecodeSolidIco(L"MultiSizeIcon.ico", 64, 64)); // nothing is that large, so the largest
}

void DecodingUnitTests::IcoUndecodableImage()
{
    // MultiSizeIconSmallPng.ico holds a solid 16x16 green PNG image next to 32x32 blue and 48x48 red BMP images.
    // WIC lists the PNG image but fails to decode it, so the decode keeps frame 0 instead of failing.
    const uint32_t frame0Pixel = DecodeSolidIco(L"MultiSizeIconSmallPng.ico", 0, 0);
    VERIFY_ARE_EQUAL(frame0Pixel, DecodeSolidIco(L"MultiSizeIconSmallPng.ico", 16, 16));
}

void DecodingUnitTests::ValidateImage(
    const WEX::Common::String& fileName,
    const WEX::Common::String& testIdentifier,
    uint32_t expectedCrc32
    )
{
    auto spEncodedImageData = GetFileEncodedData(fileName);

    auto spWicImageDecoder = WicService::GetInstance().CreateDefaultDecoder(spEncodedImageData->GetMetadata());

    auto spDecodeParams = make_xref<ImageDecodeParams>(pixelColor32bpp_A8R8G8B8, 0, 0, false, 0 /* imageId */, xstring_ptr::EmptyString());

    wrl::ComPtr<IWICBitmapSource> bitmapSource;
    std::chrono::milliseconds delay;
    VERIFY_SUCCEEDED(spWicImageDecoder->DecodeFrame(
        *spEncodedImageData,
        *spDecodeParams,
        0 /* frameIndex */,
        bitmapSource,
        delay));

    xref_ptr<OfferableSoftwareBitmap> softwareBitmap;
    VERIFY_SUCCEEDED(ImagingUtility::RealizeBitmapSource(
        spEncodedImageData->GetMetadata(),
        bitmapSource.Get(),
        *spDecodeParams,
        softwareBitmap));

    VERIFY_IS_TRUE(ValidateImageCrc(
        softwareBitmap,
        testIdentifier,
        expectedCrc32));
}

uint32_t DecodingUnitTests::DecodeSolidIco(
    const wchar_t* fileName,
    uint32_t decodeWidth,
    uint32_t decodeHeight
    )
{
    auto spEncodedImageData = GetFileEncodedData(GetImageResourcesPath() + fileName);

    auto spWicImageDecoder = WicService::GetInstance().CreateDefaultDecoder(spEncodedImageData->GetMetadata());

    auto spDecodeParams = make_xref<ImageDecodeParams>(pixelColor32bpp_A8R8G8B8, decodeWidth, decodeHeight, false, 0 /* imageId */, xstring_ptr::EmptyString());

    wrl::ComPtr<IWICBitmapSource> bitmapSource;
    std::chrono::milliseconds delay;
    VERIFY_SUCCEEDED(spWicImageDecoder->DecodeFrame(
        *spEncodedImageData,
        *spDecodeParams,
        0 /* frameIndex */,
        bitmapSource,
        delay));

    xref_ptr<OfferableSoftwareBitmap> softwareBitmap;
    VERIFY_SUCCEEDED(ImagingUtility::RealizeBitmapSource(
        spEncodedImageData->GetMetadata(),
        bitmapSource.Get(),
        *spDecodeParams,
        softwareBitmap));

    if (decodeWidth != 0)
    {
        VERIFY_ARE_EQUAL(decodeWidth, softwareBitmap->GetWidth());
        VERIFY_ARE_EQUAL(decodeHeight, softwareBitmap->GetHeight());
    }

    // Each image is a single color, so every decoded pixel identifies the image it came from.
    const uint32_t firstPixel = *static_cast<const uint32_t*>(softwareBitmap->GetBuffer());
    uint32_t mismatchedPixels = 0;
    auto row = static_cast<const uint8_t*>(softwareBitmap->GetBuffer());
    for (uint32_t y = 0; y < softwareBitmap->GetHeight(); ++y, row += softwareBitmap->GetStride())
    {
        auto pixels = reinterpret_cast<const uint32_t*>(row);
        for (uint32_t x = 0; x < softwareBitmap->GetWidth(); ++x)
        {
            mismatchedPixels += (pixels[x] != firstPixel) ? 1 : 0;
        }
    }

    LOG_OUTPUT(L"%s at %ux%u: pixel 0x%08x", fileName, decodeWidth, decodeHeight, firstPixel);
    VERIFY_ARE_EQUAL(0u, mismatchedPixels);
    return firstPixel;
}


} } } } } }
