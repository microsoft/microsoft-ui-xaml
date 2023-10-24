// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MenuFlyoutItemAutomationPeer.g.h"
#include "MenuFlyoutItem.g.h"
#include "MenuFlyoutPresenter.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT MenuFlyoutItemAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IMenuFlyoutItem* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IMenuFlyoutItemAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IMenuFlyoutItemAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml::IUIElement* ownerAsUIE = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));

    IFC(ActivateInstance(pOuter,
        static_cast<MenuFlyoutItem*>(owner)->GetHandle(),
        &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<MenuFlyoutItemAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

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

IFACEMETHODIMP MenuFlyoutItemAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** returnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(returnValue);
    *returnValue = NULL;

    if (patternInterface == xaml_automation_peers::PatternInterface_Invoke)
    {
        *returnValue = ctl::as_iinspectable(this);
        ctl::addref_interface(this);
    }
    else
    {
        IFC(MenuFlyoutItemAutomationPeerGenerated::GetPatternCore(patternInterface, returnValue));
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP MenuFlyoutItemAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;
    IFCPTR(returnValue);
    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"MenuFlyoutItem")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP MenuFlyoutItemAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* returnValue)
{
    HRESULT hr = S_OK;
    IFCPTR(returnValue);
    *returnValue = xaml_automation_peers::AutomationControlType_MenuItem;

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP MenuFlyoutItemAutomationPeer::GetAcceleratorKeyCore(_Out_ HSTRING* returnValue)
{
    IFC_RETURN(MenuFlyoutItemAutomationPeerGenerated::GetAcceleratorKeyCore(returnValue));

    if (!*returnValue)
    {
        // If AutomationProperties.AcceleratorKey hasn't been set, then return the value of our KeyboardAcceleratorTextOverride property.
        wrl_wrappers::HString keyboardAcceleratorTextOverride;
        ctl::ComPtr<IUIElement> owner;
        ctl::ComPtr<MenuFlyoutItem> ownerAsMenuFlyoutItem;
        IFC_RETURN(get_Owner(&owner));
        IFC_RETURN(owner.As(&ownerAsMenuFlyoutItem));
        IFC_RETURN(ownerAsMenuFlyoutItem->get_KeyboardAcceleratorTextOverride(keyboardAcceleratorTextOverride.ReleaseAndGetAddressOf()));
        IFC_RETURN(GetTrimmedKeyboardAcceleratorTextOverrideStatic(keyboardAcceleratorTextOverride, returnValue));
    }

    return S_OK;
}

_Check_return_ HRESULT DirectUI::MenuFlyoutItemAutomationPeer::GetPositionInSetCoreImpl(_Out_ INT* returnValue)
{
    IFCPTR_RETURN(returnValue);
    *returnValue = -1;

    // First retrieve any valid value being directly set on the container, that value will get precedence.
    IFC_RETURN(MenuFlyoutItemAutomationPeerGenerated::GetPositionInSetCoreImpl(returnValue));

    // if it still is default value, calculate it ourselves.
    if (*returnValue == -1)
    {
        ctl::ComPtr<xaml::IUIElement> owner;
        IFC_RETURN(get_Owner(&owner));
        IFC_RETURN(MenuFlyoutPresenter::GetPositionInSetHelper(owner.AsOrNull<IMenuFlyoutItemBase>(), returnValue));
    }
    
    return S_OK;
}

_Check_return_ HRESULT DirectUI::MenuFlyoutItemAutomationPeer::GetSizeOfSetCoreImpl(_Out_ INT* returnValue)
{
    IFCPTR_RETURN(returnValue);
    *returnValue = -1;

    // First retrieve any valid value being directly set on the container, that value will get precedence.
    IFC_RETURN(MenuFlyoutItemAutomationPeerGenerated::GetSizeOfSetCoreImpl(returnValue));

    // if it still is default value, calculate it ourselves.
    if (*returnValue == -1)
    {
        ctl::ComPtr<xaml::IUIElement> owner;
        IFC_RETURN(get_Owner(&owner));
        IFC_RETURN(MenuFlyoutPresenter::GetSizeOfSetHelper(owner.AsOrNull<IMenuFlyoutItemBase>(), returnValue));
    }
    
    return S_OK;
}

_Check_return_ HRESULT MenuFlyoutItemAutomationPeer::InvokeImpl()
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
    IFC((static_cast<MenuFlyoutItem*>(pOwner))->Invoke());

Cleanup:
    ReleaseInterface(pOwner);
    RRETURN(hr);
}
