// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DynamicArray.h"
#include <vector>
#include <wil\resource.h>

class HWTexture;
class HWTextRealization;
class SharedWicBitmap;
struct ID2D1DeviceContext;
struct ID2D1SolidColorBrush;
class IContentRenderer;
enum DWRITE_GLYPH_IMAGE_FORMATS;

//---------------------------------------------------------------------------
//
//  D2DTextDrawingContext
//
//  Describes text visual content using draw commands. It is actually storing a
//  set of rendering instructions that will later be used by the graphics system.
//
//---------------------------------------------------------------------------
class D2DTextDrawingContext final : public RichTextServices::TextDrawingContext
{
public:
    // Initializes a new instance of the D2DTextDrawingContext class.
    D2DTextDrawingContext(
        _In_ CTextCore *pTextCore
        );

    // PC based rendering
    _Check_return_ HRESULT HWRender(
        _In_ IContentRenderer* pContentRenderer
        );

    // D2D based rendering
    _Check_return_ HRESULT D2DEnsureResources(
        _In_ const D2DPrecomputeParams &cp,
        _In_ const CMILMatrix *pRenderTransform
        );

    _Check_return_ HRESULT D2DRender(
        _In_ const SharedRenderParams &sharedRP,
        _In_ const D2DRenderParams &d2dRP,
        _In_ XFLOAT opacity
        );

    _Check_return_ HRESULT DrawGlyphRun(
        _In_ PALText::GlyphRun *pGlyphRun,
        _In_ const XPOINTF *pGlyphRunOffset,
        _In_ XUINT32 foregroundColor
        );

    _Check_return_ HRESULT DrawGlyphRun(
        _In_ PALText::GlyphRun *pGlyphRun,
        _In_ const XPOINTF *pGlyphRunOffset,
        _In_ const xref::weakref_ptr<CDependencyObject>& pBrushSource
        );

    _Check_return_ HRESULT DrawRectangle(
        _In_ const XRECTF_RB *pRect,
        _In_ XUINT32 fillColor
        );

    _Check_return_ HRESULT DrawRectangle(
        _In_ const XRECTF_RB *pRect,
        _In_ CImageBrush* pImageBrush,
        _In_ DWORD color = 0,
        _In_opt_ XUINT32* pPixelBuffer = NULL,
        _In_ XINT32 insertPosition = -1);

    void InvalidateRegion(
        _In_ const XRECTF_RB *pRegion
        );

    // Clears the cached glyph run data (textures, edge stores)...
    void ClearGlyphRunCaches();

    void SaveAndClearGlyphRunRealizations(_In_opt_ const HWRenderParams* const pRenderParams);

    //-----------------------------------------------------------------------
    //
    // TextDrawingContext implementation.
    //
    //-----------------------------------------------------------------------

    // Determines whether rendering data has been already populated.
     bool HasRenderingData() const override;

    // Sets line parameters
    void SetLineInfo(
        _In_ XFLOAT viewportWidth,
        _In_ bool invertHorizontalAxis,
        _In_ XFLOAT yOffset,
        _In_ XFLOAT verticalAdvance
        ) override;

    // Sets whether color in fonts should be enabled for text drawing operations.
    void SetIsColorFontEnabled(bool isColorFontEnabled) override;

    void SetColorFontPaletteIndex(UINT32 colorFontPaletteIndex) override;

    // Sets context for the text foreground highlight info for the paragraph.
    // This includes selection
    void ClearForegroundHighlightInfo();

    void AppendForegroundHighlightInfo(
        uint32_t count,
        _In_reads_(count) XRECTF* rectangles,
        _In_ CSolidColorBrush* foregroundBrush,
        uint32_t startIndex,
        uint32_t countInParagraph,
        _In_ const XPOINTF& paragraphOffset);

    // Generates rendering data for run of glyphs.
    _Check_return_ HRESULT DrawGlyphRun(
        _In_ const XPOINTF &position,
        _In_ XFLOAT runWidth,
        _In_ PALText::GlyphRun *pGlyphRun,
        _In_ const xref::weakref_ptr<CDependencyObject>& pBrushSource,
        _In_opt_ XRECTF_RB const* pClipRect
        ) override;

    // Generates rendering data for run of glyphs after pre-processing by the shaping engine.
    _Check_return_ HRESULT DrawGlyphs(
        _In_ const XPOINT &position,
        _In_ XINT32 runWidth,
        _In_reads_(textLength) WCHAR const* pTextString,
        _In_reads_(textLength) XUINT16 const* pClusterMap,
        _In_reads_(textLength) PALText::ShapingTextProperties const* pTextProps,
        _In_ XUINT32 textLength,
        _In_reads_(glyphCount) XUINT16 const* pGlyphIndices,
        _In_reads_(glyphCount) PALText::ShapingGlyphProperties const* pGlyphProps,
        _In_reads_(glyphCount) XLONG const* pGlyphAdvances,
        _In_reads_(glyphCount) XLONG const* pGlyphAdvancesPreJust,
        _In_reads_(glyphCount) XPOINT const* pGlyphOffsets,
        _In_ XUINT32 glyphCount,
        _In_ PALText::IFontFace* pFontFace,
        _In_ XFLOAT fontSize,
        _In_ bool isSideways,
        _In_ XUINT8 bidiLevel,
        _In_ PALText::ScriptAnalysis const* pScriptAnalysis,
        _In_ const xref::weakref_ptr<CDependencyObject>& pBrushSource,
        _In_opt_ XRECTF_RB const* pClipRect
        ) override;

    // Generates rendering data for a line.
    RichTextServices::Result::Enum DrawLine(
        _In_ const XPOINTF &position,
        _In_ XFLOAT width,
        _In_ XFLOAT thickness,
        _In_ XUINT8 bidirectionalLevel,
        _In_ const xref::weakref_ptr<CDependencyObject>& pBrushSource,
        _In_opt_ XRECTF_RB const* pClipRect
        ) override;

    // Generates rendering data for a wavy line.
    _Check_return_ HRESULT DrawWavyLine(
            _In_ XRECTF_RB *pRect,// The rect to draw the wavy line.
            _In_ XRECTF_RB *pClipRect,// The clipping Rect.
            _In_ DWORD color, // color for the wavy line.
            _In_ XINT32 insertPosition = -1,// Position for the wavy line.
            _In_opt_ XUINT32* pPixelBuffer = NULL // Pixel buffer
        );

    // Clears all rendering data.
    void Clear() override;

    void CleanupRealizations();

    bool BaseRealizationHasSubPixelOffsets() const;
    void ClearBaseRealization();

    void SetFlipSelectionAlongHorizontalAxis(_In_ bool flipSelectionAlongHorizontalAxis);

    static void RasterizeImageFormatGlyphRun(
        _In_ ID2D1DeviceContext4 *pDeviceContext,
        _In_ DWRITE_GLYPH_RUN const &glyphRun,
        _In_ DWRITE_GLYPH_IMAGE_FORMATS glyphImageFormat,
        _In_ UINT32 fontPaletteIndex,
        _In_opt_ ID2D1Brush *pBrushNoRef
        );

    void SetBackPlateConfiguration(bool backPlateActive, bool useHyperlinkForeground);

    void SetControlEnabled(bool enabled);

private:
    static const XUINT32 WavyLineTopMargin = 2;
    static const XUINT32 WavyLineWidth     = 8;
    static const XUINT32 WavyLineHeight    = 4;

    // Stores info about the line the glyph run is associated with.
    struct LineInfo
    {
        XFLOAT ViewportWidth;
        bool InvertHorizontalAxis;
        XFLOAT YOffset;
        XFLOAT VerticalAdvance;
    };

    // Stores highlight information for the paragraph.
    struct HighlightInfo
    {
        uint32_t Count; // Total number of rects in page, including those not in paragraph.
        uint32_t StartIndex; // Start index for paragraph rects in highlight rect array.
        uint32_t CountInParagraph; // Number of highlight rects in paragraph's render space.
        XPOINTF ParagraphOffset; // Offset of paragraph in page, rects have page-level coordinates.
        std::vector<XRECTF> Rectangles; // Array of all highlight rects in page.
        xref_ptr<CSolidColorBrush> ForegroundBrush; // Foreground color to use for the interesecting highlight text
    };

    // Stores rendering data for glyph runs.
    struct GlyphRunData
    {
        PALText::GlyphRun *GlyphRun;
        XPOINTF BaselineOffset;
        XRECTF LineBounds;
        bool InvertHorizontalAxis;
        xref::weakref_ptr<CDependencyObject> BrushSource;
        CBrush *Brush;
        AcceleratedBrushParams *PALBrushParams;
        XFLOAT GlyphRunClipLeft;
        XFLOAT GlyphRunClipRight;
        XRECTF GlyphRunRealizationBounds;
        CMILMatrix GlyphRunRealizationTransform;
        DWRITE_GLYPH_IMAGE_FORMATS GlyphImageFormat;
    };

    // Stores rendering data for lines
    struct LineData
    {
        XFLOAT Width;
        XFLOAT Thickness;
        CMILMatrix Transform;
        xref::weakref_ptr<CDependencyObject> BrushSource;
        CBrush *Brush;
        AcceleratedBrushParams *PALBrushParams;
    };

    // Stores rendering data for images, currently only wavy lines use this data structure.
    struct ImageData
    {
        XRECTF_RB ImageRect;
        CImageBrush* Brush;
        XUINT32* pPixelBuffer;
        DWORD color;
    };

    // Stores rasterized data for glyphs
    struct GlyphRunRealization
    {
        HWTextRealization* TextRealization;
        XRECTF BrushBounds;
        bool HasExplicitBrush;
    };

    // Core text services.
    CTextCore *m_pTextCore;

    // Collection of glyph runs and associated data
    DynamicArray<GlyphRunData> m_glyphRuns;

    // Collection of lines and associated data
    DynamicArray<LineData> m_lines;

    // Collection of images and associated data
    DynamicArray<ImageData> m_images;

    // Collection of cached glyph textures.
    DynamicArray<GlyphRunRealization> *m_pGlyphRunRealizations;

    // Volatile state for the currently rendered text line.
    LineInfo m_currentLineInfo;

    bool m_isColorFontEnabled : 1;
    bool m_flipSelectionAlongHorizontalAxis : 1;
    bool m_backPlateActive : 1;
    bool m_useHyperlinkForeground : 1;
    bool m_controlEnabled : 1;

    UINT32 m_colorFontPaletteIndex;

    // Volatile state for highlight information from current paragraph.
    std::vector<wistd::unique_ptr<HighlightInfo>> m_highlights;

    // Realization info used to determine whether individual glyph run realizations need updating.
    HWTextRealization *m_pBaseRealization;

    // Release resources associated with the D2DTextDrawingContext.
    ~D2DTextDrawingContext() override;

    // Gets a transform that includes original position and mirroring.
    void GetLocalTransform(
        _In_ XFLOAT x,
        _In_ XFLOAT y,
        _Out_ CMILMatrix *pLocalTransform
        );

    // Gets a transform that includes original position and mirroring.
    void GetLocalTransform(
        _In_ XFLOAT x,
        _In_ XFLOAT y,
        _In_ bool invertHorizontalAxis,
        _In_ XFLOAT viewportWidth,
        _Out_ CMILMatrix *pLocalTransform
        );

    // Add glyph run to the collection representing rendering data.
    _Check_return_ HRESULT AddGlyphRun(
        _In_ PALText::GlyphRun *pGlyphRun,
        _In_ const XPOINTF& baselineOffset,
        _In_ const XRECTF& lineBounds,
        _In_ bool invertHorizontalAxis,
        _In_ const xref::weakref_ptr<CDependencyObject>& pBrushSource,
        _In_opt_ CBrush *pBrush,
        _In_opt_ XRECTF_RB const* pClipRect,
        _In_ DWRITE_GLYPH_IMAGE_FORMATS glyphImageFormat
        );

    // Translate a glyph run which is known to be color to the glyph runs which
    // should be drawn in its place.
    _Check_return_ HRESULT TranslateToColorGlyphRun(
        _In_ PALText::GlyphRun *pGlyphRun,
        _In_ const XPOINTF& baselineOffset,
        _In_ const XRECTF& lineBounds,
        _In_ bool invertHorizontalAxis,
        _In_ const xref::weakref_ptr<CDependencyObject>& pBrushSource,
        _In_opt_ CBrush *pBrush,
        _In_opt_ XRECTF_RB const* pClipRect,
        _Out_ bool *hadColorGlyphs
        );

    // Creates new glyph run which is a sub-run of the original run.
    _Check_return_ HRESULT CreateGlyphRun(
        _In_ PALText::GlyphRun *pOriginalGlyphRun,
        _In_ XUINT32 startIndex,
        _In_ XUINT32 glyphCount,
        _Outptr_ FssGlyphRun **ppGlyphRun
        );

    // Releases all resources associated with glyph run.
    static void ReleaseGlyphRun(
        _In_opt_ PALText::GlyphRun *pGlyphRun
        );

    // PC line rendering.
    _Check_return_ HRESULT HWRenderLines(
        _In_ IContentRenderer* pContentRenderer
        );

    // PC image rendering.
    _Check_return_ HRESULT HwRenderImages(
        _In_ IContentRenderer* pContentRenderer
        );

    // Builds glyph run textures using D2D.
    _Check_return_ HRESULT HWBuildGlyphRunTextures(
        _In_ const HWRenderParams *pHWRenderParams,
        _In_ const CMILMatrix *pRealizationTransform
        );

    _Check_return_ HRESULT HWBuildGlyphRunTexture(
        _In_ const HWRenderParams *pHWRenderParams,
        _In_ const CMILMatrix *pRealizationTransform,
        _In_ const XRECTF &glyphRunBatchBounds,
        XUINT32 glyphRunStartIndex,
        XUINT32 glyphRunCount
        );

    _Check_return_ HRESULT HWBuildGlyphRunSingleTexture(
        _In_ const HWRenderParams *pHWRenderParams,
        _In_ const CMILMatrix *pRealizationTransform,
        _In_ const XRECTF &glyphRunBatchBounds,
        XUINT32 glyphRunStartIndex,
        XUINT32 glyphRunCount
        );

    // PC glyph texture rendering.
    _Check_return_ HRESULT HWRenderGlyphTextures(_In_ IContentRenderer* pContentRenderer);

    _Check_return_ HRESULT GetGlyphRunTransformAndBounds(
        _In_ ID2D1DeviceContext *pSharedD2DDeviceContextNoRef,
        _In_ const GlyphRunData *pGlyphRunDataNoRef,
        _In_ const CMILMatrix *pRealizationTransform,
        bool snapToPixel,
        _Out_ CMILMatrix &glyphRunRealizationTransform,
        _Out_ XRECTF_RB &glyphRunBounds
        );

    _Check_return_ HRESULT RasterizeGlyphRunToD2DOrRawBuffer(
        _In_ GlyphRunData *pCurrentGlyphRunDataNoRef,
        _In_ CMILMatrix &glyphRunRealizationTransform,
        _In_ const XRECTF &glyphRunRealizationBounds,
        _In_ const XRECTF &glyphRunBatchBounds,
        float tileOffsetX,
        float tileOffsetY,
        _In_opt_ ID2D1DeviceContext *pTextureAtlasDeviceContext,
        _In_opt_ ID2D1SolidColorBrush *pSharedD2DSolidColorBrushNoRef,
        const bool hasNative8BitSurfaceSupport,
        _Inout_updates_opt_(textureLockStride*textureLockHeight) XUINT8* pTextureLockAddress,
        XINT32 textureLockStride,
        XUINT32 textureLockWidth,
        XUINT32 textureLockHeight,
        bool blendIntoBuffer
        );

    _Check_return_ HRESULT RasterizeWavyLine(
        _In_ XRECTF_RB const* pRect,
        _In_ DWORD color,
        _Out_ XUINT32 **ppBitmap);

    _Check_return_ HRESULT EnsureWavyLineRealization();

    // This function returns an add-ref pointer.
    CSolidColorBrush* GetAlternativeForegroundBrush(_In_ const xref::weakref_ptr<CDependencyObject>& pBrushSource);
};
