// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SplayTreeNode.h"
#include "SplayTreeCollection.h"

using namespace RichTextServices::Internal;

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeNode::SplayTreeNode
//
//  Synopsis:
//      Initializes a new SplayTreeNode instance.
//
//---------------------------------------------------------------------------
SplayTreeNode::SplayTreeNode()
{
}

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeNode::~SplayTreeNode
//
//  Synopsis:
//      Destructor.
//
//---------------------------------------------------------------------------
SplayTreeNode::~SplayTreeNode()
{
}

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeNode::RemoveIndexed
//
//  Synopsis:
//      Removes this node from its tree.
//
//  Returns:
//      The root of the original tree (NULL if this node was the only member).
//
//---------------------------------------------------------------------------
SplayTreeNode *SplayTreeNode::RemoveIndexed(_In_ XUINT32 index)
{
    Splay(index);

    SplayTreeNode *pLeftChild = DisconnectLeftChild(index);
    SplayTreeNode *pRightChild = DisconnectRightChild(index);

    OnRemoved(index);

    return JoinIndexed(index, pLeftChild, pRightChild);
}

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeNode::JoinIndexed
//
//  Synopsis:
//      Combines two trees.
//
//  Returns:
//      The root of the combined tree. Returns NULL if both trees are NULL.
//
//  Notes:
//      Node order of the trees is preserved. Every node in left tree will
//      precede every node in right tree in the combined tree.
//
//      leftNode and/or rightNode may be NULL.
//
//---------------------------------------------------------------------------
SplayTreeNode *SplayTreeNode::JoinIndexed(
    _In_ XUINT32 index,
    _In_opt_ SplayTreeNode *pLeftNode, 
    _In_opt_ SplayTreeNode *pRightNode
    )
{
    SplayTreeNode *pRootNode;

    if (pLeftNode != NULL)
    {
        pRootNode = pLeftNode->GetLastSiblingIndexed(index);
        ASSERT(pRootNode->GetParentIndexed(index) == NULL); // GetLastSibling guarantees a splay.
        ASSERT(NULL == pRootNode->GetRightChildIndexed(index)); // The last sibling never has a following node.

        if (pRightNode != NULL)
        {
            pRootNode->ConnectRightChild(index, pRightNode);
        }
    }
    else
    {
        if (pRightNode != NULL)
        {
            pRightNode->Splay(index);
        }
        pRootNode = pRightNode;
    }

    return pRootNode;
}

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeNode::Concatenate
//
//  Synopsis:
//      Combines two trees, returning the first and last nodes of the new tree.
//
//  Notes:
//      The combined tree consists of all nodes in pFirst1 - pLast1 followed
//      by all nodes in pFirst2 - pLast2.
//
//---------------------------------------------------------------------------
void SplayTreeNode::Concatenate(
    _In_opt_ SplayTreeNode *pFirst1,
        // First node of the left tree, may be NULL.
    _In_opt_ SplayTreeNode *pLast1,
        // Last node of the left tree, may be NULL.
    _In_opt_ SplayTreeNode *pFirst2,
        // First node of the right tree, may be NULL.
    _In_opt_ SplayTreeNode *pLast2,
        // Last node of the right tree, may be NULL.
    _Out_ SplayTreeNode **ppFirst,
        // Receives first node of the combined tree, may be NULL.
    _Out_ SplayTreeNode **ppLast
        // Receives last node of the combined tree, may be NULL.
    )
{
    if (NULL == pFirst1)
    {
        *ppFirst = pFirst2;
        *ppLast = pLast2;
    }
    else if (NULL == pFirst2)
    {
        *ppFirst = pFirst1;
        *ppLast = pLast1;
    }
    else
    {
        Join(pLast1, pFirst2);
        *ppFirst = pFirst1;
        *ppLast = pLast2;
    }
}

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeNode::SplitIndexed
//
//  Synopsis:
//      Divides a node's tree into two trees.
//
//  Returns:
//      The tree of all nodes originally following this node.
//
//  Notes:
//      This node becomes the root of a tree containing itself and all nodes
//      preceding.  The return value is a tree of all remaining nodes, the
//      nodes that follow this node.
//
//      The return value is NOT guaranteed to be the first node in the tree
//      of originally following nodes.  It may have a left child.
//
//---------------------------------------------------------------------------
SplayTreeNode *SplayTreeNode::SplitIndexed(_In_ XUINT32 index)
{
    Splay(index);
    return DisconnectRightChild(index);
}

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeNode::GetContainingCollection
//
//  Returns:
//      Gets the node's collection, if any, otherwise NULL.
//
//---------------------------------------------------------------------------
SplayTreeCollection *SplayTreeNode::GetContainingCollection()
{
    Splay();
    return GetParentAsContainingCollection();
}

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeNode::GetPreviousNodeIndexed
//
//  Returns:
//      The preceding node, or NULL if there is no preceding node.
//
//  Notes:
//      The returned node is always a root node.
//
//---------------------------------------------------------------------------
SplayTreeNode *SplayTreeNode::GetPreviousNodeIndexed(_In_ XUINT32 index)
{
    SplayTreeNode *pPreviousNode = GetLeftChildIndexed(index);

    if (pPreviousNode != NULL)
    {
        pPreviousNode = pPreviousNode->GetLastNode(index);
    }
    else
    {
        SplayTreeNode *pChild = this;
        SplayTreeNode *pParent = GetParentIndexed(index);

        while (pParent != NULL)
        {
            if (pChild == pParent->GetRightChildIndexed(index))
            {
                pPreviousNode = pParent;
                break;
            }
            pChild = pParent;
            pParent = pParent->GetParentIndexed(index);
        }
    }

    if (pPreviousNode != NULL)
    {
        pPreviousNode->Splay(index);
    }
    else
    {
        Splay(index);
    }

    return pPreviousNode;
}

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeNode::GetNextNode
//
//  Returns:
//      The following node, or NULL if there is no following node.
//
//  Notes:
//      The returned node is always a root node.
//
//---------------------------------------------------------------------------
SplayTreeNode *SplayTreeNode::GetNextNode()
{
    SplayTreeNode *pNextNode = GetRightChild();

    if (pNextNode != NULL)
    {
        pNextNode = pNextNode->GetFirstNode();
    }
    else
    {
        SplayTreeNode *pChild = this;
        SplayTreeNode *pParent = GetParent();

        while (pParent != NULL)
        {
            if (pChild == pParent->GetLeftChild())
            {
                pNextNode = pParent;
                break;
            }
            pChild = pParent;
            pParent = pParent->GetParent();
        }
    }

    if (pNextNode != NULL)
    {
        pNextNode->Splay();
    }
    else
    {
        Splay();
    }

    return pNextNode;
}

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeNode::GetFirstSibling
//
//  Returns:
//      The first node in the tree containing this node.
//
//---------------------------------------------------------------------------
SplayTreeNode *SplayTreeNode::GetFirstSibling()
{
    Splay();
    return GetFirstNode();
}

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeNode::GetLastSibling
//
//  Returns:
//      The last node in the tree containing this node.
//
//---------------------------------------------------------------------------
SplayTreeNode *SplayTreeNode::GetLastSiblingIndexed(_In_ XUINT32 index)
{
    Splay(index);
    return GetLastNode(index);
}

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeNode::DisconnectIndexed
//
//  Synopsis:
//      Detaches the tree containing this node from its collection.
//
//  Returns:
//      The containing collection, or NULL if this node is not contained.
//
//  Notes:
//      Use this method before editing a contained tree.
//
//          ElementCollection *pCollection = pSomeNode->Disconnect();
//
//          .. Now safely add, remove, or resize the contained tree ..
//          // E.g.,
//          ElementNode *pNodeInTree = pSomeNode->Remove();
//
//          Connect(pCollection, pNodeInTree);
//
//  This method is necessary because methods like Remove do not know how to
//  deal with containers.  In derived classes it also provides a pattern
//  for efficient (batched) updates of nested containers.
//
//---------------------------------------------------------------------------
SplayTreeCollection *SplayTreeNode::DisconnectIndexed(_In_ XUINT32 index)
{
    Splay(index);
    return DisconnectNoSplay(index);
}

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeNode::ConnectIndexed
//
//  Synopsis:
//      Attaches a tree to its collection.
//
//  Notes:
//      See SplayTreeNode::Disconnect comments for usage pattern/details.
//
//---------------------------------------------------------------------------
void SplayTreeNode::ConnectIndexed(
    _In_ XUINT32 index,
    _In_opt_ SplayTreeCollection *pContainingCollection, 
    _In_opt_ SplayTreeNode *pNode
    )
{
    if (NULL == pContainingCollection)
        return;

    if (pNode != NULL)
    {
        pNode->Splay(index);
        pNode->SetCollectionIndexed(index, pContainingCollection);
    }

    pContainingCollection->SetRootNodeIndexed(index, pNode);
}

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeNode::GetParentAsContainingCollectionIndexed
//
//  Returns:
//      If this node is the root node of its collection, returns the collection.
//      Otherwise returns NULL.
//
//---------------------------------------------------------------------------
SplayTreeCollection *SplayTreeNode::GetParentAsContainingCollectionIndexed(_In_ XUINT32 index) const
{
    SplayTreeCollection *pCollection = NULL;
    SplayTreeBase *pParent = GetParentRawIndexed(index);

    if (pParent != NULL)
    {
        pCollection = pParent->AsSplayTreeCollection();

        if (pCollection != NULL && pCollection->GetRootNodeIndexed(index) != this)
        {
            pCollection = NULL;
        }
    }

    return pCollection;
}

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeNode::GetParentIndexed
//
//  Synopsis:
//      Gets the parent of this node as a SplayTreeNode.
//
//---------------------------------------------------------------------------
SplayTreeNode *SplayTreeNode::GetParentIndexed(_In_ XUINT32 index) const
{
    SplayTreeNode *pParent = NULL;
    SplayTreeCollection *pContainingCollection = GetParentAsContainingCollectionIndexed(index);

    if (NULL == pContainingCollection)
    {
        pParent = static_cast<SplayTreeNode *>(GetParentRawIndexed(index));
    }

    return pParent;
}

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeNode::Splay
//
//  Synopsis:
//      Rotates this node to the top of its tree, while preserving the
//      relative order of all other nodes in the tree.
//
//  Notes:
//      On exit this node will be the tree root.
//
//---------------------------------------------------------------------------
void SplayTreeNode::Splay(_In_ XUINT32 index)
{
    SplayTreeNode *pNode = this;

    while (true)
    {
        SplayTreeNode *pParent = pNode->GetParentIndexed(index);

        if (NULL == pParent)
            break;

        SplayTreeNode *pGrandParent = pParent->GetParentIndexed(index);
        bool isLeftChild = (pNode == pParent->GetLeftChildIndexed(index));

        if (NULL == pGrandParent)
        {
            // ZIG: Parent is the local root.
            //
            //      Y             X         
            //     / \           / \        
            //    X   c   ==>   a   Y  
            //   / \               / \    
            //  a   b             b   c  
            //
            if (isLeftChild)
            {
                pParent->RotateRight(index);
            }
            else
            {
                pParent->RotateLeft(index);
            }
            break;
        }

        bool isParentLeftChild = (pParent == pGrandParent->GetLeftChildIndexed(index));

        if (isLeftChild == isParentLeftChild)
        {
            // ZIG-ZIG: node and parent are both left/right children.
            //
            //        |             |
            //        Z             X
            //       / \           / \
            //      Y   d         a   Y
            //     / \      ==>      / \
            //    X   c             b   Z
            //   / \                   / \
            //  a   b                 c   d
            //
            if (isLeftChild)
            {
                pGrandParent->RotateRight(index);
                pParent->RotateRight(index);
            }
            else
            {
                pGrandParent->RotateLeft(index);
                pParent->RotateLeft(index);
            }
        }
        else
        {
            // ZIG-ZAG: node is left/right child and parent is right/left child.
            //
            //      |               |
            //      Z               X
            //     / \            /   \
            //    Y   d          Y     Z
            //   / \      ==>   / \   / \
            //  a   X          a   b c   d 
            //     / \
            //    b   c
            //
            if (isLeftChild)
            {
                pParent->RotateRight(index);
                pGrandParent->RotateLeft(index);
            }
            else
            {
                pParent->RotateLeft(index);
                pGrandParent->RotateRight(index);
            }
        }
    }

    ASSERT(NULL == GetParentIndexed(index));
}

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeNode::OnNewLeftChild
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
void SplayTreeNode::OnNewLeftChild(_In_ XUINT32 index)
{
}

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeNode::OnChildRotatedLeft
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
void SplayTreeNode::OnChildRotatedLeft(_In_ XUINT32 index)
{
}

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeNode::OnRotatedRight
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
void SplayTreeNode::OnRotatedRight(_In_ XUINT32 index)
{
}

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeNode::OnRemoved
//
//  Synopsis:
//      Called after this node is removed from its tree.
//
//---------------------------------------------------------------------------
void SplayTreeNode::OnRemoved(_In_ XUINT32 index)
{
}

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeNode::GetFirstNodeNoSplay
//
//  Returns:
//      The lowest ordered node in the tree rooted by this node.
//
//  Notes:
//      The returned node is NOT splayed to the root.
//
//---------------------------------------------------------------------------
SplayTreeNode *SplayTreeNode::GetFirstNodeNoSplay()
{
    SplayTreeNode *pNode = this;

    for (;;)
    {
        SplayTreeNode *pLeftChild = pNode->GetLeftChild();

        if (NULL == pLeftChild)
            break;

        pNode = pLeftChild;
    }

    return pNode;
}


//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunNode::DisconnectNoSplay
//
//  Synopsis:
//      If the node is the root node of its collection, disconnects the
//      node's tree from its collection.  Otherwise, does nothing.
//
//  Returns:
//      The containing collection, or NULL if this node is not a root
//      contained node.
//
//  Notes:
//      This node is NOT splayed.
//
//---------------------------------------------------------------------------
SplayTreeCollection *SplayTreeNode::DisconnectNoSplay(_In_ XUINT32 index)
{
    SplayTreeCollection *pContainingCollection = GetParentAsContainingCollectionIndexed(index);

    if (pContainingCollection != NULL)
    {
        ASSERT(pContainingCollection->GetRootNodeIndexed(index) == this);
        pContainingCollection->SetRootNodeIndexed(index, NULL);
        SetCollectionIndexed(index, NULL);
    }

    return pContainingCollection;
}

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeNode::RotateLeft
//
//  Synopsis:
//      Moves a child node up one level in its tree, while preserving the
//      relative order of all other nodes in the tree.
//
//  Notes:
//
//      this == X below:
//
//            |              |
//            X              Y
//           / \            / \
//          a   Y    ==>   X   c
//             / \        / \
//            b   c      a   b
//
//---------------------------------------------------------------------------
void SplayTreeNode::RotateLeft(_In_ XUINT32 index)
{
    SplayTreeCollection *pContainingCollection = DisconnectNoSplay(index);

    ASSERT(GetRightChildIndexed(index) != NULL);

    SplayTreeNode *pRightChild = GetRightChildIndexed(index);
    SplayTreeNode *pRightChildLeftChild = pRightChild->GetLeftChildIndexed(index);
    
    SetRightChildIndexed(index, pRightChildLeftChild);
    if (pRightChildLeftChild != NULL)
    {
        pRightChildLeftChild->SetParentIndexed(index, this);
    }

    SplayTreeNode *pParent = GetParentIndexed(index);
    pRightChild->SetParentIndexed(index, pParent);
    if (pParent != NULL)
    {
        if (pParent->GetLeftChildIndexed(index) == this)
        {
            pParent->SetLeftChildIndexed(index, pRightChild);
        }
        else
        {
            pParent->SetRightChildIndexed(index, pRightChild);
        }
    }

    pRightChild->SetLeftChildIndexed(index, this);
    pParent = pRightChild;
    SetParentIndexed(index, pParent);

    pParent->OnChildRotatedLeft(index);

    if (pContainingCollection != NULL)
    {
        ASSERT(pParent != NULL);
        pContainingCollection->SetRootNodeIndexed(index, pParent);
        pParent->SetCollectionIndexed(index, pContainingCollection);
    }
}

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeNode::RotateRight
//
//  Synopsis:
//      Moves a child node up one level in its tree, while preserving the
//      relative order of all other nodes in the tree.
//
//  Notes:
//
//      this == Y below:
//
//            |             |         
//            Y             X         
//           / \           / \        
//          X   c   ==>   a   Y  
//         / \               / \    
//        a   b             b   c  
//
//---------------------------------------------------------------------------
void SplayTreeNode::RotateRight(_In_ XUINT32 index)
{
    SplayTreeCollection *pContainingCollection = DisconnectNoSplay(index);

    ASSERT(GetLeftChildIndexed(index) != NULL);

    SplayTreeNode *pLeftChild = GetLeftChildIndexed(index);
    SplayTreeNode *pLeftChildRightChild = pLeftChild->GetRightChildIndexed(index);
    
    SetLeftChildIndexed(index, pLeftChildRightChild);
    if (pLeftChildRightChild != NULL)
    {
        pLeftChildRightChild->SetParentIndexed(index, this);
    }

    SplayTreeNode *pParent = GetParentIndexed(index);
    pLeftChild->SetParentIndexed(index, pParent);
    if (pParent != NULL)
    {
        if (pParent->GetLeftChildIndexed(index) == this)
        {
            pParent->SetLeftChildIndexed(index, pLeftChild);
        }
        else
        {
            pParent->SetRightChildIndexed(index, pLeftChild);
        }
    }

    pLeftChild->SetRightChildIndexed(index, this);
    pParent = pLeftChild;
    SetParentIndexed(index, pParent);

    OnRotatedRight(index);

    if (pContainingCollection != NULL)
    {
        ASSERT(pParent != NULL);
        pContainingCollection->SetRootNodeIndexed(index, pParent);
        pParent->SetCollectionIndexed(index, pContainingCollection);
    }
}

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeNode::ConnectLeftChild
//
//  Synopsis:
//      Creates a new parent/child relationship with this node.
//
//---------------------------------------------------------------------------
void SplayTreeNode::ConnectLeftChild(
    _In_ XUINT32 index,
    _In_ SplayTreeNode *pMaxLeftChild
    )
{
    ASSERT(NULL == pMaxLeftChild || NULL == pMaxLeftChild->GetRightChildIndexed(index));
    ASSERT(NULL == GetLeftChildIndexed(index));

    Splay(index);

    SetLeftChildIndexed(index, pMaxLeftChild);
    if (pMaxLeftChild != NULL)
    {
        ASSERT(pMaxLeftChild->GetParentIndexed(index) == NULL);
        pMaxLeftChild->SetParentIndexed(index, this);
    }

    OnNewLeftChild(index);
}

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeNode::DisconnectLeftChild
//
//  Synopsis:
//      Ends a parent/child relationship.
//
//  Returns:
//      The old left child.
//
//---------------------------------------------------------------------------
SplayTreeNode *SplayTreeNode::DisconnectLeftChild(_In_ XUINT32 index)
{
    SplayTreeNode *pLeftChild = GetLeftChildIndexed(index);

    if (pLeftChild != NULL)
    {
        pLeftChild->SetParentIndexed(index, NULL);
        SetLeftChildIndexed(index, NULL);
    }

    return pLeftChild;
}

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeNode::ConnectRightChild
//
//  Synopsis:
//      Creates a new parent/child relationship with this node.
//
//---------------------------------------------------------------------------
void SplayTreeNode::ConnectRightChild(
    _In_ XUINT32 index,
    _In_ SplayTreeNode *pRightChild
    )
{
    ASSERT(NULL == GetRightChildIndexed(index));

    SetRightChildIndexed(index, pRightChild);
    if (pRightChild != NULL)
    {
        ASSERT(pRightChild->GetParentIndexed(index) == NULL);
        pRightChild->SetParentIndexed(index, this);
    }
}

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeNode::DisconnectRightChild
//
//  Synopsis:
//      Ends a parent/child relationship.
//
//  Returns:
//      The old right child.
//
//---------------------------------------------------------------------------
SplayTreeNode *SplayTreeNode::DisconnectRightChild(_In_ XUINT32 index)
{
    SplayTreeNode *pRightChild = GetRightChildIndexed(index);

    if (pRightChild != NULL)
    {
        pRightChild->SetParentIndexed(index, NULL);
        SetRightChildIndexed(index, NULL);
    }

    return pRightChild;
}

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeNode::GetFirstNode
//
//  Returns:
//      The lowest ordered node in the tree rooted by this node.
//
//  Notes:
//      The returned node is always a root node.
//
//---------------------------------------------------------------------------
SplayTreeNode *SplayTreeNode::GetFirstNode()
{
    SplayTreeNode *pNode = GetFirstNodeNoSplay();
    pNode->Splay();

    return pNode;
}

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeNode::GetLastNode
//
//  Returns:
//      The highest ordered node in the tree rooted by this node.
//
//  Notes:
//      The returned node is always a root node.
//
//---------------------------------------------------------------------------
SplayTreeNode *SplayTreeNode::GetLastNode(_In_ XUINT32 index)
{
    SplayTreeNode *pNode = this;

    for (;;)
    {
        SplayTreeNode *pRightChild = pNode->GetRightChildIndexed(index);

        if (NULL == pRightChild)
            break;

        pNode = pRightChild;
    }

    pNode->Splay(index);

    return pNode;
}

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeNode::Insert
//
//  Synopsis:
//      Inserts a tree of nodes before or after this node.
//
//  Notes:
//      The last inserted node becomes the root.
//
//---------------------------------------------------------------------------
void SplayTreeNode::Insert(
    _In_ XUINT32 index,
    _In_ SplayTreeNode *pFirstNode, 
    _In_ SplayTreeNode *pLastNode, 
    _In_ bool before
    )
{
    ASSERT(NULL == pFirstNode->GetLeftChildIndexed(index));
    ASSERT(NULL == pLastNode->GetRightChildIndexed(index));

    SplayTreeNode *pPreviousNode = before ? GetPreviousNodeIndexed(index) : this;
    SplayTreeNode *pNextNode;

    if (pPreviousNode != NULL)
    {
        pNextNode = pPreviousNode->SplitIndexed(index);
    }
    else
    {
        pNextNode = this;
    }

    pFirstNode->ConnectLeftChild(index, pPreviousNode);
    pLastNode->ConnectRightChild(index, pNextNode);
}

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeNode::SetCollectionIndexed
//
//  Returns:
//      Sets the owner of this node's tree.
//
//---------------------------------------------------------------------------
void SplayTreeNode::SetCollectionIndexed(
    _In_ XUINT32 index,
    _In_ SplayTreeCollection *pCollection)
{
    SetParentIndexed(index, pCollection);
}
