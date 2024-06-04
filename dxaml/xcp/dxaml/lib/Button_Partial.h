// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Button.g.h"

namespace DirectUI
{
    // Represents a button control.
    PARTIAL_CLASS(Button)
    {
        public:
            // This is a non virtual function which work exactly same as virtual OnProcessKeyboardAcceleratorsImpl would have worked.
            // It's intentioanlly declared non virtual to avoid cost of vtable slot on UIElement and renamed to avoid confusion with above said function.
            _Check_return_ HRESULT OnProcessKeyboardAcceleratorsImplLocal(_In_ xaml_input::IProcessKeyboardAcceleratorEventArgs* pArgs) ;
            static _Check_return_ HRESULT SuppressFlyoutOpening(_In_ CButton* button);

        protected:
            // Change to the correct visual state for the button.
            _Check_return_ HRESULT ChangeVisualState(
                // true to use transitions when updating the visual state, false
                // to snap directly to the new visual state.
                _In_ bool bUseTransitions) override;

            // Raises the Click event.
            _Check_return_ HRESULT OnClick() override;

            // Override OnCreateAutomationPeer()
            IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);
            IFACEMETHOD(OnPointerCanceled)(_In_ xaml_input::IPointerRoutedEventArgs* args) override;
            IFACEMETHOD(OnPointerCaptureLost)(_In_ xaml_input::IPointerRoutedEventArgs* args) override;
            IFACEMETHOD(OnPointerExited)(_In_ xaml_input::IPointerRoutedEventArgs* args) override;

            // In case if Button has set Flyout property, get associated Flyout and open it next to this Button.
            virtual _Check_return_ HRESULT OpenAssociatedFlyout();

        private:
            // True if we should skip the next attempt to open the flyout.
            bool m_suppressFlyoutOpening{ false };
    };
}
