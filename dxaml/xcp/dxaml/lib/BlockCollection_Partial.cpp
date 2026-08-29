// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "BlockCollection.g.h"
#include "Block.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;



//-----------------------------------------------------------------------------
//
//  DisconnectVisualChildrenRecursive
//
//  During a DisconnectVisualChildrenRecursive tree walk, clear this collection and any Inline
//  items within.
//
//-----------------------------------------------------------------------------

_Check_return_ HRESULT
BlockCollection::DisconnectVisualChildrenRecursive()
{
    HRESULT hr = S_OK;
    UINT count = 0;
    IBlock* pBlock= NULL;

    // See if there's anything to do
    IFC( get_Size( &count ));
    if( count == 0 )
        goto Cleanup;

    // Call disconnect on each of the children.
    for(UINT i = 0; i < count; i++)
    {
        IFC( static_cast<wfc::IVector<xaml_docs::Block*>*>(this)->GetAt( i, &pBlock ));
        IFC( static_cast<Block*>(pBlock)->DisconnectVisualChildrenRecursive() );
        ReleaseInterface( pBlock );
    }

    // Clear the collection
    IFC( Clear() );

Cleanup:

    ReleaseInterface( pBlock );
    RRETURN(hr);

}




