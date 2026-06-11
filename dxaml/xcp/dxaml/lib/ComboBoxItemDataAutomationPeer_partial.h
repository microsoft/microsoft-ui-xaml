// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ComboBoxItemDataAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the ComboBoxItemDataAutomationPeer
    PARTIAL_CLASS(ComboBoxItemDataAutomationPeer)
    {
        public:
            // Initializes a new instance of the ComboBoxItemDataAutomationPeer class.
            ComboBoxItemDataAutomationPeer();
            ~ComboBoxItemDataAutomationPeer() override;

            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* returnValue);

            // IScrollItemProvider
            _Check_return_ HRESULT ScrollIntoViewImpl();

            // ISelectionItemProvider
            _Check_return_ HRESULT SelectImpl() override;
            _Check_return_ HRESULT AddToSelectionImpl() override;
            _Check_return_ HRESULT RemoveFromSelectionImpl() override;

            // IVirtualizedItemProvider
            IFACEMETHOD(Realize)() override;
    };
}
