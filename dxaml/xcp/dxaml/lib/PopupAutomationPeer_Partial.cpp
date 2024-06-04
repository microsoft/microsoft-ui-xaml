// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PopupAutomationPeer.g.h"
#include "Popup.g.h"
#include "localizedResource.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Initializes a new instance of the PopupAutomationPeer class.
PopupAutomationPeer::PopupAutomationPeer()
{
}

// Deconstructor
PopupAutomationPeer::~PopupAutomationPeer()
{
}

IFACEMETHODIMP PopupAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue)
{
    ctl::ComPtr<xaml::IUIElement> spOwner;

    IFCPTR_RETURN(ppReturnValue);
    *ppReturnValue = NULL;

    IFC_RETURN(get_Owner(&spOwner));
    IFCPTR_RETURN(spOwner.Get());

    if (patternInterface == xaml_automation_peers::PatternInterface_Window)
    {
        bool shouldExposeWindowPattern = false;

        IFC_RETURN(spOwner.Cast<Popup>()->GetShouldUIAPeerExposeWindowPattern(&shouldExposeWindowPattern));

        if (shouldExposeWindowPattern)
        {
            *ppReturnValue = ctl::as_iinspectable(this);
            ctl::addref_interface(this);
        }
        else
        {
            IFC_RETURN(PopupAutomationPeerGenerated::GetPatternCore(patternInterface, ppReturnValue));
        }
    }
    else
    {
        IFC_RETURN(PopupAutomationPeerGenerated::GetPatternCore(patternInterface, ppReturnValue));
    }

    return S_OK;
}

IFACEMETHODIMP PopupAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;
    IFCPTR(returnValue);
    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"Popup")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP PopupAutomationPeer::GetNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString strAutomationName;
    IFCPTR(returnValue);
    *returnValue = NULL;

    IFC(PopupAutomationPeerGenerated::GetNameCore(returnValue));
    if (*returnValue == NULL)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_POPUP_NAME, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(strAutomationName.CopyTo(returnValue));
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP PopupAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue)
{
    HRESULT hr = S_OK;
    IFCPTR(pReturnValue);
    *pReturnValue = xaml_automation_peers::AutomationControlType_Window;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::PopupAutomationPeer::get_IsModalImpl(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IUIElement> spOwner;

    BOOLEAN bIsEnabled;

    IFCPTR(pValue);
    *pValue = TRUE;
    IFC(IsEnabled(&bIsEnabled));
    if (!bIsEnabled)
    {
        IFC(static_cast<HRESULT>(UIA_E_ELEMENTNOTENABLED));
    }

    IFC(get_Owner(&spOwner));
    IFCPTR(spOwner.Get());
    IFC(spOwner.Cast<Popup>()->get_IsOpen(pValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::PopupAutomationPeer::get_IsTopmostImpl(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IUIElement> spOwner;

    BOOLEAN bIsEnabled;

    IFCPTR(pValue);
    *pValue = TRUE;

    IFC(IsEnabled(&bIsEnabled));
    if (!bIsEnabled)
    {
        IFC(static_cast<HRESULT>(UIA_E_ELEMENTNOTENABLED));
    }

    IFC(get_Owner(&spOwner));
    IFCPTR(spOwner.Get());
    IFC(spOwner.Cast<Popup>()->get_IsOpen(pValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::PopupAutomationPeer::get_MaximizableImpl(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    IFCPTR(pValue);
    *pValue = FALSE;
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::PopupAutomationPeer::get_MinimizableImpl(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    IFCPTR(pValue);
    *pValue = FALSE;
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::PopupAutomationPeer::get_InteractionStateImpl(_Out_ xaml_automation::WindowInteractionState* pValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IUIElement> spOwner;
    BOOLEAN bIsEnabled;
    BOOLEAN bIsOpen;

    IFCPTR(pValue);
    *pValue = xaml_automation::WindowInteractionState_Running;

    IFC(IsEnabled(&bIsEnabled));
    if (!bIsEnabled)
    {
        IFC(static_cast<HRESULT>(UIA_E_ELEMENTNOTENABLED));
    }

    IFC(get_Owner(&spOwner));
    IFCPTR(spOwner.Get());
    IFC(spOwner.Cast<Popup>()->get_IsOpen(&bIsOpen));

    if (bIsOpen)
    {
        *pValue = xaml_automation::WindowInteractionState_Running;
    }
    else
    {
        *pValue = xaml_automation::WindowInteractionState_Closing;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::PopupAutomationPeer::get_VisualStateImpl(_Out_ xaml_automation::WindowVisualState* pValue)
{
    HRESULT hr = S_OK;
    IFCPTR(pValue);
    *pValue = xaml_automation::WindowVisualState_Normal;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT PopupAutomationPeer::CloseImpl()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IUIElement> spOwner;
    BOOLEAN bIsEnabled;

    IFC(IsEnabled(&bIsEnabled));
    if (!bIsEnabled)
    {
        IFC(static_cast<HRESULT>(UIA_E_ELEMENTNOTENABLED));
    }

    IFC(get_Owner(&spOwner));
    IFCPTR(spOwner.Get());
    IFC(spOwner.Cast<Popup>()->LightDismiss(xaml::FocusState_Pointer));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT PopupAutomationPeer::SetVisualStateImpl(_In_ xaml_automation::WindowVisualState state)
{
    RRETURN(S_FALSE);
}

_Check_return_ HRESULT PopupAutomationPeer::WaitForInputIdleImpl(_In_ INT milliseconds, _Out_ BOOLEAN* returnValue)
{
    RRETURN(S_OK);
}
