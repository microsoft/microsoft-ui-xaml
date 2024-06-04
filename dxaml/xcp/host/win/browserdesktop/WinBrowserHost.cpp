// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "Indexes.g.h"
#include "CommonBrowserHost.hpp"
#include "ReentrancyGuard.h"
#include <Popup.h>

#include "windowspresenttarget.h"

#include <FeatureFlags.h>
#include <KeyboardUtility.h>
#include <UIThreadScheduler.h>
#include <WindowRenderTarget.h>

#include "MetadataAPI.h"

typedef void (*PGetWindowInterface) (void **);

//-------------------------------------------------------------------------
//
//  Function:   CXcpBrowserHost::Init
//
//  Synopsis:
//     Create the XCP infrastructure
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CXcpBrowserHost::Init(_In_ IXcpHostSite *pSite, _In_ IXcpDispatcher *pDispatcher)
{
    HRESULT hr = S_OK;

    IFC(CommonBrowserHost::Init(pSite, pDispatcher));

    IFC(m_pcs->CreateErrorService(&m_pErrorService));
    m_pErrorService->CoreResetCleanup(m_pcs);

    // Register the callback for downloads.  Until the window class is backing it
    // up we don't want the core to call back to this site.
    IFC(m_pcs->RegisterDownloadSite(this));

    IFC(m_pcs->RegisterBrowserHost(this));

    // (Re-)initialize the downloader.
    ReleaseInterface(m_pDownloader);

    IFC(CDownloader::Create(
        reinterpret_cast<CDownloader **>(&m_pDownloader),
        this,
        m_pcs
        ));

    if (!m_pAsyncDownloadRequestManager)
    {
        IFC(CAsyncDownloadRequestManager::Create(this,
                                &m_pAsyncDownloadRequestManager));
    }

    static_cast<CDownloader*>(m_pDownloader)->EnableCrossDomainDownloads();

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
//      ctor
//
//-------------------------------------------------------------------------
CXcpBrowserHost::CXcpBrowserHost()
    : CommonBrowserHost()
    , m_widthOverride(0)
    , m_heightOverride(0)
    , m_previousWidth(0)
    , m_previousHeight(0)
    , m_previousPointerUpdateMsgId(0)
{
    m_bInit = FALSE;

    m_hLocalizedResource = NULL;
    m_hNonLocalizedResource = NULL;

    m_pAsyncDownloadRequestManager = NULL;
    m_bReentrancyGuard = false;

    m_fForceRedraw = false;

    m_fIsFullScreen = TRUE;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      dtor
//
//-------------------------------------------------------------------------
CXcpBrowserHost::~CXcpBrowserHost()
{
    Deinit();

    ASSERT(m_pcs == NULL);
    ASSERT(m_pNWRenderTarget == NULL);

    // Bug 30548:
    if (m_pAsyncDownloadRequestManager != NULL)
    {
        m_pAsyncDownloadRequestManager->SetIsEnabled(FALSE);
    }
    ReleaseInterface(m_pAsyncDownloadRequestManager);

// We must clean up all references to the PAL before freeing the libraries

    ReleaseInterface(m_pDownloader);

    // Don't release the interface, since we never had a reference on them, they were just softlink preserved by IXcpHostSite
    m_pSite = NULL;
    m_pDispatcherNoRef = NULL;

    FreeResourceLibraries();
}

void CXcpBrowserHost::FreeResourceLibraries()
{
    if (m_hNonLocalizedResource)
    {
        FreeLibrary(m_hNonLocalizedResource);
        m_hNonLocalizedResource = nullptr;
    }

    if (m_hLocalizedResource)
    {
        UnloadSatelliteDll(m_hLocalizedResource);
        m_hLocalizedResource = nullptr;
    }
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Cleanup
//
//-------------------------------------------------------------------------
void
CXcpBrowserHost::Deinit()
{
    CReentrancyGuard reentrancyGuard(&m_bReentrancyGuard);
    reentrancyGuard.CheckReentrancy(true);

    // Cleanup Common, as well as detach the core to make sure it's dead
    CleanupCommon();

    IGNOREHR(DetachCore());

    ASSERT(m_pcs == NULL);
    ASSERT(m_pNWRenderTarget == NULL);

    m_pDispatcherNoRef = NULL;
    m_pSite = NULL;

    m_bInit = FALSE;
}


//-------------------------------------------------------------------------
//
//  Function:   CXcpBrowserHost::put_Source
//
//  Synopsis:
//     Set the source
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
    CXcpBrowserHost::put_Source(
        _In_ XUINT32                cstr,
        _In_reads_(cstr) XUINT8*   pstr,
        _In_opt_ IPALMemory*        pPalMemory /* default is NULL */)
{
    HRESULT hr = S_OK;
    CDependencyObject *pDependencyObject = NULL;   // Instantiate the object represented by xaml
    XUINT32 iErrorCode = 0;

    TracePutSourceBegin();

    // extract the name of the string to set
    if (!m_bInit)
    {
        IFC(E_FAIL);
    }
    if (!m_pcs)
    {
        goto Cleanup;
    }

    //
    // Start dispatcher before parsing xaml file.
    // So that any potential error can be reported appropriately.
    //
    IFC(m_pDispatcherNoRef->Start());

    hr = m_pcs->LoadXaml(cstr, pstr, &pDependencyObject);
    if (FAILED(hr))
    {
        ReportError();
        goto Cleanup;
    }

    hr = m_pcs->RegisterScriptCallback(
            this,
            CXcpBrowserHost::PostAsyncScriptCallbackRequest,
            CXcpBrowserHost::SyncScriptCallbackRequest);
    if (FAILED(hr))
    {
        iErrorCode = AG_E_INIT_CALLBACK;
        goto Cleanup;
    }

    // Set the loaded object, if any, as a visual root. If we don't have an object, we still
    // call putVisualRoot to reset the visual tree.
    hr = m_pcs->putVisualRoot(pDependencyObject);

    if (FAILED(hr))
    {
        iErrorCode = AG_E_INIT_ROOTVISUAL;
        goto Cleanup;
    }

Cleanup:
    // TODO: If something fails, we should take some consistent action
    // vs. allowing different behavior depending on where it failed.

    ReleaseInterface(pDependencyObject);

    // Platform extension error are reported separately
    if (FAILED(hr) && (iErrorCode > 0 ) && (hr != E_PLATFORM_EXTENSION_FAILED))
    {
        IErrorService *pErrorService = NULL;
        VERIFYHR(m_pcs->getErrorService(&pErrorService));

        if(pErrorService)
        {
            VERIFYHR(pErrorService->ReportGenericError(hr, InitializeError, iErrorCode, 0, 0, 0, NULL, 0));
            ReportError();
        }
    }

    TracePutSourceEnd();

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CXcpBrowserHost::put_EmptySource
//
//  Synopsis:
//     Set the empty source
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CXcpBrowserHost::put_EmptySource(
    _In_ bool firstLoad
)
{
    HRESULT hr = S_OK;

    if (!m_bInit)
    {
        IFC(E_FAIL);
    }

    // If we've just started up, we don't have to reset the core--we simply start the dispatcher.
    if (firstLoad)
    {
        IFC(m_pDispatcherNoRef->Start());
    }
    else
    {
        IFC(ResetCore());
    }

    if (m_pSite)
    {
        // To ensure proper initialization, we go through the other put_Source overload
        // with empty XAML. This forces it to initialize core without actually parsing
        // anything.
        IFC(put_Source(0, NULL));
        goto Cleanup;
    }

Cleanup:
    if (FAILED(hr))
    {
        // Please note this is a very interesting model in case of failure we will do a put_Source with
        // a null tree . The reason we do this is so that we are always able to report relevant
        // error information to the user . The call below which is in XCPWindow
        // has already sent in an async message to display an error on the host so that will be processed.
        // The primary scenario driving this is XAML parse errors. We do not want to call ::Stop() here
        // because that destroys the window but the hidden sync window needs to be around as long as the control
        // is not destroyed
        (void) StopDownloads();
    }

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Tick handler
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CXcpBrowserHost::OnTick()
{
    HRESULT hr = S_OK;

    ASSERT(m_pNWRenderTarget != NULL);
    ASSERT(m_pcs != NULL);

    IFC(m_pUIThreadScheduler->BeginTick());

    {
        bool forceRedraw = m_fForceRedraw;
        bool frameDrawn = FALSE;

        // Clear before calling Draw, in case Draw's callouts causes
        // this flag to be set.
        m_fForceRedraw = false;

        // Call CORE drawing
        IFC(m_pNWRenderTarget->Draw(
            m_pcs,
            forceRedraw,
            &frameDrawn));

        // TODO: HWPC: Move frame rate counter to CCoreServices
        if (frameDrawn)
        {
            // Our frame rate is not how often we repaint but how often we
            // generate a new frame
            m_frameCounter.Inc();
            m_frameCounter.Update();
        }
    }


    IFC(m_pUIThreadScheduler->EndTick());

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CXcpBrowserHost::Create
//
//  Synopsis:
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CXcpBrowserHost::Create(
    _In_ IXcpHostSite *pSite,
    _In_ IXcpDispatcher * pDispatcher,
    _Out_ IXcpBrowserHost ** ppHost
    )
{
    HRESULT             hr = S_OK;
    CXcpBrowserHost*    pBH = NULL;

    if (pDispatcher == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pBH = new CXcpBrowserHost;

    IFC(pBH->Init(pSite, pDispatcher));

    hr = S_OK;
Cleanup:
   if (FAILED(hr))
   {
       delete pBH;
   }
   else
   {
       *ppHost = static_cast<IXcpBrowserHost *>(pBH);
   }

   RRETURN(hr);
}

//------------------------------------------------------
//
//  Download functions.
//
//------------------------------------------------------

//-------------------------------------------------------------------------
//
//  Function:   CXcpBrowserHost::CreateDownloadRequest
//
//  Synopsis:
//      Create an asynchronous download request. This will be invoked by
//  Downloader to initiate the platform specific download.
//
//      Urlmon will sometimes bind synchronously, even if asked to bind
//  asynchronously. This typically occurs when content to be downloaded
//  is small or already in the cache. Such a synchronous download can cause
//  unexpected reentrancy in the client that requested this async download.
//  So force an async download by queuing this download request and processing
//  the request when the posted WM_DOWNLOAD_ASYNC_REQUESTS is dispatched.
//
//-------------------------------------------------------------------------

_Check_return_ HRESULT
CXcpBrowserHost::CreateDownloadRequest(
        _In_ IDownloader*                           pDownloader,
        _In_ IPALUri*                               pAbsoluteUri,
        _In_ IPALDownloadRequest*                   pRequestInfo,
        _In_ XINT32                                 fCrossDomain,
        _In_ XINT32                                 fRemoveSecurityLock,
        _Outptr_result_maybenull_ IPALAbortableOperation**    ppIAbortableDownload)
{
    HRESULT hr = S_OK;
    IPALDownloadResponseCallback* pCallback = NULL;
    XUINT32 eUnsecureDownloadAction = udaNone;

    IFC(pRequestInfo->GetOptionFlags(&eUnsecureDownloadAction));

    IFCEXPECT_ASSERT(m_pAsyncDownloadRequestManager);

    IFC(pRequestInfo->GetCallback(&pCallback));

    // Queue request. Request will be processed when posted
    // WM_DOWNLOAD_ASYNC_REQUESTS is dispatched
    IFC(m_pAsyncDownloadRequestManager->QueueRequest(pDownloader,
            pAbsoluteUri, pCallback, fCrossDomain, fRemoveSecurityLock,
            ppIAbortableDownload, eUnsecureDownloadAction));

Cleanup:
    ReleaseInterface(pCallback);
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  IAsyncDownloadRequestManagerSite
//
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
//
//  Function:   CXcpBrowserHost::ProcessAsyncDownloadRequest
//
//  Synopsis:
//      Create an asynchronous download request to initiate the platform
//  specific download. This is called by the Async Download Request
//  manager.
//
//-------------------------------------------------------------------------

_Check_return_ HRESULT
CXcpBrowserHost::ProcessAsyncDownloadRequest(
    _In_ IDownloader*                           pDownloader,
    _In_ IPALUri*                               pUriAbsolute,
    _In_ IPALDownloadResponseCallback*          pICallback,
    _In_ XINT32                                 fCrossDomain,
    _In_ XINT32                                 fRemoveHttpsLock,
    _Outptr_result_maybenull_ IPALAbortableOperation**    ppIAbortableDownload,
    _In_ XUINT32                                eUnsecureDownloadAction)
{
    HRESULT hr = S_OK ;

    IFCPTR( pUriAbsolute ) ;
    IFCPTR( pICallback ) ;
    IFCPTR( pDownloader );

    IFC(ProcessAsyncDownloadRequest_BrowserImpl(pDownloader,
                                                pUriAbsolute,
                                                pICallback,
                                                fCrossDomain,
                                                ppIAbortableDownload,
                                                eUnsecureDownloadAction));

Cleanup :
    RRETURN( hr ) ;
}

//-------------------------------------------------------------------------
//
//  Function:   CXcpBrowserHost::ProcessAsyncDownloadRequest_BrowserImpl
//
//  Synopsis:
//      Create an asynchronous download request to initiate the platform
//  specific download. This is called by the Async Download Request
//  manager when the request should use the browser stack.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CXcpBrowserHost::ProcessAsyncDownloadRequest_BrowserImpl(
    _In_ IDownloader*                           pDownloader,
    _In_ IPALUri*                               pUriAbsolute,
    _In_ IPALDownloadResponseCallback*          pICallback,
    _In_ XINT32                                 fCrossDomain,
    _Outptr_result_maybenull_ IPALAbortableOperation**    ppIAbortableDownload,
    _In_ XUINT32                                eUnsecureDownloadAction)
{
    HRESULT hr = S_OK;
    CWindowsDownloadRequest* pDownloadRequest = NULL ;
    CWindowsAbortableDownload *pAbortableDownload = NULL;
    HWND hwnd = NULL;

    pDownloadRequest = new CWindowsDownloadRequest(eUnsecureDownloadAction, /* shouldSendRefererHeader */ TRUE);

    if (ppIAbortableDownload != NULL)
    {
        IFC(CWindowsAbortableDownload::Create(pDownloadRequest, &pAbortableDownload));
        *ppIAbortableDownload = (IPALAbortableOperation*) pAbortableDownload;
        pAbortableDownload = NULL;
    }

    hwnd = GetDesktopWindow();

    IFC( pDownloadRequest->InitiateRequest (
                                    pDownloader,
                                    NULL,
                                    pUriAbsolute,
                                    fCrossDomain,
                                    pICallback,
                                    false
                                    ));

Cleanup:
    ReleaseInterface( pDownloadRequest ) ;
    ReleaseInterface( pAbortableDownload ) ;

    RRETURN( hr ) ;
}


//-------------------------------------------------------------------------
//
//  Function:   CXcpBrowserHost::StartAsyncDownloadTrigger
//
//  Synopsis:
//      Use PostMessage as async trigger to process async download
//      requests
//
//-------------------------------------------------------------------------

_Check_return_ HRESULT
CXcpBrowserHost::StartAsyncDownloadTrigger()
{
    HRESULT hr = S_OK;

    ASSERT(m_pAsyncDownloadRequestManager);
    m_pAsyncDownloadRequestManager->AddRef();


    CXcpDispatcher *pDispatcher = static_cast<CXcpDispatcher *>(m_pDispatcherNoRef);
    if (FAILED(pDispatcher->PostMessageWithResource(
            WM_DOWNLOAD_ASYNC_REQUESTS,
            0,
            reinterpret_cast<LPARAM>(m_pAsyncDownloadRequestManager))))
    {
        m_pAsyncDownloadRequestManager->Release();
        IFC(E_FAIL);
    }

Cleanup:
    RRETURN(hr);
}

// Abort the abortable operation from a worker thread, to avoid blocking the UI thread.
// Currently used by CAbortableAsyncDownload.
void CXcpBrowserHost::AbortAsync(
    _In_ IPALAbortableOperation *abortable)
{
    if (m_pcs != nullptr && !m_pcs->IsDestroyingCoreServices())
    {
        xref_ptr<IPALWorkItem> workItem;
        IFCFAILFAST(m_pcs->GetWorkItemFactory()->CreateWorkItem(
            workItem.ReleaseAndGetAddressOf(),
            &CXcpBrowserHost::AbortAsyncCallback,
            abortable
            ));
        IFCFAILFAST(workItem->Submit());
    }
    else
    {
        // If the core is shutting down, then just do Abort synchronously
        abortable->Abort();
    }
}

_Check_return_ HRESULT CXcpBrowserHost::AbortAsyncCallback(_In_ IObject *data)
{
    IPALAbortableOperation* abortable = static_cast<IPALAbortableOperation*>(data);
    abortable->Abort();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Function:   CXcpBrowserHost::PostAsyncScriptCallbackRequest
//
//  Synopsis:
//      This code posts an async message to call back into script
//
//
//------------------------------------------------------------------------
HRESULT CALLBACK CXcpBrowserHost::PostAsyncScriptCallbackRequest(
    _In_ void *pVoidBH,
    _In_opt_ CDependencyObject *pListener,
    _In_opt_ EventHandle hEvent,
    _In_opt_ CDependencyObject *pSender,
    _In_opt_ CEventArgs *pArgs,
    _In_ XINT32 flags,
    _In_opt_ IScriptObject *pScriptObject,
    _In_opt_ INTERNAL_EVENT_HANDLER pHandler)
{
    CXcpBrowserHost *pBH = static_cast<CXcpBrowserHost *>(pVoidBH);
    IXcpDispatcher *pDispatcher = pBH->m_pDispatcherNoRef;

    if (!pDispatcher)
    {
        return S_OK;
    }

    std::unique_ptr<CEventInfo> eventInfo(
        new CEventInfo(
            pListener,
            hEvent,
            pSender,
            pArgs,
            flags,
            pHandler));

    IFCEXPECT_RETURN(eventInfo->IsValid);

    // Post message
    IFC_RETURN(static_cast<CXcpDispatcher *>(pDispatcher)->PostMessageWithResource(
                        WM_SCRIPT_CALL_BACK,
                        0,
                        reinterpret_cast<LPARAM>(eventInfo.get())));
    eventInfo.release(); // PostMessageWithResource succeeded, so let it handle the lifetime.

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Function:   CXcpBrowserHost::SyncScriptCallbackRequest
//
//  Synopsis:
//      This code send an sync message to call back into script
//
//------------------------------------------------------------------------

HRESULT CALLBACK CXcpBrowserHost::SyncScriptCallbackRequest(
    _In_ void *pVoidBH,
    _In_ CDependencyObject *pListener,
    _In_ EventHandle hEvent,
    _In_ CDependencyObject *pSender,
    _In_opt_ CEventArgs *pArgs,
    _In_ XINT32 flags,
    _In_opt_ IScriptObject *pScriptObject,
    _In_opt_ INTERNAL_EVENT_HANDLER pHandler)
{
    CXcpBrowserHost *pBH = static_cast<CXcpBrowserHost *>(pVoidBH);
    IXcpDispatcher *pDispatcher = pBH->m_pDispatcherNoRef;

    if (!pDispatcher)
    {
        return S_OK;
    }

    CEventInfo eventInfo(
        pListener,
        hEvent,
        pSender,
        pArgs,
        flags,
        pHandler);

    IFCEXPECT_RETURN(eventInfo.IsValid);

    UINT uMsg;

    if (EVENT_SYNC_INPUT & flags)
    {
        uMsg = WM_SCRIPT_SYNC_INPUT_CALL_BACK;
    }
    else if (EVENT_SYNC_LOADED & flags)
    {
        uMsg = WM_SCRIPT_SYNC_LOADED_CALL_BACK;
    }
    else if (eventInfo.Event == KnownEventIndex::FrameworkElement_EffectiveViewportChanged)
    {
        uMsg = WM_SCRIPT_SYNC_EFFECTIVEVIEWPORTCHANGED;
    }
    else if (eventInfo.Event == KnownEventIndex::UIElement_GettingFocus
        || eventInfo.Event == KnownEventIndex::UIElement_LosingFocus)
    {
        uMsg = WM_SYNC_CHANGING_FOCUS_CALLBACK;
    }
    else if (eventInfo.Event == KnownEventIndex::UIElement_Shown
        || eventInfo.Event == KnownEventIndex::UIElement_Hidden
        // ICoreDropOperationTarget methods are implemented by dispatching internal events, which are then
        // routed to user event handlers.  This part of the stack does not require a reentrancy guard, which
        // would otherwise interfere with a handler's ability to call back into Xaml for event arg properties, etc.
        // For details, refer to docs/design-notes/reentrancy.md
        || eventInfo.Event == KnownEventIndex::UIElement_DragEnter
        || eventInfo.Event == KnownEventIndex::UIElement_DragLeave
        || eventInfo.Event == KnownEventIndex::UIElement_DragOver
        || eventInfo.Event == KnownEventIndex::UIElement_Drop)
    {
        uMsg = WM_SCRIPT_SYNC_ALLOW_REENTRANCY_CALL_BACK;
    }
    else
    {
        uMsg = WM_SCRIPT_SYNC_CALL_BACK;
    }

    // Send message
    static_cast<CXcpDispatcher *>(pDispatcher)->SendMessage(
                uMsg,
                0,
                reinterpret_cast<LPARAM>(&eventInfo));
    return S_OK;
}


//------------------------------------------------------------------------
//
//  Function:   CXcpBrowserHost::ReportError
//
//  Synopsis:
//      Report the error from the parser.  We package up the error and
//  send a message so we can be async.
//
//
//------------------------------------------------------------------------

void
CXcpBrowserHost::ReportError()
{
    if (!m_pcs || !m_pDispatcherNoRef)
        return;

    if (m_pErrorService)
    {
        IGNOREHR(m_pDispatcherNoRef->QueueDeferredInvoke(WM_XCP_ERROR_MSG, (WPARAM) 0, (LPARAM) m_pErrorService));
    }
}


//-------------------------------------------------------------------------
//
//  Function:   CXcpBrowserHost::ApplicationStartupEventComplete
//
//  Synopsis:
//-------------------------------------------------------------------------
void
CXcpBrowserHost::ApplicationStartupEventComplete()
{
   if (!m_pcs)
        return;
   IGNOREHR(m_pcs->ApplicationStartupEventComplete());
}

//-------------------------------------------------------------------------
//
//  Function:   CXcpBrowserHost::FireApplicationStartupEventComplete
//
//  Synopsis:   Post the Deployment Complete message
//-------------------------------------------------------------------------
void
CXcpBrowserHost::FireApplicationStartupEventComplete()
{
    IGNOREHR(m_pDispatcherNoRef->QueueDeferredInvoke(WM_APPLICATION_STARTUP_EVENT_COMPLETE, (WPARAM) 0, (LPARAM) 0));
}

//-------------------------------------------------------------------------
//
//  Function:   CXcpBrowserHost::FireForceGCCollectEvent
//
//  Synopsis:
//-------------------------------------------------------------------------
void
CXcpBrowserHost::FireForceGCCollectEvent()
{
    IGNOREHR(m_pDispatcherNoRef->QueueDeferredInvoke(WM_FORCE_GC_COLLECT_EVENT, (WPARAM) 0, (LPARAM) 0));
}

//-------------------------------------------------------------------------
//
//  Function:   CMacBrowserHost::FireApplicationLoadComplete
//
//  Synopsis:   Call from core to this function indicates that, core is not
//              done lodging the Application object, creating the tree and attaching the tree
//              to RooTVisual. Its now afe to fire Controls Loaded event

//-------------------------------------------------------------------------
void
CXcpBrowserHost::FireApplicationLoadComplete()
{
    HRESULT hr = S_OK;

    IXcpHostSite *pHostSite = m_pDispatcherNoRef->GetHostSite();
    IFCPTR(pHostSite);

 Cleanup:
    return;
}

//-------------------------------------------------------------------------
//
//  Function:   CXcpBrowserHost::FireExecuteOnUIThreadMessage
//
//  Synopsis:
//     Invokes pExecuter->Execute aysnchronously on the UI thread
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CXcpBrowserHost::FireExecuteOnUIThreadMessage(_In_ IPALExecuteOnUIThread* pExecuter, const ReentrancyBehavior reentrancyBehavior)
{
    HRESULT hr = S_OK;

    IFCPTR(pExecuter);

    // This reference will be released when the message is processed
    AddRefInterface(pExecuter);

    if (m_pUIThreadExecuterQueue && m_pDispatcherNoRef)
    {
        m_pUIThreadExecuterQueue->Post(pExecuter);
        pExecuter = nullptr;

        UINT msg;
        switch (reentrancyBehavior)
        {
            case ReentrancyBehavior::AllowReentrancy:
                msg = WM_EXECUTE_ON_UI_CALLBACK_ALLOW_REENTRANCY;
                break;
            case ReentrancyBehavior::CrashOnReentrancy:
                msg = WM_EXECUTE_ON_UI_CALLBACK;
                break;
            case ReentrancyBehavior::BlockReentrancy:
                msg = WM_EXECUTE_ON_UI_CALLBACK_BLOCK_REENTRANCY;
                break;
            default:
                FAIL_FAST_ASSERT(false);
        }

        IFC(m_pDispatcherNoRef->QueueDeferredInvoke(msg, 0, 0));
    }

Cleanup:
    ReleaseInterface(pExecuter);
    RRETURN(hr);
}

void
CXcpBrowserHost::HandleKeyboardMessage(_In_ XUINT32 uMsg, _In_ MsgPacket *pMsgPack, _Inout_ InputMessage *pMsg, _Out_ bool &fHandled)
{
    // First package the message
    if((uMsg == WM_KEYDOWN) ||
       (uMsg == WM_KEYUP) ||
       (uMsg == WM_SYSKEYDOWN) ||
       (uMsg == WM_SYSKEYUP))
    {
        if (pMsgPack->m_wParam == VK_PACKET)
        {
            // This is an injected unicode character which is the IHM soft keyboard, pen, or SendInput().
            // We need to get the active keyboard state, then translate it to a corresponding virtual
            // key code from the unicode character.

            const int scanCode = (pMsgPack->m_lParam & 0x7F0000) >> 16;
            pMsgPack->m_wParam = InputUtility::Keyboard::GetVirtualKeyFromPacketInput(scanCode);

            // Populate the rest of the key status
            pMsg->m_physicalKeyStatus.m_uiRepeatCount = 1;
            pMsg->m_physicalKeyStatus.m_uiScanCode = 0;
            pMsg->m_physicalKeyStatus.m_bIsExtendedKey = 0;
        }
        else
        {
            pMsg->m_physicalKeyStatus.m_uiRepeatCount = (pMsgPack->m_lParam & 0x0000FFFF);          // bits 0-15
            pMsg->m_physicalKeyStatus.m_uiScanCode = (pMsgPack->m_lParam & 0x00FF0000) >> 16;       // bits 16-23
            pMsg->m_physicalKeyStatus.m_bIsExtendedKey = (pMsgPack->m_lParam & 0x01000000) >> 24 != 0;   // bit 24
        }

        pMsg->m_platformKeyCode = static_cast<wsy::VirtualKey>(pMsgPack->m_wParam);
        if (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN)
        {
            pMsg->m_physicalKeyStatus.m_bIsMenuKeyDown = (pMsgPack->m_lParam & 0x20000000) >> 29 != 0;   // bit 29
            pMsg->m_physicalKeyStatus.m_bWasKeyDown = (pMsgPack->m_lParam & 0x40000000) >> 30 != 0;  // bit 30
            pMsg->m_physicalKeyStatus.m_bIsKeyReleased = 0;                                     // always 0 for KEYDOWN
        }
        else
        {
            pMsg->m_physicalKeyStatus.m_bIsMenuKeyDown = (pMsgPack->m_lParam & 0x20000000) >> 29 != 0;   // bit 29
            pMsg->m_physicalKeyStatus.m_bWasKeyDown = 1;                                        // always 1 for KEYUP
            pMsg->m_physicalKeyStatus.m_bIsKeyReleased = 1;                                     // always 1 for KEYUP
        }
    }
    else if(uMsg == WM_CHAR || uMsg == WM_DEADCHAR)
    {
        if (pMsgPack->m_wParam)
        {
            pMsg->m_platformKeyCode = static_cast<wsy::VirtualKey>(pMsgPack->m_wParam);
        }

        // Specify the physical key status for WM_CHAR message
        pMsg->m_physicalKeyStatus.m_uiRepeatCount = (pMsgPack->m_lParam & 0x0000FFFF);          // bits 0-15
        pMsg->m_physicalKeyStatus.m_uiScanCode = (pMsgPack->m_lParam & 0x00FF0000) >> 16;       // bits 16-23
        pMsg->m_physicalKeyStatus.m_bIsExtendedKey = (pMsgPack->m_lParam & 0x01000000) >> 24 != 0;   // bit 24
        pMsg->m_physicalKeyStatus.m_bIsMenuKeyDown = (pMsgPack->m_lParam & 0x20000000) >> 29 != 0;   // bit 29
        pMsg->m_physicalKeyStatus.m_bWasKeyDown = (pMsgPack->m_lParam & 0x40000000) >> 30 != 0;      // bit 30
        pMsg->m_physicalKeyStatus.m_bIsKeyReleased = (pMsgPack->m_lParam & 0x80000000) >> 31 != 0;   // bit 31

    }

    switch(uMsg)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        pMsg->m_msgID = XCP_KEYDOWN;
        if (uMsg == WM_SYSKEYDOWN)
        {
            pMsg->m_bIsSecondaryMessage = TRUE;
        }
        break;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        pMsg->m_msgID = XCP_KEYUP;
        if (uMsg == WM_SYSKEYUP)
        {
            pMsg->m_bIsSecondaryMessage = TRUE;
        }
        break;
    case WM_CHAR:
        pMsg->m_msgID = XCP_CHAR;
        break;
    case WM_DEADCHAR:
        pMsg->m_msgID = XCP_DEADCHAR;
        break;
    }
}

HRESULT
CXcpBrowserHost::HandleFocusMessage(_In_ XUINT32 uMsg, _In_ MsgPacket *pMsgPack, _Inout_ InputMessage *pMsg, _Out_ bool &fHandled)
{
    switch(uMsg)
    {
    case WM_SETFOCUS:
        pMsg->m_msgID = XCP_GOTFOCUS;
        break;
    case WM_KILLFOCUS:
        pMsg->m_msgID = XCP_LOSTFOCUS;
        break;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//     This code handles the specific input message by calling into InputManager
//
//------------------------------------------------------------------------
void CXcpBrowserHost::HandleInputMessage(
    _In_ XUINT32 uMsg,
    _In_ MsgPacket *pMsgPack,
    _In_opt_ CContentRoot* contentRoot,
    bool isReplayedMessage,
    _Out_ bool &fHandled)
{
    HRESULT hr = S_OK;
    fHandled = TRUE;

    // passing null message is not ok
    if (!pMsgPack || !m_pcs)
        return;

    // unpack the packet and put it into a platform independent structure
    // Pack the input into a structure that is portable and call the method
    std::shared_ptr<InputMessage> pMsg = std::make_shared<InputMessage>();

    pMsg->m_hWindow = pMsgPack->m_hwnd;
    pMsg->m_hCoreWindow = pMsgPack->m_pCoreWindow;
    pMsg->m_isReplayedMessage = isReplayedMessage;

    pMsg->m_hPlatformPacket = static_cast<XHANDLE>(pMsgPack);

    // Set Modifier Keys
    IFC(gps->GetKeyboardModifiersState(&(pMsg->m_modifierKeys)));

    // First package the message then fill in the message type
    switch (uMsg)
    {
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_CHAR:
    case WM_DEADCHAR:
        HandleKeyboardMessage(uMsg, pMsgPack, pMsg.get(), fHandled);
        break;

    case WM_SETFOCUS:
    case WM_KILLFOCUS:
        IFC(HandleFocusMessage(uMsg, pMsgPack, pMsg.get(), fHandled));
        break;

    case WM_POINTERDOWN:
    case WM_POINTERUPDATE:
    case WM_POINTERUP:
    case WM_POINTERENTER:
    case WM_POINTERLEAVE:
    case WM_POINTERWHEEL:
    case WM_POINTERHWHEEL:
    case WM_POINTERCAPTURECHANGED:
    case WM_POINTERROUTEDAWAY:
    case DM_POINTERHITTEST:
        IFC(HandlePointerMessage(uMsg, pMsgPack, pMsg.get(), contentRoot, fHandled));
        break;

    case WM_ACTIVATE:
        IFC(HandleActivateMessage(uMsg, pMsgPack, pMsg.get(), fHandled));
        break;

    case WM_CONTEXTMENU:
        pMsg->m_msgID = XCP_CONTEXTMENU;
        break;

    case WM_INPUTLANGCHANGE:
        pMsg->m_msgID = XCP_INPUTLANGCHANGE;
        pMsg->m_langID = static_cast<XDWORD>(pMsgPack->m_lParam);
        break;

    case WM_MOVE:
        pMsg->m_msgID = XCP_WINDOWMOVE;
        break;

    case WM_POINTERROUTEDTO: // Don't handle these cases
    case WM_POINTERROUTEDRELEASED:
        fHandled = FALSE;
        goto Cleanup;

    default:
        break;
    }

    // Increment the previous pointer update message Id if this is a message we might want to replay.
    if (!isReplayedMessage && (pMsg->m_msgID == XCP_POINTERUPDATE || pMsg->m_msgID == XCP_POINTERDOWN || pMsg->m_msgID == XCP_POINTERUP))
    {
        m_previousPointerUpdateMsgId++;
    }

    if (m_pcs)
    {
        XINT32 nHandled = fHandled;
        IFC(m_pcs->ProcessInput(pMsg.get(), contentRoot, &nHandled));
        fHandled = !!nHandled;
    }

Cleanup:
    return;
}

//-------------------------------------------------------------------------
//
//  Function:   CXcpBrowserHost::ResetState
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CXcpBrowserHost::ResetState()
{
    HRESULT hr = S_OK;

    // Stop all future download requests. This only sets a flag, so it won't fail.
    (void)StopDownloads();

    // Reset the core
    IFC(ResetCore());

Cleanup :
    RRETURN( hr ) ;
}

//-------------------------------------------------------------------------
//
//  Function:   CXcpBrowserHost::ResetCore
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CXcpBrowserHost::ResetCore()
{
    HRESULT hr = S_OK;

    CDependencyObject *pDependencyObject = NULL;

    TraceCoreServicesResetBegin();

    // Destroy the dispatcher window
    if(m_pDispatcherNoRef)
    {
        m_pDispatcherNoRef->Deinit();
    }

    if (m_pcs)
    {
        // Cleanup the common stuff(abort/stop download and value store)
        CleanupCommon();

        ReleaseInterface(m_pDownloader);

        // Disconnect the core object
        IFC(DetachCore());

        ASSERT(m_pcs == NULL);
        ASSERT(m_pNWRenderTarget == NULL);
    }

    IFC(ObtainCoreServices(gps.Get(), &m_pcs));

    // Let ErrorService know that core has been reset and pass it a pointer to the new core
    if(m_pErrorService)
    {
        m_pErrorService->CoreResetCleanup(m_pcs);
    }

    m_pcs->RegisterHostSite(m_pSite);

    // Create a new compositor scheduler and render target to host the next
    // application loaded into this host.
    IFC(CreateRenderTarget());

    // Register the callback for downloads.  Until the window class is backing it
    // up we don't want the core to call back to this site.
    IFC(m_pcs->RegisterDownloadSite(this));

    // Initialize the downloader.
    IFC(CDownloader::Create(
        reinterpret_cast<CDownloader **>(&m_pDownloader),
        this,
        m_pcs
        ));

    static_cast<CDownloader*>(m_pDownloader)->EnableCrossDomainDownloads();

    IFC(m_pcs->RegisterBrowserHost(static_cast<IXcpBrowserHost*>(this)));

    // Make sure the new core is initialized.
    // Here we don't need to release the dependency object because of passing the empty xaml
    // will ensure the dependency object as NULL.
    IFC(m_pcs->LoadXaml(0, NULL, &pDependencyObject));

    // Restart the dispatcher window
    if(m_pDispatcherNoRef)
    {
        IFC(m_pDispatcherNoRef->Init(m_pSite));
        IFC(m_pDispatcherNoRef->Start());

        if (!m_pUIThreadExecuterQueue)
        {
            IFC(gps->QueueCreate(&m_pUIThreadExecuterQueue));
        }
    }

Cleanup:

    TraceCoreServicesResetEnd();

    RRETURN( hr ) ;
}

CCoreServices*
CXcpBrowserHost::GetContextInterface()
{
    return m_pcs;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Set mouse capture on
//
//-------------------------------------------------------------------------
void
CXcpBrowserHost::SetMouseCapture(HWND newWindow)
{
    SetCapture(newWindow);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Release mouse capture
//
//-------------------------------------------------------------------------

void
CXcpBrowserHost::ReleaseMouseCapture()
{
    // we only honor capture semantics in windowed mode on windows
    ReleaseCapture();
}


#ifdef ENABLE_UIAUTOMATION
//-------------------------------------------------------------------------
//
//  Function:   CXcpBrowserHost::UIAClientsAreListening
//
//  Synopsis:
//      Checks if clients are listening to a specific event
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CXcpBrowserHost::UIAClientsAreListening(_In_ UIAXcp::APAutomationEvents eAutomationEvent)
{
    HRESULT hr = S_OK;
    IFCPTR(m_pSite);

    IFC_NOTRACE(m_pSite->UIAClientsAreListening(eAutomationEvent));
Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CXcpBrowserHost::UIARaiseAutomationEvent
//
//  Synopsis:
//      Raises a UIA Event
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CXcpBrowserHost::UIARaiseAutomationEvent(_In_ CAutomationPeer *pAP,
                                         _In_ UIAXcp::APAutomationEvents eAutomationEvent)
{
    HRESULT hr = S_OK;
    IFCPTR(m_pSite);
    IFC(m_pSite->UIARaiseAutomationEvent(pAP, eAutomationEvent));
Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CXcpBrowserHost::UIARaiseAutomationPropertyChangedEvent
//
//  Synopsis:
//      Raises a UIA Property Changed Event
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CXcpBrowserHost::UIARaiseAutomationPropertyChangedEvent(_In_ CAutomationPeer *pAP,
                                                        _In_ UIAXcp::APAutomationProperties eAutomationProperty,
                                                        _In_ const CValue& oldValue,
                                                        _In_ const CValue& newValue)
{
    HRESULT hr = S_OK;
    IFCPTR(m_pSite);
    IFC(m_pSite->UIARaiseAutomationPropertyChangedEvent(pAP, eAutomationProperty, oldValue, newValue));
Cleanup:
    RRETURN(hr);
}


//-------------------------------------------------------------------------
//
//  Function:   CXcpBrowserHost::UIARaiseFocusChangedEventOnUIAWindow
//
//  Synopsis:
//      Raises a UIA Focus changed Event on CUIAWindow.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CXcpBrowserHost::UIARaiseFocusChangedEventOnUIAWindow()
{
    HRESULT hr = S_OK;
    IFCPTR(m_pSite);
    IFC(m_pSite->UIARaiseFocusChangedEventOnUIAWindow());
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
CXcpBrowserHost::UIARaiseTextEditTextChangedEvent(_In_ CAutomationPeer *pAP,
_In_ UIAXcp::AutomationTextEditChangeType eType,
_In_ CValue *pChange)
{
    IFCPTR_RETURN(m_pSite);
    IFC_RETURN(m_pSite->UIARaiseTextEditTextChangedEvent(pAP, eType, pChange));
    return S_OK;
}

_Check_return_ HRESULT CXcpBrowserHost::UIARaiseNotificationEvent(
    _In_ CAutomationPeer* ap,
    UIAXcp::AutomationNotificationKind notificationKind,
    UIAXcp::AutomationNotificationProcessing notificationProcessing,
    _In_opt_ xstring_ptr displayString,
    _In_ xstring_ptr activityId)
{
    IFCPTR_RETURN(m_pSite);
    IFC_RETURN(m_pSite->UIARaiseNotificationEvent(ap, notificationKind, notificationProcessing, displayString, activityId));
    return S_OK;
}

#endif

//------------------------------------------------------------------------
//
//  Synopsis:
//      Paint message handling for window invalidation.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CXcpBrowserHost::OnPaint()
{
    if (!m_bInit)
    {
        return S_OK;
    }

    // Explicitly poll for device lost here.  We may have no rendering work to do, and
    // just received a WM_PAINT from the DWM after losing its D3D device.  DWM expects the framework
    // to poll for device lost and re-create resources in this situation.
    bool deviceLost;
    IFC_RETURN(m_pcs->DetermineDeviceLost(&deviceLost));
    if (deviceLost)
    {
        return S_OK;
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Loads a string resource from the given module based on the user default
//      UI language.
//
//  Notes:
//      FindResourceExWithFallback is used to locate the string block.
//      If the string is not found, the function succeeds and *pFoundString is FALSE.
//
//---------------------------------------------------------------------------
_Check_return_
HRESULT CXcpBrowserHost::LoadStringResource(
    _In_ HINSTANCE module,
    _In_ XUINT32 stringId,
    _Inout_ XStringBuilder& strBuilder,
    _Out_ bool *pFoundString
    )
{
    HRESULT hr = S_OK;
    LANGID langId = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
    //strings are packed in blocks of 16
    LPTSTR block = MAKEINTRESOURCE(stringId / 16 + 1);
    int offset = stringId % 16;
    HRSRC hResInfo = FindResourceExWithFallback(module, RT_STRING, block, langId);

    *pFoundString = FALSE;

    if (hResInfo)
    {
        WCHAR *strBlock = static_cast<WCHAR*>(LoadResource(module, hResInfo));
        if (strBlock)
        {
            // Find the string. First character of each string is
            // the string length.
            WCHAR *curr = strBlock;
            int i = 0;
            while(i++ < offset)
            {
                // Skip to next string
                curr += (*curr + 1);
            }

            int len = *curr;
            curr++;

            // Copy string
            IFC(strBuilder.Append(curr, len));
            *pFoundString = TRUE;
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT CXcpBrowserHost::GetActualWidth(_Out_ XUINT32* pValue)
{
    IFCEXPECT_RETURN(m_pNWRenderTarget);
    *pValue = m_pNWRenderTarget->GetWidth();
    return S_OK;
}

_Check_return_ HRESULT CXcpBrowserHost::GetActualHeight(_Out_ XUINT32* pValue)
{
    IFCEXPECT_RETURN(m_pNWRenderTarget);
    *pValue = m_pNWRenderTarget->GetHeight();
    return S_OK;
}

_Check_return_ HRESULT CXcpBrowserHost::SetWindowSize(XSIZE size, _In_opt_ XHANDLE hwnd)
{
    m_previousWidth = size.Width;
    m_previousHeight = size.Height;

    IFC_RETURN(EnsureCorrectWindowSize(m_widthOverride != 0 ? m_widthOverride : m_previousWidth,
                                       m_heightOverride != 0 ? m_heightOverride : m_previousHeight,
                                       hwnd));
    return S_OK;
}

// Forces the window to a certain size. Called by the test framework. We accept an hwnd
// now because it's possible that the current target doesn't have a target window
// after the core was shutdown.
_Check_return_ HRESULT
CXcpBrowserHost::SetWindowSizeOverride(
    _In_ const XSIZE *pWindowSize,
    _In_ XHANDLE hwnd
    )
{
    // Only used for testing. Calling when uninitialized is unsupported.
    XCP_FAULT_ON_FAILURE(m_bInit);

    m_widthOverride = static_cast<XUINT32>(pWindowSize->Width);
    m_heightOverride = static_cast<XUINT32>(pWindowSize->Height);

    IFC_RETURN(EnsureCorrectWindowSize(m_widthOverride != 0 ? m_widthOverride : m_previousWidth,
                                       m_heightOverride != 0 ? m_heightOverride : m_previousHeight,
                                       hwnd));
    return S_OK;
}

void CXcpBrowserHost::RequestReplayPreviousPointerUpdate()
{
    // Pass the current cached pointer update message along with the replay request. When we process the request, we'll
    // use the pointer in the param to verify that the cached pointer update message is still the most up-to-date one.
    // If it isn't, we'll ignore the request and no-op.
    IFCFAILFAST(m_pDispatcherNoRef->QueueDeferredInvoke(
        WM_REPLAY_PREVIOUS_POINTERUPDATE,
        static_cast<WPARAM>(m_previousPointerUpdateMsgId),
        0
        ));
}

void CXcpBrowserHost::ReplayPreviousPointerUpdate(UINT32 previousPointerUpdateMsgId)
{
    if (m_previousPointerUpdateMsgId == previousPointerUpdateMsgId)
    {
        m_pcs->ReplayPreviousPointerUpdate();
    }
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Ensures that a present target of the correct size exists.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CXcpBrowserHost::EnsureCorrectWindowSize(
    unsigned int width,
    unsigned int height,
    _In_opt_ XHANDLE hwnd)
{
    // If we don't have a render target, just return.  When the render target is created this will get called again.
    if (m_pNWRenderTarget == nullptr)
    {
        return S_OK;
    }

    bool requestTick = false;
    if (m_pcs != NULL)
    {
        IFC_RETURN(m_pcs->DetermineDeviceLost(nullptr));
    }

    // TODO http://osgvsowi/14391361
    // XamlIslandRoots: Moving/sizing the CoreWindow shouldn't affect XamlIslandRoots-only scenarios

    // This code avoids unnecessary recreation of the present target.
    const auto& presentTarget = m_pNWRenderTarget->GetPresentTarget();
    if (presentTarget == nullptr || hwnd != presentTarget->GetTargetWindowHandle())
    {
        xref_ptr<WindowsPresentTarget> windowsPresentTarget;
        IFC_RETURN(WindowsPresentTarget::CreateWindowedPresentTarget(
            width,
            height,
            hwnd,
            windowsPresentTarget.ReleaseAndGetAddressOf()));
        m_pNWRenderTarget->Retarget(windowsPresentTarget.get());
        requestTick = true;

        TraceWindowSizeChangedInfo(width, height);
    }
    else if (width != m_pNWRenderTarget->GetWidth()
          || height != m_pNWRenderTarget->GetHeight())
    {
        IFC_RETURN(presentTarget->SetWidth(width));
        IFC_RETURN(presentTarget->SetHeight(height));

        if (GetContextInterface()->GetInitializationType() != InitializationType::IslandsOnly)
        {
            requestTick = true;
        }
        else
        {
            // If we don't have XAML content bound to the CoreWindow, there's no reason to schedule
            // a new tick.
            requestTick = false;
        }

        TraceWindowSizeChangedInfo(width, height);
    }

    if (requestTick)
    {
        // Request a tick to respond to present target size changing. The new layout pass at the new window size will
        // dirty any elements that need to be re-rendered.
        IFrameScheduler *pScheduler = GetFrameScheduler();
        if (pScheduler != NULL)
        {
            IFC_RETURN(pScheduler->RequestAdditionalFrame(0 /*immediate*/, RequestFrameReason::WindowSize));
        }
    }

    return S_OK;
}

// Requests a tick to forcibly redraw the entire tree
void CXcpBrowserHost::ForceRedraw()
{
    IFrameScheduler* pScheduler = GetFrameScheduler();
    if (pScheduler)
    {
        m_fForceRedraw = true;
        IFCFAILFAST(pScheduler->RequestAdditionalFrame(0 /*immediate*/, RequestFrameReason::Paint));
    }
}
