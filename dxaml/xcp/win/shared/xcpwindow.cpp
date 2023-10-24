// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MetadataAPI.h"
#include "wrlhelper.h"
#include <FeatureFlags.h>
#include <XcpWindow.h>
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include "Microsoft.UI.Dispatching.h"
#include <coremessaging.h>
#include "microsoft.ui.coremessaging.h"
#include "GraphicsTelemetry.h"
#include "DXamlCoreTipTests.h"

#include <FrameworkUdk/Containment.h>

// Bug 45792810: Explorer first frame - DesktopWindowXamlSource spends 30ms on RoGetActivationFactory
// Bug 46468883: [1.4 servicing] Explorer first frame - DesktopWindowXamlSource spends 30ms on RoGetActivationFactory
#define WINAPPSDK_CHANGEID_46468883 46468883

#define XCPWIN32TIMER_CLASS L"WinUI_TimerClass"

static void RegisterWindowClass (WNDPROC proc, PCWSTR pszName)
{
    WNDCLASS windowclass;

    memset (&windowclass, 0, sizeof(windowclass));

    windowclass.style         = 0;
    windowclass.lpfnWndProc   = proc;
    windowclass.hInstance     = g_hInstance;
    windowclass.hCursor       = NULL;
    windowclass.hbrBackground = NULL;
    windowclass.lpszClassName = pszName;

    RegisterClass (&windowclass);
}

// RAII wrapper around IMessageLoopExtensions::PauseNewDispatch and
// ResumeDispatch.
// Used to prevent Xaml reentrancy by making CoreMessaging stop dispatching,
// including its private window messages. Those messages will be rescheduled
// once the deferral stops (i.e. this object falls out of scope).
class PauseNewDispatch
{
public:
    PauseNewDispatch(_In_ IMessageLoopExtensions* messageLoopExtensions)
        : m_messageLoopExtensions(messageLoopExtensions)
    {
        IFCFAILFAST(m_messageLoopExtensions->PauseNewDispatch());
    }

    ~PauseNewDispatch()
    {
        IFCFAILFAST(m_messageLoopExtensions->ResumeDispatch());
    }

    // Disallow copying/moving
    PauseNewDispatch(const PauseNewDispatch&) = delete;
    PauseNewDispatch(PauseNewDispatch&&) = delete;
    PauseNewDispatch& operator=(const PauseNewDispatch&) = delete;

    PauseNewDispatch& operator=(PauseNewDispatch&&) = delete;
private:
    xref_ptr<IMessageLoopExtensions> m_messageLoopExtensions;
};

//------------------------------------------------------------------------
//
//  Synopsis:
//      Takes the SRW lock exclusively.
//
//------------------------------------------------------------------------
CExclusiveLock::CExclusiveLock(
    _In_ SRWLOCK * plock)
{
    ASSERT(plock != NULL);
    m_plock = plock;

    ::AcquireSRWLockExclusive(plock);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Releases the exclusive SRW lock.
//
//------------------------------------------------------------------------
CExclusiveLock::~CExclusiveLock()
{
    ::ReleaseSRWLockExclusive(m_plock);
    m_plock = NULL;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Performs first stage of initializing instance.
//
//------------------------------------------------------------------------
CDeferredInvoke::CDeferredInvoke()
{
    InitializeSRWLock(&m_lock);

    m_hwndTarget    = NULL;
    m_pHead         = NULL;
    m_pTail         = NULL;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Cleans up resources associated with this instance.
//
//------------------------------------------------------------------------
CDeferredInvoke::~CDeferredInvoke()
{
    Destroy();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Stops the ability to perform any deferred invokes
//
//------------------------------------------------------------------------
void CDeferredInvoke::Destroy()
{
    // We can no longer call the parent window.
    m_hwndTarget = NULL;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Performs second stage of initializing instance with operations that may fail.
//
//------------------------------------------------------------------------
void CDeferredInvoke::RegisterTarget(_In_ HWND hwndTarget)
{
    ASSERT((hwndTarget != NULL));
    ASSERT((m_hwndTarget == NULL));

    // Store the target where to receive redirected messages
    m_hwndTarget = hwndTarget;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Enqueues a deferred invoke request for the target HWND's thread.  Can be called from any
//      thread.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDeferredInvoke::QueueDeferredInvoke(
    _In_ UINT nMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _Out_ bool* isFirstItem)
{
    // The message will be enqueued to the deferred invoke queue if the Core_LowerFrameworkMessagesPriority apiset is enabled.
    // We also enqueue the message (but don't request a WM_PAINT) if ticking with CoreMessaging. The CXcpDispatcher will know to
    // request callbacks in that case, and we just need to keep a list of them.

    // Create and initialize a new instance to send to the target.
    DeferredInvoke * pInvoke = reinterpret_cast<DeferredInvoke *>(malloc(sizeof(DeferredInvoke)));
    if (pInvoke == NULL)
    {
        return E_OUTOFMEMORY;
    }

    pInvoke->Next       = NULL;
    pInvoke->Prev       = NULL;
    pInvoke->HwndTarget = m_hwndTarget;
    pInvoke->Msg        = nMsg;
    pInvoke->WParam     = wParam;
    pInvoke->LParam     = lParam;

    if (!Enqueue(pInvoke, isFirstItem))
    {
        free(pInvoke);
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

LRESULT CDeferredInvoke::DispatchQueuedMessage(_Out_ bool* dispatchedWork, _Out_ bool* hasMoreWork)
{
    LRESULT result = 0;
    *dispatchedWork = false;

    DeferredInvoke* pInvoke = Dequeue(hasMoreWork);

    if (pInvoke != nullptr)
    {
        ASSERT(pInvoke->HwndTarget == m_hwndTarget);

        // Redirect to the real WNDPROC:
        // - This may or may not call DefWindowProc(), as it chooses.
        result = CXcpDispatcher::WindowProc(
            pInvoke->HwndTarget,
            pInvoke->Msg,
            pInvoke->WParam,
            pInvoke->LParam);

        // Free resources
        free(pInvoke);

        *dispatchedWork = true;
    }

    return result;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Adds the instance into the list, making it immediately visible to all threads.  Can be
//      called from any thread.
//
//------------------------------------------------------------------------
bool CDeferredInvoke::Enqueue(DeferredInvoke * pInvoke, _Out_ bool* isFirstItem)
{
    ASSERT((pInvoke->Next == NULL) && (pInvoke->Prev == NULL));

    // We don't know what thread the caller is calling us on, so take an exclusive lock
    // when adding into the list of pending deferred invokes
    {
        CExclusiveLock usinglock(&m_lock);

        if (m_pHead == NULL)
        {
            // Queue was empty, add the first item
            m_pHead = pInvoke;
            m_pTail = pInvoke;

            *isFirstItem = true;
        }
        else
        {
            // Add item to the tail of the queue
            ASSERT(m_pTail != NULL);

            pInvoke->Prev = m_pTail;
            m_pTail->Next = pInvoke;
            m_pTail = pInvoke;

            *isFirstItem = false;
        }

        m_workCount++;
        GraphicsTelemetry::DeferredInvoke_Enqueue(m_workCount);
    }

    return true;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Removes the instance from the list, but does not free the memory.
//
// NOTE: This function expects to only be called from target thread's WM_PAINT in order to properly
// keep the INTERNALPAINT state updated.
//
//------------------------------------------------------------------------
CDeferredInvoke::DeferredInvoke * CDeferredInvoke::Dequeue(_Out_ bool* hasMoreWork)
{
    DeferredInvoke * pInvoke;

    {
        CExclusiveLock usinglock(&m_lock);

        // Remove the first item in the queue.
        pInvoke = m_pHead;

        if (m_pHead != NULL)
        {
            // Remove the first item in the list
            DeferredInvoke * pNewHead = m_pHead->Next;

            if (pNewHead != NULL)
            {
                // Queue has remaining items
                ASSERT(m_pHead != m_pTail);

                pNewHead->Prev = NULL;
                m_pHead = pNewHead;
            }
            else
            {
                // Queue is now empty
                ASSERT(m_pHead == m_pTail);

                m_pHead = NULL;
                m_pTail = NULL;
            }

            // Completely detach this item
            pInvoke->Next = NULL;
            pInvoke->Prev = NULL;
        }

        *hasMoreWork = (m_pHead != nullptr);
        m_workCount--;
        GraphicsTelemetry::DeferredInvoke_Dequeue(m_workCount);
    }

    return pInvoke;
}

CXcpDispatcher::CXcpDispatcher()
{
    XCP_WEAK(&m_pSite);
    m_pSite = NULL;
    m_hwnd = NULL;
    m_bStarted = false;
    m_pFn =  NULL;
    XCP_WEAK(&m_pControl);
    m_pControl = NULL;
    XCP_WEAK(&m_pBH);
    m_pBH = NULL;
    m_bInit = FALSE;
    m_bMessageReentrancyGuard = FALSE;
    m_pPendingMessages = NULL;
    m_fTicksEnabled = TRUE;
    m_userTimeDebugAid = {0};
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Create the XCP infrastructure
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CXcpDispatcher::Init(_In_ IXcpHostSite *pSite)
{
    HRESULT hr = S_OK;

    ASSERT(!m_pSite);

    if (!pSite)
    {
        IFC(E_INVALIDARG);
    }

    m_pSite = pSite;

    IFC(CPendingMessages::Create(this, &m_pPendingMessages));

    // Create and register the target window.
    RegisterWindowClass(&CXcpDispatcher::WindowProc, XCPWIN32TIMER_CLASS);

    m_hwnd = ::CreateWindow (XCPWIN32TIMER_CLASS,
                             L"XCP",
                             0, 0, 0, 0, 0, HWND_MESSAGE, NULL, g_hInstance, NULL);

    if (m_hwnd == NULL)
    {
        IFC(::HResultFromKnownLastError());
    }

    m_DeferredInvoke.RegisterTarget(m_hwnd);

    {
        wrl::ComPtr<msy::IDispatcherQueueStatics> dispatcherQueueStatics;
        if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_46468883>())
        {
            IFCFAILFAST(ActivationFactoryCache::GetActivationFactoryCache()->GetDispatcherQueueStatics(&dispatcherQueueStatics));
        }
        else
        {
            IFCFAILFAST(wf::GetActivationFactory(
                wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Dispatching_DispatcherQueue).Get(),
                dispatcherQueueStatics.ReleaseAndGetAddressOf()));
        }

        IFCFAILFAST(dispatcherQueueStatics->GetForCurrentThread(m_dispatcherQueue.ReleaseAndGetAddressOf()));

        // XAML requires a DispatcherQueue to exist on the thread.  This is checked (with a friendlier)
        // error message in WindowsXamlManager_partial, so here, we just fail-fast.
        FAIL_FAST_ASSERT(m_dispatcherQueue != nullptr);
    }

    if (m_dispatcherQueueTimer == nullptr)
    {
        // In order to not have input and rendering starve each other, this timer must be at the same priority as input,
        // which DispatcherQueueTimer is.
        IFCFAILFAST(m_dispatcherQueue->CreateTimer(m_dispatcherQueueTimer.GetAddressOf()));

        IFCFAILFAST(m_dispatcherQueueTimer->put_IsRepeating(false));
        IFCFAILFAST(m_dispatcherQueueTimer->put_Interval(wf::TimeSpan { 0 }));

        auto messageTimerCallback = WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<msy::DispatcherQueueTimer*, IInspectable*>>([this] (msy::IDispatcherQueueTimer*, _In_ IInspectable*) -> HRESULT
        {
            HRESULT hr = CXcpDispatcher::MessageTimerCallbackStatic(this);

            IFCFAILFAST(hr);

            return hr;
        });

        IFCFAILFAST(m_dispatcherQueueTimer->add_Tick(messageTimerCallback.Get(), &m_messageTimerCallbackToken));
    }

    if (!m_messageSession)
    {
        IFCFAILFAST(CoreMsgCreateSession(MsgCreateFlags::MsgCreateFlags_Default, m_messageSession.ReleaseAndGetAddressOf()));
    }

    if (!m_messageLoopExtensions)
    {
        IFCFAILFAST(m_messageSession->GetMessageLoopExtensions(m_messageLoopExtensions.ReleaseAndGetAddressOf()));
    }

    m_bInit = TRUE;

Cleanup:
    if (FAILED(hr))
    {
        Deinit();
    }
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Link the browserhost back to the Dispatcher
//     No AddRef since they're both controlled by IXcpHostSite (Control)
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CXcpDispatcher::SetBrowserHost(_In_ IXcpBrowserHost *pBH)
{
    HRESULT hr = S_OK;
    m_pBH = pBH;

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Cleanup
//
//-------------------------------------------------------------------------
void
CXcpDispatcher::Deinit()
{
    Stop();

    m_pSite = NULL;
    m_bInit = FALSE;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Cleanup
//
//-------------------------------------------------------------------------
CXcpDispatcher::~CXcpDispatcher()
{
    Deinit();

    m_pSite = NULL;
    m_pBH = NULL;

    if (m_weakPtrToThis)
    {
        m_weakPtrToThis->XcpDispatcher = nullptr;
    }
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Set the control and function pointer
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CXcpDispatcher::SetControl(
    _In_opt_ void *pControl,
    _In_opt_ EVENTPFN pFn)
{
    m_pControl = pControl;
    m_pFn = pFn;

    RRETURN(S_OK);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Redraw
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CXcpDispatcher::OnPaint()
{
    HRESULT hr = S_OK;

    // Prevent processing of future messages while this Draw
    // is being handled. But don't fail if this draw message itself
    // is being handled while another message was being processed, so
    // the paint can occur.

    bool reentrancyChecksEnabled = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector()->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableReentrancyChecks);
    if (reentrancyChecksEnabled)
    {
        QueueReentrancyCheck();
    }

    CReentrancyGuard reentrancyGuard(&m_bMessageReentrancyGuard);

    if (!m_bInit)
    {
        IFC(E_FAIL);
    }

    // Check window size
    if (m_pBH)
    {
        IFC(m_pBH->OnPaint());
    }

Cleanup:
    if (FAILED(hr) && m_pBH)
    {
        CCoreServices* pcs = m_pBH->GetContextInterface();
        if (pcs)
            VERIFYHR(pcs->ReportUnhandledError(hr));
    }
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates an CXcpDispatcher
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CXcpDispatcher::Create(
    _In_ IXcpHostSite *pSite,
    _Outptr_ IXcpDispatcher **ppInterface
    )
{
    HRESULT hr = S_OK;
    CXcpDispatcher *pWindow = new CXcpDispatcher();

    IFC(pWindow->Init(pSite));

    // DXamlCore.cpp - Tip Test
    // Init is called once
    // DispatcherQueue created
    tip::open<DXamlInitializeCoreTest>().set_flag(TIP_reason(DXamlInitializeCoreTest::reason::created_dispatcher_xcpwindow));

    *ppInterface = static_cast<IXcpDispatcher *>(pWindow);
    pWindow = NULL;

Cleanup:
   SAFE_DELETE(pWindow);

   RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Start the timer.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CXcpDispatcher::Start()
{
    if (IsStarted())
    {
        goto Cleanup;
    }

    m_bStarted = true;

    SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR) this);

Cleanup:
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Stop the timer.
//
//------------------------------------------------------------------------
void
CXcpDispatcher::Stop()
{
    m_bStarted = false;

    if (m_dispatcherQueueTimer)
    {
        IFCFAILFAST(m_dispatcherQueueTimer->remove_Tick(m_messageTimerCallbackToken));

        m_messageTimerCallbackToken = { 0 };
    }

    if (m_hwnd)
    {
        // Stop processing of messages
        SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR) NULL);

        // Release resources associated with pending messages in the queue
        m_pPendingMessages->ReleaseResources();

        // Destroy window
        DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }

    // Stop deferred invokes first so we don't get messages after destroying window.
    m_DeferredInvoke.Destroy();

    SAFE_DELETE(m_pPendingMessages);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Function added for Dial home support. This kills the update timer and cleans the
//      browser host
//
//------------------------------------------------------------------------
void
CXcpDispatcher::Disable()
{
    if (m_pBH)
    {
        m_pBH->Deinit();
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Post a message which has a resource in wParam and/or lParam
//   This allows the resource to be released if the window needs to
//   be destroyed before the message is received.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CXcpDispatcher::PostMessageWithResource(
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    if (m_pPendingMessages)
    {
        return m_pPendingMessages->PostMessage(uMsg, wParam, lParam);
    }
    else
    {
        return E_FAIL;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Send a message with wParam and/or lParam
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CXcpDispatcher::SendMessage(
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    // TODO: SendMessage returns LRESULT, should we really be casting to HRESULT?
    return static_cast<HRESULT>(::SendMessage((HWND)GetHandle(), uMsg, wParam, lParam));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      The wndproc
//
//------------------------------------------------------------------------
LRESULT
CXcpDispatcher::WindowProc(
    _In_ HWND   hwnd,
    _In_ UINT   msg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    LRESULT lRet = 0;
    HRESULT hr = S_OK;
    bool bDoDefault = true;

    CXcpDispatcher *pDispatcher =
            (CXcpDispatcher *) ::GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (!pDispatcher)
    {
        goto Cleanup;
    }

    IFC(pDispatcher->ProcessMessage(hwnd, msg, wParam, lParam, &lRet, &bDoDefault));

Cleanup:
    if (bDoDefault)
    {
        lRet = ::DefWindowProc(hwnd, msg, wParam, lParam);
    }

    if (FAILED(hr))
    {
        ReleaseMessageResources(hwnd, msg, wParam, lParam);
    }
    return lRet;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Process message
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CXcpDispatcher::ProcessMessage(
    _In_ HWND   hwnd,
    _In_ UINT   msg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _Out_ LRESULT *plRet,
    _Out_ bool *pbDoDefault)
{
    LRESULT lRet = 0;
    HRESULT hr = S_OK;
    bool bDoDefault = true;

    switch (msg)
    {
        // Messages that are reentrancy protected
        case WM_INTERNAL_TICK:
        case WM_APPLICATION_STARTUP_EVENT_COMPLETE:
        case WM_SCRIPT_SYNC_CALL_BACK:
        case WM_EXECUTE_ON_UI_CALLBACK:
        case WM_EXECUTE_ON_UI_CALLBACK_BLOCK_REENTRANCY:
        {
            lRet = OnReentrancyProtectedWindowMessage(
                            hwnd, msg, wParam, lParam);
            bDoDefault = FALSE;
            break;
        }

        // Messages that are not reentrancy protected
        case WM_XCP_ERROR_MSG:
        case WM_SCRIPT_CALL_BACK:
        case WM_SCRIPT_SYNC_INPUT_CALL_BACK:
        case WM_SCRIPT_SYNC_LOADED_CALL_BACK:
        case WM_SCRIPT_SYNC_ALLOW_REENTRANCY_CALL_BACK:
        case WM_DOWNLOAD_ASYNC_REQUESTS:
        case WM_FORCE_GC_COLLECT_EVENT:
        case WM_MINVER_NOTIFICATION:
        case WM_EXECUTE_ON_UI_CALLBACK_ALLOW_REENTRANCY:
        case WM_SCRIPT_SYNC_EFFECTIVEVIEWPORTCHANGED:
        case WM_REPLAY_PREVIOUS_POINTERUPDATE:
        case WM_SYNC_CHANGING_FOCUS_CALLBACK:
        case WM_DISPLAYCHANGE:
        {
            lRet = OnWindowMessage(hwnd, msg, wParam, lParam);
            bDoDefault = FALSE;
            break;
        }

        default:
        {
            // Ensure that private message is handled above
            ASSERT((msg < WM_FIRST) || (msg > WM_LAST));
            break;
        }
    }

    *pbDoDefault = bDoDefault;
    *plRet = lRet;
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Message handler for window messages that are reentrancy
//  protected. Components like Layout & Parser that callout to user
//  code are protected from being reentered by a message handler while
//  a previous message is being processed, because that previous message
//  handler may be in the same component.
//
//------------------------------------------------------------------------
LRESULT
CXcpDispatcher::OnReentrancyProtectedWindowMessage(
    _In_ HWND   hwnd,
    _In_ UINT   msg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    HRESULT hr = S_OK;
    ASSERT(m_bInit);

    // Don't process a message if another message is being processed.
    // This prevents reentrancy caused by a message handler pumping
    // messages.
    //
    // For example, the WM_INTERNAL_TICK handler can perform  layout.
    // Layout manager has callouts to user code, and that user
    // code may pump messages, causing reentrancy here.
    CReentrancyGuard reentrancyGuard(&m_bMessageReentrancyGuard);
    if (FAILED(reentrancyGuard.CheckReentrancy(FALSE /*bNullAvOnReentrancy*/)))
    {
        // Reentrancy detected. Report and fail-fast.
        if (m_pSite)
        {
            m_pSite->OnReentrancyDetected();
        }
        XCP_FAULT_ON_FAILURE(0);
    }

    bool reentrancyChecksEnabled = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector()->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableReentrancyChecks);
    if (reentrancyChecksEnabled)
    {
        QueueReentrancyCheck();
    }

    switch (msg)
    {
        case WM_INTERNAL_TICK:
            {
                // Skip ticks in the "Draining" state.
                if (m_pSite && m_state == State::Running)
                {
                    Tick();
                }
            }
            break;

        case WM_APPLICATION_STARTUP_EVENT_COMPLETE:
            {
                if (m_pSite && !m_pSite->IsWindowDestroyed())
                {
                    m_pBH->ApplicationStartupEventComplete();
                }
            }
            break;

        case WM_SCRIPT_SYNC_CALL_BACK:
            {
                // Synchronous events can cause reentrancy in Xaml if the event
                // handler does something like PeekMessage that pumps messages. Call
                // PauseNewDispatch to make CoreMessaging ignore its private window
                // messages for the duration of the synchronous event. When we
                // reenable those messages, CoreMessaging will reschedule all the
                // messages that it skipped.
                PauseNewDispatch deferReentrancy(m_messageLoopExtensions.get());
                OnScriptCallback(reinterpret_cast<CEventInfo *>(lParam));
                break;
            }

        case WM_EXECUTE_ON_UI_CALLBACK_BLOCK_REENTRANCY:
            {
                PauseNewDispatch deferReentrancy(m_messageLoopExtensions.get());
                if (m_pBH)
                {
                    IFC(m_pBH->ProcessExecuteMessage());
                }
            }
            break;

        case WM_EXECUTE_ON_UI_CALLBACK:
            {
                if (m_pBH)
                {
                    IFC(m_pBH->ProcessExecuteMessage());
                }
            }
            break;

        default:
            // Missing message handler?
            ASSERT(0);
            break;
    }

Cleanup:
    if (FAILED(hr))
    {
        // Release any resources associated with messages (Some messages
        // pass pointers in lParam)
        ReleaseMessageResources(hwnd, msg, wParam, lParam);
    }

    return 0;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Message handler for window messages that are not reentrancy
//  protected.
//
//------------------------------------------------------------------------
LRESULT
CXcpDispatcher::OnWindowMessage(
    _In_ HWND   hwnd,
    _In_ UINT   msg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    switch (msg)
    {
        case WM_DISPLAYCHANGE:
        {
        }
        break;

        // WM_XCP_ERROR_MSG can cause reentrancy, so cannot be processed
        // by OnWindowMessage(), which has a reentrancy guard.
        //
        // WM_XCP_ERROR_MSG calls ReportError, which eventually calls
        // CIEHostCallback::CallScriptFunction, which can report the error
        // to the DOM's script object and cause a MessageBox to
        // display the error. The MessageBox can cause this window's
        // messages to be pumped and cause reentrancy. It looks like
        // this reentrancy is OK because an error is being reported.
        case WM_XCP_ERROR_MSG:
        {
            OnError(reinterpret_cast<IErrorService *>(lParam));
            break;
        }

        // On async events, user code is allowed to display modal windows that
        // pump messages and cause reentrancy. Also, the code to callout doesn't
        // do much, so need not protected against reentrancy.
        case WM_SCRIPT_CALL_BACK:
        {
            OnScriptCallback(reinterpret_cast<CEventInfo *>(lParam));
            break;
        }

        // These input events are fired synchronously to the application,
        // without taking the reentrancy guard, because the code path to
        // fire the event has been hardened against reentrancy. However,
        // if the reentrancy guard is already taken, the application will
        // be halted to prevent reentrancy in XAML components that have
        // not been hardened against reentrancy
        case WM_SCRIPT_SYNC_INPUT_CALL_BACK:
        {
            CheckReentrancy();
            OnScriptCallback(reinterpret_cast<CEventInfo *>(lParam));
            break;
        }

        case WM_SYNC_CHANGING_FOCUS_CALLBACK:
        {
            OnScriptCallback(reinterpret_cast<CEventInfo *>(lParam));
            break;
        }

        case WM_SCRIPT_SYNC_EFFECTIVEVIEWPORTCHANGED:
        case WM_SCRIPT_SYNC_LOADED_CALL_BACK:
        case WM_SCRIPT_SYNC_ALLOW_REENTRANCY_CALL_BACK:
        {
            OnScriptCallback(reinterpret_cast<CEventInfo *>(lParam));
            break;
        }

        // AsyncDownload manager can handle reentrancy.
        // Also, anti-virus scanners loaded by URLMON during the resource bind,
        // can pump messages during their scan and cause reentrancy, so don't
        // protect against reentrancy.
        case WM_DOWNLOAD_ASYNC_REQUESTS:
        {
            // Download requests
            IAsyncDownloadRequestManager* pAsyncDownloadRequestManager =
                        reinterpret_cast<IAsyncDownloadRequestManager *>(lParam);
            ASSERT(pAsyncDownloadRequestManager);

            AddRefInterface(pAsyncDownloadRequestManager);
            IGNOREHR(m_pPendingMessages->ReleaseResource(msg, wParam, lParam));

            IGNOREHR(pAsyncDownloadRequestManager->DownloadRequests());
            ReleaseInterface(pAsyncDownloadRequestManager);

        }
        break;

        case WM_FORCE_GC_COLLECT_EVENT:
        {
            CCoreServices *pCore;

            if (m_pBH)
            {
                pCore = m_pBH->GetContextInterface();

                if (pCore)
                {
                    pCore->ForceGCCollect();
                }
            }
        }
        break;

        case WM_EXECUTE_ON_UI_CALLBACK_ALLOW_REENTRANCY:
        {
            // To protect against reentrancy, maintain a reference to the BH
            // until the execute message has been processed.

            IXcpBrowserHost* pBH = m_pBH;
            if (pBH)
            {
                AddRefInterface(pBH);
                IGNOREHR(pBH->ProcessExecuteMessage());
                ReleaseInterface(pBH);
            }
        }
        break;

        case WM_REPLAY_PREVIOUS_POINTERUPDATE:
        {
            if (m_pBH != nullptr)
            {
                m_pBH->ReplayPreviousPointerUpdate(static_cast<UINT32>(wParam));
            }
            break;
        }

    default:
        // Missing message handler?
        ASSERT(0);
        break;
    }

    return 0;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      WM_XCP_ERROR_MSG handler
//
//------------------------------------------------------------------------
void
CXcpDispatcher::OnError(
    _In_ IErrorService *pErrorService)
{
    IXcpHostSite  *pSite = NULL;
    IError *pError = NULL;
    XUINT32 bResetCore = 0;
    CCoreServices *pCore = NULL;

    VERIFYHR(pErrorService->GetLastReportedError(&pError));

    if (pError)
    {
       bResetCore = !(pError->GetIsRecoverable());
    }

    IXcpBrowserHost *pBH = (IXcpBrowserHost *) m_pBH;

    XUINT32 previousCoreId = (pBH != NULL && pBH->GetContextInterface() != NULL) ? pBH->GetContextInterface()->GetIdentity() : 0;

    if (bResetCore == TRUE && pBH != NULL)
    {
        if (pError != NULL && pError->GetErrorCode() == AG_E_INIT_ROOTVISUAL)
        {
            IGNOREHR(pBH->ResetVisualTree());
        }
        else
        {
            VERIFYHR(pBH->ResetVisualTree());
        }
    }

    pSite = m_pSite;

    pSite->IncrementReference();

    // Report error callsback to script which can reset the control causing the
    // current browserhost and dispatcher to get destroyed. To protect from this we
    // need to take a strong ref
    // ----:NOTE:---
    // Currently we have the assumption that the Controls Init will release previously held
    // Browserhost and Dispatcher hence this addref here.
    // IN V1.1 we need to fix that assumption and make sure that the Lifetime of
    // Browserhost and Dispatcher is same as the control. Once fixed we need to
    // remove this code.
    AddRefInterface(pBH);
    AddRef();

    if (pBH)
    {
        pCore = pBH->GetContextInterface();
        AddRefInterface(pCore);
    }

    //
    // Below is a SYNC call.
    //
    pSite->ReportError(pErrorService);

    if (bResetCore == TRUE && pBH != NULL && pSite->IsInitialized())
    {
        // We need to be sure that core was not reset by the actions of the
        // error handler. If it has already been reset, we don't need to do
        // it again.

        XUINT32 currentCoreId = (pBH->GetContextInterface() != NULL) ? pBH->GetContextInterface()->GetIdentity() : 0;
        if (previousCoreId == currentCoreId)
        {
            VERIFYHR(pBH->ResetState());
        }
        else
        {
            // Once error has been reported, remove it from the ErrorService's list
            // of errors.
            VERIFYHR(pErrorService->CleanupLastError(pError));
        }
    }
    else
    {
        // Once error has been reported, remove it from the ErrorService's list
        // of errors.
        VERIFYHR(pErrorService->CleanupLastError(pError));
    }

    ReleaseInterface(pCore);
    pSite->ReleaseReference();
    ReleaseInterface(pBH);
    Release();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      WM_SCRIPT_CALL_BACK handler
//
//------------------------------------------------------------------------
void
CXcpDispatcher::OnScriptCallback(
    _In_ CEventInfo * pEventInfo)
{
    IXcpBrowserHost* pBH = m_pBH;
    IXcpHostSite* pHost = reinterpret_cast<IXcpHostSite*>(m_pControl);
    // if there is a call back invoke it

    if (pEventInfo && pBH && pHost && m_pFn)
    {
        CCoreServices* core = pBH->GetContextInterface();

        // We rely on the control for the calling function. So we need to hold a ref here
        pHost->IncrementReference();

        // It is not possible to create a pEventInfo with a null control
        // pass 0 for handle (second parameter) since JS does not care

        // The script can cause the control to reset and destroy the browser host we need to take a
        // strong reference.
        // ----:NOTE:---
        // Currently we have the assumption that the Controls Init will release previously held
        // Browserhost and Dispatcher hence this addref here.
        // IN V1.1 we need to fix that assumption and make sure that the Lifetime of
        // Browserhost and Dispatcher is same as the control. Once fixed we need to
        // remove this code.

        AddRefInterface(core);
        AddRefInterface(pBH);
        AddRef();
        //create the temporary EventInfo due to:
        //When OnScriptCallback calls out to a script function set as an event handler, the script function can
        //cause the source to be reset, which causes the Core to be reset, which resets the Dispatcher.  When
        //this happens, the CEventInfo that was originally being used will be invalid.
        {
            // This needs to be destructed before we release the core
            CEventInfo eventInfoTemp = *pEventInfo;

            if (pEventInfo->IsValid)
            {
                TraceUIThreadCallbackBegin();
                m_pFn(
                    m_pControl,
                    eventInfoTemp.Listener,
                    eventInfoTemp.Event,
                    eventInfoTemp.Sender,
                    eventInfoTemp.Args,
                    eventInfoTemp.Flags,
                    nullptr,
                    eventInfoTemp.Handler);
                TraceUIThreadCallbackEnd();
            }

            // Check that the event handler has not deinited Dispatcher
            if (m_bInit)
            {
                IGNOREHR(m_pPendingMessages->ReleaseResource(WM_SCRIPT_CALL_BACK, 0, reinterpret_cast<LPARAM>(pEventInfo)));
            }
        }
        ReleaseInterface(core);
        pBH->Release();
        Release();

        pHost->ReleaseReference();
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Release resources held by message
//
//------------------------------------------------------------------------
void
CXcpDispatcher::ReleaseMessageResources(
    _In_ HWND   hwnd,
    _In_ UINT   msg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{

    switch (msg)
    {
        case WM_SCRIPT_CALL_BACK:
        {
            CEventInfo *pEventInfo =
                reinterpret_cast<CEventInfo*>(lParam);
            ASSERT(pEventInfo);

            delete pEventInfo;
            break;
        }

        case WM_DOWNLOAD_ASYNC_REQUESTS:
        {
            // Download requests
            IAsyncDownloadRequestManager* pAsyncDownloadRequestManager =
                        reinterpret_cast<IAsyncDownloadRequestManager *>(lParam);
            ASSERT(pAsyncDownloadRequestManager);

            pAsyncDownloadRequestManager->Release();
        }
        break;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Timer tick
//
//------------------------------------------------------------------------
void
CXcpDispatcher::Tick()
{
    HRESULT hr = S_OK;
    CCoreServices* pcs = NULL;

    if (!m_fTicksEnabled)
    {
        ITickableFrameScheduler *pFrameScheduler = m_pBH->GetFrameScheduler();

        if (pFrameScheduler)
        {
            // If we skip a tick, we need to still notify the frame scheduler that we've processed it, to ensure
            // that the internal state management of the tick scheduler is correct.
            IFC(pFrameScheduler->BeginTick());
            IFC(pFrameScheduler->EndTick());
        }

        goto Cleanup;
    }

    if (m_pBH)
    {
        // m_userTimeDebugAid is a temporary debugging aid to detect if a APPLICATION_HANG_BusyHang crash here is
        // caused by a long-running OnTick(). In windbg subtract m_userTimeDebugAid from the thread's UserMode time at
        // crash, obtained using .ttime. Then determine if this difference is less than ExecutionManager's limit for no-response
        // detection. If so, slow app or XAML code before this tick may be the cause and OnTick can be eliminated
        // as the cause. If not, XAML is taking too long in OnTick() and futher investigation is needed. Each call
        // to GetThreadTimes() took about 2 microseconds on desktop, so the overhead of adding this looks acceptable.
        FILETIME unusedTime;
        ::GetThreadTimes(::GetCurrentThread(), &unusedTime, &unusedTime, &unusedTime, &m_userTimeDebugAid);

        pcs = m_pBH->GetContextInterface();

        if (EventEnabledTickInfo())
        {
            ITickableFrameScheduler *pFrameScheduler = m_pBH ->GetFrameScheduler();
            TraceTickInfo(pFrameScheduler->IsHighPriority());
        }

        IFC(m_pBH->OnTick());
    }

Cleanup:
    if (FAILED(hr) && pcs)
    {
        IGNOREHR(pcs->ReportUnhandledError(hr));
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Reentrancy into the Silverlight control is allowed from
//  asynchronous callouts, but not synchronous callouts, because core
//  and other important components may be on the stack in synchronous
//  callouts.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CXcpDispatcher::IsReentrancyAllowed(
    _Out_ XINT32 *pbReentrancyAllowed)
{
    // m_bMessageReentrancyGuard is set for synchronous callouts
    *pbReentrancyAllowed = (m_bMessageReentrancyGuard) ? FALSE : TRUE;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Posts a tick message to the hWnd.  Can be called from any thread.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CXcpDispatcher::QueueTick()
{
    return QueueDeferredInvoke(WM_INTERNAL_TICK, 0, 0);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Queues a deferred invoke.  Can be called from any thread.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CXcpDispatcher::QueueDeferredInvoke(
    _In_ XUINT32 nMsg,
    _In_ ULONG_PTR wParam,
    _In_ LONG_PTR lParam)
{
    bool isFirstItem;
    IFC_RETURN(m_DeferredInvoke.QueueDeferredInvoke(nMsg, wParam, lParam, &isFirstItem));

    // Xaml depends on callbacks made from CoreMessaging. We kick off this process by starting
    // a dispatcher queue timer, which will cause a callback to eventually come in on the UI thread. This timer is at
    // the same priority as input messages, so ticking will be interleaved with input.
    if (isFirstItem)
    {
        GraphicsTelemetry::DispatcherQueueTimer_Start();

        // If the queue is empty then start the timer to kick things off...
        IFCFAILFAST(m_dispatcherQueueTimer->Start());
    }

    // ...otherwise we've already started the timer, and we can just wait for that tick to come in.

    return S_OK;
}

HRESULT CALLBACK CXcpDispatcher::MessageTimerCallbackStatic(void* myUserData)
{
    CXcpDispatcher* dispatcher = reinterpret_cast<CXcpDispatcher*>(myUserData);
    dispatcher->MessageTimerCallback();
    return S_OK;
}

void CXcpDispatcher::MessageTimerCallback()
{
    GraphicsTelemetry::DispatcherQueueTimer_Callback();

    bool dispatchedWork_DontCare;
    bool hasMoreWork;
    m_DeferredInvoke.DispatchQueuedMessage(&dispatchedWork_DontCare, &hasMoreWork);

    if (hasMoreWork)
    {
        // There are more queued messages to dispatch. Request another callback via the timer. This will let CoreMessaging
        // finish the currently queued input messages before calling us back again. This interleaves ticking with input and
        // prevents starvation.
        GraphicsTelemetry::DispatcherQueueTimer_Callback_Restart();
        IFCFAILFAST(m_dispatcherQueueTimer->Start());
    }
}

// Synchronously finish all the queued up work.  This is called when a XAML core is shutting down on the thread -- after
// all the elements have left the live trees, and before the core has actually been shut down.  Draining the queue here
// ensures that we fire outstanding async events such as FrameworkElement.Unloaded to the app.  Must only be called on the
// UI thread.
void CXcpDispatcher::Drain()
{
    ASSERT(m_state == State::Running);
    m_state = State::Draining;

    bool hasMoreWork {true};
    do
    {
        bool dispatchedWork_DontCare {false};
        m_DeferredInvoke.DispatchQueuedMessage(&dispatchedWork_DontCare, &hasMoreWork);
    }
    while (hasMoreWork);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Report reentrancy and halt application if reentrancy guard has been taken.
//      Note that this method does not take the reentrancy guard, it only checks
//      if the guard has already been taken.
//
//------------------------------------------------------------------------
void CXcpDispatcher::CheckReentrancy()
{
    if (m_bMessageReentrancyGuard)
    {
        // Reentrancy detected. Report and fail-fast.
        if (m_pSite)
        {
            m_pSite->OnReentrancyDetected();
        }
        XCP_FAULT_ON_FAILURE(0);
    }
}

std::shared_ptr<CXcpDispatcher::WeakPtr> CXcpDispatcher::GetWeakPtr()
{
    if (!m_weakPtrToThis)
    {
        m_weakPtrToThis = std::make_shared<WeakPtr>(this);
    }
    return m_weakPtrToThis;
}

void CXcpDispatcher::QueueReentrancyCheck()
{
    // When we call out to app code, we don't want it to do anything that can
    // pump messages in the message queue. So we prime the message queue with a
    // reentrancy check message that will be dispatched and cause a failfast if
    // someone pumps while Xaml is still on the stack. If we posted this
    // reentrancy check normally, it might be queued behind multiple input
    // messages that are already in the queue. Instead, we want this reentrancy
    // check message at the head of the queue. Otherwise the app code might pump
    // a few input messages and exit before the reentrancy check message is
    // dispatched. We use DispatcherQueuePriority_High to ensure that the
    // failfast message is at the head of the queue.
    boolean enqueued;
    m_dispatcherQueue->TryEnqueueWithPriority(
        msy::DispatcherQueuePriority::DispatcherQueuePriority_High,
        WRLHelper::MakeAgileCallback<msy::IDispatcherQueueHandler>([weakPtr = GetWeakPtr()]() -> HRESULT
            {
                if (CXcpDispatcher* that = weakPtr->XcpDispatcher)
                {
                    return that->CreateReentrancyGuardAndCheckReentrancy();
                }
                return S_OK;
            }).Get(),
        &enqueued);
}

HRESULT CXcpDispatcher::CreateReentrancyGuardAndCheckReentrancy()
{
    CReentrancyGuard reentrancyGuard(&m_bMessageReentrancyGuard);
    if (FAILED(reentrancyGuard.CheckReentrancy(false /*bNullAvOnReentrancy*/)))
    {
        // Reentrancy detected. Report and fail-fast.
        if (m_pSite)
        {
            m_pSite->OnReentrancyDetected();
        }
        XCP_FAULT_ON_FAILURE(0);
    }
    return S_OK;
}

void CXcpDispatcher::SetMessageReentrancyGuard(bool bMessageReentrancyGuard)
{
    m_bMessageReentrancyGuard = bMessageReentrancyGuard;
}

bool CXcpDispatcher::GetMessageReentrancyGuard() const
{
    return m_bMessageReentrancyGuard;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Re-enable ticks. Queue a tick to service any requests that were dropped while the
//      ticking was disabled
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CXcpDispatcher::SetTicksEnabled(bool fTicksEnabled)
{
    HRESULT hr = S_OK;

    if (fTicksEnabled && !m_fTicksEnabled)
    {
        ITickableFrameScheduler *pFrameScheduler = m_pBH->GetFrameScheduler();

        if (pFrameScheduler)
        {
            // Request a tick to account for any ticks that were dropped while ticks were disabled. Else
            // some tick client state that was reliant on eventually getting another tick may be invalid.
            IFC(pFrameScheduler->RequestAdditionalFrame(0, RequestFrameReason::EnableTicks));
        }
    }

    m_fTicksEnabled = fTicksEnabled;

Cleanup:
    RRETURN(hr);
}
