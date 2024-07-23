// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Represents a wrapping strategy for new style virtualizing panels.

#include "precomp.h"
#include "WrappingLayoutStrategyImpl.h"
#include "DoubleUtil.h"
#include "RectUtil.h"
#include "NavigationIndices.h"

// Work around disruptive max/min macros
#undef max
#undef min

namespace DirectUI { namespace Components { namespace Moco {

WrappingLayoutStrategyImpl::WrappingLayoutStrategyImpl()
    : LayoutStrategyBase(true /* useFullWidthHeaders */, true /* isWrapping */)
    , m_cellSizeSet(false)
    , m_headerSizeSet(false)
    , m_maxRowsOrColumns(-1)
    , m_itemWidthFromPanel(std::numeric_limits<double>::quiet_NaN())
    , m_itemHeightFromPanel(std::numeric_limits<double>::quiet_NaN())
    , m_cachedStackingLines(0)
    , m_cachedCellSize()
    , m_cellSize({1})
    , m_headerSize({1})
{
}

#pragma region Layout related methods

// returns the size we should use to measure a container or header with
// itemIndex - indicates an index of valid item or -1 for general, non-special items
wf::Size
WrappingLayoutStrategyImpl::GetElementMeasureSize(
    _In_ xaml_controls::ElementType elementType,
    _In_ int elementIndex,
    _In_ wf::Rect windowConstraint)
{
    wf::Size result = { std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity() };
    result.*SizeInNonVirtualizingDirection() = windowConstraint.*SizeFromRectInNonVirtualizingDirection();

    // account for group padding in measure size, reduce the size in non virtualizing direction by the amount of group padding
    const float groupPaddingInNonVirtualizingDirection = GetGroupPaddingAtStart().*SizeInNonVirtualizingDirection() + GetGroupPaddingAtEnd().*SizeInNonVirtualizingDirection();
    const float measureSizeInNonVirtualizingDirection = result.*SizeInNonVirtualizingDirection() - groupPaddingInNonVirtualizingDirection;
    result.*SizeInNonVirtualizingDirection() = std::max(0.0f, measureSizeInNonVirtualizingDirection);

    if (elementType == xaml_controls::ElementType_ItemContainer)
    {
        if (IsItemWidthPropertySet())
        {
            result.Width = static_cast<float>(m_itemWidthFromPanel);
        }
        if (IsItemHeightPropertySet())
        {
            result.Height = static_cast<float>(m_itemHeightFromPanel);
        }

        if (m_cellSizeSet && elementIndex != c_specialItemIndex)
        {
            result = m_cellSize;
        }
    }
    // the special groupindex should get the available size
    // we only limit if we have a headersize set
    // we only need to limit in parallel layouts: inline scenarios need to have a different logic
    else if (
        GetGroupHeaderStrategy() == GroupHeaderStrategy::Parallel &&
        elementIndex != c_specialGroupIndex &&
        m_headerSizeSet)
    {
        result.*SizeInNonVirtualizingDirection() = m_headerSize.*SizeInNonVirtualizingDirection();
    }

    return result;
}

_Check_return_ HRESULT
WrappingLayoutStrategyImpl::GetElementBounds(
    _In_ xaml_controls::ElementType elementType,
    _In_ int elementIndex,
    _In_ wf::Size containerDesiredSize,
    _In_ xaml_controls::LayoutReference referenceInformation,
    _In_ wf::Rect windowConstraint,
    _Out_ wf::Rect* pReturnValue)

{
    if (elementType == xaml_controls::ElementType_ItemContainer)
    {
        int indexInGroup = elementIndex;
        if (IsGrouping())
        {
            IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetGroupInformationFromItemIndex(
                elementIndex,
                nullptr /* pIndexOfGroup */,
                &indexInGroup,
                nullptr /* pItemCountInGroup */));
        }
        *pReturnValue = GetContainerBounds(elementIndex, indexInGroup, containerDesiredSize, referenceInformation, windowConstraint);
    }
    else
    {
        *pReturnValue = GetHeaderBounds(elementIndex, containerDesiredSize, referenceInformation, windowConstraint);
    }
    return S_OK;
}

wf::Rect
WrappingLayoutStrategyImpl::GetElementArrangeBounds(
    _In_ xaml_controls::ElementType elementType,
    _In_ int elementIndex,
    _In_ wf::Rect containerBounds,
    _In_ wf::Rect windowConstraint,
    _In_ wf::Size finalSize)
{
    wf::Rect result = containerBounds;

    if (elementType == xaml_controls::ElementType_ItemContainer)
    {
        // By the time we arrange, we will have a cell size.
        ASSERT(m_cellSizeSet);

        result.Width = m_cellSize.Width;
        result.Height = m_cellSize.Height;
    }
    else
    {
        if (GetGroupHeaderStrategy() == GroupHeaderStrategy::Inline)
        {
            result.*SizeFromRectInNonVirtualizingDirection() = finalSize.*SizeInNonVirtualizingDirection();

            // account for group padding, reduce the size in non virtualizing direction by the amount of group padding
            const float groupPaddingInNonVirtualizingDirection = GetGroupPaddingAtStart().*SizeInNonVirtualizingDirection() + GetGroupPaddingAtEnd().*SizeInNonVirtualizingDirection();
            const float arrangeSizeInNonVirtualizingDirection = result.*SizeFromRectInNonVirtualizingDirection() - groupPaddingInNonVirtualizingDirection;
            result.*SizeFromRectInNonVirtualizingDirection() = std::max(0.0f, arrangeSizeInNonVirtualizingDirection);
        }
        else
        {
            // By the time we arrange, we will have a cell size.
            ASSERT(m_headerSizeSet);

            result.*SizeFromRectInNonVirtualizingDirection() = std::max(result.*SizeFromRectInNonVirtualizingDirection(), m_headerSize.*SizeInNonVirtualizingDirection());
        }
    }

    return result;
}

bool
WrappingLayoutStrategyImpl::ShouldContinueFillingUpSpace(
    _In_ xaml_controls::ElementType elementType,
    _In_ int elementIndex,
    _In_ xaml_controls::LayoutReference referenceInformation,
    _In_ wf::Rect windowToFill)
{
    bool shouldContinue = false;
    const bool requestingHeader = (elementType == xaml_controls::ElementType_GroupHeader);

    if (referenceInformation.RelativeLocation == xaml_controls::ReferenceIdentity_Myself)
    {
        // always do yourself
        shouldContinue = true;
    }
    else
    {
        if (requestingHeader)
        {
            // this holds true for either a header or container reference
            if (referenceInformation.RelativeLocation == xaml_controls::ReferenceIdentity_BeforeMe)
            {
                // we're a new header that is on the right of the reference. is there room  to the right?
                shouldContinue =
                    windowToFill.*PointFromRectInVirtualizingDirection() + windowToFill.*SizeFromRectInVirtualizingDirection()
                    > referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection() + referenceInformation.ReferenceBounds.*SizeFromRectInVirtualizingDirection();
            }
            else
            {
                // Due to the possibility of inline headers, the decision to start a new backwards group should be based on the header of
                // the current group, not some container inside it.
                ASSERT(referenceInformation.ReferenceIsHeader);

                // is there room to the left?
                shouldContinue = windowToFill.*PointFromRectInVirtualizingDirection() < referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection();
            }
        }
        else
        {
            int stackingLines = 0;
            int virtualizingLine = 0;
            int stackingLine = 0;

            DetermineLineInformation(windowToFill, elementIndex, &stackingLines, &virtualizingLine, &stackingLine);

            if (referenceInformation.RelativeLocation == xaml_controls::ReferenceIdentity_BeforeMe)
            {
                if (stackingLine == 0)
                {
                    if (referenceInformation.ReferenceIsHeader &&
                        GroupHeaderStrategy::Parallel == GetGroupHeaderStrategy())
                    {
                        // we're under a header, it makes sense that if that header was created, we should also be created
                        // since we are laid out under the header (todo: currently only supporting parallel)
                        shouldContinue =
                            windowToFill.*PointFromRectInVirtualizingDirection() + windowToFill.*SizeFromRectInVirtualizingDirection()
                            > referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection();
                    }
                    else
                    {
                        // we're in a new column, so is there room to the right?
                        shouldContinue =
                            windowToFill.*PointFromRectInVirtualizingDirection() + windowToFill.*SizeFromRectInVirtualizingDirection()
                            > referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection() + referenceInformation.ReferenceBounds.*SizeFromRectInVirtualizingDirection();
                    }
                }
                else
                {
                    shouldContinue = windowToFill.*PointFromRectInVirtualizingDirection() + windowToFill.*SizeFromRectInVirtualizingDirection() > referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection();
                }
            }
            else
            {
                if (referenceInformation.ReferenceIsHeader)
                {
                    // we're a container in the last row of the group that is on the left of the referenced header
                    shouldContinue = windowToFill.*PointFromRectInVirtualizingDirection() <= referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection();
                }
                else if (stackingLine == stackingLines - 1)
                {
                    // we're a container that is at the end of a column
                    shouldContinue = windowToFill.*PointFromRectInVirtualizingDirection() < referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection();
                }
                else
                {
                    shouldContinue = windowToFill.*PointFromRectInVirtualizingDirection() < referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection() + m_cellSize.*SizeInVirtualizingDirection();
                }
            }
        }
    }

    return shouldContinue;
}

wf::Point
WrappingLayoutStrategyImpl::GetPositionOfFirstElement()
{
    const wf::Size initialPadding = GetGroupPaddingAtStart();
    return{ initialPadding.Width, initialPadding.Height };
}

#pragma endregion

#pragma region Estimation and virtualization related methods.

_Check_return_ HRESULT
WrappingLayoutStrategyImpl::EstimateElementIndex(
    _In_ xaml_controls::ElementType elementType,
    _In_ xaml_controls::EstimationReference headerReference,
    _In_ xaml_controls::EstimationReference containerReference,
    _In_ wf::Rect window,
    _Out_ wf::Rect* pTargetRect,
    _Out_ int& result)
{
    if (elementType == xaml_controls::ElementType_ItemContainer)
    {
        IFC_RETURN(EstimateItemIndexFromWindow(
            headerReference,
            containerReference,
            window,
            pTargetRect,
            result));
    }
    else
    {
        IFC_RETURN(EstimateGroupIndexFromWindow(
            headerReference,
            containerReference,
            window,
            pTargetRect,
            result));
    }

    return S_OK;
}

_Check_return_ HRESULT
WrappingLayoutStrategyImpl::EstimateElementBounds(
    _In_ xaml_controls::ElementType elementType,
    _In_ int elementIndex,
    _In_ xaml_controls::EstimationReference headerReference,
    _In_ xaml_controls::EstimationReference containerReference,
    _In_ wf::Rect window,
    _Out_ wf::Rect* pReturnValue)
{
    wf::Rect result;

    if (elementType == xaml_controls::ElementType_ItemContainer)
    {
        IFC_RETURN(EstimateContainerLocation(
            elementIndex,
            headerReference,
            containerReference,
            window,
            result));
    }
    else
    {
        IFC_RETURN(EstimateHeaderLocation(
            elementIndex,
            headerReference,
            containerReference,
            window,
            result));
    }

    *pReturnValue = result;
    return S_OK;
}

_Check_return_ HRESULT
WrappingLayoutStrategyImpl::EstimatePanelExtent(
    _In_ xaml_controls::EstimationReference lastHeaderReference,
    _In_ xaml_controls::EstimationReference lastContainerReference,
    _In_ wf::Rect windowConstraint,
    _Out_ wf::Size* pExtent)
{
    wf::Size result;

    if (IsGrouping())
    {
        IFC_RETURN(EstimateGroupedExtent(
            lastHeaderReference,
            lastContainerReference,
            windowConstraint,
            result));
    }
    else
    {
        IFC_RETURN(EstimateNonGroupedExtent(
            lastContainerReference,
            windowConstraint,
            result));
    }

    *pExtent = result;
    return S_OK;
}

#pragma endregion

#pragma region IItemLookupPanel related

// Estimates the index or the insertion index closest to the given point.
_Check_return_ HRESULT WrappingLayoutStrategyImpl::EstimateIndexFromPoint(
    _In_ bool requestingInsertionIndex,
    _In_ wf::Point point,
    _In_ xaml_controls::EstimationReference reference,
    _In_ wf::Rect windowConstraint,
    _Out_ xaml_controls::IndexSearchHint* pSearchHint,
    _Out_ xaml_controls::ElementType* pElementType,
    _Out_ INT* pElementIndex)

{
    ASSERT(m_cellSizeSet);

    *pSearchHint = xaml_controls::IndexSearchHint_NoHint;
    *pElementType = xaml_controls::ElementType_ItemContainer;

    const int stackingLines = DetermineMaxStackingLine(windowConstraint);
    ASSERT(stackingLines > 0);

    int totalItems;
    int referenceItemIndex;
    const wf::Rect referenceItemRect = reference.ElementBounds;

    // When grouping, we convert the indices to be relative to the group.
    if (IsGrouping())
    {
        IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetGroupInformationFromItemIndex(
            reference.ElementIndex,
            nullptr /* pIndexOfGroup */,
            &referenceItemIndex,
            &totalItems));
    }
    else
    {
        referenceItemIndex = reference.ElementIndex;
        IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalItemCount(&totalItems));
    }

    // Calculate the row and column of the given point using the reference container as a starting point.
    int virtualizingDirectionDiff = static_cast<INT>((point.*PointInVirtualizingDirection() - referenceItemRect.*PointFromRectInVirtualizingDirection()) / m_cellSize.*SizeInVirtualizingDirection());
    int stackingDirectionDiff = static_cast<INT>((point.*PointInNonVirtualizingDirection() - referenceItemRect.*PointFromRectInNonVirtualizingDirection()) / m_cellSize.*SizeInNonVirtualizingDirection());

    // lock stacking direction to staking line size.
    stackingDirectionDiff = std::min(std::max(stackingDirectionDiff, -(referenceItemIndex % stackingLines)), (stackingLines - 1) - (referenceItemIndex % stackingLines));

    // if we drag off the panel boundary we should correct virtualizingDirectionDiff to be the first or last line.
    if (referenceItemIndex + virtualizingDirectionDiff * stackingLines < 0)
    {
        virtualizingDirectionDiff = -1 * referenceItemIndex / stackingLines;
    }
    else if (referenceItemIndex - (referenceItemIndex % stackingLines) + virtualizingDirectionDiff * stackingLines > totalItems - 1)
    {
        virtualizingDirectionDiff = (totalItems - 1 - (referenceItemIndex - (referenceItemIndex % stackingLines))) / stackingLines;
    }

    int pointIndex = referenceItemIndex + (virtualizingDirectionDiff * stackingLines + stackingDirectionDiff);
    if (pointIndex > totalItems)
    {
        // try previous line.
        int upperLinePointIndex = referenceItemIndex + ((virtualizingDirectionDiff - 1) * stackingLines + stackingDirectionDiff);

        // if we have upper line, then use it
        if (upperLinePointIndex > 0)
        {
            pointIndex = upperLinePointIndex;
        }
    }

    *pElementIndex = std::min(totalItems, pointIndex);
    *pSearchHint = xaml_controls::IndexSearchHint_Exact;

    if (requestingInsertionIndex)
    {
        int unusedStackingLines = 0;
        int targetStackingLine = 0;
        int targetVirtualizingLine = 0;

        DetermineLineInformation(windowConstraint, *pElementIndex, &unusedStackingLines, &targetVirtualizingLine, &targetStackingLine);
        const float calculatedPosition = GetItemStackingPosition(targetStackingLine);

        // If we're below the midpoint of the container under the point, we actually want to insert at the next index.
        if ((point.*PointInNonVirtualizingDirection() - calculatedPosition) >= m_cellSize.*SizeInNonVirtualizingDirection() / 2.0f)
        {
            ++*pElementIndex;
        }
    }

    // When grouping, we convert to convert back indices to be relative to the source.
    if (IsGrouping())
    {
        const int startItemIndex = reference.ElementIndex - referenceItemIndex;
        *pElementIndex += startItemIndex;
    }

    return S_OK;
}

// Based on current element's index/type and action, return the next element index/type.
 _Check_return_ HRESULT WrappingLayoutStrategyImpl::GetTargetIndexFromNavigationAction(
     _In_ xaml_controls::ElementType elementType,
     _In_ int elementIndex,
     _In_ xaml_controls::KeyNavigationAction action,
     _In_ wf::Rect windowConstraint,
     _In_ int itemIndexHintForHeaderNavigation,
     _Out_ xaml_controls::ElementType* pTargetElementType,
     _Out_ INT* pTargetElementIndex)
{
    // If the element type is GroupHeader, then make sure that we're actually
    // grouping.
    ASSERT(elementType == xaml_controls::ElementType_ItemContainer || IsGrouping());

    int totalItems = 0;
    int totalGroups = 0;

    int targetIndex = elementIndex;
    auto targetType = elementType;

    const int maxLineLength = DetermineMaxStackingLine(windowConstraint);

    IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalItemCount(&totalItems));
    IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalGroupCount(&totalGroups));

    ASSERT(0 <= elementIndex && elementIndex < (elementType == xaml_controls::ElementType_ItemContainer ? totalItems : totalGroups));

    if (action != xaml_controls::KeyNavigationAction_Left &&
        action != xaml_controls::KeyNavigationAction_Right &&
        action != xaml_controls::KeyNavigationAction_Up &&
        action != xaml_controls::KeyNavigationAction_Down)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    int step = (action == xaml_controls::KeyNavigationAction_Left || action == xaml_controls::KeyNavigationAction_Up) ? -1 : 1;

    if (elementType == xaml_controls::ElementType_ItemContainer)
    {
        // Easy case: the action is along layout orientation, therefore handle it.
        if ((GetVirtualizationDirection() == xaml_controls::Orientation_Vertical && (action == xaml_controls::KeyNavigationAction_Left || action == xaml_controls::KeyNavigationAction_Right)) ||
            (GetVirtualizationDirection() == xaml_controls::Orientation_Horizontal && (action == xaml_controls::KeyNavigationAction_Up || action == xaml_controls::KeyNavigationAction_Down)))
        {
            targetIndex = std::min(std::max(elementIndex + step, 0), totalItems - 1);
        }
        // The action is not in layout direction.  We must jump to the next item in the same row/column.
        // Situation 1 : we are not grouping, easy.
        else if (!IsGrouping())
        {
            const int nextIndex = elementIndex + step * maxLineLength;
            if (0 <= nextIndex && nextIndex < totalItems)
            {
                targetIndex = nextIndex;
            }
        }
        // Situation 2 : we are grouping.
        else
        {
             IFC_RETURN(GetTargetItemIndexInNextLineWithGrouping(elementIndex, step, maxLineLength, targetIndex));
        }
    }
    else
    {
        ASSERT(IsGrouping());
        targetIndex = std::min(std::max(elementIndex + step, 0), totalGroups - 1);

        // In-order navigating out of a group header should not use the item index hint since
        // we're not trying to maintain navigation across a row or down a column.
        if ((GetVirtualizationDirection() == xaml_controls::Orientation_Vertical && (action == xaml_controls::KeyNavigationAction_Left || action == xaml_controls::KeyNavigationAction_Right)) ||
            (GetVirtualizationDirection() == xaml_controls::Orientation_Horizontal && (action == xaml_controls::KeyNavigationAction_Up || action == xaml_controls::KeyNavigationAction_Down)))
        {
            itemIndexHintForHeaderNavigation = -1;
        }
    }

    // Evaluate whether we need to change the element type & index.
    if (IsGrouping())
    {
        if (elementType == xaml_controls::ElementType_ItemContainer)
        {
            int targetGroupHeaderIndex;
            IFC_RETURN(TryGetTargetHeaderIndexWithItemNavigation(elementIndex, targetIndex, step, maxLineLength, targetGroupHeaderIndex));
            if (targetGroupHeaderIndex != -1)
            {
                targetIndex = targetGroupHeaderIndex;
                targetType = xaml_controls::ElementType_GroupHeader;
            }
        }
        else
        {
            int targetItemIndex = -1;
            IFC_RETURN(TryGetTargetItemIndexWithHeaderNavigation(elementIndex, targetIndex, step, maxLineLength, itemIndexHintForHeaderNavigation, targetItemIndex));
            if (targetItemIndex != -1)
            {
                targetIndex = targetItemIndex;
                targetType = xaml_controls::ElementType_ItemContainer;
            }
        }
    }

    ASSERT(0 <= targetIndex && targetIndex < (targetType == xaml_controls::ElementType_ItemContainer ? totalItems : totalGroups));
    *pTargetElementIndex = targetIndex;
    *pTargetElementType = targetType;

    return S_OK;
}

_Check_return_ HRESULT WrappingLayoutStrategyImpl::GetTargetItemIndexInNextLineWithGrouping(
    int currItemIndex,
    int step,
    int maxLineLength,
    _Out_ int& targetIndex)
{
    ASSERT(IsGrouping());

    targetIndex = currItemIndex;

    int groupIndex = 0;
    int indexInGroup = 0;
    int itemsCountInGroup = 0;

    int totalGroups = 0;
    IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalGroupCount(&totalGroups));
    IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetGroupInformationFromItemIndex(
        currItemIndex,
        &groupIndex,
        &indexInGroup,
        &itemsCountInGroup));

    int targetIndexInGroup = indexInGroup + maxLineLength * step;

    ASSERT(step == 1 || step == -1);

    // Next index is within a line in this group.
    if (0 <= targetIndexInGroup && static_cast<int>(targetIndexInGroup / maxLineLength) <= static_cast<int>((itemsCountInGroup - 1) / maxLineLength))
    {
        const int startItemIndex = currItemIndex - indexInGroup;
        targetIndexInGroup = std::min(targetIndexInGroup, itemsCountInGroup - 1);
        targetIndex = targetIndexInGroup + startItemIndex;
    }
    else
    {
        int targetElementRowOrCoumn = indexInGroup % maxLineLength;

        // Finds a non-empty group.
        while (0 <= (groupIndex + step) && (groupIndex + step) < totalGroups)
        {
            groupIndex += step;
            int startItemIndex;

            IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetGroupInformationFromGroupIndex(
                groupIndex,
                &startItemIndex,
                &itemsCountInGroup));

            if (itemsCountInGroup > 0)
            {
                targetIndexInGroup = std::min(
                    itemsCountInGroup - 1,
                    targetElementRowOrCoumn - (step == 1 ? 0 : (itemsCountInGroup % maxLineLength) - itemsCountInGroup));

                targetIndex = targetIndexInGroup + startItemIndex;
                break;
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT WrappingLayoutStrategyImpl::TryGetTargetHeaderIndexWithItemNavigation(
    int currentItemIndex,
    int targetItemIndex,
    int step,
    int maxLineLength,
    _Out_ int& targetGroupHeaderIndex)
{
    ASSERT(IsGrouping());

    targetGroupHeaderIndex = -1;

    if (step != 0)
    {
        int currIndexOfGroup = 0;
        int currIndexInsideOfGroup = 0;
        int currItemCountInGroup = 0;
        IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetGroupInformationFromItemIndex(currentItemIndex, &currIndexOfGroup, &currIndexInsideOfGroup, &currItemCountInGroup));

        int targetIndexOfGroup = 0;
        int targetIndexInsideOfGroup = 0;
        int targetItemCountInGroup = 0;
        IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetGroupInformationFromItemIndex(targetItemIndex, &targetIndexOfGroup, &targetIndexInsideOfGroup, &targetItemCountInGroup));

        if (currentItemIndex == targetItemIndex)
        {
            int lineInGroup = static_cast<int>(currIndexInsideOfGroup / maxLineLength);
            int numLinesInGroup = static_cast<int>(std::ceil(static_cast<float>(currItemCountInGroup) / maxLineLength));

            // We have at least 1 item in this group, so the number of lines should be greater than 0.
            ASSERT(numLinesInGroup > 0);

            const bool isInFirstLineInGroup = (lineInGroup == 0);
            const bool isInFirstGroupWithItems = (currentItemIndex == currIndexInsideOfGroup);

            // Handle the special-case of going up/back from the first line in the first group
            // with items.
            if (isInFirstLineInGroup && isInFirstGroupWithItems && step < 0)
            {
                targetGroupHeaderIndex = currIndexOfGroup;
            }
            else
            {
                int totalGroups = 0;
                IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalGroupCount(&totalGroups));

                int totalItems = 0;
                IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalItemCount(&totalItems));

                // Handle the special-case of going down from the last line in the last group with items.
                const bool isInLastLineInGroup = (lineInGroup == (numLinesInGroup - 1));
                const bool isInLastGroupWithItems = (currentItemIndex == (totalItems - currItemCountInGroup + currIndexInsideOfGroup));

                if (isInLastLineInGroup && isInLastGroupWithItems && step > 0 && currIndexOfGroup < (totalGroups - 1))
                {
                    targetGroupHeaderIndex = currIndexOfGroup + 1;
                }
            }
        }
        else
        {
            // Evaluate whether the new target item index would cross a group boundary
            // when compared to the previous item index.
            if (currIndexOfGroup != targetIndexOfGroup)
            {
                // If advancing forward, don't just set the new target group index to targetIndexOfGroup,
                // instead just increment the value of currIndexOfGroup.  This is so we don't skip
                // group headers that have no items.
                targetGroupHeaderIndex = (step < 0 ? currIndexOfGroup : currIndexOfGroup + step);
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
WrappingLayoutStrategyImpl::TryGetTargetItemIndexWithHeaderNavigation(
    int currentGroupIndex,
    int targetGroupIndex,
    int step,
    int maxLineLength,
    int itemIndexHintForHeaderNavigation,
    _Out_ int& targetItemIndex)
{
    ASSERT(IsGrouping());

    targetItemIndex = -1;

    if (step != 0)
    {
        int totalItems = 0;
        IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalItemCount(&totalItems));

        int currGroupStartItemIndex = 0;
        int currGroupItemCountInGroup = 0;
        IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetGroupInformationFromGroupIndex(currentGroupIndex, &currGroupStartItemIndex, &currGroupItemCountInGroup));

        const bool isInLastGroupWithItems = currGroupItemCountInGroup > 0 && (totalItems == (currGroupStartItemIndex + currGroupItemCountInGroup));

        if (currentGroupIndex == targetGroupIndex && isInLastGroupWithItems && step > 0)
        {
            // Handle the special-case of going down/forward from the group header for the last group
            // with items.

            if ((itemIndexHintForHeaderNavigation >= currGroupStartItemIndex) &&
                (itemIndexHintForHeaderNavigation < (currGroupStartItemIndex + currGroupItemCountInGroup)))
            {
                // If we're going back into the group that contains our hint item index,
                // then set it as the target index.
                targetItemIndex = itemIndexHintForHeaderNavigation;
            }
            else
            {
                if (itemIndexHintForHeaderNavigation != -1)
                {
                    IFC_RETURN(GetTargetItemIndexInNextLineWithGrouping(itemIndexHintForHeaderNavigation, step, maxLineLength, targetItemIndex));
                }
                else
                {
                    targetItemIndex = currGroupStartItemIndex;
                }
            }
        }
        else
        {
            int targetGroupStartItemIndex = 0;
            int targetGroupItemCountInGroup = 0;
            IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetGroupInformationFromGroupIndex(targetGroupIndex, &targetGroupStartItemIndex, &targetGroupItemCountInGroup));

            // If the new target index would jump over any items, then determine a new
            // target item index.  This is where we use itemIndexHintForHeaderNavigation that was
            // cached after a group header got focus.  It allows us to preserve navigation
            // down a column or across a row, depending on the orientation.
            if (targetGroupIndex < currentGroupIndex && targetGroupItemCountInGroup > 0)
            {
                // Move backwards to a new group.
                if ((itemIndexHintForHeaderNavigation >= targetGroupStartItemIndex) &&
                    (itemIndexHintForHeaderNavigation < (targetGroupStartItemIndex + targetGroupItemCountInGroup)))
                {
                    // If we're going back into the group that contains our hint item index,
                    // then set it as the target index.
                    targetItemIndex = itemIndexHintForHeaderNavigation;
                }
                else
                {
                    // If we have a item index hint, then find the item in the next line using that as a reference,
                    // otherwise focus the last item in the new group.
                    if(itemIndexHintForHeaderNavigation != -1 )
                    {
                        IFC_RETURN(GetTargetItemIndexInNextLineWithGrouping(itemIndexHintForHeaderNavigation, step, maxLineLength, targetItemIndex));
                    }
                    else
                    {

                        targetItemIndex = targetGroupStartItemIndex + targetGroupItemCountInGroup - 1;
                    }
                }
            }
            else if (targetGroupIndex > currentGroupIndex && currGroupItemCountInGroup > 0)
            {
                // Move forwards to a new group.
                if ((itemIndexHintForHeaderNavigation >= currGroupStartItemIndex) &&
                    (itemIndexHintForHeaderNavigation < (currGroupStartItemIndex + currGroupItemCountInGroup)))
                {
                    // If we're going back into the group that contains our hint item index,
                    // then set it as the target index.
                    targetItemIndex = itemIndexHintForHeaderNavigation;
                }
                else
                {
                    // If we have a item index hint, then find the item in the next line using that as a reference,
                    // otherwise focus the first item in the current group.
                    if (itemIndexHintForHeaderNavigation != -1)
                    {
                        IFC_RETURN(GetTargetItemIndexInNextLineWithGrouping(itemIndexHintForHeaderNavigation, step, maxLineLength, targetItemIndex));
                    }
                    else
                    {
                        targetItemIndex = currGroupStartItemIndex;
                    }
                }
            }
        }
    }

    return S_OK;
}

// Determines whether or not the given item index
// is a layout boundary.
_Check_return_ HRESULT WrappingLayoutStrategyImpl::IsIndexLayoutBoundary(
    _In_ xaml_controls::ElementType elementType,
    _In_ int elementIndex,
    _In_ wf::Rect windowConstraint,
    _Out_ bool* pIsLeftBoundary,
    _Out_ bool* pIsTopBoundary,
    _Out_ bool* pIsRightBoundary,
    _Out_ bool* pIsBottomBoundary)
{
    // We only support non-grouped scenarios here, since we never use this method in a grouped scenario.
    ASSERT(elementType == xaml_controls::ElementType_ItemContainer);

    // Is the stacking direction horizontal?
    const bool isHorizontal = GetVirtualizationDirection() == xaml_controls::Orientation_Vertical;
    const int maxStackingLine = DetermineMaxStackingLine(windowConstraint);

    int totalItems;
    IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalItemCount(&totalItems));

    if (isHorizontal)
    {
        *pIsLeftBoundary = (elementIndex % maxStackingLine == 0);
        *pIsBottomBoundary = (elementIndex >= static_cast<INT>(totalItems - maxStackingLine));
        *pIsTopBoundary = (elementIndex < static_cast<INT>(maxStackingLine));
        *pIsRightBoundary = ((elementIndex + 1) % maxStackingLine == 0) || (elementIndex + 1 == totalItems);
    }
    else
    {
        *pIsLeftBoundary = (elementIndex < static_cast<INT>(maxStackingLine));
        *pIsBottomBoundary = ((elementIndex + 1) % maxStackingLine == 0) || (elementIndex + 1 == totalItems);
        *pIsTopBoundary = (elementIndex % maxStackingLine == 0);
        *pIsRightBoundary = (elementIndex >= static_cast<INT>(totalItems - maxStackingLine));
    }

    return S_OK;
}

#pragma endregion

#pragma region Snap points related

bool WrappingLayoutStrategyImpl::GetRegularSnapPoints(
    _Out_ float* pNearOffset,
    _Out_ float* pFarOffset,
    _Out_ float* pSpacing)
{
    bool hasRegularSnapPoints = false;
    *pNearOffset = 0.0f;
    *pFarOffset = 0.0f;
    *pSpacing = 0.0f;

    if (!IsGrouping())
    {
        hasRegularSnapPoints = true;
        *pNearOffset = GetGroupPaddingAtStart().*SizeInVirtualizingDirection();
        *pFarOffset = GetGroupPaddingAtEnd().*SizeInVirtualizingDirection();

        if (m_cellSizeSet)
        {
            *pSpacing = m_cellSize.*SizeInVirtualizingDirection();
        }
        else
        {
            *pSpacing = 0.0f;
        }
    }

    return hasRegularSnapPoints;
}

bool WrappingLayoutStrategyImpl::HasIrregularSnapPoints(
    _In_ xaml_controls::ElementType elementType)
{
    // Non-grouped IWG always reports regular snap points.
    // Grouped IWG always snaps to headers and containers.
    return IsGrouping();
}

#pragma endregion

_Check_return_ HRESULT
WrappingLayoutStrategyImpl::GetElementTransitionsBounds(
    _In_ xaml_controls::ElementType elementType,
    _In_ int elementIndex,
    _In_ wf::Rect windowConstraint,
    _Out_ wf::Rect* pReturnValue)
{
    // Required sizes set
    ASSERT(m_cellSizeSet && (!IsGrouping() || m_headerSizeSet));

    *pReturnValue = {};
    int itemCount;

    if (elementType == xaml_controls::ElementType_ItemContainer)
    {
        IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalItemCount(&itemCount));
    }
    else
    {
        IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetGroupInformationFromItemIndex(
            elementIndex,
            nullptr /* pIndexOfGroup */,
            nullptr /* pIndexInGroup */,
            &itemCount));
    }

    int stackingLines = std::min(DetermineMaxStackingLine(windowConstraint), itemCount);

    pReturnValue->*PointFromRectInNonVirtualizingDirection() = GetItemStackingPosition(0);
    pReturnValue->*SizeFromRectInNonVirtualizingDirection() = stackingLines * m_cellSize.*SizeInNonVirtualizingDirection();

    return S_OK;
}

double WrappingLayoutStrategyImpl::GetItemsPerPage(
    _In_ wf::Rect window) const
{
    double itemsPerPage = 0.0;

    if (m_cellSizeSet &&
        !DoubleUtil::IsNaN(m_cellSize.Width) &&
        !DoubleUtil::IsNaN(m_cellSize.Height) &&
        DoubleUtil::GreaterThan(m_cellSize.Width, 0) &&
        DoubleUtil::GreaterThan(m_cellSize.Height, 0))
    {
        int rows = static_cast<INT>(DoubleUtil::Ceil(window.Height / m_cellSize.Height));
        int cols = static_cast<INT>(DoubleUtil::Ceil(window.Width / m_cellSize.Width));
        itemsPerPage = rows * cols;
    }

    return itemsPerPage;
}

void WrappingLayoutStrategyImpl::DetermineLineInformation(
    _In_ const wf::Rect &windowConstraint,
    _In_ int indexInGroup,
    _Out_ int* pStackingLines,
    _Out_ int* pVirtualizingLine,
    _Out_ int* pStackingLine)  const
{
    // a wrapgrid can deterministically calculate where the bounds are
    int stackingLines = DetermineMaxStackingLine(windowConstraint);
    int virtualizingLine = indexInGroup / stackingLines;
    int stackingLine = indexInGroup - virtualizingLine*stackingLines;

    *pStackingLines = stackingLines;
    *pVirtualizingLine = virtualizingLine;
    *pStackingLine = stackingLine;
}

int WrappingLayoutStrategyImpl::DetermineMaxStackingLine(_In_ const wf::Rect& windowConstraint) const
{
    ASSERT((!IsGrouping() || m_headerSizeSet) && m_cellSizeSet);

    float availableStackingExtent = windowConstraint.*SizeFromRectInNonVirtualizingDirection();

    // Account for the padding
    availableStackingExtent -= (GetGroupPaddingAtStart().*SizeInNonVirtualizingDirection() + GetGroupPaddingAtEnd().*SizeInNonVirtualizingDirection());

    if (GetGroupHeaderStrategy() == GroupHeaderStrategy::Parallel)
    {
        availableStackingExtent -= m_headerSize.*SizeInNonVirtualizingDirection();
    }

    availableStackingExtent = std::max(availableStackingExtent, 0.0f);

    // The available size may be infinity, so to avoid overflow issues, do this calculation
    // in floating-point, casting back to an integer on the way out.
    float maxStackingLineFloat = availableStackingExtent / m_cellSize.*SizeInNonVirtualizingDirection();
    maxStackingLineFloat = std::max(1.0f, maxStackingLineFloat);   // a minimum of 1 row, in case the window constraint was smaller than the cellsize
    if (m_maxRowsOrColumns > 0)
    {
        maxStackingLineFloat = std::min(maxStackingLineFloat, static_cast<float>(m_maxRowsOrColumns));
    }
    maxStackingLineFloat = std::min(50000.0f, maxStackingLineFloat); // hardcoded maximum. This has been discussed with the team

    return static_cast<int>(maxStackingLineFloat);
}

const float WrappingLayoutStrategyImpl::GetDistanceBetweenGroups() const
{
    // if we are laying out vertically, we are wrapping rows
    // and our groups will be horizontally aligned
    return GetGroupPaddingAtStart().*SizeInVirtualizingDirection() +
        GetGroupPaddingAtEnd().*SizeInVirtualizingDirection();
}

float WrappingLayoutStrategyImpl::GetVirtualizedGroupExtent(_In_ int itemsInGroup, _In_ int maxStackingLine, _In_ float headerExtent) const
{
    float result = GetVirtualizedExtentOfItems(itemsInGroup, maxStackingLine);

    switch (GetGroupHeaderStrategy())
    {
    case GroupHeaderStrategy::Parallel:
        // Constrain it by the average header size
        result = std::max(result, headerExtent);
        break;

    case GroupHeaderStrategy::Inline:
        // Add the average header size
        result += headerExtent;
        break;

    default:
        ASSERT(false);
        break;
    }

    return result + GetDistanceBetweenGroups();
}

float WrappingLayoutStrategyImpl::GetAverageHeaderExtent() const
{
    // TODO: Track an actual average. Just return the first one for now
    ASSERT(m_headerSizeSet);
    return m_headerSize.*SizeInVirtualizingDirection();
}

float WrappingLayoutStrategyImpl::GetVirtualizedExtentOfItems(_In_ int itemCount, _In_ int maxStackingLine) const
{
    ASSERT(m_cellSizeSet || itemCount == 0);
    ASSERT(maxStackingLine > 0);

    // Get virtualizing lines, rounding up the fractional ones
    float extent = 0;
    if (itemCount > 0)
    {
        int virtualizingLines = (itemCount + (maxStackingLine - 1)) / maxStackingLine;
        extent += m_cellSize.*SizeInVirtualizingDirection() * virtualizingLines;
    }

    return extent;
}

float WrappingLayoutStrategyImpl::GetItemStackingPosition(_In_ int stackingLine) const
{
    ASSERT(m_cellSizeSet);

    float result = stackingLine * m_cellSize.*SizeInNonVirtualizingDirection();
    result += GetGroupPaddingAtStart().*SizeInNonVirtualizingDirection();

    if (GetGroupHeaderStrategy() == GroupHeaderStrategy::Parallel)
    {
        ASSERT(m_headerSizeSet);
        result += m_headerSize.*SizeInNonVirtualizingDirection();
    }

    return result;
}

void WrappingLayoutStrategyImpl::SetItemWidth(_In_ double itemWidth)
{
    m_itemWidthFromPanel = itemWidth;
    m_cellSize.Width = static_cast<float>(itemWidth);

    // if we got a new itemWidth which is NAN,
    // make sure we re-measure the first item again next
    // time around. Caller needs to call InvalidateMeasure
    // to make sure this happens.
    m_cellSizeSet = IsItemSizePropertySet();
}

void WrappingLayoutStrategyImpl::SetItemHeight(_In_ double itemHeight)
{
    m_itemHeightFromPanel = itemHeight;
    m_cellSize.Height = static_cast<float>(itemHeight);

    // if we got a new itemHeight which is NAN,
    // make sure we re-measure the first item again next
    // time around. Caller needs to call InvalidateMeasure
    // to make sure this happens.
    m_cellSizeSet = IsItemSizePropertySet();
}

bool WrappingLayoutStrategyImpl::IsItemWidthPropertySet() const
{
    return !_isnan(m_itemWidthFromPanel) && m_itemWidthFromPanel > 0;
}

bool WrappingLayoutStrategyImpl::IsItemHeightPropertySet() const
{
    return !_isnan(m_itemHeightFromPanel) && m_itemHeightFromPanel > 0;
}

bool WrappingLayoutStrategyImpl::IsItemSizePropertySet() const
{
    return IsItemWidthPropertySet() && IsItemHeightPropertySet();
}

void WrappingLayoutStrategyImpl::InvalidateGroupCache()
{
    m_cachedGroupLocations.clear();
    m_cachedStackingLines = 0;
}

void WrappingLayoutStrategyImpl::RegisterSpecialContainerSize(_In_ int itemIndex, _In_ wf::Size containerDesiredSize)
{
    ASSERT(itemIndex == c_specialItemIndex);
    containerDesiredSize.Width = std::max(containerDesiredSize.Width, 1.0f);
    containerDesiredSize.Height = std::max(containerDesiredSize.Height, 1.0f);

    if (!IsItemWidthPropertySet())
    {
        m_cellSize.Width = containerDesiredSize.Width;
    }

    if (!IsItemHeightPropertySet())
    {
        m_cellSize.Height = containerDesiredSize.Height;
    }

    m_cellSizeSet = true;
}

void WrappingLayoutStrategyImpl::RegisterSpecialHeaderSize(_In_ int groupIndex, _In_ wf::Size headerDesiredSize)
{
    ASSERT(groupIndex == c_specialGroupIndex);
    headerDesiredSize.Width = std::max(headerDesiredSize.Width, 1.0f);
    headerDesiredSize.Height = std::max(headerDesiredSize.Height, 1.0f);
    m_headerSize = headerDesiredSize;
    m_headerSizeSet = true;
}

//
// Private
//

wf::Rect
WrappingLayoutStrategyImpl::GetContainerBounds(
    _In_ int indexInItems,
    _In_ int indexInGroup,
    _In_ wf::Size containerDesiredSize,
    _In_ xaml_controls::LayoutReference referenceInformation,
    _In_ wf::Rect windowConstraint)
{
    wf::Rect bounds = {};
    int stackingLines = 0;
    int stackingLine = 0;
    int virtualizingLine = 0;

    // Required sizes set
    ASSERT(m_cellSizeSet && (!IsGrouping() || m_headerSizeSet));

    // input
    float distanceBetweenGroups = GetDistanceBetweenGroups();
    DetermineLineInformation(windowConstraint, indexInGroup, &stackingLines, &virtualizingLine, &stackingLine);

    if (!IsGrouping())
    {
        // If nongrouping, this is exact and we don't even need the reference position
        bounds.*PointFromRectInVirtualizingDirection() = GetGroupPaddingAtStart().*SizeInVirtualizingDirection() +
            (virtualizingLine * m_cellSize.*SizeInVirtualizingDirection());
    }
    else
    {
        if (referenceInformation.ReferenceIsHeader)
        {
            // mismatch
            ASSERT(referenceInformation.RelativeLocation != xaml_controls::ReferenceIdentity_Myself);

            if (referenceInformation.RelativeLocation == xaml_controls::ReferenceIdentity_BeforeMe)
            {
                ASSERT(indexInGroup == 0); // if not, this is an invalid calculation

                // parallel layout: so take the left edge of the header and the bottom to start off
                bounds.*PointFromRectInVirtualizingDirection() = referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection();
                if (GetGroupHeaderStrategy() == GroupHeaderStrategy::Inline)
                {
                    // If our headers are inline, let's make room for the size of the header as well
                    bounds.*PointFromRectInVirtualizingDirection() += referenceInformation.ReferenceBounds.*SizeFromRectInVirtualizingDirection();
                }
            }
            else
            {
                // I'm going to be the last container in some new group that we will also be creating
                bounds.*PointFromRectInVirtualizingDirection() =
                    referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection() - m_cellSize.*SizeInVirtualizingDirection() - distanceBetweenGroups;
            }
        }
        else
        {
            // we got a sibling container
            if (referenceInformation.RelativeLocation == xaml_controls::ReferenceIdentity_BeforeMe
                && stackingLine == 0)
            {
                // new stacking line
                bounds.*PointFromRectInVirtualizingDirection() = referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection() + m_cellSize.*SizeInVirtualizingDirection();
            }
            else if (referenceInformation.RelativeLocation == xaml_controls::ReferenceIdentity_AfterMe
                && stackingLine == stackingLines - 1)
            {
                // previous stackingline
                bounds.*PointFromRectInVirtualizingDirection() = referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection() - m_cellSize.*SizeInVirtualizingDirection();
            }
            else
            {
                // same column
                bounds.*PointFromRectInVirtualizingDirection() = referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection();
            }
        }
    }

    bounds.*PointFromRectInNonVirtualizingDirection() = GetItemStackingPosition(stackingLine);
    bounds.Width = m_cellSize.Width;
    bounds.Height = m_cellSize.Height;

    return bounds;
}

wf::Rect
WrappingLayoutStrategyImpl::GetHeaderBounds(
    _In_ int groupIndex,
    _In_ wf::Size headerDesiredSize,
    _In_ xaml_controls::LayoutReference referenceInformation,
    _In_ wf::Rect windowConstraint)
{
    wf::Rect bounds = {};

    float distanceBetweenGroups = GetDistanceBetweenGroups();

    // TODO: Both the panel and the layout need to be aware that regardless of "special" status, header 0 and container 0 need to be placed in a specific location

    // the group padding logic:
    // When we are asked to give a total size, that should include the padding above and below us.
    // we will make sure to apply padding in between groups

    // Required sizes set
    ASSERT(IsGrouping() && m_headerSizeSet);

    if (referenceInformation.ReferenceIsHeader)
    {
        if (referenceInformation.RelativeLocation == xaml_controls::ReferenceIdentity_Myself)
        {
            // we did not have a correct location for this header. We will probably have estimated something
            bounds.*PointFromRectInVirtualizingDirection() = referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection();
        }
        else
        {
            bounds.*PointFromRectInVirtualizingDirection() = referenceInformation.RelativeLocation == xaml_controls::ReferenceIdentity_BeforeMe ?
                referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection() + referenceInformation.ReferenceBounds.*SizeFromRectInVirtualizingDirection() + distanceBetweenGroups
                : referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection() - headerDesiredSize.*SizeInVirtualizingDirection() - distanceBetweenGroups;
        }
    }
    else
    {
        ASSERT(referenceInformation.RelativeLocation != xaml_controls::ReferenceIdentity_Myself); // mismatch

        if (referenceInformation.RelativeLocation == xaml_controls::ReferenceIdentity_BeforeMe)
        {
            switch (GetGroupHeaderStrategy())
            {
            case GroupHeaderStrategy::Parallel:
                if (RectUtil::GetIsEmpty(referenceInformation.HeaderBounds))
                {
                    // If we are forward generating, and the reference rect belongs to a container, that container should always have an associated header
                    // with which the header bounds are populated
                    ASSERT(FALSE);
                    bounds.*PointFromRectInVirtualizingDirection() =
                        referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection() + referenceInformation.ReferenceBounds.*SizeFromRectInVirtualizingDirection();
                }
                else
                {
                    // looking for rightmost/bottommost location, based on either the last container or (if that's bigger) the header of that group
                    bounds.*PointFromRectInVirtualizingDirection() = std::max(
                        referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection() + referenceInformation.ReferenceBounds.*SizeFromRectInVirtualizingDirection(), // based on container location
                        referenceInformation.HeaderBounds.*PointFromRectInVirtualizingDirection() + referenceInformation.HeaderBounds.*SizeFromRectInVirtualizingDirection()); // based on header location
                }
                break;

            case GroupHeaderStrategy::Inline:
                // Easy, just go off the reference. No comparison of header and reference bounds needed.
                bounds.*PointFromRectInVirtualizingDirection() =
                    referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection() + referenceInformation.ReferenceBounds.*SizeFromRectInVirtualizingDirection();
                break;

            default:
                ASSERT(FALSE);
            }

            bounds.*PointFromRectInVirtualizingDirection() += distanceBetweenGroups;
        }
        else
        {
            switch (GetGroupHeaderStrategy())
            {
            case GroupHeaderStrategy::Parallel:
                // Snapping a header to the first item in its group. For parallel layout, it's left-aligned with the item
                if (RectUtil::GetIsEmpty(referenceInformation.HeaderBounds))
                {
                    bounds.*PointFromRectInVirtualizingDirection() = referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection();
                }
                else
                {
                    // Make sure we leave enough room for the header of the next group
                    bounds.*PointFromRectInVirtualizingDirection() = std::min(
                        referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection(),
                        referenceInformation.HeaderBounds.*PointFromRectInVirtualizingDirection() - headerDesiredSize.*SizeInVirtualizingDirection());
                }
                break;

            case GroupHeaderStrategy::Inline:
                // For an inline header, we need to move it over so that it comes before the container
                bounds.*PointFromRectInVirtualizingDirection() =
                    referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection() - headerDesiredSize.*SizeInVirtualizingDirection();
                break;

            default:
                ASSERT(FALSE);
            }
        }
    }

    bounds.*PointFromRectInNonVirtualizingDirection() = GetGroupPaddingAtStart().*SizeInNonVirtualizingDirection();

    bounds.*SizeFromRectInVirtualizingDirection() = headerDesiredSize.*SizeInVirtualizingDirection();
    bounds.*SizeFromRectInNonVirtualizingDirection() = headerDesiredSize.*SizeInNonVirtualizingDirection();    // not constrained

    return bounds;
}

// Estimate how many items we need to traverse to get from our reference point to a suitable anchor item for the window.
_Check_return_ HRESULT
WrappingLayoutStrategyImpl::EstimateItemIndexFromWindow(
    _In_ xaml_controls::EstimationReference headerReference,
    _In_ xaml_controls::EstimationReference containerReference,
    _In_ wf::Rect window,
    _Out_ wf::Rect* pTargetRect,
    _Out_ int& result)
{
    int totalItems;
    IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalItemCount(&totalItems));

    const int maxStackingLines = (totalItems > 0) ? DetermineMaxStackingLine(window) : 1;
    result = -1;

    if (!IsGrouping())
    {
        // If we are non-grouped, this is an exact calculation
        ASSERT(maxStackingLines > 0);
        ASSERT(m_cellSizeSet);

        // How many virtualizing lines does it take to get from the start to here?
        const float distanceFromStart = std::max(0.0f, window.*PointFromRectInVirtualizingDirection() - GetGroupPaddingAtStart().*SizeInVirtualizingDirection());
        int virtualizingLineDistance = static_cast<int>(distanceFromStart / m_cellSize.*SizeInVirtualizingDirection());

        // Calculate the index and clamp to valid range
        int targetIndex = virtualizingLineDistance * maxStackingLines;
        targetIndex = std::max(0, targetIndex);
        targetIndex = std::min(targetIndex, totalItems - 1);

        // With the final index, calculate its position
        int ignored;
        int virtualizingLine;
        int stackingLine;
        DetermineLineInformation(window, targetIndex, &ignored, &virtualizingLine, &stackingLine);

        result = targetIndex;
        pTargetRect->*PointFromRectInVirtualizingDirection() = virtualizingLine * m_cellSize.*SizeInVirtualizingDirection()
            + GetGroupPaddingAtStart().*SizeInVirtualizingDirection();
        pTargetRect->*PointFromRectInNonVirtualizingDirection() = GetItemStackingPosition(stackingLine);
    }
    else
    {
        // Determine which reference we will use
        // For now, if both are provided, we only use the header
        wf::Rect referenceRect;
        int referenceIndex;
        int referenceIndexInGroup; // Used to determine virtualizing and stacking line locations
        float headerAdjustment;
        bool referenceIsHeader;
        if (headerReference.ElementIndex == -1)
        {
            referenceIndex = containerReference.ElementIndex;
            referenceIndexInGroup = containerReference.ElementIndex;
            referenceRect = containerReference.ElementBounds;
            headerAdjustment = 0;
            referenceIsHeader = FALSE;
        }
        else
        {
            IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetGroupInformationFromGroupIndex(
                headerReference.ElementIndex,
                &referenceIndex,
                nullptr /* pItemCountInGroup */));
            referenceIndexInGroup = 0;
            referenceRect = headerReference.ElementBounds;
            float headerExtent = headerReference.ElementBounds.*SizeFromRectInVirtualizingDirection();
            if (headerExtent <= 0)
            {
                headerExtent = GetAverageHeaderExtent();
            }
            // If we have inline headers, take the size into account before we start counting items
            if (GetGroupHeaderStrategy() == GroupHeaderStrategy::Inline)
            {
                headerAdjustment = headerExtent;
            }
            else
            {
                headerAdjustment = 0;
            }
            referenceIsHeader = TRUE;
        }
        float virtualizedReferencePoint = referenceRect.*PointFromRectInVirtualizingDirection();
        float calculatedPosition = virtualizedReferencePoint;

        // Figure out which direction we're going
        RelativePosition relativeReferencePosition = GetReferenceDirectionFromWindow(referenceRect, window);

        int virtualizingLinesDelta = 0;
        ASSERT(m_cellSizeSet);
        const float itemExtent = std::max(1.0f, m_cellSize.*SizeInVirtualizingDirection());

        switch (relativeReferencePosition)
        {
        case RelativePosition::Before:
        {
            // Take into account a potential inline header
            virtualizedReferencePoint += headerAdjustment;

            // Gather information about the distance to traverse
            const float nearWindowEdge = window.*PointFromRectInVirtualizingDirection();
            const float distance = nearWindowEdge - virtualizedReferencePoint;
            if (distance > 0)
            {
                virtualizingLinesDelta = static_cast<int>(std::floor(distance / itemExtent));
            }
            else
            {
                virtualizingLinesDelta = 0;
            }
        }
            break;

        case RelativePosition::After:
        {
            // Gather information about the distance to traverse
            const float nearWindowEdge = window.*PointFromRectInVirtualizingDirection() + window.*SizeFromRectInVirtualizingDirection();
            const float distance = nearWindowEdge - virtualizedReferencePoint;
            if (distance < 0 && !referenceIsHeader)
            {
                virtualizingLinesDelta = static_cast<int>(std::floor(distance / itemExtent));
            }
            else
            {
                // If we end up estimating backwards from a header, just use the first item in the group
                virtualizingLinesDelta = 0;
            }
        }
            break;

        default:
            virtualizingLinesDelta = 0;
            break;
        }

        // Now that we know how many virtualizing lines to traverse, let's get an actual item delta
        int targetStackingLine = 0;
        if (virtualizingLinesDelta != 0)
        {
            // Figure out how many items our line delta takes us
            int targetItemIndex = referenceIndex + (maxStackingLines * virtualizingLinesDelta);

            // Let's just default to the first element in the target stack
            int referenceStackingLine;
            int referenceVirtualizingLine;
            int ignored;
            DetermineLineInformation(window, referenceIndexInGroup, &ignored, &referenceVirtualizingLine, &referenceStackingLine);
            targetItemIndex -= referenceStackingLine;

            // Bind the target item to the total item size
            targetItemIndex = std::max(0, targetItemIndex);
            targetItemIndex = std::min(targetItemIndex, totalItems - 1);

            // Now, we'll just make sure that in case we clipped the item index, we have the correct virtualizing offset
            int targetVirtualizingLine;
            int targetIndexInGroup = targetItemIndex - referenceIndex + referenceIndexInGroup;
            DetermineLineInformation(window, targetIndexInGroup, &ignored, &targetVirtualizingLine, &targetStackingLine);
            virtualizingLinesDelta = targetVirtualizingLine - referenceVirtualizingLine;
            calculatedPosition = virtualizedReferencePoint + (virtualizingLinesDelta * itemExtent);
            result = targetItemIndex;
        }
        else
        {
            // We'll just re-use the reference element
            int ignored;
            DetermineLineInformation(window, referenceIndexInGroup, &ignored, &ignored, &targetStackingLine);
            calculatedPosition = virtualizedReferencePoint;
            result = referenceIndex;
        }

        pTargetRect->*PointFromRectInVirtualizingDirection() = calculatedPosition;
        pTargetRect->*PointFromRectInNonVirtualizingDirection() = GetItemStackingPosition(targetStackingLine);
    }

    pTargetRect->Width = m_cellSize.Width;
    pTargetRect->Height = m_cellSize.Height;

    return S_OK;
}

// Estimate how many groups we need to traverse to get from our reference point to a suitable
// anchor group for the window, using items-per-group to estimate an average group extent.
_Check_return_ HRESULT
WrappingLayoutStrategyImpl::EstimateGroupIndexFromWindow(
    _In_ xaml_controls::EstimationReference headerReference,
    _In_ xaml_controls::EstimationReference containerReference,
    _In_ wf::Rect window,
    _Out_ wf::Rect* pTargetRect,
    _Out_ int& targetGroupIndex)
{
    int totalItems;
    int totalGroups;
    IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalItemCount(&totalItems));
    IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalGroupCount(&totalGroups));

    ASSERT(0 < totalGroups);

    const int maxStackingLines = (totalItems > 0) ? DetermineMaxStackingLine(window) : 1;

    const float averageHeaderExtent = GetAverageHeaderExtent();
    const float referenceHeaderSize = (headerReference.ElementBounds.*SizeFromRectInVirtualizingDirection() > 0) ?
        headerReference.ElementBounds.*SizeFromRectInVirtualizingDirection() : averageHeaderExtent;

    const float nearWindowEdge = window.*PointFromRectInVirtualizingDirection();
    const float farWindowEdge = nearWindowEdge + window.*SizeFromRectInVirtualizingDirection();
    ASSERT(nearWindowEdge <= farWindowEdge);

    float calculatedPosition = headerReference.ElementBounds.*PointFromRectInVirtualizingDirection();
    targetGroupIndex = headerReference.ElementIndex;

    if (calculatedPosition + referenceHeaderSize < nearWindowEdge)
    {
        bool found = false;
        // Our header is before of the window
        // Need to walk forward to reach it, making sure we step back if a large group overshoots the window entirely
        // To perform the backtrack in this case, we first calculate a "candidate position", accepting it only if we didn't overshoot
        int itemsInGroup;
        IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetGroupInformationFromGroupIndex(
            targetGroupIndex,
            nullptr /* pStartItemIndex */,
            &itemsInGroup));

        // Since we have this header's size and group count, let's just use it first before jumping off into averages
        float candidatePosition = calculatedPosition + GetVirtualizedGroupExtent(itemsInGroup, maxStackingLines, referenceHeaderSize);

        // Bail out if we encounter the last group, as we can't go farther than that.
        while (!found && targetGroupIndex + 1 < totalGroups)
        {
            if (farWindowEdge < candidatePosition)
            {
                // That group was big! We overshot the window, so let's not move to the next group and use the current one
                found = true;
            }
            else if (candidatePosition + averageHeaderExtent < nearWindowEdge)
            {
                // We're going to move to the next group and continue
                ++targetGroupIndex;
                calculatedPosition = candidatePosition;
                IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetGroupInformationFromGroupIndex(
                    targetGroupIndex,
                    nullptr /* pStartItemIndex */,
                    &itemsInGroup));
                candidatePosition = calculatedPosition + GetVirtualizedGroupExtent(itemsInGroup, maxStackingLines, averageHeaderExtent);
            }
            else
            {
                // Nailed it! This header looks like it will actually be in the window. Let's take it.
                ++targetGroupIndex;
                calculatedPosition = candidatePosition;
                found = true;
            }
        }
    }
    else if (farWindowEdge < calculatedPosition)
    {
        // Our header is currently after the window
        // Need to walk backward to reach it

        bool found = false;
        // If we hit group 0, we can't go any farther, so we might as well stop there
        while (!found && targetGroupIndex > 0)
        {
            --targetGroupIndex;
            int itemsInGroup;
            IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetGroupInformationFromGroupIndex(
                targetGroupIndex,
                nullptr /* pStartItemIndex */,
                &itemsInGroup));

            calculatedPosition -= GetVirtualizedGroupExtent(itemsInGroup, maxStackingLines, averageHeaderExtent);

            if (calculatedPosition <= farWindowEdge)
            {
                // Hooray! We've encountered the window. Let's stop here, now that we've traveled far enough
                found = true;
            }
        }
    }
    else
    {
        // This header is currently hitting the window. Nothing to do here!
    }

    wf::Rect targetRect;

    targetRect.*PointFromRectInVirtualizingDirection() = calculatedPosition;
    targetRect.*PointFromRectInNonVirtualizingDirection() = GetGroupPaddingAtStart().*SizeInNonVirtualizingDirection();
    if (targetGroupIndex == headerReference.ElementIndex)
    {
        targetRect.*SizeFromRectInVirtualizingDirection() = referenceHeaderSize;
    }
    else
    {
        targetRect.*SizeFromRectInVirtualizingDirection() = GetAverageHeaderExtent();
    }
    targetRect.*SizeFromRectInNonVirtualizingDirection() = m_headerSize.*SizeInNonVirtualizingDirection();
    *pTargetRect = targetRect;

    return S_OK;
}

// Estimate the location of an anchor item, given an item index
_Check_return_ HRESULT
WrappingLayoutStrategyImpl::EstimateContainerLocation(
    _In_ int targetItemIndex,
    _In_ xaml_controls::EstimationReference headerReference,
    _In_ xaml_controls::EstimationReference containerReference,
    _In_ wf::Rect window,
    _Out_ wf::Rect& result)
{
    int totalItems;
    IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalItemCount(&totalItems));

    ASSERT(0 <= targetItemIndex && targetItemIndex < totalItems);

    ASSERT(m_cellSizeSet);

    result = {};

    // Put some sanity bounds on the item index
    targetItemIndex = std::max(0, targetItemIndex);
    targetItemIndex = std::min(targetItemIndex, totalItems - 1);

    // If we are nongrouped, we can do an exact calculation here and short-circuit
    if (!IsGrouping())
    {
        int maxStackingLines;
        int virtualizingLine;
        int stackingLine;

        DetermineLineInformation(window, targetItemIndex, &maxStackingLines, &virtualizingLine, &stackingLine);
        ASSERT(maxStackingLines > 0);

        result.*PointFromRectInVirtualizingDirection() = virtualizingLine * m_cellSize.*SizeInVirtualizingDirection()
            + GetGroupPaddingAtStart().*SizeInVirtualizingDirection();

        result.*PointFromRectInNonVirtualizingDirection() = GetItemStackingPosition(stackingLine);
    }
    else
    {
        // Determine which reference we will use
        // For now, if both are provided, we only use the header
        wf::Rect referenceRect;
        int referenceIndex;
        int referenceIndexInGroup; // Used to determine virtualizing and stacking line locations
        float headerAdjustment;
        bool referenceIsHeader;
        if (headerReference.ElementIndex == -1)
        {
            // If using an item as our reference, the indexInGroup is the same as the item index
            referenceIndex = containerReference.ElementIndex;
            referenceIndexInGroup = referenceIndex;
            referenceRect = containerReference.ElementBounds;
            headerAdjustment = 0;
            referenceIsHeader = FALSE;
        }
        else
        {
            IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetGroupInformationFromGroupIndex(
                headerReference.ElementIndex,
                &referenceIndex,
                nullptr /* pItemCountInGroup */));
            referenceIndexInGroup = 0; // Treat the header as being item 0 in the group
            referenceRect = headerReference.ElementBounds;
            float headerExtent = headerReference.ElementBounds.*SizeFromRectInVirtualizingDirection();
            if (headerExtent <= 0)
            {
                headerExtent = GetAverageHeaderExtent();
            }
            // If we have inline headers, take the size into account before we start counting items
            if (GetGroupHeaderStrategy() == GroupHeaderStrategy::Inline && targetItemIndex >= referenceIndex)
            {
                headerAdjustment = headerExtent;
            }
            else
            {
                headerAdjustment = 0;
            }
            referenceIsHeader = TRUE;

            // If we attempt to estimate backwards from a header, just use the first item in the group
            targetItemIndex = std::max(targetItemIndex, referenceIndex);
        }
        const float virtualizedReferencePoint = referenceRect.*PointFromRectInVirtualizingDirection() + headerAdjustment;

        // Get our virtualizing/stacking line info for both the reference and the target index
        int ignored;
        int referenceVirtualizingLine;
        int targetVirtualizingLine;
        int targetStackingLine;
        const int targetIndexInGroup = targetItemIndex - referenceIndex + referenceIndexInGroup;
        DetermineLineInformation(window, referenceIndexInGroup, &ignored, &referenceVirtualizingLine, &ignored);
        DetermineLineInformation(window, targetIndexInGroup, &ignored, &targetVirtualizingLine, &targetStackingLine);
        int virtualizingLineDelta = targetVirtualizingLine - referenceVirtualizingLine;

        result.*PointFromRectInVirtualizingDirection() = virtualizedReferencePoint + (virtualizingLineDelta * m_cellSize.*SizeInVirtualizingDirection());
        result.*PointFromRectInNonVirtualizingDirection() = GetItemStackingPosition(targetStackingLine);
    }

    result.Width = m_cellSize.Width;
    result.Height = m_cellSize.Height;

    return S_OK;
}

// Estimate the location of an anchor group, using items-per-group to estimate an average group extent.
_Check_return_ HRESULT
WrappingLayoutStrategyImpl::EstimateHeaderLocation(
    _In_ int targetGroupIndex,
    _In_ xaml_controls::EstimationReference headerReference,
    _In_ xaml_controls::EstimationReference containerReference,
    _In_ wf::Rect window,
    _Out_ wf::Rect& targetRect)
{
    int totalItems;
    int totalGroups;
    IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalItemCount(&totalItems));
    IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalGroupCount(&totalGroups));

    ASSERT(0 < totalGroups);
    ASSERT(0 <= targetGroupIndex && targetGroupIndex < static_cast<INT>(totalGroups));

    const int maxStackingLines = (totalItems > 0) ? DetermineMaxStackingLine(window) : 1;
    const float referenceHeaderSize = (headerReference.ElementBounds.*SizeFromRectInVirtualizingDirection() > 0) ?
        headerReference.ElementBounds.*SizeFromRectInVirtualizingDirection() : GetAverageHeaderExtent();

    float calculatedPosition;
    // Let's handle the special case where we are estimating an adjacent header
    if (targetGroupIndex == headerReference.ElementIndex || targetGroupIndex == headerReference.ElementIndex + 1)
    {
        int firstItemInGroup;
        int itemCountInGroup;
        IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetGroupInformationFromGroupIndex(
            headerReference.ElementIndex,
            &firstItemInGroup,
            &itemCountInGroup));

        if (containerReference.ElementIndex != -1 && firstItemInGroup <= containerReference.ElementIndex && containerReference.ElementIndex < firstItemInGroup + itemCountInGroup)
        {
            // We're using this item to estimate the placement of an adjacent header
            const int itemIndexInGroup = containerReference.ElementIndex - firstItemInGroup;
            float itemReference = containerReference.ElementBounds.*PointFromRectInVirtualizingDirection();

            if (targetGroupIndex == headerReference.ElementIndex)
            {
                // We're looking for the header immediately before our item
                int virtualizingLinesDelta = -(itemIndexInGroup / maxStackingLines);
                calculatedPosition = itemReference + virtualizingLinesDelta * m_cellSize.*SizeInVirtualizingDirection();

                if (GetGroupHeaderStrategy() == GroupHeaderStrategy::Inline)
                {
                    // Make room for an inline header
                    calculatedPosition -= referenceHeaderSize;
                }
            }
            else
            {
                // We're looking for the header immediately after our item
                int virtualizingLinesDelta = ((itemCountInGroup - 1) / maxStackingLines) - (itemIndexInGroup / maxStackingLines);
                calculatedPosition = itemReference + virtualizingLinesDelta * m_cellSize.*SizeInVirtualizingDirection() + GetDistanceBetweenGroups();
            }
        }
        else
        {
            // We're using this header to estimate the placement of an adjacent header
            calculatedPosition = headerReference.ElementBounds.*PointFromRectInVirtualizingDirection();
            if (targetGroupIndex == headerReference.ElementIndex + 1)
            {
                calculatedPosition += GetVirtualizedGroupExtent(itemCountInGroup, maxStackingLines, referenceHeaderSize);
            }
        }
    }
    else
    {
        // Going to iterate through the group sizes and use them to come up with an estimate
        const float averageHeaderExtent = GetAverageHeaderExtent();
        const float virtualizedReferencePoint = headerReference.ElementBounds.*PointFromRectInVirtualizingDirection();
        calculatedPosition = virtualizedReferencePoint;

        // Put a sanity bound on the group index, for FRE builds
        targetGroupIndex = std::max(0, targetGroupIndex);
        targetGroupIndex = std::min(targetGroupIndex, static_cast<INT>(totalGroups - 1));

        int currentGroupIndex = headerReference.ElementIndex;

        // If we're going forward, we have this group header's actual size, so we may as well use it
        if (currentGroupIndex < targetGroupIndex)
        {
            int itemCountInGroup;
            IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetGroupInformationFromGroupIndex(
                headerReference.ElementIndex,
                nullptr /* pStartItemIndex */,
                &itemCountInGroup));

            calculatedPosition += GetVirtualizedGroupExtent(itemCountInGroup, maxStackingLines, referenceHeaderSize);
            ++currentGroupIndex;
        }

        while (currentGroupIndex < targetGroupIndex)
        {
            int itemsInGroup;
            IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetGroupInformationFromGroupIndex(
                currentGroupIndex,
                nullptr /* pStartItemIndex */,
                &itemsInGroup));
            calculatedPosition += GetVirtualizedGroupExtent(itemsInGroup, maxStackingLines, averageHeaderExtent);
            ++currentGroupIndex;
        }

        while (currentGroupIndex > targetGroupIndex)
        {
            int itemsInGroup;
            IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetGroupInformationFromGroupIndex(
                currentGroupIndex,
                nullptr /* pStartItemIndex */,
                &itemsInGroup));
            calculatedPosition -= GetVirtualizedGroupExtent(itemsInGroup, maxStackingLines, averageHeaderExtent);
            --currentGroupIndex;
        }
    }

    targetRect.*PointFromRectInVirtualizingDirection() = calculatedPosition;
    targetRect.*PointFromRectInNonVirtualizingDirection() = GetGroupPaddingAtStart().*SizeInNonVirtualizingDirection();
    if (targetGroupIndex == headerReference.ElementIndex)
    {
        targetRect.*SizeFromRectInVirtualizingDirection() = referenceHeaderSize;
    }
    else
    {
        targetRect.*SizeFromRectInVirtualizingDirection() = GetAverageHeaderExtent();
    }
    targetRect.*SizeFromRectInNonVirtualizingDirection() = m_headerSize.*SizeInNonVirtualizingDirection();

    return S_OK;
}

_Check_return_ HRESULT
WrappingLayoutStrategyImpl::EstimateNonGroupedExtent(
    _In_ xaml_controls::EstimationReference containerReference,
    _In_ wf::Rect windowConstraint,
    _Out_ wf::Size& extent)
{
    int totalItems;
    IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalItemCount(&totalItems));

    // expects to have a cellsize
    ASSERT(m_cellSizeSet);

    extent = {};

    const int maxStackingLine = DetermineMaxStackingLine(windowConstraint);
    ASSERT(maxStackingLine > 0);

    // All the containers are the same size, so this is easy
    extent.*SizeInVirtualizingDirection() = GetVirtualizedExtentOfItems(totalItems, maxStackingLine) +
        GetGroupPaddingAtStart().*SizeInVirtualizingDirection() + GetGroupPaddingAtEnd().*SizeInVirtualizingDirection();
    extent.*SizeInNonVirtualizingDirection() = GetItemStackingPosition(std::min(maxStackingLine, totalItems)) + GetGroupPaddingAtEnd().*SizeInNonVirtualizingDirection();

    return S_OK;
}

_Check_return_ HRESULT
WrappingLayoutStrategyImpl::EstimateGroupedExtent(
    _In_ xaml_controls::EstimationReference headerReference,
    _In_ xaml_controls::EstimationReference containerReference,
    _In_ wf::Rect windowConstraint,
    _Out_ wf::Size& extent)
{
    extent = {};
    int totalItems;
    int totalGroups;
    IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalItemCount(&totalItems));
    IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalGroupCount(&totalGroups));

    const float referenceHeaderExtent = (headerReference.ElementBounds.*SizeFromRectInVirtualizingDirection() > 0) ?
        headerReference.ElementBounds.*SizeFromRectInVirtualizingDirection() : GetAverageHeaderExtent();

    const int maxStackingLines = (totalItems > 0) ? DetermineMaxStackingLine(windowConstraint) : 1;

    IFC_RETURN(EnsureGroupCache(maxStackingLines));

    int itemCountInGroup;
    IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetGroupInformationFromGroupIndex(
        headerReference.ElementIndex,
        nullptr /* pStartItemIndex */,
        &itemCountInGroup));

    // The last realized element is a header, so we just do group extent estimates from here
    // Starting point is the header location, minus the left/top padding
    const float referencePoint = headerReference.ElementBounds.*PointFromRectInVirtualizingDirection() - GetGroupPaddingAtStart().*SizeInVirtualizingDirection();
    const float endOfCurrentGroup = referencePoint + GetVirtualizedGroupExtent(itemCountInGroup, maxStackingLines, referenceHeaderExtent);

    // Start with our initial guess, based on average header sizes, and adjust from there
    ASSERT(!m_cachedGroupLocations.empty());
    float estimatedExtent = m_cachedGroupLocations.back();

    // Our cached estimates were based on estimated header sizes. Now that we've laid out to this point, we can see the difference
    // between where we thought we'd be versus where we actually are, and apply that difference to where we think the last group
    // will be. In other words, we'll keep te estimates of the as-yet unrealized groups, and add them to our current location
    const float estimatedEndOfCurrentGroup = m_cachedGroupLocations[headerReference.ElementIndex];
    estimatedExtent += (endOfCurrentGroup - estimatedEndOfCurrentGroup);

    extent.*SizeInVirtualizingDirection() = estimatedExtent;
    if (totalItems > 0)
    {
        extent.*SizeInNonVirtualizingDirection() = GetItemStackingPosition(std::min(maxStackingLines, totalItems)) + GetGroupPaddingAtEnd().*SizeInNonVirtualizingDirection();
    }
    else
    {
        ASSERT(m_headerSizeSet);
        extent.*SizeInNonVirtualizingDirection() = m_headerSize.*SizeInNonVirtualizingDirection() + GetGroupPaddingAtEnd().*SizeInNonVirtualizingDirection();
    }

    return S_OK;
}

// Maintain a cache of where all groups' extent would end, so that we don't have to walk them all every frame
_Check_return_ HRESULT WrappingLayoutStrategyImpl::EnsureGroupCache(
    _In_ int maxStackingLines)
{
    int groupCount;
    IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalGroupCount(&groupCount));

    if (groupCount != static_cast<int>(m_cachedGroupLocations.size()) ||
        maxStackingLines != m_cachedStackingLines ||
        !DoubleUtil::AreClose(m_cellSize.Width, m_cachedCellSize.Width) ||
        !DoubleUtil::AreClose(m_cellSize.Height, m_cachedCellSize.Height))
    {
        m_cachedStackingLines = maxStackingLines;
        m_cachedCellSize = m_cellSize;

        m_cachedGroupLocations.clear();
        m_cachedGroupLocations.reserve(groupCount);

        float accumulatedPosition = 0;

        const float averageHeaderSize = GetAverageHeaderExtent();

        for (INT i = 0; i < groupCount; ++i)
        {
            int itemsInGroup;
            IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetGroupInformationFromGroupIndex(
                i,
                nullptr /* pStartItemIndex */,
                &itemsInGroup));
            accumulatedPosition += GetVirtualizedGroupExtent(itemsInGroup, maxStackingLines, averageHeaderSize);
            m_cachedGroupLocations.push_back(accumulatedPosition);
        }
    }

    return S_OK;
}

} } }