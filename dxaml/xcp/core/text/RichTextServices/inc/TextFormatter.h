// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Result.h"
#include "TextObject.h"

namespace RichTextServices 
{
    class TextLine;
    class TextLineBreak;
    class TextParagraphProperties;
    class TextRunCache;
    class TextSource;

    //---------------------------------------------------------------------------
    //
    //  TextFormatter
    //
    //  Provides services for formatting text and breaking text lines.
    //
    //---------------------------------------------------------------------------
    class TextFormatter : public TextObject
    {
    public:

        //-----------------------------------------------------------------------
        //
        //  Member:
        //      TextFormatter::FormatLine
        //
        //  Synopsis:
        //      Creates a TextLine that is used for formatting and displaying text content.
        //
        //-----------------------------------------------------------------------
        virtual 
        Result::Enum FormatLine(
            _In_ TextSource *pTextSource, 
                // The text source for the line.
            _In_ XUINT32 firstCharIndex, 
                // A value that specifies the character index of the starting character in the line.
            _In_ XFLOAT wrappingWidth,
                // A value that specifies the width at which the line should wrap.
            _In_ TextParagraphProperties *pTextParagraphProperties, 
                // A value that represents paragraph properties, such as alignment, or indentation.
            _In_opt_ TextLineBreak *pPreviousLineBreak, 
                // A TextLineBreak value that specifies the text formatter state, in terms of where 
                // the previous line in the paragraph was broken by the text formatting process.
            _In_opt_ TextRunCache *pTextRunCache, 
                // Provides run caching services in order to improve performance.
            _Outptr_ TextLine **ppTextLine
                // A TextLine that represents a line of text that can be displayed.
            ) = 0;

        // Creates a new instance of the TextFormatter class.
        static Result::Enum Create(
            _In_ IFontAndScriptServices *pFontAndScriptServices,
                // Provides an interface to access font and script specific data.
            _Outptr_ TextFormatter **ppTextFormatter
            );
    };
}
