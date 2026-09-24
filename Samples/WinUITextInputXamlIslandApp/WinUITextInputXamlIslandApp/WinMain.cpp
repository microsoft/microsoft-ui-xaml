#include "pch.h"

#include "App.h"
#include "MainWindow.h"

winrt::Microsoft::UI::Xaml::Hosting::WindowsXamlManager g_windowsXamlManager{ nullptr };

void OnAppCreated()
{
    // We need to initialize the WindowsXamlManager _while_ the App object is being created. If we initialize the
    // WindowsXamlManager _before_ the App object is created, this will cause the DXamlCore to start up, using it's
    // default MUX.Application object, so the app won't have a chance to use its own. If we initialize the
    // WindowsXamlManager _after_ the App object is created, the App creation will fail, because we'll call
    // MUX.Application.LoadComponent before the DXamlCore is initialized.  So, we must do it here.
    g_windowsXamlManager = winrt::Microsoft::UI::Xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread();
}

void DrainMessageQueue()
{
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        ::DispatchMessage(&msg);
    }
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    winrt::init_apartment(winrt::apartment_type::single_threaded);

    auto app = winrt::make<winrt::WinUITextInputXamlIslandApp::implementation::App>();

    ThreadManager::CreateThreadWithNewWindow(hInstance, nCmdShow);
    ThreadManager::WaitUntilZero();

    app.Exit();

    // We release our reference to our App object before we drain the queue.  We're trying to get all the XAML objects
    // cleaned up before we exit this function.  Unfortunately the DXamlCore still holds a reference to this object (in
    // the g_pApplication pointer), so our App object won't actually get deleted until microsoft.ui.xaml.dll's DllMain
    // gets called with PROCESS_DETACH, and FrameworkApplication::GlobalDeinit gets called.
    app = nullptr;

    // When we call WindowsXamlManager.Close, it enqueues a call to XamlCore.Close on the dispatcher.  We'll still need
    // to pump more messages later to make sure this XamlCore.Close is executed and XAML is actually shut down. It might
    // be nice to call WindowsXamlManager.Close from App::~App, but as per the above comment, this won't get called
    // until microsoft.ui.xaml.dll gets unloaded, and we need to deinitialize the DXamlCore on the thread before that
    // happens.
    g_windowsXamlManager.Close();
    g_windowsXamlManager = nullptr;
    
    // Drain the message queue to ensure the Xaml gets properly shut down.  For example, the WindowsXamlManager enqueues
    // a call on the DispatcherQueue to call XamlCore.Close, which requires another message to be dispatched to support.
    // Draining the message queue here allows async operations like this to be fully processed.
    DrainMessageQueue();

    // We have to comment this assert, because it will still fire -- see the above comment about g_pApplication
    // continuing to hold a ref to the App object until microsoft.ui.xaml.dll is unloaded.  It would be great if we
    // could figure out how to spin down XAML to the point where Application::Current returns null.
    //assert(winrt::Microsoft::UI::Xaml::Application::Current() == nullptr);

    return 0;
}
