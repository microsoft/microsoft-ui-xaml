// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "XamlIslandTests.h"
#include <XamlTailored.h>
#include <Microsoft.UI.Content.h>
#include <Microsoft.UI.Dispatching.h>
#include <Microsoft.UI.Xaml.h>
#include "TestEvent.h"
#include "WaitForDebugger.h"
#include <functional>
#include <array>
#include <Microsoft.UI.Composition.SystemBackdrops.h>
#include "FileLoader.h"
#include "VisualDebugTags.h"

#ifdef BUILD_WINDOWS
    #error "BUILD_WINDOWS must not be defined."
#endif
#include <Microsoft.UI.Interop.h>

#include <AutomationClient/AutomationClientManager.h>

using namespace Microsoft::UI;
using namespace Microsoft::UI::Composition;
using namespace Microsoft::UI::Composition::SystemBackdrops;
using namespace Microsoft::UI::Content;
using namespace Microsoft::UI::Dispatching;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Documents;
using namespace Microsoft::UI::Xaml::Hosting;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Interop;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Windows::Foundation;
using namespace Private::Infrastructure;
using namespace Platform;

#pragma warning(disable:4698)   // warning C4698: 'Microsoft::UI::Dispatching::DispatcherQueueController::ShutdownQueue'
                                // is for evaluation purposes only and is subject to change or removal in future updates.

#include "XamlIslandTestUtil.h"

namespace Microsoft::UI::Xaml::Tests::Foundation::Hosting {

#include "ApplicationWithMuxc.h"
#include "ApplicationLikeFileExplorer.h"
#include "XamlIslandTestHelper.h"

ref class XamlIslandTests_Application sealed : Microsoft::UI::Xaml::Application
{
public:
    XamlIslandTests_Application()
    {
        LOG_OUTPUT(L"    > [XamlIslandTests_Application] ctor called!");
    }

    int GetOnLaunchedCallCount() { return m_onLaunchedCallCount; }

    void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs^) override
    {
        m_onLaunchedCallCount++;
        LOG_OUTPUT(L"    > [XamlIslandTests_Application] OnLaunched called! New call count: %d", m_onLaunchedCallCount);
    }

private:
    int m_onLaunchedCallCount {0};
};

StackPanel^ CreateControlSubtree(
    Platform::Object^ buttonContent,
    IslandSceneKind kind = IslandSceneKind::WithButton)
{
    StackPanel^ stackPanel = ref new StackPanel();
    stackPanel->Width = 200;
    stackPanel->Height = 200;
    stackPanel->Background = ref new SolidColorBrush(ColorHelper::FromArgb(0xff, 0, 0xff, 0));

    switch (kind)
    {
        case IslandSceneKind::ManyControls:
            {
                RadioButton^ rb = ref new RadioButton();
                rb->Content = ref new Platform::String(L"Radio button");
                stackPanel->Children->Append(rb);

                TextBox^ tb = ref new TextBox();
                tb->Text = L"TextBox";
                stackPanel->Children->Append(tb);

                RichTextBlock^ rtb = ref new RichTextBlock();
                Paragraph^ p = ref new Paragraph();
                Run^ r = ref new Run();
                r->Text = L"Go to Bing";
                r->FontSize = 22.0;
                r->FontFamily = ref new FontFamily(L"Tahoma");
                r->Foreground = ref new SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(0xf0, 0x33, 0x33, 0xff));
                Hyperlink^ h = ref new Hyperlink();
                h->NavigateUri = ref new ::Windows::Foundation::Uri(ref new Platform::String(L"http://bing.com"));
                h->Inlines->Append(r);
                p->Inlines->Append(h);
                rtb->Blocks->Append(p);
                stackPanel->Children->Append(rtb);

                ListView^ list = ref new ListView();
                ListViewItem^ lvi = ref new ListViewItem();
                lvi->Content = ref new Platform::String(L"list view item 1");
                list->Items->Append(lvi);
                stackPanel->Children->Append(list);

                CalendarView^ cal = ref new CalendarView();
                stackPanel->Children->Append(cal);

                tb->Loaded += ref new Microsoft::UI::Xaml::RoutedEventHandler([=](Platform::Object^ sender, RoutedEventArgs^ e) {
                    MenuFlyout^ mf = ref new MenuFlyout();
                    MenuFlyoutItem^ item = ref new MenuFlyoutItem();
                    item->Text = ref new Platform::String(L"Item 1");
                    mf->Items->Append(item);
                    mf->ShouldConstrainToRootBounds = false;
                    mf->ShowAt(stackPanel);
                    });
            } // fall-through
        case IslandSceneKind::WithNumberBox:
            {
                NumberBox^ numberBox = ref new NumberBox();
                numberBox->Width = 125;
                numberBox->Height = 50;
                numberBox->AcceptsExpression = true;
                numberBox->Text = L"42 + 12";
                stackPanel->Children->Append(numberBox);
            } // fall-through...
        case IslandSceneKind::WithButton:
            {
                Button^ button = ref new Button();
                button->Width = 125;
                button->Height = 50;
                button->Content = buttonContent;
                stackPanel->Children->Append(button);
            }
    }

    return stackPanel;
}

DesktopWindowXamlSource^ CreateDesktopWindowXamlSource(
    const HWND hwnd,
    Platform::Object^ buttonContent,
    IslandSceneKind kind = IslandSceneKind::WithButton)
{
    WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

    DesktopWindowXamlSource^ dwxs = ref new DesktopWindowXamlSource();
    LOG_OUTPUT(L"  ui> DesktopWindowXamlSource created.");

    mu::WindowId windowId;
    VERIFY_SUCCEEDED(GetWindowIdFromWindow(hwnd, (::ABI::Microsoft::UI::WindowId*) &windowId));

    dwxs->Initialize(windowId);
    LOG_OUTPUT(L"    ui> DesktopWindowXamlSource Initialized, attached to hwnd.");

    dwxs->Content = CreateControlSubtree(buttonContent, kind);
    LOG_OUTPUT(L"  ui> DesktopWindowXamlSource.Content assigned.");

    return dwxs;
}

XamlIsland^ CreateXamlIsland(
    Platform::Object^ buttonContent,
    IslandSceneKind kind = IslandSceneKind::WithButton)
{
    WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

    XamlIsland^ xi = ref new XamlIsland();
    LOG_OUTPUT(L"  ui> XamlIsland created.");

    xi->Content = CreateControlSubtree(buttonContent, kind);
    LOG_OUTPUT(L"  ui> XamlIsland.Content assigned.");

    return xi;
}

void XamlIslandTests::ClearFlags()
{
    m_hwndMap.clear();
    m_flagMap.clear();
}

bool XamlIslandTests::TestSetup()
{
    // Need a better way to do this. This breaks between every test. Without it, we don't get a chance to attach at all.
    // Do this during ClassSetup?
    WaitForDebugger();

    WNDCLASSEX windowClass = {};
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.lpfnWndProc = DefWindowProc;
    windowClass.lpszClassName = L"XamlIslandTests_WindowClass";
    m_windowClassAtom = ::RegisterClassEx(&windowClass);

    return true;
}

bool XamlIslandTests::TestCleanup()
{
    ClearFlags();

    ::UnregisterClassW(MAKEINTATOM(m_windowClassAtom), nullptr);
    m_windowClassAtom = 0;

    return true;
}

void XamlIslandTests::SetFlagAndPokeHwnd(int id, HWND hwnd)
{
    auto unlockOnExit = m_cs.lock();
    m_flagMap[id] = true;

    if (hwnd != NULL)
    {
        // Also poke the hwnd with a message. It might be blocked in GetMessage() and won't wake up otherwise.
        ::PostMessageA(hwnd, WM_NULL, 0, 0);
    }
}

void XamlIslandTests::ClearFlag(int id)
{
    auto unlockOnExit = m_cs.lock();
    m_flagMap[id] = false;
}

void XamlIslandTests::PumpMessagesWhileWaitingForFlag(int id)
{
    DrainMessageQueue();

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (GetFlag(id))
        {
            break;
        }
    }
}

bool XamlIslandTests::GetFlag(int id)
{
    auto unlockOnExit = m_cs.lock();
    return m_flagMap[id];
}

HWND XamlIslandTests::CreateHwnd(int id, HWND parent)
{
    HWND hwnd = CreateWindowW(
        MAKEINTATOM(m_windowClassAtom),
        L"XamlIslandRootsTest Test Window",
        parent != NULL ? WS_CHILD : WS_OVERLAPPEDWINDOW,
        0,
        0,
        300 /* width */,
        300 /* height */,
        parent /* hwndParent */,
        nullptr /* hMenu */,
        nullptr /* hInstance */,
        nullptr /* lpParam */);

    ::ShowWindow(hwnd, SW_SHOW);

    ::SetFocus(hwnd);

    LOG_OUTPUT(L"  > Hwnd created.");

    {
        auto unlockOnExit = m_cs.lock();
        m_hwndMap[id] = hwnd;
    }

    return hwnd;
}

HWND XamlIslandTests::CreateHwnd()
{
    {
        auto unlockOnExit = m_cs.lock();
        ASSERT(m_hwndMap.size() == 0);
    }

    return CreateHwnd(0);
}

HWND XamlIslandTests::GetHwnd(int id)
{
    auto unlockOnExit = m_cs.lock();
    return m_hwndMap[id];
}

HWND XamlIslandTests::GetHwnd()
{
    auto unlockOnExit = m_cs.lock();
    ASSERT(m_hwndMap.size() == 1);
    return GetHwnd(0);
}

void XamlIslandTests::IslandsRequireDispatcherQueueController()
{
    auto uiThread = RunOnNewThread([]() {
        Platform::Exception^ thrownException;
        VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_MULTITHREADED));

        try
        {
            LOG_OUTPUT(L"Creating DesktopWindowXamlSource...");
            DesktopWindowXamlSource^ dwxs = ref new DesktopWindowXamlSource();
        }
        catch (Platform::Exception^ ex)
        {
            thrownException = ex;
            LOG_OUTPUT(L"Caught Exception.");
        }
        VERIFY_IS_NOT_NULL(thrownException);
        thrownException = nullptr;

        try
        {
            LOG_OUTPUT(L"Creating WindowsXamlManager...");
            WindowsXamlManager^ wxm = WindowsXamlManager::InitializeForCurrentThread();
        }
        catch (Platform::Exception^ ex)
        {
            thrownException = ex;
            LOG_OUTPUT(L"Caught Exception.");
        }
        VERIFY_IS_NOT_NULL(thrownException);
        thrownException = nullptr;

        auto dqc {DispatcherQueueController::CreateOnCurrentThread()};

        try
        {
            LOG_OUTPUT(L"Creating WindowsXamlManager...");
            WindowsXamlManager^ wxm = WindowsXamlManager::InitializeForCurrentThread();
        }
        catch (Platform::Exception^ ex)
        {
            thrownException = ex;
            LOG_OUTPUT(L"Caught Exception.");
        }

        VERIFY_IS_NULL(thrownException);

        DrainMessageQueue();
        dqc->ShutdownQueue();
    });

    WaitForSingleObjectWithTimeout(uiThread);
}

void XamlIslandTests::WindowsXamlManagerCreationScenarios()
{
    auto uiThread = RunOnNewThread([this]() {
        VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_MULTITHREADED));

        WindowsXamlManager^ wxm1;
        WindowsXamlManager^ wxm2;
        WindowsXamlManager^ wxm3;

        auto dqc {DispatcherQueueController::CreateOnCurrentThread()};

        LOG_OUTPUT(L"Creating WindowsXamlManager wxm1...");
        wxm1 = WindowsXamlManager::InitializeForCurrentThread();

        DrainMessageQueue();

        LOG_OUTPUT(L"Close wxm1 and create wxm2 before wxm1 has finished closing...");
        CloseObject(wxm1);

        wxm2 = WindowsXamlManager::InitializeForCurrentThread();

        LOG_OUTPUT(L"Drain message queue and try creating wxm2, wxm3 again...");
        DrainMessageQueue();
        wxm2 = WindowsXamlManager::InitializeForCurrentThread();
        wxm3 = WindowsXamlManager::InitializeForCurrentThread();

        DrainMessageQueue();

        CloseObject(wxm3);
        CloseObject(wxm2);
        DrainMessageQueue();

        dqc->ShutdownQueue();
    });

    WaitForSingleObjectWithTimeout(uiThread);
}

void XamlIslandTests::WindowsXamlManagerKeptAlive()
{
    auto uiThread = RunOnNewThread([this]() {
        VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_SINGLETHREADED));

        VERIFY_IS_NULL(WindowsXamlManager::GetForCurrentThread());

        auto dqc {DispatcherQueueController::CreateOnCurrentThread()};

        VERIFY_IS_NULL(WindowsXamlManager::GetForCurrentThread());

        LOG_OUTPUT(L"Creating WindowsXamlManager wxm1...");
        WindowsXamlManager^ wxm1 = WindowsXamlManager::InitializeForCurrentThread();
        IUnknown* rawUnk1 {reinterpret_cast<IUnknown*>(wxm1)};

        LOG_OUTPUT(L"Close wxm1 and let go of the reference.");
        CloseObject(wxm1);
        wxm1 = nullptr;

        DrainMessageQueue();

        LOG_OUTPUT(L"Creating WindowsXamlManager wxm2...");
        WindowsXamlManager^ wxm2 = WindowsXamlManager::InitializeForCurrentThread();
        IUnknown* rawUnk2 {reinterpret_cast<IUnknown*>(wxm2)};
        VERIFY_ARE_EQUAL(rawUnk1, rawUnk2);

        CloseObject(wxm2);

        WindowsXamlManager^ wxm3 = WindowsXamlManager::GetForCurrentThread();
        IUnknown* rawUnk3 {reinterpret_cast<IUnknown*>(wxm3)};
        VERIFY_ARE_EQUAL(rawUnk1, rawUnk3);

        dqc->ShutdownQueue();
    });

    WaitForSingleObjectWithTimeout(uiThread);
}

void XamlIslandTests::ValidateXamlShutdownCompletedOnThread()
{
    VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_MULTITHREADED));

    VERIFY_IS_NULL(Application::Current);

    auto ih1 = IslandHelper::CreateOnNewUIThreadAndNewWindow(m_windowClassAtom);
    auto ih2 = IslandHelper::CreateOnNewUIThreadAndNewWindow(m_windowClassAtom);

    VERIFY_IS_NOT_NULL(Application::Current);

    WindowsXamlManager^ wxm1;
    WindowsXamlManager^ wxm2;

    int numberOfTimesEventFired = 0;

    RunOnIslandUIThread(ih1, [&]()
    {
        safe_cast<DesktopWindowXamlSource^>(ih1->DesktopWindowXamlSource)->Content = CreateControlSubtree("Button1");

        wxm1 = WindowsXamlManager::InitializeForCurrentThread();
        wxm1->XamlShutdownCompletedOnThread +=
            ref new ::Windows::Foundation::TypedEventHandler<
                WindowsXamlManager^, XamlShutdownCompletedOnThreadEventArgs^>(
                    [&](WindowsXamlManager^ wxm, XamlShutdownCompletedOnThreadEventArgs^ args){
                        ++numberOfTimesEventFired;
                        LOG_OUTPUT(L"XamlShutdownCompletedOnThread for wxm1 raised.");
                        LOG_OUTPUT(L"Application::Current is valid? %d", Application::Current != nullptr);
                        VERIFY_IS_NOT_NULL(Application::Current);
                    });
    });

    RunOnIslandUIThread(ih2, [&]()
    {
        safe_cast<DesktopWindowXamlSource^>(ih2->DesktopWindowXamlSource)->Content = CreateControlSubtree("Button2");

        wxm2 = WindowsXamlManager::InitializeForCurrentThread();
        wxm2->XamlShutdownCompletedOnThread +=
            ref new ::Windows::Foundation::TypedEventHandler<
                WindowsXamlManager^, XamlShutdownCompletedOnThreadEventArgs^>(
                    [&](WindowsXamlManager^ wxm, XamlShutdownCompletedOnThreadEventArgs^ args){
                        ++numberOfTimesEventFired;
                        LOG_OUTPUT(L"XamlShutdownCompletedOnThread for wxm2 raised.");
                        LOG_OUTPUT(L"Application::Current is valid? %d", Application::Current != nullptr);
                        VERIFY_IS_NULL(Application::Current);
                    });
    });

    // Short sleep just to allow DWXS to be briefly visible.
    ::Sleep(200);

    LOG_OUTPUT(L"Close windows.");
    ih1 = nullptr;
    ih2 = nullptr;

    wxm1 = nullptr;
    wxm2 = nullptr;

    VERIFY_ARE_EQUAL(2, numberOfTimesEventFired);

    ::RoUninitialize();
}

void XamlIslandTests::ValidateXamlShutdownCompletedOnThreadWithDeferral()
{
    Deferral^ deferral;
    bool didWxm1RaiseEvent {false};
    bool didWxm2RaiseEvent {false};
    bool didShutdownQueueComplete {false};
    DispatcherQueueController^ uiThreadDqc;
    Event uiThreadReady;
    WindowsXamlManager^ wxm1;
    WindowsXamlManager^ wxm2;

    VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_MULTITHREADED));

    VERIFY_IS_NULL(WindowsXamlManager::GetForCurrentThread());

    auto uiThread = RunOnNewThread([&]() {
        VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_MULTITHREADED));

        VERIFY_IS_NULL(WindowsXamlManager::GetForCurrentThread());

        uiThreadDqc = DispatcherQueueController::CreateOnCurrentThread();

        VERIFY_IS_NULL(Application::Current);

        LOG_OUTPUT(L"Creating WindowsXamlManager wxm1...");
        wxm1 = WindowsXamlManager::InitializeForCurrentThread();

        VERIFY_IS_NOT_NULL(WindowsXamlManager::GetForCurrentThread());
        VERIFY_IS_NOT_NULL(Application::Current);

        wxm2 = WindowsXamlManager::InitializeForCurrentThread();

        wxm1->XamlShutdownCompletedOnThread +=
            ref new ::Windows::Foundation::TypedEventHandler<
                WindowsXamlManager^, XamlShutdownCompletedOnThreadEventArgs^>(
                    [&](WindowsXamlManager^ wxm, XamlShutdownCompletedOnThreadEventArgs^ args){
                        LOG_OUTPUT(L"XamlShutdownCompletedOnThread for wxm1 raised.");
                        VERIFY_IS_NULL(WindowsXamlManager::GetForCurrentThread());
                        VERIFY_IS_NULL(Application::Current);
                        VERIFY_IS_FALSE(didWxm1RaiseEvent);
                        VERIFY_IS_FALSE(didWxm2RaiseEvent);
                        didWxm1RaiseEvent = true;

                        deferral = args->GetDispatcherQueueDeferral();
                    });


        wxm2->XamlShutdownCompletedOnThread +=
            ref new ::Windows::Foundation::TypedEventHandler<
                WindowsXamlManager^, XamlShutdownCompletedOnThreadEventArgs^>(
                    [&](WindowsXamlManager^ wxm, XamlShutdownCompletedOnThreadEventArgs^ args){
                        LOG_OUTPUT(L"XamlShutdownCompletedOnThread for wxm2 raised.");
                        VERIFY_IS_NULL(WindowsXamlManager::GetForCurrentThread());
                        VERIFY_IS_NULL(Application::Current);
                        VERIFY_IS_TRUE(didWxm1RaiseEvent);
                        VERIFY_IS_FALSE(didWxm2RaiseEvent);
                        didWxm2RaiseEvent = true;
                    });

        VERIFY_IS_FALSE(didWxm1RaiseEvent);
        VERIFY_IS_FALSE(didWxm2RaiseEvent);

        uiThreadDqc->DispatcherQueue->ShutdownStarting +=
            ref new ::Windows::Foundation::TypedEventHandler<DispatcherQueue^,DispatcherQueueShutdownStartingEventArgs^>(
            [&](DispatcherQueue^ sender, DispatcherQueueShutdownStartingEventArgs^ args) {
                VERIFY_IS_NOT_NULL(WindowsXamlManager::GetForCurrentThread());
                LOG_OUTPUT(L"ShutdownStarting raised.");
                VERIFY_IS_FALSE(didWxm1RaiseEvent);
                VERIFY_IS_FALSE(didWxm2RaiseEvent);
            });

        uiThreadReady.Set();

        VERIFY_IS_NOT_NULL(WindowsXamlManager::GetForCurrentThread());

        LOG_OUTPUT(L"Calling ShutdownQueue()...");
        uiThreadDqc->ShutdownQueue();

        VERIFY_IS_NULL(WindowsXamlManager::GetForCurrentThread());
        didShutdownQueueComplete = true;

        VERIFY_IS_TRUE(didWxm1RaiseEvent);
        VERIFY_IS_TRUE(didWxm2RaiseEvent);
    });

    uiThreadReady.WaitForDefault();

    VERIFY_IS_NULL(WindowsXamlManager::GetForCurrentThread());

    // Ensure the DispatcherQueue is running and working at this point.
    for (int i=0; i<5; ++i)
    {
        Event waitCompleted;
        uiThreadDqc->DispatcherQueue->TryEnqueue(
            Microsoft::UI::Dispatching::DispatcherQueuePriority::Low,
            ref new Microsoft::UI::Dispatching::DispatcherQueueHandler([&](){
                LOG_OUTPUT(L"  Using DispatcherQueue to make sure deferral is working...");
                waitCompleted.Set();
            }));
        waitCompleted.WaitForDefault();
    }

    // Since we took thre deferral, shutdown should have not completed at this point.
    VERIFY_IS_NOT_NULL(deferral);
    VERIFY_IS_TRUE(didWxm1RaiseEvent);
    VERIFY_IS_TRUE(didWxm2RaiseEvent);
    VERIFY_IS_FALSE(didShutdownQueueComplete);

    deferral->Complete();

    WaitForSingleObjectWithTimeout(uiThread);
    VERIFY_IS_TRUE(didShutdownQueueComplete);

    // Release objects before RoUninitialize().
    wxm1 = nullptr;
    wxm2 = nullptr;
    deferral = nullptr;
    uiThreadDqc = nullptr;

    ::RoUninitialize();
}

void XamlIslandTests::XamlUnloadsAutomatically()
{
    auto uiThread = RunOnNewThread([this]() {
        VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_MULTITHREADED));

        WindowsXamlManager^ wxm1;

        auto dqc {DispatcherQueueController::CreateOnCurrentThread()};

        LOG_OUTPUT(L"Creating WindowsXamlManager wxm1.");
        wxm1 = WindowsXamlManager::InitializeForCurrentThread();

        DrainMessageQueue();

        LOG_OUTPUT(L"Application::Current should now be non-null");
        VERIFY_IS_NOT_NULL(Application::Current);

        dqc->ShutdownQueue();

        LOG_OUTPUT(L"DispatcherQueueController.ShutdownQueue has been called.");

        LOG_OUTPUT(L"Calling WindowsXamlManager.Close should safely no-op.");
        CloseObject(wxm1);

        LOG_OUTPUT(L"Application::Current should be null now");
        VERIFY_IS_NULL(Application::Current);
    });

    WaitForSingleObjectWithTimeout(uiThread);
}

void XamlIslandTests::XamlIslandCreationScenarios()
{
    auto uiThread = RunOnNewThread([this]() {
        VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_MULTITHREADED));

        HWND hwnd = CreateHwnd(1);
        mu::WindowId windowId;
        VERIFY_SUCCEEDED(GetWindowIdFromWindow(hwnd, (::ABI::Microsoft::UI::WindowId *)&windowId));

        auto dqc{DispatcherQueueController::CreateOnCurrentThread()};

        auto xi = CreateXamlIsland("Xaml Island Button");
        auto island = xi->ContentIsland;
        auto dispatcherQueue = island->DispatcherQueue;

        auto dcsb = DesktopChildSiteBridge::CreateWithDispatcherQueue(dispatcherQueue, windowId);

        dcsb->Connect(island);
        dcsb->Show();

        dqc->ShutdownQueue();
    });

    WaitForSingleObjectWithTimeout(uiThread);
}

DWORD WINAPI TwoWindowsOnSameThread_ThreadProc(_In_ LPVOID lpParameter)
{
    XamlIslandTests* xamlIslandTests = reinterpret_cast<XamlIslandTests*>(lpParameter);

    HWND hwnd1 = xamlIslandTests->CreateHwnd(1);
    HWND hwnd2 = xamlIslandTests->CreateHwnd(2);

    auto dqc {DispatcherQueueController::CreateOnCurrentThread()};

    DesktopWindowXamlSource^ dwxs1 = CreateDesktopWindowXamlSource(hwnd1, "Window 1 MUX button");
    DesktopWindowXamlSource^ dwxs2 = CreateDesktopWindowXamlSource(hwnd2, "Window 2 MUX button");

    xamlIslandTests->PumpMessagesWhileWaitingForFlag(0);

    LOG_OUTPUT(L"  > Cleaning up DWXS 1.");
    dwxs1 = nullptr;

    LOG_OUTPUT(L"  > Pumping messages for DWXS cleanup.");
    xamlIslandTests->PumpMessagesWhileWaitingForFlag(1);

    LOG_OUTPUT(L"  > Cleaning up DWXS 2.");
    dwxs2 = nullptr;

    LOG_OUTPUT(L"  > Pumping messages for DWXS cleanup.");
    xamlIslandTests->PumpMessagesWhileWaitingForFlag(2);    // flag will never come, but that doesn't matter since there's nothing to do afterwards anyway
    dqc->ShutdownQueue();

    return 0;
}

void XamlIslandTests::TwoWindowsOnSameThread()
{
    HANDLE uiThread = CreateThread(nullptr, 0, TwoWindowsOnSameThread_ThreadProc, this /* lpParameter */, 0, nullptr);

    LOG_OUTPUT(L"> Waiting 2 seconds for rendering...");
    Sleep(2000);
    HWND hwnd1 = GetHwnd(1);
    HWND hwnd2 = GetHwnd(2);

    LOG_OUTPUT(L"> Clean up first island");
    SetFlagAndPokeHwnd(0, hwnd1);

    LOG_OUTPUT(L"> Waiting 1 second for cleanup...");
    Sleep(1000);

    LOG_OUTPUT(L"> Clean up second island");
    SetFlagAndPokeHwnd(1, hwnd2);

    LOG_OUTPUT(L"> Waiting 1 second for cleanup...");
    Sleep(1000);

    LOG_OUTPUT(L"> No crash. Exiting test.");
    TerminateThread(uiThread, 0);
}

DWORD WINAPI TwoWindowsOnDifferentThreads_ThreadProc1(_In_ LPVOID lpParameter)
{
    XamlIslandTests* xamlIslandTests = reinterpret_cast<XamlIslandTests*>(lpParameter);

    HWND hwnd = xamlIslandTests->CreateHwnd(1);

    auto dqc {DispatcherQueueController::CreateOnCurrentThread()};

    DesktopWindowXamlSource^ dwxs = CreateDesktopWindowXamlSource(hwnd, "Window 1 MUX button");

    xamlIslandTests->PumpMessagesWhileWaitingForFlag(0);

    LOG_OUTPUT(L"  > Cleaning up DWXS 1.");
    dwxs = nullptr;

    LOG_OUTPUT(L"  > Pumping messages for DWXS cleanup.");
    DrainMessageQueue();
    dqc->ShutdownQueue();

    return 0;
}

DWORD WINAPI TwoWindowsOnDifferentThreads_ThreadProc2(_In_ LPVOID lpParameter)
{
    XamlIslandTests* xamlIslandTests = reinterpret_cast<XamlIslandTests*>(lpParameter);

    HWND hwnd = xamlIslandTests->CreateHwnd(2);

    auto dqc {DispatcherQueueController::CreateOnCurrentThread()};

    DesktopWindowXamlSource^ dwxs = CreateDesktopWindowXamlSource(hwnd, "Window 2 MUX button");

    xamlIslandTests->PumpMessagesWhileWaitingForFlag(1);

    LOG_OUTPUT(L"  > Cleaning up DWXS 2.");
    dwxs = nullptr;

    LOG_OUTPUT(L"  > Pumping messages for DWXS cleanup.");
    DrainMessageQueue();
    dqc->ShutdownQueue();

    return 0;
}

void XamlIslandTests::TwoWindowsOnDifferentThreads()
{
    ClearFlags();

    wil::unique_handle uiThread1 { CreateThread(nullptr, 0, TwoWindowsOnDifferentThreads_ThreadProc1, this /* lpParameter */, 0, nullptr) };
    wil::unique_handle uiThread2 { CreateThread(nullptr, 0, TwoWindowsOnDifferentThreads_ThreadProc2, this /* lpParameter */, 0, nullptr) };

    LOG_OUTPUT(L"> Waiting a moment for rendering...");
    Sleep(1000);
    HWND hwnd1 = GetHwnd(1);
    HWND hwnd2 = GetHwnd(2);

    LOG_OUTPUT(L"> Clean up first island");
    SetFlagAndPokeHwnd(0, hwnd1);

    LOG_OUTPUT(L"> Waiting briefly for cleanup...");
    Sleep(100);

    LOG_OUTPUT(L"> Clean up second island");
    SetFlagAndPokeHwnd(1, hwnd2);

    WaitForSingleObjectWithTimeout(uiThread1);
    WaitForSingleObjectWithTimeout(uiThread2);
}

void XamlIslandTests::IslandStressWorker(IslandSceneKind sceneKind)
{
    ClearFlags();

    VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_MULTITHREADED));
    auto dqcOuter {DispatcherQueueController::CreateOnCurrentThread()};

    auto app { ref new XamlIslandTests_ApplicationWithMuxc()};

    auto run = [&](int id){
        HWND hwnd = this->CreateHwnd(id);
        auto dqc {DispatcherQueueController::CreateOnCurrentThread()};

        for (int i=0; i<100;++i)
        {
            ClearFlag(id);
            LOG_OUTPUT(L"Thread %d iteration %d", id, i);

            DesktopWindowXamlSource^ dwxs = CreateDesktopWindowXamlSource(hwnd, "Window 1 MUX button", sceneKind);

            auto callback = ref new ::Windows::Foundation::EventHandler<RenderedEventArgs^>(
                [this,id,hwnd](::Platform::Object^, RenderedEventArgs^) {
                    this->SetFlagAndPokeHwnd(id, hwnd);
                });
            auto renderedEventToken = (CompositionTarget::Rendered += callback);

            PumpMessagesWhileWaitingForFlag(id);

            CompositionTarget::Rendered -= renderedEventToken;

            // By letting the DWXS close and pumping messages, we make XAML do the extra work of shutting down on the thread.
            dwxs = nullptr;
            DrainMessageQueue();
        }
        dqc->ShutdownQueue();
    };

    constexpr int threadCount = 7;
    std::vector<wil::unique_handle> workerThreads;

    for (int i=0; i<threadCount; ++i)
    {
        workerThreads.push_back(RunOnNewThread([=](){ run(i+1); }));
    }

    for (auto& threadHandle : workerThreads)
    {
        WaitForSingleObjectWithTimeout(threadHandle);
    }

    app->Cleanup();
    app = nullptr;
    dqcOuter->ShutdownQueue();

    VERIFY_IS_TRUE(XamlIslandTests_ApplicationWithMuxc::DidDestructorRun());
}

void XamlIslandTests::IslandsOnDifferentThreadsStress()
{
    IslandStressWorker(IslandSceneKind::Blank);
}

void XamlIslandTests::IslandsOnDifferentThreadsWithButtonStress()
{
    IslandStressWorker(IslandSceneKind::WithButton);
}

void XamlIslandTests::IslandsOnDifferentThreadsWithManyControlsStress()
{
    IslandStressWorker(IslandSceneKind::ManyControls);
}

void XamlIslandTests::WindowsXamlManagerOnDifferentThreadsStress()
{
    ClearFlags();

    VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_MULTITHREADED));

    auto run = [&](int id){
        auto dqc {DispatcherQueueController::CreateOnCurrentThread()};

        for (int i=0; i<100;++i)
        {
            ClearFlag(id);
            LOG_OUTPUT(L"Thread %d iteration %d", id, i);

            WindowsXamlManager^ wxm = WindowsXamlManager::InitializeForCurrentThread();

            DispatcherQueueTimer^ dqTimer {dqc->DispatcherQueue->CreateTimer()};
            ::Windows::Foundation::TimeSpan timeSpan{};
            constexpr int milliseconds = 50;
            timeSpan.Duration = (int64)10000 * milliseconds;
            dqTimer->Interval = timeSpan;
            dqTimer->Tick += ref new ::Windows::Foundation::TypedEventHandler<
                DispatcherQueueTimer^,Platform::Object^>(
                    [id,&dqTimer,this](DispatcherQueueTimer^,Platform::Object^)
                    {
                        dqTimer->Stop();
                        SetFlagAndPokeHwnd(id, NULL /*hwnd*/);
                    });
            dqTimer->Start();

            PumpMessagesWhileWaitingForFlag(id);

            wxm = nullptr;
            DrainMessageQueue();
        }
        dqc->ShutdownQueue();
    };

    constexpr int threadCount = 7;
    std::array<wil::unique_handle, threadCount> handleArray;

    for (int i=0; i<threadCount; ++i)
    {
        handleArray[i] = RunOnNewThread([=](){ run(i+1); });
    }

    for (int i=0; i<threadCount; ++i)
    {
        WaitForSingleObjectWithTimeout(handleArray[i]);
    }
}

void XamlIslandTests::ApplicationLikeFileExplorerStress()
{
    ClearFlags();

    VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_MULTITHREADED));

    LOG_OUTPUT(L"Creating Application object...");
    auto app { ref new ApplicationLikeFileExplorer()};

    // We want to crash immediately when we hit a failure to keep the max amount of state.
    app->DebugSettings->FailFastOnErrors = true;

    auto run = [&](int id){
        auto dqc {DispatcherQueueController::CreateOnCurrentThread()};

        for (int i=0; i<2;++i)
        {
            ClearFlag(id);

            WindowsXamlManager^ wxm = WindowsXamlManager::InitializeForCurrentThread();

            DispatcherQueueTimer^ dqTimer {dqc->DispatcherQueue->CreateTimer()};
            ::Windows::Foundation::TimeSpan timeSpan{};
            constexpr int milliseconds = 50;
            timeSpan.Duration = (int64)10000 * milliseconds;
            dqTimer->Interval = timeSpan;
            dqTimer->Tick += ref new ::Windows::Foundation::TypedEventHandler<
                DispatcherQueueTimer^,Platform::Object^>(
                    [id,&dqTimer,this](DispatcherQueueTimer^,Platform::Object^)
                    {
                        dqTimer->Stop();
                        SetFlagAndPokeHwnd(id, NULL /*hwnd*/);
                    });
            dqTimer->Start();

            PumpMessagesWhileWaitingForFlag(id);

            wxm = nullptr;
            DrainMessageQueue();
        }
        dqc->ShutdownQueue();
    };

    // I was hoping we could run this in a loop and "Reset" some things in the Application object on
    // each iteration, but I only seem to hit interesting failures on the first iteration.
    //for (int outer=0;outer<50;++outer)
    {
        //LOG_OUTPUT(L"Iteration %d", outer);

        constexpr int threadCount = 2;
        std::array<wil::unique_handle, threadCount> handleArray;

        for (int i=0; i<threadCount; ++i)
        {
            handleArray[i] = RunOnNewThread([=](){ run(i+1); });
        }

        for (int i=0; i<threadCount; ++i)
        {
            WaitForSingleObjectWithTimeout(handleArray[i]);
        }

        app->Reset();
    }
}

DWORD WINAPI TwoDesktopIslandsInSameWindow_ThreadProc(_In_ LPVOID lpParameter)
{
    XamlIslandTests* xamlIslandTests = reinterpret_cast<XamlIslandTests*>(lpParameter);

    HWND hwnd = xamlIslandTests->CreateHwnd();

    auto dqc {DispatcherQueueController::CreateOnCurrentThread()};

    DesktopWindowXamlSource^ dwxs1 = CreateDesktopWindowXamlSource(hwnd, "Island 1 MUX button");
    DesktopWindowXamlSource^ dwxs2 = CreateDesktopWindowXamlSource(hwnd, "Island 2 MUX button");

    xamlIslandTests->PumpMessagesWhileWaitingForFlag(0);

    LOG_OUTPUT(L"  > Cleaning up DWXS 1.");
    dwxs1 = nullptr;

    LOG_OUTPUT(L"  > Pumping messages for DWXS cleanup.");
    xamlIslandTests->PumpMessagesWhileWaitingForFlag(1);

    LOG_OUTPUT(L"  > Cleaning up DWXS 2.");
    dwxs2 = nullptr;

    DrainMessageQueue();
    dqc->ShutdownQueue();

    return 0;
}

void XamlIslandTests::TwoDesktopIslandsInSameWindow()
{
    wil::unique_handle uiThread { CreateThread(nullptr, 0, TwoDesktopIslandsInSameWindow_ThreadProc, this /* lpParameter */, 0, nullptr)};

    LOG_OUTPUT(L"> Waiting 2 seconds for rendering...");
    Sleep(2000);
    HWND hwnd = GetHwnd();

    LOG_OUTPUT(L"> Clean up first island");
    SetFlagAndPokeHwnd(0, hwnd);

    LOG_OUTPUT(L"> Waiting 1 second for cleanup...");
    Sleep(1000);

    LOG_OUTPUT(L"> Clean up second island");
    SetFlagAndPokeHwnd(1, hwnd);

    LOG_OUTPUT(L"> Waiting 1 second for cleanup...");
    Sleep(1000);

    LOG_OUTPUT(L"> No crash. Exiting test.");
    TerminateThread(uiThread.get(), 0);
}

DWORD WINAPI TwoXamlIslandsInSameWindow_ThreadProc(_In_ LPVOID lpParameter)
{
    XamlIslandTests *xamlIslandTests = reinterpret_cast<XamlIslandTests *>(lpParameter);

    HWND hwnd = xamlIslandTests->CreateHwnd();
    mu::WindowId windowId;
    VERIFY_SUCCEEDED(GetWindowIdFromWindow(hwnd, (::ABI::Microsoft::UI::WindowId *)&windowId));

    auto dqc{DispatcherQueueController::CreateOnCurrentThread()};

    XamlIsland ^ xi1 = CreateXamlIsland("Island 1 MUX button");
    XamlIsland ^ xi2 = CreateXamlIsland("Island 2 MUX button");

    auto island1 = xi1->ContentIsland;
    auto island2 = xi2->ContentIsland;

    auto dispatcherQueue = island1->DispatcherQueue;

    auto dcsb1 = DesktopChildSiteBridge::CreateWithDispatcherQueue(dispatcherQueue, windowId);
    auto dcsb2 = DesktopChildSiteBridge::CreateWithDispatcherQueue(dispatcherQueue, windowId);

    dcsb1->Connect(island1);
    dcsb1->Show();

    dcsb2->Connect(island2);
    dcsb2->Show();

    xamlIslandTests->PumpMessagesWhileWaitingForFlag(0);

    LOG_OUTPUT(L"  > Cleaning up XI1.");
    xi1 = nullptr;

    LOG_OUTPUT(L"  > Pumping messages for XI cleanup.");
    xamlIslandTests->PumpMessagesWhileWaitingForFlag(1);

    LOG_OUTPUT(L"  > Cleaning up XI2.");
    xi2 = nullptr;

    DrainMessageQueue();
    dqc->ShutdownQueue();

    return 0;
}

void XamlIslandTests::TwoXamlIslandsInSameWindow()
{
    wil::unique_handle uiThread{CreateThread(nullptr, 0, TwoXamlIslandsInSameWindow_ThreadProc, this /* lpParameter */, 0, nullptr)};

    LOG_OUTPUT(L"> Waiting 2 seconds for rendering...");
    Sleep(2000);
    HWND hwnd = GetHwnd();

    LOG_OUTPUT(L"> Clean up first island");
    SetFlagAndPokeHwnd(0, hwnd);

    LOG_OUTPUT(L"> Waiting 1 second for cleanup...");
    Sleep(1000);

    LOG_OUTPUT(L"> Clean up second island");
    SetFlagAndPokeHwnd(1, hwnd);

    LOG_OUTPUT(L"> Waiting 1 second for cleanup...");
    Sleep(1000);

    LOG_OUTPUT(L"> Waiting for UI thread to exit...");
    WaitForSingleObjectWithTimeout(uiThread);

    LOG_OUTPUT(L"> No crash. Exiting test.");
}
void VerifyApplicationCurrentIsNull(const wchar_t* message)
{
    LOG_OUTPUT(message);
    Application^ currentApplication = Application::Current;
    VERIFY_IS_NULL(currentApplication);
}

void VerifyApplicationCurrentIsNotNull(const wchar_t* message)
{
    LOG_OUTPUT(message);
    Application^ currentApplication = Application::Current;
    VERIFY_IS_NOT_NULL(currentApplication);
}

void VerifyApplicationCurrentIsEqualTo(Application^ expected, const wchar_t* message)
{
    LOG_OUTPUT(message);
    Application^ currentApplication = Application::Current;
    VERIFY_IS_NOT_NULL(currentApplication);
    VERIFY_ARE_EQUAL(expected, currentApplication);
}

void VerifyApplicationCurrentIsNotEqualTo(Application^ expected, const wchar_t* message)
{
    LOG_OUTPUT(message);
    Application^ currentApplication = Application::Current;
    VERIFY_IS_NOT_NULL(currentApplication);
    VERIFY_ARE_NOT_EQUAL(expected, currentApplication);
}

// Template - copy for new "don't poison xaml" tests.
DWORD WINAPI ReinitializeXamlIslandOnUIThread_ThreadProc(_In_ LPVOID lpParameter)
{
    XamlIslandTests* xamlIslandTests = reinterpret_cast<XamlIslandTests*>(lpParameter);
    HWND hwnd = xamlIslandTests->CreateHwnd();

    // Application work before initializing Xaml

    LOG_OUTPUT(L"ui> Making DWXS.");
    DesktopWindowXamlSource^ dwxs = CreateDesktopWindowXamlSource(hwnd, "MUX button");

    // Application work after initializing Xaml

    xamlIslandTests->PumpMessagesWhileWaitingForFlag(10);

    LOG_OUTPUT(L"ui> Cleaning up DWXS.");
    dwxs = nullptr;

    LOG_OUTPUT(L"  ui> Pumping messages for DWXS cleanup.");
    xamlIslandTests->PumpMessagesWhileWaitingForFlag(20);

    // Application work after deinitializing Xaml, before reinitializing Xaml

    LOG_OUTPUT(L"  ui> Making second DWXS.");
    dwxs = CreateDesktopWindowXamlSource(hwnd, "MUX button 2");

    // Application work after reinitializing Xaml

    xamlIslandTests->PumpMessagesWhileWaitingForFlag(30);

    LOG_OUTPUT(L"ui> Cleaning up second DWXS.");
    dwxs = nullptr;

    LOG_OUTPUT(L"  ui> Pumping messages for second DWXS cleanup.");
    xamlIslandTests->PumpMessagesWhileWaitingForFlag(40);

    // Application work after deinitializing Xaml a second time

    return 0;
}

// Common test thread for "don't poison xaml" tests.
// The UI thread does different things but the test thread does the same thing - signal the UI thread to make an island, to tear it down, to make another one, to tear that one down, then to exit.
// Different tests should create the UI thread, then call into this.
void XamlIslandTests::ReinitializeXamlIslandOnUIThread_Common(wil::unique_handle& uiThread)
{
    LOG_OUTPUT(L"1> Waiting 2 seconds for island creation and rendering...");
    Sleep(2000);
    HWND hwnd = GetHwnd();

    LOG_OUTPUT(L"1> Signal to clean up first island.");
    SetFlagAndPokeHwnd(10, hwnd);

    LOG_OUTPUT(L"1> Waiting 2s for cleanup...");
    Sleep(2000);

    LOG_OUTPUT(L"1> Signal to make second island.");
    SetFlagAndPokeHwnd(20, hwnd);

    LOG_OUTPUT(L"1> Waiting 2s for creation and rendering...");
    Sleep(2000);

    LOG_OUTPUT(L"1> Signal to clean up second island.");
    SetFlagAndPokeHwnd(30, hwnd);

    LOG_OUTPUT(L"1> Waiting 2s for cleanup...");
    Sleep(2000);

    LOG_OUTPUT(L"1> Signal to exit UI thread.");
    SetFlagAndPokeHwnd(40, hwnd);

    LOG_OUTPUT(L"1> Waiting for UI thread to exit...");
    WaitForSingleObjectWithTimeout(uiThread);

    LOG_OUTPUT(L"1> No crash. Exiting test.");
}

struct TestParams
{
    XamlIslandTests* XamlIslandTests;
    xaml::ShutdownModel ShutdownModel;
};

DWORD WINAPI ReinitializeXamlIslandOnUIThread_DefaultApplication_NoReferenceToApplication_ThreadProc(_In_ LPVOID lpParameter)
{
    TestParams* tp = reinterpret_cast<TestParams*>(lpParameter);
    XamlIslandTests* xamlIslandTests = tp->XamlIslandTests;
    HWND hwnd = xamlIslandTests->CreateHwnd();

    // Application work before initializing Xaml
    VerifyApplicationCurrentIsNull(L"ui> Xaml not initialized - Application::Current should return null.");

    auto dqc {DispatcherQueueController::CreateOnCurrentThread()};

    LOG_OUTPUT(L"ui> Making DWXS.");
    DesktopWindowXamlSource^ dwxs = CreateDesktopWindowXamlSource(hwnd, "MUX button");

    // Application work after initializing Xaml
    VerifyApplicationCurrentIsNotNull(L"ui> Xaml initialized - Application::Current should return something.");
    dynamic_cast<xaml::IFrameworkApplicationPrivate^>(Application::Current)->ShutdownModel = tp->ShutdownModel;

    xamlIslandTests->PumpMessagesWhileWaitingForFlag(10);

    LOG_OUTPUT(L"ui> Cleaning up DWXS.");
    dwxs = nullptr;

    LOG_OUTPUT(L"  ui> Pumping messages for DWXS cleanup.");
    xamlIslandTests->PumpMessagesWhileWaitingForFlag(20);

    if (tp->ShutdownModel == xaml::ShutdownModel::Version1)
    {
        // Application work after deinitializing Xaml, before reinitializing Xaml
        VerifyApplicationCurrentIsNull(L"ui> Xaml deinitialized - Application::Current should return null again.");
    }
    else
    {
        VerifyApplicationCurrentIsNotNull(L"ui> Xaml still alive - Application::Current should return something.");
    }

    LOG_OUTPUT(L"  ui> Making second DWXS.");
    dwxs = CreateDesktopWindowXamlSource(hwnd, "MUX button 2");

    // Application work after reinitializing Xaml
    VerifyApplicationCurrentIsNotNull(L"ui> Xaml reinitialized - Application::Current should return something.");
    dynamic_cast<xaml::IFrameworkApplicationPrivate^>(Application::Current)->ShutdownModel = tp->ShutdownModel;

    xamlIslandTests->PumpMessagesWhileWaitingForFlag(30);

    LOG_OUTPUT(L"ui> Cleaning up second DWXS.");
    dwxs = nullptr;

    LOG_OUTPUT(L"  ui> Pumping messages for second DWXS cleanup.");
    xamlIslandTests->PumpMessagesWhileWaitingForFlag(40);
    dqc->ShutdownQueue();

    // Application work after deinitializing Xaml a second time
    VerifyApplicationCurrentIsNull(L"ui> Xaml deinitialized again - Application::Current should return null again.");

    return 0;
}

void XamlIslandTests::ReinitializeXamlIslandOnUIThread_DefaultApplication_NoReferenceToApplication()
{
    LOG_OUTPUT(L"1> Creating UI thread.");
    TestParams tp {this, xaml::ShutdownModel::Version2};
    wil::unique_handle uiThread {CreateThread(nullptr, 0, ReinitializeXamlIslandOnUIThread_DefaultApplication_NoReferenceToApplication_ThreadProc, &tp /* lpParameter */, 0, nullptr)};

    ReinitializeXamlIslandOnUIThread_Common(uiThread);
}

DWORD WINAPI ReinitializeXamlIslandOnUIThread_DefaultApplication_KeepApplicationAlive_ThreadProc(_In_ LPVOID lpParameter)
{
    TestParams* tp = reinterpret_cast<TestParams*>(lpParameter);
    XamlIslandTests* xamlIslandTests = tp->XamlIslandTests;
    HWND hwnd = xamlIslandTests->CreateHwnd();

    // Application work before initializing Xaml
    VerifyApplicationCurrentIsNull(L"ui> Xaml not initialized - Application::Current should return null.");

    auto dqc {DispatcherQueueController::CreateOnCurrentThread()};

    LOG_OUTPUT(L"ui> Making DWXS.");
    DesktopWindowXamlSource^ dwxs = CreateDesktopWindowXamlSource(hwnd, "MUX button");

    // Application work after initializing Xaml
    Application^ referenceToApplication = Application::Current;
    auto shutdownModel = dynamic_cast<xaml::IFrameworkApplicationPrivate^>(Application::Current)->ShutdownModel;
    VerifyApplicationCurrentIsEqualTo(referenceToApplication, L"ui> Xaml initialized - Application::Current should return something.");

    xamlIslandTests->PumpMessagesWhileWaitingForFlag(10);

    LOG_OUTPUT(L"ui> Cleaning up DWXS.");
    dwxs = nullptr;

    LOG_OUTPUT(L"  ui> Pumping messages for DWXS cleanup.");
    xamlIslandTests->PumpMessagesWhileWaitingForFlag(20);

    if (shutdownModel == xaml::ShutdownModel::Version1)
    {
        // Application work after deinitializing Xaml, before reinitializing Xaml
        VerifyApplicationCurrentIsNull(L"ui> Xaml deinitialized but Application is kept alive by the app - Application::Current should still return null.");
    }
    else
    {
        VerifyApplicationCurrentIsNotNull(L"ui> Application is still alive because ShutdownModel is version 2.");
    }

    LOG_OUTPUT(L"  ui> Making second DWXS.");
    dwxs = CreateDesktopWindowXamlSource(hwnd, "MUX button 2");

    // Application work after reinitializing Xaml
    VerifyApplicationCurrentIsEqualTo(referenceToApplication, L"ui> Xaml reinitialized while Application is kept alive by the app - Application::Current should not have changed.");

    xamlIslandTests->PumpMessagesWhileWaitingForFlag(30);

    LOG_OUTPUT(L"ui> Cleaning up second DWXS and Application.");
    dwxs = nullptr;
    referenceToApplication = nullptr;

    LOG_OUTPUT(L"  ui> Pumping messages for second DWXS cleanup.");
    xamlIslandTests->PumpMessagesWhileWaitingForFlag(40);
    dqc->ShutdownQueue();

    // Application work after deinitializing Xaml a second time
    VerifyApplicationCurrentIsNull(L"ui> Xaml deinitialized again and Application released - Application::Current should return null again.");

    return 0;
}

void XamlIslandTests::ReinitializeXamlIslandOnUIThread_DefaultApplication_KeepApplicationAlive()
{
    LOG_OUTPUT(L"1> Creating UI thread.");
    TestParams tp {this, xaml::ShutdownModel::Version2};
    wil::unique_handle uiThread {CreateThread(nullptr, 0, ReinitializeXamlIslandOnUIThread_DefaultApplication_KeepApplicationAlive_ThreadProc, &tp /* lpParameter */, 0, nullptr)};

    ReinitializeXamlIslandOnUIThread_Common(uiThread);
}


DWORD WINAPI ReinitializeXamlIslandOnUIThread_CustomApplication_NoReferenceToApplication_ThreadProc(_In_ LPVOID lpParameter)
{
    TestParams* tp = reinterpret_cast<TestParams*>(lpParameter);
    XamlIslandTests* xamlIslandTests = tp->XamlIslandTests;
    HWND hwnd = xamlIslandTests->CreateHwnd();

    auto dqc {DispatcherQueueController::CreateOnCurrentThread()};

    // Application work before initializing Xaml
    LOG_OUTPUT(L"ui> Making a new custom XamlIslandTests_Application.");
    XamlIslandTests_Application^ customApplication = ref new XamlIslandTests_Application();
    dynamic_cast<xaml::IFrameworkApplicationPrivate^>(Application::Current)->ShutdownModel = tp->ShutdownModel;
    VERIFY_IS_NOT_NULL(customApplication);
    VerifyApplicationCurrentIsEqualTo(customApplication, L"ui> Xaml not initialized - Application::Current should return the custom XamlIslandTests_Application.");

    LOG_OUTPUT(L"ui> Making DWXS.");
    DesktopWindowXamlSource^ dwxs = CreateDesktopWindowXamlSource(hwnd, "MUX button");

    // Application work after initializing Xaml
    VerifyApplicationCurrentIsEqualTo(customApplication, L"ui> Xaml initialized - Application::Current should return the custom XamlIslandTests_Application. OnLaunch should be called once.");
    VERIFY_ARE_EQUAL(1, customApplication->GetOnLaunchedCallCount());

    xamlIslandTests->PumpMessagesWhileWaitingForFlag(10);

    LOG_OUTPUT(L"ui> Cleaning up DWXS and custom XamlIslandTests_Application.");
    dwxs = nullptr;
    customApplication = nullptr;

    LOG_OUTPUT(L"  ui> Pumping messages for DWXS cleanup.");
    xamlIslandTests->PumpMessagesWhileWaitingForFlag(20);

    if (tp->ShutdownModel == xaml::ShutdownModel::Version1)
    {
        // Application work after deinitializing Xaml, before reinitializing Xaml
        VerifyApplicationCurrentIsNull(L"ui> Xaml deinitialized - Application::Current should return null again.");

        LOG_OUTPUT(L"ui> Making a new custom XamlIslandTests_Application again.");
        customApplication = ref new XamlIslandTests_Application();
    }
    else
    {
        VerifyApplicationCurrentIsNotNull(L"ui> Application is still alive because ShutdownModel is version 2.");
        customApplication = safe_cast<XamlIslandTests_Application^>(Application::Current);
    }

    VERIFY_IS_NOT_NULL(customApplication);
    VerifyApplicationCurrentIsEqualTo(customApplication, L"ui> Xaml not reinitialized yet - Application::Current should return the custom XamlIslandTests_Application.");

    LOG_OUTPUT(L"  ui> Making second DWXS.");
    dwxs = CreateDesktopWindowXamlSource(hwnd, "MUX button 2");

    // Application work after reinitializing Xaml
    VerifyApplicationCurrentIsEqualTo(customApplication, L"ui> Xaml reinitialized - Application::Current should return the custom XamlIslandTests_Application. OnLaunch should be called once.");
    VERIFY_ARE_EQUAL(1, customApplication->GetOnLaunchedCallCount());

    xamlIslandTests->PumpMessagesWhileWaitingForFlag(30);

    LOG_OUTPUT(L"ui> Cleaning up second DWXS and custom XamlIslandTests_Application.");
    dwxs = nullptr;
    customApplication = nullptr;

    LOG_OUTPUT(L"  ui> Pumping messages for second DWXS cleanup.");
    xamlIslandTests->PumpMessagesWhileWaitingForFlag(40);
    dqc->ShutdownQueue();

    // Application work after deinitializing Xaml a second time
    VerifyApplicationCurrentIsNull(L"ui> Xaml deinitialized again - Application::Current should return null again.");

    return 0;
}

void XamlIslandTests::ReinitializeXamlIslandOnUIThread_CustomApplication_NoReferenceToApplication_v1Shutdown()
{
    LOG_OUTPUT(L"1> Creating UI thread.");
    TestParams tp {this, xaml::ShutdownModel::Version1};
    wil::unique_handle uiThread {CreateThread(nullptr, 0, ReinitializeXamlIslandOnUIThread_CustomApplication_NoReferenceToApplication_ThreadProc, &tp /* lpParameter */, 0, nullptr)};

    ReinitializeXamlIslandOnUIThread_Common(uiThread);
}

void XamlIslandTests::ReinitializeXamlIslandOnUIThread_CustomApplication_NoReferenceToApplication()
{
    LOG_OUTPUT(L"1> Creating UI thread.");
    TestParams tp {this, xaml::ShutdownModel::Version2};
    wil::unique_handle uiThread {CreateThread(nullptr, 0, ReinitializeXamlIslandOnUIThread_CustomApplication_NoReferenceToApplication_ThreadProc, &tp /* lpParameter */, 0, nullptr)};

    ReinitializeXamlIslandOnUIThread_Common(uiThread);
}



DWORD WINAPI ReinitializeXamlIslandOnUIThread_CustomApplication_KeepApplicationAlive_ThreadProc(_In_ LPVOID lpParameter)
{
    TestParams* tp = reinterpret_cast<TestParams*>(lpParameter);
    XamlIslandTests* xamlIslandTests = tp->XamlIslandTests;
    HWND hwnd = xamlIslandTests->CreateHwnd();

    auto dqc {DispatcherQueueController::CreateOnCurrentThread()};

    // Application work before initializing Xaml
    LOG_OUTPUT(L"ui> Making a new custom XamlIslandTests_Application.");
    XamlIslandTests_Application^ customApplication = ref new XamlIslandTests_Application();
    dynamic_cast<xaml::IFrameworkApplicationPrivate^>(Application::Current)->ShutdownModel = tp->ShutdownModel;
    VERIFY_IS_NOT_NULL(customApplication);
    VerifyApplicationCurrentIsEqualTo(customApplication, L"ui> Xaml not initialized - Application::Current should return the custom XamlIslandTests_Application.");

    LOG_OUTPUT(L"ui> Making DWXS.");
    DesktopWindowXamlSource^ dwxs = CreateDesktopWindowXamlSource(hwnd, "MUX button");

    // Application work after initializing Xaml
    VerifyApplicationCurrentIsEqualTo(customApplication, L"ui> Xaml initialized - Application::Current should return the custom XamlIslandTests_Application. OnLaunch should be called once.");
    VERIFY_ARE_EQUAL(1, customApplication->GetOnLaunchedCallCount());

    xamlIslandTests->PumpMessagesWhileWaitingForFlag(10);

    LOG_OUTPUT(L"ui> Cleaning up DWXS.");
    dwxs = nullptr;

    LOG_OUTPUT(L"  ui> Pumping messages for DWXS cleanup.");
    xamlIslandTests->PumpMessagesWhileWaitingForFlag(20);

    if (tp->ShutdownModel == xaml::ShutdownModel::Version1)
    {
        // Application work after deinitializing Xaml, before reinitializing Xaml
        VerifyApplicationCurrentIsNull(L"ui> Xaml deinitialized but custom XamlIslandTests_Application is kept alive - Application::Current should still return null.");
    }
    else
    {
        VerifyApplicationCurrentIsNotNull(L"ui> Application is still alive because ShutdownModel is version 2.");
    }

    LOG_OUTPUT(L"  ui> Making second DWXS.");
    dwxs = CreateDesktopWindowXamlSource(hwnd, "MUX button 2");

    // Application work after reinitializing Xaml
    VerifyApplicationCurrentIsEqualTo(customApplication, L"ui> Xaml reinitialized - Application::Current should pick up the previous custom XamlIslandTests_Application.");
    LOG_OUTPUT(L"ui> The old custom XamlIslandTests_Application's OnLaunch should be called again.");

    if (tp->ShutdownModel == xaml::ShutdownModel::Version1)
    {
        VERIFY_ARE_EQUAL(2, customApplication->GetOnLaunchedCallCount());
    }
    else
    {
        VERIFY_ARE_EQUAL(1, customApplication->GetOnLaunchedCallCount());
    }


    xamlIslandTests->PumpMessagesWhileWaitingForFlag(30);

    LOG_OUTPUT(L"ui> Cleaning up second DWXS.");
    dwxs = nullptr;
    customApplication = nullptr;

    LOG_OUTPUT(L"  ui> Pumping messages for second DWXS cleanup.");
    xamlIslandTests->PumpMessagesWhileWaitingForFlag(40);
    dqc->ShutdownQueue();

    // Application work after deinitializing Xaml a second time
    VerifyApplicationCurrentIsNull(L"ui> Xaml deinitialized again - Application::Current should return null again.");

    return 0;
}

void XamlIslandTests::ReinitializeXamlIslandOnUIThread_CustomApplication_KeepApplicationAlive_v1Shutdown()
{
    LOG_OUTPUT(L"1> Creating UI thread.");
    TestParams tp {this, xaml::ShutdownModel::Version1};
    wil::unique_handle uiThread {CreateThread(nullptr, 0, ReinitializeXamlIslandOnUIThread_CustomApplication_KeepApplicationAlive_ThreadProc, &tp /* lpParameter */, 0, nullptr)};

    ReinitializeXamlIslandOnUIThread_Common(uiThread);
}

void XamlIslandTests::ReinitializeXamlIslandOnUIThread_CustomApplication_KeepApplicationAlive()
{
    LOG_OUTPUT(L"1> Creating UI thread.");
    TestParams tp {this, xaml::ShutdownModel::Version2};
    wil::unique_handle uiThread {CreateThread(nullptr, 0, ReinitializeXamlIslandOnUIThread_CustomApplication_KeepApplicationAlive_ThreadProc, &tp /* lpParameter */, 0, nullptr)};

    ReinitializeXamlIslandOnUIThread_Common(uiThread);
}


DWORD WINAPI MultipleApplicationObjects_ThreadProc(_In_ LPVOID lpParameter)
{
    XamlIslandTests* xamlIslandTests = reinterpret_cast<XamlIslandTests*>(lpParameter);
    HWND hwnd = xamlIslandTests->CreateHwnd();

    // Application work before initializing Xaml
    LOG_OUTPUT(L"ui> Making a new custom XamlIslandTests_Application.");
    XamlIslandTests_Application^ customApplication = ref new XamlIslandTests_Application();
    VERIFY_IS_NOT_NULL(customApplication);
    VerifyApplicationCurrentIsEqualTo(customApplication, L"ui> Xaml not initialized - Application::Current should return the custom XamlIslandTests_Application.");

    bool didApplicationCreationFail = false;
    try
    {
        //
        // This just crashes before it can throw an exception.
        //
        // The failure HR gets this far before the crash:
        //
        // 0:008> k
        //  # ChildEBP RetAddr
        // 00 0578fbb8 6e34ab17     Microsoft_UI_Xaml_Tests_External_Foundation_Hosting!Microsoft::UI::Xaml::IApplicationFactory::CreateInstance+0x7b
        // 01 0578fc0c 6e34b242     Microsoft_UI_Xaml_Tests_External_Foundation_Hosting!Microsoft::UI::Xaml::Application::Application+0xb7
        // 02 0578fc68 6e34cd8f     Microsoft_UI_Xaml_Tests_External_Foundation_Hosting!Microsoft::UI::Xaml::Tests::Foundation::Hosting::XamlIslandTests_Application::XamlIslandTests_Application+0x92 [G:\l-c\dxaml\test\native\external\foundation\hosting\XamlIslandTests.cpp @ 19]
        // 03 0578fd14 75897ba9     Microsoft_UI_Xaml_Tests_External_Foundation_Hosting!Microsoft::UI::Xaml::Tests::Foundation::Hosting::MultipleApplicationObjects_ThreadProc+0x16f [G:\l-c\dxaml\test\native\external\foundation\hosting\XamlIslandTests.cpp @ 683]
        // 04 0578fd24 77a4c12b     KERNEL32!BaseThreadInitThunk+0x19 [clientcore\base\win32\client\thread.c @ 77]
        // 05 0578fd7c 77a4c0ae     ntdll!__RtlUserThreadStart+0x2b [minkernel\ntdll\rtlstrt.c @ 1192]
        // 06 0578fd8c 00000000     ntdll!_RtlUserThreadStart+0x1b [minkernel\ntdll\rtlstrt.c @ 1073]
        //
        // The lesson here seems to be "don't throw from a ctor".
        //
        XamlIslandTests_Application^ customApplication2 = ref new XamlIslandTests_Application();
    }
    catch (Platform::Exception^)
    {
        didApplicationCreationFail = true;
    }

    VERIFY_IS_TRUE(didApplicationCreationFail);

    return 0;
}

void XamlIslandTests::MultipleApplicationObjects()
{
    LOG_OUTPUT(L"1> Creating UI thread.");
    wil::unique_handle uiThread { CreateThread(nullptr, 0, MultipleApplicationObjects_ThreadProc, this /* lpParameter */, 0, nullptr) };

    LOG_OUTPUT(L"1> Waiting for UI thread to exit...");
    WaitForSingleObjectWithTimeout(uiThread);

    LOG_OUTPUT(L"1> No crash. Exiting test.");
}

void XamlIslandTests::FailFastWhenXamlNotShutdown()
{
    auto uiThread = RunOnNewThread([]() {
        VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_MULTITHREADED));

        auto dqc {DispatcherQueueController::CreateOnCurrentThread()};

        LOG_OUTPUT(L"Creating WindowsXamlManager...");
        WindowsXamlManager^ wxm = WindowsXamlManager::InitializeForCurrentThread();

        dqc->ShutdownQueue();
    });

    WaitForSingleObjectWithTimeout(uiThread);
}

void XamlIslandTests::FailFastWhenMessagesNotPumpedAfterClosing()
{
    auto uiThread = RunOnNewThread([]() {
        VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_MULTITHREADED));

        auto dqc {DispatcherQueueController::CreateOnCurrentThread()};

        LOG_OUTPUT(L"Creating WindowsXamlManager...");
        WindowsXamlManager^ wxm = WindowsXamlManager::InitializeForCurrentThread();

        DrainMessageQueue();

        LOG_OUTPUT(L"Closing WindowsXamlManager...");
        CloseObject(wxm);

        LOG_OUTPUT(L"Calling DispatcherQueueController::ShutdownQueue...");
        dqc->ShutdownQueue();
    });

    WaitForSingleObjectWithTimeout(uiThread);
}


void XamlIslandTests::IslandWithMuxcDoesntPoisonThread()
{
    VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_MULTITHREADED));

    auto dqcOuter {DispatcherQueueController::CreateOnCurrentThread()};

    LOG_OUTPUT(L"Creating Application object...");
    auto app { ref new XamlIslandTests_ApplicationWithMuxc()};

    auto uiThread = RunOnNewThread([this]() {
        VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_MULTITHREADED));

        HWND hwnd1 = this->CreateHwnd(1);

        for (int i=0; i<3; ++i)
        {
            LOG_OUTPUT(L"> Creating DispatcherQueue (iteration %d).", i);
            auto dqc {DispatcherQueueController::CreateOnCurrentThread()};

            LOG_OUTPUT(L"Create 3 distinct instances of DesktopWindowXamlSource in the same Window on the same thread.");
            for (int j=0; j<3; ++j)
            {
                LOG_OUTPUT(L"  > Creating DesktopWindowXamlSource (iteration %d.%d).", i, j);
                auto dwxs = CreateDesktopWindowXamlSource(hwnd1, L"Button number 1", IslandSceneKind::WithNumberBox);

                DrainMessageQueue();

                DispatcherQueueTimer^ dqTimer {dqc->DispatcherQueue->CreateTimer()};
                ::Windows::Foundation::TimeSpan timeSpan{};
                timeSpan.Duration = (int64)10000 * 100; // 100ms
                dqTimer->Interval = timeSpan;
                dqTimer->Tick += ref new ::Windows::Foundation::TypedEventHandler<
                    DispatcherQueueTimer^,Platform::Object^>(
                        [hwnd1,j,&dqTimer,this](DispatcherQueueTimer^,Platform::Object^)
                        {
                            dqTimer->Stop();
                            LOG_OUTPUT(L"  > Tick event fired.");
                            SetFlagAndPokeHwnd(j, hwnd1);
                        });
                dqTimer->Start();

                LOG_OUTPUT(L"  > Waiting for DispatcherQueueTimer.Tick to fire.");
                PumpMessagesWhileWaitingForFlag(j);

                LOG_OUTPUT(L"  > Closing DesktopWindowXamlSource.");
                CloseObject(dwxs);
                DrainMessageQueue();
            }

            LOG_OUTPUT(L"Calling DispatcherQueueController::ShutdownQueue.");
            dqc->ShutdownQueue();
        }
    });

    WaitForSingleObjectWithTimeout(uiThread);

    LOG_OUTPUT(L"Removing last WindowsXamlManager.");
    app->Cleanup();
    DrainMessageQueue();

    LOG_OUTPUT(L"Shutdown outer DispatcherQueue.");
    dqcOuter->ShutdownQueue();

    LOG_OUTPUT(L"Releasing reference to app.");
    app = nullptr;
    DrainMessageQueue();
}

void XamlIslandTests::IslandsWithSuddenShutdown()
{
    ClearFlags();
    VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_SINGLETHREADED));

    auto dqcOuter {DispatcherQueueController::CreateOnCurrentThread()};

    LOG_OUTPUT(L"Creating Application object...");
    auto app { ref new XamlIslandTests_ApplicationWithMuxc()};
    bool buttonUnloaded{false};

    auto uiThread = RunOnNewThread([&]() {
        VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_SINGLETHREADED));

        auto dqc {DispatcherQueueController::CreateOnCurrentThread()};

        std::array<bool,2> buttonUnloadedFired = {false, false};
        std::array<DesktopWindowXamlSource^,2> desktopWindowXamlSources;
        std::array<HWND,2> hwnds {NULL,NULL};

        for (int i=0; i<2; ++i)
        {
            hwnds[i] = this->CreateHwnd(i);

            LOG_OUTPUT(L"  > Creating DesktopWindowXamlSource.");
            auto dwxs = CreateDesktopWindowXamlSource(hwnds[i], L"Button", IslandSceneKind::WithNumberBox);
            desktopWindowXamlSources[i] = dwxs;

            Button^ b = ref new Button();
            b->Unloaded += ref new RoutedEventHandler([i,&buttonUnloadedFired](Platform::Object^, RoutedEventArgs^) {
                LOG_OUTPUT(L"    > Button %d unloaded.", i);
                buttonUnloadedFired[i] = true;
            });
            safe_cast<StackPanel^>(dwxs->Content)->Children->Append(b);
        }

        bool tickEventFired {false};
        DispatcherQueueTimer^ dqTimer {dqc->DispatcherQueue->CreateTimer()};
        ::Windows::Foundation::TimeSpan timeSpan{};
        timeSpan.Duration = (int64)10000 * 100; // 100ms
        dqTimer->Interval = timeSpan;
        dqTimer->Tick += ref new ::Windows::Foundation::TypedEventHandler<
            DispatcherQueueTimer^,Platform::Object^>(
                [&](DispatcherQueueTimer^,Platform::Object^)
                {
                    dqTimer->Stop();
                    LOG_OUTPUT(L"  > Tick event fired.");
                    SetFlagAndPokeHwnd(1, hwnds[0]);
                });
        dqTimer->Start();

        LOG_OUTPUT(L"  > Waiting for DispatcherQueueTimer.Tick to fire.");
        PumpMessagesWhileWaitingForFlag(1);


        VERIFY_IS_FALSE(buttonUnloadedFired[0]);
        VERIFY_IS_FALSE(buttonUnloadedFired[1]);

        LOG_OUTPUT(L"Calling DispatcherQueueController::ShutdownQueue.");
        dqc->ShutdownQueue();

        VERIFY_IS_TRUE(buttonUnloadedFired[0]);
        VERIFY_IS_TRUE(buttonUnloadedFired[1]);
    });

    WaitForSingleObjectWithTimeout(uiThread);

    LOG_OUTPUT(L"Shutdown outer DispatcherQueue.");
    dqcOuter->ShutdownQueue();
    app->Cleanup();
    app = nullptr;
}

void XamlIslandTests::ShutdownWithLeakDetection()
{
    XamlIslandTestHelper testHelper(this);
    testHelper.StartAppOnCurrentThread(RO_INIT_MULTITHREADED);

    HWND hwnd1 {NULL};
    DispatcherQueueController^ dqc;
    DispatcherQueue^ dq;
    WindowsXamlManager^ wxm;
    DesktopWindowXamlSource^ dwxs;
    Event uiThreadReady;

    auto uiThread = RunOnNewThread([&]()
    {
        VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_MULTITHREADED));

        hwnd1 = this->CreateHwnd(1);

        dqc = DispatcherQueueController::CreateOnCurrentThread();
        dq = dqc->DispatcherQueue;
        uiThreadReady.Set();

        wxm = WindowsXamlManager::InitializeForCurrentThread();

        dqc->DispatcherQueue->RunEventLoop();

        LOG_OUTPUT(L"  > Exiting UI Thread.");
    });
    uiThreadReady.WaitForDefault();

    RunOnDispatcherThread(dq, true, [&]()
    {
        LOG_OUTPUT(L"  > Creating DesktopWindowXamlSource.");

        // Note setting useMuxc to "true" here triggers a leak for an AutomationProperty on NumberBox
        dwxs = CreateDesktopWindowXamlSource(hwnd1, L"Button number 1", IslandSceneKind::WithButton);

        // uncomment lines to test leak detection:
        //auto leakyButton = ref new Button();
        //reinterpret_cast<IInspectable*>(safe_cast<Platform::Object^>(leakyButton))->AddRef();
    });

    // Short sleep just to allow DWXS to be briefly visible.
    ::Sleep(100);

    RunOnDispatcherThread(dq, true, [&]()
    {
        LOG_OUTPUT(L"Closing up objects on test thread.");
        CloseObject(dwxs);
        dwxs = nullptr;

        // TODO: We shouldn't need to do this, but we leak if we don't.
        // TODO: Fix XAML Resources per-thread leak for islands apps
        Application::Current->Resources = nullptr;
    });

    RunOnDispatcherThread(dq, true, [&]()
    {
        LOG_OUTPUT(L"Calling DispatcherQueueController::ShutdownQueue.");
        dqc->ShutdownQueue();
        wxm = nullptr;
    });

    // TODO: We shouldn't need to do this, but we leak if we don't.
    // TODO: Fix XAML Resources per-thread leak for islands apps
    Application::Current->Resources = nullptr;

    testHelper.Close();

    Microsoft::UI::Xaml::DxamlCoreTestHooks::PerformProcessWideLeakDetection(20 /*max stacks to show*/);

    LOG_OUTPUT(L"Sending WM_QUIT to make UI thread exit.");
    ::DestroyWindow(hwnd1);
    ::PostMessage(hwnd1, WM_QUIT, 0, 0);
    WaitForSingleObjectWithTimeout(uiThread);
}

void XamlIslandTests::ShowTeachingTipWhileIslandClosing()
{
    XamlIslandTestHelper testHelper(this);
    testHelper.StartAppOnCurrentThread();
    testHelper.StartAndPrepNewUIThreadWithWindow();

    DesktopWindowXamlSource^ dwxs;
    Microsoft::UI::Xaml::Controls::TeachingTip^ teachingTip;

    testHelper.RunOnIslandUIThread([&]()
    {
        LOG_OUTPUT(L"  > Creating DesktopWindowXamlSource.");

        // Note setting useMuxc to "true" here triggers a leak for an AutomationProperty on NumberBox
        dwxs = CreateDesktopWindowXamlSource(testHelper.m_topLevelHwnd, L"Button number 1", IslandSceneKind::WithNumberBox);

        StackPanel^ sp = safe_cast<StackPanel^>(dwxs->Content);

        teachingTip = ref new Microsoft::UI::Xaml::Controls::TeachingTip();
        teachingTip->Title = L"My Teaching Tip";
        teachingTip->Target = sp;
        sp->Children->Append(teachingTip);
    });

    // Short sleep just to allow DWXS to be briefly visible.
    ::Sleep(100);

    XamlRoot^ xamlRoot = nullptr;
    testHelper.RunOnIslandUIThread([&]()
    {
        LOG_OUTPUT(L"Open the TeachingTip.");
        teachingTip->IsOpen = true;

        // There's some strange behavior of the XamlRoot property here.  We need to hold on to a reference to the
        // XamlRoot to reproduce the bug we're hitting in the wild.  If we don't hold a reference, then when MUXC
        // queries TeachingTip.XamlRoot after the DesktopWindowXamlSource is closed, it the XamlRoot is null; if we _do_
        // hold a reference, then it returns the XamlRoot. This is because each element has a weak-ref pointer to the
        // VisualTree that contains it.  When the DesktopWindowXamlSource is closed, and if the app isn't holding any
        // references to the VisualTree, all the strong refs to the VisualTree are gone so it gets deleted. By holding a
        // reference to the XamlRoot, we keep a strong ref alive on the VisualTree.
        xamlRoot = dwxs->Content->XamlRoot;

        LOG_OUTPUT(L"Closing DesktopWindowXamlSource just after teachingTip is opened.");
        CloseObject(dwxs);
        dwxs = nullptr;

        LOG_OUTPUT(L"Ensure XamlRoot returns default values when DesktopWindowXamlSource is no longer live.");
        auto size = xamlRoot->Size;
        VERIFY_ARE_EQUAL(0.0f, size.Width);
        VERIFY_ARE_EQUAL(0.0f, size.Height);
        VERIFY_IS_FALSE(xamlRoot->IsHostVisible);
        VERIFY_IS_NULL(xamlRoot->Content);
    });

    ::Sleep(100);
}

ref class CustomBackdrop_TrackOnConnected : public SystemBackdrop
{
public:
    void OnTargetConnected(ICompositionSupportsSystemBackdrop^ connectedTarget, XamlRoot^ xamlRoot) override
    {
        LOG_OUTPUT(L"    > SystemBackdrop::OnTargetConnected called.");
        m_connectedCount++;
        m_previousConnectedTarget = connectedTarget;
    }

    void OnTargetDisconnected(ICompositionSupportsSystemBackdrop^ disconnectedTarget) override
    {
        LOG_OUTPUT(L"    > SystemBackdrop::OnTargetDisconnected called.");
        m_disconnectedCount++;
        m_previousDisconnectedTarget = disconnectedTarget;
    }

    int GetConnectedCount() { return m_connectedCount; }
    ICompositionSupportsSystemBackdrop^ GetPreviousConnectedTarget() { return m_previousConnectedTarget; }

    int GetDisconnectedCount() { return m_disconnectedCount; }
    ICompositionSupportsSystemBackdrop^ GetPreviousDisconnectedTarget() { return m_previousDisconnectedTarget; }

private:
    int m_connectedCount {0};
    ICompositionSupportsSystemBackdrop^ m_previousConnectedTarget;

    int m_disconnectedCount {0};
    ICompositionSupportsSystemBackdrop^ m_previousDisconnectedTarget;
};

void XamlIslandTests::IslandSystemBackdrop()
{
    VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_MULTITHREADED));

    auto uiThread = RunOnNewThread([this]() {
        VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_MULTITHREADED));

        HWND hwnd = CreateHwnd(1);
        mu::WindowId windowId;
        VERIFY_SUCCEEDED(GetWindowIdFromWindow(hwnd, (::ABI::Microsoft::UI::WindowId *)&windowId));

        auto dqc {DispatcherQueueController::CreateOnCurrentThread()};

        auto xi = CreateXamlIsland("Xaml Island Button");
        auto island = xi->ContentIsland;
        auto dispatcherQueue = island->DispatcherQueue;

        auto dcsb = DesktopChildSiteBridge::CreateWithDispatcherQueue(dispatcherQueue, windowId);

        dcsb->Connect(island);
        dcsb->Show();

        CustomBackdrop_TrackOnConnected^ customBackdrop = ref new CustomBackdrop_TrackOnConnected();

        ICompositionSupportsSystemBackdrop^ xiICSSB = dynamic_cast<ICompositionSupportsSystemBackdrop^>(xi);

        LOG_OUTPUT(L"  > Attaching SystemBackdrop. This should call OnTargetConnected.");
        xi->SystemBackdrop = customBackdrop;
        VERIFY_ARE_EQUAL(1, customBackdrop->GetConnectedCount());
        VERIFY_ARE_EQUAL(xiICSSB, customBackdrop->GetPreviousConnectedTarget());
        VERIFY_ARE_EQUAL(0, customBackdrop->GetDisconnectedCount());

        LOG_OUTPUT(L"  > Setting the same SystemBackdrop. This shouldn't call OnTargetConnected again.");
        xi->SystemBackdrop = customBackdrop;
        VERIFY_ARE_EQUAL(1, customBackdrop->GetConnectedCount());
        VERIFY_ARE_EQUAL(xiICSSB, customBackdrop->GetPreviousConnectedTarget());
        VERIFY_ARE_EQUAL(0, customBackdrop->GetDisconnectedCount());

        LOG_OUTPUT(L"  > Detaching SystemBackdrop. This should call OnTargetDisconnected.");
        xi->SystemBackdrop = nullptr;
        VERIFY_ARE_EQUAL(1, customBackdrop->GetConnectedCount());
        VERIFY_ARE_EQUAL(1, customBackdrop->GetDisconnectedCount());
        VERIFY_ARE_EQUAL(xiICSSB, customBackdrop->GetPreviousDisconnectedTarget());

        LOG_OUTPUT(L"  > Cleaning up XI.");

        dqc->ShutdownQueue();

        return 0;
    });

    WaitForSingleObjectWithTimeout(uiThread);
}

void XamlIslandTests::PointerInput()
{
    XamlIslandTestHelper testHelper(this);
    testHelper.StartAppOnCurrentThread();

    Popup^ windowedPopup;

    Button^ button;
    wf::EventRegistrationToken buttonClickToken = {};
    Event buttonClickEvent;

    Button^ button2;
    wf::EventRegistrationToken button2ClickToken = {};
    Event button2ClickEvent;

    auto ih = IslandHelper::CreateOnNewUIThreadAndNewWindow(m_windowClassAtom);

    RunOnIslandUIThread(ih, [&]()
    {
        StackPanel^ stackPanel = CreateControlSubtree("MUX button");

        safe_cast<DesktopWindowXamlSource^>(ih->DesktopWindowXamlSource)->Content = stackPanel;

        button = safe_cast<Button^>(stackPanel->Children->GetAt(0));

        buttonClickToken = button->Click +=
            ref new RoutedEventHandler([&] (Platform::Object^, RoutedEventArgs^) {
                buttonClickEvent.Set();
            });

        button2 = ref new Button();
        button2->Width = 100;
        button2->Height = 50;
        button2->Content = "Button in popup";
        button2->Background = ref new SolidColorBrush(ColorHelper::FromArgb(0xff, 0, 0, 0xff));
        button2->HorizontalAlignment = HorizontalAlignment::Center;

        button2ClickToken = button2->Click +=
            ref new RoutedEventHandler([&] (Platform::Object^, RoutedEventArgs^) {
                button2ClickEvent.Set();
            });

        Grid^ grid = ref new Grid();
        grid->Width = 200;
        grid->Height = 200;
        grid->Background = ref new SolidColorBrush(ColorHelper::FromArgb(0xff, 0xff, 0, 0));
        grid->Children->Append(button2);

        windowedPopup = ref new Popup();
        windowedPopup->ShouldConstrainToRootBounds = false;
        windowedPopup->HorizontalOffset = 400;
        windowedPopup->VerticalOffset = 300;
        windowedPopup->Child = grid;
        stackPanel->Children->Append(windowedPopup);
        windowedPopup->IsOpen = true;
    });

    // Short sleep just to allow DWXS to be briefly visible.
    ::Sleep(100);

    LOG_OUTPUT(L"> Clicking button in island.");
    ih->LeftMouseClick(button);
    buttonClickEvent.WaitForDefault();

    LOG_OUTPUT(L"> Clicking button in windowed popup.");
    ih->LeftMouseClick(button2);
    button2ClickEvent.WaitForDefault();

    ::Sleep(100);

    RunOnIslandUIThread(ih, [&]()
    {
        button->Click -= buttonClickToken;
        button2->Click -= button2ClickToken;
    });
}

void XamlIslandTests::LoadedImageSurface_DecodeOnShutdownCrash()
{
    VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_SINGLETHREADED));

    auto dqcOuter {DispatcherQueueController::CreateOnCurrentThread()};

    LOG_OUTPUT(L"> Creating Application object...");
    auto app { ref new XamlIslandTests_ApplicationWithMuxc()};

    HWND hwnd { nullptr };
    DispatcherQueueController^ dqc;
    DispatcherQueue^ dq;
    WindowsXamlManager^ wxm;
    DesktopWindowXamlSource^ dwxs;
    Event uiThreadReady;

    auto uiThreadHandle = RunOnNewThread([&]()
    {
        VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_SINGLETHREADED));

        LOG_OUTPUT(L"> Setting PerMonV2...");
        ::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

        hwnd = this->CreateHwnd(1);

        dqc = DispatcherQueueController::CreateOnCurrentThread();
        dq = dqc->DispatcherQueue;
        uiThreadReady.Set();

        wxm = WindowsXamlManager::InitializeForCurrentThread();

        // Note: This call lets DispatcherQueue run the message pump and turns this UI thread into a "live app" that
        // responds to input and messages. It exits when we receive the WM_QUIT message posted at the end of the test.
        dqc->DispatcherQueue->RunEventLoop();

        LOG_OUTPUT(L"  > Exiting UI Thread.");
    });
    uiThreadReady.WaitForDefault();

    RunOnDispatcherThread(dq, true, [&]()
    {
        LOG_OUTPUT(L"  > Creating DesktopWindowXamlSource.");
        dwxs = CreateDesktopWindowXamlSource(hwnd, "MUX button");
    });

    // Short sleep just to allow DWXS to be briefly visible.
    ::Sleep(100);

    auto ih = IslandHelper::CreateWithExistingIsland(dqc->DispatcherQueue);

    // Use a test hook to stop all image decode activity. We'll set up a timing where we're decoding a
    // LoadedImageSurface as Xaml shuts down.
    ih->ThrottleImageTaskDispatcher(true, 0);

    LoadedImageSurface^ lis;
    RunOnDispatcherThread(dq, true, [&]()
    {
        LOG_OUTPUT(L"  > Creating LoadedImageSurface.");

        // Use a local resource - those give instant download responses
        auto uri = ref new Uri(L"ms-resource:///Files/Microsoft.UI.Xaml/Assets/NoiseAsset_256X256_PNG.png");
        lis = LoadedImageSurface::StartLoadFromUri(uri);
    });

    ::Sleep(100);

    RunOnDispatcherThread(dq, true, [&]()
    {
        LOG_OUTPUT(L"> Shutting down island while allowing decode. Don't crash.");

        LOG_OUTPUT(L"> Nulling out island.");
        dwxs = nullptr;
        LOG_OUTPUT(L"> Nulling out WindowsXamlManager.");
        wxm = nullptr;
        LOG_OUTPUT(L"> Resuming image task dispatcher.");
        ih->ThrottleImageTaskDispatcher(false, 0);
    });

    ::Sleep(100);

    RunOnDispatcherThread(dq, true, [&]()
    {
        LOG_OUTPUT(L"Calling DispatcherQueueController::ShutdownQueue.");
        dqc->ShutdownQueue();
    });

    LOG_OUTPUT(L"Shutdown outer DispatcherQueue.");
    dqcOuter->ShutdownQueue();
    app->Cleanup();
    app = nullptr;

    LOG_OUTPUT(L"Sending WM_QUIT to make UI thread exit.");
    ::DestroyWindow(hwnd);
    ::PostMessage(hwnd, WM_QUIT, 0, 0);
    WaitForSingleObjectWithTimeout(uiThreadHandle);
}

void XamlIslandTests::ToggleAccessKeyDisplayModeOnMultipleIslands()
{
    XamlIslandTestHelper testHelper(this);
    testHelper.StartAppOnCurrentThread();
    testHelper.StartAndPrepNewUIThreadWithWindow();

    DesktopWindowXamlSource^ dwxs1;
    DesktopWindowXamlSource^ dwxs2;

    wf::EventRegistrationToken isDisplayModeEnabledChangedEventToken {};
    int isDisplayModeEnabledChangedCounter {0};

    testHelper.RunOnIslandUIThread([&]()
    {
        isDisplayModeEnabledChangedEventToken = Microsoft::UI::Xaml::Input::AccessKeyManager::IsDisplayModeEnabledChanged +=
            ref new TypedEventHandler<Platform::Object^, Platform::Object^>([&isDisplayModeEnabledChangedCounter](const auto&, const auto&) {
            ++isDisplayModeEnabledChangedCounter;
        });

        LOG_OUTPUT(L"  > Creating DesktopWindowXamlSource dwxs1.");
        dwxs1 = CreateDesktopWindowXamlSource(testHelper.m_topLevelHwnd, "MUX button");

        LOG_OUTPUT(L"  > Creating DesktopWindowXamlSource dwxs2.");
        dwxs2 = CreateDesktopWindowXamlSource(testHelper.m_topLevelHwnd, "MUX button");
    });

    DrainMessageQueue(); // If we get WaitForIdle working, do that instead.

    testHelper.RunOnIslandUIThread([&]()
    {
        LOG_OUTPUT(L"Ensure IsDisplayModeEnabled is true if any XamlRoot on the thread is in AccessKey Display Mode.");
        VERIFY_IS_FALSE(xaml::Input::AccessKeyManager::IsDisplayModeEnabled);
        VERIFY_ARE_EQUAL(0, isDisplayModeEnabledChangedCounter);

        xaml::Input::AccessKeyManager::EnterDisplayMode(dwxs1->Content->XamlRoot);
        VERIFY_IS_TRUE(xaml::Input::AccessKeyManager::IsDisplayModeEnabled);
        VERIFY_ARE_EQUAL(1, isDisplayModeEnabledChangedCounter);

        xaml::Input::AccessKeyManager::EnterDisplayMode(dwxs2->Content->XamlRoot);
        VERIFY_IS_TRUE(xaml::Input::AccessKeyManager::IsDisplayModeEnabled);
        VERIFY_ARE_EQUAL(1, isDisplayModeEnabledChangedCounter);

        xaml::Input::AccessKeyManager::ExitDisplayMode();
        VERIFY_IS_FALSE(xaml::Input::AccessKeyManager::IsDisplayModeEnabled);
        VERIFY_ARE_EQUAL(2, isDisplayModeEnabledChangedCounter);

        xaml::Input::AccessKeyManager::IsDisplayModeEnabledChanged -= isDisplayModeEnabledChangedEventToken;
    });
}

void XamlIslandTests::FindXamlRootTopLevelWindow()
{
    XamlIslandTestHelper testHelper(this);
    testHelper.StartAppOnCurrentThread();
    testHelper.StartAndPrepNewUIThreadWithWindow();

    DesktopWindowXamlSource^ dwxs1;
    HWND intermediateHwnd {NULL};

    testHelper.RunOnIslandUIThread([&]()
    {
        LOG_OUTPUT(L"  > Creating child HWND.");
        intermediateHwnd = CreateHwnd(2, testHelper.m_topLevelHwnd);

        LOG_OUTPUT(L"  > Creating DesktopWindowXamlSource dwxs1 in intermediateHwnd.");
        dwxs1 = CreateDesktopWindowXamlSource(intermediateHwnd, "MUX button");
    });

    ::Sleep(500);

    testHelper.RunOnIslandUIThread([&]()
    {
        Panel^ rootPanel = safe_cast<Panel^>(dwxs1->Content);
        Button^ button = safe_cast<Button^>(rootPanel->Children->GetAt(0));
        Microsoft::UI::Content::ContentIslandEnvironment^ contentEnvironment = button->XamlRoot->ContentIslandEnvironment;
        const Microsoft::UI::WindowId windowId = contentEnvironment->AppWindowId;

        // The WindowId property returns a WindowId type in the Microsoft::UI namespace, but GetWindowFromId from
        // Microsoft.UI.Interop.h requires the type to be in ABI::Microsoft::UI.
        const ABI::Microsoft::UI::WindowId windowIdAbi {windowId.Value};

        HWND windowIdHwnd {NULL};
        VERIFY_SUCCEEDED(ABI::Microsoft::UI::GetWindowFromWindowId(windowIdAbi, &windowIdHwnd));

        LOG_OUTPUT(L"Top-level HWND is %p", testHelper.m_topLevelHwnd);
        LOG_OUTPUT(L"Intermediate HWND is %p", intermediateHwnd);
        LOG_OUTPUT(L"WindowId is %p", windowId);

        VERIFY_ARE_EQUAL(windowIdHwnd, testHelper.m_topLevelHwnd);
    });
}

void XamlIslandTests::ValidateContentIsland()
{
    XamlIslandTestHelper testHelper(this);
    testHelper.StartAppOnCurrentThread();
    testHelper.StartAndPrepNewUIThreadWithWindow();

    DesktopWindowXamlSource^ dwxs1;
    HWND intermediateHwnd {NULL};

    Button^ button;
    XamlRoot^ xamlRoot;

    testHelper.RunOnIslandUIThread([&]()
    {
        LOG_OUTPUT(L"  > Creating child HWND.");
        intermediateHwnd = CreateHwnd(2, testHelper.m_topLevelHwnd);

        LOG_OUTPUT(L"  > Creating DesktopWindowXamlSource dwxs1 in intermediateHwnd.");
        dwxs1 = CreateDesktopWindowXamlSource(intermediateHwnd, "MUX button");
    });

    ::Sleep(500);

    testHelper.RunOnIslandUIThread([&]()
    {
        Panel^ rootPanel = safe_cast<Panel^>(dwxs1->Content);
        button = safe_cast<Button^>(rootPanel->Children->GetAt(0));
        xamlRoot = button->XamlRoot;
        Microsoft::UI::Content::ContentIslandEnvironment^ contentEnvironment = xamlRoot->ContentIslandEnvironment;
        Microsoft::UI::Content::ContentCoordinateConverter^ contentCoordinateConverter = xamlRoot->CoordinateConverter;
        Microsoft::UI::Content::ContentIsland^ contentIsland = xamlRoot->ContentIsland;

        // XamlRoot.ContentIslandEnvironment and XamlRoot.CoordinateConverter are now just convenience functions that
        // return the same result as the equivalent properties on ContentIsland.
        VERIFY_ARE_EQUAL(contentEnvironment, contentIsland->Environment);
        VERIFY_ARE_EQUAL(contentCoordinateConverter, contentIsland->CoordinateConverter);
    });

    testHelper.RunOnIslandUIThread([&]()
    {
        LOG_OUTPUT(L"  > Closing DesktopWindowXamlSource dwxs1.");
        CloseObject(dwxs1);
    });

    testHelper.RunOnIslandUIThread([&]()
    {
        // ContentIsland should be null after the underlying DWXS (or XamlIsland) has been closed.
        VERIFY_IS_NULL(xamlRoot->ContentIsland);
    });
}


void XamlIslandTests::ReparentScrollViewerAcrossIslands()
{
    XamlIslandTestHelper testHelper(this);
    testHelper.StartAppOnCurrentThread();
    testHelper.StartAndPrepNewUIThreadWithWindow();

    IslandHelper^ ih1;
    IslandHelper^ ih2;

    testHelper.RunOnIslandUIThread([&]()
    {
        ih1 = IslandHelper::CreateOnCurrentThreadAndNewWindow(m_windowClassAtom);
        ih2 = IslandHelper::CreateOnCurrentThreadAndNewWindow(m_windowClassAtom);

        safe_cast<DesktopWindowXamlSource^>(ih1->DesktopWindowXamlSource)->Content = CreateControlSubtree("MUX button");
        safe_cast<DesktopWindowXamlSource^>(ih2->DesktopWindowXamlSource)->Content = CreateControlSubtree("MUX button");
    });

    // Short sleep just to allow DWXS to be briefly visible.
    ::Sleep(100);

    // Make a Border, put a Grid inside, put a ScrollViewer inside that. Put the Border inside island 1. Render.
    // Detach the Border from island 1 and destroy island 1. Put the Border inside island 2. Render, and don't crash.
    Border^ border;
    testHelper.RunOnIslandUIThread([&]()
    {
        LOG_OUTPUT(L"  > Creating Border-Grid-ScrollViewer-Canvas.");

        Canvas^ canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 500;
        canvas->Background = ref new SolidColorBrush(ColorHelper::FromArgb(0xff, 0, 0x80, 0));

        ScrollViewer^ scrollViewer = ref new ScrollViewer();
        scrollViewer->Width = 100;
        scrollViewer->Height = 100;
        scrollViewer->Content = canvas;

        Grid^ grid = ref new Grid();
        grid->Width = 100;
        grid->Height = 100;
        grid->Children->Append(scrollViewer);

        border = ref new Border();
        border->Width = 100;
        border->Height = 100;
        border->Child = grid;

        StackPanel^ stackPanel = safe_cast<StackPanel^>(safe_cast<DesktopWindowXamlSource^>(ih1->DesktopWindowXamlSource)->Content);
        stackPanel->Children->Append(border);
    });

    ::Sleep(100);

    testHelper.RunOnIslandUIThread([&]()
    {
        LOG_OUTPUT(L"> Detaching Border from island.");
        StackPanel^ stackPanel = safe_cast<StackPanel^>(safe_cast<DesktopWindowXamlSource^>(ih1->DesktopWindowXamlSource)->Content);
        stackPanel->Children->Clear();

        LOG_OUTPUT(L"> Nulling out island.");
        ih1 = nullptr;
    });

    ::Sleep(100);

    testHelper.RunOnIslandUIThread([&]()
    {
        LOG_OUTPUT(L"> Moving Border to second island. Don't crash.");
        StackPanel^ stackPanel = safe_cast<StackPanel^>(safe_cast<DesktopWindowXamlSource^>(ih2->DesktopWindowXamlSource)->Content);
        stackPanel->Children->Append(border);
    });

    ::Sleep(100);
}

void XamlIslandTests::TwoUIThreads()
{
    XamlIslandTestHelper testHelper(this);
    testHelper.StartAppOnCurrentThread();

    Event blockUIThread1;
    Event uiThread1Done;

    auto ih1 = IslandHelper::CreateOnNewUIThreadAndNewWindow(m_windowClassAtom);
    auto ih2 = IslandHelper::CreateOnNewUIThreadAndNewWindow(m_windowClassAtom);

    RunOnIslandUIThread(ih1, [&]()
    {
        safe_cast<DesktopWindowXamlSource^>(ih1->DesktopWindowXamlSource)->Content = CreateControlSubtree("Button1");
    });

    RunOnIslandUIThread(ih2, [&]()
    {
        safe_cast<DesktopWindowXamlSource^>(ih2->DesktopWindowXamlSource)->Content = CreateControlSubtree("Button2");
    });

    // Short sleep just to allow DWXS to be briefly visible.
    ::Sleep(100);

    // Note: Async because this lambda contains a wait that will block the UI thread,
    // so we don't want to the test thread to be blocked on the UI thread work to complete.
    RunOnIslandUIThreadAsync(ih1, [&]()
    {
        LOG_OUTPUT(L"  1> UI thread 1 is waiting for event");
        blockUIThread1.WaitForDefault();
        LOG_OUTPUT(L"  1> UI thread 1 is unblocked");
        uiThread1Done.Set();
    });

    RunOnIslandUIThread(ih2, [&]()
    {
        LOG_OUTPUT(L"  2> UI thread 2 unblocking UI thread 1");
        blockUIThread1.Set();
    });

    uiThread1Done.WaitForDefault();
}

void XamlIslandTests::DeviceLostTest()
{
    XamlIslandTestHelper testHelper(this);
    testHelper.StartAppOnCurrentThread();

    auto ih1 = IslandHelper::CreateOnNewUIThreadAndNewWindow(m_windowClassAtom);
    auto ih2 = IslandHelper::CreateOnNewUIThreadAndNewWindow(m_windowClassAtom);
    Button^ button1;
    Button^ button2;
    RunOnIslandUIThread(ih1, [&]()
    {
        StackPanel^ stackPanel1 = CreateControlSubtree("MUX Button1");
        safe_cast<DesktopWindowXamlSource^>(ih1->DesktopWindowXamlSource)->Content = stackPanel1;
        button1 = safe_cast<Button^>(stackPanel1->Children->GetAt(0));
    });
    RunOnIslandUIThread(ih2, [&]()
    {
        StackPanel^ stackPanel2 = CreateControlSubtree("MUX Button2");
        safe_cast<DesktopWindowXamlSource^>(ih2->DesktopWindowXamlSource)->Content = stackPanel2;
        button2 = safe_cast<Button^>(stackPanel2->Children->GetAt(0));
    });
    // Short sleep just to allow DWXS to be briefly visible.
    LowBudgetWaitForIdle(ih1);

    // Get GraphicsDevice Pointer to compare to after DeviceLost
    INT64 d3d11DevicePtr_thread1_before = 0;
    INT64 d3d11DevicePtr_thread2_before = 0;
    LOG_OUTPUT(L"  > GetD3d11GraphicsDevice Thread 1 ");
    ih1->GetD3D11GraphicsDeviceAddress(&d3d11DevicePtr_thread1_before);
    LOG_OUTPUT(L"  > GetD3d11GraphicsDevice Thread 2 ");
    ih2->GetD3D11GraphicsDeviceAddress(&d3d11DevicePtr_thread2_before);
    VERIFY_IS_NOT_NULL(d3d11DevicePtr_thread1_before);
    VERIFY_IS_NOT_NULL(d3d11DevicePtr_thread2_before);
    VERIFY_ARE_EQUAL(d3d11DevicePtr_thread1_before, d3d11DevicePtr_thread2_before);

    // Block UI Thread 2
    Event uiThreadBlock;
    Event uiThreadDone;
    RunOnIslandUIThreadAsync(ih2, [&]()
    {
        LOG_OUTPUT(L"  2> UI thread 2 is blocked");
        uiThreadBlock.WaitForDefault();
        INT64 d3d11DevicePtr = 0;
        ih2->GetD3D11GraphicsDeviceAddress(&d3d11DevicePtr);
        VERIFY_IS_NULL(d3d11DevicePtr);
        LOG_OUTPUT(L"  2> Dirting UI tree on UIThread 2");
        button2->Content = "MUX Button 2 Dirty";
        uiThreadDone.Set();
    });

    // Trigger DeviceLost
    LOG_OUTPUT(L"  > SimulateDeviceLost ");
    ih1->SimulateDeviceLost();
    ih1->MarkDeviceInstanceLost();
    RunOnIslandUIThread(ih1, [&]()
    {
        LOG_OUTPUT(L"  1> Dirtying UI Tree on UITread 1");
        button1->Content = "MUX Button1 Dirty";
    });
    Sleep(250);

    // Get GraphicsDevice Pointer after DeviceLost
    INT64 d3d11DevicePtr_thread1_after = 0;
    INT64 d3d11DevicePtr_thread2_after = 0;
    LOG_OUTPUT(L"  > GetD3d11GraphicsDevice Thread 1");
    ih1->GetD3D11GraphicsDeviceAddress(&d3d11DevicePtr_thread1_after);
    VERIFY_IS_NOT_NULL(d3d11DevicePtr_thread1_after);
    RunOnIslandUIThread(ih1, [&]()
    {
        LOG_OUTPUT(L"  1> UI thread 1 unblocking UI thread 2");
        uiThreadBlock.Set();
    });
    uiThreadDone.WaitForDefault();
    Sleep(250);
    LOG_OUTPUT(L"  > GetD3d11GraphicsDevice Thread 2");

    ih2->GetD3D11GraphicsDeviceAddress(&d3d11DevicePtr_thread2_after);
    VERIFY_IS_NOT_NULL(d3d11DevicePtr_thread2_after);
    VERIFY_ARE_EQUAL(d3d11DevicePtr_thread1_after, d3d11DevicePtr_thread2_after);
    VERIFY_ARE_NOT_EQUAL(d3d11DevicePtr_thread1_after, d3d11DevicePtr_thread1_before);
}

void XamlIslandTests::EnsureDwxsEventOrderDuringTabTraversal()
{
    XamlIslandTestHelper testHelper(this);
    testHelper.StartAppOnCurrentThread();

    auto ih1 = IslandHelper::CreateOnNewUIThreadAndNewWindow(m_windowClassAtom);

    DesktopWindowXamlSource^ desktopWindowXamlSource;
    Button^ button1;
    Button^ button2;

    std::vector<std::wstring> observedEvents;

    auto logEvent = [&observedEvents](const wchar_t* format, ...) {
        wchar_t buffer[4096];
        va_list args;
        va_start(args, format);
        vswprintf_s(buffer, ARRAYSIZE(buffer), format, args);
        va_end(args);

        observedEvents.push_back(buffer);
        LOG_OUTPUT(buffer);
    };

    auto button1GotFocusEvent = std::make_shared<Event>();
    auto button2GotFocusEvent = std::make_shared<Event>();
    auto button1LostFocusEvent = std::make_shared<Event>();
    auto button2LostFocusEvent = std::make_shared<Event>();
    auto resetEvents = [=]() {
        button1GotFocusEvent->Reset();
        button2GotFocusEvent->Reset();
        button1LostFocusEvent->Reset();
        button2LostFocusEvent->Reset();
    };

    wf::EventRegistrationToken gotFocusToken;
    wf::EventRegistrationToken takeFocusRequestedToken;

    RunOnIslandUIThread(ih1, [&]()
    {
        // Create a Xaml scene with two buttons:
        //   +-----------------------------+
        //   |  +-------------+            |
        //   |  |   button1   |            |
        //   |  +-------------+            |
        //   |  +-------------+            |
        //   |  |   button2   |            |
        //   |  +-------------+            |
        //   |                             |
        //   +-----------------------------+

        desktopWindowXamlSource = safe_cast<DesktopWindowXamlSource^>(ih1->DesktopWindowXamlSource);
        auto stackPanel = safe_cast<StackPanel^>(xaml_markup::XamlReader::Load(
            L"<StackPanel x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
            L"  Padding='10' >"
            L"  <Button x:Name='button1' Width='125' Height='50'>button1</Button>"
            L"  <Button x:Name='button2' Width='125' Height='50'>button2</Button>"
            L"</StackPanel>"));

        button1 = safe_cast<Button^>(stackPanel->FindName("button1"));
        button2 = safe_cast<Button^>(stackPanel->FindName("button2"));

        desktopWindowXamlSource->Content = stackPanel;

        button1->GotFocus += ref new RoutedEventHandler([=](Platform::Object^,RoutedEventArgs^) {
            logEvent(L"button1 GotFocus fired.");
            button1GotFocusEvent->Set();
        });

        button2->GotFocus += ref new RoutedEventHandler([=](Platform::Object^,RoutedEventArgs^) {
            logEvent(L"button2 GotFocus fired.");
            button2GotFocusEvent->Set();
        });

        button1->LostFocus += ref new RoutedEventHandler([=](Platform::Object^,RoutedEventArgs^) {
            logEvent(L"button1 LostFocus fired.");
            button1LostFocusEvent->Set();
        });

        button2->LostFocus += ref new RoutedEventHandler([=](Platform::Object^,RoutedEventArgs^) {
            logEvent(L"button2 LostFocus fired.");
            button2LostFocusEvent->Set();
        });

        gotFocusToken = desktopWindowXamlSource->GotFocus +=
            ref new TypedEventHandler<DesktopWindowXamlSource^, DesktopWindowXamlSourceGotFocusEventArgs^>(
                [&] (DesktopWindowXamlSource^ dwxs, DesktopWindowXamlSourceGotFocusEventArgs^ args) {
                    const XamlSourceFocusNavigationReason reason = args->Request->Reason;
                    logEvent(L"Dwxs GotFocus Fired. Reason=%s", GetString(reason));
            });

        takeFocusRequestedToken = desktopWindowXamlSource->TakeFocusRequested +=
            ref new TypedEventHandler<DesktopWindowXamlSource^, DesktopWindowXamlSourceTakeFocusRequestedEventArgs^>(
                [&] (DesktopWindowXamlSource^ dwxs, DesktopWindowXamlSourceTakeFocusRequestedEventArgs^ args) {
                    const XamlSourceFocusNavigationReason reason = args->Request->Reason;
                    logEvent(L"Dwxs TakeFocusRequested Fired. Reason=%s", GetString(reason));

                    LOG_OUTPUT(L"Call win32 SetFocus on the top-level HWND (outside of Xaml) since in a real app, keyboard focus will be moving elsewhere.");
                    ::SetFocus(testHelper.m_topLevelHwnd);
            });
    });

    // Short sleep just to allow DWXS to be briefly visible.
    ::Sleep(500);

    resetEvents();
    RunOnIslandUIThread(ih1, [&]()
    {
        LOG_OUTPUT(L"Simulate a tab key entering focus into the island.");
        logEvent(L"Call NavigateFocus. Reason=First");
        XamlSourceFocusNavigationRequest^ request = ref new XamlSourceFocusNavigationRequest(XamlSourceFocusNavigationReason::First);
        desktopWindowXamlSource->NavigateFocus(request);
    });
    button1GotFocusEvent->WaitForDefault();
    LowBudgetWaitForIdle(ih1);

    // Focus is now on Button1.

    // This tab press moves focus to Button2.
    resetEvents();
    logEvent(L"Press Tab key.");
    SendTab();
    button2GotFocusEvent->WaitForDefault();
    LowBudgetWaitForIdle(ih1);

    // This tab press triggers a TakeFocusRequested, since Button2 is the last TabStop in the island.
    resetEvents();
    logEvent(L"Press Tab key.");
    SendTab();
    button2LostFocusEvent->WaitForDefault();
    LowBudgetWaitForIdle(ih1);

    resetEvents();
    RunOnIslandUIThread(ih1, [&]()
    {
        LOG_OUTPUT(L"Simulate a shift-tab key entering focus into the island from the other side.");
        logEvent(L"Call NavigateFocus. Reason=Last");
        XamlSourceFocusNavigationRequest^ request = ref new XamlSourceFocusNavigationRequest(XamlSourceFocusNavigationReason::Last);
        desktopWindowXamlSource->NavigateFocus(request);
    });
    button2GotFocusEvent->WaitForDefault();
    LowBudgetWaitForIdle(ih1);

    // Focus is now on Button2.

    // This shift+tab press moves focus to Button1.
    resetEvents();
    logEvent(L"Press Shift+Tab.");
    SendShiftTab();
    button1GotFocusEvent->WaitForDefault();
    LowBudgetWaitForIdle(ih1);

    // This tab press moves focus out of the island.
    resetEvents();
    logEvent(L"Press Shift+Tab.");
    SendShiftTab();
    button1LostFocusEvent->WaitForDefault();
    LowBudgetWaitForIdle(ih1);

    RunOnIslandUIThread(ih1, [&]()
    {
        LOG_OUTPUT(L"Clean up on island 1.");
        desktopWindowXamlSource->TakeFocusRequested -= takeFocusRequestedToken;
        desktopWindowXamlSource->GotFocus -= gotFocusToken;
    });

    const wchar_t* expectedStrings[] = {
        L"Call NavigateFocus. Reason=First",
        L"button1 GotFocus fired.",
        L"button1 GotFocus fired.",
        L"Press Tab key.",
        L"button1 LostFocus fired.",
        L"button2 GotFocus fired.",
        L"Press Tab key.",
        L"Dwxs TakeFocusRequested Fired. Reason=First",
        L"button2 LostFocus fired.",
        L"Call NavigateFocus. Reason=Last",
        L"button2 GotFocus fired.",
        L"button2 GotFocus fired.",
        L"Press Shift+Tab.",
        L"button2 LostFocus fired.",
        L"button1 GotFocus fired.",
        L"Press Shift+Tab.",
        L"Dwxs TakeFocusRequested Fired. Reason=Last",
        L"button1 LostFocus fired."};

    bool areEventLogsSame = ARRAYSIZE(expectedStrings) == observedEvents.size();
    if (areEventLogsSame)
    {
        for (int i=0; i < ARRAYSIZE(expectedStrings); ++i)
        {
            if (wcscmp(expectedStrings[i], observedEvents[i].c_str()) != 0)
            {
                areEventLogsSame = false;
                break;
            }
        }
    }

    if (!areEventLogsSame)
    {
        LOG_OUTPUT(L"Event log is not as expected!");
        LOG_OUTPUT(L"Expected event log:");
        LOG_OUTPUT(L"----------");
        for (const wchar_t* str : expectedStrings)
        {
            LOG_OUTPUT(str);
        }
        LOG_OUTPUT(L"----------");
        LOG_OUTPUT(L"");
        LOG_OUTPUT(L"Observed event log:");
        LOG_OUTPUT(L"----------");
        for (auto str : observedEvents)
        {
            LOG_OUTPUT(str.c_str());
        }
        LOG_OUTPUT(L"----------");
    }
    VERIFY_IS_TRUE(areEventLogsSame);
}

void XamlIslandTests::CommandBarFlyoutTest_VerifyCBFOverflowPopup(ContentIsland^ secondaryItemsContentIsland)
{
    VisualTreeVerifier^ verifier = VisualTreeVerifier::CreateFromVisual(secondaryItemsContentIsland->Root);
    verifier
        ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_AnimationRootVisual))
        // OverflowPopup's comp node
        ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_PublicRootVisual))
        ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_PrimaryVisual))
        ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_ContentVisual))
        // OuterOverflowContentRootV2's comp node
        ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_PrependVisual))
        ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_PrimaryVisual))
        ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_RoundedCornerClipVisual));
    // The overflow is on the bottom, so the top corners should be square
    verifier->VerifyRoundedCornerClipCorners(0, 0, 8, 8);
    verifier
        ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_ContentVisual))
        // OverflowPopupSystemBackdropRoot's comp node
        ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_PrependVisual))
        ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_PrimaryVisual));
    // 1 for the Content visual and 1 for the CEBL.PlacementVisual hand-in
    verifier->VerifyChildCount(2);
}

void XamlIslandTests::CommandBarFlyoutTest_VerifyCBFPopup(ContentIsland^ primaryMenuContentIsland)
{
    VisualTreeVerifier^ verifier = VisualTreeVerifier::CreateFromVisual(primaryMenuContentIsland->Root);
    verifier
        ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_AnimationRootVisual))
        // Popup's comp node
        ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_PublicRootVisual))
        ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_PrimaryVisual))
        ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_ContentVisual))
        // custom FlyoutPresenter's comp node
        ->WalkThroughSimpleCompNode()
        // FlyoutPresenter's border's comp node
        ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_PrependVisual))
        ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_PrimaryVisual))
        ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_RoundedCornerClipVisual));
    // The primary items are on top, so the bottom corners should be square
    verifier->VerifyRoundedCornerClipCorners(8, 8, 0, 0);
    verifier
        ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_ContentVisual))
        // ContentPresenter's comp node
        ->WalkThroughSimpleCompNode()
        // CommandBar's comp node
        ->WalkThroughSimpleCompNode()
        // LayoutRoot's comp node
        ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_PrependVisual))
        ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_PrimaryVisual))
        ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_RoundedCornerClipVisual));
    // There's a duplicate clip set down here
    verifier->VerifyRoundedCornerClipCorners(8, 8, 0, 0);
    verifier
        ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_ContentVisual))
        // PrimaryItemsSystemBackdropRoot's comp node
        ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_PrependVisual))
        ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_PrimaryVisual));
    // 1 for the Content visual and 1 for the CEBL.PlacementVisual hand-in
    verifier->VerifyChildCount(2);
}

void XamlIslandTests::CommandBarFlyoutTest_VerifyCBF(IslandHelper^ ih)
{
    RunOnIslandUIThread(ih, [&]()
    {
        auto contentIslands = ih->GetAllContentIslands();
        VERIFY_ARE_EQUAL(3, contentIslands->Size, "There should be the island and two open windowed popups for the CommandBarFlyout.");

        ContentIsland^ secondaryItemsContentIsland = safe_cast<ContentIsland^>(contentIslands->GetAt(1));
        VERIFY_IS_NOT_NULL(secondaryItemsContentIsland, "The second island should be the CommandBarFlyout's overflow windowed popup.");
        CommandBarFlyoutTest_VerifyCBFOverflowPopup(secondaryItemsContentIsland);

        ContentIsland^ primaryMenuContentIsland = safe_cast<ContentIsland^>(contentIslands->GetAt(2));
        VERIFY_IS_NOT_NULL(primaryMenuContentIsland, "The third island should be the CommandBarFlyout's windowed popup.");
        CommandBarFlyoutTest_VerifyCBFPopup(primaryMenuContentIsland);
    });
}

void XamlIslandTests::CommandBarFlyoutTest()
{
    XamlIslandTestHelper testHelper(this);
    testHelper.StartAppOnCurrentThread();

    auto ih = IslandHelper::CreateOnNewUIThreadAndNewWindow(m_windowClassAtom);

    Button^ button;
    CommandBarFlyout^ cbf;
    AppBarButton^ view;
    AppBarButton^ sort;

    RunOnIslandUIThread(ih, [&]()
    {
        LOG_OUTPUT(L"  > Creating tree.");

        DesktopWindowXamlSource^ dwxs = safe_cast<DesktopWindowXamlSource^>(ih->DesktopWindowXamlSource);

        StackPanel^ stackPanel = ref new StackPanel();
        stackPanel->Width = 200;
        stackPanel->Height = 200;
        stackPanel->Background = ref new SolidColorBrush(ColorHelper::FromArgb(0xff, 0, 0xff, 0));
        dwxs->Content = stackPanel;

        button = ref new Button();
        button->Content = "Button";
        stackPanel->Children->Append(button);
    });
    LowBudgetWaitForIdle(ih);

    RunOnIslandUIThread(ih, [&]()
    {
        LOG_OUTPUT(L"  > Creating CommandBarFlyout.");

        cbf = ref new CommandBarFlyout();
        cbf->AlwaysExpanded = true;

        auto paste = ref new xaml_controls::AppBarButton();
        paste->Label = "Paste";
        paste->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::Paste);
        cbf->PrimaryCommands->Append(paste);

        view = ref new xaml_controls::AppBarButton();
        view->Label = "View";
        view->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::ViewAll);
        cbf->SecondaryCommands->Append(view);

        {
            MenuFlyout^ subMenu = ref new xaml_controls::MenuFlyout();
            view->Flyout = subMenu;

            auto largeIcons = ref new xaml_controls::MenuFlyoutItem();
            largeIcons->Text = "Large icons";
            largeIcons->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::ViewAll);
            subMenu->Items->Append(largeIcons);

            auto mediumIcons = ref new xaml_controls::MenuFlyoutItem();
            mediumIcons->Text = "Medium icons";
            mediumIcons->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::ViewAll);
            subMenu->Items->Append(mediumIcons);

            auto smallIcons = ref new xaml_controls::MenuFlyoutItem();
            smallIcons->Text = "Small icons";
            smallIcons->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::ViewAll);
            subMenu->Items->Append(smallIcons);
        }

        sort = ref new xaml_controls::AppBarButton();
        sort->Label = "Sort";
        sort->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::Sort);
        cbf->SecondaryCommands->Append(sort);

        auto refresh = ref new xaml_controls::AppBarButton();
        refresh->Label = "Refresh";
        refresh->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::Refresh);
        cbf->SecondaryCommands->Append(refresh);

        FlyoutShowOptions^ options = ref new xaml_primitives::FlyoutShowOptions();
        options->ShowMode = xaml_primitives::FlyoutShowMode::Standard;
        options->Placement = xaml_primitives::FlyoutPlacementMode::RightEdgeAlignedTop;

        LOG_OUTPUT(L"  > Opening CommandBarFlyout.");
        cbf->ShowAt(button, options);
    });
    LowBudgetWaitForIdle(ih);

    CommandBarFlyoutTest_VerifyCBF(ih);

    RunOnIslandUIThread(ih, [&]()
    {
        LOG_OUTPUT(L"  > Modifying CommandBarFlyout to the same size.");

        auto refresh = ref new xaml_controls::AppBarButton();
        refresh->Label = "Refresh";
        refresh->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::Refresh);
        cbf->SecondaryCommands->SetAt(2, refresh);
    });
    LowBudgetWaitForIdle(ih);

    CommandBarFlyoutTest_VerifyCBF(ih);

    RunOnIslandUIThread(ih, [&]()
    {
        LOG_OUTPUT(L"  > Modifying CommandBarFlyout to a different size.");

        auto refresh = ref new xaml_controls::AppBarButton();
        refresh->Label = "Refresh but waaaaaaaaaaaaaaaaaaaaay wider";
        refresh->Icon = ref new xaml_controls::SymbolIcon(xaml_controls::Symbol::Refresh);
        cbf->SecondaryCommands->SetAt(2, refresh);
    });
    LowBudgetWaitForIdle(ih);

    CommandBarFlyoutTest_VerifyCBF(ih);

    LOG_OUTPUT(L"  > Opening submenu.");
    ih->LeftMouseClick(view);
    LowBudgetWaitForIdle(ih);

    RunOnIslandUIThread(ih, [&]()
    {
        auto contentIslands = ih->GetAllContentIslands();
        VERIFY_ARE_EQUAL(4, contentIslands->Size, "There should be the island, two open windowed popups for the CommandBarFlyout, and one open windowed popup for the submenu.");

        ContentIsland^ submenuContentIsland = safe_cast<ContentIsland^>(contentIslands->GetAt(1));
        VERIFY_IS_NOT_NULL(submenuContentIsland, "The second island should be the submenu's windowed popup.");

        {
            // The nested menu should have a backdrop as well.
            VisualTreeVerifier^ verifier = VisualTreeVerifier::CreateFromVisual(submenuContentIsland->Root);
            verifier
                ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_AnimationRootVisual))
                ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_SystemBackdropPlacementVisual));
        }

        ContentIsland^ secondaryItemsContentIsland = safe_cast<ContentIsland^>(contentIslands->GetAt(2));
        VERIFY_IS_NOT_NULL(secondaryItemsContentIsland, "The third island should be the CommandBarFlyout's overflow windowed popup.");
        CommandBarFlyoutTest_VerifyCBFOverflowPopup(secondaryItemsContentIsland);

        ContentIsland^ primaryMenuContentIsland = safe_cast<ContentIsland^>(contentIslands->GetAt(3));
        VERIFY_IS_NOT_NULL(primaryMenuContentIsland, "The fourth island should be the CommandBarFlyout's windowed popup.");
        CommandBarFlyoutTest_VerifyCBFPopup(primaryMenuContentIsland);
    });

    LOG_OUTPUT(L"  > Closing nested menu and CommandBarFlyout.");
    ih->LeftMouseClick(sort);
    LowBudgetWaitForIdle(ih);

    RunOnIslandUIThread(ih, [&]()
    {
        LOG_OUTPUT(L"  > Closing CommandBarFlyout.");
        cbf->Hide();
    });
    LowBudgetWaitForIdle(ih);

    // We get an assert during island teardown if we don't do an extra wait here for the popup to dismiss.
    ::Sleep(100);
}

void XamlIslandTests::EnsureWin32SetOnProgrammaticFocusToFocusedElement()
{
    // This test is based on a scenario that came up in File Explorer's AddressBar control.  It has a BreadcrumbBar
    // that, when clicked, focuses the TextBox of an AutoSuggestBox.  It normally works fine, but fails in the case
    // that the TextBox has focus, and then the user moves focus out of the DesktopWindowXamlSource, and then clicks
    // on the BreadcrumbBar.  The app's BreadcrumbBar.PointerPressed handler then calls Focus() on the ASB's TextBox.
    // But it fails because Xaml sees the TextBox already has focus, and neglects to call win32 ::SetFocus on it.
    // We set up a similar Xaml scene here:
    // Island 1:
    //  +-------------------------------------------+
    //  |                     o                     |   << Small button just for initial focus.
    //  |              +---------------+            |
    //  |              |    textBox    |            |
    //  |              +---------------+            |
    //  |              +---------------+            |
    //  |              |   rectangle   |            |   << When clicked, sets focus to textBox.
    //  |              +---------------+            |
    //  +-------------------------------------------+
    //
    // Island 2:
    //  +-------------------------------------------+
    //  |              +---------------+            |
    //  |              | island2Button |            |   << Clicked just to move focus out of island 1.
    //  |              +---------------+            |
    //  +-------------------------------------------+

    XamlIslandTestHelper testHelper(this);
    testHelper.StartAppOnCurrentThread();

    TextBox^ textBox;
    wf::EventRegistrationToken textBoxGotFocusToken {};
    wf::EventRegistrationToken textBoxLostFocusToken {};
    Event textBoxGotFocusEvent;
    Event textBoxLostFocusEvent;

    Shapes::Rectangle^ rectangle;
    wf::EventRegistrationToken rectanglePointerPressedToken {};

    Button^ island2Button;

    DesktopWindowXamlSource^ dwxs1;
    DesktopWindowXamlSource^ dwxs2;

    LOG_OUTPUT(L"Create island 1...");
    auto ih1 = IslandHelper::CreateOnNewUIThreadAndNewWindow(m_windowClassAtom);
    IslandHelper^ ih2;
    RunOnIslandUIThread(ih1, [&]()
    {
        auto stackPanel = safe_cast<StackPanel^>(xaml_markup::XamlReader::Load(
            L"<StackPanel x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
            L"  Padding='10' Background='LightGray'>"
            L"  <Button x:Name='buttonWithInitialFocus' Width='10' Height='10'/>"
            L"  <TextBox x:Name='textBox' Width='125' Text='textBox' />"
            L"  <Rectangle x:Name='rectangle' Width='50' Height='50' Fill='Green'/>"
            L"</StackPanel>"));

        textBox = safe_cast<TextBox^>(stackPanel->FindName(L"textBox"));
        rectangle = safe_cast<Shapes::Rectangle^>(stackPanel->FindName(L"rectangle"));

        textBoxGotFocusToken = textBox->GotFocus += ref new RoutedEventHandler([&](Platform::Object^ sender, RoutedEventArgs^ args){
            LOG_OUTPUT(L"=== textBox GotFocus fired.");
            textBoxGotFocusEvent.Set();
        });

        textBoxLostFocusToken = textBox->LostFocus += ref new RoutedEventHandler([&](Platform::Object^ sender, RoutedEventArgs^ args){
            LOG_OUTPUT(L"=== textBox LostFocus fired.");
            textBoxLostFocusEvent.Set();
        });

        rectanglePointerPressedToken = rectangle->PointerPressed += ref new PointerEventHandler([&](Platform::Object^ sender, PointerRoutedEventArgs^ args)
        {
            LOG_OUTPUT(L"=== rectangle PointerPressed fired.  Setting focus to textBox.");
            args->Handled = true;
            textBox->Focus(FocusState::Pointer);
        });

        dwxs1 = safe_cast<DesktopWindowXamlSource^>(ih1->DesktopWindowXamlSource);
        dwxs1->SiteBridge->MoveAndResize({0,0,200,100});
        dwxs1->Content = stackPanel;

        Microsoft::UI::Content::ContentIslandEnvironment^ contentIslandEnvironment = stackPanel->XamlRoot->ContentIslandEnvironment;
        auto topLevelWindowId = contentIslandEnvironment->AppWindowId;

        LOG_OUTPUT(L"Create island 2...");
        ih2 = IslandHelper::CreateOnCurrentThreadAndWindow(topLevelWindowId);

        auto stackPanel2 = safe_cast<StackPanel^>(xaml_markup::XamlReader::Load(
            L"<StackPanel x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
            L"  Padding='10' Background='Cyan'>"
            L"  <Button x:Name='island2Button'>island2Button in island2</Button>"
            L"</StackPanel>"));

        island2Button = safe_cast<Button^>(stackPanel2->FindName(L"island2Button"));

        dwxs2 = safe_cast<DesktopWindowXamlSource^>(ih2->DesktopWindowXamlSource);
        dwxs2->SiteBridge->MoveAndResize({0,150,200,50});
        dwxs2->Content = stackPanel2;
    });

    // Short sleep just to allow DWXS to be briefly visible.
    ::Sleep(100);

    LOG_OUTPUT(L"> Clicking Rectangle in island1.  This will set focus to the TextBox.");
    ih1->LeftMouseClick(rectangle);
    textBoxGotFocusEvent.WaitForDefault(false);
    LowBudgetWaitForIdle(ih1);
    RunOnIslandUIThread(ih1, [&]()
    {
        VERIFY_IS_TRUE(dwxs1->HasFocus);
        VERIFY_IS_FALSE(dwxs2->HasFocus);
    });

    LOG_OUTPUT(L"> Clicking Button in island2.");
    ih2->LeftMouseClick(island2Button);
    textBoxLostFocusEvent.WaitForDefault(false);
    LowBudgetWaitForIdle(ih1);
    RunOnIslandUIThread(ih1, [&]()
    {
        VERIFY_IS_FALSE(dwxs1->HasFocus);
        VERIFY_IS_TRUE(dwxs2->HasFocus);
    });

    LOG_OUTPUT(L"> Clicking Rectangle in island1 again.");
    textBoxGotFocusEvent.Reset();
    ih1->LeftMouseClick(rectangle);
    textBoxGotFocusEvent.WaitForDefault(false);
    LowBudgetWaitForIdle(ih1);
    RunOnIslandUIThread(ih1, [&]()
    {
        VERIFY_IS_TRUE(dwxs1->HasFocus);
        VERIFY_IS_FALSE(dwxs2->HasFocus);
    });

    RunOnIslandUIThread(ih1, [&]()
    {
        textBox->GotFocus -= textBoxGotFocusToken;
        textBox->LostFocus -= textBoxLostFocusToken;
        rectangle->PointerPressed -= rectanglePointerPressedToken;
    });
    LowBudgetWaitForIdle(ih1);
}

void XamlIslandTests::StagedFlyoutTest()
{
    XamlIslandTestHelper testHelper(this);
    testHelper.StartAppOnCurrentThread();
    testHelper.StartAndPrepNewUIThreadWithWindow();

    IslandHelper^ ih;

    testHelper.RunOnIslandUIThread([&]()
    {
        ih = IslandHelper::CreateOnCurrentThreadAndNewWindow(m_windowClassAtom);
    });

    Button^ button;
    MenuFlyout^ flyout1;
    MenuFlyoutItem^ item1;
    MenuFlyout^ flyout2;
    MenuFlyoutItem^ item2;
    MenuFlyout^ flyout3;
    MenuFlyoutItem^ item3;

    testHelper.RunOnIslandUIThread([&]()
    {
        LOG_OUTPUT(L"  > Creating tree.");

        DesktopWindowXamlSource^ dwxs = safe_cast<DesktopWindowXamlSource^>(ih->DesktopWindowXamlSource);

        button = ref new Button();
        button->Width = 125;
        button->Height = 50;
        button->Content = L"Button";

        StackPanel^ stackPanel = ref new StackPanel();
        stackPanel->Width = 200;
        stackPanel->Height = 200;
        stackPanel->Background = ref new SolidColorBrush(ColorHelper::FromArgb(0xff, 0, 0xff, 0));
        stackPanel->Children->Append(button);
        dwxs->Content = stackPanel;
    });

    LowBudgetWaitForIdle(ih);

    testHelper.RunOnIslandUIThread([&]()
    {
        LOG_OUTPUT(L"  > Creating Flyouts.");

        flyout1 = ref new MenuFlyout();
        item1 = ref new MenuFlyoutItem();
        item1->Text = ref new Platform::String(L"Flyout");
        item1->CompositeMode = ElementCompositeMode::SourceOver;    // Give it a comp node
        flyout1->Items->Append(item1);
        flyout1->ShouldConstrainToRootBounds = false;

        flyout2 = ref new MenuFlyout();
        item2 = ref new MenuFlyoutItem();
        item2->Text = ref new Platform::String(L"Staged flyout");
        item2->CompositeMode = ElementCompositeMode::SourceOver;    // Give it a comp node
        flyout2->Items->Append(item2);
        flyout2->ShouldConstrainToRootBounds = false;

        flyout3 = ref new MenuFlyout();
        item3 = ref new MenuFlyoutItem();
        item3->Text = ref new Platform::String(L"Staged flyout 2");
        item3->CompositeMode = ElementCompositeMode::SourceOver;    // Give it a comp node
        flyout3->Items->Append(item3);
        flyout3->ShouldConstrainToRootBounds = false;

        LOG_OUTPUT(L"  > Opening Flyout1.");
        flyout1->ShowAt(button);
    });

    LowBudgetWaitForIdle(ih);

    testHelper.RunOnIslandUIThread([&]()
    {
        VisualTreeVerifier^ verifier = VisualTreeVerifier::CreateFromElement(item1);
        verifier->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_PrimaryVisual))
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_ContentVisual));
        verifier->VerifyChildCount(1);

        LOG_OUTPUT(L"  > Opening Flyout2. It's now staged while Flyout1 closes.");
        flyout2->ShowAt(button);
    });

    LowBudgetWaitForIdle(ih);

    testHelper.RunOnIslandUIThread([&]()
    {
        VisualTreeVerifier^ verifier = VisualTreeVerifier::CreateFromElement(item2);
        verifier->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_PrimaryVisual))
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_ContentVisual));
        verifier->VerifyChildCount(1);

        LOG_OUTPUT(L"  > Opening Flyout3. It's now staged waiting on Flyout2 to close.");
        flyout3->ShowAt(button);

        // Destroy the island immediately. We want it cleaned up by the time flyout2 gets the presenter unloaded event.
        // Naively we'll open the staged flyout3 when flyout2's presenter unloads, but without any islands flyout3's
        // popup will AV when opening.
        ih = nullptr;
    });

    LOG_OUTPUT(L"> Don't crash during teardown.");
}

void XamlIslandTests::CloseWindowSuddenly()
{
    XamlIslandTestHelper testHelper(this);
    testHelper.StartAppOnCurrentThread();

    auto ih = IslandHelper::CreateOnNewUIThreadAndNewWindow(m_windowClassAtom);

    HWND topLevelHwnd {};
    HWND middleHwnd {};
    DesktopWindowXamlSource^ dwxs1;
    DesktopWindowXamlSource^ dwxs2;

    RunOnIslandUIThread(ih, [&]()
    {
        StackPanel^ stackPanel = ref new StackPanel();
        dwxs1 = safe_cast<DesktopWindowXamlSource^>(ih->DesktopWindowXamlSource);
        dwxs1->Content = stackPanel;

        auto topLevelWindowId = stackPanel->XamlRoot->ContentIslandEnvironment->AppWindowId;
        ABI::Microsoft::UI::WindowId windowIdAbi { topLevelWindowId.Value };
        VERIFY_SUCCEEDED(ABI::Microsoft::UI::GetWindowFromWindowId(windowIdAbi, &topLevelHwnd));

        middleHwnd = CreateHwnd(2, topLevelHwnd /*parent*/);
        ::SetWindowPos(middleHwnd, NULL, 10, 10, 200, 200, SWP_NOZORDER);
        ABI::Microsoft::UI::WindowId middleWindowId {};
        VERIFY_SUCCEEDED(ABI::Microsoft::UI::GetWindowIdFromWindow(middleHwnd, &middleWindowId));

        dwxs2 = ref new DesktopWindowXamlSource();
        dwxs2->Initialize(Microsoft::UI::WindowId {middleWindowId.Value });
        dwxs2->Content = CreateControlSubtree("Inner Button");
    });

    LowBudgetWaitForIdle(ih);

    RunOnIslandUIThread(ih, [&]()
    {
        LOG_OUTPUT(L"Destroying middle HWND.");
        DestroyWindow(middleHwnd);
    });
    LowBudgetWaitForIdle(ih);

    RunOnIslandUIThread(ih, [&]()
    {
        LOG_OUTPUT(L"Validate that calling DesktopWindowXamlSource.NavigateFocus when the window has been closed results in a no-op.");
        XamlSourceFocusNavigationResult^ result = dwxs2->NavigateFocus(ref new XamlSourceFocusNavigationRequest(XamlSourceFocusNavigationReason::First));
        VERIFY_IS_FALSE(result->WasFocusMoved);

        VERIFY_IS_FALSE(dwxs2->HasFocus);
    });

    RunOnIslandUIThread(ih, [&]()
    {
        CloseObject(dwxs2);

        CloseObject(dwxs1); // We didn't actually use this one.
    });
    LowBudgetWaitForIdle(ih);
}

void XamlIslandTests::UnrootedAutoSuggestBox()
{
    XamlIslandTestHelper testHelper(this);
    testHelper.StartAppOnCurrentThread();
    testHelper.StartAndPrepNewUIThreadWithWindow();

    IslandHelper^ ih;

    testHelper.RunOnIslandUIThread([&]()
    {
        ih = IslandHelper::CreateOnCurrentThreadAndNewWindow(m_windowClassAtom);
    });

    StackPanel^ disconnected;
    AutoSuggestBox^ asb;

    testHelper.RunOnIslandUIThread([&]()
    {
        LOG_OUTPUT(L"  > Creating tree.");

        DesktopWindowXamlSource^ dwxs = safe_cast<DesktopWindowXamlSource^>(ih->DesktopWindowXamlSource);

        disconnected = ref new StackPanel();
        disconnected->Width = 200;
        disconnected->Height = 200;
        disconnected->Background = ref new SolidColorBrush(ColorHelper::FromArgb(0xff, 0, 0xff, 0));
        dwxs->Content = disconnected;

        // Put this in the tree to start with so it can run layout and apply its template. This lets the AutoSuggestBox
        // find its suggestion popup. If we keep it out of the tree, the ASB doesn't have a popup and never tries to
        // open anything.
        asb = ref new AutoSuggestBox();
        asb->Margin = xaml::ThicknessHelper::FromLengths(50, 100, 0, 0);
        disconnected->Children->Append(asb);
    });

    LowBudgetWaitForIdle(ih);

    testHelper.RunOnIslandUIThread([&]()
    {
        LOG_OUTPUT(L"  > Taking AutoSuggestBox out of the tree.");

        DesktopWindowXamlSource^ dwxs = safe_cast<DesktopWindowXamlSource^>(ih->DesktopWindowXamlSource);
        dwxs->Content = nullptr;
    });

    LowBudgetWaitForIdle(ih);

    testHelper.RunOnIslandUIThread([&]()
    {
        LOG_OUTPUT(L"  > Destroy the island.");
        ih = nullptr;
    });

    ::Sleep(50);

    testHelper.RunOnIslandUIThread([&]()
    {
        LOG_OUTPUT(L"  > Opening AutoSuggestBox's popup. Don't crash.");

        asb->IsSuggestionListOpen = true;
    });

    ::Sleep(50);
}

void XamlIslandTests::CloseIslandWhileAsbPopupOpening()
{
    XamlIslandTestHelper testHelper(this);
    testHelper.StartAppOnCurrentThread();
    testHelper.StartAndPrepNewUIThreadWithWindow();

    IslandHelper^ ih;

    testHelper.RunOnIslandUIThread([&]()
    {
        ih = IslandHelper::CreateOnCurrentThreadAndNewWindow(m_windowClassAtom);
    });

    StackPanel^ sp;
    AutoSuggestBox^ asb;

    testHelper.RunOnIslandUIThread([&]()
    {
        LOG_OUTPUT(L"  > Creating tree.");

        DesktopWindowXamlSource^ dwxs = safe_cast<DesktopWindowXamlSource^>(ih->DesktopWindowXamlSource);

        sp = ref new StackPanel();
        sp->Width = 200;
        sp->Height = 200;
        sp->Background = ref new SolidColorBrush(ColorHelper::FromArgb(0xff, 0, 0xff, 0));
        dwxs->Content = sp;

        // Put this in the tree to start with so it can run layout and apply its template. This lets the AutoSuggestBox
        // find its suggestion popup. If we keep it out of the tree, the ASB doesn't have a popup and never tries to
        // open anything.
        asb = ref new AutoSuggestBox();
        asb->Margin = xaml::ThicknessHelper::FromLengths(50, 100, 0, 0);
        sp->Children->Append(asb);
    });

    LowBudgetWaitForIdle(ih);

    testHelper.RunOnIslandUIThread([&]()
    {
        LOG_OUTPUT(L"  > opening popup and destroying island.");

        DesktopWindowXamlSource^ dwxs = safe_cast<DesktopWindowXamlSource^>(ih->DesktopWindowXamlSource);
        asb->IsSuggestionListOpen = true;
        ih = nullptr;
    });

    ::Sleep(50);
}

void XamlIslandTests::ValidateUiaAfterVisualTreeReset()
{
    XamlIslandTestHelper testHelper(this);
    testHelper.StartAppOnCurrentThread();
    testHelper.StartAndPrepNewUIThreadWithWindow();

    IslandHelper^ ih;
    DesktopWindowXamlSource^ dwxs;
    HWND dwxsHwnd {};

    testHelper.RunOnIslandUIThread([&]()
    {
        ih = IslandHelper::CreateOnCurrentThreadAndNewWindow(m_windowClassAtom);
    });

    Button^ button1;

    testHelper.RunOnIslandUIThread([&]()
    {
        dwxs = safe_cast<DesktopWindowXamlSource^>(ih->DesktopWindowXamlSource);

        button1 = ref new Button();
        button1->Content = "BeforeButton";
        xaml_automation::AutomationProperties::SetName(button1, ref new Platform::String(L"BeforeButton"));
        dwxs->Content = button1;

        // The WindowId property returns a WindowId type in the Microsoft::UI namespace, but GetWindowFromId from
        // Microsoft.UI.Interop.h requires the type to be in ABI::Microsoft::UI.
        VERIFY_SUCCEEDED(ABI::Microsoft::UI::GetWindowFromWindowId(
            ABI::Microsoft::UI::WindowId {dwxs->SiteBridge->WindowId.Value},
            &dwxsHwnd));
    });
    LowBudgetWaitForIdle(ih);

    auto findUiaElement = [&](const wchar_t* elementName) -> wrl::ComPtr<IUIAutomationElement> {
        wrl::ComPtr<IUIAutomationElement> automationElement;
        testHelper.RunOnIslandUIThread([&]()
        {
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = elementName;
            uiaInfo.m_AutomationID = elementName;
            uiaInfo.m_cType = UIA_ButtonControlTypeId;

            auto spAutomationClientManager = Automation::AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo, dwxsHwnd);
            spAutomationClientManager->GetCurrentUIAutomationElement(&automationElement);
        });
        return automationElement;
    };

    VERIFY_IS_NOT_NULL(findUiaElement(L"BeforeButton").Get());

    testHelper.RunOnIslandUIThread([&]()
    {
        LOG_OUTPUT(L"Set DesktopWindowXamlSource.Content to null.");
        dwxs->Content = nullptr;
    });
    LowBudgetWaitForIdle(ih);

    VERIFY_IS_NULL(findUiaElement(L"BeforeButton").Get());

    testHelper.RunOnIslandUIThread([&]()
    {
        LOG_OUTPUT(L"Set DesktopWindowXamlSource.Content to new Button 'AfterButton'.");
        Button^ afterButton = ref new Button();
        afterButton->Content = "AfterButton";
        xaml_automation::AutomationProperties::SetName(afterButton, ref new Platform::String(L"AfterButton"));
        dwxs->Content = afterButton;
    });
    LowBudgetWaitForIdle(ih);

    VERIFY_IS_NULL(findUiaElement(L"BeforeButton").Get());

    LowBudgetWaitForIdle(ih);

    VERIFY_IS_NOT_NULL(findUiaElement(L"AfterButton").Get());

    LowBudgetWaitForIdle(ih);
}

void XamlIslandTests::ValidateDispatcherShutdownModeInIslandsApp()
{
    XamlIslandTestHelper testHelper(this);
    testHelper.StartAppOnCurrentThread();

    LOG_OUTPUT(L"DispatcherShutdownMode should default to OnExplicitShutdown.");
    VERIFY_ARE_EQUAL(DispatcherShutdownMode::OnExplicitShutdown, Application::Current->DispatcherShutdownMode);

    testHelper.StartAndPrepNewUIThreadWithWindow();

    Event dqExitEvent;

    testHelper.GetUIThreadDispatcherQueueController()->DispatcherQueue->ShutdownStarting +=
        ref new TypedEventHandler<DispatcherQueue^,DispatcherQueueShutdownStartingEventArgs^>(
            [&](DispatcherQueue^,DispatcherQueueShutdownStartingEventArgs^){
                LOG_OUTPUT(L"ShutdownStarting raised.");
                dqExitEvent.Set();
        });

    testHelper.RunOnIslandUIThread([&]()
    {
        LOG_OUTPUT(L"DispatcherShutdownMode should default to OnExplicitShutdown.");
        VERIFY_ARE_EQUAL(DispatcherShutdownMode::OnExplicitShutdown, Application::Current->DispatcherShutdownMode);

        LOG_OUTPUT(L"Creating Window.");
        Window^ window = ref new Window();
        window->Activate();
        window->Close();
        LOG_OUTPUT(L"Closed Window.");
    });

    LowBudgetWaitForIdle(testHelper.GetUIThreadDispatcherQueueController()->DispatcherQueue);

    // DQ Should still be running.
    VERIFY_IS_FALSE(dqExitEvent.HasFired());

    testHelper.RunOnIslandUIThread([&]()
    {
        Application::Current->DispatcherShutdownMode = DispatcherShutdownMode::OnLastWindowClose;

        // Verify the prop round-trips.
        VERIFY_ARE_EQUAL(DispatcherShutdownMode::OnLastWindowClose, Application::Current->DispatcherShutdownMode);

        LOG_OUTPUT(L"Creating Window 2.");
        Window^ window = ref new Window();
        window->Activate();
        window->Close();
        LOG_OUTPUT(L"Closed Window 2.");
    });

    LOG_OUTPUT(L"The DispatcherQueue on the UI thread should automatically exit.");
    dqExitEvent.WaitForDefault();

    LOG_OUTPUT(L"DispatcherShutdownMode on the test thread should still be the default.");
    VERIFY_ARE_EQUAL(DispatcherShutdownMode::OnExplicitShutdown, Application::Current->DispatcherShutdownMode);

    testHelper.WaitForUIThreadExit();
}

void XamlIslandTests::ValidateDispatcherShutdownModeInDesktopApp()
{
    auto uiThread = RunOnNewThread([this]() {
        VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_SINGLETHREADED));

        LOG_OUTPUT(L"Start an Application the way a WinUI Desktop app does.");
        Application::Start(ref new ApplicationInitializationCallback(
            [](ApplicationInitializationCallbackParams^) {
                auto app = ref new Application();
                LOG_OUTPUT(L"DispatcherShutdownMode should default to OnLastWindowClose in this case.");
                VERIFY_ARE_EQUAL(DispatcherShutdownMode::OnLastWindowClose, Application::Current->DispatcherShutdownMode);

                LOG_OUTPUT(L"Exit should do a PostQuitMessage.");
                Application::Current->Exit();
            }));
    });
    WaitForSingleObjectWithTimeout(uiThread);
}

void XamlIslandTests::EffectiveViewportChangedInWindowedPopup()
{
    XamlIslandTestHelper testHelper(this);
    testHelper.StartAppOnCurrentThread();
    testHelper.StartAndPrepNewUIThreadWithWindow();

    IslandHelper^ ih1;

    wf::EventRegistrationToken islandEffectiveViewportChangedToken = {};
    Event islandEffectiveViewportChangedEvent;
    float islandEffectiveViewportWidth = 0;
    float islandEffectiveViewportHeight = 0;
    bool islandEffectiveViewportChangedRaised = false;

    wf::EventRegistrationToken windowedPopupEffectiveViewportChangedToken = {};
    Event windowedPopupEffectiveViewportChangedEvent;
    float windowedPopupEffectiveViewportWidth = 0;
    float windowedPopupEffectiveViewportHeight = 0;
    bool windowedPopupEffectiveViewportChangedRaised = false;

    testHelper.RunOnIslandUIThread([&]()
    {
        LOG_OUTPUT(L"> Make a small island...");
        ih1 = IslandHelper::CreateOnCurrentThreadAndNewWindow(m_windowClassAtom);

        Grid^ rootGrid = ref new Grid();
        rootGrid->Width = 100;
        rootGrid->Height = 50;
        rootGrid->Background = ref new SolidColorBrush(ColorHelper::FromArgb(0xff, 0, 0xff, 0));
        safe_cast<DesktopWindowXamlSource^>(ih1->DesktopWindowXamlSource)->Content = rootGrid;
        safe_cast<DesktopWindowXamlSource^>(ih1->DesktopWindowXamlSource)->SiteBridge->MoveAndResize({ 0, 0, 100, 50 });

        ScrollViewer^ scrollViewer = ref new ScrollViewer();
        rootGrid->Children->Append(scrollViewer);

        Grid^ grid = ref new Grid();
        grid->Width = 1000;
        grid->Height = 1000;
        scrollViewer->Content = grid;

        islandEffectiveViewportChangedToken = grid->EffectiveViewportChanged +=
            ref new ::Windows::Foundation::TypedEventHandler<FrameworkElement^, EffectiveViewportChangedEventArgs^>(
                [&](FrameworkElement^, EffectiveViewportChangedEventArgs^ args)
                {
                    // The EffectiveViewportChanged event in the small island should be clipped by the island size (i.e. be 100x50).
                    // Save the values and VERIFY_IS_TRUE outside the event handler. Failing a VERIFY_IS_TRUE check here
                    // crashes the test (as opposed to just failing the test, which is much faster).
                    islandEffectiveViewportWidth = args->EffectiveViewport.Width;
                    islandEffectiveViewportHeight = args->EffectiveViewport.Height;
                    LOG_OUTPUT(L"  > island ScrollViewer.Content EffectiveViewportChanged, viewport size is %.2f x %.2f", islandEffectiveViewportWidth, islandEffectiveViewportHeight);

                    islandEffectiveViewportChangedRaised = true;
                    islandEffectiveViewportChangedEvent.Set();
                });

        LOG_OUTPUT(L"> With a large windowed popup...");
        Popup^ windowedPopup = ref new Popup();
        windowedPopup->ShouldConstrainToRootBounds = false;
        rootGrid->Children->Append(windowedPopup);

        Grid^ windowedPopupRoot = ref new Grid();
        windowedPopupRoot->Width = 300;
        windowedPopupRoot->Height = 600;
        windowedPopup->Child = windowedPopupRoot;

        ScrollViewer^ popupScrollViewer = ref new ScrollViewer();
        windowedPopupRoot->Children->Append(popupScrollViewer);

        Grid^ popupGrid = ref new Grid();
        popupGrid->Width = 1000;
        popupGrid->Height = 1000;
        popupScrollViewer->Content = popupGrid;

        windowedPopupEffectiveViewportChangedToken = popupGrid->EffectiveViewportChanged +=
            ref new ::Windows::Foundation::TypedEventHandler<FrameworkElement^, EffectiveViewportChangedEventArgs^>(
                [&](FrameworkElement^, EffectiveViewportChangedEventArgs^ args)
                {
                    // The EffectiveViewportChanged event in the windowed popup should not be clipped by the island size (i.e. be 300x600).
                    // Save the values and VERIFY_IS_TRUE outside the event handler. Failing a VERIFY_IS_TRUE check here
                    // crashes the test (as opposed to just failing the test, which is much faster).
                    windowedPopupEffectiveViewportWidth = args->EffectiveViewport.Width;
                    windowedPopupEffectiveViewportHeight = args->EffectiveViewport.Height;
                    LOG_OUTPUT(L"  > popup ScrollViewer.Content EffectiveViewportChanged, viewport size is %.2f x %.2f", windowedPopupEffectiveViewportWidth, windowedPopupEffectiveViewportHeight);

                    windowedPopupEffectiveViewportChangedRaised = true;
                    windowedPopupEffectiveViewportChangedEvent.Set();
                });

        windowedPopup->IsOpen = true;
    });

    // Wait to verify both EffectiveViewportChanged events.
    islandEffectiveViewportChangedEvent.WaitForDefault();
    VERIFY_IS_TRUE(islandEffectiveViewportChangedRaised);
    // The MoveAndResize rect is in screen pixels. If there's a display scale, this viewport will be scaled by the display scale.
    // Use <= to allow for different display scales.
    VERIFY_IS_TRUE(0 < islandEffectiveViewportWidth && islandEffectiveViewportWidth <= 100);
    VERIFY_IS_TRUE(0 < islandEffectiveViewportHeight && islandEffectiveViewportHeight <= 50);

    windowedPopupEffectiveViewportChangedEvent.WaitForDefault();
    VERIFY_IS_TRUE(windowedPopupEffectiveViewportChangedRaised);
    // This viewport comes from the layout size imposed by windowedPopupRoot. That size is in logical pixels so we expect an exact match.
    VERIFY_IS_TRUE(windowedPopupEffectiveViewportWidth == 300);
    VERIFY_IS_TRUE(windowedPopupEffectiveViewportHeight == 600);

}

// This is a regression test for a bug where the UIA FindAll() would not find normal Xaml content in the tree if a windowed popup was open.
void XamlIslandTests::ValidateUiaFindAllWithWindowedPopup()
{
    XamlIslandTestHelper testHelper(this);
    testHelper.StartAppOnCurrentThread();

    auto ih1 = IslandHelper::CreateOnNewUIThreadAndNewWindow(m_windowClassAtom);

    DesktopWindowXamlSource^ desktopWindowXamlSource;
    Button^ buttonInPanel;
    Button^ buttonInPopup;
    Popup^ popup;
    HWND dwxsHwnd{};

    RunOnIslandUIThread(ih1, [&]()
    {
        desktopWindowXamlSource = safe_cast<DesktopWindowXamlSource^>(ih1->DesktopWindowXamlSource);
        auto stackPanel = safe_cast<StackPanel^>(xaml_markup::XamlReader::Load(
            L"<StackPanel x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
            L"  Padding='10' >"
            L"  <Popup x:Name='popup' ShouldConstrainToRootBounds='False' >"
            L"    <Border Background='LightBlue'>"
            L"      <Button x:Name='buttonInPopup' AutomationProperties.Name='buttonInPopup' Margin='10' Width='125' Height='50'>buttonInPopup</Button>"
            L"    </Border>"
            L"  </Popup>"
            L"  <Button x:Name='buttonInPanel' AutomationProperties.Name='buttonInPanel' Margin='60' Width='125' Height='50'>buttonInPanel</Button>"
            L"</StackPanel>"));

        buttonInPanel = safe_cast<Button^>(stackPanel->FindName("buttonInPanel"));
        buttonInPopup = safe_cast<Button^>(stackPanel->FindName("buttonInPopup"));
        popup = safe_cast<Popup^>(stackPanel->FindName("popup"));

        desktopWindowXamlSource->Content = stackPanel;

        VERIFY_SUCCEEDED(ABI::Microsoft::UI::GetWindowFromWindowId(
            ABI::Microsoft::UI::WindowId {desktopWindowXamlSource->SiteBridge->WindowId.Value},
            &dwxsHwnd));
    });

    // Short sleep just to allow DWXS to be briefly visible.
    ::Sleep(500);

    LowBudgetWaitForIdle(ih1);
    
    RunOnIslandUIThread(ih1, [&]()
    {
        popup->IsOpen = true;
    });
    LowBudgetWaitForIdle(ih1);

    bool buttonInPanelFound = false;
    bool buttonInPopupFound = false;

    // We specifically run this NOT on Xaml's UI thread.  When run on Xaml's UI thread, the bug doesn't repro.
    {
        wrl::ComPtr<IUIAutomation> automation;

        auto automationClientManager = std::make_shared<Automation::AutomationClient::AutomationClientManager>();
        automationClientManager->GetAutomation(&automation);

        wrl::ComPtr<IUIAutomationElement> windowElement;
        LogThrow_IfFailed(automation->ElementFromHandle(::GetParent(dwxsHwnd), &windowElement));

        auto visitElement = [&buttonInPanelFound, &buttonInPopupFound](const wchar_t* label, IUIAutomationElement* element)
        {
            Common::AutoBSTR currentName;
            Common::AutoBSTR currentClassName;
            element->get_CurrentName(currentName.ReleaseAndGetAddressOf());
            element->get_CurrentClassName(currentClassName.ReleaseAndGetAddressOf());

            LOG_OUTPUT(L"%s: Name='%s' Class='%s'", label, currentName.Get(), currentClassName.Get());

            if (currentName && wcscmp(currentName, L"buttonInPanel") == 0)
            {
                LOG_OUTPUT(L"(found buttonInPanel)");
                buttonInPanelFound = true;
            }
            else if (currentName && wcscmp(currentName, L"buttonInPopup") == 0)
            {
                LOG_OUTPUT(L"(found buttonInPopup)");
                buttonInPopupFound = true;
            }
        };

        visitElement(L"Root", windowElement.Get());
        
        wrl::ComPtr<IUIAutomationCondition> trueCondition;
        LogThrow_IfFailed(automation->CreateTrueCondition(&trueCondition));
        
        wrl::ComPtr<IUIAutomationElementArray> children;
        LogThrow_IfFailed(windowElement->FindAll(TreeScope_Descendants, trueCondition.Get(), &children));

        int childCount{};
        LogThrow_IfFailed(children->get_Length(&childCount));
        for (int i=0; i < childCount; ++i)
        {
            wrl::ComPtr<IUIAutomationElement> child;
            LogThrow_IfFailed(children->GetElement(i, &child));
            visitElement(L"Child", child.Get());
        }
    }

    VERIFY_IS_TRUE(buttonInPopupFound);
    VERIFY_IS_TRUE(buttonInPanelFound);
}

ref class MyNavView sealed
    : public Microsoft::UI::Xaml::Controls::NavigationView
{
public:
    MyNavView() {}
    virtual ~MyNavView() { LOG_OUTPUT(L"In MyNavView::~MyNavView");}
};

int GetRefCount(IUnknown* unk)
{
    unk->AddRef();
    return unk->Release();
}

void XamlIslandTests::ValidateNavigationView()
{
    XamlIslandTestHelper testHelper(this);
    testHelper.StartAppOnCurrentThread();

    IUnknown* unk {nullptr};    

    LOG_OUTPUT(L"Create MyNavView...");
    {
        auto navView = ref new MyNavView();
        unk = reinterpret_cast<IUnknown*>(navView);

        // Add 3 extra refs, we'll inspect the refcount later.
        unk->AddRef();
        unk->AddRef();
        unk->AddRef();
        
        navView->SelectedItem = 1;
    }
    
    int refcount = GetRefCount(unk);
    VERIFY_ARE_EQUAL(3, refcount);

    while (unk->Release() > 0)
    {
        // Release all the extra refs.
    }

    // Short sleep just to allow DWXS to be briefly visible.
    ::Sleep(500);
}

void XamlIslandTests::BasicHwndlessIslandTest()
{
    XamlIslandTestHelper testHelper(this);
    auto [islandHelper, xamlIsland] = testHelper.StartStandardHwndlessXamlIslandTest();

    Event childRootLoaded;

    testHelper.RunOnIslandUIThread([&]()
        {
            auto childRoot = safe_cast<Panel^>(XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" VerticalAlignment="Stretch" HorizontalAlignment="Stretch">
                        <TextBlock Text="HWND-less island" />
                    </Grid>)"));

            childRoot->Loaded += ref new RoutedEventHandler([&](Platform::Object^ sender, RoutedEventArgs^ e)
                {
                    LOG_OUTPUT(L"Child root loaded.");
                    childRootLoaded.Set();
                });

            xamlIsland->Content = childRoot;
        });

    childRootLoaded.WaitForDefault();
    LowBudgetWaitForIdle(islandHelper);
}

void XamlIslandTests::ContentRendersInHwndlessIslands()
{
    XamlIslandTestHelper testHelper(this);
    auto [islandHelper, xamlIsland] = testHelper.StartStandardHwndlessXamlIslandTest();

    Grid^ childRoot = nullptr;
    Event childRootLoaded;

    testHelper.RunOnIslandUIThread([&]()
        {
            childRoot = safe_cast<Grid^>(XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" VerticalAlignment="Stretch" HorizontalAlignment="Stretch">
                        <StackPanel>
                            <TextBlock Text="HWND-less island" />
                            <TextBox Text="A text box" />
                            <Button Content="A button" />
                        </StackPanel>
                    </Grid>)"));

            childRoot->Loaded += ref new RoutedEventHandler([&](Platform::Object^ sender, RoutedEventArgs^ e)
                {
                    LOG_OUTPUT(L"Child root loaded.");
                    childRootLoaded.Set();
                });

            xamlIsland->Content = childRoot;
        });

    childRootLoaded.WaitForDefault();
    LowBudgetWaitForIdle(islandHelper);

    RunOnIslandUIThread(islandHelper, [&]()
    {
        VisualTreeVerifier^ verifier = VisualTreeVerifier::CreateFromVisual(childRoot->XamlRoot->ContentIsland->Root);
        verifier->WalkToChildAtIndex(0, 1);
        verifier->VerifyVisualOffset(0, 0);
        verifier->VerifyVisualSize(284, 261);
    });
}

void XamlIslandTests::KeyboardInputWorksInHwndlessIslands()
{
    XamlIslandTestHelper testHelper(this);
    auto [islandHelper, xamlIsland] = testHelper.StartStandardHwndlessXamlIslandTest();

    Event childRootLoaded;
    TextBox^ textBox = nullptr;

    testHelper.RunOnIslandUIThread([&]()
        {
            auto childRoot = safe_cast<Panel^>(XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" VerticalAlignment="Stretch" HorizontalAlignment="Stretch">
                        <TextBox x:Name="TheTextBox" />
                    </Grid>)"));

            textBox = safe_cast<TextBox^>(childRoot->FindName(L"TheTextBox"));

            childRoot->Loaded += ref new RoutedEventHandler([&](Platform::Object^ sender, RoutedEventArgs^ e)
                {
                    LOG_OUTPUT(L"Child root loaded.");
                    childRootLoaded.Set();
                });

            xamlIsland->Content = childRoot;
        });

    childRootLoaded.WaitForDefault();
    LowBudgetWaitForIdle(islandHelper);

    Event textBoxGotFocus;

    testHelper.RunOnIslandUIThread([&]()
        {
            textBox->GotFocus += ref new RoutedEventHandler([&](Platform::Object^ sender, RoutedEventArgs^ e)
                {
                    LOG_OUTPUT(L"TextBox got focus.");
                    textBoxGotFocus.Set();
                });

            textBox->Focus(xaml::FocusState::Programmatic);
        });

    textBoxGotFocus.WaitForDefault();
    LowBudgetWaitForIdle(islandHelper);

    Event textBoxInputComplete;
    int textChangeCount = 0;

    testHelper.RunOnIslandUIThread([&]()
        {
            textBox->TextChanged += ref new Controls::TextChangedEventHandler([&](Platform::Object^ sender, Controls::TextChangedEventArgs^ e)
                {
                    LOG_OUTPUT(L"TextBox text changed.");
                    textChangeCount++;

                    if (textChangeCount == 5)
                    {
                        textBoxInputComplete.Set();
                    }
                });
        });

    SendKeyPresses({ 'H', 'E', 'L', 'L', 'O' });

    textBoxInputComplete.WaitForDefault();
    LowBudgetWaitForIdle(islandHelper);

    testHelper.RunOnIslandUIThread([&]()
        {
            VERIFY_ARE_EQUAL(ref new Platform::String(L"hello"), textBox->Text);
            textBoxInputComplete.Set();
        });
}

void XamlIslandTests::PointerInputWorksInHwndlessIslands()
{
    XamlIslandTestHelper testHelper(this);
    auto [islandHelper, xamlIsland] = testHelper.StartStandardHwndlessXamlIslandTest();

    Event childRootLoaded;
    Button^ button = nullptr;
    Event buttonClicked;

    testHelper.RunOnIslandUIThread([&]()
        {
            auto childRoot = safe_cast<Panel^>(XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" VerticalAlignment="Stretch" HorizontalAlignment="Stretch">
                        <Button x:Name="TheButton" Content="Click me" />
                    </Grid>)"));

            button = safe_cast<Button^>(childRoot->FindName(L"TheButton"));

            button->Click += ref new RoutedEventHandler([&](Platform::Object^ sender, RoutedEventArgs^ e)
                {
                    LOG_OUTPUT(L"Button clicked.");
                    buttonClicked.Set();
                });

            childRoot->Loaded += ref new RoutedEventHandler([&](Platform::Object^ sender, RoutedEventArgs^ e)
                {
                    LOG_OUTPUT(L"Child root loaded.");
                    childRootLoaded.Set();
                });

            xamlIsland->Content = childRoot;
        });

    childRootLoaded.WaitForDefault();
    LowBudgetWaitForIdle(islandHelper);

    islandHelper->LeftMouseClick(button);

    buttonClicked.WaitForDefault();
    LowBudgetWaitForIdle(islandHelper);
}

void XamlIslandTests::PopupsWorkInHwndlessIslands()
{
    XamlIslandTestHelper testHelper(this);
    auto [islandHelper, xamlIsland] = testHelper.StartStandardHwndlessXamlIslandTest();

    Event childRootLoaded;
    Button^ windowlessFlyoutButton = nullptr;
    Button^ windowedFlyoutButton = nullptr;
    Event windowlessFlyoutOpened;
    Event windowlessFlyoutClosed;
    Event windowedFlyoutOpened;
    Event windowedFlyoutClosed;

    testHelper.RunOnIslandUIThread([&]()
        {
            auto childRoot = safe_cast<Panel^>(XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" VerticalAlignment="Stretch" HorizontalAlignment="Stretch">
                        <StackPanel>
                            <Button x:Name="WindowlessFlyoutButton" Content="Click to open windowless flyout">
                                <Button.Flyout>
                                    <Flyout x:Name="WindowlessFlyout">
                                        <TextBlock Text="Flyout content" />
                                    </Flyout>
                                </Button.Flyout>    
                            </Button>
                            <Button x:Name="WindowedFlyoutButton" Content="Click to open windowed flyout">
                                <Button.Flyout>
                                    <MenuFlyout x:Name="WindowedFlyout">
                                        <MenuFlyoutItem Text="Item 1" />
                                        <MenuFlyoutItem Text="Item 2" />
                                        <MenuFlyoutItem Text="Item 3" />
                                    </MenuFlyout>
                                </Button.Flyout>    
                            </Button>
                        </StackPanel>
                    </Grid>)"));

            windowlessFlyoutButton = safe_cast<Button^>(childRoot->FindName(L"WindowlessFlyoutButton"));
            windowedFlyoutButton = safe_cast<Button^>(childRoot->FindName(L"WindowedFlyoutButton"));
            auto windowlessFlyout = safe_cast<Flyout^>(childRoot->FindName(L"WindowlessFlyout"));
            auto windowedFlyout = safe_cast<MenuFlyout^>(childRoot->FindName(L"WindowedFlyout"));

            childRoot->Loaded += ref new RoutedEventHandler([&](Platform::Object^ sender, RoutedEventArgs^ e)
                {
                    LOG_OUTPUT(L"Child root loaded.");
                    childRootLoaded.Set();
                });

            windowlessFlyout->Opened += ref new EventHandler<Object^>([&](Platform::Object^ sender, Object^ e)
                {
                    LOG_OUTPUT(L"Windowless flyout opened.");
                    windowlessFlyoutOpened.Set();
                });

            windowlessFlyout->Closed += ref new EventHandler<Object^>([&](Platform::Object^ sender, Object^ e)
                {
                    LOG_OUTPUT(L"Windowless flyout closed.");
                    windowlessFlyoutClosed.Set();
                });

            windowedFlyout->Opened += ref new EventHandler<Object^>([&](Platform::Object^ sender, Object^ e)
                {
                    LOG_OUTPUT(L"Windowed flyout opened.");
                    windowedFlyoutOpened.Set();
                });

            windowedFlyout->Closed += ref new EventHandler<Object^>([&](Platform::Object^ sender, Object^ e)
                {
                    LOG_OUTPUT(L"Windowed flyout closed.");
                    windowedFlyoutClosed.Set();
                });

            xamlIsland->Content = childRoot;
        });

    childRootLoaded.WaitForDefault();
    LowBudgetWaitForIdle(islandHelper);

    islandHelper->LeftMouseClick(windowlessFlyoutButton);

    windowlessFlyoutOpened.WaitForDefault();
    LowBudgetWaitForIdle(islandHelper);

    islandHelper->LeftMouseClick(windowlessFlyoutButton);

    windowlessFlyoutClosed.WaitForDefault();
    LowBudgetWaitForIdle(islandHelper);

    islandHelper->LeftMouseClick(windowedFlyoutButton);

    windowedFlyoutOpened.WaitForDefault();
    LowBudgetWaitForIdle(islandHelper);

    islandHelper->LeftMouseClick(windowedFlyoutButton);

    windowedFlyoutClosed.WaitForDefault();
    LowBudgetWaitForIdle(islandHelper);
}

} // namespace Microsoft::UI::Xaml::Tests::Foundation::Hosting
