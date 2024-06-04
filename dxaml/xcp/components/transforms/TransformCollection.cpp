// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TransformCollection.h"
#include <TransformGroup.h>

void CTransformCollection::OnCollectionChanged()
{
    // Collection changes propagate dirty flags in a different order than normal property changes.
    // Normal property changes update the value, and then propagate dirty flags, so any code running during
    // propagation can safely query the new value.
    // Collections update the parent pointers on the items (which propagates the dirty flags) first, and then add
    // the item to the collection.
    // In the case of TransformGroups, this can cause the cached transform to be re-calculated before the child
    // collection was updated, and leave it stale afterwards. The lowest impact fix is to propagate dirty flags a
    // second time here, after the collection itself has changed, which will ensure the cached transform is
    // re-calculated again correctly next time it is queried.
    CDependencyObject *pOwnerNoRef = GetOwner();
    if (pOwnerNoRef != NULL)
    {
        ASSERT(pOwnerNoRef->OfTypeByIndex<KnownTypeIndex::TransformGroup>());
        CTransformGroup::NWSetTransformsDirty(pOwnerNoRef, DirtyFlags::Render);
    }
}

// Since transforms can be shared, ensure that we don't have a cycle more exhaustively than the base class does.
_Check_return_ HRESULT CTransformCollection::CycleCheck(_In_ CDependencyObject *pObject)
{
    IFC_RETURN(__super::CycleCheck(pObject));

    // Only Transform groups pose a problem
    if (pObject->OfTypeByIndex<KnownTypeIndex::TransformGroup>())
    {
        CTransformGroup *pTG = static_cast<CTransformGroup *>(pObject);

        // Test top-level
        if (pTG->m_pChild == this)
        {
            IFC_RETURN(E_INVALIDARG);
        }
        // Test children
        else if (pTG->m_pChild != nullptr)
        {
            for (auto item : *(pTG->m_pChild))
            {
                IFC_RETURN(CycleCheck(item));
            }
        }
    }

    return S_OK;
}
