// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TextBreak.h"

namespace RichTextServices
{
    class TextFormatter;

    //---------------------------------------------------------------------------
    //
    //  TextLineBreak
    //
    //  Contains state at the point where text line is broken by the line breaking 
    //  process.
    //
    //---------------------------------------------------------------------------
    class TextLineBreak : public TextBreak
    {
    public:

        // Returns the TextFormatter that produced this break record, or nullptr when the break
        // record is not bound to a specific formatter. A break record is bound to the exact
        // formatting context that created it, so continuation formatting must run on the
        // originating formatter. Callers select that formatter when acquiring a formatter (see
        // BlockLayoutHelpers::GetTextFormatter) instead of having the formatter re-route the call.
        // Non ref-counting: the returned formatter is kept alive by this break record's own
        // reference.
        virtual TextFormatter* GetTextFormatter() const { return nullptr; }
    };
}
