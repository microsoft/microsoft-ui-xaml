﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include <DoubleUtil.h>
#include "ItemsRepeater.common.h"
#include "FlowLayoutAlgorithm.h"
#include "StackLayoutState.h"
#include "StackLayout.h"
#include "RuntimeProfiler.h"
#include "VirtualizingLayoutContext.h"

#pragma region IStackLayout

StackLayout::StackLayout()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_StackLayout);
    LayoutId(L"StackLayout");

    UpdateIndexBasedLayoutOrientation(winrt::Orientation::Vertical);
}

#pragma endregion

#pragma region IVirtualizingLayoutOverrides

void StackLayout::InitializeForContextCore(winrt::VirtualizingLayoutContext const& context)
{
    auto state = context.LayoutState();
    winrt::com_ptr<StackLayoutState> stackState = nullptr;
    if (state)
    {
        stackState = GetAsStackState(state);
    }

    if (!stackState)
    {
        if (state)
        {
            throw winrt::hresult_error(E_FAIL, L"LayoutState must derive from StackLayoutState.");
        }

        // Custom deriving layouts could potentially be stateful.
        // If that is the case, we will just create the base state required by UniformGridLayout ourselves.
        stackState = winrt::make_self<StackLayoutState>();
    }

    stackState->InitializeForContext(context, this);
}

void StackLayout::UninitializeForContextCore(winrt::VirtualizingLayoutContext const& context)
{
    if (auto stackState = GetAsStackState(context.LayoutState()))
    {
        stackState->UninitializeForContext(context);
    }
}

winrt::Size StackLayout::MeasureOverride(
    winrt::VirtualizingLayoutContext const& context,
    winrt::Size const& availableSize)
{
    if (!context.LayoutState())
    {
        return {};
    }

    GetAsStackState(context.LayoutState())->OnMeasureStart();

    const auto desiredSize = GetFlowAlgorithm(context).Measure(
        availableSize,
        context,
        false, /* isWrapping*/
        0 /* minItemSpacing */,
        m_itemSpacing,
        MAXUINT /* maxItemsPerLine */,
        GetScrollOrientation(),
        DisableVirtualization(),
        LayoutId());
    return { desiredSize.Width, desiredSize.Height };
}

winrt::Size StackLayout::ArrangeOverride(
    winrt::VirtualizingLayoutContext const& context,
    winrt::Size const& finalSize)
{
    if (!context.LayoutState())
    {
        return {};
    }

    const auto value = GetFlowAlgorithm(context).Arrange(
        finalSize,
        context,
        false, /* isWraping */
        FlowLayoutAlgorithm::LineAlignment::Start,
        LayoutId());

    return { value.Width, value.Height };
}

void StackLayout::OnItemsChangedCore(
    winrt::VirtualizingLayoutContext const& context,
    winrt::IInspectable const& source,
    winrt::NotifyCollectionChangedEventArgs const& args)
{
    if (auto layoutState = context.LayoutState())
    {
        auto& flow = GetAsStackState(layoutState)->FlowAlgorithm();
        flow.OnItemsSourceChanged(source, args, context);
    }
    
    // Always invalidate layout to keep the view accurate.
    InvalidateLayout();
}

#pragma endregion

#pragma region IStackLayoutOverrides

winrt::FlowLayoutAnchorInfo StackLayout::GetAnchorForRealizationRect(
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
        const auto state = GetAsStackState(context.LayoutState());
        const auto lastExtent = state->FlowAlgorithm().LastExtent();

        const double averageElementSize = GetAverageElementSize(availableSize, context, state) + m_itemSpacing;
        const double realizationWindowOffsetInExtent = MajorStart(realizationRect) - MajorStart(lastExtent);
        const double majorSize = MajorSize(lastExtent) == 0 ? std::max(0.0, averageElementSize * itemsCount - m_itemSpacing) : MajorSize(lastExtent);
        if (itemsCount > 0 &&
            MajorSize(realizationRect) >= 0 &&
            // MajorSize = 0 will account for when a nested repeater is outside the realization rect but still being measured. Also,
            // note that if we are measuring this repeater, then we are already realizing an element to figure out the size, so we could
            // just keep that element alive. It also helps in XYFocus scenarios to have an element realized for XYFocus to find a candidate
            // in the navigating direction.
            realizationWindowOffsetInExtent + MajorSize(realizationRect) >= 0 && realizationWindowOffsetInExtent <= majorSize)
        {
            anchorIndex = (int)(realizationWindowOffsetInExtent / averageElementSize);
            offset = anchorIndex * averageElementSize + MajorStart(lastExtent);
            anchorIndex = std::max(0, std::min(itemsCount - 1, anchorIndex));
        }
    }

    return { anchorIndex, offset };
}

winrt::Rect StackLayout::GetExtent(
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

    // Constants
    const int itemsCount = context.ItemCount();
    const auto stackState = GetAsStackState(context.LayoutState());
    const double averageElementSize = GetAverageElementSize(availableSize, context, stackState) + m_itemSpacing;

    MinorSize(extent) = static_cast<float>(stackState->MaxArrangeBounds());
    MajorSize(extent) = std::max(0.0f, static_cast<float>(itemsCount * averageElementSize - m_itemSpacing));
    if (itemsCount > 0)
    {
        if (firstRealized)
        {
            MUX_ASSERT(lastRealized);
            MajorStart(extent) = static_cast<float>(MajorStart(firstRealizedLayoutBounds) - firstRealizedItemIndex * averageElementSize);
            auto remainingItems = itemsCount - lastRealizedItemIndex - 1;
            MajorSize(extent) = MajorEnd(lastRealizedLayoutBounds) - MajorStart(extent) + static_cast<float>(remainingItems* averageElementSize);
        }
        else
        {
            ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_IND_STR_STR_FLT_FLT, METH_NAME, this,
                winrt::get_self<VirtualizingLayoutContext>(context)->Indent(), LayoutId().data(),
                L"Estimating extent with no realized elements.");
        }
    }
    else
    {
        MUX_ASSERT(firstRealizedItemIndex == -1);
        MUX_ASSERT(lastRealizedItemIndex == -1);
    }

#ifdef DBG
    const int indentDbg = winrt::get_self<VirtualizingLayoutContext>(context)->Indent();

    ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_IND_STR_STR_FLT_FLT, METH_NAME, this,
        indentDbg, LayoutId().data(),
        L"Extent:", extent.Width, extent.Height);
    ITEMSREPEATER_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_IND_STR_STR_FLT, METH_NAME, this, 
        indentDbg, LayoutId().data(),
        L"Based on average:", averageElementSize);
#endif // DBG

    return extent;
}

void StackLayout::OnElementMeasured(
    winrt::UIElement const& /*element*/,
    int index,
    winrt::Size const& /*availableSize*/,
    winrt::Size const& /*measureSize*/,
    winrt::Size const& /*desiredSize*/,
    winrt::Size const& provisionalArrangeSize,
    winrt::VirtualizingLayoutContext const& context)
{

    const auto virtualContext = context.try_as<winrt::VirtualizingLayoutContext>();
    if (virtualContext)
    {
        const auto stackState = GetAsStackState(virtualContext.LayoutState());
        const auto provisionalArrangeSizeWinRt = provisionalArrangeSize;
        stackState->OnElementMeasured(
            index,
            Major(provisionalArrangeSizeWinRt),
            Minor(provisionalArrangeSizeWinRt));
    }
}

#pragma endregion

#pragma region IFlowLayoutAlgorithmDelegates

winrt::Size StackLayout::Algorithm_GetMeasureSize(int /*index*/, const winrt::Size & availableSize, const winrt::VirtualizingLayoutContext& /*context*/)
{
    return availableSize;
}

winrt::Size StackLayout::Algorithm_GetProvisionalArrangeSize(int /*index*/, const winrt::Size & measureSize, winrt::Size const& desiredSize, const winrt::VirtualizingLayoutContext& /*context*/)
{
    const auto measureSizeMinor = Minor(measureSize);
    return MinorMajorSize(
        std::isfinite(measureSizeMinor) ?
            std::max(measureSizeMinor, Minor(desiredSize)) :
            Minor(desiredSize),
        Major(desiredSize));
}

bool StackLayout::Algorithm_ShouldBreakLine(int /*index*/, double /*remainingSpace*/)
{
    return true;
}

winrt::FlowLayoutAnchorInfo StackLayout::Algorithm_GetAnchorForRealizationRect(
    const winrt::Size & availableSize,
    const winrt::VirtualizingLayoutContext & context)
{
    return GetAnchorForRealizationRect(availableSize, context);
}

winrt::FlowLayoutAnchorInfo StackLayout::Algorithm_GetAnchorForTargetElement(
    int targetIndex,
    const winrt::Size & availableSize,
    const winrt::VirtualizingLayoutContext & context)
{
    double offset = DoubleUtil::NaN;
    int index = -1;
    const int itemsCount = context.ItemCount();

    if (targetIndex >= 0 && targetIndex < itemsCount)
    {
        index = targetIndex;
        const auto state = GetAsStackState(context.LayoutState());
        const double averageElementSize = GetAverageElementSize(availableSize, context, state) + m_itemSpacing;
        offset = index * averageElementSize + MajorStart(state->FlowAlgorithm().LastExtent());
    }

    return winrt::FlowLayoutAnchorInfo{ index, offset };
}

winrt::Rect StackLayout::Algorithm_GetExtent(
    const winrt::Size & availableSize,
    const winrt::VirtualizingLayoutContext & context,
    const winrt::UIElement & firstRealized,
    int firstRealizedItemIndex,
    const winrt::Rect & firstRealizedLayoutBounds,
    const winrt::UIElement & lastRealized,
    int lastRealizedItemIndex,
    const winrt::Rect & lastRealizedLayoutBounds)
{
    return GetExtent(
        availableSize,
        context,
        firstRealized,
        firstRealizedItemIndex,
        firstRealizedLayoutBounds,
        lastRealized,
        lastRealizedItemIndex,
        lastRealizedLayoutBounds);
}

void StackLayout::Algorithm_OnElementMeasured(
    const winrt::UIElement & element,
    int index,
    const winrt::Size & availableSize,
    const winrt::Size & measureSize,
    const winrt::Size & desiredSize,
    const winrt::Size & provisionalArrangeSize,
    const winrt::VirtualizingLayoutContext & context)
{
    OnElementMeasured(
        element,
        index,
        availableSize,
        measureSize,
        desiredSize,
        provisionalArrangeSize,
        context);
}

#pragma endregion

void StackLayout::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    const auto property = args.Property();
    if (property == s_OrientationProperty)
    {
        const auto orientation = unbox_value<winrt::Orientation>(args.NewValue());

        //Note: For StackLayout Vertical Orientation means we have a Vertical ScrollOrientation.
        //Horizontal Orientation means we have a Horizontal ScrollOrientation.
        const ScrollOrientation scrollOrientation = (orientation == winrt::Orientation::Horizontal) ? ScrollOrientation::Horizontal : ScrollOrientation::Vertical;
        OrientationBasedMeasures::SetScrollOrientation(scrollOrientation);

        UpdateIndexBasedLayoutOrientation(orientation);
    }
    else if (property == s_SpacingProperty)
    {
        m_itemSpacing = unbox_value<double>(args.NewValue());
    }

    InvalidateLayout();
}

#pragma region private helpers

double StackLayout::GetAverageElementSize(
    winrt::Size availableSize,
    winrt::VirtualizingLayoutContext context,
    const winrt::com_ptr<StackLayoutState>& stackLayoutState)
{
    double averageElementSize = 0;

    if (context.ItemCount() > 0)
    {
        if (stackLayoutState->TotalElementsMeasured() == 0)
        {
            const auto tmpElement = context.GetOrCreateElementAt(0, winrt::ElementRealizationOptions::ForceCreate | winrt::ElementRealizationOptions::SuppressAutoRecycle);
            stackLayoutState->FlowAlgorithm().MeasureElement(tmpElement, 0, availableSize, context);
            context.RecycleElement(tmpElement);
        }

        MUX_ASSERT(stackLayoutState->TotalElementsMeasured() > 0);
        averageElementSize = round(stackLayoutState->TotalElementSize() / stackLayoutState->TotalElementsMeasured());
    }

    return averageElementSize;
}

void StackLayout::UpdateIndexBasedLayoutOrientation(const winrt::Orientation& orientation)
{
    SetIndexBasedLayoutOrientation(orientation == winrt::Orientation::Horizontal ?
        winrt::IndexBasedLayoutOrientation::LeftToRight : winrt::IndexBasedLayoutOrientation::TopToBottom);
}

#pragma endregion
