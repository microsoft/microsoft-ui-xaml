// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"

#include "DesktopWindow.h"
#include "resource.h"
#include <inspectable.h>
#include <Microsoft.UI.Dispatching.Interop.h> // For ContentPreTranslateMessage

using namespace winrt;
using namespace winrt::Microsoft::UI;
using namespace winrt::Microsoft::UI::Content;
using namespace winrt::Microsoft::UI::Input;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Input;
using namespace winrt::Microsoft::UI::Xaml::Hosting;
using namespace winrt::Windows::System;


const auto static invalidReason = static_cast<FocusNavigationReason>(-1);

FocusNavigationReason GetReasonFromKey(WPARAM key)
{
    if (key == VK_TAB)
    {
        byte keyboardState[256] = {};
        WINRT_VERIFY(::GetKeyboardState(keyboardState));
        auto reason = (keyboardState[VK_SHIFT] & 0x80) ?
            FocusNavigationReason::Last :
            FocusNavigationReason::First;
        return reason;
    }
    if (key == VK_LEFT)
    {
        return FocusNavigationReason::Left;
    }
    if (key == VK_RIGHT)
    {
        return FocusNavigationReason::Right;
    }
    if (key == VK_UP)
    {
        return FocusNavigationReason::Up;
    }
    if (key == VK_DOWN)
    {
        return FocusNavigationReason::Down;
    }
    return invalidReason;
}

FocusNavigationReason GetReasonFromKey(InputKeyboardSource kbSource, VirtualKey key)
{
    if (key == VirtualKey::Tab)
    {
        auto reason = (kbSource.GetKeyState(VirtualKey::Shift) == VirtualKeyStates::Down) ?
            FocusNavigationReason::Left :
            FocusNavigationReason::Right;
        return reason;
    }
    if (key == VirtualKey::Left)
    {
        return FocusNavigationReason::Left;
    }
    if (key == VirtualKey::Right)
    {
        return FocusNavigationReason::Right;
    }
    if (key == VirtualKey::Up)
    {
        return FocusNavigationReason::Up;
    }
    if (key == VirtualKey::Down)
    {
        return FocusNavigationReason::Down;
    }
    return invalidReason;
}

// Given an index in our list of focusable elements and a starting place, navigate focus to the next one
FocusNavigationResult DesktopWindow::NavigateFocusHelper(int startingFocusableIndex, FocusNavigationRequest request)
{
    if (m_focusables.size() == 1)
    {
        return FocusNavigationResult::NoFocusableElements;
    }

    FocusNavigationReason reason = request.Reason();
    const int direction =
        (reason == FocusNavigationReason::First ||
            reason == FocusNavigationReason::Down ||
            reason == FocusNavigationReason::Right) ? 1 : -1;

    // Send to the next bridge or hwnd
    for (int i = 1; i < (int)m_focusables.size(); i++)
    {
        int nextIndex = (int)(startingFocusableIndex + (i * direction) + m_focusables.size()) % m_focusables.size();
        IFocusable* focusable = m_focusables[nextIndex];

        FocusNavigationResult result = focusable->NavigateFocus(request);

        // Focus APIs still under construction, it is returning the wrong thing here.
        // Assume focus navigation succeeded here despite return value.
        if (result == FocusNavigationResult::NotMoved || result == FocusNavigationResult::Moved)
        {
            return FocusNavigationResult::Moved;
        }
    }

    return FocusNavigationResult::NotMoved;
}

// This method is called from a ContentSiteBridge's FocusNavigationHost::DepartFocusRequested when
// a ContentIsland wants the Hosting App (DesktopWindow) to find another component to take focus.
// That might be another ContentIsland, a sibling HWND control, or the hosting app itself.
FocusNavigationResult DesktopWindow::NavigateFocus(InputFocusNavigationHost sender, FocusNavigationRequest request)
{
    // Find our sender
    for (int focusableIndex = 0; focusableIndex < (int)m_focusables.size(); focusableIndex++)
    {
        InputFocusNavigationHost focusNavigationHost = m_focusables[focusableIndex]->GetFocusNavigationHost();
        if (focusNavigationHost != nullptr)
        {
            if (focusNavigationHost == sender)
            {
                return NavigateFocusHelper(focusableIndex, request);
            }
        }
    }

    // We should have registered our ContentIsland as a focusable element, something went wrong.
    assert(false);
    return FocusNavigationResult::NotMoved;
}

// This method is operates on messages before they are dispatched to the destination WndProc
// It will only do this if one of our registered "focusable" HWNDs has focus.
bool DesktopWindow::NavigateFocus(MSG* msg)
{
    // When we were only operating on HWNDs, we could use the Win32 API
    //     ::IsDialogMessage(m_hMainWnd, msg) != 0;
    // to automatically set focus to the next HWND tab stop. And while this technially works with
    // ContentIslands, it is intercepting that keyboard message before the ContentIsland gets a
    // chance to do FocusNavigation, which provides more navigation information than that Win32
    // ::IsDialogMessage() or ::SetFocus() call.

    if (msg->message == WM_KEYDOWN)
    {
        // Find currently focused hwnd
        HWND currentFocus = ::GetFocus();
        int currentFocusableIndex = -1;
        for (int i = 0; i < (int)m_focusables.size(); i++)
        {
            if (m_focusables[i]->GetHwnd() == currentFocus)
            {
                currentFocusableIndex = i;
                break;
            }
        }

        // If the object in focus was a Win32 control, pass focus to the next one
        if (currentFocusableIndex != -1 || currentFocus == m_hMainWnd)
        {
            FocusNavigationReason reason = GetReasonFromKey(msg->wParam);
            if (reason != invalidReason)
            {
                FocusNavigationRequest request = FocusNavigationRequest::Create(reason);
                if (NavigateFocusHelper(currentFocusableIndex, request) == FocusNavigationResult::NotMoved)
                {
                    // This message was processed, don't send it to the destination WndProc
                    return true;
                }
            }
        }
    }

    // Otherwise we will let the message go the HWND (possibly a ContentIsland) for processing
    return false;
}

int DesktopWindow::MessageLoop(HACCEL hAccelTable)
{
    MSG msg = {};
    
    while (GetMessage(&msg, nullptr, 0, 0))
    {        
        if (!ContentPreTranslateMessage(&msg) && !TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            if (!NavigateFocus(&msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    return (int)msg.wParam;
}

void DesktopWindow::OnNCCreate(HWND const window, LPARAM const lparam) noexcept
{
    auto cs = reinterpret_cast<CREATESTRUCT*>(lparam);
    auto that = static_cast<DesktopWindow*>(cs->lpCreateParams);
    WINRT_ASSERT(that);
    WINRT_ASSERT(!that->GetHandle());
    that->m_hMainWnd = window;
    SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(that));
}

XamlIsland DesktopWindow::CreateXamlIsland()
{
    XamlIsland xamlIsland = winrt::Microsoft::UI::Xaml::XamlIsland{};

    m_xamlIslands.push_back(xamlIsland);

    return xamlIsland;
}

void DesktopWindow::CreateFocusNavigationHost(winrt::Microsoft::UI::Content::IContentSiteBridge bridge)
{
    InputFocusNavigationHost focusNavigationHost = InputFocusNavigationHost::GetForSiteBridge(bridge);
    focusNavigationHost.DepartFocusRequested(
        [this](InputFocusNavigationHost const& sender, FocusNavigationRequestEventArgs const& args) {
            // Navigate focus through this desktop window.
            args.Result(NavigateFocus(sender, args.Request()));
        });
}

UIElement LoadXamlControl(uint32_t id)
{
    auto rc = ::FindResource(nullptr, MAKEINTRESOURCE(id), MAKEINTRESOURCE(XAMLRESOURCE));
    if (!rc)
    {
        winrt::check_hresult(HRESULT_FROM_WIN32(GetLastError()));
    }
    HGLOBAL rcData = ::LoadResource(nullptr, rc);
    if (!rcData)
    {
        winrt::check_hresult(HRESULT_FROM_WIN32(GetLastError()));
    }
    auto pData = static_cast<wchar_t*>(::LockResource(rcData));
    auto content = Markup::XamlReader::Load(winrt::get_abi(pData));
    auto uiElement = content.as<UIElement>();
    return uiElement;
}
