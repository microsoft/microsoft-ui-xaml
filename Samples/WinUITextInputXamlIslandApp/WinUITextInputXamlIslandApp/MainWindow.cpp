#include "pch.h"

#include "MainWindow.h"
#include "DesktopWindow.h"

using namespace winrt;
using namespace winrt::Windows::Foundation;

using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;

#define MAX_LOADSTRING 100

HINSTANCE hInst = nullptr; // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

void MainWindow::RegisterWindowClass(HINSTANCE hInstance)
{
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_ISLANDSAMPLEWINUI3, szWindowClass, MAX_LOADSTRING);

    WNDCLASSEXW wcex = {};

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = MainWindow::WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ISLANDSAMPLEWINUI3));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_ISLANDSAMPLEWINUI3);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    WINRT_VERIFY(RegisterClassExW(&wcex));
    WINRT_ASSERT(!GetHandle());
}

MainWindow::MainWindow(HINSTANCE hInstance, int nCmdShow, ThreadManager* threadmngr) noexcept
{
    m_pThreadMngr = threadmngr;
    m_hInstance = hInstance;
    m_nCmdShow = nCmdShow;

    MainWindow::RegisterWindowClass(hInstance);
    InitInstance(m_hInstance, m_nCmdShow);
}

[[nodiscard]] LRESULT MainWindow::MessageHandler(UINT const message, WPARAM const wParam, LPARAM const lParam) noexcept
{
    switch (message)
    {
        HANDLE_MSG(GetHandle(), WM_CREATE, OnCreate);
        HANDLE_MSG(GetHandle(), WM_COMMAND, OnCommand);
        HANDLE_MSG(GetHandle(), WM_DESTROY, OnDestroy);
        HANDLE_MSG(GetHandle(), WM_SIZE, OnResize);
        HANDLE_MSG(GetHandle(), WM_PAINT, OnPaint);
    }

    return base_type::MessageHandler(message, wParam, lParam);
}

void MainWindow::InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable
    
    m_hMainWnd = CreateWindowW(szWindowClass,
                                  szTitle,
                                  WS_OVERLAPPEDWINDOW,
                                  CW_USEDEFAULT,
                                  0,
                                  CW_USEDEFAULT,
                                  0,
                                  nullptr,
                                  nullptr,
                                  hInstance,
                                  this);

    if (!m_hMainWnd)
    {
        winrt::check_hresult(E_FAIL);
        return;
    }

    ShowWindow(m_hMainWnd, nCmdShow);
    UpdateWindow(m_hMainWnd);

    SetFocus(m_hMainWnd);
}

bool MainWindow::OnCreate(HWND, LPCREATESTRUCT)
{
    m_mainUserControlUpper = StackPanel();
    m_mainUserControlLower = StackPanel();

    m_hWndXamlIslandUpper = wil::unique_hwnd(CreateDesktopWindowsXamlSource(WS_TABSTOP, m_mainUserControlUpper));
    m_hWndXamlIslandLower = wil::unique_hwnd(CreateDesktopWindowsXamlSource(WS_TABSTOP, m_mainUserControlLower));

    ::SetWindowPos(m_hWndXamlIslandUpper.get(), NULL, 20, 20, 400, 300, SWP_SHOWWINDOW);
    ::SetWindowPos(m_hWndXamlIslandLower.get(), NULL, 20, 20 + 600, 400, 200, SWP_SHOWWINDOW);

    auto textBlock = TextBlock();
    wchar_t textInfo[MAX_PATH]{ 0 };
    wsprintf(textInfo, L"window %d in thread %d", m_pThreadMngr->GetWindowsCount() + 1, ThreadManager::GetThreadsCount());
    textBlock.Text(textInfo);

    auto textBoxUpper = TextBox();
    textBoxUpper.Text(L"Text Edit a");

    auto textBoxLower = TextBox();
    textBoxLower.Text(L"Text Edit b");

    auto comboBoxUpper = ComboBox();
    comboBoxUpper.Items().Append(winrt::box_value(L"Item 1a"));
    comboBoxUpper.Items().Append(winrt::box_value(L"Item 2a"));

    auto comboBoxLower = ComboBox();
    comboBoxLower.Items().Append(winrt::box_value(L"Item 1b"));
    comboBoxLower.Items().Append(winrt::box_value(L"Item 2b"));
    
    auto clickHandlerCreateThread = [this](IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
    {
        ThreadManager::CreateThreadWithNewWindow(m_hInstance, m_nCmdShow);
    };

    Button btnForNewThread = Button();
    btnForNewThread.Content(box_value(L"Create new thread"));
    btnForNewThread.Click(clickHandlerCreateThread);

    auto clickHandlerCreateWin = [this](IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
    {
        m_pThreadMngr->CreateWinOnCurrentThread();
    };

    Button btnForNewWindow = Button();
    btnForNewWindow.Content(box_value(L"Create new window"));
    btnForNewWindow.Click(clickHandlerCreateWin);

    m_mainUserControlUpper.Children().Append(textBlock);
    m_mainUserControlUpper.Children().Append(textBoxUpper);
    m_mainUserControlUpper.Children().Append(comboBoxUpper);
    m_mainUserControlUpper.Children().Append(btnForNewThread);

    m_mainUserControlLower.Children().Append(textBoxLower);
    m_mainUserControlLower.Children().Append(comboBoxLower);
    m_mainUserControlLower.Children().Append(btnForNewWindow);
    return true;
}

void MainWindow::OnCommand(HWND, int id, HWND hwndCtl, UINT )
{
    UNREFERENCED_PARAMETER(hwndCtl);

    switch (id)
    {
    case IDM_EXIT:
        PostQuitMessage(0);
        break;
    }
}

void MainWindow::OnDestroy(HWND hwnd)
{
    base_type::OnDestroy(hwnd);
    size_t winCount = m_pThreadMngr->RemoveWin(this);
    if( winCount == 0)
    {
        PostQuitMessage(0);
    }
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

    // TODO: Add any drawing code that uses hdc here...
    RECT rc;
    GetClientRect(GetHandle(), &rc);
    SetDCBrushColor(hdc, RGB(212, 212, 220));
    FillRect(hdc, &rc, (HBRUSH)GetStockObject(DC_BRUSH));

    EndPaint(GetHandle(), &ps);
}

void MainWindow::OnXamlButtonClick(IInspectable const& sender, RoutedEventArgs const&)
{
    UNREFERENCED_PARAMETER(sender);
}
