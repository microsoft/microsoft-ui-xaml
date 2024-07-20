// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <palnetwork.h>
#include <ImageCache.h>
#include <ImageCacheDownloadResponseTask.h>

ImageCacheDownloadResponseTask::ImageCacheDownloadResponseTask(
    _In_ xref::weakref_ptr<ImageCache> imageCache,
    _In_ xref_ptr<IPALDownloadResponse> downloadResponse,
    HRESULT hr
    )
    : m_imageCache(std::move(imageCache))
    , m_downloadResponse(std::move(downloadResponse))
    , m_hr(hr)
{
}

ImageCacheDownloadResponseTask::~ImageCacheDownloadResponseTask()
{
}

_Check_return_ HRESULT
ImageCacheDownloadResponseTask::Execute()
{
    auto imageCacheStrong = m_imageCache.lock();
    if (imageCacheStrong)
    {
        IFC_RETURN(imageCacheStrong->GotDownloadResponse(std::move(m_downloadResponse), m_hr));
    }
    return S_OK;
}
