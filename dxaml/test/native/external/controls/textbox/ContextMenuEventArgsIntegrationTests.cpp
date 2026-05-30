// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ContextMenuEventArgsIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ContextMenuEventArgs {

    bool ContextMenuEventArgsIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ContextMenuEventArgsIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ContextMenuEventArgsIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Validates that we can successfully fire a ContextMenuEventArgs.
    //
    void ContextMenuEventArgsIntegrationTests::CanTextBoxFireContextMenuEvent()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::TextBox^ textBox = nullptr;
        xaml_controls::Button^ button1 = nullptr;
        xaml_controls::Button^ button2 = nullptr;
        auto gotFocusEventOnTextBox = std::make_shared<Event>();
        auto gotFocusEventOnButton = std::make_shared<Event>();
        auto contextMenuOpeningEvent = std::make_shared<Event>();
        auto gotFocusOnTextBoxRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);
        auto gotFocusOnButtonRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
        auto contextMenuOpeningRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, ContextMenuOpening);
        Platform::String^ contextMenuKeySequence = "$d$_apps#$u$_apps";

        RunOnUIThread([&]()
        {
            auto rootStackPanel = ref new xaml_controls::StackPanel();

            button1 = ref new xaml_controls::Button();
            button1->Content = "DummyButton1";

            button2 = ref new xaml_controls::Button();
            button2->Content = "DummyButton2";

            textBox = ref new xaml_controls::TextBox();
            VERIFY_IS_NOT_NULL(textBox);
            textBox->Width = 300;
            textBox->Height = 200;

            rootStackPanel->Children->Append(button1);
            rootStackPanel->Children->Append(textBox);
            rootStackPanel->Children->Append(button2);

            gotFocusOnTextBoxRegistration.Attach(
                textBox,
                ref new xaml::RoutedEventHandler(
                [gotFocusEventOnTextBox](Platform::Object^ sender, xaml::IRoutedEventArgs^)
            {
                auto senderAsTextBox = dynamic_cast<xaml_controls::TextBox^>(sender);
                LOG_OUTPUT(L"CanTextBoxFireContextMenuEvent: GotFocus event is fired on TextBox.");
                senderAsTextBox->Text = L"GotFocus on the TextBox!";
                gotFocusEventOnTextBox->Set();
            }));

            gotFocusOnButtonRegistration.Attach(
                button2,
                ref new xaml::RoutedEventHandler(
                [gotFocusEventOnButton](Platform::Object^ sender, xaml::IRoutedEventArgs^)
            {
                gotFocusEventOnButton->Set();
            }));

            contextMenuOpeningRegistration.Attach(
                textBox,
                ref new xaml_controls::ContextMenuOpeningEventHandler(
                    [contextMenuOpeningEvent](Platform::Object^ sender, xaml_controls::ContextMenuEventArgs^ args)
            {
                auto senderAsTextBox = dynamic_cast<xaml_controls::TextBox^>(sender);
                LOG_OUTPUT(L"CanTextBoxFireContextMenuEvent: ContextMenuOpening event is fired. CursorLeft=%f CursorLeft=%f", args->CursorLeft, args->CursorTop);
                contextMenuOpeningEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = rootStackPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            textBox->Focus(xaml::FocusState::Keyboard);
        });

        TestServices::WindowHelper->WaitForIdle();
        gotFocusEventOnTextBox->WaitForDefault();

        LOG_OUTPUT(L"CanTextBoxFireContextMenuEvent: Execute the opening ContextMenu on TextBox.");
        TestServices::KeyboardHelper->PressKeySequence(contextMenuKeySequence);

        contextMenuOpeningEvent->WaitForDefault();

        LOG_OUTPUT(L"CanTextBoxFireContextMenuEvent: Execute the closing ContextMenu on TextBox.");
        TestServices::KeyboardHelper->Escape();

        // Ensure the context menu closing by click the out of context menu window.
        LOG_OUTPUT(L"CanTextBoxFireContextMenuEvent: Tap on the left top corner of window to ensure the closing context menu.");
        TestServices::InputHelper->Tap(wf::Point(0, 0));

        // Move the focus to the Button2 that ensuring the close soft keyboard.
        LOG_OUTPUT(L"CanTextBoxFireContextMenuEvent: Tap on button2.");
        TestServices::InputHelper->Tap(button2);

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"CanTextBoxFireContextMenuEvent: Done!");
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::ContextMenuEventArgs
