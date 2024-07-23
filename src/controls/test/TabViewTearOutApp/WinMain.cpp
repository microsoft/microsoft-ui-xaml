#include "pch.h"

#include "App.h"
#include "MainWindow.h"

extern HINSTANCE g_hInst;

#ifdef MULTITHREADED_WINDOWS
extern std::vector<std::thread> g_childThreads;
#endif

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    g_hInst = hInstance;

#ifdef MULTITHREADED_WINDOWS
    MainWindow::CreateNewWindow(nCmdShow);

    for (int i = 0; i < g_childThreads.size(); i++)
    {
        g_childThreads[i].join();
    }
#else
    winrt::init_apartment(winrt::apartment_type::single_threaded);

    // Start a DispatcherQueueController.  A DispatcherQueue must be running on the thread for XAML to work.
    auto dqc = winrt::Microsoft::UI::Dispatching::DispatcherQueueController::CreateOnCurrentThread();

    MainWindow::CreateNewWindow(nCmdShow, {});

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

#endif

    return 0;
}
