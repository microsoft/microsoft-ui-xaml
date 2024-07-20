// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "BitmapData.h"

BitmapData::BitmapData()
    : m_bytes{}
    , m_capacity{}
    , m_stride{}
    , m_bitmapDescription{}
    , m_bitmapSourceDescription{}
{
}

static size_t _CoTaskMemSize(_In_ _Post_writable_byte_size_(return) void *pv)
{
    size_t cb = 0;
    IMalloc *pMalloc;
    if (SUCCEEDED(CoGetMalloc(1, &pMalloc))) // should never fail (static v-table)
    {
        // Returns (size_t)-1 if pv is NULL.
        // Result is indeterminate if pv does not belong to CoTaskMemAlloc.
        cb = pMalloc->GetSize(pv);
        pMalloc->Release();
    }
    return cb;
}

_Use_decl_annotations_
HRESULT BitmapData::RuntimeClassInitialize(
    unsigned int capacity,
    unsigned int stride,
    BitmapDescription bitmapDescription,
    BitmapDescription bitmapSourceDescription)
{
    m_bytes = static_cast<decltype(m_bytes)>(CoTaskMemAlloc(capacity));
    if (m_bytes)
    {
#ifdef COM_SUPPORT_MALLOC_SPIES
                // Zero-initialize the buffer
                // The actual size might be larger than cb due to spies present.
                // Initialize to the actual size in case of realloc later,
                // or there might be an uninitialized gap in between.
                const size_t cbActual = _CoTaskMemSize(m_bytes);
                ZeroMemory(m_bytes, cbActual);
#else
                ZeroMemory(m_bytes, capacity);
#endif
    }
    else
    {
        IFC_RETURN(E_OUTOFMEMORY);
    }

    m_capacity = capacity;
    m_stride = stride;
    m_bitmapDescription = bitmapDescription;
    m_bitmapSourceDescription = bitmapSourceDescription;

    return S_OK;
}

BitmapData::~BitmapData()
{
    CoTaskMemFree(m_bytes);
}

BYTE* BitmapData::GetBytes() const
{
    return m_bytes;
}

_Use_decl_annotations_
IFACEMETHODIMP
BitmapData::CopyBytesTo(
    unsigned int sourceOffsetInBytes,
    unsigned int maxBytesToCopy,
    BYTE *pvBytes,
    unsigned int *numberOfBytesCopied)
{
    IFCPTR_RETURN(pvBytes);
    IFCPTR_RETURN(numberOfBytesCopied);

    unsigned int bytesToCopy = m_capacity > sourceOffsetInBytes ? std::min(maxBytesToCopy, m_capacity - sourceOffsetInBytes) : 0;

    VERIFY_COND(memcpy_s(
        pvBytes,                        // _Dst
        maxBytesToCopy,                 // _DstSize
        m_bytes + sourceOffsetInBytes,  // _Src
        bytesToCopy),                   // _MaxCount
        == 0);

    *numberOfBytesCopied = bytesToCopy;

    return S_OK;
}

_Use_decl_annotations_
IFACEMETHODIMP
BitmapData::GetStride(
    unsigned int* pStride)
{
    IFCPTR_RETURN(pStride);

    *pStride = m_stride;

    return S_OK;
}

_Use_decl_annotations_
IFACEMETHODIMP
BitmapData::GetBitmapDescription(
    BitmapDescription *pBitmapDescription)
{
    IFCPTR_RETURN(pBitmapDescription);

    *pBitmapDescription = m_bitmapDescription;

    return S_OK;
}

_Use_decl_annotations_
IFACEMETHODIMP
BitmapData::GetSourceBitmapDescription(
    BitmapDescription *pBitmapDescription)
{
    IFCPTR_RETURN(pBitmapDescription);

    *pBitmapDescription = m_bitmapSourceDescription;

    return S_OK;
}
