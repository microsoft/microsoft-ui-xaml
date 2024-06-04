// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "LandmarkTargetAutomationPeer.g.h"

namespace DirectUI
{
    // Represents an automation peer for an element that has either the AutomationProperties.LandmarkType or 
    // AutomationProperties.LocalizedLandmarkType set and that wouldn't otherwise have an AutomationPeer
    PARTIAL_CLASS(LandmarkTargetAutomationPeer)
    {
        public:
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* returnValue);
    };
}
