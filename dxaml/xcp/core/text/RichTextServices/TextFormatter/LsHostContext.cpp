// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      LsHostContext implementation.

#include "precomp.h"
#include "LsHostContext.h"

using namespace RichTextServices;
using namespace RichTextServices::Internal;

//---------------------------------------------------------------------------
//
//  Member:
//      LsTextLine::Create
//
//  Synopsis:
//      Creates a new instance of the LsHostContext struct.
//
//---------------------------------------------------------------------------
LsHostContext::LsHostContext(
    _In_ IFontAndScriptServices *pFontAndScriptServices
    )
{
    // TODO: use ctor initialization list + convert struct members to PascalCase
    this->pFontAndScriptServices = pFontAndScriptServices;
    pTextSource = NULL;
    pTextStore = NULL;
    pParagraphProperties = NULL;
    pDrawingContext = NULL;
    hasMultiCharacterClusters = FALSE;
    lineStartIndex = 0;
    collapsedLine  = FALSE;
    useEmergencyBreaking = FALSE;
    clipLastWordOnLine = FALSE;
}
