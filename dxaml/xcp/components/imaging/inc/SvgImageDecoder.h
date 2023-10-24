// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "ImagingInterfaces.h"

class CCoreServices;
struct ID2D1Factory1;

class SvgImageDecoder final
    : public IImageDecoder
{
public:
    SvgImageDecoder(ctl::ComPtr<ID2D1Factory1> d2dFactory);

    _Check_return_ HRESULT DecodeFrame(
        _In_ EncodedImageData& encodedImageData,
        _In_ const ImageDecodeParams& decodeParams,
        int frameIndex,
        _Outref_ wrl::ComPtr<IWICBitmapSource>& bitmapSource,
        _Outref_ std::chrono::milliseconds& frameDelay
        ) final;

    // Test hook - simulates a device lost error when we're setting up a SVG decoder
    // Access to this is not synchronized. The test is expected to prime this flag before doing any decoding.
    static bool s_testHook_ForceDeviceLostOnCreatingSvgDecoder;

private:
    ctl::ComPtr<ID2D1Factory1> m_d2dFactory;
};
