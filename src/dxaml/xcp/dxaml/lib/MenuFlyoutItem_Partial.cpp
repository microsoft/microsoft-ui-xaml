// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MenuFlyoutItem.g.h"
#include "PointerRoutedEventArgs.g.h"
#include "AutomationPeer.g.h"
#include "MenuFlyoutPresenter.g.h"
#include "MenuFlyout.g.h"
#include "MenuFlyoutItemAutomationPeer.g.h"
#include "MenuFlyoutItemTemplateSettings.g.h"
#include "KeyboardAccelerator.g.h"
#include "TextBlock.g.h"
#include "CommandingHelpers.h"
#include "ElementSoundPlayerService_Partial.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

MenuFlyoutItem::MenuFlyoutItem()
    : m_bIsPointerOver(FALSE)
    , m_bIsPressed(FALSE)
    , m_bIsPointerLeftButtonDown(FALSE)
    , m_bIsSpaceOrEnterKeyDown(FALSE)
    , m_bIsNavigationAcceptOrGamepadAKeyDown(FALSE)
    , m_shouldPerformActions(FALSE)
{
}

// Prepares object's state
_Check_return_ HRESULT
MenuFlyoutItem::Initialize()
{
    HRESULT hr = S_OK;

    IFC(MenuFlyoutItemGenerated::Initialize());

    IFC(m_epLoadedHandler.AttachEventHandler(this,
        [this](IInspectable *pSender, IRoutedEventArgs *pArgs)
    {
        RRETURN(ClearStateFlags());
    }));

Cleanup:
    RRETURN(hr);
}

// Apply a template to the MenuFlyoutItem.
IFACEMETHODIMP
MenuFlyoutItem::OnApplyTemplate()
{
    IFC_RETURN(MenuFlyoutItemGenerated::OnApplyTemplate());

    ctl::ComPtr<xaml_controls::ITextBlock> keyboardAcceleratorTextBlock;
    IFC_RETURN(GetTemplatePart<xaml_controls::ITextBlock>(STR_LEN_PAIR(L"KeyboardAcceleratorTextBlock"), keyboardAcceleratorTextBlock.ReleaseAndGetAddressOf()));
    SetPtrValue(m_tpKeyboardAcceleratorTextBlock, keyboardAcceleratorTextBlock.Get());

    static_cast<CControl*>(GetHandle())->SuppressIsEnabled(FALSE);
    IFC_RETURN(UpdateCanExecute());

    IFC_RETURN(InitializeKeyboardAcceleratorText());

    // Sync the logical and visual states of the control
    IFC_RETURN(UpdateVisualState());

    return S_OK;
}

// PointerPressed event handler.
IFACEMETHODIMP
MenuFlyoutItem::OnPointerPressed(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN handled = FALSE;

    IFC(MenuFlyoutItemGenerated::OnPointerPressed(pArgs));

    IFC(pArgs->get_Handled(&handled));
    if (!handled)
    {
        ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
        ctl::ComPtr<ixp::IPointerPointProperties> spPointerProperties;
        BOOLEAN bIsLeftButtonPressed = FALSE;

        IFC(pArgs->GetCurrentPoint(this, &spPointerPoint));
        IFCPTR(spPointerPoint);
        IFC(spPointerPoint->get_Properties(&spPointerProperties));
        IFCPTR(spPointerProperties);
        IFC(spPointerProperties->get_IsLeftButtonPressed(&bIsLeftButtonPressed));

        if (bIsLeftButtonPressed)
        {
            m_bIsPointerLeftButtonDown = TRUE;
            m_bIsPressed = TRUE;

            IFC(pArgs->put_Handled(TRUE));
            IFC(UpdateVisualState());
        }
    }

Cleanup:
    RRETURN(hr);
}

// PointerReleased event handler.
IFACEMETHODIMP
MenuFlyoutItem::OnPointerReleased(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN handled = FALSE;

    IFC(MenuFlyoutItemGenerated::OnPointerReleased(pArgs));

    IFC(pArgs->get_Handled(&handled));
    if (!handled)
    {
        m_bIsPointerLeftButtonDown = FALSE;

        m_shouldPerformActions = m_bIsPressed && !m_bIsSpaceOrEnterKeyDown && !m_bIsNavigationAcceptOrGamepadAKeyDown;
        if (m_shouldPerformActions)
        {
            GestureModes gestureFollowing = GestureModes::None;

            m_bIsPressed = FALSE;
            IFC(UpdateVisualState());
            IFC(static_cast<PointerRoutedEventArgs*>(pArgs)->get_GestureFollowing(&gestureFollowing));

            // Note that we are intentionally NOT handling the args
            // if we do not fall through here because basically we are no_opting in that case.
            if (gestureFollowing != GestureModes::RightTapped)
            {
                IFC(pArgs->put_Handled(TRUE));
                IFC(PerformPointerUpAction());
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
MenuFlyoutItem::OnRightTappedUnhandled(
    _In_ xaml_input::IRightTappedRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN handled = FALSE;

    IFC(MenuFlyoutItemGenerated::OnRightTappedUnhandled(pArgs));

    IFC(pArgs->get_Handled(&handled));
    if (!handled)
    {
        IFC(PerformPointerUpAction());
        IFC(pArgs->put_Handled(TRUE));
    }

Cleanup:
    RRETURN(hr);
}

// Contains the logic to be employed if we decide to handle pointer released.
_Check_return_ HRESULT
MenuFlyoutItem::PerformPointerUpAction()
{
    HRESULT hr = S_OK;

    if (m_shouldPerformActions)
    {
        BOOLEAN wasFocused = FALSE;

        IFC(Focus(xaml::FocusState_Pointer, &wasFocused));
        IFC(Invoke());
    }

Cleanup:
    m_shouldPerformActions = FALSE;
    RRETURN(hr);
}

// Performs appropriate actions upon a mouse/keyboard invocation of a MenuFlyoutItem.
_Check_return_ HRESULT
MenuFlyoutItem::Invoke()
{
    ClickEventSourceType* pEventSource = nullptr;
    ctl::ComPtr<RoutedEventArgs> spArgs;
    ctl::ComPtr<MenuFlyoutPresenter> spParentMenuFlyoutPresenter;

    // Create the args
    IFC_RETURN(ctl::make(&spArgs));
    IFC_RETURN(spArgs->put_OriginalSource(ctl::as_iinspectable(this)));

    // Raise the event
    IFC_RETURN(GetClickEventSourceNoRef(&pEventSource));
    IFC_RETURN(pEventSource->Raise(ctl::as_iinspectable(this), spArgs.Get()));

    IFC_RETURN(AutomationPeer::RaiseEventIfListener(this, xaml_automation_peers::AutomationEvents_InvokePatternOnInvoked));

    // Execute command associated with the button
    IFC_RETURN(ExecuteCommand());

    BOOLEAN shouldPreventDismissOnPointer;
    IFC_RETURN(get_PreventDismissOnPointer(&shouldPreventDismissOnPointer));
    if (!shouldPreventDismissOnPointer)
    {
        // Close the MenuFlyout.
        IFC_RETURN(GetParentMenuFlyoutPresenter(&spParentMenuFlyoutPresenter));
        if (spParentMenuFlyoutPresenter)
        {
            ctl::ComPtr<IMenu> owningMenu;

            IFC_RETURN(spParentMenuFlyoutPresenter->get_OwningMenu(&owningMenu));
            if (owningMenu)
            {
                // We need to make close all menu flyout sub items before we hide the parent menu flyout,
                // otherwise selecting an MenuFlyoutItem in a sub menu will hide the parent menu a
                // significant amount of time before closing the sub menu.
                IFC_RETURN(spParentMenuFlyoutPresenter->CloseSubMenu());
                IFC_RETURN(owningMenu->Close());
            }
        }
    }

    IFC_RETURN(ElementSoundPlayerService::RequestInteractionSoundForElementStatic(xaml::ElementSoundKind_Invoke, this));

    return S_OK;
}

_Check_return_ HRESULT MenuFlyoutItem::AddProofingItemHandlerStatic(_In_ CDependencyObject* pMenuFlyoutItem, _In_ INTERNAL_EVENT_HANDLER eventHandler)
{
    ctl::ComPtr<DependencyObject> peer;

    IFC_RETURN(DXamlCore::GetCurrent()->TryGetPeer(pMenuFlyoutItem, &peer));

    if (!peer)
    {
        return S_OK;
    }

    ctl::ComPtr<MenuFlyoutItem> peerAsMenuFlyoutItem;
    IFC_RETURN(peer.As(&peerAsMenuFlyoutItem));
    IFCPTR_RETURN(peerAsMenuFlyoutItem);

    IFC_RETURN(peerAsMenuFlyoutItem->AddProofingItemHandler(eventHandler));

    return S_OK;
}

_Check_return_ HRESULT MenuFlyoutItem::AddProofingItemHandler(_In_ INTERNAL_EVENT_HANDLER eventHandler)
{
    IFC_RETURN(m_epMenuFlyoutItemClickEventCallback.AttachEventHandler(this, [eventHandler](IInspectable* pSender, IInspectable* pArgs)
    {
        ctl::ComPtr<DependencyObject> spSender;
        ctl::ComPtr<EventArgs> spArgs;

        IFCPTR_RETURN(pSender);
        IFCPTR_RETURN(pArgs);

        IFC_RETURN(ctl::do_query_interface(spSender, pSender));
        IFC_RETURN(ctl::do_query_interface(spArgs, pArgs));

        eventHandler(spSender->GetHandle(), spArgs->GetCorePeer());

        return S_OK;
    }));

    return S_OK;
}

// Executes MenuFlyoutItem.Command if CanExecute() returns true.
_Check_return_ HRESULT
MenuFlyoutItem::ExecuteCommand()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ICommand> spCommand;

    IFC(get_Command(&spCommand));
    if (spCommand)
    {
        ctl::ComPtr<IInspectable> spCommandParameter;
        BOOLEAN canExecute;

        IFC(get_CommandParameter(&spCommandParameter));
        IFC(spCommand->CanExecute(spCommandParameter.Get(), &canExecute));
        if (canExecute)
        {
            IFC(spCommand->Execute(spCommandParameter.Get()));
        }
    }

Cleanup:
    RRETURN(hr);
}

// PointerEnter event handler.
IFACEMETHODIMP
MenuFlyoutItem::OnPointerEntered(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    ctl::ComPtr<MenuFlyoutPresenter> spParentPresenter;

    IFC_RETURN(MenuFlyoutItemGenerated::OnPointerEntered(pArgs));

    m_bIsPointerOver = TRUE;

    IFC_RETURN(GetParentMenuFlyoutPresenter(&spParentPresenter));
    if (spParentPresenter)
    {
        ctl::ComPtr<IMenuPresenter> subPresenter;

        IFC_RETURN(spParentPresenter->get_SubPresenter(&subPresenter));
        if (subPresenter)
        {
            ctl::ComPtr<ISubMenuOwner> subPresenterOwner;

            IFC_RETURN(subPresenter->get_Owner(&subPresenterOwner));

            if (subPresenterOwner)
            {
                IFC_RETURN(subPresenterOwner->DelayCloseSubMenu());
            }
        }
    }

    IFC_RETURN(UpdateVisualState());
    return S_OK;
}

// PointerExited event handler.
IFACEMETHODIMP
MenuFlyoutItem::OnPointerExited(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(MenuFlyoutItemGenerated::OnPointerExited(pArgs));

    // MenuFlyoutItem does not capture pointer, so PointerExit means the item is no longer pressed.
    m_bIsPressed = FALSE;
    m_bIsPointerLeftButtonDown = FALSE;
    m_bIsPointerOver = FALSE;
    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

// PointerCaptureLost event handler.
IFACEMETHODIMP
MenuFlyoutItem::OnPointerCaptureLost(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(MenuFlyoutItemGenerated::OnPointerCaptureLost(pArgs));

    // MenuFlyoutItem does not capture pointer, so PointerCaptureLost means the item is no longer pressed.
    m_bIsPressed = FALSE;
    m_bIsPointerLeftButtonDown = FALSE;
    m_bIsPointerOver = FALSE;
    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

// Called when the IsEnabled property changes.
_Check_return_ HRESULT MenuFlyoutItem::OnIsEnabledChanged(_In_ IsEnabledChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsEnabled = FALSE;

    IFC(MenuFlyoutItemGenerated::OnIsEnabledChanged(pArgs));

    IFC(get_IsEnabled(&bIsEnabled));
    if (!bIsEnabled)
    {
        IFC(ClearStateFlags());
    }
    else
    {
        IFC(UpdateVisualState());
    }

Cleanup:
    RRETURN(hr);
}

// Called when the control got focus.
IFACEMETHODIMP
MenuFlyoutItem::OnGotFocus(
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(MenuFlyoutItemGenerated::OnGotFocus(pArgs));

    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

// LostFocus event handler.
IFACEMETHODIMP
MenuFlyoutItem::OnLostFocus(
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(MenuFlyoutItemGenerated::OnLostFocus(pArgs));

    if (!m_bIsPointerLeftButtonDown)
    {
        m_bIsSpaceOrEnterKeyDown = FALSE;
        m_bIsNavigationAcceptOrGamepadAKeyDown = FALSE;
        m_bIsPressed = FALSE;
    }

    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

// KeyDown event handler.
IFACEMETHODIMP
MenuFlyoutItem::OnKeyDown(
    _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN handled = FALSE;

    IFC(MenuFlyoutItemGenerated::OnKeyDown(pArgs));

    IFC(pArgs->get_Handled(&handled));
    if (!handled)
    {
        wsy::VirtualKey key = wsy::VirtualKey_None;

        IFC(pArgs->get_Key(&key));
        hr = KeyPress::MenuFlyout::KeyDown<MenuFlyoutItem, MenuFlyoutPresenter>(key, this, &handled);
        IFC(hr);
        IFC(pArgs->put_Handled(handled));
    }

Cleanup:
    RRETURN(hr);
}

// KeyUp event handler.
IFACEMETHODIMP
MenuFlyoutItem::OnKeyUp(
    _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN handled = FALSE;

    IFC(MenuFlyoutItemGenerated::OnKeyUp(pArgs));

    IFC(pArgs->get_Handled(&handled));
    if (!handled)
    {
        wsy::VirtualKey key = wsy::VirtualKey_None;

        IFC(pArgs->get_Key(&key));
        IFC(KeyPress::MenuFlyout::KeyUp<MenuFlyoutItem>(key, this, &handled));
        IFC(pArgs->put_Handled(TRUE));
    }

Cleanup:
    RRETURN(hr);
}

// Handle the custom property changed event and call the OnPropertyChanged2 methods.
_Check_return_ HRESULT
MenuFlyoutItem::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    IFC(MenuFlyoutItemGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::UIElement_Visibility:
        IFC(OnVisibilityChanged());
        break;

    case KnownPropertyIndex::MenuFlyoutItem_Command:
        {
            ctl::ComPtr<IInspectable> spOldValue, spNewValue;
            IFC(CValueBoxer::UnboxObjectValue(args.m_pOldValue, args.m_pDP->GetPropertyType(), &spOldValue));
            IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, args.m_pDP->GetPropertyType(), &spNewValue));
            IFC(OnCommandChanged(spOldValue.Get(), spNewValue.Get()));
        }
        break;

    case KnownPropertyIndex::MenuFlyoutItem_CommandParameter:
        IFC(UpdateCanExecute());
        break;

    case KnownPropertyIndex::MenuFlyoutItem_KeyboardAcceleratorTextOverride:
        // If KeyboardAcceleratorTextOverride is being set outside of us internally setting the value,
        // then we no longer own its value, and we should not override it with anything.
        if (!m_isSettingKeyboardAcceleratorTextOverride)
        {
            m_ownsKeyboardAcceleratorTextOverride = false;
        }
        break;
    }

Cleanup:
    RRETURN(hr);
}

// Update the visual states when the Visibility property is changed.
_Check_return_ HRESULT
MenuFlyoutItem::OnVisibilityChanged()
{
    HRESULT hr = S_OK;
    xaml::Visibility visibility = xaml::Visibility_Collapsed;

    IFC(get_Visibility(&visibility));
    if (xaml::Visibility_Visible != visibility)
    {
        IFC(ClearStateFlags());
    }

Cleanup:
    RRETURN(hr);
}

// Called when the element enters the tree. Attaches event handler to Command.CanExecuteChanged.
_Check_return_ HRESULT
MenuFlyoutItem::EnterImpl(
    _In_ bool bLive,
    _In_ bool bSkipNameRegistration,
    _In_ bool bCoercedIsEnabled,
    _In_ bool bUseLayoutRounding)
{
    IFC_RETURN(MenuFlyoutItemGenerated::EnterImpl(bLive, bSkipNameRegistration, bCoercedIsEnabled, bUseLayoutRounding));

    if (bLive && !m_epCanExecuteChangedHandler)
    {
        ctl::ComPtr<ICommand> spCommand;
        IFC_RETURN(get_Command(&spCommand));

        if (spCommand)
        {
            IFC_RETURN(m_epCanExecuteChangedHandler.AttachEventHandler(static_cast<IMenuFlyoutItem*>(this), spCommand.Get(),
                [this](IInspectable *pSource, IInspectable *pArgs)
            {
                return UpdateCanExecute();
            }));
        }
        // In case we missed an update to CanExecute while the CanExecuteChanged handler was unhooked,
        // we need to update our value now.
        IFC_RETURN(UpdateCanExecute());
    }

    return S_OK;
}

// Called when the element leaves the tree. Detaches event handler from Command.CanExecuteChanged.
_Check_return_ HRESULT
MenuFlyoutItem::LeaveImpl(
    _In_ bool bLive,
    _In_ bool bSkipNameRegistration,
    _In_ bool bCoercedIsEnabled,
    _In_ bool bVisualTreeBeingReset)
{
    IFC_RETURN(MenuFlyoutItemGenerated::LeaveImpl(bLive, bSkipNameRegistration, bCoercedIsEnabled, bVisualTreeBeingReset));

    if (bLive && m_epCanExecuteChangedHandler)
    {
        ctl::ComPtr<ICommand> spCommand;
        IFC_RETURN(get_Command(&spCommand));

        if (spCommand)
        {
            IFC_RETURN(m_epCanExecuteChangedHandler.DetachEventHandler(spCommand.Get()));
        }
    }

    return S_OK;
}

// Called when the MenuFlyoutItem.Command property changes.
_Check_return_ HRESULT
MenuFlyoutItem::OnCommandChanged(
    _In_ IInspectable* pOldValue,
    _In_ IInspectable* pNewValue)
{
    // Remove handler for CanExecuteChanged from the old value
    if (m_epCanExecuteChangedHandler)
    {
        IFC_RETURN(m_epCanExecuteChangedHandler.DetachEventHandler(pOldValue));
    }

    if (pOldValue)
    {
        ctl::ComPtr<ICommand> oldCommand;
        IFC_RETURN(ctl::do_query_interface(oldCommand, pOldValue));

        ctl::ComPtr<IXamlUICommand> oldCommandAsUICommand = oldCommand.AsOrNull<IXamlUICommand>();

        if (oldCommandAsUICommand)
        {
            IFC_RETURN(CommandingHelpers::ClearBindingIfSet(oldCommandAsUICommand.Get(), this, KnownPropertyIndex::MenuFlyoutItem_Text));
            IFC_RETURN(CommandingHelpers::ClearBindingIfSet(oldCommandAsUICommand.Get(), this, KnownPropertyIndex::MenuFlyoutItem_Icon));
            IFC_RETURN(CommandingHelpers::ClearBindingIfSet(oldCommandAsUICommand.Get(), this, KnownPropertyIndex::UIElement_KeyboardAccelerators));
            IFC_RETURN(CommandingHelpers::ClearBindingIfSet(oldCommandAsUICommand.Get(), this, KnownPropertyIndex::UIElement_AccessKey));
            IFC_RETURN(CommandingHelpers::ClearBindingIfSet(oldCommandAsUICommand.Get(), this, KnownPropertyIndex::AutomationProperties_HelpText));
            IFC_RETURN(CommandingHelpers::ClearBindingIfSet(oldCommandAsUICommand.Get(), this, KnownPropertyIndex::ToolTipService_ToolTip));
        }
    }

    // Subscribe to the CanExecuteChanged event on the new value
    if (pNewValue)
    {
        ctl::ComPtr<ICommand> spNewCommand;

        IFC_RETURN(ctl::do_query_interface(spNewCommand, pNewValue));
        IFC_RETURN(m_epCanExecuteChangedHandler.AttachEventHandler(static_cast<IMenuFlyoutItem*>(this), spNewCommand.Get(),
            [this](IInspectable *pSource, IInspectable *pArgs)
        {
            return UpdateCanExecute();
        }));

        ctl::ComPtr<IXamlUICommand> newCommandAsUICommand = spNewCommand.AsOrNull<IXamlUICommand>();

        if (newCommandAsUICommand)
        {
            IFC_RETURN(CommandingHelpers::BindToLabelPropertyIfUnset(newCommandAsUICommand.Get(), this, KnownPropertyIndex::MenuFlyoutItem_Text));
            IFC_RETURN(CommandingHelpers::BindToIconPropertyIfUnset(newCommandAsUICommand.Get(), this, KnownPropertyIndex::MenuFlyoutItem_Icon));
            IFC_RETURN(CommandingHelpers::BindToKeyboardAcceleratorsIfUnset(newCommandAsUICommand.Get(), this));
            IFC_RETURN(CommandingHelpers::BindToAccessKeyIfUnset(newCommandAsUICommand.Get(), this));
            IFC_RETURN(CommandingHelpers::BindToDescriptionPropertiesIfUnset(newCommandAsUICommand.Get(), this));
        }
    }

    // Coerce the button enabled state with the CanExecute state of the command.
    IFC_RETURN(UpdateCanExecute());

    return S_OK;
}

// Coerces the MenuFlyoutItem's enabled state with the CanExecute state of the Command.
_Check_return_ HRESULT
MenuFlyoutItem::UpdateCanExecute()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ICommand> spCommand;
    ctl::ComPtr<IInspectable> spCommandParameter;
    BOOLEAN canExecute = TRUE;

    IFC(get_Command(&spCommand));
    if (spCommand)
    {
        IFC(get_CommandParameter(&spCommandParameter));
        IFC(spCommand->CanExecute(spCommandParameter.Get(), &canExecute));
    }

    static_cast<CControl*>(GetHandle())->SuppressIsEnabled(!canExecute);

Cleanup:
    RRETURN(hr);
}

// Change to the correct visual state for the MenuFlyoutItem.
_Check_return_ HRESULT
MenuFlyoutItem::ChangeVisualState(
    // true to use transitions when updating the visual state, false
    // to snap directly to the new visual state.
    _In_ bool bUseTransitions)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<MenuFlyoutPresenter> spPresenter;
    bool hasToggleMenuItem = false;
    bool hasIconMenuItem = false;
    bool hasMenuItemWithKeyboardAcceleratorText = false;
    BOOLEAN bIsEnabled = FALSE;
    BOOLEAN bIgnored = FALSE;
    bool shouldBeNarrow = false;
    xaml::FocusState focusState = xaml::FocusState_Unfocused;
    bool isKeyboardPresent = false;

    IFC(GetParentMenuFlyoutPresenter(&spPresenter));
    if (spPresenter)
    {
        hasToggleMenuItem = spPresenter->GetContainsToggleItems();
        hasIconMenuItem = spPresenter->GetContainsIconItems();
        hasMenuItemWithKeyboardAcceleratorText = spPresenter->GetContainsItemsWithKeyboardAcceleratorText();
    }

    IFC(get_IsEnabled(&bIsEnabled));
    IFC(get_FocusState(&focusState));
    IFC(GetShouldBeNarrow(&shouldBeNarrow));

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
    else if (m_bIsPressed)
    {
        IFC(GoToState(bUseTransitions, L"Pressed", &bIgnored));
    }
    else if (m_bIsPointerOver)
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

// Clear flags relating to the visual state.  Called when IsEnabled is set to FALSE
// or when Visibility is set to Hidden or Collapsed.
_Check_return_ HRESULT
MenuFlyoutItem::ClearStateFlags()
{
    HRESULT hr = S_OK;

    m_bIsPressed = FALSE;
    m_bIsPointerLeftButtonDown = FALSE;
    m_bIsPointerOver = FALSE;
    m_bIsSpaceOrEnterKeyDown = FALSE;
    m_bIsNavigationAcceptOrGamepadAKeyDown = FALSE;

    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

// Create MenuFlyoutItemAutomationPeer to represent the MenuFlyoutItem.
IFACEMETHODIMP MenuFlyoutItem::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IMenuFlyoutItemAutomationPeer> spMenuFlyoutItemAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IMenuFlyoutItemAutomationPeerFactory> spMenuFlyoutItemAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;

    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::MenuFlyoutItemAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As<xaml_automation_peers::IMenuFlyoutItemAutomationPeerFactory>(&spMenuFlyoutItemAPFactory));

    IFC(spMenuFlyoutItemAPFactory.Cast<MenuFlyoutItemAutomationPeerFactory>()->CreateInstanceWithOwner(this,
        NULL,
        &spInner,
        &spMenuFlyoutItemAutomationPeer));
    IFC(spMenuFlyoutItemAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
MenuFlyoutItem::GetPlainText(_Out_ HSTRING* strPlainText)
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString strText;
    IFC(get_Text(strText.ReleaseAndGetAddressOf()));
    IFC(strText.CopyTo(strPlainText));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT MenuFlyoutItem::get_KeyboardAcceleratorTextOverrideImpl(_Out_ HSTRING* pValue)
{
    IFC_RETURN(InitializeKeyboardAcceleratorText());
    IFC_RETURN(GetValueByKnownIndex(KnownPropertyIndex::MenuFlyoutItem_KeyboardAcceleratorTextOverride, pValue));

    return S_OK;
}

_Check_return_ HRESULT MenuFlyoutItem::put_KeyboardAcceleratorTextOverrideImpl(_In_opt_ HSTRING value)
{
    IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::MenuFlyoutItem_KeyboardAcceleratorTextOverride, value));
    return S_OK;
}

_Check_return_ HRESULT MenuFlyoutItem::GetKeyboardAcceleratorTextDesiredSize(_Out_ wf::Size* desiredSize)
{
    *desiredSize = { 0, 0 };

    if (!m_isTemplateApplied)
    {
        BOOLEAN templateApplied = FALSE;
        IFC_RETURN(ApplyTemplate(&templateApplied));
        m_isTemplateApplied = !!templateApplied;
    }

    if (m_tpKeyboardAcceleratorTextBlock)
    {
        xaml::Thickness margin;

        IFC_RETURN(m_tpKeyboardAcceleratorTextBlock.Cast<TextBlock>()->Measure({ static_cast<FLOAT>(DoubleUtil::PositiveInfinity), static_cast<FLOAT>(DoubleUtil::PositiveInfinity) }));
        IFC_RETURN(m_tpKeyboardAcceleratorTextBlock.Cast<TextBlock>()->get_DesiredSize(desiredSize));
        IFC_RETURN(m_tpKeyboardAcceleratorTextBlock.Cast<TextBlock>()->get_Margin(&margin));

        desiredSize->Width -= static_cast<float>(margin.Left + margin.Right);
        desiredSize->Height -= static_cast<float>(margin.Top + margin.Bottom);
    }

    return S_OK;
}

_Check_return_ HRESULT MenuFlyoutItem::InitializeKeyboardAcceleratorText()
{
    // If we have no keyboard accelerator text already provided by the app,
    // then we'll see if we can construct it ourselves based on keyboard accelerators
    // set on this item.  For example, if a keyboard accelerator with key "S" and modifier "Control"
    // is set, then we'll convert that into the keyboard accelerator text "Ctrl+S".
    if (m_ownsKeyboardAcceleratorTextOverride)
    {
        HSTRING keyboardAcceleratorTextOverride;
        IFC_RETURN(KeyboardAccelerator::GetStringRepresentationForUIElement(this, &keyboardAcceleratorTextOverride));

        // If we were able to get a string representation from keyboard accelerators,
        // then we should now set that as the value of KeyboardAcceleratorText.
        if (keyboardAcceleratorTextOverride != nullptr)
        {
            auto guard = wil::scope_exit([&]()
            {
                m_isSettingKeyboardAcceleratorTextOverride = false;
            });

            m_isSettingKeyboardAcceleratorTextOverride = true;
            IFC_RETURN(put_KeyboardAcceleratorTextOverrideImpl(keyboardAcceleratorTextOverride));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT MenuFlyoutItem::UpdateTemplateSettings(_In_ double maxKeyboardAcceleratorTextWidth)
{
    if (m_maxKeyboardAcceleratorTextWidth != maxKeyboardAcceleratorTextWidth)
    {
        m_maxKeyboardAcceleratorTextWidth = maxKeyboardAcceleratorTextWidth;

        ctl::ComPtr<xaml_primitives::IMenuFlyoutItemTemplateSettings> templateSettings;
        IFC_RETURN(get_TemplateSettings(&templateSettings));

        if (!templateSettings)
        {
            ctl::ComPtr<MenuFlyoutItemTemplateSettings> templateSettingsImplementation;
            IFC_RETURN(ctl::make(&templateSettingsImplementation));
            IFC_RETURN(put_TemplateSettings(templateSettingsImplementation.Get()));
            templateSettings = templateSettingsImplementation;
        }

        IFC_RETURN(templateSettings.Cast<MenuFlyoutItemTemplateSettings>()->put_KeyboardAcceleratorTextMinWidth(m_maxKeyboardAcceleratorTextWidth));
    }

    return S_OK;
}

