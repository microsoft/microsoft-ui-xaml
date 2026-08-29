// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RepeatButton.g.h"

namespace DirectUI
{
    class IsEnabledChangedEventArgs;
    class TimelineTimer;

    // Represents a repeat button control.
    PARTIAL_CLASS(RepeatButton)
    {
        // Allow TimelineTimer to access the TickCallback method.
        friend class TimelineTimer;

        public:
            // Sets a value indicating whether the RepeatButton reacts to touch input or not.
           _Check_return_ HRESULT put_IgnoreTouchInput(_In_ BOOLEAN value);

        protected:
            RepeatButton();
            ~RepeatButton() override;

            // Prepares object's state
            _Check_return_ HRESULT Initialize() override;

            // Change to the correct visual state for the repeat button.
            _Check_return_ HRESULT ChangeVisualState(
                // true to use transitions when updating the visual state, false
                // to snap directly to the new visual state.
                _In_ bool bUseTransitions) override;

            // Raises the Click event.
            _Check_return_ HRESULT OnClick() override;

            // Handle the custom property changed event and call the
            // OnPropertyChanged methods. 
            _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

            // IsEnabled property changed handler.
            _Check_return_ HRESULT OnIsEnabledChanged(
                _In_ DirectUI::IsEnabledChangedEventArgs* pArgs) override;

            // KeyDown event handler.
            IFACEMETHOD(OnKeyDown)(
                _In_ xaml_input::IKeyRoutedEventArgs* pArgs) override;

            // KeyUp event handler.
            IFACEMETHOD(OnKeyUp)(
                _In_ xaml_input::IKeyRoutedEventArgs* pArgs) override;

            // LostFocus event handler.
            IFACEMETHOD(OnLostFocus)(
                _In_ xaml::IRoutedEventArgs* pArgs) override;

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

            // Override OnCreateAutomationPeer()
            IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);

        private:
            _Check_return_ HRESULT TickCallback();

            _Check_return_ HRESULT StartTimer();

            _Check_return_ HRESULT StopTimer();

            _Check_return_ HRESULT UpdateRepeatState();

            // Called when the Delay value changed.
            _Check_return_ HRESULT OnDelayPropertyChanged(_In_ IInspectable *pNewDelay);
            
            // Called when the Interval value changed.
            _Check_return_ HRESULT OnIntervalPropertyChanged(_In_ IInspectable *pNewInterval);

            // Called to evaluate the current input should be ignored
            _Check_return_ HRESULT ShouldIgnoreInput(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs, 
                _Out_ BOOLEAN* pIgnoreInput);

        private:
            // True if keyboard was used to initiate the repeat button, false otherwise.
            BOOLEAN m_keyboardCausingRepeat;

            // True if pointer was used to initiate the repeat button, false otherwise.
            BOOLEAN m_pointerCausingRepeat;

            // True if we want to ignore all touch pointer inputs (set by mouse scrollbar)
            BOOLEAN m_ignoreTouchInput;

            TrackerPtr<TimelineTimer> m_tpTimer;
    };
}
