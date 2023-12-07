// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ToggleMenuFlyoutItem.g.h"
#include "ToggleMenuFlyoutItemAutomationPeer.g.h"
#include "MenuFlyoutPresenter.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Change to the correct visual state for the MenuFlyoutItem.
_Check_return_ HRESULT 
ToggleMenuFlyoutItem::ChangeVisualState(
    // true to use transitions when updating the visual state, false
    // to snap directly to the new visual state.
    _In_ bool bUseTransitions)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsPressed = FALSE;
    BOOLEAN bIsPointerOver = FALSE;
    BOOLEAN bIsEnabled = FALSE;
    BOOLEAN bIsChecked = FALSE;
    bool hasIconMenuItem = false;
    bool hasMenuItemWithKeyboardAcceleratorText = false;
    BOOLEAN bIgnored = FALSE;
    bool shouldBeNarrow = false;
    xaml::FocusState focusState = xaml::FocusState_Unfocused;
    ctl::ComPtr<MenuFlyoutPresenter> spPresenter;
    bool isKeyboardPresent = false;

    GetIsPressed(&bIsPressed);
    GetIsPointerOver(&bIsPointerOver);
    IFC(get_IsEnabled(&bIsEnabled));
    IFC(get_IsChecked(&bIsChecked));
    IFC(get_FocusState(&focusState));
    IFC(GetShouldBeNarrow(&shouldBeNarrow));

    IFC(GetParentMenuFlyoutPresenter(&spPresenter));
    if (spPresenter)
    {
        hasIconMenuItem = spPresenter->GetContainsIconItems();
        hasMenuItemWithKeyboardAcceleratorText = spPresenter->GetContainsItemsWithKeyboardAcceleratorText();
    }
    
    // We only care about finding if we have a keyboard if we also have a menu item with accelerator text,
    // since if we don't have any menu items with accelerator text, we won't be showing any accelerator text anyway.
    if (hasMenuItemWithKeyboardAcceleratorText)
    {
        isKeyboardPresent = DXamlCore::GetCurrent()->GetIsKeyboardPresent();
    }
    
    // CommonStates
    if (!bIsEnabled)
    {
        IFC(GoToState(bUseTransitions, L"Disabled", &bIgnored));
    }
    else if (bIsPressed)
    {
        IFC(GoToState(bUseTransitions, L"Pressed", &bIgnored));
    }    
    else if (bIsPointerOver)
    {
        IFC(GoToState(bUseTransitions, L"PointerOver", &bIgnored));
    }
    else
    {
        IFC(GoToState(bUseTransitions, L"Normal", &bIgnored));
    }

    // FocusStates
    if (xaml::FocusState_Unfocused != focusState && bIsEnabled)
    {
        if (xaml::FocusState_Pointer == focusState) 
        {
            IFC(GoToState(bUseTransitions, L"PointerFocused", &bIgnored));
        }
        else
        {
            IFC(GoToState(bUseTransitions, L"Focused", &bIgnored));
        }
    }
    else
    {
        IFC(GoToState(bUseTransitions, L"Unfocused", &bIgnored));
    }

    // CheckStates
    if (bIsChecked && hasIconMenuItem)
    {
        IFC(GoToState(bUseTransitions, L"CheckedWithIcon", &bIgnored));
    }
    else if (hasIconMenuItem)
    {
        IFC(GoToState(bUseTransitions, L"UncheckedWithIcon", &bIgnored));
    }
    else if (bIsChecked)
    {
        IFC(GoToState(bUseTransitions, L"Checked", &bIgnored));
    }
    else
    {
        IFC(GoToState(bUseTransitions, L"Unchecked", &bIgnored));
    }

    // PaddingSizeStates
    if (shouldBeNarrow)
    {
        IFC(GoToState(bUseTransitions, L"NarrowPadding", &bIgnored));
    }
    else
    {
        IFC(GoToState(bUseTransitions, L"DefaultPadding", &bIgnored));
    }

    // We'll make the accelerator text visible if any item has accelerator text,
    // as this causes the margin to be applied which reserves space, ensuring that accelerator text
    // in one item won't be at the same horizontal position as label text in another item.
    if (hasMenuItemWithKeyboardAcceleratorText && isKeyboardPresent)
    {
        IFC(GoToState(bUseTransitions, L"KeyboardAcceleratorTextVisible", &bIgnored));
    }
    else
    {
        IFC(GoToState(bUseTransitions, L"KeyboardAcceleratorTextCollapsed", &bIgnored));
    }

Cleanup:
    RRETURN(hr);
}

// Performs appropriate actions upon a mouse/keyboard invocation of a MenuFlyoutItem.
_Check_return_ HRESULT
ToggleMenuFlyoutItem::Invoke()
{
    HRESULT hr = S_OK;
    BOOLEAN isChecked = FALSE;
    
    IFC(get_IsChecked(&isChecked));
    IFC(put_IsChecked(!isChecked));

    IFC(ToggleMenuFlyoutItemGenerated::Invoke());

Cleanup:
    RRETURN(hr);
}

// Handle the custom property changed event and call the OnPropertyChanged2 methods.
_Check_return_ HRESULT 
ToggleMenuFlyoutItem::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;
    BOOLEAN bAutomationListener = FALSE;

    IFC(ToggleMenuFlyoutItemGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::ToggleMenuFlyoutItem_IsChecked:
        IFC(UpdateVisualState());

        IFC(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_PropertyChanged, &bAutomationListener));
        if (bAutomationListener)
        {
            ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeer;
            ctl::ComPtr<xaml_automation_peers::IToggleMenuFlyoutItemAutomationPeer> spToggleButtonAutomationPeer;
            
            IFC(GetOrCreateAutomationPeer(&spAutomationPeer));
            if(spAutomationPeer)
            {
                ctl::ComPtr<IInspectable> spOldValue, spNewValue;
                IFC(CValueBoxer::UnboxObjectValue(args.m_pOldValue, args.m_pDP->GetPropertyType(), &spOldValue));
                IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, args.m_pDP->GetPropertyType(), &spNewValue));

                IFC(spAutomationPeer.As(&spToggleButtonAutomationPeer));
                IFC(spToggleButtonAutomationPeer.Cast<ToggleMenuFlyoutItemAutomationPeer>()->RaiseToggleStatePropertyChangedEvent(spOldValue.Get(), spNewValue.Get()));
            }
        }

        break;
    }

Cleanup:
    RRETURN(hr);
}

// Create ToggleMenuFlyoutItemAutomationPeer to represent the ToggleMenuFlyoutItem.
IFACEMETHODIMP ToggleMenuFlyoutItem::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IToggleMenuFlyoutItemAutomationPeer> spToggleMenuFlyoutItemAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IToggleMenuFlyoutItemAutomationPeerFactory> spToggleMenuFlyoutItemAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;
    
    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::ToggleMenuFlyoutItemAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As<xaml_automation_peers::IToggleMenuFlyoutItemAutomationPeerFactory>(&spToggleMenuFlyoutItemAPFactory));

    IFC(spToggleMenuFlyoutItemAPFactory.Cast<ToggleMenuFlyoutItemAutomationPeerFactory>()->CreateInstanceWithOwner(this, 
        NULL, 
        &spInner, 
        &spToggleMenuFlyoutItemAutomationPeer));
    IFC(spToggleMenuFlyoutItemAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}
