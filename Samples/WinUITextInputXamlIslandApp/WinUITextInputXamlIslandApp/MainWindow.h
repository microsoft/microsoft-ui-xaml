#pragma once

#include "resource.h"
#include "DesktopWindow.h"
#include "ThreadManager.h"

class ThreadManager;

class MainWindow : public DesktopWindowT<MainWindow>, std::enable_shared_from_this<MainWindow>
{
public:
    MainWindow(HINSTANCE hInstance, int nCmdShow, ThreadManager* threadmngr) noexcept;
    [[nodiscard]] LRESULT MessageHandler(UINT const message, WPARAM const wParam, LPARAM const lParam) noexcept;

private:

    wil::unique_hwnd m_hWndXamlIslandUpper{ nullptr };
    wil::unique_hwnd m_hWndXamlIslandLower{ nullptr };

    winrt::Microsoft::UI::Xaml::Controls::StackPanel m_mainUserControlUpper{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::StackPanel m_mainUserControlLower{ nullptr };
   
    void InitInstance(HINSTANCE hInstance, int nCmdShow);
    static void RegisterWindowClass(HINSTANCE hInstance);

    bool OnCreate(HWND, LPCREATESTRUCT);
    void OnCommand(HWND, int id, HWND hwndCtl, UINT codeNotify);
    void OnDestroy(HWND hwnd);
    void OnResize(HWND, UINT state, int cx, int cy);
    void OnPaint(HWND);
    
    void OnXamlButtonClick(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);
    ThreadManager* m_pThreadMngr;
    HINSTANCE m_hInstance;
    int m_nCmdShow;

public:
    HWND m_hMainWnd;
};




