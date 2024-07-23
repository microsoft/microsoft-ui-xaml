// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifndef JUPITER_CONTROL_H_
#define JUPITER_CONTROL_H_

#include <ControlBase.h>
#include "host.h"
#include <fwd/Microsoft.UI.Xaml.h>
#include <ShObjIdl_core.h>

class CUIAHostWindow;
class CUIAWrapper;
class CJupiterWindow;
class CContentRoot;

class CJupiterXcpHostCallback : public IXcpHostCallback, public CReferenceCount
{
public:

    // IObject functions

    XUINT32 AddRef() override
    {
        return CReferenceCount::AddRef();
    }

    XUINT32 Release() override
    {
        return CReferenceCount::Release();
    }

    // IXcpHostCallback functions

    HRESULT GetBASEUrlFromDocument(
        _Out_ IPALUri** ppBaseUri,
        _Out_ IPALUri** ppDocUri) override;

};

class CJupiterControl final : public CControlBase, public CReferenceCount
{
public:
    static _Check_return_ HRESULT Create(_Outptr_ CJupiterControl** ppControl);

    _Check_return_ HRESULT Init() override;
    void Deinitialize();

    void DisconnectUIA();
    _Check_return_ HRESULT ResetVisualTree();

    bool HandleWindowMessage(UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam, _In_opt_ CContentRoot* contentRoot);

    LRESULT HandleGetObjectMessage(_In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);
    HRESULT CreateUIAHostWindowForHwnd(_In_ HWND hwnd);
    wrl::ComPtr<CUIAHostWindow> GetUIAHostWindow() { return m_pUIAWindow; }

    void OnDisplayChanged();
    static HRESULT NotifyImmersiveColorSetChanged();
    static HRESULT OnThemeChanged();

    HRESULT UpdateFontScale(_In_ XFLOAT newFontScale);

    HRESULT OnSettingChanged(HSTRING settingName);

    _Check_return_ HRESULT OnJupiterWindowSizeChanged(CJupiterWindow &jupiterWindow, HWND hwndContext);

    _Check_return_ HRESULT SetWindow(_In_ CJupiterWindow* pWindow);

    _Check_return_ HRESULT RunCoreWindowMessageLoop();

    _Check_return_ HRESULT ActivateWindow();

    _Check_return_ HRESULT NotifyFirstFramePending();

    CCoreServices* GetCoreServices() const
    {
        IXcpBrowserHost* pBH = GetBrowserHost();
        return static_cast<CCoreServices*>(pBH ? pBH->GetContextInterface() : nullptr);
    }

    // IXcpHostSite functions

    void IncrementReference() override { AddRef(); }

    void ReleaseReference() override { Release(); }

    void ReportError(_In_ IErrorService *pErrInfo) override
    {
    }

    CJupiterWindow* GetJupiterWindow() const { return m_pWindow; }

    _Check_return_ HRESULT GetActualHeight(_Out_ XUINT32* actualHeight) override
    {
        return m_pBH->GetActualHeight(actualHeight);
    }

    _Check_return_ HRESULT GetActualWidth(_Out_ XUINT32* actualWidth) override
    {
        return m_pBH->GetActualWidth(actualWidth);
    }

    _Check_return_ XHANDLE GetXcpControlWindow() override;

    _Check_return_ HRESULT OnFirstFrameDrawn() override;

    bool IsWindowDestroyed() override;

    _Check_return_ HRESULT CreateResourceManager(_Outptr_ IPALResourceManager** ppResourceManager) override;

    void OnReentrancyDetected() override;

    bool IsHdrOutput() const override;

    // CControlBase functions

    HRESULT UpdateSource() override
    {
        return CControlBase::UpdateSource(&CControlBase::ScriptCallback);
    }

    // Helper functions

    _Check_return_ HRESULT ConfigureJupiterWindow(_In_opt_ wuc::ICoreWindow* pCoreWindow);

    void ScreenToClient(_Inout_ POINT* pPixelPoint);
    void ClientToScreen(_Inout_ POINT* pPixelPoint);

    _Check_return_ HRESULT SetTicksEnabled(bool fTicksEnabled);

    // UIAutomation Functions
    _Check_return_ HRESULT UIAClientsAreListening(_In_ UIAXcp::APAutomationEvents eAutomationEvent) override;
    _Check_return_ HRESULT UIARaiseAutomationEvent(_In_ CAutomationPeer *pAP,
        _In_ UIAXcp::APAutomationEvents eAutomationEvent) override;
    _Check_return_ HRESULT UIARaiseAutomationPropertyChangedEvent(_In_ CAutomationPeer *pAP,
        _In_ UIAXcp::APAutomationProperties eAutomationProperty,
        _In_ const CValue& oldValue,
        _In_ const CValue& newValue) override;
    _Check_return_ HRESULT UIARaiseFocusChangedEventOnUIAWindow() override;
    _Check_return_ HRESULT UIARaiseTextEditTextChangedEvent(
        _In_ CAutomationPeer *pAP,
        _In_ UIAXcp::AutomationTextEditChangeType eAutomationProperty,
        _In_ CValue *cValue) override;
    _Check_return_ HRESULT UIARaiseNotificationEvent(
        _In_ CAutomationPeer* ap,
        UIAXcp::AutomationNotificationKind notificationKind,
        UIAXcp::AutomationNotificationProcessing notificationProcessing,
        _In_opt_ xstring_ptr displayString,
        _In_ xstring_ptr activityId) override;
    _Check_return_ HRESULT GetUIAWindow(
        _In_ CDependencyObject *pElement,
        _In_ XHANDLE hWnd,
        _In_ bool onlyGet,
        _Outptr_ CUIAWindow** uiaWindowNoRef) override;
    _Check_return_ HRESULT CreateProviderForAP(_In_ CAutomationPeer* pAP, _Outptr_result_maybenull_ CUIAWrapper** ppRet);

    LRESULT ForwardWindowedPopupMessageToJupiterWindow(
        _In_ HWND window,
        _In_ UINT message,
        _In_ WPARAM wParam,
        _In_ LPARAM lParam,
        _In_opt_ CContentRoot* contentRoot);

    void SetMockUIAClientsListening(bool isEnabledMockUIAClientsListening);

    bool HandlePointerMessage(
        _In_ UINT uMsg,
        _In_ WPARAM wParam,
        _In_ LPARAM lParam,
        _In_opt_ CContentRoot* contentRoot,
        bool isGeneratedMessage = false,
        _In_opt_ ixp::IPointerPoint* pointerPoint = nullptr,
        _In_opt_ ixp::IPointerEventArgs* pointerEventArgs = nullptr
        );

    void HandleNonClientPointerMessage(
        _In_ UINT uMsg,
        _In_ UINT32 pointerId,
        _In_opt_ CContentRoot* contentRoot,
        bool isGeneratedMessage = false,
        _In_opt_ ixp::IPointerPoint* pointerPoint = nullptr
        );

    // Test hooks
    void SetHdrOutputOverride(bool isHdrOutputOverride);

private:
    CJupiterControl();
    ~CJupiterControl() override;

    _Check_return_ HRESULT UpdateHdr() const;
    void Paint();
    bool HandleKeyMessage(_In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam, _In_opt_ CContentRoot* contentRoot);
    bool HandleGenericMessage(_In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam, _In_opt_ CContentRoot* contentRoot);
    bool HandleUpdateUIStateMessage(_In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam, _In_opt_ CContentRoot* contentRoot);

    CJupiterWindow* m_pWindow;
    wrl::ComPtr<CUIAHostWindow> m_pUIAWindow;

    bool m_isEnabledMockUIAClientsListening;

    mutable ULONG m_lastDisplaySettingsUniqueness = 0;
    mutable RECT m_lastWindowRect = {};
    mutable HMONITOR m_lastMonitor = nullptr;
    mutable bool m_cachedIsHdr = false;
    mutable bool m_isCachedHdrValid = false;
    bool m_isHdrOutputOverride = false;
};

class CJupiterErrorServiceListener final : public IErrorServiceListener, public CReferenceCount
{
public:
    void NotifyErrorAdded(HRESULT hrToOriginate, _In_ IErrorService* pErrorService) override;

    XUINT32 AddRef() override
    {
        return CReferenceCount::AddRef();
    }

    XUINT32 Release() override
    {
        return CReferenceCount::Release();
    }
};

#endif // JUPITER_CONTROL_H_
