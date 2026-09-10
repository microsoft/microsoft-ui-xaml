// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "ImagingInterfaces.h"
#include <weakref_ptr.h>

class CCoreServices;
class CD3D11Device;
struct ID2D1Factory1;
struct ID2D1DeviceContext5;

class SvgImageDecoder final
    : public IImageDecoder
{
public:
    SvgImageDecoder(ctl::ComPtr<ID2D1Factory1> d2dFactory, _In_opt_ CD3D11Device* graphicsDevice = nullptr);

    _Check_return_ HRESULT DecodeFrame(
        _In_ EncodedImageData& encodedImageData,
        _In_ const ImageDecodeParams& decodeParams,
        int frameIndex,
        _Out_ wrl::ComPtr<IWICBitmapSource>& bitmapSource,
        _Out_ std::chrono::milliseconds& frameDelay
        ) final;

    // Test hook - simulates a device lost error when we're setting up a SVG decoder
    // Access to this is not synchronized. The test is expected to prime this flag before doing any decoding.
    static bool s_testHook_ForceDeviceLostOnCreatingSvgDecoder;

private:
    _Check_return_ HRESULT DrawSvg(
        _In_ EncodedImageData& encodedImageData,
        uint32_t width,
        uint32_t height,
        _In_ ID2D1DeviceContext5* d2dDeviceContext5);

    _Check_return_ HRESULT DecodeFrameWithDevice(
        _In_ CD3D11Device* graphicsDevice,
        _In_ EncodedImageData& encodedImageData,
        uint32_t width,
        uint32_t height,
        _Out_ wrl::ComPtr<IWICBitmapSource>& bitmapSource);

    ctl::ComPtr<ID2D1Factory1> m_d2dFactory;
    xref::weakref_ptr<CD3D11Device> m_graphicsDevice;
};
