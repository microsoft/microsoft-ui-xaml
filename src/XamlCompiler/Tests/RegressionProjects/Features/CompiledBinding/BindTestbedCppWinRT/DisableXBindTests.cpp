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
        // The C#, VB and C++/CX code generators implement IXamlBindScopeDiagnostics on the generated
        // XamlBindings class when EnableXBindDiagnostics is set (see the ShouldGenerateDisableXBind
        // branches in MoComCppBindingInfoPass1.tt / MoComCppBindingInfoPass2.tt).  The C++/WinRT
        // generator has no equivalent, so Bindings does not expose Disable(line, column) and the
        // query below is expected to fail today.  The page is still exercised for the x:Bind code
        // generation it shares with its C# and C++/CX counterparts: function bindings, event
        // bindings and two-way bindings.
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
