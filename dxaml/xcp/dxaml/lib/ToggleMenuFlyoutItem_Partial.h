// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ToggleMenuFlyoutItem.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(ToggleMenuFlyoutItem)
    {
    public:
        // Performs appropriate actions upon a mouse/keyboard invocation of a MenuFlyoutItem.
        _Check_return_ HRESULT Invoke() override;

        bool HasToggle() const override { return true; }

    protected:
        // Change to the correct visual state for the MenuFlyoutItem.
        _Check_return_ HRESULT ChangeVisualState(
            // true to use transitions when updating the visual state, false
            // to snap directly to the new visual state.
            _In_ bool bUseTransitions) override;

        // Handle the custom property changed event and call the OnPropertyChanged methods.
        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        // Override OnCreateAutomationPeer()
        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue) override;
    };
}
