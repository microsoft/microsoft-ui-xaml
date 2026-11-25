// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "App.h"

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::NoAppContainerProperty::implementation;

App::App()
{
    InitializeComponent();
}

void App::OnLaunched(LaunchActivatedEventArgs const&)
{
    m_window = Window();
    m_window.Activate();
}
