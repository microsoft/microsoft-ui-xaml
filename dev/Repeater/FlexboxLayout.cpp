// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "FlexboxLayout.h"
#include "RuntimeProfiler.h"

FlexboxLayout::FlexboxLayout()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_FlexboxLayout);
}

void FlexboxLayout::InitializeForContextCore(winrt::LayoutContext const& context)
{
#if FALSE
    auto state = context.LayoutState();
    winrt::com_ptr<FlexboxLayoutState> flexboxState = nullptr;
    if (state)
    {
        flexboxState = GetAsFlexboxState(state);
    }

    if (!flexboxState)
    {
        if (state)
        {
            throw winrt::hresult_error(E_FAIL, L"LayoutState must derive from FlexboxLayoutState.");
        }

        // Custom deriving layouts could potentially be stateful.
        // If that is the case, we will just create the base state required by ourselves.
        flexboxState = winrt::make_self<FlexboxLayoutState>();
    }

    flexboxState->InitializeForContext(context, this);
#endif
}

void FlexboxLayout::UninitializeForContextCore(winrt::LayoutContext const& context)
{
#if FALSE
    auto flexboxState = GetAsFlexboxState(context.LayoutState());
    flexboxState->UninitializeForContext(context);
#endif
}

winrt::Size FlexboxLayout::MeasureOverride(
    winrt::LayoutContext const& context,
    winrt::Size const& availableSize)
{
    return { availableSize.Width, availableSize.Height > 9999 ? 9999 : availableSize.Height };
}

winrt::Size FlexboxLayout::ArrangeOverride(
    winrt::LayoutContext const& context,
    winrt::Size const& finalSize)
{
    return { finalSize.Width, finalSize.Height };
}

void FlexboxLayout::OnPlaceholderPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
}
