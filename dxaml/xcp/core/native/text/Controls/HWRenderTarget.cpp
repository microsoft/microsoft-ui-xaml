// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "HWRenderTarget.h"
#include "HWSolidColorBrush.h"
#include "HWStrokeStyle.h"
#include "D2DTextDrawingContext.h"
#include "TextHelpers.h"
#include "HwTextBitmap.h"
#include "float.h"
#include "ColorUtil.h"
#include "KnownColors.h"

//---------------------------------------------------------------------------
//
//  Creates and initializes a new instance of the WinTextCore class.
//
//---------------------------------------------------------------------------
HRESULT HWRenderTarget::Create(
    _In_ CTextCore *pTextCore,
    _Outptr_ HWRenderTarget **ppRenderTarget
    )
{
    HRESULT hr = S_OK;
    HWRenderTarget *pRenderTarget = NULL;
    pRenderTarget = new HWRenderTarget(pTextCore);
    pRenderTarget->m_pTextDrawingContext = new D2DTextDrawingContext(pTextCore);
    *ppRenderTarget = pRenderTarget;
    RRETURN(hr);//RRETURN_REMOVAL
}

//---------------------------------------------------------------------------
//
//  Initializes a new instance of the HWRenderTarget class.
//
//---------------------------------------------------------------------------
HWRenderTarget::HWRenderTarget(
    _In_ CTextCore *pTextCore
    )
    : m_referenceCount(1)
    , m_pTextDrawingContext(NULL)
    , m_pCore(pTextCore->GetCoreServices())
    , m_alpha(0xFF)
    , m_overrideShouldIgnoreTextAlpha(false)
    , m_fLastDrawGlyphRunHasClipping(false)
{
    XCP_WEAK(&m_pCore);
    EmptyRectF(&m_clipRect);
}

//---------------------------------------------------------------------------
//
//  Release resources associated with the HWRenderTarget.
//
//---------------------------------------------------------------------------
HWRenderTarget::~HWRenderTarget()
{
    ReleaseInterface(m_pTextDrawingContext);
}

HRESULT HWRenderTarget::QueryInterface(
    REFIID riid,
    _Outptr_ void **ppvObject
    )
{
    if (__uuidof(ID2DRenderTargetRichEditExtensions) == riid)
    {
        *ppvObject = static_cast<ID2DRenderTargetRichEditExtensions*>(this);
    }
    else if (__uuidof(IUnknown) == riid)
    {
        *ppvObject = static_cast<IUnknown*>(static_cast<ID2D1RenderTarget*>(this));
    }
    else if (__uuidof(ID2D1RenderTarget) == riid)
    {
        *ppvObject = static_cast<ID2D1RenderTarget*>(this);
    }
    else if (__uuidof(ID2D1Factory) == riid)
    {
        *ppvObject = static_cast<ID2D1Factory*>(this);
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    AddRef();

    return S_OK;

}

ULONG HWRenderTarget::AddRef()
{
    return ++m_referenceCount;
}

ULONG HWRenderTarget::Release()
{
    ASSERT(m_referenceCount > 0);

    XUINT32 referenceCount = --m_referenceCount;
    if (0 == m_referenceCount)
    {
        delete this;
    }
    return referenceCount;
}

HRESULT HWRenderTarget::ReloadSystemMetrics()
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

void HWRenderTarget::GetDesktopDpi(
    _Out_ FLOAT *dpiX,
    _Out_ FLOAT *dpiY
    )
{
    ASSERT(FALSE);
}

HRESULT HWRenderTarget::CreateRectangleGeometry(
    _In_ CONST D2D1_RECT_F *rectangle,
    _Outptr_ ID2D1RectangleGeometry **rectangleGeometry
    )
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

HRESULT HWRenderTarget::CreateRoundedRectangleGeometry(
    _In_ CONST D2D1_ROUNDED_RECT *roundedRectangle,
    _Outptr_ ID2D1RoundedRectangleGeometry **roundedRectangleGeometry
    )
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

HRESULT HWRenderTarget::CreateEllipseGeometry(
    _In_ CONST D2D1_ELLIPSE *ellipse,
    _Outptr_ ID2D1EllipseGeometry **ellipseGeometry
    )
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

HRESULT HWRenderTarget::CreateGeometryGroup(
    D2D1_FILL_MODE fillMode,
    _In_reads_(geometriesCount) ID2D1Geometry **geometries,
    UINT geometriesCount,
    _Outptr_ ID2D1GeometryGroup **geometryGroup
    )
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

HRESULT HWRenderTarget::CreateTransformedGeometry(
    _In_ ID2D1Geometry *sourceGeometry,
    _In_ CONST D2D1_MATRIX_3X2_F *transform,
    _Outptr_ ID2D1TransformedGeometry **transformedGeometry
    )
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

HRESULT HWRenderTarget::CreatePathGeometry(
    _Outptr_ ID2D1PathGeometry **pathGeometry
    )
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

HRESULT HWRenderTarget::CreateStrokeStyle(
    _In_ CONST D2D1_STROKE_STYLE_PROPERTIES *strokeStyleProperties,
    _In_reads_opt_(dashesCount) CONST FLOAT *dashes,
    UINT dashesCount,
    _Outptr_ ID2D1StrokeStyle **strokeStyle
    )
{

    IFCPTR_RETURN(strokeStyleProperties);
    xref_ptr<HWStrokeStyle> pStrokeStyle;
    pStrokeStyle.attach(new HWStrokeStyle(strokeStyleProperties));
    if (dashes != NULL && dashesCount > 0)
    {
        IFC_RETURN(pStrokeStyle->SetDashes(dashes, dashesCount));
    }
    *strokeStyle = pStrokeStyle.detach();

    return S_OK;
}

HRESULT HWRenderTarget::CreateDrawingStateBlock(
    _In_opt_ CONST D2D1_DRAWING_STATE_DESCRIPTION *drawingStateDescription,
    _In_opt_ IDWriteRenderingParams *textRenderingParams,
    _Outptr_ ID2D1DrawingStateBlock **drawingStateBlock
    )
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

HRESULT HWRenderTarget::CreateWicBitmapRenderTarget(
    _In_ IWICBitmap *target,
    _In_ CONST D2D1_RENDER_TARGET_PROPERTIES *renderTargetProperties,
    _Outptr_ ID2D1RenderTarget **renderTarget
    )
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

HRESULT HWRenderTarget::CreateHwndRenderTarget(
    _In_ CONST D2D1_RENDER_TARGET_PROPERTIES *renderTargetProperties,
    _In_ CONST D2D1_HWND_RENDER_TARGET_PROPERTIES *hwndRenderTargetProperties,
    _Outptr_ ID2D1HwndRenderTarget **hwndRenderTarget
    )
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

HRESULT HWRenderTarget::CreateDxgiSurfaceRenderTarget(
    _In_ IDXGISurface *dxgiSurface,
    _In_ CONST D2D1_RENDER_TARGET_PROPERTIES *renderTargetProperties,
    _Outptr_ ID2D1RenderTarget **renderTarget
    )
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

HRESULT HWRenderTarget::CreateDCRenderTarget(
    _In_ CONST D2D1_RENDER_TARGET_PROPERTIES *renderTargetProperties,
    _Outptr_ ID2D1DCRenderTarget **dcRenderTarget
    )
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

void HWRenderTarget::GetFactory(
    _Outptr_ ID2D1Factory **factory
    ) const
{
    HWRenderTarget *pThis = const_cast<HWRenderTarget *>(this);
    *factory = static_cast<ID2D1Factory *>(pThis);
    AddRefInterface(pThis);
}

HRESULT HWRenderTarget::CreateBitmap(
    D2D1_SIZE_U size,
    _In_opt_ CONST void *srcData,
    UINT32 pitch,
    _In_ CONST D2D1_BITMAP_PROPERTIES *bitmapProperties,
    _Outptr_ ID2D1Bitmap **bitmap
    )
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

HRESULT HWRenderTarget::CreateBitmapFromWicBitmap(
    _In_ IWICBitmapSource *wicBitmapSource,
    _In_opt_ CONST D2D1_BITMAP_PROPERTIES *bitmapProperties,
    _Outptr_ ID2D1Bitmap **bitmap
    )
{
    xref_ptr<HwTextBitmap> pHwTextBitmap;

    // Generate a fake ID2D1Bitmap that wraps the bits retrieved from
    // the WIC bitmap.
    IFC_RETURN(HwTextBitmap::Create(wicBitmapSource, m_pCore, pHwTextBitmap.ReleaseAndGetAddressOf()));
    *bitmap = pHwTextBitmap.detach();

    return S_OK;
}

HRESULT HWRenderTarget::CreateSharedBitmap(
    _In_ REFIID riid,
    _Inout_ void *data,
    _In_opt_ CONST D2D1_BITMAP_PROPERTIES *bitmapProperties,
    _Outptr_ ID2D1Bitmap **bitmap
    )
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

HRESULT HWRenderTarget::CreateBitmapBrush(
    _In_ ID2D1Bitmap *bitmap,
    _In_opt_ CONST D2D1_BITMAP_BRUSH_PROPERTIES *bitmapBrushProperties,
    _In_opt_ CONST D2D1_BRUSH_PROPERTIES *brushProperties,
    _Outptr_ ID2D1BitmapBrush **bitmapBrush
    )
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

HRESULT HWRenderTarget::CreateSolidColorBrush(
    _In_ CONST D2D1_COLOR_F *color,
    _In_opt_ CONST D2D1_BRUSH_PROPERTIES *brushProperties,
    _Outptr_ ID2D1SolidColorBrush **solidColorBrush
    )
{
    HRESULT hr = S_OK;
    HWSolidColorBrush *pSolidColorBrush = NULL;

    pSolidColorBrush = new HWSolidColorBrush(color);
    *solidColorBrush = pSolidColorBrush;
    RRETURN(hr);//RRETURN_REMOVAL
}

HRESULT HWRenderTarget::CreateGradientStopCollection(
    _In_reads_(gradientStopsCount) const D2D1_GRADIENT_STOP *gradientStops,
    _In_range_(>=,1) UINT gradientStopsCount,
    D2D1_GAMMA colorInterpolationGamma,
    D2D1_EXTEND_MODE extendMode,
    _Outptr_ ID2D1GradientStopCollection **gradientStopCollection
    )
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

HRESULT HWRenderTarget::CreateLinearGradientBrush(
    _In_ CONST D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES *linearGradientBrushProperties,
    _In_opt_ CONST D2D1_BRUSH_PROPERTIES *brushProperties,
    _In_ ID2D1GradientStopCollection *gradientStopCollection,
    _Outptr_ ID2D1LinearGradientBrush **linearGradientBrush
    )
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

HRESULT HWRenderTarget::CreateRadialGradientBrush(
    _In_ CONST D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES *radialGradientBrushProperties,
    _In_opt_ CONST D2D1_BRUSH_PROPERTIES *brushProperties,
    _In_ ID2D1GradientStopCollection *gradientStopCollection,
    _Outptr_ ID2D1RadialGradientBrush **radialGradientBrush
    )
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

HRESULT HWRenderTarget::CreateCompatibleRenderTarget(
    _In_opt_ CONST D2D1_SIZE_F *desiredSize,
    _In_opt_ CONST D2D1_SIZE_U *desiredPixelSize,
    _In_opt_ CONST D2D1_PIXEL_FORMAT *desiredFormat,
    D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS options,
    _Outptr_ ID2D1BitmapRenderTarget **bitmapRenderTarget
    )
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

HRESULT HWRenderTarget::CreateLayer(
    _In_opt_ CONST D2D1_SIZE_F *size,
    _Outptr_ ID2D1Layer **layer
    )
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

HRESULT HWRenderTarget::CreateMesh(
    _Outptr_ ID2D1Mesh **mesh
    )
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

void HWRenderTarget::DrawLine(
    D2D1_POINT_2F point0,
    D2D1_POINT_2F point1,
    _In_ ID2D1Brush *brush,
    FLOAT strokeWidth,
    _In_opt_ ID2D1StrokeStyle *strokeStyle
    )
{
    //
    // PC rendering walk does not support Line w/Stroke rendering.
    // Draw lines as rectangles to workaround this limitation.
    //
    XRECTF_RB rect;
    XRECTF_RB clipRect;
    bool isClipped = false;

    ASSERT(point0.x <= point1.x);

    if (point0.y <= point1.y)
    {
        rect.top = point0.y + (point1.y - point0.y - strokeWidth) / 2;
    }
    else
    {
        rect.top = point1.y + (point0.y - point1.y - strokeWidth) / 2;
    }
    rect.left   = point0.x;
    rect.right  = point1.x;
    rect.bottom = rect.top + strokeWidth;

    //
    // Check if the line is within the current clip region.
    //
    if (GetCurrentClipRect(&clipRect))
    {
        isClipped = !DoRectsIntersect(rect, clipRect);
    }

    if (!isClipped)
    {
        FLOAT cx = point1.x - point0.x;
        FLOAT cy = point0.y > point1.y ? point0.y - point1.y : point1.y - point0.y;

        auto WithinEpsilon = [] (FLOAT a, FLOAT b) -> bool
        {
            FLOAT f = a - b;
            return -FLT_EPSILON <= f && f <= FLT_EPSILON;
        };

        //
        // Special case diagonal lines at 45 degrees - these could be squiggles.
        // If we just replace diagonals with a single rectangle, the overall
        // squiggle underline will become a straight line.
        //
        if (WithinEpsilon(cx, cy))
        {
            FLOAT strokeHalfWidth = strokeWidth / 2.0f;
            auto DrawDot = [this, &brush, &rect, strokeHalfWidth, strokeWidth] (FLOAT x, FLOAT y)
            {
                rect.left   = x  - strokeHalfWidth;
                rect.top    = y  - strokeHalfWidth;
                rect.right  = rect.left + strokeWidth;
                rect.bottom = rect.top  + strokeWidth;

                VERIFYHR(this->m_pTextDrawingContext->DrawRectangle(&rect, static_cast<HWSolidColorBrush*>(brush)->GetArgb()));
            };

            // Draw the first and intermediate dots.
            FLOAT x = point0.x;
            FLOAT y = point0.y;
            FLOAT eDir = point0.y < point1.y ? 1.0f : -1.0f;

            do
            {
                DrawDot(x, y);
                x += 1.0f;
                y += eDir;
            } while (x < (point1.x - FLT_EPSILON));

            // Draw the last dot.
            DrawDot(point1.x, point1.y);
        }
        else
        {
            VERIFYHR(m_pTextDrawingContext->DrawRectangle(&rect, static_cast<HWSolidColorBrush*>(brush)->GetArgb()));
        }
    }
}

void HWRenderTarget::DrawRectangle(
    _In_ CONST D2D1_RECT_F *rect,
    _In_ ID2D1Brush *brush,
    FLOAT strokeWidth,
    _In_opt_ ID2D1StrokeStyle *strokeStyle
    )
{
    // Do nothing, RichEdit calls this function to draw a rectange around the image object when it is selected
}

void HWRenderTarget::FillRectangle(
    _In_ CONST D2D1_RECT_F *rect,
    _In_ ID2D1Brush *brush
    )
{
    XRECTF_RB rectBounds = GetRectF_RB(*rect);
    XRECTF_RB clipRect;
    bool isClipped = false;

    //
    // Check if the rectangle is within the current clip region.
    //
    if (GetCurrentClipRect(&clipRect))
    {
        isClipped = !DoRectsIntersect(rectBounds, clipRect);
    }

    if (!isClipped)
    {
        VERIFYHR(m_pTextDrawingContext->DrawRectangle(&rectBounds, static_cast<HWSolidColorBrush*>(brush)->GetArgb()));
    }
}

void HWRenderTarget::DrawRoundedRectangle(
    _In_ CONST D2D1_ROUNDED_RECT *roundedRect,
    _In_ ID2D1Brush *brush,
    FLOAT strokeWidth,
    _In_opt_ ID2D1StrokeStyle *strokeStyle
    )
{
    ASSERT(FALSE);
}

void HWRenderTarget::FillRoundedRectangle(
    _In_ CONST D2D1_ROUNDED_RECT *roundedRect,
    _In_ ID2D1Brush *brush
    )
{
    ASSERT(FALSE);
}

void HWRenderTarget::DrawEllipse(
    _In_ CONST D2D1_ELLIPSE *ellipse,
    _In_ ID2D1Brush *brush,
    FLOAT strokeWidth,
    _In_opt_ ID2D1StrokeStyle *strokeStyle
    )
{
    ASSERT(FALSE);
}

void HWRenderTarget::FillEllipse(
    _In_ CONST D2D1_ELLIPSE *ellipse,
    _In_ ID2D1Brush *brush
    )
{
    ASSERT(FALSE);
}

void HWRenderTarget::DrawGeometry(
    _In_ ID2D1Geometry *geometry,
    _In_ ID2D1Brush *brush,
    FLOAT strokeWidth,
    _In_opt_ ID2D1StrokeStyle *strokeStyle
    )
{
    ASSERT(FALSE);
}

void HWRenderTarget::FillGeometry(
    _In_ ID2D1Geometry *geometry,
    _In_ ID2D1Brush *brush,
    _In_opt_ ID2D1Brush *opacityBrush
    )
{
    ASSERT(FALSE);
}

void HWRenderTarget::FillMesh(
    _In_ ID2D1Mesh *mesh,
    _In_ ID2D1Brush *brush
    )
{
    ASSERT(FALSE);
}

void HWRenderTarget::FillOpacityMask(
    _In_ ID2D1Bitmap *opacityMask,
    _In_ ID2D1Brush *brush,
    D2D1_OPACITY_MASK_CONTENT content,
    _In_opt_ CONST D2D1_RECT_F *destinationRectangle,
    _In_opt_ CONST D2D1_RECT_F *sourceRectangle
    )
{
    ASSERT(FALSE);
}

// TODO: Will RichEdit ever call this function? If not ASSERT(FALSE);
void HWRenderTarget::DrawBitmap(
    _In_ ID2D1Bitmap *bitmap,
    _In_opt_ CONST D2D1_RECT_F *destinationRectangle,
    FLOAT opacity,
    D2D1_BITMAP_INTERPOLATION_MODE interpolationMode,
    _In_opt_ CONST D2D1_RECT_F *sourceRectangle
    )
{
    HRESULT hr = S_OK;
    CImageBrush* pImageBrush = NULL;
    IUnknown* pTemp = NULL;
    XRECTF_RB rect = GetRectF_RB(*destinationRectangle);
    XRECTF_RB clipRect;
    bool isClipped = false;

    //
    // Check if the bitmap is within the current clip region.
    //
    if (GetCurrentClipRect(&clipRect))
    {
        isClipped = !DoRectsIntersect(rect, clipRect);
    }

    if (!isClipped)
    {
        // We need to test that the bitmap passed in is our HwTextBitmap before
        // we can safely reinterpret cast it. QI the bitmap for IUnknown, our HwTextBitmap
        // will return E_NOTIMPL. If it is a real D2D bitmap, it will return S_OK from QI.
        hr = bitmap->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**>(&pTemp));
        if (hr == E_NOTIMPL)
        {
            hr = S_OK;
            CWriteableBitmap* pImageSource = reinterpret_cast<HwTextBitmap*>(bitmap)->m_pWriteableBitmap;
            CREATEPARAMETERS cp(m_pCore);

            // Construct an ImageBrush with the WriteableBitmap as the image source. Then
            // ask the text drawing context to fill a rectangle with this ImageBrush.
            IFC(CImageBrush::Create((CDependencyObject**)&pImageBrush, &cp));
            SetInterface(pImageBrush->m_pImageSource, pImageSource);
            IFC(m_pTextDrawingContext->DrawRectangle(&rect, pImageBrush));
        }
        else
        {
            IFC(E_FAIL);
        }
    }

Cleanup:
    ReleaseInterface(pImageBrush);
    ReleaseInterface(pTemp);
}

void HWRenderTarget::DrawText(
    _In_reads_(stringLength) const WCHAR *string,
    UINT stringLength,
    _In_ IDWriteTextFormat *textFormat,
    _In_ CONST D2D1_RECT_F *layoutRect,
    _In_ ID2D1Brush *defaultForegroundBrush,
    D2D1_DRAW_TEXT_OPTIONS options,
    DWRITE_MEASURING_MODE measuringMode
    )
{
    ASSERT(FALSE);
}

void HWRenderTarget::DrawTextLayout(
    D2D1_POINT_2F origin,
    _In_ IDWriteTextLayout *textLayout,
    _In_ ID2D1Brush *defaultForegroundBrush,
    D2D1_DRAW_TEXT_OPTIONS options
    )
{
    ASSERT(FALSE);
}

void HWRenderTarget::DrawGlyphRun(
    D2D1_POINT_2F baselineOrigin,
    _In_ CONST DWRITE_GLYPH_RUN *glyphRun,
    _In_ ID2D1Brush *foregroundBrush,
    DWRITE_MEASURING_MODE measuringMode
    )
{
    HRESULT hr = S_OK;
    DWriteFontFace *pFontFace = NULL;
    PALText::GlyphRun *pPALGlyphRun = NULL;
    XUINT16 *pGlyphIndices = NULL;
    XFLOAT *pGlyphAdvances = NULL;
    PALText::GlyphOffset *pGlyphOffsets = NULL;
    XRECTF_RB clipRect;
    bool isClipped = false;

    //
    // Check if the glyph run is within the current clip region.
    //
    if (GetCurrentClipRect(&clipRect))
    {
        XUINT32 glyphIndex = 0;
        XPOINTF glyphRunStartPoint = { baselineOrigin.x, baselineOrigin.y };
        XFLOAT dirModifier = (glyphRun->bidiLevel & 1) == 0 ? 1.0f : -1.0f;

        isClipped = true;

        // To avoid clipping runs in case of overhangs (fractional values), move hit-test
        // position to the middle of the first non-zero with glyph in the glyph run.
        while (glyphIndex < glyphRun->glyphCount)
        {
            if (glyphRun->glyphAdvances[glyphIndex] > 0)
            {
                glyphRunStartPoint.x += (glyphRun->glyphAdvances[glyphIndex] / 2) * dirModifier;
                break;
            }
            glyphIndex++;
        }

        if (glyphIndex < glyphRun->glyphCount)
        {
            if (DoesRectContainPoint(clipRect, glyphRunStartPoint))
            {
                isClipped = false;
            }
            else
            {
                XPOINTF glyphRunEndPoint;
                XRECTF_RB glyphRunRect;

                // If the start of the glyph run does not intersect with the clipping rect,
                // check the end of the glyph run. Apply similar logic as above to eliminate overhangs.
                glyphIndex = 0;
                glyphRunEndPoint.x = baselineOrigin.x;
                glyphRunEndPoint.y = glyphRunStartPoint.y;

                while (glyphIndex < glyphRun->glyphCount - 1)
                {
                    glyphRunEndPoint.x += glyphRun->glyphAdvances[glyphIndex] * dirModifier;
                    glyphIndex++;
                }
                glyphRunEndPoint.x += (glyphRun->glyphAdvances[glyphIndex] / 2) * dirModifier;

                // In case of  RTL text we might get negative glyph progression, so use MIN/MAX to build appropriate rectangle.
                glyphRunRect.left = MIN(glyphRunStartPoint.x, glyphRunEndPoint.x);
                glyphRunRect.top = glyphRunStartPoint.y;
                glyphRunRect.right = MAX(glyphRunStartPoint.x, glyphRunEndPoint.x);
                glyphRunRect.bottom = glyphRunEndPoint.y;
                // Inclusive comparison to avoid corner case where the glyphRunStartPoint's y is exactly at the bottom of line height
                if (DoRectsIntersectInclusive(clipRect, glyphRunRect))
                {
                    isClipped = false;
                }
            }
        }
    }

    if (!isClipped)
    {
        //
        // Create a copy of the glyph run data, since this data is stored for later use.
        //
        pGlyphIndices  = new XUINT16[glyphRun->glyphCount];
        pGlyphAdvances = new XFLOAT [glyphRun->glyphCount];
        pGlyphOffsets  = new PALText::GlyphOffset[glyphRun->glyphCount];
        memcpy(pGlyphIndices,  glyphRun->glyphIndices,  glyphRun->glyphCount * sizeof(XUINT16));
        memcpy(pGlyphAdvances, glyphRun->glyphAdvances, glyphRun->glyphCount * sizeof(XFLOAT));
        memcpy(pGlyphOffsets,  glyphRun->glyphOffsets,  glyphRun->glyphCount * sizeof(PALText::GlyphOffset));

        pFontFace = new DWriteFontFace(glyphRun->fontFace, NULL);
        pPALGlyphRun = new PALText::GlyphRun();
        pPALGlyphRun->FontFace      = pFontFace;
        pPALGlyphRun->FontEmSize    = glyphRun->fontEmSize;
        pPALGlyphRun->GlyphCount    = glyphRun->glyphCount;
        pPALGlyphRun->GlyphIndices  = pGlyphIndices;
        pPALGlyphRun->GlyphAdvances = pGlyphAdvances;
        pPALGlyphRun->GlyphOffsets  = pGlyphOffsets;
        pPALGlyphRun->IsSideways    = !!glyphRun->isSideways;
        pPALGlyphRun->BidiLevel     = glyphRun->bidiLevel;

        //
        // Draw the glyph run using TextDrawingContext.
        // foregroundBrush is from RichEdit, it does not support alpha mask so we apply the previously cached alpha value here.
        //
        const UINT32 argb = (m_alpha << 24) | (static_cast<HWSolidColorBrush*>(foregroundBrush)->GetArgb() & 0xFFFFFF);

        IFC(m_pTextDrawingContext->DrawGlyphRun(
            pPALGlyphRun,
            &GetPointF(baselineOrigin),
            argb));

        pPALGlyphRun = NULL;
        pFontFace = NULL;
        pGlyphIndices = NULL;
        pGlyphAdvances = NULL;
        pGlyphOffsets = NULL;
    }

Cleanup:
    m_fLastDrawGlyphRunHasClipping = isClipped;
    delete pPALGlyphRun;
    ReleaseInterface(pFontFace);
    delete [] pGlyphIndices;
    delete [] pGlyphAdvances;
    delete [] pGlyphOffsets;

    VERIFYHR(hr);
}

void HWRenderTarget::SetTransform(
    _In_ CONST D2D1_MATRIX_3X2_F *transform
    )
{
    ASSERT(FALSE);
}

void HWRenderTarget::GetTransform(
    _Out_ D2D1_MATRIX_3X2_F *transform
    ) const
{
    *transform = D2D1::Matrix3x2F::Identity();
}

void HWRenderTarget::SetAntialiasMode(
    D2D1_ANTIALIAS_MODE antialiasMode
    )
{
    ASSERT(FALSE);
}

D2D1_ANTIALIAS_MODE HWRenderTarget::GetAntialiasMode(
    ) const
{
    ASSERT(FALSE);
    return D2D1_ANTIALIAS_MODE_PER_PRIMITIVE;
}

void HWRenderTarget::SetTextAntialiasMode(
    D2D1_TEXT_ANTIALIAS_MODE textAntialiasMode
    )
{
    ASSERT(FALSE);
}

D2D1_TEXT_ANTIALIAS_MODE HWRenderTarget::GetTextAntialiasMode(
    ) const
{
    ASSERT(FALSE);
    return D2D1_TEXT_ANTIALIAS_MODE_DEFAULT;
}

void HWRenderTarget::SetTextRenderingParams(
    _In_opt_ IDWriteRenderingParams *textRenderingParams
    )
{
    ASSERT(FALSE);
}

void HWRenderTarget::GetTextRenderingParams(
    _Outptr_result_maybenull_ IDWriteRenderingParams **textRenderingParams
    ) const
{
    ASSERT(FALSE);
}

void HWRenderTarget::SetTags(
    D2D1_TAG tag1,
    D2D1_TAG tag2
    )
{
    ASSERT(FALSE);
}

void HWRenderTarget::GetTags(
    _Out_opt_ D2D1_TAG *tag1,
    _Out_opt_ D2D1_TAG *tag2
    ) const
{
    ASSERT(FALSE);
}

void HWRenderTarget::PushLayer(
    _In_ CONST D2D1_LAYER_PARAMETERS *layerParameters,
    _In_opt_ ID2D1Layer *layer
    )
{
    ASSERT(FALSE);
}

void HWRenderTarget::PopLayer(
    )
{
    ASSERT(FALSE);
}

HRESULT HWRenderTarget::Flush(
    _Out_opt_ D2D1_TAG *tag1,
    _Out_opt_ D2D1_TAG *tag2
    )
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

void HWRenderTarget::SaveDrawingState(
    _Inout_ ID2D1DrawingStateBlock *drawingStateBlock
    ) const
{
    ASSERT(FALSE);
}

void HWRenderTarget::RestoreDrawingState(
    _In_ ID2D1DrawingStateBlock *drawingStateBlock
    )
{
    ASSERT(FALSE);
}

void HWRenderTarget::PushAxisAlignedClip(
    _In_ CONST D2D1_RECT_F *clipRect,
    D2D1_ANTIALIAS_MODE antialiasMode
    )
{
    VERIFYHR(m_clipRects.push_back(GetRectF_RB(*clipRect)));
}

void HWRenderTarget::PopAxisAlignedClip(
    )
{
    VERIFYHR(m_clipRects.pop_back());
}

void HWRenderTarget::Clear(
    _In_opt_ CONST D2D1_COLOR_F *clearColor
    )
{
    ASSERT(FALSE);
}

void HWRenderTarget::BeginDraw(
    )
{
    ASSERT(FALSE);
}

HRESULT HWRenderTarget::EndDraw(
    _Out_opt_ D2D1_TAG *tag1,
    _Out_opt_ D2D1_TAG *tag2
    )
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

D2D1_PIXEL_FORMAT HWRenderTarget::GetPixelFormat(
    ) const
{
    D2D1_PIXEL_FORMAT pixelFormat = { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_UNKNOWN };
    ASSERT(FALSE);
    return pixelFormat;
}

void HWRenderTarget::SetDpi(
    FLOAT dpiX,
    FLOAT dpiY
    )
{
    ASSERT(FALSE);
}

void HWRenderTarget::GetDpi(
    _Out_ FLOAT *dpiX,
    _Out_ FLOAT *dpiY
    ) const
{
    *dpiX = *dpiY = 96.0;
}

D2D1_SIZE_F HWRenderTarget::GetSize(
    ) const
{
    D2D1_SIZE_F size = { 0.0f, 0.0f };
    ASSERT(FALSE);
    return size;
}

D2D1_SIZE_U HWRenderTarget::GetPixelSize(
    ) const
{
    D2D1_SIZE_U size = { 0, 0 };
    ASSERT(FALSE);
    return size;
}


UINT32 HWRenderTarget::GetMaximumBitmapSize(
    ) const
{
    ASSERT(FALSE);
    return 0;
}


BOOL HWRenderTarget::IsSupported(
    _In_ CONST D2D1_RENDER_TARGET_PROPERTIES *renderTargetProperties
    ) const
{
    ASSERT(FALSE);
    return FALSE;
}

HRESULT HWRenderTarget::DrawBaseLine(long baseLineY, long left, long right, UINT32 crText)
{
    XRECTF_RB rect = {static_cast<float>(left), static_cast<float>(baseLineY), static_cast<float>(right), static_cast<float>(baseLineY + 1)};
    XRECTF_RB clipRect;
    bool isClipped = false;

    //
    // Check if the line is within the current clip region.
    //
    if (GetCurrentClipRect(&clipRect))
    {
        isClipped = !DoRectsIntersect(rect, clipRect);
    }

    if (!isClipped)
    {
         VERIFYHR(m_pTextDrawingContext->DrawRectangle(&rect, crText));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  PC based rendering.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT HWRenderTarget::HWRender(
    _In_ IContentRenderer* pContentRenderer
    )
{
    ASSERT(m_pTextDrawingContext != NULL);
    return m_pTextDrawingContext->HWRender(pContentRenderer);
}

//---------------------------------------------------------------------------
//
//  Clears the rendering data.
//
//---------------------------------------------------------------------------
void HWRenderTarget::InvalidateContent()
{
    ASSERT(m_clipRects.size() == 0);
    ASSERT(m_pTextDrawingContext != NULL);
    EmptyRectF(&m_clipRect);
    m_pTextDrawingContext->Clear();
}

//---------------------------------------------------------------------------
//
//  Clears the render walk data (textures/edge stores).
//
//---------------------------------------------------------------------------
void HWRenderTarget::InvalidateRenderCache()
{
    ASSERT(m_clipRects.size() == 0);
    ASSERT(m_pTextDrawingContext != NULL);
    m_pTextDrawingContext->ClearGlyphRunCaches();
}

//---------------------------------------------------------------------------
//
//  Clears the rendering data within specified region.
//
//---------------------------------------------------------------------------
void HWRenderTarget::InvalidateRegion(
    const XRECT_RB &region
    )
{
    XRECTF_RB invalidRegion = {
        static_cast<XFLOAT>(region.left),
        static_cast<XFLOAT>(region.top),
        static_cast<XFLOAT>(region.right),
        static_cast<XFLOAT>(region.bottom)
    };

    ASSERT(m_clipRects.size() == 0);
    ASSERT(m_pTextDrawingContext != NULL);
    m_clipRect = invalidRegion;
    m_pTextDrawingContext->InvalidateRegion(&invalidRegion);
}


//---------------------------------------------------------------------------
//
//   Sets whether color in fonts should be enabled for text drawing operations.
//
//---------------------------------------------------------------------------
void HWRenderTarget::SetIsColorFontEnabled(bool isColorFontEnabled)
{
    m_pTextDrawingContext->SetIsColorFontEnabled(isColorFontEnabled);
}

//---------------------------------------------------------------------------
//
//  Gets the current clipping rectangle by intersecting all pushed clip rects.
//
//---------------------------------------------------------------------------
bool HWRenderTarget::GetCurrentClipRect(
    _Out_ XRECTF_RB *pClipRect
    ) const
{
    bool needsClipping = false;
    EmptyRectF(pClipRect);

    if (m_clipRects.size() > 0)
    {
        needsClipping = TRUE;
        *pClipRect = m_clipRects[0];
        for (UINT32 index = 1; index < m_clipRects.size(); index++)
        {
            IntersectRect(pClipRect, &m_clipRects[index]);
        }
    }

    if (!IsEmptyRectF(m_clipRect))
    {
        if (needsClipping)
        {
            IntersectRect(pClipRect, &m_clipRect);
        }
        else
        {
            needsClipping = TRUE;
            *pClipRect = m_clipRect;
        }
    }

    return needsClipping;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      The method to clean up all the device related realizations
//      on this object.
//
//-----------------------------------------------------------------------------
void HWRenderTarget::CleanupRealizations()
{
    if (m_pTextDrawingContext != nullptr)
    {
        m_pTextDrawingContext->CleanupRealizations();
    }
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Implements callback to draw squiggly lines.
//
//-----------------------------------------------------------------------------
bool HWRenderTarget::DrawUnderline(BYTE underlineType, DWORD color, const RECT* fillrect)
{
    // We only draw CFU_UNDERLINEIMEWAVE and CFU_UNDERLINEWAVE for red misspelling wavy lines.
    switch(underlineType)
    {
        case CFU_UNDERLINEIMEWAVE:
        case CFU_UNDERLINEWAVE:
        {
            XRECTF_RB clipRect;
            XRECTF_RB rect;
            //
            // Check if the line is within the current clip region.
            //
            rect.top = static_cast<XFLOAT>(fillrect->top);
            rect.left = static_cast<XFLOAT>(fillrect->left);
            rect.bottom = static_cast<XFLOAT>(fillrect->bottom);
            rect.right = static_cast<XFLOAT>(fillrect->right);
            GetCurrentClipRect(&clipRect);
            color = ConvertFromABGRToARGB(color | 0xFF000000);
            VERIFYHR(m_pTextDrawingContext->DrawWavyLine(&rect, &clipRect, color));
            return true;
        }

        default:
            return false;
    }
}

void HWRenderTarget::SetBackPlateConfiguration(bool backPlateActive, bool useHyperlinkForeground)
{
    if (m_pTextDrawingContext != nullptr)
    {
        m_pTextDrawingContext->SetBackPlateConfiguration(backPlateActive, useHyperlinkForeground);
    }
}


