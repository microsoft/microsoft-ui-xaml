// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace RichTextServices
{
    //---------------------------------------------------------------------------
    //
    //  LogicalDirection
    //
    //  Specifies a direction in the address space of the backing store.  Backward direction faces
    //  preceding content; Forward direction faces following content.
    //
    //  LogicalDirection is absolute. It is never affected by physical render (if any) or 
    //  bidirectional scripts or ordering codes.
    //
    //---------------------------------------------------------------------------
    struct LogicalDirection
    {
        enum Enum
        {
            // Facing preceding content.
            Backward = 0,

            // Facing following content.
            Forward = 1
        };
    };
}