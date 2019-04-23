// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "FlexboxLayout.h"
#include "RuntimeProfiler.h"
#include "FlexboxLayoutState.h"

FlexboxLayout::FlexboxLayout()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_FlexboxLayout);
}

winrt::FlexboxWrap FlexboxLayout::Wrap()
{
    return m_wrap;
}

void FlexboxLayout::Wrap(winrt::FlexboxWrap const& value)
{
    m_wrap = value;
}

winrt::FlexboxDirection FlexboxLayout::Direction()
{
    return m_direction;
}

void FlexboxLayout::Direction(winrt::FlexboxDirection const& value)
{
    m_direction = value;
}

winrt::FlexboxJustifyContent FlexboxLayout::JustifyContent()
{
    return m_justifyContent;
}

void FlexboxLayout::JustifyContent(winrt::FlexboxJustifyContent const& value)
{
    m_justifyContent = value;
}

winrt::FlexboxAlignItems FlexboxLayout::AlignItems()
{
    return m_alignItems;
}

void FlexboxLayout::AlignItems(winrt::FlexboxAlignItems const& value)
{
    m_alignItems = value;
}

winrt::FlexboxAlignContent FlexboxLayout::AlignContent()
{
    return m_alignContent;
}

void FlexboxLayout::AlignContent(winrt::FlexboxAlignContent const& value)
{
    m_alignContent = value;
}

bool FlexboxLayout::IsHorizontal()
{
    return (m_direction == winrt::FlexboxDirection::Row || m_direction == winrt::FlexboxDirection::RowReverse);
}

bool FlexboxLayout::IsReversed()
{
    return (m_direction == winrt::FlexboxDirection::RowReverse || m_direction == winrt::FlexboxDirection::ColumnReverse);
}

bool FlexboxLayout::IsWrapping()
{
    return (m_wrap != winrt::FlexboxWrap::NoWrap);
}

float FlexboxLayout::MainAxis(winrt::Size const& value)
{
    return (IsHorizontal() ? value.Width : value.Height);
}

float FlexboxLayout::CrossAxis(winrt::Size const& value)
{
    return (IsHorizontal() ? value.Height : value.Width);
}

winrt::Size FlexboxLayout::CreateSize(float mainAxis, float crossAxis)
{
    return IsHorizontal() ?
        winrt::Size{ mainAxis, crossAxis } :
        winrt::Size{ crossAxis, mainAxis };
}

winrt::Point FlexboxLayout::CreatePoint(float mainAxis, float crossAxis)
{
    return IsHorizontal() ?
        winrt::Point{ mainAxis, crossAxis } :
        winrt::Point{ crossAxis, mainAxis };
}

std::vector<winrt::UIElement> FlexboxLayout::ChildrenSortedByOrder(winrt::NonVirtualizingLayoutContext const& context)
{
    std::vector<winrt::UIElement> sorted;

    for (auto const& child : context.Children())
    {
        sorted.push_back(child);
    }
    // PORT_TODO
#if FALSE
    sorted.Sort((winrt::UIElement const& a, winrt::UIElement const& b) = >
    {
        return (GetOrder(a) - GetOrder(b));
    });
#endif

    return sorted;
}

void FlexboxLayout::InitializeForContextCore(winrt::LayoutContext const& context)
{
    auto state = context.LayoutState();
    winrt::com_ptr<FlexboxLayoutState> flexboxState = nullptr;
    if (state)
    {
        flexboxState = state.as<FlexboxLayoutState>();
    }

    if (!flexboxState)
    {
        if (state)
        {
            throw winrt::hresult_error(E_FAIL, L"LayoutState must derive from FlexboxLayoutState.");
        }

        flexboxState = winrt::make_self<FlexboxLayoutState>();
    }

    context.LayoutStateCore(*flexboxState);
}

void FlexboxLayout::UninitializeForContextCore(winrt::LayoutContext const& context)
{
}

winrt::Size FlexboxLayout::MeasureOverride(
    winrt::LayoutContext const& context,
    winrt::Size const& availableSize)
{
    auto state = context.LayoutState().as<FlexboxLayoutState>();
    state->Rows.clear();

    unsigned int itemsInRow = 0;
    float growInRow = 0.0;

    float usedInCurrentMainAxis = 0;
    float usedInCurrentCrossAxis = 0;

    float usedMainAxis = 0;
    float usedCrossAxis = 0;

    auto completeRow = [&]()
    {
        FlexboxLayoutState::RowMeasureInfo newRow;
        newRow.MainAxis = usedInCurrentMainAxis;
        newRow.CrossAxis = usedInCurrentCrossAxis;
        newRow.Count = itemsInRow;
        newRow.Grow = growInRow;
        state->Rows.emplace_back(newRow);

        itemsInRow = 0;
        growInRow = 0.0;
        usedMainAxis = std::max(usedMainAxis, usedInCurrentMainAxis);
        usedInCurrentMainAxis = 0;
        usedCrossAxis += usedInCurrentCrossAxis;
        usedInCurrentCrossAxis = 0;
    };

    std::vector<winrt::UIElement> sortedChildren = ChildrenSortedByOrder(context.try_as<winrt::NonVirtualizingLayoutContext>());
    for (winrt::UIElement const& child : sortedChildren)
    {
        // Give each child the maximum available space
        // TODO: What about flex-shrink? Should we try them with less?
        // TODO: This is where flex-basis would come into play
        child.Measure(availableSize);
        winrt::Size childDesiredSize = child.DesiredSize();

        if (usedInCurrentMainAxis + MainAxis(childDesiredSize) > MainAxis(availableSize))
        {
            // Not enough space, time for a new row
            if (IsWrapping())
            {
                completeRow();

                // It's possible that even making a new row won't work. Sorry, you're not going to fit!
                if (usedCrossAxis + CrossAxis(childDesiredSize) > CrossAxis(availableSize))
                {
                    // TODO: What about flex-shrink?
                    break;
                }
            }
            else
            {
                // Without the ability to wrap, we just flat out can't fit this item
                // TODO: What about flex-shrink?
                break;
            }
        }

        // Contribute our space
        usedInCurrentMainAxis += MainAxis(childDesiredSize);
        usedInCurrentCrossAxis = std::max(usedInCurrentCrossAxis, CrossAxis(childDesiredSize));
        itemsInRow++;
        // PORT_TODO
        //growInRow += GetGrow(child);
    }

    // Incorporate any contribution from the pending row into our total calculation
    if (usedInCurrentMainAxis > 0)
    {
        completeRow();
    }

    winrt::Size returnSize = CreateSize(
        usedMainAxis,
        usedCrossAxis);

    return returnSize;
}

winrt::Size FlexboxLayout::ArrangeOverride(
    winrt::LayoutContext const& context,
    winrt::Size const& finalSize)
{
    auto state = context.LayoutState().as<FlexboxLayoutState>();

    int rowIndex = 0;
    float usedInCurrentMainAxis = 0;
    float crossOffsetForCurrentRow = 0;
    float usedInCurrentCrossAxis = 0;

    // In reverse wrap mode we work our way from the bottom up
    // TODO: Using finalSize here is causing us to right/bottom align, which probably isn't correct.
    if ((m_wrap == winrt::FlexboxWrap::WrapReverse) && (state->Rows.size() > 1))
    {
        crossOffsetForCurrentRow = CrossAxis(finalSize) - (state->Rows[state->Rows.size() - 1].CrossAxis);
    }

    std::vector<winrt::UIElement> sortedChildren = ChildrenSortedByOrder(context.try_as<winrt::NonVirtualizingLayoutContext>());
    for (winrt::UIElement const& child : sortedChildren)
    {
        FlexboxLayoutState::RowMeasureInfo info = state->Rows[rowIndex];

        winrt::Size childDesiredSize = child.DesiredSize();
        if (usedInCurrentMainAxis + MainAxis(childDesiredSize) > MainAxis(finalSize))
        {
            // If we're not wrapping just hide all the remaining elements
            if (!IsWrapping())
            {
                usedInCurrentMainAxis = MainAxis(finalSize);
                child.Arrange(winrt::Rect{ 0, 0, 0, 0 });
                continue;
            }

            usedInCurrentMainAxis = 0;
            float effectiveUsedInCurrentCrossAxis = (usedInCurrentCrossAxis > 0 ? usedInCurrentCrossAxis : info.CrossAxis);
            if (m_wrap == winrt::FlexboxWrap::WrapReverse)
            {
                crossOffsetForCurrentRow -= effectiveUsedInCurrentCrossAxis;
            }
            else
            {
                crossOffsetForCurrentRow += effectiveUsedInCurrentCrossAxis;
            }
            rowIndex++;
            info = state->Rows[rowIndex];
        }

        float mainOffset = usedInCurrentMainAxis;
        float excessMainAxis = (MainAxis(finalSize) - info.MainAxis);

        // Remove excess according to growing items
        float growSlice = 0.0;
        if (info.Grow > 0.0)
        {
            growSlice = excessMainAxis / info.Grow;
            excessMainAxis = 0.0;
        }

        // Grow to take up leftover space according to the grow ratio
        // PORT_TODO
        float grow = 0.0;
        //float grow = GetGrow(child);
        if (grow > 0.0)
        {
            childDesiredSize = CreateSize(MainAxis(childDesiredSize) + (grow * growSlice), CrossAxis(childDesiredSize));
        }

        switch (m_justifyContent)
        {
        case winrt::FlexboxJustifyContent::Start:
            break;
        case winrt::FlexboxJustifyContent::Center:
            mainOffset += (excessMainAxis * 0.5f);
            break;
        case winrt::FlexboxJustifyContent::End:
            mainOffset += excessMainAxis;
            break;
        case winrt::FlexboxJustifyContent::SpaceBetween:
            if (info.Count > 1)
            {
                usedInCurrentMainAxis += (excessMainAxis / (info.Count - 1));
            }
            break;
        case winrt::FlexboxJustifyContent::SpaceAround:
        {
            float spaceSlice = (excessMainAxis / info.Count);
            if (usedInCurrentMainAxis == 0)
            {
                usedInCurrentMainAxis = mainOffset = (spaceSlice * 0.5f);
                mainOffset = usedInCurrentMainAxis;
            }
            usedInCurrentMainAxis += spaceSlice;
            break;
        }
        case winrt::FlexboxJustifyContent::SpaceEvenly:
        {
            float spaceSlice = (excessMainAxis / (info.Count + 1));
            mainOffset += spaceSlice;
            usedInCurrentMainAxis += spaceSlice;
        }
        break;
        }

        float crossOffset = crossOffsetForCurrentRow;
        float excessCrossAxisInRow;
        // If there's only one row then the cross axis size is actually the final arrange size
        if (state->Rows.size() == 1)
        {
            excessCrossAxisInRow = CrossAxis(finalSize) - CrossAxis(childDesiredSize);
        }
        else
        {
            excessCrossAxisInRow = (info.CrossAxis - CrossAxis(childDesiredSize));
        }

        float totalCrossAxis = 0.0;
        for (auto& row : state->Rows)
        {
            totalCrossAxis += row.CrossAxis;
        }
        float excessCrossAxis = (CrossAxis(finalSize) - totalCrossAxis);

        switch (m_alignContent)
        {
        case winrt::FlexboxAlignContent::Start:
            break;
        case winrt::FlexboxAlignContent::Center:
            crossOffset += (excessCrossAxis * 0.5f);
            break;
        case winrt::FlexboxAlignContent::End:
            crossOffset += excessCrossAxis;
            break;
        case winrt::FlexboxAlignContent::Stretch:
        {
            float extra = (excessCrossAxis / state->Rows.size());
            usedInCurrentCrossAxis = (info.CrossAxis + extra);
            excessCrossAxisInRow = 0;
            childDesiredSize = CreateSize(MainAxis(childDesiredSize), usedInCurrentCrossAxis);
            break;
        }
        case winrt::FlexboxAlignContent::SpaceBetween:
            if (state->Rows.size() > 1)
            {
                float spaceBetween = (excessCrossAxis / (state->Rows.size() - 1));
                usedInCurrentCrossAxis = (info.CrossAxis + spaceBetween);
            }
            break;
        case winrt::FlexboxAlignContent::SpaceAround:
        {
            float spaceAround = (excessCrossAxis / state->Rows.size());
            crossOffset += (spaceAround * 0.5f);
            usedInCurrentCrossAxis = (info.CrossAxis + spaceAround);
        }
        break;
        }

        winrt::FlexboxAlignItems alignItems;

        // PORT_TODO
        switch (winrt::FlexboxAlignSelf::Auto)
        //switch (GetAlignSelf(child))
        {
        default:
        case winrt::FlexboxAlignSelf::Auto:
            alignItems = AlignItems();
            break;
        case winrt::FlexboxAlignSelf::Start:
            alignItems = winrt::FlexboxAlignItems::Start;
            break;
        case winrt::FlexboxAlignSelf::End:
            alignItems = winrt::FlexboxAlignItems::End;
            break;
        case winrt::FlexboxAlignSelf::Center:
            alignItems = winrt::FlexboxAlignItems::Center;
            break;
        case winrt::FlexboxAlignSelf::Stretch:
            alignItems = winrt::FlexboxAlignItems::Stretch;
            break;
        }

        switch (alignItems)
        {
        case winrt::FlexboxAlignItems::Start:
            break;
        case winrt::FlexboxAlignItems::Center:
            crossOffset += (excessCrossAxisInRow * 0.5f);
            break;
        case winrt::FlexboxAlignItems::End:
            crossOffset += excessCrossAxisInRow;
            break;
        case winrt::FlexboxAlignItems::Stretch:
            childDesiredSize = CreateSize(MainAxis(childDesiredSize), (usedInCurrentCrossAxis > 0 ? usedInCurrentCrossAxis : info.CrossAxis));
            break;
        }

        // In Reversed mode we need to swap the coordinates so that items grom from right/bottom to left/top
        // TODO: Using finalSize here means things will become right/bottom aligned, which doesn't seem right
        if (IsReversed())
        {
            mainOffset = MainAxis(finalSize) - mainOffset - MainAxis(childDesiredSize);
        }

        child.Arrange(winrt::Rect(CreatePoint(mainOffset, crossOffset), childDesiredSize));

        usedInCurrentMainAxis += MainAxis(childDesiredSize);
    }
    return finalSize;
}

void FlexboxLayout::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
}
