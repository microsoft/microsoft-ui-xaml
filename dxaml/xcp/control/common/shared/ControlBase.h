// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//   Implement Base class for browser based controls/plugins.
//   This class should compile both on Win and Mac
//   and are shared implementation among all browsers.

#pragma once

#include "host.h"

// This class implements all the functionality that is common to all controls and specific sublclasses per
// platform can choose to override specifics

class CEventArgs;

class CControlBase:
    public IXcpHostSite
{
public:
    CControlBase()
    {
        m_fInitialized = false;

        XCP_STRONG(&m_pDispatcher);
        m_pDispatcher = NULL;

        XCP_STRONG(&m_pBH);
        m_pBH = NULL;
        m_bFirstLoad = true;
        m_isFrameCounterEnabled = FALSE;

        m_pBaseUri = NULL;
        m_pDocUri = NULL;
        m_pXcpHostCallback = NULL;

        m_objIdentity = 0;
    }

    virtual ~CControlBase()
    {
        ReleaseInterface(m_pDispatcher);
        ReleaseInterface(m_pBH);
        ReleaseInterface(m_pXcpHostCallback);
        ReleaseInterface(m_pBaseUri);
        ReleaseInterface(m_pDocUri);
        m_objIdentity = 0;
    }

    inline bool IsInit() { return m_fInitialized;}

    virtual _Check_return_ HRESULT Init();

    inline IXcpDispatcher* GetXcpDispatcher() override
    {
        return m_pDispatcher;
    }

    IPALUri* GetBaseUri() override
    {
        if (!m_pBaseUri)
            (void) GetClientURI();
        return m_pBaseUri;
    }

    IPALUri* GetSecurityUri() override    {
        if (!m_pDocUri)
            (void) GetClientURI();
        return m_pDocUri;
    }

    _Check_return_ HRESULT SetAppUri(_In_ IPALUri *pAppUri) override
    {
        HRESULT hr = S_OK;
        IPALUri* pBaseUri = NULL;

        IFCPTR(pAppUri);

        IFC(pAppUri->CreateBaseURI(&pBaseUri));

        ReleaseInterface(m_pDocUri);
        m_pDocUri = pAppUri;
        m_pDocUri->AddRef();

        ReleaseInterface(m_pBaseUri);
        m_pBaseUri = pBaseUri;
        pBaseUri = NULL;

    Cleanup:
        ReleaseInterface(pBaseUri);

        RRETURN(hr);
    }

    void ResetAppUri()
    {
        ReleaseInterface(m_pDocUri);
        ReleaseInterface(m_pBaseUri);
        (void) GetClientURI();
    }

    _Check_return_ HRESULT OnPaint();

    virtual HRESULT UpdateSource() = 0;

    static void setPlatformUtilities(_In_ IPlatformUtilities* pUtilities)
    {
        s_pUtilities.Set(pUtilities);
    }

    IXcpBrowserHost * GetBrowserHost() const override { return m_pBH; };

    XINT32 IsInitialized() override
    {
        return m_fInitialized;
    }

    // Returns the Identity of current core
    XUINT32 GetIdentity() override{ return m_objIdentity;}


    _Check_return_ HRESULT IsDependencyObjectValid(_In_ CDependencyObject* pDO) override;

    bool GetEnableFrameRateCounter() const override
    {
        return m_isFrameCounterEnabled;
    }

    _Check_return_ HRESULT SetEnableFrameRateCounter(bool isEnabled) override;

    _Check_return_ HRESULT OnDebugSettingsChanged();

    _Check_return_ XHANDLE GetXcpControlWindow() override
    {
        return NULL;
    }


// UIAutomation Functions
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

    _Check_return_ HRESULT UIARaiseTextEditTextChangedEvent(_In_ CAutomationPeer *pAP,
        _In_ UIAXcp::AutomationTextEditChangeType eAutomationProperty,
        _In_ CValue *cValue) override
    {
        return E_NOTIMPL;
    }

    _Check_return_ HRESULT UIARaiseNotificationEvent(
        _In_ CAutomationPeer* ap,
        UIAXcp::AutomationNotificationKind notificationKind,
        UIAXcp::AutomationNotificationProcessing notificationProcessing,
        _In_opt_ xstring_ptr displayString,
        _In_ xstring_ptr activityId) override
    {
        return E_NOTIMPL;
    }

    HRESULT GetUIAWindow(
        _In_ CDependencyObject *pElement, 
        _In_ XHANDLE hWnd, 
        _In_ bool onlyGet, 
        _Outptr_ CUIAWindow** uiaWindowNoRef) override
    {
        return E_NOTIMPL;
    }

    HRESULT GetAccessibleRootObjectReference(
            XHANDLE hWnd,
            XDWORD dwFlags,
            XDWORD dwObjId,
            _Out_ XDWORD* pdwReference,
            _Out_ bool* pbHandled) override
    {
        return E_NOTIMPL;
    }


    virtual bool IsUIAutomationCoreSupported()
    {
        return false;
    }

    _Check_return_ HRESULT OnFirstFrameDrawn() override
    {
        return S_OK;
    }

    void PurgeThreadMessages()
    {
        m_pBH->PurgeThreadMessages();
    }

protected:
    HRESULT UpdateSource(_In_ EVENTPFN pFn);
    // If the installed version is not supported, we should override the control's source with
    // content that prompts for upgrading.
    void Deinit();
    HRESULT GetClientURI();
    static HRESULT ScriptCallback(
        _In_reads_bytes_(sizeof(CControlBase)) void* pControl,
        _In_ CDependencyObject *pListener,
        _In_ EventHandle hEvent,
        _In_opt_ CDependencyObject* pSender,
        _In_opt_ CEventArgs* pArgs,
        _In_ XINT32 flags,
        _In_opt_ IScriptObject* pScriptObject,
        _In_opt_ INTERNAL_EVENT_HANDLER pHandler
        );
    _Check_return_ HRESULT CheckReentrancy();


// Data Members
protected:
    IPALUri* m_pBaseUri;
    IPALUri* m_pDocUri;

    IXcpDispatcher* m_pDispatcher;
    IXcpBrowserHost* m_pBH;
    IXcpHostCallback* m_pXcpHostCallback;

    bool m_fInitialized;
    bool m_isFrameCounterEnabled;
    bool m_bFirstLoad;

    static EncodedPtr<IPlatformUtilities> s_pUtilities;

public:
    // Security token.
    XUINT32 m_objIdentity;
};

