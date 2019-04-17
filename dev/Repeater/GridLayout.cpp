// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "GridLayout.h"
#include "RuntimeProfiler.h"

GridLayout::GridLayout()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_GridLayout);
}

void GridLayout::InitializeForContextCore(winrt::LayoutContext const& context)
{
#if FALSE
    auto state = context.LayoutState();
    winrt::com_ptr<GridLayoutState> gridState = nullptr;
    if (state)
    {
        gridState = GetAsGridState(state);
    }

    if (!gridState)
    {
        if (state)
        {
            throw winrt::hresult_error(E_FAIL, L"LayoutState must derive from GridLayoutState.");
        }

        // Custom deriving layouts could potentially be stateful.
        // If that is the case, we will just create the base state required by ourselves.
        gridState = winrt::make_self<GridLayoutState>();
    }

    gridState->InitializeForContext(context, this);
#endif
}

void GridLayout::UninitializeForContextCore(winrt::LayoutContext const& context)
{
#if FALSE
    auto gridState = GetAsGridState(context.LayoutState());
    gridState->UninitializeForContext(context);
#endif
}

winrt::Size GridLayout::MeasureOverride(
    winrt::LayoutContext const& context,
    winrt::Size const& availableSize)
{
    return { availableSize.Width, availableSize.Height };
}

winrt::Size GridLayout::ArrangeOverride(
    winrt::LayoutContext const& context,
    winrt::Size const& finalSize)
{
    return { finalSize.Width, finalSize.Height };
}
