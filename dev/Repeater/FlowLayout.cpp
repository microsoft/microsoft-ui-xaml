// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include <DoubleUtil.h>
#include "ItemsRepeater.common.h"
#include "FlowLayoutAlgorithm.h"
#include "FlowLayoutState.h"
#include "FlowLayout.h"
#include "VirtualizingLayoutContext.h"

FlowLayout::FlowLayout()
{
    LayoutId(L"FlowLayout");
}

#pragma region IVirtualizingLayoutOverrides

void FlowLayout::InitializeForContextCore(winrt::VirtualizingLayoutContext const& context)
{
    auto state = context.LayoutState();
    winrt::com_ptr<FlowLayoutState> flowState = nullptr;
    if (state)
    {
        flowState = GetAsFlowState(state);
    }

    if (!flowState)
    {
        if (state)
        {
            throw winrt::hresult_error(E_FAIL, L"LayoutState must derive from FlowLayoutState.");
        }

        // Custom deriving layouts could potentially be stateful.
        // If that is the case, we will just create the base state required by FlowLayout ourselves.
        flowState = winrt::make_self<FlowLayoutState>();
    }

    flowState->InitializeForContext(context, this);
}

void FlowLayout::UninitializeForContextCore(winrt::VirtualizingLayoutContext const& context)
{
    auto flowState = GetAsFlowState(context.LayoutState());
    flowState->UninitializeForContext(context);
}

winrt::Size FlowLayout::MeasureOverride(
    winrt::VirtualizingLayoutContext const& context,
    winrt::Size const& availableSize)
{
    auto desiredSize = GetFlowAlgorithm(context).Measure(
        availableSize,
        context,
        true, /* isWrapping*/
        MinItemSpacing(),
        LineSpacing(),
        std::numeric_limits<unsigned int>::max() /* maxItemsPerLine */,
        OrientationBasedMeasures::GetScrollOrientation(),
        false /* disableVirtualization */,
        LayoutId());
    return desiredSize;
}

winrt::Size FlowLayout::ArrangeOverride(
    winrt::VirtualizingLayoutContext const& context,
    winrt::Size const& finalSize)
{
    auto value = GetFlowAlgorithm(context).Arrange(
        finalSize,
        context,
        true, /* isWrapping */
        static_cast<FlowLayoutAlgorithm::LineAlignment>(m_lineAlignment),        
        LayoutId());
    return value;
}


void FlowLayout::OnItemsChangedCore(
    winrt::VirtualizingLayoutContext const& context,
    winrt::IInspectable const& source,
    winrt::NotifyCollectionChangedEventArgs const& args)
{
    GetFlowAlgorithm(context).OnItemsSourceChanged(source, args, context);
    // Always invalidate layout to keep the view accurate.
    InvalidateLayout();
}

#pragma endregion

#pragma region IFlowLayoutOverrides

winrt::Size FlowLayout::GetMeasureSize(
    int /*index*/,
    winrt::Size const& availableSize)
{
    return availableSize;
}

winrt::Size FlowLayout::GetProvisionalArrangeSize(
    int /*index*/,
    winrt::Size const& /*measureSize*/,
    winrt::Size const& desiredSize)
{
    return desiredSize;
}

bool FlowLayout::ShouldBreakLine(
    int /*index*/,
    double remainingSpace)
{
    return remainingSpace < 0;
}

winrt::FlowLayoutAnchorInfo FlowLayout::GetAnchorForRealizationRect(
    winrt::Size const& availableSize,
    winrt::VirtualizingLayoutContext const& context)
{
    int anchorIndex = -1;
    double offset = DoubleUtil::NaN;

    // Constants
    const int itemsCount = context.ItemCount();
    if (itemsCount > 0)
    {
        const auto realizationRect = context.RealizationRect();
        const auto state = context.LayoutState();
        const auto flowState = GetAsFlowState(state);
        const auto lastExtent = flowState->FlowAlgorithm().LastExtent();

        double averageItemsPerLine = 0;
        const double averageLineSize = GetAverageLineInfo(availableSize, context, flowState, averageItemsPerLine) + LineSpacing();
        MUX_ASSERT(averageItemsPerLine != 0);

        const double extentMajorSize = MajorSize(lastExtent) == 0 ? (itemsCount / averageItemsPerLine) * averageLineSize : MajorSize(lastExtent);
        if (itemsCount > 0 &&
            MajorSize(realizationRect) > 0 &&
            DoesRealizationWindowOverlapExtent(realizationRect, MinorMajorRect(MinorStart(lastExtent), MajorStart(lastExtent), Minor(availableSize), static_cast<float>(extentMajorSize))))
        {
            const double realizationWindowStartWithinExtent = MajorStart(realizationRect) - MajorStart(lastExtent);
            const int lineIndex = std::max(0, (int)(realizationWindowStartWithinExtent / averageLineSize));
            anchorIndex = (int)(lineIndex * averageItemsPerLine);

            // Clamp it to be within valid range
            anchorIndex = std::max(0, std::min(itemsCount - 1, anchorIndex));
            offset = lineIndex * averageLineSize + MajorStart(lastExtent);
        }
    }

    return { anchorIndex, offset };
}

winrt::FlowLayoutAnchorInfo FlowLayout::GetAnchorForTargetElement(
    int targetIndex,
    winrt::Size const& availableSize,
    winrt::VirtualizingLayoutContext const& context)
{
    double offset = DoubleUtil::NaN;
    int index = -1;
    const int itemsCount = context.ItemCount();

    if (targetIndex >= 0 && targetIndex < itemsCount)
    {
        index = targetIndex;
        const auto state = context.LayoutState();
        auto flowState = GetAsFlowState(state);
        double averageItemsPerLine = 0;
        const double averageLineSize = GetAverageLineInfo(availableSize, context, flowState, averageItemsPerLine) + LineSpacing();
        const int lineIndex = (int)(targetIndex / averageItemsPerLine);
        offset = lineIndex * averageLineSize + MajorStart(flowState->FlowAlgorithm().LastExtent());
    }

    return { index, offset };
}

winrt::Rect FlowLayout::GetExtent(
    winrt::Size const& availableSize,
    winrt::VirtualizingLayoutContext const& context,
    winrt::UIElement const& firstRealized,
    int firstRealizedItemIndex,
    winrt::Rect const& firstRealizedLayoutBounds,
    winrt::UIElement const& lastRealized,
    int lastRealizedItemIndex,
    winrt::Rect const& lastRealizedLayoutBounds)
{
    UNREFERENCED_PARAMETER(lastRealized);

    auto extent = winrt::Rect{};

    const int itemsCount = context.ItemCount();

    if (itemsCount > 0)
    {
        const float availableSizeMinor = Minor(availableSize);
        const auto state = context.LayoutState();
        const auto flowState = GetAsFlowState(state);
        double averageItemsPerLine = 0;
        const double averageLineSize = GetAverageLineInfo(availableSize, context, flowState, averageItemsPerLine) + LineSpacing();

        MUX_ASSERT(averageItemsPerLine != 0);
        if (firstRealized)
        {
            MUX_ASSERT(lastRealized);
            const int linesBeforeFirst = static_cast<int>(firstRealizedItemIndex / averageItemsPerLine);
            const double extentMajorStart = MajorStart(firstRealizedLayoutBounds) - linesBeforeFirst * averageLineSize;
            MajorStart(extent) = static_cast<float>(extentMajorStart);
            const int remainingItems = itemsCount - lastRealizedItemIndex - 1;
            const int remainingLinesAfterLast = static_cast<int>((remainingItems / averageItemsPerLine));
            const double extentMajorSize = MajorEnd(lastRealizedLayoutBounds) - MajorStart(extent) + remainingLinesAfterLast * averageLineSize;
            MajorSize(extent) = static_cast<float>(extentMajorSize);

            // If the available size is infinite, we will have realized all the items in one line.
            // In that case, the extent in the non virtualizing direction should be based on the
            // right/bottom of the last realized element.
            MinorSize(extent) =
                std::isfinite(availableSizeMinor) ?
                availableSizeMinor :
                std::max(0.0f, MinorEnd(lastRealizedLayoutBounds));
        }
        else
        {
            const auto lineSpacing = LineSpacing();
            const auto minItemSpacing = MinItemSpacing();
            // We dont have anything realized. make an educated guess.
            const int numLines = (int)std::ceil(itemsCount / averageItemsPerLine);
            extent =
                std::isfinite(availableSizeMinor) ?
                MinorMajorRect(0, 0, availableSizeMinor, std::max(0.0f, static_cast<float>(numLines * averageLineSize - lineSpacing))) :
                MinorMajorRect(
                    0,
                    0,
                    std::max(0.0f, static_cast<float>((Minor(flowState->SpecialElementDesiredSize()) + minItemSpacing) * itemsCount - minItemSpacing)),
                    std::max(0.0f, static_cast<float>(averageLineSize - lineSpacing)));
            REPEATER_TRACE_INFO(L"%*s: \tEstimating extent with no realized elements. \n", winrt::get_self<VirtualizingLayoutContext>(context)->Indent(), LayoutId().data());
        }

        REPEATER_TRACE_INFO(L"%*s: \tExtent is {%.0f,%.0f}. Based on average line size {%.0f} and average items per line {%.0f}. \n",
            winrt::get_self<VirtualizingLayoutContext>(context)->Indent(), LayoutId().data(), extent.Width, extent.Height, averageLineSize, averageItemsPerLine);
    }
    else
    {
        MUX_ASSERT(firstRealizedItemIndex == -1);
        MUX_ASSERT(lastRealizedItemIndex == -1);

        REPEATER_TRACE_INFO(L"%*s: \tExtent is {%.0f,%.0f}. ItemCount is 0 \n",
            winrt::get_self<VirtualizingLayoutContext>(context)->Indent(), LayoutId().data(), extent.Width, extent.Height);
    }

    return extent;
}

void FlowLayout::OnElementMeasured(
    winrt::UIElement const& /*element*/,
    int /*index*/,
    winrt::Size const& /*availableSize*/,
    winrt::Size const& /*measureSize*/,
    winrt::Size const& /*desiredSize*/,
    winrt::Size const& /*provisionalArrangeSize*/,
    winrt::VirtualizingLayoutContext const& /*context*/)
{
}

void FlowLayout::OnLineArranged(
    int startIndex,
    int countInLine,
    double lineSize,
    winrt::VirtualizingLayoutContext const& context)
{

    REPEATER_TRACE_INFO(L"%*s: \tOnLineArranged startIndex:%d Count:%d LineHeight:%d \n",
        winrt::get_self<VirtualizingLayoutContext>(context)->Indent(), LayoutId().data(), startIndex, countInLine, lineSize);

    const auto flowState = GetAsFlowState(context.LayoutState());
    flowState->OnLineArranged(startIndex, countInLine, lineSize, context);
}

#pragma endregion

#pragma region IFlowLayoutAlgorithmDelegates

winrt::Size FlowLayout::Algorithm_GetMeasureSize(int index, const winrt::Size & availableSize, const winrt::VirtualizingLayoutContext& /*context*/)
{
    return overridable().GetMeasureSize(index, availableSize);
}

winrt::Size FlowLayout::Algorithm_GetProvisionalArrangeSize(int index, const winrt::Size & measureSize, winrt::Size const& desiredSize, const winrt::VirtualizingLayoutContext& /*context*/)
{
    return overridable().GetProvisionalArrangeSize(index, measureSize, desiredSize);
}

bool FlowLayout::Algorithm_ShouldBreakLine(int index, double remainingSpace)
{
    return overridable().ShouldBreakLine(index, remainingSpace);
}

winrt::FlowLayoutAnchorInfo FlowLayout::Algorithm_GetAnchorForRealizationRect(
    const winrt::Size & availableSize,
    const winrt::VirtualizingLayoutContext & context)
{
    return overridable().GetAnchorForRealizationRect(availableSize, context);
}

winrt::FlowLayoutAnchorInfo FlowLayout::Algorithm_GetAnchorForTargetElement(
    int targetIndex,
    const winrt::Size & availableSize,
    const winrt::VirtualizingLayoutContext & context)
{
    return overridable().GetAnchorForTargetElement(targetIndex, availableSize, context);
}

winrt::Rect FlowLayout::Algorithm_GetExtent(
    const winrt::Size & availableSize,
    const winrt::VirtualizingLayoutContext & context,
    const winrt::UIElement & firstRealized,
    int firstRealizedItemIndex,
    const winrt::Rect & firstRealizedLayoutBounds,
    const winrt::UIElement & lastRealized,
    int lastRealizedItemIndex,
    const winrt::Rect & lastRealizedLayoutBounds)
{
    return overridable().GetExtent(
        availableSize,
        context,
        firstRealized,
        firstRealizedItemIndex,
        firstRealizedLayoutBounds,
        lastRealized,
        lastRealizedItemIndex,
        lastRealizedLayoutBounds);
}

void FlowLayout::Algorithm_OnElementMeasured(
    const winrt::UIElement & element,
    const int index,
    const winrt::Size & availableSize,
    const winrt::Size & measureSize,
    const winrt::Size & desiredSize,
    const winrt::Size & provisionalArrangeSize,
    const winrt::VirtualizingLayoutContext & context)
{
    overridable().OnElementMeasured(
        element,
        index,
        availableSize,
        measureSize,
        desiredSize,
        provisionalArrangeSize,
        context);
}

void FlowLayout::Algorithm_OnLineArranged(
    int startIndex,
    int countInLine,
    double lineSize,
    const winrt::VirtualizingLayoutContext & context)
{
    return overridable().OnLineArranged(
        startIndex,
        countInLine,
        lineSize,
        context);
}

#pragma endregion

void FlowLayout::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    auto property = args.Property();
    if (property == s_OrientationProperty)
    {
        auto orientation = unbox_value<winrt::Orientation>(args.NewValue());

        //Note: For FlowLayout Vertical Orientation means we have a Horizontal ScrollOrientation. Horizontal Orientation means we have a Vertical ScrollOrientation.
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
    else if (property == s_LineAlignmentProperty)
    {
        m_lineAlignment = unbox_value<winrt::FlowLayoutLineAlignment>(args.NewValue());
    }

    InvalidateLayout();
}

#pragma region private helpers

double FlowLayout::GetAverageLineInfo(
    const winrt::Size& availableSize,
    const winrt::VirtualizingLayoutContext& context,
    const winrt::com_ptr<FlowLayoutState>& flowState,
    double& avgCountInLine)
{
    // default to 1 item per line with 0 size
    double avgLineSize = 0;
    avgCountInLine = 1;

    MUX_ASSERT(context.ItemCountCore() > 0);
    if (flowState->TotalLinesMeasured() == 0)
    {
        const auto tmpElement = context.GetOrCreateElementAt(0, winrt::ElementRealizationOptions::ForceCreate | winrt::ElementRealizationOptions::SuppressAutoRecycle);
        const auto desiredSize = flowState->FlowAlgorithm().MeasureElement(tmpElement, 0, availableSize, context);
        context.RecycleElement(tmpElement);

        const int estimatedCountInLine = std::max(1, static_cast<int>(Minor(availableSize) / Minor(desiredSize)));
        flowState->OnLineArranged(0, estimatedCountInLine, Major(desiredSize), context);
        flowState->SpecialElementDesiredSize(desiredSize);
    }

    avgCountInLine = std::max(1.0, flowState->TotalItemsPerLine() / flowState->TotalLinesMeasured());
    avgLineSize = round(flowState->TotalLineSize() / flowState->TotalLinesMeasured());

    return avgLineSize;
}

#pragma endregion
