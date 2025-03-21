// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//------------------------------------------------------------------------
//
//  Abstract:
//
//      XAML types.
//      NOTE: This file was generated by a tool.
//
//------------------------------------------------------------------------

#include "CollectionViewGroup.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::CollectionViewGroupGenerated::CollectionViewGroupGenerated()
{
}

DirectUI::CollectionViewGroupGenerated::~CollectionViewGroupGenerated()
{
}

HRESULT DirectUI::CollectionViewGroupGenerated::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::CollectionViewGroup)))
    {
        *ppObject = static_cast<DirectUI::CollectionViewGroup*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Data::ICollectionViewGroup)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Data::ICollectionViewGroup*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Data::ICustomPropertyProvider)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Data::ICustomPropertyProvider*>(this);
    }
    else
    {
        RRETURN(DirectUI::DependencyObject::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
IFACEMETHODIMP DirectUI::CollectionViewGroupGenerated::get_Group(_Outptr_result_maybenull_ IInspectable** ppValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(ppValue);
    *ppValue={};
    IFC(CheckThread());
    IFC(static_cast<CollectionViewGroup*>(this)->get_GroupImpl(ppValue));
Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::CollectionViewGroupGenerated::get_GroupItems(_Outptr_result_maybenull_ ABI::Windows::Foundation::Collections::IObservableVector<IInspectable*>** ppValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(ppValue);
    *ppValue={};
    IFC(CheckThread());
    IFC(static_cast<CollectionViewGroup*>(this)->get_GroupItemsImpl(ppValue));
Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::CollectionViewGroupGenerated::get_Type(_Out_ ABI::Windows::UI::Xaml::Interop::TypeName* pValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(pValue);
    *pValue={};
    IFC(CheckThread());
    IFC(static_cast<CollectionViewGroup*>(this)->get_TypeImpl(pValue));
Cleanup:
    RRETURN(hr);
}

// Events.

// Methods.
IFACEMETHODIMP DirectUI::CollectionViewGroupGenerated::GetCustomProperty(_In_ HSTRING name, _Outptr_ ABI::Microsoft::UI::Xaml::Data::ICustomProperty** ppReturnValue)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "CollectionViewGroup_GetCustomProperty", 0);
    }
    ARG_NOTNULL(name, "name");
    ARG_VALIDRETURNPOINTER(ppReturnValue);
    *ppReturnValue={};
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<CollectionViewGroup*>(this)->GetCustomPropertyImpl(name, ppReturnValue));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "CollectionViewGroup_GetCustomProperty", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::CollectionViewGroupGenerated::GetIndexedProperty(_In_ HSTRING name, ABI::Windows::UI::Xaml::Interop::TypeName type, _Outptr_ ABI::Microsoft::UI::Xaml::Data::ICustomProperty** ppReturnValue)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "CollectionViewGroup_GetIndexedProperty", 0);
    }
    ARG_NOTNULL(name, "name");
    ARG_VALIDRETURNPOINTER(ppReturnValue);
    *ppReturnValue={};
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<CollectionViewGroup*>(this)->GetIndexedPropertyImpl(name, type, ppReturnValue));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "CollectionViewGroup_GetIndexedProperty", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::CollectionViewGroupGenerated::GetStringRepresentation(_Out_ HSTRING* pReturnValue)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "CollectionViewGroup_GetStringRepresentation", 0);
    }
    ARG_VALIDRETURNPOINTER(pReturnValue);
    *pReturnValue={};
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<CollectionViewGroup*>(this)->GetStringRepresentationImpl(pReturnValue));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "CollectionViewGroup_GetStringRepresentation", hr);
    }
    RRETURN(hr);
}


namespace DirectUI
{
}
