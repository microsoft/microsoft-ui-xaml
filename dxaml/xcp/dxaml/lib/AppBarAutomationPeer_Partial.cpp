// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AppBarAutomationPeer.g.h"
#include "AppBar.g.h"
#include "ToggleButtonAutomationPeer.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT AppBarAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IAppBar* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IAppBarAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAppBarAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml::IUIElement* ownerAsUIE = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));
    
    IFC(ActivateInstance(pOuter,
            static_cast<AppBar*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<AppBarAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

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

// Initializes a new instance of the ButtonAutomationPeer class.
AppBarAutomationPeer::AppBarAutomationPeer()
{
}

// Deconstructor
AppBarAutomationPeer::~AppBarAutomationPeer()
{
}

IFACEMETHODIMP AppBarAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue)
{
    bool shouldReturnWindowPattern = false;

    if (patternInterface == xaml_automation_peers::PatternInterface_Window)
    {
        BOOLEAN isOpen = FALSE;
        ctl::ComPtr<xaml::IUIElement> owner;

        IFC_RETURN(get_Owner(&owner));
        IFC_RETURN(owner.Cast<AppBar>()->get_IsOpen(&isOpen));
        shouldReturnWindowPattern = !!isOpen;
    }

    if ((patternInterface == xaml_automation_peers::PatternInterface_ExpandCollapse) || 
        (patternInterface == xaml_automation_peers::PatternInterface_Toggle) ||
        (shouldReturnWindowPattern))
    {
        *ppReturnValue = ctl::as_iinspectable(this);
         ctl::addref_interface(this);
    } 
    else
    {
        IFC_RETURN(AppBarAutomationPeerGenerated::GetPatternCore(patternInterface, ppReturnValue));
    }

    return S_OK;
}

IFACEMETHODIMP AppBarAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"ApplicationBar")).CopyTo(returnValue));
    
Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP AppBarAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue)
{
    *pReturnValue = xaml_automation_peers::AutomationControlType_AppBar;

    RRETURN(S_OK);
}

// IToggleProvider
_Check_return_ HRESULT AppBarAutomationPeer::ToggleImpl()
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;
    BOOLEAN bIsEnabled;
    BOOLEAN isOpen = FALSE;

    IFC(IsEnabled(&bIsEnabled));
    if(!bIsEnabled)
    {
        IFC(static_cast<HRESULT>(UIA_E_ELEMENTNOTENABLED));
    }

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);
    IFC((static_cast<AppBar*>(pOwner))->get_IsOpen(&isOpen));
    IFC((static_cast<AppBar*>(pOwner))->put_IsOpen(!isOpen));

Cleanup:
    ReleaseInterface(pOwner);
    RRETURN(hr);
}

_Check_return_ HRESULT AppBarAutomationPeer::get_ToggleStateImpl(_Out_ xaml_automation::ToggleState* pReturnValue)
{
    HRESULT hr = S_OK;
    BOOLEAN isOpen = FALSE;
    xaml::IUIElement* pOwner = NULL;

    IFCPTR(pReturnValue);
    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);
    IFC((static_cast<AppBar*>(pOwner))->get_IsOpen(&isOpen));

    if(isOpen)
    {
        *pReturnValue = xaml_automation::ToggleState::ToggleState_On;
    }
    else
    {
        *pReturnValue = xaml_automation::ToggleState::ToggleState_Off;
    }

Cleanup:
    ReleaseInterface(pOwner);
    RRETURN(hr);
}

_Check_return_ HRESULT AppBarAutomationPeer::RaiseToggleStatePropertyChangedEvent(
        _In_ IInspectable* pOldValue, 
        _In_ IInspectable* pNewValue)
{
    HRESULT hr = S_OK;
    xaml_automation::ToggleState oldValue;
    xaml_automation::ToggleState newValue;
    CValue valueOld;
    CValue valueNew;
    
    IFC(ToggleButtonAutomationPeer::ConvertToToggleState(pOldValue, &oldValue))
    IFC(ToggleButtonAutomationPeer::ConvertToToggleState(pNewValue, &newValue))
    
    if(oldValue != newValue)
    {
        IFC(CValueBoxer::BoxEnumValue(&valueOld, oldValue));
        IFC(CValueBoxer::BoxEnumValue(&valueNew, newValue));

        IFC(CoreImports::AutomationRaiseAutomationPropertyChanged(static_cast<CAutomationPeer*>(GetHandle()), UIAXcp::APAutomationProperties::APToggleStateProperty, valueOld, valueNew));
    }

Cleanup:
    RRETURN(hr);
}

// IExpandCollapseProvider
_Check_return_ HRESULT AppBarAutomationPeer::ExpandImpl()
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;
    BOOLEAN bIsEnabled;

    IFC(IsEnabled(&bIsEnabled));
    if(!bIsEnabled)
    {
        IFC(static_cast<HRESULT>(UIA_E_ELEMENTNOTENABLED));
    }

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);
    IFC((static_cast<AppBar*>(pOwner))->put_IsOpen(TRUE));

Cleanup:
    ReleaseInterface(pOwner);
    RRETURN(hr);
}

_Check_return_ HRESULT AppBarAutomationPeer::CollapseImpl()
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;
    BOOLEAN bIsEnabled;

    IFC(IsEnabled(&bIsEnabled));
    if(!bIsEnabled)
    {
        IFC(static_cast<HRESULT>(UIA_E_ELEMENTNOTENABLED));
    }

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);
    IFC((static_cast<AppBar*>(pOwner))->put_IsOpen(FALSE));

Cleanup:
    ReleaseInterface(pOwner);
    RRETURN(hr);
}

_Check_return_ HRESULT AppBarAutomationPeer::get_ExpandCollapseStateImpl(_Out_ xaml_automation::ExpandCollapseState* pReturnValue)
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;
    BOOLEAN isOpen = TRUE;

    IFCPTR(pReturnValue);
    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);
    IFC((static_cast<AppBar*>(pOwner))->get_IsOpen(&isOpen));
    
    *pReturnValue = isOpen ? xaml_automation::ExpandCollapseState_Expanded : xaml_automation::ExpandCollapseState_Collapsed;

Cleanup:
    ReleaseInterface(pOwner);
    RRETURN(hr);
}

// IWindowProvider
_Check_return_ HRESULT 
AppBarAutomationPeer::get_IsModalImpl(
    _Out_ BOOLEAN* pValue)
{
    *pValue = TRUE;
    return S_OK;
}

_Check_return_ HRESULT 
AppBarAutomationPeer::get_IsTopmostImpl(
    _Out_ BOOLEAN* pValue)
{
    *pValue = TRUE;
    return S_OK;
}

_Check_return_ HRESULT 
AppBarAutomationPeer::get_MaximizableImpl(
    _Out_ BOOLEAN* pValue)
{
    *pValue = FALSE;
    return S_OK;
}

_Check_return_ HRESULT 
AppBarAutomationPeer::get_MinimizableImpl(
    _Out_ BOOLEAN* pValue)
{
    *pValue = FALSE;
    return S_OK;
}

_Check_return_ HRESULT 
AppBarAutomationPeer::get_InteractionStateImpl(
    _Out_ xaml_automation::WindowInteractionState* pValue)
{
    *pValue = xaml_automation::WindowInteractionState_Running;
    return S_OK;
}

_Check_return_ HRESULT 
AppBarAutomationPeer::get_VisualStateImpl(
    _Out_ xaml_automation::WindowVisualState* pValue)
{
    *pValue = xaml_automation::WindowVisualState_Normal;
    return S_OK;
}

_Check_return_ HRESULT 
AppBarAutomationPeer::CloseImpl()
{
    return S_OK;
}

_Check_return_ HRESULT 
AppBarAutomationPeer::SetVisualStateImpl(
    _In_ xaml_automation::WindowVisualState /* state */)
{
    return S_OK;
}

_Check_return_ HRESULT 
AppBarAutomationPeer::WaitForInputIdleImpl(
    _In_ INT /* milliseconds */, 
    _Out_ BOOLEAN* /* pValue */)
{
    return S_OK;
}

// Raise events for ExpandCollapseState changes to UIAutomation Clients.
_Check_return_ HRESULT AppBarAutomationPeer::RaiseExpandCollapseAutomationEvent(_In_ BOOLEAN isOpen)
{
    HRESULT hr = S_OK;

    xaml_automation::ExpandCollapseState oldValue;
    xaml_automation::ExpandCollapseState newValue;
    CValue valueOld;
    CValue valueNew;
    
    // Converting isOpen to appropriate enumerations
    if(isOpen)
    {
        oldValue = xaml_automation::ExpandCollapseState::ExpandCollapseState_Collapsed;
        newValue = xaml_automation::ExpandCollapseState::ExpandCollapseState_Expanded;
    } 
    else
    {
        oldValue = xaml_automation::ExpandCollapseState::ExpandCollapseState_Expanded;
        newValue = xaml_automation::ExpandCollapseState::ExpandCollapseState_Collapsed;
    }
 
    IFC(CValueBoxer::BoxEnumValue(&valueOld, oldValue));
    IFC(CValueBoxer::BoxEnumValue(&valueNew, newValue));
    IFC(CoreImports::AutomationRaiseAutomationPropertyChanged(static_cast<CAutomationPeer*>(GetHandle()), UIAXcp::APAutomationProperties::APExpandCollapseStateProperty, valueOld, valueNew));

Cleanup:
    RRETURN(hr);
}
