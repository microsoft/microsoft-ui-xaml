// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "MainWindow.h"
#include "MainWindow.g.cpp"
#include "DeviceResources.h"
#include <Microsoft.UI.Xaml.Window.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::NativeDX12DesktopSample::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();
    }

    MainWindow::~MainWindow()
    {
        StopRenderLoop();
    }

    void MainWindow::mySwapChainPanel_SizeChanged(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::SizeChangedEventArgs const& e)
    {
        if (m_deviceResources == nullptr) return;

        if (e.NewSize().Height == 0 || e.NewSize().Width == 0)
        {
            StopRenderLoop();
            return;
        }
        m_deviceResources->SetLogicalSize(e.NewSize().Width, e.NewSize().Height);
        if (m_renderLoopWorker == nullptr)
        {
            StartRenderLoop();
        }

    }

    void MainWindow::mySwapChainPanel_Loaded(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        
        winrt::com_ptr<IWindowNative> windowNative = try_as<IWindowNative>();
        if (!windowNative) throw hresult_no_interface();

        HWND hwnd{};
        HRESULT hr = windowNative->get_WindowHandle(&hwnd);
        if (FAILED(hr)) throw_hresult(hr);

        m_deviceResources = std::make_shared<DX::DeviceResources>(hwnd);
        m_deviceResources->SetSwapChainPanel(mySwapChainPanel());
        m_deviceResources->Initialize();
        StartRenderLoop();
    }

    void MainWindow::StartRenderLoop()
    {
        // If the animation render loop is already running then do not start another thread.
        if (m_renderLoopWorker != nullptr && m_renderLoopWorker.Status() == winrt::Windows::Foundation::AsyncStatus::Started)
        {
            return;
        }

        // Create a task that will be run on a background thread.
#pragma warning( push )
#pragma warning( disable : 26110 ) // Bug in analysis that doesn't property recongize reference to a mutex
        auto workItemHandler = ::Windows::System::Threading::WorkItemHandler([this](winrt::Windows::Foundation::IAsyncAction action)
            {
                std::shared_ptr<DX::DeviceResources> deviceResources = m_deviceResources;
                // Calculate the updated frame and render once per vertical blanking interval.
                while (action.Status() == winrt::Windows::Foundation::AsyncStatus::Started)
                {
                    std::lock_guard lock(deviceResources->GetMutex());
                    Update();
                    deviceResources->Render();
                    deviceResources->Present();
                }
            });

        // Run task on a dedicated high priority background thread.
        m_renderLoopWorker = winrt::Windows::System::Threading::ThreadPool::RunAsync(workItemHandler, winrt::Windows::System::Threading::WorkItemPriority::High, winrt::Windows::System::Threading::WorkItemOptions::TimeSliced);
    }
#pragma warning( pop )

    void MainWindow::StopRenderLoop()
    {
        if (m_renderLoopWorker != nullptr && m_renderLoopWorker.Status() == winrt::Windows::Foundation::AsyncStatus::Started)
        {
            m_renderLoopWorker.Cancel();
        }
        m_renderLoopWorker = nullptr;
    }

    // Updates the application state once per frame.
    void MainWindow::Update()
    {
        //ProcessInput();

        // Update scene objects.
        m_timer.Tick([&]()
            {
                m_deviceResources->UpdateFrame();
                // TODO: Replace this with your app's content update functions.
            });
    }
}
