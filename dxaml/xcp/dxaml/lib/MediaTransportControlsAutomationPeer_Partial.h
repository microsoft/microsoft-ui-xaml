// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MediaTransportControlsAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the MediaTransportControlsAutomationPeer
    PARTIAL_CLASS(MediaTransportControlsAutomationPeer)
    {
    public:
        IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
        IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue);
    };
}
