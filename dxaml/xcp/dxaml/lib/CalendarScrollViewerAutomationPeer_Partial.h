// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CalendarScrollViewerAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the CalendarScrollViewerAutomationPeer
    PARTIAL_CLASS(CalendarScrollViewerAutomationPeer)
    {
        public:
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetChildrenCore)(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** returnValue);


            /// Implement GetChildrenCore.
    };
}
