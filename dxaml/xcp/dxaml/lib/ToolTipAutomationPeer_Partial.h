// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ToolTipAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the ToolTipAutomationPeer
    PARTIAL_CLASS(ToolTipAutomationPeer)
    {
        public:
            // Initializes a new instance of the ToggleButtonAutomationPeer class.
            ToolTipAutomationPeer();
            ~ToolTipAutomationPeer() override;

            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue);
    };
}
