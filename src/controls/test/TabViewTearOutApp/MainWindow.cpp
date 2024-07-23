#include "pch.h"

#include "App.h"
#include "MainWindow.h"
#include "DesktopWindow.h"
#include <Psapi.h>
#include "XamlUtil.h"

#include "MainPage.xaml.h"

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Graphics;

using namespace winrt::Microsoft::UI;
using namespace winrt::Microsoft::UI::Input;
using namespace winrt::Microsoft::UI::Windowing;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Automation;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::UI::Xaml::Markup;
using namespace winrt::Microsoft::UI::Xaml::Media;
using namespace winrt::TabViewTearOutApp;

#define MAX_LOADSTRING 100

HINSTANCE g_hInst = nullptr;

#ifdef MULTITHREADED_WINDOWS
std::vector<std::thread> g_childThreads;
#endif

HWND MainWindow::CreateNewWindow(int nCmdShow, winrt::com_array<IInspectable> const& stringList)
{
#ifdef MULTITHREADED_WINDOWS
    auto windowReadyEvent = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
    HWND newWindowHwnd = nullptr;

    auto createNewWindowThreadFunc = [nCmdShow, windowReadyEvent, stringList, &newWindowHwnd]()
        {
            winrt::init_apartment(winrt::apartment_type::single_threaded);

            // Start a DispatcherQueueController.  A DispatcherQueue must be running on the thread for XAML to work.
            auto dqc = winrt::Microsoft::UI::Dispatching::DispatcherQueueController::CreateOnCurrentThread();

            MainWindow newWindow(nCmdShow, stringList);
            newWindowHwnd = newWindow.GetHandle();
            ::SetEvent(windowReadyEvent);
            newWindow.Run();

            dqc.ShutdownQueue();
        };

    g_childThreads.push_back(std::thread(createNewWindowThreadFunc));
    WaitForSingleObject(windowReadyEvent, INFINITE);

    return newWindowHwnd;
#else
    auto newWindow = new MainWindow(nCmdShow, stringList);
    return newWindow->GetHandle();
#endif
}

void MainWindow::Run()
{
    HACCEL hAccelTable = LoadAccelerators(g_hInst, MAKEINTRESOURCE(IDC_WINDOWCLASS));
    m_returnValue = MessageLoop(hAccelTable);
}

MainWindow::MainWindow(int nCmdShow, winrt::com_array<IInspectable> const& stringList) noexcept
    : m_stringList(winrt::com_array<IInspectable>(stringList.cbegin(), stringList.cend()))
{
    WCHAR g_szTitle[MAX_LOADSTRING];
    WCHAR g_szWindowClass[MAX_LOADSTRING];

    LoadStringW(g_hInst, IDS_APP_TITLE, g_szTitle, MAX_LOADSTRING);
    LoadStringW(g_hInst, IDC_WINDOWCLASS, g_szWindowClass, MAX_LOADSTRING);

    WNDCLASSEXW wcex = {};

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = g_hInst;
    wcex.hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_LARGE));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = CreateSolidBrush(RGB(234, 246, 249));
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_WINDOWCLASS);
    wcex.lpszClassName = g_szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    WINRT_VERIFY(RegisterClassExW(&wcex));
    WINRT_ASSERT(!GetHandle());

    HWND hMainWnd = CreateWindowW(
        g_szWindowClass,
        g_szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        800,
        300,
        nullptr,
        nullptr,
        g_hInst,
        this);

    if (!hMainWnd)
    {
        winrt::check_hresult(E_FAIL);
        return;
    }

    if (stringList.empty())
    {
        ShowWindow(hMainWnd, nCmdShow);
        UpdateWindow(hMainWnd);
    }
}

[[nodiscard]] LRESULT MainWindow::MessageHandler(UINT const message, WPARAM const wParam, LPARAM const lParam) noexcept
{
    switch (message)
    {
        HANDLE_MSG(GetHandle(), WM_CREATE, OnCreate);
        HANDLE_MSG(GetHandle(), WM_CLOSE, OnClose);
        HANDLE_MSG(GetHandle(), WM_DESTROY, OnDestroy);
        HANDLE_MSG(GetHandle(), WM_ACTIVATE, OnActivate);
        HANDLE_MSG(GetHandle(), WM_SIZE, OnSize);
    }

    return base_type::MessageHandler(message, wParam, lParam);
}

bool MainWindow::OnCreate(HWND, LPCREATESTRUCT)
{
    s_windowCount++;
    winrt::TabViewTearOutApp::implementation::App::InitInstance();

    // If XAML's not already running on the thread, creating a XAML element before creating the DesktopWindowXamlSource will fail.
    m_desktopWindowXamlSource = CreateDesktopWindowsXamlSource(WS_TABSTOP);

    auto mainPage = winrt::make_self<winrt::TabViewTearOutApp::implementation::MainPage>();
    mainPage->Init(m_stringList);
    m_desktopWindowXamlSource.Content(*mainPage);

    m_desktopWindowXamlSource.SystemBackdrop(MicaBackdrop());

    RECT windowRect;
    ::GetWindowRect(m_hMainWnd, &windowRect);
    SizeXamlWindow(windowRect.right - windowRect.left, 200);

    return true;
}

void MainWindow::OnClose(HWND hwnd)
{
    m_desktopWindowXamlSource.Close();
    ::DestroyWindow(hwnd);
}

void MainWindow::OnDestroy(HWND)
{
    s_windowCount--;

    if (s_windowCount == 0)
    {
        ::PostQuitMessage(0);
    }
}

void MainWindow::OnActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized)
{
    UNREFERENCED_PARAMETER(hwnd);
    UNREFERENCED_PARAMETER(hwndActDeact);
    UNREFERENCED_PARAMETER(fMinimized);

    if (state == WA_ACTIVE)
    {
        RECT windowRect;
        ::GetWindowRect(m_hMainWnd, &windowRect);
        SizeXamlWindow(windowRect.right - windowRect.left, 200);
    }
}

void MainWindow::OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    UNREFERENCED_PARAMETER(hwnd);
    UNREFERENCED_PARAMETER(state);
    UNREFERENCED_PARAMETER(cy);

    SizeXamlWindow(cx, 200);
}

void MainWindow::SizeXamlWindow(int width, int height)
{
    SIZE margin = { 10, 10 };
    RectInt32 dwxsPosition = { margin.cx, margin.cy, width - margin.cx * 2, height };

    if (m_desktopWindowXamlSource)
    {
        m_desktopWindowXamlSource.SiteBridge().MoveAndResize(dwxsPosition);
    }
}
