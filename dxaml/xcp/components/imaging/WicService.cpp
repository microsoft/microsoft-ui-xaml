// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <wincodec.h>
#include "ImageProviderInterfaces.h"
#include "RawData.h"
#include "ImageMetadata.h"
#include "ImagingUtility.h"
#include "WicSingleImageDecoder.h"
#include "WicAnimatedGifDecoder.h"
#include "WicService.h"
#include <wil\resource.h>
#include "propkey.h"
#include "Switcher.h"
#include "OrientationSupportedLegacy.h"

WicService::WicService()
{
    IFCFAILFAST(CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&m_spIWICFactory)));
}

const wrl::ComPtr<IWICImagingFactory>& WicService::GetFactory()
{
    return m_spIWICFactory;
}

_Check_return_ HRESULT WicService::GetBitmapDecoder(
    _In_ IRawData& rawData,
    _Out_ wrl::ComPtr<IWICBitmapDecoder>& spBitmapDecoder
    )
{
    wrl::ComPtr<IWICStream> spStream;
    IFC_RETURN(m_spIWICFactory->CreateStream(&spStream));

#pragma warning(push)
#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data
    IFC_RETURN(spStream->InitializeFromMemory(
        const_cast<uint8_t*>(rawData.GetData()),
        rawData.GetSize()));
#pragma warning(pop)

    if (FAILED(m_spIWICFactory->CreateDecoderFromStream(
        spStream.Get(),
        &GUID_VendorMicrosoftBuiltIn,
        WICDecodeMetadataCacheOnDemand,
        &spBitmapDecoder)))
    {
        // Revert to non-MS system registered codec
        IFC_RETURN(m_spIWICFactory->CreateDecoderFromStream(
            spStream.Get(),
            nullptr,
            WICDecodeMetadataCacheOnDemand,
            &spBitmapDecoder));
    }

    return S_OK;
}

static uint32_t GetLoopCount(IWICMetadataQueryReader &metadataQueryReader)
{
    uint32_t loopCount = 1u;

    // First check to see if the application block in the Application Extension
    // contains "NETSCAPE2.0" and "ANIMEXTS1.0", which indicates the gif animation
    // has looping information associated with it.
    // 
    // If we fail to get the looping information, loop the animation infinitely.
    wil::unique_prop_variant applicationExtension;
    if (SUCCEEDED(metadataQueryReader.GetMetadataByName(L"/appext/application", &applicationExtension)) &&
        applicationExtension.vt == (VT_UI1 | VT_VECTOR) &&
        applicationExtension.caub.cElems == 11 &&  // Length of the application block
        (memcmp(applicationExtension.caub.pElems, "NETSCAPE2.0", applicationExtension.caub.cElems) == 0 ||
         memcmp(applicationExtension.caub.pElems, "ANIMEXTS1.0", applicationExtension.caub.cElems) == 0) )
    {
        wil::unique_prop_variant applicationData;
        if (SUCCEEDED(metadataQueryReader.GetMetadataByName(L"/appext/data", &applicationData)))
        {
            //  The data is in the following format:
            //  byte 0: extsize (must be > 1)
            //  byte 1: loopType (1 == animated gif)
            //  byte 2: loop count (least significant byte)
            //  byte 3: loop count (most significant byte)
            //  byte 4: set to zero
            if ((applicationData.vt == (VT_UI1 | VT_VECTOR)) &&
                (applicationData.caub.cElems >= 4) &&
                (applicationData.caub.pElems[0] > 0u) &&
                (applicationData.caub.pElems[1] == 1))
            {
                loopCount = MAKEWORD(
                    applicationData.caub.pElems[2],
                    applicationData.caub.pElems[3]);
            }
        }
    }

    return loopCount;
}

static _Check_return_ HRESULT GetGIFImageSize(IWICMetadataQueryReader &metadataQueryReader, uint32_t& outWidth, uint32_t& outHeight)
{
    wil::unique_prop_variant globalWidth;
    IFC_RETURN(metadataQueryReader.GetMetadataByName(L"/logscrdesc/Width", &globalWidth));
    IFCCHECK_RETURN(globalWidth.vt == VT_UI2);
    outWidth = globalWidth.uiVal;

    wil::unique_prop_variant globalHeight;
    IFC_RETURN(metadataQueryReader.GetMetadataByName(L"/logscrdesc/Height", &globalHeight));
    IFCCHECK_RETURN(globalHeight.vt == VT_UI2);
    outHeight = globalHeight.uiVal;

    wil::unique_prop_variant aspectRatio;
    IFC_RETURN(metadataQueryReader.GetMetadataByName(L"/logscrdesc/PixelAspectRatio", &aspectRatio));
    IFCCHECK_RETURN(aspectRatio.vt == VT_UI1);
    if (aspectRatio.bVal > 0)
    {
        // Need to calculate the ratio. The value in uPixelAspRatio 
        // allows specifying widest pixel 4:1 to the tallest pixel of 
        // 1:4 in increments of 1/64th
        float actualAspectRatio = (aspectRatio.bVal + 15.f) / 64.f;

        // Calculate the image width and height in pixel based on the
        // pixel aspect ratio. Only shrink the image.
        if (actualAspectRatio > 1.f)
        {
            outHeight = static_cast<uint32_t>(outHeight / actualAspectRatio);
        }
        else
        {
            outWidth = static_cast<uint32_t>(outWidth * actualAspectRatio);
        }
    }

    return S_OK;
}

static _Check_return_ HRESULT GetImageSize(IWICBitmapSource &bitmapSource, bool swapDimensions, uint32_t& outWidth, uint32_t& outHeight)
{
    auto width = 0u;
    auto height = 0u;
    IFC_RETURN(bitmapSource.GetSize(&width, &height));

    if (swapDimensions)
    {
        // Swap the width/height values so they match the orientation of the image.
        outWidth = height;
        outHeight = width;
    }
    else
    {
        outWidth = width;
        outHeight = height;
    }

    return S_OK;
}

_Check_return_ HRESULT WicService::WICGetTransformOptionFromMetadata(
    _In_ IWICMetadataQueryReader *pQueryReader,
    _Out_ WICBitmapTransformOptions *pOptions)
{
    *pOptions = WICBitmapTransformRotate0;

    PROPVARIANT varOrientation = {};
    HRESULT hr = pQueryReader->GetMetadataByName(L"System.Photo.Orientation", &varOrientation);
    if (hr == WINCODEC_ERR_PROPERTYNOTFOUND ||
        varOrientation.vt != VT_UI2)
    {
        *pOptions = WICBitmapTransformRotate0;
        hr = S_OK;
    }
    else if (SUCCEEDED(hr))
    {
        // These values of System.Photo.Orientation from in %INETROOT%\public\sdk\inc\propkey.h are given for each case.
        // System.Photo.Orientation rotations are in CW direction whereas WIC rotations are CCW
        switch (varOrientation.uiVal)
        {
        case PHOTO_ORIENTATION_ROTATE270:
            *pOptions = WICBitmapTransformRotate90; break;
        case PHOTO_ORIENTATION_ROTATE180:
            *pOptions = WICBitmapTransformRotate180; break;
        case PHOTO_ORIENTATION_ROTATE90:
            *pOptions = WICBitmapTransformRotate270; break;
        case PHOTO_ORIENTATION_TRANSPOSE:
            *pOptions = static_cast<WICBitmapTransformOptions>(WICBitmapTransformRotate90 | WICBitmapTransformFlipVertical); break;
        case PHOTO_ORIENTATION_TRANSVERSE:
            *pOptions = static_cast<WICBitmapTransformOptions>(WICBitmapTransformRotate270 | WICBitmapTransformFlipVertical); break;
        case PHOTO_ORIENTATION_FLIPHORIZONTAL:
            *pOptions = WICBitmapTransformFlipHorizontal; break;
        case PHOTO_ORIENTATION_FLIPVERTICAL:
            *pOptions = WICBitmapTransformFlipVertical; break;
        case PHOTO_ORIENTATION_NORMAL:
        default: // Unexpected values are ignored and no orientation is assumed
            *pOptions = WICBitmapTransformRotate0; break;
        }
    }

    PropVariantClear(&varOrientation);

    return hr;
}

_Check_return_ HRESULT WicService::GetMetadata(
    _In_ const wrl::ComPtr<IWICBitmapDecoder>& spBitmapDecoder,
    _Out_ ImageMetadata& imageMetadata
    )
{
    wrl::ComPtr<IWICBitmapFrameDecode> spBitmapFrameDecode;
    IFC_RETURN(spBitmapDecoder->GetFrame(0, &spBitmapFrameDecode));

    if (Switcher::HighestContractSupported() >= OSVersion::WIN10_19H1 || IsOrientationSupportedLegacy(spBitmapDecoder.Get()))
    {
        wrl::ComPtr<IWICMetadataQueryReader> spMetadataQueryReader;        
        const HRESULT hr = spBitmapFrameDecode->GetMetadataQueryReader(&spMetadataQueryReader);
        if (SUCCEEDED(hr))
        {
            IFC_RETURN(WICGetTransformOptionFromMetadata(spMetadataQueryReader.Get(), &imageMetadata.orientation));
        }
        // If the decoder doesn’t support metadata (BMP, ICO), this will return WINCODEC_ERR_UNSUPPORTEDOPERATION, which is expected. However, for rest of 
        // of the cases we will ifc retrun error codes.
        else if (hr != WINCODEC_ERR_UNSUPPORTEDOPERATION) 
        {
            IFC_RETURN(hr);
        }
    }

    wrl::ComPtr<IWICBitmapSource> spBitmapSource;
    IFC_RETURN(spBitmapFrameDecode.As(&spBitmapSource));

    IFC_RETURN(spBitmapDecoder->GetContainerFormat(&imageMetadata.containerFormat));

    // GIF is the only format right now that we support loop count with.
    // Metadata Query Reader may be not available for some images
    wrl::ComPtr<IWICMetadataQueryReader> spMetadataQueryReader;
    if (imageMetadata.containerFormat == GUID_ContainerFormatGif &&
        SUCCEEDED(spBitmapDecoder->GetMetadataQueryReader(&spMetadataQueryReader)))
    {
        imageMetadata.loopCount = GetLoopCount(*spMetadataQueryReader.Get());
        IFC_RETURN(spBitmapDecoder->GetFrameCount(&imageMetadata.frameCount));
        IFC_RETURN(GetGIFImageSize(*spMetadataQueryReader.Get(), imageMetadata.width, imageMetadata.height));
    }
    else
    {
        imageMetadata.loopCount = 0;
        imageMetadata.frameCount = 1;
        IFC_RETURN(GetImageSize(
            *spBitmapSource.Get(),
            imageMetadata.AreDimensionsSwapped(),
            imageMetadata.width,
            imageMetadata.height));
    }

    // JPEG images never support transparency.
    if (imageMetadata.containerFormat == GUID_ContainerFormatJpeg)
    {
        imageMetadata.supportsAlpha = false;
    }
    else
    {
        IFC_RETURN(ImagingUtility::IsHdrSource(spBitmapSource.Get(), &imageMetadata.isHdr));

        // Determine if frame 0 supports transparency
        IFC_RETURN(ImagingUtility::DoesSourceSupportAlpha(
            spBitmapSource,
            imageMetadata.supportsAlpha));
    }

    return S_OK;
}

std::unique_ptr<IImageDecoder> WicService::CreateDefaultDecoder(
    _In_ const ImageMetadata& imageMetadata
    )
{
    if (imageMetadata.containerFormat == GUID_ContainerFormatGif)
    {
        return std::unique_ptr<IImageDecoder>(wil::make_unique_failfast<WicAnimatedGifDecoder>().release());
    }
    else
    {
        return std::unique_ptr<IImageDecoder>(wil::make_unique_failfast<WicSingleImageDecoder>().release());
    }
}