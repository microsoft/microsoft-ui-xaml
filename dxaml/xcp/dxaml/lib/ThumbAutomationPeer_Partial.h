// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ThumbAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the ThumbAutomationPeer
    PARTIAL_CLASS(ThumbAutomationPeer)
    {
        public:
            // Initializes a new instance of the ThumbAutomationPeer class.
            ThumbAutomationPeer();
            ~ThumbAutomationPeer() override;

            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue);
    };
}
