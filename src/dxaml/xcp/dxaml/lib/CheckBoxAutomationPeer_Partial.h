// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CheckBoxAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the CheckBoxAutomationPeer
    PARTIAL_CLASS(CheckBoxAutomationPeer)
    {
        public:
            // Initializes a new instance of the CheckBoxAutomationPeer class.
            CheckBoxAutomationPeer();
            ~CheckBoxAutomationPeer() override;

            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue);
    };
}
