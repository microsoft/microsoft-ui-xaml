// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Synopsis:
//      (ctor) - Create a new sink for geometry groups.
//
//------------------------------------------------------------------------
GeometryGroupSink::GeometryGroupSink(
    _In_ IPALGeometrySink* pSink
    )
    : m_pSink(pSink)
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      (dtor) - Clean up geometry group sink.
//
//------------------------------------------------------------------------
GeometryGroupSink::~GeometryGroupSink(
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
GeometryGroupSink::BeginFigure(
    const XPOINTF& startPoint,
    bool fIsHollow
    )
{
    m_pSink->BeginFigure(startPoint, fIsHollow);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      End the current figure.
//
//------------------------------------------------------------------------
void
GeometryGroupSink::EndFigure(
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
GeometryGroupSink::AddArc(
    const XPOINTF& point,
    const XSIZEF& size,
    XFLOAT rotationAngle,
    bool fIsClockwise,
    bool fIsLargeArc
    )
{
    m_pSink->AddArc(point, size, rotationAngle, fIsClockwise, fIsLargeArc);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a Bezier curve to the figure.
//
//------------------------------------------------------------------------
void
GeometryGroupSink::AddBezier(
    const XPOINTF& controlPoint1,
    const XPOINTF& controlPoint2,
    const XPOINTF& endPoint
    )
{
    m_pSink->AddBezier(controlPoint1, controlPoint2, endPoint);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a line to the figure.
//
//------------------------------------------------------------------------
void
GeometryGroupSink::AddLine(
    const XPOINTF& point
    )
{
    m_pSink->AddLine(point);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a quadratic Bezier to the figure.
//
//------------------------------------------------------------------------
void
GeometryGroupSink::AddQuadraticBezier(
    const XPOINTF& controlPoint,
    const XPOINTF& endPoint
    )
{
    m_pSink->AddQuadraticBezier(controlPoint, endPoint);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add multiple lines to the figure.
//
//------------------------------------------------------------------------
void
GeometryGroupSink::AddLines(
    _In_reads_(uiCount) const XPOINTF *pPoints,
    XUINT32 uiCount
    )
{
    m_pSink->AddLines(pPoints, uiCount);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add multiple Bezier curves to the figure.
//
//------------------------------------------------------------------------
void
GeometryGroupSink::AddBeziers(
    _In_reads_(uiCount) const XPOINTF *pPoints,
    XUINT32 uiCount
    )
{
    m_pSink->AddBeziers(pPoints, uiCount);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add multiple quadratic Beziers to the figure.
//
//------------------------------------------------------------------------
void
GeometryGroupSink::AddQuadraticBeziers(
    _In_reads_(uiCount) const XPOINTF *pPoints,
    XUINT32 uiCount
    )
{
    m_pSink->AddQuadraticBeziers(pPoints, uiCount);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Set the fill mode for the figure.
//
//------------------------------------------------------------------------
void
GeometryGroupSink::SetFillMode(
    GeometryFillMode fillMode
    )
{
    //
    // Ignore fill mode changes for children.
    //
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Close the sink.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
GeometryGroupSink::Close(
    )
{
    //
    // This should never be called.
    //
    ASSERT(FALSE);

    RRETURN(E_UNEXPECTED);
}
