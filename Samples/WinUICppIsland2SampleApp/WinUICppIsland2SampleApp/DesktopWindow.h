// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "Focusable.h"

class DesktopWindow
{
public:
    int MessageLoop(HACCEL hAccelTable);

private:
    bool NavigateFocus(MSG* msg);
    winrt::Microsoft::UI::Input::FocusNavigationResult NavigateFocus(winrt::Microsoft::UI::Input::InputFocusNavigationHost sender, winrt::Microsoft::UI::Input::FocusNavigationRequest request);
    winrt::Microsoft::UI::Input::FocusNavigationResult NavigateFocusHelper(int currentFocusableIndex, winrt::Microsoft::UI::Input::FocusNavigationRequest request);

protected:

    winrt::Microsoft::UI::Xaml::XamlIsland CreateXamlIsland();

    void AddFocusableHwnd(HWND hwnd) { m_focusables.push_back(new HwndFocusable(hwnd)); };
    void AddFocusableBridge(winrt::Microsoft::UI::Content::IContentSiteBridge bridge) {
        m_focusables.push_back(new ContentFocusable(bridge));
        CreateFocusNavigationHost(bridge);
    };

    HWND GetHandle() const
    {
        return m_hMainWnd;
    }

    static void OnNCCreate(HWND const window, LPARAM const lparam) noexcept;

    bool m_shouldQuitMessagePump { false };

    HWND m_hMainWnd { NULL };
    winrt::guid lastFocusRequestId{};
    std::vector<winrt::Microsoft::UI::Xaml::XamlIsland> m_xamlIslands;
    std::vector<IFocusable*> m_focusables;

private:
    void CreateFocusNavigationHost(winrt::Microsoft::UI::Content::IContentSiteBridge);

};

template <typename T>
class DesktopWindowT : public DesktopWindow
{
protected:
    using base_type = DesktopWindowT<T>;

    static LRESULT __stdcall WndProc(HWND const window, UINT const message, WPARAM const wparam, LPARAM const lparam) noexcept
    {
        WINRT_ASSERT(window);

        if (WM_NCCREATE == message)
        {
            DesktopWindow::OnNCCreate(window, lparam);
        }
        else if (T * that = GetThisFromHandle(window))
        {
            return that->MessageHandler(message, wparam, lparam);
        }

        return DefWindowProc(window, message, wparam, lparam);
    }

    [[nodiscard]] LRESULT MessageHandler(UINT const message, WPARAM const wParam, LPARAM const lParam) noexcept
    {
        switch (message)
        {
            HANDLE_MSG(GetHandle(), WM_DESTROY, OnDestroy);
            HANDLE_MSG(GetHandle(), WM_ACTIVATE, OnActivate);
            HANDLE_MSG(GetHandle(), WM_SETFOCUS, OnSetFocus);
        }
        return DefWindowProc(GetHandle(), message, wParam, lParam);
    }

    void OnDestroy(HWND hwnd)
    {
        // Sometimes we make this Window a child of a top-level window.  In that case, don't post the Quit message because
        // we don't want closing this window to quit the whole thread.
        if (GetParent(hwnd) == nullptr)
        {
            ::PostQuitMessage(0);
        }
    }

private:

    void OnActivate(HWND, UINT state, HWND hwndActDeact, BOOL fMinimized)
    {
        UNREFERENCED_PARAMETER(hwndActDeact);
        UNREFERENCED_PARAMETER(fMinimized);

        if (state == WA_INACTIVE)
        {
            m_hwndLastFocus = GetFocus();
        }
    }

    void OnSetFocus(HWND, HWND hwndOldFocus)
    {
        UNREFERENCED_PARAMETER(hwndOldFocus);

        if (m_hwndLastFocus) 
        {
            SetFocus(m_hwndLastFocus);
        }
    }

    static T* GetThisFromHandle(HWND const window) noexcept
    {
        return reinterpret_cast<T*>(GetWindowLongPtr(window, GWLP_USERDATA));
    }

    HWND m_hwndLastFocus = nullptr;
};

winrt::Microsoft::UI::Xaml::UIElement LoadXamlControl(uint32_t id);

template<typename T>
T LoadXamlControl(uint32_t id)
{
    const auto uiElement = LoadXamlControl(id);
    return uiElement.as<T>();
}
