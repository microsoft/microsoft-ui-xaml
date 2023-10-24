// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
//      Implement top level control model

__interface IMessageSession;
__interface IMessageLoopExtensions;

class CompositorScheduler;

extern HINSTANCE g_hInstance;

LONG XcpRegQueryValue( _In_ HKEY    hkey,
                       _In_opt_ LPCWSTR  lpValue,
                       _In_opt_ DWORD    dwFlags,
                       _Out_writes_bytes_opt_(*pcbData) PVOID   pvData,
                       _Inout_opt_ LPDWORD pcbData);

#ifdef SetWindowLongPtrW
#undef SetWindowLongPtrW
inline LONG_PTR SetWindowLongPtrW( _In_ HWND hWnd, int nIndex, LONG_PTR dwNewLong )
{
    return( ::SetWindowLongW( hWnd, nIndex, LONG( dwNewLong ) ) );
}
#endif
#ifdef GetWindowLongPtrW
#undef GetWindowLongPtrW
inline LONG_PTR GetWindowLongPtrW( _In_ HWND hWnd, int nIndex )
{
    return( ::GetWindowLongW( hWnd, nIndex ) );
}
#endif

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

// With CoreMessaging, we want the queue of input and tick events. We request
// a callback via a dispatcher queue timer, which is at the same priority as input messages. This puts rendering and input
// in FIFO order, and neither can starve the other. We no longer need different behaviors between desktop and phone.
class CDeferredInvoke
{
public:
    CDeferredInvoke();
    ~CDeferredInvoke();
    void Destroy();

    void RegisterTarget(_In_ HWND hwndTarget);
    _Check_return_ HRESULT QueueDeferredInvoke(_In_ UINT nMsg, _In_ WPARAM wParam, _In_ LPARAM lParam, _Out_ bool* isFirstItem);

    LRESULT DispatchQueuedMessage(_Out_ bool* dispatchedWork, _Out_ bool* hasMoreWork);

private:
    struct DeferredInvoke
    {
        DeferredInvoke * Next;
        DeferredInvoke * Prev;
        HWND HwndTarget;
        UINT Msg;
        LPARAM WParam;
        WPARAM LParam;
    };

    bool Enqueue(DeferredInvoke* pInvoke, _Out_ bool* hasMoreWork);
    DeferredInvoke* Dequeue(_Out_ bool* hasMoreWork);

private:
    SRWLOCK m_lock;
    HWND m_hwndTarget;
    DeferredInvoke * m_pHead;
    DeferredInvoke * m_pTail;
    uint32_t m_workCount {0};
};

class CXcpDispatcher final :
    public CXcpObjectBase<IXcpDispatcher>
{
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

    // Hwnd, ...
    XHANDLE GetHandle() { return m_hwnd;}

// Internal winproc
    static LRESULT CALLBACK
                WindowProc(
                    _In_ HWND   hwnd,
                    _In_ UINT   msg,
                    _In_ WPARAM wParam,
                    _In_ LPARAM lParam);

    _Check_return_ HRESULT PostMessageWithResource(_In_ UINT uMsg,
                                    _In_ WPARAM wParam, _In_ LPARAM lParam);

    static void ReleaseMessageResources(_In_ HWND hwnd, _In_ UINT msg,
                            _In_ WPARAM wParam, _In_ LPARAM lParam);

    _Check_return_ HRESULT SendMessage(_In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);

    _Check_return_ HRESULT SetTicksEnabled(bool fTicksEnabled);

    void SetMessageReentrancyGuard(bool bMessageReentrancyGuard);
    bool GetMessageReentrancyGuard() const;

    // Run all the outstanding async work left
    void Drain();

private:
    inline bool IsStarted() { return m_bStarted; }

    _Check_return_ HRESULT ReloadSource();

    _Check_return_ HRESULT ProcessMessage(
        _In_ HWND   hwnd,
        _In_ UINT   msg,
        _In_ WPARAM wParam,
        _In_ LPARAM lParam,
        _Out_ LRESULT *plRet,
        _Out_ bool *pbDoDefault);

    LRESULT OnReentrancyProtectedWindowMessage(_In_ HWND hwnd, _In_ UINT msg,
                _In_ WPARAM wParam, _In_ LPARAM lParam);

    LRESULT OnWindowMessage(_In_ HWND hwnd, _In_ UINT msg, _In_ WPARAM wParam,
                            _In_ LPARAM lParam);
    void OnError(_In_ IErrorService *pErrorService);
    void OnScriptCallback(_In_ CEventInfo * pEventInfo);

    void CheckReentrancy();
    void QueueReentrancyCheck();
    HRESULT CreateReentrancyGuardAndCheckReentrancy();

    static HRESULT CALLBACK MessageTimerCallbackStatic(void* myUserData);
    void MessageTimerCallback();

    struct WeakPtr
    {
        WeakPtr(_In_ CXcpDispatcher* dispatcher) : XcpDispatcher(dispatcher) {}
        CXcpDispatcher* XcpDispatcher;
    };
    std::shared_ptr<CXcpDispatcher::WeakPtr> GetWeakPtr();

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

    HWND m_hwnd;

    // List of pending messages with associated resources
    CPendingMessages * m_pPendingMessages;

    bool m_fTicksEnabled;

    // Temporary timestamp debugging aid to investigate APPLICATION_HANG_BusyHang in OnTick
    FILETIME m_userTimeDebugAid;

    wrl::ComPtr<msy::IDispatcherQueue> m_dispatcherQueue;
    wrl::ComPtr<msy::IDispatcherQueueTimer> m_dispatcherQueueTimer;

    EventRegistrationToken m_messageTimerCallbackToken = {};

    xref_ptr<IMessageSession> m_messageSession;
    xref_ptr<IMessageLoopExtensions> m_messageLoopExtensions;

    std::shared_ptr<WeakPtr> m_weakPtrToThis;

    enum class State
    {
        Running,    // The normal state.
        Draining    // We're running down the last messages before shutdown.
                    // We skip some kinds of messages in this state.
    };
    State m_state { State::Running };

};
