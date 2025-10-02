// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
//      Implement top level control model



class CompositorScheduler;
class CCoreServices;

extern HINSTANCE g_hInstance;

LONG XcpRegQueryValue( _In_ HKEY    hkey,
                       _In_opt_ LPCWSTR  lpValue,
                       _In_opt_ DWORD    dwFlags,
                       _Out_writes_bytes_opt_(*pcbData) PVOID   pvData,
                       _Inout_opt_ LPDWORD pcbData);

struct CEventInfo;

class CPendingMessages;

class CExclusiveLock
{
public:
    CExclusiveLock(_In_ SRWLOCK * plock);
    ~CExclusiveLock();

private:
    SRWLOCK * m_plock;
};

class CXcpDispatcher;

// With CoreMessaging, we want the queue of input and tick events. We request
// a callback via a dispatcher queue timer, which is at the same priority as input messages. This puts rendering and input
// in FIFO order, and neither can starve the other. We no longer need different behaviors between desktop and phone.
class CDeferredInvoke
{
public:
    CDeferredInvoke();
    ~CDeferredInvoke();
    void Destroy();

    void RegisterTarget(CXcpDispatcher* dispatcher);
    _Check_return_ HRESULT QueueDeferredInvoke(_In_ UINT nMsg, _In_ WPARAM wParam, _In_ LPARAM lParam, _Out_ bool* isFirstItem);

    void DispatchQueuedMessage(_Out_ bool* dispatchedWork, _Out_ bool* hasMoreWork);

    uint32_t GetWorkCount() const { return m_workCount; }

private:
    struct DeferredInvoke
    {
        DeferredInvoke * Next;
        DeferredInvoke * Prev;
        UINT Msg;
        LPARAM WParam;
        WPARAM LParam;
    };

    bool Enqueue(DeferredInvoke* pInvoke, _Out_ bool* hasMoreWork);
    DeferredInvoke* Dequeue(_Out_ bool* hasMoreWork);

private:
    SRWLOCK m_lock;

    // The CXcpDispatcher contains this object, so we don't need to hold a ref.  The lifetimes are the same.
    CXcpDispatcher* m_dispatcher {nullptr};

    DeferredInvoke * m_pHead;
    DeferredInvoke * m_pTail;
    uint32_t m_workCount {0};
};

class CXcpDispatcher final :
    public CXcpObjectBase<IXcpDispatcher>
{
    friend class PauseNewDispatch;
    friend class PauseNewDispatchAtControl;

public:

    static _Check_return_ HRESULT Create(
        _In_ IXcpHostSite *pSite,
        _Outptr_ IXcpDispatcher **ppDispatcher
        );

    CXcpDispatcher();
    ~CXcpDispatcher() override;
// IXcpDispatcher

////////VTABLE do not add in between only at end and if you do please add in IXCPDispatcher in host.h////
    IXcpBrowserHost * GetBrowserHost() override { return m_pBH; };
    IXcpHostSite * GetHostSite() override { return m_pSite; };

    _Check_return_ HRESULT Start() override;
    void Stop() override;
    void Disable() override;

    _Check_return_ HRESULT SetControl( _In_opt_ void *pControl, _In_opt_ EVENTPFN pFn) override;
    _Check_return_ HRESULT OnPaint() override;

    void Deinit() override;
    _Check_return_ HRESULT Init(_In_ IXcpHostSite *pSite) override;
    _Check_return_ HRESULT SetBrowserHost(_In_ IXcpBrowserHost *pBH) override;
    _Check_return_ HRESULT IsReentrancyAllowed(
                            _Out_ XINT32 *pbReentrancyAllowed) override;

    _Check_return_ HRESULT QueueTick() override;
    _Check_return_ HRESULT QueueDeferredInvoke(_In_ XUINT32 nMsg, _In_ ULONG_PTR wParam, _In_ LONG_PTR lParam) override;

    void Tick() override;
    msy::IDispatcherQueue* GetDispatcherQueueNoRef() override  {   return m_dispatcherQueue.Get();   }

////////VTABLE do not add in between only at end and if you do please add in IXCPDispatcher in host.h////

    _Check_return_ HRESULT PostMessageWithResource(_In_ UINT uMsg,
                                    _In_ WPARAM wParam, _In_ LPARAM lParam);

    static void ReleaseMessageResources(_In_ UINT msg, _In_ LPARAM lParam);

    void SendMessage(_In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);

    _Check_return_ HRESULT SetTicksEnabled(bool fTicksEnabled);

    void SetMessageReentrancyGuard(bool bMessageReentrancyGuard);
    bool GetMessageReentrancyGuard() const;

    // Run all the outstanding async work left
    void Drain();

    void ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam);

private:
    _Check_return_ HRESULT ReloadSource();

    void OnReentrancyProtectedWindowMessage(_In_ UINT msg,
                _In_ WPARAM wParam, _In_ LPARAM lParam);

    void OnWindowMessage(_In_ UINT msg, _In_ WPARAM wParam,
                            _In_ LPARAM lParam);
    void OnError(_In_ IErrorService *pErrorService);
    void OnScriptCallback(_In_ CEventInfo * pEventInfo);

    void CheckReentrancy();
    void QueueReentrancyCheck();
    void QueueReentrancyCheckAllowPaused();
    HRESULT CreateReentrancyGuardAndCheckReentrancy();

    static HRESULT CALLBACK MessageTimerCallbackStatic(void* myUserData);
    void MessageTimerCallback();

    struct WeakPtr
    {
        WeakPtr(_In_ CXcpDispatcher* dispatcher) : XcpDispatcher(dispatcher) {}
        CXcpDispatcher* XcpDispatcher;
    };
    std::shared_ptr<CXcpDispatcher::WeakPtr> GetWeakPtr();

    // Used to pause/resume dispatching at the Xaml layer without affecting CoreMessaging. Meant to be called only on
    // the UI thread.
    void PauseDispatch();
    void ResumeDispatch();

private:
    CDeferredInvoke m_DeferredInvoke;

    IXcpHostSite *m_pSite;
    IXcpBrowserHost *m_pBH;
    bool m_bStarted;
    XUINT8 m_bInit;

    // Reentrancy guard for messages
    bool m_bMessageReentrancyGuard;

    void * m_pControl;
    EVENTPFN m_pFn;

    // List of pending messages with associated resources
    CPendingMessages * m_pPendingMessages;

    bool m_fTicksEnabled;

    // Temporary timestamp debugging aid to investigate APPLICATION_HANG_BusyHang in OnTick
    FILETIME m_userTimeDebugAid;

    wrl::ComPtr<msy::IDispatcherQueue> m_dispatcherQueue;
    wrl::ComPtr<msy::IDispatcherQueueTimer> m_dispatcherQueueTimer;

    EventRegistrationToken m_messageTimerCallbackToken = {};



    std::shared_ptr<WeakPtr> m_weakPtrToThis;

    enum class State
    {
        Running,    // The normal state.
        Suspended,  // We've paused dispatching at the Xaml level to guard against reentrancy crashes. The alternative
                    // is to pause CoreMessaging dispatching altogether, but that affects CM in the entire process, and
                    // other threads could be using CM for other tasks.
        Draining    // We're running down the last messages before shutdown.
                    // We skip some kinds of messages in this state.
    };
    State m_state { State::Running };
};

// RAII wrapper around IMessageLoopExtensions::PauseNewDispatch and
// ResumeDispatch.
// Used to prevent Xaml reentrancy by making CoreMessaging stop dispatching,
// including its private window messages. Those messages will be rescheduled
// once the deferral stops (i.e. this object falls out of scope).
class PauseNewDispatch
{
public:
    PauseNewDispatch(_In_opt_ CCoreServices* coreServices);
    PauseNewDispatch(_In_ CXcpDispatcher* dispatcher);
    ~PauseNewDispatch();

    // Disallow copying
    PauseNewDispatch(const PauseNewDispatch&) = delete;
    PauseNewDispatch(PauseNewDispatch&&) = delete;
    PauseNewDispatch& operator=(const PauseNewDispatch&) = delete;
    PauseNewDispatch& operator=(PauseNewDispatch&&) = delete;

private:
    // NoRef pointer. The dispatcher is expected to be on the call stack while this object is alive (also on the stack).
    CXcpDispatcher* m_dispatcherNoRef { nullptr };
};

class PauseNewDispatchAtControl
{
public:
    PauseNewDispatchAtControl(_In_opt_ CCoreServices* coreServices);
    PauseNewDispatchAtControl(_In_ CXcpDispatcher* dispatcher);

    ~PauseNewDispatchAtControl();

    // Disallow copying
    PauseNewDispatchAtControl(const PauseNewDispatchAtControl&) = delete;
    PauseNewDispatchAtControl(PauseNewDispatchAtControl&&) = delete;
    PauseNewDispatchAtControl& operator=(const PauseNewDispatchAtControl&) = delete;
    PauseNewDispatchAtControl& operator=(PauseNewDispatchAtControl&&) = delete;

    void PauseNewDispatch();
    void ResumeNewDispatch();

private:
    // NoRef pointer. The dispatcher is expected to be on the call stack while this object is alive (also on the stack).
    CXcpDispatcher* m_dispatcherNoRef { nullptr };
    // Reference count for pause requests
    int m_pauseCount = 0; 

};
