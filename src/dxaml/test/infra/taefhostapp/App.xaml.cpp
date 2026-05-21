// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

using namespace TaefHostApp;

App::App()
{
    RequiresPointerMode = Microsoft::UI::Xaml::ApplicationRequiresPointerMode::WhenRequested;
    InitializeComponent();
}

void App::OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs^ e)
{
    Microsoft::UI::Xaml::Window::Current->Activate();
    Microsoft::VisualStudio::TestPlatform::TestExecutor::WinRTCore::UnitTestClient::Run(e->Arguments);
}
