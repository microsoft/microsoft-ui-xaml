// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

using namespace RichTextServices;
using namespace RichTextServices::Internal;

//-----------------------------------------------------------------------
//
//  Member:
//      TextCollapsingCharacters::TextCollapsingCharacters
//
//  Synopsis:
//      Constructor.
//
//-----------------------------------------------------------------------
TextCollapsingCharacters::TextCollapsingCharacters(
    _In_reads_(length) const WCHAR *pCharacters,
        // Character string displayed as collapsing symbol.
        // TextCollapsingCharacters takes ownership of the string and deletes it at the end of its lifetime.
    _In_ XUINT32 length,
        // Length of collapsing string.
    _In_reads_(length) const XFLOAT *pWidths,
        // Array of widths of collapsing characters.
        // TextCollapsingCharacters takes ownership of the widths array and deletes it at the end of its lifetime.
    _In_ XFLOAT totalWidth,
        // Total width of collapsing symbol.
    _In_ RichTextServices::FlowDirection::Enum flowDirection,
        // Flow direction of collapsing characters.
    _In_ TextRunProperties *pProperties,
        // TextRunProperties applied to collapsing characters.
        // TextCollapsingCharacters takes ownership of properties and releases it at the end of its lifetime.
    _In_opt_ IFssFontFace *pFontFace
    )
{
    ASSERT(pCharacters != NULL);
    ASSERT(pProperties != NULL);
    ASSERT(pWidths != NULL);

    m_pCharacters = pCharacters;
    m_length = length;
    m_pWidths = pWidths;
    m_totalWidth = totalWidth;
    m_flowDirection = flowDirection;
    m_pProperties = pProperties;
    m_pProperties->AddRef();
    m_pFontFace = pFontFace;
    AddRefInterface(m_pFontFace);
}

//-----------------------------------------------------------------------
//
//  Member:
//      TextCollapsingCharacters::~TextCollapsingCharacters
//
//  Synopsis:
//      Destructor.
//
//-----------------------------------------------------------------------
TextCollapsingCharacters::~TextCollapsingCharacters()
{
    // TextCollapsingCharacters takes ownership of characters, widths and properties
    // and must delete them.
    delete m_pCharacters;
    delete m_pWidths;
    m_pProperties->Release();
    ReleaseInterface(m_pFontFace);
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextCollapsingCharacters::GetWidth
//
//  Synopsis:
//      Gets width of all collapsing characters.
//
//---------------------------------------------------------------------------
XFLOAT TextCollapsingCharacters::GetWidth() const
{
    return m_totalWidth;
}

//-----------------------------------------------------------------------
//
//  Member:
//      TextCollapsingCharacters::Draw
//
//  Synopsis:
//      Creates rendering data for the collapsing characters.
//
//-----------------------------------------------------------------------
_Check_return_ Result::Enum TextCollapsingCharacters::Draw(
    _In_ TextDrawingContext *pDrawingContext,
        // The TextDrawingContext object onto which the TextLine is drawn.
    _In_ const XPOINTF &origin,
        // A point value that represents the drawing origin.
    _In_ XFLOAT viewportWidth,
        // A value that represents the viewport width available for rendering.
    _In_ RichTextServices::FlowDirection::Enum flowDirection
        // Paragraph's flow direction.
    )
{
    Result::Enum txhr = Result::Success;
    XPOINTF adjustedOrigin = origin;
    PALText::GlyphRun *pGlyphRun = NULL;
    XUINT32 *pCodePoints = NULL;
    XUINT16 *pGlyphIndices = NULL;
    XFLOAT  *pGlyphAdvances = NULL;

    IFC_EXPECT_RTS(pDrawingContext);

    if (flowDirection == RichTextServices::FlowDirection::RightToLeft)
    {
        adjustedOrigin.x = viewportWidth - adjustedOrigin.x;
    }

    if (m_flowDirection != flowDirection)
    {
        // If the collapsing characters FlowDirection is different than the dominant
        // paragraph FlowDirection, adjust so characters will be drawn correctly.
        if (m_flowDirection == RichTextServices::FlowDirection::LeftToRight && flowDirection == RichTextServices::FlowDirection::RightToLeft)
        {
            // LTR symbol, RTL para. Shift origin to the left.
            adjustedOrigin.x -= m_totalWidth;
        }
        else if (m_flowDirection == RichTextServices::FlowDirection::RightToLeft && flowDirection == RichTextServices::FlowDirection::LeftToRight)
        {
            // RTL symbol, LTR para. Shift origin to the right.
            adjustedOrigin.x += m_totalWidth;
        }
    }

    if (m_pFontFace != NULL)
    {
        XUINT32 glyphIndex;

        // Covert UTF16 codepoints to UTF32 codepoints and get corresponding glyph indices.
        // There is no need to use DecodeUtf16Character in this case, since currently collapsing symbol
        // is only character-based.
        IFC_OOM_RTS(pCodePoints    = new XUINT32[m_length]);
        IFC_OOM_RTS(pGlyphIndices  = new XUINT16[m_length]);
        IFC_OOM_RTS(pGlyphAdvances = new XFLOAT [m_length]);
        for (glyphIndex = 0; glyphIndex < m_length; glyphIndex++)
        {
            pCodePoints[glyphIndex]    = m_pCharacters[glyphIndex];
            pGlyphAdvances[glyphIndex] = m_pWidths[glyphIndex];
        }
        IFC_FROM_HRESULT_RTS(m_pFontFace->GetGlyphIndices(pCodePoints, m_length, pGlyphIndices));

        // Initialize glyph run structure and pass it to the drawing context.

        IFC_OOM_RTS(pGlyphRun = new FssGlyphRun());
        pGlyphRun->FontFace      = m_pFontFace;
        pGlyphRun->FontEmSize    = m_pProperties->GetFontSize();
        pGlyphRun->GlyphCount    = m_length;
        pGlyphRun->GlyphIndices  = pGlyphIndices;
        pGlyphRun->GlyphAdvances = pGlyphAdvances;
        pGlyphRun->GlyphOffsets  = NULL;
        pGlyphRun->IsSideways    = FALSE;
        pGlyphRun->BidiLevel     = (m_flowDirection == RichTextServices::FlowDirection::RightToLeft) ? 1 : 0;

        AddRefInterface(pGlyphRun->FontFace);
        pGlyphIndices  = NULL;
        pGlyphAdvances = NULL;

        IFC_FROM_HRESULT_RTS(pDrawingContext->DrawGlyphRun(adjustedOrigin, m_totalWidth, pGlyphRun, m_pProperties->GetForegroundBrushSource(), NULL));
        pGlyphRun = NULL;
    }

Cleanup:
    delete [] pCodePoints;
    delete [] pGlyphIndices;
    delete [] pGlyphAdvances;
    if (pGlyphRun != NULL)
    {
        delete [] pGlyphRun->GlyphIndices;
        delete [] pGlyphRun->GlyphAdvances;
        ReleaseInterface(pGlyphRun->FontFace);
        delete pGlyphRun;
    }

    return txhr;
}
