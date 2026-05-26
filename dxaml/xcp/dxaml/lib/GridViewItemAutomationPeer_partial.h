// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "GridViewItemAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the GridViewItemAutomationPeer
    PARTIAL_CLASS(GridViewItemAutomationPeer)
    {
        public:
            // Initializes a new instance of the GridViewItemAutomationPeer class.
            GridViewItemAutomationPeer();
            ~GridViewItemAutomationPeer() override;

            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
    };
}
