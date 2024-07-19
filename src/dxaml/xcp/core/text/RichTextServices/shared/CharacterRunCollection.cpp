// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CharacterRunCollection.h"

using namespace RichTextServices::Internal;

//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunCollection::GetRootNodeIndexed
//
//  Returns:
//      The root node of the tree of nodes held in this collection.
//
//---------------------------------------------------------------------------
SplayTreeNode *CharacterRunCollection::GetRootNodeIndexed(_In_ XUINT32 index) const
{
    ASSERT(CharTreeIndex == index);
    return m_pRootNode;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunCollection::SetRootNodeIndexed
//
//  Synopsis:
//      Sets the root node of the tree of nodes held in this collection.
//
//---------------------------------------------------------------------------
void CharacterRunCollection::SetRootNodeIndexed(
    _In_ XUINT32 index,
    _In_ SplayTreeNode *pRootNode
    )
{
    ASSERT(CharTreeIndex == index);
    m_pRootNode = static_cast<CharacterRunNode *>(pRootNode);
}

//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunCollection::AsCharacterRunNode
//
//  Returns:
//      This collection as a CharacterRunNode, or NULL if this collection is
//      not a CharacterRunNode.
//
//---------------------------------------------------------------------------
CharacterRunNode *CharacterRunCollection::AsCharacterRunNode()
{
    return NULL;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunCollection::GetNodeAtOffset
//
//  Returns:
//      The node covering a specified character.
//
//  Notes:
//      Does NOT recurse into contained collections.  The return value is directly
//      contained by this collection and may itself be a collection.
//
//      nodeOffset receives the node's start edge offset.
//
//      Ambiguous positions between two elements always return the following element.
//
//---------------------------------------------------------------------------
CharacterRunNode *CharacterRunCollection::GetNodeAtOffset(
    _In_ XUINT32 offset,
    _Out_ XUINT32 *pNodeOffset
    )
{
    // Since the collection start edge occupies one symbol, a contained offset must be >= 1.
    ASSERT(offset > 0);

    CharacterRunNode *pNode = NULL;

    if (m_pRootNode != NULL)
    {
        pNode = m_pRootNode->GetSiblingNodeAtOffset(offset - 1, pNodeOffset);
        (*pNodeOffset) += 1;
    }

    return pNode;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunCollection::Uninitialize
//
//  Synopsis:
//      Frees contained nodes.
//
//---------------------------------------------------------------------------
void CharacterRunCollection::Uninitialize(
    _In_opt_ CharacterRunNode::IDeleteTreeListener *pListener
    )
{
    CharacterRunNode::DeleteTree(m_pRootNode, pListener);
}
