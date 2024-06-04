// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CharacterRunNode.h"
#include "SplayTreeCollection.h"

namespace RichTextServices
{
    namespace Internal
    {
        //---------------------------------------------------------------------------
        //
        //  CharacterRunCollection
        //
        //  Holds a collection of CharacterRunNodes.
        //
        //---------------------------------------------------------------------------
        class CharacterRunCollection : public SplayTreeCollection
        {
        public:

            // Initializes a new CharacterRunCollection instance.
            CharacterRunCollection();

            // Destructor.
            ~CharacterRunCollection() override;

            //-----------------------------------------------------------------------
            //
            // SplayTreeCollection overrides.
            //
            //-----------------------------------------------------------------------
            SplayTreeNode *GetRootNodeIndexed(_In_ XUINT32 index) const override;
            void SetRootNodeIndexed(
                _In_ XUINT32 index,
                _In_ SplayTreeNode *pRootNode
                ) override;

            // Returns this collection as a CharacterRunNode, or NULL if this collection
            // is not a CharacterRunNode.
            virtual CharacterRunNode *AsCharacterRunNode();

            // Returns the node covering a specified character.
            CharacterRunNode *GetNodeAtOffset(
                _In_ XUINT32 offset,
                _Out_ XUINT32 *pNodeOffset
                );

            // Strongly typed SplayTreeCollection wrappers.
            CharacterRunNode *GetRootNode() const;

        protected:
            // Frees contained nodes.
            void Uninitialize(_In_opt_ CharacterRunNode::IDeleteTreeListener *pListener);

        private:

            CharacterRunNode *m_pRootNode;
                // Root of the tree of nodes in this collection.
        };

        inline
        CharacterRunNode *CharacterRunCollection::GetRootNode() const
        {
            return m_pRootNode;
        }
    }
}

//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunCollection::CharacterRunCollection
//
//  Synopsis:
//      Initializes a new CharacterRunCollection instance.
//
//---------------------------------------------------------------------------
inline
RichTextServices::Internal::CharacterRunCollection::CharacterRunCollection()
{
    m_pRootNode = NULL;
}

//---------------------------------------------------------------------------
//
//  Member:
//      CharacterRunNodeContainer::~CharacterRunNodeContainer
//
//  Synopsis:
//      Destructor, frees contained nodes.
//
//---------------------------------------------------------------------------
inline
RichTextServices::Internal::CharacterRunCollection::~CharacterRunCollection()
{
    // Derived class and/or owner must call Unitialize explicitly before teardown.
    // Because we call virtual methods in DeleteTree, we can't do clean up in the dtor.
    // During ctor/dtor, virtuals do surprising things (cf _purecall).
    ASSERT(NULL == m_pRootNode);
}
