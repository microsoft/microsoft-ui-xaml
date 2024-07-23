// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "ImagingInterfaces.h"

class WicSingleImageDecoder
    : public IImageDecoder
{
public:
    // IImageDecoder
    _Check_return_ HRESULT DecodeFrame(
        _In_ EncodedImageData& encodedImageData,
        _In_ const ImageDecodeParams& decodeParams,
        int frameIndex,
        _Out_ wrl::ComPtr<IWICBitmapSource>& bitmapSource,
        _Out_ std::chrono::milliseconds& frameDelay
        ) override;
};
