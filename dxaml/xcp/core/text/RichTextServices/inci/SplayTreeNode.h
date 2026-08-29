// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SplayTreeBase.h"

namespace RichTextServices
{
    namespace Internal
    {
        class SplayTreeCollection;

         //---------------------------------------------------------------------------
        //
        //  SplayTreeNode
        //
        //  A node in a splay tree, hosted by a SplayTreeCollection.
        //
        //  A "splay tree" is a binary tree which balances itself dynamically as nodes
        //  are accessed.  Splay trees are accessed or modified in O(log n) time 
        //  generally but achieve O(1) when successive operations have good locality.
        //
        //  A node may exist in multiple trees simulaneously, thus methods below often
        //  take an index parameter which specifices which tree (or dimension) to
        //  access.  For example, nodes in the layout tree are sorted by u/character
        //  and v dimensions simultaneously.  Methods which do not take index parameters
        //  implicitly use index = 0.
        //
        //---------------------------------------------------------------------------
        class SplayTreeNode : public SplayTreeBase
        {
        public:

            // Initializes a new SplayTreeNode instance.
            SplayTreeNode();

            // Destructor.
            virtual ~SplayTreeNode();

            // Inserts a node before this node.
            void InsertBeforeIndexed(
                _In_ XUINT32 index,
                _In_ SplayTreeNode *pNode
                );
            void InsertBefore(_In_ SplayTreeNode *pNode);

            // Inserts a tree of nodes before this node.
            void InsertBeforeIndexed(
                _In_ XUINT32 index,
                _In_ SplayTreeNode *pFirstNode,
                _In_ SplayTreeNode *pLastNode
                );
            void InsertBefore(
                _In_ SplayTreeNode *pFirstNode,
                _In_ SplayTreeNode *pLastNode
                );

            // Inserts a node after this node.
            void InsertAfterIndexed(
                _In_ XUINT32 index,
                _In_ SplayTreeNode *pNode
                );
            void InsertAfter(_In_ SplayTreeNode *pNode);

            // Inserts a tree of nodes after this node.
            void InsertAfterIndexed(
                _In_ XUINT32 index,
                _In_ SplayTreeNode *pFirstNode, 
                _In_ SplayTreeNode *pLastNode
                );
            void InsertAfter(
                _In_ SplayTreeNode *pFirstNode, 
                _In_ SplayTreeNode *pLastNode
                );

            // Removes this node from its tree.
            SplayTreeNode *RemoveIndexed(_In_ XUINT32 index);
            SplayTreeNode *Remove();

            // Combines two trees.
            static 
            SplayTreeNode *JoinIndexed(
                _In_ XUINT32 index,
                _In_opt_ SplayTreeNode *pLeftNode, 
                _In_opt_ SplayTreeNode *pRightNode
                );
            static 
            SplayTreeNode *Join(
                _In_opt_ SplayTreeNode *pLeftNode, 
                _In_opt_ SplayTreeNode *pRightNode
                );

            // Combines two trees, returning the first and last nodes of the new tree.
            static
            void Concatenate(
                _In_opt_ SplayTreeNode *pFirst1,
                _In_opt_ SplayTreeNode *pLast1,
                _In_opt_ SplayTreeNode *pFirst2,
                _In_opt_ SplayTreeNode *pLast2,
                _Out_ SplayTreeNode **ppFirst,
                _Out_ SplayTreeNode **ppLast
                );

            // Divides a node's tree into two trees.
            SplayTreeNode *SplitIndexed(_In_ XUINT32 index);
            SplayTreeNode *Split();

            // Gets the node's collection, if any.
            SplayTreeCollection *GetContainingCollection();

            // Returns the preceding node, or NULL if there is no preceding node.
            SplayTreeNode *GetPreviousNodeIndexed(_In_ XUINT32 index);
            SplayTreeNode *GetPreviousNode();

            // Returns the following node, or NULL if there is no following node.
            SplayTreeNode *GetNextNode();

            // Gets the first node in the tree containing this node.
            SplayTreeNode *GetFirstSibling();

            // Gets the last node in the tree containing this node.
            SplayTreeNode *GetLastSiblingIndexed(_In_ XUINT32 index);
            SplayTreeNode *GetLastSibling();

            // Returns true iff this node is not part of any tree or collection.
            bool HasNoReferencesOrReferersIndexed(_In_ XUINT32 index) const;
            bool HasNoReferencesOrReferers() const;

            // Returns true iff this node has preceding or following nodes.
            bool HasSiblings() const;

            // Detaches the tree containing this node from its collection.
            SplayTreeCollection *DisconnectIndexed(_In_ XUINT32 index);
            SplayTreeCollection *Disconnect();

            // Attaches a node's tree to a collection.
            static
            void ConnectIndexed(
                _In_ XUINT32 index,
                _In_opt_ SplayTreeCollection *pContainingCollection, 
                _In_opt_ SplayTreeNode *pNode
                );
            static
            void Connect(
                _In_opt_ SplayTreeCollection *pContainingCollection, 
                _In_opt_ SplayTreeNode *pNode
                );

        protected:

            // If this node is the root node of its collection, returns the collection.
            // Otherwise returns NULL.
            SplayTreeCollection *GetParentAsContainingCollectionIndexed(_In_ XUINT32 index) const;
            SplayTreeCollection *GetParentAsContainingCollection() const;

            //---------------------------------------------------------------------------
            //
            //  Member:
            //      GetParentRawIndexed
            //
            //  Synopsis:
            //      Gets the parent of this node, either a containing collection or another
            //      sibling node.
            //
            //---------------------------------------------------------------------------
            virtual SplayTreeBase *GetParentRawIndexed(_In_ XUINT32 index) const = 0;
            SplayTreeBase *GetParentRaw() const;

            // Gets the parent of this node as a SplayTreeNode.
            SplayTreeNode *GetParentIndexed(_In_ XUINT32 index) const;
            SplayTreeNode *GetParent() const;

            //---------------------------------------------------------------------------
            //
            //  Member:
            //      SetParentIndexed
            //
            //  Synopsis:
            //      Sets the parent of this node.
            //
            //---------------------------------------------------------------------------
            virtual void SetParentIndexed(
                _In_ XUINT32 index,
                _In_ SplayTreeBase *pParent
                ) = 0;
            void SetParent(_In_ SplayTreeBase *pParent);

            //---------------------------------------------------------------------------
            //
            //  Member:
            //      GetLeftChildIndexed
            //
            //  Synopsis:
            //      Gets the left child of this node.
            //
            //---------------------------------------------------------------------------
            virtual SplayTreeNode *GetLeftChildIndexed(_In_ XUINT32 index) const = 0;
            SplayTreeNode *GetLeftChild() const;

            //---------------------------------------------------------------------------
            //
            //  Member:
            //      SetLeftChildIndexed
            //
            //  Synopsis:
            //      Sets the left child of this node.
            //
            //---------------------------------------------------------------------------
            virtual void SetLeftChildIndexed(
                _In_ XUINT32 index, 
                _In_ SplayTreeNode *pLeftChild
                ) = 0;
            void SetLeftChild(_In_ SplayTreeNode *pLeftChild);

            //---------------------------------------------------------------------------
            //
            //  Member:
            //      GetRightChildIndexed
            //
            //  Synopsis:
            //      Gets the right child of this node.
            //
            //---------------------------------------------------------------------------
            virtual SplayTreeNode *GetRightChildIndexed(_In_ XUINT32 index) const = 0;
            SplayTreeNode *GetRightChild() const;

            //---------------------------------------------------------------------------
            //
            //  Member:
            //      SetRightChildIndexed
            //
            //  Synopsis:
            //      Sets the right child of this node.
            //
            //---------------------------------------------------------------------------
            virtual void SetRightChildIndexed(
                _In_ XUINT32 index, 
                _In_ SplayTreeNode *pRightChild
                ) = 0;
            void SetRightChild(_In_ SplayTreeNode *pRightChild);

            // Rotates this node to the top of its tree while preserving the relative order of all 
            // other nodes in the tree.
            void Splay();
            void Splay(_In_ XUINT32 index);

            // Called after this node receives a new left child.
            virtual void OnNewLeftChild(_In_ XUINT32 index);

            // Called after this node's left child is rotated left.
            virtual void OnChildRotatedLeft(_In_ XUINT32 index);

            // Called after this node is rotated right.
            virtual void OnRotatedRight(_In_ XUINT32 index);

            // Called after this node is removed from its tree.
            virtual void OnRemoved(_In_ XUINT32 index);

            // Gets the lowest ordered node in the tree rooted by this node, without splaying the
            // node.
            SplayTreeNode *GetFirstNodeNoSplay();

            // Sets the owner of this node's tree.
            void SetCollectionIndexed(
                _In_ XUINT32 index, 
                _In_ SplayTreeCollection *pCollection
                );
            void SetCollection(_In_ SplayTreeCollection *pCollection);

        private:

            // Moves this node up one level in its tree, while preserving the relative order of all 
            // other nodes in the tree.
            void RotateRight(_In_ XUINT32 index);
            void RotateLeft(_In_ XUINT32 index);

            // Attaches a new left child.
            void ConnectLeftChild(
                _In_ XUINT32 index,
                _In_ SplayTreeNode *pMaxLeftChild
                );

            // Detaches the left child.
            SplayTreeNode *DisconnectLeftChild(_In_ XUINT32 index);

            // Attaches a new right child.
            void ConnectRightChild(
                _In_ XUINT32 index,
                _In_ SplayTreeNode *pRightChild
                );

            // Detaches the right child.
            SplayTreeNode *DisconnectRightChild(_In_ XUINT32 index);

            // Returns the lowest ordered node in the tree rooted by this node.
            SplayTreeNode *GetFirstNode();

            // Returns the highest ordered node in the tree rooted by this node.
            SplayTreeNode *GetLastNode(_In_ XUINT32 index);

            // Inserts a tree of nodes before or after this node.
            void Insert(
                _In_ XUINT32 index,
                _In_ SplayTreeNode *pFirstNode, 
                _In_ SplayTreeNode *pLastNode, 
                _In_ bool before
                );

            // If the node is the root node of its collection, disconnects the node's tree from its
            // collection.  Otherwise, does nothing.
            SplayTreeCollection *DisconnectNoSplay(_In_ XUINT32 index);
        };

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      SplayTreeNode::InsertBefore
        //
        //  Synopsis:
        //      Inserts a node before this node.
        //
        //---------------------------------------------------------------------------
        inline
        void SplayTreeNode::InsertBeforeIndexed(
            _In_ XUINT32 index,
            _In_ SplayTreeNode *pNode
            )
        { 
            ASSERT(pNode->HasNoReferencesOrReferersIndexed(index));
            Insert(index, pNode, pNode, true /* before */); 
        }
        inline
        void SplayTreeNode::InsertBefore(_In_ SplayTreeNode *pNode)
        { 
            return InsertBeforeIndexed(0 /* index */, pNode);
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      SplayTreeNode::InsertBefore
        //
        //  Synopsis:
        //      Inserts a tree of nodes before this node.
        //
        //---------------------------------------------------------------------------
        inline
        void SplayTreeNode::InsertBeforeIndexed(
            _In_ XUINT32 index,
            _In_ SplayTreeNode *pFirstNode, 
            _In_ SplayTreeNode *pLastNode
            )
        { 
            Insert(index, pFirstNode, pLastNode, true /* before */);
        }
        inline
        void SplayTreeNode::InsertBefore(
            _In_ SplayTreeNode *pFirstNode, 
            _In_ SplayTreeNode *pLastNode
            )
        { 
            return InsertBeforeIndexed(0 /* index */, pFirstNode, pLastNode);
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      SplayTreeNode::InsertAfter
        //
        //  Synopsis:
        //      Inserts a node after this node.
        //
        //---------------------------------------------------------------------------
        inline
        void SplayTreeNode::InsertAfterIndexed(
            _In_ XUINT32 index,
            _In_ SplayTreeNode *pNode
            )
        {
            ASSERT(pNode->HasNoReferencesOrReferersIndexed(index));
            Insert(index, pNode, pNode, false /* before */);
        }
        inline
        void SplayTreeNode::InsertAfter(_In_ SplayTreeNode *pNode)
        {
            return InsertAfterIndexed(0 /* index */, pNode);
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      SplayTreeNode::InsertAfter
        //
        //  Synopsis:
        //      Inserts a tree of nodes after this node.
        //
        //---------------------------------------------------------------------------
        inline
        void SplayTreeNode::InsertAfterIndexed(
            _In_ XUINT32 index,
            _In_ SplayTreeNode *pFirstNode, 
            _In_ SplayTreeNode *pLastNode
            )
        { 
            Insert(index, pFirstNode, pLastNode, false /* before */);
        }
        inline
        void SplayTreeNode::InsertAfter(
            _In_ SplayTreeNode *pFirstNode, 
            _In_ SplayTreeNode *pLastNode
            )
        {
            return InsertAfterIndexed(0 /* index */, pFirstNode, pLastNode);
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      SplayTreeNode::Remove
        //
        //  Synopsis:
        //      Removes this node from its tree.
        //
        //---------------------------------------------------------------------------
        inline
        SplayTreeNode *SplayTreeNode::Remove()
        {
            return RemoveIndexed(0 /* index */);
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      SplayTreeNode::Join
        //
        //  Synopsis:
        //      Combines two trees.
        //
        //---------------------------------------------------------------------------
        inline
        SplayTreeNode *SplayTreeNode::Join(
            _In_opt_ SplayTreeNode *pLeftNode, 
            _In_opt_ SplayTreeNode *pRightNode
            )
        {
            return JoinIndexed(0 /* index */, pLeftNode, pRightNode);
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      SplayTreeNode::Split
        //
        //  Synopsis:
        //      Divides a node's tree into two trees.
        //
        //---------------------------------------------------------------------------
        inline
        SplayTreeNode *SplayTreeNode::Split()
        {
            return SplitIndexed(0 /* index */);
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      SplayTreeNode::GetPreviousNode
        //
        //  Synopsis:
        //      Returns the preceding node, or NULL if there is no preceding node.
        //
        //---------------------------------------------------------------------------
        inline
        SplayTreeNode *SplayTreeNode::GetPreviousNode()
        {
            return GetPreviousNodeIndexed(0 /* index */);
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      SplayTreeNode::GetLastSibling
        //
        //  Synopsis:
        //      Gets the last node in the tree containing this node.
        //
        //---------------------------------------------------------------------------
        inline
        SplayTreeNode *SplayTreeNode::GetLastSibling()
        {
            return GetLastSiblingIndexed(0 /* index */);
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      SplayTreeNode::HasNoReferencesOrReferersIndexed
        //
        //  Returns:
        //      true iff this node is not part of any tree or collection.
        //
        //---------------------------------------------------------------------------
        inline
        bool SplayTreeNode::HasNoReferencesOrReferersIndexed(_In_ XUINT32 index) const
        {
            return GetParentRawIndexed(index) == NULL && 
                   GetLeftChildIndexed(index) == NULL && 
                   GetRightChildIndexed(index) == NULL;
        }
        inline
        bool SplayTreeNode::HasNoReferencesOrReferers() const
        {
            return HasNoReferencesOrReferersIndexed(0 /* index */);
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      SplayTreeNode::HasSiblings
        //
        //  Returns:
        //      true iff this node has preceding or following nodes.
        //
        //---------------------------------------------------------------------------
        inline
        bool SplayTreeNode::HasSiblings() const
        {
            return (GetLeftChild() != NULL || GetRightChild() != NULL || GetParent() != NULL);
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      SplayTreeNode::GetParentAsContainingCollection
        //
        //  Returns:
        //      If this node is the root node of its collection, returns the collection.
        //      Otherwise returns NULL.
        //
        //---------------------------------------------------------------------------
        inline 
        SplayTreeCollection *SplayTreeNode::GetParentAsContainingCollection() const
        {
            return GetParentAsContainingCollectionIndexed(0 /* index */);
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      SplayTreeNode::GetParentRaw
        //
        //  Returns:
        //      The parent of this node.
        //
        //---------------------------------------------------------------------------
        inline
        SplayTreeBase *SplayTreeNode::GetParentRaw() const
        {
            return GetParentRawIndexed(0 /* index */);
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      SplayTreeNode::GetParent
        //
        //  Returns:
        //      The parent of this node as a SplayTreeNode.
        //
        //---------------------------------------------------------------------------
        inline
        SplayTreeNode *SplayTreeNode::GetParent() const
        {
            return GetParentIndexed(0 /* index */);
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      SplayTreeNode::SetParent
        //
        //  Synopsis:
        //      Sets the parent of this node.
        //
        //---------------------------------------------------------------------------
        inline
        void SplayTreeNode::SetParent(_In_ SplayTreeBase *pParent)
        {
            SetParentIndexed(0 /* index */, pParent);
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      SplayTreeNode::GetLeftChild
        //
        //  Returns:
        //      The left child of this node.
        //
        //---------------------------------------------------------------------------
        inline
        SplayTreeNode *SplayTreeNode::GetLeftChild() const
        {
            return GetLeftChildIndexed(0 /* index */);
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      SplayTreeNode::SetLeftChild
        //
        //  Synopsis:
        //      Sets the left child of this node.
        //
        //---------------------------------------------------------------------------
        inline
        void SplayTreeNode::SetLeftChild(_In_ SplayTreeNode *pLeftChild)
        {
            SetLeftChildIndexed(0 /* index */, pLeftChild);
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      SplayTreeNode::GetRightChild
        //
        //  Returns:
        //      The right child of this node.
        //
        //---------------------------------------------------------------------------
        inline
        SplayTreeNode *SplayTreeNode::GetRightChild() const
        {
            return GetRightChildIndexed(0 /* index */);
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      SplayTreeNode::SetRightChild
        //
        //  Synopsis:
        //      Sets the right child of this node.
        //
        //---------------------------------------------------------------------------
        inline
        void SplayTreeNode::SetRightChild(_In_ SplayTreeNode *pRightChild)
        {
            SetRightChildIndexed(0 /* index */, pRightChild);
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      SplayTreeNode::Splay
        //
        //  Synopsis:
        //      Rotates this node to the top of its tree while preserving the relative
        //      order of all other nodes in the tree.
        //
        //---------------------------------------------------------------------------
        inline
        void SplayTreeNode::Splay()
        {
            Splay(0 /* index */);
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      SplayTreeNode::SetCollection
        //
        //  Synopsis:
        //      Sets the owner of this node's tree.
        //
        //---------------------------------------------------------------------------
        inline
        void SplayTreeNode::SetCollection(_In_ SplayTreeCollection *pCollection)
        {
            SetCollectionIndexed(0 /* index */, pCollection);
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      SplayTreeNode::Disconnect
        //
        //  Synopsis:
        //      Detaches the tree containing this node from its collection.
        //
        //  Returns:
        //      The node's former containing collection.
        //
        //---------------------------------------------------------------------------
        inline
        SplayTreeCollection *SplayTreeNode::Disconnect()
        {
            return DisconnectIndexed(0 /* index */);
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      SplayTreeNode::Connect
        //
        //  Synopsis:
        //      Attaches a node's tree to a collection.
        //
        //---------------------------------------------------------------------------
        inline 
        void SplayTreeNode::Connect(
            _In_opt_ SplayTreeCollection *pContainingCollection, 
            _In_opt_ SplayTreeNode *pNode
            )
        {
            return ConnectIndexed(0 /* index */, pContainingCollection, pNode);
        }
    }
}

