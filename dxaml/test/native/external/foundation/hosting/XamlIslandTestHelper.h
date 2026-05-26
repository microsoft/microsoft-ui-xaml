// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//
// RAII helper to wrap common XamlIslandTests tasks.
//
// Initializes and manages an Application object and a DispatcherQueueController on the test thread.
//
// Optionally manages a UI thread and an hwnd for the test as well. There's some overlap here with IslandHelper,
// depending on the scenario.
//
// For a single UI thread with a single hwnd with multiple islands, XamlIslandTestHelper should be used to manage the UI
// thread and the hwnd. Multiple IslandHelpers should be used to manage the islands.
//
// For a single UI thread with multiple hwnds, XamlIslandTestHelper should be used to manage the UI thread. Multiple
// IslandHelpers should be used to manage the hwnds (assuming one island in each).
//
// For a single UI thread with a single hwnd and a single island, there are many options:
//   1. XamlIslandTestHelper manages the UI thread and hwnd, and IslandHelper manages the island.
//   2. XamlIslandTestHelper manages the UI thread, and IslandHelper manages the hwnd and the island.
//   3. XamlIslandTestHelper manages nothing (except the Application and the test thread DQC), and IslandHelper manages
//      the UI thread, the hwnd, and the island.
//
// The overarching pattern is that there's only ever one XamlIslandTestHelper, and as many IslandHelpers as needed.
//
struct XamlIslandTestHelper
{
    XamlIslandTestHelper(Microsoft::UI::Xaml::Tests::Foundation::Hosting::XamlIslandTests* xamlIslandTests)
        : m_xamlIslandTests(xamlIslandTests) {}

    void StartAppOnCurrentThread(RO_INIT_TYPE initType = RO_INIT_SINGLETHREADED)
    {
        VERIFY_SUCCEEDED(::RoInitialize(initType));

        m_testThreadDqc = Microsoft::UI::Dispatching::DispatcherQueueController::CreateOnCurrentThread();
        LOG_OUTPUT(L"> Creating Application object...");
        m_application = ref new XamlIslandTests_ApplicationWithMuxc();
    }

    void StartAndPrepNewUIThreadWithWindow(RO_INIT_TYPE initType = RO_INIT_SINGLETHREADED)
    {
        Microsoft::UI::Dispatching::DispatcherQueueController^ dqc;
        Microsoft::UI::Xaml::Hosting::WindowsXamlManager^ wxm;
        Microsoft::UI::Xaml::Hosting::DesktopWindowXamlSource^ dwxs;
        Event uiThreadReady;

        m_uiThreadHandle = RunOnNewThread([&, initType]()
            {
                VERIFY_SUCCEEDED(::RoInitialize(initType));

                // Note: even if the XamlIslandTestHelper doesn't manage the hwnds with the islands, we need this hwnd.
                // The scenario is a single UI thread with multiple hwnds that are managed by IslandHelpers. We manage the
                // UI thread here, and something needs to post a WM_QUIT to an hwnd on the UI thread to get us to exit the
                // event loop. That something can't be an IslandHelper - the multiple windows can come and go and they
                // shouldn't shut down the event loop. It has to be us, because we manage the UI thread and event loop. So
                // this is just a message hwnd for shutting down and won't have an island inside it.
                m_topLevelHwnd = m_xamlIslandTests->CreateHwnd(1);

                m_uiThreadDqc = Microsoft::UI::Dispatching::DispatcherQueueController::CreateOnCurrentThread();
                m_uiThreadDq = m_uiThreadDqc->DispatcherQueue;
                uiThreadReady.Set();

                m_initialWindowsXamlManager = Microsoft::UI::Xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread();

                // Note: This call lets DispatcherQueue run the message pump and turns this UI thread into a "live app" that
                // responds to input and messages. It exits when we receive the WM_QUIT message posted at the end of the test.
                m_uiThreadDqc->DispatcherQueue->RunEventLoop();

                LOG_OUTPUT(L"  > Event loop has finished, shut down the DispatcherQueue.");

                m_uiThreadDqc->ShutdownQueue();

                LOG_OUTPUT(L"  > Exiting UI Thread.");
            });
        uiThreadReady.WaitForDefault();
    }

    template <typename TFunction>
    void RunOnIslandUIThread(const TFunction& function)
    {
        RunOnDispatcherThread(m_uiThreadDq, true, function);
    }

    DispatcherQueueController^ GetUIThreadDispatcherQueueController()
    {
        return m_uiThreadDqc;
    }

    void WaitForUIThreadExit()
    {
        WaitForSingleObjectWithTimeout(m_uiThreadHandle);
        m_uiThreadHandle = nullptr;
    }

    void Close()
    {
        VERIFY_IS_FALSE(m_isClosed);

        if (m_childSiteLink)
        {
            CloseObject(m_childSiteLink);
            m_childSiteLink = nullptr;
        }

        if (m_uiThreadHandle)
        {
            LOG_OUTPUT(L"> Sending WM_QUIT to make UI thread exit.");
            ::DestroyWindow(m_topLevelHwnd);
            ::PostMessage(m_topLevelHwnd, WM_QUIT, 0, 0);

            WaitForSingleObjectWithTimeout(m_uiThreadHandle);
            m_uiThreadHandle = nullptr;
        }

        if (m_application)
        {
            LOG_OUTPUT(L"> Shutdown m_testThreadDqc and m_application.");
            m_testThreadDqc->ShutdownQueue();
            m_application->Cleanup();
            m_application = nullptr;
        }

        m_isClosed = true;
    }

    std::tuple<IslandHelper^, XamlIsland^> StartStandardHwndlessXamlIslandTest()
    {
        StartAppOnCurrentThread();
        StartAndPrepNewUIThreadWithWindow();

        IslandHelper^ islandHelper;

        RunOnIslandUIThread([&]()
            {
                islandHelper = IslandHelper::CreateOnCurrentThreadAndNewWindow(m_xamlIslandTests->GetWindowClassAtom());
            });

        Border^ hostElement = nullptr;

        RunOnIslandUIThread([&]()
            {
                LOG_OUTPUT(L"  > Creating tree.");

                auto root = safe_cast<Panel^>(XamlReader::Load(
                    LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Background="{ThemeResource ApplicationPageBackgroundThemeBrush}">
                        <Border x:Name="HostElement" />
                    </Grid>)"));

                hostElement = safe_cast<Border^>(root->FindName("HostElement"));

                DesktopWindowXamlSource^ dwxs = safe_cast<DesktopWindowXamlSource^>(islandHelper->DesktopWindowXamlSource);
                dwxs->Content = root;
            });

        LowBudgetWaitForIdle(islandHelper);

        auto xamlIsland = ConnectHwndlessXamlIsland(hostElement);

        return { islandHelper, xamlIsland };
    }

    XamlIsland^ ConnectHwndlessXamlIsland(UIElement^ hostElement)
    {
        XamlIsland^ xamlIsland = nullptr;

        RunOnIslandUIThread([&]()
            {
                auto visual = safe_cast<ContainerVisual^>(ElementCompositionPreview::GetElementVisual(hostElement));
                m_childSiteLink = Microsoft::UI::Content::ChildSiteLink::Create(hostElement->XamlRoot->ContentIsland, visual);

                xamlIsland = ref new XamlIsland();

                m_childSiteLink->Connect(xamlIsland->ContentIsland);
                m_childSiteLink->ActualSize = hostElement->ActualSize;

                auto transform = hostElement->TransformToVisual(nullptr);
                auto point = transform->TransformPoint(wf::Point{ 0, 0 });
                m_childSiteLink->LocalToParentTransformMatrix = wfn::float4x4(
                    1, 0, 0, 0,
                    0, 1, 0, 0,
                    0, 0, 1, 0,
                    static_cast<float>(point.X), static_cast<float>(point.Y), 0, 1);
            });

        return xamlIsland;
    }

    ~XamlIslandTestHelper()
    {
        if (!m_isClosed)
        {
            Close();
        }
    }

    HWND m_topLevelHwnd{ NULL };
    wil::unique_handle m_uiThreadHandle;
    XamlIslandTests_ApplicationWithMuxc^ m_application;
    Microsoft::UI::Xaml::Hosting::WindowsXamlManager^ m_initialWindowsXamlManager;
    Microsoft::UI::Dispatching::DispatcherQueueController^ m_uiThreadDqc;
    Microsoft::UI::Dispatching::DispatcherQueue^ m_uiThreadDq;
    Microsoft::UI::Dispatching::DispatcherQueueController^ m_testThreadDqc;
    Microsoft::UI::Content::ChildSiteLink^ m_childSiteLink;
    Microsoft::UI::Xaml::Tests::Foundation::Hosting::XamlIslandTests* m_xamlIslandTests {nullptr};

    bool m_isClosed{ false };
};
