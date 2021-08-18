// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WebView2AutomationPeer.g.h"
#include "WebView2.h"
#include "UIAutomationCore.h"

class WebView2AutomationPeer :
    public ReferenceTracker<WebView2AutomationPeer, winrt::implementation::WebView2AutomationPeerT
#if WINUI3
    , IAutomationPeerHwndInterop
#endif
    >
{
public:
    WebView2AutomationPeer(winrt::WebView2 const& owner);

    // IAutomationPeerOverrides
    winrt::hstring GetClassNameCore();
    winrt::AutomationControlType GetAutomationControlTypeCore();

    winrt::IInspectable GetFocusedElementCore();
    winrt::IInspectable NavigateCore(winrt::AutomationNavigationDirection direction);
    winrt::IInspectable GetElementFromPointCore(winrt::Point pointInWindowCoordinates);

    // IAutomationPeerHwndInterop
#if WINUI3
    HRESULT STDMETHODCALLTYPE GetRawElementProviderSimple(_Outptr_opt_ IRawElementProviderSimple** value);
    HRESULT STDMETHODCALLTYPE IsCorrectPeerForHwnd(HWND hwnd, _Out_ bool* value);
#endif

private:
    com_ptr<WebView2> GetImpl();
    bool InitProvider();

    com_ptr<IRawElementProviderSimple> m_provider;
    winrt::IInspectable m_providerWrapper;
};

CppWinRTActivatableClassWithBasicFactory(WebView2AutomationPeer)
