// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "VirtualizingLayout.h"
#include "UniformGridLayout.g.h"
#include "UniformGridLayout.properties.h"
#include "IFlowLayoutAlgorithmDelegates.h"
#include "OrientationBasedMeasures.h"
#include "UniformGridLayoutState.h"

class UniformGridLayout :
    public ReferenceTracker<UniformGridLayout, winrt::implementation::UniformGridLayoutT, VirtualizingLayout>,
    public IFlowLayoutAlgorithmDelegates,
    public OrientationBasedMeasures,
    public UniformGridLayoutProperties
{
public:
    UniformGridLayout();

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
        const winrt::UIElement& /*element*/,
        int /*index*/,
        const winrt::Size& /*availableSize*/,
        const winrt::Size& /*measureSize*/,
        const winrt::Size& /*desiredSize*/,
        const winrt::Size& /*provisionalArrangeSize*/,
        const winrt::VirtualizingLayoutContext& /*context*/) override {}
    void Algorithm_OnLineArranged(
        int /*startIndex*/,
        int /*countInLine*/,
        double /*lineSize*/,
        const winrt::VirtualizingLayoutContext& /*context*/) override {}
#pragma endregion

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

private:
    // Methods
    int GetItemsPerLine(winrt::Size const& availableSize, winrt::VirtualizingLayoutContext const& context);
    float GetMinorSizeWithSpacing(winrt::VirtualizingLayoutContext const& context);
    float GetMajorSizeWithSpacing(winrt::VirtualizingLayoutContext const& context);

    winrt::Rect GetLayoutRectForDataIndex(const winrt::Size& availableSize, int index, const winrt::Rect& lastExtent, const winrt::VirtualizingLayoutContext& context);

    winrt::com_ptr<UniformGridLayoutState> GetAsGridState(const winrt::IInspectable& state)
    {
        return winrt::get_self<UniformGridLayoutState>(state.as<winrt::UniformGridLayoutState>())->get_strong();
    }

    ::FlowLayoutAlgorithm& GetFlowAlgorithm(const winrt::VirtualizingLayoutContext& context)
    {
        return GetAsGridState(context.LayoutState())->FlowAlgorithm();
    }

    void InvalidateLayout()
    {
        __super::InvalidateMeasure();
    }

    double LineSpacing()
    {
        return Orientation() == winrt::Orientation::Horizontal ? m_minRowSpacing: m_minColumnSpacing;
    }

    double MinItemSpacing()
    {
        return Orientation() == winrt::Orientation::Horizontal ? m_minColumnSpacing: m_minRowSpacing;
    }

    // Fields
    double m_minItemWidth{NAN};
    double m_minItemHeight{NAN};
    double m_minRowSpacing{};
    double m_minColumnSpacing{};
    winrt::UniformGridLayoutItemsJustification m_itemsJustification{ winrt::UniformGridLayoutItemsJustification::Start };
    winrt::UniformGridLayoutItemsStretch m_itemsStretch{ winrt::UniformGridLayoutItemsStretch::None };
    unsigned int m_maximumRowsOrColumns{MAXUINT};
    // !!! WARNING !!!
    // Any storage here needs to be related to layout configuration.
    // layout specific state needs to be stored in UniformGridLayoutState.
 };
