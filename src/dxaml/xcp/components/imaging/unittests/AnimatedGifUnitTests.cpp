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
        0x6fd7a759,
        0x3d537909,
        0x3559f111,
        0x26385c76,
        0x5a850fea,
        0x26385c76,
        0x3559f111,
        0x3d537909,
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
        0x6fd7a759,
        0x3d537909,
        0x3559f111,
        0x26385c76,
        0x5a850fea,
        0x26385c76,
        0x3559f111,
        0x3d537909,
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
        0x68935d04,
        0x993d559d,
        0x0fd5cb8e,
        0x4079d008,
        0xd7b85c37,
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
        0x68935d04,
        0x993d559d,
        0x0fd5cb8e,
        0x4079d008,
        0xd7b85c37,
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
        0x68935d04,
        0xce6bae2a,
        0x74971f94,
        0xbf71c59c,
        0x351d123a,
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
        0x68935d04,
        0xce6bae2a,
        0x74971f94,
        0xbf71c59c,
        0x351d123a,
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
        0xbbe01c7e,
        0x09c955f7,
        0x72234020,
        0x677a73c7,
        0xe0c95280,
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
        0x748dfb26,
        0x179b2241,
        0xeae8a1b7,
        0xfadd17b7,
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
        0xc66fdb3a,
        0xd6503788,
        0x019ef820,
        0x025e4d3c,
        0x3a368440,
        0x21b55536,
        0x96c78036,
        0xc1ae41b2,
        0x3a4cce94,
        0x0681509a,
        0x5f0a857c,
        0xf82c1be4,
        0xa342de58,
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
