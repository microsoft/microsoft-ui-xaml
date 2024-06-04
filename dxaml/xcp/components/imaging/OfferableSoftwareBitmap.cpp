// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <precomp.h>
#include <MUX-ETWEvents.h>
#include "OfferableSoftwareBitmap.h"
#include <PixelFormat.h>

OfferableSoftwareBitmap::OfferableSoftwareBitmap(
    PixelFormat pixelFormat,
    uint32_t width,
    uint32_t height,
    bool isOpaque
    )
    : m_pixelFormat(pixelFormat)
    , m_width(width)
    , m_height(height)
    , m_isOpaque(isOpaque)
    , m_offerableMemory(GetBufferSize())
{
    TraceOfferableSoftwareBitmapAllocInfo(reinterpret_cast<uint64_t>(this), m_width, m_height);
}

OfferableSoftwareBitmap::~OfferableSoftwareBitmap()
{
    FireOnDelete();

    TraceOfferableSoftwareBitmapFreeInfo(reinterpret_cast<uint64_t>(this));
}

void* OfferableSoftwareBitmap::GetBuffer()
{
    EnsureValid();
    return m_offerableMemory.GetBuffer();
}

const void* OfferableSoftwareBitmap::GetBuffer() const
{
    EnsureValid();
    return m_offerableMemory.GetBuffer();
}

uint32_t OfferableSoftwareBitmap::GetBufferSize() const
{
    return m_height * GetStride();
}

uint32_t OfferableSoftwareBitmap::GetStride() const
{
    uint32_t pixelBytesSize = GetPixelPlaneBitStride(m_pixelFormat) / 8;
    return m_width * pixelBytesSize;
}

_Check_return_ HRESULT OfferableSoftwareBitmap::Lock(
    _Outptr_result_bytebuffer_(*pStride * *pHeight) void** ppAddress,
    _Out_ int32_t* pStride,
    _Out_ uint32_t* pWidth,
    _Out_ uint32_t* pHeight)
{
    EnsureValid();
    
    *ppAddress = m_offerableMemory.GetBuffer();
    *pStride = static_cast<int32_t>(GetStride());
    *pWidth = GetWidth();
    *pHeight = GetHeight();

    return S_OK;
}

_Check_return_ HRESULT OfferableSoftwareBitmap::Unlock()
{
    return S_OK;
}

_Check_return_ HRESULT OfferableSoftwareBitmap::Offer()
{
    m_offerableMemory.Offer();
    m_isOffered = true;
    return S_OK;
}

_Check_return_ HRESULT OfferableSoftwareBitmap::Reclaim(
    _Out_ bool* pWasDiscarded
    )
{
    *pWasDiscarded = m_offerableMemory.Reclaim();
    m_isOffered = false;
    return S_OK;
}

void OfferableSoftwareBitmap::EnsureValid() const
{
    // Surface must be reclaimed before it can be used again if it was offered.
    FAIL_FAST_ASSERT(!m_isOffered);
}
