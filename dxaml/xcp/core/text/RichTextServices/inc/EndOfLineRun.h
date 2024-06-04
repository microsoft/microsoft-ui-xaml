// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      EndOfLineRun class represents line terminating content.

#pragma once

#include "TextRun.h"

namespace RichTextServices
{
    //---------------------------------------------------------------------------
    //
    //  EndOfLineRun
    //
    //  EndOfLineRun class represents line terminating content.
    //
    //---------------------------------------------------------------------------
    class EndOfLineRun : public TextRun
    {
    public:
        // Initializes a new instance of the EndOfLineRun class.
        EndOfLineRun(_In_ XUINT32 characterIndex);

    protected:
        // Release resources associated with the EndOfLineRun.
        ~EndOfLineRun() override;
    };
}
