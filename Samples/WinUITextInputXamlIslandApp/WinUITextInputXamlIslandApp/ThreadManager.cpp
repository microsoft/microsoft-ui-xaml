#include "pch.h"
#include <vector>
#include <algorithm>
#include <Microsoft.UI.Dispatching.Interop.h> // For ContentPreTranslateMessage

#include "ThreadManager.h"

ThreadManager::ThreadManager(HINSTANCE hInstance, int nCmdShow)
    :m_windowsXamlManager(winrt::Microsoft::UI::Xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread()),
    m_hAccelTable(LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ISLANDSAMPLEWINUI3))),
    m_hInstance(hInstance),
    m_nCmdShow(nCmdShow)
{
}

auto ThreadManager::TakeReference()
{
    return m_threadsWaiter.take_reference();
}

ThreadManager::~ThreadManager()
{
    m_windowsXamlManager.Close();
    m_windowsXamlManager = nullptr;

    m_wins.clear();
}

void ThreadManager::WaitUntilZero()
{
    m_threadsWaiter.wait_until_zero();
}

void ThreadManager::CreateThreadWithNewWindow(HINSTANCE hInstance, int nCmdShow)
{
    auto queueController = winrt::Microsoft::UI::Dispatching::DispatcherQueueController::CreateOnDedicatedThread();
    queueController.DispatcherQueue().TryEnqueue([nCmdShow, hInstance, threadRef = ThreadManager::TakeReference(), queueController]()
    {
        ThreadManager thread(hInstance, nCmdShow);
        thread.CreateWinOnCurrentThread();
        thread.RunMessageLoop();
        queueController.ShutdownQueue();
    });
}

void ThreadManager::CreateWinOnCurrentThread()
{
    MainWindow* mw = new MainWindow(m_hInstance, m_nCmdShow, this);
    m_wins.push_back(mw);
}

void ThreadManager::RunMessageLoop()
{
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (std::find_if(m_wins.begin(), m_wins.end(), [&](auto&& win)
            {
                return win->m_hMainWnd == msg.hwnd;
            }) != m_wins.end())
        {
            auto that = reinterpret_cast<DesktopWindow*>(GetWindowLongPtr(msg.hwnd, GWLP_USERDATA));
            if (that != nullptr)
            {
                if (!ContentPreTranslateMessage(&msg) && !TranslateAccelerator(msg.hwnd, m_hAccelTable, &msg))
                {
                    if (!that->NavigateFocus(&msg))
                    {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                }
            }
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

size_t ThreadManager::GetWindowsCount()
{
    return m_wins.size();
}

size_t ThreadManager::RemoveWin(MainWindow *wnd)
{
    m_wins.erase(std::remove(m_wins.begin(), m_wins.end(), wnd), m_wins.end());

    return m_wins.size();
}

int ThreadManager::GetThreadsCount()
{
    return m_threadsWaiter.thread_count();
}
