// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <memory>
#include <MUX-ETWEvents.h>
#include <wincodec.h>
#include "DoubleUtil.h"
#include "ImageProviderInterfaces.h"
#include "PixelFormat.h"
#include "palgfx.h"
#include "GraphicsUtility.h"
#include "SurfaceDecodeParams.h"
#include "ImageDecodeParams.h"
#include "OfferableSoftwareBitmap.h"
#include "ImageMetadata.h"
#include "WicBitmapLock.h"
#include "WicService.h"
#include "ImagingUtility.h"
#include "AsyncImageDecoder.h"
#include "ImagingTelemetry.h"
#include <Mferror.h>
#include "TinyRGB.h"

using namespace DirectUI;

template<typename T>
using VectorOfVectors = std::vector<std::vector<T>>;

// TODO: Change SurfaceUpdateList to use a new type for hardware surfaces that isn't CXcpObject...
//       this will need to be integrated into the rest of XAML.

static _Check_return_ HRESULT IsKnownSRGBProfile(
    _In_ const wrl::ComPtr<IWICColorContext>& spColorContext,
    _Outref_ bool& isKnown
    )
{
    isKnown = false;

    // The size of the sRGB IEC 61966-2.1 profile
    // We use this to check if an embedded profile is the length of this specific profile
    static const uint32_t s_sRGBProfileBytesSize = 3144;

    auto bytesRead = 0u;
    IFC_RETURN(spColorContext->GetProfileBytes(0, nullptr, &bytesRead));

    // Check to see if the embedded ICC profile is 3144 bytes (the standard sRGB profile is 3144 bytes)
    if (bytesRead == s_sRGBProfileBytesSize)
    {
        uint8_t profileBytes[s_sRGBProfileBytesSize];

        // Might be sRGB; check the bytes in the header
        IFC_RETURN(spColorContext->GetProfileBytes(bytesRead, profileBytes, &bytesRead));

        // We parse the profile header ourselves instead of loading in mscms.dll (500kb)
        // ICC profile headers are simple 128 byte structures defined at color.org
        // 0x34 is the offset to the DeviceID tag in the ICC header
        // for sRGB profiles, the DeviceID == 'sRGB'
        if (0 == memcmp(profileBytes + 0x34, "sRGB", 4))
        {
            isKnown = true;
        }
    }
    // Check to see whether embedded ICC profile is 524 bytes (facebook's sRGB profile is 524 bytes)
    else if (bytesRead == _countof(s_facebookProfile))
    {
        uint8_t profileBytes[_countof(s_facebookProfile)];

        // Get the color profile.
        IFC_RETURN(spColorContext->GetProfileBytes(bytesRead, profileBytes, &bytesRead));

        // Compare the profile to the stored profile. If they match we recognize it as sRGB.
        if (0 == memcmp(profileBytes, s_facebookProfile, bytesRead))
        {
            isKnown = true;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ImagingUtility::ClearBitmap(
    _In_ wrl::ComPtr<IWICBitmap>& spBitmap,
    _In_ const WICRect& rect,
    WICColor color
    )
{
    WicBitmapLock wicBitmapLock(nullptr, WICBitmapLockFlags::WICBitmapLockWrite, spBitmap);

    auto left = rect.X;
    auto top = rect.Y;
    auto right = rect.X + rect.Width;
    auto bottom = rect.Y + rect.Height;

    IFCCHECK_RETURN(right <= static_cast<int32_t>(wicBitmapLock.GetWidth()));
    IFCCHECK_RETURN(bottom <= static_cast<int32_t>(wicBitmapLock.GetHeight()));

    uint8_t* pBufferLine = wicBitmapLock.GetBuffer() + (top * wicBitmapLock.GetStride());

    for (auto y = top; y < bottom; y++)
    {
        uint32_t* pBufferLineInPixels = reinterpret_cast<uint32_t*>(pBufferLine);

        for (auto x = left; x < right; x++)
        {
            pBufferLineInPixels[x] = color;
        }

        pBufferLine += wicBitmapLock.GetStride();
    }

    return S_OK;
}

// Optimized software blt that assumes an input IWICBitmap in the
// GUID_WICPixelFormat32bppPBGRA pixel format and rectangles of equal size.
// It assumes the A-channel can only have one of two values: 0xff of 0x00
_Check_return_ HRESULT ImagingUtility::GifBltPBGRANoBlend(
    _In_opt_ const WICRect* pSrcRect,
    _In_ const wrl::ComPtr<IWICBitmap>& spSrcWicBitmap,
    _In_opt_ const WICRect* pDstRect,
    _In_ const wrl::ComPtr<IWICBitmap>& spDstWicBitmap
    )
{
    WicBitmapLock srcFrameLock(
        pSrcRect,
        WICBitmapLockFlags::WICBitmapLockRead,
        spSrcWicBitmap);

    WicBitmapLock dstBitmapLock(
        pDstRect,
        static_cast<WICBitmapLockFlags>(WICBitmapLockFlags::WICBitmapLockWrite | WICBitmapLockFlags::WICBitmapLockRead),
        spDstWicBitmap);

    IFCCHECK_RETURN(srcFrameLock.GetWidth() == dstBitmapLock.GetWidth());
    IFCCHECK_RETURN(srcFrameLock.GetHeight() == dstBitmapLock.GetHeight());

    // TODO: Cancellation would work nicely here.
    for (uint32_t y = 0; y < srcFrameLock.GetHeight(); y++)
    {
        auto src = reinterpret_cast<const uint32_t*>(srcFrameLock.GetBuffer() + y * srcFrameLock.GetStride());
        auto dst = reinterpret_cast<uint32_t*>(dstBitmapLock.GetBuffer() + y * dstBitmapLock.GetStride());

        for (uint32_t x = 0; x < srcFrameLock.GetWidth(); x++)
        {
            // Note that signed shift has an implementation defined behavior.
            // Visual C++ compiler performs the sign extension which is what want.
            int32_t mask = static_cast<int32_t>(*src) >> 24;

            // The mask is 0x00000000 for transparent source and is 0xffffffff for opaque source.
            // No need to mask the source as it already comes pre-multiplied with alpha.
            *dst = *src | *dst & ~mask;

            dst++;
            src++;
        }
    }

    return S_OK;
}

// Optimized software blt that assumes an input IWICBitmap in the
// GUID_WICPixelFormat32bppBGRA pixel format and rectangles of equal size.
_Check_return_ HRESULT ImagingUtility::BltBGRA(
    _In_opt_ const WICRect* pSrcRect,
    _In_ const wrl::ComPtr<IWICBitmap>& spSrcWicBitmap,
    _In_opt_ const WICRect* pDstRect,
    _In_ const wrl::ComPtr<IWICBitmap>& spDstWicBitmap
    )
{
    WicBitmapLock srcFrameLock(
        pSrcRect,
        WICBitmapLockFlags::WICBitmapLockRead,
        spSrcWicBitmap);

    WicBitmapLock dstBitmapLock(
        pDstRect,
        static_cast<WICBitmapLockFlags>(WICBitmapLockFlags::WICBitmapLockWrite | WICBitmapLockFlags::WICBitmapLockRead),
        spDstWicBitmap);

    IFCCHECK_RETURN(srcFrameLock.GetWidth() == dstBitmapLock.GetWidth());
    IFCCHECK_RETURN(srcFrameLock.GetHeight() == dstBitmapLock.GetHeight());

    auto lineLengthInBytes = srcFrameLock.GetWidthInBytes();
    uint8_t* pDeltaFrameRow = srcFrameLock.GetBuffer();
    uint8_t* pCompositionFrameRow = dstBitmapLock.GetBuffer();

    // Enable full frame optimization if the width in bytes is equal to the stride for both images.
    // Then it can use a straight memcpy without iterating through the lines.
    // Note that height also has to match, but it was verified above.
    bool fullFrameOptimization =
        (lineLengthInBytes == srcFrameLock.GetStride()) &&
        (lineLengthInBytes == dstBitmapLock.GetStride());

    if (fullFrameOptimization)
    {
        memcpy(pCompositionFrameRow, pDeltaFrameRow, srcFrameLock.GetBufferSize());
    }
    else
    {
        // TODO: Explicitly test this path.
        // TODO: Cancellation would work nicely here.
        for (auto y = 0u; y < srcFrameLock.GetHeight(); y++)
        {
            memcpy(pCompositionFrameRow, pDeltaFrameRow, lineLengthInBytes);

            pDeltaFrameRow += srcFrameLock.GetStride();
            pCompositionFrameRow += dstBitmapLock.GetStride();
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ImagingUtility::IsHdrSource(
    _In_ IWICBitmapSource* bitmapSource,
    _Out_ bool* isHdrSource
    )
{
    *isHdrSource = false;

    WICPixelFormatGUID pixelFormat;
    IFC_RETURN(bitmapSource->GetPixelFormat(&pixelFormat));

    if (pixelFormat == GUID_WICPixelFormat64bppPRGBAHalf ||
        pixelFormat == GUID_WICPixelFormat64bppRGBAHalf ||
        pixelFormat == GUID_WICPixelFormat64bppRGBHalf )
    {
        *isHdrSource = true;
    }

    return S_OK;
}

_Check_return_ HRESULT ImagingUtility::DoesSourceSupportAlpha(
    _In_ const wrl::ComPtr<IWICBitmapSource>& spBitmapSource,
    _Out_ bool& supportsAlpha
    )
{
    auto spWicFactory = WicService::GetInstance().GetFactory();

    // Determine if the image supports transparency
    supportsAlpha = true;

    // If the container supports transparency look at the data type and palette
    // to see if the image has transparency.
    WICPixelFormatGUID pixelFormat;
    IFC_RETURN(spBitmapSource->GetPixelFormat(&pixelFormat));

    wrl::ComPtr<IWICComponentInfo> spComponentInfo;
    IFC_RETURN(spWicFactory->CreateComponentInfo(pixelFormat, &spComponentInfo));

    wrl::ComPtr<IWICPixelFormatInfo2> spPixelFormatInfo2;
    IFC_RETURN(spComponentInfo.As(&spPixelFormatInfo2));

    auto pixelFormatSupportsTransparency = FALSE;
    IFC_RETURN(spPixelFormatInfo2->SupportsTransparency(&pixelFormatSupportsTransparency));

    // For indexed pixel formats WIC will return FALSE for the
    // SupportsTransparency check although this depends on the palette.
    if (!pixelFormatSupportsTransparency
        && pixelFormat != GUID_WICPixelFormat1bppIndexed
        && pixelFormat != GUID_WICPixelFormat2bppIndexed
        && pixelFormat != GUID_WICPixelFormat4bppIndexed
        && pixelFormat != GUID_WICPixelFormat8bppIndexed)
    {
        supportsAlpha = false;
    }
    else
    {
        wrl::ComPtr<IWICPalette> spPalette;
        IFC_RETURN(spWicFactory->CreatePalette(&spPalette));

        if (SUCCEEDED(spBitmapSource->CopyPalette(spPalette.Get())))
        {
            auto paletteSupportsAlpha = FALSE;
            IFC_RETURN(spPalette->HasAlpha(&paletteSupportsAlpha));
            supportsAlpha = !!paletteSupportsAlpha;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ImagingUtility::CreateDefaultScaler(
    uint32_t targetWidth,
    uint32_t targetHeight,
    _In_ const wrl::ComPtr<IWICBitmapSource>& spInputSource,
    _Out_ wrl::ComPtr<IWICBitmapScaler>& spBitmapScaler
    )
{
    auto spWicFactory = WicService::GetInstance().GetFactory();

    // The logic below is to determine if XAML should use FANT scaling mode (high quality)
    // vs linear (low quality).  FANT has higher CPU cost but produces better results.
    // The goal is to determine the cases for a large scaling ratio where FANT quality
    // will be noticeably better than linear.  This logic is similar to Internet Explorer
    // and was provided as recommendation by the WIC team.
    WICBitmapInterpolationMode interpolationMode = WICBitmapInterpolationModeFant;

    bool isHdrSource = false;
    IFC_RETURN(IsHdrSource(spInputSource.Get(), &isHdrSource));

    // Only High-quality cubic scaling in WIC supports HDR sources
    if (isHdrSource)
    {
        interpolationMode = WICBitmapInterpolationModeHighQualityCubic;
    }
    else
    {
        auto sourceWidth = 0u;
        auto sourceHeight = 0u;
        IFC_RETURN(spInputSource->GetSize(&sourceWidth, &sourceHeight));

        wrl::ComPtr<IWICBitmapSourceTransform> spBitmapSourceTransform;
        if (SUCCEEDED(spInputSource.As(&spBitmapSourceTransform)))
        {
            // WIC Scalers QI their sources for IWICBitmapSourceTransform and
            // use that interface to determine if the source can accomplish
            // part of the scale.  If it can, the bitmap scaler uses this support
            // to scale to the closest size and then scales to the target size using
            // the interpolation mode specified.  This is most important when the
            // source is the WIC jpeg frame decoder.  The jpeg decoder performs DCT
            // down scales by factors of 2x, 4x, and 8x.  This means we can choose
            // linear in more cases if we perform a similar check here.
            auto closestWidth = targetWidth;
            auto closestHeight = targetHeight;
            IFC_RETURN(spBitmapSourceTransform->GetClosestSize(&closestWidth, &closestHeight));

            // Only improve the check when the source's scaling provides a downscale
            // that is larger or equal to our target size.
            if ((closestWidth < sourceWidth && closestHeight < sourceHeight) &&
                (closestWidth >= targetWidth && closestHeight >= targetHeight))
            {
                sourceWidth = closestWidth;
                sourceHeight = closestHeight;
            }
        }

        if ((targetWidth >= (sourceWidth / 2)) &&
            (targetHeight >= (sourceHeight / 2)))
        {
            interpolationMode = WICBitmapInterpolationModeLinear;
        }
    }

    IFC_RETURN(spWicFactory->CreateBitmapScaler(&spBitmapScaler));
    IFC_RETURN(spBitmapScaler->Initialize(
        spInputSource.Get(),
        targetWidth,
        targetHeight,
        interpolationMode));

    return S_OK;
}

_Check_return_ HRESULT ImagingUtility::CreateDefaultConverter(
    const WICPixelFormatGUID& targetFormat,
    _In_ const wrl::ComPtr<IWICBitmapSource>& spInputSource,
    _Out_ wrl::ComPtr<IWICFormatConverter>& spFormatConverter
    )
{
    auto spWicFactory = WicService::GetInstance().GetFactory();

    IFC_RETURN(spWicFactory->CreateFormatConverter(&spFormatConverter));
    IFC_RETURN(spFormatConverter->Initialize(
        spInputSource.Get(),
        targetFormat,
        WICBitmapDitherTypeNone,
        nullptr /* pIPalette */,
        0.0f /* alphaThresholdPercent */,
        WICBitmapPaletteTypeCustom));

    return S_OK;
}

_Check_return_ HRESULT ImagingUtility::CreateDefaultColorTransform(
    _In_ const WICPixelFormatGUID& targetFormat,
    _In_ const wrl::ComPtr<IWICBitmapFrameDecode>& spBitmapFrameDecode,
    _In_ const wrl::ComPtr<IWICBitmapSource>& spInputSource,
    _Out_ wrl::ComPtr<IWICColorTransform>& spColorTransform
    )
{
    spColorTransform = nullptr;

    auto spWicFactory = WicService::GetInstance().GetFactory();

    // Check if there is any color content.  This function may return WINCODEC_ERR_UNSUPPORTEDOPERATION
    // for some image types, so don't trace on errors.
    auto actualColorContext = 0u;
    IFC_NOTRACE_RETURN(spBitmapFrameDecode->GetColorContexts(0, nullptr, &actualColorContext));

    if (actualColorContext > 0)
    {
        wrl::ComPtr<IWICColorContext> spContextSrc;
        IFC_RETURN(spWicFactory->CreateColorContext(&spContextSrc));

        // We are only going to convert for the first color profile.
        IFC_NOTRACE_RETURN(spBitmapFrameDecode->GetColorContexts(1, spContextSrc.GetAddressOf(), &actualColorContext));

        // Check for embedded profiles
        // the image could have both an EXIF ColorSpace tag and an embedded profile
        auto colorContextType = WICColorContextUninitialized;
        IFC_RETURN(spContextSrc->GetType(&colorContextType));

        // We will only look at WICColorContextProfile
        // We don't care about the EXIF tag
        // It will either be sRGB, AdobeRGB or Uncalibrated; we would not convert in any of those cases
        // Because they are images with standard color profiles and should be
        // able to display correctly without any color correction. This will
        // save some CPU time and match the IE behavior.
        if (colorContextType == WICColorContextProfile)
        {
            auto isKnownSRGBProfile = true;
            IFC_RETURN(IsKnownSRGBProfile(spContextSrc, isKnownSRGBProfile));
            if (!isKnownSRGBProfile) // Do color transform
            {
                TraceImageUsesColorTransformInfo();

                wrl::ComPtr<IWICColorContext> spContextDst;
                IFC_RETURN(spWicFactory->CreateColorContext(&spContextDst));
                IFC_RETURN(spContextDst->InitializeFromExifColorSpace(1)); //sRGB

                IFC_RETURN(spWicFactory->CreateColorTransformer(&spColorTransform));
                IFC_RETURN(spColorTransform->Initialize(
                    spInputSource.Get(),
                    spContextSrc.Get(),
                    spContextDst.Get(),
                    targetFormat));
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ImagingUtility::CreateDefaultFlipRotator(
    WICBitmapTransformOptions transformOptions,
    _In_ const wrl::ComPtr<IWICBitmapSource>& spInputSource,
    _Out_ wrl::ComPtr<IWICBitmapFlipRotator>& spBitmapFlipRotator
    )
{
    auto spWicFactory = WicService::GetInstance().GetFactory();

    IFC_RETURN(spWicFactory->CreateBitmapFlipRotator(&spBitmapFlipRotator));
    IFC_RETURN(spBitmapFlipRotator->Initialize(spInputSource.Get(), transformOptions));

    return S_OK;
}

static _Check_return_ HRESULT CopyToSoftwareBitmap(
    _In_ IWICBitmapSource *bitmapSource,
    _Out_ xref_ptr<OfferableSoftwareBitmap>& spSoftwareBitmap
    )
{
    // TODO: Consider doing a cancellable staged copy...
    IFC_RETURN(bitmapSource->CopyPixels(
        nullptr,
        spSoftwareBitmap->GetStride(),
        spSoftwareBitmap->GetBufferSize(),
        reinterpret_cast<uint8_t*>(spSoftwareBitmap->GetBuffer())));

    return S_OK;
}

static _Check_return_ HRESULT CopyToHardwareTiles(
    _In_ IWICBitmapSource *bitmapSource,
    _In_ const ImageMetadata& imageMetadata,
    _Out_ const SurfaceUpdateList& surfaceUpdateList
    )
{
    // TODO: Clean this up to be new style... use auto initialization and
    //                 declare variables before use.

    // Copy size is a control variable that can be tweaked for Memory/Performance efficiency
    // It is used to determine the approximate size that is copied in each iteration.
    // If the image is rotated/flipped, WIC does no caching internally which greatly increases decode
    // time if you decode in strips, so in this case, increase CopySize to 8 MB to mitigate this bottleneck.
    static const uint32_t CopySize = (imageMetadata.orientation != WICBitmapTransformRotate0) ? (8 * 1024 * 1024) : (1024 * 1024);


    uint32_t width = 0;
    uint32_t height = 0;
    IFC_RETURN(bitmapSource->GetSize(&width, &height));

    WICPixelFormatGUID wicPixelFormat;
    IFC_RETURN(bitmapSource->GetPixelFormat(&wicPixelFormat));

    PixelFormat pixelFormat;
    IFC_RETURN(ImagingUtility::ConvertWicPixelFormat(wicPixelFormat, &pixelFormat));

    // Do the decode operation into a smaller temporary buffer (expensive operation) and then
    // lock the surface and copy to the DCompSurface (cheaper operation).  This way we don't
    // monopolize the D3DDevice lock for a long period of time and reduce the peak memory allocation
    // since it isn't allocating a single large surface to copy at a later time.
    //
    // The tiles must be copied in scanline order and the temporary buffer must span
    // the entire scanline.  The dimensions of the temporary buffer must be Width x N with
    // N as the number of lines to copy.  This must be done in order for WIC to continue a decoding
    // process incrementally, otherwise it will restart and re-decode parts of the image.
    //
    // If there is only 1 tile, then height should be calculated to an appropriate value
    // based on memory/performance consideration.  If there are multiple tiles, then height
    // should set to the tile height and all tiles should be equal height (except for the tiles at the bottom).
    // The reason for doing this is so that WIC will decode Width x Tile Height and then copy all tiles
    // in that scanline region before moving onto the next scanline region.
    uint32_t tileCount = static_cast<uint32_t>(surfaceUpdateList.size());

    FAIL_FAST_ASSERT(tileCount > 0);

    uint32_t tempBufferStride = GetPlaneStride(pixelFormat, 0, width);
    uint32_t tempBufferPixelSize = GetPixelStride(pixelFormat);
    uint32_t lineDecodeCount = 0;
    uint32_t tilesPerScanline = 0;
    uint32_t maxVerticalTiles = 0;
    uint32_t tileWidth = surfaceUpdateList[0]->GetRect().Width;
    uint32_t tileHeight = surfaceUpdateList[0]->GetRect().Height;

    if (tileCount == 1)
    {
        uint32_t maxLineDecodeCount = MAX(CopySize / tempBufferStride, 1);
        lineDecodeCount = MIN(maxLineDecodeCount, height);
        tilesPerScanline = 1;
        maxVerticalTiles = 1;
    }
    else
    {
        lineDecodeCount = tileHeight;

        tilesPerScanline = DivideIntAndRoundUp<uint32_t>(
            width,
            static_cast<uint32_t>(tileWidth));

        maxVerticalTiles = DivideIntAndRoundUp<uint32_t>(
            height,
            static_cast<uint32_t>(tileHeight));
    }

    FAIL_FAST_ASSERT((tilesPerScanline * maxVerticalTiles) == tileCount);

    // Do some tile validation so that it doesn't need to be done during the inner loop
    // This validates pre-conditions for this algorithm to operate efficiently.
    // This also creates scanline lists for all tiles that exist within the same scanlines
    auto pScanlineUpdateList = wil::make_unique_failfast<VectorOfVectors<SurfaceDecodeParams*>>(maxVerticalTiles);

    for (const xref_ptr<SurfaceDecodeParams>& spTile : surfaceUpdateList)
    {
        auto pSurface = spTile->GetSurface();
        int rectX = spTile->GetRect().X;
        int rectY = spTile->GetRect().Y;
        int rectWidth = spTile->GetRect().Width;
        int rectHeight = spTile->GetRect().Height;

        const bool formatCheck = (pSurface->GetPixelFormat() == pixelFormat);
        const bool lineCheck = ((rectY % lineDecodeCount) == 0);
        const bool isVirtual = pSurface->IsVirtual();
        const bool rectWidthCheck = isVirtual ? (width == pSurface->GetWidth()) : (rectWidth == pSurface->GetWidth());
        const bool rectHeightCheck = isVirtual ? (height == pSurface->GetHeight()) : (rectHeight == pSurface->GetHeight());
        const bool widthBoundsCheck = ((rectX + rectWidth) <= static_cast<int>(width));
        const bool heightBoundsCheck = ((rectY + rectHeight) <= static_cast<int>(height));

        FAIL_FAST_ASSERT(
            formatCheck &&
            lineCheck &&
            rectWidthCheck &&
            rectHeightCheck &&
            widthBoundsCheck &&
            heightBoundsCheck);

        uint32_t verticalTileIndex = spTile->GetRect().Y / tileHeight;

        (*pScanlineUpdateList)[verticalTileIndex].push_back(spTile.get());
    }

    // Allocate the temporary buffer
    uint32_t copySegmentSize = tempBufferStride * lineDecodeCount;
    auto pTempBuffer = wil::make_unique_failfast<uint8_t[]>(copySegmentSize);

    // Go through all the tiles and hold the flush so that they are flushed at the end.
    // This is to prevent textures from updating at inconsistent times for the user.
    for (auto& tile : surfaceUpdateList)
    {
        tile->GetSurface()->SetHoldFlush(true);
    }

    auto allowFlushOnExit = wil::scope_exit([&]
    {
        for (auto& tile : surfaceUpdateList)
        {
            tile->GetSurface()->SetHoldFlush(false);
        }
    });

    // Decode multiple scanlines from WIC
    WICRect wicSourceRect = {};
    wicSourceRect.X = 0;
    wicSourceRect.Y = 0;
    wicSourceRect.Width = width;
    wicSourceRect.Height = lineDecodeCount;

    for (uint32_t currentLine = 0; currentLine < height; currentLine += lineDecodeCount)
    {
        // Check to see if this is the last segment and then adjust the lines for the last segment.
        if ((currentLine + lineDecodeCount) >= height)
        {
            wicSourceRect.Height = height - currentLine;
            copySegmentSize = wicSourceRect.Height * tempBufferStride;
        }

        // Despite being called CopyPixels, this is what actually invokes the WIC pipeline to do the decoding
        IFC_RETURN(bitmapSource->CopyPixels(&wicSourceRect, tempBufferStride, copySegmentSize, pTempBuffer.get()));

        // Iterate through all the tiles.  This was intentionally made to iterate over all tiles
        // and match tiles for the current scanline since CTiledSurface does not guarantee tile
        // ordering.
        uint32_t verticalTileIndex = currentLine / tileHeight;

        for (SurfaceDecodeParams* pTile : (*pScanlineUpdateList)[verticalTileIndex])
        {
            auto pSurface = pTile->GetSurface();
            int rectX = pTile->GetRect().X;
            int rectY = pTile->GetRect().Y;
            int rectWidth = pTile->GetRect().Width;
            int rectHeight = pTile->GetRect().Height;

            const bool heightCheck = (rectHeight >= wicSourceRect.Height);
            const bool lineCheck = (currentLine >= static_cast<uint32_t>(rectY));

            FAIL_FAST_ASSERT(
                heightCheck &&
                lineCheck);

            // Early abort mechanic, do a quick AddRef/Release to get the reference count on the surface
            // If it equal to 1, then this decode is holding onto the last reference which means
            // the decode no longer needs to occur and can be released early.  This is also important
            // for device lost scenario in which case the hardware surface gets released, so early exit
            // is important so the hardware surface allocation isn't holding the device from being released.
            pSurface->AddRef();
            if (pSurface->Release() == 1)
            {
                return E_ABORT;
            }

            uint8_t* pDestinationBuffer = nullptr;
            int32_t destinationStride = 0;
            uint32_t destinationWidth = 0;
            uint32_t destinationHeight = 0;

            // First lock will allocate the staging texture ultimately through CD3D11DeviceInstance::AllocateFromSysMemBitsPool
            // This requires multi-threaded protection which is currently enforced with CD3D11SharedDeviceGuard
            if (pSurface->IsVirtual())
            {
                // Virtual will use the rect with offset to lock a rect inside a virtual surface.
                IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(pSurface->LockRect(
                    pTile->GetRect(),
                    reinterpret_cast<void**>(&pDestinationBuffer),
                    &destinationStride,
                    &destinationWidth,
                    &destinationHeight));
            }
            else
            {
                // Non-virtual needs to adjust the rect to use the same size but with X and Y as 0 since the surface
                // positioning is handled externally.
                // TODO_WinRTSprites: This else block can probably be deleted along with the rest of tiling code
                //                              when SpriteVisualsEnabled is universally enabled.
                XRECT lockRect = { 0, 0, rectWidth, rectHeight };

                IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(pSurface->LockRect(
                    lockRect,
                    reinterpret_cast<void**>(&pDestinationBuffer),
                    &destinationStride,
                    &destinationWidth,
                    &destinationHeight));
            }

            // Note: If any IFC_RETURN's are added in-between Lock and Unlock, then a cleanup check
            //       will be necessary to Unlock the surface if the IFC_RETURN fails.  Or IFCFAILFAST
            FAIL_FAST_ASSERT(destinationStride >= 0);

            // Copy the decoded lines from the temp buffer to the destination surface
            uint8_t* pDestinationLine = pDestinationBuffer + (destinationStride * (currentLine - rectY));
            uint32_t pixelOffset = rectX * tempBufferPixelSize;
            uint8_t* pTempBufferLine = pTempBuffer.get() + pixelOffset;
            uint32_t rectWidthInBytes = rectWidth * tempBufferPixelSize;

            for (int copyLine = 0; copyLine < wicSourceRect.Height; copyLine++)
            {
                memcpy_s(pDestinationLine, destinationStride, pTempBufferLine, rectWidthInBytes);

                pDestinationLine += destinationStride;
                pTempBufferLine += tempBufferStride;
            }

            // Unlock the surface and only queue the update when the last line to the surface/tile has been written.
            bool lastSurfaceUpdate = (wicSourceRect.Y + wicSourceRect.Height >= rectY + rectHeight);
            IFCFAILFAST(pSurface->Unlock(lastSurfaceUpdate));
        }

        wicSourceRect.Y += lineDecodeCount;
    }

    return S_OK;
}

XSIZE ImagingUtility::CalculateScaledSize(
    _In_ const ImageMetadata& imageMetadata,
    _In_ const ImageDecodeParams& decodeParams,
    bool clampToMetadataSize
    )
{
    uint32_t scaledWidth = decodeParams.GetDecodeWidth();
    uint32_t scaledHeight = decodeParams.GetDecodeHeight();

    auto metadataWidth = imageMetadata.width;
    auto metadataHeight = imageMetadata.height;

    if (imageMetadata.AreDimensionsSwapped())
    {
        scaledWidth = decodeParams.GetDecodeHeight();
        scaledHeight = decodeParams.GetDecodeWidth();

        metadataWidth = imageMetadata.height;
        metadataHeight = imageMetadata.width;
    }

    ASSERT(scaledWidth != 0 || scaledHeight != 0);

    if (scaledWidth == 0)
    {
        if (clampToMetadataSize)
        {
            scaledHeight = std::min(scaledHeight, metadataHeight);
        }

        double heightRatio = static_cast<double>(scaledHeight) / static_cast<double>(metadataHeight);

        scaledWidth = static_cast<uint32_t>(DoubleUtil::Round(metadataWidth * heightRatio, 0));
    }
    else if (scaledHeight == 0)
    {
        if (clampToMetadataSize)
        {
            scaledWidth = std::min(scaledWidth, metadataWidth);
        }

        double widthRatio = static_cast<double>(scaledWidth) / static_cast<double>(metadataWidth);

        scaledHeight = static_cast<uint32_t>(DoubleUtil::Round(metadataHeight * widthRatio, 0));
    }

    return XSIZE{ static_cast<int>(scaledWidth), static_cast<int>(scaledHeight) };
}

_Check_return_ HRESULT ImagingUtility::RealizeBitmapSource(
    _In_ const ImageMetadata& imageMetadata,
    _In_ IWICBitmapSource* bitmapSource,
    _In_ const ImageDecodeParams& decodeParams,
    _Out_ xref_ptr<OfferableSoftwareBitmap>& spSoftwareBitmap)
{
    // If input tiles aren't provided, then create a single temporary tile for decoding and return
    // the newly created surface in the response.
    // If input tiles are provided, decode directly to those surfaces and return a null surface pointer
    // in the response (which is typically used to populate the software surface).
    auto surfaceUpdateList = decodeParams.GetSurfaceUpdateList();

    if (AsyncImageDecoder::IsOffThreadDecodingSuspended())
    {
        // Bug 1667366 <Andromeda crash due to race condition in LoadedImageSource resize>
        // Test hook to repro a rare timing problem that leads to a crash. We need the decoding thread to proceed to
        // this point (having read the decode params off of the shared state with the UI thread), then we need the decoding
        // thread to switch out and the UI thread to resize the underlying buffer. We force a yield here to set that up.
        while (AsyncImageDecoder::IsOffThreadDecodingSuspended())
        {
            SwitchToThread();
        }

        // Sleep another 3 seconds once we're unblocked. This works around a deadlock that can occur. This method is called
        // while holding a mutex that the UI thread also touches, and the UI thread is the thread where the test helper
        // unsuspends off-thread decoding, so there is a likely deadlock here. The workaround is for the UI thread to unsuspend
        // off-thread decoding, and for the off-thread decode to wait 3 more seconds before proceeding. In those 3 seconds, the
        // UI thread can set up the repro for the crash.
        Sleep(3000);
    }

    const bool isHardwareDecode = !surfaceUpdateList.empty();

    ImagingTelemetry::OffThreadDecodeStart(decodeParams.GetImageId(), decodeParams.GetStrSource().GetBuffer(), isHardwareDecode);

    if (isHardwareDecode)
    {
        // http://osgvsowi/6846173 it is possible to return E_ABORT which is handled as a special case
        // and fails gracefully.
        SuspendFailFastOnStowedException suspender;

        IFC_RETURN(CopyToHardwareTiles(bitmapSource, imageMetadata, surfaceUpdateList));
    }
    else
    {
        auto pipelineOutputWidth = 0u;
        auto pipelineOutputHeight = 0u;
        IFC_RETURN(bitmapSource->GetSize(&pipelineOutputWidth, &pipelineOutputHeight));

        spSoftwareBitmap = make_xref<OfferableSoftwareBitmap>(
            decodeParams.GetFormat(),
            pipelineOutputWidth,
            pipelineOutputHeight,
            !imageMetadata.supportsAlpha);

        SuspendFailFastOnStowedException suspender(MF_E_TOPO_CODEC_NOT_FOUND);
        IFC_RETURN(CopyToSoftwareBitmap(bitmapSource, spSoftwareBitmap));
    }

    ImagingTelemetry::OffThreadDecodeStop(decodeParams.GetImageId());

    return S_OK;
}

_Check_return_ HRESULT ImagingUtility::ConvertWicPixelFormat(
    _In_ WICPixelFormatGUID wicPixelFormat,
    _Out_ PixelFormat* pixelFormat
    )
{
    *pixelFormat = pixelUnknown;

    if (wicPixelFormat == GUID_WICPixelFormat32bppPBGRA ||
        wicPixelFormat == GUID_WICPixelFormat32bppBGRA ||
        wicPixelFormat == GUID_WICPixelFormat32bppBGR)
    {
        *pixelFormat = pixelColor32bpp_A8R8G8B8;
    }
    else if (wicPixelFormat == GUID_WICPixelFormat64bppPRGBAHalf)
    {
        *pixelFormat = pixelColor64bpp_R16G16B16A16_Float;
    }
    else
    {
        return E_FAIL;
    }

    return S_OK;
}

