// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CommandBarElementCollection.g.h"
#include "VectorChangedEventArgs.g.h"
#include "CommandBar.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

void
CommandBarElementCollection::Init(_In_ bool notifyCollectionChanging)
{
    m_notifyCollectionChanging = notifyCollectionChanging;
}

IFACEMETHODIMP
CommandBarElementCollection::SetAt(_In_ UINT index, _In_opt_ ICommandBarElement* item)
{
    HRESULT hr = S_OK;

    XUINT32 size = 0;
    IFC(get_Size(&size));
    IFCEXPECTRC(index < size, E_BOUNDS);

    IFC(RaiseVectorChanging(wfc::CollectionChange_ItemChanged, index));
    IFC(CommandBarElementCollectionGenerated::SetAt(index, item));
    IFC(RaiseVectorChanged(wfc::CollectionChange_ItemChanged, index));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
CommandBarElementCollection::InsertAt(_In_ UINT index, _In_opt_ ICommandBarElement* item)
{
    HRESULT hr = S_OK;

    XUINT32 size = 0;
    IFC(get_Size(&size));
    IFCEXPECTRC(index <= size, E_BOUNDS);

    IFC(RaiseVectorChanging(wfc::CollectionChange_ItemInserted, index));
    IFC(CommandBarElementCollectionGenerated::InsertAt(index, item));
    IFC(RaiseVectorChanged(wfc::CollectionChange_ItemInserted, index));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
CommandBarElementCollection::RemoveAt(_In_ UINT index)
{
    XUINT32 size = 0;
    IFC_RETURN(get_Size(&size));
    IFCEXPECTRC_RETURN(index < size, E_BOUNDS);

    IFC_RETURN(RaiseVectorChanging(wfc::CollectionChange_ItemRemoved, index));
    IFC_RETURN(CommandBarElementCollectionGenerated::RemoveAt(index));
    IFC_RETURN(RaiseVectorChanged(wfc::CollectionChange_ItemRemoved, index));

    return S_OK;
}

IFACEMETHODIMP
CommandBarElementCollection::Append(_In_opt_ ICommandBarElement* item)
{
    HRESULT hr = S_OK;
    XUINT32 insertedIndex = 0;

    IFC(get_Size(&insertedIndex));
    IFC(RaiseVectorChanging(wfc::CollectionChange_ItemInserted, insertedIndex));
    IFC(CommandBarElementCollectionGenerated::Append(item));
    IFC(RaiseVectorChanged(wfc::CollectionChange_ItemInserted, insertedIndex));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
CommandBarElementCollection::RemoveAtEnd()
{
    HRESULT hr = S_OK;
    XUINT32 size = 0;

    IFC(get_Size(&size));

    IFCEXPECTRC(size > 0, E_BOUNDS);

    IFC(RemoveAt(size - 1));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
CommandBarElementCollection::Clear()
{
    HRESULT hr = S_OK;

    IFC(RaiseVectorChanging(wfc::CollectionChange_Reset, 0));
    IFC(CommandBarElementCollectionGenerated::Clear());
    IFC(RaiseVectorChanged(wfc::CollectionChange_Reset, 0));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
CommandBarElementCollection::RaiseVectorChanging(
    _In_ wfc::CollectionChange change,
    _In_ UINT32 changeIndex)
{
    HRESULT hr = S_OK;

    if (m_notifyCollectionChanging)
    {
        CDependencyObject* const pOwnerNoRef = static_cast<CCollection*>(GetHandle())->GetOwner();
        ctl::ComPtr<DependencyObject> spOwnerAsDO;
        ctl::ComPtr<xaml_controls::ICommandBar> spOwnerAsICommandBar;
        IFC(DXamlCore::GetCurrent()->GetPeer(pOwnerNoRef, &spOwnerAsDO));
        spOwnerAsICommandBar = spOwnerAsDO.AsOrNull<ICommandBar>();

        if (spOwnerAsICommandBar)
        {
            IFC(spOwnerAsICommandBar.Cast<CommandBar>()->NotifyElementVectorChanging(
                this,
                change,
                changeIndex));
        }
    }
Cleanup:
    return hr;
}

_Check_return_ HRESULT
CommandBarElementCollection::RaiseVectorChanged(
    _In_ wfc::CollectionChange change,
    _In_ XUINT32 changeIndex)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<VectorChangedEventSourceType> spEventSource;
    ctl::ComPtr<VectorChangedEventArgs> spArgs;

    IFC(ctl::make(&spArgs));
    IFC(spArgs->put_CollectionChange(change));
    IFC(spArgs->put_Index(changeIndex));

    IFC(GetVectorChangedEventSource(&spEventSource));
    IFC(spEventSource->Raise(this, spArgs.Get()));

Cleanup:
    RRETURN(hr);
}

// Ensures the item type is valid based on the type of the content property.
_Check_return_ HRESULT
CommandBarElementCollection::ValidateItem(_In_ CDependencyObject* pItem)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spItemManagedPeer;

    IFC(DXamlCore::GetCurrent()->GetPeer(pItem, &spItemManagedPeer));
    if (!ctl::is<ICommandBarElement>(spItemManagedPeer))
    {
        IFC(E_INVALIDARG);
    }

Cleanup:
    RRETURN(hr);
}
