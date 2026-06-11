// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <ImageProviderInterfaces.h>

class ImageCache;
class ImageDecodeRequest;
struct IImageAvailableResponse;

class ImageCacheDecodeHandlerTask : public CXcpObjectBase< IImageTask >
{
public:
    ImageCacheDecodeHandlerTask(
        _In_ xref_ptr<ImageCache> imageCache,
        _In_ xref_ptr<IImageAvailableResponse> decodeResponse,
        _In_ uint64_t requestId
        );
    ~ImageCacheDecodeHandlerTask() override;

    // IImageTask
    uint64_t GetRequestId() const override { return m_requestId; }
    _Check_return_ HRESULT Execute() override;

private:
    xref_ptr<ImageCache> m_imageCache;
    xref_ptr<IImageAvailableResponse> m_decodeResponse;
    uint64_t m_requestId;
};
