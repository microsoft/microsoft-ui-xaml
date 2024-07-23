// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// NOTE: The core algorithm in this file was referenced from Microsoft Animated Gif Sample
//       https://code.msdn.microsoft.com/Windows-Imaging-Component-65abbc6a

#include "precomp.h"
#include <memory>
#include <memory>
#include <MUX-ETWEvents.h>
#include <wincodec.h>
#include "ImageProviderInterfaces.h"
#include "PixelFormat.h"
#include "palgfx.h"
#include "RawData.h"
#include "ImageMetadata.h"
#include "SurfaceDecodeParams.h"
#include "ImageDecodeParams.h"
#include "OfferableSoftwareBitmap.h"
#include "EncodedImageData.h"
#include "ImagingUtility.h"
#include "WicBitmapLock.h"
#include "WicService.h"
#include "WicAnimatedGifDecoder.h"
#include "DoubleUtil.h"

// OPTIMIZE: Potential memory and performance optimization
//       Pre-scan the gif at the beginning of decoding for the following :
//           DisposalMethod::Previous(Disregard previous frames)
//               Saves a surface
//           Only DisposalMethod::None with full frame and no alpha(don't need previous frames)
//               Saves another surface and allows for DTRS
//
//       Many internet standard gif's would need to be tested for these properties and
//       the performance of the metadata scanning would need to be tested.

static void ClipForBlt(uint32_t width, uint32_t height, _Inout_ WICRect &dstRect, _Out_ WICRect &srcClipped)
{
    // These are stored as unsigned in gif and never expected to be negative
    FAIL_FAST_ASSERT(dstRect.X >= 0);
    FAIL_FAST_ASSERT(dstRect.Y >= 0);
    FAIL_FAST_ASSERT(dstRect.Width >= 0);
    FAIL_FAST_ASSERT(dstRect.Height >= 0);

    if (dstRect.X + dstRect.Width > static_cast<INT>(width))
    {
        dstRect.Width = std::max(0, static_cast<INT>(width) - dstRect.X);
    }

    if (dstRect.Y + dstRect.Height > static_cast<INT>(height))
    {
        dstRect.Height = std::max(0, static_cast<INT>(height) - dstRect.Y);
    }

    srcClipped.X = 0;
    srcClipped.Y = 0;
    srcClipped.Width = dstRect.Width;
    srcClipped.Height = dstRect.Height;
}

_Check_return_ HRESULT WicAnimatedGifDecoder::DecodeFrame(
    _In_ EncodedImageData& encodedImageData,
    _In_ const ImageDecodeParams& decodeParams,
    int frameIndex,
    _Out_ wrl::ComPtr<IWICBitmapSource>& bitmapSource,
    _Out_ std::chrono::milliseconds& frameDelay
    )
{
    bitmapSource.Reset();

    TraceDecodeToSurfaceBegin();
    auto traceGuard = wil::scope_exit([] { TraceDecodeToSurfaceEnd(); });

    auto &imageMetadata = encodedImageData.GetMetadata();
    ASSERT(frameIndex < static_cast<int>(imageMetadata.frameCount));

    if (frameIndex < m_currentFrameIndex)
    {
        m_currentFrameIndex = -1;
    }

    while (m_currentFrameIndex != frameIndex)
    {
        // Loop handling is done by the caller of this function.
        m_currentFrameIndex++;
        if (m_currentFrameIndex == 0)
        {
            m_currentDeltaFrameInfo.disposalMethod = DisposalMethod::Background;
            m_currentDeltaFrameInfo.bounds = WICRect{ 0, 0, (INT)imageMetadata.width, (INT)imageMetadata.height };
        }

        wrl::ComPtr<IWICBitmapDecoder> spBitmapDecoder;
        IFC_RETURN(encodedImageData.CreateWicBitmapDecoder(spBitmapDecoder));

        // TODO: All wic operations should be mutexed through the service and never provide direct
        //                 access to the factory.
        auto spWicFactory = WicService::GetInstance().GetFactory();

        // Only format we currently support is BGRA.
        // Future support for other formats would need to be done by converting PixelFormat to the appropriate
        // WICPixelFormatGUID and adding hardware support in HWTexture.  For now, this keeps it simple.
        FAIL_FAST_ASSERT(decodeParams.GetFormat() == pixelColor32bpp_A8R8G8B8);

        if (m_spCurrentBitmap == nullptr)
        {
            IFC_RETURN(CreateWicBitmap(
                spWicFactory,
                imageMetadata,
                m_spCurrentBitmap.ReleaseAndGetAddressOf()));
        }

        // Disposal method disposes the previous frame to prepare the current frame for composition.
        // The current frame is composed on the current frame with the appropriate bounds rect.
        switch (m_currentDeltaFrameInfo.disposalMethod)
        {
        case DisposalMethod::Undefined:
        case DisposalMethod::None:
            break;
        case DisposalMethod::Background:
            // GIF standard suggests using the background color but we use
            // transparent here to be consistent with what modern web browsers do.
            IFC_RETURN(ImagingUtility::ClearBitmap(m_spCurrentBitmap, m_currentDeltaFrameInfo.bounds, 0x00000000));
            break;
        case DisposalMethod::Previous:
            // This assert should never happen under the precondition that current DisposalMethod is initialized to
            // Undefined and it sets the m_spSavedBitmap when used.
            ASSERT(m_spSavedBitmap != nullptr);
            m_spCurrentBitmap.Swap(m_spSavedBitmap);
            break;
        }

        wrl::ComPtr<IWICBitmapSource> spDeltaFrameSource;
        IFC_RETURN(CreateDeltaFrameSource(spBitmapDecoder, m_currentFrameIndex, m_currentDeltaFrameInfo, spDeltaFrameSource));

        // Make a copy of the current bitmap to restore it later when disposing the current delta frame
        if (m_currentDeltaFrameInfo.disposalMethod == DisposalMethod::Previous)
        {
            if (m_spSavedBitmap == nullptr)
            {
                IFC_RETURN(CreateWicBitmap(
                    spWicFactory,
                    imageMetadata,
                    m_spSavedBitmap.ReleaseAndGetAddressOf()));
            }
            IFC_RETURN(ImagingUtility::BltBGRA(nullptr, m_spCurrentBitmap, nullptr, m_spSavedBitmap));
        }

        // Decode into a temporary SoftwareBitmap buffer.  This could be done by decoding
        // in-place into the composition bitmap.  However, the composition surface bits are shared outside
        // for software rasterization purposes and creating a read lock on the buffer for a long period of time
        // while decoding could cause a major performance issue on the UI thread.  The surface is also
        // shared to save memory by not creating extra copies when they are not explicitly needed.
        wrl::ComPtr<IWICBitmap> spDeltaFrameBitmap;
        IFC_RETURN(spWicFactory->CreateBitmapFromSource(
            spDeltaFrameSource.Get(),
            WICBitmapCreateCacheOption::WICBitmapCacheOnLoad,
            &spDeltaFrameBitmap));

        WICRect srcRect;
        ClipForBlt(imageMetadata.width, imageMetadata.height, m_currentDeltaFrameInfo.bounds, srcRect);

        // Draw the delta frame on top of the composition bitmap using alpha blending if appropriate
        if (m_currentDeltaFrameInfo.supportsAlpha)
        {
            // Blt using pre-multiplied alpha (PBGRA)
            // Note the 'P'
            IFC_RETURN(ImagingUtility::GifBltPBGRANoBlend(&srcRect, spDeltaFrameBitmap, &m_currentDeltaFrameInfo.bounds, m_spCurrentBitmap));
        }
        else
        {
            // Blt without any alpha blending (BGRA)
            // Note the lack of 'P'
            IFC_RETURN(ImagingUtility::BltBGRA(&srcRect, spDeltaFrameBitmap, &m_currentDeltaFrameInfo.bounds, m_spCurrentBitmap));
        }
    }

    wrl::ComPtr<IWICBitmapSource> spCurrentStage = m_spCurrentBitmap;

    if (decodeParams.GetDecodeWidth() != 0 || decodeParams.GetDecodeHeight() != 0)
    {
        auto scaledSize = ImagingUtility::CalculateScaledSize(imageMetadata, decodeParams);

        wrl::ComPtr<IWICBitmapScaler> spScaler;
        IFC_RETURN(ImagingUtility::CreateDefaultScaler(scaledSize.Width, scaledSize.Height, spCurrentStage, spScaler));
        spCurrentStage = spScaler;
    }

    bitmapSource = std::move(spCurrentStage);
    frameDelay = m_currentDeltaFrameInfo.delay;

    return S_OK;
}

HRESULT WicAnimatedGifDecoder::CreateDeltaFrameSource(
    _In_ const wrl::ComPtr<IWICBitmapDecoder>& spBitmapDecoder,
    int frameIndex,
    _Out_ DeltaFrameInfo& deltaFrameInfo,
    _Out_ wrl::ComPtr<IWICBitmapSource>& spBitmapSource
    )
{
    wrl::ComPtr<IWICBitmapFrameDecode> spWicBitmapFrameDecode;
    IFC_RETURN(spBitmapDecoder->GetFrame(
        frameIndex,
        &spWicBitmapFrameDecode));

    IFC_RETURN(GetDeltaFrameInfo(spWicBitmapFrameDecode, deltaFrameInfo));

    wrl::ComPtr<IWICFormatConverter> spFormatConverter;
    IFC_RETURN(ImagingUtility::CreateDefaultConverter(
        GUID_WICPixelFormat32bppPBGRA,
        spWicBitmapFrameDecode,
        spFormatConverter));

    IFC_RETURN(spFormatConverter.As(&spBitmapSource));

    return S_OK;
}

_Check_return_ HRESULT WicAnimatedGifDecoder::GetDeltaFrameInfo(
    _In_ const wrl::ComPtr<IWICBitmapFrameDecode>& spBitmapFrameDecode,
    _Out_ DeltaFrameInfo& deltaFrameInfo
    )
{
    wrl::ComPtr<IWICMetadataQueryReader> spMetadataQueryReader;
    IFC_RETURN(spBitmapFrameDecode->GetMetadataQueryReader(&spMetadataQueryReader));

    wil::unique_prop_variant propLeft;
    IFC_RETURN(spMetadataQueryReader->GetMetadataByName(L"/imgdesc/Left", &propLeft));
    IFCCHECK_RETURN(propLeft.vt == VT_UI2);
    deltaFrameInfo.bounds.X = static_cast<INT>(propLeft.uiVal);

    wil::unique_prop_variant propTop;
    IFC_RETURN(spMetadataQueryReader->GetMetadataByName(L"/imgdesc/Top", &propTop));
    IFCCHECK_RETURN(propTop.vt == VT_UI2);
    deltaFrameInfo.bounds.Y = static_cast<INT>(propTop.uiVal);

    wil::unique_prop_variant propWidth;
    IFC_RETURN(spMetadataQueryReader->GetMetadataByName(L"/imgdesc/Width", &propWidth));
    IFCCHECK_RETURN(propWidth.vt == VT_UI2);
    deltaFrameInfo.bounds.Width = static_cast<INT>(propWidth.uiVal);

    wil::unique_prop_variant propHeight;
    IFC_RETURN(spMetadataQueryReader->GetMetadataByName(L"/imgdesc/Height", &propHeight));
    IFCCHECK_RETURN(propHeight.vt == VT_UI2);
    deltaFrameInfo.bounds.Height = static_cast<INT>(propHeight.uiVal);

    wil::unique_prop_variant propDelay;
    if (SUCCEEDED(spMetadataQueryReader->GetMetadataByName(L"/grctlext/Delay", &propDelay)))
    {
        IFCCHECK_RETURN(propDelay.vt == VT_UI2);
    }
    else
    {
        // Failed to get delay from optional graphic control extension. Possibly a
        // single frame image (non-animated gif)
        propDelay.uiVal = 0;
    }

    // Replicate browsers behavior which is:
    //
    // Per GIF89a, the animated GIF delay time is in hundredths (1/100) of a second. Since we track delay in milliseconds,
    // these values will always be in multiples of 10. Some GIF creation tools specified very small delays of 0 or 1, expecting
    // browsers to be limited to an effective speed of 10 (100 ms). However, performance has increased and GIF files now use
    // delays of 2 expecting to get 50fps.
    // Therefore:
    //      1. Honor the delay down to and including 2
    //      2. If the delay value is 0 or 1, set it to 10 intervals (100 ms).
    if (propDelay.uiVal < 2)
    {
        propDelay.uiVal = 10;
    }

    // Convert the delay retrieved in 10 ms units to a delay in 1 ms units
    deltaFrameInfo.delay = std::chrono::milliseconds(propDelay.uiVal * 10);

    wil::unique_prop_variant propDisposal;
    if (SUCCEEDED(spMetadataQueryReader->GetMetadataByName(L"/grctlext/Disposal", &propDisposal)))
    {
        IFCCHECK_RETURN(propDisposal.vt == VT_UI1);
        deltaFrameInfo.disposalMethod = (propDisposal.bVal >= 0 && propDisposal.bVal <= 3) ? 
            static_cast<DisposalMethod>(propDisposal.bVal) :
            DisposalMethod::Undefined;
    }
    else
    {
        // Failed to get the disposal method, use default. Possibly a non-animated gif.
        deltaFrameInfo.disposalMethod = DisposalMethod::Undefined;
    }

    // Each frame could have different alpha support, this information is used
    // to determine if the del
    IFC_RETURN(ImagingUtility::DoesSourceSupportAlpha(
        spBitmapFrameDecode,
        deltaFrameInfo.supportsAlpha));

    return S_OK;
}

// static
_Check_return_ HRESULT WicAnimatedGifDecoder::CreateWicBitmap(
    const wrl::ComPtr<IWICImagingFactory> spFactory,
    const ImageMetadata &imageMetadata,
    _Outptr_result_nullonfailure_ IWICBitmap **result)
{
    *result = nullptr;
    wrl::ComPtr<IWICBitmap> spResult;

    WICPixelFormatGUID pixelFormat = imageMetadata.supportsAlpha ? GUID_WICPixelFormat32bppPBGRA : GUID_WICPixelFormat32bppBGR;

    IFC_RETURN(spFactory->CreateBitmap(
        imageMetadata.width,
        imageMetadata.height,
        pixelFormat,
        WICBitmapCreateCacheOption::WICBitmapCacheOnDemand,
        &spResult));

    *result = spResult.Detach();

    return S_OK;
}
