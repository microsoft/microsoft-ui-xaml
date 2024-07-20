// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CD3D11Device;
struct ID2D1SvgDocument;
class D2DTextDrawingContext;

class CClipLayerStack
{
public:
    XUINT32 Size();

    _Check_return_ HRESULT Push(
        _In_ const XRECTF_RB* pClipRect
        );

    _Check_return_ HRESULT Pop();

    void Clear();

    bool Intersect(
        _Inout_ XRECTF_RB* pRect
        );

    const XRECTF_RB& Rect(
        XUINT32 index
        );

private:
    xvector<XRECTF_RB> m_stack;
};


template<typename T>
class CD2DRenderTarget : public CXcpObjectBase<T>
{
public:
    explicit CD2DRenderTarget(ID2D1DeviceContext* pD2DDeviceContext);

    //
    // IPALAcceleratedRenderTarget
    //

    _Check_return_ HRESULT BeginDraw() override;
    _Check_return_ HRESULT EndDraw() override;

    XUINT32 GetWidth() override;
    XUINT32 GetHeight() override;

    _Check_return_ HRESULT CreateBitmap(
        _In_ IPALSurface *pBitmapSurface,
        _Outptr_ IPALAcceleratedBitmap **ppPALBitmap
        ) override;

    _Check_return_ HRESULT CreateSolidColorBrush(
        XUINT32 uiColor,
        XFLOAT rOpacity,
        _Outptr_ IPALAcceleratedBrush **ppPALBrush
        ) override;

    _Check_return_ HRESULT CreateBitmapBrush(
        _In_ IPALSurface *pBitmapSurface,
        XFLOAT rOpacity,
        _Outptr_ IPALAcceleratedBrush **ppPALBrush
        ) override;

    _Check_return_ HRESULT CreateBitmapBrush(
        _In_ IPALAcceleratedBitmapBrush *pPALBitmapBrush,
        XFLOAT rOpacity,
        _Outptr_ IPALAcceleratedBrush **ppPALBrush
        ) override;

    _Check_return_ HRESULT CreateBitmapBrush(
        _In_ ID2D1SvgDocument* d2dSvgDocument,
        XFLOAT opacity,
        uint32_t width,
        uint32_t height,
        _Outptr_ IPALAcceleratedBrush** palBrushOut
        ) override;

    _Check_return_ HRESULT CreateSharedBitmapBrush(
        _In_ IPALAcceleratedBitmap *pInputBitmap,
        XFLOAT rOpacity,
        _Outptr_ IPALAcceleratedBrush **ppPALBrush
        ) override;

    _Check_return_ HRESULT CreateLinearGradientBrush(
        XPOINTF startPoint,
        XPOINTF endPoint,
        _In_reads_(gradientStopsCount) XcpGradientStop *pGradientStopsArray,
        XUINT32 gradientStopsCount,
        InterpolationMode interpolationMode,
        GradientWrapMode gradientWrapMode,
        XFLOAT rOpacity,
        _Outptr_ IPALAcceleratedBrush **ppPALBrush
        ) override;

    _Check_return_ HRESULT CreateRadialGradientBrush(
        XPOINTF center,
        XPOINTF gradientOriginOffset,
        XFLOAT rRadiusX,
        XFLOAT rRadiusY,
        _In_reads_(gradientStopsCount) XcpGradientStop *pGradientStopsArray,
        XUINT32 gradientStopsCount,
        InterpolationMode interpolationMode,
        GradientWrapMode gradientWrapMode,
        XFLOAT rOpacity,
        _Outptr_ IPALAcceleratedBrush **ppPALBrush
        ) override;

    _Check_return_ HRESULT GetRenderTarget(_Outptr_ ID2D1RenderTarget **ppD2DRenderTarget) const
    {
        return m_pD2DDeviceContext->QueryInterface(IID_PPV_ARGS(ppD2DRenderTarget));
    }

    _Check_return_ HRESULT GetDeviceContext(_Outptr_ ID2D1DeviceContext5 **ppD2DDeviceContext) const override
    {
        return m_pD2DDeviceContext->QueryInterface(IID_PPV_ARGS(ppD2DDeviceContext));
    }

    //
    // IPALAcceleratedRender
    //
    _Check_return_ HRESULT SetTransform(
        _In_ const CMILMatrix *pMatrix
        ) override;

    _Check_return_ HRESULT FillGeometry(
        _In_ IPALAcceleratedGeometry *pGeometry,
        _In_ IPALAcceleratedBrush *pBrush,
        XFLOAT opacity
        ) override;

    _Check_return_ HRESULT DrawGeometry(
        _In_ IPALAcceleratedGeometry *pGeometry,
        _In_ IPALAcceleratedBrush *pBrush,
        XFLOAT rStrokeWidth,
        _In_opt_ IPALStrokeStyle *pStrokeStyle,
        XFLOAT opacity
        ) override;

    _Check_return_ HRESULT FillRectangle(
        _In_ const XRECTF_RB &rect,
        _In_ IPALAcceleratedBrush *pBrush,
        XFLOAT opacity
        ) override;

    _Check_return_ HRESULT DrawRectangle(
        _In_ const XRECTF_RB &rect,
        _In_ IPALAcceleratedBrush *pBrush,
        XFLOAT rStrokeWidth,
        _In_opt_ IPALStrokeStyle *pStrokeStyle,
        XFLOAT opacity
        ) override;

    _Check_return_ HRESULT FillRoundedRectangle(
        _In_ const XRECTF_RB &rect,
        XFLOAT rRadiusX,
        XFLOAT rRadiusY,
        _In_ IPALAcceleratedBrush *pBrush,
        XFLOAT opacity
        ) override;

    _Check_return_ HRESULT DrawRoundedRectangle(
        _In_ const XRECTF_RB &rect,
        XFLOAT rRadiusX,
        XFLOAT rRadiusY,
        _In_ IPALAcceleratedBrush *pBrush,
        XFLOAT rStrokeWidth,
        _In_opt_ IPALStrokeStyle *pStrokeStyle,
        XFLOAT opacity
        ) override;

    _Check_return_ HRESULT FillEllipse(
        _In_ const XPOINTF &center,
        XFLOAT rRadiusX,
        XFLOAT rRadiusY,
        _In_ IPALAcceleratedBrush *pBrush,
        XFLOAT opacity
        ) override;

    _Check_return_ HRESULT DrawEllipse(
        _In_ const XPOINTF &center,
        XFLOAT rRadiusX,
        XFLOAT rRadiusY,
        _In_ IPALAcceleratedBrush *pBrush,
        XFLOAT rStrokeWidth,
        _In_opt_ IPALStrokeStyle *pStrokeStyle,
        XFLOAT opacity
        ) override;

    _Check_return_ HRESULT DrawGlyphRun(
        _In_ const PALText::GlyphRun *pGlyphRun,
        _In_ IPALAcceleratedBrush *pBrush,
        XFLOAT opacity,
        UINT32 fontPaletteIndex,
        _In_ DWRITE_GLYPH_IMAGE_FORMATS glyphImageFormat
        ) override;

    _Check_return_ HRESULT PushOpacityLayer(
        XFLOAT rOpacity,
        _In_opt_ const XRECTF_RB *pContentBounds
        ) override;

    _Check_return_ HRESULT PushClipLayer(
        _In_ IPALAcceleratedGeometry *pPALClipGeometry,
        _In_opt_ const XRECTF_RB *pContentBounds
        ) override;

    _Check_return_ HRESULT PopLayer(
        ) override;

    void PushAxisAlignedClip(
        _In_ const XRECTF_RB *pClip
        ) override;

    void PopAxisAlignedClip() override;

    // Direct public methods
    _Check_return_ HRESULT SetDxgiTarget(
        _In_ IDXGISurface* pDXGISurface);

protected:
    CD2DRenderTarget();
    ~CD2DRenderTarget() override;

    void ResolveClipStack();

    CD2DFactory* m_pFactory;    // TODO: D2D: This is only needed for D2DPrintTarget, and it gets used to create a device context. Merge this with printing.
    ID2D1DeviceContext *m_pD2DDeviceContext;
    bool m_isEmpty;

    CClipLayerStack m_clipLayerStack;
};
