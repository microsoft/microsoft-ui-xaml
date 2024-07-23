// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "command.h"
#include "hardwarecommand.h"
#include <XamlBehaviorMode.h>
#include <WindowsGraphicsDeviceManager.h>
#include <DependencyLocator.h>
#include <D3D11Device.h>
#include "RefreshRateInfo.h"
#include "Scheduler.h"
#include "GraphicsTelemetry.h"

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

    m_scheduler.Attach(new Scheduler(m_graphicsDeviceManager->GetRefreshRateInfo(), m_pClock));

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
    _Outptr_ RefreshAlignedClock **ppIClock
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

//
// Figures out when to wake the UI thread, and wakes it up as appropriate.
//
// We have a UIThreadScheduler object that's shared with the UI thread. The UI thread calls RequestAdditionalFrame with
// a time and a reason whenever it finds we need another frame. This could be from the tree being dirty or a
// DispatcherTimer elapsing or many other reasons. Internally the UIThreadScheduler keeps track of the next time we need
// a frame. For example, if a DispatcherTimer elapses in 2 seconds, then the tree is dirtied immediately, then we need a
// frame immediately.
//
// Here we get the next frame time from the UIThreadScheduler. If it's in the future, then we can just wait until then.
// If it's immediately, then we wake the UI thread while putting a max limit on how many frames we can have per second.
//
void CompositorScheduler::RenderThreadFrame()
{
    HRESULT hr = S_OK;

    // Generally, the scheduler should wait for more work or for the next frame scheduled,
    // but it never runs faster than the refresh rate of the primary display.
    XUINT32 timeToNextWorkInMilliseconds = XINFINITE;

    {
        auto lock = m_pDrawListsLock.lock();

        TraceCompositorLockBegin();

        // Move the clock forward.
        const XDOUBLE frameTickTime = m_pClock->Tick();

        // Now that the animation clock has been ticked, queue a tick to the UI thread.
        // It is important that this happens after ticking the clock so that the UI thread sees up-to-date
        // values. In virtualization scenarios, if the UI thread sees an "old" value, then it may choose to sleep until
        // the next vsync rather than generating new content.
        {
            auto guard = m_UIThreadSchedulerLock.lock();

            // Handle UI thread scheduling, which is slightly different than compositor scheduling since the tick is posted
            // to the UI thread and handled asynchronously.  Scheduling new ticks is also completely asynchronous.
            if (m_pUIThreadSchedulerNoRef != NULL)
            {
                const XUINT32 uiThreadRequest = m_pUIThreadSchedulerNoRef->GetScheduledIntervalInMilliseconds(frameTickTime);

                TraceLoggingProviderWrite(
                    GraphicsTelemetry, "Scheduling_UIThreadRequest",
                    TraceLoggingUInt32(uiThreadRequest, "TimeInMilliseconds"),
                    TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

                if (uiThreadRequest > 0)
                {
                    // Schedule the future tick.
                    timeToNextWorkInMilliseconds = uiThreadRequest;
                }
                else
                {
                    // Handles throttling to the refresh rate if needed
                    IFC(m_scheduler->OnImmediateUIThreadFrame());

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

        TraceCompositorLockEnd();
    } /* DrawListsLock*/

    ASSERT(timeToNextWorkInMilliseconds > 0);

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

    TraceLoggingProviderWrite(
        GraphicsTelemetry, "Scheduling_RenderThreadWaitForWork",
        TraceLoggingUInt32(1, "IsStart"),
        TraceLoggingUInt32(timeToNextWorkInMilliseconds, "TimeInMilliseconds"),
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // If the call to WaitForObjects has no timeout
    // then the render thread is only going to wake up
    // when there is more work from the UI thread.
    // Save this state so the UI thread can query it
    if (timeToNextWorkInMilliseconds == XINFINITE)
    {
        InterlockedExchange(&m_waitingForWork, TRUE);
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

    // Restore the m_waitingForWork state back to its original value
    if (timeToNextWorkInMilliseconds == XINFINITE)
    {
        InterlockedExchange(&m_waitingForWork, FALSE);
    }

    TraceLoggingProviderWrite(
        GraphicsTelemetry, "Scheduling_RenderThreadWaitForWork",
        TraceLoggingUInt32(0, "IsStart"),
        TraceLoggingUInt32(timeToNextWorkInMilliseconds, "TimeInMilliseconds"),
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));
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
