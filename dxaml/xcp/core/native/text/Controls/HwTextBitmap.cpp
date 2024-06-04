// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "HwTextBitmap.h"
#include <SafeInt/SafeInt.hpp>

_Check_return_ HRESULT HwTextBitmap::Create(
    _In_ IWICBitmapSource* pWICBitmapSource,
    _In_ CCoreServices* pCore,
    _Out_ HwTextBitmap** ppHwTextBitmap)
{
    XUINT32 bitmapWidth = 0, bitmapHeight = 0, bitmapStride = 0, bitmapSize = 0;
    CREATEPARAMETERS cp(pCore);
    WICPixelFormatGUID pixelFormat;

    IFC_RETURN(pWICBitmapSource->GetSize(&bitmapWidth, &bitmapHeight));
    IFC_RETURN(pWICBitmapSource->GetPixelFormat(&pixelFormat));
    if (pixelFormat != GUID_WICPixelFormat32bppBGR && pixelFormat != GUID_WICPixelFormat32bppPBGRA)
    {
        // Sanity check to ensure we can assume 4 bytes per pixel
        IFC_RETURN(E_FAIL);
    }

    xref_ptr<HwTextBitmap> pHwTextBitmap;
    pHwTextBitmap.attach(new HwTextBitmap());

    IFCCHECK_RETURN(SafeMultiply(bitmapWidth, bitmapHeight, bitmapSize));
    IFCCHECK_RETURN(SafeMultiply(bitmapSize, 4, bitmapSize));
    IFCCHECK_RETURN(SafeMultiply(bitmapWidth, 4, bitmapStride));
    
    pHwTextBitmap->m_pPixelsBuffer = new(NO_FAIL_FAST) XBYTE[bitmapSize];
    IFCOOM_RETURN(pHwTextBitmap->m_pPixelsBuffer);
    IFC_RETURN(pWICBitmapSource->CopyPixels(NULL, bitmapStride, bitmapSize, pHwTextBitmap->m_pPixelsBuffer));

    // Create a WriteableBitmap that holds the image data retrieved from the WIC bitmap.
    // Later on when RichEdit calls us to draw the HwTextBitmap, we will create an ImageBrush
    // and set this WriteableBitmap as the image source.
    xref_ptr<CWriteableBitmap> pWriteableBitmap;
    IFC_RETURN(CWriteableBitmap::Create((CDependencyObject**)pWriteableBitmap.ReleaseAndGetAddressOf(), &cp));
    IFC_RETURN(pWriteableBitmap->Create(pHwTextBitmap->m_pPixelsBuffer, bitmapWidth, bitmapHeight));

    pHwTextBitmap->m_pWriteableBitmap = pWriteableBitmap.detach();
    *ppHwTextBitmap = pHwTextBitmap.detach();

    return S_OK;
}

ULONG HwTextBitmap::AddRef()
{
    return ++m_referenceCount;
}

ULONG HwTextBitmap::Release()
{
    ASSERT(m_referenceCount > 0);

    XUINT32 referenceCount = --m_referenceCount;
    if (0 == m_referenceCount)
    {
        delete this;
    }
    return referenceCount;
}

HRESULT HwTextBitmap::QueryInterface(
    REFIID riid,
    _Outptr_ void **ppvObject
    )
{
    // Do not ASSERT. We call this method to distinguish between HwTextBitmap
    // and any other implementation of ID2D1Bitmap.
    *ppvObject = NULL;
    return E_NOTIMPL;
}

void __stdcall HwTextBitmap::GetFactory(
  _Out_ ID2D1Factory **factory
) const
{
    ASSERT(FALSE);
}

HRESULT HwTextBitmap::CopyFromBitmap(
    _In_opt_ const D2D1_POINT_2U *destPoint,
    _In_ ID2D1Bitmap *bitmap,
    _In_opt_ const D2D1_RECT_U *srcRect
)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

HRESULT HwTextBitmap::CopyFromMemory(
    _In_opt_ const D2D1_RECT_U *dstRect,
    _In_ const void *srcData,
    UINT32 pitch
)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

HRESULT HwTextBitmap::CopyFromRenderTarget(
    _In_opt_ const D2D1_POINT_2U *destPoint,
    _In_ ID2D1RenderTarget *renderTarget,
    _In_opt_ const D2D1_RECT_U *srcRect
)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
}

void HwTextBitmap::GetDpi(
    _Out_ FLOAT *dpiX,
    _Out_ FLOAT *dpiY
) const
{
    ASSERT(FALSE);
}

D2D1_PIXEL_FORMAT HwTextBitmap::GetPixelFormat() const
{
    ASSERT(FALSE);
    return D2D1::PixelFormat();
}

D2D1_SIZE_U HwTextBitmap::GetPixelSize() const
{
    ASSERT(FALSE);
    return D2D1_SIZE_U();
}

D2D1_SIZE_F HwTextBitmap::GetSize() const
{
    ASSERT(FALSE);
    return D2D1_SIZE_F();
}
