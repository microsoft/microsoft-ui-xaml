// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "TextBreak.h"

using namespace RichTextServices;

//---------------------------------------------------------------------------
//
//  Member:
//      TextBreak ::Equals
//
//  Synopsis:
//      Compares with another TextBreak  object and checks for equality.
//
//---------------------------------------------------------------------------
bool TextBreak ::Equals(
    _In_ const TextBreak  *pBreak
        // Pointer to TextBreak to be compared.
    ) const
{
    // Base class just returns reference equals.
    return (this == pBreak);
}
