// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TimingCollection.h"
#include "Timeline.h"

void CTimingCollection::SetTimingOwner( _In_ CTimeline* pTimingOwner )
{
    m_pTimingOwner = pTimingOwner;
}

// Returns S_OK in case this collection is in a state where it can be
// modified.  Returns an appropriate error message otherwise.
_Check_return_ HRESULT CTimingCollection::CheckCanBeModified()
{
    if (m_pTimingOwner)
    {
        // check current state of timing owner's root timeline
        IFC_RETURN(m_pTimingOwner->CheckCanBeModified());
    }

    return S_OK;
}

_Check_return_ HRESULT CTimingCollection::Clear()
{
    IFC_RETURN(CheckCanBeModified());

    IFC_RETURN(CDOCollection::Clear());

    return S_OK;
}

_Check_return_ HRESULT CTimingCollection::Append(_In_ CDependencyObject *pObject, _Out_ XUINT32 *pnIndex)
{
    IFC_RETURN(CheckCanBeModified());

    IFC_RETURN(CDOCollection::Append(pObject, pnIndex));

    return S_OK;
}

_Check_return_ HRESULT CTimingCollection::Insert(_In_ XUINT32 nIndex, _In_ CDependencyObject *pObject)
{
    IFC_RETURN(CheckCanBeModified());

    IFC_RETURN(CDOCollection::Insert(nIndex, pObject));

    return S_OK;
}

_Check_return_ void* CTimingCollection::RemoveAt(_In_ XUINT32 nIndex)
{
    if (FAILED(CheckCanBeModified()))
    {
        return NULL;
    }

    return CDOCollection::RemoveAt(nIndex);
}
