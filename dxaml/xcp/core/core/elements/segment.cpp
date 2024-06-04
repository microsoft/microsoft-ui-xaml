// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

extern void
GenerateBezierFromQuadratic(
    _In_reads_(3) const XPOINTF *pptQuadratic,
    _Out_writes_(4) XPOINTF *pptCubic
    );

extern void
UpdateBounds(
    _Inout_ XRECTF_RB *prcLocalSpaceBounds,          // The local space bounds
    _In_reads_(cPoints) const XPOINTF *pPtsSource,  // Source points
    _In_ XINT32 cPoints                              // Count of points
    );

extern void
UpdateBezierBounds(
    _Inout_ XRECTF_RB *prcLocalSpaceBounds,          // The local space bounds
    _In_reads_(cPoints) const XPOINTF *pPtsSource,  // Source points
    _In_range_(4,4) XINT32 cPoints                   // Count of points
    );

//-----------------------------------------------------------------------------
//
//  Function:  StartBounds
//
//  Synopsis:
//      Start bounding box based on input point
//
//-----------------------------------------------------------------------------
void
StartBounds(
    _Out_ XRECTF_RB* pBounds,          // The local space bounds
    _In_ const XPOINTF* pStartPoint    // Source points
    )
{
    pBounds->left = pStartPoint->x;
    pBounds->right = pStartPoint->x;

    pBounds->top = pStartPoint->y;
    pBounds->bottom = pStartPoint->y;
}

//------------------------------------------------------------------------
//
//  Method:   Create
//
//  Synopsis:
//      Creates an instance of the object.  This is an abstract class that
// can't get created but we need it to have a unique creation method for
// derived type matching.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CPathSegment::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
)
{
    CPathSegment *_this;

// There is only one kind of path segment.

    _this = new CPathSegment(pCreate->m_pCore);
    delete _this;

    return E_UNEXPECTED;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Builds D2D geometries.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CPathSegment::AddAcceleratedSegment(
    _In_ IPALGeometrySink *pPALGeometrySink
    )
{
    RRETURN(E_NOTIMPL);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Builds D2D geometries.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CLineSegment::AddAcceleratedSegment(
    _In_ IPALGeometrySink *pPALGeometrySink
    )
{
    pPALGeometrySink->AddLine(m_pt);
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Builds D2D geometries.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CBezierSegment::AddAcceleratedSegment(
    _In_ IPALGeometrySink *pPALGeometrySink
    )
{
    pPALGeometrySink->AddBezier(
        m_ptOne,
        m_ptTwo,
        m_ptThree
        );
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Builds D2D geometries.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CQuadraticSegment::AddAcceleratedSegment(
    _In_ IPALGeometrySink *pPALGeometrySink
    )
{
    pPALGeometrySink->AddQuadraticBezier(
        m_ptOne,
        m_ptTwo
        );
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Builds D2D geometries.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CArcSegment::AddAcceleratedSegment(
    _In_ IPALGeometrySink *pPALGeometrySink
    )
{
    pPALGeometrySink->AddArc(
        m_ptBase,
        m_size,
        m_eAngle,
        m_bClockwise,
        m_bLarge
        );
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Method:   dtor
//
//  Synopsis:
//      Destructor for a CPolyLineSegment object
//
//------------------------------------------------------------------------

CPolyLineSegment::~CPolyLineSegment()
{
    ReleaseInterface(m_pPoints);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Builds D2D geometries.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CPolyLineSegment::AddAcceleratedSegment(
    _In_ IPALGeometrySink *pPALGeometrySink
    )
{
    if (m_pPoints && m_pPoints->GetCount() > 0)
    {
        pPALGeometrySink->AddLines(
            m_pPoints->GetCollection().data(),
            m_pPoints->GetCount()
            );
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   dtor
//
//  Synopsis:
//      Destructor for a CPolyBezierSegment object
//
//------------------------------------------------------------------------

CPolyBezierSegment::~CPolyBezierSegment()
{
    ReleaseInterface(m_pPoints);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Builds D2D geometries.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CPolyBezierSegment::AddAcceleratedSegment(
    _In_ IPALGeometrySink *pPALGeometrySink
    )
{
    if (m_pPoints && m_pPoints->GetCount() > 0)
    {
        pPALGeometrySink->AddBeziers(
            m_pPoints->GetCollection().data(),
            m_pPoints->GetCount()
            );
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   dtor
//
//  Synopsis:
//      Destructor for a PolyQuadraticSegment object
//
//------------------------------------------------------------------------

CPolyQuadraticSegment::~CPolyQuadraticSegment()
{
    ReleaseInterface(m_pPoints);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Builds D2D geometries.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CPolyQuadraticSegment::AddAcceleratedSegment(
    _In_ IPALGeometrySink *pPALGeometrySink
    )
{
    if (m_pPoints && m_pPoints->GetCount() > 0)
    {
        pPALGeometrySink->AddQuadraticBeziers(
            m_pPoints->GetCollection().data(),
            m_pPoints->GetCount()
            );
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Builds D2D geometries.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CPathSegmentCollection::AddAcceleratedSegments(
    _In_ IPALGeometrySink *pPALGeometrySink
    )
{
    if (GetCount() > 0)
    {
        for (auto& segment : GetCollection())
        {
            IFC_RETURN(static_cast<CPathSegment*>(segment)->AddAcceleratedSegment(pPALGeometrySink));
        }
    }

    return S_OK;
}
