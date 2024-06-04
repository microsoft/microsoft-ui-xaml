// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Synopsis:
//      Wrapper around GDI HRGN
//
//------------------------------------------------------------------------
class WinRegion : public CXcpObjectBase< IPALRegion >
{
public:
    WinRegion();
    ~WinRegion() override;

    _Check_return_ HRESULT Initialize();
    _Check_return_ HRESULT Add(_In_ const XRECT *pRect) override;
    _Check_return_ HRESULT Remove(_In_ const XRECT *pRect) override;
    _Check_return_ HRESULT Intersect(_In_ const XRECT *pRect) override;
    _Check_return_ HRESULT GetRects(_Outptr_result_buffer_(*pRectCount) XRECT **ppRects, _Out_ XUINT32 *pRectCount) override;
    _Check_return_ HRESULT GetInverseRects(_Inout_ xvector<XRECT> *pArray, _In_ const XRECT_RB *pBounds) override;
    _Check_return_ HRESULT IsSubsetOf(_In_ IPALRegion *pOtherRegion, _Out_ bool *pIsSubset) override;
    _Check_return_ HRESULT Contains(_In_ const XRECT *pRect, _Out_ bool *pContainsRect) override;

private:
    _Check_return_ HRESULT Combine(_In_ const XRECT *pRect, XINT32 combineMode);

    HRGN m_rgn;
};

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a region object.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CWindowsServices::CreateRegion(_Outptr_ IPALRegion **ppRegion)
{
    HRESULT hr = S_OK;
    *ppRegion = NULL;

    WinRegion *pWinRegion = new WinRegion;

    IFC(pWinRegion->Initialize());

    *ppRegion = pWinRegion;
    pWinRegion = NULL;

Cleanup:
    ReleaseInterface(pWinRegion);
    RRETURN(hr);
}

WinRegion::WinRegion()
{
    m_rgn = NULL;
}

WinRegion::~WinRegion()
{
    if (m_rgn)
    {
        DeleteObject(m_rgn);
        m_rgn = NULL;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Should only be called once.  Initializes the GDI region object.
//      A region starts out empty.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
WinRegion::Initialize()
{
    ASSERT(m_rgn == NULL);

    m_rgn = CreateRectRgn(0, 0, 0, 0);
    IFCOOMFAILFAST(m_rgn);

    return S_OK;
    // RRETURN(hr);// RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Computes the union of a rectangle and the region
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
WinRegion::Add(_In_ const XRECT *pRect)
{
    return Combine(pRect, RGN_OR);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Subtracts a rectangle from the region
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
WinRegion::Remove(_In_ const XRECT *pRect)
{
    return Combine(pRect, RGN_DIFF);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Computes the intersection of a rectangle and the region
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
WinRegion::Intersect(_In_ const XRECT *pRect)
{
    return Combine(pRect, RGN_AND);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the the region as an array of rectangles.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
WinRegion::GetRects(
    _Outptr_result_buffer_(*pRectCount) XRECT **ppRects,
    _Out_ XUINT32 *pRectCount
    )
{
    HRESULT hr = S_OK;

    ASSERT(m_rgn); // Initialize() should have been called

    XRECT *pRects = NULL;
    XUINT32 rectCount = 0;

    // Extract the rectangles from the region.
    XBYTE *pBuffer = NULL;
    XUINT32 requiredSize = GetRegionData(m_rgn, 0, NULL);

    if (requiredSize > 0)
    {
        pBuffer = new BYTE[requiredSize];

        RGNDATA *pRgnData = reinterpret_cast<RGNDATA *>(pBuffer);

        XUINT32 result = GetRegionData(m_rgn, requiredSize, pRgnData);
        IFCCHECK(result == requiredSize);

        RECT *pRgnRects = reinterpret_cast<RECT *>(pRgnData->Buffer);

        rectCount = pRgnData->rdh.nCount;
        pRects = new XRECT[rectCount];

        for (XUINT32 i = 0; i < rectCount; i++)
        {
            pRects[i].X = pRgnRects[i].left;
            pRects[i].Y = pRgnRects[i].top;
            pRects[i].Width = pRgnRects[i].right - pRgnRects[i].left;
            pRects[i].Height = pRgnRects[i].bottom - pRgnRects[i].top;
        }
    }

    *ppRects = pRects;
    pRects = NULL;

    *pRectCount = rectCount;

Cleanup:
    SAFE_DELETE_ARRAY(pRects);
    SAFE_DELETE_ARRAY(pBuffer);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the inverse of the region (clipped to specified bounds) as an array of rectangles.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
WinRegion::GetInverseRects(
    _Inout_ xvector<XRECT> *pArray,
    _In_ const XRECT_RB *pBounds
    )
{
    ASSERT(m_rgn); // Initialize() should have been called

    HRESULT hr = S_OK;
    HRGN resultRgn = NULL;
    HRGN rectRgn = NULL;
    XINT32 returnCode = 0;
    XUINT32 requiredSize = 0;
    XBYTE *pBuffer = NULL;
    RGNDATA *pRgnData = NULL;
    RECT *pRects = NULL;

    pArray->clear();

    // Create an empty region to hold the inverse
    resultRgn = CreateRectRgn(0, 0, 0, 0);
    IFCOOMFAILFAST(resultRgn);

    // Wrap pBounds in a GDI region
    rectRgn = CreateRectRgn(
        pBounds->left,
        pBounds->top,
        pBounds->right,
        pBounds->bottom
        );
    IFCOOMFAILFAST(rectRgn);

    // Compute the difference between the bounds and this region
    returnCode = CombineRgn(
        resultRgn,
        rectRgn,
        m_rgn,
        RGN_DIFF
        );
    IFCCATASTROPHIC(ERROR != returnCode);

    // Extract the rectangles
    requiredSize = GetRegionData(resultRgn, 0, NULL);
    if (requiredSize > 0)
    {
        pBuffer = new BYTE[requiredSize];

        pRgnData = reinterpret_cast<RGNDATA *>(pBuffer);

        XUINT32 result = GetRegionData(resultRgn, requiredSize, pRgnData);
        IFCEXPECT(result == requiredSize);

        pRects = reinterpret_cast<RECT *>(pRgnData->Buffer);

        for (XUINT32 i = 0; i < pRgnData->rdh.nCount; i++)
        {
            XRECT rect =
            {
                pRects[i].left,
                pRects[i].top,
                pRects[i].right - pRects[i].left,
                pRects[i].bottom - pRects[i].top
            };

            IFC(pArray->push_back(rect));
        }
    }

Cleanup:
    delete[] pBuffer;

    if (rectRgn)
    {
        DeleteObject(rectRgn);
    }

    if (resultRgn)
    {
        DeleteObject(resultRgn);
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Internal method that combines the current region and a rectangle with a given operator
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
WinRegion::Combine(
    _In_ const XRECT *pRect,
    XINT32 combineMode
    )
{
    ASSERT(m_rgn); // Initialize() should have been called

    HRESULT hr = S_OK;
    HRGN rectRgn = NULL;
    XINT32 returnCode = 0;

    // Wrap the input rectangle in a GDI HRGN object
    rectRgn = CreateRectRgn(
        pRect->X,
        pRect->Y,
        pRect->X + pRect->Width,
        pRect->Y + pRect->Height
        );
    IFCOOMFAILFAST(rectRgn);

    // Do the operation
    // Note that it is important that m_rgn be Src1 and not Src2
    // because RGN_DIFF is not symmetric.
    returnCode = CombineRgn(
        m_rgn,   // Dst
        m_rgn,   // Src1
        rectRgn, // Src2
        combineMode
        );
    IFCCATASTROPHIC(ERROR != returnCode);

Cleanup:
    if (rectRgn)
    {
        DeleteObject(rectRgn);
        rectRgn = NULL;
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Determines if this region is a subset of pIOtherRegion
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
WinRegion::IsSubsetOf(
    _In_ IPALRegion *pIOtherRegion,
    _Out_ bool *pIsSubset
    )
{
    HRESULT hr = S_OK;
    HRGN diffRgn = NULL;
    XINT32 returnCode;
    *pIsSubset = FALSE;

    WinRegion *pOtherRegion = static_cast<WinRegion *>(pIOtherRegion);

    // Create a region object to hold the difference
    diffRgn = CreateRectRgn(0, 0, 0, 0);
    IFCOOMFAILFAST(diffRgn);

    // set diffRgn = points that are in this region that are not in pOtherRegion
    returnCode = CombineRgn(
        diffRgn,
        m_rgn,
        pOtherRegion->m_rgn,
        RGN_DIFF
        );
    IFCCATASTROPHIC(ERROR != returnCode);

    // This region is a subset of pOtherRegion if there are no points in the difference
    *pIsSubset = (NULLREGION == returnCode);

Cleanup:
    if (diffRgn)
    {
        DeleteObject(diffRgn);
        diffRgn = NULL;
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Determines if this region completely contains a rectangle
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
WinRegion::Contains(
    _In_ const XRECT *pRect,
    _Out_ bool *pContainsRect
    )
{
    HRESULT hr = S_OK;
    HRGN rectRgn = NULL;
    XINT32 returnCode;

    *pContainsRect = FALSE;

    // Wrap the input rectangle in a GDI HRGN object
    rectRgn = CreateRectRgn(
        pRect->X,
        pRect->Y,
        pRect->X + pRect->Width,
        pRect->Y + pRect->Height
        );
    IFCOOMFAILFAST(rectRgn);

    // Compute the set of points that are inside *pRect and not inside of m_rgn
    returnCode = CombineRgn(
        rectRgn,
        rectRgn,
        m_rgn,
        RGN_DIFF
        );
    IFCCATASTROPHIC(ERROR != returnCode);

    // If there are no points that are inside the rect and not inside of m_rgn,
    // then m_rgn completely contains the rect.
    *pContainsRect = (NULLREGION == returnCode);

Cleanup:
    if (rectRgn)
    {
        DeleteObject(rectRgn);
        rectRgn = NULL;
    }

    RRETURN(hr);
}
