// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "FlipViewItemAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the FlipViewItemAutomationPeer
    PARTIAL_CLASS(FlipViewItemAutomationPeer)
    {
        public:
            // Initializes a new instance of the FlipViewItemAutomationPeer class.
            FlipViewItemAutomationPeer();
            ~FlipViewItemAutomationPeer() override;

            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* returnValue);
    };
}
