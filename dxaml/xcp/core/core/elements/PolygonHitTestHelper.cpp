// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Synopsis:
//      (ctor) - Create a new polygon hit test helper.
//
//------------------------------------------------------------------------
PolygonHitTestHelper::PolygonHitTestHelper(
    const HitTestPolygon& hitTestPolygon,
    XFLOAT tolerance,
    _In_opt_ const CMILMatrix* pTransform
    )
    : HitTestHelper(tolerance, pTransform)
    , m_hitTestPolygon(hitTestPolygon)
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      (dtor) - Clean up the polygon hit test helper.
//
//------------------------------------------------------------------------
PolygonHitTestHelper::~PolygonHitTestHelper(
    )
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handle a new point.
//
//------------------------------------------------------------------------
void
PolygonHitTestHelper::AcceptPoint(
    _In_ const XPOINTF& endPoint
    )
{
    CheckForNaN(endPoint);

    VERIFYHR(m_points.push_back(endPoint));

    //
    // Update next start point.
    //
    m_currentPoint = endPoint;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get the hit test result.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
PolygonHitTestHelper::GetResult(bool *pWasHit)
{
    IFCEXPECT_RETURN(!m_encounteredNaN);

    if (m_points.empty())
    {
        *pWasHit = FALSE;
    }
    else
    {
        EnsureCounterClockwiseWindingOrder(m_points.size(), &m_points[0]);

        if (IsPolygonConcave(m_points.size(), &m_points[0]))
        {
            // Hit the bounding rectangle of the polygon if it is concave.

            XRECTF bounds = {};
            BoundPoints(&m_points[0], m_points.size(), &bounds);
            *pWasHit = m_hitTestPolygon.IntersectsRect(bounds);
        }
        else
        {
            IFC_RETURN(m_hitTestPolygon.IntersectsPolygon(m_points.size(), &m_points[0], pWasHit));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Reset state.
//
//------------------------------------------------------------------------
void
PolygonHitTestHelper::Reset()
{
    m_points.clear();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get the intersection of the hit test polygon and the geometry.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
PolygonHitTestHelper::GetIntersection(
    _Out_ HitTestPolygon& resultPolygon
    )
{
    m_hitTestPolygon.CopyTo(&resultPolygon);

    if (!IsPolygonConcave(m_points.size(), &m_points[0]))
    {
        // Note: If the geometry is concave, we won't modify the incoming polygon.
        // The hit testing logic in CUIElement will still clip to the element
        // bounds as a first approximation.
        IFC_RETURN(resultPolygon.ClipToPolygon(m_points.size(), &m_points[0]));
    }

    return S_OK;
}
