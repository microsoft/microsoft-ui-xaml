// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      EndOfParagraphRun represents paragraph terminating content.

#pragma once

#include "TextRun.h"

namespace RichTextServices
{
    //---------------------------------------------------------------------------
    //
    //  EndOfParagraphRun
    //
    //  EndOfParagraphRun represents paragraph terminating content.
    //
    //---------------------------------------------------------------------------
    class EndOfParagraphRun : public TextRun
    {
    public:

        // Initializes a new instance of the EndOfParagraphRun class.
        EndOfParagraphRun(_In_ XUINT32 length, _In_ XUINT32 characterIndex);

    protected:
        // Release resources associated with the EndOfParagraphRun.
        ~EndOfParagraphRun() override;
    };
}
