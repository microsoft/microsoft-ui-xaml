// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "InlineUIContainer.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(InlineUIContainer)
    {
    protected:
        IFACEMETHOD(OnDisconnectVisualChildren)() override;
    };
}
