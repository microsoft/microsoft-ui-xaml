// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"


TextOptions::TextOptions()
{
    m_textHintingMode = DirectUI::TextHintingMode::Fixed;
    m_textRenderingMode = DirectUI::TextRenderingMode::Auto;
    m_textFormattingMode = DirectUI::TextFormattingMode::Ideal;
}

TextOptions::TextOptions(
    DirectUI::TextHintingMode    textHintingMode,
    DirectUI::TextRenderingMode  textRenderingMode,
    DirectUI::TextFormattingMode textFormattingMode
)
:   m_textHintingMode   (textHintingMode),
    m_textRenderingMode (textRenderingMode),
    m_textFormattingMode(textFormattingMode)
{}

const TextOptions TextOptions::Default(
    DirectUI::TextHintingMode::Fixed,
    DirectUI::TextRenderingMode::Auto,
    DirectUI::TextFormattingMode::Ideal
);




//------------------------------------------------------------------------
//
//  Maps formatting and rendering modes to the glyph sizing method to use
//  when measuring.
//
//------------------------------------------------------------------------

MeasuringMode::Enum TextOptions::GetMeasuringMode() const
{
    MeasuringMode::Enum result = MeasuringMode::Natural;

    if (m_textHintingMode == DirectUI::TextHintingMode::Fixed
        &&  m_textFormattingMode == DirectUI::TextFormattingMode::Display)
    {
        result = MeasuringMode::GdiClassic;
    }

    return result;
}



//------------------------------------------------------------------------
//
//  Maps the text options to the kind of entry to cache for rendering.
//
//------------------------------------------------------------------------

GlyphCacheKind::Enum TextOptions::GetCacheKind() const
{
    if (m_textHintingMode == DirectUI::TextHintingMode::Animated)
    {
        return GlyphCacheKind::Subpixel;
    }

    if (m_textRenderingMode == DirectUI::TextRenderingMode::Aliased)
    {
        return GlyphCacheKind::Aliased;
    }

    // ClearType or Grayscale rendering
    if (m_textFormattingMode == DirectUI::TextFormattingMode::Ideal)
    {
        return GlyphCacheKind::Subpixel;
    }

    return GlyphCacheKind::Compatible;
}


