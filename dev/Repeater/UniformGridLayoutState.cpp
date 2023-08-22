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
    const double layoutItemHeight,
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
            realizedElement.Measure(CalculateAvailableSize(availableSize, orientation, stretch, maxItemsPerLine, layoutItemWidth, layoutItemHeight, minRowSpacing, minColumnSpacing));
            SetSize(realizedElement.DesiredSize(), layoutItemWidth, layoutItemHeight, availableSize, stretch, orientation, minRowSpacing, minColumnSpacing, maxItemsPerLine);
        }
        else
        {
            // Not realized by flowlayout, so do this now!
            if (const auto firstElement = context.GetOrCreateElementAt(0, winrt::ElementRealizationOptions::ForceCreate))
            {
                firstElement.Measure(CalculateAvailableSize(availableSize,orientation, stretch, maxItemsPerLine, layoutItemWidth, layoutItemHeight, minRowSpacing, minColumnSpacing));
                SetSize(firstElement.DesiredSize(), layoutItemWidth, layoutItemHeight, availableSize, stretch, orientation, minRowSpacing, minColumnSpacing, maxItemsPerLine);
                context.RecycleElement(firstElement);
            }
        }
    }
}

winrt::Size UniformGridLayoutState::CalculateAvailableSize(const winrt::Size availableSize,
    const winrt::Orientation orientation,
    const winrt::UniformGridLayoutItemsStretch& stretch,
    const unsigned int maxItemsPerLine,
    const double itemWidth,
    const double itemHeight,
    double minRowSpacing,
    double minColumnSpacing)
{
    // Since some controls might have certain requirements when rendering (e.g. maintaining an aspect ratio),
    // we will let elements know the actual size they will get within our layout and let them measure based on that assumption.
    // That way we ensure that no gaps will be created within our layout because of a control deciding it doesn't need as much height (or width)
    // for the column width (or row height) being provided.
    if (orientation == winrt::Orientation::Horizontal)
    {
        if (!isnan(itemWidth))
        {
            double allowedColumnWidth = itemWidth;
            if (stretch != winrt::UniformGridLayoutItemsStretch::None)
            {
                allowedColumnWidth += CalculateExtraPixelsInLine(maxItemsPerLine, availableSize.Width, itemWidth, minColumnSpacing);
            }
            return winrt::Size{ (float)allowedColumnWidth, availableSize.Height};
        }
    }
    else {
        if (!isnan(itemHeight))
        {
            double allowedRowHeight = itemHeight;
            if (stretch != winrt::UniformGridLayoutItemsStretch::None)
            {
                allowedRowHeight += CalculateExtraPixelsInLine(maxItemsPerLine, availableSize.Height, itemHeight, minRowSpacing);
            }
            return winrt::Size{availableSize.Width, (float)itemHeight};
        }
    }
    return availableSize;
}

double UniformGridLayoutState::CalculateExtraPixelsInLine(unsigned int maxItemsPerLine,
    const float availableSizeMinor,
    const double itemSizeMinor,
    const double minorItemSpacing)
{
    const auto numItemsPerColumn = [](unsigned int maxItemsPerLine, const float availableSizeMinor, const double itemSizeMinor, const double minorItemSpacing){
        const unsigned int numItemsBasedOnSize = static_cast<unsigned int>(std::max(1.0, availableSizeMinor / (itemSizeMinor + minorItemSpacing)));
        if (numItemsBasedOnSize == 0) {
            return maxItemsPerLine;
        }
        else {
            return std::min(
                maxItemsPerLine,
                numItemsBasedOnSize);
        }
    }(maxItemsPerLine,availableSizeMinor,itemSizeMinor,minorItemSpacing);
    const auto usedSpace = (numItemsPerColumn * (itemSizeMinor + minorItemSpacing)) - minorItemSpacing;
    const auto remainingSpace = ((int)(availableSizeMinor - usedSpace));
    return remainingSpace / ((int)numItemsPerColumn);
}

void UniformGridLayoutState::SetSize(
    const winrt::Size& desiredItemSize,
    const double layoutItemWidth,
    const double layoutItemHeight,
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
    m_effectiveItemHeight = (std::isnan(layoutItemHeight) ? desiredItemSize.Height : layoutItemHeight);

    const auto availableSizeMinor = orientation == winrt::Orientation::Horizontal ? availableSize.Width : availableSize.Height;
    const auto minorItemSpacing = orientation == winrt::Orientation::Vertical ? minRowSpacing : minColumnSpacing;

    const auto itemSizeMinor = orientation == winrt::Orientation::Horizontal ? m_effectiveItemWidth : m_effectiveItemHeight;

    double extraMinorPixelsForEachItem = 0.0;
    if (std::isfinite(availableSizeMinor))
    {
        extraMinorPixelsForEachItem = CalculateExtraPixelsInLine(maxItemsPerLine, availableSizeMinor, itemSizeMinor, minorItemSpacing);
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
