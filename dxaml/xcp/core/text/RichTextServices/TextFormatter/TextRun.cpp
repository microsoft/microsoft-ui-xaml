// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      TextRun class contains character and formatting data for one run of text.

#include "precomp.h"

using namespace RichTextServices;

//---------------------------------------------------------------------------
//
//  Member:
//      TextRun::TextRun
//
//  Synopsis:
//      Initializes a new instance of the TextRun class.
//
//---------------------------------------------------------------------------
TextRun::TextRun(
    _In_ XUINT32 length,
        // Number of characters in the run.
    _In_ XUINT32 characterIndex,
        // Index of the first character of the run.
    _In_ TextRunType::Enum type
        // Type of data stored in this run - hidden characters, plain text, etc.
    ) :
    m_length(length),
    m_characterIndex(characterIndex),
    m_type(type)
{
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextRun::~TextRun
//
//  Synopsis:
//      Release resources associated with this instance of TextRun.
//
//---------------------------------------------------------------------------
TextRun::~TextRun()
{
}

