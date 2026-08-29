// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "WebView2AutomationPeer.h"

WebView2AutomationPeer::WebView2AutomationPeer(winrt::WebView2 const& owner)
    : ReferenceTracker(owner)
{}

com_ptr<WebView2> WebView2AutomationPeer::GetImpl()
{
    com_ptr<WebView2> impl;
    winrt::WebView2 webview = Owner().try_as<winrt::WebView2>();

    if (webview)
    {
        impl = winrt::get_self<WebView2>(webview)->get_strong();
    }

    return impl;
}

hstring WebView2AutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::WebView2>();
}

winrt::AutomationControlType WebView2AutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::Pane;
}

HRESULT WebView2AutomationPeer::GetRawElementProviderSimple(_Outptr_opt_ IRawElementProviderSimple** value)
{
    InitProvider();
    m_provider.copy_to(value);
    return S_OK;
}

HRESULT WebView2AutomationPeer::IsCorrectPeerForHwnd(HWND hwnd, _Out_ bool* value)
{
    *value = false;
    if (!InitProvider())
    {
        return S_OK;
    }

    auto hwndProvider = GetImpl()->GetProviderForHwnd(hwnd).try_as<IRawElementProviderSimple>();
    if (hwndProvider && hwndProvider == m_provider)
    {
        *value = true;
    }
    return S_OK;
}

bool WebView2AutomationPeer::InitProvider()
{
    if (!m_provider)
    {
        m_provider = GetImpl()->GetWebView2Provider().try_as<IRawElementProviderSimple>();
    }
    return !!m_provider;
}
