// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "HubSectionCollection.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;


// IIterator<IInspectable*> implementation
IFACEMETHODIMP HubSectionCollectionIterator::get_Current(
    _Outptr_ IInspectable** current)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IHubSection> spCurrentSection;

    IFC(TrackerIterator<xaml_controls::HubSection*>::get_Current(&spCurrentSection));
    IFC(spCurrentSection.MoveTo(current));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP HubSectionCollectionIterator::get_HasCurrent(
    _Out_ BOOLEAN *hasCurrent)
{
    RRETURN(TrackerIterator<xaml_controls::HubSection*>::get_HasCurrent(hasCurrent));
}

IFACEMETHODIMP HubSectionCollectionIterator::MoveNext(
    _Out_ BOOLEAN *hasCurrent)
{
    RRETURN(TrackerIterator<xaml_controls::HubSection*>::MoveNext(hasCurrent));
}

// IIterable<IInspectable*> implementation
IFACEMETHODIMP HubSectionCollection::First(
    _Outptr_ wfc::IIterator<IInspectable*> **iterator)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IIterator<IInspectable*>> spResult;

    IFC(CheckThread());
    ARG_VALIDRETURNPOINTER(iterator);

    IFC(ctl::ComObject<HubSectionCollectionIterator>::CreateInstance(spResult.ReleaseAndGetAddressOf()));
    spResult.Cast<HubSectionCollectionIterator>()->SetCollection(this);

    IFC(spResult.MoveTo(iterator));

Cleanup:
    RRETURN(hr);
}


// IIterable<xaml_controls::IHubSection*> overrides
IFACEMETHODIMP HubSectionCollection::First(
    _Outptr_ wfc::IIterator<xaml_controls::HubSection*> **iterator)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IIterator<xaml_controls::HubSection*>> spResult;

    IFC(CheckThread());
    ARG_VALIDRETURNPOINTER(iterator);

    IFC(ctl::ComObject<HubSectionCollectionIterator>::CreateInstance(spResult.ReleaseAndGetAddressOf()));
    spResult.Cast<HubSectionCollectionIterator>()->SetCollection(this);

    IFC(spResult.MoveTo(iterator));

Cleanup:
    RRETURN(hr);
}


// IVector<IInspectable*> implementation
IFACEMETHODIMP HubSectionCollection::GetAt(_In_ UINT index, _Outptr_ IInspectable** item)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IHubSection> spSection;

    IFC(HubSectionCollectionGenerated::GetAt(index, &spSection));
    IFC(spSection.MoveTo(item));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP HubSectionCollection::get_Size(_Out_ UINT* value)
{
    RRETURN(HubSectionCollectionGenerated::get_Size(value));
}

IFACEMETHODIMP HubSectionCollection::GetView(_Outptr_result_maybenull_ wfc::IVectorView<IInspectable*>** view)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVectorView<IInspectable*>> spResult;

    IFC(CheckThread());
    ARG_VALIDRETURNPOINTER(view);

    IFC(ctl::ComObject<TrackerView<IInspectable*>>::CreateInstance(spResult.ReleaseAndGetAddressOf()));
    spResult.Cast<TrackerView<IInspectable*>>()->SetCollection(this);

    *view = spResult.Detach();

Cleanup:

    RRETURN(hr);
}

IFACEMETHODIMP HubSectionCollection::IndexOf(_In_opt_ IInspectable* value, _Out_ UINT* index, _Out_ BOOLEAN* found)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IHubSection> spSection;

    IFC(do_query_interface(spSection, value));
    IFC(HubSectionCollectionGenerated::IndexOf(spSection.Get(), index, found));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP HubSectionCollection::SetAt(_In_ UINT index, _In_opt_ IInspectable* item)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IHubSection> spSection;

    IFC(do_query_interface(spSection, item));
    IFC(HubSectionCollectionGenerated::SetAt(index, spSection.Get()));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP HubSectionCollection::InsertAt(_In_ UINT index, _In_ IInspectable* item)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IHubSection> spSection;

    IFC(do_query_interface(spSection, item));
    IFC(HubSectionCollectionGenerated::InsertAt(index, spSection.Get()));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP HubSectionCollection::RemoveAt(_In_ UINT index)
{
    RRETURN(HubSectionCollectionGenerated::RemoveAt(index));
}

IFACEMETHODIMP HubSectionCollection::Append(_In_opt_ IInspectable* item)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IHubSection> spSection;

    IFC(do_query_interface(spSection, item));
    IFC(HubSectionCollectionGenerated::Append(spSection.Get()));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP HubSectionCollection::RemoveAtEnd()
{
    RRETURN(HubSectionCollectionGenerated::RemoveAtEnd());
}

IFACEMETHODIMP HubSectionCollection::Clear()
{
    RRETURN(HubSectionCollectionGenerated::Clear());
}

// Ensures the item type is valid based on the type of the content property.
_Check_return_ HRESULT
HubSectionCollection::ValidateItem(_In_ CDependencyObject* pItem)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spItemManagedPeer;

    IFC(DXamlCore::GetCurrent()->GetPeer(pItem, &spItemManagedPeer));
    if (!ctl::is<IHubSection>(spItemManagedPeer))
    {
        IFC(E_INVALIDARG);
    }

Cleanup:
    RRETURN(hr);
}