// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
class HwTextBitmap final : public ID2D1Bitmap
{
public:
    //
    // IUnknown interface
    //
    virtual HRESULT __stdcall QueryInterface(
        REFIID riid,
        _Outptr_ void **ppvObject
        );
    virtual ULONG __stdcall AddRef();
    virtual ULONG __stdcall Release();

    static _Check_return_ HRESULT Create(
        _In_ IWICBitmapSource* pWICBitmapSource,
        _In_ CCoreServices* pCore,
        _Out_ HwTextBitmap** ppHwTextBitmap);

    virtual void __stdcall GetFactory(
      _Out_ ID2D1Factory **factory
    ) const;

    virtual HRESULT __stdcall CopyFromBitmap(
      _In_opt_ const D2D1_POINT_2U *destPoint,
      _In_ ID2D1Bitmap *bitmap,
      _In_opt_ const D2D1_RECT_U *srcRect
    );

    virtual HRESULT __stdcall CopyFromMemory(
      _In_opt_ const D2D1_RECT_U *dstRect,
      _In_ const void *srcData,
      UINT32 pitch
    );

    virtual HRESULT __stdcall CopyFromRenderTarget(
        _In_opt_ const D2D1_POINT_2U *destPoint,
        _In_ ID2D1RenderTarget *renderTarget,
        _In_opt_ const D2D1_RECT_U *srcRect
    );

    virtual void __stdcall GetDpi(
      _Out_ FLOAT *dpiX,
      _Out_ FLOAT *dpiY
    ) const;

    virtual D2D1_PIXEL_FORMAT __stdcall GetPixelFormat() const;

    virtual D2D1_SIZE_U __stdcall GetPixelSize() const;

    virtual D2D1_SIZE_F __stdcall GetSize() const;

    CWriteableBitmap* m_pWriteableBitmap;
    XBYTE* m_pPixelsBuffer;

protected:
    HwTextBitmap()
    {
        m_pWriteableBitmap = nullptr;
        m_referenceCount = 1;
        m_pPixelsBuffer = nullptr;
    };
    virtual ~HwTextBitmap()
    {
        ReleaseInterface(m_pWriteableBitmap);

        if (m_pPixelsBuffer != nullptr)
        {
            SAFE_DELETE_ARRAY(m_pPixelsBuffer);
        }
    };

private:
    XUINT32 m_referenceCount;
};

