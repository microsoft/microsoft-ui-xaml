// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SplayTreeCollection.h"

using namespace RichTextServices::Internal;

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeCollection::~SplayTreeCollection
//
//  Synopsis:
//      Destructor.
//
//---------------------------------------------------------------------------
SplayTreeCollection::~SplayTreeCollection()
{
}

//---------------------------------------------------------------------------
//
//  Member:
//      SplayTreeCollection::DisconnectNodes
//
//  Synopsis:
//      Called before any editing operation that adds or removes contained nodes.
//
//  Notes:
//      Use this method before editing a collection.
//
//          pCollection->DisconnectNodes();
//
//          .. Now safely add, remove, or resize the contained tree ..
//          // E.g.,
//          SplayTreeNode *pNewRootNode = pSomeNode->Remove();
//
//          pCollection->ConnectNodes(pNewRootNode, delta);
//
//  This method is necessary because methods like Remove do not know how to
//  deal with containers.  In derived classes it also provides a pattern
//  for efficient (batched) updates of nested containers.
//
//---------------------------------------------------------------------------
void SplayTreeCollection::DisconnectNodes()
{
    SplayTreeNode *pRootNode = GetRootNode();
    if (pRootNode != NULL)
    {
        SplayTreeCollection *pContainingCollection = NULL;
        pContainingCollection = pRootNode->Disconnect();
        ASSERT(pContainingCollection == this);
    }
}

