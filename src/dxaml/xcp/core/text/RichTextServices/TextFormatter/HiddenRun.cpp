// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      HiddenRun class implementation.

#include "precomp.h"

using namespace RichTextServices;

//---------------------------------------------------------------------------
//
//  Member:
//      HiddenRun::HiddenRun
//
//  Synopsis:
//      Initializes a new instance of the HiddenRun class.
//
//---------------------------------------------------------------------------
HiddenRun::HiddenRun(
    _In_ XUINT32 characterIndex
        // Index of the first character of the run.
    ) :
    TextRun(1, characterIndex, TextRunType::Hidden)
{
}

//---------------------------------------------------------------------------
//
//  Member:
//      HiddenRun::~HiddenRun
//
//  Synopsis:
//      Release resources associated with this instance of HiddenRun.
//
//---------------------------------------------------------------------------
HiddenRun::~HiddenRun()
{
}
