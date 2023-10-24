// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      EndOfParagraphRun class implementation.

#include "precomp.h"

using namespace RichTextServices;

//---------------------------------------------------------------------------
//
//  Member:
//      EndOfParagraphRun::EndOfParagraphRun
//
//  Synopsis:
//      Initializes a new instance of the EndOfParagraphRun class, allowing 
//      length to be specified.
//
//  Notes:
//      TODO: EOP run should only allow length 1. Some SL Controls
//      (TextBlock and TextBox) specify synthetic EOP character not in the 
//      backing store. If EOP run with length 1 is used, the backing store end 
//      will be passed, which causes failures higher up the stack. The controls
//      should be written to not introduce synthetic EOP.
//
//---------------------------------------------------------------------------
EndOfParagraphRun::EndOfParagraphRun(
    _In_ XUINT32 length,
        // Number of characters in the run.
    _In_ XUINT32 characterIndex
        // Index of the first character of the run.
    ) :
    TextRun(length, characterIndex, TextRunType::EndOfParagraph)
{
    ASSERT(length <= 1);
}


//---------------------------------------------------------------------------
//
//  Member:
//      EndOfParagraphRun::~EndOfParagraphRun
//
//  Synopsis:
//      Release resources associated with this instance of EndOfParagraphRun.
//
//---------------------------------------------------------------------------
EndOfParagraphRun::~EndOfParagraphRun()
{
}
