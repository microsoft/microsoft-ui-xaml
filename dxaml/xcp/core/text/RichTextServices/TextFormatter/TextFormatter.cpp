// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      TextFormatter class implementation.

#include "precomp.h"
#include "LsTextFormatter.h"

using namespace RichTextServices;
using namespace RichTextServices::Internal;

//---------------------------------------------------------------------------
//
//  Member:
//      TextFormatter::Create
//
//  Synopsis:
//      Creates a new instance of the TextFormatter class.
//
//---------------------------------------------------------------------------
Result::Enum TextFormatter::Create(
    _In_ IFontAndScriptServices *pFontAndScriptServices,
        // Provides an interface to access font and script specific data.
    _Outptr_ TextFormatter **ppTextFormatter
        // An instance of TextFormatter.
    )
{
    Result::Enum txhr = Result::Success;
    IFC_OOM_RTS(*ppTextFormatter = new LsTextFormatter(pFontAndScriptServices));

Cleanup:
    return txhr;
}
