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
};
