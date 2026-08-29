// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <ImageCache.h>
#include <ImageCacheDecodeHandlerTask.h>

ImageCacheDecodeHandlerTask::ImageCacheDecodeHandlerTask(
    _In_ xref_ptr<ImageCache> imageCache,
    _In_ xref_ptr<IImageAvailableResponse> decodeResponse,
    _In_ uint64_t requestId
    )
    : m_imageCache(std::move(imageCache))
    , m_decodeResponse(std::move(decodeResponse))
    , m_requestId(requestId)
{
}

ImageCacheDecodeHandlerTask::~ImageCacheDecodeHandlerTask()
{
}

_Check_return_ HRESULT ImageCacheDecodeHandlerTask::Execute()
{
    IFC_RETURN(m_imageCache->HandleDecodeResponse(
        std::move(m_decodeResponse),
        m_requestId
        ));
    m_imageCache.reset();

    return S_OK;
}
