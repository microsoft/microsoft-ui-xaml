// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ImageMetadata.h"
#include <mutex>

class CWindowRenderTarget;
class CD3D11Device;
class IRawData;
struct ID2D1DeviceContext5;
struct IWICBitmapDecoder;

// TODO: Ideally this is sub-classed so that SVG or WIC derived classes could provide
//                 information relevant to that type.

// This class stores a pointer to the raw image data and parses a bitmap decoder and metadata
// which can be reused and is used so that tasks are not repeated throughout the pipeline.
class EncodedImageData
{
public:
    // Initialize will cause the EncodedImageData to take sole ownership of the raw data.
    //
    // SVG is tied to the type of encoded image data it is and it is information that is
    // known ahead of time when parsing due to extension check or explicit creation of
    // an SvgImageSource.  Knowing the type ahead of time can help optimize parsing.
    EncodedImageData(
        _In_ wistd::unique_ptr<IRawData> pRawData,
        uint32_t scalePercentage = 0,
        bool isSvg = false);
    ~EncodedImageData();

    bool IsSvg() const { return m_isSvg; }

    _Check_return_ HRESULT Parse(_In_opt_ CD3D11Device* graphicsDevice, wf::Size maxRootSize);

    const ImageMetadata& GetMetadata() const;

    _Check_return_ HRESULT CreateWicBitmapDecoder(
        _Outref_ wrl::ComPtr<IWICBitmapDecoder>& spWicBitmapDecoder
        );
    bool IsAnimatedImage() const;

    bool IsMetadataAvailable() const { return m_isParsed; }

    _Check_return_ HRESULT CreateIStream(_Outref_ wrl::ComPtr<IStream>& spIStream);

    std::uint64_t GetRawDataSize() const;

    // Test hook - simulates a device lost error when we're doing an off-thread decode
    //             by returning DXGI_ERROR_DEVICE_REMOVED from EncodedImageData::Parse() from 2nd and later calls / attempts.
    //
    // Access to this is not synchronized. The test is expected to prime this flag before doing any off-thread decoding.
    static bool s_testHook_ForceDeviceLostOnMetadataParse;

    // Helper for s_testHook_ForceDeviceLostOnNextDecode to skip initial attempt to parse file.
    bool m_enabled_testHook_ForceDeviceLostOnMetadataParse {false};

private:

    // Call to explicitly process the raw data passed on.  If the raw data is invalid for
    // decoding, this is the first location it can fail.
    _Check_return_ HRESULT ParseSvg(
        _In_ ID2D1DeviceContext5* d2dDeviceContext5,
        XRECT maxBounds);
    _Check_return_ HRESULT ParseWic();

    wistd::unique_ptr<IRawData> m_pRawData;
    uint32_t m_scalePercentage;
    
    bool m_isSvg = false;
    bool m_isParsed = false;
    ImageMetadata m_imageMetadata = {};
    wrl::ComPtr<IWICBitmapDecoder> m_spWicBitmapDecoder;

    // Must guard this with a mutex because multiple decoding thread could use this encoded image
    // to decode multiple sizes.
    std::mutex m_mutex;
};
