// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <weakref_ptr.h>
#include <ImageCache.h>
#include <ImageTaskDispatcher.h>
#include <ImageCacheDownloadCallbackMarshaller.h>
#include <ImageCacheDownloadResponseTask.h>
#include <ImageCacheDownloadProgressTask.h>

ImageCacheDownloadCallbackMarshaller::ImageCacheDownloadCallbackMarshaller(
    _In_ ImageTaskDispatcher* dispatcher,
    _In_ ImageCache* imageCache
    )
    : m_dispatcherNoRef(dispatcher)
    , m_imageCache(xref::get_weakref(imageCache))
{
    m_creatorThreadId = ::GetCurrentThreadId();
}

ImageCacheDownloadCallbackMarshaller::~ImageCacheDownloadCallbackMarshaller()
{
}

_Check_return_ HRESULT ImageCacheDownloadCallbackMarshaller::GotResponse(
    _In_ xref_ptr<IPALDownloadResponse> response, 
    HRESULT status
    )
{
    auto responseTask = make_xref<ImageCacheDownloadResponseTask>(
        m_imageCache,
        std::move(response),
        status);

    if (m_creatorThreadId != ::GetCurrentThreadId())
    {
        IFC_RETURN(m_dispatcherNoRef->QueueTask(responseTask.get()));
    }
    else
    {
        IFC_RETURN(responseTask->Execute());
    }
    return S_OK;
}

_Check_return_ HRESULT ImageCacheDownloadCallbackMarshaller::GotData(
    uint64_t size,
    uint64_t totalSize
    )
{
    auto progressTask = make_xref<ImageCacheDownloadProgressTask>(
        m_imageCache,
        size,
        totalSize);

    if (m_creatorThreadId != ::GetCurrentThreadId())
    {
        IFC_RETURN(m_dispatcherNoRef->QueueTask(progressTask.get()));
    }
    else
    {
        IFC_RETURN(progressTask->Execute());
    }
    return S_OK;
}
