// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MenuFlyoutItemBaseCollection.g.h"
#include "MenuFlyout.g.h"
#include "MenuFlyoutSubItem.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

IFACEMETHODIMP
MenuFlyoutItemBaseCollection::SetAt(_In_ UINT index, _In_opt_ IMenuFlyoutItemBase* item)
{
    IFC_RETURN(MenuFlyoutItemBaseCollectionGenerated::SetAt(index, item));
    IFC_RETURN(NotifyMenuFlyoutOfCollectionChange());

    return S_OK;
}

IFACEMETHODIMP
MenuFlyoutItemBaseCollection::InsertAt(_In_ UINT index, _In_opt_ IMenuFlyoutItemBase* item)
{;
    IFC_RETURN(MenuFlyoutItemBaseCollectionGenerated::InsertAt(index, item));
    IFC_RETURN(NotifyMenuFlyoutOfCollectionChange());

    return S_OK;
}

IFACEMETHODIMP
MenuFlyoutItemBaseCollection::RemoveAt(_In_ UINT index)
{
    IFC_RETURN(MenuFlyoutItemBaseCollectionGenerated::RemoveAt(index));
    IFC_RETURN(NotifyMenuFlyoutOfCollectionChange());

    return S_OK;
}

IFACEMETHODIMP
MenuFlyoutItemBaseCollection::Append(_In_opt_ IMenuFlyoutItemBase* item)
{
    IFC_RETURN(MenuFlyoutItemBaseCollectionGenerated::Append(item));
    IFC_RETURN(NotifyMenuFlyoutOfCollectionChange());

    return S_OK;
}

IFACEMETHODIMP
MenuFlyoutItemBaseCollection::RemoveAtEnd()
{
    XUINT32 size = 0;
    IFC_RETURN(get_Size(&size));
    IFC_RETURN(RemoveAt(size - 1));

    return S_OK;
}

IFACEMETHODIMP
MenuFlyoutItemBaseCollection::Clear()
{
    IFC_RETURN(MenuFlyoutItemBaseCollectionGenerated::Clear());
    IFC_RETURN(NotifyMenuFlyoutOfCollectionChange());

    return S_OK;
}

// IIterator<IInspectable*> implementation
IFACEMETHODIMP MenuFlyoutItemBaseCollectionIterator::get_Current(
    _Outptr_ IInspectable** current)
{
    ctl::ComPtr<IMenuFlyoutItemBase> spCurrentItem;

    IFC_RETURN(TrackerIterator<xaml_controls::MenuFlyoutItemBase*>::get_Current(&spCurrentItem));
    IFC_RETURN(spCurrentItem.MoveTo(current));

    return S_OK;
}

IFACEMETHODIMP MenuFlyoutItemBaseCollectionIterator::get_HasCurrent(
    _Out_ BOOLEAN *hasCurrent)
{
    IFC_RETURN(TrackerIterator<xaml_controls::MenuFlyoutItemBase*>::get_HasCurrent(hasCurrent));
    return S_OK;
}

IFACEMETHODIMP MenuFlyoutItemBaseCollectionIterator::MoveNext(
    _Out_ BOOLEAN *hasCurrent)
{
    IFC_RETURN(TrackerIterator<xaml_controls::MenuFlyoutItemBase*>::MoveNext(hasCurrent));
    return S_OK;
}

// IIterable<IInspectable*> implementation
IFACEMETHODIMP MenuFlyoutItemBaseCollection::First(
    _Outptr_ wfc::IIterator<IInspectable*> **iterator)
{
    ctl::ComPtr<wfc::IIterator<IInspectable*>> spResult;

    IFC_RETURN(CheckThread());
    ARG_VALIDRETURNPOINTER(iterator);

    IFC_RETURN(ctl::ComObject<MenuFlyoutItemBaseCollectionIterator>::CreateInstance(spResult.ReleaseAndGetAddressOf()));
    spResult.Cast<MenuFlyoutItemBaseCollectionIterator>()->SetCollection(this);

    IFC_RETURN(spResult.MoveTo(iterator));
    return S_OK;
}


// IIterable<ABI::Microsoft::UI::Xaml::Controls::MenuFlyoutItemBase*> overrides
IFACEMETHODIMP MenuFlyoutItemBaseCollection::First(
    _Outptr_ wfc::IIterator<xaml_controls::MenuFlyoutItemBase*> **iterator)
{
    ctl::ComPtr<wfc::IIterator<xaml_controls::MenuFlyoutItemBase*>> spResult;

    IFC_RETURN(CheckThread());
    ARG_VALIDRETURNPOINTER(iterator);

    IFC_RETURN(ctl::ComObject<MenuFlyoutItemBaseCollectionIterator>::CreateInstance(spResult.ReleaseAndGetAddressOf()));
    spResult.Cast<MenuFlyoutItemBaseCollectionIterator>()->SetCollection(this);

    IFC_RETURN(spResult.MoveTo(iterator));
    return S_OK;
}

// Ensures the item type is valid based on the type of the content property.
_Check_return_ HRESULT
MenuFlyoutItemBaseCollection::ValidateItem(_In_ CDependencyObject* pItem)
{
    ctl::ComPtr<DependencyObject> spItemManagedPeer;

    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(pItem, &spItemManagedPeer));
    if (!ctl::is<IMenuFlyoutItemBase>(spItemManagedPeer))
    {
        IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}

_Check_return_ HRESULT
MenuFlyoutItemBaseCollection::NotifyMenuFlyoutOfCollectionChange()
{
    ctl::ComPtr<DependencyObject> ownerAsDO;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(static_cast<CCollection*>(GetHandle())->GetOwner(), &ownerAsDO));

    auto ownerAsMenuFlyout = ownerAsDO.AsOrNull<IMenuFlyout>();

    if (ownerAsMenuFlyout)
    {
        IFC_RETURN(ownerAsMenuFlyout.Cast<MenuFlyout>()->QueueRefreshItemsSource());
    }
    else
    {
        auto ownerAsMenuFlyoutSubItem = ownerAsDO.AsOrNull<IMenuFlyoutSubItem>();

        // MenuFlyoutItemBaseCollection is only used by MenuFlyout and MenuFlyoutSubItem.
        // If another type is added, this will need to change.
        IFCEXPECT_RETURN(ownerAsMenuFlyoutSubItem != nullptr);

        IFC_RETURN(ownerAsMenuFlyoutSubItem.Cast<MenuFlyoutSubItem>()->QueueRefreshItemsSource());
    }

    return S_OK;
}