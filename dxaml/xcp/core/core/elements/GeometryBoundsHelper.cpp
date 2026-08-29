// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

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

extern void
StartBounds(
    _Out_ XRECTF_RB* pBounds,          // The local space bounds
    _In_ const XPOINTF* pStartPoint    // Source points
    );

//------------------------------------------------------------------------
//
//  Synopsis:
//      (ctor) - Create a new geometry bounds helper.
//
//------------------------------------------------------------------------
GeometryBoundsHelper::GeometryBoundsHelper(
    )
    : m_hasFirstPoint(FALSE)
{
    EmptyRectF(&m_bounds);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      (dtor) - Clean up the geometry bounds helper.
//
//------------------------------------------------------------------------
GeometryBoundsHelper::~GeometryBoundsHelper(
    )
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Set the current start point.
//
//------------------------------------------------------------------------
void 
GeometryBoundsHelper::StartAt(
    _In_ const XPOINTF& firstPoint
    )
{
    AcceptPoint(firstPoint);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handle a new line segment.
//
//------------------------------------------------------------------------
void 
GeometryBoundsHelper::DoLine(
    _In_ const XPOINTF& endPoint
    )
{
    AcceptPoint(endPoint);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handle a new Bezier curve.
//
//------------------------------------------------------------------------
void
GeometryBoundsHelper::DoBezier(
    _In_ const XPOINTF& controlPoint1,
    _In_ const XPOINTF& controlPoint2,
    _In_ const XPOINTF& endPoint
    )
{
    XPOINTF bezierPoints[4] = 
    {
        m_currentPoint,
        controlPoint1,
        controlPoint2,
        endPoint
    };

    UpdateBezierBounds(&m_bounds, bezierPoints, ARRAY_SIZE(bezierPoints));

    m_currentPoint = endPoint;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handle a new quadratic Bezier curve.
//
//------------------------------------------------------------------------
void
GeometryBoundsHelper::DoQuadraticBezier(
    _In_ const XPOINTF& controlPoint1,
    _In_ const XPOINTF& endPoint
    )
{
    XPOINTF bezierPoints[4];

    //
    // By the degree-elevation formula for Bezier curves the cubic Bezier 
    // points of this quadratic Bezier curve are:
    //      pt0
    //      (1 / 3) * pt0 + (2 / 3) * pt1
    //      (2 / 3) * pt1 + (1 / 3) * pt2
    //      pt2
    //

    bezierPoints[0].x = m_currentPoint.x;
    bezierPoints[0].y = m_currentPoint.y;
    bezierPoints[1].x = (1 / 3.0f) * m_currentPoint.x + (2 / 3.0f) * controlPoint1.x;
    bezierPoints[1].y = (1 / 3.0f) * m_currentPoint.y + (2 / 3.0f) * controlPoint1.y;
    bezierPoints[2].x = (2 / 3.0f) * controlPoint1.x + (1 / 3.0f) * endPoint.x;
    bezierPoints[2].y = (2 / 3.0f) * controlPoint1.y + (1 / 3.0f) * endPoint.y;
    bezierPoints[3].x = endPoint.x;
    bezierPoints[3].y = endPoint.y;

    UpdateBezierBounds(&m_bounds, bezierPoints, ARRAY_SIZE(bezierPoints));

    m_currentPoint = endPoint;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handle a new arc.
//
//------------------------------------------------------------------------
void
GeometryBoundsHelper::DoArc(
    const XPOINTF& point,
    const XSIZEF& size,
    XFLOAT rotationAngle,
    bool fIsClockwise,
    bool fIsLargeArc
    )
{
    //
    // Turn the arc into up to 4 Beziers.
    //
    XPOINTF bezierPoints[12];
    XINT32 curveCount = 0;

    ArcToBezier(m_currentPoint.x, m_currentPoint.y, size.width, size.height, rotationAngle, fIsLargeArc, fIsClockwise, point.x, point.y, bezierPoints, &curveCount);
    
    if (curveCount > 0)
    {
        XPOINTF* pBezierPoints = bezierPoints;

        //
        // Handle each Bezier curve from the arc.
        //
        for (XINT32 i = 0; i < curveCount; ++i)
        {
            DoBezier(pBezierPoints[0], pBezierPoints[1], pBezierPoints[2]);

            pBezierPoints += 3;
        }
    }
    else
    {
        //
        // The arc is a line as it has no curves.
        //
        DoLine(point);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handle a new point.
//
//------------------------------------------------------------------------
void 
GeometryBoundsHelper::AcceptPoint(
    _In_ const XPOINTF& endPoint
    )
{
    if (!m_hasFirstPoint)
    {
        m_hasFirstPoint = TRUE;
        
        StartBounds(&m_bounds, &endPoint);
    }
    else
    {
        UpdateBounds(&m_bounds, &endPoint, 1);
    }

    //
    // Update next start point.
    //
    m_currentPoint = endPoint;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handle a new point.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT 
GeometryBoundsHelper::GetBounds(
    _Out_ XRECTF_RB* pBounds
    )
{
    HRESULT hr = S_OK;

    *pBounds = m_bounds;

    RRETURN(hr);
}
