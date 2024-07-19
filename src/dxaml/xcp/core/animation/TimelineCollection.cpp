// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TimelineCollection.h"
#include "Timeline.h"

KnownTypeIndex CTimelineCollection::GetTypeIndex() const
{
    return DependencyObjectTraits<CTimelineCollection>::Index;
}

// Override to allow the timing owner to act as namescope parent.
CDependencyObject* CTimelineCollection::GetStandardNameScopeParent()
{
    CDependencyObject* pNamescopeParent = CTimingCollection::GetStandardNameScopeParent();
    if (!pNamescopeParent)
    {
        pNamescopeParent = m_pTimingOwner;
    }
    return pNamescopeParent;
}

// Append a reference to an object to the collection.
//
// This method was added to ensure that the logic in
// CTimelineGroup::AddChild still happens to CTimelines added directly to a
// CTimelineCollection by the new XAML parser.  The old XAML parser would
// call CTimelineGroup::SetValue(null, child) which in turn invoked the
// AddChild that correctly associated child CTimelines with their owners.
_Check_return_ HRESULT CTimelineCollection::Append(_In_ CDependencyObject *pObject, _Out_ XUINT32 *pnIndex)
{
    CTimeline* pTimeline = NULL;

    // Ensure that the argument is a CTimeline that hasn't been associated with
    // another element or collection
    pTimeline = do_pointer_cast<CTimeline>(pObject);
    if (!pTimeline || pTimeline->IsAssociated())
    {
        IFC_RETURN(E_INVALIDARG);
    }

    IFC_RETURN(CTimingCollection::Append(pObject, pnIndex));

    // Associate the argument with its CTimeline parent (i.e., the CTimeline
    // that owns this collection)
    pTimeline->SetTimingParent(m_pTimingOwner);

    return S_OK;
}

// Check to see if adding this item will result in a cycle....
_Check_return_ HRESULT CTimelineCollection::CycleCheck(_In_ CDependencyObject *pObject)
{
    CTimeline *pParent = NULL;
    CTimeline *pTimingObject = do_pointer_cast<CTimeline>(pObject);
    IFCPTR_RETURN(pTimingObject);

    IFC_RETURN( CDOCollection::CycleCheck(pObject) );

    pParent = m_pTimingOwner;

    while( pParent )
    {
        if ( pParent == pTimingObject )
        {
            IFC_RETURN(E_INVALIDARG);
        }

        // Move Next
        pParent = pParent->GetTimingParent();
    }

    return S_OK;
}

_Check_return_ HRESULT CTimelineCollection::OnAddToCollection(CDependencyObject *pDO)
{
    // Keep a parent reference in the DO parent pointer - this is safe
    // since timing nodes are never part of the visual tree.
    static_cast<CTimeline*>(pDO)->SetTimingParent(static_cast<CTimeline*>(m_pTimingOwner));

    return S_OK;
}
