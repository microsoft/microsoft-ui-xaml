// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <weakref_ptr.h>
#include <ImageProviderInterfaces.h>

class ImageCache;

class ImageCacheDownloadProgressTask final
    : public CXcpObjectBase< IImageTask >
{
public:
    ImageCacheDownloadProgressTask(
        _In_ xref::weakref_ptr<ImageCache> imageCache,
        uint64_t size,
        uint64_t totalSize);
    ~ImageCacheDownloadProgressTask() override;

    uint64_t GetRequestId() const override { return 0; }
    _Check_return_ HRESULT Execute() override;

private:
    xref::weakref_ptr<ImageCache> m_imageCache;
    uint64_t m_size = 0;
    uint64_t m_totalSize = 0;
};
