// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Implementation logic for  LsParagraphSpan class.

#include "precomp.h"
#include "LsParagraphSpan.h"

using namespace RichTextServices;
using namespace RichTextServices::Internal;

//---------------------------------------------------------------------------
//
//  Member:
//      LsParagraphSpan::LsParagraphSpan
//
//  Synopsis:
//      Constructor.
//
//---------------------------------------------------------------------------
LsParagraphSpan::LsParagraphSpan(
    _In_ XUINT32 characterIndex, 
        // Start cp of span.
    _In_ RichTextServices::FlowDirection::Enum flowDirection
        // Paragraph flow direction.
        ) : LsSpan(
                   characterIndex,
                   LsSpanType::Paragraph,
                   NULL
                   )
{
    m_flowDirection = flowDirection;
}

//---------------------------------------------------------------------------
//
//  Member:
//      GetBidiLevel
//
//  Synopsis:
//      Gets bidi embedding level of this span.
//
//---------------------------------------------------------------------------
XUINT8 LsParagraphSpan::GetBidiLevel() const
{
    if (m_flowDirection == RichTextServices::FlowDirection::RightToLeft)
    {
        return 1;
    }
    return 0;
}

