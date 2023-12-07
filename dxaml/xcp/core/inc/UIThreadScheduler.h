// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <core.h>

struct IXcpDispatcher;
class CompositorScheduler;

// A list of reasons why anything can request a UI thread frame. Collected as a set of flags, logged when a new reason
// comes in, and cleared when we tick the UI thread.
// Note: must match RequestFrameReasonMap in the ETW manifest.
enum RequestFrameReason : UINT32
{
    ThemeChange             = 0x1,
    AnimationTick           = 0x2,
    StoryboardTick          = 0x4,
    TimerTick               = 0x8,
    VSMAnimation            = 0x10,
    RootVisualDirty         = 0x20,
    ImageDirty              = 0x40,
    VSISUpdate              = 0x80,
    RTBRender               = 0x100,
    MediaQueue              = 0x200,
    InputManager            = 0x400,
    EventManager            = 0x800,
    Paint                   = 0x1000,
    WindowSize              = 0x2000,
    Download                = 0x4000,
    PerFrameCallback        = 0x8000,
    SurfaceContentsLost     = 0x10000,
    SettingsChanged         = 0x20000,
    DeviceLost              = 0x40000,
    RequestCommit           = 0x80000,
    AfterResume             = 0x100000,
    EnableRender            = 0x200000,
    LayoutCompletedNeeded   = 0x400000,
    UnlockTime              = 0x800000,
    BuildTreeServiceWork    = 0x1000000,
    PhasedWork              = 0x2000000,
    ComponentHost           = 0x4000000,
    ConnectedAnimation      = 0x8000000,
    EnableTicks             = 0x10000000,
    SwapChainPanelHitTest   = 0x20000000,
    ReplayPointerUpdate     = 0x40000000,
    LoadedImageSurface      = 0x80000000
};

// Frame scheduler object that allows the UI thread ticks to be driven by the compositor
// scheduler on a different thread.
class UIThreadScheduler final : public CXcpObjectBase<ITickableFrameScheduler>
{
private:
    UIThreadScheduler(
        _In_ IXcpDispatcher *pDispatcher,
        _In_ CompositorScheduler *pCompositorScheduler
        );

    ~UIThreadScheduler() override;

public:
    static void Create(
        _In_ IXcpDispatcher *pDispatcher,
        _In_ CompositorScheduler* compositorScheduler,
        _Outptr_ UIThreadScheduler **ppUIThreadScheduler);

    // UI thread API
    _Check_return_ HRESULT RequestAdditionalFrame(
        XUINT32 nextTickIntervalInMilliseconds,
        RequestFrameReason reason) override;

    const bool HasRequestedAdditionalFrame() override { return m_nextTickIntervalInMilliseconds < XINFINITE; }

    _Check_return_ HRESULT BeginTick() override;
    _Check_return_ HRESULT EndTick() override;
    bool IsInTick() override;
    bool IsHighPriority() override;
    IPALTickableClock* GetClock() override { return m_pIClock; }

    // Render thread API
    XUINT32 GetScheduledIntervalInMilliseconds(XDOUBLE currentTickTimeInSeconds);
    _Check_return_ HRESULT QueueTick();

private:
    enum UIThreadSchedulerState
    {
        UITSS_Waiting,      // The UI thread is scheduling work as it comes and waiting for the render thread to queue a tick.
        UITSS_TickQueued,   // A tick has been enqueued to the dispatcher.  The UI thread will execute scheduled work then.
        UITSS_Ticking       // The UI thread is processing a tick. Frame requests will be for a subsequent tick.
    };

private:
    // The dispatcher associated with the UI thread.
    IXcpDispatcher *m_pDispatcher;

    // The shared clock between the UI and render thread.
    IPALTickableClock *m_pIClock;

    XDOUBLE m_lastTickTimeInSeconds;

    // The composition-thread scheduler associated with this UI thread.
    CompositorScheduler *m_pCompositorScheduler;

    // The state machine for the scheduler instance.
    UIThreadSchedulerState m_currentState;

    // Time, in milliseconds, since the previous tick that the next tick should occur.
    XUINT32 m_nextTickIntervalInMilliseconds;

    // True if this tick request is at high priority (higher than input).
    bool m_isHighPriority;

    // Lock to guard access to state shared between the UI and render threads.
    wil::critical_section m_NextTickLock;

    // A list of reasons why anybody has requested a frame. Cleared when we begin the tick.
    RequestFrameReason m_requestFrameReasons;
};
