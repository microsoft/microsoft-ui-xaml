#include "pch.h"
#include "TestAutomationHelpersPanel.xaml.h"
#if __has_include("TestAutomationHelpersPanel.g.cpp")
#include "TestAutomationHelpersPanel.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::AppTestAutomationHelpers::implementation
{
    TestAutomationHelpersPanel::TestAutomationHelpersPanel()
    {
        InitializeComponent();
        m_loadedRevoker = Loaded(winrt::auto_revoke, { this, &TestAutomationHelpersPanel::OnLoaded });

        auto app = winrt::Windows::UI::Xaml::Application::Current();
        m_unhandledExceptionRevoker = app.UnhandledException(winrt::auto_revoke, { this, &TestAutomationHelpersPanel::OnUnhandledException });
    }

    void TestAutomationHelpersPanel::OnUnhandledException(Windows::Foundation::IInspectable const& /*sender*/, Windows::UI::Xaml::UnhandledExceptionEventArgs const& args)
    {
        auto errorMessage = std::wstring(L"Application Unhandled Exception. HRESULT: ") + std::to_wstring(args.Exception().value);
        UnhandledExceptionReportingTextBox().Text(errorMessage);
    }

    void TestAutomationHelpersPanel::OnLoaded(Windows::Foundation::IInspectable const& /*sender*/, Windows::UI::Xaml::RoutedEventArgs const& /*args*/)
    {
        TestContentLoadedCheckBox().IsChecked(true);
    }

    void TestAutomationHelpersPanel::CloseAppInvokerButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        auto app = winrt::Windows::UI::Xaml::Application::Current();
        app.Exit();
    }

    void TestAutomationHelpersPanel::WaitForIdleInvokerButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        IdleStateEnteredCheckBox().IsChecked(false);

        auto spThis = get_strong();
        auto dispatcherQueue = winrt::Windows::System::DispatcherQueue::GetForCurrentThread();
        
        auto workItem = winrt::Windows::System::Threading::WorkItemHandler([spThis, dispatcherQueue](winrt::Windows::Foundation::IAsyncAction workItem) mutable
            {
                winrt::AppTestAutomationHelpers::IdleSynchronizer idle(dispatcherQueue);
                winrt::hstring errorString = idle.TryWait();

                dispatcherQueue.TryEnqueue(
                    winrt::Windows::System::DispatcherQueuePriority::Low,
                    winrt::Windows::System::DispatcherQueueHandler([spThis, errorString]()
                        {
                            if (errorString.empty())
                            {
                                spThis->IdleStateEnteredCheckBox().IsChecked(true);
                            }
                            else
                            {
                                // Setting Text will raise a property-changed event, so even if we
                                // immediately set it back to the empty string, we'll still get the
                                // error-reported event that we can detect and handle.
                                spThis->ErrorReportingTextBox().Text(errorString);
                                spThis->ErrorReportingTextBox().Text(L"");
                            }                            
                        }));
            });
        
        auto asyncAction = Windows::System::Threading::ThreadPool::RunAsync(workItem);
    }

    void TestAutomationHelpersPanel::WaitForDebuggerInvokerButton_Click(Windows::Foundation::IInspectable const& /*sender*/, Windows::UI::Xaml::RoutedEventArgs const& /*args*/)
    {
        DebuggerAttachedCheckBox().IsChecked(false);

        auto spThis = get_strong();
        auto dispatcherQueue = winrt::Windows::System::DispatcherQueue::GetForCurrentThread();

        auto workItem = winrt::Windows::System::Threading::WorkItemHandler([spThis, dispatcherQueue](winrt::Windows::Foundation::IAsyncAction workItem) mutable
            {
                while (!IsDebuggerPresent())
                {
                    Sleep(1000);
                }

                DebugBreak();

                dispatcherQueue.TryEnqueue(
                    winrt::Windows::System::DispatcherQueuePriority::Low,
                    winrt::Windows::System::DispatcherQueueHandler([spThis]()
                        {
                            spThis->DebuggerAttachedCheckBox().IsChecked(true);
                        }));
            });

        auto asyncAction = Windows::System::Threading::ThreadPool::RunAsync(workItem);
    }
}
