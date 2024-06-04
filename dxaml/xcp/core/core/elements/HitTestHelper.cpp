// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Synopsis:
//      (ctor) - Create a new hit test helper.
//
//------------------------------------------------------------------------
HitTestHelper::HitTestHelper(
    XFLOAT tolerance,
    _In_opt_ const CMILMatrix* pTransform
    )
    : m_tolerance(tolerance)
    , m_encounteredNaN(FALSE)
    , m_transform(pTransform ? *pTransform : CMILMatrix(true /*initialize*/))
{
    m_currentPoint.x = 0;
    m_currentPoint.y = 0;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      (dtor) - Clean up the hit test helper.
//
//------------------------------------------------------------------------
HitTestHelper::~HitTestHelper(
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
HitTestHelper::StartAt(
    _In_ const XPOINTF& firstPoint
    )
{
    XPOINTF transformedPoint;

    CheckForNaN(firstPoint);

    m_transform.Transform(&firstPoint, &transformedPoint, 1);

    m_currentPoint = transformedPoint;
    m_currentPointUntransformed = firstPoint;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handle a new line segment.
//
//------------------------------------------------------------------------
void
HitTestHelper::DoLine(
    _In_ const XPOINTF& endPoint
    )
{
    XPOINTF transformedPoint;

    m_transform.Transform(&endPoint, &transformedPoint, 1);

    AcceptPoint(transformedPoint);

    m_currentPointUntransformed = endPoint;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handle a new Bezier curve.
//
//------------------------------------------------------------------------
void
HitTestHelper::DoBezier(
    _In_ const XPOINTF& controlPoint1,
    _In_ const XPOINTF& controlPoint2,
    _In_ const XPOINTF& endPoint
    )
{
    XPOINTF points[3] = {0.0};

    m_transform.Transform(&controlPoint1, &points[0], 1);
    m_transform.Transform(&controlPoint2, &points[1], 1);
    m_transform.Transform(&endPoint, &points[2], 1);

    {
        CMILBezierFlattener flattener(m_currentPoint, points, this, m_tolerance, NULL);

        IGNOREHR(flattener.Flatten(FALSE));
    }

    m_currentPointUntransformed = endPoint;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handle a new quadratic Bezier curve.
//
//------------------------------------------------------------------------
void
HitTestHelper::DoQuadraticBezier(
    _In_ const XPOINTF& controlPoint1,
    _In_ const XPOINTF& endPoint
    )
{
    XPOINTF transformedControlPoint1 = {0};
    XPOINTF transformedEndPoint = {0};
    XPOINTF points[3];

    m_transform.Transform(&controlPoint1, &transformedControlPoint1, 1);
    m_transform.Transform(&endPoint, &transformedEndPoint, 1);

    //
    // By the degree-elevation formula for Bezier curves the cubic Bezier
    // points of this quadratic Bezier curve are:
    //      pt0
    //      (1 / 3) * pt0 + (2 / 3) * pt1
    //      (2 / 3) * pt1 + (1 / 3) * pt2
    //      pt2
    //

    points[0].x = (1 / 3.0f) * m_currentPoint.x + (2 / 3.0f) * transformedControlPoint1.x;
    points[0].y = (1 / 3.0f) * m_currentPoint.y + (2 / 3.0f) * transformedControlPoint1.y;
    points[1].x = (2 / 3.0f) * transformedControlPoint1.x + (1 / 3.0f) * transformedEndPoint.x;
    points[1].y = (2 / 3.0f) * transformedControlPoint1.y + (1 / 3.0f) * transformedEndPoint.y;
    points[2].x = transformedEndPoint.x;
    points[2].y = transformedEndPoint.y;

    {
        CMILBezierFlattener flattener(m_currentPoint, points, this, m_tolerance, NULL);

        IGNOREHR(flattener.Flatten(FALSE));
    }

    m_currentPointUntransformed = endPoint;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handle a new arc.
//
//------------------------------------------------------------------------
void
HitTestHelper::DoArc(
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

    //
    // NOTE: Use original untransformed starting point to generate Bezier curves as a
    //       transformed arc may no longer be an arc.
    //
    ArcToBezier(m_currentPointUntransformed.x, m_currentPointUntransformed.y, size.width, size.height, rotationAngle, fIsLargeArc, fIsClockwise, point.x, point.y, bezierPoints, &curveCount);

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
//      Handle a point from the flattener.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
HitTestHelper::AcceptPoint(
    _In_ const XPOINTF& flattenedPoint,
    _In_ XFLOAT currentParameter,
    _Out_ XINT32& aborted
    )
{
    HRESULT hr = S_OK;

    AcceptPoint(flattenedPoint);

    aborted = FALSE;

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Query if NaN has been encountered or the provided points is NaN.
//
//------------------------------------------------------------------------
void
HitTestHelper::CheckForNaN(
    _In_ const XPOINTF& point
    )
{
    bool encounteredNaN = IsNanF(point.x) || IsNanF(point.y);
    XCP_FAULT_ON_FAILURE(!encounteredNaN);

    m_encounteredNaN = m_encounteredNaN || encounteredNaN;
}
