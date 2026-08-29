// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <ImageProviderInterfaces.h>

class ImageDecodeRequest;
class ImageTaskDispatcher;
struct IImageAvailableResponse;

class ImageProviderDecodeHandlerTask final
    : public IImageDecodeCallback
    , public IImageTask
{
public:
    FORWARD_ADDREF_RELEASE(IImageDecodeCallback);

    ImageProviderDecodeHandlerTask(
        _In_ ImageTaskDispatcher* pDispatcher,
        _In_ xref_ptr<ImageDecodeRequest> spImageDecodeRequest
        );

    // IImageDecodeCallback
    _Check_return_ HRESULT OnDecode(
        _In_ xref_ptr<IImageAvailableResponse> spResponse,
        _In_ uint64_t requestId
        ) override;

    // IImageTask
    uint64_t GetRequestId() const override { return 0; };
    _Check_return_ HRESULT Execute() override;

private:
    ~ImageProviderDecodeHandlerTask() override;

    ImageTaskDispatcher* m_pDispatcher;
    xref_ptr<ImageDecodeRequest> m_spImageDecodeRequest;
    xref_ptr<IImageAvailableResponse> m_spDecodeResponse;
};
