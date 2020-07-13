// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "FlowLayoutAlgorithm.h"
#include "UniformGridLayoutState.h"
#include "UniformGridLayout.h"
#include "RuntimeProfiler.h"
#include "VirtualizingLayoutContext.h"

#pragma region IGridLayout

UniformGridLayout::UniformGridLayout()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_UniformGridLayout);
    LayoutId(L"UniformGridLayout");
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
    gridState->EnsureElementSize(availableSize, context, m_minItemWidth, m_minItemHeight, m_itemsStretch, Orientation(), MinRowSpacing(), MinColumnSpacing(), m_maximumRowsOrColumns);

    const auto desiredSize = GetFlowAlgorithm(context).Measure(
        availableSize,
        context,
        true, /* isWrapping*/
        MinItemSpacing(),
        LineSpacing(),
        m_maximumRowsOrColumns /* maxItemsPerLine */,
        OrientationBasedMeasures::GetScrollOrientation(),
        false /* disableVirtualization */,
        LayoutId());

    // If after Measure the first item is in the realization rect, then we revoke grid state's ownership,
    // and only use the layout when to clear it when it's done.
    gridState->EnsureFirstElementOwnership(context);

    return { desiredSize.Width, desiredSize.Height };
}

winrt::Size UniformGridLayout::ArrangeOverride(
    winrt::VirtualizingLayoutContext const& context,
    winrt::Size const& finalSize)
{
    const auto value = GetFlowAlgorithm(context).Arrange(
        finalSize,
        context,
        true /* isWrapping */,
        static_cast<FlowLayoutAlgorithm::LineAlignment>(m_itemsJustification),        
        LayoutId());
    return { value.Width, value.Height };
}

void UniformGridLayout::OnItemsChangedCore(
    winrt::VirtualizingLayoutContext const& context,
    winrt::IInspectable const& source,
    winrt::NotifyCollectionChangedEventArgs const& args)
{
    GetFlowAlgorithm(context).OnItemsSourceChanged(source, args, context);
    // Always invalidate layout to keep the view accurate.
    InvalidateLayout();

    auto gridState = GetAsGridState(context.LayoutState());
    gridState->ClearElementOnDataSourceChange(context, args);
}
#pragma endregion

#pragma region IFlowLayoutAlgorithmDelegates

winrt::Size UniformGridLayout::Algorithm_GetMeasureSize(int index, const winrt::Size & availableSize, const winrt::VirtualizingLayoutContext& context)
{
    const auto gridState = GetAsGridState(context.LayoutState());
    return winrt::Size{ static_cast<float>(gridState->EffectiveItemWidth()),static_cast<float>(gridState->EffectiveItemHeight()) };
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

    const int itemsCount = context.ItemCount();
    const auto realizationRect = context.RealizationRect();
    if (itemsCount > 0 && MajorSize(realizationRect) > 0)
    {
        const auto gridState = GetAsGridState(context.LayoutState());
        const auto lastExtent = gridState->FlowAlgorithm().LastExtent();
        const int itemsPerLine = std::min( // note use of unsigned ints
            std::max(1u, static_cast<unsigned int>(Minor(availableSize) / GetMinorSizeWithSpacing(context))),
            std::max(1u, m_maximumRowsOrColumns));
        const double majorSize = (itemsCount / itemsPerLine) * (double)(GetMajorSizeWithSpacing(context));
        const double realizationWindowStartWithinExtent = (double)(MajorStart(realizationRect) - MajorStart(lastExtent));
        if ((realizationWindowStartWithinExtent + MajorSize(realizationRect)) >= 0 && realizationWindowStartWithinExtent <= majorSize)
        {
            const double offset = std::max(0.0f, MajorStart(realizationRect) - MajorStart(lastExtent));
            const int anchorRowIndex = static_cast<int>(offset / GetMajorSizeWithSpacing(context));

            anchorIndex = std::max(0, std::min(itemsCount - 1, anchorRowIndex * itemsPerLine));
            bounds = GetLayoutRectForDataIndex(availableSize, anchorIndex, lastExtent, context);
        }
    }

    return winrt::FlowLayoutAnchorInfo
    {
        anchorIndex,
        MajorStart(bounds)
    };
}

winrt::FlowLayoutAnchorInfo UniformGridLayout::Algorithm_GetAnchorForTargetElement(
    int targetIndex,
    const winrt::Size & availableSize,
    const winrt::VirtualizingLayoutContext & context)
{
    int index = -1;
    double offset = NAN;
    const int count = context.ItemCount();
    if (targetIndex >= 0 && targetIndex < count)
    {
        const int itemsPerLine = std::min( // note use of unsigned ints
            std::max(1u, static_cast<unsigned int>(Minor(availableSize) / GetMinorSizeWithSpacing(context))),
            std::max(1u, m_maximumRowsOrColumns));
        const int indexOfFirstInLine = (targetIndex / itemsPerLine) * itemsPerLine;
        index = indexOfFirstInLine;
        auto state = GetAsGridState(context.LayoutState());
        offset = MajorStart(GetLayoutRectForDataIndex(availableSize, indexOfFirstInLine, state->FlowAlgorithm().LastExtent(), context));
    }

    return winrt::FlowLayoutAnchorInfo
    {
        index,
        offset
    };
}

winrt::Rect UniformGridLayout::Algorithm_GetExtent(
    const winrt::Size& availableSize,
    const winrt::VirtualizingLayoutContext& context,
    const winrt::UIElement& firstRealized,
    int firstRealizedItemIndex,
    const winrt::Rect& firstRealizedLayoutBounds,
    const winrt::UIElement& lastRealized,
    int lastRealizedItemIndex,
    const winrt::Rect& lastRealizedLayoutBounds)
{
    UNREFERENCED_PARAMETER(lastRealized);

    auto extent = winrt::Rect{};


    // Constants
    const int itemsCount = context.ItemCount();
    const float availableSizeMinor = Minor(availableSize);
    const int itemsPerLine =
        std::min( // note use of unsigned ints
            std::max(1u, std::isfinite(availableSizeMinor)
                ? static_cast<unsigned int>(availableSizeMinor / GetMinorSizeWithSpacing(context))
                : itemsCount),
        std::max(1u, m_maximumRowsOrColumns));
    const float lineSize = GetMajorSizeWithSpacing(context);

    if (itemsCount > 0)
    {
        // Only use all of the space if item stretch is fill, otherwise size layout according to items placed
        MinorSize(extent) =
            std::isfinite(availableSizeMinor) && m_itemsStretch == winrt::UniformGridLayoutItemsStretch::Fill ?
            availableSizeMinor :
            std::max(0.0f, itemsPerLine * GetMinorSizeWithSpacing(context) - static_cast<float>(MinItemSpacing()));
        MajorSize(extent) = std::max(0.0f, (itemsCount / itemsPerLine) * lineSize - static_cast<float>(LineSpacing()));

        if (firstRealized)
        {
            MUX_ASSERT(lastRealized);

            MajorStart(extent) = MajorStart(firstRealizedLayoutBounds) - (firstRealizedItemIndex / itemsPerLine) * lineSize;
            int remainingItems = itemsCount - lastRealizedItemIndex - 1;
            MajorSize(extent) = MajorEnd(lastRealizedLayoutBounds) - MajorStart(extent) + (remainingItems / itemsPerLine) * lineSize;
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
    if (property == s_OrientationProperty)
    {
        auto orientation = unbox_value<winrt::Orientation>(args.NewValue());

        //Note: For UniformGridLayout Vertical Orientation means we have a Horizontal ScrollOrientation. Horizontal Orientation means we have a Vertical ScrollOrientation.
        //i.e. the properties are the inverse of each other.
        const auto scrollOrientation = (orientation == winrt::Orientation::Horizontal) ? ScrollOrientation::Vertical : ScrollOrientation::Horizontal;
        OrientationBasedMeasures::SetScrollOrientation(scrollOrientation);
    }
    else if (property == s_MinColumnSpacingProperty)
    {
        m_minColumnSpacing = unbox_value<double>(args.NewValue());
    }
    else if (property == s_MinRowSpacingProperty)
    {
        m_minRowSpacing = unbox_value<double>(args.NewValue());
    }
    else if (property == s_ItemsJustificationProperty)
    {
        m_itemsJustification = unbox_value<winrt::UniformGridLayoutItemsJustification>(args.NewValue());
    }
    else if (property == s_ItemsStretchProperty)
    {
        m_itemsStretch = unbox_value<winrt::UniformGridLayoutItemsStretch>(args.NewValue());
    }
    else if (property == s_MinItemWidthProperty)
    {
        m_minItemWidth = unbox_value<double>(args.NewValue());
    }
    else if (property == s_MinItemHeightProperty)
    {
        m_minItemHeight = unbox_value<double>(args.NewValue());
    }
    else if (property == s_MaximumRowsOrColumnsProperty)
    {
        m_maximumRowsOrColumns = static_cast<unsigned int>(unbox_value<int>(args.NewValue()));
    }

    InvalidateLayout();
}

#pragma region private helpers

float UniformGridLayout::GetMinorSizeWithSpacing(winrt::VirtualizingLayoutContext const& context)
{
    const auto minItemSpacing = MinItemSpacing();
    const auto gridState = GetAsGridState(context.LayoutState());
    return GetScrollOrientation() == ScrollOrientation::Vertical ?
        static_cast<float>(gridState->EffectiveItemWidth() + minItemSpacing) :
        static_cast<float>(gridState->EffectiveItemHeight() + minItemSpacing);
}

float UniformGridLayout::GetMajorSizeWithSpacing(winrt::VirtualizingLayoutContext const& context)
{
    const auto lineSpacing = LineSpacing();
    const auto gridState = GetAsGridState(context.LayoutState());
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
    const int itemsPerLine = std::min( //note use of unsigned ints
        std::max(1u, static_cast<unsigned int>(Minor(availableSize) / GetMinorSizeWithSpacing(context))),
        std::max(1u, m_maximumRowsOrColumns));
    const int rowIndex = static_cast<int>(index / itemsPerLine);
    const int indexInRow = index - (rowIndex * itemsPerLine);

    auto gridState = GetAsGridState(context.LayoutState());
    const winrt::Rect bounds = MinorMajorRect(
        indexInRow * GetMinorSizeWithSpacing(context) + MinorStart(lastExtent),
        rowIndex * GetMajorSizeWithSpacing(context) + MajorStart(lastExtent),
        GetScrollOrientation() == ScrollOrientation::Vertical ? static_cast<float>(gridState->EffectiveItemWidth()) : static_cast<float>(gridState->EffectiveItemHeight()),
        GetScrollOrientation() == ScrollOrientation::Vertical ? static_cast<float>(gridState->EffectiveItemHeight()) : static_cast<float>(gridState->EffectiveItemWidth()));

    return bounds;
}

#pragma endregion
