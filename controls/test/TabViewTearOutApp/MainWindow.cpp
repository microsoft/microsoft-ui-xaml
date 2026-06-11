#include "pch.h"

#include "App.h"
#include "MainWindow.h"
#include "DesktopWindow.h"
#include <Psapi.h>
#include "XamlUtil.h"

#include "MainPage.xaml.h"

#include <Windows.UI.Composition.Interop.h>

namespace winrt
{
    using namespace winrt::Windows::Foundation;
    using namespace winrt::Windows::Foundation::Collections;
    using namespace winrt::Windows::Graphics;
    using namespace winrt::Windows::System;
    using namespace winrt::Windows::UI::Composition;

    using namespace winrt::Microsoft::UI;
    using namespace winrt::Microsoft::UI::Composition::SystemBackdrops;
    using namespace winrt::Microsoft::UI::Input;
    using namespace winrt::Microsoft::UI::Windowing;
    using namespace winrt::Microsoft::UI::Xaml;
    using namespace winrt::Microsoft::UI::Xaml::Automation;
    using namespace winrt::Microsoft::UI::Xaml::Controls;
    using namespace winrt::Microsoft::UI::Xaml::Input;
    using namespace winrt::Microsoft::UI::Xaml::Markup;
    using namespace winrt::Microsoft::UI::Xaml::Media;
    using namespace winrt::TabViewTearOutApp;
}

#define MAX_LOADSTRING 100

HINSTANCE g_hInst = nullptr;

#ifdef MULTITHREADED_WINDOWS
std::vector<std::thread> g_childThreads;
#endif

HWND MainWindow::CreateNewWindow(int nCmdShow, winrt::com_array<winrt::IInspectable> const& stringList)
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

MainWindow::MainWindow(int nCmdShow, _In_ winrt::com_array<winrt::IInspectable> const& documentList) noexcept
    : m_documentList(winrt::com_array<winrt::IInspectable>(documentList.cbegin(), documentList.cend()))
{
    WCHAR g_szTitle[MAX_LOADSTRING];
    WCHAR g_szWindowClass[MAX_LOADSTRING];

    LoadStringW(g_hInst, IDS_APP_TITLE, g_szTitle, MAX_LOADSTRING);
    LoadStringW(g_hInst, IDC_WINDOWCLASS, g_szWindowClass, MAX_LOADSTRING);

    WNDCLASSEXW wcex = {};

    wcex.cbSize = sizeof(WNDCLASSEX);
    auto titleBarColorref = RGB(234, 246, 249);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = g_hInst;
    wcex.hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_LARGE));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = CreateSolidBrush(titleBarColorref);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_WINDOWCLASS);
    wcex.lpszClassName = g_szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    WINRT_VERIFY(RegisterClassExW(&wcex));
    WINRT_ASSERT(!GetHandle());

    HWND hMainWnd = CreateWindowExW(
        WS_EX_COMPOSITED,
        g_szWindowClass,
        g_szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1000,
        300,
        nullptr,
        nullptr,
        g_hInst,
        this);

    m_titleBar = winrt::AppWindow::GetFromWindowId(winrt::GetWindowIdFromWindow(hMainWnd)).TitleBar();
    m_titleBar.ExtendsContentIntoTitleBar(true);

    auto titleBarColor = winrt::ColorHelper::FromArgb(255, GetRValue(titleBarColorref), GetGValue(titleBarColorref), GetBValue(titleBarColorref));

    m_titleBar.BackgroundColor(titleBarColor);
    m_titleBar.ButtonBackgroundColor(titleBarColor);
    m_titleBar.ButtonInactiveBackgroundColor(titleBarColor);

    WINRT_ASSERT(hMainWnd);

    if (documentList.empty())
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
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            CloseWindow();
        }
        break;
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
    mainPage->Init(m_documentList);
    m_mainPageLoadedRevoker = (*mainPage).Loaded(winrt::auto_revoke, { this, &MainWindow::OnPageLoaded });
    m_mainPageKeyDownRevoker = (*mainPage).KeyDown(winrt::auto_revoke, { this, &MainWindow::OnPageKeyDown });
    m_desktopWindowXamlSource.Content(*mainPage);
    m_desktopWindowXamlSource.SystemBackdrop(winrt::MicaBackdrop());

    SizeXamlWindow();

    return true;
}

void MainWindow::OnClose(HWND hwnd)
{
    RemoveDesktopWindowXamlSource(m_desktopWindowXamlSource);
    m_desktopWindowXamlSource = nullptr;

    m_mainPageKeyDownRevoker.revoke();

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
        SizeXamlWindow();
    }
}

void MainWindow::OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    UNREFERENCED_PARAMETER(hwnd);
    UNREFERENCED_PARAMETER(state);
    UNREFERENCED_PARAMETER(cx);
    UNREFERENCED_PARAMETER(cy);

    SizeXamlWindow();
}

void MainWindow::SizeXamlWindow()
{
    RECT clientRect;
    ::GetClientRect(m_hMainWnd, &clientRect);
    winrt::RectInt32 dwxsPosition = { clientRect.left, clientRect.top, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top };

    if (m_desktopWindowXamlSource)
    {
        m_desktopWindowXamlSource.SiteBridge().MoveAndResize(dwxsPosition);
    }

    // Since we have removed the built-in non-client area, we need to add back our own non-client area in order to allow users to drag the window
    // using the top of the window.
    auto nonClientPointerSource = winrt::InputNonClientPointerSource::GetForWindowId(winrt::GetWindowIdFromWindow(GetHandle()));
    auto captionRects = nonClientPointerSource.GetRegionRects(winrt::NonClientRegionKind::Caption);

    std::vector<winrt::RectInt32> captionRegions;

    std::for_each(captionRects.cbegin(), captionRects.cend(), [&](winrt::RectInt32 const& rect)
        {
            if (!m_captionRectSet || rect != m_captionRect)
            {
                captionRegions.push_back(rect);
            }
        });

    RECT windowRect;
    ::GetWindowRect(GetHandle(), &windowRect);
    int captionRectHeight = 0;

    if (auto tabViewListView = GetVisualChildByName(m_desktopWindowXamlSource.Content(), L"TabListView"))
    {
        if (auto tabViewListViewAsFE = tabViewListView.try_as<winrt::FrameworkElement>())
        {
            auto bottomRightPoint = tabViewListViewAsFE.TransformToVisual(nullptr).TransformPoint(winrt::Point{ static_cast<float>(tabViewListViewAsFE.ActualWidth()), static_cast<float>(tabViewListViewAsFE.ActualHeight()) });
            captionRectHeight = static_cast<int>(bottomRightPoint.Y);
        }
    }

    int captionButtonsWidth = 0;

    // We don't want our caption region to extend over the minimize/maximize/close buttons, so we'll subtract off their width.
    if (m_titleBar)
    {
        captionButtonsWidth = m_titleBar.RightInset();
    }

    if (IsZoomed(GetHandle()))
    {
        m_captionRect = { -8, -8, windowRect.right - windowRect.left - captionButtonsWidth, captionRectHeight + 8 };
    }
    else
    {
        m_captionRect = { 0, 0, windowRect.right - windowRect.left - captionButtonsWidth, captionRectHeight };
    }

    captionRegions.push_back(m_captionRect);

    nonClientPointerSource.SetRegionRects(winrt::NonClientRegionKind::Caption, captionRegions);
    m_captionRectSet = true;
}

void MainWindow::OnPageLoaded(winrt::IInspectable const& sender, winrt::IInspectable const& args)
{
    UNREFERENCED_PARAMETER(sender);
    UNREFERENCED_PARAMETER(args);

    // We'll add padding to the right of the tab view's tabs so they move out of the way when the caption buttons would otherwise be sized on top of them.
    auto rightContentPresenter = GetVisualChildByName(sender.as<winrt::DependencyObject>(), L"RightContentPresenter").as<winrt::ContentPresenter>();
    auto rightContentPresenterPadding = rightContentPresenter.Padding();
    rightContentPresenterPadding.Right += m_titleBar.RightInset();
    rightContentPresenter.Padding(rightContentPresenterPadding);

    SizeXamlWindow();
}

void MainWindow::OnPageKeyDown(winrt::IInspectable const& sender, winrt::KeyRoutedEventArgs const& args)
{
    UNREFERENCED_PARAMETER(sender);

    if (args.Key() == winrt::VirtualKey::Escape)
    {
        CloseWindow();
    }
}

void MainWindow::CloseWindow()
{
    PostMessageW(GetHandle(), WM_CLOSE, static_cast<WPARAM>(0), static_cast<LPARAM>(0));
}

winrt::DependencyObject MainWindow::GetVisualChildByName(winrt::DependencyObject const& root, winrt::hstring const& name)
{
    std::queue<winrt::DependencyObject> objectList;
    objectList.push(root);

    while (!objectList.empty())
    {
        auto current = objectList.front();
        objectList.pop();

        if (auto currentAsFE = current.try_as<winrt::FrameworkElement>())
        {
            if (currentAsFE.Name() == name)
            {
                return current;
            }
        }

        int childrenCount = winrt::VisualTreeHelper::GetChildrenCount(current);

        for (int i = 0; i < childrenCount; i++)
        {
            objectList.push(winrt::VisualTreeHelper::GetChild(current, i));
        }
    }

    return nullptr;
}
