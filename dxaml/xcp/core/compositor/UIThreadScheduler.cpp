// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "UIThreadScheduler.h"
#include <PALTypes.h>
#include <CompositorScheduler.h>

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
    XUINT32 nextTickInterval,
    RequestFrameReason reason)
{
    {
        // TODO: TICK: Could probably use interlocked exchange for all m_currentState/m_nextTickIntervalInMilliseconds usage if necessary
        auto guard = m_NextTickLock.lock();

        // All future tick requests OUTSIDE of tick processing will be treated as immediate
        // because they lack a frame of reference to compare against (the last tick and next tick
        // cannot be reliably determined).
        //
        // It's expected that if the requestor still needs to be scheduled for a future time, that
        // request would be made again while processing that 'immediate' tick, since as noted above
        // tick requests do not persist tick-over-tick.
        //
        // Note that, even if there's a tick queued, we need to handle this case. It's possible that this tick
        // was posted as low-priority, but some input processing that was re-prioritized is now pushing the
        // deferred tick to high priority.
        if (m_currentState != UITSS_Ticking)
        {
            nextTickInterval = 0;

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

        // If there's already a tick queued, this schedule request can be otherwise ignored.
        // The work is going to be picked up or scheduled again anyway when the tick is processed.
        if (m_currentState != UITSS_TickQueued)
        {
            if (nextTickInterval < m_nextTickIntervalInMilliseconds)
            {
                m_nextTickIntervalInMilliseconds = nextTickInterval;

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

        // Log the reason if it's new
        if ((m_requestFrameReasons & reason) == 0)
        {
            m_requestFrameReasons = static_cast<RequestFrameReason>(m_requestFrameReasons | reason);

            TraceRequestFrameReasonInfo(
                nextTickInterval,
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
            // If the render thread's clock has progressed since this tick started, the UI thread is running behind.
            // Go ahead and queue the next tick immediately to keep the UI thread pipeline as full as possible,
            // while still throttling to the render thread's execution rate (the refresh rate).
            if (m_lastTickTimeInSeconds < m_pIClock->GetLastTickTimeInSeconds())
            {
                IFC_RETURN(QueueTick());
            }
            // Otherwise, ensure the render thread is awake to queue the tick next time it runs.
            // It will have ignored the UI thread during the 'ticking' state, but will process it now that we're
            // back in the 'waiting' state.
            else
            {
                IFC_RETURN(m_pCompositorScheduler->WakeCompositionThread());
            }
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
