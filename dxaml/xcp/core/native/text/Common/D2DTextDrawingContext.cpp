// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <vector>
#include <wil\resource.h>

#include "D2DTextDrawingContext.h"
#include "WinTextCore.h"
#include "SharedWicBitmap.h"
#include "TextHelpers.h"
#include "DWriteFontFace.h"
#include "Geometry.h"
#include "Pixelformatutils.h"
#include "D2DUtils.h"
#include "inline.h"

#include "D2DAccelerated.h"
#include "WindowsGraphicsDeviceManager.h"
#include "D3D11SharedDeviceGuard.h"

#include "DCompTreeHost.h"
#include <XamlBehaviorMode.h>
#include "XamlTraceLogging.h"
#include "hwcompnode.h"
#include <FrameworkTheming.h>
#include <ContentRenderer.h>
#include <WindowRenderTarget.h>
#include <PixelFormat.h>

#define DBG_TEXT_REALIZATIONS 0

#define SWAP(Type, a, b) {Type temp = a; a = b; b = temp;}

// Large, but not too large or the PrintRT doesn't render anything
#define PRINTRT_FLOATMAX 1e7F

using namespace RichTextServices;

template class DynamicArray<D2DTextDrawingContext::GlyphRunData>;
template class DynamicArray<D2DTextDrawingContext::LineData>;
template class DynamicArray<D2DTextDrawingContext::ImageData>;
template class DynamicArray<D2DTextDrawingContext::GlyphRunRealization>;

inline XFLOAT FromTextDpi(_In_ XINT32 value)
{
    return static_cast<XFLOAT>(value) / 300.f;
}

// All formats recognized by XAML and requested from TranslateColorGlyphRun.
auto c_allSupportedGlyphImageFormats    = DWRITE_GLYPH_IMAGE_FORMATS_TRUETYPE
                                        | DWRITE_GLYPH_IMAGE_FORMATS_CFF
                                        | DWRITE_GLYPH_IMAGE_FORMATS_COLR
                                        | DWRITE_GLYPH_IMAGE_FORMATS_SVG
                                        | DWRITE_GLYPH_IMAGE_FORMATS_PNG
                                        | DWRITE_GLYPH_IMAGE_FORMATS_JPEG
                                        | DWRITE_GLYPH_IMAGE_FORMATS_TIFF
                                        | DWRITE_GLYPH_IMAGE_FORMATS_PREMULTIPLIED_B8G8R8A8;

// All formats which support color, whether bitmap, multi-color, or layered colored outlines.
auto c_supportedColorGlyphImageFormats  = DWRITE_GLYPH_IMAGE_FORMATS_COLR
                                        | DWRITE_GLYPH_IMAGE_FORMATS_SVG
                                        | DWRITE_GLYPH_IMAGE_FORMATS_PNG
                                        | DWRITE_GLYPH_IMAGE_FORMATS_JPEG
                                        | DWRITE_GLYPH_IMAGE_FORMATS_TIFF
                                        | DWRITE_GLYPH_IMAGE_FORMATS_PREMULTIPLIED_B8G8R8A8;

// Formats that require higher color texture rather than just an alpha channel mask.
auto c_bgraColorGlyphImageFormats       = DWRITE_GLYPH_IMAGE_FORMATS_SVG
                                        | DWRITE_GLYPH_IMAGE_FORMATS_PNG
                                        | DWRITE_GLYPH_IMAGE_FORMATS_JPEG
                                        | DWRITE_GLYPH_IMAGE_FORMATS_TIFF
                                        | DWRITE_GLYPH_IMAGE_FORMATS_PREMULTIPLIED_B8G8R8A8;

// Formats with raster pixels, ignoring the foreground brush.
auto c_anyBitmapGlyphImageFormats       = DWRITE_GLYPH_IMAGE_FORMATS_PNG
                                        | DWRITE_GLYPH_IMAGE_FORMATS_JPEG
                                        | DWRITE_GLYPH_IMAGE_FORMATS_TIFF
                                        | DWRITE_GLYPH_IMAGE_FORMATS_PREMULTIPLIED_B8G8R8A8;

// Purely monochromatic formats which use the foreground brush.
auto c_monochromeGlyphImageFormats      = DWRITE_GLYPH_IMAGE_FORMATS_TRUETYPE
                                        | DWRITE_GLYPH_IMAGE_FORMATS_CFF;

//---------------------------------------------------------------------------
//
//  Initializes a new instance of the D2DTextDrawingContext class.
//
//---------------------------------------------------------------------------
D2DTextDrawingContext::D2DTextDrawingContext(
    _In_ CTextCore *pTextCore
)
    : m_pTextCore(pTextCore)
    , m_pBaseRealization(NULL)
    , m_pGlyphRunRealizations(NULL)
    , m_isColorFontEnabled(false)
    , m_colorFontPaletteIndex(0)
    , m_flipSelectionAlongHorizontalAxis(false)
    , m_backPlateActive(false)
    , m_useHyperlinkForeground(false)
    , m_controlEnabled(true)
{
    memset(&m_currentLineInfo, 0, sizeof(LineInfo));
}

//---------------------------------------------------------------------------
//
//  Release resources associated with the D2DTextDrawingContext.
//
//---------------------------------------------------------------------------
D2DTextDrawingContext::~D2DTextDrawingContext()
{
    Clear();
}

//---------------------------------------------------------------------------
//
//  Generates rendering data for run of glyphs.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT D2DTextDrawingContext::DrawGlyphRun(
    _In_ const XPOINTF &position,
        // The origin for the text output.
    _In_ XFLOAT runWidth,
        // Total width of the glyph run.
    _In_ PALText::GlyphRun *pGlyphRun,
        // Contains the information needed by renderers to draw glyph runs.
    _In_ const xref::weakref_ptr<CDependencyObject>& pBrushSource,
        // The source of foreground brush.
    _In_opt_ XRECTF_RB const* pClipRect
        // The clip rect to apply to the run.
    )
{
    PALText::GlyphRun* glyphRunRemainder = pGlyphRun;
    PALText::GlyphRun* highlightGlyphRun = nullptr;
    PALText::GlyphRun* postHighlightGlyphRun = nullptr;
    XRECTF lineBounds = { 0.0f, m_currentLineInfo.YOffset, m_currentLineInfo.ViewportWidth, m_currentLineInfo.VerticalAdvance };
    xref_ptr<CSolidColorBrush> alternativeForegroundBrush;
    XPOINTF currentPosition = position;

    // Handle release of intermediate glyph runs in the case of error.
    // NOTE: If AddGlyphRun is called, ownership is transferred and the pointer should be set to null.
    auto releaseGlyphRunsOnExit = wil::scope_exit([&] ()
    {
        ReleaseGlyphRun(glyphRunRemainder);

        ReleaseGlyphRun(highlightGlyphRun);
        ReleaseGlyphRun(postHighlightGlyphRun);
    });

    // TODO: CSolidColorBrush could potentially be changed to CBrush in the future to remove the restriction on CSolidColorBrush
    //                 for highlighter brushes.

    alternativeForegroundBrush.attach(GetAlternativeForegroundBrush(pBrushSource));

    // This algorithm works off of set reduction.  It is expected that the highlight collection and bounds collection cause traversal
    // in glyph sequential order.  After each highlight is found and processed, the remaining glyphs are tracked for the next iteration.
    // When there is nothing left to process, it completes the loop and adds the final remainging glyphs (if any) to the glyph run collection.
    for (auto& highlightInfo : m_highlights)
    {
        if (glyphRunRemainder == nullptr)
        {
            // No more glyphs to process, all remaining highlights are out of range so exit the loop.
            break;
        }

        // If selection bounds are set for the current line, intersect the glyph run with the selection bounds.
        // - If glyph run is outside of selection bounds no action is required.
        // - If entire glyph run is selected, replace the foreground brush with selection foreground brush.
        // - If part of the glyph run is selected, split the glyph run and replace the foreground brush for the selected part.
        if (highlightInfo->Count > 0)
        {
            // currentPosition.x of the glyph run always represents its logical start position, so in case of
            // RTL run it is on its right side. Adjust glyphRun?s left and right position accordingly.
            bool isLtrGlyphRun = ((glyphRunRemainder->BidiLevel & 1) == 0);
            float glyphRunLeft = isLtrGlyphRun ? currentPosition.x : position.x - runWidth;
            float glyphRunRight = glyphRunLeft + runWidth;
            float highlightTop = 0.0f;
            float highlightBottom = 0.0f;
            float currentMidpoint = 0.0f;
            auto highlightBrush = highlightInfo->ForegroundBrush;

            for (uint32_t boundsIndex = 0; boundsIndex < highlightInfo->Count; boundsIndex++)
            {
                if (glyphRunRemainder == nullptr)
                {
                    // No more glyphs to process, all remaining highlights are out of range so exit the loop.
                    break;
                }

                // If there's no intersection along the y-axis we can skip all the calculation for the x-axis.
                // To check for intersection, check that the midpoint of the glyph run along y-aixs lies within
                // selection bounds.
                highlightTop = highlightInfo->Rectangles[boundsIndex].Y - highlightInfo->ParagraphOffset.y;
                highlightBottom = highlightTop + highlightInfo->Rectangles[boundsIndex].Height;
                currentMidpoint = m_currentLineInfo.YOffset + m_currentLineInfo.VerticalAdvance / 2.0f;
                if (currentMidpoint <= highlightTop ||
                    currentMidpoint >= highlightBottom)
                {
                    // There's no reason for a selection rect to cover half or less of glyph run. Move to the next bound.
                    continue;
                }

                // Account for flow direction when getting the left and right position of the selected region.
                // Flip selection along the horizontal axis because the selection rectangle
                // will be compared against the transformed glyph positions.
                float highlightLeft = highlightInfo->Rectangles[boundsIndex].X - highlightInfo->ParagraphOffset.x;
                float highlightRight = highlightLeft + highlightInfo->Rectangles[boundsIndex].Width;
                if (m_currentLineInfo.InvertHorizontalAxis || m_flipSelectionAlongHorizontalAxis)
                {
                    highlightLeft = m_currentLineInfo.ViewportWidth - highlightRight;
                    highlightRight = highlightLeft + highlightInfo->Rectangles[boundsIndex].Width;
                }

                // Intersect glyph run?s bounds with selection bounds.
                if (highlightLeft >= glyphRunRight || highlightRight <= glyphRunLeft)
                {
                    // Glyph run doesn't intersect with selection. It will be added at the end in the default path.
                }
                else if (highlightLeft <= glyphRunLeft && highlightRight >= glyphRunRight)
                {
                    // Entire glyph run is selected. Replace the foreground brush.
                    IFC_RETURN(AddGlyphRun(
                        glyphRunRemainder,
                        currentPosition,
                        lineBounds,
                        m_currentLineInfo.InvertHorizontalAxis,
                        xref::weakref_ptr<CDependencyObject>(),
                        highlightBrush,
                        pClipRect,
                        c_allSupportedGlyphImageFormats));
                    glyphRunRemainder = nullptr;
                }
                else
                {
                    uint32_t glyphIndex = 0;
                    auto remainderGlyphCount = glyphRunRemainder->GlyphCount;
                    uint32_t highlightStartIndex = 0;
                    uint32_t highlightEndIndex = 0;
                    float directionalModifier = isLtrGlyphRun ? 1.0f : -1.0f;
                    XPOINTF highlightStartPosition = currentPosition;
                    XPOINTF highlightEndPosition;
                    currentMidpoint = glyphRunRemainder->GlyphAdvances[0] / 2;

                    // Advance to the start of the highlight.
                    while (glyphIndex < remainderGlyphCount &&
                          ((isLtrGlyphRun && (highlightStartPosition.x + currentMidpoint) < highlightLeft) ||
                           (!isLtrGlyphRun && (highlightStartPosition.x - currentMidpoint) > highlightRight)))
                    {
                        highlightStartPosition.x += glyphRunRemainder->GlyphAdvances[glyphIndex] * directionalModifier;
                        if (glyphIndex + 1 < remainderGlyphCount)
                        {
                            currentMidpoint = glyphRunRemainder->GlyphAdvances[glyphIndex + 1] / 2;
                        }
                        else
                        {
                            currentMidpoint = 0;
                        }
                        glyphIndex++;
                    }

                    // Skip zero-width glyphs
                    while (glyphIndex < remainderGlyphCount && glyphRunRemainder->GlyphAdvances[glyphIndex] == 0)
                    {
                        glyphIndex++;
                    }
                    highlightStartIndex = glyphIndex;

                    // Advance to the end of the highlight.
                    highlightEndPosition = highlightStartPosition;
                    while (glyphIndex < remainderGlyphCount &&
                          ((isLtrGlyphRun && (highlightEndPosition.x + currentMidpoint) < highlightRight) ||
                           (!isLtrGlyphRun && (highlightEndPosition.x - currentMidpoint) > highlightLeft)))
                    {
                        highlightEndPosition.x += glyphRunRemainder->GlyphAdvances[glyphIndex] * directionalModifier;
                        if (glyphIndex + 1 < remainderGlyphCount)
                        {
                            currentMidpoint = glyphRunRemainder->GlyphAdvances[glyphIndex + 1] / 2;
                        }
                        else
                        {
                            currentMidpoint = 0;
                        }
                        glyphIndex++;
                    }
                    highlightEndIndex = glyphIndex;

                    // If the highlighted part of the glyph run is non-empty, split the glyph run into 3 possible parts:
                    // 1) pre-highlight part ? original glyph run trimmed to highlightStartIndex (might be empty)
                    // 2) highlighted part ? from highlightStartIndex up to highlightEndIndex (exclusive).
                    //    Original glyph run might be reused, if the first part is empty.
                    // 3) post-highlight part ? starting from highlightEndIndex (might be empty)
                    if (highlightStartIndex != highlightEndIndex)
                    {
                        // Create glyph run to represent selected content.
                        // If selection starts at 0, don't create a new glyph run since already existing
                        // one will be reused.
                        if (highlightStartIndex > 0)
                        {
                            IFC_RETURN(CreateGlyphRun(glyphRunRemainder, highlightStartIndex, highlightEndIndex - highlightStartIndex, &highlightGlyphRun));
                        }

                        // Create glyph run to represent content after selection
                        if (highlightEndIndex < remainderGlyphCount)
                        {
                            IFC_RETURN(CreateGlyphRun(glyphRunRemainder, highlightEndIndex, remainderGlyphCount - highlightEndIndex, &postHighlightGlyphRun));
                        }

                        // If selection starts at 0, change the brush for already added glyph run.
                        // Otherwise trim the original glyph run and add a new glyph run (selected) to the collection.
                        if (highlightStartIndex == 0)
                        {
                            ASSERT(highlightGlyphRun == nullptr);
                            glyphRunRemainder->GlyphCount = highlightEndIndex;

                            IFC_RETURN(AddGlyphRun(
                                glyphRunRemainder,
                                currentPosition,
                                lineBounds,
                                m_currentLineInfo.InvertHorizontalAxis,
                                xref::weakref_ptr<CDependencyObject>(),
                                highlightBrush,
                                pClipRect,
                                c_allSupportedGlyphImageFormats));
                            glyphRunRemainder = nullptr;
                        }
                        else
                        {
                            ASSERT(highlightGlyphRun != nullptr);
                            glyphRunRemainder->GlyphCount = highlightStartIndex;

                            IFC_RETURN(AddGlyphRun(
                                glyphRunRemainder,
                                currentPosition,
                                lineBounds,
                                m_currentLineInfo.InvertHorizontalAxis,
                                pBrushSource,
                                alternativeForegroundBrush,
                                pClipRect,
                                c_allSupportedGlyphImageFormats));
                            glyphRunRemainder = nullptr;

                            IFC_RETURN(AddGlyphRun(
                                highlightGlyphRun,
                                highlightStartPosition,
                                lineBounds,
                                m_currentLineInfo.InvertHorizontalAxis,
                                xref::weakref_ptr<CDependencyObject>(),
                                highlightBrush,
                                pClipRect,
                                c_allSupportedGlyphImageFormats));
                            highlightGlyphRun = nullptr;
                        }

                        // Current glyph run remainder should have already been added so it can be used for the next segment.
                        ASSERT(glyphRunRemainder == nullptr);
                        glyphRunRemainder = postHighlightGlyphRun;
                        postHighlightGlyphRun = nullptr;

                        currentPosition = highlightEndPosition;
                    }
                }
            }
        }
    }

    // Default path - if not in high contrast or no intersection with selection was determined, add the
    // run as-is.
    if (glyphRunRemainder != nullptr)
    {
        IFC_RETURN(AddGlyphRun(
            glyphRunRemainder,
            currentPosition,
            lineBounds,
            m_currentLineInfo.InvertHorizontalAxis,
            pBrushSource,
            alternativeForegroundBrush,
            pClipRect,
            c_allSupportedGlyphImageFormats));
        glyphRunRemainder = nullptr;
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Generates rendering data for run of glyphs after pre-processing by the shaping engine.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT D2DTextDrawingContext::DrawGlyphs(
    _In_ const XPOINT &position,
    _In_ XINT32 runWidth,
    _In_reads_(textLength) WCHAR const* pTextString,
    _In_reads_(textLength) XUINT16 const* pClusterMap,
    _In_reads_(textLength) FssShapingTextProperties const* pTextProps,
    _In_ XUINT32 textLength,
    _In_reads_(glyphCount) XUINT16 const* pGlyphIndices,
    _In_reads_(glyphCount) FssShapingGlyphProperties const* pGlyphProps,
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
    )
{
    PALText::GlyphRun *pGlyphRun = new PALText::GlyphRun();
    XUINT16 *pNewGlyphIndices = new XUINT16[glyphCount];
    XFLOAT *pNewGlyphAdvances = new XFLOAT [glyphCount];
    PALText::GlyphOffset *pNewGlyphOffsets = new PALText::GlyphOffset[glyphCount];
    XPOINTF newPosition;
    XUINT32 glyphIndex;

    auto guard = wil::scope_exit([&] ()
    {
        delete [] pGlyphIndices;
        delete [] pGlyphAdvances;
        delete [] pGlyphOffsets;
        ReleaseGlyphRun(pGlyphRun);
    });

    // Convert glyph advances and positions from text measurement units to display units.

    memcpy(pNewGlyphIndices, pGlyphIndices, glyphCount * sizeof(XUINT16));
    for (glyphIndex = 0; glyphIndex < glyphCount; glyphIndex++)
    {
        pNewGlyphAdvances[glyphIndex]                = FromTextDpi(pGlyphAdvances[glyphIndex]);
        pNewGlyphOffsets [glyphIndex].AdvanceOffset  = FromTextDpi(pGlyphOffsets[glyphIndex].x);
        pNewGlyphOffsets [glyphIndex].AscenderOffset = FromTextDpi(pGlyphOffsets[glyphIndex].y);
    }

    // Initialize glyph run structure and pass it to the drawing context.

    pGlyphRun->FontFace      = pFontFace;
    pGlyphRun->FontEmSize    = fontSize;
    pGlyphRun->GlyphCount    = glyphCount;
    pGlyphRun->GlyphIndices  = pNewGlyphIndices;
    pGlyphRun->GlyphAdvances = pNewGlyphAdvances;
    pGlyphRun->GlyphOffsets  = pNewGlyphOffsets;
    pGlyphRun->IsSideways    = isSideways;
    pGlyphRun->BidiLevel     = bidiLevel;

    AddRefInterface(pGlyphRun->FontFace);

    newPosition.x = FromTextDpi(position.x);
    newPosition.y = FromTextDpi(position.y);

    IFC_RETURN(DrawGlyphRun(newPosition, FromTextDpi(runWidth), pGlyphRun, pBrushSource, pClipRect));

    guard.release();

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Generates rendering data for a line.
//
//---------------------------------------------------------------------------
Result::Enum D2DTextDrawingContext::DrawLine(
    _In_ const XPOINTF &position,
        // The start position of the line.
    _In_ XFLOAT width,
        // The width of the line.
    _In_ XFLOAT thickness,
        // Thickness of the line.
    _In_ XUINT8 bidirectionalLevel,
        // The bidirectional nesting level, even for LTR runs, odd for RTL runs
    _In_ const xref::weakref_ptr<CDependencyObject>& pBrushSource,
        // The source of foreground brush.
    _In_opt_ XRECTF_RB const* pClipRect
        // The clip rect to apply to the run.
    )
{
    Result::Enum txhr = Result::Success;
    XPOINTF bidiAdjustedPosition = position;

    //
    // Manually apply the clip if provided.
    //
    if (pClipRect)
    {
        XFLOAT left = bidiAdjustedPosition.x;
        XFLOAT right = bidiAdjustedPosition.x + width;
        left = MAX(pClipRect->left, left);
        right = MAX(left, MIN(right, pClipRect->right));

        bidiAdjustedPosition.x = left;
        width = right - left;

        // If provided these values need to be sentinels because we are ignoring them.
        ASSERT(pClipRect->top <= XFLOAT_MIN);
        ASSERT(pClipRect->bottom >= XFLOAT_MAX);
    }

    // Add line data to the collection representing rendering data.
    XUINT32 currentLineIndex = m_lines.GetCount();
    IFC_FROM_HRESULT_RTS(m_lines.SetCount(currentLineIndex + 1));
    m_lines[currentLineIndex].Width = width;
    m_lines[currentLineIndex].Thickness = thickness;
    m_lines[currentLineIndex].BrushSource = pBrushSource;
    m_lines[currentLineIndex].Brush = NULL;
    m_lines[currentLineIndex].PALBrushParams = NULL;

    if (bidirectionalLevel & 1)
    {
        // Adjust the line by its width, since RTL line will start on the right.
        bidiAdjustedPosition.x -= width;
    }

    GetLocalTransform(
        bidiAdjustedPosition.x,
        bidiAdjustedPosition.y,
        m_currentLineInfo.InvertHorizontalAxis,
        m_currentLineInfo.ViewportWidth,
        &m_lines[currentLineIndex].Transform);

Cleanup:
    return txhr;
}

//---------------------------------------------------------------------------
//
//  Generates rendering data for a wavy line.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
D2DTextDrawingContext::DrawWavyLine(
            _In_ XRECTF_RB *pRect, // The rect to draw the wavy line.
            _In_ XRECTF_RB *pCurrentClipRect, // Current clipping rectangle
            _In_ DWORD color, // Color for the Wavy line.
            _In_ XINT32 insertPosition, // if -1 then append to the end of the m_images
            _In_opt_ XUINT32* pPixelBuffer // Pixel buffer
            )
{
    XUINT32 *pPixels = NULL;

    CValue value;
    CREATEPARAMETERS param(m_pTextCore->GetCoreServices(), value);

    if (IntersectRect(pRect, pCurrentClipRect))
    {
        XRECTF_RB rcUnderlineBitmapSpace = *pRect;

        XUINT32 width = static_cast<XUINT32>(rcUnderlineBitmapSpace.right - rcUnderlineBitmapSpace.left);
        XUINT32 requestedHeight = static_cast<XUINT32>(rcUnderlineBitmapSpace.bottom - rcUnderlineBitmapSpace.top);

        if (requestedHeight <= WavyLineHeight)
        {
            // The wavy line is drawn taller than requested. Push the line down halfway but make sure it does not go lower than the clip's lower edge.
            rcUnderlineBitmapSpace.bottom = MIN(pCurrentClipRect->bottom, rcUnderlineBitmapSpace.bottom + (WavyLineHeight - requestedHeight + 1) / 2);
            rcUnderlineBitmapSpace.top = rcUnderlineBitmapSpace.bottom - WavyLineHeight;
        }
        else
        {
            // The wavy line is drawn smaller than requested. Place the line on the upper edge, leaving a standard upper margin,
            // but make sure it does not go lower than the clip's lower edge.
            rcUnderlineBitmapSpace.top = MIN(rcUnderlineBitmapSpace.top + WavyLineTopMargin, rcUnderlineBitmapSpace.bottom - WavyLineHeight);
            rcUnderlineBitmapSpace.bottom = rcUnderlineBitmapSpace.top + WavyLineHeight;
        }

        if (pPixelBuffer == NULL)
        {
            IFC_RETURN(RasterizeWavyLine(&rcUnderlineBitmapSpace, color, &pPixels));
        }
        else
        {
            ASSERT(sizeof(pPixelBuffer) == (width*WavyLineHeight*sizeof(XUINT32)));
            pPixels = pPixelBuffer;
        }

        xref_ptr<CImageBrush> pImageBrush;
        IFC_RETURN(CImageBrush::Create(reinterpret_cast<CDependencyObject **>(pImageBrush.ReleaseAndGetAddressOf()), &param));

        xref_ptr<CWriteableBitmap> pWriteableBitmap;
        IFC_RETURN(CWriteableBitmap::Create(reinterpret_cast<CDependencyObject **>(pWriteableBitmap.ReleaseAndGetAddressOf()), &param));

        IFC_RETURN(pWriteableBitmap->Create(pPixels, width, WavyLineHeight));
        pImageBrush->m_pImageSource = pWriteableBitmap.detach();

        IFC_RETURN(DrawRectangle(&rcUnderlineBitmapSpace, pImageBrush, color, pPixels, insertPosition));
    }

    return S_OK;
}
//---------------------------------------------------------------------------
//
//  Clears all rendering data.
//
//---------------------------------------------------------------------------
void D2DTextDrawingContext::Clear()
{
    memset(&m_currentLineInfo, 0, sizeof(LineInfo));

    // Release glyph run rendering data.
    for (XUINT32 i = 0, collectionSize = m_glyphRuns.GetCount(); i < collectionSize; i++)
    {
        GlyphRunData * pGlyphRunData = &m_glyphRuns[i];
        ReleaseGlyphRun(pGlyphRunData->GlyphRun);
        ReleaseInterface(pGlyphRunData->Brush);
        SAFE_DELETE(pGlyphRunData->PALBrushParams);
    }
    m_glyphRuns.Clear();

    // Release line rendering data.
    for (XUINT32 i = 0, collectionSize = m_lines.GetCount(); i < collectionSize; i++)
    {
        ReleaseInterface(m_lines[i].Brush);
        SAFE_DELETE(m_lines[i].PALBrushParams);
    }
    m_lines.Clear();

    // Release image rendering data.
    for (XUINT32 i = 0, collectionSize = m_images.GetCount(); i < collectionSize; i++)
    {
        ReleaseInterface(m_images[i].Brush);
        SAFE_DELETE_ARRAY(m_images[i].pPixelBuffer);
    }
    m_images.Clear();

    ClearGlyphRunCaches();
    SAFE_DELETE(m_pGlyphRunRealizations);
    ReleaseInterface(m_pBaseRealization);
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//       Clears the cached glyph run data (bitmaps, offsets, edge stores)...
//
//------------------------------------------------------------------------------
void D2DTextDrawingContext::ClearGlyphRunCaches()
{
    // Release cached glyph textures.
    SaveAndClearGlyphRunRealizations(nullptr);
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Clears the cached glyph run realizations.
//      Saves them to HWWalk if the render params is provided.
//      Saving is needed for RenderTargetBitmaps to keep the realizations alive.
//
//------------------------------------------------------------------------------
void D2DTextDrawingContext::SaveAndClearGlyphRunRealizations(_In_opt_ const HWRenderParams* const pRenderParams)
{
    if (m_pGlyphRunRealizations != nullptr)
    {
        ASSERT(pRenderParams == nullptr);
        for (XUINT32 i = 0, collectionSize = m_pGlyphRunRealizations->GetCount(); i < collectionSize; i++)
        {
            ReleaseInterface((*m_pGlyphRunRealizations)[i].TextRealization);
        }
        m_pGlyphRunRealizations->Clear();
    }
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      The method to clean up all the glyph realizations
//      on this object.
//
//-----------------------------------------------------------------------------
void D2DTextDrawingContext::CleanupRealizations()
{
    // Release the image brushs associated with the wavy lines, but keep the wavy line rectangles for re-realization.
    // The pixels in system memory are explicitly left intact because they will survive the device loss.
    for (XUINT32 i = 0, collectionSize = m_images.GetCount(); i < collectionSize; i++)
    {
        ImageData * pImageData = &m_images[i];
        ReleaseInterface(pImageData->Brush);
    }

    // Cleanup text realizations.
    // We do not want to clean up the brushes as they might
    // be shared resources. They will get cleaned up when they leave or
    // through their dependency object owners..
    SaveAndClearGlyphRunRealizations(nullptr);
}

bool D2DTextDrawingContext::BaseRealizationHasSubPixelOffsets() const
{
    return ((m_pBaseRealization != nullptr) && m_pBaseRealization->HasSubPixelOffsets());
}

void D2DTextDrawingContext::ClearBaseRealization()
{
    if (m_pBaseRealization != nullptr)
    {
        ReleaseInterface(m_pBaseRealization);
    }
}

//---------------------------------------------------------------------------
//
//  Determines whether rendering data has been already populated.
//
//---------------------------------------------------------------------------
bool D2DTextDrawingContext::HasRenderingData() const
{
    return (m_glyphRuns.GetCount() != 0 || m_lines.GetCount() != 0 || m_images.GetCount() != 0);
}

//---------------------------------------------------------------------------
//
//  Sets the line bounds to be used for generated glyph runs.
//
//---------------------------------------------------------------------------
void D2DTextDrawingContext::SetLineInfo(
    _In_ XFLOAT viewportWidth,
    _In_ bool invertHorizontalAxis,
    _In_ XFLOAT yOffset,
    _In_ XFLOAT verticalAdvance
    )
{
    m_currentLineInfo.ViewportWidth = viewportWidth;
    m_currentLineInfo.InvertHorizontalAxis = invertHorizontalAxis;
    m_currentLineInfo.YOffset = yOffset;
    m_currentLineInfo.VerticalAdvance = verticalAdvance;
}

//---------------------------------------------------------------------------
//
//  Sets whether color in fonts should be enabled for text drawing operations.
//
//---------------------------------------------------------------------------
void D2DTextDrawingContext::SetIsColorFontEnabled(
    bool isColorFontEnabled
    )
{
    m_isColorFontEnabled = isColorFontEnabled;
}

// Sets the palette index to use for color glyphs if color fonts are enabled.
void D2DTextDrawingContext::SetColorFontPaletteIndex(
    UINT32 colorFontPaletteIndex
    )
{
    m_colorFontPaletteIndex = colorFontPaletteIndex;
}

void D2DTextDrawingContext::ClearForegroundHighlightInfo()
{
    m_highlights.clear();
}

void D2DTextDrawingContext::AppendForegroundHighlightInfo(
    uint32_t count,
    _In_reads_(count) XRECTF *pRectangles,
    _In_ CSolidColorBrush* foregroundBrush,
    uint32_t startIndex,
    uint32_t countInParagraph,
    _In_ const XPOINTF& paragraphOffset
    )
{
    // D2DTextDrawingContext doesn't own any of the information in the context, e.g. the highlight rect array, so there's no need to
    // delete anything, just overwrite the values.
    auto highlightInfo = wil::make_unique_failfast<HighlightInfo>();
    highlightInfo->Count = count;
    highlightInfo->StartIndex = startIndex;
    highlightInfo->CountInParagraph = countInParagraph;
    highlightInfo->ParagraphOffset = paragraphOffset;
    highlightInfo->ForegroundBrush = foregroundBrush;

    // Make a copy of the rectangles so they aren't dependent on outside allocates.  This list will also be manipulated later to
    // to de-overlap rects.
    for (uint32_t i = 0; i < count; i++)
    {
        highlightInfo->Rectangles.push_back(pRectangles[i]);
    }

    m_highlights.push_back(std::move(highlightInfo));
}

//---------------------------------------------------------------------------
//
//  Gets a transform that includes original position and mirroring.
//
//---------------------------------------------------------------------------
void D2DTextDrawingContext::GetLocalTransform(
    _In_ XFLOAT x,
    _In_ XFLOAT y,
    _In_ bool invertHorizontalAxis,
    _In_ XFLOAT viewportWidth,
    _Out_ CMILMatrix *pLocalTransform
    )
{
    CMILMatrix positionTransform(TRUE);
    positionTransform.SetDx(x);
    positionTransform.SetDy(y);

    if (invertHorizontalAxis)
    {
        CMILMatrix lineMirrorTransform(TRUE);
        lineMirrorTransform.SetM11(-1.0f);
        lineMirrorTransform.SetM22(1.0f);
        lineMirrorTransform.SetDx(viewportWidth);
        positionTransform.Append(lineMirrorTransform);
    }
    *pLocalTransform = positionTransform;
}

//---------------------------------------------------------------------------
//
//  Adds a run to the GlyphRunData collection.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT D2DTextDrawingContext::AddGlyphRun(
    _In_ PALText::GlyphRun *pGlyphRun,
    _In_ const XPOINTF& baselineOffset,
    _In_ const XRECTF& lineBounds,
    _In_ bool invertHorizontalAxis,
    _In_ const xref::weakref_ptr<CDependencyObject>& pBrushSource,
    _In_opt_ CBrush *pBrush,
    _In_opt_ XRECTF_RB const* pClipRect,
    _In_ DWRITE_GLYPH_IMAGE_FORMATS glyphImageFormat
    )
{
    HRESULT hr = S_OK;
    bool translatedToColorGlyphRun = false;
    bool shouldTranslateColorFonts = false;

    // Break the formats down further if all were passed. Otherwise a single format was passed.
    if (glyphImageFormat == c_allSupportedGlyphImageFormats)
    {
        IDWriteFontFace4* fontFace = GetDWriteFontFace(pGlyphRun->FontFace);
        DWRITE_GLYPH_IMAGE_FORMATS actualGlyphImageFormats = fontFace->GetGlyphImageFormats();

        // Lest either the color checks yield false or TranslateColorGlyphRun returns no color,
        // set the glyph format to only the monochromatic glyph outlines (TrueType or CFF fonts).
        glyphImageFormat = (actualGlyphImageFormats & c_monochromeGlyphImageFormats);

        if (actualGlyphImageFormats & c_anyBitmapGlyphImageFormats)
        {
            // Always translate color bitmap fonts regardless of the higher level settings
            // (e.g. TextBlock.IsColorFontEnabled) because otherwise nothing would be drawn,
            // as color bitmap fonts do not contain monochrome outlines.
            shouldTranslateColorFonts = TRUE;
        }
        else if (m_isColorFontEnabled
            &&  (actualGlyphImageFormats & c_supportedColorGlyphImageFormats)
            &&  !m_pTextCore->GetCoreServices()->GetFrameworkTheming()->HasHighContrastTheme())
        {
            // Else translate other formats like COLR or SVG if color is enabled and
            // high contrast is disabled. Otherwise use the fallback monochromatic outlines
            // (TrueType or CFF).
            shouldTranslateColorFonts = TRUE;
        }
    }

    // Recurse back into this function with the specific glyph image formats.
    if (shouldTranslateColorFonts)
    {
        IFC(TranslateToColorGlyphRun(
            pGlyphRun,
            baselineOffset,
            lineBounds,
            invertHorizontalAxis,
            pBrushSource,
            pBrush,
            pClipRect,
            &translatedToColorGlyphRun));
    }

    if (!translatedToColorGlyphRun)
    {
        XUINT32 currentGlyphIndex = m_glyphRuns.GetCount();

        IFC(m_glyphRuns.SetCount(currentGlyphIndex + 1));
        m_glyphRuns[currentGlyphIndex].GlyphRun = pGlyphRun;
        m_glyphRuns[currentGlyphIndex].BaselineOffset = baselineOffset;
        m_glyphRuns[currentGlyphIndex].LineBounds = lineBounds;
        m_glyphRuns[currentGlyphIndex].InvertHorizontalAxis = invertHorizontalAxis;
        m_glyphRuns[currentGlyphIndex].BrushSource = pBrushSource;
        m_glyphRuns[currentGlyphIndex].Brush = pBrush;
        m_glyphRuns[currentGlyphIndex].PALBrushParams = NULL;
        if (pClipRect)
        {
            m_glyphRuns[currentGlyphIndex].GlyphRunClipLeft = pClipRect->left;
            m_glyphRuns[currentGlyphIndex].GlyphRunClipRight = pClipRect->right;

            // If provided these values need to be sentinels because we are ignoring them.
            ASSERT(pClipRect->top <= XFLOAT_MIN);
            ASSERT(pClipRect->bottom >= XFLOAT_MAX);
        }
        else
        {
            m_glyphRuns[currentGlyphIndex].GlyphRunClipLeft = XFLOAT_MIN;
            m_glyphRuns[currentGlyphIndex].GlyphRunClipRight = XFLOAT_MAX;
        }
        EmptyRectF(&m_glyphRuns[currentGlyphIndex].GlyphRunRealizationBounds);
        m_glyphRuns[currentGlyphIndex].GlyphRunRealizationTransform.SetToIdentity();
        m_glyphRuns[currentGlyphIndex].GlyphImageFormat = glyphImageFormat;

        AddRefInterface(pBrush);

        pGlyphRun = NULL;
    }

Cleanup:
    //
    // pGlyphRun will be non-NULL if ownership was not transferred to m_glyphRuns, so release it now.
    //
    ReleaseGlyphRun(pGlyphRun);
    return hr;
}


//---------------------------------------------------------------------------
//
// Translate a glyph run using a font that has a COLR table to the color glyph
// runs which should be drawn in its place.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT D2DTextDrawingContext::TranslateToColorGlyphRun(
    _In_ PALText::GlyphRun *pGlyphRun,
    _In_ const XPOINTF& baselineOffset,
    _In_ const XRECTF& lineBounds,
    _In_ bool invertHorizontalAxis,
    _In_ const xref::weakref_ptr<CDependencyObject>& pBrushSource,
    _In_opt_ CBrush *pBrush,
    _In_opt_ XRECTF_RB const* pClipRect,
    _Out_ bool *hadColorGlyphs
    )
{
    *hadColorGlyphs = FALSE;

    DWRITE_GLYPH_RUN dwriteGlyphRun = GetDWriteGlyphRun(*pGlyphRun);

    Microsoft::WRL::ComPtr<IDWriteFactory4> dWriteFactory4;
    IFC_RETURN(m_pTextCore->GetWinTextCore()->GetDWriteFactory(&dWriteFactory4));

    //
    // Kick off the enumeration.
    //

    Microsoft::WRL::ComPtr<IDWriteColorGlyphRunEnumerator1> DWriteColorGlyphRunEnumerator;

    HRESULT hrTranslate = dWriteFactory4->TranslateColorGlyphRun(
        { baselineOffset.x, baselineOffset.y },
        &dwriteGlyphRun,
        NULL,
        c_allSupportedGlyphImageFormats,
        DWRITE_MEASURING_MODE_NATURAL,
        NULL,
        m_colorFontPaletteIndex,
        DWriteColorGlyphRunEnumerator.ReleaseAndGetAddressOf());

    //
    // If this returned DWRITE_E_NOCOLOR it means the font supported color glyphs but
    // this run didn't have any.  Or this can also happen if m_colorFontPaletteIndex
    // is an invalid index, in which case we want to fallback to non-color rendering.
    //

    if (hrTranslate == DWRITE_E_NOCOLOR)
    {
        // We are using !=0 check because we are interested in logging non-default index case.
        // Non zero index suggests at least one color glyph is used, and therefore we are
        // logging because of the developer or platform failure.
        if (m_colorFontPaletteIndex != 0)
        {
            TraceLoggingWrite(
                g_hTraceProvider,
                "NoColorGlyphRendering",
                TraceLoggingUInt32(m_colorFontPaletteIndex, "ColorFontPaletteIndex"),
                TraceLoggingLevel(WINEVENT_LEVEL_LOG_ALWAYS),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY));
        }

        return S_OK;
    }

    IFC_RETURN(hrTranslate);

    *hadColorGlyphs = TRUE;

    //
    // Retrieve runs one at a time, and call back into AddGlyphRun with the resulting runs.
    //

    BOOL hasRun;
    while (SUCCEEDED(DWriteColorGlyphRunEnumerator->MoveNext(&hasRun)) && hasRun)
    {
        const DWRITE_COLOR_GLYPH_RUN1 *pColorGlyphRun;
        IFC_RETURN(DWriteColorGlyphRunEnumerator->GetCurrentRun(&pColorGlyphRun));

        if (pColorGlyphRun->glyphImageFormat == DWRITE_GLYPH_IMAGE_FORMATS_NONE)
            continue; // Skip any empty base glyph run.

        //
        // Glyph run enumeration to does not change the font.
        //

        ASSERT(pColorGlyphRun->glyphRun.fontFace == dwriteGlyphRun.fontFace);

        PALText::GlyphRun palGlyphRun = {
            pGlyphRun->FontFace,
            pColorGlyphRun->glyphRun.fontEmSize,
            pColorGlyphRun->glyphRun.glyphCount,
            pColorGlyphRun->glyphRun.glyphIndices,
            pColorGlyphRun->glyphRun.glyphAdvances,
            reinterpret_cast<PALText::GlyphOffset const*>(pColorGlyphRun->glyphRun.glyphOffsets),
            !!pColorGlyphRun->glyphRun.isSideways,
            pColorGlyphRun->glyphRun.bidiLevel
        };

        XPOINTF colorGlyphRunBaselineOffset = { pColorGlyphRun->baselineOriginX, pColorGlyphRun->baselineOriginY };

        //
        // Copy the glyph run data out with CreateGlyphRun.
        //

        PALText::GlyphRun *newGlyphRun = nullptr;

        auto releaseGlyphRunsOnExit = wil::scope_exit([&newGlyphRun] ()
        {
            ReleaseGlyphRun(newGlyphRun);
        });

        IFC_RETURN(CreateGlyphRun(&palGlyphRun, 0, palGlyphRun.GlyphCount, &newGlyphRun));

        //
        // Make a brush with the color specified by pColorGlyphRun.  If paletteIndex is 0xFFFF,
        // this means we are supposed to use the default/foreground brush associated with
        // the original run.  So we won't create a brush and then below when pSolidColorBrush
        // is NULL we'll pass the original brushes through.
        //

        xref_ptr<CSolidColorBrush> solidColorBrush;

        if (pColorGlyphRun->paletteIndex != 0xFFFF)
        {
            CValue value;
            CREATEPARAMETERS param(m_pTextCore->GetCoreServices(), value);
            IFC_RETURN(CSolidColorBrush::Create(reinterpret_cast<CDependencyObject **>(solidColorBrush.ReleaseAndGetAddressOf()), &param));

            XINT32 r = static_cast<XINT32>(pColorGlyphRun->runColor.r * 255);
            XINT32 g = static_cast<XINT32>(pColorGlyphRun->runColor.g * 255);
            XINT32 b = static_cast<XINT32>(pColorGlyphRun->runColor.b * 255);

            solidColorBrush->m_rgb = 0xFF000000 | (r << 16) | (g << 8) | b;
            solidColorBrush->m_eOpacity = pColorGlyphRun->runColor.a;
        }

        //
        // Finally, pass the data off to AddGlyphRun.
        //

        IFC_RETURN(AddGlyphRun(
            newGlyphRun,
            colorGlyphRunBaselineOffset,
            lineBounds,
            invertHorizontalAxis,
            solidColorBrush ? xref::weakref_ptr<CDependencyObject>() : pBrushSource,
            solidColorBrush ? solidColorBrush : pBrush,
            pClipRect,
            pColorGlyphRun->glyphImageFormat
            ));

        releaseGlyphRunsOnExit.release();
    }

    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Creates new glyph run which is a sub-run of the original run.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT D2DTextDrawingContext::CreateGlyphRun(
    _In_ PALText::GlyphRun *pOriginalGlyphRun,
    _In_ XUINT32 startIndex,
    _In_ XUINT32 glyphCount,
    _Outptr_ PALText::GlyphRun **ppGlyphRun
    )
{
    PALText::GlyphRun *pGlyphRun = new PALText::GlyphRun();
    XUINT16 *pGlyphIndices = new XUINT16[glyphCount];
    XFLOAT *pGlyphAdvances = new XFLOAT [glyphCount];
    PALText::GlyphOffset *pGlyphOffsets = (pOriginalGlyphRun->GlyphOffsets != nullptr) ? new PALText::GlyphOffset[glyphCount] : nullptr;

    auto guard = wil::scope_exit([&] ()
    {
        delete [] pGlyphIndices;
        delete [] pGlyphAdvances;
        delete [] pGlyphOffsets;
        ReleaseGlyphRun(pGlyphRun);
    });

    for (XUINT32 i = 0; i < glyphCount; i++)
    {
        pGlyphIndices[i]  = pOriginalGlyphRun->GlyphIndices[startIndex + i];
        pGlyphAdvances[i] = pOriginalGlyphRun->GlyphAdvances[startIndex + i];

        if (pOriginalGlyphRun->GlyphOffsets)
        {
            pGlyphOffsets[i] = pOriginalGlyphRun->GlyphOffsets[startIndex + i];
        }
    }

    pGlyphRun->BidiLevel     = pOriginalGlyphRun->BidiLevel;
    pGlyphRun->FontEmSize    = pOriginalGlyphRun->FontEmSize;
    pGlyphRun->FontFace      = pOriginalGlyphRun->FontFace;
    pGlyphRun->GlyphAdvances = pGlyphAdvances;
    pGlyphRun->GlyphCount    = glyphCount;
    pGlyphRun->GlyphIndices  = pGlyphIndices;
    pGlyphRun->GlyphOffsets  = pGlyphOffsets;
    pGlyphRun->IsSideways    = pOriginalGlyphRun->IsSideways;

    AddRefInterface(pGlyphRun->FontFace);
    *ppGlyphRun = pGlyphRun;
    guard.release();

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Releases all resources associated with glyph run.
//
//---------------------------------------------------------------------------
void D2DTextDrawingContext::ReleaseGlyphRun(
    _In_opt_ PALText::GlyphRun *pGlyphRun
    )
{
    if (pGlyphRun != NULL)
    {
        delete [] pGlyphRun->GlyphIndices;
        delete [] pGlyphRun->GlyphAdvances;
        delete [] pGlyphRun->GlyphOffsets;
        ReleaseInterface(pGlyphRun->FontFace);
        delete pGlyphRun;
    }
}

//---------------------------------------------------------------------------
//
//  PC based rendering.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
D2DTextDrawingContext::HWRender(
    _In_ IContentRenderer* pContentRenderer
    )
{
    HRESULT hr = S_OK;

    //
    // Release cached glyph textures if there's a new realization, if realizations need to be
    // updated, or if a realization texture was lost.
    //
    bool needToResetRealization = false;
    bool createTemporaryRealizations = false;

    // If new realizations are needed for RenderTargetBitmap, then
    // 1) Store away the original realizations and swap them with nulls.
    // 2) Recreate the new realizations and render using them.
    // 3) Save references of newly created realizations on RenderTargetBitmap,
    //     so that they are alive until they are submitted.
    // 4) Swap in the original realizations.
    DynamicArray<GlyphRunRealization> *pGlyphRunRealizationsOriginal = NULL;
    HWTextRealization *pBaseRealizationOriginal = NULL;
    const HWRenderParams& rp = *(pContentRenderer->GetRenderParams());

    const CMILMatrix rasterizationMatrix = pContentRenderer->GetTransformToRoot()->GetRasterizationMatrix(pContentRenderer->GetUIElement());

    if (m_pBaseRealization == NULL)
    {
        m_pBaseRealization = new HWTextRealization();
        needToResetRealization = TRUE;
    }

    if (m_pGlyphRunRealizations == NULL)
    {
        m_pGlyphRunRealizations = new DynamicArray<GlyphRunRealization>();
    }

    needToResetRealization |= m_pBaseRealization->NeedsUpdate(&rasterizationMatrix, rp.isTransformAnimating);

    if (!needToResetRealization)
    {
        for (XUINT32 i = 0, collectionSize = m_pGlyphRunRealizations->GetCount(); i < collectionSize; i++)
        {
            if ((*m_pGlyphRunRealizations)[i].TextRealization->HasLostRealizationTexture())
            {
                needToResetRealization = TRUE;
                break;
            }
        }
    }

    if (needToResetRealization)
    {
        ClearGlyphRunCaches();
        m_pBaseRealization->UpdateRealizationParameters(&rasterizationMatrix, rp.isTransformAnimating, 0, 0, false /* renderCollapsedMask */);
    }

    //
    // Generate glyph textures, if necessary
    //
    if (m_pGlyphRunRealizations->GetCount() == 0 && m_glyphRuns.GetCount() > 0)
    {
        m_pBaseRealization->UpdateRealizationParameters(&rasterizationMatrix, rp.isTransformAnimating, 0, 0, false /* renderCollapsedMask */);
        IFC(HWBuildGlyphRunTextures(&rp, &rasterizationMatrix));
    }

    // Render the lines
    IFC(HWRenderLines(pContentRenderer));

    // Render glyph textures
    IFC(HWRenderGlyphTextures(pContentRenderer));

    // Render images
    IFC(EnsureWavyLineRealization());
    IFC(HwRenderImages(pContentRenderer));

    if (createTemporaryRealizations)
    {
        SaveAndClearGlyphRunRealizations(&rp);
    }

Cleanup:
    if (createTemporaryRealizations)
    {
        if (!SUCCEEDED(hr))
        {
            SaveAndClearGlyphRunRealizations(nullptr);
        }
        SWAP(DynamicArray<GlyphRunRealization> *, pGlyphRunRealizationsOriginal, m_pGlyphRunRealizations);
        SWAP(HWTextRealization *, pBaseRealizationOriginal, m_pBaseRealization);
        ReleaseInterface(pBaseRealizationOriginal);
        SAFE_DELETE(pGlyphRunRealizationsOriginal);
    }

    return hr;
}

//---------------------------------------------------------------------------
//
// Ensure every entries in the m_images has a realized CImageBrush*.
// Note that wavy line is the only type of image we have.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT D2DTextDrawingContext::EnsureWavyLineRealization()
{
    XUINT32 wavyLineCount = m_images.GetCount();

    if (wavyLineCount > 0)
    {
        // For any wavy line that does not have a brush, rasterize one and append it to the m_images.
        for (XUINT32 i = 0, collectionSize = m_images.GetCount(); i < collectionSize; i++)
        {
            ImageData * pImageData = &m_images[i];
            if (pImageData->Brush == NULL)
            {
                ASSERT(pImageData->pPixelBuffer != NULL);
                // The rectangle passed in has already been clipped.
                IFC_RETURN(DrawWavyLine(&pImageData->ImageRect,&pImageData->ImageRect, pImageData->color, i, pImageData->pPixelBuffer));
            }
        }
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Draw the wavy line pixel by pixel into a buffer.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT D2DTextDrawingContext::RasterizeWavyLine(
    _In_ XRECTF_RB const* pRect,
    _In_ DWORD color,
    _Out_ XUINT32 **ppBitmap)
{
    HRESULT hr = S_OK;
    XUINT32 *pBitmap = NULL;

    static XBYTE Pattern[WavyLineHeight][WavyLineWidth] = {
        { 0, 1, 1, 1, 0, 0, 0, 0 },
        { 1, 1, 0, 1, 1, 0, 0, 0 },
        { 1, 0, 0, 0, 1, 1, 0, 1 },
        { 0, 0, 0, 0, 0, 1, 1, 1 }
    };

    XUINT32 width = static_cast<XUINT32>(pRect->right - pRect->left);

    ASSERT(static_cast<XUINT32>(pRect->bottom - pRect->top) == WavyLineHeight);

    pBitmap = new XUINT32[width*WavyLineHeight];
    memset(pBitmap, 0, width*WavyLineHeight*sizeof(XUINT32));

    XINT32 patternXStart = static_cast<XINT32>(pRect->left) % WavyLineWidth;
    if (patternXStart < 0) { patternXStart += WavyLineWidth; }

    XUINT32 patternYStart = 0;

    for (XUINT32 y = 0, patternY = patternYStart; y < WavyLineHeight; y++, patternY++)
    {
        if (patternY == WavyLineHeight)
        {
            patternY = 0;
        }

        XUINT32* pBitmapRow = reinterpret_cast<XUINT32*>(pBitmap + y * width);

        XUINT32 patternX = patternXStart;
        XBYTE* patternRow = Pattern[patternY];
        for (XUINT32 x = 0; x < width; x++, patternX++)
        {
            if (patternRow[patternX % WavyLineWidth])
            {
                pBitmapRow[x] = color;
            }
        }
    }
    *ppBitmap = pBitmap;
    RRETURN(hr);//RRETURN_REMOVAL
}


//---------------------------------------------------------------------------
//
//  Builds edge stores for lines.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
D2DTextDrawingContext::HWRenderLines(
    _In_ IContentRenderer* pContentRenderer
    )
{
    CMILMatrix lineTransform;
    CMILMatrix rpTransform;
    bool snapToPixel;
    const HWRenderParams& rp = *(pContentRenderer->GetRenderParams());
    HWRenderParams localRP = rp;
    HWRenderParamsOverride hwrpOverride(pContentRenderer, &localRP);
    const CMILMatrix transformToRoot = pContentRenderer->GetTransformToRoot()->Get2DTransformToRoot(pContentRenderer->GetUIElement());

    TransformAndClipStack transformsAndClips;
    IFC_RETURN(transformsAndClips.Set(rp.pTransformsAndClipsToCompNode));
    localRP.pTransformsAndClipsToCompNode = &transformsAndClips;

    // Enable pixel snapping if there is no rotation.
    rp.pTransformsAndClipsToCompNode->Get2DTransformInLeafmostProjection(&rpTransform);
    snapToPixel = transformToRoot.IsScaleOrTranslationOnly();

    for (XUINT32 i = 0, collectionSize = m_lines.GetCount(); i < collectionSize; i++)
    {
        CBrush *pBrushNoRef = nullptr;
        xref_ptr<CSolidColorBrush> alternativeForegroundBrush;

        // Try to get alternative foreground in case HighContrastAdjustment is enabled.
        alternativeForegroundBrush.attach(GetAlternativeForegroundBrush(m_lines[i].BrushSource));

        if (alternativeForegroundBrush != nullptr)
        {
            pBrushNoRef = GetForegroundBrush(m_lines[i].BrushSource, alternativeForegroundBrush, m_controlEnabled);
        }
        else
        {
            pBrushNoRef = GetForegroundBrush(m_lines[i].BrushSource, m_lines[i].Brush, m_controlEnabled);
        }

        if (pBrushNoRef != nullptr)
        {
            lineTransform = m_lines[i].Transform;
            lineTransform.Append(rpTransform);

            if (snapToPixel)
            {
                XFLOAT snapOffsetY = 0.0f;
                XPOINTF position = { 0.0f, 0.0f };
                lineTransform.Transform(&position, &position, 1);
                snapOffsetY = static_cast<XFLOAT>(XcpRound(position.y)) - position.y;
                if (snapOffsetY != 0.0f)
                {
                    CMILMatrix snapTransform(TRUE);
                    snapTransform.SetDy(snapOffsetY);
                    lineTransform.Prepend(snapTransform);
                }
            }

            transformsAndClips.Set2DTransformInLeafmostProjection(lineTransform);

            if (pBrushNoRef->GetTypeIndex() == KnownTypeIndex::SolidColorBrush)
            {
                CSolidColorBrush *pBrush = static_cast<CSolidColorBrush *>(pBrushNoRef);

                XRECTF lineBounds = {0, 0, m_lines[i].Width, m_lines[i].Thickness};

                IFC_RETURN(pContentRenderer->RenderSolidColorRectangle(lineBounds, pBrush));
            }
        }
    }

    return S_OK;
}


//---------------------------------------------------------------------------
//
//  PC image rendering
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
D2DTextDrawingContext::HwRenderImages(
    _In_ IContentRenderer* pContentRenderer
    )
{
    for (XUINT32 i = 0, collectionSize = m_images.GetCount(); i < collectionSize; i++)
    {
        if (m_images[i].Brush)
        {
            XRECTF_RB imageRectRB = m_images[i].ImageRect;
            XRECTF imageRect = {imageRectRB.left,
                                imageRectRB.top,
                                imageRectRB.right - imageRectRB.left,
                                imageRectRB.bottom - imageRectRB.top
                                };

            // The brush bounds should be relative to the upper-left corner of the rect to be rendered
            XRECTF brushBounds;
            m_images[i].Brush->GetNaturalBounds(&brushBounds);
            brushBounds.X += imageRect.X;
            brushBounds.Y += imageRect.Y;

            BrushParams emptyBrushParams;
            IFC_RETURN(pContentRenderer->GeneralImageRenderContent(
                imageRect,
                brushBounds,
                m_images[i].Brush,
                emptyBrushParams,
                nullptr,    // pUIElement
                nullptr,    // pNinegrid
                nullptr,    // pShapeHwTexture
                false       // fIsHollow
                ));
        }
    }

    return S_OK;
}


//---------------------------------------------------------------------------
//
//  PC glyph texture rendering.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
D2DTextDrawingContext::HWRenderGlyphTextures(_In_ IContentRenderer* pContentRenderer)
{
    ASSERT(m_pGlyphRunRealizations != NULL);
    for (XUINT32 i = 0, collectionSize = m_pGlyphRunRealizations->GetCount(); i < collectionSize; i++)
    {
        // TODO_WinRT: Figure out story for solid color animations
        bool allowAnimatedColor = !(*m_pGlyphRunRealizations)[i].HasExplicitBrush;

        IFC_RETURN(pContentRenderer->RenderTextRealization(
            (*m_pGlyphRunRealizations)[i].BrushBounds,
            (*m_pGlyphRunRealizations)[i].TextRealization,
            allowAnimatedColor
            ));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Creates the D2D resources for rendering.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
D2DTextDrawingContext::D2DEnsureResources(
    _In_ const D2DPrecomputeParams& cp,
    _In_ const CMILMatrix *pRenderTransform
    )
{
    ID2D1DeviceContext *pSharedD2DDeviceContextNoRef;

    // Get the shared D2DDeviceContext which can be used to retrieve GlyphRun bounds.
    IFC_RETURN(m_pTextCore->GetWinTextCore()->GetSharedD2D1DeviceContext(&pSharedD2DDeviceContextNoRef));

    for (XUINT32 i = 0, collectionSize = m_glyphRuns.GetCount(); i < collectionSize; i++)
    {
        CBrush *pBrushNoRef = GetForegroundBrush(m_glyphRuns[i].BrushSource, m_glyphRuns[i].Brush, m_controlEnabled);
        if (pBrushNoRef != NULL)
        {
            XRECTF_RB brushBounds = {};

            if (m_glyphRuns[i].PALBrushParams == NULL)
            {
                m_glyphRuns[i].PALBrushParams = new AcceleratedBrushParams();
            }

            // For non solid color brushes need to get actual bounds of the glyph run.
            // This process can be skipped for solid color brushes, since the bounds have
            // no effect on the final rendering output.
            if (pBrushNoRef->GetTypeIndex() != KnownTypeIndex::SolidColorBrush)
            {
                DWRITE_GLYPH_RUN glyphRun = GetDWriteGlyphRun(*m_glyphRuns[i].GlyphRun);
                D2D1_RECT_F d2dBrushBounds;

                //
                // Calculate the glyph run realization transform.
                //
                CMILMatrix glyphRunTransform;
                GetLocalTransform(
                    m_glyphRuns[i].BaselineOffset.x,
                    m_glyphRuns[i].BaselineOffset.y,
                    m_glyphRuns[i].InvertHorizontalAxis,
                    m_glyphRuns[i].LineBounds.Width,
                    &glyphRunTransform);
                glyphRunTransform.Append(*pRenderTransform);

                //
                // Get texture bounds for the glyph run.
                //

                pSharedD2DDeviceContextNoRef->SetTransform(&GetD2DMatrix(glyphRunTransform));
                IFC_RETURN(pSharedD2DDeviceContextNoRef->GetGlyphRunWorldBounds(
                    D2D1::Point2F(),
                    &glyphRun,
                    DWRITE_MEASURING_MODE_NATURAL,
                    &d2dBrushBounds));
                pSharedD2DDeviceContextNoRef->SetTransform(&D2D1::IdentityMatrix());

                if (glyphRunTransform.Invert())
                {
                    brushBounds = GetRectF_RB(d2dBrushBounds);
                    glyphRunTransform.TransformBounds(&brushBounds, &brushBounds);
                }
            }

            IFC_RETURN(pBrushNoRef->D2DEnsureDeviceIndependentResources(
                cp,
                pRenderTransform,
                &brushBounds,
                m_glyphRuns[i].PALBrushParams
                ));
        }
    }

    for (XUINT32 i = 0, collectionSize = m_lines.GetCount(); i < collectionSize; i++)
    {
        CBrush *pBrushNoRef = GetForegroundBrush(m_lines[i].BrushSource, m_lines[i].Brush, m_controlEnabled);
        if (pBrushNoRef != NULL)
        {
            XRECTF_RB rect = { 0, 0, m_lines[i].Width, m_lines[i].Thickness};
            m_lines[i].Transform.TransformBounds(&rect, &rect);

            if (m_lines[i].PALBrushParams == NULL)
            {
                m_lines[i].PALBrushParams = new AcceleratedBrushParams();
            }

            IFC_RETURN(pBrushNoRef->D2DEnsureDeviceIndependentResources(
                cp,
                pRenderTransform,
                &rect,
                m_lines[i].PALBrushParams
                ));
        }
    }


    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Render the content to D2D drawing surface.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
D2DTextDrawingContext::D2DRender(
    _In_ const SharedRenderParams& sharedRP,
    _In_ const D2DRenderParams& d2dRP,
    _In_ XFLOAT opacity
    )
{
    IPALAcceleratedRenderTarget *pD2DRenderTargetNoRef = d2dRP.GetD2DRenderTarget();
    AcceleratedBrushParams *pPALBrushParams = NULL;
    CMILMatrix renderTransform = *sharedRP.pCurrentTransform;

    // Draw glyph runs
    wrl::ComPtr<IPALAcceleratedBrush> pAcceleratedBrush;
    for (XUINT32 i = 0, collectionSize = m_glyphRuns.GetCount(); i < collectionSize; i++)
    {
        CBrush *pBrushNoRef = GetForegroundBrush(m_glyphRuns[i].BrushSource, m_glyphRuns[i].Brush, m_controlEnabled);
        if (pBrushNoRef != NULL)
        {
            CMILMatrix glyphRunTransform;
            GetLocalTransform(
                m_glyphRuns[i].BaselineOffset.x,
                m_glyphRuns[i].BaselineOffset.y,
                m_glyphRuns[i].InvertHorizontalAxis,
                m_glyphRuns[i].LineBounds.Width,
                &glyphRunTransform);
            glyphRunTransform.Append(renderTransform);

            if (d2dRP.GetIsPrintTarget())
            {
                // Printing uses a different D2D device than rendering. We cannot use the
                // cached D2D brush as it is bound to the rendering device.
                IFC_RETURN(pBrushNoRef->GetPrintBrush(d2dRP, &pAcceleratedBrush));
            }
            else
            {
                IFC_RETURN(pBrushNoRef->UpdateAcceleratedBrush(d2dRP));
                pAcceleratedBrush = pBrushNoRef->GetAcceleratedBrush();
            }

            if (pAcceleratedBrush != NULL)
            {
                bool pushedBrushClipLayer = false;
                bool pushedAxisAlignedBrushClip = false;

                pPALBrushParams = m_glyphRuns[i].PALBrushParams;
                IFC_RETURN(pAcceleratedBrush->SetTransform(&pPALBrushParams->m_transform));

                IFC_RETURN(pPALBrushParams->PushBrushClip(
                    !d2dRP.GetIsPrintTarget() /* allowPushAxisAlignedClip */,
                    sharedRP.pCurrentTransform,
                    pD2DRenderTargetNoRef,
                    NULL, /* pContentBounds */
                    &pushedBrushClipLayer,
                    &pushedAxisAlignedBrushClip
                    ));

                IFC_RETURN(pD2DRenderTargetNoRef->SetTransform(&glyphRunTransform));

                //
                // If the glyph run needs to be clipped, push a clip for it.
                //
                bool pushedGlyphRunClip = false;
                if ((m_glyphRuns[i].GlyphRunClipLeft != XFLOAT_MIN) || (m_glyphRuns[i].GlyphRunClipRight != XFLOAT_MAX))
                {
                    XRECTF_RB clipRectInGlyphRunSpace = {
                        MAX(m_glyphRuns[i].GlyphRunClipLeft, -PRINTRT_FLOATMAX),
                        -PRINTRT_FLOATMAX,
                        MIN(m_glyphRuns[i].GlyphRunClipRight, PRINTRT_FLOATMAX),
                        PRINTRT_FLOATMAX,
                    };

                    pD2DRenderTargetNoRef->PushAxisAlignedClip(&clipRectInGlyphRunSpace);

                    pushedGlyphRunClip = TRUE;
                }

                IFC_RETURN(pD2DRenderTargetNoRef->DrawGlyphRun(m_glyphRuns[i].GlyphRun, pAcceleratedBrush.Get(), opacity, m_colorFontPaletteIndex, m_glyphRuns[i].GlyphImageFormat));

                if (pushedGlyphRunClip)
                {
                    pD2DRenderTargetNoRef->PopAxisAlignedClip();
                }

                IFC_RETURN(CUIElement::D2DPopClipHelper(
                    pD2DRenderTargetNoRef,
                    pushedBrushClipLayer,
                    pushedAxisAlignedBrushClip
                    ));

                pAcceleratedBrush.Reset();
            }
        }
    }

    // Draw lines
    IFC_RETURN(pD2DRenderTargetNoRef->SetTransform(&renderTransform));
    for (XUINT32 i = 0, collectionSize = m_lines.GetCount(); i < collectionSize; i++)
    {
        CBrush *pBrushNoRef = GetForegroundBrush(m_lines[i].BrushSource, m_lines[i].Brush, m_controlEnabled);
        if (pBrushNoRef != NULL)
        {
            if (d2dRP.GetIsPrintTarget())
            {
                // Printing uses a different D2D device than rendering. We cannot use the
                // cached D2D brush as it is bound to the rendering device.
                IFC_RETURN(pBrushNoRef->GetPrintBrush(d2dRP, &pAcceleratedBrush));
            }
            else
            {
                IFC_RETURN(pBrushNoRef->UpdateAcceleratedBrush(d2dRP));
                pAcceleratedBrush = pBrushNoRef->GetAcceleratedBrush();
            }

            if (pAcceleratedBrush != NULL)
            {
                bool pushedBrushClipLayer = false;
                bool pushedAxisAlignedBrushClip = false;

                XRECTF_RB rect = { 0, 0, m_lines[i].Width, m_lines[i].Thickness};
                m_lines[i].Transform.TransformBounds(&rect, &rect);

                pPALBrushParams = m_lines[i].PALBrushParams;
                IFC_RETURN(pAcceleratedBrush->SetTransform(&pPALBrushParams->m_transform));

                IFC_RETURN(pPALBrushParams->PushBrushClip(
                    !d2dRP.GetIsPrintTarget() /* allowPushAxisAlignedClip */,
                    sharedRP.pCurrentTransform,
                    pD2DRenderTargetNoRef,
                    NULL, /* pContentBounds */
                    &pushedBrushClipLayer,
                    &pushedAxisAlignedBrushClip
                    ));

                IFC_RETURN(pD2DRenderTargetNoRef->FillRectangle(rect, pAcceleratedBrush.Get(), opacity));

                IFC_RETURN(CUIElement::D2DPopClipHelper(
                    pD2DRenderTargetNoRef,
                    pushedBrushClipLayer,
                    pushedAxisAlignedBrushClip
                    ));

                pAcceleratedBrush.Reset();
            }
        }
    }

    // Restore the parent element's transform in case the parent has post-children content to render.
    IFC_RETURN(pD2DRenderTargetNoRef->SetTransform(sharedRP.pCurrentTransform));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Builds glyph run textures using D2D.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
D2DTextDrawingContext::HWBuildGlyphRunTextures(
    _In_ const HWRenderParams *pHWRenderParams,
    _In_ const CMILMatrix *pRealizationTransform
    )
{
    ASSERT(m_pGlyphRunRealizations != nullptr);

    ID2D1DeviceContext *pSharedD2DDeviceContextNoRef = nullptr;

    {
        CD3D11Device *pDeviceNoRef = pHWRenderParams->pRenderTarget->GetGraphicsDeviceManager()->GetGraphicsDevice();
        IFC_RETURN(pDeviceNoRef->EnsureD2DResources());
        CD3D11SharedDeviceGuard guard;
        IFC_RETURN(pDeviceNoRef->TakeLockAndCheckDeviceLost(&guard));
        pSharedD2DDeviceContextNoRef = pDeviceNoRef->GetD2DDeviceContext(&guard);

        // Apply DPI settings to ID2D1DeviceContext in order to benefit from D2D rendering optimizations.
        m_pTextCore->GetWinTextCore()->ApplyLogicalDpiSettings(pSharedD2DDeviceContextNoRef);
    }

    //
    // Batch glyph runs together, where possible.
    // This optimization generates fewer DComp primitives and fewer separate DComp surface allocations, which reduces
    // CPU overhead from atlasing costs in the XAML process, and rendering costs in the DWM process.
    // Combining glyph runs does potentially use atlas space less efficiently, and can cause additional overdraw, so
    // this trades off CPU improvements for GPU regressions.
    //
    XUINT32 batchGlyphRunStartIndex = 0;
    XUINT32 batchGlyphRunCount = 0;
    XRECTF batchGlyphRunsBounds;
    EmptyRectF(&batchGlyphRunsBounds);

    CBrush *pBatchBrushNoRef = nullptr;
    DWRITE_GLYPH_IMAGE_FORMATS batchGlyphImageFormat = DWRITE_GLYPH_IMAGE_FORMATS_NONE;

    for (XUINT32 i = 0, collectionSize = m_glyphRuns.GetCount(); i < collectionSize; i++)
    {
        GlyphRunData *pGlyphRunDataNoRef = &m_glyphRuns[i];
        CBrush *pBrushNoRef = GetForegroundBrush(pGlyphRunDataNoRef->BrushSource, pGlyphRunDataNoRef->Brush, m_controlEnabled);

        if (pBrushNoRef != nullptr)
        {
            XRECTF_RB glyphRunBounds;
            CMILMatrix glyphRunRealizationTransform;

            GetGlyphRunTransformAndBounds(
                pSharedD2DDeviceContextNoRef,
                pGlyphRunDataNoRef,
                pRealizationTransform,
                true, // snapToPixel
                glyphRunRealizationTransform,
                glyphRunBounds
                );

            pGlyphRunDataNoRef->GlyphRunRealizationBounds = ToXRectF(glyphRunBounds);
            pGlyphRunDataNoRef->GlyphRunRealizationTransform = glyphRunRealizationTransform;
        }
        else
        {
            EmptyRectF(&pGlyphRunDataNoRef->GlyphRunRealizationBounds);
            pGlyphRunDataNoRef->GlyphRunRealizationTransform.SetToIdentity();
        }

        //
        // Determine if this glyph run can be added to the current batch or not.
        // If it cannot, draw the current batch to this point, and start a fresh batch with the current glyph run.
        //
        // Glyph runs cannot be batched if...
        // - the brush differs from the previous batch, since each primitive can only specify one texture brush.
        // - the brush is not a SolidColorBrush (for compat, since the texture transform is calculated per-glyph-run)
        // - the glyph formats differ.
        if (   pBatchBrushNoRef != pBrushNoRef
            || (pBatchBrushNoRef != nullptr && !pBatchBrushNoRef->OfTypeByIndex<KnownTypeIndex::SolidColorBrush>())
            || batchGlyphImageFormat != pGlyphRunDataNoRef->GlyphImageFormat)
        {
            IFC_RETURN(HWBuildGlyphRunTexture(
                pHWRenderParams,
                pRealizationTransform,
                batchGlyphRunsBounds,
                batchGlyphRunStartIndex,
                batchGlyphRunCount
                ));

            // Initialize state for the new batch that starts with this run.
            batchGlyphRunStartIndex = i;
            batchGlyphRunCount = 0;
            EmptyRectF(&batchGlyphRunsBounds);
            pBatchBrushNoRef = pBrushNoRef;
            batchGlyphImageFormat = pGlyphRunDataNoRef->GlyphImageFormat;
        }

        // Add the current glyph run to the batch.
        ++batchGlyphRunCount;
        UnionRectF(&batchGlyphRunsBounds, &pGlyphRunDataNoRef->GlyphRunRealizationBounds);
    }

    // Ensure the final batch is processed, if needed.
    IFC_RETURN(HWBuildGlyphRunTexture(
        pHWRenderParams,
        pRealizationTransform,
        batchGlyphRunsBounds,
        batchGlyphRunStartIndex,
        batchGlyphRunCount
        ));

    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Synopsis:
//    Builds a single glyph run texture, using D2D, for a series of runs.
//    If the runs are too large to fit within a single texture, the texture
//    will be broken into tiles.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
D2DTextDrawingContext::HWBuildGlyphRunTexture(
    _In_ const HWRenderParams *pHWRenderParams,
    _In_ const CMILMatrix *pRealizationTransform,
    _In_ const XRECTF &glyphRunBatchBounds,
    XUINT32 glyphRunStartIndex,
    XUINT32 glyphRunCount
    )
{
    return HWBuildGlyphRunSingleTexture(
        pHWRenderParams,
        pRealizationTransform,
        glyphRunBatchBounds,
        glyphRunStartIndex,
        glyphRunCount);
}

_Check_return_ HRESULT
D2DTextDrawingContext::HWBuildGlyphRunSingleTexture(
    _In_ const HWRenderParams* pHWRenderParams,
    _In_ const CMILMatrix* pRealizationTransform,
    _In_ const XRECTF& glyphRunBatchBounds,
    uint32_t glyphRunStartIndex,
    uint32_t glyphRunCount
    )
{
    const uint32_t desiredTextureWidth = XcpCeiling(glyphRunBatchBounds.Width);
    const uint32_t desiredTextureHeight = XcpCeiling(glyphRunBatchBounds.Height);

    if ((glyphRunCount > 0) &&
        (desiredTextureWidth > 0) &&
        (desiredTextureHeight > 0))
    {
        // Batches are broken up if brushes change. As a result, for a given batch, we can treat the brush data
        // from the first glyph run as the data for the entire batch.
        GlyphRunData* batchGlyphRunDataNoRef = &m_glyphRuns[glyphRunStartIndex];
        CBrush* brushNoRef = GetForegroundBrush(batchGlyphRunDataNoRef->BrushSource, batchGlyphRunDataNoRef->Brush, m_controlEnabled);
        bool isColorBitmap = !!(batchGlyphRunDataNoRef->GlyphImageFormat & c_bgraColorGlyphImageFormats);

        if (brushNoRef != nullptr)
        {
            WindowsGraphicsDeviceManager* graphicsDeviceManagerNoRef = pHWRenderParams->pRenderTarget->GetGraphicsDeviceManager();
            const bool hasNative8BitSurfaceSupport = graphicsDeviceManagerNoRef->GetDCompTreeHost()->HasNative8BitSurfaceSupport();

            CD3D11Device* deviceNoRef = nullptr;
            ID2D1SolidColorBrush* sharedD2DSolidColorBrushNoRef = nullptr;

            deviceNoRef = graphicsDeviceManagerNoRef->GetGraphicsDevice();
            CD3D11SharedDeviceGuard guard;
            IFC_RETURN(deviceNoRef->TakeLockAndCheckDeviceLost(&guard));
            sharedD2DSolidColorBrushNoRef = deviceNoRef->GetD2DSolidColorBrush(&guard);

            InitializeD2DBrush(brushNoRef, sharedD2DSolidColorBrushNoRef);

            const bool hasExplicitBrush = HasExplicitForegroundBrush(
                batchGlyphRunDataNoRef->BrushSource,
                batchGlyphRunDataNoRef->Brush
                );

            // In nearly all common cases, the batch of the glyph runs will be rendered into a single tile.
            // If the batch does break into multiple tiles, all the glyph runs in the batch will be rendered once per
            // tile - we currently leave it up to D2D to clip out the runs that are outside the bounds of a single tile.
            uint32_t flags = HWTextureFlags_None;
            PixelFormat pixelFormat;

            pixelFormat = isColorBitmap ? pixelColor32bpp_A8R8G8B8 : pixelGray8bpp;

            const uint32_t maxTextureSize = m_pTextCore->GetCoreServices()->GetMaxTextureSize();

            if ((desiredTextureWidth > maxTextureSize) ||
                (desiredTextureHeight > maxTextureSize))
            {
                flags |= HWTextureFlags_IsVirtual;
            }

            xref_ptr<HWTexture> texture;
            IFC_RETURN(pHWRenderParams->pHWWalk->GetTextureManager()->CreateTexture(
                pixelFormat,
                desiredTextureWidth,
                desiredTextureHeight,
                static_cast<HWTextureFlags>(flags),
                texture.ReleaseAndGetAddressOf()
                ));

            // Iterate through the surface as tiles.  In the case of a virtual surface, it will be locked or drawn as a tiled updateRect and
            // DComp will piece together the tiles at rendering time.
            for (uint32_t tileOffsetY = 0; tileOffsetY < desiredTextureHeight; tileOffsetY += maxTextureSize)
            {
                const uint32_t tileHeight = MIN(maxTextureSize, desiredTextureHeight - tileOffsetY);

                for (uint32_t tileOffsetX = 0; tileOffsetX < desiredTextureWidth; tileOffsetX += maxTextureSize)
                {
                    const uint32_t tileWidth = MIN(maxTextureSize, desiredTextureWidth - tileOffsetX);

                    XRECT updateRect = { tileOffsetX, tileOffsetY, tileWidth, tileHeight };

                    // Create HWTexture large enough to represent entire glyph run
                    // and clear it before writing glyph run coverage data.
                    {
                        Microsoft::WRL::ComPtr<ID2D1DeviceContext> textureAtlasDeviceContext;
                        Microsoft::WRL::ComPtr<ID2D1Image> originalTarget;
                        XPOINT textureAtlasOffset;
                        DCompSurface* compositionSurfaceNoRef = texture->GetCompositionSurface();

                        {
                            // Begin drawing with DComp, getting the D2D device context associated with the
                            // texture's specific surface.
                            // See comments above about using BeginDraw vs BeginDrawWithGutters on virtual vs non-virtual surfaces
                            // respectively.
                            if (texture->IsVirtual())
                            {
                                IFC_RETURN(compositionSurfaceNoRef->BeginDraw(
                                    &updateRect,
                                    __uuidof(ID2D1DeviceContext),
                                    reinterpret_cast<IUnknown**>(textureAtlasDeviceContext.ReleaseAndGetAddressOf()),
                                    &textureAtlasOffset
                                    ));
                            }
                            else
                            {
                                IFC_RETURN(compositionSurfaceNoRef->BeginDrawWithGutters(
                                    &updateRect,
                                    __uuidof(ID2D1DeviceContext),
                                    reinterpret_cast<IUnknown**>(textureAtlasDeviceContext.ReleaseAndGetAddressOf()),
                                    &textureAtlasOffset
                                    ));
                            }
                        }

                        // Apply DPI settings to ID2D1DeviceContext in order to benefit from D2D rendering optimizations.
                        m_pTextCore->GetWinTextCore()->ApplyLogicalDpiSettings(textureAtlasDeviceContext.Get());

                        textureAtlasDeviceContext->Clear(D2D1::ColorF(D2D1::ColorF::Black, 0));

                        // Draw all the glyph runs in this batch.
                        for (XUINT32 i = 0; i < glyphRunCount; i++)
                        {
                            GlyphRunData *pCurrentGlyphRunDataNoRef = &m_glyphRuns[i + glyphRunStartIndex];

                            IFC_RETURN(RasterizeGlyphRunToD2DOrRawBuffer(
                                pCurrentGlyphRunDataNoRef,
                                pCurrentGlyphRunDataNoRef->GlyphRunRealizationTransform,
                                pCurrentGlyphRunDataNoRef->GlyphRunRealizationBounds,
                                glyphRunBatchBounds,
                                textureAtlasOffset.x - glyphRunBatchBounds.X - tileOffsetX,
                                textureAtlasOffset.y - glyphRunBatchBounds.Y - tileOffsetY,
                                textureAtlasDeviceContext.Get(),
                                sharedD2DSolidColorBrushNoRef,
                                hasNative8BitSurfaceSupport,
                                nullptr,
                                0,
                                0,
                                0,
                                false
                            ));
                        }

                        IFC_RETURN(texture->GetCompositionSurface()->EndDraw());
                    }
                }
            }

            xref_ptr<HWTextRealization> hwTextRealization = make_xref<HWTextRealization>();
            hwTextRealization->SetForegroundBrush(brushNoRef);
            hwTextRealization->UpdateRealizationParameters(
                pRealizationTransform,
                pHWRenderParams->isTransformAnimating,
                glyphRunBatchBounds.X,
                glyphRunBatchBounds.Y,
                false /* renderCollapsedMask */);
            hwTextRealization->SetTextHwTexture(texture);
            hwTextRealization->SetIsColorBitmap(isColorBitmap);

            // Calculate brush bounding rectangle
            // NOTE: pGlyphRunData->LineBounds could be empty if rendering content produced by RichEdit-based
            //       text controls. In this case set the bounds to the texture size.  This is good enough,
            //       since text editing controls support only solid color brushes.
            //
            // For controls that do support other brush types, its okay to use the first glyph run's LineBounds
            // because it'll be the only one in the batch - we intentional prevent non-solid-color-brushes from
            // batching for this reason.
            XRECTF brushBounds = batchGlyphRunDataNoRef->LineBounds;
            pRealizationTransform->TransformBounds(&brushBounds, &brushBounds);
            if (IsEmptyRectF(batchGlyphRunDataNoRef->LineBounds))
            {
                brushBounds.Width = static_cast<XFLOAT>(desiredTextureWidth);
                brushBounds.Height = static_cast<XFLOAT>(desiredTextureHeight);
            }
            else
            {
                brushBounds.X -= batchGlyphRunDataNoRef->GlyphRunRealizationBounds.X;
                brushBounds.Y -= batchGlyphRunDataNoRef->GlyphRunRealizationBounds.Y;
            }

            // Add glyph run text realization and its associated data.
            const uint32_t currentRealizationIndex = m_pGlyphRunRealizations->GetCount();
            IFC_RETURN(m_pGlyphRunRealizations->SetCount(currentRealizationIndex + 1));
            (*m_pGlyphRunRealizations)[currentRealizationIndex].TextRealization = hwTextRealization.detach();
            (*m_pGlyphRunRealizations)[currentRealizationIndex].BrushBounds = brushBounds;
            (*m_pGlyphRunRealizations)[currentRealizationIndex].HasExplicitBrush = hasExplicitBrush;
        }
    }

    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Synopsis:
//      Measures a glyph run either using the MobileCore PAL or with D2D.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
D2DTextDrawingContext::GetGlyphRunTransformAndBounds(
    _In_ ID2D1DeviceContext *pSharedD2DDeviceContextNoRef,
    _In_ const GlyphRunData *pGlyphRunDataNoRef,
    _In_ const CMILMatrix *pRealizationTransform,
    bool snapToPixel,
    _Out_ CMILMatrix &glyphRunRealizationTransform,
    _Out_ XRECTF_RB &glyphRunBounds
)
{
    DWRITE_GLYPH_RUN glyphRun = GetDWriteGlyphRun(*pGlyphRunDataNoRef->GlyphRun);

    //
    // Calculate the glyph run realization transform, which contains:
    // - baseline offset of the glyph run
    // - [optional] mirroring transform for RTL glyph runs
    // - render transform applied to the drawing context
    //
    GetLocalTransform(
        pGlyphRunDataNoRef->BaselineOffset.x,
        pGlyphRunDataNoRef->BaselineOffset.y,
        pGlyphRunDataNoRef->InvertHorizontalAxis,
        pGlyphRunDataNoRef->LineBounds.Width,
        &glyphRunRealizationTransform);
    glyphRunRealizationTransform.Append(*pRealizationTransform);

    //
    // If baseline snapping is enabled, adjust the GlyphRun realization transform to include the baseline shift delta.
    //
    if (snapToPixel)
    {
        glyphRunRealizationTransform.SetDy(static_cast<XFLOAT>(XcpRound(glyphRunRealizationTransform.GetDy())));
    }

    //
    // Get texture bounds for the glyph run.
    //
    {
        //
        // Get texture bounds for the glyph run. Those bounds are calculated by applying
        // glyph run's realization transform first and then calling D2D to get tight
        // rectangular bounds of the glyph run.
        // NOTE: We are using "fake" ID2D1DeviceContext which is based on 1x1 bitmap.
        //       This is enough to get actual bounds of the glyph run. Drawing to this ID2D1DeviceContext is not supported.
        //
        D2D1_RECT_F d2dBrushBounds;
        pSharedD2DDeviceContextNoRef->SetTransform(&GetD2DMatrix(glyphRunRealizationTransform));

        if (pGlyphRunDataNoRef->GlyphImageFormat & c_bgraColorGlyphImageFormats)
        {
            // For bitmap and SVG glyph formats, draw into a command list to get the tight bounds.
            Microsoft::WRL::ComPtr<ID2D1CommandList> commandList;
            Microsoft::WRL::ComPtr<ID2D1Image> originalTarget;
            Microsoft::WRL::ComPtr<ID2D1DeviceContext4> deviceContext4;

            IFC_RETURN(pSharedD2DDeviceContextNoRef->CreateCommandList(&commandList));
            IFC_RETURN(pSharedD2DDeviceContextNoRef->QueryInterface(IID_PPV_ARGS(&deviceContext4)));

            pSharedD2DDeviceContextNoRef->GetTarget(&originalTarget);
            pSharedD2DDeviceContextNoRef->SetTarget(commandList.Get());
            pSharedD2DDeviceContextNoRef->BeginDraw();

            RasterizeImageFormatGlyphRun(
                deviceContext4.Get(),
                glyphRun,
                pGlyphRunDataNoRef->GlyphImageFormat,
                m_colorFontPaletteIndex,
                nullptr // default foreground ID2D1Brush may be nullptr for bitmaps and SVG.
                );

            // Restore the old target, and finish the command list to get the bounds, using 'local' and not
            // 'world' which would doubly compound the transform.
            pSharedD2DDeviceContextNoRef->SetTarget(originalTarget.Get());
            IFC_RETURN(pSharedD2DDeviceContextNoRef->EndDraw());
            IFC_RETURN(commandList->Close());
            IFC_RETURN(pSharedD2DDeviceContextNoRef->GetImageLocalBounds(commandList.Get(), /*out*/ &d2dBrushBounds));
        }
        else
        {
            // Use the world bounds of the monochromatic geometry.
            IFC_RETURN(pSharedD2DDeviceContextNoRef->GetGlyphRunWorldBounds(
                D2D1::Point2F(),
                &glyphRun,
                DWRITE_MEASURING_MODE_NATURAL,
                &d2dBrushBounds
                ));
        }

        glyphRunBounds = GetRectF_RB(d2dBrushBounds);
    }

    //
    // Clip the run if required.
    //
    if ((pGlyphRunDataNoRef->GlyphRunClipLeft != XFLOAT_MIN) || (pGlyphRunDataNoRef->GlyphRunClipRight != XFLOAT_MAX))
    {
        XRECTF_RB const clipRectInGlyphRunSpace = {
            pGlyphRunDataNoRef->GlyphRunClipLeft,
            0,
            pGlyphRunDataNoRef->GlyphRunClipRight,
            0};

        XRECTF_RB clipRectInBitmapSpace;

        glyphRunRealizationTransform.TransformBounds(&clipRectInGlyphRunSpace, &clipRectInBitmapSpace);

        glyphRunBounds.left  = MAX(clipRectInBitmapSpace.left, glyphRunBounds.left);
        glyphRunBounds.right = MAX(glyphRunBounds.left, MIN(glyphRunBounds.right, clipRectInBitmapSpace.right));
    }

    InflateRectF(&glyphRunBounds);

    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Synopsis:
//      Rasterizes a glyph run either using the MobileCore PAL or with D2D.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
D2DTextDrawingContext::RasterizeGlyphRunToD2DOrRawBuffer(
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
    )
{
    DWRITE_GLYPH_RUN glyphRun = GetDWriteGlyphRun(*pCurrentGlyphRunDataNoRef->GlyphRun);

    {
        CMILMatrix textureTransform(TRUE);
        textureTransform.SetDx((float)tileOffsetX);
        textureTransform.SetDy((float)tileOffsetY);
        textureTransform.Prepend(glyphRunRealizationTransform);

        //
        // Draw glyph run into the drawing context to create its alpha coverage bitmap.
        //
        pTextureAtlasDeviceContext->SetTransform(&GetD2DMatrix(textureTransform));

        bool pushedGlyphRunClip = false;
        if (   pCurrentGlyphRunDataNoRef->GlyphRunClipLeft != XFLOAT_MIN
            || pCurrentGlyphRunDataNoRef->GlyphRunClipRight != XFLOAT_MAX)
        {
            // Clips only apply horizontally. The vertical limits are copied from
            // D2DTextDrawingContext::D2DRender for consistency.
            XRECTF_RB clipRectInGlyphRunSpace = {
                MAX(pCurrentGlyphRunDataNoRef->GlyphRunClipLeft, -PRINTRT_FLOATMAX),
                -PRINTRT_FLOATMAX,
                MIN(pCurrentGlyphRunDataNoRef->GlyphRunClipRight, PRINTRT_FLOATMAX),
                PRINTRT_FLOATMAX,
            };

            pTextureAtlasDeviceContext->PushAxisAlignedClip(
                PALToD2DRectF(&clipRectInGlyphRunSpace),
                D2D1_ANTIALIAS_MODE_ALIASED
                );

            pushedGlyphRunClip = TRUE;
        }

        Microsoft::WRL::ComPtr<ID2D1DeviceContext4> textureAtlasDeviceContext4;
        IFC_RETURN(pTextureAtlasDeviceContext->QueryInterface(IID_PPV_ARGS(&textureAtlasDeviceContext4)));

        RasterizeImageFormatGlyphRun(
            textureAtlasDeviceContext4.Get(),
            glyphRun,
            pCurrentGlyphRunDataNoRef->GlyphImageFormat,
            m_colorFontPaletteIndex,
            pSharedD2DSolidColorBrushNoRef
            );

        if (pushedGlyphRunClip)
        {
            pTextureAtlasDeviceContext->PopAxisAlignedClip();
        }
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Renders a single potentially multicolor glyph run using D2D. Note COLR
//  has already been translated to monochromatic glyph outlines.
//
//---------------------------------------------------------------------------
void D2DTextDrawingContext::RasterizeImageFormatGlyphRun(
    _In_ ID2D1DeviceContext4 *pDeviceContext,
    _In_ DWRITE_GLYPH_RUN const &glyphRun,
    _In_ DWRITE_GLYPH_IMAGE_FORMATS glyphImageFormat,
    _In_ UINT32 fontPaletteIndex,
    _In_opt_ ID2D1Brush *pBrushNoRef // Mandatory for TrueType/CFF outlines. Optional for bitmap/SVG.
    )
{
    if (glyphImageFormat == DWRITE_GLYPH_IMAGE_FORMATS_SVG)
    {
        pDeviceContext->DrawSvgGlyphRun(
            D2D1::Point2F(),
            &glyphRun,
            pBrushNoRef, // default foreground ID2D1Brush (optional for DrawSvgGlyphRun)
            nullptr, // default ID2D1SvgGlyphStyle
            fontPaletteIndex,
            DWRITE_MEASURING_MODE_NATURAL
            );
    }
    else if (glyphImageFormat & c_anyBitmapGlyphImageFormats)
    {
        pDeviceContext->DrawColorBitmapGlyphRun(
            glyphImageFormat,
            D2D1::Point2F(),
            &glyphRun,
            DWRITE_MEASURING_MODE_NATURAL,
            D2D1_COLOR_BITMAP_GLYPH_SNAP_OPTION_DEFAULT
            );
    }
    else
    {
        // Draw monochromatic outlines for anything else.
        pDeviceContext->DrawGlyphRun(
            D2D1::Point2F(),
            &glyphRun,
            pBrushNoRef,
            DWRITE_MEASURING_MODE_NATURAL
            );
    }
}

//---------------------------------------------------------------------------
//
//  Generates rendering data for a glyph run.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
D2DTextDrawingContext::DrawGlyphRun(
    _In_ PALText::GlyphRun *pGlyphRun,
    _In_ const XPOINTF *pGlyphRunOffset,
    _In_ const xref::weakref_ptr<CDependencyObject>& pBrushSource
    )
{
    XRECTF emptyBounds = { };
    xref_ptr<CSolidColorBrush> alternativeForegroundBrush;
    alternativeForegroundBrush.attach(GetAlternativeForegroundBrush(pBrushSource));

    IFC_RETURN(AddGlyphRun(
            pGlyphRun,
            *pGlyphRunOffset,
            emptyBounds,
            FALSE,
            pBrushSource,
            alternativeForegroundBrush,
            NULL,
            c_allSupportedGlyphImageFormats
            ));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Generates rendering data for a glyph run.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
D2DTextDrawingContext::DrawGlyphRun(
    _In_ PALText::GlyphRun *pGlyphRun,
    _In_ const XPOINTF *pGlyphRunOffset,
    _In_ XUINT32 foregroundColor
    )
{
    XRECTF emptyBounds = { };

    //
    // Create SolidColorBrush representing the foreground color.
    //
    CValue value;
    CREATEPARAMETERS param(m_pTextCore->GetCoreServices(), value);
    xref_ptr<CSolidColorBrush> pSolidColorBrush;
    IFC_RETURN(CSolidColorBrush::Create(reinterpret_cast<CDependencyObject **>(pSolidColorBrush.ReleaseAndGetAddressOf()), &param));
    pSolidColorBrush->m_rgb = foregroundColor;

    IFC_RETURN(AddGlyphRun(
            pGlyphRun,
            *pGlyphRunOffset,
            emptyBounds,
            FALSE,
            xref::weakref_ptr<CDependencyObject>(),
            pSolidColorBrush,
            NULL,
            c_allSupportedGlyphImageFormats
            ));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Generates rendering data for a rectangle.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
D2DTextDrawingContext::DrawRectangle(
    _In_ const XRECTF_RB *pRect,
    _In_ XUINT32 fillColor
    )
{
    XUINT32 currentLineIndex;
    CMILMatrix lineTransform(TRUE);

    lineTransform.SetDx(pRect->left);
    lineTransform.SetDy(pRect->top);

    //
    // Create SolidColorBrush representing the foreground color.
    //
    CValue value;
    CREATEPARAMETERS param(m_pTextCore->GetCoreServices(), value);
    xref_ptr<CSolidColorBrush> pSolidColorBrush;
    IFC_RETURN(CSolidColorBrush::Create(reinterpret_cast<CDependencyObject **>(pSolidColorBrush.ReleaseAndGetAddressOf()), &param));
    pSolidColorBrush->m_rgb = fillColor;

    // Add line data to the collection representing rendering data.
    currentLineIndex = m_lines.GetCount();
    IFC_RETURN(m_lines.SetCount(currentLineIndex + 1));
    m_lines[currentLineIndex].Width          = pRect->right - pRect->left;
    m_lines[currentLineIndex].Thickness      = pRect->bottom - pRect->top;
    m_lines[currentLineIndex].Transform      = lineTransform;
    m_lines[currentLineIndex].BrushSource.reset();
    m_lines[currentLineIndex].Brush          = pSolidColorBrush.detach();
    m_lines[currentLineIndex].PALBrushParams = NULL;

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Overload. Generates rendering data for a rectangle to be filled with an ImageBrush
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
D2DTextDrawingContext::DrawRectangle(
    _In_ const XRECTF_RB *pRect,
    _In_ CImageBrush* pImageBrush,
    _In_ DWORD color,
    _In_opt_ XUINT32* pPixels,
    _In_ XINT32 insertPosition)
{
    XUINT32 currentImageIndex;

    if (insertPosition == -1)
    {
        currentImageIndex = m_images.GetCount();
        IFC_RETURN(m_images.SetCount(currentImageIndex + 1));
    }
    else
    {
        currentImageIndex = insertPosition;
    }
    SetInterface(m_images[currentImageIndex].Brush, pImageBrush);
    m_images[currentImageIndex].ImageRect = *pRect;
    m_images[currentImageIndex].pPixelBuffer = pPixels;
    m_images[currentImageIndex].color = color;

    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Generates rendering data for a rectangle.
//
//---------------------------------------------------------------------------
void D2DTextDrawingContext::InvalidateRegion(
    _In_ const XRECTF_RB *pRegion
    )
{
    XUINT32 newSize;
    XPOINTF origin = {0};
    XPOINTF transformedOrigin;

    //
    // Remove glyph runs intersecting with the invalidated region.
    // During the first iteration release invalidated glyph runs.
    // During the second iteration move remaining glyph runs to empty spots in the collection
    // to ensure there are no gaps.
    //
    newSize = m_glyphRuns.GetCount();
    for (XUINT32 i = 0, collectionSize = m_glyphRuns.GetCount(); i < collectionSize; i++)
    {
        GlyphRunData * pGlyphRunData = &m_glyphRuns[i];
        bool fLTRGlyphRun = ((pGlyphRunData->GlyphRun->BidiLevel & 1) == 0);
        CMILMatrix glyphRunTransform;
        GetLocalTransform(
            pGlyphRunData->BaselineOffset.x,
            pGlyphRunData->BaselineOffset.y,
            pGlyphRunData->InvertHorizontalAxis,
            pGlyphRunData->LineBounds.Width,
            &glyphRunTransform);

        origin.x = (pGlyphRunData->GlyphRun->GlyphCount > 0) ? (pGlyphRunData->GlyphRun->GlyphAdvances[0])/2 : 0;
        origin.x = fLTRGlyphRun ? origin.x : -1*origin.x;
        glyphRunTransform.Transform(&origin, &transformedOrigin, 1);
        // Inclusive comparison to avoid corner case where the basLineOrigin's y is exactly at the bottom of line height
        if (DoesRectContainPointInclusive(*pRegion, transformedOrigin))
        {
            newSize--;
            ReleaseGlyphRun(pGlyphRunData->GlyphRun);
            pGlyphRunData->BrushSource.reset();
            ReleaseInterface(pGlyphRunData->Brush);
            pGlyphRunData->GlyphRun = NULL;
            SAFE_DELETE(pGlyphRunData->PALBrushParams);
        }
    }
    if (newSize > 0)
    {
        XUINT32 newIndex = 0;
        for (XUINT32 i = 0, collectionSize = m_glyphRuns.GetCount(); i < collectionSize; i++)
        {
            GlyphRunData * pGlyphRunData = &m_glyphRuns[i];
            if (pGlyphRunData->GlyphRun != NULL)
            {
                m_glyphRuns[newIndex] = *pGlyphRunData;
                newIndex++;
            }
        }
        ASSERT(newSize == newIndex);
        IGNOREHR(m_glyphRuns.SetCount(newSize));
    }
    else
    {
        m_glyphRuns.Clear();
    }

    //
    // Remove lines intersecting with the invalidated region.
    // During the first iteration release invalidated lines.
    // During the second iteration move remaining lines to empty spots in the collection
    // to ensure there are no gaps.
    //
    newSize = m_lines.GetCount();
    origin.x = origin.y = 0;
    for (XUINT32 i = 0, collectionSize = m_lines.GetCount(); i < collectionSize; i++)
    {
        LineData * pLineData = &m_lines[i];
        pLineData->Transform.Transform(&origin, &transformedOrigin, 1);
        if (DoesRectContainPoint(*pRegion, transformedOrigin))
        {
            newSize--;
            pLineData->BrushSource.reset();
            ReleaseInterface(pLineData->Brush);
            SAFE_DELETE(pLineData->PALBrushParams);
        }
    }
    if (newSize > 0)
    {
        XUINT32 newIndex = 0;
        for (XUINT32 i = 0, collectionSize = m_lines.GetCount(); i < collectionSize; i++)
        {
            LineData * pLineData = &m_lines[i];
            if (pLineData->BrushSource || pLineData->Brush != NULL)
            {
                m_lines[newIndex] = *pLineData;
                newIndex++;
            }
        }
        ASSERT(newSize == newIndex);
        IGNOREHR(m_lines.SetCount(newSize));
    }
    else
    {
        m_lines.Clear();
    }

    //
    // Remove images intersecting with the invalidated region.
    // During the first iteration release invalidated image brushes.
    // During the second iteration move remaining images to empty spots in the collection
    // to ensure there are no gaps.
    //
    newSize = m_images.GetCount();
    for (XUINT32 i = 0, collectionSize = m_images.GetCount(); i < collectionSize; i++)
    {
        ImageData * pImageData = &m_images[i];
        if (DoRectsIntersect(*pRegion, pImageData->ImageRect))
        {
            newSize--;
            ReleaseInterface(pImageData->Brush);
            SAFE_DELETE_ARRAY(pImageData->pPixelBuffer);
        }
    }
    if (newSize > 0)
    {
        XUINT32 newIndex = 0;
        for (XUINT32 i = 0, collectionSize = m_images.GetCount(); i < collectionSize; i++)
        {
            ImageData * pImageData = &m_images[i];
            if (pImageData->Brush != NULL)
            {
                m_images[newIndex] = *pImageData;
                newIndex++;
            }
        }
        ASSERT(newSize == newIndex);
        IGNOREHR(m_images.SetCount(newSize));
    }
    else
    {
        m_images.Clear();
    }

    ClearGlyphRunCaches();
    ReleaseInterface(m_pBaseRealization);
}

void D2DTextDrawingContext::SetFlipSelectionAlongHorizontalAxis(_In_ bool flipSelectionAlongHorizontalAxis)
{
    m_flipSelectionAlongHorizontalAxis = flipSelectionAlongHorizontalAxis;
}

void D2DTextDrawingContext::SetBackPlateConfiguration(bool backPlateActive, bool useHyperlinkForeground)
{
    m_backPlateActive = backPlateActive;
    m_useHyperlinkForeground = useHyperlinkForeground;
}

void D2DTextDrawingContext::SetControlEnabled(bool enabled)
{
    m_controlEnabled = enabled;
}

// Gets the correct foreground to ensure HighContrast when a BackPlate is being rendered.
CSolidColorBrush* D2DTextDrawingContext::GetAlternativeForegroundBrush(
    _In_ const xref::weakref_ptr<CDependencyObject>& pBrushSource)
{
    CSolidColorBrush* pBrush = nullptr;
    if (m_backPlateActive)
    {
        xref_ptr<CDependencyObject> pDO = pBrushSource.lock();

        if (pDO != nullptr)
        {
            auto pInlineCollection = static_cast<CInlineCollection*>(pDO->GetParent());

            if (pInlineCollection)
            {
                auto pInlineCollectionOwnerDO = pInlineCollection->GetOwnerDO();

                // If this is not a Hyperlink control override the foreground
                if (!pInlineCollectionOwnerDO || !pInlineCollectionOwnerDO->OfTypeByIndex<KnownTypeIndex::Hyperlink>())
                {
                    // We use a different color override for text in HyperlinkButton,
                    // this ensures high contrast but also keeps hyperlink in a different color than normal text.
                    if (m_useHyperlinkForeground)
                    {
                        pBrush = m_pTextCore->GetCoreServices()->GetSystemColorHotlightBrush();
                    }
                    else
                    {
                        pBrush = m_pTextCore->GetCoreServices()->GetSystemColorWindowTextBrush();
                    }
                }
            }
            else
            {
                if (m_useHyperlinkForeground)
                {
                    pBrush = m_pTextCore->GetCoreServices()->GetSystemColorHotlightBrush();
                }
                else
                {
                    pBrush = m_pTextCore->GetCoreServices()->GetSystemColorWindowTextBrush();
                }
            }
        }
    }

    return pBrush;
}

