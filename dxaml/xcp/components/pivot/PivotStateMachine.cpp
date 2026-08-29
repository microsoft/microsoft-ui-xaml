// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PivotStateMachine.h"
#include "PivotCurveGenerator.h"

#undef max
#undef min

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{

//#define PVTRACE(...) TRACE(TraceAlways, __VA_ARGS__)
#define PVTRACE(...)

#if defined(DBG)
const WCHAR* PivotStateMachine::c_pivotStateStrings[] =
    { L"Uninitialized", L"MeasurePending", L"ArrangePending", L"Idle", L"InNonInertial", L"InFinishTransition"
    L"InFlyOutProgrammaticInertial", L"InFlyInProgrammaticInertial", L"InSnapBackInertial", L"ProgrammaticInertial",
    L"LockedPending", L"Locked"};
#endif
const DOUBLE PivotStateMachine::c_pivotFlickAnimationDuration = 0.4;

// Controls the percentage width of a section that we will
// support the standard DManip animation with. Increasing
// this value or decreasing the duration will increase the
// velocity of the applied animation. Adjust this value
// and the value above to change how pivot slide-in appears.
const DOUBLE PivotStateMachine::c_pivotAnimationRatio = 1.0;

PivotStateMachine::PivotStateMachine(_In_ IPivotStateMachineCallbacks* pCallbacks)
    : m_callbackPtr(pCallbacks)
    , m_state(PivotState::PivotState_Uninitialized)
    , m_pivotSectionWidth(0.0)
    , m_staticViewportOffset(0.0)
    , m_predictedViewportOffset(0.0)
    , m_currentIndex(0)
    , m_nextIndex(0)
    , m_totalItems(0)
    , m_lastArrangeSize()
    , m_direction(PivotAnimationDirection_Reset)
    , m_fIsLocked(FALSE)
    , m_fPendingInitialization(TRUE)
    , m_fPendingOutOfSequenceMove(FALSE)
    , m_fPendingViewportEnabledUpdate(TRUE)
    , m_fPendingFirstItemLayout(TRUE)
    , m_fPendingItemHostValidation(FALSE)
    , m_fSnapPointBehaviorRelaxed(FALSE)
    , m_fProgrammaticViewChangeInProgress(FALSE)
    , m_fWithinTransition(FALSE)
    , m_usingStaticHeaders(false)
    , m_viewportSize(0.0f)
    , m_pendingZoomToRect(false)
    , m_pendingZoomToRectOffset(0.0)
    , m_pendingHeaderShiftOffset(0.0)
    , m_manualAnimationInProgress(false)
    , m_deferredSetViewportOffsetData()
    , m_fPendingArrange(false)
{
}

_Check_return_ HRESULT
PivotStateMachine::Initialize(_In_ INT32 startingIndex, _In_ INT32 itemCount, _In_ BOOLEAN isLocked)
{
    ASSERT(m_state == PivotState_Uninitialized);

    m_fIsLocked = isLocked;
    m_totalItems = itemCount;
    m_currentIndex = startingIndex;

    return S_OK;
}

_Check_return_ HRESULT
PivotStateMachine::ApplyTemplateEvent(_In_ BOOLEAN isCorrect)
{
    PVTRACE(L"[PSM]: ApplyTemplate occurred. isCorrect: %d", isCorrect);
    if (isCorrect)
    {
        m_fPendingInitialization = TRUE;

        // While from ItemControl's POV the ItemHost IS technically invalid, we
        // will proceed with Pivot's typical setup loop, which waits until the validating
        // measure pass occurs before attempting to get an item container. We clear this
        // flag as it is only set to handle cases where Items collection is reset while Pivot
        // is initialized.
        m_fPendingItemHostValidation = FALSE;
        IFC_RETURN(Transition(PivotState_MeasurePending));
    }
    else
    {
        IFC_RETURN(Transition(PivotState_Uninitialized));
    }

    return S_OK;
}

_Check_return_ HRESULT
PivotStateMachine::ArrangeEvent(_In_ wf::Size finalSize)
{
    UNREFERENCED_PARAMETER(finalSize);

    if (m_state != PivotState_Uninitialized)
    {
        if (m_fPendingInitialization || !AreClose(finalSize.Width, m_lastArrangeSize.Width) || m_fPendingFirstItemLayout || m_fPendingArrange)
        {
            m_lastArrangeSize = finalSize;
            if (!m_fWithinTransition)
            {
                if (m_fIsLocked)
                {
                    IFC_RETURN(Transition(PivotState_Locked));
                }
                else
                {
                    IFC_RETURN(Transition(PivotState_Idle));
                }
            }
            else
            {
                m_fPendingArrange = false;
            }
        }
        else
        {
            PVTRACE(L"[PSM]: Skipping arrange event transition due to identical arrange size.");
        }

        ASSERT(!m_fPendingArrange);

        if (m_fPendingItemHostValidation)
        {
            PVTRACE(L"[PSM]: Clearing pendingItemHostValidation flag and setting selected index due to Items collection set.");

            // In the case we're in FlyOutProgrammaticInertial (the developer set an index we were trying to animate to, or the user was
            // manipulating the Pivot) we can simply clear this flag and let the normal logic operate.
            // We're moments away from Transitioning to FlyInInertial which will change the visual item at the correct
            // time. If we were to update it here we would throw PivotCurveGenerator out of sync: the index is being updated
            // but we're passing in an incorrect animation direction. That would force us to calculate an incorrect selected
            // item offset, and build incorrect curves around it, they would be applied until the FlyInInertial transition
            // occurred.
            if (m_state != PivotState_InFlyOutProgrammaticInertial)
            {
                IFC_RETURN(m_callbackPtr->SetSelectedIndex(
                    m_currentIndex, TRUE /* updateVisual */, FALSE /* updateIndex */, FALSE /* updateItem */, PivotAnimationDirection_Reset));
            }
            m_fPendingItemHostValidation = FALSE;
        }

        if (m_fPendingFirstItemLayout && m_totalItems > 0)
        {
            PVTRACE(L"[PSM]: First item being laid out.");
            m_fPendingFirstItemLayout = FALSE;
        }

    }

    return S_OK;
}

_Check_return_ HRESULT
PivotStateMachine::MeasureEvent(_In_ wf::Size availableSize)
{
    if (m_state != PivotState_Uninitialized)
    {
        if (m_fPendingInitialization || !AreClose(availableSize.Width, m_pivotSectionWidth) || m_fPendingFirstItemLayout)
        {
            // Calculate the midpoint of the panel and setup
            // state to be later propagated into the ScrollViewer
            // post-arrange.
            m_pivotSectionWidth = availableSize.Width;
            // It's possible to have multiple measure passes before an arrange pass.
            IFC_RETURN(m_callbackPtr->SetPivotSectionWidth(static_cast<float>(m_pivotSectionWidth)));
            if ((m_state == PivotState_Uninitialized ||
                 m_state == PivotState_MeasurePending ||
                 m_state == PivotState_Idle ||
                 m_state == PivotState_Locked)
                 && !m_fWithinTransition)
            {
                IFC_RETURN(Transition(PivotState_ArrangePending));
            }
            else
            {
                // We will update the viewport offset when we later go to idle/locked.
                m_fPendingArrange = true;
            }
        }
        else
        {
            PVTRACE(L"[PSM]: Skipping measure event transition due to identical measure size.");
        }
    }
    return S_OK;
}

double
PivotStateMachine::GetMidpointViewportOffset() const
{
    return static_cast<double>(m_pivotSectionWidth * static_cast<double>(m_callbackPtr->GetPivotPanelMultiplier()) / 2);
}

_Check_return_ HRESULT
PivotStateMachine::ViewChangedEvent(_In_ BOOLEAN isInertial, _In_ DOUBLE nextOffset, _In_ DOUBLE finalOffset, _In_ DOUBLE sectionOffset)
{
    BOOLEAN isWrong = FALSE;

    DOUBLE pivotSectionsMoved = (m_staticViewportOffset + finalOffset) / m_pivotSectionWidth;
    BOOLEAN isOnSnapPoint = AreClose(pivotSectionsMoved, floor(pivotSectionsMoved + 0.5));

    if (isInertial && !isOnSnapPoint)
    {
        // There's an invariant that states any time we're in an inertial move the final offset should fall on a snap
        // point. If we're invaliding that invariant... and ScrollViewer does at times.... we should debug print and
        // skip this ViewChanged event.
        PVTRACE(L"[PSM]: ERROR: ViewChangedEvent error. Final section of an inertial move does not fall on a snap point.");
        isWrong = TRUE;
    }

    // Reducing the noise for debug perf and my sanity...
    static INT debugTraceCounter = 0;
    debugTraceCounter++;
    if (debugTraceCounter % 2 == 0 || isWrong)
    {
        PVTRACE(L"[PSM]: ViewChangedEvent occurred. isInertial: %d, nextOffset: %f, finalOffset: %f, sectionOffset: %f",
            isInertial, nextOffset, finalOffset, sectionOffset);
    }

    if (isWrong)
    {
        return S_OK;
    }

    if (IsInitialized() && !m_fProgrammaticViewChangeInProgress && m_state != PivotState_Locked)
    {
        if (m_state == PivotState_LockedPending)
        {
            m_predictedViewportOffset =
                m_deferredSetViewportOffsetData.isPending ?
                m_deferredSetViewportOffsetData.offset :
                finalOffset;
        }
        else if (isInertial)
        {
            m_predictedViewportOffset =
                m_deferredSetViewportOffsetData.isPending ?
                m_deferredSetViewportOffsetData.offset :
                finalOffset;

            if (AreClose(finalOffset, m_staticViewportOffset, m_pivotSectionWidth / c_sectionWidthDividerForOffsetComparisons))
            {
                if (m_state == PivotState_InNonInertial)
                {
                    IFC_RETURN(Transition(PivotState_InSnapBackInertial));
                }
            }
            else
            {
                if (m_state == PivotState_InNonInertial ||
                    m_state == PivotState_InSnapBackInertial ||
                    m_state == PivotState_Idle ||
                    m_state == PivotState_ProgrammaticInertial)
                {
                    if (!m_fPendingOutOfSequenceMove)
                    {
                        m_direction = m_predictedViewportOffset < m_staticViewportOffset ?
                            PivotAnimationDirection_Left : PivotAnimationDirection_Right;
                    }
                    IFC_RETURN(Transition(PivotState_InFinishTransition));
                }
            }
        }
        else if(m_state == PivotState_InFinishTransition)
        {
            IFC_RETURN(Transition(PivotState_Idle))
        }
        else
        {
            m_predictedViewportOffset =
                m_deferredSetViewportOffsetData.isPending ?
                m_deferredSetViewportOffsetData.offset :
                finalOffset;

            IFC_RETURN(m_callbackPtr->UpdateScrollViewerDragDirection(m_predictedViewportOffset < m_staticViewportOffset ? PivotAnimationDirection_Left : PivotAnimationDirection_Right));
            if (m_state != PivotState_InFlyOutProgrammaticInertial)
            {
                // If we're doing the fly-out animation and are animating things ourselves, then we'll transition when the animation completes.
                // No need to do so here.
                IFC_RETURN(Transition(PivotState_InNonInertial));
            }
        }
    }
    else if (m_fProgrammaticViewChangeInProgress)
    {
        PVTRACE(L"[PSM]: Ignoring. Programmatic view change in progress.");
    }

    return S_OK;
}

_Check_return_ HRESULT
PivotStateMachine::FinalViewEvent()
{
    PVTRACE(L"[PSM]: FinalViewEvent occurred.");

    // If we wanted to call SetViewportOffset but were in the midst
    // of an existing animation, then we want to call that now.
    if (m_deferredSetViewportOffsetData.isPending)
    {
        PVTRACE(L"[PSM]: Pending SetViewportOffset() call, executing with (offset, shouldAnimate) = (%f, %d).", m_deferredSetViewportOffsetData.offset, m_deferredSetViewportOffsetData.shouldAnimate);

        m_deferredSetViewportOffsetData.isPending = false;
        IFC_RETURN(SetViewportOffset(m_deferredSetViewportOffsetData.offset, m_deferredSetViewportOffsetData.shouldAnimate));
    }

    bool isInDManipAnimation = false;
    IFC_RETURN(m_callbackPtr->GetIsInDManipAnimation(&isInDManipAnimation));

    PVTRACE(L"[PSM]: (isInDManipAnimation, m_manualAnimationInProgress) = (%d, %d).", isInDManipAnimation, m_manualAnimationInProgress);

    // The above code could have caused isInDManipAnimation to be set to true.
    // If it was, or if a manual animation is in progress, then we'll have another callback
    // that will revert us to idle at that point.  If neither is true, however,
    // then this is our last chance, so we should revert to idle now.
    if (!isInDManipAnimation && !m_manualAnimationInProgress)
    {
        IFC_RETURN(RevertToIdle());
    }

    return S_OK;
}

_Check_return_ HRESULT
PivotStateMachine::SelectedIndexChangedEvent(_In_ INT32 newIndex, _In_ BOOLEAN isFromHeaderTap, _In_ BOOLEAN skipAnimation)
{
    bool isInDManipAnimation = false;
    IFC_RETURN(m_callbackPtr->GetIsInDManipAnimation(&isInDManipAnimation));

    const bool shouldTransitionState =
        !skipAnimation &&
        ((m_state == PivotState_Idle || m_state == PivotState_InFlyInProgrammaticInertial || m_state == PivotState_InFinishTransition) && !m_fPendingFirstItemLayout && (!isInDManipAnimation || isFromHeaderTap));

    PVTRACE(L"[PSM]: SelectedIndex to %d occurred", newIndex);

    if (shouldTransitionState)
    {
        if (m_currentIndex != newIndex)
        {
            m_fPendingOutOfSequenceMove = TRUE;
            m_nextIndex = newIndex;

            m_direction = DetermineDirection(m_nextIndex, isFromHeaderTap);
            // When carousel is disabled, we are much more strict about the relationship between the selected index and viewport offset.
            // Whereas, when it's enabled (default), it's not strictly enforced.
            if (IsHeaderItemsCarouselEnabled())
            {
                INT itemsToShift = newIndex - m_currentIndex;

                if (m_direction == PivotAnimationDirection_Right)
                {
                    itemsToShift = PositiveMod(itemsToShift, m_totalItems);
                }
                else
                {
                    itemsToShift = -static_cast<INT>((m_totalItems - PositiveMod(itemsToShift, m_totalItems)));
                }

                m_predictedViewportOffset = m_staticViewportOffset + m_pivotSectionWidth * itemsToShift;
            }
            else
            {
                m_predictedViewportOffset = m_nextIndex * m_pivotSectionWidth;
            }

            {
                ASSERT(m_totalItems == static_cast<int>(m_headerWidths.size()));

                // If we're using static headers in Redstone or more recent builds and are animating headers ourselves,
                // we need to calculate the amount by which we need to shift the headers.
                // To do that, we calculate the expect offset with the current and new selected index, and then
                // calculate the difference.
                if(InScrollingStaticHeadersMode())
                {
                    double accumulatedItemWidths = 0.0;
                    double a_current = 0.0, a_new = 0.0;
                    double b_current = 0.0, b_new = 0.0;
                    double c_current = 0.0, c_new = 0.0;
                    for (int i = 0; i < m_totalItems; ++i)
                    {
                        if (i == m_currentIndex)
                        {
                            a_current = accumulatedItemWidths;
                            b_current = m_headerWidths[i];
                        }
                        else if (i == newIndex)
                        {
                            a_new = accumulatedItemWidths;
                            b_new = m_headerWidths[i];
                        }

                        accumulatedItemWidths += m_headerWidths[i];
                    }

                    c_current = std::max(accumulatedItemWidths - (a_current + b_current), 0.0);
                    c_new = std::max(accumulatedItemWidths - (a_new + b_new), 0.0);

                    m_pendingHeaderShiftOffset =
                        std::abs(PivotCurveGenerator::GetHeaderShiftOffset(a_new, b_new, c_new, m_viewportSize) -
                                 PivotCurveGenerator::GetHeaderShiftOffset(a_current, b_current, c_current, m_viewportSize));
                }
                // If we're using dynamic headers and are animating headers ourselves, we want to shift the header at the new index into the place that the current header occupies.
                // To determine how far we need to shift the headers, we need to add together the widths of all of the headers
                // that are between the current header and the new header that will be sliding into place, and move the header panel
                // by that distance.  If we're moving to the left, we don't consider the current header and we do consider the new header,
                // since the current header's left side is already in the position we're wanting it to be in while the new header needs to move into position.
                // If we're moving to the right, then the opposite is true: we consider the current header and don't consider the new header,
                // with similar reasoning.
                else if (!m_usingStaticHeaders)
                {
                    double headerDistanceToShift = 0.0;

                    if (m_direction == PivotAnimationDirection_Right)
                    {
                        for (int index = m_currentIndex;
                            index != m_nextIndex;
                            index = PositiveMod(index + 1, m_totalItems))
                        {
                            headerDistanceToShift += m_headerWidths[index];
                        }
                    }
                    else
                    {
                        for (int index = PositiveMod(m_currentIndex - 1, m_totalItems);
                            index != PositiveMod(m_nextIndex - 1, m_totalItems);
                            index = PositiveMod(index - 1, m_totalItems))
                        {
                            headerDistanceToShift += m_headerWidths[index];
                        }
                    }

                    m_pendingHeaderShiftOffset = headerDistanceToShift;
                }
            }

            IFC_RETURN(Transition(PivotState_ProgrammaticInertial));
        }
        else
        {
            PVTRACE(L"[PSM] - Skipping index change. Identical index specified.");
        }
    }
    else
    {
        // Either we're uninitialized or in motion...
        // If we're currently moving the best we can do
        // is create instantaneous change, there's little
        // point to try to "do the right thing" while the
        // ScrollViewer is under manipulation. There's nothing
        // we can do for the panning case without releasing all
        // contacts, and little we can do for inertial cases
        // without adding a lot of complexity.

        // We catch cases where we're essentially already doing the move
        // to avoid glitching when we can.
        if (m_fPendingOutOfSequenceMove && m_nextIndex != newIndex ||
            m_state == PivotState_InFlyInProgrammaticInertial && m_currentIndex != newIndex ||
            m_state == PivotState_InFinishTransition && m_currentIndex != newIndex ||
            m_state == PivotState_Locked && m_currentIndex != newIndex ||
            !IsInitialized() ||
            m_state == PivotState_Idle && m_fPendingFirstItemLayout)
        {
            PVTRACE(L"[PSM] - Non animating state found. Instantaneously jumping.");
            m_nextIndex = newIndex;
            m_currentIndex = newIndex;

            // When carousel is disabled, we are much more strict about the relationship between the selected index and viewport offset.
            // Whereas, when it's enabled (default), it's not strictly enforced.
            if (!IsHeaderItemsCarouselEnabled())
            {
                m_predictedViewportOffset = m_nextIndex * m_pivotSectionWidth;
                m_pendingZoomToRect = true;
                m_pendingZoomToRectOffset = m_predictedViewportOffset;
            }

            IFC_RETURN(m_callbackPtr->SetSelectedIndex(m_currentIndex, IsInitialized() /* updateVisual */, TRUE /* updateIndex */, TRUE /* updateItem */, PivotAnimationDirection_Reset));

            if (m_state == PivotState_Locked || m_fPendingFirstItemLayout)
            {
                IFC_RETURN(m_callbackPtr->SetParallaxRelationship(m_staticViewportOffset, m_pivotSectionWidth, m_viewportSize));
            }
        }
        else
        {
            PVTRACE(L"[PSM] - Skipping index change. Identical index specified or Pivot not initialized.");
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
PivotStateMachine::IsLockedChangedEvent(_In_ BOOLEAN isLocked)
{
    m_fIsLocked = isLocked;

    if (IsInitialized())
    {
        m_fPendingViewportEnabledUpdate = TRUE;

        if (isLocked)
        {
            if (m_state == PivotState_Idle)
            {
                IFC_RETURN(Transition(PivotState_Locked));
            }
            else if (m_state != PivotState_Locked && m_state != PivotState_LockedPending)
            {
                IFC_RETURN(Transition(PivotState_LockedPending));
            }
        }
        else
        {
            ASSERT(m_state == PivotState_Locked || m_state == PivotState_LockedPending);

            if (m_state == PivotState_Locked)
            {
                IFC_RETURN(Transition(PivotState_Idle));
            }
            // else... wait for transition to locked to cause
            // a subsequent transition to Idle.
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
PivotStateMachine::ItemsCollectionChangedEvent(_In_ INT32 newItemCount, _In_ INT32 changeIdx, _In_ wfc::CollectionChange changeType)
{
    INT32 oldItemCount = m_totalItems;
    m_totalItems = newItemCount;

    // Even if we're uninitialized, we still want to flag us as having a pending viewport enabled update.
    // Otherwise, we'll have no idea that we'll need to change this value once we become initialized.
    if ((oldItemCount <= 1 && newItemCount > 1)
        || (oldItemCount> 1 && newItemCount <= 1))
    {
        m_fPendingViewportEnabledUpdate = TRUE;
    }

    // When we're not initialized (Template not applied) we allow the index and
    // item to slide around, we don't validate them. This allows
    // for out-of-order setting of properties when creating a new
    // instance of this control.
    if (!IsInitialized())
    {
        return S_OK;
    }

    // NOTE: There's two different situations we can be in when handling ItemCollectionChanged
    // events. Either we're:
    // A: In a state where m_currentIndex is accurate from the POV of the control consumer
    // B: In a state where we're in a programmatic animated move. We've already fired the
    //    SelectionChanged event synchronously when the developer updated the index, but
    //    we're waiting for the animation to complete. m_currentIndex points to the old PivotItem
    //    while m_nextIndex points to the future item. m_fPendingOutOfSequenceMove is set
    //
    // We handle these situations differently to ensure SelectionChanged events are always
    // accurate and appropriate.
    INT32* pIndexOfInterest = m_fPendingOutOfSequenceMove ? &m_nextIndex : &m_currentIndex;

    BOOLEAN updateIndex = FALSE;
    BOOLEAN updateItem = FALSE;
    PivotAnimationDirection direction = PivotAnimationDirection_Reset;

    // Behavior: Force the index to zero. Update visuals and fire selection
    // changed event.
    if (changeType == wfc::CollectionChange_Reset)
    {
        // When the collection is reset the indexes are forced to 0.
        *pIndexOfInterest = 0;
        updateIndex = TRUE;
        updateItem = TRUE;

        // When the Items collection is Reset ItemsControl clears all available ItemsContainers
        // and invalidates ItemsHost, causing them to be recreated for the new Items collection
        // during the next measure. This is different than the Add/Remove/Change cases where it
        // instead synchronously updates the generated ItemsContainers. Because no containers
        // are available until Measure we cannot update the currently visible item until then.
        // m_fPendingItemHostValidate tracks this state for us and prevents other operations
        // (originating externally or internally) on Pivot that occur after the ItemsCollection
        // is reset but before the next measure pass from attempting to update the currently visible item.
        m_fPendingItemHostValidation = m_fPendingItemHostValidation || newItemCount > 0;
    }
    // Behavior: If the current item changed fire the SelectionChanged event
    // and update the visual.
    else if (changeType == wfc::CollectionChange_ItemChanged && changeIdx == *pIndexOfInterest)
    {
        // In the case of an item being 'swapped out' we only case about the case
        // where it's the currently displayed item. We'll do the appropriate visual
        // update here and fire the SelectionChanged event.
        // In the case of an animated move in progress we've
        // already fired the SelectionChanged event to the
        // new item, even through it's not visually on screen yet.
        // We need to now fire another SelectionChanged event
        // for the next new item that has replaced the previous item
        // at m_nextIndex.
        updateItem = TRUE;
    }
    // Behavior: If there were no previous items fire the SelectionChanged event
    // and select the newly inserted item. Otherwise keep the current item selected,
    // even if it means pushing the index around.
    else if (changeType == wfc::CollectionChange_ItemInserted && changeIdx <= *pIndexOfInterest)
    {
        // On first item insertion we force the index to zero.
        // It's the only valid index. This is the only place we
        // coerce the index or item inside PivotStateMachine.
        // EXPECTED BEHAVIOR: Setting an index or item before
        // filling the items collection will result in coercion
        // to the first item.
        // Otherwise when the item inserted comes before the current item
        // we move the SelectedIndex to keep the current item selected.
        SlideIndex(pIndexOfInterest, PivotAnimationDirection_Right);
        updateIndex = TRUE;
        updateItem = oldItemCount == 0;
    }
    // Behavior: If the item removed was the current item then push the
    // selection to the next item to the right (the item at the same index).
    // Otherwise if the item removed came before the current item adjust
    // the index appropriately.
    else if (changeType == wfc::CollectionChange_ItemRemoved)
    {
        direction = PivotAnimationDirection_Left;
        if (changeIdx < *pIndexOfInterest)
        {
            SlideIndex(pIndexOfInterest, PivotAnimationDirection_Left);
            updateIndex = TRUE;
        }
        else if (changeIdx == *pIndexOfInterest)
        {
            updateIndex = newItemCount <= *pIndexOfInterest;
            updateItem = TRUE;
            // Makes sure we're still within the bounds of the collection.
            SlideIndex(pIndexOfInterest, PivotAnimationDirection_Reset);
        }
    }

    const BOOLEAN updateVisual = !m_fPendingOutOfSequenceMove && !m_fPendingItemHostValidation;
    if (updateIndex || updateItem)
    {
        IFC_RETURN(m_callbackPtr->SetSelectedIndex(
            *pIndexOfInterest, updateVisual, updateIndex, updateItem, direction));
    }
    // When only 1 or 0 PivotItems are present we lock the
    // viewport and disable scrolling. If we're currently in motion
    // we'll keep our seat belts locked and the m_fPendingViewportEnabledUpdate
    // flag enabled to perform this on transition to idle.
    if (m_state == PivotState_Idle && m_fPendingViewportEnabledUpdate)
    {
        m_fPendingViewportEnabledUpdate = FALSE;
        IFC_RETURN(m_callbackPtr->SetViewportEnabled(m_totalItems > 1));
    }

    // It's important to transition to the ArrangePending state after a collection
    // change so that, when using static headers, we update the viewport offset for the current index.
    if (m_state == PivotState_Idle || m_state == PivotState_Locked)
    {
        IFC_RETURN(Transition(PivotState_ArrangePending));
    }
    else
    {
        // We will update the viewport offset when we later go to idle/locked.
        m_fPendingArrange = true;
    }

    PVTRACE(L"[PSM]: ItemChange: type: %d updateVisual: %d updateIndex: %d updateItem: %d direction: %d index: %d",
        changeType, updateVisual, updateIndex, updateItem, direction, *pIndexOfInterest);

    return S_OK;
}

_Check_return_ HRESULT
PivotStateMachine::AnimationCompleteEvent()
{
    PVTRACE(L"[PSM]: AnimationCompleteEvent occurred.");

    ASSERT(m_manualAnimationInProgress);
    m_manualAnimationInProgress = false;

    bool isInDManipAnimation = false;
    IFC_RETURN(m_callbackPtr->GetIsInDManipAnimation(&isInDManipAnimation));

    // In the case of out of sequence moves (programmatic index changing)
    // we're either using the DManip ZoomToRect API or our own animation.
    // In the case of the DManip API, it doesn't give us the usual firehose
    // of ViewChanging events, so instead we'll use the animation termination to transition.
    // We need to check m_fPendingOutOfSequence move here to ensure that
    // we only catch the cases where we're not responding to a user's touch action.
    // In the case of our own animation, we want to always transition to the fly-in state
    // when the animation completes since we don't need to care about user vs. non-user
    // DManip animations, so we don't bother checking that value.
    if (m_state == PivotState_InFlyOutProgrammaticInertial)
    {
        IFC_RETURN(Transition(PivotState_InFlyInProgrammaticInertial));
    }
    else if (m_state != PivotState_InFlyOutProgrammaticInertial && !isInDManipAnimation)
    {
        ASSERT(!m_deferredSetViewportOffsetData.isPending);

        // If we're not in the fly-out state and are animating things ourselves, then
        // the animation having completed means that we're done transitioning
        // and are now where we need to be to resume normal usage.
        IFC_RETURN(RevertToIdle());
    }

    return S_OK;
}

_Check_return_ HRESULT
PivotStateMachine::HeaderMeasureEvent(float viewportSize)
{
    m_viewportSize = viewportSize;
    // We check that m_fPendingArrange is false to avoid visual glitches
    // when the user is continuously resizing pivot.
    // This happens because we are applying the secondary relationship too
    // early. We need to wait until arrange is done and when the new viewport
    // offset is set.
    if (!m_fPendingArrange)
    {
        IFC_RETURN(m_callbackPtr->SetParallaxRelationship(m_staticViewportOffset, m_pivotSectionWidth, viewportSize));
    }
    return S_OK;
}

_Check_return_ HRESULT
PivotStateMachine::IsHeaderItemsCarouselEnabledChangedEvent()
{
    if (m_state == PivotState_Idle || m_state == PivotState_Locked)
    {
        IFC_RETURN(Transition(PivotState_ArrangePending));
    }
    return S_OK;
}

_Check_return_ HRESULT
PivotStateMachine::HeaderStateChangedEvent(_In_ bool usingStaticHeaders)
{
    if (m_usingStaticHeaders != usingStaticHeaders)
    {
        m_usingStaticHeaders = usingStaticHeaders;
        if (m_state == PivotState_Idle || m_state == PivotState_Locked)
        {
            IFC_RETURN(Transition(PivotState_ArrangePending));
        }
    }
    return S_OK;
}


void
PivotStateMachine::HeaderWidthsChangedEvent(_In_ std::vector<DOUBLE> &newHeaderWidths)
{
    m_headerWidths = newHeaderWidths;
}

_Check_return_ HRESULT
PivotStateMachine::Transition(_In_ PivotState newState)
{
    BOOLEAN pendingStateQueued = FALSE;
    PivotState pendingState = PivotState_Idle;

    if (newState == m_state)
    {
        return S_OK;
    }

    BeginTransition();

    PVTRACE(L"[PSM]: Transition from %s to %s", c_pivotStateStrings[m_state], c_pivotStateStrings[newState]);

    if (newState == PivotState_ArrangePending)
    {
        m_fPendingArrange = TRUE;
    }

    // We check for both states because sometimes we don't receive enough ViewChanged events to
    // transition to FlyInInertial.
    if (((newState == PivotState_InFlyInProgrammaticInertial
        || newState == PivotState_Idle
        || newState == PivotState_InNonInertial
        || newState == PivotState_InSnapBackInertial)
        && m_state == PivotState_InFlyOutProgrammaticInertial)
        || m_state == PivotState_InFinishTransition && newState == PivotState_Idle)
    {
        if (m_fPendingOutOfSequenceMove)
        {
            m_currentIndex = m_nextIndex;
            m_staticViewportOffset = m_predictedViewportOffset;
        }
        else
        {
            // At first glance with mandatory snap points it seems like it should only be possible
            // to move a single section at a time. In practice that isn't always the case:
            // - When we're doing a programmatic move to a PivotItem more than a single index
            //   away we will temporarily disable mandatory snap pointing to allow the Pivot to
            //   correctly be animated by DManip to this new section. If the user places a finger
            //   down on the viewport during this animation they will be able to flick the Pivot
            //   unsnap pointed (edge case). To keep headers aligned we allow the Pivot to move
            //   as far as the user can flick it.
            UINT loopGuard = 0;
            while (!AreClose(m_staticViewportOffset, m_predictedViewportOffset, m_pivotSectionWidth / c_sectionWidthDividerForOffsetComparisons) && loopGuard < 1000)
            {
                loopGuard++;
                if (m_staticViewportOffset < m_predictedViewportOffset)
                {
                    m_staticViewportOffset += m_pivotSectionWidth;
                }
                else
                {
                    m_staticViewportOffset -= m_pivotSectionWidth;
                }
                SlideIndex(&m_currentIndex, m_direction);
            }
            ASSERT(loopGuard < 1000);
        }

        PVTRACE(L"[PSM]: Updating index to %d", m_currentIndex);

        IFC_RETURN(m_callbackPtr->SetSelectedIndex(m_currentIndex,  !m_fPendingItemHostValidation /* updateVisual */,
            !m_fPendingOutOfSequenceMove /* updateIndex */, !m_fPendingOutOfSequenceMove /* updateItem */, m_direction));

        if (newState == PivotState_InFlyInProgrammaticInertial)
        {
            // If we're animating things ourselves, we're done with the fly-out animation, so now we need to set the ScrollViewer position
            // to where it's expected to be.
            if (m_pendingZoomToRect)
            {
                IFC_RETURN(SetViewportOffset(m_pendingZoomToRectOffset, FALSE /* animate */));

                m_pendingZoomToRect = false;
                m_pendingZoomToRectOffset = 0.0;
            }
            else
            {
                IFC_RETURN(SetViewportOffset(m_predictedViewportOffset, FALSE /* animate */));
            }

            DOUBLE newOffset = m_staticViewportOffset;
            if (m_direction == PivotAnimationDirection_Left)
            {
                newOffset -= c_pivotAnimationRatio * m_pivotSectionWidth;
            }
            else
            {
                newOffset += c_pivotAnimationRatio * m_pivotSectionWidth;
            }

            IFC_RETURN(m_callbackPtr->SetPivotSectionOffset(newOffset));

            IFC_RETURN(m_callbackPtr->StartFlyInAnimation(m_staticViewportOffset, m_direction == PivotAnimationDirection_Left));
            m_manualAnimationInProgress = true;
            PVTRACE(L"FlyIn animation to %f.", m_staticViewportOffset);

            if (m_fPendingViewportEnabledUpdate)
            {
                m_fProgrammaticViewChangeInProgress = TRUE;
                m_fPendingViewportEnabledUpdate = FALSE;
                IFC_RETURN(m_callbackPtr->SetViewportEnabled(m_totalItems > 1));
                m_fProgrammaticViewChangeInProgress = FALSE;
            }
        }
        else
        {
            IFC_RETURN(m_callbackPtr->SetPivotSectionOffset(m_staticViewportOffset));
        }

        // NOTE: At this point we're no longer in a pending out of sequence move. We've successfully loaded the new item,
        // causing Load/Unload events to fire and bringing the new index on screen. At this point the user can
        // override the move with a new index.
        m_fPendingOutOfSequenceMove = FALSE;
    }

    // NOTE: In some cases when transitioning from FlyOutProgrammaticInertial -> Idle or
    // from FinishTransition -> Idle the previous block of code will also execute.
    // This is by design.
    if (newState == PivotState_Idle || newState == PivotState_Locked)
    {
        const bool isHeaderItemsCarouselEnabled = IsHeaderItemsCarouselEnabled();
        const double midpointViewportOffset =
            isHeaderItemsCarouselEnabled ?
            GetMidpointViewportOffset() :
            0.0; // When carousel is disabled, we don't wrap. midpointViewportOffset's value won't be used at all.

        // NOTE: Because m_fPendingOutOfSequenceMove is set to FALSE above
        // when we're transitioning FROM PivotState_InFlyOutProgrammaticInertial or
        // PivotState_InFinishTransition we will
        // only execute this code block in the edge case where Pivot has skipped
        // those states entirely, when the UI thread is busy and
        // ScrollViewer has coalesced ViewChanging events.
        if (m_fPendingOutOfSequenceMove)
        {
            if (m_currentIndex != m_nextIndex)
            {
                // This is the case where the end user stopped
                // the pending changeview animation. They could also stop
                // it almost exactly on center, which results in us being
                // in the non inertial state.
                ASSERT(m_state == PivotState_InSnapBackInertial ||
                    m_state == PivotState_InNonInertial);

                m_currentIndex = m_nextIndex;
                IFC_RETURN(m_callbackPtr->SetSelectedIndex(m_currentIndex, !m_fPendingItemHostValidation /* updateVisual */, FALSE /* updateIndex */, FALSE /* updateItem */, PivotAnimationDirection_Reset));
            }
            m_fPendingOutOfSequenceMove = FALSE;
        }

        const bool shouldNormalize =
            isHeaderItemsCarouselEnabled &&    // Normalization is not required when carousel is disabled headers.
            abs(m_staticViewportOffset - midpointViewportOffset) > c_sectionNormalizationThreshold * m_pivotSectionWidth;

        if (m_fPendingInitialization || m_fPendingArrange || shouldNormalize)
        {
            m_staticViewportOffset =
                isHeaderItemsCarouselEnabled ?
                midpointViewportOffset :
                m_currentIndex * m_pivotSectionWidth;

            IFC_RETURN(m_callbackPtr->SetPivotSectionOffset(m_staticViewportOffset));

            IFC_RETURN(SetViewportOffset(m_staticViewportOffset, FALSE));

            // This call causes the ItemsPresenter to sync correctly and generate an inital Section changed event.
            if (m_totalItems > 0)
            {
                // If we're pending an ItemHostValidation we should never see m_fPendingInitialization
                // set. The flags are mutually exclusive and if they were both set we could end up
                // with an inconsistent Items property. When m_fPendingItemsHostValidation is set here
                // we should only be normalizing the viewport or finishing up an Arrange that also
                // occurred simultaneously.
                ASSERT(!(m_fPendingItemHostValidation && m_fPendingInitialization));
                IFC_RETURN(m_callbackPtr->SetSelectedIndex(m_currentIndex, !m_fPendingItemHostValidation /* updateVisual */, FALSE /* updateIndex */, m_fPendingInitialization /* updateItem */, PivotAnimationDirection_Reset));
            }

            PVTRACE(L"[PSM]: SectionWidth: %f, ViewportOffSet: %f",
                m_pivotSectionWidth, m_staticViewportOffset);
            m_fPendingInitialization = FALSE;
            m_fPendingArrange = FALSE;
        }

        if (m_fSnapPointBehaviorRelaxed)
        {
            IFC_RETURN(m_callbackPtr->SetSnappingBehavior(TRUE));
            m_fSnapPointBehaviorRelaxed = FALSE;
        }

        if (m_fPendingViewportEnabledUpdate)
        {
            m_fPendingViewportEnabledUpdate = FALSE;
            IFC_RETURN(m_callbackPtr->SetViewportEnabled(m_totalItems > 1 && newState != PivotState_Locked));
        }

        IFC_RETURN(m_callbackPtr->UpdateFocusFollower());
    }

    if (newState == PivotState_InFlyOutProgrammaticInertial)
    {
        PVTRACE(L"FlyOut animation from %f.", m_staticViewportOffset);
        IFC_RETURN(m_callbackPtr->StartFlyOutAnimation(m_staticViewportOffset, m_pendingHeaderShiftOffset, m_direction == PivotAnimationDirection_Right));
        m_manualAnimationInProgress = true;
        m_pendingHeaderShiftOffset = 0.0;
    }

    if (newState == PivotState_ProgrammaticInertial)
    {
        // We run into problems if the user can pan while the programmatic view change
        // is in progress, so we need to disable the viewport while that's happening.
        m_fProgrammaticViewChangeInProgress = TRUE;
        m_fPendingViewportEnabledUpdate = TRUE;
        IFC_RETURN(m_callbackPtr->SetViewportEnabled(FALSE));
        m_fProgrammaticViewChangeInProgress = FALSE;

        m_pendingZoomToRect = true;
        m_pendingZoomToRectOffset = m_predictedViewportOffset;

        // When a programmatic change occurs we force the index no matter what happens using the
        // m_fPendingOutOfSequenceMove flag. We will ensure that this index is displayed when
        // the viewport next goes idle. We fire the event handlers now to make things seem
        // a little simpler to the developer.
        IFC_RETURN(m_callbackPtr->SetSelectedIndex(m_nextIndex, FALSE /* updateVisual */, TRUE /* updateIndex */, TRUE /* updateItem */, m_direction));
        pendingState = PivotState_InFlyOutProgrammaticInertial;
        pendingStateQueued = TRUE;
    }

    if (newState == PivotState_Locked)
    {
        ASSERT(m_state == PivotState_Idle || m_state == PivotState_LockedPending || m_state == PivotState_ArrangePending);

        // If the developer toggled isLocked back while we were
        // completing the previous manipulation ensure we move back into
        // the idle state.
        if (!m_fIsLocked)
        {
            pendingState = PivotState_Idle;
            pendingStateQueued = TRUE;
        }
    }

    if (newState == PivotState_LockedPending)
    {
        // We're in a ongoing animation. This will cause the
        // viewport to stop moving and fire off a final ViewChanging/ViewChanged
        // message, which will transition us to Locked.
        m_fPendingViewportEnabledUpdate = FALSE;
        IFC_RETURN(m_callbackPtr->SetViewportEnabled(FALSE));
    }

    m_state = newState;
    EndTransition();

    if (pendingStateQueued)
    {
        IFC_RETURN(Transition(pendingState));
    }

    return S_OK;
}

void
PivotStateMachine::SlideIndex(_Inout_ INT32* idx, _In_ PivotAnimationDirection dir) const
{
    if (dir == PivotAnimationDirection_Left)
    {
        (*idx)--;
    }
    else if (dir == PivotAnimationDirection_Right)
    {
        (*idx)++;
    }

    *idx = PositiveMod(*idx, m_totalItems);
}

PivotAnimationDirection
PivotStateMachine::DetermineDirection(_In_ INT32 idx, _In_ BOOLEAN isFromHeaderTap) const
{
    if (isFromHeaderTap)
    {
        // If we're using static headers, the direction from a header tap depends on
        // whether the tapped header is to the right or to the left of the current header.
        if (m_usingStaticHeaders)
        {
            return idx < m_currentIndex ? PivotAnimationDirection_Left : PivotAnimationDirection_Right;
        }
        else
        {
            // If we're using dynamic headers or aren't animating things ourselves, then the current header is always on the left,
            // so any tapped headers are going to always be on the right.
            return PivotAnimationDirection_Right;
        }
    }
    else
    {
        if (m_totalItems == 2)
        {
            return idx < m_currentIndex ? PivotAnimationDirection_Left : PivotAnimationDirection_Right;
        }
        else
        {
            // If this is the immediately previous section increment Left, otherwise
            // increment Right.
            INT32 idxIncremented = idx;
            SlideIndex(&idxIncremented, PivotAnimationDirection_Right);
            return (idxIncremented == m_currentIndex) ? PivotAnimationDirection_Left : PivotAnimationDirection_Right;
        }
    }
}

_Check_return_ HRESULT
PivotStateMachine::SetViewportOffset(DOUBLE offset, BOOLEAN animate)
{
    PVTRACE(L"[PSM]: SetViewportOffset called. offset = %f, animate = %d", offset, animate);

    bool isInDManipAnimation = false;
    IFC_RETURN(m_callbackPtr->GetIsInDManipAnimation(&isInDManipAnimation));

    if (isInDManipAnimation)
    {
        PVTRACE(L"[PSM]: DManip animation in progress.  Canceling it and queuing the setting for later.");

        // If a DManip animation is currently in progress, then calling
        // SetViewportOffset will double-up on top of the existing animation,
        // which is bad.  To account for this, we'll cancel the existing animation,
        // then wait for it to complete (i.e., wait for FinalViewEvent() to be called),
        // and then we'll call SetViewportOffset() once that's done.
        m_deferredSetViewportOffsetData.isPending = true;
        m_deferredSetViewportOffsetData.offset = offset;
        m_deferredSetViewportOffsetData.shouldAnimate = !!animate;

        m_fProgrammaticViewChangeInProgress = TRUE;
        IFC_RETURN(m_callbackPtr->CancelDManipAnimations());
        m_fProgrammaticViewChangeInProgress = FALSE;
    }
    else
    {
        PVTRACE(L"[PSM]: No DManip animation in progress.  Setting the offset.");

        // If we happen to be at the offset that we're trying to set the viewport offset to,
        // then SetViewportOffset will return false.  In this case, we will have
        // no DManip animation in progress to wait for.
        bool success = false;

        m_fProgrammaticViewChangeInProgress = !animate;
        IFC_RETURN(m_callbackPtr->SetViewportOffset(offset, animate, &success));
        m_fProgrammaticViewChangeInProgress = FALSE;
    }

    return S_OK;
}

_Check_return_ HRESULT
PivotStateMachine::RevertToIdle()
{
    PVTRACE(L"[PSM]: Attempting to revert to idle.");

    if (m_state != PivotState_Locked &&
        m_state != PivotState_Idle &&
        m_state != PivotState_ArrangePending &&
        !m_fProgrammaticViewChangeInProgress)
    {
        if (m_fIsLocked)
        {
            IFC_RETURN(Transition(PivotState_Locked));
        }
        else
        {
            IFC_RETURN(Transition(PivotState_Idle));
        }
    }
    else if (m_fProgrammaticViewChangeInProgress)
    {
        PVTRACE(L"[PSM]: Ignoring. Programmatic view change in progress.");
    }
    else if (m_state == PivotState_ArrangePending)
    {
        PVTRACE(L"[PSM]: Ignoring. Arrange is pending. We will get a chance to go to Idle once ArrangeOverride is called.");
    }
    else
    {
        PVTRACE(L"[PSM]: Ignoring. Already in idle state.");
    }

    return S_OK;
}

bool PivotStateMachine::InScrollingStaticHeadersMode() const
{
    return m_usingStaticHeaders;
}

bool PivotStateMachine::IsHeaderItemsCarouselEnabled() const
{
    return m_callbackPtr->IsHeaderItemsCarouselEnabled();
}


} } } } XAML_ABI_NAMESPACE_END