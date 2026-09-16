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

        ref class RebindMetadataProvider sealed : public Markup::IXamlMetadataProvider
        {
        public:
            RebindMetadataProvider()
            {
                _provider = ref new XamlTypeInfo::XamlControlsXamlMetaDataProvider();
            }

            virtual Markup::IXamlType^ GetXamlType(::Windows::UI::Xaml::Interop::TypeName type)
            {
                ++LookupCount;
                return _provider->GetXamlType(type);
            }

            virtual Markup::IXamlType^ GetXamlType(Platform::String^ fullName)
            {
                ++LookupCount;
                return _provider->GetXamlType(fullName);
            }

            virtual Platform::Array<Markup::XmlnsDefinition>^ GetXmlnsDefinitions()
            {
                return _provider->GetXmlnsDefinitions();
            }

        internal:
            unsigned int LookupCount = 0;

        private:
            Markup::IXamlMetadataProvider^ _provider;
        };

        ref class RebindMetadataRegistrar sealed : public ICustomMetadataRegistrar
        {
        public:
            virtual void RegisterMetadata()
            {
                RunOnUIThread([&]() {
                    RegisteredThreadId = GetCurrentThreadId();
                    Property = DependencyProperty::RegisterAttached(
                        L"WpfHostRebindValue", int::typeid, TextBox::typeid,
                        ref new PropertyMetadata(static_cast<Platform::Object^>(0)));
                });
            }

            virtual void Dispose()
            {
                if (!_isClosed)
                {
                    RunOnUIThread([&]() {
                        ClosedThreadId = GetCurrentThreadId();
                        Property = nullptr;
                    });
                    ++CloseCount;
                    _isClosed = true;
                }
            }

            virtual ~RebindMetadataRegistrar()
            {
                Dispose();
            }

        internal:
            DependencyProperty^ Property;
            DWORD RegisteredThreadId = 0;
            DWORD ClosedThreadId = 0;
            unsigned int CloseCount = 0;

        private:
            bool _isClosed = false;
        };

        static std::weak_ptr<int> TrackWpfCallbackReferences(WindowHelper^ helper)
        {
            auto lifetime = std::make_shared<int>(0);
            helper->SetPostTickCallback(ref new PostTickCallback([lifetime]() { ++*lifetime; }));
            helper->SetPlayingSoundNodeCallback(ref new PlayingSoundNodeCallback(
                [lifetime](ElementSoundKind, bool, float, float, float, double) { ++*lifetime; }));
            helper->SetGCCollectCallback(ref new GCCollectCallback([lifetime]() { ++*lifetime; }));
            return lifetime;
        }

        static void VerifyReboundHostIsUsable(WindowHelper^ helper, DependencyProperty^ property = nullptr)
        {
            TextBox^ textBox = nullptr;
            auto cleanup = wil::scope_exit([&]() {
                RunOnUIThread([&]() { textBox = nullptr; });
                helper->ResetWindowContentAndWaitForIdle();
            });

            RunOnUIThread([&]() {
                textBox = ref new TextBox();
                Automation::AutomationProperties::SetAutomationId(textBox, L"WpfRebindTextBox");
                textBox->Width = 200;
                textBox->Height = 40;
                helper->WindowContent = textBox;
                if (property)
                {
                    textBox->SetValue(property, 42);
                    VERIFY_ARE_EQUAL(42, safe_cast<int>(textBox->GetValue(property)));
                }
                VERIFY_IS_TRUE(helper->CurrentDispatcher->HasThreadAccess);
            });
            helper->WaitForIdle();
            RunOnUIThread([&]() {
                VERIFY_IS_TRUE(helper->WindowContent == textBox);
                VERIFY_IS_TRUE(textBox->IsLoaded);
                VERIFY_IS_GREATER_THAN(textBox->ActualWidth, 0.0);
            });

            // Exercise both the rebound window and the replacement keyboard's thread-bound event.
            TestServices::InputHelper->Tap(textBox);
            TestServices::KeyboardHelper->PressKeySequence(L"a");
            helper->WaitForIdle();
            RunOnUIThread([&]() {
                VERIFY_ARE_EQUAL(Platform::StringReference(L"a"), textBox->Text);
            });
        }

        bool WpfWindowHelperTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool WpfWindowHelperTests::TestSetup()
        {
            TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool WpfWindowHelperTests::TestCleanup()
        {
            auto closeManager = wil::scope_exit([&]() {
                if (_xamlManager)
                {
                    RunOnUIThread([&]() {
                        delete _xamlManager;
                        _xamlManager = nullptr;
                    });
                }
            });
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void WpfWindowHelperTests::CachedHelperSurvivesConsecutiveIntervals()
        {
            auto helper = TestServices::WindowHelper;
            for (int interval = 0; interval < 2; ++interval)
            {
                auto dispatcher = helper->CurrentDispatcher;
                auto callbacks = TrackWpfCallbackReferences(helper);
                VerifyReboundHostIsUsable(helper);
                helper->ShutdownXaml();

                const auto allocationCount = helper->GetAllocationCount();
                helper->ShutdownXaml();
                helper->ResetWindowContentAndWaitForIdle();
                helper->ResetWindowContentAndScaleWaitForIdle(1.0f);
                VERIFY_ARE_EQUAL(allocationCount, helper->GetAllocationCount());
                VERIFY_IS_FALSE(callbacks.expired());
                VERIFY_IS_TRUE(dispatcher == helper->CurrentDispatcher);

                helper->VerifyTestCleanup();
                helper->VerifyTestCleanup(); // A pending leak check is consumed only once.
                VERIFY_IS_FALSE(callbacks.expired());
                helper->InitializeXaml();
                VERIFY_IS_TRUE(helper == TestServices::WindowHelper);
                VERIFY_IS_TRUE(dispatcher != helper->CurrentDispatcher);
                VERIFY_IS_TRUE(callbacks.expired());
            }

            auto activeDispatcher = helper->CurrentDispatcher;
            helper->InitializeXaml();
            VERIFY_IS_TRUE(activeDispatcher == helper->CurrentDispatcher);
            VerifyReboundHostIsUsable(helper);
        }

        void WpfWindowHelperTests::MetadataOverloadsUseReboundHost()
        {
            bool useRegistrar = false;
            VERIFY_SUCCEEDED(WEX::TestExecution::TestData::TryGetValue(L"UseRegistrar", useRegistrar));

            auto helper = TestServices::WindowHelper;
            auto dispatcher = helper->CurrentDispatcher;
            helper->ShutdownXaml();
            helper->VerifyTestCleanup();

            auto provider = ref new RebindMetadataProvider();
            auto registrar = ref new RebindMetadataRegistrar();
            if (useRegistrar)
            {
                helper->InitializeXaml(provider, registrar);
            }
            else
            {
                helper->InitializeXaml(provider);
            }
            VERIFY_IS_TRUE(helper == TestServices::WindowHelper);
            VERIFY_IS_TRUE(dispatcher != helper->CurrentDispatcher);
            VERIFY_IS_GREATER_THAN(provider->LookupCount, 0u);
            if (useRegistrar)
            {
                RunOnUIThread([&]() {
                    VERIFY_ARE_EQUAL(GetCurrentThreadId(), registrar->RegisteredThreadId);
                });
            }
            VerifyReboundHostIsUsable(helper, registrar->Property);
            helper->ShutdownXaml();
            if (useRegistrar)
            {
                VERIFY_ARE_EQUAL(1u, registrar->CloseCount);
                VERIFY_ARE_EQUAL(registrar->RegisteredThreadId, registrar->ClosedThreadId);
                VERIFY_IS_NULL(registrar->Property);
            }
            helper->VerifyTestCleanup();
        }

        void WpfWindowHelperTests::DirectHostInitializationRetainsHelper()
        {
            WEX::Common::String initialization;
            VERIFY_SUCCEEDED(WEX::TestExecution::TestData::TryGetValue(L"HostInitialization", initialization));

            auto helper = TestServices::WindowHelper;
            auto dispatcher = helper->CurrentDispatcher;
            auto registrar = ref new RebindMetadataRegistrar();
            helper->InitializeXaml(ref new RebindMetadataProvider(), registrar);
            auto callbacks = TrackWpfCallbackReferences(helper);
            VerifyReboundHostIsUsable(helper, registrar->Property);

            if (initialization == L"Default")
            {
                TestServices::InitializeHost();
            }
            else if (initialization == L"Dpi")
            {
                TestServices::InitializeHost(true);
            }
            else
            {
                TestServices::InitializeHost(true, false);
                RunOnUIThread([&]() {
                    VERIFY_IS_NULL(xaml::Hosting::WindowsXamlManager::GetForCurrentThread());
                    _xamlManager = xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread();
                });
            }
            VERIFY_IS_TRUE(helper == TestServices::WindowHelper);
            VERIFY_IS_TRUE(dispatcher != helper->CurrentDispatcher);
            VERIFY_IS_TRUE(callbacks.expired());
            VERIFY_ARE_EQUAL(1u, registrar->CloseCount);
            VERIFY_ARE_EQUAL(registrar->RegisteredThreadId, registrar->ClosedThreadId);
            VERIFY_IS_NULL(registrar->Property);

            helper->InitializeXaml();
            VerifyReboundHostIsUsable(helper);
        }

    }

} } } }
