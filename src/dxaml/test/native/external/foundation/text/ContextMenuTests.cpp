// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ContextMenuTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <ppltasks.h>
#include "FileLoader.h"
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <KeyboardInjectionOverride.h>
#include <FocusTestHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace Microsoft::UI::Xaml::Controls;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Text {

        bool ContextMenuTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool ContextMenuTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool ContextMenuTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        Platform::String^ ContextMenuTests::GetPathToFiles() const
        {
            return GetPackageFolder() + L"resources\\native\\foundation\\text\\";
        }

        //------------------------------------------------------------------------
        // Test case: Brings up a context menu for TextBox and RichEditBox controls.
        // Selects the 'Select All' command.
        //------------------------------------------------------------------------
        void ContextMenuTests::SelectAllWithContextMenu()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto rootLoadedEvent = std::make_shared<Event>();
            auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);

            xaml_controls::Button^ button = nullptr;
            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::RichEditBox^ richEditBox = nullptr;
            xaml_controls::StackPanel^ rootStackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"ContextMenuTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Listening to root FrameworkElement.Loaded.");
                rootLoadedRegistration.Attach(
                    rootStackPanel,
                    ref new xaml::RoutedEventHandler(
                    [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Root FE Loaded handler.");
                    rootLoadedEvent->Set();
                }));

                textBox = safe_cast<xaml_controls::TextBox^>(rootStackPanel->FindName("textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                richEditBox = safe_cast<xaml_controls::RichEditBox^>(rootStackPanel->FindName("richEditBox"));
                VERIFY_IS_NOT_NULL(richEditBox);
                button = safe_cast<xaml_controls::Button^>(rootStackPanel->FindName("button"));
                VERIFY_IS_NOT_NULL(button);

                TestServices::WindowHelper->WindowContent = rootStackPanel;
            });

            LOG_OUTPUT(L"Waiting for root FE Loaded event.");
            rootLoadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            FocusTestHelper::EnsureFocus(richEditBox, FocusState::Keyboard);
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Injecting James");
            TestServices::KeyboardHelper->PressKeySequence("James");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Clearing clipboard.");
                ::Windows::ApplicationModel::DataTransfer::Clipboard::Clear();
            });

            LOG_OUTPUT(L"Invoke the 'Select All' command with Apps Key.");
            OpenMenuAndInvokeCommand(m_keyboardSequenceApps);

            FocusTestHelper::EnsureFocus(textBox, FocusState::Keyboard);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Clearing clipboard.");
                ::Windows::ApplicationModel::DataTransfer::Clipboard::Clear();
            });

            LOG_OUTPUT(L"Invoke the 'Select All' command with Shift+F10 key.");
            OpenMenuAndInvokeCommand(m_keyboardSequenceShiftF10);

            FocusTestHelper::EnsureFocus(button, FocusState::Keyboard);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Prevent those text controls from getting focus again so input pane remains hidden.
                textBox->IsEnabled = false;
                richEditBox->IsEnabled = false;
                LOG_OUTPUT(L"Disabled both text controls.");
            });
        }

        void ContextMenuTests::TextBlockContextMenuOpeningEvent()
        {
            TestCleanupWrapper cleanup([]()
            {
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = nullptr;
            });
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\rendering\\" + "TextBlockEventTests.xaml"));

            TextBlock^ textBlock = nullptr;
            auto contextMenuOpeningEvent = std::make_shared<Event>();
            auto contextMenuOpeningRegistration = CreateSafeEventRegistration(xaml_controls::TextBlock, ContextMenuOpening);

            RunOnUIThread([&]()
            {
                textBlock = safe_cast<TextBlock^>(root->FindName(L"textBlock"));
                VERIFY_IS_NOT_NULL(textBlock);

                TestServices::WindowHelper->WindowContent = root;

                contextMenuOpeningRegistration.Attach(
                    textBlock,
                    ref new xaml_controls::ContextMenuOpeningEventHandler(
                    [contextMenuOpeningEvent](Platform::Object^, xaml_controls::ContextMenuEventArgs^)
                {
                    contextMenuOpeningEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(textBlock->Focus(FocusState::Programmatic));
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"First Tap TextBlock.");
                TestServices::InputHelper->Tap(textBlock);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                textBlock->SelectAll();
            });

            TestServices::WindowHelper->WaitForIdle();
            // Open the menu with Apps key and invoke a command. This step opens the menu, but also ensures that it is eventually closed.
            ContextMenuTests::OpenMenuAndInvokeCommand(m_keyboardSequenceApps);

            // Verify that the ContextMenuOpening event fired
            contextMenuOpeningEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
            VERIFY_IS_TRUE(contextMenuOpeningEvent->HasFired());
        }

        void ContextMenuTests::RichTextBlockContextMenuOpeningEvent()
        {
            TestCleanupWrapper cleanup([]()
            {
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = nullptr;
            });
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\rendering\\" + "TextBlockEventTests.xaml"));

            RichTextBlock^ richTextBlock = nullptr;
            auto contextMenuOpeningEvent = std::make_shared<Event>();
            auto contextMenuOpeningRegistration = CreateSafeEventRegistration(xaml_controls::RichTextBlock, ContextMenuOpening);

            RunOnUIThread([&]()
            {
                richTextBlock = safe_cast<RichTextBlock^>(root->FindName(L"richTextBlock"));
                VERIFY_IS_NOT_NULL(richTextBlock);

                TestServices::WindowHelper->WindowContent = root;

                contextMenuOpeningRegistration.Attach(
                    richTextBlock,
                    ref new xaml_controls::ContextMenuOpeningEventHandler(
                    [contextMenuOpeningEvent](Platform::Object^, xaml_controls::ContextMenuEventArgs^)
                {
                    contextMenuOpeningEvent->Set();
                }));
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"First Tap RichTextBlock.");
                TestServices::InputHelper->Tap(richTextBlock);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                richTextBlock->SelectAll();
            });

            TestServices::WindowHelper->WaitForIdle();
            // Open the menu with Shift+F10 key and invoke a command. This step opens the menu, but also ensures that it is eventually closed.
            ContextMenuTests::OpenMenuAndInvokeCommand(m_keyboardSequenceShiftF10);

            // Verify that the ContextMenuOpening event fired
            contextMenuOpeningEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
            VERIFY_IS_TRUE(contextMenuOpeningEvent->HasFired());
        }

        void ContextMenuTests::TextBoxContextMenuOpeningEvent()
        {
            TestCleanupWrapper cleanup([]()
            {
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = nullptr;
            });
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::Button^ button = nullptr;

            auto rootLoadedEvent = std::make_shared<Event>();
            auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);

            auto textSelectionChangedEvent = std::make_shared<Event>();
            auto textSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, SelectionChanged);

            auto contextMenuOpeningEvent = std::make_shared<Event>();
            auto contextMenuOpeningRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, ContextMenuOpening);

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <TextBox x:Name='textBox' Text= 'Test Test Test Test Test' FontSize='20' Width='180' Margin ='20,5,20,0' ContextFlyout='{x:Null}' SelectionFlyout='{x:Null}'/>"
                    L"</StackPanel>"));
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                button = safe_cast<xaml_controls::Button^>(rootPanel->FindName("button"));
                VERIFY_IS_NOT_NULL(button);
                
                rootLoadedRegistration.Attach(
                    rootPanel,
                    ref new xaml::RoutedEventHandler(
                        [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Root loaded handler.");
                    rootLoadedEvent->Set();
                }));

                textSelectionChangedRegistration.Attach(
                    textBox,
                    ref new xaml::RoutedEventHandler(
                        [textSelectionChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"TextBox selection changed.");
                    textSelectionChangedEvent->Set();
                }));

                contextMenuOpeningRegistration.Attach(
                    textBox,
                    ref new xaml_controls::ContextMenuOpeningEventHandler(
                        [contextMenuOpeningEvent](Platform::Object^, xaml_controls::ContextMenuEventArgs^)
                {
                    contextMenuOpeningEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            LOG_OUTPUT(L"Waiting for root loaded event.");
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
            rootLoadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            FocusTestHelper::EnsureFocus(textBox, FocusState::Keyboard);
            TestServices::WindowHelper->WaitForIdle();

            textSelectionChangedEvent->Reset();
            RunOnUIThread([&]()
            {
                textBox->SelectAll();
            });

            textSelectionChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Open the menu with Apps key and invoke a command.");
            // This step opens the menu, but also ensures that it is eventually closed.
            ContextMenuTests::OpenMenuAndInvokeCommand(m_keyboardSequenceApps);

            // Verify that the ContextMenuOpening event fired
            contextMenuOpeningEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
            VERIFY_IS_TRUE(contextMenuOpeningEvent->HasFired());

            LOG_OUTPUT(L"Open the menu with Shift+F10 key and invoke a command.");
            // This step opens the menu, but also ensures that it is eventually closed.
            ContextMenuTests::OpenMenuAndInvokeCommand(m_keyboardSequenceShiftF10);

            // Verify that the ContextMenuOpening event fired
            contextMenuOpeningEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
            VERIFY_IS_TRUE(contextMenuOpeningEvent->HasFired());

            LOG_OUTPUT(L"Open the menu with GamePadMenu key and invoke a command.");
            //This step opens the menu, but also ensures that it is eventually closed.
            ContextMenuTests::OpenMenuAndInvokeCommand(m_keyboardSequenceGamepadMenu);

            // Verify that the ContextMenuOpening event fired
            contextMenuOpeningEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
            VERIFY_IS_TRUE(contextMenuOpeningEvent->HasFired());
        }

        void ContextMenuTests::OpenMenuAndInvokeCommand(Platform::String^ openMenuKeyboardSequence)
        {
            int timeoutMilliseconds = 200;
            int maxAttempts = 10; // Wait for command invocation for 2 seconds at most
            KeyboardInjectionIgnoreEventWaitOverride keyboardInjectionIgnoreEventWaitOverride;

            // Invoke a command
            TestServices::WindowHelper->PrepareForPopupMenuWait();
            int attempts = 0;
            do
            {
                LOG_OUTPUT(L"Bringing up context menu with testing key sequence.");
                TestServices::KeyboardHelper->PressKeySequence(openMenuKeyboardSequence);

                LOG_OUTPUT(L"Pressing arrow key to select command.");
                TestServices::KeyboardHelper->Up();

                LOG_OUTPUT(L"Pressing Enter key to invoke command.");
                TestServices::KeyboardHelper->Enter();
                attempts++;
            } while (!TestServices::WindowHelper->WaitForPopupMenuCommandInvoked(timeoutMilliseconds) && attempts < maxAttempts);
            VERIFY_IS_TRUE(attempts <= maxAttempts);
            TestServices::WindowHelper->WaitForIdle();
        }
    } }
} } } }
