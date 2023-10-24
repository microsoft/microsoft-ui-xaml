// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      PC based implementation of ID2D1RenderTarget.

#pragma once

struct HWTextRenderParams;
struct D2DRenderContext;
class HWSolidColorBrush;
class HwTextBitmap;

//---------------------------------------------------------------------------
//
//  HWRenderTarget
//
//  PC based implementation of ID2D1RenderTarget.
//
//---------------------------------------------------------------------------
class HWRenderTarget final : public ID2D1RenderTarget, public ID2D1Factory, public ID2DRenderTargetRichEditExtensions
{
public:

    //
    // IUnknown interface
    //

    virtual HRESULT __stdcall QueryInterface(
        REFIID riid,
        _Outptr_ void **ppvObject
        );

    virtual ULONG __stdcall AddRef();

    virtual ULONG __stdcall Release();

    //
    // ID2D1Factory interface
    //

    virtual HRESULT __stdcall ReloadSystemMetrics();

    virtual void __stdcall GetDesktopDpi(
        _Out_ FLOAT *dpiX,
        _Out_ FLOAT *dpiY
        );

    virtual HRESULT __stdcall CreateRectangleGeometry(
        _In_ CONST D2D1_RECT_F *rectangle,
        _Outptr_ ID2D1RectangleGeometry **rectangleGeometry
        );

    virtual HRESULT __stdcall CreateRoundedRectangleGeometry(
        _In_ CONST D2D1_ROUNDED_RECT *roundedRectangle,
        _Outptr_ ID2D1RoundedRectangleGeometry **roundedRectangleGeometry
        );

    virtual HRESULT __stdcall CreateEllipseGeometry(
        _In_ CONST D2D1_ELLIPSE *ellipse,
        _Outptr_ ID2D1EllipseGeometry **ellipseGeometry
        );

    virtual HRESULT __stdcall CreateGeometryGroup(
        D2D1_FILL_MODE fillMode,
        _In_reads_(geometriesCount) ID2D1Geometry **geometries,
        UINT geometriesCount,
        _Outptr_ ID2D1GeometryGroup **geometryGroup
        );

    virtual HRESULT __stdcall CreateTransformedGeometry(
        _In_ ID2D1Geometry *sourceGeometry,
        _In_ CONST D2D1_MATRIX_3X2_F *transform,
        _Outptr_ ID2D1TransformedGeometry **transformedGeometry
        );

    virtual HRESULT __stdcall CreatePathGeometry(
        _Outptr_ ID2D1PathGeometry **pathGeometry
        );

    virtual HRESULT __stdcall CreateStrokeStyle(
        _In_ CONST D2D1_STROKE_STYLE_PROPERTIES *strokeStyleProperties,
        _In_reads_opt_(dashesCount) CONST FLOAT *dashes,
        UINT dashesCount,
        _Outptr_ ID2D1StrokeStyle **strokeStyle
        );

    virtual HRESULT __stdcall CreateDrawingStateBlock(
        _In_opt_ CONST D2D1_DRAWING_STATE_DESCRIPTION *drawingStateDescription,
        _In_opt_ IDWriteRenderingParams *textRenderingParams,
        _Outptr_ ID2D1DrawingStateBlock **drawingStateBlock
        );

    virtual HRESULT __stdcall CreateWicBitmapRenderTarget(
        _In_ IWICBitmap *target,
        _In_ CONST D2D1_RENDER_TARGET_PROPERTIES *renderTargetProperties,
        _Outptr_ ID2D1RenderTarget **renderTarget
        );

    virtual HRESULT __stdcall CreateHwndRenderTarget(
        _In_ CONST D2D1_RENDER_TARGET_PROPERTIES *renderTargetProperties,
        _In_ CONST D2D1_HWND_RENDER_TARGET_PROPERTIES *hwndRenderTargetProperties,
        _Outptr_ ID2D1HwndRenderTarget **hwndRenderTarget
        );

    virtual HRESULT __stdcall CreateDxgiSurfaceRenderTarget(
        _In_ IDXGISurface *dxgiSurface,
        _In_ CONST D2D1_RENDER_TARGET_PROPERTIES *renderTargetProperties,
        _Outptr_ ID2D1RenderTarget **renderTarget
        );

    virtual HRESULT __stdcall CreateDCRenderTarget(
        _In_ CONST D2D1_RENDER_TARGET_PROPERTIES *renderTargetProperties,
        _Outptr_ ID2D1DCRenderTarget **dcRenderTarget
        );

    //
    // ID2D1RenderTarget interface
    //

    virtual void __stdcall GetFactory(
        _Outptr_ ID2D1Factory **factory
        ) const;

    virtual HRESULT __stdcall CreateBitmap(
        D2D1_SIZE_U size,
        _In_opt_ CONST void *srcData,
        UINT32 pitch,
        _In_ CONST D2D1_BITMAP_PROPERTIES *bitmapProperties,
        _Outptr_ ID2D1Bitmap **bitmap
        );

    virtual HRESULT __stdcall CreateBitmapFromWicBitmap(
        _In_ IWICBitmapSource *wicBitmapSource,
        _In_opt_ CONST D2D1_BITMAP_PROPERTIES *bitmapProperties,
        _Outptr_ ID2D1Bitmap **bitmap
        );

    virtual HRESULT __stdcall CreateSharedBitmap(
        _In_ REFIID riid,
        _Inout_ void *data,
        _In_opt_ CONST D2D1_BITMAP_PROPERTIES *bitmapProperties,
        _Outptr_ ID2D1Bitmap **bitmap
        );

    virtual HRESULT __stdcall CreateBitmapBrush(
        _In_ ID2D1Bitmap *bitmap,
        _In_opt_ CONST D2D1_BITMAP_BRUSH_PROPERTIES *bitmapBrushProperties,
        _In_opt_ CONST D2D1_BRUSH_PROPERTIES *brushProperties,
        _Outptr_ ID2D1BitmapBrush **bitmapBrush
        );

    virtual HRESULT __stdcall CreateSolidColorBrush(
        _In_ CONST D2D1_COLOR_F *color,
        _In_opt_ CONST D2D1_BRUSH_PROPERTIES *brushProperties,
        _Outptr_ ID2D1SolidColorBrush **solidColorBrush
        );

    virtual HRESULT __stdcall CreateGradientStopCollection(
        _In_reads_(gradientStopsCount) CONST D2D1_GRADIENT_STOP *gradientStops,
        _In_range_(>=,1) UINT gradientStopsCount,
        D2D1_GAMMA colorInterpolationGamma,
        D2D1_EXTEND_MODE extendMode,
        _Outptr_ ID2D1GradientStopCollection **gradientStopCollection
        );

    virtual HRESULT __stdcall CreateLinearGradientBrush(
        _In_ CONST D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES *linearGradientBrushProperties,
        _In_opt_ CONST D2D1_BRUSH_PROPERTIES *brushProperties,
        _In_ ID2D1GradientStopCollection *gradientStopCollection,
        _Outptr_ ID2D1LinearGradientBrush **linearGradientBrush
        );

    virtual HRESULT __stdcall CreateRadialGradientBrush(
        _In_ CONST D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES *radialGradientBrushProperties,
        _In_opt_ CONST D2D1_BRUSH_PROPERTIES *brushProperties,
        _In_ ID2D1GradientStopCollection *gradientStopCollection,
        _Outptr_ ID2D1RadialGradientBrush **radialGradientBrush
        );

    virtual HRESULT __stdcall CreateCompatibleRenderTarget(
        _In_opt_ CONST D2D1_SIZE_F *desiredSize,
        _In_opt_ CONST D2D1_SIZE_U *desiredPixelSize,
        _In_opt_ CONST D2D1_PIXEL_FORMAT *desiredFormat,
        D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS options,
        _Outptr_ ID2D1BitmapRenderTarget **bitmapRenderTarget
        );

    virtual HRESULT __stdcall CreateLayer(
        _In_opt_ CONST D2D1_SIZE_F *size,
        _Outptr_ ID2D1Layer **layer
        );

    virtual HRESULT __stdcall CreateMesh(
        _Outptr_ ID2D1Mesh **mesh
        );

    virtual void __stdcall DrawLine(
        D2D1_POINT_2F point0,
        D2D1_POINT_2F point1,
        _In_ ID2D1Brush *brush,
        FLOAT strokeWidth = 1.0f,
        _In_opt_ ID2D1StrokeStyle *strokeStyle = NULL
        );

    virtual void __stdcall DrawRectangle(
        _In_ CONST D2D1_RECT_F *rect,
        _In_ ID2D1Brush *brush,
        FLOAT strokeWidth = 1.0f,
        _In_opt_ ID2D1StrokeStyle *strokeStyle = NULL
        );

    virtual void __stdcall FillRectangle(
        _In_ CONST D2D1_RECT_F *rect,
        _In_ ID2D1Brush *brush
        );

    virtual void __stdcall DrawRoundedRectangle(
        _In_ CONST D2D1_ROUNDED_RECT *roundedRect,
        _In_ ID2D1Brush *brush,
        FLOAT strokeWidth = 1.0f,
        _In_opt_ ID2D1StrokeStyle *strokeStyle = NULL
        );

    virtual void __stdcall FillRoundedRectangle(
        _In_ CONST D2D1_ROUNDED_RECT *roundedRect,
        _In_ ID2D1Brush *brush
        );

    virtual void __stdcall DrawEllipse(
        _In_ CONST D2D1_ELLIPSE *ellipse,
        _In_ ID2D1Brush *brush,
        FLOAT strokeWidth = 1.0f,
        _In_opt_ ID2D1StrokeStyle *strokeStyle = NULL
        );

    virtual void __stdcall FillEllipse(
        _In_ CONST D2D1_ELLIPSE *ellipse,
        _In_ ID2D1Brush *brush
        );

    virtual void __stdcall DrawGeometry(
        _In_ ID2D1Geometry *geometry,
        _In_ ID2D1Brush *brush,
        FLOAT strokeWidth = 1.0f,
        _In_opt_ ID2D1StrokeStyle *strokeStyle = NULL
        );

    virtual void __stdcall FillGeometry(
        _In_ ID2D1Geometry *geometry,
        _In_ ID2D1Brush *brush,
        _In_opt_ ID2D1Brush *opacityBrush = NULL
        );

    virtual void __stdcall FillMesh(
        _In_ ID2D1Mesh *mesh,
        _In_ ID2D1Brush *brush
        );

    virtual void __stdcall FillOpacityMask(
        _In_ ID2D1Bitmap *opacityMask,
        _In_ ID2D1Brush *brush,
        D2D1_OPACITY_MASK_CONTENT content,
        _In_opt_ CONST D2D1_RECT_F *destinationRectangle = NULL,
        _In_opt_ CONST D2D1_RECT_F *sourceRectangle = NULL
        );

    virtual void __stdcall DrawBitmap(
        _In_ ID2D1Bitmap *bitmap,
        _In_opt_ CONST D2D1_RECT_F *destinationRectangle = NULL,
        FLOAT opacity = 1.0f,
        D2D1_BITMAP_INTERPOLATION_MODE interpolationMode = D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
        _In_opt_ CONST D2D1_RECT_F *sourceRectangle = NULL
        );

    virtual void __stdcall DrawText(
        _In_reads_(stringLength) CONST WCHAR *string,
        UINT stringLength,
        _In_ IDWriteTextFormat *textFormat,
        _In_ CONST D2D1_RECT_F *layoutRect,
        _In_ ID2D1Brush *defaultForegroundBrush,
        D2D1_DRAW_TEXT_OPTIONS options = D2D1_DRAW_TEXT_OPTIONS_NONE,
        DWRITE_MEASURING_MODE measuringMode = DWRITE_MEASURING_MODE_NATURAL
        );

    virtual void __stdcall DrawTextLayout(
        D2D1_POINT_2F origin,
        _In_ IDWriteTextLayout *textLayout,
        _In_ ID2D1Brush *defaultForegroundBrush,
        D2D1_DRAW_TEXT_OPTIONS options = D2D1_DRAW_TEXT_OPTIONS_NONE
        );

    virtual void __stdcall DrawGlyphRun(
        D2D1_POINT_2F baselineOrigin,
        _In_ CONST DWRITE_GLYPH_RUN *glyphRun,
        _In_ ID2D1Brush *foregroundBrush,
        DWRITE_MEASURING_MODE measuringMode = DWRITE_MEASURING_MODE_NATURAL
        );

    virtual void __stdcall SetTransform(
        _In_ CONST D2D1_MATRIX_3X2_F *transform
        );

    virtual void __stdcall GetTransform(
        _Out_ D2D1_MATRIX_3X2_F *transform
        ) const;

    virtual void __stdcall SetAntialiasMode(
        D2D1_ANTIALIAS_MODE antialiasMode
        );

    virtual D2D1_ANTIALIAS_MODE __stdcall GetAntialiasMode() const;

    virtual void __stdcall SetTextAntialiasMode(
        D2D1_TEXT_ANTIALIAS_MODE textAntialiasMode
        );

    virtual D2D1_TEXT_ANTIALIAS_MODE __stdcall GetTextAntialiasMode() const;

    virtual void __stdcall SetTextRenderingParams(
        _In_opt_ IDWriteRenderingParams *textRenderingParams = NULL
        );

    virtual void __stdcall GetTextRenderingParams(
        _Outptr_result_maybenull_ IDWriteRenderingParams **textRenderingParams
        ) const;

    virtual void __stdcall SetTags(
        D2D1_TAG tag1,
        D2D1_TAG tag2
        );

    virtual void __stdcall GetTags(
        _Out_opt_ D2D1_TAG *tag1 = NULL,
        _Out_opt_ D2D1_TAG *tag2 = NULL
        ) const;

    virtual void __stdcall PushLayer(
        _In_ CONST D2D1_LAYER_PARAMETERS *layerParameters,
        _In_opt_ ID2D1Layer *layer
        );

    virtual void __stdcall PopLayer();

    virtual HRESULT __stdcall Flush(
        _Out_opt_ D2D1_TAG *tag1 = NULL,
        _Out_opt_ D2D1_TAG *tag2 = NULL
        );

    virtual void __stdcall SaveDrawingState(
        _Inout_ ID2D1DrawingStateBlock *drawingStateBlock
        ) const;

    virtual void __stdcall RestoreDrawingState(
        _In_ ID2D1DrawingStateBlock *drawingStateBlock
        );

    virtual void __stdcall PushAxisAlignedClip(
        _In_ CONST D2D1_RECT_F *clipRect,
        D2D1_ANTIALIAS_MODE antialiasMode
        );

    virtual void __stdcall PopAxisAlignedClip();

    virtual void __stdcall Clear(
        _In_opt_ CONST D2D1_COLOR_F *clearColor = NULL
        );

    virtual void __stdcall BeginDraw();

    virtual HRESULT __stdcall EndDraw(
        _Out_opt_ D2D1_TAG *tag1 = NULL,
        _Out_opt_ D2D1_TAG *tag2 = NULL
        );

    virtual D2D1_PIXEL_FORMAT __stdcall GetPixelFormat() const;

    virtual void __stdcall SetDpi(
        FLOAT dpiX,
        FLOAT dpiY
        );

    virtual void __stdcall GetDpi(
        _Out_ FLOAT *dpiX,
        _Out_ FLOAT *dpiY
        ) const;

    virtual D2D1_SIZE_F __stdcall GetSize() const;

    virtual D2D1_SIZE_U __stdcall GetPixelSize() const;

    virtual UINT32 __stdcall GetMaximumBitmapSize() const;

    virtual BOOL __stdcall IsSupported(
        _In_ CONST D2D1_RENDER_TARGET_PROPERTIES *renderTargetProperties
        ) const;

    //
    // HWRenderTarget implementation
    //

    // Initializes a new instance of the HWRenderTarget class.
    static HRESULT Create(
        _In_ CTextCore *pTextCore,
        _Outptr_ HWRenderTarget **ppRenderTarget
        );

    // PC based rendering
    _Check_return_ HRESULT HWRender(
        _In_ IContentRenderer* pContentRenderer
        );

    // Clears the rendering data.
    void InvalidateContent();
    void InvalidateRenderCache();

    // Clears the rendering data within specified region.
    void InvalidateRegion(const XRECT_RB &region);

    void CleanupRealizations();

    // Sets whether color in fonts should be enabled for text drawing operations
    void SetIsColorFontEnabled(bool isColorFontEnabled);

    // ID2DRenderTargetRichEditExtensions interface
    bool DrawUnderline(BYTE underlineType, DWORD color, const RECT* fillrect);

    void SetForegroundAlpha(BYTE alpha) { m_alpha = alpha; }
    void SetOverrideIgnoreTextColorAlphaValue(const bool shouldUseTextAlpha) { m_overrideShouldIgnoreTextAlpha = shouldUseTextAlpha; }

    bool LastDrawGlyphRunHasClipping() const { return m_fLastDrawGlyphRunHasClipping; }

    void SetBackPlateConfiguration(bool backPlateActive, bool useHyperlinkForeground);

    HRESULT DrawBaseLine(long baseLineY, long left, long right, UINT32 crText);

private:

    // Count of references to this instance.
    XUINT32 m_referenceCount;

    // Text drawing context.
    D2DTextDrawingContext *m_pTextDrawingContext;

    // Stack of clipping rectangles applied to the drawing surface by RichEdit.
    xvector<XRECTF_RB> m_clipRects;

    // Clipping rectangle applied to the drawing surface.
    XRECTF_RB m_clipRect;

    CCoreServices* m_pCore;

    BYTE m_alpha;
    bool m_overrideShouldIgnoreTextAlpha;

    bool m_fLastDrawGlyphRunHasClipping;

    // Initializes a new instance of the HWRenderTarget class.
    HWRenderTarget(_In_ CTextCore *pTextCore);

    // Release resources associated with the HWRenderTarget.
    ~HWRenderTarget();

    // Gets the current clipping rectangle by intersecting all pushed clip rects.
    bool GetCurrentClipRect(
        _Out_ XRECTF_RB *pClipRect
        ) const;
};