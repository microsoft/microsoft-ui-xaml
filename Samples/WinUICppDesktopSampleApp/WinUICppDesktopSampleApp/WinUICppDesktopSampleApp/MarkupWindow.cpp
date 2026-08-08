// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "MarkupWindow.h"
#include "MarkupWindow.g.cpp"
#include "strsafe.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Input;

namespace winrt::WinUICppDesktopSampleApp::implementation
{
    MarkupWindow::MarkupWindow()
    {
        InitializeComponent();
        Title(L"WinUI Desktop - Markup Window");
    }

    Button MarkupWindow::GetMainWindowActivateButton()
    {
        return buttonActivateMainWindow();
    }

    void MarkupWindow::ButtonCloseWindow_Click(IInspectable const&, RoutedEventArgs const&)
    {
        Close();
    }

    void MarkupWindow::ButtonExitApplication_Click(IInspectable const&, RoutedEventArgs const&)
    {
        Application::Current().Exit();
    }
}
