// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AppBarButtonAutomationPeer.g.h"
#include "AppBarButton.g.h"
#include "AppBarButtonHelpers.h"
#include "CommandBar.g.h"
#include "localizedResource.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT AppBarButtonAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IAppBarButton* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IAppBarButtonAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAppBarButtonAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml::IUIElement* ownerAsUIE = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));
    
    IFC(ActivateInstance(pOuter,
            static_cast<AppBarButton*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<AppBarButtonAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

    if (ppInner)
    {
        *ppInner = pInner;
        pInner = NULL;
    }

    *ppInstance = pInstance;
    pInstance = NULL;

Cleanup:
    ReleaseInterface(ownerAsUIE);
    ReleaseInterface(pInstance);
    ReleaseInterface(pInner);
    RRETURN(hr);
}

// Initializes a new instance of the AppBarButtonAutomationPeer class.
AppBarButtonAutomationPeer::AppBarButtonAutomationPeer()
{
}

// Deconstructor
AppBarButtonAutomationPeer::~AppBarButtonAutomationPeer()
{
}

IFACEMETHODIMP AppBarButtonAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue)
{
    *ppReturnValue = nullptr;

    ctl::ComPtr<AppBarButton> owner;
    IFC_RETURN(GetOwningAppBarButton(&owner));

    if (patternInterface == xaml_automation_peers::PatternInterface_ExpandCollapse)
    {
        // We specifically want to *not* report that we support the expand/collapse pattern when we don't have an attached flyout,
        // because then we have nothing to expand or collapse.  So we unconditionally enter this block even if owner->m_menuHelper
        // is null so we'll get the default null return value in that case.
        if (owner->m_menuHelper)
        {
            *ppReturnValue = ctl::as_iinspectable(this);
            ctl::addref_interface(this);
        }
    }
    else
    {
        IFC_RETURN(AppBarButtonAutomationPeerGenerated::GetPatternCore(patternInterface, ppReturnValue));
    }
    return S_OK;
}

IFACEMETHODIMP AppBarButtonAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"AppBarButton")).CopyTo(returnValue));
    
Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP AppBarButtonAutomationPeer::GetNameCore(_Out_ HSTRING* returnValue)
{
    // Note: We are calling FrameworkElementAutomationPeer::GetNameCore here, rather than going through
    // any of our own immediate superclasses, to avoid the logic in ButtonBaseAutomationPeer that will
    // substitute Content for the automation name if the latter is unset -- we want to either get back
    // the actual value of AutomationProperties.Name if it has been set, or null if it hasn't.
    IFC_RETURN(FrameworkElementAutomationPeer::GetNameCore(returnValue));

    if (*returnValue == NULL)
    {
        // If AutomationProperties.Name hasn't been set, then return the value of our Label property.
        ctl::ComPtr<AppBarButton> owner;
        IFC_RETURN(GetOwningAppBarButton(&owner));
        IFC_RETURN(owner->get_Label(returnValue));
    }

    return S_OK;
}

IFACEMETHODIMP AppBarButtonAutomationPeer::GetLocalizedControlTypeCore(_Out_ HSTRING* returnValue)
{
    return DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_APPBAR_BUTTON, returnValue);
}

IFACEMETHODIMP AppBarButtonAutomationPeer::GetAcceleratorKeyCore(_Out_ HSTRING* returnValue)
{
    IFC_RETURN(AppBarButtonAutomationPeerGenerated::GetAcceleratorKeyCore(returnValue));

    if (!*returnValue)
    {
        // If AutomationProperties.AcceleratorKey hasn't been set, then return the value of our KeyboardAcceleratorTextOverride property.
        wrl_wrappers::HString keyboardAcceleratorTextOverride;
        ctl::ComPtr<AppBarButton> owner;
        IFC_RETURN(GetOwningAppBarButton(&owner));
        IFC_RETURN(owner->get_KeyboardAcceleratorTextOverride(keyboardAcceleratorTextOverride.ReleaseAndGetAddressOf()));
        IFC_RETURN(GetTrimmedKeyboardAcceleratorTextOverrideStatic(keyboardAcceleratorTextOverride, returnValue));
    }

    return S_OK;
}

IFACEMETHODIMP AppBarButtonAutomationPeer::IsKeyboardFocusableCore(_Out_ BOOLEAN* returnValue)
{
    *returnValue = FALSE;

    ctl::ComPtr<AppBarButton> owner;
    IFC_RETURN(GetOwningAppBarButton(&owner));
    
    ctl::ComPtr<ICommandBar> parentCommandBar;
    IFC_RETURN(CommandBar::FindParentCommandBarForElement(owner.Get(), &parentCommandBar));

    if (parentCommandBar)
    {
        *returnValue = !!AppBarButtonHelpers::IsKeyboardFocusable(owner.Get());
    }
    else
    {
        IFC_RETURN(AppBarButtonAutomationPeerGenerated::IsKeyboardFocusableCore(returnValue));
    }

    return S_OK;
}

// IExpandCollapseProvider
_Check_return_ HRESULT AppBarButtonAutomationPeer::ExpandImpl()
{
    ctl::ComPtr<AppBarButton> owner;
    IFC_RETURN(GetOwningAppBarButton(&owner));

    if (owner->m_menuHelper)
    {
        IFC_RETURN(owner->m_menuHelper->OpenSubMenu());
    }

    return S_OK;
}

_Check_return_ HRESULT AppBarButtonAutomationPeer::CollapseImpl()
{
    ctl::ComPtr<AppBarButton> owner;
    IFC_RETURN(GetOwningAppBarButton(&owner));

    if (owner->m_menuHelper)
    {
        IFC_RETURN(owner->m_menuHelper->CloseSubMenu());
    }

    return S_OK;
}

_Check_return_ HRESULT AppBarButtonAutomationPeer::get_ExpandCollapseStateImpl(_Out_ xaml_automation::ExpandCollapseState* pReturnValue)
{
    BOOLEAN isOpen = FALSE;

    ctl::ComPtr<AppBarButton> owner;
    IFC_RETURN(GetOwningAppBarButton(&owner));

    if (owner->m_menuHelper)
    {
        IFC_RETURN(owner->get_IsSubMenuOpen(&isOpen));
    }

    *pReturnValue = isOpen ? xaml_automation::ExpandCollapseState_Expanded : xaml_automation::ExpandCollapseState_Collapsed;
    return S_OK;
}

// Raise events for ExpandCollapseState changes to UIAutomation Clients.
_Check_return_ HRESULT AppBarButtonAutomationPeer::RaiseExpandCollapseAutomationEvent(_In_ BOOLEAN isOpen)
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

_Check_return_ HRESULT DirectUI::AppBarButtonAutomationPeer::GetPositionInSetCoreImpl(_Out_ INT* returnValue)
{
    IFCPTR_RETURN(returnValue);
    *returnValue = -1;

    // First retrieve any valid value being directly set on the container, that value will get precedence.
    IFC_RETURN(AppBarButtonAutomationPeerGenerated::GetPositionInSetCoreImpl(returnValue));

    // if it still is default value, calculate it ourselves.
    if (*returnValue == -1)
    {
        ctl::ComPtr<AppBarButton> owner;
        IFC_RETURN(GetOwningAppBarButton(&owner));
        IFC_RETURN(CommandBar::GetPositionInSetStatic(owner.Get(), returnValue));
    }
    
    return S_OK;
}

_Check_return_ HRESULT DirectUI::AppBarButtonAutomationPeer::GetSizeOfSetCoreImpl(_Out_ INT* returnValue)
{
    IFCPTR_RETURN(returnValue);
    *returnValue = -1;

    // First retrieve any valid value being directly set on the container, that value will get precedence.
    IFC_RETURN(AppBarButtonAutomationPeerGenerated::GetSizeOfSetCoreImpl(returnValue));

    // if it still is default value, calculate it ourselves.
    if (*returnValue == -1)
    {
        ctl::ComPtr<AppBarButton> owner;
        IFC_RETURN(GetOwningAppBarButton(&owner));
        IFC_RETURN(CommandBar::GetSizeOfSetStatic(owner.Get(), returnValue));
    }
    
    return S_OK;
}

_Check_return_ HRESULT AppBarButtonAutomationPeer::GetOwningAppBarButton(_Outptr_ AppBarButton** owningAppBarButton)
{
    ctl::ComPtr<xaml::IUIElement> owner;
    ctl::ComPtr<AppBarButton> ownerAsAppBarButton;

    IFC_RETURN(get_Owner(&owner));
    IFC_RETURN(owner.As(&ownerAsAppBarButton));

    *owningAppBarButton = ownerAsAppBarButton.Detach();
    return S_OK;
}