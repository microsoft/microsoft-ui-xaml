// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "FlipViewItem.g.h"

namespace DirectUI
{
    // Represents an item in a FlipView control.
    PARTIAL_CLASS(FlipViewItem)
    {

    public:
        // Initializes a new instance.
        FlipViewItem();

        // Destroys an instance.
        ~FlipViewItem() override;

    protected:
        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue) override;

        IFACEMETHOD(OnGotFocus)(_In_ xaml::IRoutedEventArgs* pArgs) override;

        IFACEMETHOD(OnLostFocus)(_In_ xaml::IRoutedEventArgs* pArgs) override;

        IFACEMETHOD(OnKeyDown)(_In_ xaml_input::IKeyRoutedEventArgs* pArgs) override;

        IFACEMETHOD(OnPointerPressed)(_In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        IFACEMETHOD(OnPointerReleased)(_In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        IFACEMETHOD(OnPointerCanceled)(_In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        IFACEMETHOD(OnPointerCaptureLost)(_In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        _Check_return_ HRESULT SetParentSelector(_In_opt_ Selector* pParentSelector) final;

    private:
         // Called when we got or lost focus
        _Check_return_ HRESULT FocusChanged(
            _In_ BOOLEAN hasFocus,
            _In_ BOOLEAN isOriginalSource);

         // Item pressed
         BOOLEAN m_isPressed; 

    };
}
