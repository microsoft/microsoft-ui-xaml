//+--------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Abstract:
//     The contents of this file were copied from dwrite_3.h and d2d1_3.h
//     from the Windows SDK (25915), since this project currently compiles with
//     an older SDK which does not include these newer defintions. This file
//     can be removed when this project eventually moves to a newer SDK which
//     has these definitions.
//
//----------------------------------------------------------------------------

// This type is part of an enum. Since we can't add to the enum, declare it as a #define.
#define DWRITE_GLYPH_IMAGE_FORMATS_COLR_PAINT_TREE (DWRITE_GLYPH_IMAGE_FORMATS)0x00000100

enum DWRITE_PAINT_FEATURE_LEVEL : INT32;

#ifndef DWRITE_BEGIN_INTERFACE
#define DWRITE_BEGIN_INTERFACE(name, iid) interface DWRITE_DECLARE_INTERFACE(iid) name
#endif

DWRITE_BEGIN_INTERFACE(IDWriteFactory8, "EE0A7FB5-DEF4-4C23-A454-C9C7DC878398") : IDWriteFactory7
{
    /// <summary>
    /// Translates a glyph run to a sequence of color glyph runs, which can be
    /// rendered to produce a color representation of the original "base" run.
    /// </summary>
    /// <param name="baselineOriginX">Horizontal and vertical origin of the base glyph run in
    /// pre-transform coordinates.</param>
    /// <param name="glyphRun">Pointer to the original "base" glyph run.</param>
    /// <param name="glyphRunDescription">Optional glyph run description.</param>
    /// <param name="desiredGlyphImageFormats">Which data formats TranslateColorGlyphRun
    /// should split the runs into.</param>
    /// <param name="paintFeatureLevel">Paint feature level supported by the caller. Used
    /// when desiredGlyphImageFormats includes DWRITE_GLYPH_IMAGE_FORMATS_COLR_PAINT_TREE. See
    /// DWRITE_PAINT_FEATURE_LEVEL for more information.</param>
    /// <param name="measuringMode">Measuring mode, needed to compute the origins
    /// of each glyph.</param>
    /// <param name="worldToDeviceTransform">Matrix converting from the client's
    /// coordinate space to device coordinates (pixels), i.e., the world transform
    /// multiplied by any DPI scaling.</param>
    /// <param name="colorPaletteIndex">Zero-based index of the color palette to use.
    /// Valid indices are less than the number of palettes in the font, as returned
    /// by IDWriteFontFace2::GetColorPaletteCount.</param>
    /// <param name="colorEnumerator">If the function succeeds, receives a pointer
    /// to an enumerator object that can be used to obtain the color glyph runs.
    /// If the base run has no color glyphs, then the output pointer is NULL
    /// and the method returns DWRITE_E_NOCOLOR.</param>
    /// <returns>
    /// Returns DWRITE_E_NOCOLOR if the font has no color information, the glyph run
    /// does not contain any color glyphs, or the specified color palette index
    /// is out of range. In this case, the client should render the original glyph 
    /// run. Otherwise, returns a standard HRESULT error code.
    /// </returns>
    /// <remarks>
    /// The old IDWriteFactory2::TranslateColorGlyphRun is equivalent to passing
    /// DWRITE_GLYPH_IMAGE_FORMATS_TRUETYPE|CFF|COLR.
    /// </remarks>
    STDMETHOD(TranslateColorGlyphRun)(
        D2D1_POINT_2F baselineOrigin,
        _In_ DWRITE_GLYPH_RUN const* glyphRun,
        _In_opt_ DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
        DWRITE_GLYPH_IMAGE_FORMATS desiredGlyphImageFormats,
        DWRITE_PAINT_FEATURE_LEVEL paintFeatureLevel,
        DWRITE_MEASURING_MODE measuringMode,
        _In_opt_ DWRITE_MATRIX const* worldAndDpiTransform,
        UINT32 colorPaletteIndex,
        _COM_Outptr_ IDWriteColorGlyphRunEnumerator1** colorEnumerator
        ) PURE;
};

interface DX_DECLARE_INTERFACE("ec891cf7-9b69-4851-9def-4e0915771e62") ID2D1DeviceContext7  : public ID2D1DeviceContext6
{
    
    /// <summary>
    /// Get the maximum paint feature level supported by DrawPaintGlyphRun.
    /// </summary>
    STDMETHOD_(DWRITE_PAINT_FEATURE_LEVEL, GetPaintFeatureLevel)(
        ) PURE;
    
    /// <summary>
    /// Draws a color glyph run that has the format of
    /// DWRITE_GLYPH_IMAGE_FORMATS_COLR_PAINT_TREE.
    /// </summary>
    /// <param name="colorPaletteIndex">The index used to select a color palette within
    /// a color font. Note that this not the same as the paletteIndex in the
    /// DWRITE_COLOR_GLYPH_RUN struct, which is not relevant for paint glyphs.</param>
    STDMETHOD_(void, DrawPaintGlyphRun)(
        D2D1_POINT_2F baselineOrigin,
        _In_ CONST DWRITE_GLYPH_RUN *glyphRun,
        _In_opt_ ID2D1Brush *defaultFillBrush = NULL,
        UINT32 colorPaletteIndex = 0,
        DWRITE_MEASURING_MODE measuringMode = DWRITE_MEASURING_MODE_NATURAL 
        ) PURE;
    
    /// <summary>
    /// Draws a glyph run, using color representations of glyphs if available.
    /// </summary>
    STDMETHOD_(void, DrawGlyphRunWithColorSupport)(
        D2D1_POINT_2F baselineOrigin,
        _In_ CONST DWRITE_GLYPH_RUN *glyphRun,
        _In_opt_ CONST DWRITE_GLYPH_RUN_DESCRIPTION *glyphRunDescription,
        _In_opt_ ID2D1Brush *foregroundBrush,
        _In_opt_ ID2D1SvgGlyphStyle *svgGlyphStyle,
        UINT32 colorPaletteIndex = 0,
        DWRITE_MEASURING_MODE measuringMode = DWRITE_MEASURING_MODE_NATURAL,
        D2D1_COLOR_BITMAP_GLYPH_SNAP_OPTION bitmapSnapOption = D2D1_COLOR_BITMAP_GLYPH_SNAP_OPTION_DEFAULT 
        ) PURE;
}; // interface ID2D1DeviceContext7
