// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <wincodec.h>
#include "RawData.h"
#include "ImageMetadata.h"
#include "WicService.h"
#include "ImageTestHelper.h"
#include "WicServiceUnitTests.h"
#include <CoInitHelper.h>

using namespace ImageTestHelper;

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Imaging {

bool WicServiceUnitTests::ClassSetup()
{
    CoInitHelper::EnsureCoInitialized();
    return true;
}

void WicServiceUnitTests::JpegMetadata()
{
    ValidateFileMetadata(
        GetImageResourcesPath() + L"rainier_444_2048x1536.jpg",
        GUID_ContainerFormatJpeg,
        2048,
        1536);
}

void WicServiceUnitTests::JpegOrientation180()
{
    // TODO: Need image test content with 90 and 270 degree rotation
    ValidateFileMetadata(
        GetImageResourcesPath() + L"CN-EOS5DMarkII-ORI-BottomUp.jpg",
        GUID_ContainerFormatJpeg,
        5616,
        3744);
}

void WicServiceUnitTests::PngMetadata()
{
    ValidateFileMetadata(
        GetImageResourcesPath() + L"windows.png",
        GUID_ContainerFormatPng,
        421,
        332);
}

void WicServiceUnitTests::BmpMetadata()
{
    ValidateFileMetadata(
        GetImageResourcesPath() + L"Smiley.bmp",
        GUID_ContainerFormatBmp,
        199,
        193);
}

void WicServiceUnitTests::AnimatedGifMetadata()
{
    ValidateFileMetadata(
        GetImageResourcesPath() + L"animatedgif\\08bS2.gif",
        GUID_ContainerFormatGif,
        74,
        118);
}

void WicServiceUnitTests::BadImage()
{
    auto fileData = GetFileData(GetImageResourcesPath() + L"InvalidImage.png");

    wrl::ComPtr<IWICBitmapDecoder> spBitmapDecoder;
    VERIFY_FAILED(WicService::GetInstance().GetBitmapDecoder(fileData, spBitmapDecoder));
}

void WicServiceUnitTests::ValidateFileMetadata(
    const WEX::Common::String& fileName,
    const GUID& expectedContainerGuid,
    uint32_t expectedWidth,
    uint32_t expectedHeight)
{
    auto fileData = GetFileData(fileName);

    wrl::ComPtr<IWICBitmapDecoder> spBitmapDecoder;
    VERIFY_SUCCEEDED(WicService::GetInstance().GetBitmapDecoder(fileData, spBitmapDecoder));

    ImageMetadata info;
    VERIFY_SUCCEEDED(WicService::GetInstance().GetMetadata(spBitmapDecoder.Get(), info));

    VERIFY_IS_TRUE(expectedContainerGuid == info.containerFormat);

    VERIFY_ARE_EQUAL(info.width, expectedWidth);
    VERIFY_ARE_EQUAL(info.height, expectedHeight);
}

} } } } } }
