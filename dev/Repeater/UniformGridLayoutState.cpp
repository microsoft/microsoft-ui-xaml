// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "FlowLayoutAlgorithm.h"
#include "IFlowLayoutAlgorithmDelegates.h"
#include "ItemsRepeater.common.h"
#include "UniformGridLayoutState.h"
#include <common.h>
#include <pch.h>

CppWinRTActivatableClassWithBasicFactory(UniformGridLayoutState);

void UniformGridLayoutState::InitializeForContext(
    const winrt::VirtualizingLayoutContext& context,
    IFlowLayoutAlgorithmDelegates* callbacks)
{
    m_flowAlgorithm.InitializeForContext(context, callbacks);
    context.LayoutStateCore(*this);
}

void UniformGridLayoutState::UninitializeForContext(const winrt::VirtualizingLayoutContext&  /*context*/)
{
    m_flowAlgorithm.UninitializeForContext(context);

    if (m_cachedFirstElement)
    {
        context.RecycleElement(m_cachedFirstElement);
    }
}

void UniformGridLayoutState::EnsureElementSize(
    const winrt::Size availableSize,
    const winrt::VirtualizingLayoutContext&  /*context*/,
    const double layoutItemWidth,
    const double LayoutItemHeight,
    const winrt::UniformGridLayoutItemsStretch&  /*stretch*/,
    const winrt::Orientation& orientation,
    double minRowSpacing,
    double minColumnSpacing)
{
    if (context.ItemCount() > 0)
    {
        // If the first element is realized we don't need to cache it or to get it from the context
        if (auto realizedElement = m_flowAlgorithm.GetElementIfRealized(0))
        {
            realizedElement.Measure(availableSize);
            SetSize(realizedElement, layoutItemWidth, LayoutItemHeight, availableSize, stretch, orientation, minRowSpacing, minColumnSpacing);
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
            SetSize(m_cachedFirstElement, layoutItemWidth, LayoutItemHeight, availableSize, stretch, orientation, minRowSpacing, minColumnSpacing);

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
    const winrt::UniformGridLayoutItemsStretch&  /*stretch*/,
    const winrt::Orientation& orientation,
    double minRowSpacing,
    double minColumnSpacing)
{
    m_effectiveItemWidth = (std::isnan(layoutItemWidth) ? UIElement.DesiredSize().Width : layoutItemWidth);
    m_effectiveItemHeight = (std::isnan(LayoutItemHeight) ? UIElement.DesiredSize().Height : LayoutItemHeight);

    auto availableSizeMinor = orientation == winrt::Orientation::Horizontal ? availableSize.Width : availableSize.Height;
    auto minorItemSpacing = orientation == winrt::Orientation::Vertical ? minRowSpacing : minColumnSpacing;

    auto itemSizeMinor = orientation == winrt::Orientation::Horizontal ? m_effectiveItemWidth : m_effectiveItemHeight;
    itemSizeMinor += minorItemSpacing;

    auto numItemsPerColumn = static_cast<int>(std::max(1.0, availableSizeMinor / itemSizeMinor));
    auto remainingSpace = (static_cast<int>(availableSizeMinor)) % (static_cast<int>(itemSizeMinor));
    auto extraMinorPixelsForEachItem = remainingSpace / numItemsPerColumn;

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
            throw winrt::hresult_not_implemented();
            break;
        }

        if (shouldClear)
        {
            context.RecycleElement(m_cachedFirstElement);
            m_cachedFirstElement = nullptr;
        }
    }
}
