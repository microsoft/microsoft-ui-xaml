// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MenuFlyoutItem.g.h"
#include "MenuFlyoutKeyPressProcess.h"

namespace DirectUI
{
    class IsEnabledChangedEventArgs;

    PARTIAL_CLASS(MenuFlyoutItem)
    {
        // Give MenuFlyoutPresenter friend access so it can call protected method UpdateVisualState().
        friend class MenuFlyoutPresenter;

        // Give KeyboardInputProcessor friend access so it can call protected method UpdateVisualState().
        friend class KeyPress::MenuFlyout;

    private:
        // Whether the pointer is currently over the MenuFlyoutItem.
        bool m_bIsPointerOver : 1;

        // Whether the pointer is currently pressed over the MenuFlyoutItem.
        bool m_bIsPressed : 1;

        // Whether the pointer's left button is currently down.
        bool m_bIsPointerLeftButtonDown : 1;

        // True if the SPACE or ENTER key is currently pressed, false otherwise.
        bool m_bIsSpaceOrEnterKeyDown : 1;

        // True if the NAVIGATION_ACCEPT or GAMEPAD_A vkey is currently pressed, false otherwise.
        bool m_bIsNavigationAcceptOrGamepadAKeyDown : 1;

        // On pointer released we perform some actions depending on control. We decide to whether to perform them
        // depending on some parameters including but not limited to whether released is followed by a pressed, which
        // mouse button is pressed, what type of pointer is it etc. This BOOLEAN keeps our decision.
        bool m_shouldPerformActions : 1;

        // Event pointer for the ICommand.CanExecuteChanged event.
        ctl::WeakEventPtr<CommandCanExecuteChangedCallback> m_epCanExecuteChangedHandler;

        // Event pointer for the Loaded event.
        ctl::EventPtr<FrameworkElementLoadedEventCallback> m_epLoadedHandler;

        ctl::EventPtr<MenuFlyoutItemClickEventCallback> m_epMenuFlyoutItemClickEventCallback;

        double m_maxKeyboardAcceleratorTextWidth = 0;
        TrackerPtr<xaml_controls::ITextBlock> m_tpKeyboardAcceleratorTextBlock;

        bool m_isTemplateApplied = false;

        bool m_isSettingKeyboardAcceleratorTextOverride = false;
        bool m_ownsKeyboardAcceleratorTextOverride = true;

    public:
        // Handles the case where right tapped is raised by unhandled.
        _Check_return_ HRESULT OnRightTappedUnhandled(
            _In_ xaml_input::IRightTappedRoutedEventArgs* pArgs) override;

        // Performs appropriate actions upon a mouse/keyboard invocation of a MenuFlyoutItem.
        virtual _Check_return_ HRESULT Invoke();

        _Check_return_ HRESULT GetPlainText(_Out_ HSTRING* strPlainText) override;

        _Check_return_ HRESULT get_KeyboardAcceleratorTextOverrideImpl(_Out_ HSTRING* pValue);
        _Check_return_ HRESULT put_KeyboardAcceleratorTextOverrideImpl(_In_opt_ HSTRING value);

        _Check_return_ HRESULT GetKeyboardAcceleratorTextDesiredSize(_Out_ wf::Size* desiredSize);
        _Check_return_ HRESULT UpdateTemplateSettings(_In_ double maxKeyboardAcceleratorTextWidth);

        static _Check_return_ HRESULT AddProofingItemHandlerStatic(_In_ CDependencyObject* pMenuFlyoutItem, _In_ INTERNAL_EVENT_HANDLER eventHandler);

        _Check_return_ HRESULT AddProofingItemHandler(_In_ INTERNAL_EVENT_HANDLER eventHandler);

        virtual bool HasToggle() const { return false; }

    protected:
        // Initializes a new instance.
        MenuFlyoutItem();

        // Prepares object's state
        _Check_return_ HRESULT Initialize() override;

        // Override OnCreateAutomationPeer()
        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue) override;

        // Apply a template to the MenuFlyoutItem.
        IFACEMETHOD(OnApplyTemplate)() override;

        // PointerPressed event handler.
        IFACEMETHOD(OnPointerPressed)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        // Called when the user releases the pointer  over the ListBoxItem.
        IFACEMETHOD(OnPointerReleased)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        // PointerEnter event handler.
        IFACEMETHOD(OnPointerEntered)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        // PointerExited event handler.
        IFACEMETHOD(OnPointerExited)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        // PointerCaptureLost event handler.
        IFACEMETHOD(OnPointerCaptureLost)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        // Called when the IsEnabled property changes.
        _Check_return_ HRESULT OnIsEnabledChanged(_In_ DirectUI::IsEnabledChangedEventArgs* pArgs) override;

        // Called when the control got focus.
        IFACEMETHOD(OnGotFocus)(
            _In_ xaml::IRoutedEventArgs* pArgs) override;

        // LostFocus event handler.
        IFACEMETHOD(OnLostFocus)(
            _In_ xaml::IRoutedEventArgs* pArgs) override;

        // KeyDown event handler.
        IFACEMETHOD(OnKeyDown)(
            _In_ xaml_input::IKeyRoutedEventArgs* pArgs) override;

        // KeyUp event handler.
        IFACEMETHOD(OnKeyUp)(
            _In_ xaml_input::IKeyRoutedEventArgs* pArgs) override;

        // Handle the custom property changed event and call the OnPropertyChanged methods.
        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        // Update the visual states when the Visibility property is changed.
        _Check_return_ HRESULT OnVisibilityChanged() override;

        // Called when the element enters the tree. Attaches event handler to Command.CanExecuteChanged.
        _Check_return_ HRESULT EnterImpl(
            _In_ bool bLive,
            _In_ bool bSkipNameRegistration,
            _In_ bool bCoercedIsEnabled,
            _In_ bool bUseLayoutRounding) final;

        // Called when the element leaves the tree. Detaches event handler from Command.CanExecuteChanged.
        _Check_return_ HRESULT LeaveImpl(
            _In_ bool bLive,
            _In_ bool bSkipNameRegistration,
            _In_ bool bCoercedIsEnabled,
            _In_ bool bVisualTreeBeingReset) final;

        // Change to the correct visual state for the MenuFlyoutItem.
        _Check_return_ HRESULT ChangeVisualState(
            // true to use transitions when updating the visual state, false
            // to snap directly to the new visual state.
            _In_ bool bUseTransitions) override;

        void GetIsPressed(_Out_ BOOLEAN* pIsPressed)
        {
            *pIsPressed = m_bIsPressed;
        }

        void GetIsPointerOver(_Out_ BOOLEAN* pIsPointerOver)
        {
            *pIsPointerOver = m_bIsPointerOver;
        }

    private:
        // Contains the logic to be employed if we decide to handle pointer released.
        _Check_return_ HRESULT PerformPointerUpAction();

        // Executes MenuFlyoutItem.Command if CanExecute() returns true.
        _Check_return_ HRESULT ExecuteCommand();

        // Called when the MenuFlyoutItem.Command property changes.
        _Check_return_ HRESULT OnCommandChanged(
            _In_ IInspectable* pOldValue,
            _In_ IInspectable* pNewValue);

        // Coerces the MenuFlyoutItem's enabled state with the CanExecute state of the Command.
        _Check_return_ HRESULT UpdateCanExecute();

        // Clear flags relating to the visual state.  Called when IsEnabled is set to FALSE
        // or when Visibility is set to Hidden or Collapsed.
        _Check_return_ HRESULT ClearStateFlags();

        // Sets KeyboardAcceleratorTextOverride to a default value based on the assigned keyboard accelerator
        // if no explicit value has been set.
        _Check_return_ HRESULT InitializeKeyboardAcceleratorText();
    };
}
