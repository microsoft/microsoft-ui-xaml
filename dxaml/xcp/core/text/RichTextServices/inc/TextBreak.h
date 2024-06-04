// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TextObject.h"

namespace RichTextServices
{
    //---------------------------------------------------------------------------
    //
    //  TextBreak
    //
    //  Base class representing breaking state for text objects - lines, blocks,
    //  etc.
    //
    //---------------------------------------------------------------------------
    class TextBreak : public TextObject
    {

    public:
        // Compares with another TextBreak object and checks for equality.
        virtual 
        bool Equals(_In_ const TextBreak *pBreak) const;

    };
}
