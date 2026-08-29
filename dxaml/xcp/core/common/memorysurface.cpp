// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <PixelFormat.h>

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a CMemorySurface instance
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CMemorySurface::Create(
    PixelFormat pixelFormat,
    XUINT32 nWidth,
    XUINT32 nHeight,
    XINT32  nStride,
    XUINT32 cAddressSize,
    _In_reads_bytes_(cAddressSize) void *pAddress,
    _Outptr_ CMemorySurface **ppMemorySurface
    )
{
    IFCPTR_RETURN(pAddress);
    IFCPTR_RETURN(ppMemorySurface);

    ASSERT(pixelFormat == pixelGray8bpp
        || pixelFormat == pixelColor32bpp_A8R8G8B8);

    // Make sure that the stride is sufficient to store a row and that
    // the allocation specified will be sufficient for all the rows...
    IFC_RETURN(ValidateSurfaceBufferSize(
        pixelFormat,
        nWidth,
        nHeight,
        nStride,
        cAddressSize));

    xref_ptr<CMemorySurface> pMemorySurface;
    pMemorySurface.attach(new CMemorySurface());

    pMemorySurface->m_pixelFormat = pixelFormat;
    pMemorySurface->m_pBits   = pAddress;
    pMemorySurface->m_nStride = nStride;
    pMemorySurface->m_nHeight = nHeight;
    pMemorySurface->m_nWidth  = nWidth;

    *ppMemorySurface = pMemorySurface.detach();

    return S_OK;
}

CMemorySurface::CMemorySurface()
    : m_fIsOpaque(FALSE)
    , m_pixelFormat(pixelColor32bpp_A8R8G8B8)
{
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Locks the inner surface
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CMemorySurface::Lock(
    _Outptr_result_bytebuffer_(*pnStride * *puHeight) void **ppAddress,
    _Out_ XINT32 *pnStride,
    _Out_ XUINT32 *puWidth,
    _Out_ XUINT32 *puHeight)
{
    IFCPTR_RETURN(ppAddress);
    IFCPTR_RETURN(pnStride);
    IFCPTR_RETURN(puWidth);
    IFCPTR_RETURN(puHeight);

    *ppAddress = m_pBits;
    *pnStride = m_nStride;
    *puWidth = m_nWidth;
    *puHeight = m_nHeight;

    return S_OK;
}

_Check_return_ HRESULT CMemorySurface::Unlock()
{
    return S_OK;
}
