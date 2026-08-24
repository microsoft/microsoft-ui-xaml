#pragma once

#include "Helper.h"

#include "MainWindow.h"
class MainWindow;

class ThreadManager
{
public:
    ThreadManager(HINSTANCE hInstance, int nCmdShow);
    ~ThreadManager();

    void CreateWinOnCurrentThread();
    size_t GetWindowsCount();
    size_t RemoveWin(MainWindow *wnd);
    void RunMessageLoop();

    static void CreateThreadWithNewWindow(HINSTANCE hInstance, int nCmdShow);
    static void WaitUntilZero();
    static int GetThreadsCount();
    static auto TakeReference();

private:
    inline static reference_waiter m_threadsWaiter;
    std::optional<reference_waiter::reference_waiter_holder> m_threadHolder;

    winrt::Microsoft::UI::Xaml::Hosting::WindowsXamlManager m_windowsXamlManager{ nullptr };
    bool isMessageLoopOn{ false };

    HACCEL m_hAccelTable;
    HINSTANCE m_hInstance;
    int m_nCmdShow;
    std::vector<MainWindow*> m_wins;
};




