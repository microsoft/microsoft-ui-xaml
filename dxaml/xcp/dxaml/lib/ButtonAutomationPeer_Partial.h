// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ButtonAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the ButtonAutomationPeer
    PARTIAL_CLASS(ButtonAutomationPeer)
    {
        public:
            // Initializes a new instance of the ButtonAutomationPeer class.
            ButtonAutomationPeer();
            ~ButtonAutomationPeer() override;

            IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** returnValue);
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* returnValue);

            _Check_return_ HRESULT InvokeImpl();
    };
}
