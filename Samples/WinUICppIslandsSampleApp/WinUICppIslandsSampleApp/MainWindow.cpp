// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"

#include "App.h"
#include "MainWindow.h"
#include "DesktopWindow.h"
#include <Psapi.h>
#include "XamlUtil.h"

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Graphics;

using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::UI::Xaml::Media;

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst = nullptr; // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// This pointer needs to be global because the Application object is global, there can only be one in a process.
winrt::com_ptr< winrt::WinUICppIslandsSampleApp::implementation::App> g_app{ nullptr };

[[nodiscard]] int APIENTRY MainWindow::Run(_In_ HINSTANCE hInstance, _In_ int nCmdShow)
{
    winrt::init_apartment(winrt::apartment_type::single_threaded);

    // Start a DispatcherQueueController.  A DispatcherQueue must be running on the thread for XAML to work.
    auto dqc = winrt::Microsoft::UI::Dispatching::DispatcherQueueController::CreateOnCurrentThread();

    MainWindow mainWindow(hInstance, nCmdShow);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ISLANDSAMPLEWINUI3));

    int retValue = mainWindow.MessageLoop(hAccelTable);

    // Calling DispatcherQueueController.ShutdownQueue will shutdown XAML on the thread, and automatically close some
    // other WinAppSDK objects too.
    dqc.ShutdownQueue();

    return retValue;
}

MainWindow::MainWindow(HINSTANCE hInstance, int nCmdShow, HWND parentHWnd) noexcept
{
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_ISLANDSAMPLEWINUI3, szWindowClass, MAX_LOADSTRING);

    WNDCLASSEXW wcex = {};

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ISLANDSAMPLEWINUI3));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = CreateSolidBrush(RGB(0, 255, 0));  // Green background to make desktop Acrylic context menus more apparent
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_ISLANDSAMPLEWINUI3);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    WINRT_VERIFY(RegisterClassExW(&wcex));
    WINRT_ASSERT(!GetHandle());

    InitInstance(hInstance, nCmdShow, parentHWnd);
}

MainWindow::~MainWindow()
{
    if (m_dispatcherQueueShutdownStartingToken)
    {
        if (auto dq = winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread())
        {
            dq.ShutdownStarting(m_dispatcherQueueShutdownStartingToken);
            m_dispatcherQueueShutdownStartingToken = {};
        }
    }
}

[[nodiscard]] LRESULT MainWindow::MessageHandler(UINT const message, WPARAM const wParam, LPARAM const lParam) noexcept
{
    switch (message)
    {
        HANDLE_MSG(GetHandle(), WM_CREATE, OnCreate);
        HANDLE_MSG(GetHandle(), WM_COMMAND, OnCommand);
        HANDLE_MSG(GetHandle(), WM_DESTROY, OnDestroy);
        HANDLE_MSG(GetHandle(), WM_SIZE, OnResize);
        HANDLE_MSG(GetHandle(), WM_TIMER, OnTimer);
        //HANDLE_MSG(GetHandle(), WM_PAINT, OnPaint);
    }

    return base_type::MessageHandler(message, wParam, lParam);
}

void MainWindow::InitInstance(HINSTANCE hInstance, int nCmdShow, HWND parentHWnd)
{
    hInst = hInstance; // Store instance handle in our global variable

    const DWORD style =
        ((parentHWnd == nullptr) ? (WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN) : (WS_CHILD));

    HWND hMainWnd = CreateWindowW(szWindowClass,
                                  szTitle,
                                  style,
                                  CW_USEDEFAULT,
                                  0,
                                  CW_USEDEFAULT,
                                  0,
                                  parentHWnd,
                                  nullptr,
                                  hInstance,
                                  this);

    if (!hMainWnd)
    {
        winrt::check_hresult(E_FAIL);
        return;
    }

    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);

    SetFocus(hMainWnd);
}

void MainWindow::CreateButton(const wchar_t* name, DWORD id, std::function<void(HWND)> func, int x, int y, int w, int h)
{
    ButtonCommand newButton;
    newButton.name = name;
    newButton.func = func;
    newButton.id = id;
    newButton.hwnd = CreateWindow(
        L"BUTTON",
        name,
        WS_TABSTOP | WS_VISIBLE | WS_CHILD,
        x,
        y,
        w,
        h,
        GetHandle(),     // Parent window
        reinterpret_cast<HMENU>(static_cast<size_t>(newButton.id)),  // Identifier
        (HINSTANCE)GetWindowLongPtr(GetHandle(), GWLP_HINSTANCE),
        NULL);
    SendMessage(newButton.hwnd, WM_SETFONT, (LPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    m_buttons.push_back(newButton);
}

bool MainWindow::OnCreate(HWND, LPCREATESTRUCT)
{
    m_label = CreateWindow(L"STATIC", L"Starting up...", WS_CHILD | WS_VISIBLE, 10, 10, 500, 20, m_hMainWnd, 0, NULL, NULL);
    SendMessage(m_label, WM_SETFONT, (LPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    ::SetTimer(m_hMainWnd, 101, 500, nullptr);

    static bool s_createRTL = false; // whether to create the next DesktopWindowXamlSource content as RTL
    CreateButton(L"Create DesktopWindowXamlSource", IDM_CREATE_DWXS, [this](HWND) {

        try
        {
            // Start App object if needed
            if (!XamlUtil::IsXamlRunningInProcess() && g_app == nullptr)
            {
                winrt::make<winrt::WinUICppIslandsSampleApp::implementation::App>().as(g_app);
            }

            size_t position = m_xamlSources.size();

            // If XAML's not already running on the thread, creating a XAML element before creating the DesktopWindowXamlSource will fail.
            auto dwxs = CreateDesktopWindowsXamlSource(WS_TABSTOP);

            auto stackPanel = StackPanel();
            dwxs.Content(stackPanel);
            dwxs.SystemBackdrop(MicaBackdrop());
            if (s_createRTL)
            {
                stackPanel.FlowDirection(FlowDirection::RightToLeft);
                dwxs.ShouldConstrainPopupsToWorkArea(false);
            }
            s_createRTL = !s_createRTL;

            auto token = stackPanel.Unloaded([](const IInspectable&, const RoutedEventArgs&) {
                ::OutputDebugStringW(L">>> StackPanel unloaded.\n");
                });

            auto height = 450;
            auto width = 900;
            RectInt32 rect = { 10, 80 + static_cast<int>((height+5) * position), width, height };
            dwxs.SiteBridge().MoveAndResize(rect);

            auto textBox = TextBox();
            textBox.Text(L"Text Edit");

            auto comboBox = ComboBox();
            comboBox.Items().Append(winrt::box_value(L"Item 1"));
            comboBox.Items().Append(winrt::box_value(L"Item 2"));

            stackPanel.Margin(ThicknessHelper::FromUniformLength(5.0));

            auto createModalWindowButton = Button();
            createModalWindowButton.Content(winrt::box_value(L"Create Modal Window"));
            createModalWindowButton.Click([this,dwxs](auto, auto) {
                ::OutputDebugStringW(L">>> createModalWindowButton clicked.\n");
                ::DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), m_hMainWnd, &MainWindow::DialogBoxProc, 0);
                });

            auto closeDwxsButton = Button();
            closeDwxsButton.Content(winrt::box_value(L"Close this DesktopWindowXamlSource"));
            closeDwxsButton.Click([this, dwxs](auto, auto) {
                this->RemoveDesktopWindowXamlSource(dwxs);
                });

            auto nestedPumpButton = Button();
            nestedPumpButton.Content(winrt::box_value(L"Nested pump"));
            nestedPumpButton.Click([nestedPumpButton](auto, auto) {
                nestedPumpButton.Content(winrt::box_value(L"Running..."));

                const auto startTick = ::GetTickCount();
                ::OutputDebugStringW(L">>> Run nested pump for 5 seconds.\n");
                MSG msg{};
                while (GetMessage(&msg, nullptr, 0, 0))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);

                    const auto now = ::GetTickCount();
                    if (now - startTick > 5000 || now < startTick /* mindful of overflow :) */)
                    {
                        break;
                    }
                }
                ::OutputDebugStringW(L">>> End nested pump.\n");
                nestedPumpButton.Content(winrt::box_value(L"Nested pump"));
                });

            auto textBlock = TextBlock();
            textBlock.Text(L"Xaml Island");
            textBlock.FontSize(24.0);

            stackPanel.Children().Append(textBlock);
            stackPanel.Children().Append(textBox);
            stackPanel.Children().Append(comboBox);

            auto timerStackPanel = StackPanel();
            timerStackPanel.Orientation(Orientation::Horizontal);
            stackPanel.Children().Append(timerStackPanel);

            auto timerButton = Button();
            timerButton.Content(winrt::box_value(L"500ms timer"));
            timerButton.Click([this, dwxs](auto, auto) {
                auto timer = DispatcherTimer();
                timer.Interval(TimeSpan(5000000L));
                timer.Tick([this](auto, auto) {
                    // do nothing
                    });
                timer.Start();
                });
            timerStackPanel.Children().Append(timerButton);

            auto timer2Button = Button();
            timer2Button.Content(winrt::box_value(L"0s timer"));
            timer2Button.Click([this, dwxs](auto, auto) {
                auto timer = DispatcherTimer();
                timer.Interval(TimeSpan(0));
                timer.Tick([this](auto, auto) {
                    // do nothing
                    });
                timer.Start();
                });
            timerStackPanel.Children().Append(timer2Button);

            auto timer3Button = Button();
            timer3Button.Content(winrt::box_value(L"500ms timer, no Tick handler"));
            timer3Button.Click([this, dwxs](auto, auto) {
                auto timer = DispatcherTimer();
                timer.Interval(TimeSpan(5000000L));
                timer.Start();
                });
            timerStackPanel.Children().Append(timer3Button);


            auto commandBarFlyout1 = winrt::Microsoft::UI::Xaml::Markup::XamlReader::Load(L" \
                <CommandBarFlyout xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Placement='Right'> \
                    <AppBarButton Label='Save' Icon='Save' /> \
                    <AppBarButton Label='Save' Icon='Save' /> \
                    <AppBarButton Label='Save' Icon='Save' /> \
                    <AppBarButton Label='Save' Icon='Save' /> \
                    <AppBarButton Label='Save' Icon='Save' /> \
                    <AppBarButton Label='Save' Icon='Save' /> \
                    <AppBarButton Label='Save' Icon='Save' /> \
                    <AppBarButton Label='Save' Icon='Save' /> \
                    <AppBarButton Label='Save' Icon='Save' /> \
                    <AppBarButton Label='Save' Icon='Save' /> \
                    <AppBarButton Label='Save' Icon='Save' /> \
                    <AppBarButton Label='Save' Icon='Save' /> \
                    <CommandBarFlyout.SecondaryCommands> \
                        <AppBarButton Label='Resize'> \
                            <AppBarButton.Flyout> \
                                <MenuFlyout> \
                                    <MenuFlyoutItem Text='First' /> \
                                    <MenuFlyoutItem Text='Second' /> \
                                    <MenuFlyoutSeparator /> \
                                    <MenuFlyoutItem Text='Third' /> \
                                    <MenuFlyoutSubItem Text='SubMenu'> \
                                        <MenuFlyoutItem Text='One'/> \
                                        <MenuFlyoutItem Text='Two'/> \
                                        <MenuFlyoutItem Text='Three'/> \
                                    </MenuFlyoutSubItem> \
                                </MenuFlyout> \
                            </AppBarButton.Flyout> \
                        </AppBarButton> \
                        <AppBarButton Label='Resize' /> \
                        <AppBarButton Label='Resize' /> \
                        <AppBarButton Label='Resize' /> \
                        <AppBarButton Label='Resize' /> \
                        <AppBarButton Label='Resize' /> \
                        <AppBarButton Label='Resize' /> \
                        <AppBarButton Label='Resize' /> \
                        <AppBarButton Label='Resize' /> \
                        <AppBarButton Label='Resize' /> \
                        <AppBarButton Label='Resize' /> \
                    </CommandBarFlyout.SecondaryCommands> \
                </CommandBarFlyout>").as<CommandBarFlyout>();
            commandBarFlyout1.ShouldConstrainToRootBounds(true);

            auto commandBarFlyoutButton1 = winrt::Microsoft::UI::Xaml::Markup::XamlReader::Load(L" \
                <Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Content='Inline CommandBarFlyout' />").as<Button>();
            commandBarFlyoutButton1.Click([this, commandBarFlyout1, commandBarFlyoutButton1](auto, auto)
            {
                auto myOption = winrt::Microsoft::UI::Xaml::Controls::Primitives::FlyoutShowOptions();
                myOption.ShowMode(winrt::Microsoft::UI::Xaml::Controls::Primitives::FlyoutShowMode::Transient);
                commandBarFlyout1.ShowAt(commandBarFlyoutButton1, myOption);
            });

            auto commandBarFlyout2 = winrt::Microsoft::UI::Xaml::Markup::XamlReader::Load(L" \
                <CommandBarFlyout xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Placement='Right'> \
                    <AppBarButton Label='Save' Icon='Save' /> \
                    <AppBarButton Label='Save' Icon='Save' /> \
                    <AppBarButton Label='Save' Icon='Save' /> \
                    <AppBarButton Label='Save' Icon='Save' /> \
                    <AppBarButton Label='Save' Icon='Save' /> \
                    <AppBarButton Label='Save' Icon='Save' /> \
                    <AppBarButton Label='Save' Icon='Save' /> \
                    <AppBarButton Label='Save' Icon='Save' /> \
                    <AppBarButton Label='Save' Icon='Save' /> \
                    <AppBarButton Label='Save' Icon='Save' /> \
                    <AppBarButton Label='Save' Icon='Save' /> \
                    <AppBarButton Label='Save' Icon='Save' /> \
                    <CommandBarFlyout.SecondaryCommands> \
                        <AppBarButton Label='Resize'> \
                            <AppBarButton.Flyout> \
                                <MenuFlyout> \
                                    <MenuFlyoutItem Text='First' /> \
                                    <MenuFlyoutItem Text='Second' /> \
                                    <MenuFlyoutSeparator /> \
                                    <MenuFlyoutItem Text='Third' /> \
                                    <MenuFlyoutSubItem Text='SubMenu'> \
                                        <MenuFlyoutItem Text='One'/> \
                                        <MenuFlyoutItem Text='Two'/> \
                                        <MenuFlyoutItem Text='Three'/> \
                                    </MenuFlyoutSubItem> \
                                </MenuFlyout> \
                            </AppBarButton.Flyout> \
                        </AppBarButton> \
                        <AppBarButton Label='Resize' /> \
                        <AppBarButton Label='Resize' /> \
                        <AppBarButton Label='Resize' /> \
                        <AppBarButton Label='Resize' /> \
                        <AppBarButton Label='Resize' /> \
                        <AppBarButton Label='Resize' /> \
                        <AppBarButton Label='Resize' /> \
                        <AppBarButton Label='Resize' /> \
                        <AppBarButton Label='Resize' /> \
                        <AppBarButton Label='Resize' /> \
                    </CommandBarFlyout.SecondaryCommands> \
                </CommandBarFlyout>").as<CommandBarFlyout>();

            auto commandBarFlyoutButton2 = winrt::Microsoft::UI::Xaml::Markup::XamlReader::Load(L" \
                <Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Content='Windowed CommandBarFlyout' />").as<Button>();
            commandBarFlyoutButton2.Click([this, commandBarFlyout2, commandBarFlyoutButton2](auto, auto)
            {
                auto myOption = winrt::Microsoft::UI::Xaml::Controls::Primitives::FlyoutShowOptions();
                myOption.ShowMode(winrt::Microsoft::UI::Xaml::Controls::Primitives::FlyoutShowMode::Transient);
                commandBarFlyout2.ShowAt(commandBarFlyoutButton2, myOption);
            });

            auto horizStackPanel = StackPanel();
            horizStackPanel.Orientation(Orientation::Horizontal);
            horizStackPanel.Children().Append(commandBarFlyoutButton1);
            horizStackPanel.Children().Append(commandBarFlyoutButton2);
            stackPanel.Children().Append(horizStackPanel);

            auto menubutton = winrt::Microsoft::UI::Xaml::Markup::XamlReader::Load(L" \
                <Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Content='Menu Button'> \
                    <Button.Flyout> \
                        <MenuFlyout> \
                            <MenuFlyoutItem Text='First' /> \
                            <MenuFlyoutItem Text='Second' /> \
                            <MenuFlyoutSeparator /> \
                            <MenuFlyoutItem Text='Third' /> \
                            <MenuFlyoutSubItem Text='SubMenu'> \
                                <MenuFlyoutItem Text='One'/> \
                                <MenuFlyoutItem Text='Two'/> \
                                <MenuFlyoutItem Text='Three'/> \
                                <MenuFlyoutSubItem Text='SubMenu-b'> \
                                    <MenuFlyoutItem Text='One-b'/> \
                                    <MenuFlyoutItem Text='Two-b'/> \
                                    <MenuFlyoutItem Text='Three-b'/> \
                                </MenuFlyoutSubItem> \
                            </MenuFlyoutSubItem> \
                        </MenuFlyout> \
                    </Button.Flyout> \
                    <Button.ContextFlyout> \
                        <MenuFlyout> \
                            <MenuFlyoutItem Text='Context 1' /> \
                            <MenuFlyoutItem Text='Context 2' /> \
                            <MenuFlyoutSeparator /> \
                            <MenuFlyoutItem Text='Context 3' /> \
                        </MenuFlyout> \
                    </Button.ContextFlyout> \
                </Button>").as<Button>();
            stackPanel.Children().Append(menubutton);

            horizStackPanel = StackPanel();
            horizStackPanel.Orientation(Orientation::Horizontal);
            horizStackPanel.Children().Append(closeDwxsButton);
            horizStackPanel.Children().Append(createModalWindowButton);
            horizStackPanel.Children().Append(nestedPumpButton);
            stackPanel.Children().Append(horizStackPanel);

            auto dq = winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread();

            if (!m_dispatcherQueueShutdownStartingToken)
            {
                m_dispatcherQueueShutdownStartingToken = dq.ShutdownStarting(
                    [this](const winrt::Microsoft::UI::Dispatching::DispatcherQueue&, const winrt::Microsoft::UI::Dispatching::DispatcherQueueShutdownStartingEventArgs&)
                    {
                        ::OutputDebugString(L">>> DispatcherQueue.ShutdownStarting fired\n");
                    });
            }
        }
        catch (const winrt::hresult_error& ex)
        {
            wchar_t str[200];
            ::StringCchPrintf(str, ARRAYSIZE(str), L"Error: 0x%x", ex.code().value);
            ::MessageBox(GetHandle(), str, L"Error", MB_OK);
        }
        }, 10, 40, 200, 30);

    CreateButton(L"More...", 208, [this](HWND hwnd) {
        // Show a context menu with some interesting options.
        HMENU contextMenu = ::CreatePopupMenu();

        const bool startAppEnabled = (g_app == nullptr && !XamlUtil::IsXamlRunningInProcess());
        const bool releaseAppEnabled = (g_app != nullptr);
        ::AppendMenuW(contextMenu, MF_STRING | (startAppEnabled ? 0 : MF_GRAYED), IDM_START_APP, L"Create Xaml Application object");
        ::AppendMenuW(contextMenu, MF_STRING | (releaseAppEnabled ? 0 : MF_GRAYED), IDM_RELEASE_APP, L"Release Xaml Application object");
        ::AppendMenuW(contextMenu, MF_SEPARATOR, 0, nullptr);
        ::AppendMenuW(contextMenu, MF_STRING, IDM_CREATE_NEW_THREAD, L"Create new window on new thread");
        ::AppendMenuW(contextMenu, MF_STRING, IDM_CREATE_MODAL_WINDOW, L"Create modal window on same thread");
        ::AppendMenuW(contextMenu, MF_SEPARATOR, 0, nullptr);
        ::AppendMenuW(contextMenu, MF_STRING, IDM_CLEAR_DWXS, L"Remove all DesktopWindowXamlSource objects");
        ::AppendMenuW(contextMenu, MF_STRING, IDM_CLOSE_ISLAND_HWND, L"Close HWND of all Islands");

        RECT rc{};
        ::GetWindowRect(hwnd, &rc);

        ::TrackPopupMenu(contextMenu, TPM_TOPALIGN | TPM_LEFTALIGN, rc.left + 5, rc.top + 5, 0, this->m_hMainWnd, nullptr);
        }, 210, 40, 50, 30);

    return true;
}

DWORD WINAPI MainWindow::CreateNewWindowOnNewThreadProc(_In_ LPVOID parameter)
{
    MainWindow* mainWindow = static_cast<MainWindow*>(parameter);
    auto result = MainWindow::Run((HINSTANCE)GetWindowLongPtr(mainWindow->GetHandle(), GWLP_HINSTANCE), SW_SHOW);
    return result;
}

void MainWindow::OnCommand(HWND, int id, HWND hwndCtl, UINT )
{
    switch (id)
    {
    case IDM_START_APP:
        if (winrt::WinUICppIslandsSampleApp::implementation::App::s_hasExistedInProcess)
        {
            auto result = ::MessageBox(GetHandle(),
                L"It's currently not supported to create more than one Application in a process, even if the previous "
                L"instance has been closed.  Press OK to proceed", L"Warning", MB_OKCANCEL | MB_ICONWARNING);
            if (result == IDCANCEL)
            {
                break;
            }
        }
        // This object derrives from Microsoft.UI.Xaml.Application.  Creating this object will also create a WindowsXamlManager,
        // effectively starting up XAML in the process and on the thread.
        winrt::make<winrt::WinUICppIslandsSampleApp::implementation::App>().as(g_app);
        break;
    case IDM_RELEASE_APP:
        {
            g_app->m_initialWindowsXamlManager = nullptr;
            g_app = nullptr;

            // MUXC has a special export that clears out some of its static state.
            HMODULE muxcHandle = GetModuleHandle(L"Microsoft.UI.Xaml.Controls.dll");
            if (muxcHandle != NULL)
            {
                typedef void(__stdcall* DeinitializeMuxcPtr)();
                auto deinitializeMuxcPtr = reinterpret_cast<DeinitializeMuxcPtr>(
                    GetProcAddress(muxcHandle, "DeinitializeMUXC"));
                deinitializeMuxcPtr();
            }
        }
        break;
    case IDM_CREATE_NEW_THREAD:
        ::CreateThread(nullptr, 0, &MainWindow::CreateNewWindowOnNewThreadProc, this, 0, 0);
        break;
    case IDM_CREATE_MODAL_WINDOW:
        ::DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), m_hMainWnd, &MainWindow::DialogBoxProc, 0);
        break;
    case IDM_CLEAR_DWXS:
        this->ClearXamlSources();
        break;
    case IDM_CLOSE_ISLAND_HWND:
        for (auto&& dwxs : m_xamlSources)
        {
            HWND islandWnd = GetWindowFromWindowId(dwxs.SiteBridge().WindowId());
            ::DestroyWindow(islandWnd);
        }
        break;
    case IDM_EXIT:
        PostQuitMessage(0);
        break;
    }

    for (auto b : m_buttons)
    {
        if (b.id == id)
        {
            b.func(hwndCtl);
            break;
        }
    }
}

void MainWindow::OnDestroy(HWND hwnd)
{
    base_type::OnDestroy(hwnd);
}

void MainWindow::OnResize(HWND, UINT state, int cx, int cy)
{
    UNREFERENCED_PARAMETER(cx);
    UNREFERENCED_PARAMETER(cy);
    UNREFERENCED_PARAMETER(state);
}

void MainWindow::OnPaint(HWND)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(GetHandle(), &ps);

    RECT rc;
    GetClientRect(GetHandle(), &rc);
    SetDCBrushColor(hdc, RGB(212, 212, 220));
    FillRect(hdc, &rc, (HBRUSH)GetStockObject(DC_BRUSH));

    EndPaint(GetHandle(), &ps);
}

// Called periodically to update some text that shows the user state.
void MainWindow::OnTimer(HWND, UINT)
{
    float workingSetInMb{ -1.0f };

    {
        HANDLE hProcess;
        PROCESS_MEMORY_COUNTERS pmc;
        hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
            PROCESS_VM_READ,
            FALSE, ::GetCurrentProcessId());
        if (NULL == hProcess)
            return;

        if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
        {
            workingSetInMb = (pmc.WorkingSetSize) / 1000000.0f;
        }

        CloseHandle(hProcess);
    }

    {
        wchar_t str[200];
        ::StringCchPrintf(str, ARRAYSIZE(str), L"Xaml in process? %s | Xaml on thread? %s | Working set MB: %0.3f",
            XamlUtil::IsXamlRunningInProcess() ? L"Yes" : L"No",
            XamlUtil::IsXamlRunningOnThread() ? L"Yes" : L"No",
            workingSetInMb);
        ::SetWindowText(m_label, str);
    }
}

// Just used to store information for the DialogBox
struct ModalDialogBoxInfo
{
    std::shared_ptr<MainWindow> m_mainWindow;
};

INT_PTR CALLBACK MainWindow::DialogBoxProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    ModalDialogBoxInfo* info = reinterpret_cast<ModalDialogBoxInfo*>(::GetWindowLongPtr(hDlg, GWLP_USERDATA));
    switch (message)
    {
    case WM_INITDIALOG:
        {
            // This DialogBox just creates a new MainWindow as its only child window.
            info = new ModalDialogBoxInfo;
            ::SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(info));
            info->m_mainWindow = std::make_shared<MainWindow>((HINSTANCE)GetWindowLongPtr(hDlg, GWLP_HINSTANCE), SW_SHOW, hDlg /*parentHWnd*/);
        }
        return (INT_PTR)TRUE;

    case WM_SIZE:
        {
            UINT width = LOWORD(lParam);
            UINT height = HIWORD(lParam);
            ::SetWindowPos(info->m_mainWindow->m_hMainWnd, nullptr, 5, 5, width - 10, height - 10, SWP_SHOWWINDOW);
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;

    case WM_NCDESTROY:
        delete info;
        ::SetWindowLongPtr(hDlg, GWLP_USERDATA, NULL);
        break;
    }

    return (INT_PTR)FALSE;
}
