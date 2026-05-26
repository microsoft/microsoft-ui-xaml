// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "GridViewHeaderItem.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(GridViewHeaderItem)
    {
        // Override OnCreateAutomationPeer()
        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);
    };
}
