// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Synopsis:
//      (ctor) - Create a new sink for hit testing geometry.
//
//------------------------------------------------------------------------
PolygonHitTestGeometrySink::PolygonHitTestGeometrySink(
    const HitTestPolygon& hitTestPolygon,
    XFLOAT tolerance,
    _In_opt_ const CMILMatrix* pTransform
    )
    : HitTestGeometrySink()
    , m_hitTestHelper(hitTestPolygon, tolerance, pTransform)
{
    m_pBaseHitTestHelper = &m_hitTestHelper;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      (dtor) - Clean up geometry sink.
//
//------------------------------------------------------------------------
PolygonHitTestGeometrySink::~PolygonHitTestGeometrySink(
    )
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get the result from hit testing.
//
//  Notes:
//      This ignores the fill mode of the geometry.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
PolygonHitTestGeometrySink::GetResult(
    _Out_ bool* pHit
    )
{
    IFC_RETURN(m_hitTestHelper.GetResult(pHit));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Intersects the hit test polygon with the geometry accumulated
//      by the sink.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
PolygonHitTestGeometrySink::GetIntersection(
    _Out_ HitTestPolygon& resultPolygon
    )
{
    RRETURN(m_hitTestHelper.GetIntersection(resultPolygon));
}
