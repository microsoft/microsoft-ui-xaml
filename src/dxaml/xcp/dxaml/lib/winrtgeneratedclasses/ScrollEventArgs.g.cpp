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

#include "precomp.h"
#include "ScrollEventArgs.g.h"
#include "CoreEventArgsGroup.h"

using namespace DirectUI;

// Constructors/destructors.
DirectUI::ScrollEventArgsGenerated::ScrollEventArgsGenerated()
{
}

DirectUI::ScrollEventArgsGenerated::~ScrollEventArgsGenerated()
{
}

HRESULT DirectUI::ScrollEventArgsGenerated::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::ScrollEventArgs)))
    {
        *ppObject = static_cast<DirectUI::ScrollEventArgs*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::Primitives::IScrollEventArgs)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::Primitives::IScrollEventArgs*>(this);
    }
    else
    {
        RRETURN(DirectUI::RoutedEventArgs::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Properties.
IFACEMETHODIMP DirectUI::ScrollEventArgsGenerated::get_NewValue(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(pValue);
    *pValue={};
    IFC(CheckThread());
    IFC(static_cast<ScrollEventArgs*>(this)->get_NewValueImpl(pValue));
Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::ScrollEventArgsGenerated::put_NewValue(DOUBLE value)
{
    HRESULT hr = S_OK;
    
    IFC(CheckThread());
    IFC(static_cast<ScrollEventArgs*>(this)->put_NewValueImpl(value));
Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::ScrollEventArgsGenerated::get_ScrollEventType(_Out_ ABI::Microsoft::UI::Xaml::Controls::Primitives::ScrollEventType* pValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(pValue);
    *pValue={};
    IFC(CheckThread());
    IFC(static_cast<ScrollEventArgs*>(this)->get_ScrollEventTypeImpl(pValue));
Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::ScrollEventArgsGenerated::put_ScrollEventType(ABI::Microsoft::UI::Xaml::Controls::Primitives::ScrollEventType value)
{
    HRESULT hr = S_OK;
    
    IFC(CheckThread());
    IFC(static_cast<ScrollEventArgs*>(this)->put_ScrollEventTypeImpl(value));
Cleanup:
    RRETURN(hr);
}

// Methods.


namespace DirectUI
{
    _Check_return_ HRESULT OnFrameworkCreateScrollEventArgs(_In_ CEventArgs* pCoreObject, _Out_ IInspectable** ppNewInstance)
    {
        HRESULT hr = S_OK;
        ctl::ComPtr<DirectUI::ScrollEventArgs> spInstance;
        *ppNewInstance = nullptr;
        IFC(ctl::make(pCoreObject, &spInstance));
        *ppNewInstance = ctl::iinspectable_cast(spInstance.Detach());
    Cleanup:
        RRETURN(hr);
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_ScrollEventArgs()
    {
        RRETURN(ctl::ActivationFactoryCreator<ctl::ActivationFactory<DirectUI::ScrollEventArgs>>::CreateActivationFactory());
    }
}
