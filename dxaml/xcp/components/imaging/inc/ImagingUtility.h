// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "palgfx.h"
#include <wincodec.h>

// TODO: Check if SoftwareBitmap or XAML needs to call AddMemoryPressure/RemoveMemoryPressure on ISoftwareBitmap's and IWICBitmap's
//                 Check with Mike on this as well... this could be very important... what do we do in the current code base for IWICBitmap?

struct ImageMetadata;
class ImageDecodeParams;
class OfferableSoftwareBitmap;

namespace ImagingUtility {

_Check_return_ HRESULT ClearBitmap(
    _In_ wrl::ComPtr<IWICBitmap>& spBitmap,
    _In_ const WICRect& rect,
    WICColor color);

_Check_return_ HRESULT GifBltPBGRANoBlend(
    _In_opt_ const WICRect* pSrcRect,
    _In_ const wrl::ComPtr<IWICBitmap>& spSrcWicBitmap,
    _In_opt_ const WICRect* pDstRect,
    _In_ const wrl::ComPtr<IWICBitmap>& spDstWicBitmap
    );

_Check_return_ HRESULT BltBGRA(
    _In_opt_ const WICRect* pSrcRect,
    _In_ const wrl::ComPtr<IWICBitmap>& spSrcWicBitmap,
    _In_opt_ const WICRect* pDstRect,
    _In_ const wrl::ComPtr<IWICBitmap>& spDstWicBitmap
    );

_Check_return_ HRESULT IsHdrSource(
    _In_ IWICBitmapSource* bitmapSource,
    _Out_ bool* isHdrSource
    );

_Check_return_ HRESULT DoesSourceSupportAlpha(
    _In_ const wrl::ComPtr<IWICBitmapSource>& spBitmapSource,
    _Out_ bool& supportsAlpha
    );

_Check_return_ HRESULT CreateDefaultScaler(
    uint32_t targetWidth,
    uint32_t targetHeight,
    _In_ const wrl::ComPtr<IWICBitmapSource>& spInputSource,
    _Out_ wrl::ComPtr<IWICBitmapScaler>& spBitmapScaler
    );

_Check_return_ HRESULT CreateDefaultConverter(
    const WICPixelFormatGUID& targetFormat,
    _In_ const wrl::ComPtr<IWICBitmapSource>& spInputSource,
    _Out_ wrl::ComPtr<IWICFormatConverter>& spFormatConverter
    );

_Check_return_ HRESULT CreateDefaultColorTransform(
    _In_ const WICPixelFormatGUID& targetFormat,
    _In_ const wrl::ComPtr<IWICBitmapFrameDecode>& spBitmapFrameDecode,
    _In_ const wrl::ComPtr<IWICBitmapSource>& spInputSource,
    _Out_ wrl::ComPtr<IWICColorTransform>& spColorTransform
    );

_Check_return_ HRESULT CreateDefaultFlipRotator(
    WICBitmapTransformOptions transformOptions,
    _In_ const wrl::ComPtr<IWICBitmapSource>& spInputSource,
    _Out_ wrl::ComPtr<IWICBitmapFlipRotator>& spBitmapFlipRotator
    );

XSIZE CalculateScaledSize(
    _In_ const ImageMetadata& imageMetadata,
    _In_ const ImageDecodeParams& decodeParams,
    bool clampToMetadataSize = true
    );

_Check_return_ HRESULT RealizeBitmapSource(
    _In_ const ImageMetadata& imageMetadata,
    _In_ IWICBitmapSource* bitmapSource,
    _In_ const ImageDecodeParams& decodeParams,
    _Out_ xref_ptr<OfferableSoftwareBitmap>& spSoftwareBitmap
    );

_Check_return_ HRESULT ConvertWicPixelFormat(
    _In_ WICPixelFormatGUID wicPixelFormat,
    _Out_ PixelFormat* pixelFormat
    );

} // namespace ImagingUtility
