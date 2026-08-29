// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ToggleMenuFlyoutItemAutomationPeer.g.h"
#include "ToggleMenuFlyoutItem.g.h"
#include "MenuFlyoutPresenter.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT ToggleMenuFlyoutItemAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IToggleMenuFlyoutItem* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IToggleMenuFlyoutItemAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IToggleMenuFlyoutItemAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml::IUIElement* ownerAsUIE = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));

    IFC(ActivateInstance(pOuter,
        static_cast<ToggleMenuFlyoutItem*>(owner)->GetHandle(),
        &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<ToggleMenuFlyoutItemAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

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

IFACEMETHODIMP ToggleMenuFlyoutItemAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** returnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(returnValue);
    *returnValue = NULL;

    if (patternInterface == xaml_automation_peers::PatternInterface_Toggle)
    {
        *returnValue = ctl::as_iinspectable(this);
        ctl::addref_interface(this);
    }
    else
    {
        IFC(ToggleMenuFlyoutItemAutomationPeerGenerated::GetPatternCore(patternInterface, returnValue));
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ToggleMenuFlyoutItemAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;
    IFCPTR(returnValue);
    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"ToggleMenuFlyoutItem")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ToggleMenuFlyoutItemAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* returnValue)
{
    HRESULT hr = S_OK;
    IFCPTR(returnValue);
    *returnValue = xaml_automation_peers::AutomationControlType_MenuItem;

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ToggleMenuFlyoutItemAutomationPeer::GetAcceleratorKeyCore(_Out_ HSTRING* returnValue)
{
    IFC_RETURN(ToggleMenuFlyoutItemAutomationPeerGenerated::GetAcceleratorKeyCore(returnValue));

    if (!*returnValue)
    {
        // If AutomationProperties.AcceleratorKey hasn't been set, then return the value of our KeyboardAcceleratorTextOverride property.
        wrl_wrappers::HString keyboardAcceleratorTextOverride;
        ctl::ComPtr<IUIElement> owner;
        ctl::ComPtr<ToggleMenuFlyoutItem> ownerAsToggleMenuFlyoutItem;
        IFC_RETURN(get_Owner(&owner));
        IFC_RETURN(owner.As(&ownerAsToggleMenuFlyoutItem));
        IFC_RETURN(ownerAsToggleMenuFlyoutItem->get_KeyboardAcceleratorTextOverride(keyboardAcceleratorTextOverride.ReleaseAndGetAddressOf()));
        IFC_RETURN(GetTrimmedKeyboardAcceleratorTextOverrideStatic(keyboardAcceleratorTextOverride, returnValue));
    }

    return S_OK;
}

_Check_return_ HRESULT DirectUI::ToggleMenuFlyoutItemAutomationPeer::GetPositionInSetCoreImpl(_Out_ INT* returnValue)
{
    IFCPTR_RETURN(returnValue);
    *returnValue = -1;

    // First retrieve any valid value being directly set on the container, that value will get precedence.
    IFC_RETURN(ToggleMenuFlyoutItemAutomationPeerGenerated::GetPositionInSetCoreImpl(returnValue));

    // if it still is default value, calculate it ourselves.
    if (*returnValue == -1)
    {
        ctl::ComPtr<xaml::IUIElement> owner;
        IFC_RETURN(get_Owner(&owner));
        IFC_RETURN(MenuFlyoutPresenter::GetPositionInSetHelper(owner.AsOrNull<IMenuFlyoutItemBase>(), returnValue));    
    }
    
    return S_OK;
}

_Check_return_ HRESULT DirectUI::ToggleMenuFlyoutItemAutomationPeer::GetSizeOfSetCoreImpl(_Out_ INT* returnValue)
{
    IFCPTR_RETURN(returnValue);
    *returnValue = -1;

    // First retrieve any valid value being directly set on the container, that value will get precedence.
    IFC_RETURN(ToggleMenuFlyoutItemAutomationPeerGenerated::GetSizeOfSetCoreImpl(returnValue));

    // if it still is default value, calculate it ourselves.
    if (*returnValue == -1)
    {
        ctl::ComPtr<xaml::IUIElement> owner;
        IFC_RETURN(get_Owner(&owner));
        IFC_RETURN(MenuFlyoutPresenter::GetSizeOfSetHelper(owner.AsOrNull<IMenuFlyoutItemBase>(), returnValue));
    }
    
    return S_OK;
}

_Check_return_ HRESULT ToggleMenuFlyoutItemAutomationPeer::ToggleImpl()
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
    IFCPTR(pOwner);
    IFC((static_cast<ToggleMenuFlyoutItem*>(pOwner))->Invoke());

Cleanup:
    ReleaseInterface(pOwner);

    RRETURN(hr);
}

_Check_return_ HRESULT ToggleMenuFlyoutItemAutomationPeer::get_ToggleStateImpl(_Out_ xaml_automation::ToggleState* pReturnValue)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsChecked = FALSE;
    xaml::IUIElement* pOwner = NULL;

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);
    IFC((static_cast<ToggleMenuFlyoutItem*>(pOwner))->get_IsChecked(&bIsChecked));

    if (bIsChecked)
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

_Check_return_ HRESULT ToggleMenuFlyoutItemAutomationPeer::RaiseToggleStatePropertyChangedEvent(
    _In_ IInspectable* pOldValue,
    _In_ IInspectable* pNewValue)
{
    HRESULT hr = S_OK;
    xaml_automation::ToggleState oldValue;
    xaml_automation::ToggleState newValue;
    CValue valueOld;
    CValue valueNew;

    IFC(ToggleMenuFlyoutItemAutomationPeer::ConvertToToggleState(pOldValue, &oldValue));
    IFC(ToggleMenuFlyoutItemAutomationPeer::ConvertToToggleState(pNewValue, &newValue));

    if (oldValue != newValue)
    {
        IFC(CValueBoxer::BoxEnumValue(&valueOld, oldValue));
        IFC(CValueBoxer::BoxEnumValue(&valueNew, newValue));

        IFC(CoreImports::AutomationRaiseAutomationPropertyChanged(static_cast<CAutomationPeer*>(GetHandle()), UIAXcp::APAutomationProperties::APToggleStateProperty, valueOld, valueNew));
    }

Cleanup:
    RRETURN(hr);
}

// Convert the Boolean in Inspectable to the ToggleState Enum, if the Inspectable is NULL that corresponds to Indeterminate state.
_Check_return_ HRESULT ToggleMenuFlyoutItemAutomationPeer::ConvertToToggleState(_In_ IInspectable* pValue, _Out_ xaml_automation::ToggleState* pToggleState)
{
    HRESULT hr = S_OK;
    BOOLEAN bValue = FALSE;

    *pToggleState = xaml_automation::ToggleState::ToggleState_Indeterminate;

    if (pValue)
    {
        IFC(ctl::do_get_value(bValue, pValue));

        if (bValue)
        {
            *pToggleState = xaml_automation::ToggleState::ToggleState_On;
        }
        else
        {
            *pToggleState = xaml_automation::ToggleState::ToggleState_Off;
        }
    }

Cleanup:
    RRETURN(hr);
}
