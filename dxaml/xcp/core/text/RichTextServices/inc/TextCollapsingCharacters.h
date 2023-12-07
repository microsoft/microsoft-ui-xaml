// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TextCollapsingSymbol.h"
#include "FontAndScriptServices.h"

namespace RichTextServices
{

    class TextDrawingContext;

    //---------------------------------------------------------------------------
    //
    //  TextCollapsingCharacters
    //
    //  TextCollapsingCharacters class contains logic to format and display a
    //  character string as collapsing symbol.
    //
    //---------------------------------------------------------------------------
    class TextCollapsingCharacters : public TextCollapsingSymbol
    {
    public:

        // Constructor.
        TextCollapsingCharacters(
            _In_reads_(length) const WCHAR *pCharacters,
            _In_ XUINT32 length,
            _In_reads_(length) const XFLOAT *pWidths,
            _In_ XFLOAT totalWidth,
            _In_ FlowDirection::Enum flowDirection,
            _In_ TextRunProperties *pProperties,
            _In_opt_ IFssFontFace *pFontFace
             );

        XFLOAT GetWidth() const override;

        _Check_return_ Result::Enum Draw(
            _In_ TextDrawingContext *pDrawingContext,
            _In_ const XPOINTF &origin,
            _In_ XFLOAT viewportWidth,
            _In_ FlowDirection::Enum flowDirection
            ) override;

    protected:

        // Destructor
        ~TextCollapsingCharacters() override;

    private:

        // Collapsing characters.
        const WCHAR *m_pCharacters;

        // Length of collapsing string.
        XUINT32 m_length;

        // Collapsing character widths.
        const XFLOAT *m_pWidths;

        // Cumulative width of all collapsing characters.
        XFLOAT m_totalWidth;

        // Collapsing character properties.
        TextRunProperties *m_pProperties;

        // Font face associated with collapsing characters.
        IFssFontFace *m_pFontFace;

        // Collapsing characters flow direction.
        FlowDirection::Enum m_flowDirection;
    };
}
