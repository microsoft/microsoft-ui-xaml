// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <chrono>

class ImageCache;
class EncodedImageData;
class ImageDecodeParams;
class OfferableSoftwareBitmap;
struct IImageAvailableCallback;
struct ImageMetadata;

interface IWICBitmapSource;

struct IImageDecoder
{
    virtual _Check_return_ HRESULT DecodeFrame(
        _In_ EncodedImageData& encodedImageData,
        _In_ const ImageDecodeParams& decodeParams,
        int frameIndex,
        _Outref_ wrl::ComPtr<IWICBitmapSource>& bitmapSource,
        _Outref_ std::chrono::milliseconds& frameDelay
        ) = 0;
    virtual ~IImageDecoder() = 0 {}
};

struct IAbortableImageOperation : IObject
{
    virtual void DisconnectImageOperation() = 0;
    virtual void CleanupDeviceRelatedResources() = 0;

    virtual _Check_return_ HRESULT PlayAnimation() = 0;
    virtual _Check_return_ HRESULT StopAnimation() = 0;
    virtual void SuspendAnimation() = 0;
    virtual _Check_return_ HRESULT ResumeAnimation() = 0;

    virtual bool IsDecodeInProgress() = 0;

    virtual _Check_return_ HRESULT SetDecodeParams(
        _In_ xref_ptr<IImageAvailableCallback> imageAvailableCallback,
        _In_ xref_ptr<ImageDecodeParams> spDecodeParams) = 0;

    // Returns the size of the decoded image surface as it would be according to the current decode params.
    virtual XSIZE GetDecodedSize() const = 0;

    virtual bool HasDecoder() const = 0;
};

