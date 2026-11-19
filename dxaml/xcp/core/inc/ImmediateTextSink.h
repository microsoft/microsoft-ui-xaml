// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

struct DWRITE_GLYPH_RUN;
class CMILMatrix;
class CBrush;

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

protected:
    ~ImmediateTextSink() = default;
};
