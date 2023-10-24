// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Synopsis:
//      (ctor) - Create a new sink for hit testing geometry.
//
//------------------------------------------------------------------------
HitTestGeometrySink::HitTestGeometrySink(
    )
    : m_fillMode(GeometryFillMode::Alternate)
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      (dtor) - Clean up geometry sink.
//
//------------------------------------------------------------------------
HitTestGeometrySink::~HitTestGeometrySink(
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
HitTestGeometrySink::BeginFigure(
    const XPOINTF& startPoint,
    bool fIsHollow
    )
{
    ASSERT(m_pBaseHitTestHelper != NULL);

    if (!fIsHollow)
    {
        m_figureActive = TRUE;
        m_startPoint = startPoint;
        m_pBaseHitTestHelper->StartAt(startPoint);
    }
    else
    {
        m_figureActive = FALSE;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      End the current figure.
//
//------------------------------------------------------------------------
void
HitTestGeometrySink::EndFigure(
    bool fIsClosed
    )
{
    if (m_figureActive)
    {
        m_pBaseHitTestHelper->DoLine(m_startPoint);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add an arc to the figure.
//
//------------------------------------------------------------------------
void
HitTestGeometrySink::AddArc(
    const XPOINTF& point,
    const XSIZEF& size,
    XFLOAT rotationAngle,
    bool fIsClockwise,
    bool fIsLargeArc
    )
{
    if (m_figureActive)
    {
        m_pBaseHitTestHelper->DoArc(point, size, rotationAngle, fIsClockwise, fIsLargeArc);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a Bezier curve to the figure.
//
//------------------------------------------------------------------------
void
HitTestGeometrySink::AddBezier(
    const XPOINTF& controlPoint1,
    const XPOINTF& controlPoint2,
    const XPOINTF& endPoint
    )
{
    if (m_figureActive)
    {
        m_pBaseHitTestHelper->DoBezier(controlPoint1, controlPoint2, endPoint);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a line to the figure.
//
//------------------------------------------------------------------------
void
HitTestGeometrySink::AddLine(
    const XPOINTF& point
    )
{
    if (m_figureActive)
    {
        m_pBaseHitTestHelper->DoLine(point);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a quadratic Bezier to the figure.
//
//------------------------------------------------------------------------
void
HitTestGeometrySink::AddQuadraticBezier(
    const XPOINTF& controlPoint,
    const XPOINTF& endPoint
    )
{
    if (m_figureActive)
    {
        m_pBaseHitTestHelper->DoQuadraticBezier(controlPoint, endPoint);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add multiple lines to the figure.
//
//------------------------------------------------------------------------
void
HitTestGeometrySink::AddLines(
    _In_reads_(uiCount) const XPOINTF *pPoints,
    XUINT32 uiCount
    )
{
    if (m_figureActive)
    {
        for (XUINT32 i = 0; i < uiCount; ++i)
        {
            m_pBaseHitTestHelper->DoLine(pPoints[i]);
        }
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add multiple Bezier curves to the figure.
//
//------------------------------------------------------------------------
void
HitTestGeometrySink::AddBeziers(
    _In_reads_(uiCount) const XPOINTF *pPoints,
    XUINT32 uiCount
    )
{
    ASSERT(uiCount % 3 == 0);

    if (m_figureActive)
    {
        const XPOINTF* pCurrentBezier = pPoints;

        for (XUINT32 i = 0; i < uiCount / 3; ++i)
        {
            m_pBaseHitTestHelper->DoBezier(pCurrentBezier[0], pCurrentBezier[1], pCurrentBezier[2]);

            pCurrentBezier += 3;
        }
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add multiple quadratic Beziers to the figure.
//
//------------------------------------------------------------------------
void
HitTestGeometrySink::AddQuadraticBeziers(
    _In_reads_(uiCount) const XPOINTF *pPoints,
    XUINT32 uiCount
    )
{
    ASSERT(uiCount % 2 == 0);

    if (m_figureActive)
    {
        const XPOINTF* pCurrentBezier = pPoints;

        for (XUINT32 i = 0; i < uiCount / 2; ++i)
        {
            m_pBaseHitTestHelper->DoQuadraticBezier(pCurrentBezier[0], pCurrentBezier[1]);

            pCurrentBezier += 2;
        }
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Set the fill mode for the figure.
//
//------------------------------------------------------------------------
void
HitTestGeometrySink::SetFillMode(
    GeometryFillMode fillMode
    )
{
    m_fillMode = fillMode;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Close the sink.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
HitTestGeometrySink::Close(
    )
{
    RRETURN(S_OK);
}
