// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Synopsis:
//      (ctor) - Create a new sink for hit testing geometry.
//
//------------------------------------------------------------------------
PointHitTestGeometrySink::PointHitTestGeometrySink(
    const XPOINTF& hitTestPoint,
    XFLOAT tolerance,
    _In_opt_ const CMILMatrix* pTransform
    )
    : HitTestGeometrySink()
    , m_hitTestHelper(hitTestPoint, tolerance, pTransform)
{
    m_pBaseHitTestHelper = &m_hitTestHelper;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      (dtor) - Clean up geometry sink.
//
//------------------------------------------------------------------------
PointHitTestGeometrySink::~PointHitTestGeometrySink(
    )
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get the result from hit testing.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
PointHitTestGeometrySink::GetResult(
    _Out_ bool* pHit
    )
{
    XINT32 windingNumber = 0;
    bool pointInside = false;

    IFC_RETURN(m_hitTestHelper.GetWindingNumber(&windingNumber));

    if (m_hitTestHelper.EncounteredEdgeNear())
    {
        pointInside = TRUE;
    }
    else if (m_fillMode == GeometryFillMode::Alternate)
    {
        pointInside = ((windingNumber & 1) != 0);
    }
    else
    {
        pointInside = (windingNumber != 0);
    }

    *pHit = pointInside;

    return S_OK;
}
