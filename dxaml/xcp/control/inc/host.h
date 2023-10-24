// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Contains the types and methods provided by the host abstraction layer

#pragma once

struct IObject;
struct IDownloader;
struct IManagedRuntimeHost;
struct IPALPreferences;
struct InputMessage;
class CompositorScheduler;
class RawData;
class CContentRoot;
class CUIAWindow;

enum MouseCursor : uint8_t;

enum class ReentrancyBehavior;

//------------------------------------------------------------------------
//
//  Interface:  IXcpHostSite
//
//  Synopsis:
//      Host interface. A control implements this to host Jolt.
//
//------------------------------------------------------------------------
struct IXcpHostSite
{
protected:
    ~IXcpHostSite(){}  // 'delete' not allowed
public:
    virtual void IncrementReference() = 0;
    virtual void ReleaseReference() = 0;
    virtual void ReportError(_In_ IErrorService *pErrInfo) = 0;

    // These do not addref
    virtual IPALUri * GetBaseUri() = 0;
    virtual IPALUri * GetSecurityUri() = 0;
    virtual _Check_return_ HRESULT SetAppUri(_In_ IPALUri *pAppUri) = 0;

    virtual IXcpDispatcher * GetXcpDispatcher() = 0;

    virtual IXcpBrowserHost * GetBrowserHost() const = 0;

    virtual _Check_return_ HRESULT CreateResourceManager(_Outptr_ IPALResourceManager** ppResourceManager) = 0;

    virtual XINT32 IsInitialized() = 0;

    virtual _Check_return_ HRESULT GetActualHeight(_Out_ XUINT32* actualHeight) = 0;
    virtual _Check_return_ HRESULT GetActualWidth(_Out_ XUINT32* actualWidth) = 0;

    virtual XUINT32 GetIdentity() = 0;

    virtual _Check_return_ HRESULT IsDependencyObjectValid(_In_ CDependencyObject* pDO) = 0;

    virtual bool GetEnableFrameRateCounter() const = 0;
    virtual _Check_return_ HRESULT SetEnableFrameRateCounter(bool enabled) = 0;

    virtual _Check_return_ XHANDLE GetXcpControlWindow() = 0;

    // UIAutomation Functions
    virtual _Check_return_ HRESULT UIAClientsAreListening(_In_ UIAXcp::APAutomationEvents eAutomationEvent) = 0;
    virtual _Check_return_ HRESULT UIARaiseAutomationEvent(_In_ CAutomationPeer *pAP,
        _In_ UIAXcp::APAutomationEvents eAutomationEvent) = 0;
    virtual _Check_return_ HRESULT UIARaiseAutomationPropertyChangedEvent(_In_ CAutomationPeer *pAP,
        _In_ UIAXcp::APAutomationProperties eAutomationProperty,
        _In_ const CValue& oldValue,
        _In_ const CValue& newValue) = 0;
    virtual _Check_return_ HRESULT UIARaiseFocusChangedEventOnUIAWindow() = 0;
    virtual _Check_return_ HRESULT UIARaiseTextEditTextChangedEvent(_In_ CAutomationPeer *pAP,
        _In_ UIAXcp::AutomationTextEditChangeType eAutomationTextEditChangeType,
        _In_ CValue *pChangedData) = 0;
    virtual _Check_return_ HRESULT UIARaiseNotificationEvent(
        _In_ CAutomationPeer* ap,
        UIAXcp::AutomationNotificationKind notificationKind,
        UIAXcp::AutomationNotificationProcessing notificationProcessing,
        _In_opt_ xstring_ptr displayString,
        _In_ xstring_ptr activityId) = 0;
    virtual HRESULT GetUIAWindow(
        _In_ CDependencyObject *pElement,
        _In_ XHANDLE hWnd,
        _In_ bool onlyGet,
        _Outptr_ CUIAWindow** uiaWindowNoRef) = 0;
    virtual _Check_return_ HRESULT GetAccessibleRootObjectReference(
            XHANDLE hWnd,
            XDWORD dwFlags,
            XDWORD dwObjId,
            _Out_ XDWORD* pdwReference,
            _Out_ bool* pbHandled) = 0;

    virtual _Check_return_ HRESULT OnFirstFrameDrawn() = 0;

    virtual bool IsWindowDestroyed() = 0;

    virtual void OnReentrancyDetected() = 0;

    virtual bool IsHdrOutput() const = 0;
};

// Event related info used to pass parameters to event handlers
struct CEventInfo
{
    CEventInfo(
        _In_opt_ CDependencyObject *pListener,
        _In_opt_ EventHandle hEvent,
        _In_opt_ CDependencyObject *pSender,
        _In_opt_ CEventArgs *pArgs,
        _In_ XINT32 flags,
        _In_opt_ INTERNAL_EVENT_HANDLER pInternalHandler);

    CEventInfo(const CEventInfo& rhs);

    ~CEventInfo();

    xref_ptr<CDependencyObject> Listener;
    EventHandle                 Event;
    bool                        IsValid = true;
    XINT32                      Flags = 0;
    xref_ptr<CDependencyObject> Sender;
    xref_ptr<CEventArgs>        Args;
    INTERNAL_EVENT_HANDLER      Handler = nullptr;
};


//------------------------------------------------------------------------
//
//  Interface:  IXcpBrowserHost
//
//  Synopsis:
//      Interface for a Jolt host to talk with the Core.
//
//------------------------------------------------------------------------
struct IXcpBrowserHost: public IObject
{
protected:
    ~IXcpBrowserHost(){}  // 'delete' not allowed, use 'Release' instead.
public:
    virtual bool IsTestBrowserHost() const = 0;

    virtual _Check_return_ HRESULT OnPaint() = 0;

    virtual void OnDisplayChange() = 0;

    virtual void ForceRedraw() = 0;

    virtual _Check_return_ HRESULT OnTick() = 0;

    virtual _Check_return_ HRESULT CLR_FireEvent(
        _In_ CDependencyObject *pListener,
        _In_ EventHandle hEvent,
        _In_ CDependencyObject* pSender,
        _In_ CEventArgs* pArgs,
        _In_ XUINT32 flags = 0) = 0;

    virtual _Check_return_ HRESULT ShouldFireEvent(
        _In_ CDependencyObject *pListener,
        _In_ EventHandle hEvent,
        _In_ CDependencyObject* pSender,
        _In_ CEventArgs* pArgs,
        _In_ XINT32 flags,
        _Out_ XINT32* pfSupported) = 0;

    virtual _Check_return_ HRESULT put_EmptySource(
        _In_ bool firstLoad) = 0;

    virtual float get_CurrFrameRate() = 0;

    virtual _Check_return_ HRESULT get_EnableFrameRateCounter(_Out_ bool *pIsEnabled) = 0;
    virtual _Check_return_ HRESULT set_EnableFrameRateCounter(bool isEnabled) = 0;

    virtual _Check_return_ HRESULT get_BaseUri(_Out_ IPALUri** ppBaseUri) = 0;
    virtual _Check_return_ HRESULT get_SecurityUri(_Out_ IPALUri** ppSecurityUri) = 0;

    virtual _Check_return_ HRESULT CreateFromXaml(
        _In_ XUINT32 cXaml,
        _In_reads_(cXaml) const WCHAR *pXaml,
        _In_ bool bCreateNamescope,
        _In_ bool bRequiresDefaultNamespace,
        _In_ bool bExpandTemplatesDuringParse,
        _Outptr_result_maybenull_ CDependencyObject **ppIDO) = 0;

    virtual _Check_return_ HRESULT
        GetSystemGlyphTypefaces(
        _Outptr_result_maybenull_ CDependencyObject** ppIDO) = 0;

    virtual _Check_return_ HRESULT  GetRootVisual(
        _Outptr_ CDependencyObject**     ppIDO) = 0;

    virtual _Check_return_ CDependencyObject* GetPublicOrFullScreenRootVisual() = 0;

    virtual IXcpHostSite* GetHostSite() = 0;

    virtual _Check_return_ HRESULT ResetVisualTree() = 0;

    virtual _Check_return_ HRESULT ResetState() = 0;

    virtual void HandleInputMessage(
        _In_ XUINT32 uMsg,
        _In_ MsgPacket *pMsgPack,
        _In_ CContentRoot* contentRoot,
        bool isReplayedMessage,
        _Out_ bool &fHandled) = 0;

    virtual _Check_return_ HRESULT GetActualWidth(_Out_ XUINT32* pValue) = 0;
    virtual _Check_return_ HRESULT GetActualHeight(_Out_ XUINT32* pValue) = 0;

    virtual void Deinit() = 0;
    virtual _Check_return_ bool IsShuttingDown() = 0;
    virtual void SetShuttingDown(_In_ bool state) = 0;

    virtual void FreeResourceLibraries() = 0;

    virtual void SetMouseCapture(HWND newWindow) = 0;
    virtual void ReleaseMouseCapture() = 0;

    //
    // Report Error information in host.
    // Give Core a chance to report async errors, such as MediaFailed, ImageFailed, DownloadFailed etc.
    //
    virtual void ReportError( ) = 0;

    virtual void ApplicationStartupEventComplete() = 0;
    virtual void FireApplicationStartupEventComplete() = 0;
    virtual void FireApplicationLoadComplete() = 0;
    virtual void FireForceGCCollectEvent() = 0;

    // Pass the Error service to control level's code to handle errors.
    // the Script plugin might need this to save detail error information and generate error
    // message fromt the error service.
    //
    virtual _Check_return_ HRESULT GetErrorService(_Out_ IErrorService **ppErrorService  ) = 0;

    virtual CCoreServices *GetContextInterface() = 0;

    // Disconnect from the internal core
    virtual _Check_return_ HRESULT DetachCore() = 0;

    virtual HRESULT _Check_return_ CheckUri(
        _In_ const xstring_ptr& strRelativeUri,
        _In_ XUINT32 eUnsecureDownloadAction,
        _Out_opt_ XINT32 *pfShouldSuppressCookies = NULL) = 0;

    virtual HRESULT _Check_return_ CheckUri(
        _In_ IPALUri* pUri,
        _In_ XUINT32 eUnsecureDownloadAction,
        _Out_opt_ XINT32 *pfShouldSuppressCookies = NULL) = 0;

    //Get a localized resource string
    virtual _Check_return_ HRESULT GetLocalizedResourceString(_In_ XUINT32 stringId, _Out_ xstring_ptr* resourceString) = 0;
    virtual _Check_return_ HRESULT GetNonLocalizedResourceString(_In_ XUINT32 stringId, _Out_ xstring_ptr* resourceString) = 0;
    virtual _Check_return_ HRESULT GetNonLocalizedErrorString(_In_ XUINT32 stringId, _Out_ xstring_ptr* errorString) = 0;
    virtual _Check_return_ HRESULT GetResourceData(_In_ uint32_t resourceId, _In_ const xstring_ptr_view& resourceType, _Out_ RawData* data) = 0;

    virtual _Check_return_ HRESULT UIAClientsAreListening(_In_ UIAXcp::APAutomationEvents eAutomationEvent) = 0;
    virtual _Check_return_ HRESULT UIARaiseAutomationEvent(_In_ CAutomationPeer *pAP,
        _In_ UIAXcp::APAutomationEvents eAutomationEvent) = 0;
    virtual _Check_return_ HRESULT UIARaiseAutomationPropertyChangedEvent(_In_ CAutomationPeer *pAP,
        _In_ UIAXcp::APAutomationProperties eAutomationProperty,
        _In_ const CValue& oldValue,
        _In_ const CValue& newValue) = 0;
    virtual _Check_return_ HRESULT UIARaiseFocusChangedEventOnUIAWindow() = 0;
    virtual _Check_return_ HRESULT UIARaiseTextEditTextChangedEvent(_In_ CAutomationPeer *pAP,
        _In_ UIAXcp::AutomationTextEditChangeType eAutomationTextEditChangeType,
        _In_ CValue *pChangedData) = 0;
    virtual _Check_return_ HRESULT UIARaiseNotificationEvent(
        _In_ CAutomationPeer* ap,
        UIAXcp::AutomationNotificationKind notificationKind,
        UIAXcp::AutomationNotificationProcessing notificationProcessing,
        _In_opt_ xstring_ptr displayString,
        _In_ xstring_ptr activityId) = 0;

    virtual _Check_return_ HRESULT IsReentrancyAllowed(
        _Out_ XINT32 *pbReentrancyAllowed) = 0;

    virtual _Check_return_ HRESULT FireExecuteOnUIThreadMessage(_In_ IPALExecuteOnUIThread* pExecuter, const ReentrancyBehavior reentrancyBehavior) = 0;
    virtual _Check_return_ HRESULT ProcessExecuteMessage() = 0;
    virtual void PurgeThreadMessages() = 0;

    virtual _Ret_notnull_ WindowsGraphicsDeviceManager *GetGraphicsDeviceManager() = 0;
    virtual _Ret_maybenull_ CompositorScheduler* GetCompositorScheduler() = 0;
    virtual _Ret_maybenull_ ITickableFrameScheduler *GetFrameScheduler() = 0;

    virtual bool HasRenderTarget() = 0;

    virtual IPALWindowlessHost* CreateWindowlessHost(_In_ IXcpHostSite* pHostSite, _In_ CDependencyObject* pParentEditBox, _In_ XUINT32 uRuntimeId) = 0;

    virtual void SetFullScreen(bool fIsFullScreen) = 0;

    virtual _Check_return_ HRESULT SetWindowSize(XSIZE size, _In_opt_ XHANDLE hwnd) = 0;

    virtual _Check_return_ HRESULT SetWindowSizeOverride(
        _In_ const XSIZE *pWindowSize,
        _In_ XHANDLE hwnd
        ) = 0;

    virtual void RequestReplayPreviousPointerUpdate() = 0;
    virtual void ReplayPreviousPointerUpdate(UINT32 previousPointerUpdateMsgId) = 0;

    virtual _Check_return_ HRESULT CreateRenderTarget() = 0;
    virtual _Check_return_ HRESULT CleanupRenderTarget() = 0;
    virtual void ResetDownloader() = 0;
};

//------------------------------------------------------------------------
//
//  Interface:  IXcpHostCallback
//
//  Synopsis:
//      Interface for Jolt to talk with the Host through the control
//
//------------------------------------------------------------------------
struct IXcpHostCallback: public IObject
{
    virtual HRESULT GetBASEUrlFromDocument(
        _Out_ IPALUri** ppBaseUri,
        _Out_ IPALUri** ppDocUri) = 0;

};

//------------------------------------------------------------------------
//
//  Interface:  IXcpDispatcher
//  Synopsis:
//       This interface deals with message dispatching and message synchronization
//       The rendering calls are also routed through this interface since they require message sync
//       Pre-Sprint 9, this was the IXcpWindow interface, and it is no longer the channel between BH and Control.
//
//------------------------------------------------------------------------
struct IXcpDispatcher: public IObject
{
protected:
    ~IXcpDispatcher(){}  // 'delete' not allowed, use 'Release' instead.
public:
    // Direct tunnel to browserhost and site
    virtual IXcpBrowserHost * GetBrowserHost() = 0;
    virtual IXcpHostSite * GetHostSite() = 0;

    virtual _Check_return_ HRESULT Start() = 0;
    virtual void Stop() = 0;
    virtual void Disable() = 0;

    // These methods will be called by the control to talk to the browser host
    virtual _Check_return_ HRESULT SetControl( _In_opt_ void *pControl, _In_opt_ EVENTPFN pFn) = 0;
    virtual _Check_return_ HRESULT OnPaint() = 0;

    virtual void Deinit() = 0;
    virtual _Check_return_ HRESULT Init(_In_ IXcpHostSite *pSite) = 0;
    virtual _Check_return_ HRESULT SetBrowserHost(_In_ IXcpBrowserHost *pBH) = 0;

    virtual _Check_return_ HRESULT IsReentrancyAllowed(
        _Out_ XINT32 *pbReentrancyAllowed) = 0;

    virtual _Check_return_ HRESULT QueueTick() = 0;
    virtual _Check_return_ HRESULT QueueDeferredInvoke(XUINT32 nMsg, ULONG_PTR wParam, LONG_PTR lParam) = 0;
    virtual void Tick() = 0;
    virtual msy::IDispatcherQueue* GetDispatcherQueueNoRef() = 0;
};
