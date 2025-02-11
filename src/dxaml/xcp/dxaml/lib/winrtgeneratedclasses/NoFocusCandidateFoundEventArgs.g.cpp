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
#include "NoFocusCandidateFoundEventArgs.g.h"
#include "CoreEventArgsGroup.h"

using namespace DirectUI;

// Constructors/destructors.
DirectUI::NoFocusCandidateFoundEventArgs::NoFocusCandidateFoundEventArgs()
{
}

DirectUI::NoFocusCandidateFoundEventArgs::~NoFocusCandidateFoundEventArgs()
{
}

HRESULT DirectUI::NoFocusCandidateFoundEventArgs::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::NoFocusCandidateFoundEventArgs)))
    {
        *ppObject = static_cast<DirectUI::NoFocusCandidateFoundEventArgs*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Input::INoFocusCandidateFoundEventArgs)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Input::INoFocusCandidateFoundEventArgs*>(this);
    }
    else
    {
        RRETURN(DirectUI::RoutedEventArgs::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Properties.
IFACEMETHODIMP DirectUI::NoFocusCandidateFoundEventArgs::get_Direction(_Out_ ABI::Microsoft::UI::Xaml::Input::FocusNavigationDirection* pValue)
{
    HRESULT hr = S_OK;
    CEventArgs* pCoreEventArgsNoRef = nullptr;

    DirectUI::FocusNavigationDirection valueCore;

    ARG_VALIDRETURNPOINTER(pValue);
    *pValue={};

    IFC(GetCorePeerNoRefWithValidation(&pCoreEventArgsNoRef));
    IFC(static_cast<CNoFocusCandidateFoundEventArgs*>(pCoreEventArgsNoRef)->get_Direction(&valueCore));

    IFC(CValueBoxer::ConvertToFramework(valueCore, pValue, /* fReleaseCoreValue */ TRUE));

Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::NoFocusCandidateFoundEventArgs::get_Handled(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    CEventArgs* pCoreEventArgsNoRef = nullptr;

    BOOLEAN valueCore;

    ARG_VALIDRETURNPOINTER(pValue);
    *pValue={};

    IFC(GetCorePeerNoRefWithValidation(&pCoreEventArgsNoRef));
    IFC(static_cast<CNoFocusCandidateFoundEventArgs*>(pCoreEventArgsNoRef)->get_Handled(&valueCore));

    IFC(CValueBoxer::ConvertToFramework(valueCore, pValue, /* fReleaseCoreValue */ TRUE));

Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::NoFocusCandidateFoundEventArgs::put_Handled(BOOLEAN value)
{
    HRESULT hr = S_OK;
    CEventArgs* pCoreEventArgsNoRef = nullptr;

    BOOLEAN valueCore = value;

    

    IFC(GetCorePeerNoRefWithValidation(&pCoreEventArgsNoRef));
    IFC(static_cast<CNoFocusCandidateFoundEventArgs*>(pCoreEventArgsNoRef)->put_Handled(valueCore));


Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::NoFocusCandidateFoundEventArgs::get_InputDevice(_Out_ ABI::Microsoft::UI::Xaml::Input::FocusInputDeviceKind* pValue)
{
    HRESULT hr = S_OK;
    CEventArgs* pCoreEventArgsNoRef = nullptr;

    DirectUI::FocusInputDeviceKind valueCore;

    ARG_VALIDRETURNPOINTER(pValue);
    *pValue={};

    IFC(GetCorePeerNoRefWithValidation(&pCoreEventArgsNoRef));
    IFC(static_cast<CNoFocusCandidateFoundEventArgs*>(pCoreEventArgsNoRef)->get_InputDevice(&valueCore));

    IFC(CValueBoxer::ConvertToFramework(valueCore, pValue, /* fReleaseCoreValue */ TRUE));

Cleanup:
    RRETURN(hr);
}

// Methods.


namespace DirectUI
{
    _Check_return_ HRESULT OnFrameworkCreateNoFocusCandidateFoundEventArgs(_In_ CEventArgs* pCoreObject, _Out_ IInspectable** ppNewInstance)
    {
        HRESULT hr = S_OK;
        ctl::ComPtr<DirectUI::NoFocusCandidateFoundEventArgs> spInstance;
        *ppNewInstance = nullptr;
        IFC(ctl::make(pCoreObject, &spInstance));
        *ppNewInstance = ctl::iinspectable_cast(spInstance.Detach());
    Cleanup:
        RRETURN(hr);
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_NoFocusCandidateFoundEventArgs()
    {
        RRETURN(ctl::ActivationFactoryCreator<ctl::AbstractActivationFactory>::CreateActivationFactory());
    }
}
