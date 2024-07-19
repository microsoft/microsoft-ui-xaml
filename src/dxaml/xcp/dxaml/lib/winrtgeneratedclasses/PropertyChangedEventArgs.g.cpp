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

#include "PropertyChangedEventArgs.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::PropertyChangedEventArgs::PropertyChangedEventArgs()
{
}

DirectUI::PropertyChangedEventArgs::~PropertyChangedEventArgs()
{
}

HRESULT DirectUI::PropertyChangedEventArgs::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::PropertyChangedEventArgs)))
    {
        *ppObject = static_cast<DirectUI::PropertyChangedEventArgs*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Data::IPropertyChangedEventArgs)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Data::IPropertyChangedEventArgs*>(this);
    }
    else
    {
        RRETURN(ctl::WeakReferenceSource::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
IFACEMETHODIMP DirectUI::PropertyChangedEventArgs::get_PropertyName(_Out_ HSTRING* pValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(pValue);
    IFC(CheckThread());
    IFC(m_propertyName.CopyTo(pValue));
Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::PropertyChangedEventArgs::put_PropertyName(_In_opt_ HSTRING value)
{
    HRESULT hr = S_OK;
    IFC(CheckThread());
    IFC(m_propertyName.Set(value));
Cleanup:
    RRETURN(hr);
}

// Events.

// Methods.

HRESULT DirectUI::PropertyChangedEventArgsFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Data::IPropertyChangedEventArgsFactory)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Data::IPropertyChangedEventArgsFactory*>(this);
    }
    else
    {
        RRETURN(ctl::AggregableActivationFactory<DirectUI::PropertyChangedEventArgs>::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Factory methods.
IFACEMETHODIMP DirectUI::PropertyChangedEventArgsFactory::CreateInstance(_In_opt_ HSTRING name, _In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Data::IPropertyChangedEventArgs** ppInstance)
{
    HRESULT hr = S_OK;
    
    ARG_VALIDRETURNPOINTER(ppInstance);
    IFC(CreateInstanceImpl(name, pOuter, ppInner, ppInstance));
Cleanup:
    return hr;
}

// Dependency properties.

// Attached properties.

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_PropertyChangedEventArgs()
    {
        RRETURN(ctl::ActivationFactoryCreator<PropertyChangedEventArgsFactory>::CreateActivationFactory());
    }
}
