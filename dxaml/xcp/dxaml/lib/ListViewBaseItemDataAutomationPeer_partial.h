// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ListViewBaseItemDataAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the ListViewBaseItemDataAutomationPeer
    PARTIAL_CLASS(ListViewBaseItemDataAutomationPeer)
    {
        public:
            // Initializes a new instance of the ListViewBaseItemDataAutomationPeer class.
            ListViewBaseItemDataAutomationPeer();
            ~ListViewBaseItemDataAutomationPeer() override;

            // IVirtualizedItemProvider
            IFACEMETHOD(Realize)() override;
    };
}
