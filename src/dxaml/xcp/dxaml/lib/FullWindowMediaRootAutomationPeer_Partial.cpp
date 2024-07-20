// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FullWindowMediaRootAutomationPeer.g.h"
#include "FullWindowMediaRoot.g.h"
#include "FullWindowMediaRootAutomationPeer_Partial.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

IFACEMETHODIMP FullWindowMediaRootAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue)
{
    ctl::ComPtr<xaml::IUIElement> spOwner;

    IFCPTR_RETURN(ppReturnValue);
    *ppReturnValue = nullptr;

    IFC_RETURN(get_Owner(&spOwner));
    IFCPTR_RETURN(spOwner.Get());

    if (patternInterface == xaml_automation_peers::PatternInterface_Window)
    {
        *ppReturnValue = ctl::as_iinspectable(this);
        ctl::addref_interface(this);
    }
    else
    {
        IFC_RETURN(__super::GetPatternCore(patternInterface, ppReturnValue));
    }

    return S_OK;
}

IFACEMETHODIMP FullWindowMediaRootAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    IFCPTR_RETURN(returnValue);
    IFC_RETURN(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"FullWindowMedia")).CopyTo(returnValue));
    return S_OK;
}

IFACEMETHODIMP FullWindowMediaRootAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue)
{
    IFCPTR_RETURN(pReturnValue);
    *pReturnValue = xaml_automation_peers::AutomationControlType_Window;
    return S_OK;
}

_Check_return_ HRESULT FullWindowMediaRootAutomationPeer::get_IsModalImpl(_Out_ BOOLEAN* pValue)
{
    IFCPTR_RETURN(pValue);
    *pValue = TRUE;
    return S_OK;
}

_Check_return_ HRESULT FullWindowMediaRootAutomationPeer::get_IsTopmostImpl(_Out_ BOOLEAN* pValue)
{
    IFCPTR_RETURN(pValue);
    *pValue = TRUE;
    return S_OK;
}

_Check_return_ HRESULT FullWindowMediaRootAutomationPeer::get_MaximizableImpl(_Out_ BOOLEAN* pValue)
{
    IFCPTR_RETURN(pValue);
    *pValue = TRUE;
    return S_OK;
}

_Check_return_ HRESULT FullWindowMediaRootAutomationPeer::get_MinimizableImpl(_Out_ BOOLEAN* pValue)
{
    IFCPTR_RETURN(pValue);
    *pValue = FALSE;
    return S_OK;
}

_Check_return_ HRESULT FullWindowMediaRootAutomationPeer::get_InteractionStateImpl(_Out_ xaml_automation::WindowInteractionState* pValue)
{
    IFCPTR_RETURN(pValue);
    *pValue = xaml_automation::WindowInteractionState_Running;
    return S_OK;
}

_Check_return_ HRESULT FullWindowMediaRootAutomationPeer::get_VisualStateImpl(_Out_ xaml_automation::WindowVisualState* pValue)
{
    IFCPTR_RETURN(pValue);
    *pValue = xaml_automation::WindowVisualState_Maximized;
    return S_OK;
}

_Check_return_ HRESULT FullWindowMediaRootAutomationPeer::CloseImpl()
{
    return S_OK;
}

_Check_return_ HRESULT FullWindowMediaRootAutomationPeer::SetVisualStateImpl(_In_ xaml_automation::WindowVisualState state)
{
    return S_FALSE;
}

_Check_return_ HRESULT FullWindowMediaRootAutomationPeer::WaitForInputIdleImpl(_In_ INT milliseconds, _Out_ BOOLEAN* returnValue)
{
    return S_OK;
}

