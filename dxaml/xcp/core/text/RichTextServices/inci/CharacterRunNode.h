// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SplayTreeNode.h"

namespace RichTextServices
{
    namespace Internal
    {
        class CharacterRunCollection;

        //---------------------------------------------------------------------------
        //
        //  CharacterRunNode
        //
        //  A node sorted in character offset space.  Tracks the sum of all characters
        //  preceding the node (the offset of the node within its collection) and all
        //  characters covered by the node.
        //
        //  CharacterRunNodes live within a containing CharacterRunCollection.
        //
        //---------------------------------------------------------------------------
        class CharacterRunNode : public SplayTreeNode
        {
        public:

            //---------------------------------------------------------------------------
            //
            //  IDeleteTreeListener
            //
            //  Callback interface for DeleteTree method.
            //
            //---------------------------------------------------------------------------
            struct IDeleteTreeListener
            {
                //---------------------------------------------------------------------------
                //
                //  Member:
                //      IDeleteTreeListener::OnNodeRemoved
                //
                //  Synopsis:
                //      Called before a node is removed from a collection.
                //
                //---------------------------------------------------------------------------
                virtual void OnNodeRemoved(_In_ CharacterRunNode *pNode) = 0;
            };

            // Initializes a new CharacterRunNode instance.
            CharacterRunNode(_In_ XUINT32 charCount);

            // Destructor.
            ~CharacterRunNode() override;

            // Gets the character count of this node, including contained nodes if this
            // node is also a CharacterRunCollection.
            XUINT32 GetCount() const;

            // Sets the character count of this node.
            void SetCount(_In_ XUINT32 count);

            // Modifies the character count of this node.
            void AdjustCount(
                _In_ XINT32 delta
                    // Character count to add to existing value, negative values are legal.
                );

            // Called when this node is no longer referenced by its collection.
            virtual void ReleaseCollectionReference();

            // Returns the character offset of the start edge of this node, relative to its collection.
            XUINT32 GetSiblingOffset();

            // Returns the character offset of the end edge of this node, relative to its collection.
            XUINT32 GetSiblingEndOffset();

            // Returns the character offset of the start edge of this node.
            XUINT32 GetOffset();

            // Returns the length of the tree rooted by this CharacterRunNode.
            XUINT32 GetLengthOfTree();

            // Returns the sibling node covering a specified character.
            virtual CharacterRunNode *GetSiblingNodeAtOffset(
                _In_ XUINT32 offset,
                _Out_ XUINT32 *pNodeOffset
                );

            // Deletes every node in the tree containing node.
            static
            void DeleteTree(
                _In_opt_ CharacterRunNode *pNode,
                _In_opt_ IDeleteTreeListener *pListener
                );

            // Strongly typed SplayTreeNode wrappers.
            CharacterRunNode *Remove();
            CharacterRunCollection *GetContainingCollection();
            CharacterRunNode *GetNextNode();
            CharacterRunNode *GetFirstSibling();
            CharacterRunNode *GetLastSibling();

        protected:

            // Returns this node as a CharacterRunCollection, or NULL if this node is not a
            // CharacterRunCollection.
            virtual CharacterRunCollection *AsCharacterRunCollection();

            //-----------------------------------------------------------------------
            //
            // SplayTreeNode overrides.
            //
            //-----------------------------------------------------------------------
            SplayTreeBase *GetParentRawIndexed(_In_ XUINT32 index) const override;
            void SetParentIndexed(
                _In_ XUINT32 index,
                _In_ SplayTreeBase *pParent
                ) override;
            SplayTreeNode *GetLeftChildIndexed(_In_ XUINT32 index) const override;
            void SetLeftChildIndexed(
                _In_ XUINT32 index,
                _In_ SplayTreeNode *pLeftChild
                ) override;
            SplayTreeNode *GetRightChildIndexed(_In_ XUINT32 index) const override;
            void SetRightChildIndexed(
                _In_ XUINT32 index,
                _In_ SplayTreeNode *pRightChild
                ) override;
            void OnNewLeftChild(_In_ XUINT32 index) override;
            void OnChildRotatedLeft(_In_ XUINT32 index) override;
            void OnRotatedRight(_In_ XUINT32 index) override;
            void OnRemoved(_In_ XUINT32 index) override;

            // Returns the character count of the left child subtree.
            XUINT32 GetLeftCharCount() const;

            // Sets the character count of the left child subtree.
            void SetLeftCharCount(_In_ XUINT32 leftCharCount);

            // Strongly typed SplayTreeNode wrappers.
            CharacterRunNode *GetParent() const;
            CharacterRunNode *GetLeftChild() const;
            CharacterRunNode *GetRightChild() const;
            CharacterRunNode *GetFirstNodeNoSplay();
            CharacterRunCollection *GetParentAsContainingCollection() const;

        private:

            SplayTreeBase *m_pParent;
                // Parent node.  A CharacterRunNode (sibling) or CharacterRunCollection (container).

            CharacterRunNode *m_pLeftChild;
                // Left child.

            CharacterRunNode *m_pRightChild;
                // Right child.

            XUINT32 m_charCount;
                // Sum of all characters covered by this node.

            XUINT32 m_leftCharCount;
                // Sum of all characters in the left child subtree.
        };

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      CharacterRunNode::GetCount
        //
        //  Returns:
        //      The character count of this node, including contained nodes if this
        //      node is also a CharacterRunCollection.
        //
        //---------------------------------------------------------------------------
        inline
        XUINT32 CharacterRunNode::GetCount() const
        {
            return m_charCount;
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      CharacterRunNode::Remove
        //
        //  Synopsis:
        //      Strongly typed SplayTreeNode wrapper.
        //
        //---------------------------------------------------------------------------
        inline
        CharacterRunNode *CharacterRunNode::Remove()
        {
            return static_cast<CharacterRunNode *>(SplayTreeNode::Remove());
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      CharacterRunNode::GetNextNode
        //
        //  Synopsis:
        //      Strongly typed SplayTreeNode wrapper.
        //
        //---------------------------------------------------------------------------
        inline
        CharacterRunNode *CharacterRunNode::GetNextNode()
        {
            return static_cast<CharacterRunNode *>(SplayTreeNode::GetNextNode());
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      CharacterRunNode::GetFirstSibling
        //
        //  Synopsis:
        //      Strongly typed SplayTreeNode wrapper.
        //
        //---------------------------------------------------------------------------
        inline
        CharacterRunNode *CharacterRunNode::GetFirstSibling()
        {
            return static_cast<CharacterRunNode *>(SplayTreeNode::GetFirstSibling());
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      CharacterRunNode::GetLastSibling
        //
        //  Synopsis:
        //      Strongly typed SplayTreeNode wrapper.
        //
        //---------------------------------------------------------------------------
        inline
        CharacterRunNode *CharacterRunNode::GetLastSibling()
        {
            return static_cast<CharacterRunNode *>(SplayTreeNode::GetLastSibling());
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      CharacterRunNode::GetLeftCharCount
        //
        //  Returns:
        //      The character count of the left child subtree.
        //
        //---------------------------------------------------------------------------
        inline
        XUINT32 CharacterRunNode::GetLeftCharCount() const
        {
            return m_leftCharCount;
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      CharacterRunNode::SetLeftCharCount
        //
        //  Synopsis:
        //      Sets the character count of the left child subtree.
        //
        //---------------------------------------------------------------------------
        inline
        void CharacterRunNode::SetLeftCharCount(_In_ XUINT32 leftCharCount)
        {
            m_leftCharCount = leftCharCount;
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      CharacterRunNode::GetParent
        //
        //  Synopsis:
        //      Strongly typed SplayTreeNode wrapper.
        //
        //---------------------------------------------------------------------------
        inline
        CharacterRunNode *CharacterRunNode::GetParent() const
        {
            return static_cast<CharacterRunNode *>(GetParentIndexed(CharTreeIndex));
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      CharacterRunNode::GetLeftChild
        //
        //  Synopsis:
        //      Strongly typed SplayTreeNode wrapper.
        //
        //---------------------------------------------------------------------------
        inline
        CharacterRunNode *CharacterRunNode::GetLeftChild() const
        {
            return m_pLeftChild;
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      CharacterRunNode::GetRightChild
        //
        //  Synopsis:
        //      Strongly typed SplayTreeNode wrapper.
        //
        //---------------------------------------------------------------------------
        inline
        CharacterRunNode *CharacterRunNode::GetRightChild() const
        {
            return m_pRightChild;
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      CharacterRunNode::GetFirstNodeNoSplay
        //
        //  Synopsis:
        //      Strongly typed SplayTreeNode wrapper.
        //
        //---------------------------------------------------------------------------
        inline
        CharacterRunNode *CharacterRunNode::GetFirstNodeNoSplay()
        {
            return static_cast<CharacterRunNode *>(SplayTreeNode::GetFirstNodeNoSplay());
        }
    }
}
