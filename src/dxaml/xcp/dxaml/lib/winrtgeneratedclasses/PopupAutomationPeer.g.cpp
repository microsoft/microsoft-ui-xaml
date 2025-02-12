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

#include "PopupAutomationPeer.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::PopupAutomationPeerGenerated::PopupAutomationPeerGenerated()
{
}

DirectUI::PopupAutomationPeerGenerated::~PopupAutomationPeerGenerated()
{
}

HRESULT DirectUI::PopupAutomationPeerGenerated::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::PopupAutomationPeer)))
    {
        *ppObject = static_cast<DirectUI::PopupAutomationPeer*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Automation::Provider::IWindowProvider)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Automation::Provider::IWindowProvider*>(this);
    }
    else
    {
        RRETURN(DirectUI::FrameworkElementAutomationPeer::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
IFACEMETHODIMP DirectUI::PopupAutomationPeerGenerated::get_InteractionState(_Out_ ABI::Microsoft::UI::Xaml::Automation::WindowInteractionState* pValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(pValue);
    *pValue={};
    IFC(CheckThread());
    IFC(static_cast<PopupAutomationPeer*>(this)->get_InteractionStateImpl(pValue));
Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::PopupAutomationPeerGenerated::get_IsModal(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(pValue);
    *pValue={};
    IFC(CheckThread());
    IFC(static_cast<PopupAutomationPeer*>(this)->get_IsModalImpl(pValue));
Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::PopupAutomationPeerGenerated::get_IsTopmost(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(pValue);
    *pValue={};
    IFC(CheckThread());
    IFC(static_cast<PopupAutomationPeer*>(this)->get_IsTopmostImpl(pValue));
Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::PopupAutomationPeerGenerated::get_Maximizable(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(pValue);
    *pValue={};
    IFC(CheckThread());
    IFC(static_cast<PopupAutomationPeer*>(this)->get_MaximizableImpl(pValue));
Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::PopupAutomationPeerGenerated::get_Minimizable(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(pValue);
    *pValue={};
    IFC(CheckThread());
    IFC(static_cast<PopupAutomationPeer*>(this)->get_MinimizableImpl(pValue));
Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::PopupAutomationPeerGenerated::get_VisualState(_Out_ ABI::Microsoft::UI::Xaml::Automation::WindowVisualState* pValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(pValue);
    *pValue={};
    IFC(CheckThread());
    IFC(static_cast<PopupAutomationPeer*>(this)->get_VisualStateImpl(pValue));
Cleanup:
    RRETURN(hr);
}

// Events.

// Methods.
IFACEMETHODIMP DirectUI::PopupAutomationPeerGenerated::Close()
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "PopupAutomationPeer_Close", 0);
    }
    
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<PopupAutomationPeer*>(this)->CloseImpl());
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "PopupAutomationPeer_Close", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::PopupAutomationPeerGenerated::SetVisualState(ABI::Microsoft::UI::Xaml::Automation::WindowVisualState state)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "PopupAutomationPeer_SetVisualState", 0);
    }
    
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<PopupAutomationPeer*>(this)->SetVisualStateImpl(state));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "PopupAutomationPeer_SetVisualState", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::PopupAutomationPeerGenerated::WaitForInputIdle(INT milliseconds, _Out_ BOOLEAN* pReturnValue)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "PopupAutomationPeer_WaitForInputIdle", 0);
    }
    ARG_VALIDRETURNPOINTER(pReturnValue);
    *pReturnValue={};
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<PopupAutomationPeer*>(this)->WaitForInputIdleImpl(milliseconds, pReturnValue));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "PopupAutomationPeer_WaitForInputIdle", hr);
    }
    RRETURN(hr);
}


namespace DirectUI
{
}
