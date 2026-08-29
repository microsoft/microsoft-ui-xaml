// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <d2d1_1.h>
#include <DWrite_3.h>

_Check_return_ HRESULT UnwrapD2DGeometry(
    _In_ IPALAcceleratedGeometry *pGeometry,
    _Outptr_ ID2D1Geometry **ppD2DGeometry
    );

_Check_return_ HRESULT UnwrapD2DStrokeStyle(
    _In_ IPALStrokeStyle *pPALStrokeStyle,
    _Outptr_ ID2D1StrokeStyle **ppD2DStrokeStyle
    );

class CD2DStrokeStyle : public CXcpObjectBase<IPALStrokeStyle>
{
public:
    static _Check_return_ HRESULT Create(
        _In_ ID2D1StrokeStyle *pD2DStrokeStyle,
        _Outptr_ IPALStrokeStyle **ppPALStrokeStyle
        );

    _Ret_notnull_ ID2D1StrokeStyle* GetD2DStrokeStyle()
    {
        return m_pStrokeStyle;
    }

protected:
    CD2DStrokeStyle();
    ~CD2DStrokeStyle() override;

private:
    _Check_return_ HRESULT Initialize(
        _In_ ID2D1StrokeStyle *pD2DStrokeStyle
        );

    ID2D1StrokeStyle *m_pStrokeStyle;
};

template<typename T>
class CD2DGeometry : public CXcpObjectBase<T>
{
public:
    GeometryType GetType() override
    {
        return GeometryType::Generic;
    }

    _Ret_notnull_ ID2D1Geometry* GetD2DGeometry()
    {
        return m_pGeometry;
    }

    _Check_return_ HRESULT GetBounds(
        _Out_ XRECTF_RB *pBounds
        ) override;

    _Check_return_ HRESULT GetWidenedBounds(
        XFLOAT rStrokeWidth,
        _Out_ XRECTF_RB *pBounds
        ) override;

    _Check_return_ HRESULT Fill(
        _In_ IPALAcceleratedRender *pRenderTarget,
        _In_ IPALAcceleratedBrush *pBrush,
        XFLOAT opacity
        ) override;

    _Check_return_ HRESULT Draw(
        _In_ IPALAcceleratedRender *pRenderTarget,
        _In_ IPALAcceleratedBrush *pBrush,
        XFLOAT rStrokeWidth,
        _In_opt_ IPALStrokeStyle *pStrokeStyle,
        XFLOAT opacity
        ) override;

    bool IsAxisAlignedRectangle() override;

protected:
    CD2DGeometry() : m_pGeometry(NULL)
    {
    }

    ~CD2DGeometry() override
    {
        ReleaseInterface(m_pGeometry);
    }

    _Check_return_ HRESULT Initialize(_In_ ID2D1Geometry* pGeometry)
    {
        HRESULT hr = S_OK;

        SetInterface(m_pGeometry, pGeometry);

        RRETURN(hr);
    }

    ID2D1Geometry* m_pGeometry;
};


typedef CD2DGeometry<IPALAcceleratedGeometry> CD2DGenericGeometry;


class CD2DRectangleGeometry : public CD2DGeometry<IPALAcceleratedGeometry>
{
public:
    static _Check_return_ HRESULT Create(
        _In_ ID2D1RectangleGeometry *pD2DGeometry,
        XRECTF_RB rect,
        _Outptr_ IPALAcceleratedGeometry **ppPALGeometry
        );

    GeometryType GetType() override;

    _Check_return_ HRESULT Fill(
        _In_ IPALAcceleratedRender *pRenderTarget,
        _In_ IPALAcceleratedBrush *pBrush,
        XFLOAT opacity
        ) override;

    _Check_return_ HRESULT Draw(
        _In_ IPALAcceleratedRender *pRenderTarget,
        _In_ IPALAcceleratedBrush *pBrush,
        XFLOAT rStrokeWidth,
        _In_opt_ IPALStrokeStyle *pStrokeStyle,
        XFLOAT opacity
        ) override;

    bool IsAxisAlignedRectangle() override;

protected:
    CD2DRectangleGeometry();
    ~CD2DRectangleGeometry() override;

    _Check_return_ HRESULT Initialize(
        _In_ ID2D1RectangleGeometry *pD2DGeometry,
        XRECTF_RB rect
        );

    XRECTF_RB m_rect;
};


class CD2DRoundedRectangleGeometry : public CD2DGeometry<IPALAcceleratedGeometry>
{
public:
    static _Check_return_ HRESULT Create(
        _In_ ID2D1RoundedRectangleGeometry *pD2DGeometry,
        XRECTF_RB rect,
        XFLOAT rRadiusX,
        XFLOAT rRadiusY,
        _Outptr_ IPALAcceleratedGeometry **ppPALGeometry
        );

    GeometryType GetType() override;

    _Check_return_ HRESULT Fill(
        _In_ IPALAcceleratedRender *pRenderTarget,
        _In_ IPALAcceleratedBrush *pBrush,
        XFLOAT opacity
        ) override;

    _Check_return_ HRESULT Draw(
        _In_ IPALAcceleratedRender *pRenderTarget,
        _In_ IPALAcceleratedBrush *pBrush,
        XFLOAT rStrokeWidth,
        _In_opt_ IPALStrokeStyle *pStrokeStyle,
        XFLOAT opacity
        ) override;

protected:
    CD2DRoundedRectangleGeometry();
    ~CD2DRoundedRectangleGeometry() override;

    _Check_return_ HRESULT Initialize(
        _In_ ID2D1RoundedRectangleGeometry *pD2DGeometry,
        XRECTF_RB rect,
        XFLOAT rRadiusX,
        XFLOAT rRadiusY
        );

    XRECTF_RB m_rect;
    XFLOAT m_rRadiusX;
    XFLOAT m_rRadiusY{};
};


class CD2DEllipseGeometry : public CD2DGeometry<IPALAcceleratedGeometry>
{
public:
    static _Check_return_ HRESULT Create(
        _In_ ID2D1EllipseGeometry *pD2DGeometry,
        XPOINTF center,
        XFLOAT rRadiusX,
        XFLOAT rRadiusY,
        _Outptr_ IPALAcceleratedGeometry **ppPALGeometry
        );

    GeometryType GetType() override;

    _Check_return_ HRESULT Fill(
        _In_ IPALAcceleratedRender *pRenderTarget,
        _In_ IPALAcceleratedBrush *pBrush,
        XFLOAT opacity
        ) override;

    _Check_return_ HRESULT Draw(
        _In_ IPALAcceleratedRender *pRenderTarget,
        _In_ IPALAcceleratedBrush *pBrush,
        XFLOAT rStrokeWidth,
        _In_opt_ IPALStrokeStyle *pStrokeStyle,
        XFLOAT opacity
        ) override;

protected:
    CD2DEllipseGeometry();
    ~CD2DEllipseGeometry() override;

    _Check_return_ HRESULT Initialize(
        _In_ ID2D1EllipseGeometry *pD2DGeometry,
        XPOINTF center,
        XFLOAT rRadiusX,
        XFLOAT rRadiusY
        );

    XPOINTF m_center;
    XFLOAT m_rRadiusX;
    XFLOAT m_rRadiusY;
};


class CD2DPathGeometry : public CD2DGeometry<IPALAcceleratedPathGeometry>
{
public:
    static _Check_return_ HRESULT Create(
        _In_ ID2D1PathGeometry *pD2DGeometry,
        _Outptr_ IPALAcceleratedPathGeometry **ppPALGeometry
        );

    GeometryType GetType() override;

    _Check_return_ HRESULT Open(
        _Outptr_ IPALGeometrySink **ppPALGeometrySink
        ) override;

protected:
    CD2DPathGeometry();
    ~CD2DPathGeometry() override;
};


class CD2DGeometryGroup : public CD2DGeometry<IPALAcceleratedGeometry>
{
public:
    static _Check_return_ HRESULT Create(
        _In_ ID2D1GeometryGroup *pD2DGeometry,
        _Outptr_ IPALAcceleratedGeometry **ppPALGeometry
        );

    GeometryType GetType() override;

protected:
    CD2DGeometryGroup();
    ~CD2DGeometryGroup() override;
};


class CD2DTransformedGeometry : public CD2DGeometry<IPALAcceleratedGeometry>
{
public:
    static _Check_return_ HRESULT Create(
        _In_ IPALAcceleratedGeometry *pGeometry,
        _In_ const CMILMatrix* pMatrix,
        _In_ ID2D1TransformedGeometry *pD2DGeometry,
        _Outptr_ IPALAcceleratedGeometry **ppPALGeometry
        );

    GeometryType GetType() override;

    bool IsAxisAlignedRectangle() override;

protected:
    CD2DTransformedGeometry(
        _In_ bool isAxisAlignedRect
        );
    ~CD2DTransformedGeometry() override;

private:
    bool m_isAxisAlignedRectangle;
};


class CD2DGeometrySink : public CXcpObjectBase<IPALGeometrySink>
{
    friend class CD2DPathGeometry;

public:
    void BeginFigure(
        const XPOINTF& startPoint,
        bool fIsHollow
        ) override;

    void EndFigure(
        bool fIsClosed
        ) override;

    void AddArc(
        const XPOINTF& point,
        const XSIZEF& size,
        XFLOAT rotationAngle,
        bool fIsClockwise,
        bool fIsLargeArc
        ) override;

    void AddBezier(
        const XPOINTF& controlPoint1,
        const XPOINTF& controlPoint2,
        const XPOINTF& endPoint
        ) override;

    void AddLine(
        const XPOINTF& point
        ) override;

    void AddQuadraticBezier(
        const XPOINTF& controlPoint,
        const XPOINTF& endPoint
        ) override;

    void AddLines(
        _In_reads_(uiCount) const XPOINTF *pPoints,
        XUINT32 uiCount
        ) override;

    void AddBeziers(
        _In_reads_(uiCount) const XPOINTF *pPoints,
        XUINT32 uiCount
        ) override;

    void AddQuadraticBeziers(
        _In_reads_(uiCount) const XPOINTF *pPoints,
        XUINT32 uiCount
        ) override;

    void SetFillMode(
        GeometryFillMode fillMode
        ) override;

    _Check_return_ HRESULT Close() override;

private:
    static _Check_return_ HRESULT Create(
        _In_ ID2D1GeometrySink *pD2DGeometrySink,
        _Outptr_ IPALGeometrySink **ppPALGeometrySink
        );

    CD2DGeometrySink();
    ~CD2DGeometrySink() override;

    _Check_return_ HRESULT Initialize(
        _In_ ID2D1GeometrySink *pD2DGeometrySink
        );

    ID2D1GeometrySink *m_pD2DGeometrySink;
};

