// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ParallelTimeline.h"
#include "Timemgr.h"
#include "TimelineCollection.h"
#include "RepeatBehavior.h"
#include <UIThreadScheduler.h>
#include "TimeSpan.h"

using namespace DirectUI;

KnownTypeIndex CParallelTimeline::GetTypeIndex() const
{
    return DependencyObjectTraits<CParallelTimeline>::Index;
}

// Recursively fire Complete event for itself and children Timelines.
HRESULT CParallelTimeline::FireCompletedEvent()
{
    ASSERT(!IsCompletedEventFired());

    if (m_pChild != NULL)
    {
        XUINT32 nCount = 0;
        nCount = m_pChild->GetCount();
        for (XUINT32 i = 0; i < nCount; i++)
        {
            xref_ptr<CTimeline> pTimeline;
            pTimeline.attach(static_cast<CTimeline*>(m_pChild->GetItemWithAddRef(i)));
            if (!pTimeline->IsCompletedEventFired())
            {
                IFC_RETURN(pTimeline->FireCompletedEvent());
            }
        }
    }
    IFC_RETURN(CTimeline::FireCompletedEvent());

    return S_OK;
}

// Determine if this is an interesting timeline for animation tracking purposes.
bool CParallelTimeline::IsInterestingForAnimationTracking()
{
    // If the timeline is delayed, ignore it: e.g. it may be speculative
    // animation scheduled into the future to auto-hide etc.
    if (m_pBeginTime && ((m_pBeginTime->m_rTimeSpan * 1000) > c_AnimationTrackingMaxBeginTimeInMs))
    {
        return false;
    }

    // We will require at least one child to be of interest...
    bool isInterestingForAnimationTracking = false;
    CTimeline *pTimeline = NULL;
    XUINT32 nCount = 0;

    if (m_pChild != NULL)
    {
        nCount = m_pChild->GetCount();
        for (XUINT32 i = 0; i < nCount; i++)
        {
            pTimeline = reinterpret_cast<CTimeline *>(m_pChild->GetItemWithAddRef(i));
            ASSERT(pTimeline != nullptr);
            isInterestingForAnimationTracking = pTimeline->IsInterestingForAnimationTracking();
            if (isInterestingForAnimationTracking)
            {
                break;
            }

            ReleaseInterface(pTimeline);
        }
    }

    ReleaseInterface(pTimeline);

    return isInterestingForAnimationTracking;
}

void CParallelTimeline::AnimationTrackingCollectInfoNoRef(
    _Inout_ CDependencyObject** ppTarget,
    _Inout_ CDependencyObject** ppDynamicTimeline
    )
{
    CTimeline *pTimeline = NULL;
    XUINT32 nCount = 0;

    if (m_pChild != NULL)
    {
        nCount = m_pChild->GetCount();
        for (XUINT32 i = 0; i < nCount; i++)
        {
            pTimeline = reinterpret_cast<CTimeline *>(m_pChild->GetItemWithAddRef(i));
            pTimeline->AnimationTrackingCollectInfoNoRef(ppTarget, ppDynamicTimeline);

            // If we got all the information, break out early.
            if (*ppTarget && *ppDynamicTimeline)
            {
                break;
            }

            ReleaseInterface(pTimeline);
        }
    }

    ReleaseInterface(pTimeline);
}

// Handles scheduling frames and firing the Completed event.
// TODO: ANIM: This common implementation should probably move all the way down to Timeline (where the Completed event is defined), but I didn't want to impact DispatcherTimer or the root timelines for now.
_Check_return_ HRESULT CParallelTimeline::UpdateAnimation(
    _In_ const ComputeStateParams &parentParams,
    _Inout_ ComputeStateParams &myParams,
    XDOUBLE beginTime,
    DurationType durationType,
    XFLOAT durationValue,
    _In_ const COptionalDouble &expirationTime,
    _Out_ bool *pIsIndependentAnimation
    )
{
    switch (m_clockState)
    {
        case DirectUI::ClockState::NotStarted:
            if (parentParams.pFrameSchedulerNoRef != NULL)
            {
                // Request a tick to the active period when running forwards.
                IFC_RETURN(RequestTickToActivePeriodBegin(parentParams, myParams, beginTime));
            }

            break;

        case DirectUI::ClockState::Stopped:
            // We only want to call any event handlers and/or finalization code if we actually started once
            if (m_fInitialized)
            {
                // We only want this to be fired once on the last transition
                // There could be the case when the previous state was Stopped and current state was so late that
                // it got computed stopped, in such case we need to make sure that we fire the completed event.
                if (!m_IsCompletedEventFired)
                {
                    IFC_RETURN(FireCompletedEvent());
                }

                IFC_RETURN(FinalizeIteration());
            }

            if (parentParams.pFrameSchedulerNoRef != NULL)
            {
                // Request a tick to the active period when running in reverse.
                IFC_RETURN(RequestTickToActivePeriodEnd(parentParams, myParams, expirationTime));
            }

            break;

        case DirectUI::ClockState::Active:
            ASSERT(parentParams.hasTime);
            ASSERT(parentParams.time >= beginTime);

            // The natural duration should always be a finite timespan or infinite duration.
            // Automatic durations will have been resolved to one or the other.
            ASSERT(durationType == DirectUI::DurationType::TimeSpan || durationType == DirectUI::DurationType::Forever);

            if (CanRequestTicksWhileActive() && parentParams.pFrameSchedulerNoRef != NULL && !myParams.isPaused)
            {
                // ParallelTimelines don't need to constantly tick while active. The frequency of ticks in a single iteration
                // is determined by the animation children of the timeline, if any. The ParallelTimeline does have to
                // tick at the beginning of each iteration, however, because the animation children could be 'Filling' in
                // the current iteration, and not requesting ticks, but will restart in 'Active' mode in the next iteration.
                // The ParallelTimeline also needs to ensure it requests a tick for the time it expires, so that we can fire
                // the Completed event, even if there are no animation children. Since the expiration time will also be at
                // the terminus of an iteration, it's covered by the same logic.
                //
                // The timeline only has meaningful endpoints for each iteration if it is finite.
                if (durationType == DirectUI::DurationType::TimeSpan)
                {
                    const XDOUBLE timeElapsedInIteration = XcpMod(m_rCurrentTime, durationValue);

                    // Note that we use the combined 'isInReverse' flag, including the local AutoReverse value,
                    // to match that the inputs to this calculation are in local time units.
                    XDOUBLE timeRemainingInIterationInSeconds = myParams.isReversed
                                                             ? timeElapsedInIteration
                                                             : durationValue - timeElapsedInIteration;

                    // Note that if the timeline ends up exactly at (or over, due to floating point error) the end of an
                    // iteration and is still active, it still requests a frame. This guarantees that we always tick at the
                    // beginning of the next iteration, so any child timelines have a chance to restart.
                    timeRemainingInIterationInSeconds = MAX(0.0f, timeRemainingInIterationInSeconds);

                    IFC_RETURN(parentParams.pFrameSchedulerNoRef->RequestAdditionalFrame(
                        XcpCeiling(timeRemainingInIterationInSeconds * 1000.0 / myParams.speedRatio),
                        RequestFrameReason::AnimationTick));
                }
            }
            break;

        case DirectUI::ClockState::Filling:
            if (m_fInitialized)
            {
                // Also want to send a completed event here so that animations without
                // FillBehavior="Stop" get the events as well
                if (!m_IsCompletedEventFired)
                {
                    IFC_RETURN(FireCompletedEvent());
                }
            }

            if (parentParams.pFrameSchedulerNoRef != NULL)
            {
                // Request a tick to the active period when running in reverse.
                IFC_RETURN(RequestTickToActivePeriodEnd(parentParams, myParams, expirationTime));
            }

            break;

        default:
            ASSERT(FALSE, L"Unsupported state");
    }

    // Parallel timelines are not independent animations themselves (e.g. Storyboard, DynamicTimeline).
    // They might have independent animation children, which are accounted for in CTimelineGroup::ComputeState.
    *pIsIndependentAnimation = FALSE;

    return S_OK;
}
