// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "MisspellingWavyLinesTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <ppltasks.h>
#include "FileLoader.h"
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <WUCRenderingScopeGuard.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Text {

        bool MisspellingWavyLinesTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool MisspellingWavyLinesTests::ClassCleanup()
        {
            return true;
        }

        bool MisspellingWavyLinesTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool MisspellingWavyLinesTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        Platform::String^ MisspellingWavyLinesTests::GetPathToFiles() const
        {
            // Get the deployment directory, and then append our test's directory to the end
            auto deploymentDir = GetTestDeploymentDir();
            return ref new Platform::String(deploymentDir + L"resources\\native\\foundation\\text\\");
        }

        //------------------------------------------------------------------------
        // Test case: Validates the TextBox and RichEditBox controls show red
        // misspelling wavy lines.
        //------------------------------------------------------------------------
        void MisspellingWavyLinesTests::CheckTextControlsWavyLines()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestCleanupWrapper cleanup([]()
            {
                TestServices::WindowHelper->SetPostTickCallback(nullptr);
            });

            std::shared_ptr<Event> stopTestEvent = std::make_shared<Event>();

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto buttonGotFocusEvent = std::make_shared<Event>();
            auto textBoxGotFocusEvent = std::make_shared<Event>();
            auto textBoxTextChangedEvent = std::make_shared<Event>();
            auto richEditBoxGotFocusEvent = std::make_shared<Event>();
            auto richEditBoxTextChangedEvent = std::make_shared<Event>();

            auto buttonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
            auto textBoxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);
            auto textBoxTextChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, TextChanged);
            auto richEditBoxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, GotFocus);
            auto richEditBoxTextChangedRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, TextChanged);

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::RichEditBox^ richEditBox = nullptr;
            xaml_controls::Button^ button = nullptr;
            xaml_controls::StackPanel^ rootStackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"MisspellingWavyLinesTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;

                textBox = safe_cast<xaml_controls::TextBox^>(rootStackPanel->FindName("textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                richEditBox = safe_cast<xaml_controls::RichEditBox^>(rootStackPanel->FindName("richEditBox"));
                VERIFY_IS_NOT_NULL(richEditBox);
                button = safe_cast<xaml_controls::Button^>(rootStackPanel->FindName("button"));
                VERIFY_IS_NOT_NULL(button);

                textBoxGotFocusRegistration.Attach(
                    textBox,
                    ref new xaml::RoutedEventHandler(
                    [textBoxGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"TextBox control GotFocus handler.");
                    textBoxGotFocusEvent->Set();
                }));

                textBoxTextChangedRegistration.Attach(
                    textBox,
                    ref new xaml_controls::TextChangedEventHandler(
                    [textBoxTextChangedEvent](Platform::Object^, xaml_controls::TextChangedEventArgs^)
                {
                    LOG_OUTPUT(L"TextBox control TextChanged handler.");
                    textBoxTextChangedEvent->Set();
                }));

                richEditBoxGotFocusRegistration.Attach(
                    richEditBox,
                    ref new xaml::RoutedEventHandler(
                    [richEditBoxGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"RichEditBox control GotFocus handler.");
                    richEditBoxGotFocusEvent->Set();
                }));

                richEditBoxTextChangedRegistration.Attach(
                    richEditBox,
                    ref new xaml::RoutedEventHandler(
                    [richEditBoxTextChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"RichEditBox control TextChanged handler.");
                    richEditBoxTextChangedEvent->Set();
                }));

                buttonGotFocusRegistration.Attach(
                    button,
                    ref new xaml::RoutedEventHandler(
                    [buttonGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Button control GotFocus handler.");
                    buttonGotFocusEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Set the text of the controls
                textBox->Text = L"A test";
                richEditBox->Document->SetText(Microsoft::UI::Text::TextSetOptions::None, L"A test");
            });

            textBoxTextChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            richEditBoxTextChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            // Record original DComp tree without wavy lines.
            LOG_OUTPUT(L"Recording original DComp tree without wavy lines.");
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

            LOG_OUTPUT(L"Setting focus to the TextBox control by tapping.");
            TestServices::InputHelper->Tap(textBox);

            textBoxGotFocusEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            // Move cursor to the end of the text
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_end#$u$_end");
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Injecting u and space keystrokes to form A testu and show wavy line.");
            TestServices::KeyboardHelper->PressKeySequence("u ");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Setting focus to the RichEditBox control with keyboard.");
                richEditBox->Focus(xaml::FocusState::Keyboard);
            });

            richEditBoxGotFocusEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Tapping RichEditBox control to position insertion cursor.");
            TestServices::InputHelper->Tap(richEditBox);
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Injecting u and space keystrokes to form A testu and show wavy line.");
            TestServices::KeyboardHelper->PressKeySequence("u ");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Focus the Button to discard the input pane.");
                button->Focus(FocusState::Pointer);
            });

            buttonGotFocusEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            TestServices::WindowHelper->SetPostTickCallback(ref new PostTickCallback([&]()
            {
                bool succeeded = false;

                LOG_OUTPUT(L"Recording DComp tree, hopefully with Wavy lines");
                LOG_OUTPUT(L"NOTE: This might fail because the spellchecking module is not loaded.");
                LOG_OUTPUT(L"Ignore MockDComp spew - we'll manually check if verification succeeded.");
                TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString(), true /*ignoreVerification*/, &succeeded);

                if (succeeded)
                {
                    LOG_OUTPUT(L"Succeeded! Wavy Lines found in output.");
                    stopTestEvent->Set();
                }
                else
                {
                    LOG_OUTPUT(L"Failed MockDCompOutput, but we'll try again once the spellchecking module is loaded/draws an underline.");
                    LOG_OUTPUT(L"This test will fail if we continually don't have correct MockDComp output leading to the stopTestEvent not firing.");
                }
            }));

            TestServices::WindowHelper->SynchronouslyTickUIThread(1);
            stopTestEvent->WaitForDefault();

            // We need to null the callback here, otherwise disabling the text controls (below) will trigger more PostTickCallbacks.
            TestServices::WindowHelper->SetPostTickCallback(nullptr);

            RunOnUIThread([&]()
            {
                // Prevent those text controls from getting focus again so input pane remains hidden.
                textBox->IsEnabled = false;
                richEditBox->IsEnabled = false;
                LOG_OUTPUT(L"Disabled both text controls.");
            });
        }
    } }
} } } }
