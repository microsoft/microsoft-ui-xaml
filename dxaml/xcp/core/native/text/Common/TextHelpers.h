// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Various text helper functions.

#pragma once

#include <weakref_ptr.h>
class DWriteFontFace;

//---------------------------------------------------------------------------
//
//  Returns D2D1_RENDER_TARGET_PROPERTIES used for text rendering
//
//---------------------------------------------------------------------------
inline D2D1_RENDER_TARGET_PROPERTIES GetCommonD2DRenderTargetProperties()
{
    return D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_SOFTWARE,
        D2D1::PixelFormat(DXGI_FORMAT_A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        96.0f,
        96.0f,
        D2D1_RENDER_TARGET_USAGE_NONE,
        D2D1_FEATURE_LEVEL_DEFAULT);
}

//---------------------------------------------------------------------------
//
//  Converts PALText::GlyphRun to DWRITE_GLYPH_RUN.
//
//---------------------------------------------------------------------------
inline DWRITE_GLYPH_RUN GetDWriteGlyphRun(const PALText::GlyphRun &glyphRun)
{
    DWRITE_GLYPH_RUN dwriteGlyphRun = {
        reinterpret_cast<DWriteFontFace *>(glyphRun.FontFace)->GetFontFace(),
        glyphRun.FontEmSize,
        glyphRun.GlyphCount,
        glyphRun.GlyphIndices,
        glyphRun.GlyphAdvances,
        reinterpret_cast<DWRITE_GLYPH_OFFSET const *>(glyphRun.GlyphOffsets),
        glyphRun.IsSideways,
        glyphRun.BidiLevel
    };
    return dwriteGlyphRun;
}

//---------------------------------------------------------------------------
//
//  Gets the underlying DirectWrite font face from the PAL font face.
//
//---------------------------------------------------------------------------
inline IDWriteFontFace4 *GetDWriteFontFace(_In_ PALText::IFontFace *fontFace)
{
    return reinterpret_cast<DWriteFontFace*>(fontFace)->GetFontFace();
}

//---------------------------------------------------------------------------
//
//  Converts CMILMatrix to D2D1::Matrix3x2F
//
//---------------------------------------------------------------------------
inline D2D1::Matrix3x2F GetD2DMatrix(const CMILMatrix &transform)
{
    return D2D1::Matrix3x2F(
        transform._11,
        transform._12,
        transform._21,
        transform._22,
        transform._31,
        transform._32);
}

//---------------------------------------------------------------------------
//
//  Converts D2D1::Matrix3x2F to CMILMatrix
//
//---------------------------------------------------------------------------
inline CMILMatrix GetMILMatrix(const D2D1::Matrix3x2F &matrix)
{
    CMILMatrix transform;
    transform._11 = matrix._11;
    transform._12 = matrix._12;
    transform._21 = matrix._21;
    transform._22 = matrix._22;
    transform._31 = matrix._31;
    transform._32 = matrix._32;
    return transform;
}

//---------------------------------------------------------------------------
//
//  Converts XRECTF_RB to D2D1_RECT_F
//
//---------------------------------------------------------------------------
inline D2D1_RECT_F GetD2DRectF(const XRECTF_RB &rect)
{
    return D2D1::RectF(rect.left, rect.top, rect.right, rect.bottom);
}

//---------------------------------------------------------------------------
//
//  Converts D2D1_RECT_F to XRECTF_RB
//
//---------------------------------------------------------------------------
inline XRECTF_RB GetRectF_RB(const D2D1_RECT_F &rect)
{
    XRECTF_RB xrect = { rect.left, rect.top, rect.right, rect.bottom };
    return xrect;
}

//---------------------------------------------------------------------------
//
//  Converts D2D1_POINT_2F to XPOINTF
//
//---------------------------------------------------------------------------
inline XPOINTF GetPointF(const D2D1_POINT_2F &point)
{
    XPOINTF xpoint = { point.x, point.y };
    return xpoint;
}

//---------------------------------------------------------------------------
//
//  Initializes ID2D1SolidColorBrush from CBrush.
//
//---------------------------------------------------------------------------
void InitializeD2DBrush(
    _In_ CBrush *pBrush,
    _Inout_ ID2D1SolidColorBrush *pD2DSolidColorBrush
    );

//---------------------------------------------------------------------------
//
//  Get Foreground brush from the property source.
//
//---------------------------------------------------------------------------
CBrush * GetForegroundBrush(_In_ const xref::weakref_ptr<CDependencyObject>& pBrushSource, _In_opt_ CBrush *pBrush, bool controlEnabled);

//---------------------------------------------------------------------------
//
//  Determines whether brush is explicitly set or inherited from the parent FE.
//
//---------------------------------------------------------------------------
bool HasExplicitForegroundBrush(_In_ const xref::weakref_ptr<CDependencyObject>& pBrushSource, _In_opt_ CBrush *pBrush);
