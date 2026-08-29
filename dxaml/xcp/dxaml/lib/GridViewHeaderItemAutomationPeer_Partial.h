// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "GridViewHeaderItemAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the GridViewHeaderItemAutomationPeer
    PARTIAL_CLASS(GridViewHeaderItemAutomationPeer)
    {
        public:
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
    };
}
