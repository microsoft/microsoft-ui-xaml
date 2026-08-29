// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ComboBoxItemAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the ComboBoxItemAutomationPeer
    PARTIAL_CLASS(ComboBoxItemAutomationPeer)
    {
        public:
            // Initializes a new instance of the ComboBoxItemAutomationPeer class.
            ComboBoxItemAutomationPeer();
            ~ComboBoxItemAutomationPeer() override;

            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* returnValue);
            IFACEMETHOD(GetChildrenCore)(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** ppReturnValue);
            IFACEMETHOD(IsOffscreenCore)(_Out_ BOOLEAN* returnValue);
            IFACEMETHOD(GetBoundingRectangleCore)(_Out_ wf::Rect* returnValue);
    };
}
