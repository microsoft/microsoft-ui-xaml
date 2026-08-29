#pragma once

#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Markup.h"
#include "winrt/Microsoft.UI.Xaml.Controls.Primitives.h"
#include "TestAutomationHelpersPanel.g.h"

namespace winrt::AppTestAutomationHelpers::implementation
{
    struct TestAutomationHelpersPanel : TestAutomationHelpersPanelT<TestAutomationHelpersPanel>
    {
        TestAutomationHelpersPanel();

        void OnLoaded(winrt::Windows::Foundation::IInspectable const& /*sender*/, Microsoft::UI::Xaml::RoutedEventArgs const& /*args*/);

        void CloseAppInvokerButton_Click(winrt::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void WaitForIdleInvokerButton_Click(winrt::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        
        void WaitForDebuggerInvokerButton_Click(winrt::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);

        void OnUnhandledException(winrt::Windows::Foundation::IInspectable const& /*sender*/, Microsoft::UI::Xaml::UnhandledExceptionEventArgs const& args);

    private:
        winrt::Microsoft::UI::Xaml::FrameworkElement::Loaded_revoker m_loadedRevoker{};
        winrt::Microsoft::UI::Xaml::Application::UnhandledException_revoker m_unhandledExceptionRevoker{};
    };
}

namespace winrt::AppTestAutomationHelpers::factory_implementation
{
    struct TestAutomationHelpersPanel : TestAutomationHelpersPanelT<TestAutomationHelpersPanel, implementation::TestAutomationHelpersPanel>
    {
    };
}
