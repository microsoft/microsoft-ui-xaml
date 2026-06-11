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
    static HWND CreateNewWindow(int nCmdShow, winrt::com_array<winrt::Windows::Foundation::IInspectable> const& documentList);

    void Run();

    MainWindow(int nCmdShow, _In_ winrt::com_array<winrt::Windows::Foundation::IInspectable> const& documentList) noexcept;

    [[nodiscard]] LRESULT MessageHandler(UINT const message, WPARAM const wParam, LPARAM const lParam) noexcept;

    int GetReturnValue() const { return m_returnValue; }

private:
    bool OnCreate(HWND, LPCREATESTRUCT);
    void OnClose(HWND hwnd);
    void OnDestroy(HWND hwnd);
    void OnActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized);
    void OnSize(HWND hwnd, UINT state, int cx, int cy);

    void SizeXamlWindow();

    void OnPageLoaded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::Foundation::IInspectable const& args);
    void OnPageKeyDown(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const& args);
    void CloseWindow();

    winrt::Microsoft::UI::Xaml::DependencyObject GetVisualChildByName(winrt::Microsoft::UI::Xaml::DependencyObject const& root, winrt::hstring const& name);

    winrt::Microsoft::UI::Windowing::AppWindowTitleBar m_titleBar{ nullptr };
    winrt::Microsoft::UI::Xaml::Hosting::DesktopWindowXamlSource m_desktopWindowXamlSource{ nullptr };
    winrt::Microsoft::UI::Xaml::FrameworkElement::Loaded_revoker m_mainPageLoadedRevoker{};
    winrt::Microsoft::UI::Xaml::UIElement::KeyDown_revoker m_mainPageKeyDownRevoker{};
    winrt::com_array<winrt::Windows::Foundation::IInspectable> m_documentList{};
    LONG m_clientWidth{ -1 };

    winrt::Windows::Graphics::RectInt32 m_captionRect{};
    bool m_captionRectSet{ false };
    
    int m_returnValue{ 0 };
};
