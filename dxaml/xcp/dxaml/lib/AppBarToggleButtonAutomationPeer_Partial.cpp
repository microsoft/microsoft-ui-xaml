// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AppBarToggleButtonAutomationPeer.g.h"
#include "AppBarToggleButton.g.h"
#include "AppBarButtonHelpers.h"
#include "CommandBar.g.h"
#include "localizedResource.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT AppBarToggleButtonAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IAppBarToggleButton* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IAppBarToggleButtonAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAppBarToggleButtonAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml::IUIElement* ownerAsUIE = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));
    
    IFC(ActivateInstance(pOuter,
            static_cast<AppBarToggleButton*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<AppBarToggleButtonAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

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

// Initializes a new instance of the AppBarToggleButtonAutomationPeer class.
AppBarToggleButtonAutomationPeer::AppBarToggleButtonAutomationPeer()
{
}

// Deconstructor
AppBarToggleButtonAutomationPeer::~AppBarToggleButtonAutomationPeer()
{
}

IFACEMETHODIMP AppBarToggleButtonAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"AppBarToggleButton")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP AppBarToggleButtonAutomationPeer::GetNameCore(_Out_ HSTRING* returnValue)
{
    // Note: We are calling FrameworkElementAutomationPeer::GetNameCore here, rather than going through
    // any of our own immediate superclasses, to avoid the logic in ButtonBaseAutomationPeer that will
    // substitute Content for the automation name if the latter is unset -- we want to either get back
    // the actual value of AutomationProperties.Name if it has been set, or null if it hasn't.
    IFC_RETURN(FrameworkElementAutomationPeer::GetNameCore(returnValue));

    if (*returnValue == NULL)
    {
        // If AutomationProperties.Name hasn't been set, then return the value of our Label property.
        ctl::ComPtr<AppBarToggleButton> owner;
        IFC_RETURN(GetOwningAppBarToggleButton(&owner));
        IFC_RETURN(owner->get_Label(returnValue));
    }

    return S_OK;
}

IFACEMETHODIMP AppBarToggleButtonAutomationPeer::GetLocalizedControlTypeCore(_Out_ HSTRING* returnValue)
{
    return DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_APPBAR_TOGGLEBUTTON, returnValue);
}

IFACEMETHODIMP AppBarToggleButtonAutomationPeer::GetAcceleratorKeyCore(_Out_ HSTRING* returnValue)
{
    IFC_RETURN(AppBarToggleButtonAutomationPeerGenerated::GetAcceleratorKeyCore(returnValue));

    if (!*returnValue)
    {
        // If AutomationProperties.AcceleratorKey hasn't been set, then return the value of our KeyboardAcceleratorTextOverride property.
        wrl_wrappers::HString keyboardAcceleratorTextOverride;
        ctl::ComPtr<AppBarToggleButton> owner;
        IFC_RETURN(GetOwningAppBarToggleButton(&owner));
        IFC_RETURN(owner->get_KeyboardAcceleratorTextOverride(keyboardAcceleratorTextOverride.ReleaseAndGetAddressOf()));
        IFC_RETURN(GetTrimmedKeyboardAcceleratorTextOverrideStatic(keyboardAcceleratorTextOverride, returnValue));
    }

    return S_OK;
}

IFACEMETHODIMP AppBarToggleButtonAutomationPeer::IsKeyboardFocusableCore(_Out_ BOOLEAN* returnValue)
{
    *returnValue = FALSE;

    ctl::ComPtr<AppBarToggleButton> owner;
    IFC_RETURN(GetOwningAppBarToggleButton(&owner));
    
    ctl::ComPtr<ICommandBar> parentCommandBar;
    IFC_RETURN(CommandBar::FindParentCommandBarForElement(owner.Get(), &parentCommandBar));

    if (parentCommandBar)
    {
        *returnValue = !!AppBarButtonHelpers::IsKeyboardFocusable(owner.Get());
    }
    else
    {
        IFC_RETURN(AppBarToggleButtonAutomationPeerGenerated::IsKeyboardFocusableCore(returnValue));
    }

    return S_OK;
}

_Check_return_ HRESULT AppBarToggleButtonAutomationPeer::ToggleImpl()
{
    BOOLEAN isEnabled = FALSE;
    IFC_RETURN(IsEnabled(&isEnabled));
    
    if (!isEnabled)
    {
        IFC_RETURN(static_cast<HRESULT>(UIA_E_ELEMENTNOTENABLED));
    }

    ctl::ComPtr<AppBarToggleButton> owner;
    IFC_RETURN(GetOwningAppBarToggleButton(&owner));
    IFC_RETURN(owner->AutomationToggleButtonOnToggle());

    return S_OK;
}

_Check_return_ HRESULT AppBarToggleButtonAutomationPeer::get_ToggleStateImpl(_Out_ xaml_automation::ToggleState* pReturnValue)
{
    ctl::ComPtr<AppBarToggleButton> owner;
    IFC_RETURN(GetOwningAppBarToggleButton(&owner));
    
    ctl::ComPtr<wf::IReference<bool>> isCheckedReference;
    IFC_RETURN(owner->get_IsChecked(&isCheckedReference));

    if (isCheckedReference == NULL)
    {
        *pReturnValue = xaml_automation::ToggleState::ToggleState_Indeterminate;
    }
    else
    {
        BOOLEAN isChecked = FALSE;
        IFC_RETURN(isCheckedReference->get_Value(&isChecked));
        
        if (isChecked)
        {
            *pReturnValue = xaml_automation::ToggleState::ToggleState_On;
        }
        else
        {
            *pReturnValue = xaml_automation::ToggleState::ToggleState_Off;
        }
    }
    
    return S_OK;
}

_Check_return_ HRESULT AppBarToggleButtonAutomationPeer::RaiseToggleStatePropertyChangedEvent(
        _In_ IInspectable* pOldValue, 
        _In_ IInspectable* pNewValue)
{
    xaml_automation::ToggleState oldValue;
    xaml_automation::ToggleState newValue;
    CValue valueOld;
    CValue valueNew;
    
    IFC_RETURN(AppBarToggleButtonAutomationPeer::ConvertToToggleState(pOldValue, &oldValue))
    IFC_RETURN(AppBarToggleButtonAutomationPeer::ConvertToToggleState(pNewValue, &newValue))
    
    if(oldValue != newValue)
    {
        IFC_RETURN(CValueBoxer::BoxEnumValue(&valueOld, oldValue));
        IFC_RETURN(CValueBoxer::BoxEnumValue(&valueNew, newValue));

        IFC_RETURN(CoreImports::AutomationRaiseAutomationPropertyChanged(static_cast<CAutomationPeer*>(GetHandle()), UIAXcp::APAutomationProperties::APToggleStateProperty, valueOld, valueNew));
    }

    return S_OK;
}

// Convert the Boolean in Inspectable to the ToggleState Enum, if the Inspectable is NULL that corresponds to Indeterminate state.
_Check_return_ HRESULT AppBarToggleButtonAutomationPeer::ConvertToToggleState(_In_ IInspectable* pValue, _Out_ xaml_automation::ToggleState* pToggleState)
{
    BOOLEAN bValue = FALSE;
    
    *pToggleState = xaml_automation::ToggleState::ToggleState_Indeterminate;

    if (pValue)
    {
        IFC_RETURN(ctl::do_get_value(bValue, pValue));
        
        if(bValue)
        {
            *pToggleState = xaml_automation::ToggleState::ToggleState_On;
        } 
        else
        {
            *pToggleState = xaml_automation::ToggleState::ToggleState_Off;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT DirectUI::AppBarToggleButtonAutomationPeer::GetPositionInSetCoreImpl(_Out_ INT* returnValue)
{
    IFCPTR_RETURN(returnValue);
    *returnValue = -1;

    // First retrieve any valid value being directly set on the container, that value will get precedence.
    IFC_RETURN(AppBarToggleButtonAutomationPeerGenerated::GetPositionInSetCoreImpl(returnValue));

    // if it still is default value, calculate it ourselves.
    if (*returnValue == -1)
    {
        ctl::ComPtr<AppBarToggleButton> owner;
        IFC_RETURN(GetOwningAppBarToggleButton(&owner));
        IFC_RETURN(CommandBar::GetPositionInSetStatic(owner.Get(), returnValue));
    }
    
    return S_OK;
}

_Check_return_ HRESULT DirectUI::AppBarToggleButtonAutomationPeer::GetSizeOfSetCoreImpl(_Out_ INT* returnValue)
{
    IFCPTR_RETURN(returnValue);
    *returnValue = -1;

    // First retrieve any valid value being directly set on the container, that value will get precedence.
    IFC_RETURN(AppBarToggleButtonAutomationPeerGenerated::GetSizeOfSetCoreImpl(returnValue));

    // if it still is default value, calculate it ourselves.
    if (*returnValue == -1)
    {
        ctl::ComPtr<AppBarToggleButton> owner;
        IFC_RETURN(GetOwningAppBarToggleButton(&owner));
        IFC_RETURN(CommandBar::GetSizeOfSetStatic(owner.Get(), returnValue));
    }
    
    return S_OK;
}

_Check_return_ HRESULT AppBarToggleButtonAutomationPeer::GetOwningAppBarToggleButton(_Outptr_ AppBarToggleButton** owningAppBarToggleButton)
{
    ctl::ComPtr<xaml::IUIElement> owner;
    ctl::ComPtr<AppBarToggleButton> ownerAsAppBarToggleButton;

    IFC_RETURN(get_Owner(&owner));
    IFC_RETURN(owner.As(&ownerAsAppBarToggleButton));

    *owningAppBarToggleButton = ownerAsAppBarToggleButton.Detach();
    return S_OK;
}