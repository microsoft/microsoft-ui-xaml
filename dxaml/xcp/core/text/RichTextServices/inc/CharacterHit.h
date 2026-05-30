// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace RichTextServices
{
    //---------------------------------------------------------------------------
    //
    //  CharacterHit
    //
    //  Represents information about a character hit within a glyph run.
    //
    //---------------------------------------------------------------------------
    struct CharacterHit
    {
        // The index of the first character that got hit.
        XINT32 firstCharacterIndex;

        // The trailing length value for the character that got hit.
        XINT32 trailingLength;
    };
}
