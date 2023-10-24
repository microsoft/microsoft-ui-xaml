// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "command.h"
#include "hardwarecommand.h"
#include <XamlBehaviorMode.h>
#include <WindowsGraphicsDeviceManager.h>
#include <DependencyLocator.h>
#include <D3D11Device.h>

XUINT32 g_csPreviousBatchCount = 0;
XUINT32 g_csMaxBatchCount = 0;
XUINT32 g_csPreviousVisiblePrimitiveCount = 0;
XUINT32 g_csMaxVisiblePrimitiveCount = 0;

//------------------------------------------------------------------------------
//
//  Synopsis:
//      CompositorScheduler ctor
//
//------------------------------------------------------------------------------
CompositorScheduler::CompositorScheduler(
    _In_ WindowsGraphicsDeviceManager *pGraphicsDeviceManager
    )
    : m_pClock(NULL)
    , m_graphicsDeviceManager(pGraphicsDeviceManager)
    , m_pRenderThread(NULL)
    , m_fShutdownThread(FALSE)
    , m_pCompositorWait(NULL)
    , m_pUIThreadSchedulerNoRef(NULL)
    , m_pRefreshRateInfo(NULL)
    , m_waitingForWork(FALSE)
    , m_renderThreadId(0)
    , m_renderThreadHR(S_OK)
    , m_pRenderStateChangedCommand(NULL)
    , m_isRenderEnabled(TRUE)
{
    XCP_WEAK(&m_pUIThreadSchedulerNoRef);
}


//------------------------------------------------------------------------------
//
//  Synopsis:
//      CompositorScheduler dtor
//
//------------------------------------------------------------------------------
CompositorScheduler::~CompositorScheduler()
{
    // The compositor thread needs to be explicitly shut down before releasing the scheduler.
    XCP_FAULT_ON_FAILURE(m_pRenderThread == NULL);

    // The UI thread scheduler must be unregistered, since it holds a reference
    // on the compositor scheduler.
    ASSERT(m_pUIThreadSchedulerNoRef == NULL);

    // Note: Delete()/Close() on a sync object actually C++ deletes it...

    if (m_pCompositorWait != NULL)
    {
        VERIFYHR(m_pCompositorWait->Close());
        m_pCompositorWait = NULL;
    }

    ReleaseInterface(m_pClock);
    ReleaseInterface(m_pRefreshRateInfo);

    delete m_pRenderStateChangedCommand;
    m_pRenderStateChangedCommand = nullptr;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      CompositorScheduler static factory method
//
//------------------------------------------------------------------------------
/* static */ _Check_return_ HRESULT
CompositorScheduler::Create(
    _In_ WindowsGraphicsDeviceManager *pGraphicsDeviceManager,
    _Outptr_ CompositorScheduler **ppCompositorScheduler
    )
{
    xref_ptr<CompositorScheduler> pCompositorScheduler;
    pCompositorScheduler.attach(new CompositorScheduler(pGraphicsDeviceManager));

    IFC_RETURN(pCompositorScheduler->Initialize());

    *ppCompositorScheduler = pCompositorScheduler.detach();

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Initialize a new instance.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CompositorScheduler::Initialize()
{
    IFC_RETURN(RefreshAlignedClock::Create(&m_pClock));

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Starts the composition thread.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CompositorScheduler::Startup()
{
    // Initialize the composition thread.
    ASSERT(!IsStarted());

    ASSERT(m_pCompositorWait == NULL);
    IFC_RETURN(gps->EventCreate(
        &m_pCompositorWait,
        FALSE /* bSignaled */,
        FALSE /* bManual */
        ));

    ASSERT(m_pRenderThread == NULL);
    IFC_RETURN(gps->ThreadCreate(
        &m_pRenderThread,
        CompositorScheduler::RenderThreadMainStatic,
        1 /* cData */,
        reinterpret_cast<XUINT8 *>(this)
        ));

    ASSERT(IsStarted());

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Shuts down the compositor thread.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CompositorScheduler::Shutdown()
{
    if (m_pRenderThread != NULL)
    {
        ASSERT(m_pCompositorWait != NULL);

        // Set the shutdown flag...
        m_fShutdownThread = TRUE;

        // ...and then wake the compositor thread up:
        IFC_RETURN(m_pCompositorWait->Set());

        // Now, wait for the compositor thread to finish execution...
        IFCW32_RETURN(gps->WaitForObjects(1, &m_pRenderThread, TRUE, XINFINITE) != WAIT_FAILED);

        // ...and dispose of it. Close() actually C++ deletes it:
        IFC_RETURN(m_pRenderThread->Close());

        m_pRenderThread = NULL;
    }

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Wakes up the compositor thread, this will guarantee that all
//      compositors will get a chance to execute their current commands
//      and present.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CompositorScheduler::WakeCompositionThread()
{
    if (IsStarted())
    {
        IFC_RETURN(m_pCompositorWait->Set());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//     Registers a UI thread with this scheduler.
//
//------------------------------------------------------------------------
void
CompositorScheduler::RegisterUIThreadScheduler(
    _In_ UIThreadScheduler *pScheduler,
    _Outptr_ IPALTickableClock **ppIClock
    )
{
    auto guard = m_UIThreadSchedulerLock.lock();

    // Only one UI thread is supported right now.
    ASSERT(m_pUIThreadSchedulerNoRef == NULL);
    m_pUIThreadSchedulerNoRef = pScheduler;

    SetInterface(*ppIClock, m_pClock);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//     Unregisters a UI thread with this scheduler.
//
//------------------------------------------------------------------------
void
CompositorScheduler::UnregisterUIThreadScheduler(
    _In_ UIThreadScheduler *pScheduler
    )
{
    auto guard = m_UIThreadSchedulerLock.lock();

    // Only one UI thread is supported right now.
    ASSERT(m_pUIThreadSchedulerNoRef == pScheduler);
    m_pUIThreadSchedulerNoRef = NULL;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//     Static required by ThreadCreate that calls the instance method.
//
//------------------------------------------------------------------------
/* static */ XINT32
CompositorScheduler::RenderThreadMainStatic(
    _In_ XUINT8 *pData
    )
{
    CompositorScheduler *pCompositorScheduler =
        reinterpret_cast<CompositorScheduler *>(pData);

    return pCompositorScheduler->RenderThreadMain();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//     The top-level loop for the render thread.
//
//------------------------------------------------------------------------
#if DBG
volatile bool g_dbgCompositorThreadExitedGracefully = false;
#endif /* DBG */

XINT32
CompositorScheduler::RenderThreadMain()
{
    // Run the render thread at high priority
    IGNOREHR(gps->ThreadSetPriority(NULL, PAL_THREAD_PRIORITY_REAL_TIME));

    m_renderThreadId = GetCurrentThreadId();

    // Initialize COM for DirectManipulation support
    if (SUCCEEDED(gps->CallCoInitializeMTA()))
    {
        //
        // Render loop
        //
        while (!m_fShutdownThread)
        {
            RenderThreadFrame();
        }

        gps->CallCoUninitialize();
    }

#if DBG
    g_dbgCompositorThreadExitedGracefully = TRUE;
#endif /* DBG */

    DependencyLocator::UninitializeThread();

    return 0;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//     The method comprising everything the render thread does in a single frame.
//
//------------------------------------------------------------------------
void
CompositorScheduler::RenderThreadFrame()
{
    HRESULT hr = S_OK;

    // Generally, the scheduler should wait for more work or for the next frame scheduled,
    // but it never runs faster than the refresh rate of the primary display.
    XUINT32 timeToNextWorkInMilliseconds = XINFINITE;

    TraceRenderThreadFrameBegin();

    {
        // Scope the lock to update independent property values.
        // Both animations and manipulations can be ticked on the UI thread and this thread.

        auto lock = m_pDrawListsLock.lock();

        TraceCompositorLockBegin();

        // Move the clock forward.
        const XDOUBLE frameTickTime = m_pClock->Tick();

        // Now that the animation clock and DM have been ticked, queue a tick to the UI thread.
        // It is important that this happens after ticking the clock and DM so that the UI thread sees up-to-date
        // values. In virtualization scenarios, if the UI thread sees an "old" value, then it may choose to sleep until
        // the next vsync rather than generating new content.
        {
            auto guard = m_UIThreadSchedulerLock.lock();

            // Handle UI thread scheduling, which is slightly different than compositor scheduling since the tick is posted
            // to the UI thread and handled asynchronously.  Scheduling new ticks is also completely asynchronous.
            if (m_pUIThreadSchedulerNoRef != NULL)
            {
                const XUINT32 uiThreadRequest = m_pUIThreadSchedulerNoRef->GetScheduledIntervalInMilliseconds(frameTickTime);
                if (uiThreadRequest > 0)
                {
                    // Schedule the future tick.
                    timeToNextWorkInMilliseconds = MIN(timeToNextWorkInMilliseconds, uiThreadRequest);
                }
                else
                {
                    // Queue the requested tick now.
                    IFC(m_pUIThreadSchedulerNoRef->QueueTick());
                }
            }
        }

        {
            auto guard = m_SchedulerCommandLock.lock();

            if (m_pRenderStateChangedCommand != NULL)
            {
                hr = m_pRenderStateChangedCommand->Execute();
                SAFE_DELETE(m_pRenderStateChangedCommand);
                IFC(hr);
            }
        }

        if (m_isRenderEnabled && SUCCEEDED(GetRenderThreadHR()))
        {
            SuspendFailFastOnStowedException suspender;

            IFC(EnsureRefreshRateInfo());
        }

        TraceCompositorLockEnd();
    } /* DrawListsLock*/

    //
    // Determine how long to wait for work and/or the next scheduled frame. The next scheduled frame should have come
    // from the UI thread scheduler.
    //
    if (timeToNextWorkInMilliseconds != XINFINITE && m_pRefreshRateInfo != NULL)
    {
        XFLOAT refreshInterval = m_pRefreshRateInfo->GetRefreshIntervalInMilliseconds();

        XUINT32 roundedRefreshInterval = XcpCeiling(refreshInterval);
        // The vBlank interrupt will fire for at least vBlankTimeout times after our last Present (from observation, it's currently ~10).
        // Sleeping using a timer for any period of time less than this number of vBlanks gains nothing for battery life, since
        // the interrupt continues to fire anyway.  Also, doing so loses out on predictability of updates since timers don't fire
        // accurately compared to the hardware interrupt.  So, if the time-to-next-frame is less than 5 vBlanks, continue to
        // block on vBlanks instead of sleeping on a timer.
        // Beyond this timeout period, it's better for power consumption to sleep on a timer, which gives the system the
        // opportunity to go idle, at the cost of some variability in when the thread wakes up.  We'll always wait for
        // vBlank at least one time, though to ensure we don't tick more frequently than the refresh rate in on-demand mode.
        // TODO: TICK: Variability in timer wake-up could be reduced by waking up earlier by the timer resolution error, and then using vBlank(s) again for the remaining time.
        // TODO: TICK: Should switch to using a coalescable timer (SetWaitableTimerEx)
        const XUINT32 vBlankTimeout = 5;

        if (timeToNextWorkInMilliseconds <= vBlankTimeout * roundedRefreshInterval)
        {
            timeToNextWorkInMilliseconds = 0;
        }
        else
        {
            ASSERT(timeToNextWorkInMilliseconds > roundedRefreshInterval);

            // We're going to wait for vBlank and then a timer.
            // Reduce the timer interval by the max wait time (the refresh interval) to make sure we don't wake up too late.
            timeToNextWorkInMilliseconds -= roundedRefreshInterval;
        }
    }

    TraceRenderThreadLogTimeToNextWorkInfo(timeToNextWorkInMilliseconds);

    TraceRenderThreadFrameEnd();

    //
    // If we're scheduled immediately but we shouldn't produce another frame immediately,
    // wait for the next display refresh before proceeding.  Generally, there's no reason to
    // tick or render faster than the screen can display the results.
    //
    if (m_pRefreshRateInfo != NULL)
    {
        IFC(m_pRefreshRateInfo->WaitForRefreshInterval());
    }
    else
    {
        // We skip waiting for the vblank if there are no device resources available yet. We go idle and
        // wait for the UI thread to take action instead (by initializing resources). This only typically
        // happens during start-up.
        timeToNextWorkInMilliseconds = XINFINITE;
    }

Cleanup:
    // All render thread failures are treated as recoverable by notifying the UI thread.
    if (FAILED(hr))
    {
        // TODO: HWPC: Most failures in this method are recoverable, but there is no guarantee that we will recover.
        // TODO: HWPC: Need to add an appropriate error reporting mechanism.
        TRACE(TraceAlways, L"Render thread failure: HRESULT 0x%08x", hr);

        {
            auto guard = m_UIThreadSchedulerLock.lock();
            if (m_pUIThreadSchedulerNoRef != NULL)
            {
                IGNOREHR(m_pUIThreadSchedulerNoRef->RequestAdditionalFrame(0 /*immediate*/, RequestFrameReason::DeviceLost));
            }
        }

        // Save the error. The UI thread will detect it on the next tick and will start device lost recovery. It
        // will clear the error after recreating the CD3D11Device and submitting the frame. The window render targets
        // will recover themselves when they render.
        //
        // Save the error before updating the state of the compositor scheduler. The UI thread may be generating a
        // frame at this time, and can overwrite the scheduler state after we write to it here. To prevent this, it
        // will only overwrite the state if there is no error code. Otherwise, it will need to render another frame
        // to recover the device and clean up the error before updating the scheduler state.
        //
        PAL_InterlockedExchange(
            reinterpret_cast<XINT32 *>(&m_renderThreadHR),
            static_cast<XINT32>(hr)
            );
    }

    if (timeToNextWorkInMilliseconds > 0)
    {
        TraceRenderThreadWaitForWorkBegin();
    }

    // If the call to WaitForObjects has no timeout
    // then the render thread is only going to wake up
    // when there is more work from the UI thread.
    // Save this state so the UI thread can query it
    if (timeToNextWorkInMilliseconds == XINFINITE)
    {
        InterlockedExchange(
            &m_waitingForWork,
            TRUE
            );
    }

    // If we weren't scheduled immediately, this will let the render thread go idle until the scheduled time,
    // or until more work is submitted.
    // Even if scheduled immediately, make sure that we always reset the event.
    IGNORERESULT(gps->WaitForObjects(
        1,
        reinterpret_cast<IPALWaitable **>(&m_pCompositorWait),
        TRUE,
        timeToNextWorkInMilliseconds
        ));

    // Restore the m_waitingForWork state back to it's original value
    if (timeToNextWorkInMilliseconds == XINFINITE)
    {
        InterlockedExchange(
            &m_waitingForWork,
            FALSE
            );
    }

    if (timeToNextWorkInMilliseconds > 0)
    {
        TraceRenderThreadWaitForWorkEnd();
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Ensures that a refresh rate info object exists. Re-creates the
//      object if it's no longer valid.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CompositorScheduler::EnsureRefreshRateInfo()
{
    const auto& device = m_graphicsDeviceManager->GetGraphicsDevice();

    // If the IDXGIOutput inside the refresh rate info is no longer valid, then release it and create a new one.
    if (m_pRefreshRateInfo != NULL && !m_pRefreshRateInfo->IsValid())
    {
        ReleaseInterface(m_pRefreshRateInfo);
        m_pClock->SetRefreshRateInfo(NULL);
    }

    // Get the object for querying for the refresh rate and waiting for vblank.
    //
    // It's possible that either the graphics device or the refresh-rate info it returns are NULL, but only for
    // the period before device resources have been fully initialized.
    //
    // Whenever the UI thread submits a frame these objects will be available again because the UI thread blocks
    // waiting for them to exist. It's not a good idea for the render thread to block on resource initialization for two
    // reasons - first, it's not necessary, and second, the render thread is higher priority than the UI thread, and
    // blocking causes the resource initialization background thread also to increase in priority and c-switch out the
    // UI thread during start-up.
    if (m_pRefreshRateInfo == NULL
        && device != nullptr)
    {
        IFC_RETURN(device->GetRefreshRateInfo(&m_pRefreshRateInfo));
        m_pClock->SetRefreshRateInfo(m_pRefreshRateInfo);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//     Returns TRUE if the render thread has no more work to do
//     and is waiting for the UI thread.
//     This is called from the UI thread.  Note that if there
//     is queued work from the UI thread when this is called
//     then this can return TRUE even though the render thread
//     will soon wake up and process that work.  Therefore
//     this should only be called when there is no pending work from the UI thread.
//
//------------------------------------------------------------------------
bool
CompositorScheduler::IsIdle()
{
    // Set the flag to FALSE if it's already FALSE -- we're interested in only the return value.
    return !!InterlockedCompareExchange(
                &m_waitingForWork,
                FALSE,
                FALSE);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the last failing HRESULT encountered by the render thread.
//
//------------------------------------------------------------------------
HRESULT
CompositorScheduler::GetRenderThreadHR()
{
    // Set the flag to S_OK if it's already S_OK -- we're interested in only the return value.
    return static_cast<HRESULT>(PAL_InterlockedCompareExchange(
        reinterpret_cast<XINT32 *>(&m_renderThreadHR),
        static_cast<XINT32>(S_OK),
        static_cast<XINT32>(S_OK)
        ));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Clears the last failing HRESULT encountered by the render thread.
//      Called on the UI thread.
//
//------------------------------------------------------------------------
void
CompositorScheduler::ClearRenderThreadHR()
{
    ASSERT(GetCurrentThreadId() != m_renderThreadId);

    PAL_InterlockedExchange(
        reinterpret_cast<XINT32 *>(&m_renderThreadHR),
        static_cast<XINT32>(S_OK)
        );
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//     Called on UI thread. Adds a scheduler command to accept the new
//      render state information on next render thread frame.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CompositorScheduler::OnRenderStateChanged(bool isRenderEnabled)
{
    HRESULT hr = S_OK;
    CNotifyRenderStateChangedCommand *pCommand = NULL;

    {
        auto Lock = m_SchedulerCommandLock.lock();

        SAFE_DELETE(m_pRenderStateChangedCommand);

        pCommand = new CNotifyRenderStateChangedCommand(this, isRenderEnabled);
        m_pRenderStateChangedCommand = pCommand;
    }

    RRETURN(hr);//RRETURN_REMOVAL
}
