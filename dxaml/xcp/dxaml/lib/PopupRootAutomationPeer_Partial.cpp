// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PopupRootAutomationPeer.g.h"
#include "PopupRoot.g.h"
#include "Popup.g.h"
#include "localizedResource.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Initializes a new instance of the PopupRootAutomationPeer class.
PopupRootAutomationPeer::PopupRootAutomationPeer()
{
}

// Deconstructor
PopupRootAutomationPeer::~PopupRootAutomationPeer()
{
}

IFACEMETHODIMP PopupRootAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** returnValue)
{
    BOOLEAN isEnabled;

    IFC_RETURN(IsEnabled(&isEnabled));
    if (!isEnabled)
    {
        IFC_RETURN(static_cast<HRESULT>(UIA_E_ELEMENTNOTENABLED));
    }

    ctl::ComPtr<xaml::IUIElement> owner;
    BOOLEAN isLightDismiss;

    IFC_RETURN(get_Owner(&owner));
    IFC_RETURN(owner.Cast<PopupRoot>()->IsTopmostPopupInLightDismissChain(&isLightDismiss));

    if (isLightDismiss && patternInterface == xaml_automation_peers::PatternInterface_Invoke)
    {
        *returnValue = ctl::as_iinspectable(this);
        ctl::addref_interface(this);
    }
    else
    {
        IFC_RETURN(PopupRootAutomationPeerGenerated::GetPatternCore(patternInterface, returnValue));
    }

    return S_OK;
}

IFACEMETHODIMP PopupRootAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    RRETURN(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"PopupRoot")).CopyTo(returnValue));
}

IFACEMETHODIMP PopupRootAutomationPeer::GetNameCore(_Out_ HSTRING* returnValue)
{
    ctl::ComPtr<xaml::IUIElement> owner;
    BOOLEAN bLightDismiss;

    IFCPTR_RETURN(returnValue);
    IFC_RETURN(get_Owner(&owner));
    IFC_RETURN(owner.Cast<PopupRoot>()->IsTopmostPopupInLightDismissChain(&bLightDismiss));

    if (bLightDismiss)
    {
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_LIGHTDISMISS_NAME, returnValue));
    }
    else
    {
        return S_FALSE;
    }

    return S_OK;
}

IFACEMETHODIMP PopupRootAutomationPeer::GetAutomationIdCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(returnValue);
    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"Light Dismiss")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP PopupRootAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* returnValue)
{
    ctl::ComPtr<xaml::IUIElement> owner;
    BOOLEAN bLightDismiss;

    IFCPTR_RETURN(returnValue);
    IFC_RETURN(get_Owner(&owner));
    IFC_RETURN(owner.Cast<PopupRoot>()->IsTopmostPopupInLightDismissChain(&bLightDismiss));

    if (bLightDismiss)
    {
        *returnValue = xaml_automation_peers::AutomationControlType_Button;
    }
    else
    {
        return S_FALSE;
    }

    return S_OK;
}

IFACEMETHODIMP PopupRootAutomationPeer::IsControlElementCore(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    IFCPTR(pValue);
    *pValue = TRUE;
Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP PopupRootAutomationPeer::IsContentElementCore(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    IFCPTR(pValue);
    *pValue = TRUE;
Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP PopupRootAutomationPeer::GetChildrenCore(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** /*returnValue*/)
{
    RRETURN(S_OK);
}

_Check_return_ HRESULT PopupRootAutomationPeer::InvokeImpl()
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;
    BOOLEAN bIsEnabled;

    IFC(IsEnabled(&bIsEnabled));
    if (!bIsEnabled)
    {
        IFC(static_cast<HRESULT>(UIA_E_ELEMENTNOTENABLED));
    }

    IFC(get_Owner(&pOwner));
    IFC(static_cast<PopupRoot*>(pOwner)->CloseTopmostPopup());

Cleanup:
    ReleaseInterface(pOwner);
    RRETURN(hr);
}


_Check_return_ HRESULT PopupRootAutomationPeer::GetFlowsFromCoreImpl(_Outptr_ wfc::IIterable<xaml_automation_peers::AutomationPeer*>** returnValue)
{
    ctl::ComPtr<TrackerCollection<xaml_automation_peers::AutomationPeer*>> spPeers;
    ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAP;

    IFC_RETURN(ctl::make(&spPeers));
    IFC_RETURN(GetLightDismissingPopupAP(&spAP));
    IFC_RETURN(spPeers->Append(spAP.Get()));

    *returnValue = spPeers.Detach();
    return S_OK;
}

_Check_return_ HRESULT PopupRootAutomationPeer::GetFlowsToCoreImpl(_Outptr_ wfc::IIterable<xaml_automation_peers::AutomationPeer*>** returnValue)
{
    ctl::ComPtr<TrackerCollection<xaml_automation_peers::AutomationPeer*>> spPeers;
    ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAP;

    IFC_RETURN(ctl::make(&spPeers));
    IFC_RETURN(GetLightDismissingPopupAP(&spAP));
    IFC_RETURN(spPeers->Append(spAP.Get()));

    *returnValue = spPeers.Detach();
    return S_OK;
}

_Check_return_ HRESULT PopupRootAutomationPeer::GetLightDismissingPopupAP(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IUIElement> spOwner;
    CDependencyObject* pLightDismissingPopupDO = NULL;
    ctl::ComPtr<DependencyObject> spLightDismissingPopup;
    BOOLEAN bIsEnabled;

    IFC(IsEnabled(&bIsEnabled));
    if (!bIsEnabled)
    {
        IFC(static_cast<HRESULT>(UIA_E_ELEMENTNOTENABLED));
    }

    IFC(get_Owner(&spOwner));
    IFC(static_cast<CPopupRoot*>(spOwner.Cast<PopupRoot>()->GetHandle())->GetTopmostPopupInLightDismissChain(&pLightDismissingPopupDO));
    if (pLightDismissingPopupDO)
    {
        IFC(DXamlCore::GetCurrent()->GetPeer(pLightDismissingPopupDO, &spLightDismissingPopup));
    }

    if (spLightDismissingPopup)
    {
        IFC(spLightDismissingPopup.Cast<Popup>()->GetOrCreateAutomationPeer(returnValue));
    }

Cleanup:
    ReleaseInterface(pLightDismissingPopupDO);
    RRETURN(hr);
}
