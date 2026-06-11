// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <memory>
#include <MUX-ETWEvents.h>
#include <wincodec.h>
#include "ImageProviderInterfaces.h"
#include "PixelFormat.h"
#include "palgfx.h"
#include "DoubleUtil.h"
#include "ImageMetadata.h"
#include "SurfaceDecodeParams.h"
#include "ImageDecodeParams.h"
#include "EncodedImageData.h"
#include "OfferableSoftwareBitmap.h"
#include "ThreadPoolService.h"
#include "ImagingUtility.h"
#include "WicSingleImageDecoder.h"

// TODO: Consider creating a simplified version of the Decode which just accepts a stream

using namespace DirectUI;

_Check_return_ HRESULT WicSingleImageDecoder::DecodeFrame(
    _In_ EncodedImageData& encodedImageData,
    _In_ const ImageDecodeParams& decodeParams,
    int frameIndex,
    _Out_ wrl::ComPtr<IWICBitmapSource>& bitmapSource,
    _Out_ std::chrono::milliseconds& frameDelay
    )
{
    bitmapSource.Reset();
    frameDelay = std::chrono::milliseconds(0);

    // TODO: These ETW events should ideally be moved up to the base class?  Or at least ported to animated gif class.
    TraceDecodeToSurfaceBegin();
    auto traceGuard = wil::scope_exit([] { TraceDecodeToSurfaceEnd(); });

    auto imageMetadata = encodedImageData.GetMetadata();

    wrl::ComPtr<IWICBitmapDecoder> spBitmapDecoder;
    IFC_RETURN(encodedImageData.CreateWicBitmapDecoder(spBitmapDecoder));

    wrl::ComPtr<IWICBitmapFrameDecode> spBitmapFrameDecode;
    IFC_RETURN(spBitmapDecoder->GetFrame(0, &spBitmapFrameDecode));

    // Current stage tracks the WIC pipeline as it is constructed since some pieces
    // are optional like the scaler and the color transform.
    wrl::ComPtr<IWICBitmapSource> spCurrentStage = spBitmapFrameDecode;

    if (decodeParams.GetDecodeWidth() != 0 || decodeParams.GetDecodeHeight() != 0)
    {
        auto scaledSize = ImagingUtility::CalculateScaledSize(imageMetadata, decodeParams);

        wrl::ComPtr<IWICBitmapScaler> spScaler;
        IFC_RETURN(ImagingUtility::CreateDefaultScaler(scaledSize.Width, scaledSize.Height, spCurrentStage, spScaler));
        spCurrentStage = spScaler;
    }

    // Perform color management, ignore errors.
    // Color management may not be necessary in which case spColorTransform will be null and it should
    // passthrough the current stage.

    WICPixelFormatGUID pixelFormat = GUID_WICPixelFormatUndefined;
    switch (decodeParams.GetFormat())
    {
    case pixelColor64bpp_R16G16B16A16_Float:
        pixelFormat = GUID_WICPixelFormat64bppPRGBAHalf;
        break;
    case pixelColor32bpp_A8R8G8B8: // FIXME: BGRA?
        pixelFormat = GUID_WICPixelFormat32bppPBGRA;
        break;
    default:
        ASSERT(false);
    }

    wrl::ComPtr<IWICColorTransform> spColorTransform;
    if (SUCCEEDED(ImagingUtility::CreateDefaultColorTransform(pixelFormat, spBitmapFrameDecode, spCurrentStage, spColorTransform)) &&
        (spColorTransform != nullptr))
    {
        spCurrentStage = spColorTransform;
    }
    else
    {
        // The color transform also does pixel conversion.  If it was not used, check to see if a format conversion
        // needs to happen and create the converter if necessary.
        WICPixelFormatGUID currentStagePixelFormat;

        IFC_RETURN(spCurrentStage->GetPixelFormat(&currentStagePixelFormat));

        if (currentStagePixelFormat != pixelFormat)
        {
            wrl::ComPtr<IWICFormatConverter> spFormatConverter;
            IFC_RETURN(ImagingUtility::CreateDefaultConverter(pixelFormat, spCurrentStage, spFormatConverter));
            spCurrentStage = spFormatConverter;
        }
    }

    // Automatically adjust orientation for photos with embedded rotation flag
    if (imageMetadata.orientation != WICBitmapTransformRotate0)
    {
        wrl::ComPtr<IWICBitmapFlipRotator> spBitmapFlipRotator;
        IFC_RETURN(ImagingUtility::CreateDefaultFlipRotator(imageMetadata.orientation, spCurrentStage, spBitmapFlipRotator));
        spCurrentStage = spBitmapFlipRotator;
    }

    bitmapSource = std::move(spCurrentStage);

    return S_OK;
}
