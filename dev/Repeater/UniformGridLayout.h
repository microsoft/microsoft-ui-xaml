// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "VirtualizingLayout.h"
#include "UniformGridLayout.g.h"
#include "IFlowLayoutAlgorithmDelegates.h"
#include "OrientationBasedMeasures.h"
#include "UniformGridLayoutState.h"

class UniformGridLayout :
    public ReferenceTracker<UniformGridLayout, winrt::implementation::UniformGridLayoutT, VirtualizingLayout>,
    public IFlowLayoutAlgorithmDelegates,
    public OrientationBasedMeasures
{
public:
#pragma region IGridLayout
    winrt::Orientation Orientation();
    void Orientation(winrt::Orientation const& value);

    double MinItemWidth();
    void MinItemWidth(double value);

    double MinItemHeight();
    void MinItemHeight(double value);
    
    double MinRowSpacing();
    void MinRowSpacing(double value);

    double MinColumnSpacing();
    void MinColumnSpacing(double value);

    winrt::UniformGridLayoutItemsJustification ItemsJustification();
    void ItemsJustification(winrt::UniformGridLayoutItemsJustification const& value);

    winrt::UniformGridLayoutItemsStretch ItemsStretch();
    void ItemsStretch(winrt::UniformGridLayoutItemsStretch const& value);
#pragma endregion

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
        const winrt::VirtualizingLayoutContext& /*context*/)override {}
    void Algorithm_OnLineArranged(
        int /*startIndex*/,
        int /*countInLine*/,
        double /*lineSize*/,
        const winrt::VirtualizingLayoutContext& /*context*/)override {}
#pragma endregion

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    static winrt::DependencyProperty OrientationProperty() { return s_orientationProperty; }
    static winrt::DependencyProperty MinItemWidthProperty() { return s_minItemWidthProperty; }
    static winrt::DependencyProperty MinItemHeightProperty() { return s_minItemHeightProperty; }
    static winrt::DependencyProperty MinRowSpacingProperty() { return s_minRowSpacingProperty; }
    static winrt::DependencyProperty MinColumnSpacingProperty() { return s_minColumnSpacingProperty; }
    static winrt::DependencyProperty ItemsJustificationProperty() { return s_itemsJustificationProperty; }
    static winrt::DependencyProperty ItemsStretchProperty() { return s_itemsStretchProperty; }

    static GlobalDependencyProperty s_orientationProperty;
    static GlobalDependencyProperty s_minItemWidthProperty;
    static GlobalDependencyProperty s_minItemHeightProperty;
    static GlobalDependencyProperty s_minRowSpacingProperty;
    static GlobalDependencyProperty s_minColumnSpacingProperty;
    static GlobalDependencyProperty s_itemsJustificationProperty;
    static GlobalDependencyProperty s_itemsStretchProperty;

    static void EnsureProperties();
    static void ClearProperties();

private:
    static void UniformGridLayout::OnPropertyChanged(
        const winrt::DependencyObject& sender,
        const winrt::DependencyPropertyChangedEventArgs& args);

    // Methods
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
        return ScrollOrientation() == ScrollOrientation::Vertical ? m_minColumnSpacing : m_minRowSpacing;
    }

    double MinItemSpacing()
    {
        return ScrollOrientation() == ScrollOrientation::Vertical ? m_minRowSpacing : m_minColumnSpacing;
    }

    // Fields
    double m_minItemWidth{NAN};
    double m_minItemHeight{NAN};
    double m_minRowSpacing{};
    double m_minColumnSpacing{};
    winrt::UniformGridLayoutItemsJustification m_itemsJustification{ winrt::UniformGridLayoutItemsJustification::Start };
    winrt::UniformGridLayoutItemsStretch m_itemsStretch{ winrt::UniformGridLayoutItemsStretch::None };
    // !!! WARNING !!!
    // Any storage here needs to be related to layout configuration. 
    // layout specific state needs to be stored in UniformGridLayoutState.
 };
