// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "UIThreadScheduler.h"
#include <PALTypes.h>
#include <CompositorScheduler.h>
#include "GraphicsTelemetry.h"

//------------------------------------------------------------------------------
//
//  Synopsis:
//      UIThreadScheduler ctor
//
//------------------------------------------------------------------------------
UIThreadScheduler::UIThreadScheduler(
    _In_ IXcpDispatcher *pDispatcher,
    _In_ CompositorScheduler *pCompositorScheduler
    )
    : m_pDispatcher(pDispatcher)
    , m_pIClock(NULL)
    , m_lastTickTimeInSeconds(0.0)
    , m_pCompositorScheduler(pCompositorScheduler)
    , m_currentState(UITSS_Waiting)
    , m_nextTickIntervalInMilliseconds(XINFINITE)
    , m_isHighPriority(FALSE)
    , m_requestFrameReasons(static_cast<RequestFrameReason>(0))
{
    XCP_STRONG(&m_pDispatcher);
    m_pDispatcher->AddRef();
    m_pCompositorScheduler->AddRef();
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      UIThreadScheduler ctor
//
//------------------------------------------------------------------------------
UIThreadScheduler::~UIThreadScheduler()
{
    // Unregister first, so the render thread doesn't call into this object while it's being cleaned up.
    m_pCompositorScheduler->UnregisterUIThreadScheduler(this);
    ReleaseInterface(m_pCompositorScheduler);

    ReleaseInterface(m_pDispatcher);
    ReleaseInterface(m_pIClock);
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      UIThreadScheduler static factory method
//
//------------------------------------------------------------------------------
/* static */ void UIThreadScheduler::Create(
    _In_ IXcpDispatcher *pDispatcher,
    _In_ CompositorScheduler* compositorScheduler,
    _Outptr_ UIThreadScheduler **ppUIThreadScheduler)
{
    UIThreadScheduler *pScheduler = new UIThreadScheduler(pDispatcher, compositorScheduler);
    ASSERT(pScheduler != nullptr);

    compositorScheduler->RegisterUIThreadScheduler(pScheduler, &(pScheduler->m_pIClock));

    *ppUIThreadScheduler = pScheduler;  // pass ownership of the object to the caller
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Schedules another UI thread frame to be ticked at the requested interval since the previous
//      tick.  nextTickInterval is in milliseconds.  Requesting a frame with 0 interval means the UI
//      thread should be ticked again whenever the render thread next ticks.
//      Called on UI thread or other media scheduling threads.
//
//      NOTE: Requesting ticks at a future point is NOT stateful.  When processing a tick, all future
//                requests should be resubmitted if still required (minus elapsed time, of course).
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
UIThreadScheduler::RequestAdditionalFrame(
    XUINT32 nextTickIntervalInMilliseconds,
    RequestFrameReason reason)
{
    {
        // TODO: TICK: Could probably use interlocked exchange for all m_currentState/m_nextTickIntervalInMilliseconds usage if necessary
        auto guard = m_NextTickLock.lock();

        //
        // Within a frame, all requests for additional frames are relative to the same time, so that multiple requests
        // for the same interval all target the same time. We use the m_lastTickTimeInSeconds, snapped at the beginning
        // of the frame, as the point of reference. The assumption is that a frame (at least in steady-state) is quick
        // enough that the passage of time during the tick can be ignored.
        //
        // But m_lastTickTimeInSeconds is meaningless outside a frame. We may have been idle for a long time, so the
        // actual time has progressed significantly since the frame started. If we just use m_lastTickTimeInSeconds as
        // the point of reference, we'll have a target time that's much sooner than intended. For example, if it's been
        // 500ms since m_lastTickTimeInSeconds, and a request comes in for 2000ms from now, then that's actually 2500ms
        // since m_lastTickTimeInSeconds. We need to account for the difference.
        //
        if (nextTickIntervalInMilliseconds > 0 && m_currentState != UITSS_Ticking)
        {
            uint64_t currentTimeInMilliseconds = static_cast<uint64_t>(m_pIClock->GetNextTickTimeInSeconds() * 1000);
            uint64_t lastTickTimeInMilliseconds = static_cast<uint64_t>(m_lastTickTimeInSeconds * 1000);
            uint32_t originalIntervalInMilliseconds = nextTickIntervalInMilliseconds;
            nextTickIntervalInMilliseconds = originalIntervalInMilliseconds + static_cast<uint32_t>(currentTimeInMilliseconds - lastTickTimeInMilliseconds);

            TraceLoggingProviderWrite(
                GraphicsTelemetry, "Scheduling_RequestAdditionalFrameOutsideTick",
                TraceLoggingUInt32(originalIntervalInMilliseconds, "OriginalIntervalInMilliseconds"),
                TraceLoggingUInt64(currentTimeInMilliseconds, "CurrentTimeInMilliseconds"),
                TraceLoggingUInt64(lastTickTimeInMilliseconds, "LastTickTimeInMilliseconds"),
                TraceLoggingUInt32(nextTickIntervalInMilliseconds, "NextTickIntervalInMilliseconds"),
                TraceLoggingUInt32(static_cast<uint32_t>(reason), "RequestFrameReason"),
                TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

            // Tick requests OUTSIDE of tick processing will be treated as high-priority.
            // This includes changing layout, rendering, or animation state directly from input events or
            // from any async event handler. A high priority tick, once posted, will be processed before any
            // additional input messages.
            //
            // Tick requests coming from within the tick itself are treated as low-priority.
            // This includes subsequent animation ticks, dependent animation updates, and
            // and CompositionTarget.Rendering callbacks. A low priority tick will be deferred until all input
            // is processed.
            m_isHighPriority = TRUE;
        }

        bool wasNextTickIntervalInMillisecondsUpdated = false;

        // If there's already a tick queued, this schedule request can be otherwise ignored.
        // The work is going to be picked up or scheduled again anyway when the tick is processed.
        if (m_currentState != UITSS_TickQueued)
        {
            if (nextTickIntervalInMilliseconds < m_nextTickIntervalInMilliseconds)
            {
                m_nextTickIntervalInMilliseconds = nextTickIntervalInMilliseconds;
                wasNextTickIntervalInMillisecondsUpdated = true;

                // If the request came when no other work was scheduled, ensure the render thread
                // is awake to process it and queue another tick.
                // All requests dispatched while ticking are delayed until the transition to the waiting state.
                // This prevents waking up the render thread until all requests from the current frame are
                // combined, and the UI thread is ready to have a tick queued again.
                if (m_currentState == UITSS_Waiting)
                {
                    IFC_RETURN(m_pCompositorScheduler->WakeCompositionThread());
                }
            }
        }

        // Log the reason if it's new or if we asked for a sooner tick.
        if (wasNextTickIntervalInMillisecondsUpdated || ((m_requestFrameReasons & reason) == 0))
        {
            m_requestFrameReasons = static_cast<RequestFrameReason>(m_requestFrameReasons | reason);

            TraceRequestFrameReasonInfo(
                nextTickIntervalInMilliseconds,
                m_nextTickIntervalInMilliseconds,
                reason,
                m_requestFrameReasons);
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Begin tick processing on the UI thread.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
UIThreadScheduler::BeginTick()
{
    // TODO: TICK: Do we really want the UI thread to tick to whenever the last compositor tick was?
    // TODO: TICK: It might be better to move non-animated values forward by a number of frames in anticipation of when they'll hit the screen.
    m_lastTickTimeInSeconds = m_pIClock->GetLastTickTimeInSeconds();

    {
        auto guard = m_NextTickLock.lock();

        // We should only tick if one was queued.
        ASSERT(m_currentState == UITSS_TickQueued);
        m_currentState = UITSS_Ticking;

        // Reset the tick interval.  From this point forward, calls to RequestAdditionalFrame
        // will be treated as requests for a tick beyond the one currently being processed.
        m_nextTickIntervalInMilliseconds = XINFINITE;
        m_requestFrameReasons = static_cast<RequestFrameReason>(0);
    }

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      End tick processing on the UI thread.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
UIThreadScheduler::EndTick()
{
    {
        auto guard = m_NextTickLock.lock();

        // We can only stop ticking if we were in the process of ticking.
        ASSERT(m_currentState == UITSS_Ticking);

        // Enter the state where we're waiting for the render thread to queue a tick again.
        m_currentState = UITSS_Waiting;

        // If there was a frame scheduled during ticking, ensure another UI thread frame will be scheduled.
        if (m_nextTickIntervalInMilliseconds < XINFINITE)
        {
            double lastClockTimeInSeconds = m_pIClock->GetLastTickTimeInSeconds();
            if (m_lastTickTimeInSeconds < lastClockTimeInSeconds)
            {
                TraceLoggingProviderWrite(
                    GraphicsTelemetry, "Scheduling_ClockAdvancedDuringTick",
                    TraceLoggingUInt64(reinterpret_cast<uint64_t>(m_pIClock), "ClockPointer"),
                    TraceLoggingUInt64(static_cast<uint64_t>(lastClockTimeInSeconds * 1000000), "LastClockTimeInMilliseconds"),
                    TraceLoggingUInt64(static_cast<uint64_t>(m_lastTickTimeInSeconds * 1000000), "LastTickTimeInMilliseconds"),
                    TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));
            }

            // Wake the scheduling thread and have it queue a tick. This ensures we get throttled properly to the frame
            // rate, if needed.
            IFC_RETURN(m_pCompositorScheduler->WakeCompositionThread());
        }

        // Reset the priority now that the tick is finished.
        m_isHighPriority = FALSE;
    }

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Returns TRUE if the scheduler is currently processing a tick.
//
//------------------------------------------------------------------------------
bool
UIThreadScheduler::IsInTick()
{
    auto guard = m_NextTickLock.lock();

    return m_currentState == UITSS_Ticking;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Returns TRUE if the scheduler is currently processing a tick.
//
//------------------------------------------------------------------------------
bool
UIThreadScheduler::IsHighPriority()
{
    auto guard = m_NextTickLock.lock();

    return m_isHighPriority;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Enqueue a tick to the UI thread by notifying the dispatcher.  Called on the render thread.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
UIThreadScheduler::QueueTick()
{
    bool shouldQueueTick = false;

    {
        auto guard = m_NextTickLock.lock();

        // The UI thread must be waiting for a tick to be enqueued in order to proceed.
        //
        // The render thread and UI thread might both try to queue a tick if the render thread were
        // switched out just after moving the clock forward, since that doesn't occur under this lock.
        // We prevent 2 ticks from being queued by ignoring whatever request occurs second.
        if (m_currentState == UITSS_Waiting)
        {
            m_currentState = UITSS_TickQueued;

            shouldQueueTick = TRUE;
        }
        else
        {
            ASSERT(m_currentState == UITSS_TickQueued);
        }
    }

    TraceLoggingProviderWrite(
        GraphicsTelemetry, "Scheduling_QueueTick",
        TraceLoggingBoolean(shouldQueueTick, "WasTickQueued"),
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    if (shouldQueueTick)
    {
        IFC_RETURN(m_pDispatcher->QueueTick());
    }

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the interval from the current tick time that the UI thread wanted a tick.
//
//------------------------------------------------------------------------------
XUINT32
UIThreadScheduler::GetScheduledIntervalInMilliseconds(XDOUBLE currentTickTimeInSeconds)
{
    auto guard = m_NextTickLock.lock();

    ASSERT(currentTickTimeInSeconds >= m_lastTickTimeInSeconds);

    // If the UI thread is waiting for a tick, return whether one is scheduled and the requested interval has elapsed.
    // If the UI thread has a tick enqueued or is processing a tick, do not schedule another frame yet.
    if (m_currentState == UITSS_Waiting && m_nextTickIntervalInMilliseconds < XINFINITE)
    {
        XDOUBLE requestTimeInSeconds = m_lastTickTimeInSeconds + static_cast<XDOUBLE>(m_nextTickIntervalInMilliseconds) / 1000.0;
        if (currentTickTimeInSeconds >= requestTimeInSeconds)
        {
            return 0;
        }
        else
        {
            return static_cast<XUINT32>(1000.0 * (requestTimeInSeconds - currentTickTimeInSeconds));
        }
    }
    else
    {
        return XINFINITE;
    }
}
