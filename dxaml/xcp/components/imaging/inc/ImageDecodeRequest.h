// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <imaginginterfaces.h>
#include <memory>

class CCoreServices; // fixme

class AsyncImageDecoder;
class ImageCache;
class ImageDecodeParams;
struct IImageAvailableCallback;
struct IImageAvailableResponse;

class ImageDecodeRequest final : public CXcpObjectBase<IAbortableImageOperation>
{
public:
    explicit ImageDecodeRequest(_In_opt_ CCoreServices* core);
    ImageDecodeRequest(_In_opt_ CCoreServices* core, xref_ptr<ImageCache> imageCache);

    ~ImageDecodeRequest() override;

    _Check_return_ HRESULT SetImageDecoder(std::unique_ptr<AsyncImageDecoder> imageDecoder);

    const xref_ptr<ImageDecodeParams>& GetDecodeParams() const { return m_decodeParams; }

    _Check_return_ HRESULT NotifyCallback(_In_ IImageAvailableResponse *response);
    const xref_ptr<IImageAvailableCallback>& GetImageAvailableCallback() const;

    uint64_t GetRequestId() const { return m_requestId; }

    // IAbortableImageOperation
    void DisconnectImageOperation() override;
    void CleanupDeviceRelatedResources() override;

    _Check_return_ HRESULT PlayAnimation() override;
    _Check_return_ HRESULT StopAnimation() override;
    void SuspendAnimation() override;
    _Check_return_ HRESULT ResumeAnimation() override;

    bool IsDecodeInProgress() override;

    _Check_return_ HRESULT SetDecodeParams(
        _In_ xref_ptr<IImageAvailableCallback> imageAvailableCallback,
        _In_ xref_ptr<ImageDecodeParams> spDecodeParams) override;

    XSIZE GetDecodedSize() const override;

    bool HasDecoder() const override { return m_imageDecoder != nullptr; }

private:
    void AddToGlobalCount();
    void RemoveFromGlobalCount();

private:
    uint64_t m_requestId;
    CCoreServices* m_coreNoRef;
    xref_ptr<ImageCache> m_imageCache;
    xref_ptr<IImageAvailableCallback> m_imageAvailableCallback;
    std::unique_ptr<AsyncImageDecoder> m_imageDecoder;
    xref_ptr<ImageDecodeParams> m_decodeParams;
    bool m_inPendingGlobalCount;
};
