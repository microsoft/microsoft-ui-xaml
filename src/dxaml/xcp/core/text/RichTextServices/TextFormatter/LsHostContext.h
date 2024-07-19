// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      LsHostContext provides access to various text-related services used by LS during 
//      formatting and display.

#pragma once

namespace RichTextServices
{
    class TextDrawingContext;
    class TextSource;
    class TextParagraphProperties;

    namespace Internal
    {
        class TextStore;

        //---------------------------------------------------------------------------
        //
        //  LsHostContext
        //
        //  Provides access to various text-related services used by LS during formatting and display.
        //
        //---------------------------------------------------------------------------
        struct LsHostContext
        {
            // Creates a new instance of the LsHostContext struct.
            LsHostContext(_In_ IFontAndScriptServices *pFontAndScriptServices);

            // Provides an interface to access font and script specific data
            IFontAndScriptServices *pFontAndScriptServices;

            // Provides an interface to get character and formatting data (available only during formatting)
            TextSource *pTextSource;

            // Provides run caching services in order to improve performance (available only during formatting)
            Internal::TextStore *pTextStore;

            // Provides a set of properties, such as alignment or indentation (available only during formatting)
            TextParagraphProperties *pParagraphProperties;

            // Provides an interface to populate text visual content using draw commands (available only during display)
            TextDrawingContext *pDrawingContext;

            // TRUE iff a line has at least one group of more than one characters mapped onto any
            // number of glyphs.  Set during formatting.
            bool hasMultiCharacterClusters;

            // TRUE if a collapsed line is being formatted.
            bool collapsedLine;

            // TRUE if emergency breaking is on.
            bool useEmergencyBreaking;

            // TRUE if we should include one more word on this line and clip it
            bool clipLastWordOnLine;

            // Start index of current line to be formatted.
            XUINT32 lineStartIndex;
        };
    }
}
