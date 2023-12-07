// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ButtonBase.g.h"
#include "ButtonBaseKeyProcess.h"

namespace DirectUI
{
    class IsEnabledChangedEventArgs;

    // Represents the base class for all Button controls.
    PARTIAL_CLASS(ButtonBase)
    {
        // Grant friend access to the KeyboardNavigation class so it can get and
        // set the m_bKeyboardNavigationAcceptsReturn member.
        friend class KeyboardNavigation;

        // Grant friend access to the KeyPress::Button class so it can access
        // the OnClick method.
        friend class KeyPress::ButtonBase;

        protected:
            // Last known position of the pointer with respect to this button.
            // Declared as protected so the RepeatButton can access it.
            wf::Point m_pointerPosition;

        private:
            // True if the SPACE or ENTER key is currently pressed, false otherwise.
            bool m_bIsSpaceOrEnterKeyDown : 1;

            // True if the NAVIGATION_ACCEPT or GAMEPAD_A vkey is currently pressed, false otherwise.
            bool m_bIsNavigationAcceptOrGamepadAKeyDown : 1;

            // True if the pointer's left button is currently down, false otherwise.
            bool m_bIsPointerLeftButtonDown : 1;

            // True if ENTER key is equivalent to SPACE
            bool m_bKeyboardNavigationAcceptsReturn : 1;

            // On pointer released we perform some actions depending on control. We decide to whether to perform them
            // depending on some parameters including but not limited to whether released is followed by a pressed, which
            // mouse button is pressed, what type of pointer is it etc. This boolean keeps our decision.
            bool m_shouldPerformActions : 1;

            // Whether the button should handle keyboard ENTER and SPACE key input.
            // HubSection's header button sets this to false for non-interactive headers so that they
            // are still focusable tab stobs but so that they do not invoke.
            bool m_handlesKeyboardInput : 1;

            // Event registration token corresponding to subscription for
            // ICommand.CanExecuteChanged event.
            ctl::WeakEventPtr<CommandCanExecuteChangedCallback> m_epCanExecuteChangedHandler;

            // Previously we released the capture from the pointer that we extracted from pointer released args. However, righttapped args
            // does not carry pointer information. So if we defer the action that we previously did until we get onrighttappedunhandled, we need
            // to store the pointer to release its capture.
            TrackerPtr<xaml_input::IPointer> m_tpPointerForPendingRightTapped;

            // True if the SPACE or ENTER key is currently pressed, false otherwise.
            BOOLEAN m_bIsPointerCaptured;

        public:
            // Apply a template to the ButtonBase.
            IFACEMETHOD(OnApplyTemplate)() override;

            _Check_return_ HRESULT ProgrammaticClick();

            // Handles the case where right tapped isr raised by unhandled.
            _Check_return_ HRESULT OnRightTappedUnhandled(_In_ xaml_input::IRightTappedRoutedEventArgs* pArgs) override;

            void put_HandlesKeyboardInput(_In_ bool handlesKeyboardInput)
            {
                m_handlesKeyboardInput = handlesKeyboardInput;
            }

        protected:
            // Initializes a new instance of the ButtonBase class.
            ButtonBase();

            // Destroys an instance of the ButtonBase class.
            ~ButtonBase() override;

            // Prepares object's state
            _Check_return_ HRESULT Initialize() override;

            _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

            void SetAcceptsReturn(bool value);

            // GotFocus event handler.
            IFACEMETHOD(OnGotFocus)(
                _In_ xaml::IRoutedEventArgs* pArgs) override;

            // LostFocus event handler.
            IFACEMETHOD(OnLostFocus)(
                _In_ xaml::IRoutedEventArgs* pArgs) override;

            // KeyDown event handler.
            IFACEMETHOD(OnKeyDown)(
                _In_ xaml_input::IKeyRoutedEventArgs* pArgs) override;

            // Handle key down events.
            virtual _Check_return_ HRESULT OnKeyDownInternal(
                _In_ wsy::VirtualKey nKey,
                _Out_ BOOLEAN* pbHandled);

            // KeyUp event handler.
            IFACEMETHOD(OnKeyUp)(
                _In_ xaml_input::IKeyRoutedEventArgs* pArgs) override;

            // Handle key up events.
            void OnKeyUpInternal(
                _In_ wsy::VirtualKey nKey,
                _Out_ BOOLEAN* pbHandled);

            // PointerPressed event handler.
            IFACEMETHOD(OnPointerPressed)(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

            // PointerReleased event handler.
            IFACEMETHOD(OnPointerReleased)(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

            // PointerEnter event handler.
            IFACEMETHOD(OnPointerEntered)(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

            // PointerMoved event handler.
            IFACEMETHOD(OnPointerMoved)(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

            // PointerExited event handler.
            IFACEMETHOD(OnPointerExited)(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

            // PointerCaptureLost event handler.
            IFACEMETHOD(OnPointerCaptureLost)(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

            // Raises the Click event.
            virtual _Check_return_ HRESULT OnClick();

            // IsEnabled property changed handler.
            _Check_return_ HRESULT OnIsEnabledChanged(_In_ DirectUI::IsEnabledChangedEventArgs* pArgs) override;

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

            //-----------------------------------------------------------------------------
            // Commanding Methods
            //-----------------------------------------------------------------------------
            virtual _Check_return_ HRESULT OnCommandChanged(_In_  IInspectable* pOldValue, _In_ IInspectable* pNewValue);

            _Check_return_ HRESULT UpdateCanExecute();

            _Check_return_ HRESULT ExecuteCommand();

        private:
            // Validate the ClickMode property when its value is changed.
            _Check_return_ HRESULT OnClickModeChanged(_In_ xaml_controls::ClickMode eNewClickMode);

            // Update the visual states when the IsPressed property is changed.
            _Check_return_ HRESULT OnIsPressedChanged();

            // Clear flags relating to the visual state.  Called when IsEnabled is set to FALSE
            // or when Visibility is set to Hidden or Collapsed.
            _Check_return_ HRESULT ClearStateFlags();

            // Contains the logic to be employed if we decide to handle pointer released.
            _Check_return_ HRESULT PerformPointerUpAction();

            // Capture the pointer.
            _Check_return_ HRESULT CapturePointerInternal(
                _In_ xaml_input::IPointer* pointer);

            // Release pointer capture if we already had it.
            _Check_return_ HRESULT ReleasePointerCaptureInternal(
                _In_ xaml_input::IPointer* pointer);

            // Determine if the pointer is above the button based on its last
            // known position.
            _Check_return_ HRESULT IsValidPointerPosition(
                _Out_ BOOLEAN* pbIsValid);

            // Loaded event handler.
            _Check_return_ HRESULT OnLoaded(
                _In_ IInspectable* pSender,
                _In_ xaml::IRoutedEventArgs* pArgs);
    };
}
