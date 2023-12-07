// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <PixelFormat.h>

void
CWriteableBitmap::Reset()
{
    ResetSurfaces(true /* mustKeepSoftwareSurface */, false /* mustKeepHardwareSurfaces */);
}

CWriteableBitmap::~CWriteableBitmap()
{
    Reset();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a new WriteableBitmap by specifying Width and Height
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CWriteableBitmap::Create(
    _In_reads_((nWidth * nHeight)) void* pvPixels,
    _In_ XINT32 nWidth,
    _In_ XINT32 nHeight)
{
    HRESULT hr = S_OK;
    XUINT32 nBytes = 0;

    // Validate Parameters
    if ((nWidth <= 0) || (nHeight <= 0))
    {
        IFC(E_INVALIDARG);
    }
    IFCPTR(pvPixels);

    m_nWidth = nWidth;
    m_nHeight = nHeight;
    IFC(UInt32Mult(m_nWidth, m_nHeight, &m_nLength));
    IFC(UInt32Mult(m_nLength, sizeof(XUINT32), &nBytes));

    m_pPixels = (XINT32*)pvPixels;

    IFC(InitializeSurface(m_pPixels));

Cleanup:
    if (FAILED(hr))
    {
        Reset();
    }
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Inits an instance of CWriteableBitmap
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CWriteableBitmap::InitInstance()
{
    RRETURN(ImageSurfaceWrapper::Create(GetContext(), TRUE /* mustKeepSoftwareSurface */, &m_pImageSurfaceWrapper));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called from WriteableBitmap.SetSource
//      WriteableBitmap.SetSource uses the base implementation found in
//      BitmapSource to decode a source stream into WriteableBitmap's
//      surface. This is done on the native side.
//      WriteableBitmap allows access to the pixels, hence, the surface
//      contents must be copied to a managed array and that array will be
//      used by WriteableBitmap's surface.
//      This method takes in a pointer to the managed array, initializes
//      it with the current contents of WriteableBitmap's surface and then
//      sets that pointer as the bits of the surface.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CWriteableBitmap::CopyPixels(
    _In_ void* pvPixels
    )
{
    HRESULT hr = S_OK;
    XINT32* pPixels = NULL;
    XINT32 stride = 0;
    XUINT32 nBytes = 0;
    bool surfaceLocked = false;
    IPALSurface *pOriginalSurface = NULL;

    IFCEXPECT(pvPixels);

    // Fail if the surface doesn't exist, which can happen if SetSource was called
    // but the decode failed
    IFCCHECK(m_pImageSurfaceWrapper != NULL);
    IFCCHECK(m_pImageSurfaceWrapper->GetSoftwareSurface() != NULL);

    m_pPixels = static_cast<XINT32*>(pvPixels);

    ASSERT(m_pImageSurfaceWrapper->MustKeepSystemMemory());
    SetInterface(pOriginalSurface, m_pImageSurfaceWrapper->GetSoftwareSurface());
    IFC(pOriginalSurface->Lock(
        reinterpret_cast<void**>(&pPixels),
        &stride,
        &m_nWidth,
        &m_nHeight
        ));

    IFCCATASTROPHIC(pPixels != NULL);
    surfaceLocked = TRUE;

    IFC(UInt32Mult(m_nWidth, m_nHeight, &m_nLength));
    IFC(UInt32Mult(m_nLength, sizeof(XUINT32), &nBytes));

    // Now, copy the pixels.
    memcpy(m_pPixels, pPixels, nBytes);

    Reset();

    // Release the old surface and create a new one with the new pixels.
    IFC(InitializeSurface(m_pPixels));

Cleanup:
    if (surfaceLocked)
    {
        VERIFYHR(pOriginalSurface->Unlock());
    }
    ReleaseInterface(pOriginalSurface);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Invalidates the WriteableBitmap.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CWriteableBitmap::Invalidate()
{
    if (m_nLength == 0)
    {
        return S_OK;
    }

    // Mark this object as dirty for rendering.
    CDependencyObject::NWSetRenderDirty(this, DirtyFlags::Render);
    if (GetSoftwareSurface() != NULL)
    {
        InvalidateContent();
    }

    return S_OK;
}

_Check_return_ HRESULT CWriteableBitmap::InitializeSurface(__in_xcount(m_nLength) XINT32 *pPixels)
{
    HRESULT hr = S_OK;
    CMemorySurface *pImageSurface = NULL;

    //
    // Wrap memory pointer in a surface
    //
    IFC(CMemorySurface::Create(
        pixelColor32bpp_A8R8G8B8,
        m_nWidth,
        m_nHeight,
        m_nWidth * sizeof(XUINT32),
        m_nLength * sizeof(XUINT32),
        pPixels,
        &pImageSurface
        ));

    m_pImageSurfaceWrapper->SetSoftwareSurface(pImageSurface);

    InvalidateContent();

Cleanup:
    ReleaseInterface(pImageSurface);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      When a download is available, the content is invalid
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CWriteableBitmap::OnDownloadImageAvailableImpl(
    _In_ IImageAvailableResponse* pResponse
    )
{
    IFC_RETURN(CBitmapSource::OnDownloadImageAvailableImpl(pResponse));

    InvalidateContent();

    return S_OK;
}
