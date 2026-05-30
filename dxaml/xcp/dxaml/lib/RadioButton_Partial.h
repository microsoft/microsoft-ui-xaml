// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RadioButton.g.h"

namespace DirectUI
{
    // Represents the RadioButton control
    PARTIAL_CLASS(RadioButton)
    {
    protected:
        // Initializes a new instance of the RadioButton class.
        RadioButton();
        ~RadioButton() override;

        // Prepares object's state
        _Check_return_ HRESULT Initialize() override;

        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        // Raises the OnChecked event
        _Check_return_ HRESULT OnChecked() override;
               
        // Disconnect framework peer and remove RadioButton from Group
        _Check_return_ HRESULT DisconnectFrameworkPeerCore() override;

        // Override OnCreateAutomationPeer()
        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);
        IFACEMETHOD(OnKeyDown)(_In_ xaml_input::IKeyRoutedEventArgs* pArgs) override;

        // Change to the correct visual state for the RadioButton.
        _Check_return_ HRESULT ChangeVisualState(
            // true to use transitions when updating the visual state, false
            // to snap directly to the new visual state.
            _In_ bool bUseTransitions) override;

        // Called when the element enters the tree. Ensures multiple radio buttons in a group are not checked
        _Check_return_ HRESULT EnterImpl(
            _In_ bool bLive,
            _In_ bool bSkipNameRegistration,
            _In_ bool bCoercedIsEnabled,
            _In_ bool bUseLayoutRounding) final;

    public:
        // Called by RadioButtonAutomationPeer to Toggle
        _Check_return_ HRESULT AutomationRadioButtonOnToggle();

        // Move the button to its next IsChecked value.
        IFACEMETHOD(OnToggle)() override;

    private:
        _Check_return_ HRESULT OnGroupNamePropertyChanged(
            _In_ IInspectable* pOldValue, 
            _In_ IInspectable* pNewValue);

        static _Check_return_ HRESULT Register(_In_opt_ HSTRING hGroupName, _In_ RadioButton * pRadioButton);

        // Unregister by searching for the instance in all groups. This is safer during shutdown, because 
        // get_GroupName may access an external (CLR) string which may have been GC'ed.
        static _Check_return_ HRESULT UnregisterSafe(_In_ RadioButton * pRadioButton);

        // Unregister within a specific group name. This is faster, but not safe during shutdown.
        static _Check_return_ HRESULT Unregister(_In_opt_ HSTRING hGroupName, _In_ RadioButton * pRadioButton);

        static _Check_return_ HRESULT UnregisterFromGroup(_In_ std::list<ctl::WeakRefPtr>* groupElements, _In_ RadioButton* pRadioButton, _Out_ bool* found);

        _Check_return_ HRESULT FocusNextElementInGroup(_In_ bool moveForward, _Out_ bool* wasFocused);

        _Check_return_ HRESULT GetGroupName(_Out_ bool* groupNameExists, _Out_ xstring_ptr* groupName);
        _Check_return_ HRESULT GetParentForGroup(_In_ bool groupNameExists, _In_ RadioButton* radioButton, _Outptr_ CDependencyObject** parent);

        _Check_return_ HRESULT UpdateRadioButtonGroup();
    };
}
