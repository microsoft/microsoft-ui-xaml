// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TextObject.h"

namespace RichTextServices
{

    class TextDrawingContext;

    //---------------------------------------------------------------------------
    //
    //  TextCollapsingSymbol
    //
    //  TextCollapsingSymbol class contains properties methods used to format
    //  and dispay the symbol shown when a TextLine is collapsed.
    //
    //---------------------------------------------------------------------------
    class TextCollapsingSymbol : public TextObject
    {
    public:

        //-----------------------------------------------------------------------
        //
        //  Member:
        //      TextCollapsingSymbol::GetWidth
        //
        //  Synopsis:
        //      Gets the width of the collapsing symbol.
        //
        //-----------------------------------------------------------------------
        virtual
        XFLOAT GetWidth() const = 0;

        //-----------------------------------------------------------------------
        //
        //  Member:
        //      TextCollapsingSymbol::Draw
        //
        //  Synopsis:
        //      Creates rendering data for the symbol.
        //
        //-----------------------------------------------------------------------
        virtual
        Result::Enum Draw(
            _In_ TextDrawingContext *pDrawingContext,
                // The TextDrawingContext object onto which the TextLine is drawn.
            _In_ const XPOINTF &origin,
                // A point value that represents the drawing origin.
            _In_ XFLOAT viewportWidth,
                // A value that represents the viewport width available for rendering.
            _In_ FlowDirection::Enum flowDirection
                // Paragraph's FlowDirection, indicating direction the symbol is drawn.
            ) = 0;

    protected:

        // Destructor
        ~TextCollapsingSymbol() override {};
    };
}
