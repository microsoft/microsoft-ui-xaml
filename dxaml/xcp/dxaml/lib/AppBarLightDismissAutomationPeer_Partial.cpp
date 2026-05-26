// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AppBarLightDismissAutomationPeer.g.h"
#include "AppBarLightDismiss.g.h"
#include "localizedResource.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

IFACEMETHODIMP AppBarLightDismissAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppReturnValue);
    *ppReturnValue = NULL;

    if (patternInterface == xaml_automation_peers::PatternInterface_Invoke)
    {
        *ppReturnValue = ctl::as_iinspectable(this);
        ctl::addref_interface(this);
    }
    else
    {
        IFC(AppBarLightDismissAutomationPeerGenerated::GetPatternCore(patternInterface, ppReturnValue));
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP AppBarLightDismissAutomationPeer::GetClassNameCore(_Out_ HSTRING* pReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pReturnValue);
    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"AppBarLightDismiss")).CopyTo(pReturnValue));
    
Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP AppBarLightDismissAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pReturnValue);
    *pReturnValue = xaml_automation_peers::AutomationControlType_Button;

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP AppBarLightDismissAutomationPeer::GetNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;
    IFCPTR(returnValue);
    IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_LIGHTDISMISS_NAME, returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP AppBarLightDismissAutomationPeer::GetAutomationIdCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(returnValue);
    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"Light Dismiss")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

// Support the IInvokeProvider interface.
_Check_return_ HRESULT AppBarLightDismissAutomationPeer::InvokeImpl()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IUIElement> spOwner;
    IFC(get_Owner(&spOwner));
    IFCPTR(spOwner.Get());
    IFC(spOwner.Cast<AppBarLightDismiss>()->AutomationClick());

Cleanup:
    RRETURN(hr);
}
