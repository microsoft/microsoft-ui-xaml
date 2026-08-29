// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TextObject.h"

struct IEmbeddedElementHost;

namespace RichTextServices
{
    class TextRun;

    //---------------------------------------------------------------------------
    //
    //  TextSource
    //
    //  TextSource provides character and formatting data to the TextFormatter.
    //  It translates platform-specific types and values into types understood
    //  by the TextFormatter class.
    //
    //---------------------------------------------------------------------------
    class TextSource : public TextObject
    {
    public:

        //-----------------------------------------------------------------------
        //
        //  Member:
        //      TextSource::GetTextRun
        //
        //  Synopsis:
        //      Fetches a run of text starting at the specified character index.
        //
        //-----------------------------------------------------------------------
        virtual 
        Result::Enum GetTextRun(
            _In_ XUINT32 characterIndex, 
                // Index of first character in the text run.
            _Outptr_ TextRun **ppTextRun
                // Address of pointer to the fetched run.
            ) = 0;

        //-----------------------------------------------------------------------
        //
        //  Member:
        //      TextSource::GetEmbeddedElementHost
        //
        //  Synopsis:
        //      Gets the embedded object host for the TextSource in current formatting context.
        //
        //-----------------------------------------------------------------------
        virtual IEmbeddedElementHost *GetEmbeddedElementHost() const = 0;
    };
}
