// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ListBoxItem.g.h"

namespace DirectUI
{
    // forward declarations
    class IsEnabledChangedEventArgs;
    class Selector;

    /// <summary>
    /// Represents a selectable item in a ListBox.
    /// </summary>
    PARTIAL_CLASS(ListBoxItem)
    {

    public:
        // Initializes a new instance.
        ListBoxItem();

        // Destroys an instance.
        ~ListBoxItem() override;

        // Apply a template to the ListBoxItem.
        IFACEMETHOD(OnApplyTemplate)() override;

        // Change to the correct visual state for the ListBoxItem.
        _Check_return_ HRESULT ChangeVisualState(
            // true to use transitions when updating the visual state, false
            // to snap directly to the new visual state.
            _In_ bool bUseTransitions) override;

        void SetExpectingGotFocusEventFromParentInteraction(BOOLEAN expectingGotFocus)
        {
            m_expectingGotFocusEventFromParentInteraction = expectingGotFocus;
        }

        // Handles the case where right tapped is raised by unhandled.
        _Check_return_ HRESULT OnRightTappedUnhandled(_In_ xaml_input::IRightTappedRoutedEventArgs* pArgs) override;

    protected:
        // Prepares object's state
        _Check_return_ HRESULT Initialize() override;

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

        // Override OnCreateAutomationPeer()
        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue) override;

        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        // Update the visual states when the Visibility property is changed.
        _Check_return_ HRESULT OnVisibilityChanged() override;

    private:
        // Loaded event handler.
        _Check_return_ HRESULT OnLoaded( 
            _In_ IInspectable* pSender,
            _In_ xaml::IRoutedEventArgs* pArgs);

         // Called when we got or lost focus
        _Check_return_ HRESULT FocusChanged(
            _In_ BOOLEAN hasFocus,
            _In_ BOOLEAN self);

        // Contains the logic to be employed if we decide to handle pointer released.
        _Check_return_ HRESULT PerformPointerUpAction();

        // Item pressed
        BOOLEAN m_bIsPressed;

        // Flag indicating that we expect to receive and handle a GotFocus event coming from the
        // parent ListBox calling Focus() on this ListBoxItem.
        BOOLEAN m_expectingGotFocusEventFromParentInteraction;

        // On pointer released we perform some actions depending on control. We decide to whether to perform them
        // depending on some parameters including but not limited to whether released is followed by a pressed, which
        // mouse button is pressed, what type of pointer is it etc. This BOOLEAN keeps our decision.
        BOOLEAN m_shouldPerformActions;
    };
}
