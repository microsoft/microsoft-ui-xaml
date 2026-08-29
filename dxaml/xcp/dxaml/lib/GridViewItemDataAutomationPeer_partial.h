// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "GridViewItemDataAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the GridViewItemDataAutomationPeer
    PARTIAL_CLASS(GridViewItemDataAutomationPeer)
    {
        public:
            // Initializes a new instance of the GridViewItemDataAutomationPeer class.
            GridViewItemDataAutomationPeer();
            ~GridViewItemDataAutomationPeer() override;

            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            
            // IScrollItemProvider
            _Check_return_ HRESULT ScrollIntoViewImpl();
    };
}
