// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      EndOfLineRun class implementation.

#include "precomp.h"

using namespace RichTextServices;

//---------------------------------------------------------------------------
//
//  Member:
//      EndOfLineRun::EndOfLineRun
//
//  Synopsis:
//      Initializes a new instance of the EndOfLineRun class.
//
//---------------------------------------------------------------------------
EndOfLineRun::EndOfLineRun(
    _In_ XUINT32 characterIndex
        // Index of the first character of the run.
    ) :
    TextRun(1, characterIndex, TextRunType::EndOfLine)
{
}

//---------------------------------------------------------------------------
//
//  Member:
//      EndOfLineRun::~EndOfLineRun
//
//  Synopsis:
//      Release resources associated with this instance of EndOfLineRun.
//
//---------------------------------------------------------------------------
EndOfLineRun::~EndOfLineRun()
{
}

