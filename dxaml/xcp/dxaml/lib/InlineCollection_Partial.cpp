// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "InlineCollection.g.h"
#include "Inline.g.h"

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
InlineCollection::DisconnectVisualChildrenRecursive()
{
    HRESULT hr = S_OK;
    UINT count = 0;
    IInline* pInline = NULL;

    // See if there's anything to do.
    IFC( get_Size( &count ));
    if( count == 0 )
        goto Cleanup;

    // Forward the Disconnect call to Inline items
    for(UINT i = 0; i < count; i++)
    {
        IFC( static_cast<wfc::IVector<xaml_docs::Inline*>*>(this)->GetAt( i, &pInline ));
        IFC( static_cast<Inline*>(pInline)->DisconnectVisualChildrenRecursive() );
        ReleaseInterface( pInline );
    }

    // Clear the collection
    IFC( Clear() );

Cleanup:

    ReleaseInterface( pInline );
    RRETURN(hr);

}




