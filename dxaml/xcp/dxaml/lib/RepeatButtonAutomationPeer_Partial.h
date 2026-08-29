// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RepeatButtonAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the RepeatButtonAutomationPeer
    PARTIAL_CLASS(RepeatButtonAutomationPeer)
    {
        public:
            // Initializes a new instance of the RepeatButtonAutomationPeer class.
            RepeatButtonAutomationPeer();
            ~RepeatButtonAutomationPeer() override;

            IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue);
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue);

            _Check_return_ HRESULT InvokeImpl();
    };
}
