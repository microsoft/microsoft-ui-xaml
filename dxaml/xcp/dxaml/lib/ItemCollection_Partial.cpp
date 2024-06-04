// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ItemCollection.g.h"
#include "ItemsControl.g.h"
#include "CollectionViewManager.h"

using namespace DirectUI;
using namespace xaml_interop;
using namespace xaml;

ItemCollection::ItemCollection()
    : m_bInItemChanging(FALSE)
    , m_bBusy(FALSE)
{
}

_Check_return_
HRESULT
ItemCollection::Init(_In_ ItemsControl* pOwner)
{
    RRETURN(ctl::AsWeak(pOwner, &m_wrOwner));
}

ItemCollection::~ItemCollection()
{
    if (m_epVectorChangedHandler)
    {
        auto spItemsObservableVector = m_tpItemsObservableVector.GetSafeReference();
        if (spItemsObservableVector)
        {
            VERIFYHR(m_epVectorChangedHandler.DetachEventHandler(spItemsObservableVector.Get()));
        }
    }
}

// Support the IItemCollection interface.
_Check_return_
HRESULT
ItemCollection::QueryInterfaceImpl(
    _In_ REFIID iid,
    _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(xaml_controls::IItemCollection)))
    {
        *ppObject = static_cast<xaml_controls::IItemCollection*>(this);
    }
    else
    {
        RRETURN(ItemCollectionGenerated::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

IFACEMETHODIMP
ItemCollection::First(
    _Outptr_result_maybenull_ wfc::IIterator<IInspectable*>** first)
{
    if(ItemsSourceActive())
    {
        RRETURN(m_tpItemsSource->First(first));
    }
    else
    {
        RRETURN(ItemCollectionGenerated::First(first));
    }
}

IFACEMETHODIMP
ItemCollection::get_Size(_Out_ UINT* size)
{
    if(ItemsSourceActive() && m_tpItemsView)
    {
        RRETURN(m_tpItemsView->get_Size(size));
    }
    else
    {
        RRETURN(ItemCollectionGenerated::get_Size(size));
    }
}

IFACEMETHODIMP
ItemCollection::GetAt(
    _In_opt_ UINT index,
    _Out_ IInspectable** item)
{
    if (ItemsSourceActive() && m_tpItemsView)
    {
        RRETURN(m_tpItemsView->GetAt(index, item));
    }
    else
    {
        RRETURN(ItemCollectionGenerated::GetAt(index, item));
    }
}

IFACEMETHODIMP
ItemCollection::GetView(
    _Outptr_result_maybenull_ wfc::IVectorView<IInspectable*>** view)
{
    if(ItemsSourceActive() && m_tpItemsView)
    {
        if (view)
        {
            RRETURN(m_tpItemsView->GetView(view));
        }
        RRETURN(E_NOTIMPL);
    }
    else
    {
        RRETURN(ItemCollectionGenerated::GetView(view));
    }
}

IFACEMETHODIMP
ItemCollection::IndexOf(
    _In_opt_ IInspectable* value,
    _Out_ UINT* index,
    _Out_ BOOLEAN* found)
{
    if(ItemsSourceActive() && m_tpItemsView)
    {
        RRETURN(m_tpItemsView->IndexOf(value, index, found));
    }
    else
    {
        RRETURN(ItemCollectionGenerated::IndexOf(value, index, found));
    }
}

IFACEMETHODIMP
ItemCollection::SetAt(
    _In_ UINT index,
    _In_opt_ IInspectable* item)
{
    HRESULT hr = S_OK;

    IFC(CheckReentrancy());
    //TODO: Report Error ItemCollection_NotSupportedReadOnlyCollection
    // Bug#95997
    IFCEXPECT(!ItemsSourceActive());

    UINT size = 0;
    IFC(get_Size(&size));
    IFCEXPECTRC(index < size, E_BOUNDS);

    m_bInItemChanging = TRUE;
    IFC(ItemCollectionGenerated::SetAt(index, item));
    IFC(RaiseVectorChanged(wfc::CollectionChange_ItemChanged, index));

Cleanup:
    m_bInItemChanging = FALSE;
    RRETURN(hr);
}

IFACEMETHODIMP
ItemCollection::InsertAt(
    _In_ UINT index,
    _In_opt_ IInspectable* item)
{
    HRESULT hr = S_OK;
    UINT size = 0;

    IFC(CheckReentrancy());
    //TODO: Report Error ItemCollection_NotSupportedReadOnlyCollection
    // Bug#95997
    IFCEXPECT(!ItemsSourceActive());

    IFC(get_Size(&size));
    //TODO: Report Error ArgumentOutOfRangeException
    // Bug#95997
    IFCEXPECTRC(index <= size, E_BOUNDS);

    IFC(ItemCollectionGenerated::InsertAt(index, item));
    if (!m_bInItemChanging)
    {
        IFC(RaiseVectorChanged(wfc::CollectionChange_ItemInserted, index));
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
ItemCollection::RemoveAt(_In_ UINT index)
{
    HRESULT hr = S_OK;

    IFC(CheckReentrancy());
    //TODO: Report Error ItemCollection_NotSupportedReadOnlyCollection
    // Bug#95997
    IFCEXPECT(!ItemsSourceActive());

    UINT size = 0;
    IFC(get_Size(&size));
    IFCEXPECTRC(index < size, E_BOUNDS);

    IFC(ItemCollectionGenerated::RemoveAt(index));
    if (!m_bInItemChanging)
    {
        IFC(RaiseVectorChanged(wfc::CollectionChange_ItemRemoved, index));
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
ItemCollection::Append(_In_opt_ IInspectable* item)
{
    HRESULT hr = S_OK;
    UINT index = 0;

    IFC(CheckReentrancy());
    //TODO: Report Error ItemCollection_NotSupportedReadOnlyCollection
    // Bug#95997
    IFCEXPECT(!ItemsSourceActive());

    IFC(ItemCollectionGenerated::Append(item));
    IFC(get_Size(&index));
    IFC(RaiseVectorChanged(wfc::CollectionChange_ItemInserted, index - 1));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
ItemCollection::Clear()
{
    HRESULT hr = S_OK;
    IFC(CheckReentrancy());
    //TODO: Report Error ItemCollection_NotSupportedReadOnlyCollection
    // Bug#95997
    IFCEXPECT(!ItemsSourceActive());

    IFC(ItemCollectionGenerated::Clear());
    IFC(RaiseVectorChanged(wfc::CollectionChange_Reset, 0));

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
ItemCollection::NotifyCollectionReady()
{
    HRESULT hr = S_OK;
    UINT nCount = 0;

    IFC(get_Size(&nCount));

    for (UINT index = 0; index < nCount; index++)
    {
        // Notify the ItemsControl of all of the items present in the collection
        IFC(RaiseVectorChanged(wfc::CollectionChange_ItemInserted, index));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
ItemCollection::CheckReentrancy()
{
    HRESULT hr = S_OK;

    if (m_bBusy)
    {
        IFC(ErrorHelper::OriginateErrorUsingResourceID(E_UNEXPECTED, ERROR_ITEMCOLLECTION_REENTRANCY));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
ItemCollection::RaiseVectorChanged(
    _In_ wfc::CollectionChange action,
    UINT index)
{
    HRESULT hr = S_OK;
    wfc::IVectorChangedEventArgs* pArgsAsI = NULL;
    VectorChangedEventArgs* pArgs = NULL;

    // Create the args
    IFC(ctl::ComObject<VectorChangedEventArgs>::CreateInstance(&pArgsAsI));
    pArgs = static_cast<VectorChangedEventArgs*>(pArgsAsI);

    IFC(pArgs->put_CollectionChange(action));
    IFC(pArgs->put_Index(index));

    IFC(OnCollectionChanged(this, pArgs));

Cleanup:
    ReleaseInterface(pArgsAsI);
    RRETURN(hr);
}

_Check_return_
HRESULT
ItemCollection::UpdateItemsSourceList(
    _In_ wfc::IIterable<IInspectable*>* pValue)
{
    wrl::ComPtr<wfc::IVector<IInspectable*>> itemsView;

    // Check if the old value was non null and disconnect from the
    // CollectionChanged event
    if (m_tpItemsObservableVector)
    {
        if (m_epVectorChangedHandler)
        {
            IFC_RETURN(m_epVectorChangedHandler.DetachEventHandler(m_tpItemsObservableVector.Get()));
        }

        m_tpItemsObservableVector.Clear();
    }

    // Release the old source and keep the new one
    m_tpItemsView.Clear();
    SetPtrValue(m_tpItemsSource, pValue);

    // Attempt to get the new source as an IVector (wrapping it if it isn't one already)
    IFC_RETURN(CollectionViewManager::GetVectorFromSource(m_tpItemsSource.Get(), &itemsView));
    SetPtrValue(m_tpItemsView, itemsView.Get());

    // Check to see too if the vector is observable
    SetPtrValueWithQIOrNull(m_tpItemsObservableVector, m_tpItemsView.Get());

    // If the ItemsSource supports notifications, attach a listener.
    if (m_tpItemsObservableVector)
    {
        // Using weak reference in event handler callback guaranteed that
        // by the time we get callback from custom collection
        // while ItemsCollection marked as unreachable we won't call into it
        // as resolving weak reference returns NULL.
        ctl::WeakRefPtr weakThis;
        IFC_RETURN(ctl::AsWeak(this, &weakThis));
        auto handler = [weakThis](IInspectable *sender, wfc::IVectorChangedEventArgs*args) mutable
        {
            if (auto This = weakThis.AsOrNull<IItemCollection>())
            {
                ItemCollection* itemCollection = This.Cast<ItemCollection>();

                IFC_RETURN(itemCollection->OnCollectionChanged(static_cast<wfc::IObservableVector<IInspectable*>*>(itemCollection), args));
            }
            return S_OK;
        };
        IFC_RETURN(m_epVectorChangedHandler.AttachEventHandler(m_tpItemsObservableVector.Get(), handler));
    }

    IFC_RETURN(RaiseVectorChanged(wfc::CollectionChange_Reset, 0));
    return S_OK;
}

BOOLEAN
ItemCollection::ItemsSourceActive()
{
    return m_tpItemsSource;
}

_Check_return_
HRESULT ItemCollection::OnCollectionChanged(
    _In_ wfc::IObservableVector<IInspectable*>* pSender,
    _In_ wfc::IVectorChangedEventArgs* e)
{
    HRESULT hr = S_OK;
    VectorChangedEventSourceType* pEventSource = NULL;
    ctl::ComPtr<xaml_controls::IItemsControl> spOwner;

    IFC(CheckReentrancy());

    // Raise the event
    IFC(GetVectorChangedEventSource(&pEventSource));
    m_bBusy = TRUE;

    spOwner = m_wrOwner.AsOrNull<xaml_controls::IItemsControl>();

    if (spOwner)
    {
        IFC(spOwner.Cast<ItemsControl>()->NotifyOfSourceChanged(pSender, e));
    }

    IFC(pEventSource->Raise(pSender, e));

Cleanup:
    m_bBusy = FALSE;
    ctl::release_interface(pEventSource);
    RRETURN(hr);
}



//-----------------------------------------------------------------------------
//
//  DisconnectVisualChildrenRecursive
//
//  During a DisconnectVisualChildrenRecursive tree walk, clear all items and then this collection
//  itself (unless this is generated from ItemsSource).
//
//-----------------------------------------------------------------------------

_Check_return_ HRESULT
ItemCollection::DisconnectVisualChildrenRecursive()
{
    HRESULT hr = S_OK;
    UINT count = 0;
    IInspectable* pItem = NULL;
    UIElement *pElement = NULL;

    // If this is generated from ItemsSource, we don't do anything.  Note, however, that
    // the items might still be visited by the visual tree walk.
    if( ItemsSourceActive() )
        goto Cleanup;

    // See if there's anything to do
    IFC( get_Size( &count ));
    if( count == 0 )
        goto Cleanup;

    // Disconnect each of the items (if it's a UIElement)
    for(UINT i = 0; i < count; i++ )
    {
        IFC( static_cast<wfc::IVector<IInspectable*>*>(this)->GetAt( i, &pItem ));
        pElement = static_cast<UIElement*>( ctl::query_interface<IUIElement>( pItem ));
        ReleaseInterface(pItem);

        if( pElement != NULL )
        {
            IFC( static_cast<UIElement*>(pElement)->DisconnectChildrenRecursive() );
            ctl::release_interface( pElement );
        }
    }

    // Clear the collection itself
    IFC( Clear() );

Cleanup:

    ReleaseInterface( pItem );
    ctl::release_interface( pElement );

    RRETURN(hr);

}
