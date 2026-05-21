// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <wincodec.h>
#include <shlobj_core.h>
#include <shlwapi.h>
#include <wil\filesystem.h>
#include <MockDComp-UnitTestHelpers.h> // For ComputeCrc32
#include "RawData.h"
#include "ImageMetadata.h"
#include "EncodedImageData.h"
#include "OfferableSoftwareBitmap.h"
#include "PixelFormat.h"
#include "ImageTestHelper.h"
#include <XamlLogging.h>

namespace ImageTestHelper
{
    WEX::Common::String GetImageResourcesPath()
    {
        WEX::Common::String deploymentDir;
        LogThrow_IfFailed(
            WEX::TestExecution::RuntimeParameters::TryGetValue(WEX::TestExecution::RuntimeParameterConstants::c_szTestDeploymentDir, deploymentDir));

        if(deploymentDir.Right(1) != "\\")
        {
            deploymentDir.Append(L"\\");
        }

        return deploymentDir + "resources\\native\\external\\foundation\\graphics\\image\\";
    }

    WEX::Common::String GetImageMastersPath()
    {
        WEX::Common::String deploymentDir;
        LogThrow_IfFailed(
            WEX::TestExecution::RuntimeParameters::TryGetValue(WEX::TestExecution::RuntimeParameterConstants::c_szTestDeploymentDir, deploymentDir));

        if(deploymentDir.Right(1) != "\\")
        {
            deploymentDir.Append(L"\\");
        }
        
        return deploymentDir + "images\\";
    }

    WEX::Common::String GetImageOutputPath()
    {
        WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

        WCHAR picturesPath[wil::max_path_length];

        VERIFY_SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_MYPICTURES, NULL, SHGFP_TYPE_CURRENT, picturesPath));

        WEX::Common::String outputPath(picturesPath);
        outputPath += "\\XamlTAEFOutput\\";

        VERIFY_SUCCEEDED(wil::CreateDirectoryDeepNoThrow(outputPath));

        return outputPath;
    }

    xref_ptr<OfferableSoftwareBitmap> CreateSoftwareBitmapFromWicBitmapSource(
        _In_ const wrl::ComPtr<IWICBitmapSource>& spWicBitmapSource
        )
    {
        auto width = 0u;
        auto height = 0u;
        VERIFY_SUCCEEDED(spWicBitmapSource->GetSize(&width, &height));

        WICPixelFormatGUID pixelFormat = {};
        VERIFY_SUCCEEDED(spWicBitmapSource->GetPixelFormat(&pixelFormat));

        bool isOpaque = false;
        if (pixelFormat == GUID_WICPixelFormat32bppBGR)
        {
            isOpaque = false;
        }
        else if (pixelFormat == GUID_WICPixelFormat32bppPBGRA)
        {
            isOpaque = true;
        }
        else
        {
            LOG_OUTPUT(L"Invalid WIC pixel format when creating OfferableSoftwareBitmap");
        }

        xref_ptr<OfferableSoftwareBitmap> spSoftwareBitmap = make_xref<OfferableSoftwareBitmap>(
            pixelColor32bpp_A8R8G8B8,
            width,
            height,
            isOpaque);

        VERIFY_SUCCEEDED(spWicBitmapSource->CopyPixels(
            nullptr,
            spSoftwareBitmap->GetStride(),
            spSoftwareBitmap->GetBufferSize(),
            reinterpret_cast<uint8_t*>(spSoftwareBitmap->GetBuffer())));

        return spSoftwareBitmap;
    }

    RawData GetFileData(
        _In_ const WEX::Common::String& fileName
        )
    {
        WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

        wil::unique_handle hFile(CreateFile(
            fileName,
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr));

        VERIFY_ARE_NOT_EQUAL(hFile.get(), INVALID_HANDLE_VALUE);

        LARGE_INTEGER fileSize = { 0 };

        VERIFY_ARE_EQUAL(GetFileSizeEx(hFile.get(), &fileSize), TRUE);

        RawData fileData;
        fileData.Allocate(static_cast<size_t>(fileSize.QuadPart));

        DWORD bytesRead = 0;
        VERIFY_ARE_EQUAL(ReadFile(
            hFile.get(),
            fileData.GetData(),
            static_cast<DWORD>(fileData.GetSize()),
            &bytesRead,
            nullptr),
            TRUE);

        VERIFY_ARE_EQUAL(fileData.GetSize(), bytesRead);

        return std::move(fileData);
    }

    std::shared_ptr<EncodedImageData> GetFileEncodedData(
        _In_ const WEX::Common::String& fileName,
        bool parse
        )
    {
        WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

        wistd::unique_ptr<RawData> pRawData = wil::make_unique_failfast<RawData>(
            std::move(GetFileData(fileName)));

        auto spEncodedImageData = std::make_shared<EncodedImageData>(std::move(pRawData), 0u);
        if (parse)
        {
            VERIFY_SUCCEEDED(spEncodedImageData->Parse(nullptr, wf::Size{ 0, 0 }));
        }

        return spEncodedImageData;
    }

    bool CompareImages(
        _In_ const xref_ptr<OfferableSoftwareBitmap>& spImage1,
        _In_ const xref_ptr<OfferableSoftwareBitmap>& spImage2)
    {
        WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

        // Get info for SB1
        auto width1 = spImage1->GetWidth();
        auto height1 = spImage1->GetHeight();
        auto format1 = spImage1->GetPixelFormat();
        auto bufferSize1 = spImage1->GetBufferSize();

        // Get info for SB2
        auto width2 = spImage2->GetWidth();
        auto height2 = spImage2->GetHeight();
        auto format2 = spImage2->GetPixelFormat();
        auto bufferSize2 = spImage2->GetBufferSize();

        // Compare size between SB1 and SB2
        if ((width1 != width2) ||
            (height1 != height2) ||
            (format1 != format2) ||
            (bufferSize1 != bufferSize2))
        {
            LOG_OUTPUT(L"Image1 parameters do not match Image2 parameters");
            return false;
        }

        // Compare data between SB1 and SB2
        if (memcmp(spImage1->GetBuffer(), spImage2->GetBuffer(), bufferSize1) != 0)
        {
            LOG_OUTPUT(L"Image1 pixels do not match Image2 pixels");
            return false;
        }

        return true;
    }

    xref_ptr<OfferableSoftwareBitmap> LoadImageFromFile(
        _In_ const WEX::Common::String& fileName
        )
    {
        WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

        // Setup WIC and write the data to a file
        wrl::ComPtr<IWICImagingFactory> spWICFactory;
        VERIFY_SUCCEEDED(CoCreateInstance(
            CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&spWICFactory)
            ));

        // Create a decoder for the given image file
        wrl::ComPtr<IWICBitmapDecoder> spWICBitmapDecoder;
        VERIFY_SUCCEEDED(spWICFactory->CreateDecoderFromFilename(
            fileName,
            nullptr,
            GENERIC_READ,
            WICDecodeMetadataCacheOnDemand,
            &spWICBitmapDecoder));

        // Retrieve the requested frame of the image from the decoder
        wrl::ComPtr<IWICBitmapFrameDecode> spWICBitmapFrameDecode;
        VERIFY_SUCCEEDED(spWICBitmapDecoder->GetFrame(0, &spWICBitmapFrameDecode));

        UINT width = 0;
        UINT height = 0;
        VERIFY_SUCCEEDED(spWICBitmapFrameDecode->GetSize(&width, &height));

        wrl::ComPtr<IWICFormatConverter> spWICFormatConverter;
        VERIFY_SUCCEEDED(spWICFactory->CreateFormatConverter(&spWICFormatConverter));
        VERIFY_SUCCEEDED(spWICFormatConverter->Initialize(
            spWICBitmapFrameDecode.Get(),
            GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone,
            NULL,
            0.f,
            WICBitmapPaletteTypeCustom
            ));

        // TODO: Add rotator here to rotate based on EXIF data.

        return CreateSoftwareBitmapFromWicBitmapSource(spWICFormatConverter);
    }

    void SaveImageToPngFile(
        _In_ const xref_ptr<OfferableSoftwareBitmap>& spImage,
        _In_ const WEX::Common::String& fileName
        )
    {
        WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

        VERIFY_ARE_EQUAL(pixelColor32bpp_A8R8G8B8, spImage->GetPixelFormat());

        // Get SoftwareBitmap info
        auto width = spImage->GetWidth();
        auto height = spImage->GetHeight();
        auto isOpaque = spImage->IsOpaque();

        WICPixelFormatGUID pixelFormat = {};

        if (isOpaque)
        {
            pixelFormat = GUID_WICPixelFormat32bppBGR;
        }
        else
        {
            pixelFormat = GUID_WICPixelFormat32bppPBGRA;
        }

        // Setup WIC and write the data to a file
        wrl::ComPtr<IWICImagingFactory> spWICFactory;
        VERIFY_SUCCEEDED(CoCreateInstance(
            CLSID_WICImagingFactory,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&spWICFactory)
            ));

        wrl::ComPtr<IWICBitmap> spWICBitmap;
        VERIFY_SUCCEEDED(spWICFactory->CreateBitmapFromMemory(
            width,
            height,
            pixelFormat,
            spImage->GetStride(),
            spImage->GetBufferSize(),
            reinterpret_cast<uint8_t*>(spImage->GetBuffer()),
            &spWICBitmap
            ));

        wrl::ComPtr<IWICStream> spWicStream;
        VERIFY_SUCCEEDED(spWICFactory->CreateStream(&spWicStream));
        VERIFY_SUCCEEDED(spWicStream->InitializeFromFilename(fileName, GENERIC_WRITE));

        wrl::ComPtr<IWICBitmapEncoder> spWicBitmapEncoder;
        VERIFY_SUCCEEDED(spWICFactory->CreateEncoder(GUID_ContainerFormatPng, NULL, &spWicBitmapEncoder));
        VERIFY_SUCCEEDED(spWicBitmapEncoder->Initialize(spWicStream.Get(), WICBitmapEncoderNoCache));

        wrl::ComPtr<IWICBitmapFrameEncode> spWicBitmapFrameEncode;
        VERIFY_SUCCEEDED(spWicBitmapEncoder->CreateNewFrame(&spWicBitmapFrameEncode, NULL));
        VERIFY_SUCCEEDED(spWicBitmapFrameEncode->Initialize(NULL));

        WICPixelFormatGUID encoderPixelFormat = GUID_WICPixelFormatDontCare;
        VERIFY_SUCCEEDED(spWicBitmapFrameEncode->SetPixelFormat(&encoderPixelFormat));
        VERIFY_SUCCEEDED(spWicBitmapFrameEncode->SetSize(width, height));
        VERIFY_SUCCEEDED(spWicBitmapFrameEncode->WriteSource(spWICBitmap.Get(), NULL));
        VERIFY_SUCCEEDED(spWicBitmapFrameEncode->Commit());
        VERIFY_SUCCEEDED(spWicBitmapEncoder->Commit());
    }

    bool ValidateImage(
        _In_ const xref_ptr<OfferableSoftwareBitmap>& spImage,
        _In_ const WEX::Common::String& imageName
        )
    {
        WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

        auto masterFileName = GetImageMastersPath() + imageName + ".master.png";
        auto masterExists = ::PathFileExists(masterFileName);
        bool match = true;

        if (masterExists)
        {
            auto spMasterImage = LoadImageFromFile(masterFileName);

            if (!CompareImages(spImage, spMasterImage))
            {
                LOG_OUTPUT(L"Image mismatch with: " + imageName);
                match = false;
            }
        }
        else
        {
            LOG_OUTPUT(L"Image master does not exist: " + masterFileName);
            match = false;
        }

        if (!match)
        {
            auto outputFileName = GetImageOutputPath() + imageName + ".out.png";
            SaveImageToPngFile(spImage, outputFileName);
        }

        return match;
    }

    bool ValidateImage(
        _In_ const xref_ptr<OfferableSoftwareBitmap>& spImage,
        _In_ const WEX::Common::String& imageName,
        _In_ const WEX::Common::String& variation
        )
    {
        return ValidateImage(
            spImage,
            imageName + "." + variation);
    }

    bool ValidateImageCrc(
        _In_ const xref_ptr<OfferableSoftwareBitmap>& spImage,
        _In_ const WEX::Common::String& imageName,
        uint32_t expectedCrc32
        )
    {
        WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

        auto actualCrc32 = GetImageCrc32(spImage);
        auto match = (expectedCrc32 == actualCrc32);

        if (!match)
        {
            LOG_OUTPUT(L"Image mismatch with: " + imageName);
            LOG_OUTPUT(L"Expected CRC = 0x%08lx, Actual CRC = 0x%08lx", expectedCrc32, actualCrc32);

            auto outputFileName = GetImageOutputPath() + imageName + ".out.png";
            SaveImageToPngFile(spImage, outputFileName);
        }

        return match;
    }

    uint32_t GetImageCrc32(
        _In_ const xref_ptr<OfferableSoftwareBitmap>& spImage
        )
    {
        return ComputeCrc32(
            0ul,
            reinterpret_cast<const uint8_t*>(spImage->GetBuffer()),
            spImage->GetBufferSize());
    }
}
