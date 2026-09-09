// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winrt/Windows.UI.Popups.h"
#include "winrt/Microsoft.UI.Xaml.Markup.h"
#include "DisableXBindTests.h"
#include "DisableXBindTests.g.cpp"
#include "DetectLeaksPage.h"

namespace winrt::BindTestbed::implementation
{
    using namespace ::winrt::Windows::UI::Popups;

    DisableXBindTests::DisableXBindTests()
    {
        InitializeComponent();
        DetectLeaksPage::TrackObject(*this, xaml_typename<BindTestbed::DisableXBindTests>().Name);
    }

    void DisableXBindTests::Click_RegularArgs(IInspectable const&, wux::RoutedEventArgs const&)
    {
        auto dlg = MessageDialog(L"Regular arguments clicked");
        auto t = dlg.ShowAsync();
    }

    void DisableXBindTests::Click_NoArgs()
    {
        auto dlg = MessageDialog(L"No argument Clicked");
        auto t = dlg.ShowAsync();
    }

    void DisableXBindTests::On_Loaded(IInspectable const&, wux::RoutedEventArgs const&)
    {
        // C++/WinRT does not generate IXamlBindScopeDiagnostics, so Disable(line, column) is
        // unavailable. The page still covers function, event, and two-way x:Bind generation.
        if (!Bindings)
        {
            return;
        }

        if (auto diagnostics = Bindings.try_as<wux::Markup::IXamlBindScopeDiagnostics>())
        {
            for (int32_t lineNumber = 0; lineNumber < 40; lineNumber++)
            {
                for (int32_t columnNumber = 0; columnNumber < 100; columnNumber++)
                {
                    diagnostics.Disable(lineNumber, columnNumber);
                }
            }
        }
    }
}
