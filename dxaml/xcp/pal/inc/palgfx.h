// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
//      Contains the basic Graphics support for the core platform
//      abstraction layer

#ifndef __PAL__GFX__
#define __PAL__GFX__

#include <vector>
#include "Refcounting.h"
#include "xref_ptr.h"
#include "Matrix.h"

enum PixelFormat;

enum DWRITE_GLYPH_IMAGE_FORMATS;

struct IUnknown;
struct ID2D1SvgDocument;
struct ID2D1DeviceContext5;

//------------------------------------------------------------------------------
//
//  Synopsis:
//      An abstraction for surfaces created and consumed by graphics devices.
//
//------------------------------------------------------------------------------
struct CNotifyOnDelete;

// TODO: Move this to components/graphics and rename it everywhere in XAML.
//                 This will need to be done in an isolated change along with cleaning
//                 it up with appropriate types.  In addition, change it to a ctl::interface<IUnknown> with uuid
struct IPALSurface : public IObject
{
    virtual _Check_return_ HRESULT Lock(
        _Outptr_result_bytebuffer_(*pnStride * *puHeight) void **ppAddress,
        _Out_ XINT32 *pnStride,
        _Out_ XUINT32 *puWidth,
        _Out_ XUINT32 *puHeight) = 0;

    virtual _Check_return_ HRESULT Unlock() = 0;

    virtual XUINT32 GetWidth() const = 0;
    virtual XUINT32 GetHeight() const = 0;
    virtual PixelFormat GetPixelFormat() = 0;
    virtual bool IsOpaque() = 0;
    virtual bool IsVirtual() = 0;
    virtual void SetIsOpaque(bool isOpaque) = 0;
    virtual HRESULT GetNotifyOnDelete(_Outptr_ CNotifyOnDelete **ppNotifyOnDelete) = 0;

    // Surfaces can be offered.  Not every surface type can support this.  Those that cannot
    // should return S_OK and set pWasDiscarded = FALSE in Reclaim
    // WasDiscarded matches the semantic of the DXGI ReclaimResources API which returns TRUE to indicate
    // the backing memory was discarded or FALSE if the content is still intact
    virtual HRESULT Offer() = 0;
    virtual HRESULT Reclaim(_Out_ bool *pWasDiscarded) = 0;
};

struct IPALByteAccessSurface : public IObject
{
    virtual _Check_return_ HRESULT ReadBytes(
        _Out_ XUINT32 &length,
        _Outptr_result_bytebuffer_(length) XBYTE **ppBytes) = 0;
};

struct IPALAcceleratedGraphicsFactory;

struct IPALEvent;

class HWCompTreeNode;
class DCompSurface;

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Interface used to obtain graphics devices.
//
//------------------------------------------------------------------------------
class DCompTreeHost;

#if DBG
struct IPALDebugDeviceFinalReleaseAsserter : public IObject
{
    virtual void ReleaseAllWithAssert() = 0;
};
#endif

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Graphics device dependent resources can register themselves with
//      the core services object for device change notifications.
//
//------------------------------------------------------------------------------
struct IPALGraphicsDeviceChangeListener
{
    virtual void OnGraphicsDeviceChanged() = 0;
};

// Assume a refresh rate of 60Hz before we can query the system for the actual refresh rate.
const XFLOAT DefaultRefreshIntervalInMilliseconds = 1000.0f / 60.0f;

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Refresh rate abstraction
//
//------------------------------------------------------------------------------
struct IPALRefreshRateInfo : public IObject
{
    virtual XFLOAT GetRefreshIntervalInMilliseconds() = 0;
    virtual _Check_return_ HRESULT WaitForRefreshInterval() = 0;
    virtual bool IsValid() = 0;
};

enum class InterpolationMode
{
    Gamma_2_2,
    Gamma_1_0
};

enum class GradientWrapMode
{
    Clamp,
    Wrap,
    Mirror
};

enum class BrushType
{
    Solid,
    Bitmap,
    LinearGradient,
    RadialGradient
};

enum class GeometryType
{
    Generic,
    Rectangle,
    RoundedRectangle,
    Ellipse,
    Path,
    Group,
    Transformed
};

enum class GeometryCombineMode
{
    Union,
    Intersect,
    Xor,
    Exclude
};

enum class LineCapStyle
{
    Flat,
    Square,
    Round,
    Triangle
};

enum class LineJoin
{
    Miter,
    Bevel,
    Round,
    MiterOrBevel
};

enum class DashStyle
{
    Solid,
    Dash,
    Dot,
    DashDot,
    DashDotDot,
    Custom
};

//------------------------------------------------------------------------------
//
//  Synopsis:
//      The fill mode of a geometry. Enum order must match D2D1_FILL_MODE.
//
//------------------------------------------------------------------------------
enum class GeometryFillMode
{
    Alternate,
    Winding
};

enum class PALPreviewPageCountType
{
    Final,
    Intermediate
};


struct IPALAcceleratedGraphicsFactory;
struct IPALAcceleratedBrush;
struct IPALAcceleratedBitmapBrush;
struct XcpGradientStop;
struct IPALAcceleratedRender;
struct IPALAcceleratedBrush;
struct IPALAcceleratedBitmap;
namespace PALText { struct GlyphRun; }


//------------------------------------------------------------------------------
//
//  Synopsis:
//      PAL interface for ID2D1StrokeStyle.
//
//------------------------------------------------------------------------------
struct IPALStrokeStyle : public IObject
{
};


//------------------------------------------------------------------------------
//
//  Synopsis:
//      PAL interface for ID2D1Geometry.
//
//------------------------------------------------------------------------------
struct IPALAcceleratedGeometry : public IObject
{
    virtual GeometryType GetType() = 0;

    virtual _Check_return_ HRESULT GetBounds(
        _Out_ XRECTF_RB *pBounds
        ) = 0;

    virtual _Check_return_ HRESULT GetWidenedBounds(
        XFLOAT rStrokeWidth,
        _Out_ XRECTF_RB *pBounds
        ) = 0;

    virtual _Check_return_ HRESULT Fill(
        _In_ IPALAcceleratedRender *pRenderTarget,
        _In_ IPALAcceleratedBrush *pBrush,
        XFLOAT opacity
        ) = 0;

    virtual _Check_return_ HRESULT Draw(
        _In_ IPALAcceleratedRender *pRenderTarget,
        _In_ IPALAcceleratedBrush *pBrush,
        XFLOAT rStrokeWidth,
        _In_opt_ IPALStrokeStyle *pStrokeStyle,
        XFLOAT opacity
        ) = 0;

    virtual bool IsAxisAlignedRectangle() = 0;
};


//------------------------------------------------------------------------------
//
//  Synopsis:
//      PAL interface for ID2D1GeometrySink. Used to build a path geometry.
//
//------------------------------------------------------------------------------
struct IPALGeometrySink : public IObject
{
    virtual void BeginFigure(
        const XPOINTF& startPoint,
        bool fIsHollow
        ) = 0;

    virtual void EndFigure(
        bool fIsClosed
        ) = 0;

    virtual void AddArc(
        const XPOINTF& point,
        const XSIZEF& size,
        XFLOAT rotationAngle,
        bool fIsClockwise,
        bool fIsLargeArc
        ) = 0;

    virtual void AddBezier(
        const XPOINTF& controlPoint1,
        const XPOINTF& controlPoint2,
        const XPOINTF& endPoint
        ) = 0;

    virtual void AddLine(
        const XPOINTF& point
        ) = 0;

    virtual void AddQuadraticBezier(
        const XPOINTF& controlPoint,
        const XPOINTF& endPoint
        ) = 0;

    virtual void AddLines(
        _In_reads_(uiCount) const XPOINTF *pPoints,
        XUINT32 uiCount
        ) = 0;

    virtual void AddBeziers(
        _In_reads_(uiCount) const XPOINTF *pPoints,
        XUINT32 uiCount
        ) = 0;

    virtual void AddQuadraticBeziers(
        _In_reads_(uiCount) const XPOINTF *pPoints,
        XUINT32 uiCount
        ) = 0;

    virtual void SetFillMode(
        GeometryFillMode fillMode
        ) = 0;

    virtual _Check_return_ HRESULT Close() = 0;
};


//------------------------------------------------------------------------------
//
//  Synopsis:
//      PAL interface for ID2D1PathGeometry.
//
//------------------------------------------------------------------------------
struct IPALAcceleratedPathGeometry : public IPALAcceleratedGeometry
{
    virtual _Check_return_ HRESULT Open(
        _Outptr_ IPALGeometrySink **ppPALGeometrySink
        ) = 0;
};

//------------------------------------------------------------------------------
//
//  Synopsis:
//      D2D rendering interface. Implemented by the render target to accept D2D
//      draw calls during the D2D render walk.
//
//------------------------------------------------------------------------------
struct IPALAcceleratedRender
{
    virtual _Check_return_ HRESULT SetTransform(
        _In_ const CMILMatrix* pMatrix
        ) = 0;

    virtual _Check_return_ HRESULT FillGeometry(
        _In_ IPALAcceleratedGeometry *pGeometry,
        _In_ IPALAcceleratedBrush *pBrush,
        XFLOAT opacity
        ) = 0;

    virtual _Check_return_ HRESULT DrawGeometry(
        _In_ IPALAcceleratedGeometry *pGeometry,
        _In_ IPALAcceleratedBrush *pBrush,
        XFLOAT rStrokeWidth,
        _In_opt_ IPALStrokeStyle *pStrokeStyle,
        XFLOAT opacity
        ) = 0;

    virtual _Check_return_ HRESULT FillRectangle(
        _In_ const XRECTF_RB &rect,
        _In_ IPALAcceleratedBrush *pBrush,
        XFLOAT opacity
        ) = 0;

    virtual _Check_return_ HRESULT DrawRectangle(
        _In_ const XRECTF_RB &rect,
        _In_ IPALAcceleratedBrush *pBrush,
        XFLOAT rStrokeWidth,
        _In_opt_ IPALStrokeStyle *pStrokeStyle,
        XFLOAT opacity
        ) = 0;

    virtual _Check_return_ HRESULT FillRoundedRectangle(
        _In_ const XRECTF_RB &rect,
        XFLOAT rRadiusX,
        XFLOAT rRadiusY,
        _In_ IPALAcceleratedBrush *pBrush,
        XFLOAT opacity
        ) = 0;

    virtual _Check_return_ HRESULT DrawRoundedRectangle(
        _In_ const XRECTF_RB &rect,
        XFLOAT rRadiusX,
        XFLOAT rRadiusY,
        _In_ IPALAcceleratedBrush *pBrush,
        XFLOAT rStrokeWidth,
        _In_opt_ IPALStrokeStyle *pStrokeStyle,
        XFLOAT opacity
        ) = 0;

    virtual _Check_return_ HRESULT FillEllipse(
        _In_ const XPOINTF &center,
        XFLOAT rRadiusX,
        XFLOAT rRadiusY,
        _In_ IPALAcceleratedBrush *pBrush,
        XFLOAT opacity
        ) = 0;

    virtual _Check_return_ HRESULT DrawEllipse(
        _In_ const XPOINTF &center,
        XFLOAT rRadiusX,
        XFLOAT rRadiusY,
        _In_ IPALAcceleratedBrush *pBrush,
        XFLOAT rStrokeWidth,
        _In_opt_ IPALStrokeStyle *pStrokeStyle,
        XFLOAT opacity
        ) = 0;

    virtual _Check_return_ HRESULT DrawGlyphRun(
        _In_ const PALText::GlyphRun *pGlyphRun,
        _In_ IPALAcceleratedBrush *pBrush,
        XFLOAT opacity,
        UINT32 fontPaletteIndex,
        _In_ DWRITE_GLYPH_IMAGE_FORMATS glyphImageFormat
        ) = 0;

    virtual _Check_return_ HRESULT PushOpacityLayer(
        XFLOAT rOpacity,
        _In_ const XRECTF_RB *pContentBounds
        ) = 0;

    virtual _Check_return_ HRESULT PushClipLayer(
        _In_ IPALAcceleratedGeometry *pPALClipGeometry,
        _In_opt_ const XRECTF_RB *pContentBounds
        ) = 0;

    virtual _Check_return_ HRESULT PopLayer(
        ) = 0;

    virtual void PushAxisAlignedClip(
        _In_ const XRECTF_RB *pClip
        ) = 0;

    virtual void PopAxisAlignedClip(
        ) = 0;

};


//------------------------------------------------------------------------------
//
//  Synopsis:
//      PAL interface for creating D2D render targets.
//
//------------------------------------------------------------------------------
struct IPALAcceleratedGraphicsFactory : public IObject
{
    virtual _Check_return_ HRESULT CreateStrokeStyle(
        LineCapStyle startCap,
        LineCapStyle endCap,
        LineCapStyle dashCap,
        LineJoin lineJoin,
        XFLOAT fMiterLimit,
        DashStyle dashStyle,
        XFLOAT fDashOffset,
        _In_opt_ const std::vector<float>* dashes,
        _Outptr_ IPALStrokeStyle **ppPALStrokeStyle
        ) = 0;

    virtual _Check_return_ HRESULT CreateRectangleGeometry(
        _In_ const XRECTF &rect,
        _Outptr_ IPALAcceleratedGeometry **ppGeometry
        ) = 0;

    virtual _Check_return_ HRESULT CreateRoundedRectangleGeometry(
        _In_ const XRECTF &rect,
        XFLOAT rRadiusX,
        XFLOAT rRadiusY,
        _Outptr_ IPALAcceleratedGeometry **ppGeometry
        ) = 0;

    virtual _Check_return_ HRESULT CreateEllipseGeometry(
        _In_ const XPOINTF &center,
        XFLOAT rRadiusX,
        XFLOAT rRadiusY,
        _Outptr_ IPALAcceleratedGeometry **ppPALGeometry
        ) = 0;

    virtual _Check_return_ HRESULT CreatePathGeometry(
        _Outptr_ IPALAcceleratedPathGeometry **ppPALGeometry
        ) = 0;

    virtual _Check_return_ HRESULT CreateGeometryGroup(
        bool fIsWinding,
        _In_reads_(uiCount) IPALAcceleratedGeometry **ppPALGeometries,
        XUINT32 uiCount,
        _Outptr_ IPALAcceleratedGeometry **ppPALGeometry
        ) = 0;

    virtual _Check_return_ HRESULT CreateTransformedGeometry(
        _In_ IPALAcceleratedGeometry *pGeometry,
        _In_ const CMILMatrix* pMatrix,
        _Outptr_ IPALAcceleratedGeometry **ppPALGeometry
        ) = 0;

    virtual _Check_return_ HRESULT CombineGeometry(
        _In_ IPALAcceleratedGeometry *pGeometry1,
        _In_ IPALAcceleratedGeometry *pGeometry2,
        GeometryCombineMode combineMode,
        _Outptr_ IPALAcceleratedGeometry **ppPALGeometry
        ) = 0;
};

//------------------------------------------------------------------------------
//
//  Synopsis:
//      PAL interface for ID2D1RenderTarget.
//
//------------------------------------------------------------------------------
struct IPALAcceleratedRenderTarget
    : public IObject
    , public IPALAcceleratedRender
{
    virtual _Check_return_ HRESULT BeginDraw() = 0;
    virtual _Check_return_ HRESULT EndDraw() = 0;

    virtual XUINT32 GetWidth() = 0;
    virtual XUINT32 GetHeight() = 0;

    virtual _Check_return_ HRESULT CreateBitmap(
        _In_ IPALSurface *pBitmapSurface,
        _Outptr_ IPALAcceleratedBitmap **ppPALBitmap
        ) = 0;

    virtual _Check_return_ HRESULT CreateSolidColorBrush(
        XUINT32 uiColor,
        XFLOAT rOpacity,
        _Outptr_ IPALAcceleratedBrush **ppPALBrush
        ) = 0;

    virtual _Check_return_ HRESULT CreateBitmapBrush(
        _In_ ID2D1SvgDocument* d2dSvgDocument,
        XFLOAT opacity,
        uint32_t width,
        uint32_t height,
        _Outptr_ IPALAcceleratedBrush** palBrushOut
        ) = 0;

    virtual _Check_return_ HRESULT CreateBitmapBrush(
        _In_ IPALSurface *pBitmapSurface,
        XFLOAT rOpacity,
        _Outptr_ IPALAcceleratedBrush **ppPALBrush
        ) = 0;

    virtual _Check_return_ HRESULT CreateBitmapBrush(
        _In_ IPALAcceleratedBitmapBrush *pPALBitmapBrush,
        XFLOAT rOpacity,
        _Outptr_ IPALAcceleratedBrush **ppPALBrush
        ) = 0;

    virtual _Check_return_ HRESULT CreateSharedBitmapBrush(
        _In_ IPALAcceleratedBitmap *pAcceleratedBitmap,
        XFLOAT rOpacity,
        _Outptr_ IPALAcceleratedBrush **ppPALBrush
        ) = 0;

    virtual _Check_return_ HRESULT CreateLinearGradientBrush(
        XPOINTF startPoint,
        XPOINTF endPoint,
        _In_reads_(gradientStopsCount) XcpGradientStop *pGradientStopsArray,
        XUINT32 gradientStopsCount,
        InterpolationMode interpolationMode,
        GradientWrapMode gradientWrapMode,
        XFLOAT rOpacity,
        _Outptr_ IPALAcceleratedBrush **ppPALBrush
        ) = 0;

    virtual _Check_return_ HRESULT CreateRadialGradientBrush(
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
        ) = 0;

    virtual _Check_return_ HRESULT GetDeviceContext(_Outptr_ ID2D1DeviceContext5 **ppD2DDeviceContext) const = 0;
};

struct IPALPrintingData;
struct IPALWorkItem;

struct IPALPrintTarget : public IPALAcceleratedRenderTarget
{
    virtual _Check_return_ HRESULT BeginPreview(_In_  IPALPrintingData* pPreviewData) = 0;
    virtual _Check_return_ HRESULT InvalidatePreview() = 0;
    virtual _Check_return_ HRESULT EndPreview() = 0;
    virtual _Check_return_ HRESULT SetPreviewPageCount(PALPreviewPageCountType pageCountType, XINT32 pageCount) = 0;
    virtual _Check_return_ HRESULT BeginPrint(_In_ IPALPrintingData* pPD) = 0;
    virtual _Check_return_ HRESULT EndPrint() = 0;
    virtual _Check_return_ HRESULT BeginPage(XINT32 pageNumber) = 0;
    virtual _Check_return_ HRESULT EndPage() = 0;
    virtual _Check_return_ HRESULT CancelPage() = 0;
};

//------------------------------------------------------------------------------
//
//  Synopsis:
//      PAL interface for ID2D1Brush.
//
//------------------------------------------------------------------------------
struct IPALAcceleratedBrush : public IObject
{
    virtual BrushType GetType() = 0;

    virtual _Check_return_ HRESULT SetTransform(_In_ const CMILMatrix* pMatrix) = 0;
    virtual _Check_return_ HRESULT SetOpacity(XFLOAT Opacity) = 0;
};


//------------------------------------------------------------------------------
//
//  Synopsis:
//      PAL interface for ID2D1SolidColorBrush.
//
//------------------------------------------------------------------------------
struct IPALAcceleratedSolidColorBrush : public IPALAcceleratedBrush
{
    virtual XUINT32 GetColor() = 0;
};


//------------------------------------------------------------------------------
//
//  Synopsis:
//      PAL interface for ID2D1BitmapBrush.
//
//------------------------------------------------------------------------------
struct IPALAcceleratedBitmapBrush : public IPALAcceleratedBrush
{
};


//------------------------------------------------------------------------------
//
//  Synopsis:
//      PAL interface for ID2D1LinearGradientBrush.
//
//------------------------------------------------------------------------------
struct IPALAcceleratedLinearGradientBrush : public IPALAcceleratedBrush
{
};

//------------------------------------------------------------------------------
//
//  Synopsis:
//      PAL interface for ID2D1Bitmap
//
//------------------------------------------------------------------------------
struct IPALAcceleratedBitmap : public IObject
{
    virtual XUINT32 GetWidth() = 0;
    virtual XUINT32 GetHeight() = 0;
};

enum DCOMPOSITION_COMPOSITE_MODE;

template <typename TData>
class xvector;
class CSwapChainElement;

#endif //#ifndef __PAL__GFX__
