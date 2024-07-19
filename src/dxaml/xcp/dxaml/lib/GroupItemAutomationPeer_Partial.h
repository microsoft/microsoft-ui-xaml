// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "GroupItemAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the GroupItemAutomationPeer
    PARTIAL_CLASS(GroupItemAutomationPeer)
    {
        public:
            // Initializes a new instance of the GroupItemAutomationPeer class.
            GroupItemAutomationPeer();
            ~GroupItemAutomationPeer() override;

            IFACEMETHOD(GetChildrenCore)(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** returnValue);
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue);

            _Check_return_ HRESULT get_ParentItemsControlAutomationPeer(_Out_ xaml_automation_peers::IItemsControlAutomationPeer** ppParentItemsControl);
    };
}
