// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "PageNodeBreak.h"

//---------------------------------------------------------------------------
//
//  Member:
//      PageNodeBreak::PageNodeBreak
//
//  Synopsis:
//      Initializes an instance of PageNodeBreak.
//
//---------------------------------------------------------------------------
PageNodeBreak::PageNodeBreak(
    _In_ XUINT32 breakIndex,
        // Index of break relative to page start.
    _In_ XUINT32 blockIndexInCollection,
        // Index of block in page-level block collection at which page starts formatting.
    _In_opt_ BlockNodeBreak *pBlockBreak
        // Break info for partially formatted block if it exists.
    ) : 
    BlockNodeBreak(breakIndex),
    m_blockIndexInCollection(blockIndexInCollection),
    m_pBlockBreak(pBlockBreak)
{
    AddRefInterface(m_pBlockBreak);
}

//---------------------------------------------------------------------------
//
//  Member:
//      PageNodeBreak::~PageNodeBreak
//
//  Synopsis:
//      Releases resources associated with PageNodeBreak.
//
//---------------------------------------------------------------------------
PageNodeBreak::~PageNodeBreak()
{
    ReleaseInterface(m_pBlockBreak);
}

//---------------------------------------------------------------------------
//
//  Member:
//      PageNodeBreak::Equals
//
//  Synopsis:
//      
//      BlockNodeBreak::Equals override for page breaks. Checks that the break's
//      absolute index, block index and block break matches this object's.
//
//---------------------------------------------------------------------------
bool PageNodeBreak::Equals(
    _In_opt_ BlockNodeBreak *pBreak
    )
{
    bool equals = false;
    PageNodeBreak *pPageBreak = static_cast<PageNodeBreak *>(pBreak);

    if (pPageBreak != NULL)
    {
        equals = ((pPageBreak->GetBreakIndex() == GetBreakIndex()) &&
                  (pPageBreak->GetBlockIndexInCollection() == GetBlockIndexInCollection()) &&
                  BlockNodeBreak::Equals(pPageBreak->GetBlockBreak(), GetBlockBreak()));
    }

    return equals;
}

