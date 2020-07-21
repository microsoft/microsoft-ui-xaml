// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ElementManager.h"

#include "UniformGridLayoutState.g.h"
#include "FlowLayoutAlgorithm.h"

class UniformGridLayoutState :
    public ReferenceTracker<UniformGridLayoutState, winrt::implementation::UniformGridLayoutStateT, winrt::composing>
{
public:
    void InitializeForContext(
        const winrt::VirtualizingLayoutContext& context,
        IFlowLayoutAlgorithmDelegates* callbacks);
    void UninitializeForContext(const winrt::VirtualizingLayoutContext& context);

    ::FlowLayoutAlgorithm& FlowAlgorithm() { return m_flowAlgorithm; }
    double EffectiveItemWidth() { return m_effectiveItemWidth; }
    double EffectiveItemHeight() { return m_effectiveItemHeight; }

    void EnsureElementSize(
        const winrt::Size availableSize,
        const winrt::VirtualizingLayoutContext& context,
        const double itemWidth,
        const double itemHeight,
        const winrt::UniformGridLayoutItemsStretch& stretch,
        const winrt::Orientation& orientation,
        double minRowSpacing,
        double minColumnSpacing,
        unsigned int maxItemsPerLine);

private:
    ::FlowLayoutAlgorithm m_flowAlgorithm{ this };
    double m_effectiveItemWidth{ 0.0 };
    double m_effectiveItemHeight{ 0.0 };

    void SetSize(const winrt::Size& desiredItemSize,
        const double itemWidth,
        const double itemHeight,
        const winrt::Size availableSize,
        const winrt::UniformGridLayoutItemsStretch& stretch,
        const winrt::Orientation& orientation,
        double minRowSpacing,
        double minColumnSpacing,
        unsigned int maxItemsPerLine);

    // We need to measure the element at index 0 to know what size to measure all other items.
    // If FlowlayoutAlgorithm has already realized element 0 then we can use that.
    // If it does not, then we need to do context.GetElement(0) at which point we have requested an element and are on point to clear it.
    // If we are responsible for clearing element 0 we keep m_cachedFirstElement valid.
    // If we are not (because FlowLayoutAlgorithm is holding it for us) then we just null out this field and use the one from FlowLayoutAlgorithm.
    bool hasCachedSize = false;
    winrt::Size m_cachedSize;
};
