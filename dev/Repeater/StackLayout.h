// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "VirtualizingLayout.h"
#include "StackLayout.g.h"
#include "StackLayout.properties.h"
#include "IFlowLayoutAlgorithmDelegates.h"
#include "OrientationBasedMeasures.h"
#include "StackLayoutState.h"

class StackLayout :
    public ReferenceTracker<StackLayout, winrt::implementation::StackLayoutT, VirtualizingLayout>,
    public IFlowLayoutAlgorithmDelegates,
    public OrientationBasedMeasures,
    public StackLayoutProperties
{
public:
    StackLayout();

#pragma region IVirtualizingLayoutOverrides
    void InitializeForContextCore(winrt::VirtualizingLayoutContext const& context);
    void UninitializeForContextCore(winrt::VirtualizingLayoutContext const& context);
    winrt::Size MeasureOverride(
        winrt::VirtualizingLayoutContext const& context,
        winrt::Size const& availableSize);
    winrt::Size ArrangeOverride(
        winrt::VirtualizingLayoutContext const& context,
        winrt::Size const& finalSize);
    void OnItemsChangedCore(
        winrt::VirtualizingLayoutContext const& context,
        winrt::IInspectable const& source,
        winrt::NotifyCollectionChangedEventArgs const& args);
#pragma endregion

#pragma region IStackLayoutOverrides
    winrt::FlowLayoutAnchorInfo GetAnchorForRealizationRect(
        winrt::Size const& availableSize,
        winrt::VirtualizingLayoutContext const& context);

    winrt::Rect GetExtent(
        winrt::Size const& availableSize,
        winrt::VirtualizingLayoutContext const& context,
        winrt::UIElement const& firstRealized,
         int firstRealizedItemIndex,
        winrt::Rect const& firstRealizedLayoutBounds,
        winrt::UIElement const& lastRealized,
         int lastRealizedItemIndex,
        winrt::Rect const& lastRealizedLayoutBounds);

    void OnElementMeasured(
        winrt::UIElement const& element,
        int index,
        winrt::Size const& availableSize,
        winrt::Size const& measureSize,
        winrt::Size const& desiredSize,
        winrt::Size const& provisionalArrangeSize,
        winrt::VirtualizingLayoutContext const& context);
#pragma endregion

#pragma region IFlowLayoutAlgorithmDelegates
    winrt::Size Algorithm_GetMeasureSize(int index, const winrt::Size& availableSize, const winrt::VirtualizingLayoutContext& context) override;
    winrt::Size Algorithm_GetProvisionalArrangeSize(int index, const winrt::Size& measureSize, winrt::Size const& desiredSize, const winrt::VirtualizingLayoutContext& context) override;
    bool Algorithm_ShouldBreakLine(int index, double remainingSpace) override;
    winrt::FlowLayoutAnchorInfo Algorithm_GetAnchorForRealizationRect(
        const winrt::Size& availableSize,
        const winrt::VirtualizingLayoutContext& context) override;
    winrt::FlowLayoutAnchorInfo Algorithm_GetAnchorForTargetElement(
        int targetIndex,
        const winrt::Size& availableSize,
        const winrt::VirtualizingLayoutContext& context) override;
    winrt::Rect Algorithm_GetExtent(const winrt::Size& availableSize,
        const winrt::VirtualizingLayoutContext& context,
        const winrt::UIElement& firstRealized,
        int firstRealizedItemIndex,
        const winrt::Rect& firstRealizedLayoutBounds,
        const winrt::UIElement& lastRealized,
        int lastRealizedItemIndex,
        const winrt::Rect& lastRealizedLayoutBounds) override;
    void Algorithm_OnElementMeasured(
        const winrt::UIElement& element,
        int index,
        const winrt::Size& availableSize,
        const winrt::Size& measureSize,
        const winrt::Size& desiredSize,
        const winrt::Size& provisionalArrangeSize,
        const winrt::VirtualizingLayoutContext& context) override;
    void Algorithm_OnLineArranged(
        int /*startIndex*/,
        int /*countInLine*/,
        double /*lineSize*/,
        const winrt::VirtualizingLayoutContext& /*context*/) override {}
#pragma endregion

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

private:
    double GetAverageElementSize(
        winrt::Size availableSize,
        winrt::VirtualizingLayoutContext context,
        const winrt::com_ptr<StackLayoutState>& layoutState);

    winrt::com_ptr<StackLayoutState> GetAsStackState(const winrt::IInspectable& state)
    {
        return winrt::get_self<StackLayoutState>(state.as<winrt::StackLayoutState>())->get_strong();
    }

    void InvalidateLayout()
    {
        __super::InvalidateMeasure();
    }

    ::FlowLayoutAlgorithm& GetFlowAlgorithm(const winrt::VirtualizingLayoutContext& context)
    {
        return GetAsStackState(context.LayoutState())->FlowAlgorithm();
    }

    // Fields
    double m_itemSpacing{};

    // !!! WARNING !!!
    // Any storage here needs to be related to layout configuration. 
    // layout specific state needs to be stored in StackLayoutState.
};
