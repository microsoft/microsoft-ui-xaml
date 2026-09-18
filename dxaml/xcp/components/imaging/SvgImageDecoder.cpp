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
#include <d3d11device.h>
#include <D3D11SharedDeviceGuard.h>

using namespace DirectUI;

bool SvgImageDecoder::s_testHook_ForceDeviceLostOnCreatingSvgDecoder = false;

SvgImageDecoder::SvgImageDecoder(
    ctl::ComPtr<ID2D1Factory1> d2dFactory,
    _In_opt_ CD3D11Device* graphicsDevice
    )
    : m_d2dFactory(std::move(d2dFactory))
    , m_graphicsDevice(xref::get_weakref(graphicsDevice))
{
}

_Check_return_ HRESULT SvgImageDecoder::DecodeFrame(
    _In_ EncodedImageData& encodedImageData,
    _In_ const ImageDecodeParams& decodeParams,
    int frameIndex,
    _Out_ wrl::ComPtr<IWICBitmapSource>& bitmapSource,
    _Out_ std::chrono::milliseconds& frameDelay
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

    if (auto graphicsDevice = m_graphicsDevice.lock())
    {
        if (SUCCEEDED(DecodeFrameWithDevice(graphicsDevice.get(), encodedImageData, width, height, bitmapSource)))
        {
            return S_OK;
        }
    }

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

    IFC_RETURN(DrawSvg(encodedImageData, width, height, d2dDeviceContext5.Get()));
    bitmapSource = std::move(wicBitmap);
    return S_OK;
}

_Check_return_ HRESULT SvgImageDecoder::DrawSvg(
    _In_ EncodedImageData& encodedImageData,
    uint32_t width,
    uint32_t height,
    _In_ ID2D1DeviceContext5* d2dDeviceContext5)
{
    wrl::ComPtr<IStream> istream;
    IFC_RETURN(encodedImageData.CreateIStream(istream));

    wrl::ComPtr<ID2D1SvgDocument> svgDocument;
    IFC_RETURN(d2dDeviceContext5->CreateSvgDocument(
        istream.Get(),
        D2D1::SizeF(static_cast<FLOAT>(width), static_cast<FLOAT>(height)), // viewportSize
        svgDocument.ReleaseAndGetAddressOf()
        ));

    //
    // The SVG itself may have a natural size, defined by its width/height/viewBox properties, and we want to stretch it
    // up or down to a different size to match the Xaml container that will be displaying the SVG. One way to do this is
    // to decode the SVG to a surface matching its natural size, then scale that surface to match Xaml. But since we're
    // working with _Scalable_ Vector Graphics, there's a better way to do this scale.
    //
    // We wrap the SVG in a container SVG whose width/height matches the size that Xaml needs. Then the scaling can be
    // done inside the SVG by D2D, and we end up with a crisp image.
    //
    // We do this wrapping rather than override the natural size of the SVG (i.e. set its width/height) because the SVG
    // itself may be scaling its contents with preserveAspectRatio (which specifies both alignment and whether it
    // stretches up to fill the smaller or larger dimension), and we don't want to mess with that. We'll let the SVG
    // scale itself in the way that it was defined, then scale the result according to what Xaml needs.
    //
    // For example,
    //
    //      <svg width="50" height="50" viewBox="0 0 25 75" preserveAspectRatio="xMidYMid slice">
    //          <rect width="25" height="25" y="0" fill="red" />
    //          <rect width="25" height="25" y="25" fill="yellow" />
    //          <rect width="25" height="25" y="50" fill="green" />
    //      </svg>
    //      <!-- ...and Xaml wants to decode this SVG into a 100x300 surface -->
    //
    //      This SVG shows just the yellow square in the middle due to the preserveAspectRatio="slice". If Xaml were
    //      scaling this image, we should scale just the yellow square. But if we override the width/height on the
    //      SVG itself to 100x300, we'd see all three squares show up.
    //
    // We do this wrapping even if the SVG didn't define both the width and height. In these cases the SVG will scale
    // itself up to maintain its aspect ratio and show the complete image, but it may have a preserveAspectRatio
    // property that causes alignment or clipping issues if we impose explicit dimensions on it. We wrap the SVG to
    // reset the preserveAspectRatio when imposing Xaml's decode size on the image.
    //
    // For example,
    //
    //      <svg viewBox="0 0 25 75" preserveAspectRatio="xMidYMid slice">
    //          <rect width="25" height="25" y="0" fill="red" />
    //          <rect width="25" height="25" y="25" fill="yellow" />
    //          <rect width="25" height="25" y="50" fill="green" />
    //      </svg>
    //      <!-- ...and Xaml wants to decode this SVG into a 100x100 surface -->
    //
    //      This SVG shows all three squares because no explicit size was defined. If Xaml were scaling this image, we
    //      should scale all three squares. But if we set the width/height on the SVG itself to 100x100, we'd see only
    //      the yellow square due to the preserveAspectRatio="slice".
    //
    // We _skip_ this wrapping when the SVG itself defines its width/height in terms of percentages. In these cases the
    // SVG has no natural size and is defined to occupy some percentage of the SVG viewport, so it should already be
    // capable of sizing itself to the dimensions that Xaml requests.
    //
    // For example,
    //
    //      <svg height="50%" viewBox="0 0 25 75" preserveAspectRatio="xMidYMid slice">
    //          <rect width="25" height="25" y="0" fill="red" />
    //          <rect width="25" height="25" y="25" fill="yellow" />
    //          <rect width="25" height="25" y="50" fill="green" />
    //      </svg>
    //      <!-- ...and Xaml wants to decode this SVG into a 300x300 surface -->
    //
    //      Putting this in a HTML div sized to 300x300 looks different from wrapping it in a
    //      <svg width="300" height="300" viewBox="0 0 300 300">. It's not clear how we should wrap this to make it
    //      look correct, so we just let the SVG handle the scaling. Having top-level SVGs that specifically want to
    //      occupy only part of their container is also expected to be a rare scenario.
    //
    // In order to do this wrapping, we need to set a viewBox property on the container SVG. We use the explicit natural
    // size of the inner SVG, if it's available. If only one dimension is specified, we infer the other based on the
    // viewBox of the inner SVG. If neither dimension is specified, we just use the inner SVG's viewBox. Note that the
    // inner SVG must define a viewBox if it wants to scale its contents.
    //

    wrl::ComPtr<ID2D1SvgElement> rootElement;
    svgDocument->GetRoot(rootElement.ReleaseAndGetAddressOf());

    D2D1_SVG_LENGTH rootWidth = {};
    const bool hasWidth = SUCCEEDED(rootElement->GetAttributeValue(L"width", &rootWidth));
    const bool hasWidthInPercentages = hasWidth && (rootWidth.units == D2D1_SVG_LENGTH_UNITS_PERCENTAGE);
    D2D1_SVG_LENGTH rootHeight = {};
    const bool hasHeight = SUCCEEDED(rootElement->GetAttributeValue(L"height", &rootHeight));
    const bool hasHeightInPercentages = hasHeight && (rootHeight.units == D2D1_SVG_LENGTH_UNITS_PERCENTAGE);

    wrl::ComPtr<ID2D1SvgDocument> svgContainerDocument;
    if (hasWidthInPercentages || hasHeightInPercentages)
    {
        // If the SVG has specified a size in percentages, then don't bother wrapping it in a container. Let it stretch
        // to Xaml's requested dimensions.
        svgContainerDocument = svgDocument;
    }
    else
    {
        // Build a container SVG and put the original SVG inside
        IFC_RETURN(d2dDeviceContext5->CreateSvgDocument(
            nullptr,
            D2D1::SizeF(static_cast<FLOAT>(width), static_cast<FLOAT>(height)), // default viewportSize
            svgContainerDocument.ReleaseAndGetAddressOf()));

        wrl::ComPtr<ID2D1SvgElement> containerElement;
        svgContainerDocument->GetRoot(containerElement.ReleaseAndGetAddressOf());
        IFC_RETURN(containerElement->AppendChild(rootElement.Get()));

        // Figure out the viewBox for the container SVG...
        D2D1_SVG_VIEWBOX containerViewBox = {};

        if (hasWidth && hasHeight)
        {
            // ...If the original SVG has an explicit size, use that as our container viewBox.

            containerViewBox.width = rootWidth.value;
            containerViewBox.height = rootHeight.value;
            IFC_RETURN(containerElement->SetAttributeValue(L"viewBox", D2D1_SVG_ATTRIBUTE_POD_TYPE_VIEWBOX, &containerViewBox, sizeof(containerViewBox)));
        }
        else
        {
            // ...The original SVG has no explicit size. It'll maintain its aspect ratio. Figure out what the final size is.

            D2D1_SVG_VIEWBOX originalViewBox = {};
            const bool svgHasViewBox = SUCCEEDED(rootElement->GetAttributeValue(L"viewBox", D2D1_SVG_ATTRIBUTE_POD_TYPE_VIEWBOX, &originalViewBox, sizeof(containerViewBox)));

            // If the original SVG has no viewBox (or has a degenerate viewBox), then it doesn't scale. In that case we
            // don't need to set a viewBox on the container either.
            if (svgHasViewBox && originalViewBox.width > 0 && originalViewBox.height > 0)
            {
                if (!hasWidth && !hasHeight)
                {
                    // If neither width nor height was specified, then just match the viewBoxes...
                    containerViewBox = originalViewBox;

                    // ...but reset the origin of the view box for the outer SVG. The inner SVG will be positioned at
                    // the origin when it's stretched (1:1) in the outer SVG.
                    containerViewBox.x = 0;
                    containerViewBox.y = 0;
                }
                else
                {
                    // If only one dimension was specified, calculate the missing dimension and use those dimensions as the container viewBox.
                    if (hasWidth)
                    {
                        containerViewBox.width = rootWidth.value;
                        containerViewBox.height = rootWidth.value * originalViewBox.height / originalViewBox.width;
                    }
                    else
                    {
                        containerViewBox.width = rootHeight.value * originalViewBox.width / originalViewBox.height;
                        containerViewBox.height = rootHeight.value;
                    }
                }

                IFC_RETURN(containerElement->SetAttributeValue(L"viewBox", D2D1_SVG_ATTRIBUTE_POD_TYPE_VIEWBOX, &containerViewBox, sizeof(containerViewBox)));
            }
        }
    }

    d2dDeviceContext5->BeginDraw();
    d2dDeviceContext5->Clear();
    d2dDeviceContext5->DrawSvgDocument(svgContainerDocument.Get());
    IFC_RETURN(d2dDeviceContext5->EndDraw());

    return S_OK;
}

_Check_return_ HRESULT SvgImageDecoder::DecodeFrameWithDevice(
    _In_ CD3D11Device* graphicsDevice,
    _In_ EncodedImageData& encodedImageData,
    uint32_t width,
    uint32_t height,
    _Out_ wrl::ComPtr<IWICBitmapSource>& bitmapSource)
{
    IFC_RETURN(graphicsDevice->EnsureD2DResources());
    wrl::ComPtr<ID2D1DeviceContext> context;
    {
        CD3D11SharedDeviceGuard guard;
        IFC_RETURN(graphicsDevice->TakeLockAndCheckDeviceLost(&guard));
        // Keep WARP on the WIC path to avoid the extra readback surfaces.
        if (graphicsDevice->IsWarpDevice())
        {
            return DXGI_ERROR_UNSUPPORTED;
        }
        IFC_RETURN(graphicsDevice->GetD2DDevice(&guard)->CreateDeviceContext(
            D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS, &context));
    }

    // Like DrawDummyText, use a private context without holding the shared lock over drawing/readback.
    wrl::ComPtr<ID2D1DeviceContext5> svgContext;
    IFC_RETURN(context.As(&svgContext));
    const auto format = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);
    const auto targetProperties = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET, format, 96.0f, 96.0f);
    wrl::ComPtr<ID2D1Bitmap1> target;
    IFC_RETURN(svgContext->CreateBitmap(D2D1::SizeU(width, height), nullptr, 0, &targetProperties, &target));
    svgContext->SetTarget(target.Get());
    IFC_RETURN(DrawSvg(encodedImageData, width, height, svgContext.Get()));
    svgContext->SetTarget(nullptr);

    const auto readbackProperties = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_CPU_READ | D2D1_BITMAP_OPTIONS_CANNOT_DRAW, format, 96.0f, 96.0f);
    wrl::ComPtr<ID2D1Bitmap1> readback;
    IFC_RETURN(svgContext->CreateBitmap(D2D1::SizeU(width, height), nullptr, 0, &readbackProperties, &readback));
    IFC_RETURN(readback->CopyFromBitmap(nullptr, target.Get(), nullptr));

    D2D1_MAPPED_RECT mapped = {};
    IFC_RETURN(readback->Map(D2D1_MAP_OPTIONS_READ, &mapped));
    auto unmap = wil::scope_exit([&] { IGNOREHR(readback->Unmap()); });
    uint32_t bufferSize = 0;
    IFC_RETURN(UInt32Mult(mapped.pitch, height, &bufferSize));
    wrl::ComPtr<IWICBitmap> bitmap;
    IFC_RETURN(WicService::GetInstance().GetFactory()->CreateBitmapFromMemory(
        width, height, GUID_WICPixelFormat32bppPBGRA, mapped.pitch, bufferSize, mapped.bits, &bitmap));
    bitmapSource = std::move(bitmap);
    return S_OK;
}
