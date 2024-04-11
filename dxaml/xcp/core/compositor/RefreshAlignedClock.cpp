// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RefreshAlignedClock.h"
#include "XamlTelemetry.h"
#include <FrameworkUdk/Containment.h>

// Bug 49537618: [1.5 servicing] DispatcherTimer callbacks are taking much longer on Cadmus
#define WINAPPSDK_CHANGEID_49537618 49537618

//------------------------------------------------------------------------
//
//  Synopsis:
//     Create method
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
RefreshAlignedClock::Create(
    _Outptr_ RefreshAlignedClock **ppClock
   )
{
    xref_ptr<RefreshAlignedClock> pClock;
    pClock.attach(new RefreshAlignedClock());

    IFC_RETURN(pClock->Initialize());

    *ppClock = pClock.detach();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//     ctor
//
//------------------------------------------------------------------------
RefreshAlignedClock::RefreshAlignedClock()
    : m_pIClock(NULL)
    , m_lastReportedTime(0.0f)
    , m_pRefreshRateInfo(NULL)
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//     dtor
//
//------------------------------------------------------------------------
RefreshAlignedClock::~RefreshAlignedClock()
{
    ReleaseInterface(m_pIClock);
    ReleaseInterface(m_pRefreshRateInfo);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//     Initializer
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
RefreshAlignedClock::Initialize()
{
    IFC_RETURN(gps->CreateClock(&m_pIClock));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//     Moves the clock state ahead by one tick. The clock will always move forward and
//     return a new value each Tick().
//
//------------------------------------------------------------------------
XDOUBLE
RefreshAlignedClock::Tick()
{
    auto guard = m_Lock.lock(); // can be called from ui thread + compositor thread simultaneously

    m_lastReportedTime = GetNextTickTimeInSeconds();

    if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_49537618>())
    {
        TraceLoggingProviderWrite(
            XamlTelemetry, "RefreshAlignedClock_Tick",
            TraceLoggingUInt64(reinterpret_cast<uint64_t>(this), "ObjectPointer"),
            TraceLoggingUInt64(static_cast<uint64_t>(m_lastReportedTime * 1000.0), "LastReportedTimeInMilliseconds"),
            TraceLoggingUInt64(reinterpret_cast<uint64_t>(m_pIClock), "ClockPointer"),
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));
    }

    return m_lastReportedTime;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//     Returns what the next tick time would be without changing the clock state.
//     This clock rounds its tick times to 'refresh rate' intervals.
//
//------------------------------------------------------------------------
XDOUBLE
RefreshAlignedClock::GetNextTickTimeInSeconds() const
{
    auto guard = m_Lock.lock(); // can be called from ui thread + compositor thread simultaneously

    if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_49537618>())
    {
        double currentTime = m_pIClock->GetAbsoluteTimeInSeconds();

        // Make sure time never flows backwards. If we calculated something less than the last reported time, just return
        // the last reported time.
        if (currentTime < m_lastReportedTime)
        {
            TraceLoggingProviderWrite(
                XamlTelemetry, "RefreshAlignedClock_GetNextTickTimeInSeconds_TimeMovedBackwards",
                TraceLoggingUInt64(reinterpret_cast<uint64_t>(this), "ObjectPointer"),
                TraceLoggingUInt64(static_cast<uint64_t>(currentTime * 1000.0), "CurrentTimeInMilliseconds"),
                TraceLoggingUInt64(static_cast<uint64_t>(m_lastReportedTime * 1000.0), "LastReportedTimeInMilliseconds"),
                TraceLoggingUInt64(reinterpret_cast<uint64_t>(m_pIClock), "ClockPointer"),
                TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

            currentTime = m_lastReportedTime;
        }

        return currentTime;
    }
    else
    {
        XDOUBLE refreshIntervalInSeconds;

        if (m_pRefreshRateInfo != NULL)
        {
            refreshIntervalInSeconds = static_cast<XDOUBLE>(m_pRefreshRateInfo->GetRefreshIntervalInMilliseconds()) / 1000.0;
        }
        else
        {
            // The refresh rate can't be read from the system yet.
            refreshIntervalInSeconds = static_cast<XDOUBLE>(DefaultRefreshIntervalInMilliseconds) / 1000.0;
        }

        // Round time forward to the refresh rate interval.
        XDOUBLE unroundedTime = static_cast<XDOUBLE>(m_pIClock->GetAbsoluteTimeInSeconds());
        XDOUBLE roundedTime = static_cast<XDOUBLE>(XcpCeiling(unroundedTime / refreshIntervalInSeconds)) * refreshIntervalInSeconds;
        XDOUBLE tolerance = 0.00001; // allow 0.01ms error

        // Make sure we advance time by at least a frame each time the composition thread ticks.
        if (roundedTime <= m_lastReportedTime + tolerance)
        {
            roundedTime = m_lastReportedTime + refreshIntervalInSeconds;
        }

        ASSERT(roundedTime > m_lastReportedTime);
        return roundedTime;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//     Return the same time returned by the last Tick().
//
//------------------------------------------------------------------------
XDOUBLE
RefreshAlignedClock::GetLastTickTimeInSeconds() const
{
    auto guard = m_Lock.lock();

    return m_lastReportedTime;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the object used to get the refresh rate.
//
//------------------------------------------------------------------------
void
RefreshAlignedClock::SetRefreshRateInfo(_In_opt_ IPALRefreshRateInfo *pRefreshRateInfo)
{
    // The UI thread could be reading the field in GetNextTickTimeInSeconds via a DispatcherTimer, so take the lock before
    // writing to the field on the render thread. All other reads occur on the render thread or on the UI thread inside
    // the render thread lock and do not require additional locking.
    auto guard = m_Lock.lock();
    ReplaceInterface(m_pRefreshRateInfo, pRefreshRateInfo);
}

