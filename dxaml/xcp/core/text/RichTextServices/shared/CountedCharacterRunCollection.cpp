// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CountedCharacterRunCollection.h"
#include "CharacterRunNode.h"

using namespace RichTextServices;
using namespace RichTextServices::Internal;

//---------------------------------------------------------------------------
//
//  Member:
//      CountedCharacterRunCollection::AdjustCollectionCounts
//
//  Synopsis:
//      Modifies the character count of an CountedCharacterRunCollection and any containing
//      CountedCharacterRunCollection.
//
//---------------------------------------------------------------------------
void CountedCharacterRunCollection::AdjustCollectionCounts(
    _In_ XINT32 delta
        // Change in char count, negative values are legal.
    )
{
    if (delta != 0)
    {
        CountedCharacterRunCollection *pCollection = this;

        do
        {
            pCollection->AdjustCount(delta);

            CharacterRunNode *pNode = pCollection->AsCharacterRunNode();

            if (NULL == pNode)
                break;

            pCollection = static_cast<CountedCharacterRunCollection *>(pNode->GetContainingCollection());
        }
        while (pCollection != NULL);
    }
}

//---------------------------------------------------------------------------
//
//  Member:
//      CountedCharacterRunCollection::Connect
//
//  Synopsis:
//      Attaches a tree of CharacterRunNodes to a CountedCharacterRunCollection.
//
//  Notes:
//      See CharacterRunNode::Disconnect comments for usage pattern/details.
//
//---------------------------------------------------------------------------
void CountedCharacterRunCollection::Connect(
    _In_opt_ CountedCharacterRunCollection *pContainingCollection, 
    _In_opt_ CharacterRunNode *pRootNode, 
    _In_ XINT32 delta
        // Change in character count value, negative values are legal.
    )
{
    SplayTreeNode::Connect(pContainingCollection, pRootNode);

    if (pContainingCollection != NULL)
    {
        pContainingCollection->AdjustCollectionCounts(delta);
    }
}
