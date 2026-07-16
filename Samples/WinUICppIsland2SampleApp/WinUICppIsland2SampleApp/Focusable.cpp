// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "Focusable.h"

using namespace winrt::Microsoft::UI::Input;
using namespace winrt::Microsoft::UI::Content;

// HwndFocusable
HwndFocusable::HwndFocusable(HWND hwnd)
{
    m_hwnd = hwnd;
}

HWND HwndFocusable::GetHwnd() const
{
    return m_hwnd;
}

InputFocusNavigationHost HwndFocusable::GetFocusNavigationHost() const
{
    return nullptr;
}

FocusNavigationResult HwndFocusable::NavigateFocus(FocusNavigationRequest)
{
    // HWND controls don't support focus navigation, only regular Win32 focus APIs
    ::SetFocus(m_hwnd);

    return FocusNavigationResult::Moved;
}

// ContentFocusable
ContentFocusable::ContentFocusable(IContentSiteBridge bridge)
{
    m_focusNavigationHost = InputFocusNavigationHost::GetForSiteBridge(bridge);
}

HWND ContentFocusable::GetHwnd() const
{
    // No HWNDs allowed
    return nullptr;
}

InputFocusNavigationHost ContentFocusable::GetFocusNavigationHost() const
{
    return m_focusNavigationHost;
}

FocusNavigationResult ContentFocusable::NavigateFocus(FocusNavigationRequest request)
{
    return m_focusNavigationHost.NavigateFocus(request);
}