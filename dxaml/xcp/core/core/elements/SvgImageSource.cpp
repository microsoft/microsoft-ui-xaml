// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SvgImageSource.h"
#include <limits>
#include <EncodedImageData.h>
#include <ImageCache.h>
#include <ImageCacheIdentifier.h>
#include <ImageMetadataView.h>
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include <ImageAvailableCallback.h>
#include <GraphicsUtility.h>
#include <ImageDecodeBoundsFinder.h>
#include <ImagingInterfaces.h>
#include <RawData.h>
#include <DOPointerCast.h>
#include <UIThreadScheduler.h>
#include <windowrendertarget.h>
#include <WindowsGraphicsDeviceManager.h>
#include "D3D11SharedDeviceGuard.h"
#include <d3d11device.h>
#include <WinTextCore.h>
#include <xref_ptr.h>

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

CSvgImageSource::CSvgImageSource(
    _In_ CCoreServices* pCore
    )
    : CImageSource(pCore)
    , m_rasterizeWidth(std::numeric_limits<float>::infinity())
    , m_rasterizeHeight(std::numeric_limits<float>::infinity())
{
}

_Check_return_ HRESULT
CSvgImageSource::Create(
    _Outptr_ CDependencyObject** retObject,
    _In_ CREATEPARAMETERS* createParameters
    )
{
    xref_ptr<CSvgImageSource> svgImageSource;
    svgImageSource.attach(new CSvgImageSource(createParameters->m_pCore));

    IFC_RETURN(svgImageSource->SetupImageSource(false /*mustKeepSoftwareSurface*/, createParameters));

    *retObject = svgImageSource.detach();

    return S_OK;
}

// TODO: This is very similar code to what is in CBitmapImage::OnDownloadImageAvailableImpl.  Refactor this
//                 to remove code duplication.
_Check_return_ HRESULT
CSvgImageSource::OnDownloadImageAvailableImpl(
    _In_ IImageAvailableResponse* response
    )
{
    TraceImageDownloadAvailableBegin(reinterpret_cast<XUINT64>(this), m_strSource.GetBuffer());
    auto traceEtwOnExit = wil::scope_exit([&]
    {
        TraceImageDownloadAvailableEnd(reinterpret_cast<XUINT64>(this), m_strSource.GetBuffer());
    });

    // Reset the hardware surface for the DecodeToRenderSize case since it may have been decoded to a new size
    if (m_fDecodeToRenderSize &&
        (response->GetSurface() != nullptr))
    {
        // Preserve the existing "must keep system memory" flag on the image surface wrapper.
        // This is required in a scenario where this CBitmapImage is being used as the source
        // of multiple brushes, one using hardware and the other using software.  If we clear
        // the "must keep system memory" flag, the surface will get released as soon as we
        // update the hardware resources for the hardware path, making the software surface
        // unavailable for the software path, causing it to not render anything.
        // The image surface wrapper already has the proper flag set depending on how it's being
        // used, so we simply preserve that flag as we're resetting the surfaces.
        // Additionally, pSurface is available (software surface) which means that hardware surfaces
        // must be reset in order to re-allocate the hardware surfaces to match the software surface size.
        ResetSurfaces(!!m_pImageSurfaceWrapper->MustKeepSystemMemory(), false);
    }

    IFC_RETURN(__super::OnDownloadImageAvailableImpl(response));

    if (FAILED(response->GetDecodingResult()))
    {
        DisconnectImageOperation();
    }
    else
    {
        GetContext()->StartTrackingAnimatedImage(this);
    }

    if (m_pendingDecodeForLostSoftwareSurface)
    {
        // Image data was recovered for lost software surface, put the image back in the decoded state.
        // It is possible for any decode to be aborted if an API call to set a new source interrupts the decode.
        if (m_pImageSurfaceWrapper->HasAnySurface() && IsMetadataAvailable())
        {
            SetBitmapState(BitmapImageState::Decoded);
        }
    }
    else
    {
        auto hrImageResult = response->GetDecodingResult();

        if (SUCCEEDED(hrImageResult) && (m_frameIndex == 0))
        {
            // Retain the natural size of the image
            // This was introduced to preserve the natural size for hit-testing when applications use an
            // ImageBrush for a button.  This originated from a bug pre-RS1 that apps took a dependency on
            // whereby the image natural size was used for layout and hit-testing even if the source was changed.
            // It would use the size of the previous source until the new image is done decoding.
            // Refer to TFS 7302806 for info on the bug and an attached min-repro app.
            m_retainedNaturalWidth = m_imageMetadataView->GetImageMetadata()->width;
            m_retainedNaturalHeight = m_imageMetadataView->GetImageMetadata()->height;

            if (m_fDecodeToRenderSize &&
                !m_pImageSurfaceWrapper->HasAnySurface())
            {
                // Check to see if the decode size has changed since the source was set and now.  If it has
                // then, decode should be issued.  This was done because the ability to set the DecodePixelWidth
                // and DecodePixelHeight at any time was added with the DecodeToRenderSize feature (including mid-download).
                m_fDecodeToRenderSize = ShouldDecodeToRenderSize();
                if (!m_fDecodeToRenderSize)
                {
                    SetBitmapState(BitmapImageState::Decoding);
                    IFC_RETURN(RedecodeEncodedImage(FALSE));
                }
                else
                {
                    SetBitmapState(BitmapImageState::DecodePending);
                }
            }
            else
            {
                // This will get called from OnStreamImageAvailable after any decoding is complete to
                // set the decoded state and fire the image opened event if the decoded surface is available.
                IFC_RETURN(FireImageOpened());
            }
        }
        else if (GraphicsUtility::IsDeviceLostError(hrImageResult))
        {
            // D2D device was lost during decode, re-queue the decode job.
            IFCFAILFAST(RedecodeEncodedImage(FALSE /* lostSurface */));
        }
        else if (FAILED(hrImageResult) && (hrImageResult != E_ABORT))
        {
            HRESULT errorToRaise = AG_E_NETWORK_ERROR;

            if (hrImageResult == E_INVALIDARG)
            {
                errorToRaise = hrImageResult;
            }
            //
            // If the source Uri had invalid characters, report specific error, as opposed to generic network error.
            //
            if (hrImageResult == E_INVALID_CHARS_IN_URI)
            {
                errorToRaise = AG_E_INVALID_SOURCE_URI;
            }

            IFC_RETURN(FireImageFailed(errorToRaise));
        }

        if (m_pImageSurfaceWrapper->HasAnySurface())
        {
            m_frameIndex++;
        }
    }

    return S_OK;
}

// This function is for printing, it is called from CImageBrush::GetPrintBrush
_Check_return_ HRESULT CSvgImageSource::GetID2D1SvgDocument(
    _In_ ID2D1DeviceContext5* d2dDeviceContextNoRef, //Note the ID2D1SvgDocument object needs to be created by the same D2D factory, that's why this ID2D1DeviceContext5 is passed in as a parameter.
    _Out_ wrl::ComPtr<ID2D1SvgDocument>& d2dSvgDocumentOut,
    _Out_ uint32_t* width,
    _Out_ uint32_t* height
    )
{
    d2dSvgDocumentOut.Reset();

    // Various code paths such as printing call to get the document.  It should first ensure that there is encoded image data available to parse.
    if (IsMetadataAvailable())
    {
        wrl::ComPtr<IStream> istream;
        IFC_RETURN(m_imageCache->GetEncodedImageData()->CreateIStream(istream));
        ASSERT(istream);

        auto metadataWidth = m_imageMetadataView->GetImageMetadata()->width;
        auto metadataHeight = m_imageMetadataView->GetImageMetadata()->height;

        wrl::ComPtr<ID2D1SvgDocument> d2dSvgDocument;
        IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(d2dDeviceContextNoRef->CreateSvgDocument(
            istream.Get(),
            D2D1::SizeF(static_cast<FLOAT>(metadataWidth), static_cast<FLOAT>(metadataHeight)), // viewportSize
            d2dSvgDocument.ReleaseAndGetAddressOf()
            ));

        if (width != nullptr)
        {
            *width = metadataWidth;
        }

        if (height != nullptr)
        {
            *height = metadataHeight;
        }

        d2dSvgDocumentOut = std::move(d2dSvgDocument);
    }

    return S_OK;
}

_Check_return_ HRESULT
CSvgImageSource::GetSvgDocumentString(
    _In_z_ const wchar_t* elementId,
    _Outptr_ HSTRING* output
    )
{
    *output = nullptr;

    // Check that there is a valid encoded image data before ensuring the D2D device context.
    // Getting the svg document will also check, but this can save allocating D2D resources if they are not necessary.
    if (!IsMetadataAvailable())
    {
        // If there is no content there is not output string.
        return S_OK;
    }

    // It's possible that Xaml just released a lost device but hasn't created a new one since the window is still
    // invisible. In that case there won't be a device, so default to returning a null string.
    auto deviceNoRef = GetContext()->NWGetWindowRenderTarget()->GetGraphicsDeviceManager()->GetGraphicsDevice();
    if (deviceNoRef)
    {
        IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(deviceNoRef->EnsureD2DResources());

        CD3D11SharedDeviceGuard guard;
        IFC_RETURN(deviceNoRef->TakeLockAndCheckDeviceLost(&guard));
        auto d2dDeviceContextNoRef = deviceNoRef->GetD2DDeviceContext(&guard);
        ctl::ComPtr<ID2D1DeviceContext5> d2dDeviceContext5;
        IFCFAILFAST(ctl::do_query_interface(d2dDeviceContext5, d2dDeviceContextNoRef));

        wrl::ComPtr<ID2D1SvgDocument> d2dSvgDocument;
        IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(GetID2D1SvgDocument(d2dDeviceContext5.Get(), d2dSvgDocument, nullptr, nullptr));

        // The document should not be null as long as a valid source was set.  An error would return immediately.
        ASSERT(d2dSvgDocument != nullptr);

        // Check for S_OK because S_FALSE indicates it was not found.
        // Other errors indicate failure that should result in no accessibility text (DOM parsing error from invalid SVG file).
        ctl::ComPtr<ID2D1SvgElement> foundElement;
        if (FindSvgElement(d2dSvgDocument.Get(), elementId, foundElement) == S_OK)
        {
            // The text content is the first child of the found element according to the D2D team.
            ctl::ComPtr<ID2D1SvgElement> textElement;
            foundElement->GetFirstChild(textElement.ReleaseAndGetAddressOf());
            if ((textElement != nullptr) &&
                textElement->IsTextContent())
            {
                std::vector<wchar_t> textString(textElement->GetTextValueLength() + 1);

                if (SUCCEEDED(textElement->GetTextValue(&textString[0], static_cast<UINT32>(textString.size()))))
                {
                    IFCFAILFAST(wrl_wrappers::HStringReference(&textString[0]).CopyTo(output));
                }
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CSvgImageSource::FindSvgElement(
    _In_ ID2D1SvgDocument* svgDocument,
    _In_z_ const wchar_t* elementId,
    _Out_ ctl::ComPtr<ID2D1SvgElement>& foundElement
    )
{
    ctl::ComPtr<ID2D1SvgElement> rootElement;
    svgDocument->GetRoot(rootElement.ReleaseAndGetAddressOf());

    if (rootElement != nullptr)
    {
        ctl::ComPtr<ID2D1SvgElement> currentChild;
        rootElement->GetFirstChild(currentChild.ReleaseAndGetAddressOf());

        while (currentChild != nullptr)
        {
            std::vector<wchar_t> tagName(currentChild->GetTagNameLength() + 1);
            IFC_NOTRACE_RETURN(currentChild->GetTagName(&tagName[0], tagName.size()));

            if (wcscmp(&tagName[0], elementId) == 0)
            {
                foundElement = currentChild;
                return S_OK;
            }

            ctl::ComPtr<ID2D1SvgElement> nextChild;
            IFC_NOTRACE_RETURN(rootElement->GetNextChild(currentChild.Get(), nextChild.ReleaseAndGetAddressOf()));
            currentChild = nextChild;
        }
    }

    return S_FALSE;
}