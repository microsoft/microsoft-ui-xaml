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

    if (m_cachedFirstElement)
    {
        context.RecycleElement(m_cachedFirstElement);
    }
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
        // If the first element is realized we don't need to cache it or to get it from the context
        if (auto realizedElement = m_flowAlgorithm.GetElementIfRealized(0))
        {
            realizedElement.Measure(availableSize);
            SetSize(realizedElement, layoutItemWidth, LayoutItemHeight, availableSize, stretch, orientation, minRowSpacing, minColumnSpacing, maxItemsPerLine);
            m_cachedFirstElement = nullptr;
        }
        else
        {
            if (!m_cachedFirstElement)
            {
                // we only cache if we aren't realizing it
                m_cachedFirstElement = context.GetOrCreateElementAt(0, winrt::ElementRealizationOptions::ForceCreate | winrt::ElementRealizationOptions::SuppressAutoRecycle); // expensive
            }

            m_cachedFirstElement.Measure(availableSize);
            SetSize(m_cachedFirstElement, layoutItemWidth, LayoutItemHeight, availableSize, stretch, orientation, minRowSpacing, minColumnSpacing, maxItemsPerLine);

            // See if we can move ownership to the flow algorithm. If we can, we do not need a local cache.
            bool added = m_flowAlgorithm.TryAddElement0(m_cachedFirstElement);
            if (added)
            {
                m_cachedFirstElement = nullptr;
            }
        }
    }
}

void UniformGridLayoutState::SetSize(
    const winrt::UIElement& UIElement,
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

    m_effectiveItemWidth = (std::isnan(layoutItemWidth) ? UIElement.DesiredSize().Width : layoutItemWidth);
    m_effectiveItemHeight = (std::isnan(LayoutItemHeight) ? UIElement.DesiredSize().Height : LayoutItemHeight);

    auto availableSizeMinor = orientation == winrt::Orientation::Horizontal ? availableSize.Width : availableSize.Height;
    auto minorItemSpacing = orientation == winrt::Orientation::Vertical ? minRowSpacing : minColumnSpacing;

    auto itemSizeMinor = orientation == winrt::Orientation::Horizontal ? m_effectiveItemWidth : m_effectiveItemHeight;

    double extraMinorPixelsForEachItem = 0.0;
    if (std::isfinite(availableSizeMinor))
    {
        auto numItemsPerColumn = std::min(
            maxItemsPerLine,
            static_cast<unsigned int>(std::max(1.0, availableSizeMinor / (itemSizeMinor + minorItemSpacing))));
        auto usedSpace = (numItemsPerColumn * (itemSizeMinor + minorItemSpacing)) - minorItemSpacing;
        auto remainingSpace = ((int)(availableSizeMinor - usedSpace));
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
        auto itemSizeMajor = orientation == winrt::Orientation::Horizontal ? m_effectiveItemHeight : m_effectiveItemWidth;
        auto extraMajorPixelsForEachItem = itemSizeMajor * (extraMinorPixelsForEachItem / itemSizeMinor);
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

void UniformGridLayoutState::EnsureFirstElementOwnership(winrt::VirtualizingLayoutContext const& context)
{
    if (m_cachedFirstElement != nullptr && m_flowAlgorithm.GetElementIfRealized(0))
    {
        // We created the element, but then flowlayout algorithm took ownership, so we can clear it and
        // let flowlayout algorithm do its thing.
        context.RecycleElement(m_cachedFirstElement);
        m_cachedFirstElement = nullptr;
    }
}

void UniformGridLayoutState::ClearElementOnDataSourceChange(winrt::VirtualizingLayoutContext const& context, winrt::NotifyCollectionChangedEventArgs const& args)
{
    if (m_cachedFirstElement)
    {
        bool shouldClear = false;
        // We should only clear the first element, used by determine the size of all elements,
        // if it was modified by the action or the whole collection was reset
        switch (args.Action())
        {
        case winrt::NotifyCollectionChangedAction::Add:
            shouldClear = args.NewStartingIndex() == 0;
            break;

        case winrt::NotifyCollectionChangedAction::Replace:
            shouldClear = args.NewStartingIndex() == 0 || args.OldStartingIndex() == 0;
            break;

        case winrt::NotifyCollectionChangedAction::Remove:
            shouldClear = args.OldStartingIndex() == 0;
            break;

        case winrt::NotifyCollectionChangedAction::Reset:
            shouldClear = true;
            break;

        case winrt::NotifyCollectionChangedAction::Move:
            shouldClear = args.NewStartingIndex() == 0 || args.OldStartingIndex() == 0;
            break;
        }

        if (shouldClear)
        {
            context.RecycleElement(m_cachedFirstElement);
            m_cachedFirstElement = nullptr;
        }
    }
}
