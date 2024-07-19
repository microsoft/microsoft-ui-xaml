// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ParagraphNodeBreak.h"

using namespace RichTextServices;

//---------------------------------------------------------------------------
//
//  Member:
//      ParagraphNodeBreak::ParagraphNodeBreak
//
//  Synopsis:
//      Initializes instance of ParagraphNodeBreak.
//
//---------------------------------------------------------------------------
ParagraphNodeBreak::ParagraphNodeBreak(
    _In_ XUINT32 breakIndex,
    _In_opt_ TextLineBreak *pLineBreak,
    _In_ TextRunCache *pRunCache
    ) :
    BlockNodeBreak(breakIndex),
    m_pLineBreak(pLineBreak),
    m_pRunCache(pRunCache)
{
    AddRefInterface(m_pLineBreak);
    AddRefInterface(m_pRunCache);
}

//---------------------------------------------------------------------------
//
//  Member:
//      ParagraphNodeBreak::~ParagraphNodeBreak
//
//  Synopsis:
//      Releases resources associated with this ParagraphNodeBreak.
//
//---------------------------------------------------------------------------
ParagraphNodeBreak::~ParagraphNodeBreak()
{
    ReleaseInterface(m_pLineBreak);
    ReleaseInterface(m_pRunCache);
}

//---------------------------------------------------------------------------
//
//  Member:
//      ParagraphNodeBreak::Equals
//
//  Synopsis:
//      
//      BlockNodeBreak::Equals override for paragraph breaks. Checks that the break's
//      paragraph-relative index matches this objects.
//
//---------------------------------------------------------------------------
bool ParagraphNodeBreak::Equals(
    _In_opt_ BlockNodeBreak *pBreak
    )
{
    bool equals = false;
    ParagraphNodeBreak *pParagraphBreak = static_cast<ParagraphNodeBreak *>(pBreak);

    if (pParagraphBreak  != NULL)
    {
        // Index equality is enough here, if any actual content changed that is processed
        // through content changed handlers and Measure will be invalidated on affected
        // nodes, negating bypass.
        equals = (pParagraphBreak ->GetBreakIndex() == GetBreakIndex());
    }

    return equals;
}

