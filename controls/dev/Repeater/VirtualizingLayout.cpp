// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "VirtualizingLayout.h"
#include "RuntimeProfiler.h"

#include "VirtualizingLayout.properties.cpp"

#pragma region IVirtualizingLayoutOverrides

VirtualizingLayout::VirtualizingLayout()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_VirtualizingLayout);
}

void VirtualizingLayout::InitializeForContextCore(winrt::VirtualizingLayoutContext const& context)
{

}

void VirtualizingLayout::UninitializeForContextCore(winrt::VirtualizingLayoutContext const& context)
{

}

winrt::Size VirtualizingLayout::MeasureOverride(winrt::VirtualizingLayoutContext const& context, winrt::Size const& availableSize)
{
    throw winrt::hresult_not_implemented();
}

winrt::Size VirtualizingLayout::ArrangeOverride(winrt::VirtualizingLayoutContext const& context, winrt::Size const& finalSize)
{
    // Do not throw. If the layout decides to arrange its
    // children during measure, then an ArrangeOverride is not required.
    return finalSize;
}

void VirtualizingLayout::OnItemsChangedCore(winrt::VirtualizingLayoutContext const& context, winrt::IInspectable const& source, winrt::NotifyCollectionChangedEventArgs const& args)
{
    InvalidateMeasure();
}

#pragma endregion
