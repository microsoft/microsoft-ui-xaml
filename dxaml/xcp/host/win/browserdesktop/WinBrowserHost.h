// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

extern EncodedPtr<IPlatformServices> gps;

#define DEFAULT_BACKGROUND_COLOR L"white"
#define AGFULLSCREENWINDOW_CLASS L"FullScreenWinClass"

struct IPALSurface;

// CXcpBrowserHost
class CXcpBrowserHost final :
    public CommonBrowserHost,
    public IAsyncDownloadRequestManagerSite
{
public:
    static _Check_return_ HRESULT
        Create(
        _In_ IXcpHostSite *pSite,
        _In_ IXcpDispatcher * pWin,
        _Out_ IXcpBrowserHost ** ppHost);

    CXcpBrowserHost();

    ~CXcpBrowserHost() override;

    void FreeResourceLibraries() override;

public:

    void Deinit() override;

    _Check_return_ HRESULT OnPaint() override;

    void OnDisplayChange() override
    {
        // Immediately after a TDR render thread should try to
        // Present at least once, so that it could fail and trigger the device
        // recovery logic. This does not happen if the app is not
        // actively drawing and hence would result in showing up a blank screen.
        // We fix this by listening to StateChanged on the
        // CXamlIslandRoot's IContentWindow and forcing
        // a redraw on next WM_PAINT.
        m_forceRedrawOnPaint = TRUE;
    }

    void ForceRedraw() override;

    _Check_return_ HRESULT OnTick() override;

    _Check_return_ HRESULT put_EmptySource(_In_ bool firstLoad) override;

    _Check_return_ HRESULT ResetCore() override;

    _Check_return_ HRESULT ResetState() override;

    void HandleInputMessage(
        _In_ XUINT32 uMsg,
        _In_ MsgPacket* pMsgPack,
        _In_opt_ CContentRoot* contentRoot,
        bool isReplayedMessage,
        _Out_ bool& fHandled) final;

    void SetMouseCapture(HWND newWindow) override;

    void ReleaseMouseCapture() override;

    //  IHalServices

    HRESULT _Check_return_
        CreateDownloadRequest(
        _In_ IDownloader*                           pDownloader,
        _In_ IPALUri*                               pAbsoluteUri,
        _In_ IPALDownloadRequest*                   pRequestInfo,
        _In_ XINT32                                 fCrossDomain,
        _In_ XINT32                                 fRemoveSecurityLock,
        _Outptr_result_maybenull_ IPALAbortableOperation**    ppIAbortableDownload) override;

    HRESULT GetServiceProvider(
        IServiceProvider**                          ppStream);

    // IAsyncDownloadRequestManagerSite
    _Check_return_ HRESULT ProcessAsyncDownloadRequest(
        _In_ IDownloader*                           pDownloader,
        _In_ IPALUri*                               pUriAbsolute,
        _In_ IPALDownloadResponseCallback*          pCallback,
        _In_ XINT32                                 fCrossDomain,
        _In_ XINT32                                 fRemoveHttpsLock,
        _Outptr_result_maybenull_ IPALAbortableOperation**    ppIAbortableDownload,
        _In_ XUINT32                                eUnsecureDownloadAction) override;

    _Check_return_ HRESULT StartAsyncDownloadTrigger() override;

    void AbortAsync(_In_ IPALAbortableOperation *abortable) override;
    static _Check_return_ HRESULT AbortAsyncCallback(_In_ IObject *data);

    // call back to script for event handling
    static HRESULT
        PostAsyncScriptCallbackRequest(
        _In_ void*                                  pVoidBH,
        _In_ CDependencyObject*                     pListener,
        _In_ EventHandle                            hEvent,
        _In_ CDependencyObject*                     pSender,
        _In_opt_ CEventArgs*                        pArgs,
        _In_ XINT32                                 flags,
        _In_opt_ IScriptObject*                     pScriptObject,
        _In_opt_ INTERNAL_EVENT_HANDLER             pHandler);

    static HRESULT
        SyncScriptCallbackRequest(
        _In_ void*                                  pVoidBH,
        _In_ CDependencyObject*                     pListener,
        _In_ EventHandle                            hEvent,
        _In_ CDependencyObject*                     pSender,
        _In_opt_ CEventArgs*                        pArgs,
        _In_ XINT32                                 flags,
        _In_opt_ IScriptObject*                     pScriptObject,
        _In_opt_ INTERNAL_EVENT_HANDLER             pHandler);

    _Check_return_ HRESULT put_Source(
        _In_ XUINT32                                cstr,
        _In_reads_(cstr) XUINT8*                    pstr,
        _In_opt_ IPALMemory*                        pPalMemory = NULL ) override;

    // Report Error from Core.
    void ReportError() override;

    void FireApplicationStartupEventComplete() override;

    void FireForceGCCollectEvent() override;

    void ApplicationStartupEventComplete() override;

    void FireApplicationLoadComplete() override;

    CCoreServices* GetContextInterface() override;

    //localized resources services
    //Get a localized resource string
    _Check_return_ HRESULT GetLocalizedResourceString(_In_ XUINT32 stringId, _Out_ xstring_ptr* pstrResourceString) override;
    _Check_return_ HRESULT GetNonLocalizedResourceString(_In_ XUINT32 stringId, _Out_ xstring_ptr* pstrResourceString) override;
    _Check_return_ HRESULT GetNonLocalizedErrorString(_In_ XUINT32 stringId, _Out_ xstring_ptr* pstrErrorString) override;
    _Check_return_ HRESULT GetResourceData(_In_ uint32_t resourceId, _In_ const xstring_ptr_view& resourceType, _Out_ RawData* data) override;

    // Native Windowless host overrides
    IPALWindowlessHost* CreateWindowlessHost(_In_ IXcpHostSite* pHostSite, _In_ CDependencyObject* pParentEditBox, _In_ XUINT32 uRuntimeId) override;

#ifdef ENABLE_UIAUTOMATION

    // UIAutomation Functions
    _Check_return_ HRESULT UIAClientsAreListening(_In_ UIAXcp::APAutomationEvents eAutomationEvent) override;
    _Check_return_ HRESULT UIARaiseAutomationEvent(_In_ CAutomationPeer *pAP,
        _In_ UIAXcp::APAutomationEvents eAutomationEvent) override;
    _Check_return_ HRESULT UIARaiseAutomationPropertyChangedEvent(_In_ CAutomationPeer *pAP,
        _In_ UIAXcp::APAutomationProperties eAutomationProperty,
        _In_ const CValue& oldValue,
        _In_ const CValue& newValue) override;
    _Check_return_ HRESULT UIARaiseFocusChangedEventOnUIAWindow() override;
    _Check_return_ HRESULT UIARaiseTextEditTextChangedEvent(_In_ CAutomationPeer *pAP,
        _In_ UIAXcp::AutomationTextEditChangeType eType,
        _In_ CValue *pChange) override;
    _Check_return_ HRESULT UIARaiseNotificationEvent(
        _In_ CAutomationPeer* ap,
        UIAXcp::AutomationNotificationKind notificationKind,
        UIAXcp::AutomationNotificationProcessing notificationProcessing,
        _In_opt_ xstring_ptr displayString,
        _In_ xstring_ptr activityId) override;

#endif

    _Check_return_ HRESULT FireExecuteOnUIThreadMessage(_In_ IPALExecuteOnUIThread* pExecuter, const ReentrancyBehavior reentrancyBehavior) override;

    _Check_return_ HRESULT GetActualWidth(_Out_ XUINT32* pValue) override;
    _Check_return_ HRESULT GetActualHeight(_Out_ XUINT32* pValue) override;

    _Check_return_ HRESULT SetWindowSize(XSIZE size, _In_opt_ XHANDLE hwnd) override;
    _Check_return_ HRESULT SetWindowSizeOverride(_In_ const XSIZE *pWindowSize, _In_ XHANDLE hwnd) override;

    // Send a WM_REPLAY_PREVIOUS_POINTERUPDATE message back to Xaml so that we can try another hit test using the
    // current location of the pointer and update hover state for the element tree.
    void RequestReplayPreviousPointerUpdate() override;

    // Do a hit test using the current position of the pointer. Used to update the hover state of Xaml's UI after
    // an animation completes.
    void ReplayPreviousPointerUpdate(UINT32 previousPointerUpdateMsgId) override;

private:
    void SetFullScreen(bool fIsFullScreen) override
    {
        m_fIsFullScreen = fIsFullScreen;
    }

    bool IsFullScreen()
    {
        return m_fIsFullScreen;
    }

    // Silverlight and Jupiter use different satellite DLL unloading APIs. Each
    // has a separate implementation of this function.
    void UnloadSatelliteDll(_In_ HINSTANCE dll);

    _Check_return_ HRESULT GetResourceString(HINSTANCE hResource, _In_ XUINT32 stringId, _Out_ xstring_ptr* pstrResourceString);

    _Check_return_ HRESULT LoadStringResource(
        _In_ HINSTANCE module,
        _In_ XUINT32 stringId,
        _Inout_ XStringBuilder& strBuilder,
        _Out_ bool *pFoundString
        );

    _Check_return_ HRESULT EnsureCorrectWindowSize(
        unsigned int width,
        unsigned int height,
        _In_opt_ XHANDLE hwnd
        );

    HINSTANCE           m_hLocalizedResource;
    HINSTANCE           m_hNonLocalizedResource;

    XUINT8              m_bInit;

    bool                m_bReentrancyGuard;

    bool               m_forceRedrawOnPaint;

    // Async Download Request manager. Used to force async
    // download requests.
    IAsyncDownloadRequestManager * m_pAsyncDownloadRequestManager;

private:
    // Init
    _Check_return_ HRESULT Init(_In_ IXcpHostSite *pSite, _In_ IXcpDispatcher *pWin) override;

    // Load satellite DLLs for resources.
    HRESULT LoadResourceSatelliteDlls();

    void HandleKeyboardMessage(_In_ XUINT32 uMsg, _In_ MsgPacket *pMsgPack, _Inout_ InputMessage *pMsg, _Out_ bool &fHandled);
    HRESULT HandleFocusMessage(_In_ XUINT32 uMsg, _In_ MsgPacket *pMsgPack, _Inout_ InputMessage *pMsg, _Out_ bool &fHandled);
    HRESULT HandlePointerMessage(_In_ XUINT32 uMsg, _In_ MsgPacket *pMsgPack, _Inout_ InputMessage *pMsg, _In_opt_ CContentRoot* contentRoot, _Out_ bool &fHandled);
    HRESULT HandleActivateMessage(_In_ XUINT32 uMsg, _In_ MsgPacket *pMsgPack, _Inout_ InputMessage *pMsg, _Out_ bool &fHandled);

    _Check_return_ HRESULT ProcessAsyncDownloadRequest_BrowserImpl(
        _In_ IDownloader*                           pDownloader,
        _In_ IPALUri*                               pUriAbsolute,
        _In_ IPALDownloadResponseCallback*          pCallback,
        _In_ XINT32                                 fCrossDomain,
        _Outptr_result_maybenull_ IPALAbortableOperation**    ppIAbortableDownload,
        _In_ XUINT32                                eUnsecureDownloadAction);

private:
    bool m_fIsFullScreen;

    //
    // Used by the test framework. Overrides the size of the window to produce consistent results
    // when rendering on different platforms.
    //
    XUINT32 m_widthOverride;
    XUINT32 m_heightOverride;
    XUINT32 m_previousWidth;
    XUINT32 m_previousHeight;

    UINT32 m_previousPointerUpdateMsgId;
};

