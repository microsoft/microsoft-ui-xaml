// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace RichTextServices
{
    //---------------------------------------------------------------------------
    //
    //  TextAlignment
    //
    //  Specifies the placement of text relative to a containing Paragraph.
    //
    //---------------------------------------------------------------------------
    struct TextAlignment
    {
        enum Enum
        {
            // Text is aligned to the left.
            Left = 0,

            // Text is aligned to the right.
            Right = 1,

            // Text is centered.
            Center = 2,

            // Text is justified.
            Justify = 3,
        };
    };
}
