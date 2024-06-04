// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MenuFlyoutSubItemAutomationPeer.g.h"
#include "MenuFlyoutSubItem.g.h"
#include "MenuFlyoutPresenter.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;


IFACEMETHODIMP MenuFlyoutSubItemAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue)
{
    IFCPTR_RETURN(ppReturnValue);
    *ppReturnValue = nullptr;

    if (patternInterface == xaml_automation_peers::PatternInterface_ExpandCollapse)
    {
        *ppReturnValue = ctl::as_iinspectable(this);
        ctl::addref_interface(this);
    }
    else
    {
        IFC_RETURN(MenuFlyoutSubItemAutomationPeerGenerated::GetPatternCore(patternInterface, ppReturnValue));
    }
    return S_OK;
}

IFACEMETHODIMP MenuFlyoutSubItemAutomationPeer::GetClassNameCore(_Out_ HSTRING* pReturnValue)
{
    IFCPTR_RETURN(pReturnValue);
    IFC_RETURN(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"MenuFlyoutSubItem")).CopyTo(pReturnValue));

    return S_OK;
}

IFACEMETHODIMP MenuFlyoutSubItemAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue)
{
    IFCPTR_RETURN(pReturnValue);
    *pReturnValue = xaml_automation_peers::AutomationControlType_MenuItem;
    return S_OK;
}

_Check_return_ HRESULT DirectUI::MenuFlyoutSubItemAutomationPeer::GetPositionInSetCoreImpl(_Out_ INT* returnValue)
{
    IFCPTR_RETURN(returnValue);
    *returnValue = -1;

    // First retrieve any valid value being directly set on the container, that value will get precedence.
    IFC_RETURN(MenuFlyoutSubItemAutomationPeerGenerated::GetPositionInSetCoreImpl(returnValue));

    // if it still is default value, calculate it ourselves.
    if (*returnValue == -1)
    {
        ctl::ComPtr<xaml::IUIElement> owner;
        IFC_RETURN(get_Owner(&owner));
        IFC_RETURN(MenuFlyoutPresenter::GetPositionInSetHelper(owner.AsOrNull<IMenuFlyoutItemBase>(), returnValue));    
    }
    
    return S_OK;
}

_Check_return_ HRESULT DirectUI::MenuFlyoutSubItemAutomationPeer::GetSizeOfSetCoreImpl(_Out_ INT* returnValue)
{
    IFCPTR_RETURN(returnValue);
    *returnValue = -1;

    // First retrieve any valid value being directly set on the container, that value will get precedence.
    IFC_RETURN(MenuFlyoutSubItemAutomationPeerGenerated::GetSizeOfSetCoreImpl(returnValue));

    // if it still is default value, calculate it ourselves.
    if (*returnValue == -1)
    {
        ctl::ComPtr<xaml::IUIElement> owner;
        IFC_RETURN(get_Owner(&owner));
        IFC_RETURN(MenuFlyoutPresenter::GetSizeOfSetHelper(owner.AsOrNull<IMenuFlyoutItemBase>(), returnValue));
    }
    
    return S_OK;
}

// IExpandCollapseProvider
_Check_return_ HRESULT MenuFlyoutSubItemAutomationPeer::ExpandImpl()
{
    ctl::ComPtr<xaml::IUIElement> spOwner;
    IFC_RETURN(get_Owner(&spOwner));

    IFC_RETURN(spOwner.Cast<MenuFlyoutSubItem>()->Open());
    return S_OK;
}

_Check_return_ HRESULT MenuFlyoutSubItemAutomationPeer::CollapseImpl()
{
    ctl::ComPtr<xaml::IUIElement> spOwner;
    IFC_RETURN(get_Owner(&spOwner));

    IFC_RETURN(spOwner.Cast<MenuFlyoutSubItem>()->Close());
    return S_OK;
}

_Check_return_ HRESULT MenuFlyoutSubItemAutomationPeer::get_ExpandCollapseStateImpl(_Out_ xaml_automation::ExpandCollapseState* pReturnValue)
{
    IFCPTR_RETURN(pReturnValue);
    
    ctl::ComPtr<xaml::IUIElement> spOwner;
    IFC_RETURN(get_Owner(&spOwner));

    BOOLEAN isOpen = FALSE;
    IFC_RETURN(spOwner.Cast<MenuFlyoutSubItem>()->get_IsOpen(&isOpen));

    *pReturnValue = isOpen ? xaml_automation::ExpandCollapseState_Expanded : xaml_automation::ExpandCollapseState_Collapsed;
    return S_OK;
}

// Raise events for ExpandCollapseState changes to UIAutomation Clients.
_Check_return_ HRESULT MenuFlyoutSubItemAutomationPeer::RaiseExpandCollapseAutomationEvent(_In_ BOOLEAN isOpen)
{
    xaml_automation::ExpandCollapseState oldValue;
    xaml_automation::ExpandCollapseState newValue;
    CValue valueOld;
    CValue valueNew;

    // Converting isOpen to appropriate enumerations
    if (isOpen)
    {
        oldValue = xaml_automation::ExpandCollapseState::ExpandCollapseState_Collapsed;
        newValue = xaml_automation::ExpandCollapseState::ExpandCollapseState_Expanded;
    }
    else
    {
        oldValue = xaml_automation::ExpandCollapseState::ExpandCollapseState_Expanded;
        newValue = xaml_automation::ExpandCollapseState::ExpandCollapseState_Collapsed;
    }

    IFC_RETURN(CValueBoxer::BoxEnumValue(&valueOld, oldValue));
    IFC_RETURN(CValueBoxer::BoxEnumValue(&valueNew, newValue));
    IFC_RETURN(CoreImports::AutomationRaiseAutomationPropertyChanged(static_cast<CAutomationPeer*>(GetHandle()), UIAXcp::APAutomationProperties::APExpandCollapseStateProperty, valueOld, valueNew));
    return S_OK;
}