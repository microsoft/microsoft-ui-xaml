// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "collectionbase.h"

// Sets the collection owner. From now on, it will be set as the parent of all items added to the collection.
// Also set as the owner of items already added.
_Check_return_ HRESULT CCollection::SetAndPropagateOwner(_In_opt_ CDependencyObject* pOwner, _In_opt_ RENDERCHANGEDPFN pfnOwnerDirtyFlag)
{
    CDependencyObject* const pOldOwner = GetOwner();

    if (NeedsOwnerInfo())
    {
        ASSERT(pOwner == NULL || !pOwner->OfTypeByIndex<KnownTypeIndex::PresentationFrameworkCollection>());
        IFC_RETURN(SetOwner(pOwner, pfnOwnerDirtyFlag));
        IFC_RETURN(PropagateOwnerInfo(pOwner, pOldOwner));
    }

    return S_OK;
}
