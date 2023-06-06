// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WebView2AutomationPeer.g.h"
#include "WebView2.h"
#include "UIAutomationCore.h"

MIDL_INTERFACE("865F5B88-6506-4E64-A4C5-4B7723650731")
IAutomationPeerHwndInterop : public IUnknown
{
public:
    virtual /* [propput] */ HRESULT STDMETHODCALLTYPE GetRawElementProviderSimple(
        /* [retval][out] */ _Outptr_opt_ IRawElementProviderSimple * *value) = 0;

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE IsCorrectPeerForHwnd(
                            HWND hwnd,
        /* [retval][out] */ _Out_ bool* value) = 0;
};

class WebView2AutomationPeer :
    public ReferenceTracker<WebView2AutomationPeer, winrt::implementation::WebView2AutomationPeerT, IAutomationPeerHwndInterop>
{
public:
    WebView2AutomationPeer(winrt::WebView2 const& owner);

    // IAutomationPeerOverrides
    winrt::hstring GetClassNameCore();
    winrt::AutomationControlType GetAutomationControlTypeCore();
    winrt::Rect GetBoundingRectangleCore();

    winrt::IInspectable GetFocusedElementCore();
    winrt::IInspectable NavigateCore(winrt::AutomationNavigationDirection direction);
    winrt::IInspectable GetElementFromPointCore(winrt::Point pointInWindowCoordinates);

    // IAutomationPeerHwndInterop
    HRESULT STDMETHODCALLTYPE GetRawElementProviderSimple(_Outptr_opt_ IRawElementProviderSimple** value);
    HRESULT STDMETHODCALLTYPE IsCorrectPeerForHwnd(HWND hwnd, _Out_ bool* value);

private:
    com_ptr<WebView2> GetImpl();
    bool InitProvider();

    com_ptr<IRawElementProviderSimple> m_provider;
    winrt::IInspectable m_providerWrapper;
};

CppWinRTActivatableClassWithBasicFactory(WebView2AutomationPeer)
