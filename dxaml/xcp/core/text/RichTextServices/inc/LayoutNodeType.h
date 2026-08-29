// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace RichTextServices
{
    //---------------------------------------------------------------------------
    //
    //  LayoutNodeType
    //
    //  Identifies the role of a LayoutNode.
    //
    //---------------------------------------------------------------------------
    struct LayoutNodeType
    {
        enum Enum
        {
            // Root container of all other nodes.
            Page,

            // Maps to a Paragraph element.
            Paragraph,

            // Maps to a line of text.
            Line,
        };
    };
}
