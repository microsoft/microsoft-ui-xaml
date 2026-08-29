// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "BlockNodeBreak.h"

//---------------------------------------------------------------------------
//
//  PageNodeBreak
//
//  Contains state at the point where a PageNode is broken.
//
//---------------------------------------------------------------------------
class PageNodeBreak : public BlockNodeBreak
{
public:
    PageNodeBreak(
        _In_ XUINT32 breakIndex,
        _In_ XUINT32 blockIndexInCollection,
        _In_opt_ BlockNodeBreak *pBlockBreak
        );

    BlockNodeBreak *GetBlockBreak() const;

    XUINT32 GetBlockIndexInCollection() const;

    // Equality operator - BlockNodeBreak override.
    bool Equals(_In_opt_ BlockNodeBreak *pBreak);

private:
    // Block break for this page break.
    BlockNodeBreak *m_pBlockBreak;

    // PageNode is based on a BlockCollection. PageNodeBreak indicates
    // the index in the collection at which PageNode should start formatting. If a BlockBreak
    // is also included, then the formatting is partially completed on a previous page.
    XUINT32 m_blockIndexInCollection;

    virtual
    ~PageNodeBreak();
};

//---------------------------------------------------------------------------
//
//  Member:
//      PageNodeBreak::GetBlockBreak
//
//  Returns:
//      The BlockBreak associated with this PageNode break.
//
//---------------------------------------------------------------------------
inline BlockNodeBreak *PageNodeBreak::GetBlockBreak() const
{
    return m_pBlockBreak;
}

//---------------------------------------------------------------------------
//
//  Member:
//      PageNodeBreak::GetBlockIndexInCollection
//
//  Returns:
//      The index of the block at which formatting starts.
//
//---------------------------------------------------------------------------
inline XUINT32 PageNodeBreak::GetBlockIndexInCollection() const
{
    return m_blockIndexInCollection;
}
