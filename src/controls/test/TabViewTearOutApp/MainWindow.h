#pragma once

#include "resource.h"
#include "DesktopWindow.h"

// Uncomment this to enable multi-threaded windows (not currently supported by LiftedIXP - see deliverable 48456352 for the work needed before we can enable it).
//#define MULTITHREADED_WINDOWS

struct Win32Button
{
    Win32Button(HWND hwnd, std::function<void(HWND)> func) : m_hwnd(hwnd), m_func(func) {}

    HWND m_hwnd;
    std::function<void(HWND)> m_func;
};

class MainWindow : public DesktopWindowT<MainWindow>
{
public:
    static HWND CreateNewWindow(int nCmdShow, winrt::com_array<winrt::Windows::Foundation::IInspectable> const& stringList);

    void Run();

    MainWindow(int nCmdShow, _In_ winrt::com_array<winrt::Windows::Foundation::IInspectable> const& stringList) noexcept;

    [[nodiscard]] LRESULT MessageHandler(UINT const message, WPARAM const wParam, LPARAM const lParam) noexcept;

    int GetReturnValue() const { return m_returnValue; }

private:
    bool OnCreate(HWND, LPCREATESTRUCT);
    void OnClose(HWND hwnd);
    void OnDestroy(HWND hwnd);
    void OnActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized);
    void OnSize(HWND hwnd, UINT state, int cx, int cy);

    void SizeXamlWindow(int width, int height);

    winrt::Microsoft::UI::Xaml::Hosting::DesktopWindowXamlSource m_desktopWindowXamlSource{ nullptr };
    winrt::com_array<winrt::Windows::Foundation::IInspectable> m_stringList{};
    
    int m_returnValue{ 0 };
};
