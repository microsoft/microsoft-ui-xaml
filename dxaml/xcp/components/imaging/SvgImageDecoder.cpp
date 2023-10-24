// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <memory>
#include <MUX-ETWEvents.h>
#include <wincodec.h>
#include <ImageProviderInterfaces.h>
#include <PixelFormat.h>
#include <palgfx.h>
#include <DoubleUtil.h>
#include <ImageMetadata.h>
#include <SurfaceDecodeParams.h>
#include <ImageDecodeParams.h>
#include <EncodedImageData.h>
#include <OfferableSoftwareBitmap.h>
#include <ThreadPoolService.h>
#include <ImagingUtility.h>
#include <SvgImageDecoder.h>
#include <Corep.h>
#include <d2d1_3.h>
#include <WicService.h>

using namespace DirectUI;

bool SvgImageDecoder::s_testHook_ForceDeviceLostOnCreatingSvgDecoder = false;

SvgImageDecoder::SvgImageDecoder(
    ctl::ComPtr<ID2D1Factory1> d2dFactory
    )
    : m_d2dFactory(std::move(d2dFactory))
{
}

_Check_return_ HRESULT SvgImageDecoder::DecodeFrame(
    _In_ EncodedImageData& encodedImageData,
    _In_ const ImageDecodeParams& decodeParams,
    int frameIndex,
    _Outref_ wrl::ComPtr<IWICBitmapSource>& bitmapSource,
    _Outref_ std::chrono::milliseconds& frameDelay
    )
{
    bitmapSource.Reset();
    frameDelay = std::chrono::milliseconds(0);

    TraceDecodeToSurfaceBegin();
    auto traceGuard = wil::scope_exit([] { TraceDecodeToSurfaceEnd(); });

    auto imageMetadata = encodedImageData.GetMetadata();

    uint32_t width = imageMetadata.width;
    uint32_t height = imageMetadata.height;
    if (decodeParams.GetDecodeWidth() != 0 || decodeParams.GetDecodeHeight() != 0)
    {
        auto scaledSize = ImagingUtility::CalculateScaledSize(imageMetadata, decodeParams, false /* clampToMetadataSize */);
        width = scaledSize.Width;
        height = scaledSize.Height;
    }

    ASSERT(width > 0 && height > 0);

    auto spWicFactory = WicService::GetInstance().GetFactory();
    WICPixelFormatGUID pixelFormat = GUID_WICPixelFormat32bppPBGRA;
    wrl::ComPtr<IWICBitmap> wicBitmap;
    IFC_RETURN(spWicFactory->CreateBitmap(
        width,
        height,
        pixelFormat,
        WICBitmapCreateCacheOption::WICBitmapCacheOnDemand,
        wicBitmap.ReleaseAndGetAddressOf()));

    auto renderTargetProperties = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_SOFTWARE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM),
        96.0f,
        96.0f,
        D2D1_RENDER_TARGET_USAGE_NONE,
        D2D1_FEATURE_LEVEL_DEFAULT);

    wrl::ComPtr<ID2D1RenderTarget> renderTarget;
    IFC_RETURN(m_d2dFactory->CreateWicBitmapRenderTarget(wicBitmap.Get(), &renderTargetProperties, renderTarget.ReleaseAndGetAddressOf()));

    wrl::ComPtr<ID2D1DeviceContext5> d2dDeviceContext5;
    IFC_RETURN(renderTarget.As(&d2dDeviceContext5));

    wrl::ComPtr<IStream> istream;
    IFC_RETURN(encodedImageData.CreateIStream(istream));

    wrl::ComPtr<ID2D1SvgDocument> svgDocument;
    IFC_RETURN(d2dDeviceContext5->CreateSvgDocument(
        istream.Get(),
        D2D1::SizeF(static_cast<FLOAT>(width), static_cast<FLOAT>(height)), // viewportSize
        svgDocument.ReleaseAndGetAddressOf()
        ));

    d2dDeviceContext5->BeginDraw();
    d2dDeviceContext5->Clear();
    d2dDeviceContext5->DrawSvgDocument(svgDocument.Get());
    IFC_RETURN(d2dDeviceContext5->EndDraw());

    bitmapSource = std::move(wicBitmap);
    return S_OK;
}
