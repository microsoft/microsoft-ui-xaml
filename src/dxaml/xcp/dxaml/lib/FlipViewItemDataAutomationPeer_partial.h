// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "FlipViewItemDataAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the FlipViewItemDataAutomationPeer
    PARTIAL_CLASS(FlipViewItemDataAutomationPeer)
    {
        public:
            // Initializes a new instance of the FlipViewItemDataAutomationPeer class.
            FlipViewItemDataAutomationPeer();
            ~FlipViewItemDataAutomationPeer() override;

            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* returnValue);

            // IScrollItemProvider
            _Check_return_ HRESULT ScrollIntoViewImpl();

            // IVirtualizedItemProvider
            IFACEMETHOD(Realize)() override;
    };
}
