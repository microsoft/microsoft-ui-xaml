// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <memory>
#include <wincodec.h>
#include <d2d1_3.h>
#include <d2d1svg.h>
#include <windowrendertarget.h>
#include <windowsgraphicsdevicemanager.h>
#include <d3d11device.h>
#include "D3D11SharedDeviceGuard.h"
#include <GraphicsUtility.h>
#include "RawData.h"
#include "ImageMetadata.h"
#include "WicService.h"
#include "EncodedImageData.h"

bool EncodedImageData::s_testHook_ForceDeviceLostOnMetadataParse = false;

EncodedImageData::EncodedImageData(
    _In_ wistd::unique_ptr<IRawData> pRawData,
    uint32_t scalePercentage,
    bool isSvg
    )
    : m_pRawData(std::move(pRawData))
    , m_scalePercentage(scalePercentage)
    , m_isSvg(isSvg)
{
    ASSERT(m_pRawData != nullptr);
}

EncodedImageData::~EncodedImageData()
{
}

_Check_return_ HRESULT EncodedImageData::Parse(_In_opt_ CD3D11Device* graphicsDevice, wf::Size maxRootSize)
{
    // Don't need to parse it multiple times.
    if (m_isParsed)
    {
        return S_OK;
    }

    if (IsSvg())
    {
        if (graphicsDevice)
        {
            if (s_testHook_ForceDeviceLostOnMetadataParse)
            {
                if (m_enabled_testHook_ForceDeviceLostOnMetadataParse)
                {
                    IFC_RETURN(DXGI_ERROR_DEVICE_REMOVED);
                }
                else
                {
                    // Apply static test hook to this EncodedImageData instance next time we're in Parse().
                    // This is intentionally applied starting with the 2nd call of Parse() to
                    // simulate the conditions of
                    // Bug 32816679: [Watson Failure] caused by stack overflow from
                    // CSvgImageSource::OnDownloadImageAvailableImpl (INVALID_STACK_ACCESS_c0000005_Windows.UI.Xaml.dll!GetCallerReturnAddressFromDirectCaller)
                    //
                    // In that bug, a previous Parse() of SVG content completed successfully, but a subsequent device lost
                    // condition leads to an endlessly failing re-decode loop since the SVG re-decode requires the device which isn't
                    // available yet, and the bug caused us to access the device before checking for cached metadata.
                    m_enabled_testHook_ForceDeviceLostOnMetadataParse = true;
                }
            }
            else
            {
                // Immediately diasble test hook on EncodedImageData image when static test hook is off
                m_enabled_testHook_ForceDeviceLostOnMetadataParse = false;
            }

            XRECT maxBounds = { 0, 0, static_cast<int>(round(maxRootSize.Width)), static_cast<int>(round(maxRootSize.Height)) };

            IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(graphicsDevice->EnsureD2DResources());

            CD3D11SharedDeviceGuard guard;
            IFC_RETURN(graphicsDevice->TakeLockAndCheckDeviceLost(&guard));
            auto d2dDeviceContextNoRef = graphicsDevice->GetD2DDeviceContext(&guard);
            ctl::ComPtr<ID2D1DeviceContext5> d2dDeviceContext5;
            IFCFAILFAST(ctl::do_query_interface(d2dDeviceContext5, d2dDeviceContextNoRef));

            IFC_RETURN(ParseSvg(d2dDeviceContext5.Get(), maxBounds));
        }
    }
    else
    {
        IFC_RETURN(ParseWic());
    }

    return S_OK;
}

_Check_return_ HRESULT EncodedImageData::ParseWic()
{
   ASSERT(!m_isParsed);

    std::lock_guard<std::mutex> lock(m_mutex);
    SuspendFailFastOnStowedException suspender; // http://osgvsowi/6897785 it is possible to have invalid URI from toast payload

    IFC_RETURN(WicService::GetInstance().GetBitmapDecoder(*m_pRawData, m_spWicBitmapDecoder));
    IFC_RETURN(WicService::GetInstance().GetMetadata(m_spWicBitmapDecoder, m_imageMetadata));
    m_imageMetadata.scalePercentage = m_scalePercentage;

    m_isParsed = true;

    return S_OK;
}

// TODO: Potential future optimizations involve caching the document but care needs to be taken with device lost
//                 since the document could be lost.  This applies to calls like SvgImageSource::GetID2D1SvgDocument and
//                 SvgImageDecoder::DecodeFrame as well.
_Check_return_ HRESULT EncodedImageData::ParseSvg(
    _In_ ID2D1DeviceContext5* d2dDeviceContext5,
    XRECT maxBounds
    )
{
    ASSERT(!m_isParsed);

    // If the ViewBox attributes are not available in the svg document, use 1/1 as fallback.
    float maxWidth = static_cast<float>(maxBounds.Width);
    float maxHeight = static_cast<float>(maxBounds.Height);

    wrl::ComPtr<IStream> istream;
    IFC_RETURN(CreateIStream(istream));
    std::lock_guard<std::mutex> lock(m_mutex);

    // If maxWidth or maxHeight are equal to 0, we'll instead clamp them at a minimum of 1, since the SVG file
    // might contain an override of width or height, in which case those values won't be used.
    // If not, we'll return an error later on.
    wrl::ComPtr<ID2D1SvgDocument> svgDocument;
    IFC_RETURN(d2dDeviceContext5->CreateSvgDocument(
        istream.Get(),
        D2D1::SizeF(static_cast<FLOAT>(std::max(1.0f, maxWidth)), static_cast<FLOAT>(std::max(1.0f, maxHeight))), // viewportSize
        svgDocument.ReleaseAndGetAddressOf()));

    wrl::ComPtr<ID2D1SvgElement> rootElement;
    svgDocument->GetRoot(rootElement.ReleaseAndGetAddressOf());

    // Get the width and height of the document to assess the aspect ratio of the image if it is available.
    float documentWidth = 1.0f;
    bool widthSpecified = false;
    D2D1_SVG_LENGTH rootWidth = {};
    D2D1_SVG_LENGTH_UNITS widthUnits = {};
    if (rootElement->IsAttributeSpecified(L"width"))
    {
        if (SUCCEEDED(rootElement->GetAttributeValue(L"width", &rootWidth)))
        {
            widthUnits = rootWidth.units;
            widthSpecified = true;
        }
    }

    float documentHeight = 1.0f;
    bool heightSpecified = false;
    D2D1_SVG_LENGTH rootHeight = {};
    D2D1_SVG_LENGTH_UNITS heightUnits = {};
    if (rootElement->IsAttributeSpecified(L"height"))
    {
        if (SUCCEEDED(rootElement->GetAttributeValue(L"height", &rootHeight)))
        {
            heightUnits = rootHeight.units;
            heightSpecified = true;
        }
    }

    bool documentBoundsDetermined = false;

    if (widthSpecified && heightSpecified)
    {
        // Use the width and height properties to determine aspect ratio.
        ASSERT(widthUnits == heightUnits);
        documentWidth = rootWidth.value;
        documentHeight = rootHeight.value;
        documentBoundsDetermined = true;
    }
    else
    {
        // Use the SVG view box to determine aspect ratio.
        D2D1_SVG_VIEWBOX viewBox = {};

        if (rootElement->IsAttributeSpecified(L"viewBox"))
        {
            if (SUCCEEDED(rootElement->GetAttributeValue(L"viewBox", D2D1_SVG_ATTRIBUTE_POD_TYPE_VIEWBOX, &viewBox, sizeof(viewBox))))
            {
                documentWidth = viewBox.width;
                documentHeight = viewBox.height;
                documentBoundsDetermined = true;
            }
        }
    }

    // A max width or max height of 0 is OK only if the SVG file contains overrides for those values.
    // Otherwise, we'll try to rasterize the SVG to a size of 0x0, which is not allowed.
    if (documentBoundsDetermined)
    {
        if (maxWidth == 0.0f)
        {
            maxWidth = documentWidth;
        }

        if (maxHeight == 0.0f)
        {
            maxHeight = documentHeight;
        }
    }

    if (maxWidth == 0.0f || maxHeight == 0.0f)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    // If the width/height/viewBox were all unavailable, it will assume an aspect ratio of 1:1

    // Initialize the encoded size to be large enough to cover the entire screen, while keeping the aspect ratio.
    if ((maxWidth / documentWidth) > (maxHeight / documentHeight))
    {
        m_imageMetadata.height = static_cast<uint32_t>(maxWidth * (documentHeight / documentWidth));
        m_imageMetadata.width = static_cast<uint32_t>(maxWidth);
    }
    else
    {
        m_imageMetadata.height = static_cast<uint32_t>(maxHeight);
        m_imageMetadata.width = static_cast<uint32_t>(maxHeight * (documentWidth / documentHeight));
    }

    m_isParsed = true;
    return S_OK;
}

const ImageMetadata& EncodedImageData::GetMetadata() const
{
    ASSERT(m_isParsed, L"Cannot get metadata before the encoded data is parsed.");

    return m_imageMetadata;
}

_Check_return_ HRESULT EncodedImageData::CreateWicBitmapDecoder(
    _Outref_ wrl::ComPtr<IWICBitmapDecoder>& spWicBitmapDecoder
    )
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_spWicBitmapDecoder != nullptr)
    {
        // The wic bitmap decoder can only be used once.  It is initialized for metadata and re-used
        // for the first decode operation.  After that, it must be re-created for subsequent decodes otherwise
        // decoding with the same object to different sizes could will cause a crash in WIC.
        spWicBitmapDecoder = std::move(m_spWicBitmapDecoder);
        m_spWicBitmapDecoder.Reset();
    }
    else
    {
        IFC_RETURN(WicService::GetInstance().GetBitmapDecoder(*m_pRawData, spWicBitmapDecoder));
    }

    return S_OK;
}

bool EncodedImageData::IsAnimatedImage() const
{
    return GetMetadata().frameCount > 1;
}

_Check_return_ HRESULT EncodedImageData::CreateIStream(_Outref_ wrl::ComPtr<IStream>& spIStream)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Create an HGLOBAL large enough for the whole block and copy to it.
    auto globalHandle = GlobalAlloc(GMEM_MOVEABLE, m_pRawData->GetSize());
    IFCOOM_RETURN(globalHandle);

    auto globalLockPtr = GlobalLock(globalHandle);
    // We have to check for a NULL return from the GlobalLock because GlobalAlloc can give us
    // any old handle ptr for a GMEM_MOVEABLE request. We won't really know how valid the handle is
    // till we call lock. If the handle is garbage then we will get a NULL ptr back and can check there.
    IFCOOM_RETURN(globalLockPtr);

    memcpy(globalLockPtr, m_pRawData->GetData(), m_pRawData->GetSize());
    GlobalUnlock(globalHandle);

    // Now create a stream on it and set in the reader.
    IFC_RETURN(CreateStreamOnHGlobal(globalHandle, TRUE, spIStream.ReleaseAndGetAddressOf()));

    return S_OK;
}

std::uint64_t EncodedImageData::GetRawDataSize() const
{
    return m_pRawData->GetSize();
}