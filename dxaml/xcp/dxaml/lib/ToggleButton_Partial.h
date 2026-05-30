// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ToggleButton.g.h"

namespace DirectUI
{
    // Represents the ToggleButton control
    PARTIAL_CLASS(ToggleButton)
    {
        protected:
            // Initializes a new instance of the ToggleButton class.
            ToggleButton();
            
            // Destroys an instance of the ToggleButton class.
            ~ToggleButton() override;
            
            // Prepares object's state
            _Check_return_ HRESULT Initialize() override;

            _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;
                
            // Change to the correct visual state for the ToggleButton.
            _Check_return_ HRESULT ChangeVisualState(
                // true to use transitions when updating the visual state, false
                // to snap directly to the new visual state.
                _In_ bool bUseTransitions) override;

            // Raises the Click event.
            _Check_return_ HRESULT OnClick() override;
            // Raises the OnChecked event
            virtual _Check_return_ HRESULT OnChecked();
            // Raises the OnUnchecked event
            virtual _Check_return_ HRESULT OnUnchecked();
            // Raises the OnIndeterminate event
            virtual _Check_return_ HRESULT OnIndeterminate();

            // Override OnCreateAutomationPeer()
            IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);
            
        public:
            // Called by ToggleButtonAutomationPeer to Toggle
            _Check_return_ HRESULT AutomationToggleButtonOnToggle();

            // Called by ComboBox to make sure it's templated toggle button don't have an AutomationPeer.
            _Check_return_ HRESULT SetSkipAutomationPeerCreation();

            // Move the button to its next IsChecked value.
            _Check_return_ HRESULT OnToggleImpl();

        private:
            // Handle the IsChecked status change, resulting in updated VSM states and raising events
            virtual _Check_return_ HRESULT OnIsCheckedChanged();

            BOOLEAN _skipCreateAutomationPeer;
    };
}
