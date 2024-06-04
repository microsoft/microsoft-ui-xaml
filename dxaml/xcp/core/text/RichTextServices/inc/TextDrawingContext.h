// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      TextDrawingContext describes text visual content using draw commands.
//      It is actually storing a set of rendering instructions that will later be used by
//      the graphics system.

#pragma once

#include "TextObject.h"
#include "Result.h"

namespace RichTextServices
{
    class TextRunProperties;

    //---------------------------------------------------------------------------
    //
    //  TextDrawingContext
    //
    //  Describes text visual content using draw commands. It is actually storing a
    //  set of rendering instructions that will later be used by the graphics system.
    //
    //---------------------------------------------------------------------------
    class TextDrawingContext : public TextObject
    {
    public:

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      DrawGlyphRun
        //
        //  Synopsis:
        //      Generates rendering data for run of glyphs.
        //
        //---------------------------------------------------------------------------
        virtual
        _Check_return_ HRESULT DrawGlyphRun(
            _In_ const XPOINTF &position,
                // The origin for the text output.
            _In_ XFLOAT runWidth,
                // Total width of the glyph run.
            _In_ FssGlyphRun *pGlyphRun,
                // Contains the information needed by renderers to draw glyph runs.
            _In_ const xref::weakref_ptr<CDependencyObject>& pBrushSource,
                // The source of foreground brush.
            _In_opt_ XRECTF_RB const* pClipRect
                // The clip rect to apply to the run.
            ) = 0;

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      DrawGlyphs
        //
        //  Synopsis:
        //      Generates rendering data for run of glyphs after pre-processing by the shaping engine.
        //
        //---------------------------------------------------------------------------
        virtual _Check_return_ HRESULT DrawGlyphs(
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
            _In_ IFssFontFace* pFontFace,
            _In_ XFLOAT fontSize,
            _In_ bool isSideways,
            _In_ XUINT8 bidiLevel,
            _In_ FssScriptAnalysis const* pScriptAnalysis,
            _In_ const xref::weakref_ptr<CDependencyObject>& pBrushSource,
            _In_opt_ XRECTF_RB const* pClipRect
            ) = 0;

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      DrawLine
        //
        //  Synopsis:
        //      Generates rendering data for a line.
        //
        //---------------------------------------------------------------------------
        virtual
        Result::Enum DrawLine(
            _In_ const XPOINTF &position,
                // The start position of the line.
            _In_ XFLOAT width,
                // The width of the line.
            _In_ XFLOAT thickness,
                // Thickness of the line.
            _In_ XUINT8 bidirectionalLevel,
                // The bidirectional nesting level, even for LTR runs, odd for RTL runs
            _In_ const xref::weakref_ptr<CDependencyObject>&pBrushSource,
                // The source of foreground brush.
            _In_opt_ XRECTF_RB const* pClipRect
                // The clip rect to apply to the run.
            ) = 0;

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      Clear
        //
        //  Synopsis:
        //      Clears all rendering data.
        //
        //---------------------------------------------------------------------------
        virtual void Clear() = 0;

        // Determines whether rendering data has been already populated.
        virtual bool HasRenderingData() const = 0;

        // Sets line parameters
        virtual void SetLineInfo(
            _In_ XFLOAT viewportWidth,
            _In_ bool invertHorizontalAxis,
            _In_ XFLOAT yOffset,
            _In_ XFLOAT verticalAdvance
            ) = 0;

        // Sets whether color fonts should be enabled for text drawing operations
        virtual void SetIsColorFontEnabled(bool isColorFontEnabled) = 0;

        // Sets the palette index to use for color glyphs if color fonts are enabled
        virtual void SetColorFontPaletteIndex(UINT32 colorFontPaletteIndex) = 0;
    };
}
