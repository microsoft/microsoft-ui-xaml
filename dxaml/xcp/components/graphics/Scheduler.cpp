// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Scheduler.h"
#include "RefreshAlignedClock.h"
#include "RefreshRateInfo.h"

Scheduler::Scheduler(_In_ RefreshRateInfo* refreshRateInfo, _In_ RefreshAlignedClock* clock)
    : m_refreshRateInfo(refreshRateInfo)
    , m_clock(clock)
{
}

Scheduler::~Scheduler()
{
}

bool Scheduler::ShouldWaitForVBlank(float currentTimeInMilliseconds, float previousFrameTimeInMilliseconds, float refreshIntervalInMilliseconds)
{
    return (currentTimeInMilliseconds - previousFrameTimeInMilliseconds) < refreshIntervalInMilliseconds;
}

_Check_return_ HRESULT Scheduler::OnImmediateUIThreadFrame()
{
    //
    // We need a frame immediately. We'll also throttle the UI thread here. Generally, there's no reason
    // to tick or render faster than the screen can display the results. The correct way to do this is
    // to throttle to the refresh rate, but that comes at a performance cost. Calling WaitForVBlank (in
    // our case, via WaitForCompositorClock) prevents the display from going into a low-power state, so
    // it's even more expensive than just keeping the CPU busy. We try to not call WaitForVBlank if we
    // don't have to.
    //
    // We'll compare QPC times first. If it's been more than a whole vblank since the previous frame,
    // let the UI thread go now. Otherwise we'll take the battery life hit and go through WaitForVBlank.
    // This handles scenarios like (reasonable) DispatcherTimers nicely - they'll schedule UI thread
    // frames more than a vblank apart, so we can skip WaitForVBlank in cases where the UI thread wakes
    // up just to call out to the app for a DispatcherTimer. This should also handle the case of sparse
    // UI tree updates from the user moving the pointer around or typing text. We should only be using
    // WaitForVBlank in cases like CompositionTarget.Rendering or a degenerate 0ms timer where the UI
    // thread requests frames immediately and nonstop.
    //
    float currentTimeInMilliseconds = m_clock->GetAbsoluteTimeInMilliseconds();
    float refreshIntervalInMilliseconds = m_refreshRateInfo->GetRefreshIntervalInMilliseconds();
    if (Scheduler::ShouldWaitForVBlank(currentTimeInMilliseconds, m_previousFrameTimeInMilliseconds, refreshIntervalInMilliseconds))
    {
        IFC_RETURN(m_refreshRateInfo->WaitForRefreshInterval());
        m_previousFrameTimeInMilliseconds = m_clock->GetAbsoluteTimeInMilliseconds();
    }
    else
    {
        m_previousFrameTimeInMilliseconds = currentTimeInMilliseconds;
    }

    return S_OK;
}
