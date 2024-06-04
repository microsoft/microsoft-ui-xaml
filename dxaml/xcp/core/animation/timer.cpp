// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Timer.h"
#include "Timemgr.h"
#include "TimeSpan.h"
#include <UIThreadScheduler.h>
#include "XamlTelemetry.h"

_Check_return_ HRESULT CDispatcherTimer::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
)
{
    CREATEPARAMETERS cp(pCreate->m_pCore);
    xref_ptr<CDispatcherTimer> pDispatcherTimer;

    pDispatcherTimer.attach(new CDispatcherTimer(pCreate->m_pCore));

    // Initialize with a default timespan.
    xref_ptr<CTimeSpan> pTime;
    IFC_RETURN(CTimeSpan::Create((CDependencyObject**)pTime.ReleaseAndGetAddressOf(), &cp));

    pTime->m_rTimeSpan = 0.0;
    pDispatcherTimer->m_pInterval = pTime.detach();
    *ppObject = pDispatcherTimer.detach();


    return S_OK;

}

CDispatcherTimer::~CDispatcherTimer()
{
    auto core = GetContext();

    CTimeManager *pTimeManager = core->GetTimeManager();
    CEventManager *pEventManager = core->GetEventManager();

    if (m_pEventList && pEventManager)
    {
        // Add the events in...
        CXcpList<REQUEST>::XCPListNode *pTemp = m_pEventList->GetHead();
        while (pTemp)
        {
            REQUEST * pRequest = (REQUEST *)pTemp->m_pData;
            VERIFYHR(pEventManager->RemoveRequest(this, pRequest));
            pTemp = pTemp->m_pNext;
        }
        m_pEventList->Clean();
        delete m_pEventList;
        m_pEventList = NULL;
    }

    // ... and remove ourselves from the timing tree if we are not already removed.
    if (( pTimeManager != NULL ) && ( GetTimingParent() != NULL ))
    {
        VERIFYHR(pTimeManager->RemoveTimeline(this));
    }

    ReleaseInterface(m_pInterval);
}

_Check_return_ HRESULT CDispatcherTimer::ComputeStateImpl(
    _In_ const ComputeStateParams &parentParams,
    _Inout_ ComputeStateParams &myParams,
    _Inout_opt_ bool *pHasNoExternalReferences,
    bool hadIndependentAnimationLastTick,
    _Out_ bool *pHasIndependentAnimation
    )
{
    ASSERT(!parentParams.isReversed);

    // The frame scheduler is NULL when ticking animations for continuous handoff. Dispatcher timers never hand off to anything.
    ASSERT(parentParams.pFrameSchedulerNoRef != NULL);

    // Don't tick if there aren't any event handlers - the point of a DispatcherTimer is for app code to be called at a
    // later time. If there's no app code to be called, there's no need to tick. Note that the start time was snapped
    // when Start was called. If someone attached an event handler while the DispatcherTimer is running, we'll be able
    // to continue ticking and fire the Tick event at the appropriate time, rather than effectively restarting the timer
    // when the Tick event handler was added.
    if (m_pEventList && m_fRunning && !m_fWorkPending)
    {
        // Dispatcher timers are root timelines, so the parent time should always be provided.
        ASSERT(parentParams.hasTime);

        // Time remaining is total time minus time elapsed.
        XDOUBLE timeRemainingInMilliseconds = (m_pInterval->m_rTimeSpan - (parentParams.time - m_rLastTickTime)) * 1000.0;

        // If we've reached the point where the timer should fire, raise the event and start time over.
        if (timeRemainingInMilliseconds <= 0)
        {
            TraceLoggingProviderWrite(
                XamlTelemetry, "DispatcherTimer_Tick",
                TraceLoggingUInt64(reinterpret_cast<uint64_t>(this), "ObjectPointer"),
                TraceLoggingWideString(m_strName.GetBuffer(), "TimerName"),
                TraceLoggingUInt64(static_cast<uint64_t>(m_pInterval->m_rTimeSpan * 1000.0), "IntervalInMilliseconds"),
                TraceLoggingInt64(static_cast<int64_t>(timeRemainingInMilliseconds), "TimeRemainingInMilliseconds"),
                TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

            FireTickEvent();

            // We'll snap the start time for the next iteration in WorkComplete after the Tick event hander finishes running.
        }
        else
        {
            // Request a tick at the point when the DispatcherTimer should fire next.
            IFC_RETURN(parentParams.pFrameSchedulerNoRef->RequestAdditionalFrame(
                XcpCeiling(timeRemainingInMilliseconds),
                RequestFrameReason::TimerTick));
        }
    }

    // Check external references
    CheckHasNoExternalRefs(pHasNoExternalReferences);

    *pHasIndependentAnimation = FALSE;
    myParams.isReversed = false;

    return S_OK;
}

// Call the event manager to fire this event when the interval hits.
void CDispatcherTimer::FireTickEvent()
{
    ASSERT(m_pEventList);

    CEventManager *pEventManager = GetContext()->GetEventManager();
    if (pEventManager)
    {
        m_fWorkPending = TRUE;
        pEventManager->Raise(EventHandle(KnownEventIndex::DispatcherTimer_Tick), TRUE, this, NULL);
    }
}

_Check_return_ HRESULT CDispatcherTimer::AddEventListener(
    _In_ EventHandle hEvent,
    _In_ CValue *pValue,
    _In_ XINT32 iListenerType,
    _Out_opt_ CValue *pResult,
    _In_ bool fHandledEventsToo)
{
    return CEventManager::AddEventListener(this, &m_pEventList, hEvent, pValue, iListenerType, pResult, fHandledEventsToo);
}

_Check_return_ HRESULT CDispatcherTimer::RemoveEventListener(
    _In_ EventHandle hEvent,
    _In_ CValue *pValue)
{
    IFC_RETURN(CEventManager::RemoveEventListener(this, m_pEventList, hEvent, pValue));
    if (m_pEventList->GetHead() == NULL)
    {
        // we have no listeners
        delete m_pEventList;
        m_pEventList = NULL;
    }

    return S_OK;
}

// Begins the dispatcher timer's timeline.  If the timeline is already running, resets it.
_Check_return_ HRESULT CDispatcherTimer::Start()
 {
    auto core = GetContext();

    CTimeManager *pTimeManager = core->GetTimeManager();
    IFCPTR_RETURN(pTimeManager);

    if (!m_fAddedToManager)
    {
        IFC_RETURN(pTimeManager->AddTimeline(this));
        m_fRunning = TRUE;
        m_fAddedToManager = TRUE;
    }

    m_rLastTickTime  =  pTimeManager->GetEstimatedNextTickTime();

    TraceLoggingProviderWrite(
        XamlTelemetry, "DispatcherTimer_Start",
        TraceLoggingUInt64(reinterpret_cast<uint64_t>(this), "ObjectPointer"),
        TraceLoggingWideString(m_strName.GetBuffer(), "TimerName"),
        TraceLoggingUInt64(static_cast<uint64_t>(m_pInterval->m_rTimeSpan * 1000.0), "IntervalInMilliseconds"),
        TraceLoggingUInt64(static_cast<uint64_t>(m_rLastTickTime * 1000.0), "StartTickTimeInMilliseconds"),
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // Request a tick to update the timeline.
    // The browser host and/or frame scheduler can be NULL during shutdown.
    IXcpBrowserHost *pBH = core->GetBrowserHost();

    if (pBH != NULL)
    {
        ITickableFrameScheduler *pFrameScheduler = pBH->GetFrameScheduler();

        if (pFrameScheduler != NULL)
        {
            VERIFYHR(pFrameScheduler->RequestAdditionalFrame(0 /* immediate */, RequestFrameReason::TimerTick));
        }
    }

    return S_OK;
 }

_Check_return_ HRESULT CDispatcherTimer::Stop()
{
    CTimeManager *pTimeManager = NULL;

    if (m_fAddedToManager)
    {
        pTimeManager = GetContext()->GetTimeManager();
        IFCPTR_RETURN(pTimeManager);

        IFC_RETURN(pTimeManager->RemoveTimeline(this));
        m_fRunning = FALSE;
        m_fAddedToManager = FALSE;
    }

    TraceLoggingProviderWrite(
        XamlTelemetry, "DispatcherTimer_Stop",
        TraceLoggingUInt64(reinterpret_cast<uint64_t>(this), "ObjectPointer"),
        TraceLoggingWideString(m_strName.GetBuffer(), "TimerName"),
        TraceLoggingUInt64(static_cast<uint64_t>(m_pInterval->m_rTimeSpan * 1000.0), "IntervalInMilliseconds"),
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    return S_OK;
}

// After all work is completed in response to the fired Tick event, this callback is made
// to restart the timer.
_Check_return_ HRESULT CDispatcherTimer::WorkComplete()
{
    auto core = GetContext();
    CTimeManager *pTimeManager = core->GetTimeManager();

    IFCPTR_RETURN(pTimeManager);

    // Snap the time manager's 'wall clock' time when callback finished. The next interval is
    // now relative to this time.  This supercedes the snapped time when the tick was fired in
    // ComputeState.
    m_fWorkPending = FALSE;
    m_rLastTickTime  =  pTimeManager->GetEstimatedNextTickTime();

    TraceLoggingProviderWrite(
        XamlTelemetry, "DispatcherTimer_TickComplete",
        TraceLoggingUInt64(reinterpret_cast<uint64_t>(this), "ObjectPointer"),
        TraceLoggingWideString(m_strName.GetBuffer(), "TimerName"),
        TraceLoggingUInt64(static_cast<uint64_t>(m_pInterval->m_rTimeSpan * 1000.0), "IntervalInMilliseconds"),
        TraceLoggingUInt64(static_cast<uint64_t>(m_rLastTickTime * 1000.0), "StartTickTimeInMilliseconds"),
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // Request a tick to update the timeline now that the app's handler is done. Note that we didn't request a tick when
    // the timer elapsed because we didn't know how long the app handler would take yet.
    // The browser host and/or frame scheduler can be NULL during shutdown.
    IXcpBrowserHost *pBH = core->GetBrowserHost();
    if (pBH != NULL)
    {
        ITickableFrameScheduler *pFrameScheduler = pBH->GetFrameScheduler();

        if (pFrameScheduler != NULL)
        {
            VERIFYHR(pFrameScheduler->RequestAdditionalFrame(XcpCeiling(m_pInterval->m_rTimeSpan * 1000.0), RequestFrameReason::TimerTick));
        }
    }

    return S_OK;
 }
