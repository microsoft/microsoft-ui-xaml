// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "AccessKeyIntegrationTests.h"

#include <XamlTailored.h>
#include <TestCleanupWrapper.h>

#include "AccessKeyTestHelper.h"


using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace AccessKeys {

    //
    // Test Cases
    //

    void AccessKeyIntegrationTests::ButtonBasicIntegrationTest()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Button^ button = AccessKeyTestHelper::CreateControl<xaml_controls::Button>();

        std::shared_ptr<Event> buttonClickEvent = std::make_shared<Event>();
        auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);
        RunOnUIThread([&]()
        {
            button->Content = "Button";
            button->ExitDisplayModeOnAccessKeyInvoked = false;
            clickRegistration.Attach(button,
                ref new RoutedEventHandler([&](Platform::Object^, RoutedEventArgs^)
            {
                LOG_OUTPUT(L"Btn click event fired!");
                buttonClickEvent->Set();
            }));
        });
        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        AccessKeyTestHelper::BasicAccessKeyTest(AccessKeyTestHelper::SubTestEnum::SubTest_TestAll, button, buttonClickEvent);
    }

    void AccessKeyIntegrationTests::RadioButtonBasicIntegrationTest()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::RadioButton^ button = AccessKeyTestHelper::CreateControl<xaml_controls::RadioButton>();

        std::shared_ptr<Event> checkedEvent = std::make_shared<Event>();
        auto checkRegistration = CreateSafeEventRegistration(xaml_controls::RadioButton, Checked);
        RunOnUIThread([&]()
        {
            button->Content = "RadioButton";
            button->ExitDisplayModeOnAccessKeyInvoked = false;
            checkRegistration.Attach(button,
                ref new RoutedEventHandler([&](Platform::Object^, RoutedEventArgs^)
            {
                LOG_OUTPUT(L"Btn Checked event fired!");
                checkedEvent->Set();
            }));
        });
        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        // for radio button, the invocation alternates through checked and unchecked so not all sub-test can run
        // also need to reset the IsChecked for each subtest
        for (int testEnum = static_cast<int>(AccessKeyTestHelper::SubTestEnum::SubTest_TestAll)+1; testEnum < static_cast<int>(AccessKeyTestHelper::SubTestEnum::SubTest_End); testEnum++)
        {
            if (testEnum != static_cast<int>(AccessKeyTestHelper::SubTestEnum::SubTest_DuplicateAccessKeyPressed))
            {
                AccessKeyTestHelper::BasicAccessKeyTest(static_cast<AccessKeyTestHelper::SubTestEnum>(testEnum), button, checkedEvent);
                RunOnUIThread([&]()
                {
                    button->IsChecked = false;
                });
                TestServices::WindowHelper->WaitForIdle();
            }
        }
    }

    void AccessKeyIntegrationTests::RepeatButtonBasicIntegrationTest()
    {
        TestCleanupWrapper cleanup;
        xaml_primitives::RepeatButton^ button = AccessKeyTestHelper::CreateControl<xaml_primitives::RepeatButton>();

        std::shared_ptr<Event> buttonClickEvent = std::make_shared<Event>();
        auto clickRegistration = CreateSafeEventRegistration(xaml_primitives::RepeatButton, Click);
        RunOnUIThread([&]()
        {
            button->Content = "RepeatButton";
            button->ExitDisplayModeOnAccessKeyInvoked = false;
            clickRegistration.Attach(button,
                ref new RoutedEventHandler([&](Platform::Object^, RoutedEventArgs^)
            {
                LOG_OUTPUT(L"Btn click event fired!");
                buttonClickEvent->Set();
            }));
        });
        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        AccessKeyTestHelper::BasicAccessKeyTest(AccessKeyTestHelper::SubTestEnum::SubTest_TestAll, button, buttonClickEvent);
    }

    void AccessKeyIntegrationTests::HyperlinkButtonBasicIntegrationTest()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::HyperlinkButton^ button = AccessKeyTestHelper::CreateControl<xaml_controls::HyperlinkButton>();

        std::shared_ptr<Event> buttonClickEvent = std::make_shared<Event>();
        auto clickRegistration = CreateSafeEventRegistration(xaml_controls::HyperlinkButton, Click);
        RunOnUIThread([&]()
        {
            button->Content = "HyperlinkButton";
            button->ExitDisplayModeOnAccessKeyInvoked = false;
            clickRegistration.Attach(button,
                ref new RoutedEventHandler([&](Platform::Object^, RoutedEventArgs^)
            {
                LOG_OUTPUT(L"Btn click event fired!");
                buttonClickEvent->Set();
            }));
        });
        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        AccessKeyTestHelper::BasicAccessKeyTest(AccessKeyTestHelper::SubTestEnum::SubTest_TestAll, button, buttonClickEvent);
    }

    void AccessKeyIntegrationTests::CheckBoxBasicIntegrationTest()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::CheckBox^ button = AccessKeyTestHelper::CreateControl<xaml_controls::CheckBox>();

        std::shared_ptr<Event> checkedEvent = std::make_shared<Event>();
        auto checkRegistration = CreateSafeEventRegistration(xaml_controls::CheckBox, Checked);
        RunOnUIThread([&]()
        {
            button->Content = "CheckBox";
            button->ExitDisplayModeOnAccessKeyInvoked = false;
            checkRegistration.Attach(button,
                ref new RoutedEventHandler([&](Platform::Object^, RoutedEventArgs^)
            {
                LOG_OUTPUT(L"CheckBox Checked event fired!");
                checkedEvent->Set();
            }));
        });
        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        // for CheckBox, the invocation alternates through checked and unchecked so not all sub-test can run
        // also need to reset the IsChecked for each subtest
        for (int testEnum = static_cast<int>(AccessKeyTestHelper::SubTestEnum::SubTest_TestAll) + 1; testEnum < static_cast<int>(AccessKeyTestHelper::SubTestEnum::SubTest_End); testEnum++)
        {
            if (testEnum != static_cast<int>(AccessKeyTestHelper::SubTestEnum::SubTest_DuplicateAccessKeyPressed))
            {
                AccessKeyTestHelper::BasicAccessKeyTest(static_cast<AccessKeyTestHelper::SubTestEnum>(testEnum), button, checkedEvent);
                RunOnUIThread([&]()
                {
                    button->IsChecked = false;
                });
                TestServices::WindowHelper->WaitForIdle();
            }
        }
    }

    void AccessKeyIntegrationTests::InvokeAccessKeysOnMultipleButtons()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::StackPanel^ rootPanel = nullptr;

        std::vector<xaml_primitives::ButtonBase^> buttons;

        RunOnUIThread([&]()
        {
            rootPanel = ref new xaml_controls::StackPanel();
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto button = ref new xaml_controls::Button();
            button->Content = L"button";
            button->ExitDisplayModeOnAccessKeyInvoked = false;
            rootPanel->Children->Append(button);
            buttons.push_back(std::move(button));

            auto repeatButton = ref new xaml_primitives::RepeatButton();
            rootPanel->Children->Append(repeatButton);
            buttons.push_back(std::move(repeatButton));

            auto hyperlinkButton = ref new xaml_controls::HyperlinkButton();
            hyperlinkButton->Content = L"Hyperlink Button";
            rootPanel->Children->Append(hyperlinkButton);
            buttons.push_back(std::move(hyperlinkButton));

        });

        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();
        
        AccessKeyTestHelper::InvokeAccessKeysOnMultipleButtons(buttons);
    }

} } } } } 
