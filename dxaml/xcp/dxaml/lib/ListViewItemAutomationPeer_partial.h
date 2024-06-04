// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ListViewItemAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the ListViewItemAutomationPeer
    PARTIAL_CLASS(ListViewItemAutomationPeer)
    {
        public:
            // Initializes a new instance of the ListViewItemAutomationPeer class.
            ListViewItemAutomationPeer();
            ~ListViewItemAutomationPeer() override;

            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(SetFocusCore)();

            _Check_return_ HRESULT SetRemovableItemAutomationPeer(_In_ xaml_automation_peers::IItemAutomationPeer* itemAP);

        private:
            // The removable ItemAutomationPeer means that the native UIAWrapper object can be destroyed.
            // Remove ItemAutomationPeer from ItemsControlAutomationPeer's storage if ItemAP's UIAWrapper is unavailable.
            ctl::WeakRefPtr  m_wpRemovableItemAutomationPeer;
    };
}
