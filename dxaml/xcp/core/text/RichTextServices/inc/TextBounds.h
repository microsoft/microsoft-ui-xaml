// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace RichTextServices
{
    //---------------------------------------------------------------------------
    //
    //  TextBounds
    //
    //  Represents information about text bounds including bounding rect and 
    //  flow direction.
    //
    //---------------------------------------------------------------------------
    struct TextBounds
    {
        // Bounding rect.
        XRECTF rect;

        // Flow Direction.
        FlowDirection::Enum flowDirection;
    };
}
