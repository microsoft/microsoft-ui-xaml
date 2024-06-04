// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TextStore.h"

using namespace RichTextServices;
using namespace RichTextServices::Internal;

//---------------------------------------------------------------------------
//
//  Member:
//      TextRunCache::Create
//
//  Synopsis:
//      Creates a new instance of the TextRunCache class.
//
//---------------------------------------------------------------------------
Result::Enum TextRunCache::Create(
    _Outptr_ TextRunCache **ppTextRunCache
        // An instance of TextRunCache
    )
{
    Result::Enum txhr = Result::Success;
    IFC_OOM_RTS(*ppTextRunCache = new TextStore());

Cleanup:
    return txhr;
}
