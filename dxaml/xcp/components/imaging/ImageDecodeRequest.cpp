// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <AsyncDecodeResponse.h>
#include <AsyncImageDecoder.h>
#include <EncodedImageData.h>
#include <ImageCache.h>
#include <ImageDecodeParams.h>
#include <ImageDecodeRequest.h>
#include <ImagingUtility.h>
#include <corep.h>

static std::atomic<uint64_t> s_uniqueIdGen;

ImageDecodeRequest::ImageDecodeRequest(_In_opt_ CCoreServices* core)
    : ImageDecodeRequest(core, nullptr)
{
}

ImageDecodeRequest::ImageDecodeRequest(_In_opt_ CCoreServices* core, xref_ptr<ImageCache> imageCache)
    : m_requestId(++s_uniqueIdGen)
    , m_coreNoRef(core)
    , m_imageCache(imageCache)
    , m_inPendingGlobalCount(false)
{
    XCP_WEAK(&m_coreNoRef);
    AddToGlobalCount();
}

ImageDecodeRequest::~ImageDecodeRequest()
{
    RemoveFromGlobalCount();

    if (m_imageCache)
    {
        if (m_imageDecoder)
        {
            VERIFYHR(m_imageCache->OnDecoderDetached(this, std::move(m_imageDecoder)));
        }
        VERIFYHR(m_imageCache->OnRequestReleasing(this));
    }
}

HRESULT ImageDecodeRequest::SetImageDecoder(std::unique_ptr<AsyncImageDecoder> imageDecoder)
{
    m_imageDecoder = std::move(imageDecoder);

    IFC_RETURN(m_imageDecoder->SetDecodeParams(m_decodeParams, m_requestId));

    return S_OK;
}

HRESULT ImageDecodeRequest::SetDecodeParams(
    _In_ xref_ptr<IImageAvailableCallback> imageAvailableCallback,
    _In_ xref_ptr<ImageDecodeParams> decodeParams)
{
    m_imageAvailableCallback = std::move(imageAvailableCallback);
    m_decodeParams = std::move(decodeParams);

    if (m_imageDecoder)
    {
        IFC_RETURN(m_imageDecoder->SetDecodeParams(m_decodeParams, m_requestId));
    }

    return S_OK;
}

// Returns the size of the decoded image surface as it would be according to the current decode params.
XSIZE ImageDecodeRequest::GetDecodedSize() const
{
    XSIZE decodedSize = {};

    if (auto encodedImageData = m_imageCache ? m_imageCache->GetEncodedImageData() : nullptr)
    {
        if (encodedImageData->IsMetadataAvailable() && m_decodeParams != nullptr)
        {
            if (m_decodeParams->GetDecodeWidth() != 0 || m_decodeParams->GetDecodeHeight() != 0)
            {
                decodedSize = ImagingUtility::CalculateScaledSize(
                    encodedImageData->GetMetadata(),
                    *m_decodeParams,
                    !encodedImageData->IsSvg() /* clampToMetadataSize */);
            }
            else
            {
                decodedSize.Width = encodedImageData->GetMetadata().width;
                decodedSize.Height = encodedImageData->GetMetadata().height;
            }
        }
    }

    return decodedSize;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Special method to simply disconnect this request from its callback.
// 
//  Note:
//      Today, Abort doesn't actually cancel decoding.  If we ever fix this, this
//      Disconnect method should be revamped to find a way to cancel without
//      notifying the callback.
//
//------------------------------------------------------------------------
void ImageDecodeRequest::DisconnectImageOperation()
{
    // Release callback first to guarantee it's never called
    m_imageAvailableCallback.reset();

    if (m_imageCache && m_imageDecoder)
    {
        VERIFYHR(m_imageCache->OnDecoderDetached(this, std::move(m_imageDecoder)));
    }
    else
    {
        m_imageDecoder.reset();
    }

    RemoveFromGlobalCount();
}

void ImageDecodeRequest::CleanupDeviceRelatedResources()
{
    if (m_imageDecoder != nullptr)
    {
        m_imageDecoder->CleanupDeviceRelatedResources();
    }
}

_Check_return_ HRESULT ImageDecodeRequest::PlayAnimation()
{
    if (m_imageDecoder != nullptr)
    {
        IFC_RETURN(m_imageDecoder->PlayAnimation());
    }
    return S_OK;
}

_Check_return_ HRESULT ImageDecodeRequest::StopAnimation()
{
    if (m_imageDecoder != nullptr)
    {
        IFC_RETURN(m_imageDecoder->StopAnimation());
    }
    return S_OK;
}

void ImageDecodeRequest::SuspendAnimation()
{
    if (m_imageDecoder != nullptr)
    {
        m_imageDecoder->SuspendAnimation();
    }
}

HRESULT ImageDecodeRequest::ResumeAnimation()
{
    if (m_imageDecoder != nullptr)
    {
        IFC_RETURN(m_imageDecoder->ResumeAnimation());
    }
    return S_OK;
}

bool ImageDecodeRequest::IsDecodeInProgress()
{
    if (m_imageDecoder != nullptr)
    {
        return m_imageDecoder->IsDecodeInProgress();
    }
    return false;
}

_Check_return_ HRESULT ImageDecodeRequest::NotifyCallback(_In_ IImageAvailableResponse* pResponse)
{
    xref_ptr<ImageDecodeRequest> strongThis(this); // in case we get released by the callback
    if (m_imageAvailableCallback != NULL)
    {
        IFC_RETURN(m_imageAvailableCallback->OnImageAvailable(pResponse));
    }
    strongThis->RemoveFromGlobalCount();

    return S_OK;
}

const xref_ptr<IImageAvailableCallback>& ImageDecodeRequest::GetImageAvailableCallback() const
{
    return m_imageAvailableCallback;
}

void ImageDecodeRequest::AddToGlobalCount()
{
    if (!m_inPendingGlobalCount)
    {
        // no core in unit tests
        if (m_coreNoRef != nullptr)
        {
            m_coreNoRef->IncrementPendingDecodeCount();
        }
        m_inPendingGlobalCount = true;
    }
}

void ImageDecodeRequest::RemoveFromGlobalCount()
{
    if (m_inPendingGlobalCount)
    {
        // no core in unit tests
        if (m_coreNoRef != nullptr)
        {
            m_coreNoRef->DecrementPendingDecodeCount();
        }
        m_inPendingGlobalCount = false;
    }
}
