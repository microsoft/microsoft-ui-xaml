// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SplayTreeNode.h"

namespace RichTextServices
{
    namespace Internal
    {
        //---------------------------------------------------------------------------
        //
        //  SplayTreeCollection
        //
        //  Holds a collection of SplayTreeNodes.
        //
        //  A collection may be accessed with an index.  This is used when nodes are
        //  sorted in multiple dimensions, each dimension referenced with a unique
        //  index.
        //
        //  Methods that do not take an index parameter use the implicit index 0.
        //  
        //---------------------------------------------------------------------------
        class SplayTreeCollection : public SplayTreeBase
        {
        public:

            // Initializes a new SplayTreeCollection instance.
            SplayTreeCollection();

            // Destructor.
            virtual ~SplayTreeCollection();

            // Gets the first node in the collection.
            SplayTreeNode *GetFirstContainedNode();

            // Gets the last node in the collection.
            SplayTreeNode *GetLastContainedNode();

            // Detaches any contained nodes from this collection.
            void DisconnectNodes();

            // Attaches nodes to this collection.
            void ConnectNodes(_In_ SplayTreeNode *pRootNode);

            //---------------------------------------------------------------------------
            //
            //  Member:
            //      GetRootNodeIndexed
            //
            //  Returns:
            //      The root node of the tree of nodes held in this collection.
            //
            //---------------------------------------------------------------------------
            virtual SplayTreeNode *GetRootNodeIndexed(
                _In_ XUINT32 index
                    // Specifies which dimension to access.
                ) const = 0;

            // Gets the root node of the tree of nodes held in this collection.
            SplayTreeNode *GetRootNode() const;

            //---------------------------------------------------------------------------
            //
            //  Member:
            //      SetRootNodeIndexed
            //
            //  Synopsis:
            //      Sets the root node of the tree of nodes held in this collection.
            //
            //---------------------------------------------------------------------------
            virtual void SetRootNodeIndexed(
                _In_ XUINT32 index,
                    // Specifies which dimension to access.
                _In_ SplayTreeNode *pRootNode
                    // The new root node.
                ) = 0;

            // Sets the root root of the tree of nodes in this collection.
            void SetRootNode(_In_ SplayTreeNode *pRootNode);
        };

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      SplayTreeCollection::SplayTreeCollection
        //
        //  Synopsis:
        //      Initializes a new SplayTreeCollection instance.
        //
        //---------------------------------------------------------------------------
        inline
        SplayTreeCollection::SplayTreeCollection()
        {
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      SplayTreeCollection::GetFirstContainedNode
        //
        //  Returns:
        //      First node in the collection.
        //
        //---------------------------------------------------------------------------
        inline
        SplayTreeNode *SplayTreeCollection::GetFirstContainedNode()
        { 
            SplayTreeNode *pRootNode = GetRootNode();
            return (pRootNode == NULL) ? NULL : pRootNode->GetFirstSibling();
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      SplayTreeCollection::GetLastContainedNode
        //
        //  Returns:
        //      Last node in the collection.
        //
        //---------------------------------------------------------------------------
        inline
        SplayTreeNode *SplayTreeCollection::GetLastContainedNode()
        {
            SplayTreeNode *pRootNode = GetRootNode();
            return (pRootNode == NULL) ? NULL : pRootNode->GetLastSibling();
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      SplayTreeCollection::ConnectNodes
        //
        //  Synopsis:
        //      Called after any editing operation that adds or removes contained nodes.
        //
        //  Notes:
        //      See SplayTreeCollection::DisconnectNodes comments for usage pattern/details.
        //
        //---------------------------------------------------------------------------
        inline 
        void SplayTreeCollection::ConnectNodes(_In_ SplayTreeNode *pNode)
        {
            SplayTreeNode::Connect(this, pNode);
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      SplayTreeCollection::GetRootNode
        //
        //  Returns:
        //      The root node of the tree of nodes held in this collection.
        //
        //---------------------------------------------------------------------------
        inline
        SplayTreeNode *SplayTreeCollection::GetRootNode() const
        {
            return GetRootNodeIndexed(0 /* index */);
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      SplayTreeCollection::SetRootNode
        //
        //  Synopsis:
        //      Sets the root of the tree of nodes in this collection.
        //
        //---------------------------------------------------------------------------
        inline
        void SplayTreeCollection::SetRootNode(_In_ SplayTreeNode *pRootNode)
        {
            SetRootNodeIndexed(0 /* index */, pRootNode);
        }
    }
}
