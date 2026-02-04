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
    ValidateImage(
        GetImageResourcesPath() + L"rainier_444_2048x1536.jpg",
        L"DecodingUnitTests_Jpeg",
        0x5fd3ea9b);
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
        0x3fa918f5);
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


} } } } } }
