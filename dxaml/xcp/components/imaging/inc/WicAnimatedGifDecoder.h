// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "ImagingInterfaces.h"

struct WICRect;
struct IWICBitmap;
struct IWICBitmapDecoder;
struct IWICMetadataQueryReader;
struct IWICBitmapSource;
struct ImageMetadata;
struct IImageDecodeCallback;
class OfferableSoftwareBitmap;
class ImageDecodeParams;

class WicAnimatedGifDecoder
    : public IImageDecoder
{
public:

    enum class DisposalMethod
    {
        Undefined = 0,
        None = 1,
        Background = 2,
        Previous = 3,
    };

    struct DeltaFrameInfo
    {
        bool supportsAlpha;
        WICRect bounds;
        std::chrono::milliseconds delay;
        DisposalMethod disposalMethod;
    };

    // IImageDecoder
    _Check_return_ HRESULT DecodeFrame(
        _In_ EncodedImageData& encodedImageData,
        _In_ const ImageDecodeParams& decodeParams,
        int frameIndex,
        _Outref_ wrl::ComPtr<IWICBitmapSource>& bitmapSource,
        _Outref_ std::chrono::milliseconds& frameDelay
        ) override;

    static _Check_return_ HRESULT CreateDeltaFrameSource(
        _In_ const wrl::ComPtr<IWICBitmapDecoder>& spBitmapDecoder,
        int frameIndex,
        _Outref_ DeltaFrameInfo& gifDeltaFrameInfo,
        _Outref_ wrl::ComPtr<IWICBitmapSource>& spBitmapSource
        );

private:
    static _Check_return_ HRESULT GetDeltaFrameInfo(
        _In_ const wrl::ComPtr<IWICBitmapFrameDecode>& spBitmapFrameDecode,
        _Outref_ DeltaFrameInfo& deltaFrameInfo
        );

    DeltaFrameInfo m_currentDeltaFrameInfo = {};
    int m_currentFrameIndex = -1; // no frame has been decoded yet

    // TODO: It is possible to make these offerable bitmap's and save
    //                 some memory.  However, that would mean a good catch-up
    //                 solution should be available to recover from a state
    //                 of not having saved or previous bitmap available.
    wrl::ComPtr<IWICBitmap> m_spSavedBitmap;
    wrl::ComPtr<IWICBitmap> m_spCurrentBitmap;

    static _Check_return_ HRESULT CreateWicBitmap(
        const wrl::ComPtr<IWICImagingFactory> spFactory,
        const ImageMetadata &imageMetadata,
        _Outptr_result_nullonfailure_ IWICBitmap **result);
};
