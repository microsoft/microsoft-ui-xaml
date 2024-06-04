// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ContextMenuEventArgs.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(ContextMenuEventArgs)
    {
    protected:
        CEventArgs * CreateCorePeer() override;
    };
}