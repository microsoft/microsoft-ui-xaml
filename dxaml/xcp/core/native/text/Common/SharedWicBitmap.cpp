// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SharedWicBitmap.h"
#include <SafeInt/SafeInt.hpp>

//---------------------------------------------------------------------------
//
//  Creates and initializes a new instance of the SharedWicBitmap class with 
//  external memory buffer.
//
//---------------------------------------------------------------------------
HRESULT SharedWicBitmap::Create(
    _In_ XUINT32 width,
    _In_ XUINT32 height,
    _In_ XUINT32 stride,
    _In_ WICPixelFormatGUID const& pixelFormat,
    _In_opt_ XUINT8 *pBuffer,
    _Outptr_ SharedWicBitmap **ppWICBitmap
    )
{
    // The texture stride need not strictly be 4-byte aligned as D2D has
    // work-arounds for WARP limitations, but we still want callers to use
    // 32-bit aligned accesses because it is faster, as D2D doesn't need
    // to copy into intermediate textures.
    ASSERT((stride & 3) == 0);

    HRESULT hr = S_OK;
    bool usesSharedMemory = (pBuffer != nullptr);
    if (!usesSharedMemory)
    {
        XUINT32 size;
        IFCCHECK(SafeMultiply(stride, height, size));
        pBuffer = new(NO_FAIL_FAST) XUINT8[size];
        IFCOOM(pBuffer);
        memset(pBuffer, 0x00, size);
    }
    *ppWICBitmap = new SharedWicBitmap(width, height, stride, pixelFormat, pBuffer, usesSharedMemory);
    pBuffer = NULL;

Cleanup:
    delete [] pBuffer;
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Initializes a new instance of the SharedWicBitmap class.
//
//---------------------------------------------------------------------------
SharedWicBitmap::SharedWicBitmap(
    _In_ XUINT32 width,
    _In_ XUINT32 height,
    _In_ XUINT32 stride,
    _In_ WICPixelFormatGUID pixelFormat,
    _In_ XUINT8 *pBuffer,
    _In_ bool usesSharedMemory
    )
    : m_refCount(1)
    , m_width(width)
    , m_height(height)
    , m_stride(stride)
    , m_pixelFormat(pixelFormat)
    , m_pBuffer(pBuffer)
    , m_usesSharedMemory(usesSharedMemory)
{
}

//---------------------------------------------------------------------------
//
//  Release resources associated with the SharedWicBitmap.
//
//---------------------------------------------------------------------------
SharedWicBitmap::~SharedWicBitmap()
{
    if (!m_usesSharedMemory)
    {
        delete [] m_pBuffer;
    }
}

//---------------------------------------------------------------------------
//
//  Makes the internal buffer sharable and detaches ownership.
//
//---------------------------------------------------------------------------
void SharedWicBitmap::MakeBufferSharable()
{
    m_usesSharedMemory = TRUE;
}

//---------------------------------------------------------------------------
//
//  Provided access to the internal buffer.
//
//---------------------------------------------------------------------------
void SharedWicBitmap::GetBuffer(
    _Out_ XUINT32 *pWidth,
    _Out_ XUINT32 *pHeight,
    _Out_ XUINT32 *pStride,
    _Outptr_result_buffer_((*pStride) * (*pHeight)) XUINT8 **pBuffer
    )
{
    *pWidth = m_width;
    *pHeight = m_height;
    *pStride = m_stride;
    *pBuffer = m_pBuffer;
}

//---------------------------------------------------------------------------
//
//  IUnknown::QueryInterface implementation.
//
//---------------------------------------------------------------------------
HRESULT SharedWicBitmap::QueryInterface(
    _In_ REFIID riid,
    _Outptr_ void **ppvObject
    )
{
    if (__uuidof(IUnknown) == riid)
    {
        *ppvObject = static_cast<IUnknown*>(static_cast<IWICBitmap*>(this));
    }
    else if (__uuidof(IWICBitmap) == riid)
    {
        *ppvObject = static_cast<IWICBitmap*>(this);
    }
    else if (__uuidof(IWICBitmapLock) == riid)
    {
        *ppvObject = static_cast<IWICBitmapLock*>(this);
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    AddRef();

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  IUnknown::AddRef implementation.
//
//---------------------------------------------------------------------------
ULONG SharedWicBitmap::AddRef()
{
    return ++m_refCount;
}

//---------------------------------------------------------------------------
//
//  IUnknown::Release implementation.
//
//---------------------------------------------------------------------------
ULONG SharedWicBitmap::Release()
{
    XUINT32 newRefCount = --m_refCount;
    if (newRefCount == 0)
    {
        delete this;
    }

    return newRefCount;
}

//---------------------------------------------------------------------------
//
//  IWICBitmap and IWICBitmapLock::GetSize implementation.
//
//---------------------------------------------------------------------------
HRESULT SharedWicBitmap::GetSize(
    _Out_ UINT *pWidth,
    _Out_ UINT *pHeight
    )
{
    *pWidth = m_width;
    *pHeight = m_height;
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  IWICBitmapLock::GetStride implementation.
//
//---------------------------------------------------------------------------
HRESULT SharedWicBitmap::GetStride(
    _Out_ UINT *pStride
    )
{
    *pStride = m_stride;
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  IWICBitmap and IWICBitmapLock::GetPixelFormat implementation.
//
//---------------------------------------------------------------------------
HRESULT SharedWicBitmap::GetPixelFormat(
    _Out_ WICPixelFormatGUID *pPixelFormat
    )
{
    *pPixelFormat = m_pixelFormat;
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  IWICBitmap::GetResolution implementation.
//
//---------------------------------------------------------------------------
HRESULT SharedWicBitmap::GetResolution(
    _Out_ double *pDpiX,
    _Out_ double *pDpiY
    )
{
    *pDpiX = 96.0f;
    *pDpiY = 96.0f;
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  IWICBitmap::CopyPalette implementation.
//
//---------------------------------------------------------------------------
HRESULT SharedWicBitmap::SetResolution(
    double dpiX,
    double dpiY
    )
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
//
//  IWICBitmap::CopyPalette implementation.
//
//---------------------------------------------------------------------------
HRESULT SharedWicBitmap::CopyPalette(
    _In_opt_ IWICPalette *pIPalette
    )
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
//
//  IWICBitmap::SetPalette implementation.
//
//---------------------------------------------------------------------------
HRESULT SharedWicBitmap::SetPalette(
    _In_opt_ IWICPalette *pIPalette
    )
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
//
//  IWICBitmap::CopyPixels implementation.
//
//---------------------------------------------------------------------------
HRESULT SharedWicBitmap::CopyPixels(
    _In_opt_ const WICRect *prc,
    UINT cbStride,
    UINT cbBufferSize,
    _Out_writes_all_(cbBufferSize) BYTE *pbBuffer
    )
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
//
//  IWICBitmap::Lock implementation.
//
//---------------------------------------------------------------------------
HRESULT SharedWicBitmap::Lock(
    _In_opt_ const WICRect *prcLock,
    DWORD flags,
    _Outptr_result_maybenull_ IWICBitmapLock **ppILock
    )
{
    *ppILock = this;
    AddRef();
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  IWICBitmapLock::GetDataPointer implementation.
//
//---------------------------------------------------------------------------
HRESULT SharedWicBitmap::GetDataPointer(
    _Out_ UINT *pcbBufferSize,
    _Outptr_result_buffer_all_maybenull_(*pcbBufferSize) BYTE **ppData
    )
{
    *pcbBufferSize = m_stride * m_height;
    *ppData = m_pBuffer;
    return S_OK;
}
