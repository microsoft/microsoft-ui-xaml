// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Synopsis:
//      (ctor) - Create a new sink for bounding geometry.
//
//------------------------------------------------------------------------
BoundsGeometrySink::BoundsGeometrySink(
    )
    : m_fillMode(GeometryFillMode::Alternate)
    , m_figureActive(FALSE)
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      (dtor) - Clean up geometry sink.
//
//------------------------------------------------------------------------
BoundsGeometrySink::~BoundsGeometrySink(
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
BoundsGeometrySink::BeginFigure(
    const XPOINTF& startPoint,
    bool fIsHollow
    )
{
    if (!fIsHollow)
    {
        m_figureActive = TRUE;
        m_boundsHelper.StartAt(startPoint);
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
BoundsGeometrySink::EndFigure(
    bool fIsClosed
    )
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add an arc to the figure.
//
//------------------------------------------------------------------------
void
BoundsGeometrySink::AddArc(
    const XPOINTF& point,
    const XSIZEF& size,
    XFLOAT rotationAngle,
    bool fIsClockwise,
    bool fIsLargeArc
    )
{
    if (m_figureActive)
    {
        m_boundsHelper.DoArc(point, size, rotationAngle, fIsClockwise, fIsLargeArc);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a Bezier curve to the figure.
//
//------------------------------------------------------------------------
void
BoundsGeometrySink::AddBezier(
    const XPOINTF& controlPoint1,
    const XPOINTF& controlPoint2,
    const XPOINTF& endPoint
    )
{
    if (m_figureActive)
    {
        m_boundsHelper.DoBezier(controlPoint1, controlPoint2, endPoint);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a line to the figure.
//
//------------------------------------------------------------------------
void
BoundsGeometrySink::AddLine(
    const XPOINTF& point
    )
{
    if (m_figureActive)
    {
        m_boundsHelper.DoLine(point);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a quadratic Bezier to the figure.
//
//------------------------------------------------------------------------
void
BoundsGeometrySink::AddQuadraticBezier(
    const XPOINTF& controlPoint,
    const XPOINTF& endPoint
    )
{
    if (m_figureActive)
    {
        m_boundsHelper.DoQuadraticBezier(controlPoint, endPoint);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add multiple lines to the figure.
//
//------------------------------------------------------------------------
void
BoundsGeometrySink::AddLines(
    _In_reads_(uiCount) const XPOINTF *pPoints,
    XUINT32 uiCount
    )
{
    if (m_figureActive)
    {
        for (XUINT32 i = 0; i < uiCount; ++i)
        {
            m_boundsHelper.DoLine(pPoints[i]);
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
BoundsGeometrySink::AddBeziers(
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
            m_boundsHelper.DoBezier(pCurrentBezier[0], pCurrentBezier[1], pCurrentBezier[2]);

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
BoundsGeometrySink::AddQuadraticBeziers(
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
            m_boundsHelper.DoQuadraticBezier(pCurrentBezier[0], pCurrentBezier[1]);

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
BoundsGeometrySink::SetFillMode(
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
BoundsGeometrySink::Close(
    )
{
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get the result from bounding.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
BoundsGeometrySink::GetBounds(
    _Out_ XRECTF_RB* pBounds
    )
{
    IFC_RETURN(m_boundsHelper.GetBounds(pBounds));

    return S_OK;
}
