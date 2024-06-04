// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NamedContainerAutomationPeer.g.h"

namespace DirectUI
{
    // Represents an automation peer for an element that has either the AutomationProperties.Name or
    // AutomationProperties.LabeledBy set and that wouldn't otherwise have an AutomationPeer
    PARTIAL_CLASS(NamedContainerAutomationPeer)
    {
        public:
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue) final;
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* returnValue) final;
    };
}
