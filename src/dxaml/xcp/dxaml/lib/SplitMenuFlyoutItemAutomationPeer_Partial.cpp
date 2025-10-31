// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SplitMenuFlyoutItemAutomationPeer.g.h"
#include "SplitMenuFlyoutItem.g.h"
#include "MenuFlyoutPresenter.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// AutomationId used to identify and filter out the primary button peer
static const WCHAR c_primaryButtonAutomationId[] = L"SplitMenuFlyoutItemPrimaryButton";

_Check_return_ HRESULT SplitMenuFlyoutItemAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::ISplitMenuFlyoutItem* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::ISplitMenuFlyoutItemAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::ISplitMenuFlyoutItemAutomationPeer* pInstance = nullptr;
    IInspectable* pInner = nullptr;
    xaml::IUIElement* ownerAsUIE = nullptr;
    ctl::ComPtr<SplitMenuFlyoutItem> spOwnerAsSplitMenuFlyoutItem;
    ctl::ComPtr<SplitMenuFlyoutItemAutomationPeer> spAutomationPeer;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == nullptr || ppInner != nullptr);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));

    IFC(ctl::do_query_interface(spOwnerAsSplitMenuFlyoutItem, owner));

    IFC(ActivateInstance(pOuter,
        spOwnerAsSplitMenuFlyoutItem->GetHandle(),
        &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    
    IFC(ctl::do_query_interface(spAutomationPeer, pInstance));
    IFC(spAutomationPeer->put_Owner(ownerAsUIE));

    if (ppInner)
    {
        *ppInner = pInner;
        pInner = nullptr;
    }

    *ppInstance = pInstance;
    pInstance = nullptr;

Cleanup:
    ReleaseInterface(ownerAsUIE);
    ReleaseInterface(pInstance);
    ReleaseInterface(pInner);
    RRETURN(hr);
}

IFACEMETHODIMP SplitMenuFlyoutItemAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** returnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(returnValue);
    *returnValue = nullptr;

    if (patternInterface == xaml_automation_peers::PatternInterface_Invoke || 
        patternInterface == xaml_automation_peers::PatternInterface_ExpandCollapse)
    {
        *returnValue = ctl::as_iinspectable(this);
        ctl::addref_interface(this);
    }
    else
    {
        IFC(SplitMenuFlyoutItemAutomationPeerGenerated::GetPatternCore(patternInterface, returnValue));
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP SplitMenuFlyoutItemAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;
    IFCPTR(returnValue);
    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"SplitMenuFlyoutItem")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP SplitMenuFlyoutItemAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* returnValue)
{
    HRESULT hr = S_OK;
    IFCPTR(returnValue);
    *returnValue = xaml_automation_peers::AutomationControlType_MenuItem;

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP SplitMenuFlyoutItemAutomationPeer::GetAcceleratorKeyCore(_Out_ HSTRING* returnValue)
{
    IFC_RETURN(SplitMenuFlyoutItemAutomationPeerGenerated::GetAcceleratorKeyCore(returnValue));

    if (!*returnValue)
    {
        // If AutomationProperties.AcceleratorKey hasn't been set, then return the value of our KeyboardAcceleratorTextOverride property.
        wrl_wrappers::HString keyboardAcceleratorTextOverride;
        ctl::ComPtr<IUIElement> owner;
        ctl::ComPtr<SplitMenuFlyoutItem> ownerAsSplitMenuFlyoutItem;
        IFC_RETURN(get_Owner(&owner));
        IFC_RETURN(owner.As(&ownerAsSplitMenuFlyoutItem));
        IFC_RETURN(ownerAsSplitMenuFlyoutItem->get_KeyboardAcceleratorTextOverride(keyboardAcceleratorTextOverride.ReleaseAndGetAddressOf()));
        IFC_RETURN(GetTrimmedKeyboardAcceleratorTextOverrideStatic(keyboardAcceleratorTextOverride, returnValue));
    }

    return S_OK;
}

_Check_return_ HRESULT DirectUI::SplitMenuFlyoutItemAutomationPeer::GetPositionInSetCoreImpl(_Out_ INT* returnValue)
{
    IFCPTR_RETURN(returnValue);
    *returnValue = -1;

    // First retrieve any valid value being directly set on the container, that value will get precedence.
    IFC_RETURN(SplitMenuFlyoutItemAutomationPeerGenerated::GetPositionInSetCoreImpl(returnValue));

    // if it still is default value, calculate it ourselves.
    if (*returnValue == -1)
    {
        ctl::ComPtr<xaml::IUIElement> owner;
        IFC_RETURN(get_Owner(&owner));
        IFC_RETURN(MenuFlyoutPresenter::GetPositionInSetHelper(owner.AsOrNull<IMenuFlyoutItemBase>(), returnValue));    
    }
    
    return S_OK;
}

_Check_return_ HRESULT DirectUI::SplitMenuFlyoutItemAutomationPeer::GetSizeOfSetCoreImpl(_Out_ INT* returnValue)
{
    IFCPTR_RETURN(returnValue);
    *returnValue = -1;

    // First retrieve any valid value being directly set on the container, that value will get precedence.
    IFC_RETURN(SplitMenuFlyoutItemAutomationPeerGenerated::GetSizeOfSetCoreImpl(returnValue));

    // if it still is default value, calculate it ourselves.
    if (*returnValue == -1)
    {
        ctl::ComPtr<xaml::IUIElement> owner;
        IFC_RETURN(get_Owner(&owner));
        IFC_RETURN(MenuFlyoutPresenter::GetSizeOfSetHelper(owner.AsOrNull<IMenuFlyoutItemBase>(), returnValue));
    }
    
    return S_OK;
}

_Check_return_ HRESULT SplitMenuFlyoutItemAutomationPeer::InvokeImpl()
{
    BOOLEAN bIsEnabled;

    IFC_RETURN(IsEnabled(&bIsEnabled));
    if (!bIsEnabled)
    {
        IFC_RETURN(static_cast<HRESULT>(UIA_E_ELEMENTNOTENABLED));
    }

    ctl::ComPtr<xaml::IUIElement> spOwner;
    IFC_RETURN(get_Owner(&spOwner));
    
    ctl::ComPtr<SplitMenuFlyoutItem> spOwnerAsSplitMenuFlyoutItem;
    IFC_RETURN(spOwner.As(&spOwnerAsSplitMenuFlyoutItem));
    
    IFC_RETURN(spOwnerAsSplitMenuFlyoutItem->Invoke());

    return S_OK;
}

IFACEMETHODIMP SplitMenuFlyoutItemAutomationPeer::HasKeyboardFocusCore(_Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;
    IFCPTR(returnValue);
    *returnValue = FALSE;

    // Since the EventsSource of the primary button is set to the SplitMenuFlyoutItem's automation peer,
    // we need to redirect focus queries to check if primary button has focus.
    xaml_automation_peers::IAutomationPeer* pPrimaryButtonPeer = GetPrimaryButtonPeer();
    
    if (pPrimaryButtonPeer)
    {
        IFC(pPrimaryButtonPeer->HasKeyboardFocus(returnValue));
    }

Cleanup:
    ReleaseInterface(pPrimaryButtonPeer);
    RRETURN(hr);
}

IFACEMETHODIMP SplitMenuFlyoutItemAutomationPeer::IsKeyboardFocusableCore(_Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;
    IFCPTR(returnValue);
    *returnValue = FALSE;

    xaml_automation_peers::IAutomationPeer* pPrimaryButtonPeer = GetPrimaryButtonPeer();
    
    if (pPrimaryButtonPeer)
    {
        IFC(pPrimaryButtonPeer->IsKeyboardFocusable(returnValue));
    }
            
Cleanup:
    ReleaseInterface(pPrimaryButtonPeer);
    RRETURN(hr);
}

IFACEMETHODIMP SplitMenuFlyoutItemAutomationPeer::GetPeerFromPointCore(
    _In_ wf::Point point,
    _Outptr_ xaml_automation_peers::IAutomationPeer** returnValue)
{
    HRESULT hr = S_OK;
    IFCPTR(returnValue);
    *returnValue = nullptr;

    // When using narrator with a touch screen, focus the primary button programmatically
    // to sync the narrator focus with the keyboard focus.
    xaml_automation_peers::IAutomationPeer* pPrimaryButtonPeer = GetPrimaryButtonPeer();
    
    if (pPrimaryButtonPeer)
    {
        ctl::ComPtr<xaml::IUIElement> spOwner;
        ctl::ComPtr<SplitMenuFlyoutItem> spOwnerAsSplitMenuFlyoutItem;
        IFC(get_Owner(&spOwner));
        IFC(spOwner.As(&spOwnerAsSplitMenuFlyoutItem));

        if (spOwnerAsSplitMenuFlyoutItem->m_tpPrimaryButton)
        {
            ctl::ComPtr<xaml_controls::IControl> spPrimaryButtonAsControl;
            IFC(spOwnerAsSplitMenuFlyoutItem->m_tpPrimaryButton.As(&spPrimaryButtonAsControl));
            
            BOOLEAN focusUpdated = FALSE;
            IFC(spPrimaryButtonAsControl.Cast<Control>()->Focus(xaml::FocusState_Programmatic, &focusUpdated));
        }

        *returnValue = pPrimaryButtonPeer;
        pPrimaryButtonPeer = nullptr; // Don't release since we're returning it
        goto Cleanup;
    }

    // Fall back to base implementation if no primary button
    IFC(SplitMenuFlyoutItemAutomationPeerGenerated::GetPeerFromPointCore(point, returnValue));

Cleanup:
    ReleaseInterface(pPrimaryButtonPeer);
    RRETURN(hr);
}

IFACEMETHODIMP SplitMenuFlyoutItemAutomationPeer::GetBoundingRectangleCore(_Out_ wf::Rect* returnValue)
{
    HRESULT hr = S_OK;
    IFCPTR(returnValue);
    
    // Initialize to empty rectangle
    returnValue->X = 0;
    returnValue->Y = 0;
    returnValue->Width = 0;
    returnValue->Height = 0;

    // Get the bounding rectangle from the primary button since that's where
    // narrator focus will be and what users will interact with
    xaml_automation_peers::IAutomationPeer* pPrimaryButtonPeer = GetPrimaryButtonPeer();
    
    if (pPrimaryButtonPeer)
    {
        IFC(pPrimaryButtonPeer->GetBoundingRectangle(returnValue));
    }
    else
    {
        // Fall back to base implementation if no primary button
        IFC(SplitMenuFlyoutItemAutomationPeerGenerated::GetBoundingRectangleCore(returnValue));
    }

Cleanup:
    ReleaseInterface(pPrimaryButtonPeer);
    RRETURN(hr);
}

IFACEMETHODIMP SplitMenuFlyoutItemAutomationPeer::GetChildrenCore(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** returnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml_automation_peers::AutomationPeer*>> spChildrenPeers;
    ctl::ComPtr<wfc::IVector<xaml_automation_peers::AutomationPeer*>> spFilteredPeers;

    IFCPTR(returnValue);
    *returnValue = nullptr;

    // Get the default children from the base implementation
    IFC(SplitMenuFlyoutItemAutomationPeerGenerated::GetChildrenCore(&spChildrenPeers));

    // Filter out the primary button from the children, similar to how Expander excludes the toggle button
    if (spChildrenPeers)
    {    
        UINT count = 0;
        IFC(spChildrenPeers->get_Size(&count));
        IFC(ctl::ComObject<TrackerCollection<xaml_automation_peers::AutomationPeer*>>::CreateInstance(spFilteredPeers.ReleaseAndGetAddressOf()));
        
        wrl_wrappers::HStringReference primaryButtonAutomationIdRef(c_primaryButtonAutomationId);

        for (UINT i = 0; i < count; i++)
        {
            ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spPeer;
            IFC(spChildrenPeers->GetAt(i, &spPeer));

            wrl_wrappers::HString automationId;
            IFC(spPeer->GetAutomationId(automationId.GetAddressOf()));
            
            INT32 comparisonResult = 0;
            IFC(WindowsCompareStringOrdinal(automationId.Get(), primaryButtonAutomationIdRef.Get(), &comparisonResult));
            
            if (comparisonResult != 0)
            {
                IFC(spFilteredPeers->Append(spPeer.Get()));
            }
        }
        
        *returnValue = spFilteredPeers.Detach();
    }

Cleanup:
    RRETURN(hr);
}

xaml_automation_peers::IAutomationPeer* SplitMenuFlyoutItemAutomationPeer::GetPrimaryButtonPeer()
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAutomationPeer* pPrimaryButtonPeer = nullptr;
    ctl::ComPtr<xaml::IUIElement> spOwner;
    ctl::ComPtr<SplitMenuFlyoutItem> spOwnerAsSplitMenuFlyoutItem;
    ctl::ComPtr<xaml_primitives::IButtonBase> spPrimaryButton;
    ctl::ComPtr<xaml::IUIElement> spPrimaryButtonAsUIE;

    IFC(get_Owner(&spOwner));
    IFC(spOwner.As(&spOwnerAsSplitMenuFlyoutItem));

    spPrimaryButton = spOwnerAsSplitMenuFlyoutItem->m_tpPrimaryButton.GetSafeReference();
    
    if (spPrimaryButton)
    {
        IFC(spPrimaryButton.As(&spPrimaryButtonAsUIE));
        IFC(spPrimaryButtonAsUIE.Cast<UIElement>()->GetOrCreateAutomationPeer(&pPrimaryButtonPeer));
    }

Cleanup:
    // Return the peer (could be null if primary button doesn't exist or doesn't have a peer)
    return pPrimaryButtonPeer;
}

// IExpandCollapseProvider
_Check_return_ HRESULT SplitMenuFlyoutItemAutomationPeer::ExpandImpl()
{
    ctl::ComPtr<xaml::IUIElement> spOwner;
    IFC_RETURN(get_Owner(&spOwner));

    IFC_RETURN(spOwner.Cast<SplitMenuFlyoutItem>()->Open());
    return S_OK;
}

_Check_return_ HRESULT SplitMenuFlyoutItemAutomationPeer::CollapseImpl()
{
    ctl::ComPtr<xaml::IUIElement> spOwner;
    IFC_RETURN(get_Owner(&spOwner));

    IFC_RETURN(spOwner.Cast<SplitMenuFlyoutItem>()->Close());
    return S_OK;
}

_Check_return_ HRESULT SplitMenuFlyoutItemAutomationPeer::get_ExpandCollapseStateImpl(_Out_ xaml_automation::ExpandCollapseState* pReturnValue)
{
    IFCPTR_RETURN(pReturnValue);
    
    ctl::ComPtr<xaml::IUIElement> spOwner;
    IFC_RETURN(get_Owner(&spOwner));

    BOOLEAN isOpen = FALSE;
    IFC_RETURN(spOwner.Cast<SplitMenuFlyoutItem>()->get_IsOpen(&isOpen));

    *pReturnValue = isOpen ? xaml_automation::ExpandCollapseState_Expanded : xaml_automation::ExpandCollapseState_Collapsed;
    return S_OK;
}

_Check_return_ HRESULT SplitMenuFlyoutItemAutomationPeer::RaiseExpandCollapseAutomationEvent(_In_ BOOLEAN isOpen)
{
    xaml_automation::ExpandCollapseState oldValue;
    xaml_automation::ExpandCollapseState newValue;
    CValue valueOld;
    CValue valueNew;

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

