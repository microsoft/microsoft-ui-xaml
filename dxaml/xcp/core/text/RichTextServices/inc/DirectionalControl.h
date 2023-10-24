// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace RichTextServices
{
    //---------------------------------------------------------------------------
    //
    //  DirectionalControl
    //
    //  DirectionalControl enumerates types of directional control a 
    //  DirectionalControlRun may specify.
    //
    //---------------------------------------------------------------------------
    struct DirectionalControl
    {
        enum Enum
        {
            // No directional control.
            None, 

            // Directional control change from left to right.
            LeftToRightEmbedding,

            // Directional control change from right to left.
            RightToLeftEmbedding,

            // Pop directional formatting - paired with embedding run.
            PopDirectionalFormatting,

            // Explicit left to right mark.
            LeftToRightMark,

            // Explicit right to left mark.
            RightToLeftMark,

        };
    };
}