// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace RichTextServices
{
    //---------------------------------------------------------------------------
    //
    //  FlowDirection
    //
    //  FlowDirection enumerates ways in which text can flow on the page.
    //
    //---------------------------------------------------------------------------
    struct FlowDirection
    {
        enum Enum
        {
            // Text flows from left to right.
            LeftToRight = 0,

            // Text flows from right to left.
            RightToLeft = 1,
        };
    };
}
