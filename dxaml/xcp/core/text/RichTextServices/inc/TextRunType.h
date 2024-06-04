// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace RichTextServices
{
    //---------------------------------------------------------------------------
    //
    //  TextRunType
    //
    //  TextRunType enumerates possible types of data contained in a TextRun.
    //
    //---------------------------------------------------------------------------
    struct TextRunType
    {
        enum Enum
        {
            // Unicode text.
            Text,

            // Hidden run - element start or end, or other hidden data.
            Hidden,

            // End of line.
            EndOfLine,

            // End of paragraph.
            EndOfParagraph,

            // Object - UIElement embedded in text.
            Object,

            // Directional control - explicit change in flow direction.
            DirectionalControl
        };
    };
}