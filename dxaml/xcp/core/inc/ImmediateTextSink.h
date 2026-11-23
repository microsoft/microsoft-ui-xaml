// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

struct DWRITE_GLYPH_RUN;
class CMILMatrix;
class CBrush;
class CImageBrush;

// "Retained XAML, immediate rendering" prototype: receives the render data XAML's text stack already produced (the same
// glyph runs, lines and highlight rects the D2D/PC render paths draw) so ImmediateRenderer can replay them through an
// immediate painter. Transforms map into the coordinate space of the element whose content is being visited.
class ImmediateTextSink
{
public:
    virtual void HighlightRect(const XRECTF& rect, const CMILMatrix& transform, _In_opt_ CBrush* brush) = 0;
    // clipLeft/clipRight are in glyph-run space; -FLT_MAX/FLT_MAX (XFLOAT_MIN/XFLOAT_MAX) when unclipped.
    virtual void GlyphRun(const DWRITE_GLYPH_RUN& run, const CMILMatrix& transform, _In_opt_ CBrush* brush, float clipLeft, float clipRight) = 0;
    virtual void Line(const XRECTF_RB& rect, const CMILMatrix& transform, _In_opt_ CBrush* brush) = 0;

    // ---- text ----
    // Images the text stack records (spell-check wavy underlines, RichEdit inline images): brush (a WriteableBitmap
    // ImageBrush with a software surface) stretched over rect.
    virtual void Image(const XRECTF_RB& rect, const CMILMatrix& transform, _In_ CImageBrush* brush) = 0;
    // Glyph image formats XAML resolved for the next GlyphRun (DWRITE_GLYPH_IMAGE_FORMATS). Outline-only formats mean
    // the run must be drawn monochrome (IsColorFontEnabled=False or high contrast); color runs arrive already translated.
    virtual void GlyphRunImageFormats(uint32_t formats) = 0;

protected:
    ~ImmediateTextSink() = default;
};
