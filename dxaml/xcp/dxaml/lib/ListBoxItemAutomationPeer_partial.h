// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ListBoxItemAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the ListBoxItemAutomationPeer
    PARTIAL_CLASS(ListBoxItemAutomationPeer)
    {
        public:
            // Initializes a new instance of the ListBoxItemAutomationPeer class.
            ListBoxItemAutomationPeer();
            ~ListBoxItemAutomationPeer() override;

            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* returnValue);
    };
}
