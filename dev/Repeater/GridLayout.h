// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "VirtualizingLayout.h"
#include "GridLayout.g.h"

class GridLayout :
    public ReferenceTracker<GridLayout, winrt::implementation::GridLayoutT, VirtualizingLayout>
{
public:
    GridLayout();
};
