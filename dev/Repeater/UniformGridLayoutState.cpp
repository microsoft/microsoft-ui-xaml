// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "IFlowLayoutAlgorithmDelegates.h"
#include "FlowLayoutAlgorithm.h"
#include "UniformGridLayoutState.h"

#include "UniformGridLayoutState.properties.cpp"

void UniformGridLayoutState::InitializeForContext(
    const winrt::VirtualizingLayoutContext& context,
    IFlowLayoutAlgorithmDelegates* callbacks)
{
    m_flowAlgorithm.InitializeForContext(context, callbacks);
    context.LayoutStateCore(*this);
}

void UniformGridLayoutState::UninitializeForContext(const winrt::VirtualizingLayoutContext& context)
{
    m_flowAlgorithm.UninitializeForContext(context);
}

void UniformGridLayoutState::EnsureElementSize(
    const winrt::Size availableSize,
    const winrt::VirtualizingLayoutContext& context,
    const double layoutItemWidth,
    const double LayoutItemHeight,
    const winrt::UniformGridLayoutItemsStretch& stretch,
    const winrt::Orientation& orientation,
    double minRowSpacing,
    double minColumnSpacing,
    unsigned int maxItemsPerLine)
{
    if (maxItemsPerLine == 0)
    {
        maxItemsPerLine = 1;
    }

    if (context.ItemCount() > 0)
    {
        // If the first element is realized we don't need to get it from the context
        if (auto realizedElement = m_flowAlgorithm.GetElementIfRealized(0))
        {
            realizedElement.Measure(availableSize);
            SetSize(realizedElement.DesiredSize(), layoutItemWidth, LayoutItemHeight, availableSize, stretch, orientation, minRowSpacing, minColumnSpacing, maxItemsPerLine);
        }
        else
        {
            // Not realized by flowlayout, so do this now!
            if (const auto firstElement = context.GetOrCreateElementAt(0, winrt::ElementRealizationOptions::ForceCreate))
            {
                firstElement.Measure(availableSize);
                SetSize(firstElement.DesiredSize(), layoutItemWidth, LayoutItemHeight, availableSize, stretch, orientation, minRowSpacing, minColumnSpacing, maxItemsPerLine);
                context.RecycleElement(firstElement);
            }
        }
    }
}

void UniformGridLayoutState::SetSize(
    const winrt::Size& desiredItemSize,
    const double layoutItemWidth,
    const double LayoutItemHeight,
    const winrt::Size availableSize,
    const winrt::UniformGridLayoutItemsStretch& stretch,
    const winrt::Orientation& orientation,
    double minRowSpacing,
    double minColumnSpacing,
    unsigned int maxItemsPerLine)
{
    if (maxItemsPerLine == 0)
    {
        maxItemsPerLine = 1;
    }

    m_effectiveItemWidth = (std::isnan(layoutItemWidth) ? desiredItemSize.Width : layoutItemWidth);
    m_effectiveItemHeight = (std::isnan(LayoutItemHeight) ? desiredItemSize.Height : LayoutItemHeight);

    const auto availableSizeMinor = orientation == winrt::Orientation::Horizontal ? availableSize.Width : availableSize.Height;
    const auto minorItemSpacing = orientation == winrt::Orientation::Vertical ? minRowSpacing : minColumnSpacing;

    const auto itemSizeMinor = orientation == winrt::Orientation::Horizontal ? m_effectiveItemWidth : m_effectiveItemHeight;

    double extraMinorPixelsForEachItem = 0.0;
    if (std::isfinite(availableSizeMinor))
    {
        const auto numItemsPerColumn = std::min(
            maxItemsPerLine,
            static_cast<unsigned int>(std::max(1.0, availableSizeMinor / (itemSizeMinor + minorItemSpacing))));
        const auto usedSpace = (numItemsPerColumn * (itemSizeMinor + minorItemSpacing)) - minorItemSpacing;
        const auto remainingSpace = ((int)(availableSizeMinor - usedSpace));
        extraMinorPixelsForEachItem = remainingSpace / ((int)numItemsPerColumn);
    }

    if (stretch == winrt::UniformGridLayoutItemsStretch::Fill)
    {
        if (orientation == winrt::Orientation::Horizontal)
        {
            m_effectiveItemWidth += extraMinorPixelsForEachItem;
        }
        else
        {
            m_effectiveItemHeight += extraMinorPixelsForEachItem;
        }
    }
    else if (stretch == winrt::UniformGridLayoutItemsStretch::Uniform)
    {
        const auto itemSizeMajor = orientation == winrt::Orientation::Horizontal ? m_effectiveItemHeight : m_effectiveItemWidth;
        const auto extraMajorPixelsForEachItem = itemSizeMajor * (extraMinorPixelsForEachItem / itemSizeMinor);
        if (orientation == winrt::Orientation::Horizontal)
        {
            m_effectiveItemWidth += extraMinorPixelsForEachItem;
            m_effectiveItemHeight += extraMajorPixelsForEachItem;
        }
        else
        {
            m_effectiveItemHeight += extraMinorPixelsForEachItem;
            m_effectiveItemWidth += extraMajorPixelsForEachItem;
        }
    }
}
