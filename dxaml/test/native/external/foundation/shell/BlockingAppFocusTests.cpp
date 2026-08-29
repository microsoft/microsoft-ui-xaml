// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <ppltasks.h>
#include "FileLoader.h"
#include "BlockingAppFocusTests.h"

using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Input;

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Shell {

        bool BlockingAppFocusTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool BlockingAppFocusTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool BlockingAppFocusTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        Platform::String^ BlockingAppFocusTests::GetPathToFiles() const
        {
            return GetPackageFolder() + L"resources\\native\\foundation\\shell\\";
        }

        void BlockingAppFocusTests::CheckTabWraparound()
        {
            TestCleanupWrapper cleanup([]()
            {
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"BlockingAppTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            auto listViewFocusRegistration = CreateSafeEventRegistration(ListView, GotFocus);
            auto buttonFocusRegistration = CreateSafeEventRegistration(Button, GotFocus);
            std::shared_ptr<Event> listViewGotFocusEvent = std::make_shared<Event>();
            std::shared_ptr<Event> buttonGotFocusEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;

                auto lstbegin = safe_cast<ListView^>(rootStackPanel->FindName(L"lstBegin"));

                listViewFocusRegistration.Attach(
                    lstbegin,
                    ref new xaml::RoutedEventHandler([listViewGotFocusEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"ListView GotFocus Event Fired");
                    listViewGotFocusEvent->Set();
                }));

                auto btnEnd = safe_cast<Button^>(rootStackPanel->FindName(L"btnEnd"));

                buttonFocusRegistration.Attach(
                    btnEnd,
                    ref new xaml::RoutedEventHandler([buttonGotFocusEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Button GotFocus Event Fired");
                    buttonGotFocusEvent->Set();
                }));
            });
            TestServices::WindowHelper->WaitForIdle();

            // Ensure the list view has the focus
            RunOnUIThread([&]()
            {
                auto lstBegin = safe_cast<ListView^>(rootStackPanel->FindName(L"lstBegin"));

                lstBegin->Focus(FocusState::Programmatic);
            });
            TestServices::WindowHelper->WaitForIdle();
            listViewGotFocusEvent->WaitForDefault();
            listViewGotFocusEvent->Reset();

            // Tab to the button
            TestServices::KeyboardHelper->Tab();
            buttonGotFocusEvent->WaitForDefault();

            // Tab back to the list view
            TestServices::KeyboardHelper->Tab();
            listViewGotFocusEvent->WaitForDefault();
        }

        void BlockingAppFocusTests::CheckShiftTabWraparound()
        {
            TestCleanupWrapper cleanup([]()
            {
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"BlockingAppTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            auto listViewFocusRegistration = CreateSafeEventRegistration(ListView, GotFocus);
            auto buttonFocusRegistration = CreateSafeEventRegistration(Button, GotFocus);
            std::shared_ptr<Event> listViewGotFocusEvent = std::make_shared<Event>();
            std::shared_ptr<Event> buttonGotFocusEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;

                auto lstbegin = safe_cast<ListView^>(rootStackPanel->FindName(L"lstBegin"));

                listViewFocusRegistration.Attach(
                    lstbegin,
                    ref new xaml::RoutedEventHandler([listViewGotFocusEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"ListView GotFocus Event Fired");
                    listViewGotFocusEvent->Set();
                }));

                auto btnEnd = safe_cast<Button^>(rootStackPanel->FindName(L"btnEnd"));

                buttonFocusRegistration.Attach(
                    btnEnd,
                    ref new xaml::RoutedEventHandler([buttonGotFocusEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Button GotFocus Event Fired");
                    buttonGotFocusEvent->Set();
                }));
            });
            TestServices::WindowHelper->WaitForIdle();

            // Ensure the list view has the focus
            RunOnUIThread([&]()
            {
                auto lstBegin = safe_cast<ListView^>(rootStackPanel->FindName(L"lstBegin"));

                lstBegin->Focus(FocusState::Programmatic);
            });
            TestServices::WindowHelper->WaitForIdle();
            listViewGotFocusEvent->WaitForDefault();
            listViewGotFocusEvent->Reset();

            // Shift-tab from the listview to the button
            TestServices::KeyboardHelper->ShiftTab();
            buttonGotFocusEvent->WaitForDefault();
        }

    } }
} } } }
