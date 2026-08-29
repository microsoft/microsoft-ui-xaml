// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <weakref_ptr.h>
#include <ImageCache.h>
#include <ImageCacheDownloadProgressTask.h>

ImageCacheDownloadProgressTask::ImageCacheDownloadProgressTask(
    _In_ xref::weakref_ptr<ImageCache> imageCache,
    uint64_t size,
    uint64_t totalSize
    )
    : m_imageCache(std::move(imageCache))
    , m_size(size)
    , m_totalSize(totalSize)
{
}

ImageCacheDownloadProgressTask::~ImageCacheDownloadProgressTask()
{
}

_Check_return_ HRESULT
ImageCacheDownloadProgressTask::Execute()
{
    auto imageCacheStrong = m_imageCache.lock();
    if (imageCacheStrong)
    {
        IFC_RETURN(imageCacheStrong->GotDownloadProgressUpdate(m_size, m_totalSize));
    }
    return S_OK;
}
