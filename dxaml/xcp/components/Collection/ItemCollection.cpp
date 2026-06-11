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
#include <double.h>
#include <Point.h>
#include <dopointercast.h>
#include <corep.h>
#include "ICollectionChangeCallback.h"
#include <NoParentShareableDependencyObject.h>
#include <Panel.h>
#include <Template.h>
#include <XAMLItemCollection.h>
#include <CControl.h>
#include <ItemsPresenter.h>
#include <MultiParentShareableDependencyObject.h>
#include <Layouttransition.h>
#include <TransitionCollection.h>
#include <ItemsControl.h>
#include <FxCallbacks.h>

CItemCollection::~CItemCollection()
{
    ClearLogicalParentForItems();
}

// Items in the CItemCollection aren't necessarily in the visual tree (e.g. CComboBox items aren't in the tree until the popup
// is opened), and aren't considered live until then. This is needed because any live Enters we call on the items will not have
// a matching live Leave (see CItemsControl::LeaveImpl), so we don't allow any live Enters at all via this collection.
bool CItemCollection::IsActive() const
{
    return false;
}

//
//  CItemCollection::SetOwner
//
//  Sets the items control as owner for this collection
//  and propagates call into the managed side to also establish
//  the connection there.
//
_Check_return_
HRESULT
CItemCollection::SetOwner(_In_opt_ CDependencyObject *pOwner, _In_opt_ RENDERCHANGEDPFN pfnOwnerDirtyFlag)
{
    CItemsControl* pItemsControl = NULL;

    IFC_RETURN(CDOCollection::SetOwner(pOwner, pfnOwnerDirtyFlag));

    if (pOwner != NULL)
    {
        if (FAILED(DoPointerCast(pItemsControl, pOwner)))
        {
            IFC_RETURN(E_UNEXPECTED);  // Owner must be an ItemsControl
        }

        IFC_RETURN(FxCallbacks::ItemsControl_SetItemCollection(this, pItemsControl));
    }
    else
    {
        ClearLogicalParentForItems();
    }
    m_pItemsControl = pItemsControl;

    return S_OK;
}


//
//  CItemCollection::RemoveAt
//
//  Clear Logical child before an item is removed from the collection
//
_Check_return_
void *
CItemCollection::RemoveAt(_In_ XUINT32 nIndex)
{
    CDependencyObject *pDO = GetItemDOWithAddRef(nIndex);
    // We assume that we got correct index from managed side.
    // If somebody going to decide to use ItemCollection from native
    // we will assert in case of wrong index.
    ASSERT(pDO != NULL);
    m_pItemsControl->RemoveLogicalChild(pDO);

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

//  CItemCollection::Append
//
//  Before calling to base to add the item, validate that
//  it's OK to be using Items (i.e. that ItemsSource isn't being
//  used right now).
//
_Check_return_
HRESULT
CItemCollection::Append(_In_ CDependencyObject *pObject, _Out_opt_ XUINT32 *pnIndex)
{
    IFCEXPECT_RETURN(m_pItemsControl);

    if(!pObject)
    {
        return E_INVALIDARG;
    }

    IFC_RETURN(CDOCollection::Append(pObject, pnIndex));

    //set the logical parent of the new item to the itemscontrol
    IFC_RETURN(m_pItemsControl->AddLogicalChild(pObject));

    return S_OK;
}

//
//  CItemCollection::Insert
//
//  Before calling to base to insert the item, validate that
//  it's OK to be using Items (i.e. that ItemsSource isn't being
//  used right now).
//
_Check_return_
HRESULT
CItemCollection::Insert(_In_ XUINT32 nIndex, _In_ CDependencyObject *pObject)
{
    IFCEXPECT_RETURN(m_pItemsControl);

    if(!pObject)
    {
        return E_INVALIDARG;
    }

    IFC_RETURN(CDOCollection::Insert(nIndex, pObject));

    //set the logical parent of the new item to the itemscontrol
    IFC_RETURN(m_pItemsControl->AddLogicalChild(pObject));

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
//  CItemCollection::Clear
//
//  ItemsControl needs to call ClearContainerForItemOverride for each of the items *before* they go away.
//
_Check_return_
HRESULT CItemCollection::Clear()
{
    IFCEXPECT_RETURN(m_pItemsControl);

    ClearLogicalParentForItems();

    IFC_RETURN( CDOCollection::Clear() );
    IFC_RETURN( m_pItemsControl->InvalidateItemsHost(/*bHostIsReplaced*/ false) );

    {
        auto callback = m_wrChangeCallback.lock();

        if (callback)
        {
            IFC_RETURN(callback->CollectionCleared());
        }
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   ClearLogicalParentForItems
//
//  Synopsis:   clears logical parent property on all items of the collection
//                   if pLogicalParent is the logical parent.
//
//-------------------------------------------------------------------------
void CItemCollection::ClearLogicalParentForItems()
{
    if(m_pItemsControl == nullptr)
    {
        return;
    }

    for(XUINT32 itemIndex = 0; itemIndex < GetCount(); itemIndex++)
    {
        CDependencyObject* pItem = GetItemDOWithAddRef(itemIndex);
        m_pItemsControl->RemoveLogicalChild(pItem);
        ReleaseInterface(pItem);
    }
}

_Check_return_ HRESULT CItemCollection::MoveInternal(_In_ XINT32 nIndex, _In_ XINT32 nPosition)
{
    ASSERT(nIndex >= 0);
    ASSERT(nPosition >= 0);

    IFC_RETURN(CDOCollection::MoveInternal(nIndex, nPosition));

    {
        auto callback = m_wrChangeCallback.lock();

        if (callback)
        {
            IFC_RETURN(callback->ElementMoved(
                    static_cast<UINT32>(nIndex),
                    static_cast<UINT32>(nPosition)));
        }
    }

    return S_OK;
}
