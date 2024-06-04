// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ListViewItemDataAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the ListViewItemDataAutomationPeer
    PARTIAL_CLASS(ListViewItemDataAutomationPeer)
    {
        public:
            // Initializes a new instance of the ListViewItemDataAutomationPeer class.
            ListViewItemDataAutomationPeer();
            ~ListViewItemDataAutomationPeer() override;
            
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            
            // IScrollItemProvider
            _Check_return_ HRESULT ScrollIntoViewImpl();
    };
}
