// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      SharedWicBitmap wraps provided memory buffer into IWICBitmap.

#pragma once

//---------------------------------------------------------------------------
//
//  SharedWicBitmap
//
//  SharedWicBitmap wraps provided memory buffer into IWICBitmap.
//
//---------------------------------------------------------------------------
class SharedWicBitmap final : public IWICBitmap, public IWICBitmapLock
{
public:

    // Creates and initializes a new instance of the SharedWicBitmap class with
    // either an externally owned memory buffer or an internally allocated buffer
    // if none is passed.
    static HRESULT Create(
        _In_ XUINT32 width,
        _In_ XUINT32 height,
        _In_ XUINT32 stride,
        _In_ WICPixelFormatGUID const& pixelFormat,
        _In_opt_ XUINT8 *pBuffer, // Explicit buffer, otherwise allocated internally
        _Outptr_ SharedWicBitmap **ppWICBitmap
        );

    // Makes the internal buffer shareable and detaches ownership
    void MakeBufferSharable();

    // Provided access to the internal buffer. Does not affect ownership.
    void GetBuffer(
        _Out_ XUINT32 *pWidth,
        _Out_ XUINT32 *pHeight,
        _Out_ XUINT32 *pStride,
        _Outptr_result_buffer_((*pStride) * (*pHeight)) XUINT8 **pBuffer
        );

    //
    // IWICBitmap and IWICBitmapLock interface
    //

    virtual HRESULT __stdcall QueryInterface(
        _In_ REFIID riid,
        _Outptr_ void **ppvObject
        );
    virtual ULONG __stdcall AddRef();
    virtual ULONG __stdcall Release();
    virtual HRESULT __stdcall GetSize(
        _Out_ UINT *pWidth,
        _Out_ UINT *pHeight
        );
    virtual HRESULT __stdcall GetStride(
        _Out_ UINT *pStride
        );
    virtual HRESULT __stdcall GetPixelFormat(
        _Out_ WICPixelFormatGUID *pPixelFormat
        );
    virtual HRESULT __stdcall GetResolution(
        _Out_ double *pDpiX,
        _Out_ double *pDpiY
        );
    virtual HRESULT __stdcall SetResolution(
        double dpiX,
        double dpiY
        );
    virtual HRESULT __stdcall CopyPalette(
        _In_opt_ IWICPalette *pIPalette
        );
    virtual HRESULT __stdcall SetPalette(
        _In_opt_ IWICPalette *pIPalette
        );
    virtual HRESULT __stdcall CopyPixels(
        _In_opt_ const WICRect *prc,
        UINT cbStride,
        UINT cbBufferSize,
        _Out_writes_all_(cbBufferSize) BYTE *pbBuffer
        );
    virtual HRESULT __stdcall Lock(
        _In_opt_ const WICRect *prcLock,
        DWORD flags,
        _Outptr_result_maybenull_ IWICBitmapLock **ppILock
        );
    virtual HRESULT __stdcall GetDataPointer(
        _Out_ UINT *pcbBufferSize,
        _Outptr_result_buffer_all_maybenull_(*pcbBufferSize) BYTE **ppData
        );

private:
    XUINT32 m_refCount;
    XUINT32 m_width;
    XUINT32 m_height;
    XUINT32 m_stride;
    WICPixelFormatGUID m_pixelFormat;
    XUINT8 *m_pBuffer;
    bool m_usesSharedMemory;

    // Private ctor and dctor.
    SharedWicBitmap(
        _In_ XUINT32 width,
        _In_ XUINT32 height,
        _In_ XUINT32 stride,
        _In_ WICPixelFormatGUID pixelFormat,
        _In_ XUINT8 *pBuffer,
        _In_ bool usesSharedMemory
        );
    ~SharedWicBitmap();
};
