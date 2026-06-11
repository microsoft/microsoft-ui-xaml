// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "minpal.h"
#include "xref_ptr.h"
#include <vector>
#include <memory>

class ImageCache;
class DecodedImageCache;
class OfferableSoftwareBitmap;
class ImageCopyParams;
class SurfaceDecodeParams;
struct IPALWorkItem;

using SurfaceUpdateList = std::vector<xref_ptr<SurfaceDecodeParams>>;

struct IImageAvailableResponse : public IObject
{
    virtual _Check_return_ HRESULT GetDecodingResult() const = 0;

    virtual const xref_ptr<OfferableSoftwareBitmap>& GetSurface() const = 0;
};

struct IImageDecodeCallback : public IObject
{
    virtual _Check_return_ HRESULT OnDecode(
        _In_ xref_ptr<IImageAvailableResponse> response,
        _In_ uint64_t requestId
        ) = 0;

    virtual ~IImageDecodeCallback()
    {
        m_ref_count.end_destroying();
    }

    _Ret_notnull_ xref::details::control_block* EnsureControlBlock()
    {
        return m_ref_count.ensure_control_block();
    }

    XUINT32 AddRef() override
    {
        return m_ref_count.ThreadSafeAddRef();
    }

    XUINT32 Release() override
    {
        auto refCount = m_ref_count.ThreadSafeRelease();
        if (0 == refCount)
        {
            m_ref_count.start_destroying();
            delete this;
        }
        return refCount;
    }

private:
    xref::details::optional_ref_count m_ref_count;
};

struct IImageAvailableCallback : public IObject
{
    virtual _Check_return_ HRESULT OnImageAvailable(
        _In_ IImageAvailableResponse* response
        ) = 0;
};

struct IInvalidateImageCacheCallback
{
    virtual void OnImageCacheInvalidated(
        _In_ ImageCache* cache
        ) = 0;
};

struct IInvalidateDecodedImageCacheCallback
{
    virtual _Check_return_ HRESULT OnCacheInvalidated(
        _In_ DecodedImageCache* cache
        ) = 0;
};

struct IImageTask : public IObject
{
    virtual uint64_t GetRequestId() const = 0;
    virtual _Check_return_ HRESULT Execute() = 0;
};


struct IAsyncImageFactory : public IObject
{
    virtual _Check_return_ HRESULT CopyAsync(
        _In_ const xref_ptr<ImageCopyParams>& spCopyParams,
        _In_ const xref_ptr<IImageDecodeCallback>& spCallback,
        _Outptr_ IPALWorkItem** ppWork
        ) = 0;
};
