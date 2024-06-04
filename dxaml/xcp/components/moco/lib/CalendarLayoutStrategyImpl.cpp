// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CalendarLayoutStrategyImpl.h"
#include "DoubleUtil.h"
#include "RectUtil.h"
#include "NavigationIndices.h"

// Work around disruptive max/min macros
#undef max
#undef min

namespace DirectUI { namespace Components { namespace Moco {

CalendarLayoutStrategyImpl::CalendarLayoutStrategyImpl()
    : LayoutStrategyBase(false, true /* isWrapping */)
    , m_cellSize({1, 1})
    , m_cellMinSize({0, 0})
    , m_rows(1)
    , m_cols(1)
{
}

#pragma region Layout related methods

// returns the size we should use to measure a container or header with
// itemIndex - indicates an index of valid item or -1 for general, non-special items
wf::Size
CalendarLayoutStrategyImpl::GetElementMeasureSize(
    _In_ xaml_controls::ElementType elementType,
    _In_ int elementIndex,
    _In_ wf::Rect windowConstraint)
{
    if (elementType == xaml_controls::ElementType_ItemContainer)
    {
        return m_cellSize;
    }
    else
    {
        // we don't support header but MCBP still asks for header size.
        return { -1, -1 };
    }
}

wf::Rect
CalendarLayoutStrategyImpl::GetElementBounds(
    _In_ xaml_controls::ElementType elementType,
    _In_ int elementIndex,
    _In_ wf::Size containerDesiredSize,
    _In_ xaml_controls::LayoutReference referenceInformation,
    _In_ wf::Rect windowConstraint)
{
    ASSERT(elementType == xaml_controls::ElementType_ItemContainer);

    // Nongrouping, this is exact and we don't even need the reference position
    return GetItemBounds(elementIndex);
}
 
wf::Rect
CalendarLayoutStrategyImpl::GetElementArrangeBounds(
    _In_ xaml_controls::ElementType elementType,
    _In_ int elementIndex,
    _In_ wf::Rect containerBounds,
    _In_ wf::Rect windowConstraint,
    _In_ wf::Size finalSize)
{
    ASSERT(elementType == xaml_controls::ElementType_ItemContainer);
    // For calendar item, the arrange bound is same as measure bound so we can always call GetItemBounds
    // to get the arrange bounds, but we can simply use measure bound (containerBounds) because they are same.

    // However for garbage we can't call GetItemBounds because garbage index is -1 which will cause the 
    // rect computed from GetItemBounds is just one item rect away from the first item. (when we pan fast or when
    // the first item is not at {0,0}, we can see the garbage).
    // fortunately for garbage we can use measure bound (containerBounds) as its arrange bound as well, because
    // garbage item's measure bound is far away from visible window and we don't care the arrange size of a garbage item.
#ifdef DBG
    if (elementIndex >= 0)
    {
        // make sure the measure bound is arrange bound for non-garbage.
        auto bounds = GetItemBounds(elementIndex);
        ASSERT(bounds.X == containerBounds.X
            && bounds.Y == containerBounds.Y
            && bounds.Width == containerBounds.Width
            && bounds.Height == containerBounds.Height);
    }
#endif

    return containerBounds;
}

bool CalendarLayoutStrategyImpl::ShouldContinueFillingUpSpace(
    _In_ xaml_controls::ElementType elementType,
    _In_ int elementIndex,
    _In_ xaml_controls::LayoutReference referenceInformation,
    _In_ wf::Rect windowToFill)
{
    bool shouldContinue = false;

    ASSERT(elementType == xaml_controls::ElementType_ItemContainer);

    if (referenceInformation.RelativeLocation == xaml_controls::ReferenceIdentity_Myself)
    {
        // always do yourself
        shouldContinue = true;
    }
    else
    {
        int stackingLines = 0;
        int virtualizingLine = 0;
        int stackingLine = 0;

        int visualIndex = m_indexCorrectionTable.ActualIndexToVisualIndex(elementIndex);
        DetermineLineInformation(visualIndex, &stackingLines, &virtualizingLine, &stackingLine);

        if (referenceInformation.RelativeLocation == xaml_controls::ReferenceIdentity_BeforeMe)
        {
            if (stackingLine == 0)
            {
                // we're in a new row, so is there room to the bottom?
                shouldContinue = windowToFill.*PointFromRectInVirtualizingDirection() + windowToFill.*SizeFromRectInVirtualizingDirection()
                            > referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection() + referenceInformation.ReferenceBounds.*SizeFromRectInVirtualizingDirection();
            }
            else
            {
                shouldContinue = windowToFill.*PointFromRectInVirtualizingDirection() + windowToFill.*SizeFromRectInVirtualizingDirection() > referenceInformation.ReferenceBounds.*PointFromRectInVirtualizingDirection();
            }
        }
        else // AfterMe
        {
            if (stackingLine == stackingLines - 1)
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

    return shouldContinue;
}

#pragma endregion

#pragma region Estimation and virtualization related methods.

// Estimate how many items we need to traverse to get from our reference point to a suitable anchor item for the window.
_Check_return_ HRESULT
CalendarLayoutStrategyImpl::EstimateElementIndex(
    _In_ xaml_controls::ElementType elementType,
    _In_ xaml_controls::EstimationReference headerReference,
    _In_ xaml_controls::EstimationReference containerReference,
    _In_ wf::Rect window,
    _Out_ wf::Rect* pTargetRect,
    _Out_ INT* pReturnValue)
{
    *pReturnValue = -1;
    int totalItems = 0;

    ASSERT(elementType == xaml_controls::ElementType_ItemContainer);
    IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalItemCount(&totalItems));

    const int maxStackingLines = (totalItems > 0) ? DetermineMaxStackingLine() : 1;

    // We are non-grouped, this is an exact calculation
    ASSERT(maxStackingLines > 0);

    // How many virtualizing lines does it take to get from the start to here?
    const float virtualizingDistanceFromStart = std::max(0.0f, window.*PointFromRectInVirtualizingDirection());
    int virtualizingLineDistance = static_cast<int>(virtualizingDistanceFromStart / m_cellSize.*SizeInVirtualizingDirection());

    const float stackingDistanceFromStart = std::max(0.0f, window.*PointFromRectInNonVirtualizingDirection());
    int stackingLineDistance = static_cast<int>(stackingDistanceFromStart / m_cellSize.*SizeInNonVirtualizingDirection());

    int targetVisualIndex = virtualizingLineDistance * maxStackingLines + stackingLineDistance;

    // clamp targetvisualIndex.

    int firstVisualIndex = m_indexCorrectionTable.ActualIndexToVisualIndex(0);
    int lastVisualIndex = m_indexCorrectionTable.ActualIndexToVisualIndex(totalItems - 1);
    targetVisualIndex = std::min(targetVisualIndex, lastVisualIndex);
    targetVisualIndex = std::max(targetVisualIndex, firstVisualIndex);

    // With the final index, calculate its position
    int ignored;
    int virtualizingLine;
    int stackingLine;
    DetermineLineInformation(targetVisualIndex, &ignored, &virtualizingLine, &stackingLine);

    *pReturnValue = m_indexCorrectionTable.VisualIndexToActualIndex(targetVisualIndex);

    pTargetRect->*PointFromRectInVirtualizingDirection() = virtualizingLine * m_cellSize.*SizeInVirtualizingDirection();
    pTargetRect->*PointFromRectInNonVirtualizingDirection() = GetItemStackingPosition(stackingLine);
    
    return S_OK;
}

_Check_return_ HRESULT CalendarLayoutStrategyImpl::EstimateElementBounds(
    _In_ xaml_controls::ElementType elementType,
    _In_ int elementIndex,
    _In_ xaml_controls::EstimationReference headerReference,
    _In_ xaml_controls::EstimationReference containerReference,
    _In_ wf::Rect window,
    _Out_ wf::Rect* pReturnValue)
{
    int totalItems;

    ASSERT(elementType == xaml_controls::ElementType_ItemContainer);
    IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalItemCount(&totalItems));
    ASSERT(0 <= elementIndex && elementIndex < totalItems);

    int maxStackingLines;
    int virtualizingLine;
    int stackingLine;
    
    auto visualIndex = m_indexCorrectionTable.ActualIndexToVisualIndex(elementIndex);
    DetermineLineInformation(visualIndex, &maxStackingLines, &virtualizingLine, &stackingLine);
    ASSERT(maxStackingLines > 0);

    pReturnValue->*PointFromRectInVirtualizingDirection() = virtualizingLine * m_cellSize.*SizeInVirtualizingDirection();
    pReturnValue->*PointFromRectInNonVirtualizingDirection() = GetItemStackingPosition(stackingLine);
    pReturnValue->Width = m_cellSize.Width;
    pReturnValue->Height = m_cellSize.Height;

    return S_OK;
}

_Check_return_ HRESULT CalendarLayoutStrategyImpl::EstimatePanelExtent(
    _In_ xaml_controls::EstimationReference lastHeaderReference,
    _In_ xaml_controls::EstimationReference lastContainerReference,
    _In_ wf::Rect windowConstraint, 
    _Out_ wf::Size* pExtent)
{
    int totalItems = 0;
    IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalItemCount(&totalItems));


    const int maxStackingLine = DetermineMaxStackingLine();
    ASSERT(maxStackingLine > 0);

    //actual panel size
    pExtent->*SizeInVirtualizingDirection() = GetVirtualizedExtentOfItems(totalItems, maxStackingLine);
    pExtent->*SizeInNonVirtualizingDirection() = GetItemStackingPosition(std::min(maxStackingLine, totalItems));

    return S_OK;
}

#pragma endregion

#pragma region IItemLookupPanel related

// Based on current element's index/type and action, return the next element index/type.
_Check_return_ HRESULT CalendarLayoutStrategyImpl::GetTargetIndexFromNavigationAction(
    _In_ xaml_controls::ElementType elementType,
    _In_ int elementIndex,
    _In_ xaml_controls::KeyNavigationAction action,
    _In_ wf::Rect windowConstraint,
    _Out_ xaml_controls::ElementType* pTargetElementType,
    _Out_ INT* pTargetElementIndex)
{
    int totalItems;
    int totalGroups;
    IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalItemCount(&totalItems));
    IFC_RETURN(GetLayoutDataInfoProviderNoRef()->GetTotalGroupCount(&totalGroups));

    ASSERT(0 <= elementIndex && elementIndex < totalItems);
    ASSERT(elementType == xaml_controls::ElementType_ItemContainer);

    *pTargetElementType = xaml_controls::ElementType_ItemContainer;

    if (action != xaml_controls::KeyNavigationAction_Left &&
        action != xaml_controls::KeyNavigationAction_Right &&
        action != xaml_controls::KeyNavigationAction_Up &&
        action != xaml_controls::KeyNavigationAction_Down)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    int step = (action == xaml_controls::KeyNavigationAction_Left || action == xaml_controls::KeyNavigationAction_Up) ? -1 : 1;
    *pTargetElementIndex = elementIndex;

    // Easy case: the action is along layout orientation, therefore handle it.
    if ((GetVirtualizationDirection() == xaml_controls::Orientation_Vertical && (action == xaml_controls::KeyNavigationAction_Left || action == xaml_controls::KeyNavigationAction_Right)) ||
        (GetVirtualizationDirection() == xaml_controls::Orientation_Horizontal && (action == xaml_controls::KeyNavigationAction_Up || action == xaml_controls::KeyNavigationAction_Down)))
    {
        *pTargetElementIndex = std::min(std::max(elementIndex + step, 0), totalItems - 1);
    }
    // The action is not in layout direction.  We must jump to the next item in the same row/column.
    // We are not grouping, easy.
    else 
    {
        // TODO: verify startAt
        const int maxLineLength = DetermineMaxStackingLine();
        const int nextIndex = elementIndex + step * maxLineLength;
        if (0 <= nextIndex && nextIndex < totalItems)
        {
            *pTargetElementIndex = nextIndex;
        }
    }

    ASSERT(0 <= *pTargetElementIndex && *pTargetElementIndex < totalItems);

    return S_OK;
}

#pragma endregion

#pragma region Snap points related

bool CalendarLayoutStrategyImpl::GetRegularSnapPoints(
    _Out_ float* pNearOffset,
    _Out_ float* pFarOffset,
    _Out_ float* pSpacing)
{
    *pNearOffset = 0.0f;
    *pFarOffset = 0.0f;
    *pSpacing = m_cellSize.*SizeInVirtualizingDirection();
    
    // when there is no snapPointFilterFunction, we have regular snap points.
    return !m_snapPointFilterFunction; /*hasRegularSnapPoints*/;
}

bool CalendarLayoutStrategyImpl::HasIrregularSnapPoints(
    _In_ xaml_controls::ElementType elementType)
{
    return !!m_snapPointFilterFunction;
}

_Check_return_ HRESULT CalendarLayoutStrategyImpl::HasSnapPointOnElement(
    _In_ xaml_controls::ElementType elementType,
    _In_ INT elementIndex,
    _Out_ bool& hasSnapPointOnElement)
{
    hasSnapPointOnElement = false;
    ASSERT(m_snapPointFilterFunction);
    
    IFC_RETURN(m_snapPointFilterFunction(elementIndex, &hasSnapPointOnElement));

    return S_OK;
}

#pragma endregion

void CalendarLayoutStrategyImpl::DetermineLineInformation(_In_ int visualIndex, _Out_ int* pStackingLines, _Out_ int* pVirtualizingLine, _Out_ int* pStackingLine)  const
{
    int stackingLines = DetermineMaxStackingLine();

    int virtualizingLine = visualIndex / stackingLines;
    int stackingLine = visualIndex - virtualizingLine * stackingLines;

    *pStackingLines = stackingLines;
    *pVirtualizingLine = virtualizingLine;
    *pStackingLine = stackingLine;
}

int CalendarLayoutStrategyImpl::DetermineMaxStackingLine() const
{
    switch (GetVirtualizationDirection())
    {
    case xaml_controls::Orientation_Horizontal:
        return m_rows;
    case xaml_controls::Orientation_Vertical:
        return m_cols;
    default:
        ASSERT(FALSE);
        return 0;
    }
}

float CalendarLayoutStrategyImpl::GetVirtualizedExtentOfItems(_In_ int itemCount, _In_ int maxStackingLine) const
{
    ASSERT(maxStackingLine > 0);

    // Get virtualizing lines, rounding up the fractional ones
    float extent = 0;
    
    if (itemCount > 0)
    {
        int lastIndex = itemCount - 1;
        int lastVisualIndex = m_indexCorrectionTable.ActualIndexToVisualIndex(lastIndex);
        int virtualizingLines = lastVisualIndex / maxStackingLine + 1;
        extent += m_cellSize.*SizeInVirtualizingDirection() * virtualizingLines;
    }

    return extent;
}

float CalendarLayoutStrategyImpl::GetItemStackingPosition(_In_ int stackingLine) const
{
    float result = stackingLine * m_cellSize.*SizeInNonVirtualizingDirection();

    return result;
}

// check if our items can perfectly fit in the given viewport
// if not, we'll update the cell size and ask for an additional measure pass.
void CalendarLayoutStrategyImpl::SetViewportSize(_In_ wf::Size size, _Out_ bool* pNeedsRemeasure)
{
    *pNeedsRemeasure = false;

    float newWidth = size.Width / m_cols;
    float newHeight = size.Height / m_rows;


    // The newSize should be always greater than or equal to the minSize. 
    // However under some scale factors, we need to use "close to" to replace "equal to".
    const float epsilon = 0.0001f;
    ASSERT(newWidth > m_cellMinSize.Width || DoubleUtil::AreWithinTolerance(newWidth, m_cellMinSize.Width, epsilon));
    ASSERT(newHeight > m_cellMinSize.Height || DoubleUtil::AreWithinTolerance(newHeight, m_cellMinSize.Height, epsilon));

    if (newWidth != m_cellSize.Width || newHeight != m_cellSize.Height)
    {
        // Always make sure that we do not end up with 0 width or height. That can
        // end up devirtualizing the entire calendarview.
        m_cellSize.Width = newWidth == 0 ? m_cellMinSize.Width : newWidth;
        m_cellSize.Height = newHeight == 0 ? m_cellMinSize.Height: newHeight;
        *pNeedsRemeasure = true;
    }
}

// our Parent(SCP)'s  desired size is determined by the min cell size.
wf::Size CalendarLayoutStrategyImpl::GetDesiredViewportSize() const
{
    wf::Size desiredViewportSize{ m_cellMinSize.Width * m_cols, m_cellMinSize.Height * m_rows };
    return desiredViewportSize;
}

void CalendarLayoutStrategyImpl::SetItemMinimumSize(_In_ wf::Size size, _Out_ bool* pNeedsRemeasure)
{
    *pNeedsRemeasure = false;

    if (m_cellMinSize.Width != size.Width || m_cellMinSize.Height != size.Height)
    {
        m_cellMinSize = size;
        // once cell minsize is updated, we also update cell size and ask for an additional measure pass.
        m_cellSize = m_cellMinSize;
        *pNeedsRemeasure = true;
    }
}

wf::Rect CalendarLayoutStrategyImpl::GetItemBounds(_In_ int index)
{
    wf::Rect bounds = {};
    int stackingLines = 0;
    int stackingLine = 0;
    int virtualizingLine = 0;

    int visualIndex = m_indexCorrectionTable.ActualIndexToVisualIndex(index);
    DetermineLineInformation(visualIndex, &stackingLines, &virtualizingLine, &stackingLine);

    bounds.*PointFromRectInVirtualizingDirection() = virtualizingLine * m_cellSize.*SizeInVirtualizingDirection();
    bounds.*PointFromRectInNonVirtualizingDirection() = GetItemStackingPosition(stackingLine);
    bounds.Width = m_cellSize.Width;
    bounds.Height = m_cellSize.Height;

    return bounds;
}


void CalendarLayoutStrategyImpl::IndexCorrectionTable::SetCorrectionEntryForSkippedDay(_In_ int index, _In_ int correction)
{
    ASSERT(index >= 0);
    ASSERT(correction >= 0);    // it is a skip day, so the correction must be non-negative number.
    m_indexCorrectionTable[1].first = index;
    m_indexCorrectionTable[1].second = correction;
}

void CalendarLayoutStrategyImpl::IndexCorrectionTable::SetCorrectionEntryForElementStartAt(_In_ int correction)
{
    ASSERT(correction >= 0);
    m_indexCorrectionTable[0].first = 0; // this is always 0, which means this correction applies for all items.
    m_indexCorrectionTable[0].second = correction;
}

int CalendarLayoutStrategyImpl::IndexCorrectionTable::VisualIndexToActualIndex(_In_ int visualIndex) const
{
    ASSERT(m_indexCorrectionTable[0].first <= m_indexCorrectionTable[1].first);  // always in order.
    int actualIndex = visualIndex;
    for (auto& entry : m_indexCorrectionTable)
    {
        if (actualIndex >= entry.first)
        {
            ASSERT(actualIndex >= entry.first + entry.second);
            actualIndex -= entry.second;
        }
        else
        {
            break;
        }
    }
    return actualIndex;
}

int CalendarLayoutStrategyImpl::IndexCorrectionTable::ActualIndexToVisualIndex(_In_ int actualIndex) const
{
    ASSERT(m_indexCorrectionTable[0].first <= m_indexCorrectionTable[1].first);  // always in order.
    int visualIndex = actualIndex;
    for (auto& entry : m_indexCorrectionTable)
    {
        if (actualIndex >= entry.first)
        {
            visualIndex += entry.second;
        }
        else
        {
            break;
        }
    }
    return visualIndex;
}

} } }
