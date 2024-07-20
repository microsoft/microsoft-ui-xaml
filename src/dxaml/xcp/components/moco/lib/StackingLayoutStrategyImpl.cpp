// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

 // New stacking strategy to mimick stackpanel
 
#include "precomp.h"
#include "StackingLayoutStrategyImpl.h"
#include "DoubleUtil.h"
#include "RectUtil.h"
#include "NavigationIndices.h"

// Work around disruptive max/min macros
#undef max
#undef min

namespace DirectUI { namespace Components { namespace Moco {

StackingLayoutStrategyImpl::StackingLayoutStrategyImpl()
    : LayoutStrategyBase(true /* useFullWidthHeaders */, false /* isWrapping */)
    , m_headerSize()
    , m_headerSizeSet(false)
    , m_containerSizesTotal(0)
    , m_headerSizesTotal(0)
    , m_containerSizesStoredTotal(0)
    , m_headerSizesStoredTotal(0)
    , m_extentInNonVirtualizingDirection(0.0f)
{
}

void StackingLayoutStrategyImpl::Initialize()
{
    m_containerSizes.assign(c_totalSizesForAveraging, 0);
    m_headerSizes.assign(c_totalSizesForAveraging, 0);
}

#pragma region Layout related methods

// returns the size we should use to measure a container or header with
// itemIndex - indicates an index of valid item or -1 for general, non-special items
wf::Size 
StackingLayoutStrategyImpl::GetElementMeasureSize(
    _In_ xaml_controls::ElementType elementType,
    _In_ int elementIndex,
    _In_ wf::Rect windowConstraint)
{
    wf::Size result = { std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity() };

    // try to limit to the constraint
    result.*SizeInNonVirtualizingDirection() = windowConstraint.*SizeFromRectInNonVirtualizingDirection();

    // account for group padding in measure size, reduce the size in non virtualizing direction by the amount of group padding
    const float groupPaddingInNonVirtualizingDirection = GetGroupPaddingAtStart().*SizeInNonVirtualizingDirection() + GetGroupPaddingAtEnd().*SizeInNonVirtualizingDirection();
    const float measureSizeInNonVirtualizingDirection = result.*SizeInNonVirtualizingDirection() - groupPaddingInNonVirtualizingDirection;
    result.*SizeInNonVirtualizingDirection() = std::max(0.0f, measureSizeInNonVirtualizingDirection);

    // the special groupindex should get the available size
    // we only limit if we have a headersize set
    // we only need to limit in parallel layouts: inline scenarios need to have a different logic
    if (GetGroupHeaderStrategy() == GroupHeaderStrategy::Parallel &&
        elementType == xaml_controls::ElementType_GroupHeader &&
        elementIndex != c_specialGroupIndex &&
        m_headerSizeSet)
    {
        // in parallel groups, the header size influences the space that we have to measure with
        // this is a hint to the header it will only have this amount of space
        result.*SizeInNonVirtualizingDirection() = m_headerSize.*SizeInNonVirtualizingDirection();
    }

    return result;
}

_Check_return_ HRESULT
StackingLayoutStrategyImpl::GetElementBounds(
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

    m_extentInNonVirtualizingDirection = std::max(
        m_extentInNonVirtualizingDirection,
        pReturnValue->*PointFromRectInNonVirtualizingDirection() + pReturnValue->*SizeFromRectInNonVirtualizingDirection());

    return S_OK;
}

wf::Rect
StackingLayoutStrategyImpl::GetElementArrangeBounds(
    _In_ xaml_controls::ElementType elementType,
    _In_ int elementIndex,
    _In_ wf::Rect containerBounds,
    _In_ wf::Rect windowConstraint,
    _In_ wf::Size finalSize)
{
    wf::Rect result = containerBounds;
    if (elementType == xaml_controls::ElementType_ItemContainer)
    {
        // we will give the container what it requested if bigger than the constraint, and let the clipping occur by the ScrollViewer
        // we will give the container the constraint of the window (the viewport really) so that it can be laid out inside of the 
        // viewport. Basically this means that a bigger container does not influence the alignment of smaller elements.
        // we do not use the finalsize, because that will represent the largest element in the viewport.
        result.*SizeFromRectInNonVirtualizingDirection() = std::max(containerBounds.*SizeFromRectInNonVirtualizingDirection(), windowConstraint.*SizeFromRectInNonVirtualizingDirection());

        // unfortunate, but incorrectly configured panels (for instance, panel is set to orient horizontally, where ScrollViewer is set to enable scrolling vertically
        // will potentially have infinity here. Also, the listview itself might have been inside of ScrollViewer that allowed infinite in this direction
        result.*SizeFromRectInNonVirtualizingDirection() = std::min(result.*SizeFromRectInNonVirtualizingDirection(), finalSize.*SizeInNonVirtualizingDirection());

        if (m_headerSizeSet && GetGroupHeaderStrategy() == GroupHeaderStrategy::Parallel)
        {
            result.*SizeFromRectInNonVirtualizingDirection() -= m_headerSize.*SizeInNonVirtualizingDirection();
        }

        result.*SizeFromRectInNonVirtualizingDirection() -= GetGroupPaddingAtStart().*SizeInNonVirtualizingDirection() + GetGroupPaddingAtEnd().*SizeInNonVirtualizingDirection();
    }
    else
    {
        if (GetGroupHeaderStrategy() == GroupHeaderStrategy::Inline)
        {
            result.*SizeFromRectInNonVirtualizingDirection() = windowConstraint.*SizeFromRectInNonVirtualizingDirection();

            // unfortunate, but incorrectly configured panels (for instance, panel is set to orient horizontally, where ScrollViewer is set to enable scrolling vertically
            // will potentially have infinity here. Also, the listview itself might have been inside of ScrollViewer that allowed infinite in this direction
            result.*SizeFromRectInNonVirtualizingDirection() = std::min(result.*SizeFromRectInNonVirtualizingDirection(), finalSize.*SizeInNonVirtualizingDirection());

            // account for group padding, reduce the size in non virtualizing direction by the amount of group padding
            const float groupPaddingInNonVirtualizingDirection = GetGroupPaddingAtStart().*SizeInNonVirtualizingDirection() + GetGroupPaddingAtEnd().*SizeInNonVirtualizingDirection();
            const float arrangeSizeInNonVirtualizingDirection = result.*SizeFromRectInNonVirtualizingDirection() - groupPaddingInNonVirtualizingDirection;
            result.*SizeFromRectInNonVirtualizingDirection() = std::max(0.0f, arrangeSizeInNonVirtualizingDirection);
        }
        else
        {
            if (m_headerSizeSet)
            {
                result.*SizeFromRectInNonVirtualizingDirection() = std::max(result.*SizeFromRectInNonVirtualizingDirection(), m_headerSize.*SizeInNonVirtualizingDirection());
            }
        }
    }
    return result;
}

bool
StackingLayoutStrategyImpl::ShouldContinueFillingUpSpace(
    _In_ xaml_controls::ElementType elementType,
    _In_ int elementIndex,
    _In_ xaml_controls::LayoutReference referenceInformation,
    _In_ wf::Rect windowToFill)
{
    bool shouldContinue = FALSE;
    const bool requestingHeader = (elementType == xaml_controls::ElementType_GroupHeader);
    UNREFERENCED_PARAMETER(elementIndex);

    if (referenceInformation.RelativeLocation == xaml_controls::ReferenceIdentity_Myself)
    {
        // always do yourself
        shouldContinue = TRUE;
    }
    else if (referenceInformation.RelativeLocation == xaml_controls::ReferenceIdentity_BeforeMe)
    {
        // is there room on the right?
        if (referenceInformation.ReferenceIsHeader &&
            GroupHeaderStrategy::Parallel == GetGroupHeaderStrategy() &&
            !requestingHeader)
        {
            // If we are going from a header to a container, and the header is parallel, then we can generate a container
            // if only the first edge of the header is in view
            shouldContinue =
                windowToFill.*PointFromRectInVirtualizingDirection() + windowToFill.*SizeFromRectInVirtualizingDirection()
                >= referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection();

        }
        else
        {
            // with an inline header, or following an item, we shouldn't continue until
            // the entire reference is in the window
            shouldContinue =
                windowToFill.*PointFromRectInVirtualizingDirection() + windowToFill.*SizeFromRectInVirtualizingDirection()
                >= referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection() + referenceInformation.ReferenceBounds.*SizeFromRectInVirtualizingDirection();
        }
    }
    else
    {
        // is there room to the left?
        shouldContinue = windowToFill.*PointFromRectInVirtualizingDirection() <= referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection();
    }

    return shouldContinue;
}

wf::Point
StackingLayoutStrategyImpl::GetPositionOfFirstElement()
{
    const wf::Size initialPadding = GetGroupPaddingAtStart();
    return { initialPadding.Width, initialPadding.Height };
}

#pragma endregion

#pragma region Estimation and virtualization related methods.

_Check_return_ HRESULT
StackingLayoutStrategyImpl::EstimateElementIndex(
    _In_ xaml_controls::ElementType elementType,
    _In_ xaml_controls::EstimationReference headerReference,
    _In_ xaml_controls::EstimationReference containerReference,
    _In_ wf::Rect window,
    _Out_ wf::Rect* pTargetRect,
    _Out_ INT* pReturnValue)
{
    int result = -1;
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

    *pReturnValue = result;
    return S_OK;
}

_Check_return_ HRESULT
StackingLayoutStrategyImpl::EstimateElementBounds(
    _In_ xaml_controls::ElementType elementType,
    _In_ int elementIndex,
    _In_ xaml_controls::EstimationReference headerReference,
    _In_ xaml_controls::EstimationReference containerReference,
    _In_ wf::Rect window,
    _Out_ wf::Rect* pReturnValue)
{
    wf::Rect result = {};

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
StackingLayoutStrategyImpl::EstimatePanelExtent(
    _In_ xaml_controls::EstimationReference lastHeaderReference,
    _In_ xaml_controls::EstimationReference lastContainerReference,
    _In_ wf::Rect windowConstraint,
    _Out_ wf::Size* pExtent)
{
    wf::Size result = {};

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
_Check_return_ HRESULT StackingLayoutStrategyImpl::EstimateIndexFromPoint(
    _In_ bool requestingInsertionIndex,
    _In_ wf::Point point,
    _In_ xaml_controls::EstimationReference reference,
    _In_ wf::Rect windowConstraint,
    _Out_ xaml_controls::IndexSearchHint* pSearchHint,
    _Out_ xaml_controls::ElementType* pElementType,
    _Out_ INT* pElementIndex)
{
    *pElementIndex = reference.ElementIndex;
    const wf::Rect referenceItemRect = reference.ElementBounds;
    *pElementType = xaml_controls::ElementType_ItemContainer;

    int totalItems;
    IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalItemCount(&totalItems));

    // compare the point to the rect and either return exact, above or below
    // notice how I only care about the virtualizing direction.
    if (point.*PointInVirtualizingDirection() < referenceItemRect.*PointFromRectInVirtualizingDirection() &&
        *pElementIndex > 0)
    {
        *pSearchHint = xaml_controls::IndexSearchHint_SearchBackwards;
    }
    else if 
        ((point.*PointInVirtualizingDirection() > referenceItemRect.*PointFromRectInVirtualizingDirection() + referenceItemRect.*SizeFromRectInVirtualizingDirection()) &&
        (totalItems - 1 > *pElementIndex))
    {
        // the point is further along than the rect, so indicate that to find it, you should search backwards
        *pSearchHint = xaml_controls::IndexSearchHint_SearchForwards;
    }
    else
    {
        *pSearchHint = xaml_controls::IndexSearchHint_Exact;
    }

    // if exact, just follow below calculation if we are returning the insertion index.
    if (requestingInsertionIndex &&
        *pSearchHint == xaml_controls::IndexSearchHint_Exact)
    {
        // If we're below the midpoint of the container under the point, we actually want to insert at the next index.
        if (point.*PointInVirtualizingDirection() - referenceItemRect.*PointFromRectInVirtualizingDirection()
            >= (referenceItemRect.*SizeFromRectInVirtualizingDirection() / 2.0f))
        {
            ++*pElementIndex;
        }
    }

    return S_OK;
}

// Based on current element's index/type and action, return the next element index/type.
_Check_return_ HRESULT StackingLayoutStrategyImpl::GetTargetIndexFromNavigationAction(
    _In_ xaml_controls::ElementType elementType,
    _In_ int elementIndex,
    _In_ xaml_controls::KeyNavigationAction action,
    _In_ wf::Rect windowConstraint,
    _Out_ xaml_controls::ElementType* pTargetElementType,
    _Out_ INT* pTargetElementIndex)
{
    int targetIndex = elementIndex;
    auto targetType = elementType;

    if (action != xaml_controls::KeyNavigationAction_Left &&
        action != xaml_controls::KeyNavigationAction_Right &&
        action != xaml_controls::KeyNavigationAction_Up &&
        action != xaml_controls::KeyNavigationAction_Down)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    int step = 0;
    int targetIndexMax = 0;

    if (elementType == xaml_controls::ElementType_ItemContainer)
    {
        IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalItemCount(&targetIndexMax));
    }
    else
    {
        ASSERT(IsGrouping());

        IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalGroupCount(&targetIndexMax));
    }

    ASSERT(0 <= elementIndex && elementIndex < targetIndexMax);

    // The action is along layout orientation, therefore handle it.
    if ((GetVirtualizationDirection() == xaml_controls::Orientation_Horizontal && (action == xaml_controls::KeyNavigationAction_Left || action == xaml_controls::KeyNavigationAction_Right)) ||
        (GetVirtualizationDirection() == xaml_controls::Orientation_Vertical && (action == xaml_controls::KeyNavigationAction_Up || action == xaml_controls::KeyNavigationAction_Down)))
    {
        step = (action == xaml_controls::KeyNavigationAction_Left || action == xaml_controls::KeyNavigationAction_Up) ? -1 : 1;
    }

    targetIndex = std::min(std::max(elementIndex + step, 0), targetIndexMax - 1);

    // Evaluate whether we need to change the element type & index.
    if (IsGrouping())
    {
        if (elementType == xaml_controls::ElementType_ItemContainer)
        {
            int targetGroupHeaderIndex;
            IFC_RETURN(TryGetTargetHeaderIndexWithItemNavigation(elementIndex, targetIndex, step, targetGroupHeaderIndex));
            if (targetGroupHeaderIndex != -1)
            {
                targetIndex = targetGroupHeaderIndex;
                targetType = xaml_controls::ElementType_GroupHeader;
            }
        }
        else
        {
            int targetItemIndex = -1;
            IFC_RETURN(TryGetTargetItemIndexWithHeaderNavigation(elementIndex, targetIndex, step, targetItemIndex));
            if (targetItemIndex != -1)
            {
                targetIndex = targetItemIndex;
                targetType = xaml_controls::ElementType_ItemContainer;
            }
        }
    }

    *pTargetElementType = targetType;
    *pTargetElementIndex = targetIndex;

    return S_OK;
}

_Check_return_ HRESULT StackingLayoutStrategyImpl::TryGetTargetHeaderIndexWithItemNavigation(
    int currentItemIndex, 
    int targetItemIndex, 
    int step, 
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

        if (currentItemIndex == targetItemIndex)
        {
            // Handle the special-case of going up/back from the first line in the first group
            // with items.
            if (currentItemIndex == 0 && step < 0)
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
                const bool isInLastGroupWithItems = (currentItemIndex == (totalItems - currItemCountInGroup + currIndexInsideOfGroup));
                if (isInLastGroupWithItems && step > 0 && currIndexOfGroup < (totalGroups - 1))
                {
                    targetGroupHeaderIndex = currIndexOfGroup + 1;
                }
            }
        }
        else
        {
            // Evaluate whether the new target item index would cross a group boundary
            // when compared to the previous item index.
            int targetIndexOfGroup = 0;
            int targetIndexInsideOfGroup = 0;
            int targetItemCountInGroup = 0;
            IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetGroupInformationFromItemIndex(targetItemIndex, &targetIndexOfGroup, &targetIndexInsideOfGroup, &targetItemCountInGroup));

            if (currIndexOfGroup != targetIndexOfGroup)
            {
                // If advancing forward, don't just set the new target group index to targetIndexOfGroup,
                // instead just increment the value of currIndexOfGroup.  This is so we don't skip
                // group headers that have no items.
                targetGroupHeaderIndex = (step < 0 ? currIndexOfGroup : currIndexOfGroup + 1);
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT StackingLayoutStrategyImpl::TryGetTargetItemIndexWithHeaderNavigation(
    int currentGroupIndex, 
    int targetGroupIndex, 
    int step, 
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
            targetItemIndex = currGroupStartItemIndex;
        }
        else if (currentGroupIndex != targetGroupIndex)
        {
            int targetGroupStartItemIndex = 0;
            int targetGroupItemCountInGroup = 0;
            IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetGroupInformationFromGroupIndex(targetGroupIndex, &targetGroupStartItemIndex, &targetGroupItemCountInGroup));

            // Change the targetIndex to point to an item index if jumping between groups would
            // jump us over some items.
            if (targetGroupIndex < currentGroupIndex && targetGroupItemCountInGroup > 0)
            {
                targetItemIndex = targetGroupStartItemIndex + targetGroupItemCountInGroup - 1;
            }
            else if (targetGroupIndex > currentGroupIndex && currGroupItemCountInGroup > 0)
            {
                targetItemIndex = currGroupStartItemIndex;
            }
        }
    }

    return S_OK;
}

// Determines whether or not the given item index
// is a layout boundary.
_Check_return_ HRESULT StackingLayoutStrategyImpl::IsIndexLayoutBoundary(
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
    
    int totalItems;
    IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalItemCount(&totalItems));

    const bool isHorizontal = (GetVirtualizationDirection() == xaml_controls::Orientation_Horizontal);
    if (isHorizontal)
    {
        *pIsLeftBoundary = (elementIndex == 0);
        *pIsBottomBoundary = true;
        *pIsTopBoundary = true;
        *pIsRightBoundary = (elementIndex == totalItems - 1);
    }
    else
    {
        *pIsLeftBoundary = true;
        *pIsBottomBoundary = (elementIndex == totalItems - 1);
        *pIsTopBoundary = (elementIndex == 0);
        *pIsRightBoundary = true;
    }

    return S_OK;
}

#pragma endregion

#pragma region Snap points related

// Stacking layout doesn't have regular snap points
bool StackingLayoutStrategyImpl::GetRegularSnapPoints(
    _Out_ float* pNearOffset,
    _Out_ float* pFarOffset,
    _Out_ float* pSpacing)
{
    *pNearOffset = 0.0f;
    *pFarOffset = 0.0f;
    *pSpacing = 0.0f;
    return false;
}

bool StackingLayoutStrategyImpl::HasIrregularSnapPoints(
    _In_ xaml_controls::ElementType elementType)
{
    bool result;
    if (elementType == xaml_controls::ElementType_ItemContainer)
    {
        // Always give snap points for items
        result = true;
    }
    else
    {
        // ISP returns header snap points if we are grouping and we are drawing inline headers
        result = (GetGroupHeaderStrategy() == GroupHeaderStrategy::Inline);
    }

    return result;
}

#pragma endregion

wf::Rect
StackingLayoutStrategyImpl::GetElementTransitionsBounds(
    _In_ xaml_controls::ElementType elementType,
    _In_ int elementIndex,
    _In_ wf::Rect windowConstraint)
{
    return {};
}


const float StackingLayoutStrategyImpl::GetDistanceBetweenGroups() const
{
    const xaml::Thickness groupPadding = GetGroupPadding();
    return GetVirtualizationDirection() == xaml_controls::Orientation_Vertical ?
        static_cast<float>(groupPadding.Bottom + groupPadding.Top)
        : static_cast<float>(groupPadding.Left + groupPadding.Right);

}

float StackingLayoutStrategyImpl::GetVirtualizedGroupExtent(_In_ float itemsInGroup, _In_ float headerExtent) const
{
    float result = GetAverageVirtualizedExtentOfItems(itemsInGroup);

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

// todo: validate that this needs the header size as well, seems like it from usage
float StackingLayoutStrategyImpl::GetAverageVirtualizedGroupExtent(_In_ float averageItemsPerGroup) const
{
    const float averageHeaderSize = GetAverageHeaderSize();

    return GetVirtualizedGroupExtent(averageItemsPerGroup, averageHeaderSize);
}

float StackingLayoutStrategyImpl::GetAverageVirtualizedExtentOfItems(_In_ float itemCount) const
{
    if (itemCount > 0)
    {
        float averageItemSize = GetAverageContainerSize();
        return averageItemSize * itemCount;
    }

    return 0.0f;
}

void StackingLayoutStrategyImpl::RegisterSpecialContainerSize(_In_ int itemIndex, _In_ wf::Size containerDesiredSize)
{
    ASSERT(itemIndex == c_specialItemIndex);
    RegisterSize(itemIndex, false /* isHeader */, containerDesiredSize.*SizeInVirtualizingDirection());
}
void StackingLayoutStrategyImpl::RegisterSpecialHeaderSize(_In_ int groupIndex, _In_ wf::Size headerDesiredSize)
{
    ASSERT(groupIndex == c_specialGroupIndex);
    m_headerSize = headerDesiredSize;
    m_headerSizeSet = true;
    RegisterSize(groupIndex, true /* isHeader */, headerDesiredSize.*SizeInVirtualizingDirection());
}

// registers a size so that we can be better at averaging
void StackingLayoutStrategyImpl::RegisterSize(_In_ int index, _In_ bool isHeader, _In_ float newSize)
{
    // if we scrolled past 10.000, let's start looking at these new ones.
    // in general we do this mostly so that we can deal with a panel that starts of scrolled to the right
    // and would not be registering any sizes at all if it were not for 
    int moddedIndex = index % c_totalSizesForAveraging;

    if (isHeader)
    {
        float oldSize = m_headerSizes[moddedIndex];
        m_headerSizes.at(moddedIndex) = newSize;
        m_headerSizesTotal += newSize - oldSize;

        if (DoubleUtil::AreClose(0, oldSize) && !DoubleUtil::AreClose(0, newSize))
        {
            ++m_headerSizesStoredTotal;
        }
        if (!DoubleUtil::AreClose(0, oldSize) && DoubleUtil::AreClose(0, newSize))
        {
            --m_headerSizesStoredTotal;
        }
    }
    else
    {
        float oldSize = m_containerSizes[moddedIndex];
        m_containerSizes.at(moddedIndex) = newSize;
        m_containerSizesTotal += newSize - oldSize;

        if (DoubleUtil::AreClose(0, oldSize) && !DoubleUtil::AreClose(0, newSize))
        {
            ++m_containerSizesStoredTotal;
        }
        if (!DoubleUtil::AreClose(0, oldSize) && DoubleUtil::AreClose(0, newSize))
        {
            --m_containerSizesStoredTotal;
        }
    }
}

float StackingLayoutStrategyImpl::GetAverageHeaderSize() const
{
    // if we have just 1 group header and it is collapsed there will be no registered size for it.
    return static_cast<float>(m_headerSizesStoredTotal > 0 ? m_headerSizesTotal / m_headerSizesStoredTotal : 10.0);

}

float StackingLayoutStrategyImpl::GetAverageContainerSize() const
{
    // if we have just 1 container and it is collapsed there will be no registered size for it.
    return  static_cast<float>(m_containerSizesStoredTotal > 0 ? m_containerSizesTotal / m_containerSizesStoredTotal : 10.0);
}

//
// Private methods
//

wf::Rect
StackingLayoutStrategyImpl::GetContainerBounds(
    _In_ int indexInItems,
    _In_ int indexInGroup,
    _In_ wf::Size containerDesiredSize,
    _In_ xaml_controls::LayoutReference referenceInformation,
    _In_ wf::Rect windowConstraint)
{
    wf::Rect bounds = {};

    // Required sizes set
    ASSERT(!IsGrouping() || (IsGrouping() && m_headerSizeSet));

    // register the size of this item for better averaging
    RegisterSize(indexInItems, FALSE /* isHeader */, containerDesiredSize.*SizeInVirtualizingDirection());

    if (referenceInformation.ReferenceIsHeader)
    {
        // input
        const float distanceBetweenGroups = GetDistanceBetweenGroups();

        // mismatch
        ASSERT(referenceInformation.RelativeLocation != xaml_controls::ReferenceIdentity_Myself);

        if (referenceInformation.RelativeLocation == xaml_controls::ReferenceIdentity_BeforeMe)
        {
            ASSERT(indexInGroup == 0); // if not, this is an invalid calculation

            bounds.*PointFromRectInVirtualizingDirection() = referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection();
            if (GroupHeaderStrategy::Inline == GetGroupHeaderStrategy())
            {
                // Add the size of the inline header
                bounds.*PointFromRectInVirtualizingDirection() += referenceInformation.ReferenceBounds.*SizeFromRectInVirtualizingDirection();
            }
        }
        else
        {
            // I'm going to be the last container in some new header that we will also be creating
            bounds.*PointFromRectInVirtualizingDirection() =
                referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection() - containerDesiredSize.*SizeInVirtualizingDirection() - distanceBetweenGroups;
        }
    }
    else
    {
        // we got a sibling container
        if (referenceInformation.RelativeLocation == xaml_controls::ReferenceIdentity_BeforeMe)
        {
            bounds.*PointFromRectInVirtualizingDirection() = referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection() + referenceInformation.ReferenceBounds.*SizeFromRectInVirtualizingDirection();
        }
        else if (referenceInformation.RelativeLocation == xaml_controls::ReferenceIdentity_AfterMe)
        {
            bounds.*PointFromRectInVirtualizingDirection() = referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection() - containerDesiredSize.*SizeInVirtualizingDirection();
        }
        else
        {
            bounds.*PointFromRectInVirtualizingDirection() = referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection();
        }
    }

    bounds.*PointFromRectInNonVirtualizingDirection() = GetGroupPaddingAtStart().*SizeInNonVirtualizingDirection();
    if (GroupHeaderStrategy::Parallel == GetGroupHeaderStrategy())
    {
        // If we have a parallel header, move out of its way
        bounds.*PointFromRectInNonVirtualizingDirection() += m_headerSize.*SizeInNonVirtualizingDirection();
    }

    bounds.*SizeFromRectInVirtualizingDirection() = containerDesiredSize.*SizeInVirtualizingDirection();
    bounds.*SizeFromRectInNonVirtualizingDirection() = containerDesiredSize.*SizeInNonVirtualizingDirection();

    return bounds;
}

wf::Rect 
StackingLayoutStrategyImpl::GetHeaderBounds(
    _In_ int groupIndex,
    _In_ wf::Size headerDesiredSize,
    _In_ xaml_controls::LayoutReference referenceInformation,
    _In_ wf::Rect windowConstraint)
{
    wf::Rect bounds = {};

    float distanceBetweenGroups = GetDistanceBetweenGroups();

    // Required sizes set
    ASSERT(IsGrouping() && m_headerSizeSet);

    // register the size of this item for better averaging
    RegisterSize(groupIndex, TRUE /* isHeader */, headerDesiredSize.*SizeInVirtualizingDirection());

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
    bounds.*SizeFromRectInNonVirtualizingDirection() = headerDesiredSize.*SizeInNonVirtualizingDirection();  // not constrained

    return bounds;
}

// Estimate how many items we need to traverse to get from our reference point to a suitable anchor item for the window.
_Check_return_ HRESULT
StackingLayoutStrategyImpl::EstimateItemIndexFromWindow(
    _In_ xaml_controls::EstimationReference headerReference,
    _In_ xaml_controls::EstimationReference containerReference,
    _In_ wf::Rect window,
    _Out_ wf::Rect* pTargetRect,
    _Out_ int& targetItemIndex)
{
    int totalItems;
    IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalItemCount(&totalItems));

    ASSERT(0 < totalItems);

    // todo: it is known that this estimation is not perfect
    // we will improve this as we log bugs for non-accuracy

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
            headerExtent = GetAverageHeaderSize();
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

    const float averageContainerSize = GetAverageContainerSize();

    int itemDelta = 0;
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
            itemDelta = static_cast<int>(std::floor(distance / averageContainerSize));
        }
        else
        {
            itemDelta = 0;
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
            itemDelta = static_cast<int>(std::floor(distance / averageContainerSize));
        }
        else
        {
            // In case we're going backwards from a header, just pick the first item in the group.
            itemDelta = 0;
        }
    }
        break;

    default:
        itemDelta = 0;
        break;
    }

    // Get the target index, bounds-check it, and calculate the distance moved
    targetItemIndex = referenceIndex + itemDelta;
    targetItemIndex = std::max(0, targetItemIndex);
    targetItemIndex = std::min(targetItemIndex, totalItems - 1);
    itemDelta = targetItemIndex - referenceIndex;
    calculatedPosition = virtualizedReferencePoint + (itemDelta * averageContainerSize);

    pTargetRect->*PointFromRectInVirtualizingDirection() = calculatedPosition;
    switch (GetGroupHeaderStrategy())
    {
    case GroupHeaderStrategy::Parallel:
        pTargetRect->*PointFromRectInNonVirtualizingDirection() = m_headerSize.*SizeInNonVirtualizingDirection();
        break;

    case GroupHeaderStrategy::Inline:
    case GroupHeaderStrategy::None:
        pTargetRect->*PointFromRectInNonVirtualizingDirection() = 0;
        break;
    }
    pTargetRect->*PointFromRectInNonVirtualizingDirection() += GetGroupPaddingAtStart().*SizeInNonVirtualizingDirection();
    pTargetRect->*SizeFromRectInVirtualizingDirection() = averageContainerSize;
    pTargetRect->*SizeFromRectInNonVirtualizingDirection() = 0;

    return S_OK;
}

// Estimate how many groups we need to traverse to get from our reference point to a suitable
// anchor group for the window, by walking through the groups.
_Check_return_ HRESULT
StackingLayoutStrategyImpl::EstimateGroupIndexFromWindow(
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

    const float averageHeaderSize = GetAverageHeaderSize();
    const float referenceHeaderSize = (headerReference.ElementBounds.*SizeFromRectInVirtualizingDirection() > 0) ?
        headerReference.ElementBounds.*SizeFromRectInVirtualizingDirection() : averageHeaderSize;
    const float nearWindowEdge = window.*PointFromRectInVirtualizingDirection();
    const float farWindowEdge = nearWindowEdge + window.*SizeFromRectInVirtualizingDirection();

    ASSERT(nearWindowEdge <= farWindowEdge);

    // CalculatedPosition is the current position we are looking at. candidatePosition is the next one
    // to be looked at.
    float calculatedPosition = headerReference.ElementBounds.*PointFromRectInVirtualizingDirection();
    targetGroupIndex = headerReference.ElementIndex;

    if (calculatedPosition + referenceHeaderSize < nearWindowEdge)
    {
        // Our header is before of the window
        // Need to walk forward to reach it, making sure we step back if a large group overshoots the window entirely
        // To perform the backtrack in this case, we first calculate a "candidate position", accepting it only if we didn't overshoot

        bool found = false;
        int itemsInGroup;
        IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetGroupInformationFromGroupIndex(
            targetGroupIndex,
            nullptr /* pStartItemIndex */,
            &itemsInGroup));

        // Since we have this header's size and group count, let's just use it first before jumping off into averages
        float candidatePosition = calculatedPosition + GetVirtualizedGroupExtent(static_cast<float>(itemsInGroup), referenceHeaderSize);

        // Bail out if we encounter the last group, as we can't go farther than that.
        while (!found && targetGroupIndex + 1 < totalGroups)
        {
            if (farWindowEdge < candidatePosition)
            {
                // That group was big! We overshot the window, so let's not move to the next group and use the current one
                found = true;
            }
            else if (candidatePosition + averageHeaderSize < nearWindowEdge)
            {
                // We're going to move to the next group and continue
                ++targetGroupIndex;
                calculatedPosition = candidatePosition;
                IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetGroupInformationFromGroupIndex(
                    targetGroupIndex,
                    nullptr /* pStartItemIndex */,
                    &itemsInGroup));
                candidatePosition = calculatedPosition + GetVirtualizedGroupExtent(static_cast<float>(itemsInGroup), averageHeaderSize);
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

            calculatedPosition -= GetVirtualizedGroupExtent(static_cast<float>(itemsInGroup), averageHeaderSize);

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
        targetRect.*SizeFromRectInVirtualizingDirection() = GetAverageHeaderSize();
    }
    targetRect.*SizeFromRectInNonVirtualizingDirection() = m_headerSize.*SizeInNonVirtualizingDirection();
    *pTargetRect = targetRect;

    return S_OK;
}

// Estimate the location of an anchor item, given an item index delta.
_Check_return_ HRESULT
StackingLayoutStrategyImpl::EstimateContainerLocation(
    _In_ int targetItemIndex,
    _In_ xaml_controls::EstimationReference headerReference,
    _In_ xaml_controls::EstimationReference containerReference,
    _In_ wf::Rect window,
    _Out_ wf::Rect& targetRect)
{
    int totalItems;
    IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalItemCount(&totalItems));

    ASSERT(0 < totalItems);
    ASSERT(0 <= targetItemIndex && targetItemIndex < totalItems);

    targetRect = {};

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
            headerExtent = GetAverageHeaderSize();
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

        // If we attempt to estimate backwards from a header, just use the first item in the group
        targetItemIndex = std::max(targetItemIndex, referenceIndex);
    }
    const float virtualizedReferencePoint = referenceRect.*PointFromRectInVirtualizingDirection() + headerAdjustment;
    const float averageContainerSize = GetAverageContainerSize();

    const int itemDelta = targetItemIndex - referenceIndex;
    targetRect.*PointFromRectInVirtualizingDirection() = virtualizedReferencePoint + (itemDelta * averageContainerSize);

    switch (GetGroupHeaderStrategy())
    {
    case GroupHeaderStrategy::Parallel:
        targetRect.*PointFromRectInNonVirtualizingDirection() = m_headerSize.*SizeInNonVirtualizingDirection();
        break;

    case GroupHeaderStrategy::Inline:
    case GroupHeaderStrategy::None:
        targetRect.*PointFromRectInNonVirtualizingDirection() = 0;
        break;
    }
    targetRect.*PointFromRectInNonVirtualizingDirection() += GetGroupPaddingAtStart().*SizeInNonVirtualizingDirection();
    targetRect.*SizeFromRectInVirtualizingDirection() = averageContainerSize;
    targetRect.*SizeFromRectInNonVirtualizingDirection() = 0;

    return S_OK;
}

// Estimate the location of an anchor group, using items-per-group to estimate an average group extent.
_Check_return_ HRESULT 
StackingLayoutStrategyImpl::EstimateHeaderLocation(
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

    targetRect = {};

    const float averageHeaderSize = GetAverageHeaderSize();
    const float referenceHeaderSize = (headerReference.ElementBounds.*SizeFromRectInVirtualizingDirection() > 0) ?
        headerReference.ElementBounds.*SizeFromRectInVirtualizingDirection() : averageHeaderSize;

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
            const float averageContainerSize = GetAverageContainerSize();

            float referenceItemSize = (containerReference.ElementBounds.*SizeFromRectInVirtualizingDirection() > 0) ?
                containerReference.ElementBounds.*SizeFromRectInVirtualizingDirection() : averageContainerSize;

            // We're using this item to estimate the placement of an adjacent header
            const int itemIndexInGroup = containerReference.ElementIndex - firstItemInGroup;
            float itemReference = containerReference.ElementBounds.*PointFromRectInVirtualizingDirection();

            if (targetGroupIndex == headerReference.ElementIndex)
            {
                // We're looking for the header immediately before our item
                calculatedPosition = itemReference - itemIndexInGroup * averageContainerSize;

                if (GetGroupHeaderStrategy() == GroupHeaderStrategy::Inline)
                {
                    // Make room for an inline header
                    calculatedPosition -= referenceHeaderSize;
                }
            }
            else
            {
                // We're looking for the header immediately after our item
                // Add the item we know about
                calculatedPosition = itemReference + referenceItemSize;
                // And now, add the rest
                int remainingItems = itemCountInGroup - itemIndexInGroup - 1;
                if (remainingItems > 0)
                {
                    calculatedPosition += remainingItems * averageContainerSize;
                }
            }
        }
        else
        {
            // We're using this header to estimate the placement of an adjacent header
            calculatedPosition = headerReference.ElementBounds.*PointFromRectInVirtualizingDirection();
            if (targetGroupIndex == headerReference.ElementIndex + 1)
            {
                calculatedPosition += GetVirtualizedGroupExtent(static_cast<float>(itemCountInGroup), referenceHeaderSize);
            }
        }
    }
    else
    {
        const float virtualizedReferencePoint = headerReference.ElementBounds.*PointFromRectInVirtualizingDirection();
        calculatedPosition = virtualizedReferencePoint;

        // Put a sanity bound on the group index, for FRE builds
        targetGroupIndex = std::max(0, targetGroupIndex);
        targetGroupIndex = std::min(targetGroupIndex, static_cast<INT>(totalGroups - 1));

        // Determine which direction we're going, and calculate average group sizes accordingly
        RelativePosition relativePos;
        if (headerReference.ElementIndex < targetGroupIndex)
        {
            // Going forward, i.e., our reference is before the estimation region
            relativePos = RelativePosition::Before;
        }
        else if (targetGroupIndex < headerReference.ElementIndex)
        {
            // Going backward, i.e., our reference is after the estimation region
            relativePos = RelativePosition::After;
        }
        else
        {
            relativePos = RelativePosition::Inside;
        }

        int firstItemIndexInGroup;
        IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetGroupInformationFromGroupIndex(
            headerReference.ElementIndex,
            &firstItemIndexInGroup,
            nullptr /* pItemCountInGroup */));

        int remainingGroups = GetRemainingGroups(headerReference.ElementIndex, totalGroups, relativePos);
        int remainingItems = GetRemainingItems(firstItemIndexInGroup, totalItems, relativePos);

        if (remainingGroups > 0)
        {
            const float averageItemsPerRemainingGroup = (remainingItems) / static_cast<float>(remainingGroups);
            const float averageExtentOfRemainingGroups = GetAverageVirtualizedGroupExtent(averageItemsPerRemainingGroup);
            calculatedPosition = virtualizedReferencePoint + averageExtentOfRemainingGroups * (targetGroupIndex - headerReference.ElementIndex);
        }
        else
        {
            calculatedPosition = virtualizedReferencePoint;
        }
    }

    targetRect.*PointFromRectInVirtualizingDirection() = calculatedPosition;
    targetRect.*PointFromRectInNonVirtualizingDirection() = GetGroupPaddingAtStart().*SizeInNonVirtualizingDirection();
    if (targetGroupIndex == headerReference.ElementIndex)
    {
        // If the target group is the same as the reference group, just reuse its size
        targetRect.*SizeFromRectInVirtualizingDirection() = referenceHeaderSize;
    }
    else
    {
        targetRect.*SizeFromRectInVirtualizingDirection() = averageHeaderSize;
    }
    targetRect.*SizeFromRectInNonVirtualizingDirection() = m_headerSize.*SizeInNonVirtualizingDirection();

    return S_OK;
}

_Check_return_ HRESULT 
StackingLayoutStrategyImpl::EstimateNonGroupedExtent(
    _In_ xaml_controls::EstimationReference containerReference,
    _In_ wf::Rect windowConstraint,
    _Out_ wf::Size& estimate)
{
    estimate = {};
    int totalItems;
    IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalItemCount(&totalItems));

    // containers are variable size, so going off just the average is wrong
    float distance = 0;
    if (totalItems > containerReference.ElementIndex + 1)
    {
        distance = GetAverageVirtualizedExtentOfItems(static_cast<float>(totalItems - (containerReference.ElementIndex + 1))); // don't include lastitem itself
    }

    estimate.*SizeInVirtualizingDirection() = distance +
        containerReference.ElementBounds.*PointFromRectInVirtualizingDirection() + containerReference.ElementBounds.*SizeFromRectInVirtualizingDirection();

    // Account for the padding at the end
    estimate.*SizeInVirtualizingDirection() += GetGroupPaddingAtEnd().*SizeInVirtualizingDirection();
    estimate.*SizeInNonVirtualizingDirection() = m_extentInNonVirtualizingDirection;

    return S_OK;
}

_Check_return_ HRESULT
StackingLayoutStrategyImpl::EstimateGroupedExtent(
    _In_ xaml_controls::EstimationReference headerReference,
    _In_ xaml_controls::EstimationReference containerReference,
    _In_ wf::Rect windowConstraint,
    _Out_ wf::Size& extent)

{
    ASSERT(IsGrouping() && m_headerSizeSet);

    int totalItems;
    int totalGroups;
    IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalItemCount(&totalItems));
    IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalGroupCount(&totalGroups));

    extent = {};

    const float averageHeaderSize = GetAverageHeaderSize();
    const float referenceHeaderExtent = (headerReference.ElementBounds.*SizeFromRectInVirtualizingDirection() > 0) ?
        headerReference.ElementBounds.*SizeFromRectInVirtualizingDirection() : averageHeaderSize;

    float referencePoint;
    int firstItemIndexInGroup;
    int itemCountInGroup;
    IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetGroupInformationFromGroupIndex(
        headerReference.ElementIndex,
        &firstItemIndexInGroup,
        &itemCountInGroup));

    if (containerReference.ElementIndex != -1 && containerReference.ElementIndex >= firstItemIndexInGroup)
    {
        // This item will gives us an idea of the group extent that's better than going from the header
        // So, we'll estimate from here to the end of the group, then pick back up with average group sizes
        referencePoint = containerReference.ElementBounds.*PointFromRectInVirtualizingDirection() + containerReference.ElementBounds.*SizeFromRectInVirtualizingDirection();

        ASSERT(containerReference.ElementIndex < firstItemIndexInGroup + itemCountInGroup);
        int itemInGroup = containerReference.ElementIndex - firstItemIndexInGroup;
        int itemsLeftInThisGroup = itemCountInGroup - itemInGroup - 1;
        ASSERT(itemsLeftInThisGroup >= 0);

        const float averageContainerSize = GetAverageContainerSize();
        referencePoint += averageContainerSize * itemsLeftInThisGroup;

        // Make sure we don't have a header that's bigger than this
        float headerEnd = headerReference.ElementBounds.*PointFromRectInVirtualizingDirection() + referenceHeaderExtent;
        if (headerEnd > referencePoint)
        {
            referencePoint = headerEnd;
        }

        // Take the group padding at the end of this group into account
        referencePoint += GetGroupPaddingAtEnd().*SizeInVirtualizingDirection();
    }
    else
    {
        // Don't have an item in this group. Just estimate it based on item count and average item sizes
        referencePoint = headerReference.ElementBounds.*PointFromRectInVirtualizingDirection();
        referencePoint += GetVirtualizedGroupExtent(static_cast<float>(itemCountInGroup), referenceHeaderExtent);

        // GetVirtualizedGroupExtent adds in the group padding on both ends, but our header has already had the left/top padding applied
        // Subtract it out so it's not counted twice
        referencePoint -= GetGroupPaddingAtStart().*SizeInVirtualizingDirection();
    }

    // Now we have a point at the end of the reference group
    // Let's estimate any remaining groups based on an average group size
    const int remainingGroups = GetRemainingGroups(headerReference.ElementIndex + 1, totalGroups, RelativePosition::Before);
    const int remainingItems = GetRemainingItems(firstItemIndexInGroup + itemCountInGroup, totalItems, RelativePosition::Before);

    if (remainingGroups > 0)
    {
        const float averageItemsPerRemainingGroup = remainingItems / static_cast<float>(remainingGroups);
        const float averageExtentOfRemainingGroups = GetAverageVirtualizedGroupExtent(averageItemsPerRemainingGroup);
        referencePoint += remainingGroups * averageExtentOfRemainingGroups;
    }

    extent.*SizeInVirtualizingDirection() = referencePoint;
    extent.*SizeInNonVirtualizingDirection() = m_extentInNonVirtualizingDirection;

    return S_OK;
}

} } }