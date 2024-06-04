// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "strsafe.h"
#include "Storyboard.h"
#include "Timemgr.h"
#include "TimelineCollection.h"
#include "RepeatBehavior.h"
#include "animation.h"
#include "DCompTreeHost.h"
#include <VisualTransitionCompletedData.h>
#include <UIThreadScheduler.h>
#include "TimeSpan.h"

using namespace DirectUI;

// Callback we invoke to notify the test framework that we started a storyboard.
/*static*/ std::function<HRESULT(CDependencyObject* /* storyboard */, CDependencyObject* /* target */)> CStoryboard::s_storyboardStartedCallback;

CStoryboard::~CStoryboard()
{
    // This storyboard has been deleted prior to the transition completed event being fired.
    // Cleanup after the transition object...

    if (m_pLayoutTransitionCompletedData)
    {
        delete m_pLayoutTransitionCompletedData;
        m_pLayoutTransitionCompletedData = NULL;
    }

    AnimationStopTracking();
}

// Overridden just to fire ETW events and signal animation tracking.
_Check_return_ HRESULT CStoryboard::UpdateAnimation(
    _In_ const ComputeStateParams &parentParams,
    _Inout_ ComputeStateParams &myParams,
    XDOUBLE beginTime,
    DurationType durationType,
    XFLOAT durationValue,
    _In_ const COptionalDouble &expirationTime,
    _Out_ bool *pIsIndependentAnimation
    )
{
    if ((m_clockState == DirectUI::ClockState::Stopped) || (m_clockState == DirectUI::ClockState::Filling))
    {
        AnimationStopTracking();
    }

    if (EventEnabledEndStoryboardInfo())
    {
        switch (m_clockState)
        {
        case DirectUI::ClockState::NotStarted:
            break;

        case DirectUI::ClockState::Stopped:
            if (m_fInitialized)
            {
                if (!m_IsCompletedEventFired)
                {
                    TraceEndStoryboardInfo((XUINT64)static_cast<CDependencyObject*>(this));
                }
            }
            break;

        case DirectUI::ClockState::Active:
            break;

        case DirectUI::ClockState::Filling:
            if (m_fInitialized)
            {
                if (!m_IsCompletedEventFired)
                {
                    TraceEndStoryboardInfo((XUINT64)static_cast<CDependencyObject*>(this));
                }
            }
            break;

        default:
            ASSERT(FALSE, L"Unsupported state");
        }
    }

    // Call into base implementation to do the real work.
    IFC_RETURN(CParallelTimeline::UpdateAnimation(
        parentParams,
        myParams,
        beginTime,
        durationType,
        durationValue,
        expirationTime,
        pIsIndependentAnimation
        ));

    return S_OK;
}

_Check_return_ HRESULT CStoryboard::ComputeStateImpl(
    _In_ const ComputeStateParams &parentParams,
    _Inout_ ComputeStateParams &myParams,
    _Inout_opt_ bool *pHasNoExternalReferences,
    bool hadIndependentAnimationLastTick,
    _Out_ bool *pHasIndependentAnimation
    )
{
    bool hasIndependentAnimation = false;

    // Stopped storyboards are skipped over.
    if (m_fIsStopped)
    {
        ASSERT(!m_fIsPaused && !m_fIsResuming && !m_fIsBeginning && !m_fIsSeeking);
    }
    else
    {
        if (m_fIsPaused && !myParams.isPaused)
        {
            myParams.isPaused = true;
        }

        if (parentParams.hasTime)
        {
            bool wasSeeking = m_fIsSeeking;

            // The effective parent time is the adjusted time at which this storyboard will be computed.
            ComputeStateParams effectiveParentParams(parentParams);

            // If this Storyboard hasn't been ticked in the timing tree yet, snap its initial parent time now.
            // Storyboards that Begin and Pause immediately, for example, will Pause at the first tick.
            // Note that when SeekAlignedToLastTick is called before the storyboard starts, m_lastParentTime
            // will remain invalid.
            if (m_lastParentTime == XDOUBLE_MAX)
            {
                //ASSERT(IsInPrimaryTimeManager());
                m_lastParentTime = parentParams.time;
            }

            // If this is the first tick of a top-level storyboard after Begin, snap its time
            // delta relative to the parent's time now so this becomes the initial tick time.
            if (m_fIsBeginning)
            {
                // The SB might be paused, resuming, or seeking as well as beginning.
                ASSERT(IsTopLevelStoryboard());
                //ASSERT(IsInPrimaryTimeManager());
                m_rTimeDelta = -parentParams.time;

                m_fIsBeginning = FALSE;
            }

            // If the storyboard is paused, the last snapped parent time is when the paused occurred.
            const XDOUBLE rPauseStartTime = m_lastParentTime;

            // If paused, use the pause start time instead of the current parent time to compute state.
            // If not paused, keep the snapped parent time value up-to-date.
            if (m_fIsPaused)
            {
                // The SB might have been beginning, and might be seeking as well as paused.
                ASSERT(IsTopLevelStoryboard());
                // TODO: HWPC: We should be able to ASSERT that this state-handling code isn't needed on the composition thread for any of these states.
                // TODO: HWPC: This is blocked by dynamic timelines getting cloned over when paused - see Win8 #567534.
                //  ASSERT(IsInPrimaryTimeManager());
                ASSERT(!m_fIsResuming);
                ASSERT(rPauseStartTime >= 0.0);
                effectiveParentParams.time = rPauseStartTime;

                // NOTE: There's a potential issue where a Pause occurs exactly one tick after a Begin.  In that case,
                //       the independent time delta will have been snapped, but the last tick time (which is zero) will
                //       not yet include it.  The pause time will then end up being zero minus the independent time delta
                //       instead of just zero.  However, CTimeline::ComputeState clamps the current time to zero on the low
                //       end, so this isn't a practical concern.
            }
            else
            {
                m_lastParentTime = parentParams.time;
            }

            // If the storyboard is resuming, increase the time delta from the parent timeline by the time spent paused.
            if (m_fIsResuming)
            {
                // The SB might have been beginning, and might be seeking as well as resuming.
                ASSERT(IsTopLevelStoryboard());
                //ASSERT(IsInPrimaryTimeManager());
                ASSERT(!m_fIsPaused);
                ASSERT(rPauseStartTime >= 0.0);
                ASSERT(rPauseStartTime <= parentParams.time);
                m_rTimeDelta += rPauseStartTime - parentParams.time;

                m_fIsResuming = FALSE;
            }

            // Calculate a time delta so that the result of adjusting the parent time is exactly the seeked time.
            if (m_fIsSeeking)
            {
                // The SB might have been beginning, paused, or resuming in addition to seeking.
                ASSERT(IsTopLevelStoryboard());
                //ASSERT(IsInPrimaryTimeManager());
                ASSERT(m_rPendingSeekTime != XDOUBLE_MAX);

                // If the SB has a valid parent time, the seek can complete by adjusting the time delta from the parent to the seeked time.
                // Otherwise, this is a SeekAlignedToLastTick call before the SB was actually ticked, which is special-cased later.
                if (m_lastParentTime != XDOUBLE_MAX)
                {
                    ASSERT(effectiveParentParams.time >= 0.0);
                    m_rTimeDelta = m_rPendingSeekTime - effectiveParentParams.time;
                    m_fIsSeeking = FALSE;
                }
            }
            else
            {
                // m_lastParentTime is always initialized on the first tick of the storyboard.
                // ComputeState can only be called before the first tick via SeekAlignedToLastTick,
                // so if we weren't seeking it must be valid.
                ASSERT(m_lastParentTime >= 0.0 && m_lastParentTime != XDOUBLE_MAX);
            }

            // Adjust for the storyboard's difference in time from its parent's time.
            if (m_fIsSeeking)
            {
                // Special case for SeekAlignedToLastTick calls before the SB was ever ticked from the time manager.
                // In this case, we don't have a valid parent time yet so we can't account for the seek time by adjusting
                // our time delta from the parent.  Instead, we use the seek time directly for this ComputeState, so the
                // values get set correctly for the synchronous call, but we remain in the seeking state until the first
                // real tick where the seek adjustment will be incorporated into the stored time delta as usual.
                effectiveParentParams.time = m_rPendingSeekTime;
            }
            else
            {
                // TODO: HWPC: This calculation induces floating point error that can grow the longer the app has been running.
                // TODO: HWPC: This is especially noticeable when calling SeekPrivate(t_seek). The following calculation:
                // TODO: HWPC: m_rTimeDelta = m_rPendingSeekTime - rEffectiveParentTime
                // TODO: HWPC: rEffectiveParentTime = rEffectiveParentTime + m_rTimeDelta
                // TODO: HWPC: is expected to return rEffectiveParentTime == m_rPendingSeekTime. However, the
                // TODO: HWPC: magnitude of the parent time and time delta can be much larger than the seek
                // TODO: HWPC: time, causing the result to differ by more than IsCloseReal allows.
                ASSERT(m_rTimeDelta != XDOUBLE_MAX);
                effectiveParentParams.time += m_rTimeDelta;
            }

            // Time adjustments should never be necessary for nested storyboards.
            ASSERT(IsTopLevelStoryboard() || m_rTimeDelta == 0.0);

            //
            // Due to floating point errors, sometimes the effective parent time is slightly before the begin time when
            // it should be equal. Include a tolerance in calculations to allow these storyboards to begin. Currently most
            // of the error comes from converting from double to float, but even switching entirely to float will not
            // solve the problem without including a tolerance.
            //
            // We only make this adjustment for root storyboards, since they're the only ones with independent start deltas.
            // We also want to avoid snapping the time forward in each level of nested storyboards.
            //
            if (this == GetRootTimingParent())
            {
                const XDOUBLE rBeginTime = m_pBeginTime ? m_pBeginTime->m_rTimeSpan : 0.0;
                const XDOUBLE secondsToBeginTime = rBeginTime - effectiveParentParams.time;

                if (0.0 < secondsToBeginTime && secondsToBeginTime < CTimeline::s_timeTolerance
                    && (!wasSeeking || m_rPendingSeekTime >= rBeginTime))
                {
                    effectiveParentParams.time = rBeginTime;
                }
            }

            // Compute local state
            IFC_RETURN(CTimelineGroup::ComputeStateImpl(
                effectiveParentParams,
                myParams,
                pHasNoExternalReferences,
                hadIndependentAnimationLastTick,
                &hasIndependentAnimation
                ));
        }
        else
        {
            // Top-level storyboards should always be provided a valid parent time from the TimeManager.
            // Nested storyboards might not have a parent time, in the case where their parent timeline has not started yet.
            ASSERT(!IsTopLevelStoryboard());
            ASSERT(GetTimingParent() != NULL && GetTimingParent()->m_clockState == DirectUI::ClockState::NotStarted);

            // The effective parent time is the adjusted time at which this storyboard will be computed.
            ComputeStateParams parentParamsNoTime(parentParams);
            parentParamsNoTime.hasTime = false;

            // Compute local state
            IFC_RETURN(CTimelineGroup::ComputeStateImpl(
                parentParamsNoTime,
                myParams,
                pHasNoExternalReferences,
                hadIndependentAnimationLastTick,
                &hasIndependentAnimation
                ));
        }
    }

    *pHasIndependentAnimation = hasIndependentAnimation;

    return S_OK;
}

// Pauses an unpaused storyboard.  No-ops if stopped.
_Check_return_ HRESULT CStoryboard::PausePrivate()
{
    AnimationStopTracking();

    if (!m_fIsStopped && !m_fIsPaused)
    {
        // When paused, the parent time no longer moves forward during ComputeState.
        m_fIsPaused = TRUE;
        m_fIsResuming = FALSE;

        // There's no need to notify the time manager of a timing change for pausing here, since the pause is
        // handled on the next tick.  If the storyboard was independent and is paused on the next tick, it'll be
        // removed from the time manager and we'll detect the independent-animation change there.

        // Request a tick to update the now-inactive storyboard.
        // The browser host and/or frame scheduler can be NULL during shutdown.
        IXcpBrowserHost *pBH = GetContext()->GetBrowserHost();
        if (pBH != NULL)
        {
            ITickableFrameScheduler *pFrameScheduler = pBH->GetFrameScheduler();

            if (pFrameScheduler != NULL)
            {
                IFC_RETURN(pFrameScheduler->RequestAdditionalFrame(0 /* immediate */, RequestFrameReason::StoryboardTick));
            }
        }

        TracePauseStoryboardInfo((XUINT64)static_cast<CDependencyObject*>(this));
    }

    // We requested another tick, which will process the pause and removes the timeline from the time manager.
    // That will mark an independent timeline change, which will cause us to commit the DComp device.
    AddPendingDCompPause();

    return S_OK;
}

// Resumes a paused storyboard.  No-ops if stopped.
_Check_return_ HRESULT CStoryboard::ResumePrivate()
{
    auto core = GetContext();
    CTimeManager* pTimeManager = core->GetTimeManager();
    IFCPTR_RETURN(pTimeManager);

    if (!m_fIsStopped && m_fIsPaused)
    {
        // When resumed, the time delta from the parent timeline will be adjusted on the next tick.
        m_fIsPaused = FALSE;
        m_fIsResuming = TRUE;

        TraceResumeStoryboardInfo((XUINT64)static_cast<CDependencyObject*>(this));

        // Add the Storyboard to the timing tree, if necessary.
        if (GetTimingParent() == NULL)
        {
            IFC_RETURN(pTimeManager->AddTimeline(this));
        }
        // If the Storyboard is already running and has independent animations, notify the time manager of the timing change.
        // This will generate a frame for the render thread to pick up the adjusted storyboard time.
        else if (HasIndependentAnimation())
        {
            pTimeManager->NotifyIndependentAnimationChange();
        }

        // Request a tick to update the now-active storyboard.
        // The browser host and/or frame scheduler can be NULL during shutdown.
        IXcpBrowserHost *pBH = core->GetBrowserHost();
        if (pBH != NULL)
        {
            ITickableFrameScheduler *pFrameScheduler = pBH->GetFrameScheduler();

            if (pFrameScheduler != NULL)
            {
                IFC_RETURN(pFrameScheduler->RequestAdditionalFrame(0 /* immediate */, RequestFrameReason::StoryboardTick));
            }
        }
    }

    // We added the timeline to the time manager and requested another tick, which will processe the resume and
    // will find that this is an independent animation. That will mark an independent timeline change in the time
    // manager, which will cause us to commit the DComp device.
    AddPendingDCompResume();

    return S_OK;
}

// Starts the current Storyboard from the beginning.  Overrides any previous stops/pauses/seeks.
_Check_return_ HRESULT CStoryboard::BeginPrivate(bool fIsTopLevel)
{
    auto cleanupGuard = wil::scope_exit([&]
    {
        // Reset our state if we fail to begin so that completed doesn't fire on error
        m_fIsStopped = TRUE;
        m_IsCompletedEventFired = TRUE;
    });

    auto core = GetContext();
    CTimeManager* pTimeManager = core->GetTimeManager();
    IFCPTR_RETURN(pTimeManager);

    // Add the Storyboard to the timing tree if necessary.
    if (!GetTimingParent())
    {
        IFC_RETURN(pTimeManager->AddTimeline(this));
    }
    // If the Storyboard is already running and has independent animations, notify the time manager of the timing change.
    // This will generate a frame for the render thread to pick up the re-started storyboard.
    else if (HasIndependentAnimation())
    {
        pTimeManager->NotifyIndependentAnimationChange();
    }

    // We need to reset internal iteration helpers by re-initializing
    m_clockState = DirectUI::ClockState::NotStarted;
    ResetCompletedEventFired();

    // Notify child timelines as needed.  It's important that this happens before we reset
    // the rest of the storyboard's state for ticking, e.g. m_fIsStopped, because notifying child
    // timelines might modify the timeline tree, which can only occur when stopped.
    if (m_pChild)
    {
        HRESULT hrChild;

        // Tell children nodes that we are beginning
        for (const auto& child : (*m_pChild))
        {
            CTimeline* pTimeline = do_pointer_cast<CTimeline>(child);
            IFCPTR_RETURN(pTimeline);

            // Compute begin behavior for child nodes ...
            hrChild = pTimeline->OnBegin();
            if (FAILED(hrChild))
            {
                // Make sure we destroy any references
                IFC_RETURN(FinalizeIteration());
                IFC_RETURN(hrChild);
            }

            if (EventEnabledBeginStoryboardInfo())
            {
                CDependencyObject* pTargetNoRef = pTimeline->GetTargetObjectNoRef();
                TraceBeginStoryboardInfo(reinterpret_cast<UINT64>(static_cast<CDependencyObject*>(this)), reinterpret_cast<UINT64>(pTargetNoRef));
            }

            if (EventEnabledBeginStoryboardWithSourceInfo())
            {
                xstring_ptr uri;
                xstring_ptr hash;
                unsigned int line = 0;
                unsigned int column = 0;

                if (TryGetSourceInfoFromPeer(&line, &column, &uri, &hash))
                {
                    TraceBeginStoryboardWithSourceInfo(
                        reinterpret_cast<UINT64>(this),
                        reinterpret_cast<UINT64>(pTimeline),
                        uri.GetBuffer(),
                        line,
                        column,
                        hash.GetBuffer());
                }
            }
        }
    }

    // For top level storyboards, signal the beginning of a scenario for animation tracking.
    if (fIsTopLevel)
    {
        AnimationTrackingBeginScenario(/*pTransition*/nullptr, /*pTargetObject*/nullptr);
        EnsureTelemetryName();
    }

    // Top-level storyboards need to offset time from their parent timeline (the time manager).
    // Nested timelines inherit their parent's time and need no adjustment.
    //
    // Since the framework ticks only as needed, the last tick time is not reliably close to
    // the actual time when the Begin API is called.  It's also not acceptable to use the
    // actual wall clock time, since multiple storyboards started in one callback are expected
    // to start simultaneously.  The only option left is to snap the time at the next tick instead,
    // where the storyboard will actually start.
    m_rTimeDelta = 0.0;
    m_fIsBeginning = fIsTopLevel;

    // Reset the last parent time to put the Storyboard back into its initial state, as though it hasn't ticked yet.
    // This is okay because any Pause before the Begin is cancelled, and any subsequent Pause should work just like
    // when the Storyboard was first added to the timing tree.
    m_lastParentTime = XDOUBLE_MAX;

    // Reset other state.  Begin removes the paused state and overrides pending resumes and seeks.
    m_fIsStopped = FALSE;
    m_fIsPaused = FALSE;
    m_fIsResuming = FALSE;
    m_fIsSeeking = FALSE;

    // Request a tick to update the now-active storyboard.
    // The browser host and/or frame scheduler can be NULL during shutdown.
    IXcpBrowserHost *pBH = core->GetBrowserHost();
    if (pBH != NULL)
    {
        ITickableFrameScheduler *pFrameScheduler = pBH->GetFrameScheduler();

        if (pFrameScheduler != NULL)
        {
            IFC_RETURN(pFrameScheduler->RequestAdditionalFrame(0 /* immediate */, RequestFrameReason::StoryboardTick));
        }
    }

    if (fIsTopLevel)
    {
        if (m_fAutoComplete && !m_fIsStopped)
        {
            IFC_RETURN(CompleteInternal(TRUE, TRUE));
        }
    }

    // We let the test framework know about the storyboard we just started.
    if (s_storyboardStartedCallback)
    {
        IFC_RETURN(s_storyboardStartedCallback(this, GetTargetObjectNoRef()));
    }

    InitializeIteration();

    cleanupGuard.release();

    return S_OK;
}

// Stops the current storyboard and does not fill. Overrides previous begins/pauses/seeks.
_Check_return_ HRESULT CStoryboard::StopPrivate()
{
    m_fIsStopped = TRUE;
    m_fIsPaused = FALSE;
    m_fIsResuming = FALSE;
    m_fIsBeginning = FALSE;
    m_fIsSeeking = FALSE;

    m_lastParentTime = XDOUBLE_MAX;
    m_rPendingSeekTime = XDOUBLE_MAX;
    m_rTimeDelta = XDOUBLE_MAX;

    AnimationStopTracking();

    m_storyboardTelemetryName = xstring_ptr::NullString();

    if (m_clockState != DirectUI::ClockState::Stopped)
    {
        auto core = GetContext();

        m_clockState = DirectUI::ClockState::Stopped;

        if (core->GetTimeManager()->IsRunning())
        {
            // Force all timing subtree back to the base value
            IFC_RETURN(FinalizeIteration());

            // WinRT DComp will still fire Completed events if the animations are explicitly stopped. Make sure we unattach from
            // the events and clear out the map in CTimeManager so that Xaml doesn't think the WUC animations completed.
            DetachDCompCompletedHandlerOnStop();

            // If we are running, we should also remove ourselves from the time manager.
            // If this Storyboard was independently animating, it'll notify the time manager here.
            IFC_RETURN(core->GetTimeManager()->RemoveTimeline(this));

            // Request a tick to update the now-inactive storyboard.
            // The browser host and/or frame scheduler can be NULL during shutdown.
            IXcpBrowserHost *pBH = core->GetBrowserHost();

            if (pBH != NULL)
            {
                ITickableFrameScheduler *pFrameScheduler = pBH->GetFrameScheduler();

                if (pFrameScheduler != NULL)
                {
                    IFC_RETURN(pFrameScheduler->RequestAdditionalFrame(0 /* immediate */, RequestFrameReason::StoryboardTick));
                }
            }

            TraceStopStoryboardInfo((XUINT64)static_cast<CDependencyObject*>(this));
        }
    }

    return S_OK;
}

// Moves the current time to a specified interval.  No-ops if stopped.
_Check_return_ HRESULT CStoryboard::SeekPrivate(_In_ CTimeSpan *pSeekTime)
{
    HRESULT hr = SeekInternal(pSeekTime);

    AddPendingDCompSeek(pSeekTime->m_rTimeSpan);

    return hr;
}

// Moves the current time to a specified interval.  No-ops if stopped.
_Check_return_ HRESULT CStoryboard::SeekInternal(_In_ CTimeSpan *pSeekTime)
{
    auto core = GetContext();
    CTimeManager* pTimeManager = core->GetTimeManager();
    IFCPTR_RETURN(pTimeManager);

    IFCPTR_RETURN(pSeekTime);

    if (!m_fIsStopped)
    {
        // When we seek to a given time, we update our offset from the parent on the next tick
        // to exactly the time seeked.
        // Note that if we have a nonzero BeginTime, this will seek off the logical zero
        // time of the animation:
        //    e.g. if BeginTime=2sec, and you seek by 1sec,
        //    you will be 1sec before the active period and not 1sec into it.
        m_rPendingSeekTime = pSeekTime->m_rTimeSpan;
        m_fIsSeeking = TRUE;

        if (GetTimingParent() == NULL)
        {
            IFC_RETURN(pTimeManager->AddTimeline(this));
        }
        // If the Storyboard is already running and has independent animations, notify the time manager
        // of the timing change.  This will generate a frame for the render thread to pick up the adjusted storyboard.
        else if (HasIndependentAnimation())
        {
            pTimeManager->NotifyIndependentAnimationChange();
        }

        // Request a tick to update the storyboard.
        // The browser host and/or frame scheduler can be NULL during shutdown.
        IXcpBrowserHost *pBH = core->GetBrowserHost();
        if (pBH != NULL)
        {
            ITickableFrameScheduler *pFrameScheduler = pBH->GetFrameScheduler();

            if (pFrameScheduler != NULL)
            {
                IFC_RETURN(pFrameScheduler->RequestAdditionalFrame(0 /* immediate */, RequestFrameReason::StoryboardTick));
            }
        }
    }

    return S_OK;
}

// Moves the current time to a specified interval and forces an immediate update.
_Check_return_ HRESULT CStoryboard::SeekAlignedToLastTick(_In_ CTimeSpan *pSeekTime)
{
    // First, Seek normally. Auto-completion is handled later in this method, if necessary.
    IFC_RETURN(SeekInternal(pSeekTime));

    // In addition to requesting a pending seek for later, also do a synchronous DComp animation seek now. This is necessary
    // because we might read back from the DComp animation. Since SeekAlignedToLastTick is synchronous, the animated value
    // should update immediately, which means we need to seek the DComp animation so that we'll read back the new value. If
    // we waited until the tick, GetValue on the animation target could return stale values. Note that if there is no DComp
    // animation, then we'll apply the value based on the Xaml animation, so it's safe to no-op.
    //
    // The synchronous seek cannot replace the pending seek because the DComp animation instance can still be released if the
    // Xaml timeline is marked dirty for some other reason. When we re-create the DComp animation, we'll need to reapply the
    // seek, so we need to both do a DComp seek now and do one on the next tick.
    SeekDCompAnimationInstances(pSeekTime->m_rTimeSpan);
    AddPendingDCompSeek(pSeekTime->m_rTimeSpan);
    auto core = GetContext();

    // SeekAlignedToLastTick synchronously applies the animation values, so we'll need do readbacks
    // from DComp if those animations exist. Since this applies outside the tick, we'll snap the
    // animation read back time manually. It will be unsnapped the next time we commit the DComp device.
    //
    // The render target is not guaranteed to exist. During shutdown we'll clear out the time manager, which clears out animation
    // targets, which can snap VSM animations to the end, which can get to here. No-op in those cases. Note that VSM uses SkipToFill,
    // which explicitly uses SeekAlignedToLastTick and not the regular Seek.

    // Now, update the timing tree at the new position.
    {
        // The browser host and/or frame scheduler can be nullptr during shutdown.
        IXcpBrowserHost *pBH = core->GetBrowserHost();
        if (pBH != nullptr)
        {
            IFrameScheduler *pScheduler = pBH->GetFrameScheduler();
            if (pScheduler != nullptr)
            {
                ComputeStateParams parentParams;
                parentParams.hasTime = true;
                parentParams.time = m_lastParentTime;
                parentParams.pFrameSchedulerNoRef = pScheduler;
                parentParams.isReversed = false;    // default since this is a top-level storyboard
                parentParams.isPaused = m_fIsPaused;

                // Synchronously re-compute the values for the seeked time, but
                // do not move the clock forward.
                IFC_RETURN(ComputeState(parentParams, nullptr));
            }
        }
    }

    // Queue up another Seek. Our first seek was resolved by the synchronous ComputeState call,
    // which computed a time delta that gave a local time of pSeekTime. However, since that
    // synchronous call wasn't called with the real global time, but instead with m_lastParentTime,
    // there's no guarantee that the time delta we calculated is actually correct with respect to
    // the real global clock. By queuing up another seek, we update the time delta again on the
    // next UI thread frame using the real global clock.
    //
    // e.g. The last time that this storyboard ticked was at global time 8, so m_lastParentTime = 8.
    // We then went idle for about 92 frames, and the global time is now 100. Somebody calls
    // SeekATLT(0), and we call ComputeState with the global time = m_lastParentTime = 8. That means
    // we set the time delta to (seek time - global time) = 0 - 8 = -8, and calculate the local time
    // to be (global time + time delta) = 8 - 8 = 0, as required. However, on the next UI thread frame,
    // this storyboard ticks with the real global time of 101, and it calculates the local time to be
    // (global + delta) = 101 - 8 = 93. This is way past the end of the storyboard, so we go inactive.
    // Instead we should be ticking in the beginning of the storyboard. The fix is to queue another
    // seek, so that the next UI thread tick will update the time delta to be (seek - global) =
    // (0 - 101) = -101, and the local time will be (global + delta) = 101 - 101 = 0. The next tick at
    // global time 102 will have local tie (global + delta) = 102 - 101 = 1, and this storyboard can
    // tick forward normally.
    IFC_RETURN(SeekInternal(pSeekTime));

    return S_OK;
}

// Synchronously moves the current time to the fill portion of the storyboard.
// No-ops for Storyboard's with no finite expiration time, or Storyboards that are not
// running.
_Check_return_ HRESULT CStoryboard::SkipToFill()
{
    if (!m_fIsStopped)
    {
        IFC_RETURN(CompleteInternal(
            FALSE /*stopInfiniteTimelines*/,
            TRUE /*isSynchronous*/
            ));
    }

    return S_OK;
}

// Seeks to the end if our duration is finite. Stops the storyboard if it's infinite.
_Check_return_ HRESULT CStoryboard::Complete()
{
    RRETURN(CompleteInternal(
        TRUE /*stopInfiniteTimelines*/,
        FALSE /*isSynchronous*/
        ));
}

// Seeks to the end if our duration is finite. This can happen synchronously, or on the
// next tick. Infinite storyboards are either stopped or ignored.
_Check_return_ HRESULT CStoryboard::CompleteInternal(
    bool stopInfiniteTimelines,
    bool isSynchronous
    )
{

    // Assume the storyboard is finite - it we discover otherwise we'll handle that at the end.
    bool isFinite = true;

    DurationType durationType = DurationType::Automatic;
    XFLOAT duration = 0.0f;
    GetNaturalDuration(&durationType, &duration);

    if (durationType == DurationType::TimeSpan)
    {
        const XDOUBLE beginTime = m_pBeginTime ? m_pBeginTime->m_rTimeSpan : 0.0f;
        COptionalDouble expirationTime;

        // Calculate the true end time of this storyboard and seek to it, taking into account
        // the effect of all timeline properties on our natural duration.
        ComputeExpirationTime(
            beginTime,
            durationType,
            duration,
            m_rSpeedRatio,
            &expirationTime
            );

        if (expirationTime.m_fHasValue)
        {
            // This storyboard has a finite expiration time, allowing us to seek to its end time.
            // There is floating point error in the time calculation, however, so seeking exactly to the
            // end time can result in some Active frames. Seeking to any positive value past the expiration
            // time will work, so to avoid this we seek to positive infinity instead.
            CREATEPARAMETERS cp(GetContext());
            xref_ptr<CTimeSpan> pTimeSpan;
            IFC_RETURN(CTimeSpan::Create(reinterpret_cast<CDependencyObject **>(pTimeSpan.ReleaseAndGetAddressOf()), &cp));

            // NOTE: We use XFLOAT_MAX for 'unset' values in Storyboard calculations.
            //           XFLOAT_INF is _not_ treated the same way - it is a valid number for calculating time.
            pTimeSpan->m_rTimeSpan = static_cast<XDOUBLE>(XFLOAT_INF);

            if (isSynchronous)
            {
                IFC_RETURN(SeekAlignedToLastTick(pTimeSpan));
            }
            else
            {
                IFC_RETURN(SeekInternal(pTimeSpan));
            }
        }
        else
        {
            // This storyboard has a finite natural duration, but an infinite expiration time (e.g. RepeatBehavior=DirectUI::DurationType::Forever).
            isFinite = FALSE;
        }
    }
    else
    {
        // This storyboard has an infinite natural duration (e.g. Duration=DirectUI::DurationType::Forever).
        isFinite = FALSE;
    }

    // If requested, StopPrivate() infinite storyboards. Otherwise, the call will no-op if infinite.
    if (!isFinite && stopInfiniteTimelines)
    {
        IFC_RETURN(StopPrivate());
    }

    return S_OK;
}

// Moves the current time to a specified interval on the next tick.
// This wrapper is called by the public pInvoke to do argument checking, etc.
_Check_return_ HRESULT CStoryboard::Seek(
    _In_ const CValue& seekTime
    )
{
    CTimeline* pParent = NULL;
    CTimeline* pRoot = NULL;
    CTimeManager* pTimeManager = NULL;
    auto core = GetContext();
    CREATEPARAMETERS cp(core, seekTime);

    // Make sure only root storyboards get called on this
    pTimeManager = core->GetTimeManager();
    IFCPTR_RETURN(pTimeManager);

    pParent = GetTimingParent();
    pRoot = pTimeManager->GetRootTimeline();

    // if Seek is called on a non-timeMgr rooted SB, then it must be unparented
    if ((pParent != pRoot) && (pParent != NULL))
    {
        if (CTimeManager::ShouldFailFast())
        {
            IFCFAILFAST(static_cast<HRESULT>(E_NER_INVALID_OPERATION));
        }
        else
        {
            IFC_RETURN(SetAndOriginateError(E_NER_INVALID_OPERATION, RuntimeError, AG_E_RUNTIME_SB_MUST_BE_ROOT));
        }
    }

    xref_ptr<CTimeSpan> pTimeSpan;
    IFC_RETURN(CTimeSpan::Create(reinterpret_cast<CDependencyObject**>(pTimeSpan.ReleaseAndGetAddressOf()), &cp));

    IFC_RETURN(SeekPrivate(pTimeSpan));

    //  If anything affected custom properties, we would have queued some
    // asynchronous value changes for them.  Run them now, before returning,
    // so our managed caller can see the results.
    IFC_RETURN(core->FlushDeferredAnimationOperationQueue());

    return S_OK;
}

// Moves the current time to a specified interval and forces an immediate update.
// This wrapper is called by the public pInvoke to do argument checking, etc.
_Check_return_ HRESULT
CStoryboard::SeekAlignedToLastTickPublic(
    _In_ const CValue& seekTime
    )
{
    CTimeline* pParent = NULL;
    CTimeline* pRoot = NULL;
    CTimeManager* pTimeManager = NULL;
    auto core = GetContext();
    CREATEPARAMETERS cp(core, seekTime);

    // Make sure only root storyboards get called on this
    pTimeManager = core->GetTimeManager();
    IFCPTR_RETURN(pTimeManager);

    pParent = GetTimingParent();
    pRoot = pTimeManager->GetRootTimeline();

    // if SATLT is called on a non-timeMgr rooted SB, then it must be unparented
    if ((pParent != pRoot) && (pParent != NULL))
    {
        if (CTimeManager::ShouldFailFast())
        {
            IFCFAILFAST(static_cast<HRESULT>(E_NER_INVALID_OPERATION));
        }
        else
        {
            IFC_RETURN(SetAndOriginateError(E_NER_INVALID_OPERATION, RuntimeError, AG_E_RUNTIME_SB_MUST_BE_ROOT));
        }
    }

    xref_ptr<CTimeSpan> pTimeSpan;
    IFC_RETURN(CTimeSpan::Create(reinterpret_cast<CDependencyObject**>(pTimeSpan.ReleaseAndGetAddressOf()), &cp));

    IFC_RETURN(SeekAlignedToLastTick(pTimeSpan));

    // If anything affected custom properties, we would have queued some
    // asynchronous value changes for them.  Run them now, before returning,
    // so our managed caller can see the results.
    IFC_RETURN(core->FlushDeferredAnimationOperationQueue());

    return S_OK;
}

// Synchronously moves the current time to the fill portion of the storyboard.
// No-ops for Storyboard's with no finite expiration time.
_Check_return_ HRESULT CStoryboard::SkipToFillPublic()
{
    CTimeline *pRoot = NULL;
    CTimeline *pParent = NULL;
    auto core = GetContext();

    // Make sure only root storyboards get called on this
    CTimeManager *pTimeManager = core->GetTimeManager();
    IFCPTR_RETURN(pTimeManager);
    pParent = GetTimingParent();
    pRoot = pTimeManager->GetRootTimeline();

    // if SkipToFill is called on a non-timeMgr rooted SB, then it must be unparented
    if ((pParent != pRoot) && (pParent != NULL))
    {
        if (CTimeManager::ShouldFailFast())
        {
            IFCFAILFAST(static_cast<HRESULT>(E_NER_INVALID_OPERATION));
        }
        else
        {
            IFC_RETURN(SetAndOriginateError(E_NER_INVALID_OPERATION, RuntimeError, AG_E_RUNTIME_SB_MUST_BE_ROOT));
        }
    }

    IFC_RETURN(SkipToFill());

    // If anything affected custom properties, we would have queued some
    // asynchronous value changes for them.  Run them now, before returning,
    // so our managed caller can see the results.
    IFC_RETURN(core->FlushDeferredAnimationOperationQueue() );

    return S_OK;
}

// Static wrapper to Pause the Storyboard
_Check_return_ HRESULT CStoryboard::Pause()
{
    CTimeline* pParent = NULL;
    CTimeline* pRoot = NULL;
    CTimeManager* pTimeManager = NULL;
    auto core = GetContext();

    // Make sure only root storyboards get called on this
    pTimeManager = core->GetTimeManager();
    IFCPTR_RETURN(pTimeManager);
    pParent = GetTimingParent();
    pRoot = pTimeManager->GetRootTimeline();

    // if Pause is called on a non-timeMgr rooted SB, then it must be unparented
    if ((pParent != pRoot) && (pParent != NULL))
    {
        if (CTimeManager::ShouldFailFast())
        {
            IFCFAILFAST(static_cast<HRESULT>(E_NER_INVALID_OPERATION));
        }
        else
        {
            IFC_RETURN(SetAndOriginateError(E_NER_INVALID_OPERATION, RuntimeError, AG_E_RUNTIME_SB_MUST_BE_ROOT));
        }
    }

    IFC_RETURN(PausePrivate());

    //  If anything affected custom properties, we would have queued some
    // asynchronous value changes for them.  Run them now, before returning,
    // so our managed caller can see the results.
    IFC_RETURN(core->FlushDeferredAnimationOperationQueue());

    return S_OK;
}

// Wrapper to Resume the Storyboard
_Check_return_ HRESULT CStoryboard::Resume()
{
    CTimeline* pParent = NULL;
    CTimeline* pRoot = NULL;
    CTimeManager* pTimeManager = NULL;
    auto core = GetContext();

    // Make sure only root storyboards get called on this
    pTimeManager = core->GetTimeManager();
    IFCPTR_RETURN(pTimeManager);
    pParent = GetTimingParent();
    pRoot = pTimeManager->GetRootTimeline();

    // if Resume is called on a non-timeMgr rooted SB, then it must be unparented
    if ((pParent != pRoot) && (pParent != NULL))
    {
        if (CTimeManager::ShouldFailFast())
        {
            IFCFAILFAST(static_cast<HRESULT>(E_NER_INVALID_OPERATION));
        }
        else
        {
            IFC_RETURN(SetAndOriginateError(E_NER_INVALID_OPERATION, RuntimeError, AG_E_RUNTIME_SB_MUST_BE_ROOT));
        }
    }

    IFC_RETURN(ResumePrivate());

    //  If anything affected custom properties, we would have queued some
    // asynchronous value changes for them.  Run them now, before returning,
    // so our managed caller can see the results.
    IFC_RETURN(core->FlushDeferredAnimationOperationQueue());

    return S_OK;
}

// Wrapper to Stop the Storyboard
_Check_return_ HRESULT CStoryboard::Stop()
{
    CTimeline* pParent = NULL;
    CTimeline* pRoot = NULL;
    CTimeManager* pTimeManager = NULL;
    auto core = GetContext();

    // Make sure only root storyboards get called on this
    pTimeManager = core->GetTimeManager();
    IFCPTR_RETURN(pTimeManager);
    pParent = GetTimingParent();
    pRoot = pTimeManager->GetRootTimeline();

    // if Stop is called on a non-timeMgr rooted SB, then it must be unparented
    if ((pParent != pRoot) && (pParent != NULL))
    {
        if (CTimeManager::ShouldFailFast())
        {
            IFCFAILFAST(static_cast<HRESULT>(E_NER_INVALID_OPERATION));
        }
        else
        {
            IFC_RETURN(SetAndOriginateError(E_NER_INVALID_OPERATION, RuntimeError, AG_E_RUNTIME_SB_MUST_BE_ROOT));
        }
    }

    IFC_RETURN(StopPrivate());

    // If there are pending DComp pauses or seeks, clear them out so we won't apply them when creating animation instances.
    ClearPendingDCompAnimationOperationsRecursive();

    //  If anything affected custom properties, we would have queued some
    // asynchronous value changes for them.  Run them now, before returning,
    // so our managed caller can see the results.
    IFC_RETURN(core->FlushDeferredAnimationOperationQueue());

    return S_OK;
}

// Wrapper to Begin the Storyboard
_Check_return_ HRESULT CStoryboard::Begin()
{
    CTimeManager* pTimeManager = NULL;
    auto core = GetContext();

    pTimeManager = core->GetTimeManager();
    IFCPTR_RETURN(pTimeManager);

    // Make sure only root storyboards get called on this
    // if Begin is called on a non-timeMgr rooted SB, then it must be unparented
    if (!IsTopLevelStoryboard() && GetTimingParent() != NULL)
    {
        if (CTimeManager::ShouldFailFast())
        {
            IFCFAILFAST(static_cast<HRESULT>(E_NER_INVALID_OPERATION));
        }
        else
        {
            IFC_RETURN(SetAndOriginateError(E_NER_INVALID_OPERATION, RuntimeError, AG_E_RUNTIME_SB_MUST_BE_ROOT));
        }
    }

    // At this point we can only begin top-level storyboards - let the Begin
    // call know that.
    IFC_RETURN(BeginPrivate(TRUE /* fIsTopLevel */));

    // If there are pending DComp pauses or seeks, clear them out so we won't apply them when creating animation instances.
    ClearPendingDCompAnimationOperationsRecursive();

    // If anything affected custom properties, we would have queued some
    // asynchronous value changes for them.  Run them now, before returning,
    // so our managed caller can see the results.
    IFC_RETURN(core->FlushDeferredAnimationOperationQueue());

    return S_OK;
}

// Returns true if this Storyboard is the root of its timing tree.
// As implemented, this means it gets its time directly from the time manager.
bool CStoryboard::IsTopLevelStoryboard()
{
    return IsTopLevelTimeline();
}

// Checks if animation tracking is enabled and allowed for this storyboard.
bool CStoryboard::IsAnimationTrackingAllowed()
{
    if (!GetContext()->IsAnimationTrackingEnabled())
    {
        return false;
    }

    return true;
}

void CStoryboard::AnimationTrackingBeginScenario(
    _In_opt_ CTransition* pTransition,
    _In_opt_ CDependencyObject* pTargetObject
    )
{
    // If animation tracking is not enabled or not allowed, bail.
    if (!IsAnimationTrackingAllowed())
    {
        return;
    }

    // If this storyboard does not look to be interesting for animation tracking
    // purposes (e.g. due to delayed start etc.), bail.
    // Transitions are always of interest, regardless of any staggering delays.
    if (!pTransition && !IsInterestingForAnimationTracking())
    {
        return;
    }

    // Calculate the expected duration.
    DurationType durationType = DirectUI::DurationType::TimeSpan;
    XFLOAT rDurationValue = 0;
    IGNOREHR(GetDuration(&durationType, &rDurationValue));

    // Not interested in instant storyboards.
    if ((durationType != DirectUI::DurationType::TimeSpan) || (rDurationValue <= 0))
    {
        return;
    }

    // If we have a significant begin delay, e.g. due staggering, do not begin a
    // scenario but reference an existing one. Otherwise account for it in the
    // duration.
    if (m_pBeginTime && m_pBeginTime->m_rTimeSpan >= 0)
    {
        if ((m_pBeginTime->m_rTimeSpan * 1000) > c_AnimationTrackingMaxBeginTimeInMs)
        {
            AnimationStartTracking();
            return;
        }
        rDurationValue += static_cast<XFLOAT>(m_pBeginTime->m_rTimeSpan);
    }

    // Not interested in storyboards that run for too long
    if (rDurationValue >= c_AnimationTrackingMaxDurationInS)
    {
        return;
    }

    // Now we want to determine an appropriate name and details for this storyboard.
    // Interesting things we want to capture include transition, dynamic timelines,
    // visual state names.
    CDependencyObject* pDOTarget = pTargetObject;
    CDependencyObject* pDynamicTimeline = nullptr;
    AnimationTrackingCollectInfoNoRef(&pDOTarget, &pDynamicTimeline);

    // Look for the first named UI element parent of the target.
    CUIElement* pUITarget = pDOTarget->GetNamedSelfOrParent<CUIElement>();

    // If we did not find a suitable target, this is not an interesting storyboard.
    if (!pUITarget)
    {
        return;
    }

    // Build the name.
    xstring_ptr name;
    xstring_ptr details;
    XUINT16 scenarioPriority = c_AnimationTrackingDefaultPriority;

    // If tracing/telemetry is not enabled, avoid the cost of building a description.
    // Specifying the default priority will also be sufficient.
    if (EventEnabledPerfTrackDetectionInfo())
    {
        name = GetNameForTracking(pTransition, pUITarget, pDynamicTimeline, &scenarioPriority);
        details = GetTargetPathForTracking(pUITarget, true /* startFromParentOfTarget */);

        // We can go through here multiple times (once from layout transitions, and again from starting the storyboard).
        // Cache the name the first time. We'll clear it when the storyboard stops.
        if (m_storyboardTelemetryName.IsNull())
        {
            m_storyboardTelemetryName = name;
        }
    }
    else
    {
        IGNOREHR(xstring_ptr::CloneBuffer(L"XamlAnim", &name));
        IGNOREHR(xstring_ptr::CloneBuffer(L"", &details));
    }

    // Start the animation scenario.
    AnimationTrackingScenarioInfo info = {0};
    info.scenarioName = name.GetBuffer();
    info.scenarioDetails = details.GetBuffer();
    info.msIntendedDuration = static_cast<XUINT32>(rDurationValue * 1000);
    info.priority = scenarioPriority;
    GetContext()->AnimationTrackingScenarioBegin(&info);

    // Also start tracking this storyboard as a sub-animation.
    AnimationStartTracking();
}

void CStoryboard::EnsureTelemetryName()
{
    if (m_storyboardTelemetryName.IsNull())
    {
        m_storyboardTelemetryName = GetNameForTracking(nullptr, nullptr, nullptr, nullptr);
    }
}

// Start tracking this storyboard for animation tracking as a sub animation.
void CStoryboard::AnimationStartTracking()
{
    if (!IsAnimationTrackingAllowed())
    {
        return;
    }

    if (!m_fIsTrackedForAnimationTracking)
    {
        XUINT64 key = reinterpret_cast<XUINT64>(static_cast<CDependencyObject*>(this));
        GetContext()->AnimationTrackingScenarioReference(key);
        m_fIsTrackedForAnimationTracking = TRUE;
    }
}

// Stop tracking this storyboard for animation tracking as a sub animation.
void CStoryboard::AnimationStopTracking()
{
    if (m_fIsTrackedForAnimationTracking)
    {
        XUINT64 key = reinterpret_cast<XUINT64>(static_cast<CDependencyObject*>(this));
        GetContext()->AnimationTrackingScenarioUnreference(key);
        m_fIsTrackedForAnimationTracking = FALSE;
    }
}

// Returns the duration of the storyboard in seconds, and the duration type
_Check_return_ HRESULT CStoryboard::GetDuration(
    _Out_ DurationType* pDurationType,
    _Out_ XFLOAT* prDuration
    )
{
    DurationType durationType = DirectUI::DurationType::Automatic;
    XFLOAT rDuration = 0.0f;

    if (m_duration == nullptr ||
        m_duration->Value().GetDurationType() == DirectUI::DurationType::Automatic)
    {
        GetNaturalDuration(&durationType, &rDuration);
    }
    else
    {
        durationType = m_duration->Value().GetDurationType();
        rDuration = static_cast<XFLOAT>(m_duration->Value().GetTimeSpanInSec());
    }

    switch (durationType)
    {
        case DirectUI::DurationType::TimeSpan:
            if (m_fAutoReverse)
            {
                rDuration *= 2.0;
            }

            // Account for RepeatBehavior
            if (m_repeatBehavior)
            {
                switch (m_repeatBehavior->Value().GetRepeatBehaviorType())
                {
                    case DirectUI::RepeatBehaviorType::Forever:
                        // storyboard repeats forever (override previous value)
                        rDuration = XFLOAT_MAX;
                        durationType = DirectUI::DurationType::Forever;
                        break;

                    case DirectUI::RepeatBehaviorType::Duration:
                        // We are giving a specific repeat behavior
                        //   which either clips our duration or extends our repeat time.
                        rDuration = static_cast<XFLOAT>(m_repeatBehavior->Value().GetDurationInSec());
                        break;

                    case DirectUI::RepeatBehaviorType::Count:
                    default:
                        // If we have a count, use it - this defaults to 1.0
                        rDuration *= m_repeatBehavior->Value().GetCount();
                        break;
                }
            }

            *prDuration = rDuration;
            break;

        case DirectUI::DurationType::Forever:
            *prDuration = XFLOAT_MAX;
            break;

        case DirectUI::DurationType::Automatic:
        default:
            // Automatic means that the subtree could not be resolved ...
            // At the storyboard level, we should always be able to compute the duration.
            IFC_RETURN(E_UNEXPECTED);
            break;
    }

    *pDurationType = durationType;

    return S_OK;
}

namespace CoreImports
{
    _Check_return_ HRESULT Storyboard_SetTarget(
        _In_ CTimeline *pTimeline,
        _In_ CDependencyObject *pDO )
    {
        IFCPTR_RETURN(pTimeline);
        IFCPTR_RETURN(pDO);

        IFC_RETURN( pTimeline->SetTargetObject(pDO) );

        return S_OK;
    }
}

CDependencyObject* CStoryboard::GetStandardNameScopeParent()
{
    auto parent = CParallelTimeline::GetStandardNameScopeParent();

    // If we're not in the visual tree, see if we have a mentor instead.
    if (parent == nullptr)
    {
        parent = GetMentor();
    }

    return parent;
}

namespace StoryboardHelpers
{
    HRESULT ZeroSeekAlignedToLastTick(_In_ CStoryboard* pStoryboard)
    {
        xref_ptr<CTimeSpan> zeroTime;
        CREATEPARAMETERS cp(pStoryboard->GetContext());
        IFC_RETURN(CTimeSpan::Create(reinterpret_cast<CDependencyObject**>(zeroTime.ReleaseAndGetAddressOf()), &cp));
        IFC_RETURN(pStoryboard->SeekAlignedToLastTick(zeroTime.get()));
        return S_OK;
    }

    HRESULT EndSeekAlignedToLastTick(_In_ CStoryboard* pStoryboard)
    {
        xref_ptr<CTimeSpan> infTime;
        CREATEPARAMETERS cp(pStoryboard->GetContext());
        IFC_RETURN(CTimeSpan::Create(reinterpret_cast<CDependencyObject**>(infTime.ReleaseAndGetAddressOf()), &cp));
        infTime->m_rTimeSpan = static_cast<XDOUBLE>(XFLOAT_INF);
        IFC_RETURN(pStoryboard->SeekAlignedToLastTick(infTime.get()));
        return S_OK;
    }

    HRESULT
    ModifyStoryboard(_In_opt_ CStoryboard* storyboard, bool forceStart, _In_ const std::function<HRESULT(void)>& modifyFunc)
    {
        if (!storyboard)
        {
            // If no storyboard, just run the modification function
            IFC_RETURN(modifyFunc());
            return S_OK;
        }

        const bool wasStopped = storyboard->IsInStoppedState();
        const bool wasPaused = storyboard->IsPaused();

        // Grab the current time before we stop the storyboard. We must stop it in order
        // to modify it.
        double currentTime = storyboard->m_rCurrentTime;
        if (!wasStopped)
        {
            MICROSOFT_TELEMETRY_ASSERT_HR(storyboard->Stop());
        }

        IFC_RETURN(modifyFunc());

        // if the storyboard wasn't stopped, start it and seek it to the time it was
        // so that the modifications apply
        if (!wasStopped || forceStart)
        {
            HRESULT beginHr = storyboard->Begin();
            // Begin can fail if an animation or setter has an invalid target set. This is expected
            // and an error will be originated that VisualStudio can catch, so we don't need to return an HRESULT
            // and we should only assert an HR that is unexpected.
            if (FAILED(beginHr) && beginHr != E_NER_INVALID_OPERATION)
            {
                MICROSOFT_TELEMETRY_ASSERT_HR(beginHr);
            }
        }
        xref_ptr<CTimeSpan> time;
        CREATEPARAMETERS cp(storyboard->GetContext());
        MICROSOFT_TELEMETRY_ASSERT_HR(CTimeSpan::Create(reinterpret_cast<CDependencyObject**>(time.ReleaseAndGetAddressOf()), &cp));
        time->m_rTimeSpan = currentTime;
        MICROSOFT_TELEMETRY_ASSERT_HR(storyboard->SeekAlignedToLastTick(time.get()));

        // If we were paused, then pause the storyboard again. It was unpaused when we called Begin on it.
        if (wasPaused)
        {
            MICROSOFT_TELEMETRY_ASSERT_HR(storyboard->Pause());
        }
        return S_OK;
    }
}
