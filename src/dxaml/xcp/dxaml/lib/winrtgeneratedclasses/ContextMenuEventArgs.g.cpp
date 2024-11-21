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
#include "ContextMenuEventArgs.g.h"
#include "CoreEventArgsGroup.h"

using namespace DirectUI;

// Constructors/destructors.
DirectUI::ContextMenuEventArgsGenerated::ContextMenuEventArgsGenerated()
{
}

DirectUI::ContextMenuEventArgsGenerated::~ContextMenuEventArgsGenerated()
{
}

HRESULT DirectUI::ContextMenuEventArgsGenerated::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::ContextMenuEventArgs)))
    {
        *ppObject = static_cast<DirectUI::ContextMenuEventArgs*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::IContextMenuEventArgs)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::IContextMenuEventArgs*>(this);
    }
    else
    {
        RRETURN(DirectUI::RoutedEventArgs::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Properties.
IFACEMETHODIMP DirectUI::ContextMenuEventArgsGenerated::get_Handled(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    CEventArgs* pCoreEventArgsNoRef = nullptr;

    BOOLEAN valueCore;

    ARG_VALIDRETURNPOINTER(pValue);
    *pValue={};

    IFC(GetCorePeerNoRefWithValidation(&pCoreEventArgsNoRef));
    IFC(static_cast<CContextMenuEventArgs*>(pCoreEventArgsNoRef)->get_Handled(&valueCore));

    IFC(CValueBoxer::ConvertToFramework(valueCore, pValue, /* fReleaseCoreValue */ TRUE));

Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::ContextMenuEventArgsGenerated::put_Handled(BOOLEAN value)
{
    HRESULT hr = S_OK;
    CEventArgs* pCoreEventArgsNoRef = nullptr;

    BOOLEAN valueCore = value;

    

    IFC(GetCorePeerNoRefWithValidation(&pCoreEventArgsNoRef));
    IFC(static_cast<CContextMenuEventArgs*>(pCoreEventArgsNoRef)->put_Handled(valueCore));


Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::ContextMenuEventArgsGenerated::get_CursorLeft(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    CEventArgs* pCoreEventArgsNoRef = nullptr;

    DOUBLE valueCore;

    ARG_VALIDRETURNPOINTER(pValue);
    *pValue={};

    IFC(GetCorePeerNoRefWithValidation(&pCoreEventArgsNoRef));
    IFC(static_cast<CContextMenuEventArgs*>(pCoreEventArgsNoRef)->get_CursorLeft(&valueCore));

    IFC(CValueBoxer::ConvertToFramework(valueCore, pValue, /* fReleaseCoreValue */ TRUE));

Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::ContextMenuEventArgsGenerated::put_CursorLeft(DOUBLE value)
{
    HRESULT hr = S_OK;
    CEventArgs* pCoreEventArgsNoRef = nullptr;

    DOUBLE valueCore = value;

    

    IFC(GetCorePeerNoRefWithValidation(&pCoreEventArgsNoRef));
    IFC(static_cast<CContextMenuEventArgs*>(pCoreEventArgsNoRef)->put_CursorLeft(valueCore));


Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::ContextMenuEventArgsGenerated::get_CursorTop(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    CEventArgs* pCoreEventArgsNoRef = nullptr;

    DOUBLE valueCore;

    ARG_VALIDRETURNPOINTER(pValue);
    *pValue={};

    IFC(GetCorePeerNoRefWithValidation(&pCoreEventArgsNoRef));
    IFC(static_cast<CContextMenuEventArgs*>(pCoreEventArgsNoRef)->get_CursorTop(&valueCore));

    IFC(CValueBoxer::ConvertToFramework(valueCore, pValue, /* fReleaseCoreValue */ TRUE));

Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::ContextMenuEventArgsGenerated::put_CursorTop(DOUBLE value)
{
    HRESULT hr = S_OK;
    CEventArgs* pCoreEventArgsNoRef = nullptr;

    DOUBLE valueCore = value;

    

    IFC(GetCorePeerNoRefWithValidation(&pCoreEventArgsNoRef));
    IFC(static_cast<CContextMenuEventArgs*>(pCoreEventArgsNoRef)->put_CursorTop(valueCore));


Cleanup:
    RRETURN(hr);
}

// Methods.


namespace DirectUI
{
    _Check_return_ HRESULT OnFrameworkCreateContextMenuEventArgs(_In_ CEventArgs* pCoreObject, _Out_ IInspectable** ppNewInstance)
    {
        HRESULT hr = S_OK;
        ctl::ComPtr<DirectUI::ContextMenuEventArgs> spInstance;
        *ppNewInstance = nullptr;
        IFC(ctl::make(pCoreObject, &spInstance));
        *ppNewInstance = ctl::iinspectable_cast(spInstance.Detach());
    Cleanup:
        RRETURN(hr);
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_ContextMenuEventArgs()
    {
        RRETURN(ctl::ActivationFactoryCreator<ctl::AbstractActivationFactory>::CreateActivationFactory());
    }
}
