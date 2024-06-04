// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      CharacterProperties class provides character classification based on 
//      Unicode.

#pragma once

namespace UnicodeData
{
    //---------------------------------------------------------------------------
    //
    //  CharacterProperties
    //
    //  Provides character classification based on Unicode.
    //
    //---------------------------------------------------------------------------
    class CharacterProperties
    {
    public:
        static bool ScriptNeedsCaretStopPerCharacter(_In_ XINT32 script);

        static bool IsExtendedCharacter(_In_ XUINT32 character);

        static bool IsSimpleCombiningMark(_In_ XUINT32 character);
    };
}

