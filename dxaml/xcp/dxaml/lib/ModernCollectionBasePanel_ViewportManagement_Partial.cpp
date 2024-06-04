// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ModernCollectionBasePanel.g.h"
#include "ItemsPresenter.g.h"
#include "namespacealiases.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

using xaml_controls::LayoutReference;
using xaml_controls::EstimationReference;

// Work around disruptive max/min macros
#undef max
#undef min

// When called the first time, this method starts tracking the first
// visible element. Otherwise, it does nothing.
_Check_return_ HRESULT
ModernCollectionBasePanel::BeginTrackingFirstVisibleElement(bool ignoreIsTrackingExtentEnd)
{
    INT32 index = -1;
    xaml_controls::ElementType type = xaml_controls::ElementType_ItemContainer;

    ctl::ComPtr<IUIElement> spFirstVisibleElement;
    const wf::Rect window = m_windowState.GetVisibleWindow();
    auto spScrollViewer = m_wrScrollViewer.AsOrNull<IScrollViewer>();

    const ItemsUpdatingScrollMode itemsUpdatingScrollMode = GetItemsUpdatingScrollMode();

    // We only support ItemsStackPanel for this viewport behavior.
    // Also, if we are already tracking something, we can leave.
    if (!IsMaintainViewportSupportedAndEnabled() ||
        m_viewportBehavior.isTracking ||
        (!ignoreIsTrackingExtentEnd && m_viewportBehavior.isTrackingExtentEnd) ||
        !spScrollViewer)
    {
#ifdef IUSM_DEBUG
        IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG,
            L"IUSM_DEBUG[0x%p]: ModernCollectionBasePanel::BeginTrackingFirstVisibleElement. Early exit.", m_viewportBehavior));
#endif
        return S_OK;
    }

#ifdef IUSM_DEBUG
    if (ignoreIsTrackingExtentEnd && m_viewportBehavior.isTrackingExtentEnd)
    {
        IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG,
            L"IUSM_DEBUG[0x%p]: ModernCollectionBasePanel::BeginTrackingFirstVisibleElement. Ignores isTrackingExtentEnd & skips early exit.", m_viewportBehavior));
    }
#endif

    const float windowFirstEdge = window.*PointFromRectInVirtualizingDirection();
    const float windowSecondEdge = windowFirstEdge + window.*SizeFromRectInVirtualizingDirection();

    // If we are at the very beginning and we are tracking the first visible element,
    // we don't have to do anything. KeepItemsInView actually means Keep*First*ItemInView
    // because we added KeepLastItemInView in RS1.
    if (itemsUpdatingScrollMode == ItemsUpdatingScrollMode::KeepItemsInView &&
       (windowFirstEdge <= 0.0f || DoubleUtil::IsZero(static_cast<double>(windowFirstEdge))))
    {
        return S_OK;
    }

    // On the other hand, if we are at the very end and the tracking mode is KeepLastItemInView,
    // we need to take note and scroll at the "new" end (if it's different from the current) so
    // that we have a behavior that's symmetric to KeepItemsInView.
    if (itemsUpdatingScrollMode == ItemsUpdatingScrollMode::KeepLastItemInView)
    {
        // We use m_estimatedSize instead of get_DesiredSize because the latter accounts for layout rounding.
        const wf::Size extent = m_estimatedSize;
        const float extentInVirtualizingDirection = extent.*PointFromSizeInVirtualizingDirection();
        
        if (static_cast<double>(extentInVirtualizingDirection) <= static_cast<double>(windowSecondEdge) + 0.5)
        {
            m_viewportBehavior.isTrackingExtentEnd = TRUE;
            m_viewportBehavior.originalExtent = m_viewportBehavior.currentExtent = extentInVirtualizingDirection;

#ifdef IUSM_DEBUG
            m_viewportBehavior.TraceChangesDbg(L"ModernCollectionBasePanel::BeginTrackingFirstVisibleElement for KeepLastItemInView.");
#endif
        }
    }

    // Finds the first visible element, if any.
    IFC_RETURN(GetFirstVisibleElement(&type, &index, &spFirstVisibleElement));

    if (spFirstVisibleElement)
    {
        m_viewportBehavior.isTracking = TRUE;
        m_viewportBehavior.index = index;
        m_viewportBehavior.type = type;
        m_viewportBehavior.initialViewportEdge =
            itemsUpdatingScrollMode == ItemsUpdatingScrollMode::KeepItemsInView ? windowFirstEdge : windowSecondEdge;
        m_viewportBehavior.elementBounds = GetBoundsFromElement(spFirstVisibleElement);

#ifdef IUSM_DEBUG
        m_viewportBehavior.TraceChangesDbg(L"ModernCollectionBasePanel::BeginTrackingFirstVisibleElement for FirstVisibleElement.");
#endif

        SetTrackedElementOffsetRelativeToViewport();
    }

    return S_OK;
}

// Stores the first visible element.
// We need this method on orientation change to remember the first visible element in the visible window.
// Then, when Generate() next runs (asynchronously) with the new orientation set and new realization window,
// we can center the visible window around this element that we cached.
// When switching orientations, we thus preserve the first element in the visible window.
// Note that we do not preserve offset into the first visible element.
_Check_return_ HRESULT
ModernCollectionBasePanel::CacheFirstVisibleElementBeforeOrientationChange()
{
    HRESULT hr = S_OK;

    INT32 index = -1;
    xaml_controls::ElementType type = xaml_controls::ElementType_ItemContainer;

    ctl::ComPtr<IUIElement> spFirstVisibleElement;

    m_tpFirstVisibleElementBeforeOrientationChange.Clear();
    m_typeOfFirstVisibleElementBeforeOrientationChange = xaml_controls::ElementType_ItemContainer;
    m_indexOfFirstVisibleElementBeforeOrientationChange = -1;

    // Finds the first visible element, if any.
    IFC(GetFirstVisibleElement(&type, &index, &spFirstVisibleElement));
    if (spFirstVisibleElement)
    {
        ASSERT(index >= 0);
        SetPtrValue(m_tpFirstVisibleElementBeforeOrientationChange, spFirstVisibleElement);
        m_typeOfFirstVisibleElementBeforeOrientationChange = type;
        m_indexOfFirstVisibleElementBeforeOrientationChange = index;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ModernCollectionBasePanel::BeginTrackingOnOrientationChange(_Out_ wf::Rect* pComputedWindow)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<IUIElement> spFirstVisibleElement = m_tpFirstVisibleElementBeforeOrientationChange.Get();
    wf::Rect window = m_windowState.GetVisibleWindow();

    ctl::ComPtr<IUIElement> spCurrentElementAtCachedIndex;

    // if we had mutations, we kind of give up on tracking (given milestone that we are in).
    IFC(m_containerManager.GetElementAtDataIndex(
        m_typeOfFirstVisibleElementBeforeOrientationChange,
        m_indexOfFirstVisibleElementBeforeOrientationChange,
        &spCurrentElementAtCachedIndex));

    if (spFirstVisibleElement && spFirstVisibleElement.Get() == spCurrentElementAtCachedIndex.Get())
    {
        wf::Rect elementRect;
        EstimationReference resolvedHeaderReference = CreateDefaultEstimationReference(xaml_controls::ElementType_GroupHeader);
        EstimationReference resolvedContainerReference = CreateDefaultEstimationReference(xaml_controls::ElementType_ItemContainer);
        xaml_controls::Orientation orientation;
        if (m_typeOfFirstVisibleElementBeforeOrientationChange == xaml_controls::ElementType_ItemContainer)
        {
            IFC(m_spLayoutStrategy->EstimateElementBounds(
                xaml_controls::ElementType_ItemContainer,
                m_indexOfFirstVisibleElementBeforeOrientationChange,
                resolvedHeaderReference,
                resolvedContainerReference,
                m_windowState.GetRealizationWindow(),
                &elementRect));

        }
        else
        {
            ASSERT(m_typeOfFirstVisibleElementBeforeOrientationChange == xaml_controls::ElementType_GroupHeader);

            IFC(m_spLayoutStrategy->EstimateElementBounds(
                xaml_controls::ElementType_GroupHeader,
                m_cacheManager.DataIndexToLayoutIndex(xaml_controls::ElementType_GroupHeader, m_indexOfFirstVisibleElementBeforeOrientationChange),
                resolvedHeaderReference,
                resolvedContainerReference,
                m_windowState.GetRealizationWindow(),
                &elementRect));
        }
        SetBoundsForElement(spFirstVisibleElement, elementRect);

        IFC(PlaceWindowAroundElementRect(m_windowState.GetVisibleWindow(), elementRect, xaml_controls::ScrollIntoViewAlignment_Leading, 0.0 /* offset */, &window));

        IFC(m_spLayoutStrategy->GetVirtualizationDirection(&orientation));
        if (orientation == xaml_controls::Orientation_Horizontal)
        {
            // we are scrolling horizontally
            window.Y = 0;
        }
        else
        {
            window.X = 0;
        }

        m_viewportBehavior.isTracking = TRUE;
        m_viewportBehavior.index = m_indexOfFirstVisibleElementBeforeOrientationChange;
        m_viewportBehavior.type = m_typeOfFirstVisibleElementBeforeOrientationChange;
        m_viewportBehavior.initialViewportEdge =
            GetItemsUpdatingScrollMode() == ItemsUpdatingScrollMode::KeepLastItemInView ?
            window.*PointFromRectInVirtualizingDirection() + window.*SizeFromRectInVirtualizingDirection() :
            window.*PointFromRectInVirtualizingDirection();
        m_viewportBehavior.elementBounds = GetBoundsFromElement(spFirstVisibleElement);

#ifdef IUSM_DEBUG
        m_viewportBehavior.TraceChangesDbg(L"ModernCollectionBasePanel::BeginTrackingOnOrientationChange.");
#endif

        SetTrackedElementOffsetRelativeToViewport();
    }

    *pComputedWindow = window;

Cleanup:
    m_tpFirstVisibleElementBeforeOrientationChange.Clear();
    m_typeOfFirstVisibleElementBeforeOrientationChange = xaml_controls::ElementType_ItemContainer;
    m_indexOfFirstVisibleElementBeforeOrientationChange = -1;

    RRETURN(hr);
}

_Check_return_ HRESULT
ModernCollectionBasePanel::BeginTrackingOnRefresh(_Out_ wf::Rect* newVisibleWindow)
{
    if (m_viewportBehavior.isTracking)
    {
        // Early out if we are already tracking.
        return S_OK;
    }

    if ((m_cacheManager.IsGrouping() && m_cacheManager.GetTotalLayoutGroupCount() == 0)
        || (!m_cacheManager.IsGrouping() && m_cacheManager.GetTotalItemCount() == 0))
    {
        // If the data is empty, we get out as well.
        return S_OK;
    }

    // Begin tracking when KeepLastItemInView is set and
    // either m_refreshPendingLayout is true or we are in a
    // reset like scenario where the data isn't empty but there
    // is no container realized yet.
    if (GetItemsUpdatingScrollMode() == ItemsUpdatingScrollMode::KeepLastItemInView &&
        (m_refreshPendingLayout ||
         (m_containerManager.GetValidContainerCount() == 0 &&
          m_containerManager.GetValidHeaderCount() == 0)))
    {
        int scrollToTargetIndex;
        xaml_controls::ElementType scrollToTargetType;

        // Find the index and type of the last element.
        IFC_RETURN(m_cacheManager.GetLastElementInLayout(&scrollToTargetIndex, &scrollToTargetType));
        ASSERT(scrollToTargetIndex != -1);

        // Scroll to the last element.
        if (scrollToTargetType == xaml_controls::ElementType::ElementType_GroupHeader)
        {
            IFC_RETURN(DoScrollGroupHeaderIntoView(
                scrollToTargetIndex,
                xaml_controls::ScrollIntoViewAlignment_Default,
                0.0 /* offset */,
                false /* animate */,
                newVisibleWindow));
        }
        else
        {
            IFC_RETURN(DoScrollItemIntoView(
                scrollToTargetIndex,
                xaml_controls::ScrollIntoViewAlignment_Default,
                0.0 /* offset */,
                false /* animate */,
                newVisibleWindow));
        }

#ifdef IUSM_DEBUG
        IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG,
            L"IUSM_DEBUG[0x%p]: ModernCollectionBasePanel::BeginTrackingOnRefresh. Initiated scroll to index %d.", m_viewportBehavior, scrollToTargetIndex));
#endif

        m_viewportBehavior.isTrackingExtentEnd = TRUE;
        m_viewportBehavior.originalExtent = m_viewportBehavior.currentExtent = m_estimatedSize.*PointFromSizeInVirtualizingDirection();

#ifdef IUSM_DEBUG
        m_viewportBehavior.TraceChangesDbg(L"ModernCollectionBasePanel::BeginTrackingOnRefresh.");
#endif

        auto itemsPresenter = m_wrItemsPresenter.AsOrNull<IItemsPresenter>();
        if (itemsPresenter)
        {
            IFC_RETURN(itemsPresenter.Cast<ItemsPresenter>()->LoadFooter(false /* updateLayout */));
        }
    }
    return S_OK;
}

// Starts tracking the first visible element if it's not already the case.
// If we already tracking something, it updates the tracked element so
// we can deal with its removal or replacement.
_Check_return_ HRESULT
ModernCollectionBasePanel::BeginTrackingFirstVisibleElement(
    _In_ bool isCollectionChange,
    _In_ bool isGroupChange,
    _In_ INT index,
    _In_ wfc::CollectionChange action)
{
    HRESULT hr = S_OK;
    BOOLEAN wasTrackingBeforeThisCall = m_viewportBehavior.isTracking;

    if (isCollectionChange && action == wfc::CollectionChange_Reset)
        goto Cleanup;

    IFC(BeginTrackingFirstVisibleElement());

    // We should keep the tracked element's index up to date.
    if (m_viewportBehavior.isTracking &&
        isCollectionChange &&
        ((isGroupChange && m_viewportBehavior.type == xaml_controls::ElementType_GroupHeader) ||
        (!isGroupChange && m_viewportBehavior.type == xaml_controls::ElementType_ItemContainer)) &&
        index <= m_viewportBehavior.index)
    {
        switch (action)
        {
            // an element (group or item) was inserted before the tracked element.
            // we only need to update our index in this case.
            case wfc::CollectionChange_ItemInserted:
            {
                ++m_viewportBehavior.index;
                break;
            }
            // an element (group or item) was removed.
            // if it was removed before the tracked element, we only need to update the index.
            // if the tracked element was removed, we need to pick a new one.
            // by convention, it's the realized element after.
            case wfc::CollectionChange_ItemRemoved:
            {
                if (index == m_viewportBehavior.index)
                {
                    xaml_controls::ElementType oldType = m_viewportBehavior.type;
                    IFC(ElectNewTrackedElement());

                    // -1 to account for the old tracked element that hasn't been removed yet.
                    if (m_viewportBehavior.isTracking &&
                        m_viewportBehavior.type == oldType &&
                        GetItemsUpdatingScrollMode() == ItemsUpdatingScrollMode::KeepItemsInView)
                    {
                        --m_viewportBehavior.index;
                    }
                }
                else
                {
                    --m_viewportBehavior.index;
                }
                break;
            }
            // if the first group in the valid range is replaced by a non-empty group and we are tracking
            // the group header of the old group, we should not be tracking the group header of the new group
            // because the latter is going to be a floating header and we can't generate forward from a floating header.
            case wfc::CollectionChange_ItemChanged:
            {
                if (isGroupChange && index == m_viewportBehavior.index && index == m_containerManager.GetGroupIndexFromValidIndex(0))
                {
                    INT32 itemCountInGroup = 0;
                    IFC(GetGroupInformationFromGroupIndex(index, nullptr /* pStartItemIndex */, &itemCountInGroup));
                    if (itemCountInGroup > 0)
                    {
                        IFC(ElectNewTrackedElement());
                    }
                }
                break;
            }
        }
    }

    // If an element (container or header) was inserted or removed or changed size before the tracked element.
    // We need to reevaluate the layout of the tracked element, using estimations.
    if (m_viewportBehavior.isTracking &&
        (!isCollectionChange ||
        action == wfc::CollectionChange_ItemInserted ||
        action == wfc::CollectionChange_ItemRemoved))
    {
        bool isChangeBeforeTrackedElement = false;
        bool isChangeGoingToHideTrackedGroupHeader = false;

        // Is the change before the tracked element?
        IFC(IsChangeBeforeTrackedElement(isCollectionChange, isGroupChange, index, action, &isChangeBeforeTrackedElement, &isChangeGoingToHideTrackedGroupHeader));
        if (isChangeBeforeTrackedElement)
        {
            if (isCollectionChange)
            {
                float estimatedElementBoundsShift = 0.0f;

                switch (action)
                {
                    case wfc::CollectionChange_ItemInserted:
                    case wfc::CollectionChange_ItemRemoved:
                    {
                        if (isGroupChange)
                        {
                            // First, only account for the header itself when it's inline.
                            GroupHeaderStrategy headerStrategy = GroupHeaderStrategy::None;

                            IFC(GetGroupHeaderStrategy(&headerStrategy));

                            if (headerStrategy == GroupHeaderStrategy::Inline)
                            {
                                // A group header was inserted before the tracked element.
                                // Attempt to access an average header size in the virtualized direction.
                                IFC(GetAverageHeaderSize(&estimatedElementBoundsShift));
                            }
                            // Else headers that are placed in Parallel and have no effect on the tracked element's position.
                        }
                        else
                        {
                            // A container was inserted before the tracked element.
                            // Attempt to access an average container size in the virtualized direction.
                            IFC(GetAverageContainerSize(&estimatedElementBoundsShift));
                        }

                        // When estimatedElementBoundsShift >= 0.0f, use that successfully accessed average size as the estimated shift...
                        if (estimatedElementBoundsShift < 0.0f)
                        {
                            // ...else fall back to using the tracked element's size.
                            estimatedElementBoundsShift = m_viewportBehavior.elementBounds.*SizeFromRectInVirtualizingDirection();
                        }

                        if (isGroupChange)
                        {
                            // Second, take the GroupPadding into account irrespective of the GroupHeaderStrategy value.
                            double groupPadding = 0.0;

                            IFC(GroupPaddingSizeInVirtualizingDirection(&groupPadding));

                            estimatedElementBoundsShift += static_cast<float>(groupPadding);
                        }
                        break;
                    }
                }

                switch (action)
                {
                    case wfc::CollectionChange_ItemInserted:
                    {
                        // When an item was inserted before the tracked element, its estimated bounds are increased.
                        m_viewportBehavior.elementBounds.*PointFromRectInVirtualizingDirection() += estimatedElementBoundsShift;
                        break;
                    }

                    case wfc::CollectionChange_ItemRemoved:
                    {
                        // When an item was inserted before the tracked element, its estimated bounds are decreased.
                        m_viewportBehavior.elementBounds.*PointFromRectInVirtualizingDirection() -= estimatedElementBoundsShift;
                        m_viewportBehavior.elementBounds.*PointFromRectInVirtualizingDirection() = std::max(m_viewportBehavior.elementBounds.*PointFromRectInVirtualizingDirection(), 0.0f);
                        break;
                    }
                }
            }
        }
        // Before this call, we were not tracking and it turns out the modification (collection change or size change)
        // happened *after* the first visible element. For these cases, we don't have to do anything...
        // ...except in one specific situation: HidesIfEmpty is true and we are tracking a group header whose group is now empty.
        // In this case, keep tracking the header and we will elect a new one during the Generate pass when it gets destroyed.
        // We won't do it here because HidesIfEmpty's value might change by then.
        else if (!(wasTrackingBeforeThisCall || isChangeGoingToHideTrackedGroupHeader || m_viewportBehavior.isTrackingExtentEnd))
        {
            m_viewportBehavior.Reset();
        }
    }

#ifdef IUSM_DEBUG
    m_viewportBehavior.TraceChangesDbg(L"ModernCollectionBasePanel::BeginTrackingFirstVisibleElement.");
#endif

Cleanup:
    RRETURN(hr);
}

// Wrapper that calls BeginTrackingFirstVisibleElement when a collection change happens.
_Check_return_ HRESULT
ModernCollectionBasePanel::BeginTrackingOnCollectionChange(
    _In_ bool isGroupChange,
    _In_ INT index,
    _In_ wfc::CollectionChange action)
{
    RRETURN(BeginTrackingFirstVisibleElement(true /* isCollectionChange */, isGroupChange, index, action));
}

// Wrapper that calls BeginTrackingFirstVisibleElement when an element changes size.
_Check_return_ HRESULT
ModernCollectionBasePanel::BeginTrackingOnMeasureChange(_In_ bool isGroupChange, _In_ INT index)
{
    RRETURN(BeginTrackingFirstVisibleElement(false /* isCollectionChange */, isGroupChange, index));
}

// Wrapper that calls BeginTrackingFirstVisibleElement when we detect a viewport size change.
_Check_return_ HRESULT
ModernCollectionBasePanel::BeginTrackingOnViewportSizeChange(
    const wf::Rect& previousWindow,
    const wf::Rect& currentWindow)
{
    // We only need to track the first visible element (from the bottom/right) if
    // we are not already tracking, KeepLastItemInView is set and the viewport size has
    // changed. The goal is to keep the first visible element in the same position relative
    // to the viewport edge (bottom/right end of the viewport).
    if (!m_viewportBehavior.isTracking &&
        GetItemsUpdatingScrollMode() == ItemsUpdatingScrollMode::KeepLastItemInView &&
        !DoubleUtil::AreClose(
            static_cast<double>(previousWindow.*SizeFromRectInVirtualizingDirection()),
            static_cast<double>(currentWindow.*SizeFromRectInVirtualizingDirection())))
    {
        // Since m_viewportBehavior.isTrackingExtentEnd is unconditionally reset below, BeginTrackingFirstVisibleElement
        // must not exit early when m_viewportBehavior.isTrackingExtentEnd is True. Instead, it must carry on and update
        // the m_viewportBehavior fields originalExtent, isTracking, index, type, initialViewportEdge and elementBounds.
        // This is particularly critical when the viewport shrinks in both dimensions.
        IFC_RETURN(BeginTrackingFirstVisibleElement(true /*ignoreIsTrackingExtentEnd*/));
        // Don't allow tracking the extent end because it's not going to change.
        // We need to track a specific element and its offset to the bottom of the viewport
        // that just changed size.
        m_viewportBehavior.isTrackingExtentEnd = false;

#ifdef IUSM_DEBUG
        m_viewportBehavior.TraceChangesDbg(L"ModernCollectionBasePanel::BeginTrackingOnViewportSizeChange.");
#endif
    }
    return S_OK;
}

// Called during the Generate pass to shift all the elements, including the
// tracked element, to the new realization window.
_Check_return_ HRESULT
ModernCollectionBasePanel::ApplyTrackedElementShift()
{
    ctl::ComPtr<IUIElement> spTrackedElement;

    ASSERT(m_viewportBehavior.isTracking);

    IFC_RETURN(GetTrackedElement(&spTrackedElement));

    // Because of bug 14508725, GetTrackedElement may return null here even when m_viewportBehavior.isTracking is True.
    if (spTrackedElement)
    {
        m_viewportBehavior.trackedElementShift =
            m_viewportBehavior.elementBounds.*PointFromRectInVirtualizingDirection() -
            GetBoundsFromElement(spTrackedElement).*PointFromRectInVirtualizingDirection();

#ifdef IUSM_DEBUG
        m_viewportBehavior.TraceChangesDbg(L"ModernCollectionBasePanel::ApplyTrackedElementShift.");
#endif

        for (int typeIndex = 0; typeIndex < ElementType_Count; ++typeIndex)
        {
            const xaml_controls::ElementType type = static_cast<xaml_controls::ElementType>(typeIndex);
            for (int validIndex = 0; validIndex < m_containerManager.GetValidElementCount(type); ++validIndex)
            {
                ctl::ComPtr<IUIElement> spElement;
                IFC_RETURN(m_containerManager.GetElementAtValidIndex(type, validIndex, &spElement));
                if (spElement)
                {
                    UIElement::VirtualizationInformation* pVirtualizationInfo = GetVirtualizationInformationFromElement(spElement);
                    wf::Rect bounds = pVirtualizationInfo->GetBounds();
                    bounds.*PointFromRectInVirtualizingDirection() += m_viewportBehavior.trackedElementShift;
                    pVirtualizationInfo->SetBounds(bounds);
                }
            }
        }
    }

    return S_OK;
}

// Returns whether a change happened before the tracked element.
_Check_return_ HRESULT
ModernCollectionBasePanel::IsChangeBeforeTrackedElement(
    _In_ bool isCollectionChange,
    _In_ bool isGroupChange,
    _In_ INT index,
    _In_ wfc::CollectionChange action,
    _Out_ bool* pIsChangeBeforeTrackedElement,
    _Out_ bool* pIsChangeGoingToHideTrackedGroupHeader)
{
    HRESULT hr = S_OK;
    bool isChangeBeforeTrackedElement = false;
    bool isChangeGoingToHideTrackedGroupHeader = false;

    ASSERT(!isCollectionChange ||
        action == wfc::CollectionChange_ItemInserted ||
        action == wfc::CollectionChange_ItemRemoved);

    if (isCollectionChange)
    {
        if (m_viewportBehavior.type == xaml_controls::ElementType_ItemContainer)
        {
            if (isGroupChange)
            {
                // We are tracking an item and a group was added.
                if (action == wfc::CollectionChange_ItemInserted)
                {
                    INT startItemIndexOfNewGroup = 0;

                    // At this point, the cache manager know about the new group. But we haven't
                    // got the item added notification yet (we won't get any for empty groups).

                    IFC(m_cacheManager.GetGroupInformationFromGroupIndex(index, &startItemIndexOfNewGroup, nullptr));
                    isChangeBeforeTrackedElement = (startItemIndexOfNewGroup <= m_viewportBehavior.index);
                }
                // We are tracking an item and a group was removed.
                else if (action == wfc::CollectionChange_ItemRemoved)
                {
                    INT groupIndexOfTrackedItem = 0;

                    // Information on the removed group is not in the cache manager.
                    // All we know is its index, so let's compare between group indices.

                    IFC(m_cacheManager.GetGroupInformationFromItemIndex(m_viewportBehavior.index, &groupIndexOfTrackedItem, nullptr, nullptr));
                    isChangeBeforeTrackedElement = (index <= groupIndexOfTrackedItem);
                }
            }
            // isGroupChange == false
            else
            {
                // We are tracking an item and an item was added
                if (action == wfc::CollectionChange_ItemInserted)
                {
                    isChangeBeforeTrackedElement = (index < m_viewportBehavior.index);
                }
                // we are tracking an item and an item was removed.
                else if (action == wfc::CollectionChange_ItemRemoved)
                {
                    // Note: we also check for equality on remove because the tracked element index was
                    // updated at this point.
                    isChangeBeforeTrackedElement = (index <= m_viewportBehavior.index);
                }
            }
        }
        else
        {
            if (isGroupChange)
            {
                // We are tracking a header and a group was added.
                if (action == wfc::CollectionChange_ItemInserted)
                {
                    // At this point, the cache manager knows about the new group.
                    isChangeBeforeTrackedElement = (index < m_viewportBehavior.index);
                }
                // We are tracking a header and a group was removed.
                else if (action == wfc::CollectionChange_ItemRemoved)
                {
                    // At this point, information on the removed group is not in the cache manager.
                    // All we know is its index, so let's compare between group indices.
                    isChangeBeforeTrackedElement = (index <= m_viewportBehavior.index);
                }
            }
            // isGroupChange == false
            else
            {
                // We are tracking a header and an item was added.
                if (action == wfc::CollectionChange_ItemInserted)
                {
                    INT groupIndexOfNewItem = 0;
                    IFC(m_cacheManager.GetGroupInformationFromItemIndex(index, &groupIndexOfNewItem, nullptr, nullptr));

                    isChangeBeforeTrackedElement = (groupIndexOfNewItem < m_viewportBehavior.index);
                }
                // We are tracking a header and an item was removed.
                else if (action == wfc::CollectionChange_ItemRemoved)
                {
                    // If the group is being reset, the cache has no information about it at this point.
                    // Fortunately, we stored the index.
                    if (m_containerManager.IsGroupBeingReset())
                    {
                        isChangeBeforeTrackedElement = (m_containerManager.GetIndexOfGroupBeingReset() < m_viewportBehavior.index);
                    }
                    else
                    {
                        // We use the cache history here because after a group item is removed and the cache is reset,
                        // there is no way to tell precisely what group it used to be in.
                        // In other words, we run into this ambiguity: did we delete the first item of the tracked group
                        // or the last item of the previous group?
                        INT groupIndexOfRemovedItem = 0;
                        IFC(m_cacheManager.GetGroupHistoryInformationFromItemIndex(index, &groupIndexOfRemovedItem, nullptr, nullptr));

                        isChangeBeforeTrackedElement = (groupIndexOfRemovedItem < m_viewportBehavior.index);
                    }

                    isChangeGoingToHideTrackedGroupHeader = !ShouldElementBeVisible(xaml_controls::ElementType_GroupHeader, m_viewportBehavior.index);
                }
            }
        }
    }
    // isCollectionChange == false
    else
    {
        CollectionIterator iterator(m_cacheManager, m_viewportBehavior.index, m_viewportBehavior.type);
        auto trackedElement = iterator.GetCurrent();

        // Check if the tracked element or something before it changed size.
        // The 'before tracked element' idea for measure changes is relative to the far edge.

        // We are tracking an item...
        if (m_viewportBehavior.type == xaml_controls::ElementType_ItemContainer &&
            ((isGroupChange && index <= trackedElement.groupIndex) ||
            (!isGroupChange && index <= trackedElement.itemIndex)))
        {
            isChangeBeforeTrackedElement = TRUE;
        }
        // We are tracking a group header...
        else if (m_viewportBehavior.type == xaml_controls::ElementType_GroupHeader &&
            ((isGroupChange && index <= trackedElement.groupIndex) ||
            (!isGroupChange && index < trackedElement.itemIndex)))
        {
            isChangeBeforeTrackedElement = TRUE;
        }
    }

Cleanup:
    *pIsChangeBeforeTrackedElement = isChangeBeforeTrackedElement;
    *pIsChangeGoingToHideTrackedGroupHeader = isChangeGoingToHideTrackedGroupHeader;
    RRETURN(hr);
}

_Check_return_ HRESULT
ModernCollectionBasePanel::TrackLastElement()
{
    if (m_viewportBehavior.isTrackingExtentEnd)
    {
        // We only need to follow the extent's end from now on.
        m_viewportBehavior.isTracking = false;

        wf::Size panelExtent;
        IFC_RETURN(EstimatePanelExtent(&panelExtent));

        const float requiredViewportShift =
            panelExtent.*PointFromSizeInVirtualizingDirection() - m_viewportBehavior.currentExtent;

        m_viewportBehavior.currentExtent = panelExtent.*PointFromSizeInVirtualizingDirection();

        if (requiredViewportShift > 0.0)
        {
            auto func = [this, requiredViewportShift](wf::Rect* pNewVisibleWindow)
            {
                xaml_controls::Orientation orientation;
                wf::Rect newWindow = m_windowState.GetVisibleWindow();
                IFC_RETURN(m_spLayoutStrategy->GetVirtualizationDirection(&orientation));
                if (orientation == xaml_controls::Orientation::Orientation_Horizontal)
                {
                    newWindow.X += requiredViewportShift;
                }
                else
                {
                    newWindow.Y += requiredViewportShift;
                }

                *pNewVisibleWindow = newWindow;
                return S_OK;
            };
            m_windowState.m_command = func;
        }

#ifdef IUSM_DEBUG
        m_viewportBehavior.TraceChangesDbg(L"ModernCollectionBasePanel::TrackLastElement.");
#endif
    }
    return S_OK;
}

// Called during arrange.
// Moves the viewport to the new location of the first visible element.
_Check_return_ HRESULT
ModernCollectionBasePanel::EndTrackingFirstVisibleElement()
{
    float viewportShift = 0.0f;

    if (m_viewportBehavior.isTrackingExtentEnd)
    {
        wf::Size extent;
        IFC_RETURN(get_DesiredSize(&extent));
        wf::Point extentPoint{ extent.Width, extent.Height };
        viewportShift = extentPoint.*PointFromPointInVirtualizingDirection() - m_viewportBehavior.originalExtent;
        m_viewportBehavior.Reset();
    }
    else if (m_viewportBehavior.isTracking)
    {
        ctl::ComPtr<IUIElement> spTrackedElement;

        IFC_RETURN(GetTrackedElement(&spTrackedElement));

        // Because of bug 14508453, GetTrackedElement may return null here even when m_viewportBehavior.isTracking is True.
        if (spTrackedElement)
        {
            m_viewportBehavior.elementBounds = GetBoundsFromElement(spTrackedElement);

            const float newViewportEdge = GetNewViewportEdge();

            viewportShift = newViewportEdge - m_viewportBehavior.initialViewportEdge;

            // Shifts the viewport.
            const wf::Rect visibleWindow = m_windowState.GetVisibleWindow();
            const float windowShift =
                newViewportEdge -
                (GetItemsUpdatingScrollMode() == ItemsUpdatingScrollMode::KeepItemsInView ?
                    visibleWindow.*PointFromRectInVirtualizingDirection() :
                    visibleWindow.*PointFromRectInVirtualizingDirection() + visibleWindow.*SizeFromRectInVirtualizingDirection()) +
                m_viewportBehavior.viewportOffsetDelta;

            m_viewportBehavior.Reset();
            if (!DoubleUtil::IsZero(windowShift))
            {
                wf::Point correction = {};
                correction.*PointFromPointInVirtualizingDirection() = windowShift;

                // Scroll to the new window by coercing the visible window.
                m_windowState.ApplyAdjustment(correction);

#ifdef IUSM_DEBUG
                IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG,
                    L"IUSM_DEBUG[0x%p]: ModernCollectionBasePanel::EndTrackingFirstVisibleElement. Applied windowShift: %f.", m_viewportBehavior, windowShift));
#endif
            }
        }
    }

#ifdef IUSM_DEBUG
    m_viewportBehavior.TraceChangesDbg(L"ModernCollectionBasePanel::EndTrackingFirstVisibleElement.");
#endif

    IFC_RETURN(AdjustLayoutTransitions(viewportShift));
    return S_OK;
}

// Called during MeasureExistingElements to get the realized window accounting for the viewport shift.
// This make sure ShouldContinueFillingUpSpace and recycling logic behaves correctly.
wf::Rect
ModernCollectionBasePanel::GetRealizationWindowAfterViewportShift()
{
    ASSERT(m_viewportBehavior.isTracking);

    wf::Rect realizedWindow = m_windowState.GetVisibleWindow();
    const float newViewportOffset =
        GetNewViewportEdge() -
        (GetItemsUpdatingScrollMode() == ItemsUpdatingScrollMode::KeepItemsInView ? 0 : realizedWindow.*SizeFromRectInVirtualizingDirection());

    // Adjust the visible and realized window without scrolling.
    realizedWindow.*PointFromRectInVirtualizingDirection() =
        std::max(newViewportOffset, 0.0f);

    return realizedWindow;
}

// Called before arrange.
// Adjust layout transitions to account for viewport shift.
_Check_return_ HRESULT
ModernCollectionBasePanel::AdjustLayoutTransitions(float viewportShift)
{
    if (!DoubleUtil::IsZero(viewportShift))
    {
        xaml_controls::Orientation layoutOrientation;
        IFC_RETURN(m_spLayoutStrategy->GetVirtualizationDirection(&layoutOrientation));

        // If we are tracking the last element and the list is not scrollable,
        // there is not going to be any "viewport shift" from the user's perspective
        // unless we are bottom/right aligned. If we are not, we should bail out.
        if (GetItemsUpdatingScrollMode() == ItemsUpdatingScrollMode::KeepLastItemInView)
        {
            bool isListScrollable = false;
            auto scrollViewer = m_wrScrollViewer.AsOrNull<IScrollViewer>();
            if (scrollViewer)
            {
                double scrollableSize;
                IFC_RETURN(layoutOrientation == xaml_controls::Orientation::Orientation_Horizontal ?
                    scrollViewer->get_ScrollableWidth(&scrollableSize) :
                    scrollViewer->get_ScrollableHeight(&scrollableSize));
                isListScrollable = !DoubleUtil::IsZero(scrollableSize);
            }

            if (!isListScrollable)
            {
                bool isBottomOrRightAligned = false;
                auto itemsPresenter = m_wrItemsPresenter.AsOrNull<IItemsPresenter>();
                if (itemsPresenter)
                {
                    IFC_RETURN(IsBottomOrRightAligned(itemsPresenter.Cast<ItemsPresenter>(), layoutOrientation, &isBottomOrRightAligned));
                }
                if (!isBottomOrRightAligned)
                {
                    IFC_RETURN(IsBottomOrRightAligned(this, layoutOrientation, &isBottomOrRightAligned));
                }
                if (!isBottomOrRightAligned) { return S_OK; }
            }
        }

        ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
        IFC_RETURN(m_cacheManager.GetChildren(&spChildren));

        // Containers and headers usually transition from their old location to their new location location.
        // To account for the viewport shift, they should transition from their (old location + viewport shift)
        // to their new location.
        for (INT32 i = 0; i < m_containerManager.StartOfGarbageSection(); ++i)
        {
            ctl::ComPtr<IUIElement> spElement;
            IFC_RETURN(spChildren->GetAt(i, &spElement));
            IFC_RETURN(AdjustLayoutTransitionsForElement(spElement, viewportShift, layoutOrientation));
        }

        unsigned unloadingElementsCount;
        IFC_RETURN(m_unloadingElements->get_Size(&unloadingElementsCount));
        for (unsigned i = 0; i < unloadingElementsCount; ++i)
        {
            ctl::ComPtr<IUIElement> spElement;
            IFC_RETURN(m_unloadingElements->GetAt(i, &spElement));
            IFC_RETURN(AdjustLayoutTransitionsForElement(spElement, viewportShift, layoutOrientation));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
ModernCollectionBasePanel::AdjustLayoutTransitionsForElement(
    _In_ const ctl::ComPtr<IUIElement>& element,
    _In_ float viewportShift,
    _In_ xaml_controls::Orientation layoutOrientation)
{
    UIElement::VirtualizationInformation* virtualizationInformation;
    virtualizationInformation = GetVirtualizationInformationFromElement(element);

    CUIElement* pTarget = static_cast<CUIElement*>(element.Cast<UIElement>()->GetHandle());

    // Ignore newly realized elements.
    if (pTarget->m_enteredTreeCounter != 0 &&
        pTarget->HasLayoutStorage())
    {
        // finalRect includes Margin of the element.
        // We sould use m_offset and m_size to provide right element size to transition location context.
        auto currentOffset = pTarget->GetLayoutStorage()->m_offset;
        auto currentSize = pTarget->GetLayoutStorage()->m_size;

                if (layoutOrientation == xaml_controls::Orientation_Horizontal)
                {
                    currentOffset.x += viewportShift;
                }
                else
                {
                    currentOffset.y += viewportShift;
                }

        bool hasTransitions = false;
        IFC_RETURN(CoreImports::UIElement_GetHasTransition(pTarget, &hasTransitions, nullptr));
        if (hasTransitions)
        {
            // pTarget is fading out (being unloaded). It's too late to set its current
            // transition location with CoreImports::UIElement_SetCurrentTransitionLocation. We need
            // to update m_transformStart so that it gets picked up by the transition system.
            LayoutTransitionStorage* transitionStorage = pTarget->GetLayoutTransitionStorage();
            ASSERT(transitionStorage != nullptr);
            transitionStorage->m_transformStart.SetDx(currentOffset.x);
            transitionStorage->m_transformStart.SetDy(currentOffset.y);
        }
        else
        {
            IFC_RETURN(element->InvalidateArrange());
            IFC_RETURN(CoreImports::UIElement_SetCurrentTransitionLocation(
                pTarget,
                currentOffset.x,
                currentOffset.y,
                currentSize.width,
                currentSize.height));
        }
    }

    return S_OK;
}

// Returns the first visible element's type, index and offset relative to the top.
_Check_return_ HRESULT
ModernCollectionBasePanel::GetFirstVisibleElementForSerialization(
    _Out_ xaml_controls::ElementType* pType,
    _Out_ DOUBLE* pOffset,
    _Outptr_result_maybenull_ xaml::IUIElement** ppUIElement)
{
    HRESULT hr = S_OK;
    INT elementIndex = 0;
    const wf::Rect window = m_windowState.GetVisibleWindow();
    wf::Rect actualWindow = {}; // adjusted for sticky headers, if any.

    *pOffset = 0.0;

    // Finds the first visible element, if any.
    IFC(GetFirstElementInWindow(window, pType, &elementIndex, ppUIElement, &actualWindow));

    if (elementIndex != -1)
    {
        ASSERT(*ppUIElement);
        const wf::Rect elementBounds = GetBoundsFromElement(*ppUIElement);
        *pOffset =
            GetItemsUpdatingScrollMode() == ItemsUpdatingScrollMode::KeepLastItemInView ?
            actualWindow.*PointFromRectInVirtualizingDirection() + actualWindow.*SizeFromRectInVirtualizingDirection() - elementBounds.*PointFromRectInVirtualizingDirection() :
            actualWindow.*PointFromRectInVirtualizingDirection() - elementBounds.*PointFromRectInVirtualizingDirection();
    }

Cleanup:
    RRETURN(hr);
}

double ModernCollectionBasePanel::AdjustViewportOffsetForDeserialization(double offset)
{
    if (GetItemsUpdatingScrollMode() == ItemsUpdatingScrollMode::KeepLastItemInView)
    {
        offset = offset - m_windowState.GetVisibleWindow().*SizeFromRectInVirtualizingDirection();
    }
    return offset;
}

// Returns the first visible element's type, index and UIElement.
_Check_return_ HRESULT
ModernCollectionBasePanel::GetFirstVisibleElement(
    _Out_ xaml_controls::ElementType* elementType,
    _Out_ INT* elementIndex,
    _Outptr_result_maybenull_ xaml::IUIElement** ppUIElement)
{
    RRETURN(GetFirstElementInWindow(m_windowState.GetVisibleWindow(), elementType, elementIndex, ppUIElement, nullptr /* pAdjustedWindowForStickyHeaders */));
}

// Returns the first visible element's type, index and UIElement
// in a given window.
// The pAdjustedWindowForStickyHeaders parameter is the given window adjusted for sticky headers if they
// are active. Otherwise, it's the same as window.
_Check_return_ HRESULT
ModernCollectionBasePanel::GetFirstElementInWindow(
    _In_ const wf::Rect& window,
    _Out_ xaml_controls::ElementType* elementType,
    _Out_ INT* elementIndex,
    _Outptr_result_maybenull_ xaml::IUIElement** ppUIElement,
    _Out_opt_ wf::Rect* pAdjustedWindowForStickyHeaders)
{
    BOOLEAN found = FALSE;

    std::array<INT, ElementType_Count> currentValidIndices;
    std::array<DOUBLE, ElementType_Count> elementsOffsets;
    std::array<ctl::ComPtr<IUIElement>, ElementType_Count> currentUIElements;
    wf::Rect adjustedWindowForStickyHeaders = window;
    adjustedWindowForStickyHeaders.Y += m_lastVisibleWindowClippingHeight;
    adjustedWindowForStickyHeaders.Height -= m_lastVisibleWindowClippingHeight;

    currentValidIndices.fill(0);
    elementsOffsets.fill(DoubleUtil::MaxValue);

    *ppUIElement = nullptr;
    *elementType = xaml_controls::ElementType_ItemContainer;
    *elementIndex = -1;
    if (pAdjustedWindowForStickyHeaders)
    {
        *pAdjustedWindowForStickyHeaders = window;
    }

    const ItemsUpdatingScrollMode itemsUpdatingScrollMode = GetItemsUpdatingScrollMode();

    // GetFirstElementInWindow is used when elected a new tracked element.
    // In this case, we would like to ignore the currently tracked element.
    int trackedElementValidIndex = m_viewportBehavior.isTracking
        ? m_containerManager.GetValidElementIndexFromDataIndex(m_viewportBehavior.type, m_viewportBehavior.index)
        : -1;

    // We will be going through the valid range.
    // For each type, retrieve the offset relative to the viewport and store it.
    for (int typeIndex = 0; typeIndex < ElementType_Count; ++typeIndex)
    {
        const xaml_controls::ElementType type = static_cast<xaml_controls::ElementType>(typeIndex);
        if (m_containerManager.GetValidElementCount(type) == 0) continue;

        const wf::Rect intersectionWindow = (typeIndex == xaml_controls::ElementType_ItemContainer) ? adjustedWindowForStickyHeaders : window;

        const int startIndex =
            itemsUpdatingScrollMode == ItemsUpdatingScrollMode::KeepLastItemInView ?
            m_containerManager.GetValidElementCount(type) - 1 :
            currentValidIndices[typeIndex];

        const int endIndex =
            itemsUpdatingScrollMode == ItemsUpdatingScrollMode::KeepLastItemInView ?
            currentValidIndices[typeIndex] - 1 :
            m_containerManager.GetValidElementCount(type);

        const int increment = (itemsUpdatingScrollMode == ItemsUpdatingScrollMode::KeepLastItemInView) ? -1 : 1;

        for (int validIndex = startIndex; validIndex != endIndex; validIndex += increment)
        {
            wf::Rect bounds = {};
            ctl::ComPtr<IUIElement> spElement;

            if (validIndex == trackedElementValidIndex && m_viewportBehavior.type == type)
                continue;

            IFC_RETURN(m_containerManager.GetElementAtValidIndex(type, validIndex, &spElement));

            // Ignores any sentinel.
            if (GetElementIsSentinel(spElement))
                continue;

            bounds = GetBoundsFromElement(spElement);

            // We don't want elements to overlap over one line.
            bounds.*SizeFromRectInVirtualizingDirection() -= EdgeOverlayDisambiguationDelta;

            if (!RectUtil::AreDisjoint(bounds, intersectionWindow))
            {
                elementsOffsets[typeIndex] =
                    itemsUpdatingScrollMode == ItemsUpdatingScrollMode::KeepLastItemInView ?
                    (window.*PointFromRectInVirtualizingDirection() + window.*SizeFromRectInVirtualizingDirection()) -
                        (bounds.*PointFromRectInVirtualizingDirection() + bounds.*SizeFromRectInVirtualizingDirection()) :
                    bounds.*PointFromRectInVirtualizingDirection() - window.*PointFromRectInVirtualizingDirection();
                currentUIElements[typeIndex] = spElement;
                currentValidIndices[typeIndex] = m_containerManager.GetDataIndexFromValidIndex(type, validIndex);
                found = TRUE;
                break;
            }
        }
    }

    // If we found something, return it.
    if (found)
    {
        // Preference goes to header.
        auto type = (elementsOffsets[xaml_controls::ElementType_GroupHeader] <= elementsOffsets[xaml_controls::ElementType_ItemContainer])
            ? xaml_controls::ElementType_GroupHeader
            : xaml_controls::ElementType_ItemContainer;

        *elementType = type;
        *elementIndex = currentValidIndices[type];
        *ppUIElement = currentUIElements[type].Detach();

        if (pAdjustedWindowForStickyHeaders && m_bUseStickyHeaders && type == xaml_controls::ElementType_ItemContainer)
        {
            *pAdjustedWindowForStickyHeaders = adjustedWindowForStickyHeaders;
        }
    }

    return S_OK;
}

// Sets the offset between the tracked element and the visible window.
void ModernCollectionBasePanel::SetTrackedElementOffsetRelativeToViewport()
{
    ASSERT(m_viewportBehavior.isTracking);

    // Note: trackedElementShift's value is always zero outside of Measure/Arrange.
    const FLOAT effectiveViewport = m_viewportBehavior.initialViewportEdge + m_viewportBehavior.trackedElementShift;
    const wf::Rect bounds = m_viewportBehavior.elementBounds;
    const ItemsUpdatingScrollMode itemsUpdatingScrollMode = GetItemsUpdatingScrollMode();
    const float boundsFirstEdge =
        itemsUpdatingScrollMode == ItemsUpdatingScrollMode::KeepLastItemInView ?
        bounds.*PointFromRectInVirtualizingDirection() + bounds.*SizeFromRectInVirtualizingDirection() :
        bounds.*PointFromRectInVirtualizingDirection();

    // The calculated offset between the element and the viewport is relative to an edge (first or second from the top\left).
    if (itemsUpdatingScrollMode != ItemsUpdatingScrollMode::KeepLastItemInView ||
        (itemsUpdatingScrollMode == ItemsUpdatingScrollMode::KeepLastItemInView && boundsFirstEdge <= effectiveViewport))
    {
        m_viewportBehavior.isOffsetRelativeToSecondEdge = FALSE;
        m_viewportBehavior.elementOffset = effectiveViewport - boundsFirstEdge;
    }
    else
    {
        const float boundsSecondEdge =
            itemsUpdatingScrollMode == ItemsUpdatingScrollMode::KeepLastItemInView ?
            bounds.*PointFromRectInVirtualizingDirection() :
            bounds.*PointFromRectInVirtualizingDirection() + bounds.*SizeFromRectInVirtualizingDirection();

        m_viewportBehavior.isOffsetRelativeToSecondEdge = TRUE;
        m_viewportBehavior.elementOffset = effectiveViewport - boundsSecondEdge;
    }

#ifdef IUSM_DEBUG
    m_viewportBehavior.TraceChangesDbg(L"ModernCollectionBasePanel::SetTrackedElementOffsetRelativeToViewport.");
#endif
}

// Returns the tracked UIElement or null.
// Because of bugs 14508453 & 14508725, this method may return null even when m_viewportBehavior.isTracking is True.
_Check_return_ HRESULT
ModernCollectionBasePanel::GetTrackedElement(_Out_ ctl::ComPtr<IUIElement>* pspTrackedElement)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spElement;

    const INT32 validIndex = m_containerManager.GetValidElementIndexFromDataIndex(m_viewportBehavior.type, m_viewportBehavior.index);

    ASSERT(m_containerManager.GetValidElementCount(m_viewportBehavior.type) > 0);
    ASSERT(m_containerManager.IsValidElementIndexWithinBounds(m_viewportBehavior.type, validIndex));

    IFC(m_containerManager.GetElementAtValidIndex(
        m_viewportBehavior.type,
        validIndex,
        &spElement));

    *pspTrackedElement = std::move(spElement);
Cleanup:
    RRETURN(hr);
}

// When the tracked element gets removed, we need to elect a new one.
_Check_return_ HRESULT
ModernCollectionBasePanel::ElectNewTrackedElement()
{
    HRESULT hr = S_OK;

    ctl::ComPtr<IUIElement> spOldTrackedElement;
    ctl::ComPtr<IUIElement> spNewTrackedElement;
    xaml_controls::ElementType elementType = xaml_controls::ElementType_ItemContainer;
    INT elementIndex = 0;
    wf::Rect windowStartingAtOldTrackedElement = m_windowState.GetVisibleWindow();

    ASSERT(m_viewportBehavior.isTracking);

    IFC(GetTrackedElement(&spOldTrackedElement));

    // The new tracked element will be the first one that visually come after/before the currently
    // tracked element .
    windowStartingAtOldTrackedElement.*PointFromRectInVirtualizingDirection() =
        GetItemsUpdatingScrollMode() == ItemsUpdatingScrollMode::KeepItemsInView ?
        GetBoundsFromElement(spOldTrackedElement).*PointFromRectInVirtualizingDirection() :
        GetBoundsFromElement(spOldTrackedElement).*PointFromRectInVirtualizingDirection() - windowStartingAtOldTrackedElement.*SizeFromRectInVirtualizingDirection();

    IFC(GetFirstElementInWindow(windowStartingAtOldTrackedElement, &elementType, &elementIndex, &spNewTrackedElement, nullptr /* pAdjustedWindowForStickyHeaders */));

    const wf::Rect elementBounds = spNewTrackedElement ? GetBoundsFromElement(spNewTrackedElement) : wf::Rect{};

    if (spNewTrackedElement &&
       !RectUtil::AreDisjoint(elementBounds, m_windowState.GetVisibleWindow()))
    {
        m_viewportBehavior.index = elementIndex;
        m_viewportBehavior.type = elementType;
        m_viewportBehavior.elementBounds = elementBounds;

        SetTrackedElementOffsetRelativeToViewport();
    }
    else
    {
        // If we can't elect a new tracked element from the realized range, cancel the tracking.
        // The reason we don't allow tracked elements outside the visible window is that we can't
        // guarantee that they will be protected from recycling during the next layout.
        // In other words, they might be outside the window returned by GetRealizationWindowAfterViewportShift.
        // The latter is based off the size of the visible window.
        m_viewportBehavior.Reset();
    }

Cleanup:
#ifdef IUSM_DEBUG
    m_viewportBehavior.TraceChangesDbg(L"ModernCollectionBasePanel::ElectNewTrackedElement.");
#endif
    RRETURN(hr);
}

// Returns where the viewport should move to 'follow' the tracked element.
FLOAT ModernCollectionBasePanel::GetNewViewportEdge() const
{
    FLOAT newViewportOffset = 0;

    ASSERT(m_viewportBehavior.isTracking);

    const wf::Rect bounds = m_viewportBehavior.elementBounds;
    const ItemsUpdatingScrollMode itemsUpdatingScrollMode = GetItemsUpdatingScrollMode();

    if (m_viewportBehavior.isOffsetRelativeToSecondEdge)
    {
        const float boundsSecondEdge =
            itemsUpdatingScrollMode == ItemsUpdatingScrollMode::KeepLastItemInView ?
            bounds.*PointFromRectInVirtualizingDirection() :
            bounds.*PointFromRectInVirtualizingDirection() + bounds.*SizeFromRectInVirtualizingDirection();

        newViewportOffset = boundsSecondEdge + m_viewportBehavior.elementOffset;
    }
    else
    {
        const float boundsFirstEdge =
            itemsUpdatingScrollMode == ItemsUpdatingScrollMode::KeepLastItemInView ?
            bounds.*PointFromRectInVirtualizingDirection() + bounds.*SizeFromRectInVirtualizingDirection() :
            bounds.*PointFromRectInVirtualizingDirection();

        newViewportOffset = boundsFirstEdge + m_viewportBehavior.elementOffset;
    }

    return newViewportOffset;
}

_Check_return_ HRESULT
ModernCollectionBasePanel::IsBottomOrRightAligned(
    _In_ xaml::IFrameworkElement* element,
    _In_ xaml_controls::Orientation orientation,
    _Out_ bool* result)
{
    *result = false;
    if (orientation == xaml_controls::Orientation::Orientation_Horizontal)
    {
        xaml::HorizontalAlignment alignment;
        IFC_RETURN(element->get_HorizontalAlignment(&alignment));
        *result = (alignment == xaml::HorizontalAlignment::HorizontalAlignment_Right);
    }
    else
    {
        ASSERT(orientation == xaml_controls::Orientation::Orientation_Vertical);
        xaml::VerticalAlignment alignment;
        IFC_RETURN(element->get_VerticalAlignment(&alignment));
        *result = (alignment == xaml::VerticalAlignment::VerticalAlignment_Bottom);
    }
    return S_OK;
}

void ModernCollectionBasePanel::UpdateViewportOffsetDelta(
    _In_ const wf::Rect& oldVisibleWindow,
    _In_ const wf::Rect& newVisibleWindow)
{
    // This condition is true when we get a ViewChanging notification
    // between the time we started tracking (before layout, ie during a collection change)
    // and the time we run layout (IsMeasureDirty returns false).
    // We use the viewport delta to adjust, later during arrange, the window shift.
    if (m_viewportBehavior.isTracking &&
        m_viewportBehavior.viewportOffsetDelta == 0.0f &&
        static_cast<CUIElement*>(GetHandle())->GetIsMeasureDirty())
    {
        m_viewportBehavior.viewportOffsetDelta =
            newVisibleWindow.*PointFromRectInVirtualizingDirection() -
            oldVisibleWindow.*PointFromRectInVirtualizingDirection();

#ifdef IUSM_DEBUG
        m_viewportBehavior.TraceChangesDbg(L"ModernCollectionBasePanel::UpdateViewportOffsetDelta.");
#endif
    }
}
