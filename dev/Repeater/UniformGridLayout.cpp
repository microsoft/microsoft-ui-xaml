// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "FlowLayoutAlgorithm.h"
#include "UniformGridLayoutState.h"
#include "UniformGridLayout.h"
#include "UniformGridLayoutFactory.h"

#pragma region IGridLayout

winrt::Orientation UniformGridLayout::Orientation()
{
    return auto_unbox(GetValue(s_orientationProperty));
}

void UniformGridLayout::Orientation(winrt::Orientation const& value)
{
    SetValue(s_orientationProperty, box_value(value));
}

double UniformGridLayout::MinItemWidth()
{
    return m_minItemWidth;
}

void UniformGridLayout::MinItemWidth(double value)
{
    SetValue(s_minItemWidthProperty, box_value(value));
}

double UniformGridLayout::MinItemHeight()
{
    return m_minItemHeight;
}

void UniformGridLayout::MinItemHeight(double value)
{
    SetValue(s_minItemHeightProperty, box_value(value));
}

double UniformGridLayout::MinRowSpacing()
{
    return m_minRowSpacing;
}

void UniformGridLayout::MinRowSpacing(double value)
{
    SetValue(s_minRowSpacingProperty, box_value(value));
}

double UniformGridLayout::MinColumnSpacing()
{
    return m_minColumnSpacing;
}

void UniformGridLayout::MinColumnSpacing(double value)
{
    SetValue(s_minColumnSpacingProperty, box_value(value));
}

winrt::UniformGridLayoutItemsJustification UniformGridLayout::ItemsJustification()
{
    return m_itemsJustification;
}

void UniformGridLayout::ItemsJustification(winrt::UniformGridLayoutItemsJustification const& value)
{
    SetValue(s_itemsJustificationProperty, box_value(value));
}

winrt::UniformGridLayoutItemsStretch UniformGridLayout::ItemsStretch()
{
    return m_itemsStretch;
}

void UniformGridLayout::ItemsStretch(winrt::UniformGridLayoutItemsStretch const& value)
{
    SetValue(s_itemsStretchProperty, box_value(value));
}

#pragma endregion

#pragma region IVirtualizingLayoutOverrides

void UniformGridLayout::InitializeForContextCore(winrt::VirtualizingLayoutContext const& context)
{
    auto state = context.LayoutState();
    winrt::com_ptr<UniformGridLayoutState> gridState = nullptr;
    if (state)
    {
        gridState = GetAsGridState(state);
    }

    if (!gridState)
    {
        if (state)
        {
            throw winrt::hresult_error(E_FAIL, L"LayoutState must derive from UniformGridLayoutState.");
        }

        // Custom deriving layouts could potentially be stateful.
        // If that is the case, we will just create the base state required by UniformGridLayout ourselves.
        gridState = winrt::make_self<UniformGridLayoutState>();
    }

    gridState->InitializeForContext(context, this);
}

void UniformGridLayout::UninitializeForContextCore(winrt::VirtualizingLayoutContext const& context)
{
    auto gridState = GetAsGridState(context.LayoutState());
    gridState->UninitializeForContext(context);
}

winrt::Size UniformGridLayout::MeasureOverride(
    winrt::VirtualizingLayoutContext const& context,
    winrt::Size const& availableSize)
{
    // Set the width and height on the grid state. If the user already set them then use the preset. 
    // If not, we have to measure the first element and get back a size which we're going to be using for the rest of the items.
    auto gridState = GetAsGridState(context.LayoutState());
    gridState->EnsureElementSize(availableSize, context, m_minItemWidth, m_minItemHeight);

    auto desiredSize = GetFlowAlgorithm(context).Measure(
        availableSize,
        context,
        true, /* isWrapping*/
        MinItemSpacing(),
        LineSpacing(),
        OrientationBasedMeasures::GetScrollOrientation(),
        LayoutId());

    // If after Measure the first item is in the realization rect, then we revoke grid state's ownership,
    // and only use the layout when to clear it when it's done.
    gridState->EnsureFirstElementOwnership();

    return { desiredSize.Width, desiredSize.Height };
}

winrt::Size UniformGridLayout::ArrangeOverride(
    winrt::VirtualizingLayoutContext const& context,
    winrt::Size const& finalSize)
{
    auto value = GetFlowAlgorithm(context).Arrange(
        finalSize,
        context,
        static_cast<FlowLayoutAlgorithm::LineAlignment>(m_itemsJustification),
        LayoutId());
    return { value.Width, value.Height };
}

void UniformGridLayout::OnItemsChangedCore(
    winrt::VirtualizingLayoutContext const& context,
    winrt::IInspectable const& source,
    winrt::NotifyCollectionChangedEventArgs const& args)
{
    GetFlowAlgorithm(context).OnDataSourceChanged(source, args, context);
    // Always invalidate layout to keep the view accurate.
    InvalidateLayout();

    auto gridState = GetAsGridState(context.LayoutState());
    gridState->ClearElementOnDataSourceChange(context, args);
}
#pragma endregion

#pragma region IFlowLayoutAlgorithmDelegates

winrt::Size UniformGridLayout::Algorithm_GetMeasureSize(int index, const winrt::Size & availableSize, const winrt::VirtualizingLayoutContext& context)
{
    // The first element should always take as much space it wants, all the others should be limited to the size of the first one.
    // We pass along all the available size so that it has the option to grow/shrink everytime it's resized rather than having a static size from the first time.
    if (index == 0)
    {
        return availableSize;
    }
    else
    {
        const auto gridState = GetAsGridState(context.LayoutState());
        return winrt::Size{ static_cast<float>(gridState->EffectiveItemWidth()),static_cast<float>(gridState->EffectiveItemHeight()) };
    }
}

winrt::Size UniformGridLayout::Algorithm_GetProvisionalArrangeSize(int /*index*/, const winrt::Size & /*measureSize*/, winrt::Size const& /*desiredSize*/, const winrt::VirtualizingLayoutContext& context)
{
    const auto gridState = GetAsGridState(context.LayoutState());
    return winrt::Size{ static_cast<float>(gridState->EffectiveItemWidth()),static_cast<float>(gridState->EffectiveItemHeight()) };
}

bool UniformGridLayout::Algorithm_ShouldBreakLine(int /*index*/, double remainingSpace)
{
    return remainingSpace < 0;
}

winrt::FlowLayoutAnchorInfo UniformGridLayout::Algorithm_GetAnchorForRealizationRect(
    const winrt::Size & availableSize,
    const winrt::VirtualizingLayoutContext & context)
{
    winrt::Rect bounds = winrt::Rect{ NAN, NAN, NAN, NAN };
    int anchorIndex = -1;

    int itemsCount = context.ItemCount();
    auto realizationRect = context.RealizationRect();
    if (itemsCount > 0 && realizationRect.*MajorSize() > 0)
    {
        const auto gridState = GetAsGridState(context.LayoutState());
        const auto lastExtent = gridState->FlowAlgorithm().LastExtent();
        const int itemsPerLine = std::max(1, static_cast<int>(availableSize.*Minor() / GetMinorSizeWithSpacing(context)));
        const double majorSize = (itemsCount / itemsPerLine) * GetMajorSizeWithSpacing(context);
        const double realizationWindowStartWithinExtent = realizationRect.*MajorStart() - lastExtent.*MajorStart();
        if ((realizationWindowStartWithinExtent + realizationRect.*MajorSize()) >= 0 && realizationWindowStartWithinExtent <= majorSize)
        {
            const double offset = std::max(0.0f, realizationRect.*MajorStart() - lastExtent.*MajorStart());
            const int anchorRowIndex = static_cast<int>(offset / GetMajorSizeWithSpacing(context));

            anchorIndex = std::max(0, std::min(itemsCount - 1, anchorRowIndex * itemsPerLine));
            bounds = GetLayoutRectForDataIndex(availableSize, anchorIndex, lastExtent, context);
        }
    }

    return winrt::FlowLayoutAnchorInfo
    {
        anchorIndex,
        bounds.*MajorStart()
    };
}

winrt::FlowLayoutAnchorInfo UniformGridLayout::Algorithm_GetAnchorForTargetElement(
    int targetIndex,
    const winrt::Size & availableSize,
    const winrt::VirtualizingLayoutContext & context)
{
    int index = -1;
    double offset = NAN;
    int count = context.ItemCount();
    if (targetIndex >= 0 && targetIndex < count)
    {
        int itemsPerLine = std::max(1, static_cast<int>(availableSize.*Minor() / GetMinorSizeWithSpacing(context)));
        int indexOfFirstInLine = (targetIndex / itemsPerLine) * itemsPerLine;
        index = indexOfFirstInLine;
        auto state = GetAsGridState(context.LayoutState());
        offset = GetLayoutRectForDataIndex(availableSize, indexOfFirstInLine, state->FlowAlgorithm().LastExtent(), context).*MajorStart();
    }

    return winrt::FlowLayoutAnchorInfo
    {
        index,
        offset
    };
}

winrt::Rect UniformGridLayout::Algorithm_GetExtent(
    const winrt::Size & availableSize,
    const winrt::VirtualizingLayoutContext & context,
    const winrt::UIElement & firstRealized,
    int firstRealizedItemIndex,
    const winrt::Rect & firstRealizedLayoutBounds,
    const winrt::UIElement & lastRealized,
    int lastRealizedItemIndex,
    const winrt::Rect & lastRealizedLayoutBounds)
{
    UNREFERENCED_PARAMETER(lastRealized);

    auto extent = winrt::Rect{};


    // Constants
    const int itemsCount = context.ItemCount();
    const float availableSizeMinor = availableSize.*Minor();
    const int itemsPerLine = std::max(1, std::isfinite(availableSizeMinor) ?
        static_cast<int>(availableSizeMinor / GetMinorSizeWithSpacing(context)) : itemsCount);
    const float lineSize = GetMajorSizeWithSpacing(context);

    extent.*MinorSize() =
        std::isfinite(availableSizeMinor) ?
        availableSizeMinor :
        std::max(0.0f, itemsCount * GetMinorSizeWithSpacing(context) - static_cast<float>(MinItemSpacing()));
    extent.*MajorSize() = std::max(0.0f, (itemsCount / itemsPerLine) * lineSize - static_cast<float>(LineSpacing()));

    if (itemsCount > 0)
    {
        if (firstRealized)
        {
            MUX_ASSERT(lastRealized);

            extent.*MajorStart() = firstRealizedLayoutBounds.*MajorStart() - (firstRealizedItemIndex / itemsPerLine) * lineSize;
            int remainingItems = itemsCount - lastRealizedItemIndex - 1;
            extent.*MajorSize() = MajorEnd(lastRealizedLayoutBounds) - extent.*MajorStart() + (remainingItems / itemsPerLine) * lineSize;
        }
        else
        {
            REPEATER_TRACE_INFO(L"%ls: \tEstimating extent with no realized elements. \n", LayoutId().data());
        }
    }
    else
    {
        MUX_ASSERT(firstRealizedItemIndex == -1);
        MUX_ASSERT(lastRealizedItemIndex == -1);
    }

    REPEATER_TRACE_INFO(L"%ls: \tExtent is (%.0f,%.0f). Based on lineSize %.0f and items per line %.0f. \n",
        LayoutId().data(), extent.Width, extent.Height, lineSize, itemsPerLine);
    return extent;
}

#pragma endregion

void UniformGridLayout::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    auto property = args.Property();
    if (property == s_orientationProperty)
    {
        auto orientation = unbox_value<winrt::Orientation>(args.NewValue());

        //Note: For UniformGridLayout Vertical Orientation means we have a Horizontal ScrollOrientation. Horizontal Orientation means we have a Vertical ScrollOrientation.
        //i.e. the properties are the inverse of each other.
        ScrollOrientation scrollOrientation = (orientation == winrt::Orientation::Horizontal) ? ScrollOrientation::Vertical : ScrollOrientation::Horizontal;
        OrientationBasedMeasures::SetScrollOrientation(scrollOrientation);
    }
    else if (property == s_minColumnSpacingProperty)
    {
        m_minColumnSpacing = unbox_value<double>(args.NewValue());
    }
    else if (property == s_minRowSpacingProperty)
    {
        m_minRowSpacing = unbox_value<double>(args.NewValue());
    }
    else if (property == s_itemsJustificationProperty)
    {
        m_itemsJustification = unbox_value<winrt::UniformGridLayoutItemsJustification>(args.NewValue());
    }
    else if (property == s_itemsStretchProperty)
    {
        m_itemsStretch = unbox_value<winrt::UniformGridLayoutItemsStretch>(args.NewValue());
    }
    else if (property == s_minItemWidthProperty)
    {
        m_minItemWidth = unbox_value<double>(args.NewValue());
    }
    else if (property = s_minItemHeightProperty)
    {
        m_minItemHeight = unbox_value<double>(args.NewValue());
    }

    InvalidateLayout();
}

#pragma region private helpers

float UniformGridLayout::GetMinorSizeWithSpacing(winrt::VirtualizingLayoutContext const& context)
{
    auto minItemSpacing = MinItemSpacing();
    auto gridState = GetAsGridState(context.LayoutState());
    return GetScrollOrientation() == ScrollOrientation::Vertical ?
        static_cast<float>(gridState->EffectiveItemWidth() + minItemSpacing) :
        static_cast<float>(gridState->EffectiveItemHeight() + minItemSpacing);
}

float UniformGridLayout::GetMajorSizeWithSpacing(winrt::VirtualizingLayoutContext const& context)
{
    auto lineSpacing = LineSpacing();
    auto gridState = GetAsGridState(context.LayoutState());
    return GetScrollOrientation() == ScrollOrientation::Vertical ?
        static_cast<float>(gridState->EffectiveItemHeight() + lineSpacing) :
        static_cast<float>(gridState->EffectiveItemWidth() + lineSpacing);
}

winrt::Rect UniformGridLayout::GetLayoutRectForDataIndex(
    const winrt::Size& availableSize,
    int index,
    const winrt::Rect& lastExtent, 
    const winrt::VirtualizingLayoutContext& context)
{
    int itemsPerLine = std::max(1, static_cast<int>(availableSize.*Minor() / GetMinorSizeWithSpacing(context)));
    int rowIndex = static_cast<int>(index / itemsPerLine);
    int indexInRow = index - (rowIndex * itemsPerLine);

    auto gridState = GetAsGridState(context.LayoutState());
    winrt::Rect bounds = MinorMajorRect(
        indexInRow * GetMinorSizeWithSpacing(context) + lastExtent.*MinorStart(),
        rowIndex * GetMajorSizeWithSpacing(context) + lastExtent.*MajorStart(),
        GetScrollOrientation() == ScrollOrientation::Vertical ? static_cast<float>(gridState->EffectiveItemWidth()) : static_cast<float>(gridState->EffectiveItemHeight()),
        GetScrollOrientation() == ScrollOrientation::Vertical ? static_cast<float>(gridState->EffectiveItemHeight()) : static_cast<float>(gridState->EffectiveItemWidth()));

    return bounds;
}

#pragma endregion
