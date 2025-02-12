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

#pragma region IUniformGridLayout

UniformGridLayout::UniformGridLayout()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_UniformGridLayout);
    LayoutId(L"UniformGridLayout");

    UpdateIndexBasedLayoutOrientation(winrt::Orientation::Horizontal);
}

#pragma endregion

#pragma region IVirtualizingLayoutOverrides

void UniformGridLayout::InitializeForContextCore(
    winrt::VirtualizingLayoutContext const& context)
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

void UniformGridLayout::UninitializeForContextCore(
    winrt::VirtualizingLayoutContext const& context)
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
        true /* isVirtualizationEnabled */,
        LayoutId());

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

    auto gridState = GetAsGridState(context.LayoutState());
    gridState->InvalidateElementSize();

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
}
#pragma endregion

#pragma region IFlowLayoutAlgorithmDelegates

winrt::Size UniformGridLayout::Algorithm_GetMeasureSize(
    int index,
    const winrt::Size& availableSize,
    const winrt::VirtualizingLayoutContext& context)
{
    const auto gridState = GetAsGridState(context.LayoutState());
    return winrt::Size{ static_cast<float>(gridState->EffectiveItemWidth()),static_cast<float>(gridState->EffectiveItemHeight()) };
}

winrt::Size UniformGridLayout::Algorithm_GetProvisionalArrangeSize(
    int /*index*/,
    const winrt::Size& /*measureSize*/,
    const winrt::Size& /*desiredSize*/,
    const winrt::VirtualizingLayoutContext& context)
{
    const auto gridState = GetAsGridState(context.LayoutState());
    return winrt::Size{ static_cast<float>(gridState->EffectiveItemWidth()),static_cast<float>(gridState->EffectiveItemHeight()) };
}

bool UniformGridLayout::Algorithm_ShouldBreakLine(
    int /*index*/,
    double remainingSpace)
{
    return remainingSpace < 0;
}

winrt::FlowLayoutAnchorInfo UniformGridLayout::Algorithm_GetAnchorForRealizationRect(
    const winrt::Size& availableSize,
    const winrt::VirtualizingLayoutContext& context)
{
    winrt::Rect bounds = winrt::Rect{ NAN, NAN, NAN, NAN };
    int anchorIndex = -1;

    const int itemsCount = context.ItemCount();
    const auto realizationRect = context.RealizationRect();

    if (itemsCount > 0 && MajorSize(realizationRect) > 0)
    {
        const auto gridState = GetAsGridState(context.LayoutState());
        const auto lastExtent = gridState->FlowAlgorithm().LastExtent();
        const unsigned int itemsPerLine = GetItemsPerLine(availableSize, context);
        const float majorSize = GetMajorSize(itemsCount, itemsPerLine, GetMajorItemSizeWithSpacing(context));
        const float realizationWindowStartWithinExtent = MajorStart(realizationRect) - MajorStart(lastExtent);

        if (realizationWindowStartWithinExtent + MajorSize(realizationRect) >= 0.0f && realizationWindowStartWithinExtent <= majorSize)
        {
            const double offset = std::max(0.0f, MajorStart(realizationRect) - MajorStart(lastExtent));
            const int anchorLineIndex = static_cast<int>(offset / GetMajorItemSizeWithSpacing(context));

            anchorIndex = std::max(0, std::min(itemsCount - 1, static_cast<int>(anchorLineIndex * itemsPerLine)));
            bounds = GetLayoutRectForDataIndex(availableSize, anchorIndex, lastExtent, context);
        }
    }

    return { anchorIndex, MajorStart(bounds) };
}

winrt::FlowLayoutAnchorInfo UniformGridLayout::Algorithm_GetAnchorForTargetElement(
    int targetIndex,
    const winrt::Size& availableSize,
    const winrt::VirtualizingLayoutContext& context)
{
    const int count = context.ItemCount();

    if (targetIndex >= 0 && targetIndex < count)
    {
        // The anchor index returned is NOT the first index in the targetIndex's line. It is the targetIndex
        // itself, in order to stay consistent with the ElementManager::DiscardElementsOutsideWindow method
        // which keeps a single element prior to the realization window. If the first index in the targetIndex's
        // line were used as the anchor, it would be discarded and re-recreated in an infinite loop.
        const auto gridState = GetAsGridState(context.LayoutState());
        const double offset = MajorStart(GetLayoutRectForDataIndex(availableSize, targetIndex, gridState->FlowAlgorithm().LastExtent(), context));

        return { targetIndex, offset };
    }

    return { -1, NAN };
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
    const unsigned int itemsPerLine =
        std::min( // note use of unsigned ints
            std::max(1u, std::isfinite(availableSizeMinor)
                ? static_cast<unsigned int>((availableSizeMinor + MinItemSpacing()) / GetMinorItemSizeWithSpacing(context))
                : itemsCount),
            std::max(1u, m_maximumRowsOrColumns));
    const float lineSize = GetMajorItemSizeWithSpacing(context);

    if (itemsCount > 0)
    {
        // Only use all of the space if item stretch is fill, otherwise size layout according to items placed
        MinorSize(extent) =
            std::isfinite(availableSizeMinor) && m_itemsStretch == winrt::UniformGridLayoutItemsStretch::Fill ?
            availableSizeMinor :
            std::max(0.0f, itemsPerLine * GetMinorItemSizeWithSpacing(context) - static_cast<float>(MinItemSpacing()));
        MajorSize(extent) = GetMajorSize(itemsCount, itemsPerLine, lineSize);

        if (firstRealized)
        {
            MUX_ASSERT(lastRealized);

            MajorStart(extent) = MajorStart(firstRealizedLayoutBounds) - (firstRealizedItemIndex / itemsPerLine) * lineSize;
            const int remainingItemsOnLastRealizedLine = std::min(itemsCount - lastRealizedItemIndex - 1, static_cast<int>(itemsPerLine - ((lastRealizedItemIndex + 1) % itemsPerLine)));
            const int remainingItems = itemsCount - lastRealizedItemIndex - 1 - remainingItemsOnLastRealizedLine;
            const float remainingItemsMajorSize = GetMajorSize(remainingItems, itemsPerLine, lineSize);
            MajorSize(extent) = MajorEnd(lastRealizedLayoutBounds) - MajorStart(extent) + (remainingItemsMajorSize > 0.0f ? (static_cast<float>(LineSpacing()) + remainingItemsMajorSize) : 0.0f);
        }
        else
        {
            ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_STR, METH_NAME, this, LayoutId().data(), L"Estimating extent with no realized elements.");
        }
    }
    else
    {
        MUX_ASSERT(firstRealizedItemIndex == -1);
        MUX_ASSERT(lastRealizedItemIndex == -1);
    }

    ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_STR_FLT_FLT, METH_NAME, this,
        LayoutId().data(),
        L"Extent X,Y:", extent.X, extent.Y);
    ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_STR_FLT_FLT, METH_NAME, this,
        LayoutId().data(),
        L"Extent W,H:", extent.Width, extent.Height);
    ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_FLT, METH_NAME, this,
        L"Based on lineSize:", lineSize);
    ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR_INT, METH_NAME, this,
        L"Based on items per line:", itemsPerLine);
    return extent;
}

#ifdef DBG
int UniformGridLayout::Algorithm_GetFlowLayoutLogItemIndexDbg()
{
    return LogItemIndexDbg();
}

void UniformGridLayout::Algorithm_SetFlowLayoutAnchorInfoDbg(int index, double offset)
{
    SetLayoutAnchorInfoDbg(index, offset);
}
#endif // DBG

#pragma endregion

void UniformGridLayout::OnPropertyChanged(
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    auto property = args.Property();
    if (property == s_OrientationProperty)
    {
        auto orientation = unbox_value<winrt::Orientation>(args.NewValue());

        //Note: For UniformGridLayout Vertical Orientation means we have a Horizontal ScrollOrientation. Horizontal Orientation means we have a Vertical ScrollOrientation.
        //i.e. the properties are the inverse of each other.
        const auto scrollOrientation = (orientation == winrt::Orientation::Horizontal) ? ScrollOrientation::Vertical : ScrollOrientation::Horizontal;
        OrientationBasedMeasures::SetScrollOrientation(scrollOrientation);

        UpdateIndexBasedLayoutOrientation(orientation);
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

unsigned int UniformGridLayout::GetItemsPerLine(
    winrt::Size const& availableSize,
    winrt::VirtualizingLayoutContext const& context)
{
    const float availableSizeMinor = Minor(availableSize);
    const unsigned int maximumRowsOrColumns = std::max(1u, m_maximumRowsOrColumns);

    if (std::isfinite(availableSizeMinor))
    {
        return std::min(
            static_cast<unsigned int>((availableSizeMinor + MinItemSpacing()) / GetMinorItemSizeWithSpacing(context)),
            maximumRowsOrColumns);
    }

    return maximumRowsOrColumns;
}

float UniformGridLayout::GetMajorSize(
    int itemsCount,
    unsigned int itemsPerLine,
    float majorItemSizeWithSpacing)
{
    MUX_ASSERT(itemsPerLine > 0);

    const int fullLinesCount = itemsCount / itemsPerLine;
    const int partialLineCount = (itemsCount % itemsPerLine) == 0 ? 0 : 1;
    const int totalLinesCount = fullLinesCount + partialLineCount;

    if (totalLinesCount > 0)
    {
        return totalLinesCount * majorItemSizeWithSpacing - static_cast<float>(LineSpacing());
    }

    return 0.0f;
}

float UniformGridLayout::GetMinorItemSizeWithSpacing(
    winrt::VirtualizingLayoutContext const& context)
{
    const auto minItemSpacing = MinItemSpacing();
    const auto gridState = GetAsGridState(context.LayoutState());
    return GetScrollOrientation() == ScrollOrientation::Vertical ?
        static_cast<float>(gridState->EffectiveItemWidth() + minItemSpacing) :
        static_cast<float>(gridState->EffectiveItemHeight() + minItemSpacing);
}

float UniformGridLayout::GetMajorItemSizeWithSpacing(
    winrt::VirtualizingLayoutContext const& context)
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
    const unsigned int itemsPerLine = GetItemsPerLine(availableSize, context);
    const int lineIndex = index / itemsPerLine;
    const int indexInLine = index - (lineIndex * itemsPerLine);

    const auto gridState = GetAsGridState(context.LayoutState());
    const winrt::Rect bounds = MinorMajorRect(
        indexInLine * GetMinorItemSizeWithSpacing(context) + MinorStart(lastExtent),
        lineIndex * GetMajorItemSizeWithSpacing(context) + MajorStart(lastExtent),
        GetScrollOrientation() == ScrollOrientation::Vertical ? static_cast<float>(gridState->EffectiveItemWidth()) : static_cast<float>(gridState->EffectiveItemHeight()),
        GetScrollOrientation() == ScrollOrientation::Vertical ? static_cast<float>(gridState->EffectiveItemHeight()) : static_cast<float>(gridState->EffectiveItemWidth()));

    return bounds;
}

void UniformGridLayout::UpdateIndexBasedLayoutOrientation(
    const winrt::Orientation& orientation)
{
    SetIndexBasedLayoutOrientation(orientation == winrt::Orientation::Horizontal ?
        winrt::IndexBasedLayoutOrientation::LeftToRight : winrt::IndexBasedLayoutOrientation::TopToBottom);
}

#pragma endregion
