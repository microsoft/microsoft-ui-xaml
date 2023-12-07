// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class WindowsGraphicsDeviceManager;
class CCoreServices;

class CommonBrowserHost :
    public IXcpBrowserHost,
    public ICoreServicesSite,
    public IDownloaderSite

{
public:
    CommonBrowserHost();
    virtual ~CommonBrowserHost();

public:
    bool IsTestBrowserHost() const final
    {
        return false;
    }

    float get_CurrFrameRate() final { return m_frameCounter.GetFPS(); }

    // This member is used for cases where there are legitimate exceptions
    // to site-of-origin download restriction.  If you make a call to this
    // member, comment the call as to why the exception is valid and secure.
    _Check_return_ HRESULT
        UnsecureDownload(
        _In_ IPALDownloadRequest                    *pDownloadRequest,
        _Outptr_opt_ IPALAbortableOperation      **ppIAbortableDownload,
        _In_opt_ IPALUri                            *pPreferredBaseUri = NULL) override;


    //IDownloaderSite

    HRESULT _Check_return_ CheckUri(
        _In_ const xstring_ptr&                     theRelativeUri,
        _In_ XUINT32                 eUnsecureDownloadAction,
        _Out_opt_ XINT32 *pfShouldSuppressCookies = NULL) final;

    HRESULT _Check_return_ CheckUri(
        _In_ IPALUri* pUri,
        _In_ XUINT32 eUnsecureDownloadAction,
        _Out_opt_ XINT32 *pfShouldSuppressCookies = NULL) final;

    // IXcpBrowserHost /////////////////////////////////////vtable do not muck
    XUINT32
        AddRef() final;

    XUINT32
        Release() final;

     HRESULT
        CLR_FireEvent(
        _In_ CDependencyObject*             pListener,
        _In_ EventHandle                    hEvent,
        _In_ CDependencyObject*             pSender,
        _In_ CEventArgs*                    pArgs,
        _In_ XUINT32                        flags = 0) final;

     HRESULT
        ShouldFireEvent(
        _In_ CDependencyObject*             pListener,
        _In_ EventHandle                    hEvent,
        _In_ CDependencyObject*             pSender,
        _In_ CEventArgs*                    pArgs,
        _In_ XINT32                         flags,
        _Out_ XINT32*                       pfShouldFire) final;

    virtual _Check_return_ HRESULT
        put_Source(
        _In_ XUINT32                        cstr,
        _In_reads_(cstr) XUINT8*           pstr,
        _In_opt_ IPALMemory*                pPalMemory = NULL ) = 0;

    _Check_return_ HRESULT get_EnableFrameRateCounter(_Out_ bool *pIsEnabled) final;
    _Check_return_ HRESULT set_EnableFrameRateCounter(bool isEnabled) final;

    _Check_return_ HRESULT get_BaseUri(_Out_ IPALUri** ppBaseUri) final;
    _Check_return_ HRESULT get_SecurityUri(_Out_ IPALUri** ppBaseUri) final;

    _Check_return_ HRESULT GetDownloaderSiteBaseUri(_Out_ IPALUri** ppBaseUri) override;
    _Check_return_ HRESULT GetDownloaderSiteSecurityUri(_Out_ IPALUri** ppSecurityUri) override;

    bool IsNetworkingUnrestricted() override;

    _Check_return_ HRESULT
        CreateFromXaml(
        _In_ XUINT32 cXaml,
        _In_reads_(cXaml) const WCHAR *pXaml,
        _In_ bool bCreateNamescope,
        _In_ bool bRequireDefaultNamespace,
        _In_ bool bExpandTemplatesDuringParse,
        _Outptr_ CDependencyObject **ppIDO) final;

    HRESULT _Check_return_
        GetSystemGlyphTypefaces(
        _Outptr_ CDependencyObject**     ppDo) final;

    _Check_return_ HRESULT
        GetRootVisual(
        _Outptr_ CDependencyObject**     ppIDO) final;

    _Check_return_ CDependencyObject* GetPublicOrFullScreenRootVisual() final;

    _Check_return_ HRESULT
        ResetVisualTree() final;

    _Check_return_ bool IsShuttingDown() final;

    void SetShuttingDown(bool isShuttingDown) final;

    virtual _Check_return_ HRESULT
        ResetCore() = 0;

    //          Warning!!!!
    //
    //          DeInit() must be called by a derived/subclass destructor, not the base class
    //              destructor. This is because virtual methods in derived classes called by
    //              destructors (and constructors) may not function as expected, or even crash.
    //              See Item #9 in  "Effective C++" by Scott Meyers

    _Check_return_ HRESULT IsReentrancyAllowed(
        _Out_ XINT32 *pbReentrancyAllowed) final;

    // Pass the Error service to control level's code to handle errors.
    // the Script plugin might need this to save detail error information and generate error
    // message fromt the error service.
    _Check_return_ HRESULT GetErrorService(_Out_ IErrorService **ppErrorService) final;

    CCoreServices *GetContextInterface() override;

    _Check_return_ HRESULT
        DetachCore() final;

    // Init
    virtual _Check_return_ HRESULT
        Init(
        _In_ IXcpHostSite*              pSite,
        _In_ IXcpDispatcher*            pDispatcher);

    // UIAutomationAccessibility
    _Check_return_ HRESULT UIAClientsAreListening(_In_ UIAXcp::APAutomationEvents eAutomationEvent) override
    {
        return E_NOTIMPL;
    }
    _Check_return_ HRESULT UIARaiseAutomationEvent(_In_ CAutomationPeer *pAP,
        _In_ UIAXcp::APAutomationEvents eAutomationEvent) override
    {
        return E_NOTIMPL;
    }
    _Check_return_ HRESULT UIARaiseAutomationPropertyChangedEvent(_In_ CAutomationPeer *pAP,
        _In_ UIAXcp::APAutomationProperties eAutomationProperty,
        _In_ const CValue& oldValue,
        _In_ const CValue& newValue) override
    {
        return E_NOTIMPL;
    }
    _Check_return_ HRESULT UIARaiseFocusChangedEventOnUIAWindow() override
    {
        return E_NOTIMPL;
    }

    _Check_return_ HRESULT ProcessExecuteMessage() final;
    void PurgeThreadMessages() final;

    _Ret_notnull_ WindowsGraphicsDeviceManager *GetGraphicsDeviceManager() final
    {
        return m_graphicsDeviceManager.get();
    }

    _Ret_maybenull_ CompositorScheduler* GetCompositorScheduler() final
    {
        return m_pNWCompositorScheduler;
    }

    _Ret_maybenull_ ITickableFrameScheduler *GetFrameScheduler() final
    {
        return m_pUIThreadScheduler;
    }

    bool HasRenderTarget() final
    {
        return (m_pNWRenderTarget != NULL);
    }

    IXcpHostSite *GetHostSite() override
    {
        return m_pSite;
    }

    _Check_return_ HRESULT CreateRenderTarget() override;

    _Check_return_ HRESULT CleanupRenderTarget() override;

    void CleanupErrorService();

    void ResetDownloader() override;

protected:
    HRESULT _Check_return_
        EnsureDownloader();

    void
        CleanupCommon();

protected:
    IXcpHostSite*           m_pSite;
    CCoreServices*          m_pcs;
    IErrorService*          m_pErrorService;

    xref_ptr<WindowsGraphicsDeviceManager> m_graphicsDeviceManager;
    CompositorScheduler *m_pNWCompositorScheduler;
    ITickableFrameScheduler *m_pUIThreadScheduler;
    CWindowRenderTarget* m_pNWRenderTarget;

    IXcpDispatcher*         m_pDispatcherNoRef;
    CFrameCounter           m_frameCounter;
    IDownloader*            m_pDownloader ;

    // script callback
    void*                   m_pControl;

    IPALQueue*              m_pUIThreadExecuterQueue;

private:
    // Instance variables
    XUINT32                 m_cRef;

protected:
    bool                    m_bInResetVisualTree;
    bool                    m_fForceRedraw;

    // Private methods

    _Check_return_ HRESULT
        StopDownloads();
};
