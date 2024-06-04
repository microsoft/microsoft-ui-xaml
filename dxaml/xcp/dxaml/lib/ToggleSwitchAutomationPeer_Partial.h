// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ToggleSwitchAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the ToggleSwitchAutomationPeer
    PARTIAL_CLASS(ToggleSwitchAutomationPeer)
    {
        public:
            // Initializes a new instance of the ToggleSwitchAutomationPeer class.
            ToggleSwitchAutomationPeer();
            ~ToggleSwitchAutomationPeer() override;

            IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue);
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue);
            IFACEMETHOD(GetLocalizedControlTypeCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetClickablePointCore)(_Out_ wf::Point* returnValue);

            _Check_return_ HRESULT ToggleImpl();
            _Check_return_ HRESULT get_ToggleStateImpl(_Out_ xaml_automation::ToggleState* pReturnValue);

            virtual _Check_return_ HRESULT RaiseToggleStatePropertyChangedEvent(
                _In_ IInspectable* pOldValue, 
                _In_ IInspectable* pNewValue);
    };
}
