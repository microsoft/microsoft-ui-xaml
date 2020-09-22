#pragma once

#include "winrt/Windows.UI.Xaml.h"
#include "winrt/Windows.UI.Xaml.Markup.h"
#include "winrt/Windows.UI.Xaml.Controls.Primitives.h"
#include "TestAutomationHelpersPanel.g.h"

namespace winrt::AppTestAutomationHelpers::implementation
{
    struct TestAutomationHelpersPanel : TestAutomationHelpersPanelT<TestAutomationHelpersPanel>
    {
        TestAutomationHelpersPanel();

        void OnLoaded(Windows::Foundation::IInspectable const& /*sender*/, Windows::UI::Xaml::RoutedEventArgs const& /*args*/);

        void CloseAppInvokerButton_Click(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void WaitForIdleInvokerButton_Click(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        
        void WaitForDebuggerInvokerButton_Click(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);

        void OnUnhandledException(Windows::Foundation::IInspectable const& /*sender*/, Windows::UI::Xaml::UnhandledExceptionEventArgs const& args);

    private:
        winrt::Windows::UI::Xaml::FrameworkElement::Loaded_revoker m_loadedRevoker{};
        winrt::Windows::UI::Xaml::Application::UnhandledException_revoker m_unhandledExceptionRevoker{};
    };
}

namespace winrt::AppTestAutomationHelpers::factory_implementation
{
    struct TestAutomationHelpersPanel : TestAutomationHelpersPanelT<TestAutomationHelpersPanel, implementation::TestAutomationHelpersPanel>
    {
    };
}
