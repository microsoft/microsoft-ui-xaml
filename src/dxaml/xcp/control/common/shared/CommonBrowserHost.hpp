// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "corep.h"
#include "EventArgs.h"
#include <ParserAPI.h>
#include <WindowsGraphicsDeviceManager.h>
#include <WindowRenderTarget.h>
#include <CompositorScheduler.h>

CEventInfo::~CEventInfo()
{
    Handler = nullptr;
}

CEventInfo::CEventInfo(
        _In_opt_ CDependencyObject *pListener,
        _In_     EventHandle hEvent,
        _In_opt_ CDependencyObject *pSender,
        _In_opt_ CEventArgs *pArgs,
        _In_ XINT32 flags,
        _In_opt_ INTERNAL_EVENT_HANDLER pInternalHandler)
    : Flags(flags)
    , Event(hEvent)
    , Sender(pSender)
    , Args(pArgs)
{
    // If there is script object, function name, or flags we want to keep
    // the event information.  Otherwise there is nothing worth sending.
    if (pListener)
    {
        Listener = pListener;
    }
    else if (pInternalHandler)
    {
        // The handler is a static method pointer to internal code.
        Handler = pInternalHandler;
    }
    else if (!flags)
    {
        ASSERT(false);
        IsValid = false;
    }
}

CEventInfo::CEventInfo(const CEventInfo& rhs)
{
    Listener = rhs.Listener;
    Event = rhs.Event;
    IsValid = rhs.IsValid;
    Flags = rhs.Flags;
    Sender = rhs.Sender;
    Args = rhs.Args;
    Handler = rhs.Handler;
}


//-------------------------------------------------------------------------
//
//  Function:   CommonBrowserHost::AddRef
//
//  Synopsis:
//     Increments usage counter
//
//-------------------------------------------------------------------------
XUINT32
    CommonBrowserHost::AddRef()
{
    return ++m_cRef;
}

//-------------------------------------------------------------------------
//
//  Function:   CommonBrowserHost::Release
//
//  Synopsis:
//     Decrements usage counter
//
//-------------------------------------------------------------------------
XUINT32
    CommonBrowserHost::Release()
{
    XUINT32 cRef = --m_cRef;

    if (!cRef)
        delete this;
    return cRef;
}


//-------------------------------------------------------------------------
//
//  Synopsis:
//     Perform browser host initialization steps common to all platforms.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CommonBrowserHost::Init(
    _In_ IXcpHostSite* pSite,
    _In_ IXcpDispatcher* pDispatcher)
{
    HRESULT hr = S_OK;

    IFCPTR(pDispatcher);

    ASSERT(m_pDispatcherNoRef == NULL);

    m_pSite = pSite;
    m_pDispatcherNoRef = pDispatcher;

    if (m_pUIThreadExecuterQueue)
    {
        IPALExecuteOnUIThread* pExecuter = NULL;
        while (m_pUIThreadExecuterQueue->Get((void**)&pExecuter, 0) == S_OK)
        {
            ReleaseInterface(pExecuter);
        }
        IGNOREHR(m_pUIThreadExecuterQueue->Close());
        m_pUIThreadExecuterQueue = NULL;
    }

    IFC(gps->QueueCreate(&m_pUIThreadExecuterQueue));

    // Obtain the core services object.
    ReleaseInterface(m_pcs);

    IFC(ObtainCoreServices(gps.Get(), &m_pcs));

    m_pcs->RegisterHostSite(pSite);

    IFC(CreateRenderTarget());

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Create the primary CWindowRenderTarget and the CompositorScheduler
//     which will drive the render target's compositor if it uses GPU
//     acceleration.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CommonBrowserHost::CreateRenderTarget()
{
    HRESULT hr = S_OK;

    // Any old scheduler or render targets should have already been
    // cleaned up before we get here.
    ASSERT(m_pNWCompositorScheduler == NULL);
    ASSERT(m_pUIThreadScheduler == NULL);
    ASSERT(m_pNWRenderTarget == NULL);

    // Create the graphics device manager -- it will be specific to this host
    // to allow graphics device sharing within a single plugin instance.
    ASSERT(m_graphicsDeviceManager == nullptr);

    IFC(gps->CreateGraphicsDeviceManager(m_graphicsDeviceManager.ReleaseAndGetAddressOf()));

    IFC(m_pcs->CreateCompositorScheduler(
        m_graphicsDeviceManager.get(),
        &m_pNWCompositorScheduler));

    m_graphicsDeviceManager->SetCompositorScheduler(m_pNWCompositorScheduler);

    m_pcs->CreateUIThreadScheduler(
        m_pDispatcherNoRef,
        m_pNWCompositorScheduler,
        &m_pUIThreadScheduler);

    IFC(m_pcs->CreateWindowRenderTarget(
        m_graphicsDeviceManager.get(),
        m_pNWCompositorScheduler,
        NULL /*pIInitialPresentTarget*/));

    SetInterface(m_pNWRenderTarget, m_pcs->NWGetWindowRenderTarget());
    m_graphicsDeviceManager->SetRenderTarget(m_pNWRenderTarget);

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Release the primary CWindowRenderTarget, and shut down and release
//     the CompositorScheduler. These objects should be re-created by
//     calling CreateRenderTarget if this cleanup is running when
//     switching out XAPs running in the same host.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CommonBrowserHost::CleanupRenderTarget()
{
    HRESULT hr = S_OK;

    if (m_pUIThreadScheduler)
    {
        ReleaseInterface(m_pUIThreadScheduler);
    }

    if (m_pNWCompositorScheduler)
    {
        IFC(m_pNWCompositorScheduler->Shutdown());
        ReleaseInterface(m_pNWCompositorScheduler);
    }

    // Must reset graphics device manager after releasing the render target
    // This has to be the last release of the render target before we can reset
    // the graphics device manager.
    // Make sure to not leave the graphics device manager with the dangling render target pointer.
    if (m_graphicsDeviceManager != nullptr)
    {
        m_graphicsDeviceManager->SetRenderTarget(nullptr);
    }
    ReleaseInterface(m_pNWRenderTarget);
    m_graphicsDeviceManager.reset();


Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CommonBrowserHost::CleanupCommon
//
//  Synopsis:
//     Cleanup the common stuff
//
//-------------------------------------------------------------------------
void
CommonBrowserHost::CleanupCommon()
{
    (void) StopDownloads();

    if (m_pUIThreadExecuterQueue)
    {
        PurgeThreadMessages();
        IGNOREHR(m_pUIThreadExecuterQueue->Close());
        m_pUIThreadExecuterQueue = NULL;
    }
}

void
CommonBrowserHost::CleanupErrorService()
{
    ReleaseInterface(m_pErrorService);
}

void
CommonBrowserHost::ResetDownloader()
{
    if (m_pDownloader)
    {
        m_pDownloader->ResetDownloader();
    }
}

// Remove any messages that are in the m_pUIThreadExecuterQueue
void
CommonBrowserHost::PurgeThreadMessages()
{
    if (m_pUIThreadExecuterQueue)
    {
        IPALExecuteOnUIThread* pExecuter = NULL;
        while (m_pUIThreadExecuterQueue->Get((void**)&pExecuter, 0) == S_OK)
        {
            ReleaseInterface(pExecuter);
        }
    }
}

CommonBrowserHost::CommonBrowserHost()
{
    m_cRef = 1;
    XCP_WEAK(&m_pDispatcherNoRef);
    m_pDispatcherNoRef = NULL;
    XCP_WEAK(&m_pSite);
    m_pSite = NULL;
    XCP_STRONG(&m_pDownloader);
    m_pDownloader = NULL;

    m_pNWCompositorScheduler = NULL;
    m_pUIThreadScheduler = NULL;
    m_pNWRenderTarget = NULL;

    XCP_STRONG(&m_pcs);
    m_pcs = NULL;
    m_pErrorService = NULL;
    m_pControl = NULL;

    m_bInResetVisualTree = false;

    m_pUIThreadExecuterQueue = NULL;
}

//-------------------------------------------------------------------------
//
//  Function:   CommonBrowserHost::~CommonBrowserHost
//
//  Synopsis:   destructor
//     Cleanup
//
//-------------------------------------------------------------------------

CommonBrowserHost::~CommonBrowserHost()
{
    // Clear explicitly - the ordering of operations during shutdown has caused problems in the past, so we're keep the order
    // the same after switching from raw pointers to xtrf_ptr.
    m_graphicsDeviceManager.reset();

    // The compositor should have been stopped when we were told of shutdown
    // See comments in CCoreServices::CLR_Shutdown
    ASSERT(m_pUIThreadScheduler == NULL);
    XCP_FAULT_ON_FAILURE(!m_pNWCompositorScheduler);

    ReleaseInterface(m_pNWRenderTarget);

    ReleaseInterface(m_pDownloader);

    // Disconnect the core object
    IGNOREHR(DetachCore());
    ASSERT(m_pcs == NULL);

    ReleaseInterface(m_pErrorService);

    m_pSite = NULL;
    m_pDispatcherNoRef = NULL;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Gets whether or not the frame rate counter is enabled
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CommonBrowserHost::get_EnableFrameRateCounter(_Out_ bool *pIsEnabled)
{
    HRESULT hr = S_OK;

    IFCPTR(pIsEnabled);
    IFCPTR(m_pSite);

    *pIsEnabled = m_pSite->GetEnableFrameRateCounter();

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Set whether or not the frame rate counter is enabled
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CommonBrowserHost::set_EnableFrameRateCounter(bool isEnabled)
{
    HRESULT hr = S_OK;

    IFCPTR(m_pSite);

    IFC(m_pSite->SetEnableFrameRateCounter(isEnabled));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CommonBrowserHost::GetSystemGlyphTypefaces
//
//  Synopsis:
//-------------------------------------------------------------------------
_Check_return_ HRESULT
    CommonBrowserHost:: GetSystemGlyphTypefaces(
        _Outptr_ CDependencyObject*  *ppDo)
{
    HRESULT hr = S_OK;

    if (m_pcs)
        return m_pcs->GetSystemGlyphTypefaces(ppDo);

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CommonBrowserHost::CreateFromXaml
//
//  Synopsis:
//   Wrapper of the core function ParseXaml
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CommonBrowserHost::CreateFromXaml(
        _In_ XUINT32 cXaml,
        _In_reads_(cXaml) const WCHAR *pXaml,
        _In_ bool bCreateNamescope,
        _In_ bool bRequiresDefaultNamespace,
        _In_ bool bExpandTemplatesDuringParse,
        _Outptr_ CDependencyObject **ppIDO)
{
    HRESULT hr = E_FAIL;

    if (m_pcs)
    {
        // We know this XAML came from a string, so we can safely set bForceUtf16=TRUE.  Setting
        // that ensures that if the string contains <?xml encoding="..."?> then the encoding in
        // the string will be ignored rather than cause a parse failure.  Using bForceutf16=TRUE
        // ensures compatibility with Win8, where an incorrect encoding specified in the markup
        // to XamlReader.Load(str) would be ignored.
        Parser::XamlBuffer buffer;
        buffer.m_count = cXaml * sizeof(WCHAR);
        buffer.m_buffer = reinterpret_cast<const XUINT8*>(pXaml);
        buffer.m_bufferType = Parser::XamlBufferType::Text;
        hr = m_pcs->ParseXaml(
            buffer,
            true/*bForceUtf16*/,
            bCreateNamescope,
            bRequiresDefaultNamespace,
            ppIDO,
            xstring_ptr(),
            bExpandTemplatesDuringParse);
        if(FAILED(hr))
        {
            IErrorService *pErrorService = nullptr;
            VERIFYHR(m_pcs->getErrorService(&pErrorService));
            if(pErrorService)
            {
                IError *pError = nullptr;
                VERIFYHR(pErrorService->GetFirstError(&pError));
                if(pError)
                {
                    pError->SetIsRecoverable(1);
                }
            }
        }
    }


// WinBrowserHost has
//        return m_pcs->ParseXaml(WCHARCOUNT_TO_PALURIINPUTCOUNT( cXaml ), (XUINT8*) pXaml, bCreateNamescope, ppIDO);

    RRETURN(hr);

}

//-------------------------------------------------------------------------
//
//  Member:
//      CommonBrowserHost::DetachCore
//
//  Synopsis:
//      This is the method that browser host implementations use
//      in order to ensure that core services are properly disposed
//      of. This includes shutting down CLR, letting the render targets
//      go and releasing the final reference to the CCoreServices object
//      itself.
//
//-------------------------------------------------------------------------

_Check_return_ HRESULT
CommonBrowserHost::DetachCore()
{
    HRESULT hr = S_OK;

    XINT32 bReentrancyAllowed;

    // Don't allow core to be detached on a synchronous callout because
    // current implementation is not robust enough for that. Null AV
    // to halt execution. This is to handle the case where the Silverlight
    // control is removed from the browser's HTML tree by script.
    IFC(IsReentrancyAllowed(&bReentrancyAllowed));
    XCP_FAULT_ON_FAILURE(bReentrancyAllowed);

    if (m_pcs != NULL)
    {
        // Make sure that we break any cyclical dependencies between core
        // services and the render target, which might still be referenced
        // by outstanding vector buffers.
        IFC(ResetVisualTree());

        m_pcs->UnregisterHostSite();
        m_pcs->UnregisterDownloadSite();

        // null out the reference that core holds to us while detaching.
        // This reference will be set by calling RegisterBrowserHost in both
        // BH::Init and BH::ResetCore
        IFC(m_pcs->UnregisterBrowserHost());

        // RS1 bug #7300521
        // There are several queued CompositorTreeCommands, 'keep alives' and 'removes',
        // which aren't being processed when the view is closed due to an expected final frame.
        // The 'keep alives' need to be processed or cleared to release the final reference
        // on elements that have a composition peer pending removal. These elements may hold a reference
        // on core and cause it to leak.  Process the queue here before releasing the final reference
        // on core. (These commands are queued in CUIElement::RemoveCompositionPeer.)
        m_pcs->ClearCompositorTreeHostQueues();

        // RS1 bug #8775369
        // Resource manager and memory manager events hold on to the apartment and
        // increment Git Pending Registration count. Release them earlier so that
        // if CCoreServices object leaks it will not cause the entire apartment to leak.
        m_pcs->DetachApartmentEvents();
    }

    if (m_pErrorService != NULL)
    {
        m_pErrorService->CleanupErrors();
    }

    // We would like this to be the last outstanding reference to core
    // services, but the API wrappers could still be holding on to it.
    ReleaseInterface(m_pcs);

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CommonBrowserHost::GetRootVisual
//
//  Synopsis:
//   Wrapper of the core function GetRootVisual
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CommonBrowserHost::GetRootVisual(
        _Outptr_ CDependencyObject **ppIDO)
{
    HRESULT hr = E_FAIL;

    IFCPTR(ppIDO);

    if (m_pcs)
    {
        *ppIDO = m_pcs->getVisualRoot();
        hr = S_OK;
    }
Cleanup:
    RRETURN(hr);
}

_Check_return_ CDependencyObject*
CommonBrowserHost::GetPublicOrFullScreenRootVisual()
{
    if (m_pcs)
    {
        return m_pcs->GetPublicOrFullScreenRootVisual();
    }

    return nullptr;
}

//-------------------------------------------------------------------------
//
//  Function:   CommonBrowserHost::ResetVisualTree
//
//  Synopsis:
//   Wrapper of the core function ResetVisualTree
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CommonBrowserHost::ResetVisualTree()
{
    HRESULT hr = E_FAIL;
    bool bPrevInResetVisualTree = m_bInResetVisualTree;
    m_bInResetVisualTree = true;

    // Tear down old render target and compositor scheduler.
    IFC(CleanupRenderTarget());

    if (m_pcs)
    {
        IFC(m_pcs->ResetCoreWindowVisualTree());
    }

Cleanup:
    m_bInResetVisualTree = bPrevInResetVisualTree;
    RRETURN(hr);
}

_Check_return_ bool CommonBrowserHost::IsShuttingDown()
{
    if (m_pcs)
    {
        return m_pcs->IsShuttingDown();
    }
    return false;
}

void CommonBrowserHost::SetShuttingDown(bool isShuttingDown)
{
    if (m_pcs)
    {
        m_pcs->SetShuttingDown(isShuttingDown);
    }
}

//-------------------------------------------------------------------------
//
//  Function:   CommonBrowserHost::StopDownloads
//
//  Synopsis:
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
    CommonBrowserHost::StopDownloads()
{
    HRESULT hr = S_OK;

    if (m_pDownloader)
    {
        hr = m_pDownloader->StopAllDownloads();
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Function:   CommonBrowserHost::GetErrorService
//
//  Synopsis:
//         Pass the Error service to control level's code to handle errors.
//         the Script plugin might need this to save detail error information
//         and generate error message fromt the error service.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CommonBrowserHost::GetErrorService(_Out_ IErrorService **ppErrorService)
{
    HRESULT hr = S_OK;

    if (ppErrorService == NULL)
    {
        IFC(E_INVALIDARG);
    }

    //
    // Create only one instance per CommonBrowserHost.
    //
    if (m_pErrorService == NULL && m_pcs != NULL)
    {
        IFC(m_pcs->CreateErrorService(&m_pErrorService));
    }

    *ppErrorService = m_pErrorService;

Cleanup:

    RRETURN(hr);
}


//-------------------------------------------------------------------------
//
//  Function:   CommonBrowserHost::CLR_FireEvent
//
//  Synopsis:
//
//
//-------------------------------------------------------------------------
HRESULT
    CommonBrowserHost::CLR_FireEvent(
        _In_ CDependencyObject* pListener,
        _In_ EventHandle hEvent,
        _In_ CDependencyObject* pSender,
        _In_ CEventArgs* pArgs,
        _In_ XUINT32 flags)

{
    HRESULT hr = S_OK;

    IFC(m_pcs->CLR_FireEvent(pListener, hEvent, pSender, pArgs, flags));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CommonBrowserHost::ShouldFireEvent
//
//  Synopsis:
//
//
//-------------------------------------------------------------------------
HRESULT
    CommonBrowserHost::ShouldFireEvent(
        _In_ CDependencyObject *pListener,
        _In_ EventHandle hEvent,
        _In_ CDependencyObject* pSender,
        _In_ CEventArgs* pArgs,
        _In_ XINT32 flags,
        _Out_ XINT32* pfShouldFire)
{
    HRESULT hr = S_OK;

    IFC(m_pcs->ShouldFireEvent(pListener, hEvent, pSender, pArgs, flags, pfShouldFire));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CommonBrowserHost::UnsecureDownload
//
//  Synopsis:
//
// This member is used for cases where there are legitimate exceptions
// to site-of-origin download restriction.  If you make a call to this
// member, comment the call as to why the exception is valid and secure.
//-------------------------------------------------------------------------
_Check_return_ HRESULT
    CommonBrowserHost::UnsecureDownload(
                                  _In_ IPALDownloadRequest                  *pDownloadRequest,
                                  _Outptr_opt_ IPALAbortableOperation    **ppIAbortableDownload,
                                  _In_opt_ IPALUri                          *pPreferredBaseUri)
{
    HRESULT hr = S_OK;

    IFC( EnsureDownloader() );
    IFC( m_pDownloader->CreateUnsecureDownloadRequest(
                                               pDownloadRequest,
                                               ppIAbortableDownload,
                                               pPreferredBaseUri));
Cleanup:
    return(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CommonBrowserHost::CheckUri
//
//  Synopsis:
//
//     Validate the Uri for security.
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CommonBrowserHost::CheckUri(
        _In_ const xstring_ptr&     theRelativeUri,
        _In_ XUINT32 eUnsecureDownloadAction,
        _Out_opt_ XINT32 *pfShouldSuppressCookies)
{
    HRESULT hr = S_OK;

    IFC( EnsureDownloader() );
    IFC( m_pDownloader->CheckUri(theRelativeUri, eUnsecureDownloadAction, pfShouldSuppressCookies) );

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function: CommonBrowserHost::CheckUri
//
//  Synopsis: Checks an absolute URI for security.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CommonBrowserHost::CheckUri(
        _In_ IPALUri* pUri,
        _In_ XUINT32 eUnsecureDownloadAction,
        _Out_opt_ XINT32 *pfShouldSuppressCookies)
{
    HRESULT hr = S_OK;

    IFC( EnsureDownloader() );
    IFC( m_pDownloader->CheckUri(pUri, eUnsecureDownloadAction, pfShouldSuppressCookies) );

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CommonBrowserHost::EnsureDownloader
//
//  Synopsis:
//
//-------------------------------------------------------------------------
HRESULT
CommonBrowserHost::EnsureDownloader()
{
    HRESULT hr = S_OK;

    if (!m_pDispatcherNoRef)
    {
        IFC(E_FAIL);
    }

    if (m_pDownloader == NULL)
    {
        IFC( CDownloader::Create(   reinterpret_cast<CDownloader**>(&m_pDownloader),
                                    static_cast<IDownloaderSite*>(this),
                                    m_pcs));
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CommonBrowserHost::get_BaseUri
//
//  Synopsis:   Returns the base URI from the control. The returned
//              IPALUri is not AddRef'd.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
    CommonBrowserHost::get_BaseUri(_Out_ IPALUri** ppBaseUri)
{
    HRESULT hr = S_OK;

    IFCPTR(ppBaseUri);
    IFCPTR(m_pSite);

    *ppBaseUri = m_pSite->GetBaseUri();

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CommonBrowserHost::get_SecurityUri
//
//  Synopsis:   Returns the security URI from the control. The returned
//              IPALUri is not AddRef'd.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
    CommonBrowserHost::get_SecurityUri(_Out_ IPALUri** ppSecurityUri)
{
    HRESULT hr = S_OK;

    IFCPTR(ppSecurityUri);
    IFCPTR(m_pSite);

    *ppSecurityUri = m_pSite->GetSecurityUri();

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CommonBrowserHost::GetDownloaderSiteBaseUri
//
//  Synopsis:   Part of the implementation of IDownloaderSite.
//              Returns the base URI from the control. The returned
//              IPALUri is not AddRef'd.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CommonBrowserHost::GetDownloaderSiteBaseUri(_Out_ IPALUri** ppBaseUri)
{
    return get_BaseUri(ppBaseUri);
}

//-------------------------------------------------------------------------
//
//  Function:   CommonBrowserHost::GetDownloaderSiteSecurityUri
//
//  Synopsis:   Part of the implementation of IDownloaderSite.
//              Returns the security URI from the control. The returned
//              IPALUri is not AddRef'd.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CommonBrowserHost::GetDownloaderSiteSecurityUri(_Out_ IPALUri** ppSecurityUri)
{
    return get_SecurityUri(ppSecurityUri);
}

//-------------------------------------------------------------------------
//
//  Function:   CommonBrowserHost::IsNetworkingUnrestricted
//
//  Synopsis:   Returns TRUE if networking is unrestricted
//              (elevated OOB for instance)
//
//-------------------------------------------------------------------------
bool CommonBrowserHost::IsNetworkingUnrestricted()
{
    return false;
}

CCoreServices*
CommonBrowserHost::GetContextInterface()
{
    return m_pcs;
}

//------------------------------------------------------------------------
//
//  Function:   CommonBrowserHost::IsReentrancyAllowed
//
//  Synopsis:
//      Is reentrancy allowed into the Silverlight control?
//
//------------------------------------------------------------------------
_Check_return_
HRESULT CommonBrowserHost::IsReentrancyAllowed(
    _Out_ XINT32 *pbReentrancyAllowed)
{
    HRESULT hr = S_OK;

    *pbReentrancyAllowed = TRUE;

    // Dispatcher may not be available.
    if (m_pDispatcherNoRef)
    {
        IFC(m_pDispatcherNoRef->IsReentrancyAllowed(pbReentrancyAllowed));
    }

    // Reentrancy not allowed while in synchronous callout.

    // Application.Exit is synchronously fired when Visual tree
    // is reset. Don't allow reentrancy if in that synchronous
    // callout.
    if (*pbReentrancyAllowed)
    {
        *pbReentrancyAllowed = !m_bInResetVisualTree;
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   CommonBrowserHost::ProcessExecuteMessage
//  Synopsis:
//      Process a pending item in the executer queue in response to a
//      posted EXECUTE_ON_UI_THREAD message
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CommonBrowserHost::ProcessExecuteMessage()
{
    HRESULT hr = S_OK;

    IPALExecuteOnUIThread* pExecuter = NULL;

    IFCEXPECT_ASSERT(m_pUIThreadExecuterQueue);

    if (m_pUIThreadExecuterQueue->Get((void**)&pExecuter, 0) == S_OK)
    {
        IGNOREHR(pExecuter->Execute());
        ReleaseInterface(pExecuter);
        // if the Execute call is reentrant, then the queue could have
        // been flushed and deleted during the call.  If this happens,
        // exit the loop.
        if (m_pUIThreadExecuterQueue == NULL)
        {
            goto Cleanup;
        }
    }

Cleanup:
    ReleaseInterface(pExecuter);
    RRETURN(hr);
}

