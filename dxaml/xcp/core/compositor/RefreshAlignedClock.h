// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

struct IPALClock;

//
// A clock that is shared between the scheduling thread and the UI thread. This clocked is backed by
// QueryPerformanceCounter, but rather than having every "what time is it?" question go to QPC, it caches the current
// time and only updates it from the scheduling thread. Whenever the UI thread asks for the current time, it gets a
// consistent answer. This is desirable because the UI thread can ask for the current time multiple times in a frame,
// and we want to make sure that it gets the same answer each time, otherwise timers and animations may not all be in
// sync.
//
// Note that while we take the precaution to cache the current time, there aren't that many places on the UI thread that
// ask for the current time:
//
//   1. The UI thread's TimeManager will ask for the current time once and pass it down to all the timelines inside.
//      Active timelines (i.e. Storyboard animations and DispatcherTimers) should be using the time passed down, instead
//      of asking again for the current time.
//
//      The TimeManager will also ask for the current time when it starts in order to initialize its "time started"
//      field, but when it does that it can reuse the time for evaluating animations rather than asking again. This will
//      give the desired behavior that all animations start at time 0 during startup.
//
//   2. The other place that asks for the current time is the UIThreadScheduler when starting and ending a tick. If it
//      discovers that the scheduling thread has moved the clock forward during the tick, we queue another tick from the
//      UI thread directly rather than wait for the scheduling thread to do it. This may no longer be possible. This
//      only happens if the scheduling thread wakes from its sleep and does another RenderThreadFrame while the UI
//      thread is still in the middle of a tick, but the only thing that wakes the scheduling thread should be the _end_
//      of a tick.
//
// If we can reduce this to a single place that asks for the current time, this entire class becomes unnecessary - we
// can just ask QPC for the current time instead of going through this time-caching mechanism.
//
class RefreshAlignedClock final : public CXcpObjectBase<>
{
public:
    static _Check_return_ HRESULT Create(_Outptr_ RefreshAlignedClock **ppTimeManagerClock);

    // Updates the cached time. Called on the scheduling thread only.
    XDOUBLE Tick();

    // Returns the cached time.
    XDOUBLE GetLastTickTimeInSeconds() const;

    // Returns the current time, backed by a QPC call.
    XDOUBLE GetNextTickTimeInSeconds() const;

    float GetAbsoluteTimeInMilliseconds();

private:
    RefreshAlignedClock();
    ~RefreshAlignedClock();

    _Check_return_ HRESULT Initialize();

private:
    IPALClock *m_pIClock;
    mutable wil::critical_section m_Lock;
    XDOUBLE m_lastReportedTime;
};
