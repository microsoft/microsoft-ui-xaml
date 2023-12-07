// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "HitTestPolygon.h"

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets a rect.
//
//------------------------------------------------------------------------
void HitTestPolygon::SetRect(_In_ const XRECTF& rect)
{
    m_rect = rect;
    UpdateStateFromRect();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets an arbitrary polygon.
//
//------------------------------------------------------------------------
void HitTestPolygon::SetPoints(
    XUINT32 numPoints,
    _In_reads_(numPoints) const XPOINTF *pPoints)
{
    m_points.resize(numPoints);

    if (numPoints > 0)
    {
        memcpy(&m_points[0], pPoints, sizeof(XPOINTF) * numPoints);
    }

    const bool wasTransformed = UpdateStateFromPoints();
    ASSERT(wasTransformed); // We shouldn't be getting passed concave points
}

void HitTestPolygon::SetEmpty()
{
    m_points.clear();
    m_3DPoints.clear();
    m_isRect = false;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Copies this polygon to another polygon.
//
//------------------------------------------------------------------------
void HitTestPolygon::CopyTo(_In_ HitTestPolygon *pDest) const
{
    ASSERT(!m_is3D);

    if (IsEmpty())
    {
        pDest->SetEmpty();
    }
    else
    {
        pDest->SetPoints(static_cast<XUINT32>(m_points.size()), &m_points[0]);
    }
}

void HitTestPolygon::Ensure3DMode()
{
    if (!m_is3D)
    {
        m_3DPoints.reserve(m_points.size());
        for (const auto& point : m_points)
        {
            m_3DPoints.push_back({ point.x, point.y, 0, 1 });
        }

        m_is3D = true;
    }
}

bool HitTestPolygon::UpdateStateFromPoints()
{
    bool transformedTarget = true;

    if (m_is3D)
    {
        if (m_3DPoints.size() > 0)
        {
            EnsureCounterClockwiseWindingOrder(m_3DPoints);
            transformedTarget = !IsPolygonConcave(m_3DPoints);
        }

        m_isRect = false;
    }
    else
    {
        if (m_points.size() > 0)
        {
            EnsureCounterClockwiseWindingOrder(m_points);
            transformedTarget = !IsPolygonConcave(m_points);
        }

        m_isRect = FillRectFromPointsCCW(m_points, m_rect);
    }

    return transformedTarget;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Updates state after rect change.
//
//------------------------------------------------------------------------
void HitTestPolygon::UpdateStateFromRect()
{
    m_points.resize(4);

    FillPointsFromRectCCW(&m_points[0], m_rect);
    m_isRect = true;
    m_is3D = false;
}

void HitTestPolygon::Transform(const CMILMatrix& transform)
{
    if (!IsEmpty())
    {
        if (m_is3D)
        {
            transform.Transform3DPoints_PreserveW(m_3DPoints);
        }
        else
        {
            transform.Transform(m_points);
        }

        UpdateStateFromPoints();
    }
}

void HitTestPolygon::Transform(const CMILMatrix4x4& transform)
{
    if (!IsEmpty())
    {
        if (transform.Is2D())
        {
            Transform(transform.Get2DRepresentation());
        }
        else
        {
            Ensure3DMode();
            transform.Transform_PreserveW(m_3DPoints, m_3DPoints);
            UpdateStateFromPoints();
        }
    }
}

bool HitTestPolygon::TransformWorldToLocalWithInverse(const CMILMatrix4x4& worldTransform)
{
    // This method goes from a 2D polygon, through a 3D transform, and results in a 2D polygon.
    ASSERT(!m_is3D);

    bool wasTransformed = worldTransform.TransformWorldToLocalWithInverse(m_points, m_points);
    if (wasTransformed)
    {
        wasTransformed = UpdateStateFromPoints();
    }

    return wasTransformed;
}

// TODO: HitTest: pick one between WithInverse and WithInterpolation. We shouldn't need to call both.
// This doesn't rely on an invertible matrix.
bool HitTestPolygon::TransformWorldToLocalWithInterpolation(const CMILMatrix4x4& worldTransform)
{
    // This method goes from a 2D polygon, through a 3D transform, and results in a 2D polygon.
    ASSERT(!m_is3D);

    bool wasTransformed = worldTransform.TransformWorldToLocalWithInterpolation(m_points, m_points);
    if (wasTransformed)
    {
        wasTransformed = UpdateStateFromPoints();
    }

    return wasTransformed;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether or not this polygon intersects the given polygon.
//
//  Notes:
//      Assumes the input has CCW winding order.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT HitTestPolygon::IntersectsPolygon(
    XUINT32 numPoints,
    _In_reads_(numPoints) const XPOINTF *pPoints,
    _Out_ bool *pIntersects) const
{
    ASSERT(!m_is3D);

    const gsl::span<const XPOINTF> pointsSpan(pPoints, numPoints);
    if (IsPolygonConcave(pointsSpan))
    {
        ASSERT(FALSE);
        IFC_RETURN(E_INVALIDARG);
    }

    *pIntersects = !IsEmpty() ? DoPolygonsIntersect(static_cast<XUINT32>(m_points.size()), &m_points[0], numPoints, pPoints) : FALSE;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether or not polygon intersects the given rect.
//
//------------------------------------------------------------------------
bool HitTestPolygon::IntersectsRect(_In_ const XRECTF& rect) const
{
    ASSERT(!m_is3D);

    bool intersects = false;

    if (m_isRect)
    {
        intersects = DoRectsIntersectInclusive(m_rect, rect);
    }
    else
    {
        XPOINTF rectPoints[4];
        FillPointsFromRectCCW(rectPoints, rect);

        IGNOREHR(IntersectsPolygon(4, rectPoints, &intersects));
    }

    return intersects;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether or not this polygon intersects the given rect.
//
//------------------------------------------------------------------------
bool HitTestPolygon::IntersectsRect(_In_ const XRECTF_RB& rectRB) const
{
    ASSERT(!m_is3D);

    return IntersectsRect(ToXRectF(rectRB));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Clips to the given polygon.
//
//  Notes:
//      Assumes the input has CCW winding order.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT HitTestPolygon::ClipToPolygon(
    XUINT32 numPoints,
    _In_reads_(numPoints) const XPOINTF *pPoints)
{
    ASSERT(!m_is3D);

    xvector<XPOINTF> resultPoints;
    XUINT32 numResultPoints;

    const gsl::span<const XPOINTF> pointsSpan(pPoints, numPoints);
    if (IsPolygonConcave(pointsSpan))
    {
        ASSERT(FALSE);
        IFC_RETURN(E_INVALIDARG);
    }

    numResultPoints = static_cast<XUINT32>(m_points.size() + numPoints);
    IFC_RETURN(resultPoints.resize(numResultPoints));

    IFC_RETURN(::ClipToPolygon(
        static_cast<XUINT32>(m_points.size()),
        &m_points[0],
        numPoints,
        pPoints,
        &numResultPoints,
        &resultPoints[0]));

    SetPoints(numResultPoints, &resultPoints[0]);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Clips to the given rect.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT HitTestPolygon::ClipToRect(const XRECTF& rect)
{
    XPOINTF rectPoints[4];

    if (m_isRect)
    {
        ASSERT(!m_is3D);

        if (IntersectRect(&m_rect, &rect))
        {
            UpdateStateFromRect();
        }
        else
        {
            SetEmpty();
        }
    }
    else
    {
        if (m_is3D)
        {
            // Clipping a 3D geometry with a 2D rect isn't supported. We have to take this polygon to world space, and
            // take the clip rect to world space, then do an intersection there.
        }
        else
        {
            FillPointsFromRectCCW(rectPoints, rect);
            IFC_RETURN(ClipToPolygon(4, rectPoints));
        }
    }

    return S_OK;
}

XRECTF_RB HitTestPolygon::GetPolygonBounds() const
{
    if (m_is3D)
    {
        return BoundPoints_RB(m_3DPoints);
    }
    else
    {
        return BoundPoints_RB(m_points);
    }
}
