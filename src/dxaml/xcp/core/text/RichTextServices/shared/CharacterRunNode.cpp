// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CharacterRunCollection.h"
#include "CharacterRunNode.h"

using namespace RichTextServices::Internal;

//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunNode::CharacterRunNode
//
//  Synopsis:
//      Initializes a new CharacterRunNode instance.
//
//---------------------------------------------------------------------------
CharacterRunNode::CharacterRunNode(_In_ XUINT32 charCount)
{
    XCP_WEAK(&m_pParent);

    m_pParent = NULL;
    m_pLeftChild = NULL;
    m_pRightChild = NULL;
    m_charCount = charCount;
    m_leftCharCount = 0;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunNode::~CharacterRunNode
//
//  Synopsis:
//      Destructor.
//
//---------------------------------------------------------------------------
CharacterRunNode::~CharacterRunNode()
{
}

//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunNode::SetCount
//
//  Synopsis:
//      Sets the character count of this node.
//
//---------------------------------------------------------------------------
void CharacterRunNode::SetCount(_In_ XUINT32 charCount)
{
    Splay();
    m_charCount = charCount;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunNode::AdjustCount
//
//  Synopsis:
//      Modifies the character count of this node.
//
//---------------------------------------------------------------------------
void CharacterRunNode::AdjustCount(
    _In_ XINT32 delta
        // Character count to add to existing value, negative values are legal.
    )
{
    Splay();
    m_charCount += delta;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunNode::ReleaseCollectionReference
//
//  Synopsis:
//      Called when this node is no longer referenced by its collection.
//
//---------------------------------------------------------------------------
void CharacterRunNode::ReleaseCollectionReference()
{
    delete this;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunNode::GetSiblingOffset
//
//  Returns:
//      The character offset of the start edge of this node, relative to its
//      collection.
//
//---------------------------------------------------------------------------
XUINT32 CharacterRunNode::GetSiblingOffset()
{
    Splay();
    return m_leftCharCount;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunNode::GetSiblingEndOffset
//
//  Returns:
//      The character offset of the end edge of this node, relative to its tree
//      collection.
//
//---------------------------------------------------------------------------
XUINT32 CharacterRunNode::GetSiblingEndOffset()
{
    Splay();
    return m_leftCharCount + m_charCount;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunNode::GetOffset
//
//  Returns:
//      The character offset of the start edge of this node.
//
//---------------------------------------------------------------------------
XUINT32 CharacterRunNode::GetOffset()
{
    XUINT32 offset = 0;
    CharacterRunNode *pNode = this;

    do
    {
        pNode->Splay();

        offset += pNode->m_leftCharCount;

        CharacterRunCollection *pCollection = pNode->GetParentAsContainingCollection();

        // We don't expect to call GetOffset when a node is not part of a collection.
        // If we did, there is no global offset, only a sibling offset (GetSiblingOffset).
        ASSERT(pCollection != NULL);

        // Add the parent start edge.
        offset += 1;

        pNode = pCollection->AsCharacterRunNode();
    }
    while (pNode != NULL);

    return offset;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunNode::GetLengthOfTree
//
//  Returns:
//      The length of the tree rooted by this CharacterRunNode.
//
//---------------------------------------------------------------------------
XUINT32 CharacterRunNode::GetLengthOfTree()
{
    CharacterRunNode *pLastSibling = GetLastSibling();
    ASSERT(NULL == pLastSibling->m_pRightChild); // The last sibling never has a following node.

    return pLastSibling->m_leftCharCount + pLastSibling->m_charCount;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunNode::GetSiblingNodeAtOffset
//
//  Returns:
//      The sibling node covering a specified character.
//
//  Notes:
//      Does NOT recurse into contained collections.
//
//      nodeOffset receives the node's start edge offset within its collection.
//
//      Ambiguous positions between two elements always return the following element.
//
//---------------------------------------------------------------------------
CharacterRunNode *CharacterRunNode::GetSiblingNodeAtOffset(
    _In_ XUINT32 offset,
    _Out_ XUINT32 *pNodeOffset
    )
{
    Splay();

    CharacterRunNode *pNode = this;
    *pNodeOffset = 0;

    while (true)
    {
        if (offset < *pNodeOffset + pNode->m_leftCharCount)
        {
            // This node is to the right of the one we're looking for.
            pNode = pNode->m_pLeftChild;
            ASSERT(pNode != NULL);
        }
        else
        {
            XUINT32 count = pNode->m_charCount;

            if (offset <= *pNodeOffset + pNode->m_leftCharCount + count)
            {
                // We're somewhere inside this node.
                *pNodeOffset += pNode->m_leftCharCount;
                break;
            }
            else
            {
                // This node is to the left of the one we're looking for.
                *pNodeOffset += pNode->m_leftCharCount + count;
                pNode = pNode->m_pRightChild;
                ASSERT(pNode != NULL);
            }
        }
    }

    pNode->Splay();

    // If we're between two nodes, always return the following node.
    if (offset == *pNodeOffset + pNode->m_charCount)
    {
        CharacterRunNode *pNextNode = pNode->GetNextNode();
        if (pNextNode != NULL)
        {
            *pNodeOffset += pNode->m_charCount;
            pNode = pNextNode;
        }
    }

    return pNode;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunNode::GetContainingCollection
//
//  Returns:
//      Gets the node's collection, if any, otherwise NULL.
//
//---------------------------------------------------------------------------
CharacterRunCollection *CharacterRunNode::GetContainingCollection()
{
    return static_cast<CharacterRunCollection *>(SplayTreeNode::GetContainingCollection());
}

//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunNode::DeleteTree
//
//  Synopsis:
//      Deletes every node in the tree containing node.
//
//  Notes:
//      Includes contained trees.
//
//---------------------------------------------------------------------------
void CharacterRunNode::DeleteTree(
    _In_opt_ CharacterRunNode *pNode,
        // Root node to delete. May be NULL.
    _In_opt_ IDeleteTreeListener *pListener
        // Receives a callback for each deleted node. May be NULL.
    )
{
    if (pNode != NULL)
    {
        // Ensure pNode is a root.
        pNode->Splay();
        ASSERT(pNode->GetParent() == NULL);

        // Remove this tree from its container, if it has one.
        CharacterRunCollection *pCollection = pNode->GetParentAsContainingCollection();
        if (pCollection != NULL)
        {
            pCollection->DisconnectNodes();
        }

        // Free individual nodes, one (plus any contained nodes) per iteration.
        do
        {
            // Get the first node in the tree, which by definition has no left child.
            pNode = pNode->GetFirstNodeNoSplay();
            ASSERT(NULL == pNode->m_pLeftChild);

            // If this node has contained nodes, deal with them recursively.
            // We're not worried about blowing the stack because element trees are shallow, typically
            // just two levels deep (Paragraph/Run).  A Table might have six or seven levels.
            CharacterRunCollection *characterRunCollection = pNode->AsCharacterRunCollection();

            if (characterRunCollection != nullptr)
            {
                DeleteTree(characterRunCollection->GetRootNode(), pListener);
            }

            CharacterRunNode *pNextNode;
            CharacterRunNode *pParent = pNode->GetParent();
            CharacterRunNode *pRightChild = pNode->m_pRightChild;

            // Find the next node to examine and fix up the tree to account for the deletion
            // of this one.

            if (NULL == pParent)
            {
                // If pNode was a root, look at the right child next.
                pNextNode = pRightChild;
            }
            else
            {
                // If pNode was not a root, it was the left child of its parent.
                // It's an invariant that pNode has no left child, so we need to fix
                // parent's left child point to reference pNode's right child.
                pParent->m_pLeftChild = pRightChild;

                // The next node to look at is either pParent (if pRightChild is NULL) or
                // pRightChild (if pRightChild is not NULL).  We'll just assign pParent which
                // we know is non-NULL, and we'll find pRightChild at the start of the next
                // iteration if it's not NULL.
                pNextNode = pParent;
            }

            // If there is a right child, it needs to point up to the new parent.
            if (pRightChild != NULL)
            {
                pRightChild->m_pParent = pParent;
            }

            // Let the node know it's out of the tree now.
            pNode->OnRemoved(CharTreeIndex);

            if (pListener != NULL)
            {
                // This calls out into external code.
                pListener->OnNodeRemoved(pNode);
            }

            // Release the node, dropping the ref count.  External code (PersistentElementReference)
            // may still hold the node.
            pNode->ReleaseCollectionReference();

            pNode = pNextNode;
        }
        while (pNode != NULL);
    }
}

//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunNode::AsCharacterRunCollection
//
//  Returns:
//      This node as a CharacterRunCollection, NULL if this node is not a
//      CharacterRunCollection.
//
//---------------------------------------------------------------------------
CharacterRunCollection *CharacterRunNode::AsCharacterRunCollection()
{
    return NULL;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunNode::GetParentRawIndexed
//
//  Synopsis:
//      Gets the parent of this node, either a containing collection or another
//      sibling node.
//
//---------------------------------------------------------------------------
SplayTreeBase *CharacterRunNode::GetParentRawIndexed(_In_ XUINT32 index) const
{
    ASSERT(CharTreeIndex == index);
    return m_pParent;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunNode::SetParentIndexed
//
//  Synopsis:
//      Sets the parent of this node.
//
//---------------------------------------------------------------------------
void CharacterRunNode::SetParentIndexed(
    _In_ XUINT32 index,
    _In_ SplayTreeBase *pParent
    )
{
    ASSERT(CharTreeIndex == index);
    m_pParent = pParent;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunNode::GetLeftChildIndexed
//
//  Synopsis:
//      Gets the left child of this node.
//
//---------------------------------------------------------------------------
SplayTreeNode *CharacterRunNode::GetLeftChildIndexed(_In_ XUINT32 index) const
{
    ASSERT(CharTreeIndex == index);
    return m_pLeftChild;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunNode::SetLeftChildIndexed
//
//  Synopsis:
//      Sets the left child of this node.
//
//---------------------------------------------------------------------------
void CharacterRunNode::SetLeftChildIndexed(
    _In_ XUINT32 index,
    _In_ SplayTreeNode *pLeftChild
    )
{
    ASSERT(CharTreeIndex == index);
    m_pLeftChild = static_cast<CharacterRunNode *>(pLeftChild);
}

//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunNode::GetRightChildIndexed
//
//  Synopsis:
//      Gets the right child of this node.
//
//---------------------------------------------------------------------------
SplayTreeNode *CharacterRunNode::GetRightChildIndexed(_In_ XUINT32 index) const
{
    ASSERT(CharTreeIndex == index);
    return m_pRightChild;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunNode::SetRightChildIndexed
//
//  Synopsis:
//      Sets the right child of this node.
//
//---------------------------------------------------------------------------
void CharacterRunNode::SetRightChildIndexed(
    _In_ XUINT32 index,
    _In_ SplayTreeNode *pRightChild
    )
{
    ASSERT(CharTreeIndex == index);
    m_pRightChild = static_cast<CharacterRunNode *>(pRightChild);
}

//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunNode::OnNewLeftChild
//
//  Synopsis:
//      Called when this node receives a new left child.
//
//  Notes:
//      The new left child is guaranteed to have a NULL right child, i.e.
//      new child is the max of its sub-tree.
//
//          |                   |
//          X                   X
//         / \       ==>       / \
//        a                   a'
//       / \                 /
//
//---------------------------------------------------------------------------
void CharacterRunNode::OnNewLeftChild(_In_ XUINT32 index)
{
    ASSERT(CharTreeIndex == index);
    ASSERT(GetParent() == NULL); // Ensure we don'CharacterRunNode need to loop below updating left char counts.

    if (m_pLeftChild == NULL)
    {
        m_leftCharCount = 0;
    }
    else
    {
        ASSERT(m_pLeftChild->m_pRightChild == NULL); // Caller guarantees this.
        m_leftCharCount = m_pLeftChild->m_leftCharCount + m_pLeftChild->m_charCount;
    }
}

//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunNode::OnChildRotatedLeft
//
//  Synopsis:
//      Called after this node's left child is rotated left.
//
//          |               |
//          Y               X
//         / \             / \
//        a   X    ==>    Y   c
//           / \         / \
//          b   c       a   b
//
//
//---------------------------------------------------------------------------
void CharacterRunNode::OnChildRotatedLeft(_In_ XUINT32 index)
{
    ASSERT(CharTreeIndex == index);
    m_leftCharCount += m_pLeftChild->m_leftCharCount + m_pLeftChild->m_charCount;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunNode::OnRotatedRight
//
//  Synopsis:
//      Called after this node is rotated right.
//
//          |             |
//          X             Y
//         / \           / \
//        Y   c   ==>   a   X
//       / \               / \
//      a   b             b   c
//
//---------------------------------------------------------------------------
void CharacterRunNode::OnRotatedRight(_In_ XUINT32 index)
{
    ASSERT(CharTreeIndex == index);
    CharacterRunNode *pParent = GetParent();
    m_leftCharCount -= pParent->m_leftCharCount + pParent->m_charCount;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunNode::OnRemoved
//
//  Synopsis:
//      Called after this node is removed from its tree.
//
//---------------------------------------------------------------------------
void CharacterRunNode::OnRemoved(_In_ XUINT32 index)
{
    ASSERT(CharTreeIndex == index);

    // Clear out references to siblings.
    m_pParent = NULL;
    m_pLeftChild = NULL;
    m_pRightChild = NULL;
    m_leftCharCount = 0;

    // NB: m_charCount does not change, the node is still internally consistent.
}

//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunNode::GetParentAsCollection
//
//  Returns:
//      If this node is the root node of its collection, returns the collection.
//      Otherwise returns NULL.
//
//---------------------------------------------------------------------------
CharacterRunCollection *CharacterRunNode::GetParentAsContainingCollection() const
{
    return static_cast<CharacterRunCollection *>(SplayTreeNode::GetParentAsContainingCollection());
}
