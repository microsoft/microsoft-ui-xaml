// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

class DesktopWindow
{
public:
    int MessageLoop(HACCEL hAccelTable);

private:
    winrt::Microsoft::UI::Xaml::Hosting::DesktopWindowXamlSource GetFocusedIsland();
    void OnTakeFocusRequested(winrt::Microsoft::UI::Xaml::Hosting::DesktopWindowXamlSource const& sender, winrt::Microsoft::UI::Xaml::Hosting::DesktopWindowXamlSourceTakeFocusRequestedEventArgs const& args);
    winrt::Microsoft::UI::Xaml::Hosting::DesktopWindowXamlSource GetNextFocusedIsland(MSG* msg);
    bool NavigateFocus(MSG* msg);

protected:

    winrt::Microsoft::UI::Xaml::Hosting::DesktopWindowXamlSource CreateDesktopWindowsXamlSource(DWORD dwStylet);
    void ClearXamlSources();
    void RemoveDesktopWindowXamlSource(winrt::Microsoft::UI::Xaml::Hosting::DesktopWindowXamlSource dwxs);

    HWND GetHandle() const
    {
        return m_hMainWnd;
    }

    static void OnNCCreate(HWND const window, LPARAM const lparam) noexcept;

    bool m_shouldQuitMessagePump { false };

    HWND m_hMainWnd { NULL };
    winrt::guid lastFocusRequestId{};
    std::vector<winrt::Microsoft::UI::Xaml::Hosting::DesktopWindowXamlSource::TakeFocusRequested_revoker> m_takeFocusEventRevokers;
    std::vector<winrt::Microsoft::UI::Xaml::Hosting::DesktopWindowXamlSource> m_xamlSources;
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
