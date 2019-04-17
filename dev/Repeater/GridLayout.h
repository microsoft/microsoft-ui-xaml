// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NonVirtualizingLayout.h"
#include "GridLayout.g.h"

class GridLayout :
    public ReferenceTracker<GridLayout, winrt::implementation::GridLayoutT, NonVirtualizingLayout>
{
public:
    GridLayout();

#pragma region INonVirtualizingLayoutOverrides
    void InitializeForContextCore(winrt::LayoutContext const& context);
    void UninitializeForContextCore(winrt::LayoutContext const& context);

    winrt::Size MeasureOverride(winrt::LayoutContext const& context, winrt::Size const& availableSize);
    winrt::Size ArrangeOverride(winrt::LayoutContext const& context, winrt::Size const& finalSize);
#pragma endregion
};
