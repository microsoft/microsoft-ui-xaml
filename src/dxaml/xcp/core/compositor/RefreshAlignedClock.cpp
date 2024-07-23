// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RefreshAlignedClock.h"
#include "XamlTelemetry.h"

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
    auto guard = m_Lock.lock();

    m_lastReportedTime = GetNextTickTimeInSeconds();

    TraceLoggingProviderWrite(
        XamlTelemetry, "RefreshAlignedClock_Tick",
        TraceLoggingUInt64(reinterpret_cast<uint64_t>(this), "ObjectPointer"),
        TraceLoggingUInt64(static_cast<uint64_t>(m_lastReportedTime * 1000.0), "LastReportedTimeInMilliseconds"),
        TraceLoggingUInt64(reinterpret_cast<uint64_t>(m_pIClock), "ClockPointer"),
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    return m_lastReportedTime;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//     Returns what the next tick time would be without changing the clock state.
//
//------------------------------------------------------------------------
XDOUBLE
RefreshAlignedClock::GetNextTickTimeInSeconds() const
{
    auto guard = m_Lock.lock(); // can be called from ui thread + compositor thread simultaneously

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

float RefreshAlignedClock::GetAbsoluteTimeInMilliseconds()
{
    return 1000.0f * static_cast<float>(m_pIClock->GetAbsoluteTimeInSeconds());
}
