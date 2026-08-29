// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <CDependencyObject.h>
#include <collectionbase.h>
#include <DOCollection.h>
#include <DXamlServices.h>
#include <AutoReentrantReferenceLock.h>
#include "ICollectionChangeCallback.h"
#include <CValue.h>
#include <UIElement.h>
#include <dopointercast.h>
#include <corep.h>
#include "ICollectionChangeCallback.h"
#include <NoParentShareableDependencyObject.h>
#include <Panel.h>
#include <Template.h>
#include <CControl.h>
#include <MultiParentShareableDependencyObject.h>
#include <Layouttransition.h>

#include <BrushCollection.h>
#include <Brush.h>

_Check_return_ HRESULT
CBrushCollection::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
    )
{
    *ppObject = new CBrushCollection(pCreate->m_pCore);
    return S_OK;
}


//
//  CBrushCollection::RemoveAt
//
//  Clear Logical child before an item is removed from the collection
//
_Check_return_
void *
CBrushCollection::RemoveAt(_In_ XUINT32 nIndex)
{
    CDependencyObject *pDO = GetItemDOWithAddRef(nIndex);
    // We assume that we got correct index from managed side.
    // If somebody going to decide to use ItemCollection from native
    // we will assert in case of wrong index.
    ASSERT(pDO != NULL);

    {
        auto callback = m_wrChangeCallback.lock();

        if (callback)
        {
            IGNOREHR(callback->ElementRemoved(nIndex));
        }
    }

    ReleaseInterface(pDO);
    return CDOCollection::RemoveAt(nIndex);
}

//  CBrushCollection::Append
//
//  Before calling to base to add the item, validate that
//  it's OK to be using Items (i.e. that ItemsSource isn't being
//  used right now).
//
_Check_return_
HRESULT
CBrushCollection::Append(_In_ CDependencyObject *pObject, _Out_opt_ XUINT32 *pnIndex)
{
    if (!pObject)
    {
        return E_INVALIDARG;
    }

    IFC_RETURN(CDOCollection::Append(pObject, pnIndex));

    return S_OK;
}

//
//  CBrushCollection::Insert
//
//  Before calling to base to insert the item, validate that
//  it's OK to be using Items (i.e. that ItemsSource isn't being
//  used right now).
//
_Check_return_
HRESULT
CBrushCollection::Insert(_In_ XUINT32 nIndex, _In_ CDependencyObject *pObject)
{
    if (!pObject)
    {
        return E_INVALIDARG;
    }

    IFC_RETURN(CDOCollection::Insert(nIndex, pObject));

    {
        auto callback = m_wrChangeCallback.lock();

        if (callback)
        {
            IFC_RETURN(callback->ElementInserted(nIndex));
        }
    }

    return S_OK;
}

//
//  CBrushCollection::Clear
//
//
//
_Check_return_
HRESULT CBrushCollection::Clear()
{
    IFC_RETURN(CDOCollection::Clear());

    {
        auto callback = m_wrChangeCallback.lock();

        if (callback)
        {
            IFC_RETURN(callback->CollectionCleared());
        }
    }

    return S_OK;
}
