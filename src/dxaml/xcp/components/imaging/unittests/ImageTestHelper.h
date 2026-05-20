// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>
class RawData;
class EncodedImageData;
class OfferableSoftwareBitmap;
struct IWICBitmapSource;

namespace ImageTestHelper
{
    WEX::Common::String GetImageResourcesPath();
    WEX::Common::String GetImageMastersPath();
    WEX::Common::String GetImageOutputPath();

    xref_ptr<OfferableSoftwareBitmap> CreateSoftwareBitmapFromWicBitmapSource(
        _In_ const wrl::ComPtr<IWICBitmapSource>& spWicBitmapSource
        );

    RawData GetFileData(
        _In_ const WEX::Common::String& fileName
        );

    std::shared_ptr<EncodedImageData> GetFileEncodedData(
        _In_ const WEX::Common::String& fileName,
        bool parse = true
        );

    bool CompareImages(
        _In_ const xref_ptr<OfferableSoftwareBitmap>& spImage1,
        _In_ const xref_ptr<OfferableSoftwareBitmap>& spImage2
        );

    xref_ptr<OfferableSoftwareBitmap> LoadImageFromFile(
        _In_ const WEX::Common::String& fileName
        );

    void SaveImageToPngFile(
        _In_ const xref_ptr<OfferableSoftwareBitmap>& spImage,
        _In_ const WEX::Common::String& fileName
        );

    bool ValidateImage(
        _In_ const xref_ptr<OfferableSoftwareBitmap>& spImage,
        _In_ const WEX::Common::String& imageName
        );

    bool ValidateImage(
        _In_ const xref_ptr<OfferableSoftwareBitmap>& spImage,
        _In_ const WEX::Common::String& imageName,
        _In_ const WEX::Common::String& variation
        );

    bool ValidateImageCrc(
        _In_ const xref_ptr<OfferableSoftwareBitmap>& spImage,
        _In_ const WEX::Common::String& imageName,
        uint32_t expectedCrc32
        );

    uint32_t GetImageCrc32(
        _In_ const xref_ptr<OfferableSoftwareBitmap>& spImage
        );
}
