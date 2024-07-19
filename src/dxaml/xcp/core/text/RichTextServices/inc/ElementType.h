// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace RichTextServices
{
    //---------------------------------------------------------------------------
    //
    //  ElementType
    //
    //  Identifies the role of a TextElementBackingStore element.
    //
    //---------------------------------------------------------------------------
    struct ElementType
    {
        enum Enum
        {
            // Structural element that holds Inline elements.
            Paragraph = 0,

            // Formatting element that may hold other Inline elements.
            Inline = 1,

            // An explicit line break within a Paragraph.
            LineBreak = 2,

            // An embedded object (UIElement).
            Object = 3,
        };
    };
}
