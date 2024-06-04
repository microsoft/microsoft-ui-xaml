// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CharacterProperties.h"

using namespace UnicodeData;

//-----------------------------------------------------------------------
//
//  Member:
//      IsExtendedCharacter
//
//  Synopsis:
//      True for characters in Unicode's extended range, including
//      surrogates.
//
//-----------------------------------------------------------------------
bool CharacterProperties::IsExtendedCharacter(
    _In_ XUINT32 character
        // Character code.
    )
{
    return IS_SURROGATE(character);
}

//-----------------------------------------------------------------------
//
//  Member:
//      IsCombiningMark
//
//  Synopsis:
//      True for all characters that have SimpleMarkClass associated 
//      with their Unicode Class.
//
//  Remarks:
//      Simple combininfg marks are treated similar to surrogates in
//      caret navigation - i.e. as non divisible clusters regardless
//      of script, unlike diacritic combining marks where navigation
//      within clusters depends on the script.
//
//-----------------------------------------------------------------------
bool CharacterProperties::IsSimpleCombiningMark(
    _In_ XUINT32 character
        // Character code.
    )
{
    // NOTE: char is 16 bit, so values > U+FFFF cannot be expressed by char.
    //       Hence check for values > U+FFFF is not necessary.

    // All characters with unicode less than 0x0300 are not combining marks, 
    // so exclude them early on.
    if ((character & 0xFE00) == 0)
    {
        return false;
    }
    else
    {
        return
            (0x0300 <= character && character <= 0x036F) ||
            (0x1DC0 <= character && character <= 0x1DFF) ||
            (0x20D0 <= character && character <= 0x20FF) ||
            (0xFE00 <= character && character <= 0xFE0F) ||
            (0xFE20 <= character && character <= 0xFE2F);
    }
}
