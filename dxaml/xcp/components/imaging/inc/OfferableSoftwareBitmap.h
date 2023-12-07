// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <wil\resource.h>
#include "refcounting.h"
#include "palgfx.h"
#include "palnotify.h"
#include "OfferableMemory.h"

enum PixelFormat;

// TODO: Add multi-threading SRWLock on this with read/write requests.

// TODO: Modernize the CXcpObjectBase and NotifyOnDelete when
//                 doing a full pipeline rewrite and the PAL stuff is no
//                 longer needed in the XAML code.  At which point
//                 IPALSurface can be modernized.

// TODO: Try to make CNotifyOnDelete RAII class in the future which doesn't
//                 need an explicit call to FireOnDelete();

// Provides an IPALSurface software bitmap implementation around offerable memory.
class OfferableSoftwareBitmap
    : public CXcpObjectBase<IPALSurface>
    , public CNotifyOnDelete
{
public:
    OfferableSoftwareBitmap(
        PixelFormat pixelFormat,
        uint32_t width,
        uint32_t height,
        bool isOpaque = true
        );

    ~OfferableSoftwareBitmap() override;

    // OfferableSoftwareBitmap actually doesn't need to lock.  It provides those
    // interfaces for the sake of IPALSurface, but it is just more efficient to
    // operate directly on the buffer.  It also doesn't have any stride which
    // can be ignored.  GetWidth/GetHeight/GetBuffer is sufficient.
    void* GetBuffer();
    const void* GetBuffer() const;

    uint32_t GetBufferSize() const;

    uint32_t GetStride() const;

    // IPALSurface Implementation
    _Check_return_ HRESULT Lock(
        _Outptr_result_bytebuffer_(*pStride * *pHeight) void** ppAddress,
        _Outptr_ int32_t* pStride,
        _Outptr_ uint32_t* pWidth,
        _Outptr_ uint32_t* pHeight) override;

    _Check_return_ HRESULT Unlock() override;

    uint32_t GetWidth() const override { return m_width; }
    uint32_t GetHeight() const override { return m_height; }
    PixelFormat GetPixelFormat() override { return m_pixelFormat; }
    bool IsOpaque() override { return m_isOpaque; }
    bool IsVirtual() override { return false; }
    void SetIsOpaque(bool isOpaque) override { m_isOpaque = !!isOpaque;  }

    _Check_return_ HRESULT GetNotifyOnDelete(_Outptr_ CNotifyOnDelete** ppNotifyOnDelete) override
    {
        *ppNotifyOnDelete = this;
        return S_OK;
    }

    _Check_return_ HRESULT Offer() override;
    _Check_return_ HRESULT Reclaim(_Out_ bool* pWasDiscarded) override;

private:
    void EnsureValid() const;

    PixelFormat m_pixelFormat;
    uint32_t m_width;
    uint32_t m_height;
    bool m_isOpaque;
    bool m_isOffered = false;

    OfferableMemory m_offerableMemory; // must initialize after size and format
};
