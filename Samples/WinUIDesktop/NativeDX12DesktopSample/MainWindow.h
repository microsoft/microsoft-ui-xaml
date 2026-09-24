// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "MainWindow.g.h"
#include "StepTimer.h"
namespace DX
{
    class DeviceResources;
}

namespace winrt::NativeDX12DesktopSample::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();
        ~MainWindow();

    private:
        std::shared_ptr<DX::DeviceResources> m_deviceResources;
        winrt::Windows::Foundation::IAsyncAction m_renderLoopWorker;
  
        // Rendering loop timer.
        DX::StepTimer m_timer;

    public:
        void mySwapChainPanel_SizeChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::SizeChangedEventArgs const& e);
        void mySwapChainPanel_Loaded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void StartRenderLoop();
        void StopRenderLoop();
        void Update();
     };
}

namespace winrt::NativeDX12DesktopSample::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
