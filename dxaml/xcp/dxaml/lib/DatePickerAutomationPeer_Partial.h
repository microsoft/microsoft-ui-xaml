// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DatePickerAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the DatePickerAutomationPeer
    PARTIAL_CLASS(DatePickerAutomationPeer)
    {
        public:
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue);
            IFACEMETHOD(GetNameCore)(_Out_ HSTRING* returnValue);
    };
}
