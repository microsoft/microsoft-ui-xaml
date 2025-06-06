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

#include "NotifyCollectionChangedEventArgs.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::NotifyCollectionChangedEventArgs::NotifyCollectionChangedEventArgs(): m_action(), m_newStartingIndex(), m_oldStartingIndex()
{
}

DirectUI::NotifyCollectionChangedEventArgs::~NotifyCollectionChangedEventArgs()
{
}

HRESULT DirectUI::NotifyCollectionChangedEventArgs::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::NotifyCollectionChangedEventArgs)))
    {
        *ppObject = static_cast<DirectUI::NotifyCollectionChangedEventArgs*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Interop::INotifyCollectionChangedEventArgs)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Interop::INotifyCollectionChangedEventArgs*>(this);
    }
    else
    {
        RRETURN(ctl::WeakReferenceSource::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
IFACEMETHODIMP DirectUI::NotifyCollectionChangedEventArgs::get_Action(_Out_ ABI::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedAction* pValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(pValue);
    IFC(CheckThread());
    IFC(CValueBoxer::CopyValue(m_action, pValue));
Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::NotifyCollectionChangedEventArgs::put_Action(ABI::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedAction value)
{
    HRESULT hr = S_OK;
    IFC(CheckThread());
    IFC(CValueBoxer::CopyValue(value, &m_action));
Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::NotifyCollectionChangedEventArgs::get_NewItems(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Interop::IBindableVector** ppValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC(CheckThread());
    IFC(m_pNewItems.CopyTo(ppValue));
Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::NotifyCollectionChangedEventArgs::put_NewItems(_In_opt_ ABI::Microsoft::UI::Xaml::Interop::IBindableVector* pValue)
{
    HRESULT hr = S_OK;
    IFC(CheckThread());
    SetPtrValue(m_pNewItems, pValue);
Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::NotifyCollectionChangedEventArgs::get_NewStartingIndex(_Out_ INT* pValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(pValue);
    IFC(CheckThread());
    IFC(CValueBoxer::CopyValue(m_newStartingIndex, pValue));
Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::NotifyCollectionChangedEventArgs::put_NewStartingIndex(INT value)
{
    HRESULT hr = S_OK;
    IFC(CheckThread());
    IFC(CValueBoxer::CopyValue(value, &m_newStartingIndex));
Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::NotifyCollectionChangedEventArgs::get_OldItems(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Interop::IBindableVector** ppValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC(CheckThread());
    IFC(m_pOldItems.CopyTo(ppValue));
Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::NotifyCollectionChangedEventArgs::put_OldItems(_In_opt_ ABI::Microsoft::UI::Xaml::Interop::IBindableVector* pValue)
{
    HRESULT hr = S_OK;
    IFC(CheckThread());
    SetPtrValue(m_pOldItems, pValue);
Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::NotifyCollectionChangedEventArgs::get_OldStartingIndex(_Out_ INT* pValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(pValue);
    IFC(CheckThread());
    IFC(CValueBoxer::CopyValue(m_oldStartingIndex, pValue));
Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::NotifyCollectionChangedEventArgs::put_OldStartingIndex(INT value)
{
    HRESULT hr = S_OK;
    IFC(CheckThread());
    IFC(CValueBoxer::CopyValue(value, &m_oldStartingIndex));
Cleanup:
    RRETURN(hr);
}

// Events.

// Methods.

HRESULT DirectUI::NotifyCollectionChangedEventArgsFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Interop::INotifyCollectionChangedEventArgsFactory)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Interop::INotifyCollectionChangedEventArgsFactory*>(this);
    }
    else
    {
        RRETURN(ctl::AggregableActivationFactory<DirectUI::NotifyCollectionChangedEventArgs>::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Factory methods.
IFACEMETHODIMP DirectUI::NotifyCollectionChangedEventArgsFactory::CreateInstanceWithAllParameters(ABI::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedAction action, _In_opt_ ABI::Microsoft::UI::Xaml::Interop::IBindableVector* pNewItems, _In_opt_ ABI::Microsoft::UI::Xaml::Interop::IBindableVector* pOldItems, INT newIndex, INT oldIndex, _In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Interop::INotifyCollectionChangedEventArgs** ppInstance)
{
    HRESULT hr = S_OK;
    
    ARG_VALIDRETURNPOINTER(ppInstance);
    IFC(CreateInstanceWithAllParametersImpl(action, pNewItems, pOldItems, newIndex, oldIndex, pOuter, ppInner, ppInstance));
Cleanup:
    return hr;
}

// Dependency properties.

// Attached properties.

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_NotifyCollectionChangedEventArgs()
    {
        RRETURN(ctl::ActivationFactoryCreator<NotifyCollectionChangedEventArgsFactory>::CreateActivationFactory());
    }
}
