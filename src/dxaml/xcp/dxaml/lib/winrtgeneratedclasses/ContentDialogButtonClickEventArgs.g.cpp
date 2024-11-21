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
#include "ContentDialogButtonClickEventArgs.g.h"
#include "CoreEventArgsGroup.h"

using namespace DirectUI;

// Constructors/destructors.
DirectUI::ContentDialogButtonClickEventArgsGenerated::ContentDialogButtonClickEventArgsGenerated(): m_cancel()
{
}

DirectUI::ContentDialogButtonClickEventArgsGenerated::~ContentDialogButtonClickEventArgsGenerated()
{
}

HRESULT DirectUI::ContentDialogButtonClickEventArgsGenerated::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::ContentDialogButtonClickEventArgs)))
    {
        *ppObject = static_cast<DirectUI::ContentDialogButtonClickEventArgs*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::IContentDialogButtonClickEventArgs)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::IContentDialogButtonClickEventArgs*>(this);
    }
    else
    {
        RRETURN(DirectUI::EventArgs::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Properties.
IFACEMETHODIMP DirectUI::ContentDialogButtonClickEventArgsGenerated::get_Cancel(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(pValue);
    IFC(CheckThread());
    IFC(CValueBoxer::CopyValue(m_cancel, pValue));
Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::ContentDialogButtonClickEventArgsGenerated::put_Cancel(BOOLEAN value)
{
    HRESULT hr = S_OK;
    IFC(CheckThread());
    IFC(CValueBoxer::CopyValue(value, &m_cancel));
Cleanup:
    RRETURN(hr);
}

// Methods.
IFACEMETHODIMP DirectUI::ContentDialogButtonClickEventArgsGenerated::GetDeferral(_Outptr_ ABI::Microsoft::UI::Xaml::Controls::IContentDialogButtonClickDeferral** ppReturnValue)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "ContentDialogButtonClickEventArgs_GetDeferral", 0);
    }
    ARG_VALIDRETURNPOINTER(ppReturnValue);
    *ppReturnValue={};
    IFC(CheckThread());
    IFC(static_cast<ContentDialogButtonClickEventArgs*>(this)->GetDeferralImpl(ppReturnValue));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "ContentDialogButtonClickEventArgs_GetDeferral", hr);
    }
    RRETURN(hr);
}


namespace DirectUI
{
    _Check_return_ HRESULT OnFrameworkCreateContentDialogButtonClickEventArgs(_In_ CEventArgs* pCoreObject, _Out_ IInspectable** ppNewInstance)
    {
        HRESULT hr = S_OK;
        ctl::ComPtr<DirectUI::ContentDialogButtonClickEventArgs> spInstance;
        *ppNewInstance = nullptr;
        IFC(ctl::make(pCoreObject, &spInstance));
        *ppNewInstance = ctl::iinspectable_cast(spInstance.Detach());
    Cleanup:
        RRETURN(hr);
    }
    _Check_return_ IActivationFactory* CreateActivationFactory_ContentDialogButtonClickEventArgs()
    {
        RRETURN(ctl::ActivationFactoryCreator<ctl::AbstractActivationFactory>::CreateActivationFactory());
    }
}
