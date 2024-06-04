// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CheckBox.g.h"

namespace DirectUI
{
    // Represents the CheckBox control
    PARTIAL_CLASS(CheckBox)
    {
        public:
            // Initializes a new instance of the CheckBox class.
            CheckBox();
            ~CheckBox() override;

            IFACEMETHOD(OnApplyTemplate)() override;

        protected:
            // Prepares object's state
            _Check_return_ HRESULT Initialize() override;

            // Handle key down events.
            _Check_return_ HRESULT OnKeyDownInternal(
                _In_ wsy::VirtualKey nKey,
                _Out_ BOOLEAN* pbHandled) override;

            // Override OnCreateAutomationPeer()
            IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);

            // Change to the correct visual state for the CheckBox.
            _Check_return_ HRESULT ChangeVisualState(
                // true to use transitions when updating the visual state, false
                // to snap directly to the new visual state.
                _In_ bool bUseTransitions) override;
    };
}
