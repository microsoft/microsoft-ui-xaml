// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ToggleButtonAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the ToggleButtonAutomationPeer
    PARTIAL_CLASS(ToggleButtonAutomationPeer)
    {
        public:
            // Initializes a new instance of the ToggleButtonAutomationPeer class.
            ToggleButtonAutomationPeer();
            ~ToggleButtonAutomationPeer() override;

            IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue);
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue);

            _Check_return_ HRESULT ToggleImpl();
            _Check_return_ HRESULT get_ToggleStateImpl(_Out_ xaml_automation::ToggleState* pReturnValue);

            static _Check_return_ HRESULT ConvertToToggleState(_In_ IInspectable* value, _Out_ xaml_automation::ToggleState* pToggleState);

            virtual _Check_return_ HRESULT RaiseToggleStatePropertyChangedEvent(
                _In_ IInspectable* pOldValue, 
                _In_ IInspectable* pNewValue);
    };
}
