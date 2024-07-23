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
    const double minRowSpacing,
    const double minColumnSpacing,
    unsigned int maxItemsPerLine)
{
    if (maxItemsPerLine == 0)
    {
        maxItemsPerLine = 1;
    }

    if (context.ItemCount() > 0)
    {
        // If the first element is realized we don't need to get it from the context.
        if (auto realizedElement = m_flowAlgorithm.GetElementIfRealized(0))
        {
            // This is relatively cheap, when item 0 is realized, always use it to find the size. 
            realizedElement.Measure(CalculateAvailableSize(availableSize, orientation, stretch, maxItemsPerLine, layoutItemWidth, layoutItemHeight, minRowSpacing, minColumnSpacing));
            SetSize(realizedElement.DesiredSize(), layoutItemWidth, layoutItemHeight, availableSize, stretch, orientation, minRowSpacing, minColumnSpacing, maxItemsPerLine);
        }
        else
        {
            // Not realized by flowlayout, so do this now but just once per layout pass since this is expensive and
            // has the potential to repeatedly invalidate layout due to recycling causing layout cycles.
            if (!m_isEffectiveSizeValid)
            {
                if (const auto firstElement = context.GetOrCreateElementAt(0, winrt::ElementRealizationOptions::ForceCreate))
                {
                    firstElement.Measure(CalculateAvailableSize(availableSize, orientation, stretch, maxItemsPerLine, layoutItemWidth, layoutItemHeight, minRowSpacing, minColumnSpacing));
                    SetSize(firstElement.DesiredSize(), layoutItemWidth, layoutItemHeight, availableSize, stretch, orientation, minRowSpacing, minColumnSpacing, maxItemsPerLine);
                    context.RecycleElement(firstElement);
                }
            }
        }

        m_isEffectiveSizeValid = true;
    }
}

void UniformGridLayoutState::InvalidateElementSize()
{
    m_isEffectiveSizeValid = false;
}

winrt::Size UniformGridLayoutState::CalculateAvailableSize(
    const winrt::Size availableSize,
    const winrt::Orientation orientation,
    const winrt::UniformGridLayoutItemsStretch& stretch,
    const unsigned int maxItemsPerLine,
    const double itemWidth,
    const double itemHeight,
    const double minRowSpacing,
    const double minColumnSpacing) const
{
    // Since some controls might have certain requirements when rendering (e.g. maintaining an aspect ratio),
    // we will let elements know the actual size they will get within our layout and let them measure based on that assumption.
    // That way we ensure that no gaps will be created within our layout because of a control deciding it doesn't need as much height (or width)
    // for the column width (or row height) being provided.
    if (orientation == winrt::Orientation::Horizontal)
    {
        if (!isnan(itemWidth) && std::isfinite(availableSize.Width))
        {
            double allowedColumnWidth = itemWidth;
            if (stretch != winrt::UniformGridLayoutItemsStretch::None)
            {
                allowedColumnWidth += CalculateExtraPixelsInLine(maxItemsPerLine, availableSize.Width, itemWidth, minColumnSpacing);
            }
            return winrt::Size{ static_cast<float>(allowedColumnWidth), availableSize.Height };
        }
    }
    else
    {
        if (!isnan(itemHeight) && std::isfinite(availableSize.Height))
        {
            double allowedRowHeight = itemHeight;
            if (stretch != winrt::UniformGridLayoutItemsStretch::None)
            {
                allowedRowHeight += CalculateExtraPixelsInLine(maxItemsPerLine, availableSize.Height, itemHeight, minRowSpacing);
            }
            return winrt::Size{ availableSize.Width, static_cast<float>(allowedRowHeight) };
        }
    }
    return availableSize;
}

double UniformGridLayoutState::CalculateExtraPixelsInLine(
    const unsigned int maxItemsPerLine,
    const float availableSizeMinor,
    const double itemSizeMinor,
    const double minorItemSpacing) const
{
    const auto numItemsPerColumn = [](const unsigned int maxItemsPerLine, const float availableSizeMinor, const double itemSizeMinor, const double minorItemSpacing)
    {
        const unsigned int numItemsBasedOnSize = static_cast<unsigned int>(std::max(1.0, availableSizeMinor / (itemSizeMinor + minorItemSpacing)));
        if (numItemsBasedOnSize == 0)
        {
            return maxItemsPerLine;
        }
        else
        {
            return std::min(maxItemsPerLine, numItemsBasedOnSize);
        }
    }(maxItemsPerLine,availableSizeMinor,itemSizeMinor,minorItemSpacing);

    const auto usedSpace = (numItemsPerColumn * (itemSizeMinor + minorItemSpacing)) - minorItemSpacing;
    const auto remainingSpace = static_cast<int>(availableSizeMinor - usedSpace);
    return remainingSpace / static_cast<int>(numItemsPerColumn);
}

void UniformGridLayoutState::SetSize(
    const winrt::Size& desiredItemSize,
    const double layoutItemWidth,
    const double layoutItemHeight,
    const winrt::Size availableSize,
    const winrt::UniformGridLayoutItemsStretch& stretch,
    const winrt::Orientation& orientation,
    const double minRowSpacing,
    const double minColumnSpacing,
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
