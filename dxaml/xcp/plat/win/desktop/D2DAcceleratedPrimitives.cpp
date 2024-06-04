// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "D2DAccelerated.h"
#include "D2DAcceleratedBrushes.h"
#include "D2DAcceleratedPrimitives.h"
#include "D2DAcceleratedRT.h"
#include <d2d1_1.h>
#include <d2d1helper.h>

_Check_return_ HRESULT UnwrapD2DGeometry(_In_ IPALAcceleratedGeometry* pGeometry, _Outptr_ ID2D1Geometry** ppD2DGeometry)
{
    HRESULT hr = S_OK;
    ID2D1Geometry* pD2DGeometry = NULL;

    switch(pGeometry->GetType())
    {
        case GeometryType::Generic:
        {
            pD2DGeometry = (reinterpret_cast<CD2DGenericGeometry *>(pGeometry))->GetD2DGeometry();
            break;
        }

        case GeometryType::Rectangle:
        {
            pD2DGeometry = (reinterpret_cast<CD2DRectangleGeometry *>(pGeometry))->GetD2DGeometry();
            break;
        }

        case GeometryType::RoundedRectangle:
        {
            pD2DGeometry = (reinterpret_cast<CD2DRoundedRectangleGeometry *>(pGeometry))->GetD2DGeometry();
            break;
        }

        case GeometryType::Ellipse:
        {
            pD2DGeometry = (reinterpret_cast<CD2DEllipseGeometry *>(pGeometry))->GetD2DGeometry();
            break;
        }

        case GeometryType::Group:
        {
            pD2DGeometry = (reinterpret_cast<CD2DGeometryGroup *>(pGeometry))->GetD2DGeometry();
            break;
        }

        case GeometryType::Path:
        {
            pD2DGeometry = (reinterpret_cast<CD2DPathGeometry *>(pGeometry))->GetD2DGeometry();
            break;
        }

        case GeometryType::Transformed:
        {
            pD2DGeometry = (reinterpret_cast<CD2DTransformedGeometry *>(pGeometry))->GetD2DGeometry();
            break;
        }

        default:
        {
            IFC(E_FAIL);
        }
    }

    SetInterface(*ppD2DGeometry, pD2DGeometry);

Cleanup:
    RRETURN(hr);
}


_Check_return_ HRESULT UnwrapD2DStrokeStyle(
    _In_ IPALStrokeStyle *pPALStrokeStyle,
    _Outptr_ ID2D1StrokeStyle **ppD2DStrokeStyle
    )
{
    HRESULT hr = S_OK;
    ID2D1StrokeStyle *pD2DStrokeStyle = NULL;

    if (pPALStrokeStyle)
    {
        pD2DStrokeStyle = (reinterpret_cast<CD2DStrokeStyle *>(pPALStrokeStyle))->GetD2DStrokeStyle();
    }

    SetInterface(*ppD2DStrokeStyle, pD2DStrokeStyle);

    RRETURN(hr);
}



//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
template<typename T>
_Check_return_ HRESULT
CD2DGeometry<T>::GetBounds(
    _Out_ XRECTF_RB *pBounds
    )
{
    HRESULT hr = S_OK;

    IFC(m_pGeometry->GetBounds(
        D2D1::IdentityMatrix(),
        PALToD2DRectF(pBounds)
        ));

    // If the bounds are empty, D2D returns a rect where left > right
    if (pBounds->left > pBounds->right)
    {
        EmptyRectF(pBounds);
    }

Cleanup:
    RRETURN(hr);
}

template<typename T>
_Check_return_ HRESULT
CD2DGeometry<T>::GetWidenedBounds(
    XFLOAT rStrokeWidth,
    _Out_ XRECTF_RB *pBounds
    )
{
    HRESULT hr = S_OK;

    IFC(m_pGeometry->GetWidenedBounds(
        rStrokeWidth,
        NULL,
        D2D1::IdentityMatrix(),
        PALToD2DRectF(pBounds)
        ));

    // If the bounds are empty, D2D returns a rect where left > right
    if (pBounds->left > pBounds->right)
    {
        EmptyRectF(pBounds);
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Fills the geometry.
//
//------------------------------------------------------------------------
template<typename T>
_Check_return_ HRESULT
CD2DGeometry<T>::Fill(
    _In_ IPALAcceleratedRender *pRenderTarget,
    _In_ IPALAcceleratedBrush *pBrush,
    XFLOAT opacity
    )
{
    RRETURN(pRenderTarget->FillGeometry(
        this,
        pBrush,
        opacity
        ));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Draws the geometry's outline.
//
//------------------------------------------------------------------------
template<typename T>
_Check_return_ HRESULT
CD2DGeometry<T>::Draw(
    _In_ IPALAcceleratedRender *pRenderTarget,
    _In_ IPALAcceleratedBrush *pBrush,
    XFLOAT rStrokeWidth,
    _In_opt_ IPALStrokeStyle *pStrokeStyle,
    XFLOAT opacity
    )
{
    RRETURN(pRenderTarget->DrawGeometry(
        this,
        pBrush,
        rStrokeWidth,
        pStrokeStyle,
        opacity
        ));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Determines if the geometry is an axis-aligned rectangle
//
//------------------------------------------------------------------------
template<typename T>
bool
CD2DGeometry<T>::IsAxisAlignedRectangle()
{
    return false;
}


//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CD2DStrokeStyle::Create(
    _In_ ID2D1StrokeStyle *pD2DStrokeStyle,
    _Outptr_ IPALStrokeStyle **ppPALStrokeStyle
    )
{
    HRESULT hr = S_OK;
    CD2DStrokeStyle *pPALStrokeStyle = NULL;

    pPALStrokeStyle = new CD2DStrokeStyle();

    IFC(pPALStrokeStyle->Initialize(pD2DStrokeStyle));

    *ppPALStrokeStyle = pPALStrokeStyle;
    pPALStrokeStyle = NULL;

Cleanup:
    ReleaseInterface(pPALStrokeStyle);

    RRETURN(hr);
}

CD2DStrokeStyle::CD2DStrokeStyle()
    : m_pStrokeStyle(NULL)
{
}

CD2DStrokeStyle::~CD2DStrokeStyle()
{
    ReleaseInterface(m_pStrokeStyle);
}

_Check_return_ HRESULT
CD2DStrokeStyle::Initialize(
    _In_ ID2D1StrokeStyle *pD2DStrokeStyle
    )
{
    HRESULT hr = S_OK;

    SetInterface(m_pStrokeStyle, pD2DStrokeStyle);

    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CD2DRectangleGeometry::Create(
    _In_ ID2D1RectangleGeometry *pD2DGeometry,
    XRECTF_RB rect,
    _Outptr_ IPALAcceleratedGeometry **ppPALGeometry
    )
{
    HRESULT hr = S_OK;
    CD2DRectangleGeometry *pPALGeometry = NULL;

    pPALGeometry = new CD2DRectangleGeometry();

    IFC(pPALGeometry->Initialize(
        pD2DGeometry,
        rect
        ));

    *ppPALGeometry = pPALGeometry;
    pPALGeometry = NULL;

Cleanup:
    ReleaseInterface(pPALGeometry);

    RRETURN(hr);
}

CD2DRectangleGeometry::CD2DRectangleGeometry()
{
}

CD2DRectangleGeometry::~CD2DRectangleGeometry()
{
}

_Check_return_ HRESULT
CD2DRectangleGeometry::Initialize(
    _In_ ID2D1RectangleGeometry *pD2DGeometry,
    XRECTF_RB rect
    )
{
    HRESULT hr = S_OK;

    IFC(CD2DGeometry<IPALAcceleratedGeometry>::Initialize(pD2DGeometry));

    m_rect = rect;

Cleanup:
    RRETURN(hr);
}

GeometryType
CD2DRectangleGeometry::GetType()
{
    return GeometryType::Rectangle;
}

bool
CD2DRectangleGeometry::IsAxisAlignedRectangle()
{
    return true;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Fills the geometry.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CD2DRectangleGeometry::Fill(
    _In_ IPALAcceleratedRender *pRenderTarget,
    _In_ IPALAcceleratedBrush *pBrush,
    XFLOAT opacity
    )
{
    RRETURN(pRenderTarget->FillRectangle(
        m_rect,
        pBrush,
        opacity
        ));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Draws the geometry's outline.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CD2DRectangleGeometry::Draw(
    _In_ IPALAcceleratedRender *pRenderTarget,
    _In_ IPALAcceleratedBrush *pBrush,
    XFLOAT rStrokeWidth,
    _In_opt_ IPALStrokeStyle *pStrokeStyle,
    XFLOAT opacity
    )
{
    RRETURN(pRenderTarget->DrawRectangle(
        m_rect,
        pBrush,
        rStrokeWidth,
        pStrokeStyle,
        opacity
        ));
}



//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CD2DRoundedRectangleGeometry::Create(
    _In_ ID2D1RoundedRectangleGeometry *pD2DGeometry,
    XRECTF_RB rect,
    XFLOAT rRadiusX,
    XFLOAT rRadiusY,
    _Outptr_ IPALAcceleratedGeometry **ppPALGeometry
    )
{
    HRESULT hr = S_OK;
    CD2DRoundedRectangleGeometry *pPALGeometry = NULL;

    pPALGeometry = new CD2DRoundedRectangleGeometry();

    IFC(pPALGeometry->Initialize(
        pD2DGeometry,
        rect,
        rRadiusX,
        rRadiusY
        ));

    *ppPALGeometry = pPALGeometry;
    pPALGeometry = NULL;

Cleanup:
    ReleaseInterface(pPALGeometry);

    RRETURN(hr);
}

CD2DRoundedRectangleGeometry::CD2DRoundedRectangleGeometry()
{
}

CD2DRoundedRectangleGeometry::~CD2DRoundedRectangleGeometry()
{
}

_Check_return_ HRESULT
CD2DRoundedRectangleGeometry::Initialize(
    _In_ ID2D1RoundedRectangleGeometry *pD2DGeometry,
    XRECTF_RB rect,
    XFLOAT rRadiusX,
    XFLOAT rRadiusY
    )
{
    HRESULT hr = S_OK;

    IFC(CD2DGeometry<IPALAcceleratedGeometry>::Initialize(pD2DGeometry));

    m_rect = rect;
    m_rRadiusX = rRadiusX;
    m_rRadiusY = rRadiusY;

Cleanup:
    RRETURN(hr);
}

GeometryType CD2DRoundedRectangleGeometry::GetType()
{
    return GeometryType::RoundedRectangle;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Fills the geometry.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CD2DRoundedRectangleGeometry::Fill(
    _In_ IPALAcceleratedRender *pRenderTarget,
    _In_ IPALAcceleratedBrush *pBrush,
    XFLOAT opacity
    )
{
    RRETURN(pRenderTarget->FillRoundedRectangle(
        m_rect,
        m_rRadiusX,
        m_rRadiusY,
        pBrush,
        opacity
        ));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Draws the geometry's outline.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CD2DRoundedRectangleGeometry::Draw(
    _In_ IPALAcceleratedRender *pRenderTarget,
    _In_ IPALAcceleratedBrush *pBrush,
    XFLOAT rStrokeWidth,
    _In_opt_ IPALStrokeStyle *pStrokeStyle,
    XFLOAT opacity
    )
{
    RRETURN(pRenderTarget->DrawRoundedRectangle(
        m_rect,
        m_rRadiusX,
        m_rRadiusY,
        pBrush,
        rStrokeWidth,
        pStrokeStyle,
        opacity
        ));
}



//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CD2DEllipseGeometry::Create(
    _In_ ID2D1EllipseGeometry *pD2DGeometry,
    XPOINTF center,
    XFLOAT rRadiusX,
    XFLOAT rRadiusY,
    _Outptr_ IPALAcceleratedGeometry **ppPALGeometry
    )
{
    HRESULT hr = S_OK;
    CD2DEllipseGeometry *pPALGeometry = NULL;

    pPALGeometry = new CD2DEllipseGeometry();

    IFC(pPALGeometry->Initialize(
        pD2DGeometry,
        center,
        rRadiusX,
        rRadiusY
        ));

    *ppPALGeometry = pPALGeometry;
    pPALGeometry = NULL;

Cleanup:
    ReleaseInterface(pPALGeometry);

    RRETURN(hr);
}

CD2DEllipseGeometry::CD2DEllipseGeometry()
{
}

CD2DEllipseGeometry::~CD2DEllipseGeometry()
{
}

_Check_return_ HRESULT
CD2DEllipseGeometry::Initialize(
    _In_ ID2D1EllipseGeometry *pD2DGeometry,
    XPOINTF center,
    XFLOAT rRadiusX,
    XFLOAT rRadiusY
    )
{
    HRESULT hr = S_OK;

    IFC(CD2DGeometry<IPALAcceleratedGeometry>::Initialize(pD2DGeometry));

    m_center = center;
    m_rRadiusX = rRadiusX;
    m_rRadiusY = rRadiusY;

Cleanup:
    RRETURN(hr);
}

GeometryType CD2DEllipseGeometry::GetType()
{
    return GeometryType::Ellipse;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Fills the geometry.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CD2DEllipseGeometry::Fill(
    _In_ IPALAcceleratedRender *pRenderTarget,
    _In_ IPALAcceleratedBrush *pBrush,
    XFLOAT opacity
    )
{
    RRETURN(pRenderTarget->FillEllipse(
        m_center,
        m_rRadiusX,
        m_rRadiusY,
        pBrush,
        opacity
        ));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Draws the geometry's outline.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CD2DEllipseGeometry::Draw(
    _In_ IPALAcceleratedRender *pRenderTarget,
    _In_ IPALAcceleratedBrush *pBrush,
    XFLOAT rStrokeWidth,
    _In_opt_ IPALStrokeStyle *pStrokeStyle,
    XFLOAT opacity
    )
{
    RRETURN(pRenderTarget->DrawEllipse(
        m_center,
        m_rRadiusX,
        m_rRadiusY,
        pBrush,
        rStrokeWidth,
        pStrokeStyle,
        opacity
        ));
}



//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CD2DPathGeometry::Create(
    _In_ ID2D1PathGeometry *pD2DGeometry,
    _Outptr_ IPALAcceleratedPathGeometry **ppPALGeometry
    )
{
    HRESULT hr = S_OK;
    CD2DPathGeometry *pPALGeometry = NULL;

    pPALGeometry = new CD2DPathGeometry();

    IFC(pPALGeometry->Initialize(pD2DGeometry));

    *ppPALGeometry = pPALGeometry;
    pPALGeometry = NULL;

Cleanup:
    ReleaseInterface(pPALGeometry);

    RRETURN(hr);
}

CD2DPathGeometry::CD2DPathGeometry()
{
}

CD2DPathGeometry::~CD2DPathGeometry()
{
}

GeometryType CD2DPathGeometry::GetType()
{
    return GeometryType::Path;
}

_Check_return_ HRESULT CD2DPathGeometry::Open(
    _Outptr_ IPALGeometrySink **ppPALGeometrySink
    )
{
    HRESULT hr = S_OK;
    ID2D1PathGeometry *pPathGeometry = reinterpret_cast<ID2D1PathGeometry *>(m_pGeometry);
    ID2D1GeometrySink *pD2DGeometrySink = NULL;
    IPALGeometrySink *pPALGeometrySink = NULL;

    IFC(pPathGeometry->Open(&pD2DGeometrySink));

    IFC(CD2DGeometrySink::Create(
        pD2DGeometrySink,
        &pPALGeometrySink
        ));

    *ppPALGeometrySink = pPALGeometrySink;
    pPALGeometrySink = NULL;

Cleanup:
    ReleaseInterface(pD2DGeometrySink);
    ReleaseInterface(pPALGeometrySink);
    RRETURN(hr);
}



//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CD2DGeometryGroup::Create(
    _In_ ID2D1GeometryGroup *pD2DGeometry,
    _Outptr_ IPALAcceleratedGeometry **ppPALGeometry
    )
{
    HRESULT hr = S_OK;
    CD2DGeometryGroup *pPALGeometry = NULL;

    pPALGeometry = new CD2DGeometryGroup();

    IFC(pPALGeometry->Initialize(pD2DGeometry));

    *ppPALGeometry = pPALGeometry;
    pPALGeometry = NULL;

Cleanup:
    ReleaseInterface(pPALGeometry);

    RRETURN(hr);
}

CD2DGeometryGroup::CD2DGeometryGroup()
{
}

CD2DGeometryGroup::~CD2DGeometryGroup()
{
}

GeometryType CD2DGeometryGroup::GetType()
{
    return GeometryType::Group;
}



//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CD2DTransformedGeometry::Create(
    _In_ IPALAcceleratedGeometry *pGeometry,
    _In_ const CMILMatrix* pMatrix,
    _In_ ID2D1TransformedGeometry *pD2DGeometry,
    _Outptr_ IPALAcceleratedGeometry **ppPALGeometry
    )
{
    HRESULT hr = S_OK;
    CD2DTransformedGeometry *pPALGeometry = NULL;

    //
    // If the input geometry is an axis-aligned rectangle, and the transform has no rotation
    // then the transformed geometry is also an axis-aligned rectangle
    //
    bool isAxisAlignedRect = pGeometry->IsAxisAlignedRectangle() && pMatrix->IsScaleOrTranslationOnly();

    pPALGeometry = new CD2DTransformedGeometry(isAxisAlignedRect);

    IFC(pPALGeometry->Initialize(pD2DGeometry));

    *ppPALGeometry = pPALGeometry;
    pPALGeometry = NULL;

Cleanup:
    ReleaseInterface(pPALGeometry);

    RRETURN(hr);
}

CD2DTransformedGeometry::CD2DTransformedGeometry(
    _In_ bool isAxisAlignedRect
    ) : m_isAxisAlignedRectangle(isAxisAlignedRect)
{
}

CD2DTransformedGeometry::~CD2DTransformedGeometry()
{
}

GeometryType CD2DTransformedGeometry::GetType()
{
    return GeometryType::Transformed;
}

bool
CD2DTransformedGeometry::IsAxisAlignedRectangle()
{
    return m_isAxisAlignedRectangle;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a new geometry sink.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CD2DGeometrySink::Create(
    _In_ ID2D1GeometrySink *pD2DGeometrySink,
    _Outptr_ IPALGeometrySink **ppPALGeometrySink
    )
{
    HRESULT hr = S_OK;
    CD2DGeometrySink *pPALGeometrySink = NULL;

    pPALGeometrySink = new CD2DGeometrySink();

    IFC(pPALGeometrySink->Initialize(pD2DGeometrySink));

    *ppPALGeometrySink = pPALGeometrySink;
    pPALGeometrySink = NULL;

Cleanup:
    ReleaseInterface(pPALGeometrySink);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------
CD2DGeometrySink::CD2DGeometrySink()
    : m_pD2DGeometrySink(NULL)
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------
CD2DGeometrySink::~CD2DGeometrySink()
{
    ReleaseInterface(m_pD2DGeometrySink);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes the geometry sink.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CD2DGeometrySink::Initialize(
    _In_ ID2D1GeometrySink *pD2DGeometrySink
    )
{
    HRESULT hr = S_OK;

    SetInterface(m_pD2DGeometrySink, pD2DGeometrySink);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Starts a new figure in the geometry sink.
//
//------------------------------------------------------------------------
void
CD2DGeometrySink::BeginFigure(
    const XPOINTF& startPoint,
    bool fIsHollow
    )
{
    m_pD2DGeometrySink->BeginFigure(
        *PALToD2DPointF(&startPoint),
        static_cast<D2D1_FIGURE_BEGIN>(fIsHollow)
        );
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Closes the current figure in the geometry sink.
//
//------------------------------------------------------------------------
void
CD2DGeometrySink::EndFigure(
    bool fIsClosed
    )
{
    m_pD2DGeometrySink->EndFigure(
        static_cast<D2D1_FIGURE_END>(fIsClosed)
        );
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Adds an arc to the geometry sink.
//
//------------------------------------------------------------------------
void
CD2DGeometrySink::AddArc(
    const XPOINTF& point,
    const XSIZEF& size,
    XFLOAT rotationAngle,
    bool fIsClockwise,
    bool fIsLargeArc
    )
{
    D2D1_ARC_SEGMENT arc =
    {
        *PALToD2DPointF(&point),
        *PALToD2DSizeF(&size),
        rotationAngle,
        static_cast<D2D1_SWEEP_DIRECTION>(fIsClockwise),
        static_cast<D2D1_ARC_SIZE>(fIsLargeArc)
    };

    m_pD2DGeometrySink->AddArc(&arc);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Adds a cubic Bezier to the geometry sink.
//
//------------------------------------------------------------------------
void
CD2DGeometrySink::AddBezier(
    const XPOINTF& controlPoint1,
    const XPOINTF& controlPoint2,
    const XPOINTF& endPoint
    )
{
    D2D1_BEZIER_SEGMENT bezier =
    {
        *PALToD2DPointF(&controlPoint1),
        *PALToD2DPointF(&controlPoint2),
        *PALToD2DPointF(&endPoint)
    };

    m_pD2DGeometrySink->AddBezier(&bezier);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Adds a line to the geometry sink.
//
//------------------------------------------------------------------------
void
CD2DGeometrySink::AddLine(
    const XPOINTF& point
    )
{
    m_pD2DGeometrySink->AddLine(*PALToD2DPointF(&point));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Adds a quadratic Bezier to the geometry sink.
//
//------------------------------------------------------------------------
void
CD2DGeometrySink::AddQuadraticBezier(
    const XPOINTF& controlPoint,
    const XPOINTF& endPoint
    )
{
    D2D1_QUADRATIC_BEZIER_SEGMENT bezier =
    {
        *PALToD2DPointF(&controlPoint),
        *PALToD2DPointF(&endPoint)
    };

    m_pD2DGeometrySink->AddQuadraticBezier(&bezier);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Adds lines to the geometry sink.
//
//------------------------------------------------------------------------
void
CD2DGeometrySink::AddLines(
    _In_reads_(uiCount) const XPOINTF *pPoints,
    XUINT32 pointCount
    )
{
    m_pD2DGeometrySink->AddLines(
        PALToD2DPointF(pPoints),
        pointCount
        );
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Adds cubic Beziers to the geometry sink. Points are provided in
//      sets of 3: {control point 1, control point 2, endpoint}
//
//------------------------------------------------------------------------
void
CD2DGeometrySink::AddBeziers(
    _In_reads_(uiCount) const XPOINTF *pPoints,
    XUINT32 pointCount
    )
{
    m_pD2DGeometrySink->AddBeziers(
        reinterpret_cast<const D2D1_BEZIER_SEGMENT *>(pPoints),
        pointCount / 3
        );
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Adds quadratic Beziers to the geometry sink. Points are provided
//      in sets of 2: {control point, endpoint}
//
//------------------------------------------------------------------------
void
CD2DGeometrySink::AddQuadraticBeziers(
    _In_reads_(uiCount) const XPOINTF *pPoints,
    XUINT32 pointCount
    )
{
    m_pD2DGeometrySink->AddQuadraticBeziers(
        reinterpret_cast<const D2D1_QUADRATIC_BEZIER_SEGMENT *>(pPoints),
        pointCount / 2
        );
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the fill mode of the geometry sink.
//
//------------------------------------------------------------------------
void
CD2DGeometrySink::SetFillMode(
    GeometryFillMode fillMode
    )
{
    m_pD2DGeometrySink->SetFillMode(
        static_cast<D2D1_FILL_MODE>(fillMode)
        );
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Closes the geometry sink.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CD2DGeometrySink::Close()
{
    RRETURN(m_pD2DGeometrySink->Close());
}
