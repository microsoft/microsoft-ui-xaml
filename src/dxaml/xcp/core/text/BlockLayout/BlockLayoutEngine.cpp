// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "BlockLayoutEngine.h"
#include "PageNode.h"

using namespace RichTextServices;

//---------------------------------------------------------------------------
//
// BlockLayoutEngine::BlockLayoutEngine
//
//---------------------------------------------------------------------------
BlockLayoutEngine::BlockLayoutEngine(
    _In_ CDependencyObject *pOwner
    ) :
    m_pOwner(pOwner)
{
    ASSERT(m_pOwner  != NULL);
}

//---------------------------------------------------------------------------
//
// BlockLayoutEngine::~BlockLayoutEngine
//
//---------------------------------------------------------------------------
BlockLayoutEngine::~BlockLayoutEngine()
{
}

//---------------------------------------------------------------------------
//
//  BlockLayoutEngine::CreatePageNode
//
//  Synopsis:
//      Creates a page node that represents the root of block layout for its
//      owner.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT BlockLayoutEngine::CreatePageNode(
    _In_opt_ CBlockCollection *pBlocks,
    _In_ CFrameworkElement *pPageOwner,
    _Outptr_ BlockNode **ppPageNode
    )
{
    if (pBlocks == NULL)
    {
        // Block collection may be NULL or empty - NULL for TextBlock, empty for a content-less RichTextBlock.
        ASSERT(m_pOwner->OfTypeByIndex<KnownTypeIndex::TextBlock>());
    }

    // PageOwner should never be NULL, page can read padding and other properties from it.
    IFCCATASTROPHIC_RETURN(pPageOwner);

    *ppPageNode = new PageNode(this, pBlocks, pPageOwner);

    return S_OK;
}
