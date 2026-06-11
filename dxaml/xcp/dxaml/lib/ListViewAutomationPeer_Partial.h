// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ListViewAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the ListViewAutomationPeer
    PARTIAL_CLASS(ListViewAutomationPeer)
    {
        public:
            // Initializes a new instance of the ListViewAutomationPeer class.
            ListViewAutomationPeer();
            ~ListViewAutomationPeer() override;

            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* returnValue);
            
            _Check_return_ HRESULT OnCreateItemAutomationPeerImpl(_In_ IInspectable* item, _Outptr_ xaml_automation_peers::IItemAutomationPeer** returnValue) override;
    };
}
