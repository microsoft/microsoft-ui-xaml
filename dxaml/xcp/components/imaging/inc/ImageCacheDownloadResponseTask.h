// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <weakref_ptr.h>
#include <weakref_ptr.h>
#include <ImageProviderInterfaces.h>

class ImageCache;
struct IPALDownloadResponse;

class ImageCacheDownloadResponseTask
    : public CXcpObjectBase< IImageTask >
{
public:
    ImageCacheDownloadResponseTask(
        _In_ xref::weakref_ptr<ImageCache> imageCache,
        _In_ xref_ptr<IPALDownloadResponse> downloadResponse,
        HRESULT hr);
    ~ImageCacheDownloadResponseTask() override;

    // IImageTask
    uint64_t GetRequestId() const override { return 0; }
    _Check_return_ HRESULT Execute() override;

private:
    xref::weakref_ptr<ImageCache> m_imageCache;
    xref_ptr<IPALDownloadResponse> m_downloadResponse;
    HRESULT m_hr = S_OK;
};
