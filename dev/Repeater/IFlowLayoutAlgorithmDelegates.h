// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class IFlowLayoutAlgorithmDelegates
{
public:
    virtual ~IFlowLayoutAlgorithmDelegates() = default;

    virtual winrt::Size Algorithm_GetMeasureSize(int index, const winrt::Size& availableSize, const winrt::VirtualizingLayoutContext& context) = 0;
    virtual winrt::Size Algorithm_GetProvisionalArrangeSize(int index, const winrt::Size& measureSize, winrt::Size const& desiredSize, const winrt::VirtualizingLayoutContext& context) = 0;
    virtual bool Algorithm_ShouldBreakLine(int index, double remainingSpace) = 0;
    virtual winrt::FlowLayoutAnchorInfo Algorithm_GetAnchorForRealizationRect(const winrt::Size& availableSize, const winrt::VirtualizingLayoutContext& context) = 0;
    virtual winrt::FlowLayoutAnchorInfo Algorithm_GetAnchorForTargetElement(int targetIndex, const winrt::Size& availableSize, const winrt::VirtualizingLayoutContext& context) = 0;
    virtual winrt::Rect Algorithm_GetExtent(const winrt::Size& availableSize,
        const winrt::VirtualizingLayoutContext& context,
        const winrt::UIElement& firstRealized,
        int firstRealizedItemIndex,
        const winrt::Rect& firstRealizedLayoutBounds,
        const winrt::UIElement& lastRealized,
        int lastRealizedItemIndex,
        const winrt::Rect& lastRealizedLayoutBounds) = 0;
    virtual void Algorithm_OnElementMeasured(
        const winrt::UIElement& element,
        int index,
        const winrt::Size& availableSize,
        const winrt::Size& measureSize,
        const winrt::Size& desiredSize,
        const winrt::Size& provisionalArrangeSize,
        const winrt::VirtualizingLayoutContext& context) = 0;
    virtual void Algorithm_OnLineArranged(
        int startIndex,
        int countInLine,
        double lineSize,
        const winrt::VirtualizingLayoutContext& context) = 0;
};