// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"

#include "App.h"
#include "MainWindow.h"


winrt::Microsoft::UI::Xaml::Hosting::WindowsXamlManager g_windowsXamlManager{ nullptr };
bool g_appDestructed {false};

void OnAppCreated()
{
    // We need to initialize the WindowsXamlManager _while_ the App object is being created. If we initialize the
    // WindowsXamlManager _before_ the App object is created, this will cause the DXamlCore to start up, using it's
    // default MUX.Application object, so the app won't have a chance to use its own. If we initialize the
    // WindowsXamlManager _after_ the App object is created, the App creation will fail, because we'll call
    // MUX.Application.LoadComponent before the DXamlCore is initialized.  So, we must do it here.
    g_windowsXamlManager = winrt::Microsoft::UI::Xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread();
    g_appDestructed = false;
}

void OnAppDestructed()
{
    g_appDestructed = true;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    return MainWindow::Run(hInstance, nCmdShow);
}
