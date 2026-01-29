// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SplitMenuFlyoutItem.g.h"
#include "SplitMenuFlyoutItemAutomationPeer.g.h"
#include "AutomationPeer.g.h"
#include "AutomationProperties.h"
#include "MenuFlyoutPresenter.g.h"
#include "MenuFlyoutItemBaseCollection.g.h"
#include "MenuFlyout.g.h"
#include "MenuPopupThemeTransition.g.h"
#include "Popup.g.h"
#include "VisualTreeHelper.h"
#include "CascadingMenuHelper.h"
#include "ElevationHelper.h"
#include "XamlRoot.g.h"
#include "ElementSoundPlayerService_Partial.h"
#include "ButtonBase.g.h"
#include "localizedResource.h"
#include "WrlHelper.h"
#include "XamlTraceLogging.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Template part names
const WCHAR SplitMenuFlyoutItem::c_primaryButtonName[] = L"PrimaryButton";
const WCHAR SplitMenuFlyoutItem::c_secondaryButtonName[] = L"SecondaryButton";

SplitMenuFlyoutItem::SplitMenuFlyoutItem()
{
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::PrepareState()
{
    ctl::ComPtr<MenuFlyoutItemBaseCollection> spItems;

    IFC_RETURN(SplitMenuFlyoutItemGenerated::PrepareState());

    // Create the sub menu items collection and set the owner
    IFC_RETURN(ctl::make(&spItems));
    IFC_RETURN(CoreImports::Collection_SetOwner(static_cast<CCollection*>(spItems->GetHandle()), GetHandle()));
    SetPtrValue(m_tpItems, spItems);
    IFC_RETURN(put_Items(spItems.Get()));

    ctl::ComPtr<CascadingMenuHelper> menuHelper;
    IFC_RETURN(ctl::make(this, &menuHelper));
    SetPtrValue(m_menuHelper, menuHelper.Get());

    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::DisconnectFrameworkPeerCore()
{
    if (m_tpItems.GetAsCoreDO() != nullptr)
    {
        IFC_RETURN(CoreImports::Collection_Clear(static_cast<CCollection*>(m_tpItems.GetAsCoreDO())));
        IFC_RETURN(CoreImports::Collection_SetOwner(static_cast<CCollection*>(m_tpItems.GetAsCoreDO()), nullptr));
    }

    IFC_RETURN(SplitMenuFlyoutItemGenerated::DisconnectFrameworkPeerCore());

    return S_OK;
}

IFACEMETHODIMP
SplitMenuFlyoutItem::OnApplyTemplate()
{
    // Unhook old event handlers if they exist
    IFC_RETURN(UnhookTemplate());

    m_hasSecondaryButtonCustomAutomationName = false;

    IFC_RETURN(SplitMenuFlyoutItemGenerated::OnApplyTemplate());

    ctl::ComPtr<xaml_primitives::IButtonBase> spPrimaryButtonPart;
    ctl::ComPtr<xaml_primitives::IButtonBase> spSecondaryButtonPart;

    // Get template parts
    IFC_RETURN(GetTemplatePart<xaml_primitives::IButtonBase>(STR_LEN_PAIR(c_primaryButtonName), spPrimaryButtonPart.ReleaseAndGetAddressOf()));
    IFC_RETURN(GetTemplatePart<xaml_primitives::IButtonBase>(STR_LEN_PAIR(c_secondaryButtonName), spSecondaryButtonPart.ReleaseAndGetAddressOf()));

    SetPtrValue(m_tpPrimaryButton, spPrimaryButtonPart.Get());
    SetPtrValue(m_tpSecondaryButton, spSecondaryButtonPart.Get());

    // Set automation name for the secondary button
    if (m_tpSecondaryButton)
    {
        ButtonBase* pSecondaryButtonBase = static_cast<ButtonBase*>(m_tpSecondaryButton.Get());
        wrl_wrappers::HString secondaryButtonAutomationName;
        IFC_RETURN(AutomationProperties::GetNameStatic(pSecondaryButtonBase, secondaryButtonAutomationName.ReleaseAndGetAddressOf()));

        if (secondaryButtonAutomationName.Get() == nullptr)
        {
            BOOLEAN isOpen = FALSE;
            IFC_RETURN(get_IsOpen(&isOpen));
            IFC_RETURN(SetSecondaryButtonAutomationName(!!isOpen));
        }
        else
        {
            // Since a custom Automation Name is set in Xaml markup, do not overwrite it here, or in SetIsOpen.
            m_hasSecondaryButtonCustomAutomationName = true;
        }
    }

    if (m_tpPrimaryButton)
    {
        ButtonBase* pButtonBase = static_cast<ButtonBase*>(m_tpPrimaryButton.Get());

        // Setting AutomationId on the primary button to allow peer comparison
        // Used by AutomationPeer when GetChildren is called.
        IFC_RETURN(DirectUI::AutomationProperties::SetAutomationIdStatic(
            pButtonBase, wrl_wrappers::HStringReference(L"SplitMenuFlyoutItemPrimaryButton").Get()));

        // Promote primary button's AutomationProperties.Name to the SplitMenuFlyoutItem if not already set.    
        wrl_wrappers::HString automationName;
        IFC_RETURN(DirectUI::AutomationProperties::GetNameStatic(this, automationName.ReleaseAndGetAddressOf()));
        
        if(automationName.Get() == nullptr)
        {
            wrl_wrappers::HString buttonAutomationName;
            IFC_RETURN(DirectUI::AutomationProperties::GetNameStatic(pButtonBase, buttonAutomationName.ReleaseAndGetAddressOf()));
            if(buttonAutomationName.Get() != nullptr)
            {
                IFC_RETURN(DirectUI::AutomationProperties::SetNameStatic(this, buttonAutomationName.Get()));
            }    
        }    

        // Set the events source of the primary button peer to the SplitMenuFlyoutItem's automation peer.
        // This ensures that when focus moves to the primary button, the narrator will announce
        // the SplitMenuFlyoutItem's expand/collapse state and position in set, rather than
        // just announcing the button itself. [ See Expander control for more reference. ]
        ctl::ComPtr<xaml_automation_peers::IAutomationPeer> primaryButtonAutomationPeer;
        IFC_RETURN(pButtonBase->GetOrCreateAutomationPeer(&primaryButtonAutomationPeer));

        if (primaryButtonAutomationPeer)
        {
            AutomationPeer* pPrimaryButtonAP = static_cast<AutomationPeer*>(primaryButtonAutomationPeer.Get());

            ctl::ComPtr<xaml_automation_peers::IAutomationPeer> splitMenuFlyoutItemPeer;
            IFC_RETURN(GetOrCreateAutomationPeer(&splitMenuFlyoutItemPeer));
            
            if (splitMenuFlyoutItemPeer)
            {
                AutomationPeer* pSplitMenuFlyoutItemAP = static_cast<AutomationPeer*>(splitMenuFlyoutItemPeer.Get());

                ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spEventsSourceAP;
                IFC_RETURN(pSplitMenuFlyoutItemAP->get_EventsSource(&spEventsSourceAP));
                if (spEventsSourceAP)
                {
                    IFC_RETURN(pPrimaryButtonAP->put_EventsSource(spEventsSourceAP.Get()));
                }    
                else
                {
                    IFC_RETURN(pPrimaryButtonAP->put_EventsSource(pSplitMenuFlyoutItemAP));
                }    
            }    

        }    
    }        
    
    IFC_RETURN(HookTemplate());
    IFC_RETURN(m_menuHelper->OnApplyTemplate());
    IFC_RETURN(UpdateVisualState(FALSE));
    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::HookTemplate()
{
    if (m_tpPrimaryButton)
    {
        IFC_RETURN(m_epPrimaryButtonPointerEnteredHandler.AttachEventHandler(
            m_tpPrimaryButton.Cast<ButtonBase>(), 
            [this](IInspectable* pSender, IPointerRoutedEventArgs* pArgs)
        {
            IFC_RETURN(OnPrimaryButtonPointerEntered(pSender, pArgs));
            return S_OK;
        }));

        IFC_RETURN(m_epPrimaryButtonPointerExitedHandler.AttachEventHandler(
            m_tpPrimaryButton.Cast<ButtonBase>(), 
            [this](IInspectable* pSender, IPointerRoutedEventArgs* pArgs)
        {
            IFC_RETURN(OnPrimaryButtonPointerExited(pSender, pArgs));
            return S_OK;
        }));

        IFC_RETURN(m_epPrimaryButtonClickHandler.AttachEventHandler(
            m_tpPrimaryButton.Cast<ButtonBase>(), 
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
        {
            IFC_RETURN(OnPrimaryButtonClick(pSender, pArgs));
            return S_OK;
        }));

        IFC_RETURN(m_epPrimaryButtonGotFocusHandler.AttachEventHandler(
            m_tpPrimaryButton.Cast<ButtonBase>(), 
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
        {
                IFC_RETURN(UpdateVisualState());
                return S_OK;
        }));

        IFC_RETURN(m_epPrimaryButtonLostFocusHandler.AttachEventHandler(
            m_tpPrimaryButton.Cast<ButtonBase>(), 
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
        {
                IFC_RETURN(UpdateVisualState());
                return S_OK;
        }));
    }

    if (m_tpSecondaryButton)
    {
        IFC_RETURN(m_epSecondaryButtonPointerEnteredHandler.AttachEventHandler(
            m_tpSecondaryButton.Cast<ButtonBase>(), 
            [this](IInspectable* pSender, IPointerRoutedEventArgs* pArgs)
        {
            IFC_RETURN(OnSecondaryButtonPointerEntered(pSender, pArgs));
            return S_OK;
        }));

        IFC_RETURN(m_epSecondaryButtonPointerExitedHandler.AttachEventHandler(
            m_tpSecondaryButton.Cast<ButtonBase>(), 
            [this](IInspectable* pSender, IPointerRoutedEventArgs* pArgs)
        {
            IFC_RETURN(OnSecondaryButtonPointerExited(pSender, pArgs));
            return S_OK;
        }));

        IFC_RETURN(m_epSecondaryButtonClickHandler.AttachEventHandler(
            m_tpSecondaryButton.Cast<ButtonBase>(), 
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
        {
            IFC_RETURN(OnSecondaryButtonClick(pSender, pArgs));
            return S_OK;
        }));

        IFC_RETURN(m_epSecondaryButtonGotFocusHandler.AttachEventHandler(
            m_tpSecondaryButton.Cast<ButtonBase>(), 
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
        {
            IFC_RETURN(UpdateVisualState());
            return S_OK;
        }));

        IFC_RETURN(m_epSecondaryButtonLostFocusHandler.AttachEventHandler(
            m_tpSecondaryButton.Cast<ButtonBase>(), 
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
        {
            IFC_RETURN(UpdateVisualState());
            return S_OK;
        }));
    }

    return S_OK; 
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::UnhookTemplate()
{
    if (m_tpPrimaryButton)
    {
        if (auto peg = m_tpPrimaryButton.TryMakeAutoPeg())
        {
            IFC_RETURN(m_epPrimaryButtonPointerEnteredHandler.DetachEventHandler(m_tpPrimaryButton.Cast<IInspectable>()));
            IFC_RETURN(m_epPrimaryButtonPointerExitedHandler.DetachEventHandler(m_tpPrimaryButton.Cast<IInspectable>()));
            IFC_RETURN(m_epPrimaryButtonClickHandler.DetachEventHandler(m_tpPrimaryButton.Cast<IInspectable>()));
            IFC_RETURN(m_epPrimaryButtonGotFocusHandler.DetachEventHandler(m_tpPrimaryButton.Cast<IInspectable>()));
            IFC_RETURN(m_epPrimaryButtonLostFocusHandler.DetachEventHandler(m_tpPrimaryButton.Cast<IInspectable>()));
        }
        m_tpPrimaryButton.Clear();
    }

    if (m_tpSecondaryButton)
    {
        if (auto peg = m_tpSecondaryButton.TryMakeAutoPeg())
        {
            IFC_RETURN(m_epSecondaryButtonPointerEnteredHandler.DetachEventHandler(m_tpSecondaryButton.Cast<IInspectable>()));
            IFC_RETURN(m_epSecondaryButtonPointerExitedHandler.DetachEventHandler(m_tpSecondaryButton.Cast<IInspectable>()));
            IFC_RETURN(m_epSecondaryButtonClickHandler.DetachEventHandler(m_tpSecondaryButton.Cast<IInspectable>()));
            IFC_RETURN(m_epSecondaryButtonGotFocusHandler.DetachEventHandler(m_tpSecondaryButton.Cast<IInspectable>()));
            IFC_RETURN(m_epSecondaryButtonLostFocusHandler.DetachEventHandler(m_tpSecondaryButton.Cast<IInspectable>()));
        }
        m_tpSecondaryButton.Clear();
    }

    return S_OK;
}

IFACEMETHODIMP
SplitMenuFlyoutItem::OnPointerEntered(_In_ xaml_input::IPointerRoutedEventArgs* args)
{
    IFC_RETURN(SplitMenuFlyoutItemGenerated::OnPointerEntered(args));
    IFC_RETURN(UpdateParentOwner(nullptr /*parentMenuFlyoutPresenter*/));
    // Don't open submenu here - let button-specific handlers control this
    return S_OK;
}

IFACEMETHODIMP
SplitMenuFlyoutItem::OnPointerExited(_In_ xaml_input::IPointerRoutedEventArgs* args)
{
    IFC_RETURN(SplitMenuFlyoutItemGenerated::OnPointerExited(args));

    bool parentIsSubMenu = false;
    ctl::ComPtr<MenuFlyoutPresenter> parentPresenter;
    IFC_RETURN(GetParentMenuFlyoutPresenter(&parentPresenter));

    if (parentPresenter)
    {
        parentIsSubMenu = parentPresenter->IsSubPresenter();
    }

    IFC_RETURN(m_menuHelper->OnPointerExited(args, parentIsSubMenu));
    return S_OK;
}

IFACEMETHODIMP
SplitMenuFlyoutItem::OnGotFocus(_In_ xaml::IRoutedEventArgs* args)
{
    IFC_RETURN(SplitMenuFlyoutItemGenerated::OnGotFocus(args));
    
    // When the SplitMenuFlyoutItem receives focus via keyboard, 
    // redirect focus to the appropriate button.
    xaml::FocusState focusState = xaml::FocusState_Unfocused;
    IFC_RETURN(get_FocusState(&focusState));
    
    bool hasPrimaryFocus = false;
    bool hasSecondaryFocus = false;
    IFC_RETURN(HasPrimaryButtonFocus(&hasPrimaryFocus));
    IFC_RETURN(HasSecondaryButtonFocus(&hasSecondaryFocus));

    // If the focus is already set on any button, do not change it.
    if ((focusState == xaml::FocusState_Keyboard || focusState == xaml::FocusState_Programmatic) && 
        !hasPrimaryFocus && !hasSecondaryFocus)
    {
        // Focus coming from submenu closure or upward navigation - set focus to secondary button
        bool focusSecondaryButton = (m_focusComingFromSubmenu || m_focusComingFromUpwardNavigation) && m_tpSecondaryButton;
        if (focusSecondaryButton || m_tpPrimaryButton)
        {
            SetButtonFocus(focusSecondaryButton, focusState);
        }
    }
    
    m_focusComingFromSubmenu = false;
    m_focusComingFromUpwardNavigation = false;
    IFC_RETURN(m_menuHelper->OnGotFocus(args));
    return S_OK;
}

IFACEMETHODIMP
SplitMenuFlyoutItem::OnLostFocus(_In_ xaml::IRoutedEventArgs* args)
{
    IFC_RETURN(SplitMenuFlyoutItemGenerated::OnLostFocus(args));
    IFC_RETURN(m_menuHelper->OnLostFocus(args));
    return S_OK;
}

IFACEMETHODIMP
SplitMenuFlyoutItem::OnKeyDown(_In_ xaml_input::IKeyRoutedEventArgs* args)
{
    HRESULT hr = S_OK;
    BOOLEAN isHandled = false;
    IFC(args->get_Handled(&isHandled));

    if(isHandled)
    {
        goto Cleanup;
    }

    wsy::VirtualKey key = wsy::VirtualKey_None;
    IFC(args->get_Key(&key));

    const bool isLeftKey = (key == wsy::VirtualKey_Left || key == wsy::VirtualKey_GamepadDPadLeft || key == wsy::VirtualKey_GamepadLeftThumbstickLeft);
    const bool isRightKey = (key == wsy::VirtualKey_Right || key == wsy::VirtualKey_GamepadDPadRight || key == wsy::VirtualKey_GamepadLeftThumbstickRight);
    const bool isUpKey = (key == wsy::VirtualKey_Up || key == wsy::VirtualKey_GamepadDPadUp || key == wsy::VirtualKey_GamepadLeftThumbstickUp);
    const bool isDownKey = (key == wsy::VirtualKey_Down || key == wsy::VirtualKey_GamepadDPadDown || key == wsy::VirtualKey_GamepadLeftThumbstickDown);

    // Internal navigation between primary and secondary buttons logic
    bool primaryHasFocus = false;
    bool secondaryHasFocus = false;
    IFC(HasPrimaryButtonFocus(&primaryHasFocus));
    IFC(HasSecondaryButtonFocus(&secondaryHasFocus));
    
    if ((isLeftKey || isUpKey) && secondaryHasFocus)
    {
        IFC(SetButtonFocus(false, xaml::FocusState_Keyboard));
        IFC(args->put_Handled(TRUE));
        goto Cleanup;
    }
    else if ((isRightKey || isDownKey) && primaryHasFocus)
    {
        IFC(SetButtonFocus(true, xaml::FocusState_Keyboard));
        IFC(args->put_Handled(TRUE));
        goto Cleanup;
    }
    
    // Forward to external navigation logic if not handled internally
    BOOLEAN shouldHandleEvent = false;
    hr = KeyPress::MenuFlyout::KeyDown<SplitMenuFlyoutItem, MenuFlyoutPresenter>(key, this, &shouldHandleEvent);
    IFC(hr);

    // Similar to MenuFlyoutSubItem, we want to allow MenuHelper
    // to process the event even when external navigation takes place.
    IFC(m_menuHelper->OnKeyDown(args));
    IFC(args->put_Handled(!!shouldHandleEvent));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
SplitMenuFlyoutItem::OnKeyUp(_In_ xaml_input::IKeyRoutedEventArgs* args)
{
    HRESULT hr = S_OK;
    BOOLEAN isHandled = false;
    IFC(args->get_Handled(&isHandled));
    
    if(isHandled)
    {
        goto Cleanup;
    }

    wsy::VirtualKey key = wsy::VirtualKey_None;
    IFC(args->get_Key(&key));

    if (key == wsy::VirtualKey_Enter || 
        key == wsy::VirtualKey_Space || 
        key == wsy::VirtualKey_GamepadA)
    {
        ctl::ComPtr<RoutedEventArgs> spArgs;
        IFC(ctl::make(&spArgs));

        bool primaryHasFocus = false;
        bool secondaryHasFocus = false;
        IFC(HasPrimaryButtonFocus(&primaryHasFocus));
        IFC(HasSecondaryButtonFocus(&secondaryHasFocus));

        if (primaryHasFocus)
        {
            IFC(OnPrimaryButtonClick(m_tpPrimaryButton.Get(), spArgs.Get()));
            IFC(args->put_Handled(TRUE));
            goto Cleanup;
        }
        else if (secondaryHasFocus)
        {
            IFC(OnSecondaryButtonClick(m_tpSecondaryButton.Get(), spArgs.Get()));
            IFC(args->put_Handled(TRUE));
            goto Cleanup;
        }
    }

    // Forward to parent handler ( this method is called by MenuFlyoutItem )
    // We are using it here directly to prevent setting the event as handled.
    BOOLEAN shouldHandleEvent = false;
    IFC(KeyPress::MenuFlyout::KeyUp<SplitMenuFlyoutItem>(key, this, &shouldHandleEvent));

    // Similar to KeyDown, allow MenuHelper to process the event even if the control
    // should handle the event.
    IFC(m_menuHelper->OnKeyUp(args));
    IFC(args->put_Handled(!!shouldHandleEvent));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::OnIsEnabledChanged(_In_ DirectUI::IsEnabledChangedEventArgs* args)
{
    IFC_RETURN(SplitMenuFlyoutItemGenerated::OnIsEnabledChanged(args));
    IFC_RETURN(m_menuHelper->OnIsEnabledChanged(args));
    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::OnVisibilityChanged()
{
    IFC_RETURN(SplitMenuFlyoutItemGenerated::OnVisibilityChanged());
    IFC_RETURN(m_menuHelper->OnVisibilityChanged());
    return S_OK;
}

_Check_return_ HRESULT 
SplitMenuFlyoutItem::ChangeVisualState(
    _In_ bool bUseTransitions)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsEnabled = FALSE;
    bool hasToggleMenuItem = false;
    bool hasIconMenuItem = false;
    bool hasMenuItemWithKeyboardAcceleratorText = false;
    BOOLEAN bIgnored = FALSE;
    bool shouldBeNarrow = false;
    xaml::FocusState focusState = xaml::FocusState_Unfocused;
    ctl::ComPtr<MenuFlyoutPresenter> spPresenter;
    bool isKeyboardPresent = false;
    BOOLEAN isPrimaryButtonPressed = FALSE;
    BOOLEAN isSecondaryButtonPressed = FALSE;
    BOOLEAN bIsPopupOpened = FALSE;
    BOOLEAN bIsDelayCloseTimerRunning = FALSE;
    bool showSubMenuOpenedState = false;

    IFC(get_IsEnabled(&bIsEnabled));
    IFC(get_FocusState(&focusState));
    IFC(GetShouldBeNarrow(&shouldBeNarrow));

    BOOLEAN isPrimaryButtonPointerOver = FALSE;
    BOOLEAN isSecondaryButtonPointerOver = FALSE;

    if (m_tpPrimaryButton)
    {
		IFC(m_tpPrimaryButton.Cast<ButtonBase>()->get_IsPressed(&isPrimaryButtonPressed));
        IFC(m_tpPrimaryButton.Cast<ButtonBase>()->get_IsPointerOver(&isPrimaryButtonPointerOver));
    }
    if (m_tpSecondaryButton)
    {
        IFC(m_tpSecondaryButton.Cast<ButtonBase>()->get_IsPressed(&isSecondaryButtonPressed));
        IFC(m_tpSecondaryButton.Cast<ButtonBase>()->get_IsPointerOver(&isSecondaryButtonPointerOver));
    }

    IFC(GetParentMenuFlyoutPresenter(&spPresenter));
    if (spPresenter)
    {
        hasToggleMenuItem = spPresenter->GetContainsToggleItems();
        hasIconMenuItem = spPresenter->GetContainsIconItems();
        hasMenuItemWithKeyboardAcceleratorText = spPresenter->GetContainsItemsWithKeyboardAcceleratorText();
    }
    
    // Check if submenu is opened (following MenuFlyoutSubItem pattern)
    if(m_tpPopup)
    {
        IFC(m_tpPopup->get_IsOpen(&bIsPopupOpened));
    }

    IFC(m_menuHelper->IsDelayCloseTimerRunning(&bIsDelayCloseTimerRunning));
    if(bIsPopupOpened && !bIsDelayCloseTimerRunning)
    {
        showSubMenuOpenedState = true;
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
    else if (showSubMenuOpenedState)
    {
        IFC(GoToState(bUseTransitions, L"SubMenuOpened", &bIgnored));
    }
    else if (isPrimaryButtonPressed)
    {
        // Primary button is being pressed
        IFC(GoToState(bUseTransitions, L"PrimaryPressed", &bIgnored));
    }
    else if (isSecondaryButtonPressed)
    {
        // Secondary button is being pressed
        IFC(GoToState(bUseTransitions, L"SecondaryPressed", &bIgnored));
    }
    else if (isPrimaryButtonPointerOver)
    {
        // Pointer is over primary button area
        IFC(GoToState(bUseTransitions, L"PrimaryPointerOver", &bIgnored));
    }
    else if (isSecondaryButtonPointerOver)
    {
        // Pointer is over secondary button area
        IFC(GoToState(bUseTransitions, L"SecondaryPointerOver", &bIgnored));
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

    // CheckPlaceholderStates
    if (hasToggleMenuItem && hasIconMenuItem)
    {
        IFC(GoToState(bUseTransitions, L"CheckAndIconPlaceholder", &bIgnored));
    }
    else if (hasToggleMenuItem)
    {
        IFC(GoToState(bUseTransitions, L"CheckPlaceholder", &bIgnored));
    }
    else if (hasIconMenuItem)
    {
        IFC(GoToState(bUseTransitions, L"IconPlaceholder", &bIgnored));
    }
    else
    {
        IFC(GoToState(bUseTransitions, L"NoPlaceholder", &bIgnored));
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

    // As per the design team, the requirement is to not show the keyboard accelerator text
    // for this control. However, the developers may want to show it in their custom styles.
    // In order to support that scenario, I have added the visual states here, which the developers
    // can decide to handle in their custom styles or not.
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

// Handle the custom property changed event and call the OnPropertyChanged2 methods.
_Check_return_ HRESULT 
SplitMenuFlyoutItem::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;
    IFC(SplitMenuFlyoutItemGenerated::OnPropertyChanged2(args));

    if (KnownPropertyIndex::SplitMenuFlyoutItem_SubMenuPresenterStyle == args.m_pDP->GetIndex() &&
        m_tpPresenter.Get() != nullptr)
    {
        ctl::ComPtr<IInspectable> spInspectable;
        ctl::ComPtr<IStyle> spStyle;
        IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, args.m_pDP->GetPropertyType(), &spInspectable));
        if (spInspectable)
        {
            IFC(spInspectable.As(&spStyle));
        }
        IFC(SetPresenterStyle(m_tpPresenter.Cast<Control>(), spStyle.Get()));
    }

    if (KnownPropertyIndex::SplitMenuFlyoutItem_SubMenuItemStyle == args.m_pDP->GetIndex())
    {
        IFC(ApplySubMenuItemStyleToItems());
    }

    // Update secondary button automation name when Text property changes
    if (KnownPropertyIndex::MenuFlyoutItem_Text == args.m_pDP->GetIndex() ||
        KnownPropertyIndex::AutomationProperties_Name == args.m_pDP->GetIndex())
    {
        if(m_tpSecondaryButton && !m_hasSecondaryButtonCustomAutomationName)
        {
            BOOLEAN isOpen = FALSE;
            IFC(get_IsOpen(&isOpen));
            IFC(SetSecondaryButtonAutomationName(!!isOpen));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Create SplitMenuFlyoutItemAutomationPeer to represent the SplitMenuFlyoutItem.
IFACEMETHODIMP SplitMenuFlyoutItem::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    *ppAutomationPeer = nullptr;
    ctl::ComPtr<SplitMenuFlyoutItemAutomationPeer> spAutomationPeer;
    IFC_RETURN(ActivationAPI::ActivateAutomationInstance(KnownTypeIndex::SplitMenuFlyoutItemAutomationPeer, GetHandle(), spAutomationPeer.GetAddressOf()));
    IFC_RETURN(spAutomationPeer->put_Owner(this));
    *ppAutomationPeer = spAutomationPeer.Detach();
    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::QueueRefreshItemsSource()
{
    // The items source might change multiple times in a single tick, so we'll coalesce the refresh
    // into a single event once all of the changes have completed.
    if (m_tpPresenter && !m_itemsSourceRefreshPending)
    {
        ctl::ComPtr<msy::IDispatcherQueueStatics> dispatcherQueueStatics;
        ctl::ComPtr<msy::IDispatcherQueue> dispatcherQueue;

        IFC_RETURN(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Dispatching_DispatcherQueue).Get(),
            dispatcherQueueStatics.ReleaseAndGetAddressOf()));

        IFC_RETURN(dispatcherQueueStatics->GetForCurrentThread(&dispatcherQueue));

        ctl::WeakRefPtr wrThis;
        boolean enqueued = false;
        
        ctl::ComPtr<ISplitMenuFlyoutItem> thisAsSplitMenuFlyoutItem;
        thisAsSplitMenuFlyoutItem = ctl::interface_cast<ISplitMenuFlyoutItem>(this);
        IFC_RETURN(thisAsSplitMenuFlyoutItem.AsWeak(&wrThis));

        IFC_RETURN(dispatcherQueue->TryEnqueue(
            WRLHelper::MakeAgileCallback<msy::IDispatcherQueueHandler>([wrThis]() mutable {
                ctl::ComPtr<ISplitMenuFlyoutItem> thisSplitMenuFlyoutItem;
                IFC_RETURN(wrThis.As(&thisSplitMenuFlyoutItem));

                if (thisSplitMenuFlyoutItem)
                {
                    ctl::ComPtr<SplitMenuFlyoutItem> thisSplitMenuFlyoutItemImpl;
                    IFC_RETURN(thisSplitMenuFlyoutItem.As(&thisSplitMenuFlyoutItemImpl));
                    IFC_RETURN(thisSplitMenuFlyoutItemImpl->RefreshItemsSource());
                    IFC_RETURN(thisSplitMenuFlyoutItemImpl->ApplySubMenuItemStyleToItems());
                }

                return S_OK;
            }).Get(),
            &enqueued));

        IFCEXPECT_RETURN(enqueued);
        m_itemsSourceRefreshPending = true;
    }

    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::RefreshItemsSource()
{
    m_itemsSourceRefreshPending = false;

    ASSERT(m_tpPresenter);

    // Setting the items source to null and then back to Items causes the presenter to pick up any changes.
    IFC_RETURN(m_tpPresenter.Cast<ItemsControl>()->put_ItemsSource(nullptr));
    IFC_RETURN(m_tpPresenter.Cast<ItemsControl>()->put_ItemsSource(ctl::as_iinspectable(m_tpItems.Get())));

    return S_OK;
}

// ISubMenuOwner implementation
_Check_return_ HRESULT
SplitMenuFlyoutItem::get_IsSubMenuOpenImpl(_Out_ BOOLEAN* pValue)
{
    IFC_RETURN(get_IsOpen(pValue));
    return S_OK; 
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::get_ParentOwnerImpl(_Outptr_ xaml_controls::ISubMenuOwner** ppValue)
{
    ctl::ComPtr<ISubMenuOwner> parentOwner;
    parentOwner = m_wrParentOwner.AsOrNull<ISubMenuOwner>();
    *ppValue = parentOwner.Detach();

    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::put_ParentOwnerImpl(_In_ xaml_controls::ISubMenuOwner* pValue)
{
    IFC_RETURN(ctl::AsWeak(pValue, &m_wrParentOwner));
    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::SetSubMenuDirectionImpl(BOOLEAN isSubMenuDirectionUp)
{
    if (m_tpMenuPopupThemeTransition)
    {
        IFC_RETURN(m_tpMenuPopupThemeTransition.Cast<MenuPopupThemeTransition>()->put_Direction(
            isSubMenuDirectionUp ?
            xaml_primitives::AnimationDirection_Bottom :
            xaml_primitives::AnimationDirection_Top));
    }

    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::PrepareSubMenuImpl()
{
    IFC_RETURN(EnsurePopupAndPresenter());

    ASSERT(m_tpPopup);
    ASSERT(m_tpPresenter);

    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::OpenSubMenuImpl(wf::Point position)
{
    IFC_RETURN(EnsurePopupAndPresenter());
    IFC_RETURN(EnsureCloseExistingSubItems());

    ctl::ComPtr<MenuFlyoutPresenter> parentMenuFlyoutPresenter;
    IFC_RETURN(GetParentMenuFlyoutPresenter(&parentMenuFlyoutPresenter));

    ctl::ComPtr<MenuFlyout> parentMenuFlyout;
    if (parentMenuFlyoutPresenter)
    {
        ctl::ComPtr<IMenu> owningMenu;
        IFC_RETURN(parentMenuFlyoutPresenter->get_OwningMenu(&owningMenu));
        IFC_RETURN(m_tpPresenter.Cast<MenuFlyoutPresenter>()->put_OwningMenu(owningMenu.Get()));

        IFC_RETURN(parentMenuFlyoutPresenter->GetParentMenuFlyout(&parentMenuFlyout));

        if (parentMenuFlyout)
        {
            // Update the TemplateSettings before it is opened.
            IFC_RETURN(m_tpPresenter.Cast<MenuFlyoutPresenter>()->SetParentMenuFlyout(parentMenuFlyout.Get()));
            IFC_RETURN(m_tpPresenter.Cast<MenuFlyoutPresenter>()->UpdateTemplateSettings());

            // Forward the parent presenter's properties to the sub presenter
            IFC_RETURN(ForwardPresenterProperties(
                parentMenuFlyout.Get(),
                parentMenuFlyoutPresenter.Get(),
                m_tpPresenter.Cast<MenuFlyoutPresenter>()));
        }
    }

    IFC_RETURN(m_tpPopup->put_HorizontalOffset(position.X));
    IFC_RETURN(m_tpPopup->put_VerticalOffset(position.Y));
    IFC_RETURN(SetIsOpen(TRUE));

    if (parentMenuFlyout)
    {
        // Note: This is the call that propagates the SystemBackdrop object set on either this FlyoutBase or its
        // MenuFlyoutPresenter to the Popup. A convenient place to do this is in ForwardTargetPropertiesToPresenter, but we
        // see cases where the MenuFlyoutPresenter's SystemBackdrop property is null until it enters the tree via the
        // Popup::Open call above. So trying to propagate it before opening the popup actually finds no SystemBackdrop, and
        // the popup is left with a transparent background. Do the propagation after the popup opens instead. Windowed
        // popups support having a backdrop set after the popup is open.
        IFC_RETURN(ForwardSystemBackdropToPopup(parentMenuFlyout.Get()));
    }

    TraceLoggingWrite(
        g_hTraceProvider,
        "SplitMenuFlyoutItem-SubMenuOpened",
        TraceLoggingLevel(WINEVENT_LEVEL_LOG_ALWAYS),
        TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
        TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));

    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::PositionSubMenuImpl(wf::Point position)
{
    if (position.X != -std::numeric_limits<float>::infinity())
    {
        IFC_RETURN(m_tpPopup->put_HorizontalOffset(position.X));
    }

    if (position.Y != -std::numeric_limits<float>::infinity())
    {
        IFC_RETURN(m_tpPopup->put_VerticalOffset(position.Y));
    }

    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::ClosePeerSubMenusImpl()
{
    IFC_RETURN(EnsureCloseExistingSubItems());
    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::CloseSubMenuImpl()
{
    IFC_RETURN(SetIsOpen(FALSE));
    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::CloseSubMenuTreeImpl()
{
    IFC_RETURN(m_menuHelper->CloseChildSubMenus());
    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::DelayCloseSubMenuImpl()
{
    IFC_RETURN(m_menuHelper->DelayCloseSubMenu());
    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::CancelCloseSubMenuImpl()
{
    IFC_RETURN(m_menuHelper->CancelCloseSubMenu());
    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::RaiseAutomationPeerExpandCollapseImpl(_In_ BOOLEAN isOpen)
{
    ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeer;
    BOOLEAN isListener = FALSE;

    IFC_RETURN(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_PropertyChanged, &isListener));
    if (isListener)
    {
        IFC_RETURN(GetOrCreateAutomationPeer(&spAutomationPeer));
        if (spAutomationPeer)
        {
            IFC_RETURN(spAutomationPeer.Cast<SplitMenuFlyoutItemAutomationPeer>()->RaiseExpandCollapseAutomationEvent(isOpen));
        }
    }
    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::Open()
{
    IFC_RETURN(m_menuHelper->OpenSubMenu());
    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::Close()
{
    IFC_RETURN(m_menuHelper->CloseSubMenu());
    return S_OK;
}

// Helper methods delegated to CascadingMenuHelper
_Check_return_ HRESULT
SplitMenuFlyoutItem::CreateSubPresenter(_Outptr_ xaml_controls::IControl** ppReturnValue)
{
    ctl::ComPtr<MenuFlyoutPresenter> spPresenter;

    *ppReturnValue = nullptr;

    IFC_RETURN(ctl::make(&spPresenter));

    // Specify the sub MenuFlyoutPresenter
    spPresenter.Cast<MenuFlyoutPresenter>()->m_isSubPresenter = true;
    IFC_RETURN(spPresenter.Cast<MenuFlyoutPresenter>()->put_Owner(this));

    IFC_RETURN(spPresenter.MoveTo(ppReturnValue));

    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::EnsurePopupAndPresenter()
{

    if (!m_tpPopup)
    {
        ctl::ComPtr<MenuFlyout> spParentMenuFlyout;
        ctl::ComPtr<MenuFlyoutPresenter> spParentMenuFlyoutPresenter;
        ctl::ComPtr<IControl> spPresenter;
        ctl::ComPtr<IUIElement> spPresenterAsUI;
        ctl::ComPtr<IFrameworkElement> spPresenterAsFE;
        ctl::ComPtr<Popup> spPopup;

        IFC_RETURN(ctl::make(&spPopup));
        IFC_RETURN(spPopup->put_IsSubMenu(TRUE));

        IFC_RETURN(GetParentMenuFlyoutPresenter(&spParentMenuFlyoutPresenter));
        if (spParentMenuFlyoutPresenter)
        {
            IFC_RETURN(spParentMenuFlyoutPresenter->GetParentMenuFlyout(&spParentMenuFlyout));
            // Set the windowed Popup if the MenuFlyout is set the windowed Popup
            if (spParentMenuFlyout && spParentMenuFlyout->IsWindowedPopup())
            {
                IFC_RETURN(static_cast<CPopup*>(spPopup.Cast<Popup>()->GetHandle())->SetIsWindowed());

                // Ensure the sub menu is the windowed Popup
                ASSERT(static_cast<CPopup*>(spPopup.Cast<Popup>()->GetHandle())->IsWindowed());

                ctl::ComPtr<xaml::IXamlRoot> xamlRoot = XamlRoot::GetForElementStatic(spParentMenuFlyoutPresenter.Get());
                if (xamlRoot)
                {
                    IFC_RETURN(spPopup.Cast<Popup>()->put_XamlRoot(xamlRoot.Get()));
                }
            }
        }

        IFC_RETURN(CreateSubPresenter(&spPresenter));
        IFC_RETURN(spPresenter.As(&spPresenterAsUI));

        if (spParentMenuFlyoutPresenter)
        {
            UINT parentDepth = spParentMenuFlyoutPresenter->GetDepth();
            spPresenter.Cast<MenuFlyoutPresenter>()->SetDepth(parentDepth+1);
        }

        IFC_RETURN(spPopup->put_Child(spPresenterAsUI.Get()));

        IFC_RETURN(SetPtrValueWithQI(m_tpPresenter, spPresenter.Get()));
        IFC_RETURN(SetPtrValueWithQI(m_tpPopup, spPopup.Get()));

        IFC_RETURN(static_cast<ItemsControl*>(m_tpPresenter.Get())->put_ItemsSource(ctl::as_iinspectable(m_tpItems.Get())));

        // Apply SubMenuItemStyle to all items
        IFC_RETURN(ApplySubMenuItemStyleToItems());

        IFC_RETURN(spPresenter.As(&spPresenterAsFE));
        IFC_RETURN(m_epPresenterSizeChangedHandler.AttachEventHandler(
            spPresenterAsFE.Get(),
            [this](IInspectable* pSender, xaml::ISizeChangedEventArgs* args)
        {
            return OnPresenterSizeChanged(pSender, args);
        }));

        IFC_RETURN(m_menuHelper->SetSubMenuPresenter(spPresenter.Cast<Control>()));
    }

    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::UpdateParentOwner(_In_opt_ MenuFlyoutPresenter* parentMenuFlyoutPresenter)
{
    ctl::ComPtr<MenuFlyoutPresenter> parentPresenter(parentMenuFlyoutPresenter);
    if (!parentPresenter)
    {
        IFC_RETURN(GetParentMenuFlyoutPresenter(&parentPresenter));
    }

    if (parentPresenter)
    {
        ctl::ComPtr<ISubMenuOwner> parentSubMenuOwner;
        IFC_RETURN(parentPresenter.Cast<MenuFlyoutPresenter>()->get_Owner(&parentSubMenuOwner));

        if (parentSubMenuOwner)
        {
            IFC_RETURN(put_ParentOwner(parentSubMenuOwner.Get()));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::SetIsOpen(_In_ BOOLEAN isOpen)
{
    BOOLEAN isOpened = FALSE;
    BOOLEAN focusUpdated = FALSE;

    IFC_RETURN(m_tpPopup->get_IsOpen(&isOpened))

    if (isOpen != isOpened)
    {
        IFC_RETURN(m_tpPresenter.Cast<MenuFlyoutPresenter>()->put_Owner(isOpen ? this : nullptr));

        ctl::ComPtr<MenuFlyoutPresenter> parentPresenter;
        IFC_RETURN(GetParentMenuFlyoutPresenter(&parentPresenter));

        if (parentPresenter)
        {
            IFC_RETURN(parentPresenter.Cast<MenuFlyoutPresenter>()->put_SubPresenter(isOpen ? m_tpPresenter.Cast<MenuFlyoutPresenter>() : nullptr));

            ctl::ComPtr<IMenu> owningMenu;
            IFC_RETURN(parentPresenter->get_OwningMenu(&owningMenu));

            if (owningMenu)
            {
                IFC_RETURN(m_tpPresenter.Cast<MenuFlyoutPresenter>()->put_OwningMenu(isOpen ? owningMenu.Get() : nullptr));
            }

            IFC_RETURN(UpdateParentOwner(parentPresenter.Get()));
        }

        VisualTree* visualTree = VisualTree::GetForElementNoRef(GetHandle());
        if (visualTree)
        {
            // Put the popup on the same VisualTree as this flyout sub item to make sure it shows up in the right place
            static_cast<CPopup*>(m_tpPopup.Cast<Popup>()->GetHandle())->SetAssociatedVisualTree(visualTree);
        }

        // Set the popup open or close state
        IFC_RETURN(m_tpPopup->put_IsOpen(isOpen));

        // Update the secondary button automation name based on the new state
        if (m_tpSecondaryButton && !m_hasSecondaryButtonCustomAutomationName)
        {
            IFC_RETURN(SetSecondaryButtonAutomationName(!!isOpen));
        }

        // Set the focus to the displayed sub menu presenter when MenuFlyoutSubItem is opened and
        // set the focus back to the original sub item when the displayed sub menu presenter is closed.
        if (isOpen)
        {
            ctl::ComPtr<IDependencyObject> spPresenterAsDO;

            IFC_RETURN(m_tpPresenter.As(&spPresenterAsDO));

            // Set the focus to the displayed sub menu presenter to navigate the each sub items
            IFC_RETURN(DependencyObject::SetFocusedElement(
                spPresenterAsDO.Cast<DependencyObject>(),
                xaml::FocusState_Programmatic,
                FALSE /*animateIfBringIntoView*/,
                &focusUpdated));
        }
        else
        {
            // Mark that focus is coming from submenu closure
            m_focusComingFromSubmenu = true;

            ctl::ComPtr<xaml::IDependencyObject> spThisAsDO = this;

            // Set the focus to the sub menu item
            IFC_RETURN(DependencyObject::SetFocusedElement(
                spThisAsDO.Cast<DependencyObject>(),
                xaml::FocusState_Programmatic,
                FALSE /*animateIfBringIntoView*/,
                &focusUpdated));
        }

        IFC_RETURN(UpdateVisualState());
    }

    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::ClearStateFlags()
{
    m_focusComingFromSubmenu = false;
    m_focusComingFromUpwardNavigation = false;
    IFC_RETURN(m_menuHelper->ClearStateFlags());
    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::OnPresenterSizeChanged(
    _In_ IInspectable* pSender,
    _In_ xaml::ISizeChangedEventArgs* args) noexcept
{
    if (!m_tpMenuPopupThemeTransition)
    {
        ctl::ComPtr<MenuFlyoutPresenter> parentMenuFlyoutPresenter;
        IFC_RETURN(GetParentMenuFlyoutPresenter(&parentMenuFlyoutPresenter));

        // Get how many sub menus deep we are. We need this number to know what kind of Z
        // offset to use for displaying elevation. The menus aren't parented in the visual
        // hierarchy so that has to be applied with an additional transform.
        UINT depth = 1;
        if (parentMenuFlyoutPresenter)
        {
            depth = parentMenuFlyoutPresenter->GetDepth() + 1;
        }

        ctl::ComPtr<xaml_animation::ITransition> spMenuPopupChildTransition;
        IFC_RETURN(MenuFlyout::PreparePopupThemeTransitionsAndShadows(static_cast<Popup*>(m_tpPopup.Get()), 0.67 /* closedRatioConstant */, depth, &spMenuPopupChildTransition));
        IFC_RETURN(spMenuPopupChildTransition.Cast<MenuPopupThemeTransition>()->put_Direction(xaml_primitives::AnimationDirection_Top));
        SetPtrValue(m_tpMenuPopupThemeTransition, spMenuPopupChildTransition.Get());
    }

    IFC_RETURN(m_menuHelper->OnPresenterSizeChanged(pSender, args, m_tpPopup.Cast<Popup>()));

    // Update the OpenedLength property of the ThemeTransition.
    double openedLength = 0.0;
    IFC_RETURN(m_tpPresenter.Cast<Control>()->get_ActualHeight(&openedLength));
    IFC_RETURN(m_tpMenuPopupThemeTransition.Cast<MenuPopupThemeTransition>()->put_OpenedLength(openedLength));

    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::SetPresenterStyle(
    _In_ IControl* pPresenter,
    _In_opt_ IStyle* pStyle)
{
    HRESULT hr = S_OK;

    ASSERT(pPresenter);

    if (pStyle)
    {
        IFC(static_cast<Control*>(pPresenter)->put_Style(pStyle));
    }
    else
    {
        // Only using ClearValue on Style is not sufficient to reset the ItemsControl's
        // ItemPanel and other values to default. Hence, explicitly setting Template to null
        // and clearing the Template so as to trigger the application of the implicit style.
        IFC(static_cast<Control*>(pPresenter)->put_Template(nullptr));
        
        IFC(static_cast<Control*>(pPresenter)->ClearValue(
            MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::Control_Template)));

        IFC(static_cast<Control*>(pPresenter)->ClearValue(
            MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::FrameworkElement_Style)));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::ApplySubMenuItemStyleToItems()
{
    // Get the SubMenuItemStyle
    ctl::ComPtr<IStyle> spSubMenuItemStyle;
    IFC_RETURN(get_SubMenuItemStyle(&spSubMenuItemStyle));

    // Apply style to all items in the collection
    if (m_tpItems)
    {
        UINT itemCount = 0;
        IFC_RETURN(m_tpItems->get_Size(&itemCount));

        for (UINT i = 0; i < itemCount; i++)
        {
            ctl::ComPtr<xaml_controls::IMenuFlyoutItemBase> spItemBase;
            IFC_RETURN(m_tpItems->GetAt(i, &spItemBase));

            if (spItemBase)
            {
                IFC_RETURN(ApplySubMenuItemStyleToItem(spItemBase.Get(), spSubMenuItemStyle.Get()));
            }
        }
    }

    return S_OK;
}

// Apply SubMenuItemStyle to a single item, following the ItemsControl pattern
_Check_return_ HRESULT
SplitMenuFlyoutItem::ApplySubMenuItemStyleToItem(
    _In_ xaml_controls::IMenuFlyoutItemBase* pItem,
    _In_opt_ IStyle* pStyle)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IFrameworkElement> spItemAsFE;
    ctl::ComPtr<IInspectable> spLocalStyle;
    BOOLEAN isUnsetValue = FALSE;
    bool isStyleSetFromParentMenu = false;
    const CDependencyProperty* pStyleProperty = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::FrameworkElement_Style);

    IFCPTR(pItem);
    IFC(ctl::do_query_interface(spItemAsFE, pItem));

    // Check if style is already set locally (user explicitly set it)
    IFC(spItemAsFE.Cast<FrameworkElement>()->ReadLocalValue(pStyleProperty, &spLocalStyle));
    IFC(DependencyPropertyFactory::IsUnsetValue(spLocalStyle.Get(), isUnsetValue));

    isStyleSetFromParentMenu = static_cast<MenuFlyoutItemBase*>(pItem)->GetIsStyleSetFromParentMenu();

    // Only apply our style if no local style is set, or if we previously set the style
    if (isUnsetValue || isStyleSetFromParentMenu)
    {
        if (pStyle)
        {
            // Apply the SubMenuItemStyle
            IFC(spItemAsFE->put_Style(pStyle));
            static_cast<MenuFlyoutItemBase*>(pItem)->SetIsStyleSetFromParentMenu(true);
        }
        else
        {
            // Clear the style if SubMenuItemStyle is null
            IFC(spItemAsFE.Cast<FrameworkElement>()->ClearValue(pStyleProperty));
            static_cast<MenuFlyoutItemBase*>(pItem)->SetIsStyleSetFromParentMenu(false);
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::ForwardPresenterProperties(
    _In_ MenuFlyout* pOwnerMenuFlyout,
    _In_ MenuFlyoutPresenter* pParentMenuFlyoutPresenter,
    _In_ MenuFlyoutPresenter* pSubMenuFlyoutPresenter)
{
    ctl::ComPtr<IStyle> spStyle;
    xaml::ElementTheme parentPresenterTheme;
    ctl::ComPtr<IInspectable> spDataContext;
    xaml::FlowDirection flowDirection = xaml::FlowDirection_LeftToRight;
    wrl_wrappers::HString strLanguage;
    BOOLEAN isTextScaleFactorEnabled = TRUE;
    ctl::ComPtr<IFrameworkElement> spPopupAsFE;
    ctl::ComPtr<MenuFlyoutPresenter> spSubMenuFlyoutPresenter(pSubMenuFlyoutPresenter);
    ctl::ComPtr<IControl> spSubMenuFlyoutPresenterAsControl;
    ctl::ComPtr<xaml::IDependencyObject> spThisAsDO = this;

    ASSERT(pOwnerMenuFlyout && pParentMenuFlyoutPresenter && pSubMenuFlyoutPresenter);

    IFC_RETURN(spSubMenuFlyoutPresenter.As(&spSubMenuFlyoutPresenterAsControl));

    // Set the sub presenter style - prioritize SplitMenuFlyoutItem's SubMenuPresenterStyle over MenuFlyout's MenuFlyoutPresenterStyle
    IFC_RETURN(get_SubMenuPresenterStyle(&spStyle));
    
    if (!spStyle)
    {
        // Fall back to the MenuFlyout's presenter style if no SubMenuPresenterStyle is set
        IFC_RETURN(pOwnerMenuFlyout->get_MenuFlyoutPresenterStyle(&spStyle));
    }

    if (spStyle.Get())
    {
        IFC_RETURN(static_cast<Control*>(pSubMenuFlyoutPresenter)->put_Style(spStyle.Get()));
    }
    else
    {
        IFC_RETURN(static_cast<Control*>(pSubMenuFlyoutPresenter)->ClearValue(
            MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::FrameworkElement_Style)));
    }

    // Set the sub presenter's RequestTheme from the parent presenter's RequestTheme
    IFC_RETURN(pParentMenuFlyoutPresenter->get_RequestedTheme(&parentPresenterTheme));
    IFC_RETURN(pSubMenuFlyoutPresenter->put_RequestedTheme(parentPresenterTheme));

    // Set the sub presenter's DataContext from the parent presenter's DataContext
    IFC_RETURN(pParentMenuFlyoutPresenter->get_DataContext(&spDataContext));
    IFC_RETURN(pSubMenuFlyoutPresenter->put_DataContext(spDataContext.Get()));

    // Set the sub presenter's FlowDirection from the current sub menu item's FlowDirection
    IFC_RETURN(get_FlowDirection(&flowDirection));
    IFC_RETURN(pSubMenuFlyoutPresenter->put_FlowDirection(flowDirection));

    // Set the popup's FlowDirection from the current FlowDirection
    IFC_RETURN(m_tpPopup.As(&spPopupAsFE));
    IFC_RETURN(spPopupAsFE->put_FlowDirection(flowDirection));
    // Also set the popup's theme. If there is a SystemBackdrop on the menu, it'll be watching the theme on the popup
    // itself rather than the presenter set as the popup's child.
    IFC_RETURN(spPopupAsFE->put_RequestedTheme(parentPresenterTheme));

    // Set the sub presenter's Language from the parent presenter's Language
    IFC_RETURN(pParentMenuFlyoutPresenter->get_Language(strLanguage.GetAddressOf()));
    IFC_RETURN(pSubMenuFlyoutPresenter->put_Language(strLanguage.Get()));

    // Set the sub presenter's IsTextScaleFactorEnabledInternal from the parent presenter's IsTextScaleFactorEnabledInternal
    IFC_RETURN(pParentMenuFlyoutPresenter->get_IsTextScaleFactorEnabledInternal(&isTextScaleFactorEnabled));
    IFC_RETURN(pSubMenuFlyoutPresenter->put_IsTextScaleFactorEnabledInternal(isTextScaleFactorEnabled));

    xaml::ElementSoundMode soundMode = DirectUI::ElementSoundPlayerService::GetEffectiveSoundMode(spThisAsDO.Cast<DependencyObject>());

    IFC_RETURN(spSubMenuFlyoutPresenterAsControl.Cast<Control>()->put_ElementSoundMode(soundMode));

    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::ForwardSystemBackdropToPopup(_In_ MenuFlyout* pOwnerMenuFlyout)
{
    ctl::ComPtr<ISystemBackdrop> flyoutSystemBackdrop = pOwnerMenuFlyout->GetSystemBackdrop();
    ctl::ComPtr<ISystemBackdrop> popupSystemBackdrop;
    IFC_RETURN(m_tpPopup.Cast<DirectUI::PopupGenerated>()->get_SystemBackdrop(&popupSystemBackdrop));
    if (flyoutSystemBackdrop.Get() != popupSystemBackdrop.Get())
    {
        IFC_RETURN(m_tpPopup.Cast<DirectUI::PopupGenerated>()->put_SystemBackdrop(flyoutSystemBackdrop.Get()));
    }

    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::get_IsOpen(_Out_ BOOLEAN *pIsOpen)
{
    *pIsOpen = FALSE;
    if (m_tpPopup)
    {
        IFC_RETURN(m_tpPopup->get_IsOpen(pIsOpen))
    }
    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::EnsureCloseExistingSubItems()
{
    ctl::ComPtr<MenuFlyoutPresenter> spParentPresenter;

    IFC_RETURN(GetParentMenuFlyoutPresenter(&spParentPresenter));
    if (spParentPresenter)
    {
        ctl::ComPtr<IMenuPresenter> openedSubPresenter;

        IFC_RETURN(spParentPresenter->get_SubPresenter(&openedSubPresenter));
        if (openedSubPresenter)
        {
            ctl::ComPtr<ISubMenuOwner> subMenuOwner;

            IFC_RETURN(openedSubPresenter->get_Owner(&subMenuOwner));
            if (subMenuOwner && !ctl::are_equal(subMenuOwner.Get(), this))
            {
                IFC_RETURN(openedSubPresenter->CloseSubMenu());
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::OnPrimaryButtonPointerEntered(_In_ IInspectable* sender, _In_ xaml_input::IPointerRoutedEventArgs* args)
{
    // Close any open submenu when hovering over primary button
    BOOLEAN isOpen = FALSE;
    IFC_RETURN(get_IsOpen(&isOpen));
    if (isOpen)
    {
        IFC_RETURN(m_menuHelper->DelayCloseSubMenu());
    }
    
    IFC_RETURN(UpdateVisualState());
    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::OnPrimaryButtonPointerExited(_In_ IInspectable* sender, _In_ xaml_input::IPointerRoutedEventArgs* args)
{
    IFC_RETURN(UpdateVisualState());
    return S_OK;
}

// Secondary button event handlers
_Check_return_ HRESULT
SplitMenuFlyoutItem::OnSecondaryButtonPointerEntered(_In_ IInspectable* sender, _In_ xaml_input::IPointerRoutedEventArgs* args)
{
    IFC_RETURN(m_menuHelper->OnPointerEntered(args));
    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::OnSecondaryButtonPointerExited(_In_ IInspectable* sender, _In_ xaml_input::IPointerRoutedEventArgs* args)
{
    // Close submenu when exiting secondary button
    bool parentIsSubMenu = false;
    ctl::ComPtr<MenuFlyoutPresenter> parentPresenter;
    IFC_RETURN(GetParentMenuFlyoutPresenter(&parentPresenter));
    
    if (parentPresenter)
    {
        parentIsSubMenu = parentPresenter->IsSubPresenter();
    }
    
    IFC_RETURN(m_menuHelper->OnPointerExited(args, parentIsSubMenu));    
    return S_OK;
}

// Primary button click handler - delegates to the inherited Invoke method
_Check_return_ HRESULT
SplitMenuFlyoutItem::OnPrimaryButtonClick(_In_ IInspectable* sender, _In_ xaml::IRoutedEventArgs* args)
{
    // Log primary button click telemetry
    TraceLoggingWrite(
        g_hTraceProvider,
        "SplitMenuFlyoutItem-PrimaryButtonClicked",
        TraceLoggingLevel(WINEVENT_LEVEL_LOG_ALWAYS),
        TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
        TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));

    IFC_RETURN(Invoke());

    // There is an intermittent issue where the primary button remains in the
    // "PointerOver" visual state after clicking it. This probably happens because
    // on click the Flyout closes immediately, so the PointerExited event doesn't get
    // raised. To workaround this, we explicitly reset the button states here.
    IFC_RETURN(ResetButtonStates());
    return S_OK;
}

// Secondary button click handler - toggles the submenu
_Check_return_ HRESULT
SplitMenuFlyoutItem::OnSecondaryButtonClick(_In_ IInspectable* sender, _In_ xaml::IRoutedEventArgs* args)
{
    BOOLEAN isOpen = FALSE;
    IFC_RETURN(get_IsOpen(&isOpen));

    if (isOpen)
    {
        IFC_RETURN(m_menuHelper->CloseSubMenu());
    }
    else
    {
        IFC_RETURN(m_menuHelper->OpenSubMenu());
    }

    return S_OK;
}

// Sets focus to either the primary or secondary button
// Parameters:
//   focusSecondaryButton - true to focus secondary button, false to focus primary button
//   focusState - the FocusState to use when setting focus
_Check_return_ HRESULT
SplitMenuFlyoutItem::SetButtonFocus(_In_ bool focusSecondaryButton, _In_ xaml::FocusState focusState)
{
    auto& targetButton = focusSecondaryButton ? m_tpSecondaryButton : m_tpPrimaryButton;
    
    if (targetButton)
    {
        BOOLEAN focusUpdated = FALSE;
        ctl::ComPtr<xaml::IDependencyObject> spButtonAsDO;
        IFC_RETURN(targetButton.As(&spButtonAsDO));
        
        IFC_RETURN(DependencyObject::SetFocusedElement(
            spButtonAsDO.Cast<DependencyObject>(),
            focusState,
            FALSE /*animateIfBringIntoView*/,
            &focusUpdated));
            
        if (focusUpdated)
        {
            IFC_RETURN(UpdateVisualState());
        }
    }
    return S_OK;
}

// Helper method to check if a specific button has focus
_Check_return_ HRESULT
SplitMenuFlyoutItem::HasButtonFocus(_In_ const TrackerPtr<xaml_primitives::IButtonBase>& button, _Out_ bool* hasFocus)
{
    *hasFocus = false;
    
    if (button)
    {
        ctl::ComPtr<DependencyObject> spFocusedElement;
        IFC_RETURN(GetFocusedElement(&spFocusedElement));
        
        if (spFocusedElement)
        {
            ctl::ComPtr<xaml::IDependencyObject> spButtonAsDO;
            IFC_RETURN(button.As(&spButtonAsDO));
            
            *hasFocus = ctl::are_equal(spFocusedElement.Get(), spButtonAsDO.Get());
        }
    }
    
    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::ResetButtonStates()
{
    if(m_tpPrimaryButton)
    {
        ButtonBase* pPrimaryButton = static_cast<ButtonBase*>(m_tpPrimaryButton.Get());
        pPrimaryButton->put_IsPointerOver(FALSE);
    }

    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::HasPrimaryButtonFocus(_Out_ bool* hasFocus)
{
    IFC_RETURN(HasButtonFocus(m_tpPrimaryButton, hasFocus));
    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::HasSecondaryButtonFocus(_Out_ bool* hasFocus)
{
    IFC_RETURN(HasButtonFocus(m_tpSecondaryButton, hasFocus));
    return S_OK;
}

_Check_return_ HRESULT
SplitMenuFlyoutItem::SetSecondaryButtonAutomationName(bool isSubMenuOpen)
{
    wrl_wrappers::HString ownerAutomationName;
    IFC_RETURN(AutomationProperties::GetNameStatic(this, ownerAutomationName.ReleaseAndGetAddressOf()));

    if(ownerAutomationName.Get() == nullptr)
    {
        IFC_RETURN(GetPlainText(ownerAutomationName.GetAddressOf()));
    }

    int secondaryButtonAutomationNameResourceId = isSubMenuOpen ? UIA_LESS_BUTTON_FOR_OWNER : UIA_MORE_BUTTON_FOR_OWNER;

    wrl_wrappers::HString secondaryButtonAutomationName;
    IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(secondaryButtonAutomationNameResourceId, secondaryButtonAutomationName.ReleaseAndGetAddressOf()));

    // Replace %s in secondaryButtonAutomationName with ownerAutomationName
    WCHAR buffer[1024];
    const int len = swprintf_s(
        buffer, ARRAYSIZE(buffer),
        secondaryButtonAutomationName.GetRawBuffer(nullptr),
        ownerAutomationName.GetRawBuffer(nullptr));

    IFCEXPECT_RETURN(len >= 0);
    IFC_RETURN(secondaryButtonAutomationName.Set(buffer));    
    IFC_RETURN(AutomationProperties::SetNameStatic(m_tpSecondaryButton.Cast<ButtonBase>(), secondaryButtonAutomationName.Get()));

    return S_OK;
}