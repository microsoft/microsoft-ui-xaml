// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "BlockCollection.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(BlockCollection)
    {
    public:
        _Check_return_ HRESULT DisconnectVisualChildrenRecursive();
    };
}
