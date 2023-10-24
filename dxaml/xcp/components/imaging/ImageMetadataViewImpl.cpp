// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ImageMetadataViewImpl.h"
#include <EncodedImageData.h>
#include <DoubleUtil.h>
#include "d3d11device.h"

void ImageMetadataViewImpl::SetEncodedImageData(_In_opt_ std::shared_ptr<EncodedImageData> encodedImageData, HRESULT downloadStatus)
{
    m_encodedImageData = std::move(encodedImageData);
    m_downloadParseResult = downloadStatus;
    m_needParse = SUCCEEDED(downloadStatus) && (m_encodedImageData != nullptr);

    if (m_encodedImageData != nullptr)
    {
        m_downloadProgress = 1.0f;
    }

    TriggerViewUpdated();
}

void ImageMetadataViewImpl::SetDownloadProgress(float downloadProgress)
{
    if (m_downloadProgress != downloadProgress)
    {
        m_downloadProgress = downloadProgress;
        TriggerViewUpdated();
    }
}

const ImageMetadata* ImageMetadataViewImpl::GetImageMetadata() const
{
    if (m_encodedImageData != nullptr)
    {
        // TODO: image metadata is usually parsed when we call GetHR. GetImageMetadata is called in a lot more places,
        // so to limit code churn it hasn't been updated to trace an event.
        ParseImageMetadata(nullptr, 0);
        return m_encodedImageData->IsMetadataAvailable() ? &m_encodedImageData->GetMetadata() : nullptr;
    }

    return nullptr;
}

float ImageMetadataViewImpl::GetDownloadProgress() const
{
    return (m_encodedImageData != nullptr) ? 1 : m_downloadProgress;
}

void ImageMetadataViewImpl::SetGraphicsDevice(_In_opt_ CD3D11Device* graphicsDevice)
{
    CD3D11Device* myGraphicsDevice = m_graphicsDevice.lock().get();
    if (myGraphicsDevice != graphicsDevice)
    {
        m_graphicsDevice = xref::get_weakref(graphicsDevice);
        m_needParse = (m_encodedImageData != nullptr);
    }
}

wf::Size ImageMetadataViewImpl::GetMaxRootSize()
{
    return m_maxRootSize;
}

void ImageMetadataViewImpl::SetMaxRootSize(wf::Size maxRootSize)
{
    if (!DirectUI::DoubleUtil::AreClose(m_maxRootSize.Width, maxRootSize.Width) &&
        !DirectUI::DoubleUtil::AreClose(m_maxRootSize.Height, maxRootSize.Height))
    {
        m_maxRootSize = maxRootSize;
        m_needParse = (m_encodedImageData != nullptr);
    }
}

HRESULT ImageMetadataViewImpl::GetHR(_In_ const std::shared_ptr<ImagingTelemetry::ImageDecodeActivity>& decodeActivity, uint64_t imageId) const
{
    ParseImageMetadata(decodeActivity, imageId);
    return m_downloadParseResult;
}

void ImageMetadataViewImpl::ParseImageMetadata(_In_opt_ const std::shared_ptr<ImagingTelemetry::ImageDecodeActivity>& decodeActivity, uint64_t imageId) const
{
    if (m_needParse)
    {
        CD3D11Device* graphicsDevice = m_graphicsDevice.lock().get();
        if (decodeActivity)
        {
            decodeActivity->ParseImageMetadataStart(imageId);
        }

        // Note: graphicsDevice is allowed to be null if we're not working with a SVG file. LoadedImageSurface doesn't
        // even set a graphics device in this ImageMetadataViewImpl.
        m_downloadParseResult = m_encodedImageData->Parse(graphicsDevice, m_maxRootSize);
        m_needParse = false;

        if (decodeActivity)
        {
            decodeActivity->ParseImageMetadataStop(imageId, m_downloadParseResult);
        }
    }
}