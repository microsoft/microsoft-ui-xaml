// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Synopsis:
//      (ctor) - Create a new sink for transforming geometry.
//
//------------------------------------------------------------------------
TransformGeometrySink::TransformGeometrySink(
    _In_ IPALGeometrySink* pSink,
    _In_ const CMILMatrix& transform
    )
    : m_pSink(pSink)
    , m_transform(transform)
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      (dtor) - Clean up transform geometry sink.
//
//------------------------------------------------------------------------
TransformGeometrySink::~TransformGeometrySink(
    )
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Begin a new figure.
//
//------------------------------------------------------------------------
void
TransformGeometrySink::BeginFigure(
    const XPOINTF& startPoint,
    bool fIsHollow
    )
{
    XPOINTF transformedPoint = { };

    m_transform.Transform(&startPoint, &transformedPoint, 1);

    m_pSink->BeginFigure(transformedPoint, fIsHollow);

    m_lastUntransformedPoint = startPoint;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      End the current figure.
//
//------------------------------------------------------------------------
void
TransformGeometrySink::EndFigure(
    bool fIsClosed
    )
{
    m_pSink->EndFigure(fIsClosed);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add an arc to the figure.
//
//------------------------------------------------------------------------
void
TransformGeometrySink::AddArc(
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

    ArcToBezier(m_lastUntransformedPoint.x, m_lastUntransformedPoint.y, size.width, size.height, rotationAngle, fIsLargeArc, fIsClockwise, point.x, point.y, bezierPoints, &curveCount);

    if (curveCount > 0)
    {
        //
        // Push Beziers to the sink.
        //
        AddBeziers(bezierPoints, curveCount * 3);
    }
    else
    {
        //
        // The arc is a line as it has no curves.
        //
        AddLine(point);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a Bezier curve to the figure.
//
//------------------------------------------------------------------------
void
TransformGeometrySink::AddBezier(
    const XPOINTF& controlPoint1,
    const XPOINTF& controlPoint2,
    const XPOINTF& endPoint
    )
{
    XPOINTF points[3] =
    {
        controlPoint1,
        controlPoint2,
        endPoint
    };

    AddBeziers(points, 3);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a line to the figure.
//
//------------------------------------------------------------------------
void
TransformGeometrySink::AddLine(
    const XPOINTF& point
    )
{
    AddLines(&point, 1);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a quadratic Bezier to the figure.
//
//------------------------------------------------------------------------
void
TransformGeometrySink::AddQuadraticBezier(
    const XPOINTF& controlPoint,
    const XPOINTF& endPoint
    )
{
    XPOINTF points[2] =
    {
        controlPoint,
        endPoint
    };

    AddQuadraticBeziers(points, 2);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add multiple lines to the figure.
//
//------------------------------------------------------------------------
void
TransformGeometrySink::AddLines(
    _In_reads_(uiCount) const XPOINTF *pPoints,
    XUINT32 uiCount
    )
{
    XPOINTF transformedPoints[16] = { };
    const XPOINTF* pRemainingPoints = NULL;
    XUINT32 remainingPointsCount = 0;

    pRemainingPoints = pPoints;
    remainingPointsCount = uiCount;

    while (remainingPointsCount > 0)
    {
        XUINT32 pointsToBatch = MIN(remainingPointsCount, ARRAY_SIZE(transformedPoints));

        m_transform.Transform(pRemainingPoints, transformedPoints, pointsToBatch);

        m_pSink->AddLines(transformedPoints, pointsToBatch);

        pRemainingPoints += pointsToBatch;
        remainingPointsCount -= pointsToBatch;
    }

    if (uiCount > 0)
    {
        m_lastUntransformedPoint = pPoints[uiCount - 1];
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add multiple Bezier curves to the figure.
//
//------------------------------------------------------------------------
void
TransformGeometrySink::AddBeziers(
    _In_reads_(uiCount) const XPOINTF *pPoints,
    XUINT32 uiCount
    )
{
    XPOINTF transformedPoints[16 * 3] = { };
    const XPOINTF* pRemainingPoints = NULL;
    XUINT32 remainingPointsCount = 0;

    ASSERT(uiCount % 3 == 0);

    pRemainingPoints = pPoints;
    remainingPointsCount = uiCount;

    while (remainingPointsCount > 0)
    {
        XUINT32 pointsToBatch = MIN(remainingPointsCount, ARRAY_SIZE(transformedPoints));

        m_transform.Transform(pRemainingPoints, transformedPoints, pointsToBatch);

        m_pSink->AddBeziers(transformedPoints, pointsToBatch);

        pRemainingPoints += pointsToBatch;
        remainingPointsCount -= pointsToBatch;
    }

    if (uiCount > 0)
    {
        m_lastUntransformedPoint = pPoints[uiCount - 1];
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add multiple quadratic Beziers to the figure.
//
//------------------------------------------------------------------------
void
TransformGeometrySink::AddQuadraticBeziers(
    _In_reads_(uiCount) const XPOINTF *pPoints,
    XUINT32 uiCount
    )
{
    XPOINTF transformedPoints[16 * 2] = { };
    const XPOINTF* pRemainingPoints = NULL;
    XUINT32 remainingPointsCount = 0;

    ASSERT(uiCount % 2 == 0);

    pRemainingPoints = pPoints;
    remainingPointsCount = uiCount;

    while (remainingPointsCount > 0)
    {
        XUINT32 pointsToBatch = MIN(remainingPointsCount, ARRAY_SIZE(transformedPoints));

        m_transform.Transform(pRemainingPoints, transformedPoints, pointsToBatch);

        m_pSink->AddQuadraticBeziers(transformedPoints, pointsToBatch);

        pRemainingPoints += pointsToBatch;
        remainingPointsCount -= pointsToBatch;
    }

    if (uiCount > 0)
    {
        m_lastUntransformedPoint = pPoints[uiCount - 1];
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Set the fill mode for the figure.
//
//------------------------------------------------------------------------
void
TransformGeometrySink::SetFillMode(
    GeometryFillMode fillMode
    )
{
    m_pSink->SetFillMode(fillMode);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Close the sink.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
TransformGeometrySink::Close(
    )
{
    RRETURN(m_pSink->Close());
}
