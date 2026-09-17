// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "InfrastructureIntegrationTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Test {

        bool InfrastructureIntegrationTests::ClassSetup()
        {
            // It's very important to call EnsureInitialized on TestServices
            // from ClassSetup. This method will wait for the window to be
            // activated on launch, which avoids a race condition that will block
            // input from being routed to the app. It will also wait for the
            // debugger to attach when the waitForDebugger runtime parameter is
            // specified.
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool InfrastructureIntegrationTests::TestCleanup()
        {
            // It's very important to have your test clean up the window contents
            // when it completes. When creating new tests be sure to copy this
            // method over or implement it in a similar way. By cleaning
            // up the window content and waiting for the page to go idle you ensure
            // that if your test fails while the UI element tree is being torn down
            // that the failure is associated with your test and doesn't occur
            // nondeterministically in the future. By waiting for the page to go
            // idle you ensure that all transitions have completed and that jupiter
            // is in a 'tabula rasa' state for the next test.
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void InfrastructureIntegrationTests::ValidateWindowContentAccessor()
        {
            TestCleanupWrapper cleanup;
            RunOnUIThread([&] () {
                Grid^ testGrid = ref new Grid();

                TestServices::WindowHelper->WindowContent = testGrid;

                VERIFY_ARE_EQUAL(TestServices::WindowHelper->WindowContent->GetHashCode(), testGrid->GetHashCode());
                VERIFY_ARE_EQUAL(Window::Current->Content->GetHashCode(), testGrid->GetHashCode());
            });
        }

        void InfrastructureIntegrationTests::ValidateWaitForIdle()
        {
            TestCleanupWrapper cleanup;
            Grid^ testGrid;
            RunOnUIThread([&] () {
                testGrid = ref new Grid();
                TestServices::WindowHelper->WindowContent = testGrid;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&] () {
                VERIFY_IS_GREATER_THAN(testGrid->ActualHeight, 0.0);
            });
        }

        void InfrastructureIntegrationTests::ValidateSetupSimulatedAppPage()
        {
            TestCleanupWrapper cleanup;
            RunOnUIThread([&] () {
                Page^ returnedPage = TestServices::WindowHelper->SetupSimulatedAppPage();
                Frame^ appFrame = dynamic_cast<Frame^>(TestServices::WindowHelper->WindowContent);
                VERIFY_IS_NOT_NULL(appFrame);
                Page^ appPage = dynamic_cast<Page^>(appFrame->Content);
                VERIFY_ARE_EQUAL(returnedPage->GetHashCode(), appPage->GetHashCode());
            });
        }

        void InfrastructureIntegrationTests::ValidateTap()
        {
            TestCleanupWrapper cleanup;
            std::shared_ptr<Event> buttonClickEvent = std::make_shared<Event>();
            Button^ button = nullptr;
            wf::EventRegistrationToken buttonClickToken = {};

            RunOnUIThread([&] () {
                Grid^ buttonGrid = ref new Grid();

                button = ref new Button();
                button->Content = L"Hello world.";
                button->VerticalAlignment = VerticalAlignment::Stretch;
                button->HorizontalAlignment = HorizontalAlignment::Stretch;
                buttonClickToken = button->Click +=
                    ref new RoutedEventHandler([buttonClickEvent] (Platform::Object^, RoutedEventArgs^) {
                        buttonClickEvent->Set();
                    });
                buttonGrid->Children->Append(button);

                TestServices::WindowHelper->WindowContent = buttonGrid;
            });

            // If we don't call WaitForIdle here there's no promise that the Button will
            // have rendered to the screen by the time we're ready to simulate input.
            TestServices::WindowHelper->WaitForIdle();
            TestServices::InputHelper->Tap(button);
            buttonClickEvent->WaitForDefault();

            RunOnUIThread([&] () {
                button->Click -= buttonClickToken;
            });
        }

        void InfrastructureIntegrationTests::ValidateFlick()
        {
            TestCleanupWrapper cleanup;
            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
            ScrollViewer^ sv = nullptr;
            wf::EventRegistrationToken scrollViewerViewChangingToken = {};
            wf::EventRegistrationToken scrollViewerViewChangedToken = {};

            RunOnUIThread([&] () {
                Grid^ mainGrid = ref new Grid();

                sv = ref new ScrollViewer();
                sv->Width = 100;
                sv->Height = 100;
                sv->VerticalAlignment = VerticalAlignment::Center;
                sv->HorizontalAlignment = HorizontalAlignment::Center;
                mainGrid->Children->Append(sv);

                StackPanel^ svChild = ref new StackPanel();
                sv->Content = svChild;

                for (int i = 0; i < 10; i++)
                {
                    Microsoft::UI::Xaml::Shapes::Rectangle^ rect =
                        ref new Microsoft::UI::Xaml::Shapes::Rectangle();
                    if (i % 2 == 0)
                    {
                        rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                    }
                    else
                    {
                        rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                    }
                    rect->Width=100;
                    rect->Height=100;
                    svChild->Children->Append(rect);
                }

                scrollViewerViewChangedToken = sv->ViewChanged +=
                    ref new wf::EventHandler<ScrollViewerViewChangedEventArgs^>(
                        [viewChangedEvent] (Platform::Object^, ScrollViewerViewChangedEventArgs^ args) {
                            if (args->IsIntermediate == false)
                            {
                                viewChangedEvent->Set();
                            }
                        });

                scrollViewerViewChangingToken = sv->ViewChanging +=
                    ref new wf::EventHandler<ScrollViewerViewChangingEventArgs^>(
                        [] (Platform::Object^, ScrollViewerViewChangingEventArgs^ args) {
                            LOG_OUTPUT(L"ViewChanging, IsInertial: %d", args->IsInertial);
                        });
                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::InputHelper->Flick(sv, FlickDirection::North);
            viewChangedEvent->WaitForDefault();

            RunOnUIThread([&] () {
                sv->ViewChanging -= scrollViewerViewChangingToken;
                sv->ViewChanged -= scrollViewerViewChangedToken;
            });
        }

        void InfrastructureIntegrationTests::ValidateTab()
        {
            TestCleanupWrapper cleanup;
            std::shared_ptr<Event> gotFocusEvent = std::make_shared<Event>();
            std::shared_ptr<Event> buttonClickEvent = std::make_shared<Event>();
            Button^ buttonToTabTo = nullptr;
            Button^ buttonToTap = nullptr;
            wf::EventRegistrationToken buttonToTabToGotFocusToken = {};
            wf::EventRegistrationToken buttonToTapClickToken = {};

            RunOnUIThread([&] () {
                Grid^ mainGrid = ref new Grid();

                StackPanel^ spChild = ref new StackPanel();
                mainGrid->Children->Append(spChild);

                buttonToTap = ref new Button();
                buttonToTap->Content = L"Test Button 1";
                spChild->Children->Append(buttonToTap);

                buttonToTapClickToken = buttonToTap->Click +=
                    ref new RoutedEventHandler([buttonClickEvent] (Platform::Object^, RoutedEventArgs^) {
                        buttonClickEvent->Set();
                    });

                buttonToTabTo = ref new Button();
                buttonToTabTo->Content = L"Test Button 2";
                spChild->Children->Append(buttonToTabTo);

                buttonToTabToGotFocusToken = buttonToTabTo->GotFocus +=
                    ref new xaml::RoutedEventHandler(
                        [gotFocusEvent] (Platform::Object^, xaml::IRoutedEventArgs^) {
                            gotFocusEvent->Set();
                        });
                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::InputHelper->Tap(buttonToTap);
            buttonClickEvent->WaitForDefault();
            TestServices::KeyboardHelper->Tab();
            gotFocusEvent->WaitForDefault();

            RunOnUIThread([&] () {
                buttonToTabTo->GotFocus -= buttonToTabToGotFocusToken;
                buttonToTap->Click -= buttonToTapClickToken;
            });
        }

        void InfrastructureIntegrationTests::ValidatePressKeySequence()
        {
            TestCleanupWrapper cleanup;
            std::shared_ptr<Event> gotFocusEvent = std::make_shared<Event>();
            std::shared_ptr<Event> textChangedEvent = std::make_shared<Event>();

            wf::EventRegistrationToken tbGotFocusToken = {};
            wf::EventRegistrationToken tbTextChangedToken = {};
            Platform::String^ strToType = "Hello world";
            TextBox^ tb = nullptr;

            RunOnUIThread([&] () {
                Grid^ mainGrid = ref new Grid();

                tb = ref new TextBox();
                mainGrid->Children->Append(tb);

                tbGotFocusToken = tb->GotFocus +=
                    ref new xaml::RoutedEventHandler(
                        [gotFocusEvent] (Platform::Object^, xaml::IRoutedEventArgs^) {
                            gotFocusEvent->Set();
                        });

                tbTextChangedToken = tb->TextChanged +=
                    ref new xaml_controls::TextChangedEventHandler(
                        [textChangedEvent, &tb, &strToType] (Platform::Object^, xaml_controls::TextChangedEventArgs^) {
                            if (tb->Text->Length() == strToType->Length())
                            {
                                textChangedEvent->Set();
                            }
                        });

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::InputHelper->Tap(tb);
            gotFocusEvent->WaitForDefault();

            TestServices::KeyboardHelper->PressKeySequence(strToType);
            textChangedEvent->WaitForDefault();

            RunOnUIThread([&] () {
                tb->GotFocus -= tbGotFocusToken;
                tb->TextChanged -= tbTextChangedToken;
            });
        }

        void InfrastructureIntegrationTests::ValidateWpfShutdownLeakDetection()
        {
            // Match per-test setup before explicit host initialization. Otherwise the previous
            // shutdown's EnableCoreShutdown flag makes WindowsXamlManager defer closing its core.
            TestServices::WindowHelper->InitializeXaml();

            WEX::Common::String initialization;
            VERIFY_SUCCEEDED(WEX::TestExecution::TestData::TryGetValue(L"HostInitialization", initialization));
            bool isNegativeControl = false;
            WEX::TestExecution::RuntimeParameters::TryGetValue(L"WpfLeakNegativeControl", isNegativeControl);

            // Explicit host initialization, like WindowedPopupHighDPI, precedes test content.
            {
                auto helper = TestServices::WindowHelper;
                auto dispatcher = helper->CurrentDispatcher;
                if (initialization == L"Default")
                {
                    TestServices::InitializeHost();
                }
                else
                {
                    VERIFY_ARE_EQUAL(WEX::Common::String(L"Dpi"), initialization);
                    TestServices::InitializeHost(true);
                }
                VERIFY_IS_TRUE(helper != TestServices::WindowHelper);
                VERIFY_IS_TRUE(dispatcher != TestServices::WindowHelper->CurrentDispatcher);
            }

            TestServices::WindowHelper->InitializeXaml();
            auto shutdown = wil::scope_exit([]() {
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
                TestServices::WindowHelper->ShutdownXaml();
            });

            for (int interval = 0; interval < 2; ++interval)
            {
                auto helper = TestServices::WindowHelper;
                auto dispatcher = helper->CurrentDispatcher;
                DWORD retiringThreadId = 0;
                auto releaseThreadId = std::make_shared<DWORD>(0);
                auto lifetime = std::shared_ptr<int>(new int(0), [releaseThreadId](int* value) {
                    *releaseThreadId = GetCurrentThreadId();
                    delete value;
                });
                std::weak_ptr<int> callbacks = lifetime;
                RunOnUIThread([&]() {
                    retiringThreadId = GetCurrentThreadId();
                    SolidColorBrush^ retainedBrush = nullptr;
                    if (isNegativeControl && interval == 0)
                    {
                        LOG_OUTPUT(L"NEGATIVE CONTROL: retaining a native brush through the shutdown-time scan.");
                        retainedBrush = ref new SolidColorBrush();
                    }
                    helper->SetPostTickCallback(ref new PostTickCallback([lifetime, retainedBrush]() {
                        ++*lifetime;
                        if (retainedBrush) { (void)retainedBrush->Opacity; }
                    }));
                    helper->SetPlayingSoundNodeCallback(ref new PlayingSoundNodeCallback(
                        [lifetime](ElementSoundKind, bool, float, float, float, double) { ++*lifetime; }));
                    helper->SetGCCollectCallback(ref new GCCollectCallback([lifetime]() { ++*lifetime; }));
                });
                lifetime.reset();

                {
                    TextBox^ textBox = nullptr;
                    auto cleanup = wil::scope_exit([&]() {
                        RunOnUIThread([&]() { textBox = nullptr; });
                        helper->ResetWindowContentAndWaitForIdle();
                    });
                    RunOnUIThread([&]() {
                        textBox = ref new TextBox();
                        Automation::AutomationProperties::SetAutomationId(textBox, L"WpfShutdownTextBox");
                        textBox->Width = 200;
                        textBox->Height = 40;
                        helper->WindowContent = textBox;
                        VERIFY_IS_TRUE(helper->CurrentDispatcher->HasThreadAccess);
                    });
                    helper->WaitForIdle();
                    RunOnUIThread([&]() {
                        VERIFY_IS_TRUE(helper->WindowContent == textBox);
                        VERIFY_IS_TRUE(textBox->IsLoaded);
                        VERIFY_IS_GREATER_THAN(textBox->ActualWidth, 0.0);
                    });
                    // Input must work on the new window and its thread-bound keyboard event.
                    TestServices::InputHelper->Tap(textBox);
                    TestServices::KeyboardHelper->PressKeySequence(L"a");
                    helper->WaitForIdle();
                    RunOnUIThread([&]() {
                        VERIFY_ARE_EQUAL(Platform::StringReference(L"a"), textBox->Text);
                    });
                }

                LOG_OUTPUT(L"WPF interval %d: before ShutdownXaml.", interval);
                helper->ShutdownXaml();
                LOG_OUTPUT(L"WPF interval %d: after ShutdownXaml.", interval);

                auto replacement = TestServices::WindowHelper;
                auto replacementDispatcher = replacement->CurrentDispatcher;
                VERIFY_IS_TRUE(helper != replacement);
                VERIFY_IS_TRUE(dispatcher != replacementDispatcher);
                VERIFY_IS_TRUE(callbacks.expired());
                VERIFY_ARE_EQUAL(retiringThreadId, *releaseThreadId);

                LOG_OUTPUT(L"WPF interval %d: verifying cleanup twice; neither call may scan.", interval);
                replacement->VerifyTestCleanup();
                replacement->VerifyTestCleanup();
                LOG_OUTPUT(L"WPF interval %d: initializing the already-replaced host.", interval);
                if (interval == 0)
                {
                    replacement->InitializeXaml(ref new XamlTypeInfo::XamlControlsXamlMetaDataProvider());
                }
                else
                {
                    replacement->InitializeXaml();
                }
                VERIFY_IS_TRUE(replacement == TestServices::WindowHelper);
                VERIFY_IS_TRUE(replacementDispatcher == replacement->CurrentDispatcher);
            }
        }

    }

} } } }
