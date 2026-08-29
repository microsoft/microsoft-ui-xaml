// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <wincodec.h>
#include "ImageProviderInterfaces.h"
#include "RawData.h"
#include "ImageMetadata.h"
#include "WicService.h"
#include "OfferableSoftwareBitmap.h"
#include "ImageTestHelper.h"
#include "palgfx.h"
#include "SurfaceDecodeParams.h"
#include "ImageDecodeParams.h"
#include "EncodedImageData.h"
#include "SoftwareBitmapUtility.h"
#include "WicAnimatedGifDecoder.h"
#include "AnimatedGifUnitTests.h"
#include "ImagingUtility.h"
#include <CoInitHelper.h>
#include <XamlLogging.h>

using namespace ImageTestHelper;

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Imaging {

bool AnimatedGifUnitTests::ClassSetup()
{
    CoInitHelper::EnsureCoInitialized();
    return true;
}

void AnimatedGifUnitTests::GlobalMetadata1()
{
    auto fileName = GetImageResourcesPath() + L"animatedgif\\08bMAnim2.gif";
    auto fileData = GetFileData(fileName);

    wrl::ComPtr<IWICBitmapDecoder> spBitmapDecoder;
    VERIFY_SUCCEEDED(WicService::GetInstance().GetBitmapDecoder(fileData, spBitmapDecoder));

    ImageMetadata imageMetadata;
    VERIFY_SUCCEEDED(WicService::GetInstance().GetMetadata(spBitmapDecoder, imageMetadata));

    VERIFY_ARE_EQUAL(imageMetadata.width, 107);
    VERIFY_ARE_EQUAL(imageMetadata.height, 35);
    VERIFY_ARE_EQUAL(imageMetadata.frameCount, 15);
    VERIFY_ARE_EQUAL(imageMetadata.loopCount, 1000);
}

void AnimatedGifUnitTests::GlobalMetadata2()
{
    auto fileName = GetImageResourcesPath() + L"animatedgif\\08bS4.gif";
    auto fileData = GetFileData(fileName);

    wrl::ComPtr<IWICBitmapDecoder> spBitmapDecoder;
    VERIFY_SUCCEEDED(WicService::GetInstance().GetBitmapDecoder(fileData, spBitmapDecoder));

    ImageMetadata imageMetadata;
    VERIFY_SUCCEEDED(WicService::GetInstance().GetMetadata(spBitmapDecoder, imageMetadata));

    VERIFY_ARE_EQUAL(imageMetadata.width, 173);
    VERIFY_ARE_EQUAL(imageMetadata.height, 120);
    VERIFY_ARE_EQUAL(imageMetadata.frameCount, 2);
    VERIFY_ARE_EQUAL(imageMetadata.loopCount, 0);
}

void AnimatedGifUnitTests::DeltaFramesImages()
{
    ValidateAllDeltaFrameImages(
        GetImageResourcesPath() + L"animatedgif\\08bSAnim3.gif",
        "08bSAnim3");
}

void AnimatedGifUnitTests::DeltaFramesCrc32()
{
    // TODO: This can be changed to use Crc32List directly when the compiler supports it.
    const uint32_t expectedFrameCrcValues[] =
    {
        0x7d8c03ea,
        0xe53997cb,
        0xdd084eec,
        0x47ccdb8f,
        0xa24ae118,
        0xac944cc3,
        0x8b7d2fad,
        0xb541dc7b,
        0x8b7d2fad,
        0xac944cc3,
        0xa24ae118,
        0x47ccdb8f,
        0xdd084eec,
        0xe53997cb,
    };

    ValidateAllDeltaFramesCrc32(
        GetImageResourcesPath() + L"animatedgif\\08bSAnim3.gif",
        L"AnimatedGifUnitTests_DeltaFramesCrc32",
        Crc32List(expectedFrameCrcValues, expectedFrameCrcValues + ARRAY_SIZE(expectedFrameCrcValues)));
}

void AnimatedGifUnitTests::_08bSAnim3()
{
    const uint32_t expectedFrameCrcValues[] =
    {
        0x7d8c03ea,
        0xe53997cb,
        0xdd084eec,
        0x47ccdb8f,
        0xa24ae118,
        0xac944cc3,
        0x8b7d2fad,
        0xb541dc7b,
        0x8b7d2fad,
        0xac944cc3,
        0xa24ae118,
        0x47ccdb8f,
        0xdd084eec,
        0xe53997cb,
    };
    Crc32List crc32List(std::begin(expectedFrameCrcValues), std::end(expectedFrameCrcValues));
    crc32List.insert(crc32List.end(), std::begin(expectedFrameCrcValues), std::end(expectedFrameCrcValues));

    ValidateAllFramesCrc32(
        GetImageResourcesPath() + L"animatedgif\\08bSAnim3.gif",
        L"AnimatedGifUnitTests_08bSAnim3",
        crc32List);
}

void AnimatedGifUnitTests::c0()
{
    const uint32_t expectedFrameCrcValues[] =
    {
        0x94e85f69,
        0x2f307269,
        0xccc7888f,
        0xe9a1b656,
        0xe5d1aa1f,
    };
    Crc32List crc32List(std::begin(expectedFrameCrcValues), std::end(expectedFrameCrcValues));
    crc32List.insert(crc32List.end(), std::begin(expectedFrameCrcValues), std::end(expectedFrameCrcValues));

    ValidateAllFramesCrc32(
        GetImageResourcesPath() + L"animatedgif\\disposalmethod\\c0.gif",
        L"AnimatedGifUnitTests_c0",
        crc32List);
}

void AnimatedGifUnitTests::c1()
{
    const uint32_t expectedFrameCrcValues[] =
    {
        0xb56a3479,
        0x0faa1006,
        0x7672b965,
        0xd533c4c6,
        0xab2d5cef,
    };
    Crc32List crc32List(std::begin(expectedFrameCrcValues), std::end(expectedFrameCrcValues));
    crc32List.insert(crc32List.end(), std::begin(expectedFrameCrcValues), std::end(expectedFrameCrcValues));

    ValidateAllFramesCrc32(
        GetImageResourcesPath() + L"animatedgif\\disposalmethod\\c1.gif",
        L"AnimatedGifUnitTests_c1",
        crc32List);
}

void AnimatedGifUnitTests::c2()
{
    const uint32_t expectedFrameCrcValues[] =
    {
        0x2bdcbf2c,
        0xc21cb914,
        0x59b3d64a,
        0xb0e3ab8f,
        0x5875447f,
    };
    Crc32List crc32List(std::begin(expectedFrameCrcValues), std::end(expectedFrameCrcValues));
    crc32List.insert(crc32List.end(), std::begin(expectedFrameCrcValues), std::end(expectedFrameCrcValues));

    ValidateAllFramesCrc32(
        GetImageResourcesPath() + L"animatedgif\\disposalmethod\\c2.gif",
        L"AnimatedGifUnitTests_c2",
        crc32List);
}

void AnimatedGifUnitTests::c3()
{
    const uint32_t expectedFrameCrcValues[] =
    {
        0x15268d9c,
        0xc664584f,
        0xf9b3b088,
        0x40283dd1,
        0x678217da,
    };
    Crc32List crc32List(std::begin(expectedFrameCrcValues), std::end(expectedFrameCrcValues));
    crc32List.insert(crc32List.end(), std::begin(expectedFrameCrcValues), std::end(expectedFrameCrcValues));

    ValidateAllFramesCrc32(
        GetImageResourcesPath() + L"animatedgif\\disposalmethod\\c3.gif",
        L"AnimatedGifUnitTests_c3",
        crc32List);
}

void AnimatedGifUnitTests::GifTest()
{
    const uint32_t expectedFrameCrcValues[] =
    {
        0xfac02ba2,
        0xbd67c8cd,
        0xac2d3371,
        0x51c7508f,
        0x72b2f959,
    };
    Crc32List crc32List(std::begin(expectedFrameCrcValues), std::end(expectedFrameCrcValues));
    crc32List.insert(crc32List.end(), std::begin(expectedFrameCrcValues), std::end(expectedFrameCrcValues));

    ValidateAllFramesCrc32(
        GetImageResourcesPath() + L"animatedgif\\disposalmethod\\GifTest.gif",
        L"AnimatedGifUnitTests_GifTest",
        crc32List);
}

void AnimatedGifUnitTests::GifTest2()
{
    const uint32_t expectedFrameCrcValues[] =
    {
        0x2eb9c7e8,
        0x8c1323df,
        0xaeda9049,
        0xae7f071b,
    };
    Crc32List crc32List(std::begin(expectedFrameCrcValues), std::end(expectedFrameCrcValues));
    crc32List.insert(crc32List.end(), std::begin(expectedFrameCrcValues), std::end(expectedFrameCrcValues));

    ValidateAllFramesCrc32(
        GetImageResourcesPath() + L"animatedgif\\disposalmethod\\GifTest2.gif",
        L"AnimatedGifUnitTests_GifTest2",
        crc32List);
}

void AnimatedGifUnitTests::LoopedTransparent()
{
    const uint32_t expectedFrameCrcValues[] =
    {
        0xd8fb17ff,
        0xfbfcd13d,
        0x21ab1963,
        0xea54b68b,
        0xd7ad9a40,
        0x7c282001,
        0xd3b97534,
        0xe5f5ca98,
        0xab2aa329,
        0x043ecb69,
        0x4536f086,
        0x49dee7ec,
        0xa5a322a6,
        0xcddbade7,
        0x3743a4c4,
        0x79ec2347,
        0xbdf487f5,
        0xffedf370,
        0x54684931,
        0xfbf91c04,
        0xcdb5a3a8,
        0x836aca19,
        0xea491fdf,
        0x482a8ec1,
        0x272629f9,
        0xa5a322a6,
    };
    Crc32List crc32List(std::begin(expectedFrameCrcValues), std::end(expectedFrameCrcValues));
    crc32List.insert(crc32List.end(), std::begin(expectedFrameCrcValues), std::end(expectedFrameCrcValues));

    ValidateAllFramesCrc32(
        GetImageResourcesPath() + L"animatedgif\\looped_transparent.gif",
        L"AnimatedGifUnitTests_looped_transparent",
        crc32List);
}

void AnimatedGifUnitTests::FrameDelay()
{
    auto fileName = GetImageResourcesPath() + L"animatedgif\\08bMAnim2.gif";
    auto encodedImageData = GetFileEncodedData(fileName);
    auto imageDecoder = WicService::GetInstance().CreateDefaultDecoder(encodedImageData->GetMetadata());
    auto decodeParams = make_xref<ImageDecodeParams>(pixelColor32bpp_A8R8G8B8, 0, 0, false, 0 /* imageId */, xstring_ptr::EmptyString());

    VERIFY_ARE_EQUAL(encodedImageData->GetMetadata().frameCount, 15);

    for (int frameIndex = 0; frameIndex < 15; ++frameIndex)
    {
        // Check every frame twice because decoder may cache results
        for (int i = 0; i < 2; i++)
        {
            wrl::ComPtr<IWICBitmapSource> bitmapSource;
            std::chrono::milliseconds delay{ 0xBADBEEF };
            VERIFY_SUCCEEDED(imageDecoder->DecodeFrame(
                *encodedImageData,
                *decodeParams,
                frameIndex,
                bitmapSource,
                delay));

            VERIFY_ARE_EQUAL(delay.count(), 100);
        }
    }
}

void AnimatedGifUnitTests::ValidateAllDeltaFrameImages(
    _In_ const WEX::Common::String& fileName,
    _In_ const WEX::Common::String& imageName
    )
{
    WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

    auto fileData = GetFileData(fileName);

    wrl::ComPtr<IWICBitmapDecoder> spBitmapDecoder;
    VERIFY_SUCCEEDED(WicService::GetInstance().GetBitmapDecoder(fileData, spBitmapDecoder));

    ImageMetadata imageMetadata;
    VERIFY_SUCCEEDED(WicService::GetInstance().GetMetadata(spBitmapDecoder, imageMetadata));

    auto frameProvider = [&](uint32_t frameIndex)
    {
        WicAnimatedGifDecoder::DeltaFrameInfo gifDeltaFrameInfo;
        wrl::ComPtr<IWICBitmapSource> spBitmapSource;
        VERIFY_SUCCEEDED(WicAnimatedGifDecoder::CreateDeltaFrameSource(
            spBitmapDecoder,
            frameIndex,
            gifDeltaFrameInfo,
            spBitmapSource));

        return CreateSoftwareBitmapFromWicBitmapSource(spBitmapSource);
    };

    ValidateAllBitmaps(
        spBitmapDecoder,
        frameProvider,
        imageName);
}

void AnimatedGifUnitTests::ValidateAllBitmaps(
    _In_ const wrl::ComPtr<IWICBitmapDecoder>& spWicBitmapDecoder,
    _In_ BitmapProvider bitmapProvider,
    _In_ const WEX::Common::String& imageName
    )
{
    WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

    bool allImagesMatch = true;
    uint32_t frameCount;
    VERIFY_SUCCEEDED(spWicBitmapDecoder->GetFrameCount(&frameCount));

    for (uint32_t frameIndex = 0; frameIndex < frameCount; frameIndex++)
    {
        auto spSoftwareBitmap = bitmapProvider(frameIndex);

        WEX::Common::String imageNameNumber = imageName;
        imageNameNumber.AppendFormat(L".%lu", frameIndex);
        if (!ValidateImage(
            spSoftwareBitmap,
            imageNameNumber))
        {
            allImagesMatch = false;
        }
    }

    VERIFY_IS_TRUE(allImagesMatch);
}

void AnimatedGifUnitTests::ValidateAllDeltaFramesCrc32(
    _In_ const WEX::Common::String& fileName,
    _In_ const WEX::Common::String& imageName,
    _In_ const Crc32List& expectedFrameCrcValues
    )
{
    WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

    auto fileData = GetFileData(fileName);

    wrl::ComPtr<IWICBitmapDecoder> spBitmapDecoder;
    VERIFY_SUCCEEDED(WicService::GetInstance().GetBitmapDecoder(fileData, spBitmapDecoder));

    ImageMetadata imageMetadata;
    VERIFY_SUCCEEDED(WicService::GetInstance().GetMetadata(spBitmapDecoder, imageMetadata));

    auto frameProvider = [&](uint32_t frameIndex)
    {
        wrl::ComPtr<IWICBitmapSource> spBitmapSource;
        WicAnimatedGifDecoder::DeltaFrameInfo gifDeltaFrameInfo;
        VERIFY_SUCCEEDED(WicAnimatedGifDecoder::CreateDeltaFrameSource(
            spBitmapDecoder,
            frameIndex,
            gifDeltaFrameInfo,
            spBitmapSource));

        return CreateSoftwareBitmapFromWicBitmapSource(spBitmapSource);
    };

    ValidateAllBitmapsCrc32(
        spBitmapDecoder,
        frameProvider,
        imageName,
        expectedFrameCrcValues);
}

void AnimatedGifUnitTests::ValidateAllFramesCrc32(
    _In_ const WEX::Common::String& fileName,
    _In_ const WEX::Common::String& imageName,
    _In_ const Crc32List& expectedFrameCrcValues
    )
{
    WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

    auto spEncodedImageData = GetFileEncodedData(fileName);

    auto spWicImageDecoder = WicService::GetInstance().CreateDefaultDecoder(spEncodedImageData->GetMetadata());

    auto frameProvider = [&](uint32_t frameIndex)
    {
        auto decodeParams = make_xref<ImageDecodeParams>(pixelColor32bpp_A8R8G8B8, 0, 0, false, 0 /* imageId */, xstring_ptr::EmptyString());
        wrl::ComPtr<IWICBitmapSource> bitmapSource;
        std::chrono::milliseconds delay;
        VERIFY_SUCCEEDED(spWicImageDecoder->DecodeFrame(
            *spEncodedImageData,
            *decodeParams,
            frameIndex % spEncodedImageData->GetMetadata().frameCount,
            bitmapSource,
            delay));

        xref_ptr<OfferableSoftwareBitmap> spSoftwareBitmap;
        VERIFY_SUCCEEDED(ImagingUtility::RealizeBitmapSource(
            spEncodedImageData->GetMetadata(),
            bitmapSource.Get(),
            *decodeParams,
            spSoftwareBitmap));

        return spSoftwareBitmap;
    };

    wrl::ComPtr<IWICBitmapDecoder> spWicBitmapDecoder;
    VERIFY_SUCCEEDED(spEncodedImageData->CreateWicBitmapDecoder(spWicBitmapDecoder));

    ValidateAllBitmapsCrc32(
        spWicBitmapDecoder,
        frameProvider,
        imageName,
        expectedFrameCrcValues);
}


// This algorithm iterates through all frames, checks if there is a CRC mismatch.  If there is,
// it will output a list of all frame CRC's (which can be copy pasted into code as reference).
// It also has an optional outputImages parameter to output images for visual validation.
// TODO: During testing phase, consider moving this to a file for master verification.
void AnimatedGifUnitTests::ValidateAllBitmapsCrc32(
    _In_ const wrl::ComPtr<IWICBitmapDecoder>& spWicBitmapDecoder,
    _In_ BitmapProvider bitmapProvider,
    _In_ const WEX::Common::String& imageName,
    _In_ const Crc32List& expectedFrameCrcValues
    )
{
    WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

    bool allImagesMatch = true;
    std::vector<uint32_t> actualFrameCrcValues;

    uint32_t frameCount;
    VERIFY_SUCCEEDED(spWicBitmapDecoder->GetFrameCount(&frameCount));

    if (frameCount < expectedFrameCrcValues.size())
    {
        // For looping validation
        frameCount = static_cast<uint32_t>(expectedFrameCrcValues.size());
    }

    for (uint32_t frameIndex = 0; frameIndex < frameCount; frameIndex++)
    {
        auto spSoftwareBitmap = bitmapProvider(frameIndex);

        auto actualFrameCrc32 = GetImageCrc32(spSoftwareBitmap);
        actualFrameCrcValues.push_back(actualFrameCrc32);

        auto match = true;
        if (frameIndex >= expectedFrameCrcValues.size() ||
            actualFrameCrc32 != expectedFrameCrcValues[frameIndex])
        {
            allImagesMatch = false;
            match = false;
        }

        if (!match)
        {
            WEX::Common::String imageNameNumber = GetImageOutputPath() + imageName;
            imageNameNumber.AppendFormat(L".%lu.png", frameIndex);
            SaveImageToPngFile(spSoftwareBitmap, imageNameNumber);
        }
    }

    if (!allImagesMatch)
    {
        WEX::Common::String actualCrcList = "{\n";
        for (auto actualFrameCrcValue : actualFrameCrcValues)
        {
            actualCrcList.AppendFormat(L"    0x%08lx,\n", actualFrameCrcValue);
        }
        actualCrcList += "}\n";

        LOG_OUTPUT(L"Image mismatch, Frame CRC list:");
        LOG_OUTPUT(L"%s", actualCrcList.GetBuffer());
    }

    VERIFY_IS_TRUE(allImagesMatch);
}

} } } } } }
