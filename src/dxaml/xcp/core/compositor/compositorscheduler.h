// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

struct IXcpDispatcher;
class UIThreadScheduler;
class RefreshAlignedClock;
struct ISchedulerCommand;

#pragma once

class WindowsGraphicsDeviceManager;

class CompositorScheduler
    final : public CXcpObjectBase<IObject>
{
private:
    CompositorScheduler(_In_ WindowsGraphicsDeviceManager *pGraphicsDeviceManager);

    ~CompositorScheduler() override;

public:
#if DBG
    FORWARD_ADDREF_RELEASE(CXcpObjectBase<IObject>);
#endif /* DBG */

    static _Check_return_ HRESULT Create(
        _In_ WindowsGraphicsDeviceManager *pGraphicsDeviceManager,
        _Outptr_ CompositorScheduler **ppCompositorScheduler);

    _Check_return_ HRESULT WakeCompositionThread();

    _Check_return_ HRESULT Shutdown();

    bool IsStarted() const { return m_pCompositorWait != NULL && m_pRenderThread != NULL; }
    _Check_return_ HRESULT Startup();

    void RegisterUIThreadScheduler(
        _In_ UIThreadScheduler *pScheduler,
        _Outptr_ IPALTickableClock **ppIClock
        );

    void UnregisterUIThreadScheduler(
        _In_ UIThreadScheduler *pScheduler
        );

    bool IsIdle();

    DWORD GetRenderThreadId() { return m_renderThreadId; }

    wil::cs_leave_scope_exit GetDrawListsLock() { return m_pDrawListsLock.lock(); }

    HRESULT GetRenderThreadHR();

    void ClearRenderThreadHR();

    WindowsGraphicsDeviceManager *GetGraphicsDeviceManager() const
    {
        return m_graphicsDeviceManager.get();
    }

    void EnableRender(bool enable)
    {
        m_isRenderEnabled = enable;
    }

    _Check_return_ HRESULT OnRenderStateChanged(bool isRenderEnabled);

private:
    _Check_return_ HRESULT Initialize();

    static XINT32 RenderThreadMainStatic(
        _In_ XUINT8 *pData
        );

    XINT32 RenderThreadMain();

    void RenderThreadFrame();

    _Check_return_ HRESULT EnsureRefreshRateInfo();

private:
    RefreshAlignedClock *m_pClock;

    _Notnull_ xref_ptr<WindowsGraphicsDeviceManager> m_graphicsDeviceManager;

    _Notnull_ IPALWaitable *m_pRenderThread;
    volatile bool m_fShutdownThread;

    _Notnull_ IPALEvent *m_pCompositorWait;

    // Lock to guard access to m_pUIThreadSchedulerNoRef.
    // UIThreadScheduler registers/unregisters this weak ref during its init/shutdown.
    wil::critical_section m_UIThreadSchedulerLock;

    // Lock to guard access to the draw lists of all registered compositors. Also used to
    // ensure exclusivity of OnSubmitForDrawing and OnPickUpForDrawing on draw commands.
    // TODO: delete
    wil::critical_section m_pDrawListsLock;

    UIThreadScheduler *m_pUIThreadSchedulerNoRef;

    IPALRefreshRateInfo *m_pRefreshRateInfo;

    LONG m_waitingForWork;

    DWORD m_renderThreadId;

    //
    // The last error encountered by the render thread.
    // Written on the render thread and the UI thread.
    // Read on the UI thread.
    //
    // This error causes the UI thread to clean up after a render thread error. It will release
    // all hardware compositors registered with this scheduler, then recover them all.
    //
    HRESULT m_renderThreadHR;

    // Lock to guard the scheduler command.
    wil::critical_section m_SchedulerCommandLock;

    ISchedulerCommand *m_pRenderStateChangedCommand;
    bool m_isRenderEnabled : 1;
};
