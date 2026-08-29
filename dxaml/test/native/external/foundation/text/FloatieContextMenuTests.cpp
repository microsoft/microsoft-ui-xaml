// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "FloatieContextMenuTests.h"
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

        bool FloatieContextMenuTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool FloatieContextMenuTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();
            return true;
        }

        bool FloatieContextMenuTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        Platform::String^ FloatieContextMenuTests::GetPathToFiles() const
        {
            return GetPackageFolder() + L"resources\\native\\foundation\\text\\";
        }

        static wf::Rect GetBounds(FrameworkElement^ element)
        {
            wf::Rect rect;
            RunOnUIThread([&]()
            {
                auto point1 = element->TransformToVisual(nullptr)->TransformPoint(wf::Point(0, 0));
                auto point2 = element->TransformToVisual(nullptr)->TransformPoint(wf::Point((float)element->ActualWidth, (float)element->ActualHeight));

                rect.X = min(point1.X, point2.X);
                rect.Y = point1.Y;
                rect.Width = abs(point1.X - point2.X);
                rect.Height = point2.Y - point1.Y;
            });

            return rect;
        }

        void FloatieContextMenuTests::TextBoxProofingMenu_NoErrors()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::Button^ button = nullptr;

            auto rootLoadedEvent = std::make_shared<Event>();
            auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <TextBox x:Name='textBox' SelectionFlyout='{x:Null}' FontSize='20' Width='180' Margin ='20,5,20,0'/>"
                    L"</StackPanel>"));
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                button = safe_cast<xaml_controls::Button^>(rootPanel->FindName("button"));
                VERIFY_IS_NOT_NULL(button);

                TestServices::WindowHelper->WindowContent = rootPanel;
                rootLoadedRegistration.Attach(
                    rootPanel,
                    ref new xaml::RoutedEventHandler(
                        [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Root loaded handler.");
                    rootLoadedEvent->Set();
                }));

            });

            LOG_OUTPUT(L"Waiting for root loaded event.");
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
            rootLoadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            FocusTestHelper::EnsureFocus(textBox, FocusState::Keyboard);
            TestServices::WindowHelper->WaitForIdle();

            // Type a correctly spelled word
            TestServices::KeyboardHelper->PressKeySequence("Hello ");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Place cursor over the word
                textBox->Select(1, 0);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto proofingMenu = safe_cast<xaml_controls::MenuFlyout^>(textBox->ProofingMenuFlyout);
                VERIFY_IS_NOT_NULL(proofingMenu);
                VERIFY_IS_TRUE(proofingMenu->Items->Size == 0);
            });
        }

        void FloatieContextMenuTests::TextBoxProofingMenu_MisspelledWord()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::Button^ button = nullptr;

            auto rootLoadedEvent = std::make_shared<Event>();
            auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);

            auto menuItemClickEvent = std::make_shared<Event>();
            auto clickRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutItem, Click);

            auto openedEvent = std::make_shared<Event>();
            auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <TextBox x:Name='textBox' SelectionFlyout='{x:Null}' FontSize='20' Width='180' Margin ='20,5,20,0'/>"
                    L"</StackPanel>"));
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                button = safe_cast<xaml_controls::Button^>(rootPanel->FindName("button"));
                VERIFY_IS_NOT_NULL(button);

                TestServices::WindowHelper->WindowContent = rootPanel;
                rootLoadedRegistration.Attach(
                    rootPanel,
                    ref new xaml::RoutedEventHandler(
                        [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Root loaded handler.");
                    rootLoadedEvent->Set();
                }));
            });

            LOG_OUTPUT(L"Waiting for root loaded event.");
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
            rootLoadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            FocusTestHelper::EnsureFocus(textBox, FocusState::Keyboard);

            TestServices::WindowHelper->WaitForIdle();

            // Type a misspelled word
            TestServices::KeyboardHelper->PressKeySequence("Hellloo ");
            TestServices::WindowHelper->WaitForIdle();

            // Press backspace to delete the space at the end
            TestServices::KeyboardHelper->Backspace();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Place cursor over the word
                textBox->Select(1, 0);
            });

            TestServices::WindowHelper->WaitForIdle();

            xaml_controls::MenuFlyout^ proofingMenu = nullptr;
            xaml_controls::MenuFlyoutItem^ firstSuggestion = nullptr;
            Platform::String^ firstSuggestionText = nullptr;

            RunOnUIThread([&]()
            {
                proofingMenu = safe_cast<xaml_controls::MenuFlyout^>(textBox->ProofingMenuFlyout);
                VERIFY_IS_NOT_NULL(proofingMenu);

                openedRegistration.Attach(proofingMenu, [&]() { openedEvent->Set(); });

                auto items = proofingMenu->Items;
                VERIFY_IS_TRUE(items->Size == 6);

                firstSuggestion = safe_cast<xaml_controls::MenuFlyoutItem^>(items->GetAt(0));
                firstSuggestionText = firstSuggestion->Text;
                LOG_OUTPUT(L"First suggestion: %s", firstSuggestionText->Data());
                clickRegistration.Attach(firstSuggestion, ref new xaml::RoutedEventHandler([menuItemClickEvent](Platform::Object^ s, xaml::RoutedEventArgs^ e)
                {
                    menuItemClickEvent->Set();
                }));

                VERIFY_IS_TRUE(safe_cast<xaml_controls::MenuFlyoutItem^>(items->GetAt(4))->Text == L"Add to dictionary");
                VERIFY_IS_TRUE(safe_cast<xaml_controls::MenuFlyoutItem^>(items->GetAt(5))->Text == L"Ignore");
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                proofingMenu->ShowAt(textBox);
            });

            openedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->Tap(firstSuggestion);
            menuItemClickEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Text: %s", textBox->Text->Data());
                VERIFY_IS_TRUE(textBox->Text == firstSuggestionText);
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void FloatieContextMenuTests::TextBoxProofingMenu_RepeatedWord()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::Button^ button = nullptr;

            auto rootLoadedEvent = std::make_shared<Event>();
            auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);

            auto menuItemClickEvent = std::make_shared<Event>();
            auto clickRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutItem, Click);

            auto openedEvent = std::make_shared<Event>();
            auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <TextBox x:Name='textBox' SelectionFlyout='{x:Null}' FontSize='20' Width='180' Margin ='20,5,20,0'/>"
                    L"</StackPanel>"));
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                button = safe_cast<xaml_controls::Button^>(rootPanel->FindName("button"));
                VERIFY_IS_NOT_NULL(button);

                TestServices::WindowHelper->WindowContent = rootPanel;
                rootLoadedRegistration.Attach(
                    rootPanel,
                    ref new xaml::RoutedEventHandler(
                        [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Root loaded handler.");
                    rootLoadedEvent->Set();
                }));
            });

            LOG_OUTPUT(L"Waiting for root loaded event.");
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
            rootLoadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            FocusTestHelper::EnsureFocus(textBox, FocusState::Keyboard);
            TestServices::WindowHelper->WaitForIdle();

            // Type a misspelled word
            TestServices::KeyboardHelper->PressKeySequence("hello hello ");
            TestServices::WindowHelper->WaitForIdle();

            // Press backspace to delete the space at the end
            TestServices::KeyboardHelper->Backspace();
            TestServices::WindowHelper->WaitForIdle();

            // Press left key to set cursor on top of repeated word
            TestServices::KeyboardHelper->Left();
            TestServices::WindowHelper->WaitForIdle();

            xaml_controls::MenuFlyout^ proofingMenu = nullptr;
            xaml_controls::MenuFlyoutItem^ deleteRepeated = nullptr;

            RunOnUIThread([&]()
            {
                proofingMenu = safe_cast<xaml_controls::MenuFlyout^>(textBox->ProofingMenuFlyout);
                VERIFY_IS_NOT_NULL(proofingMenu);

                openedRegistration.Attach(proofingMenu, [&]() { openedEvent->Set(); });

                auto items = proofingMenu->Items;
                VERIFY_IS_TRUE(items->Size == 2);

                deleteRepeated = safe_cast<xaml_controls::MenuFlyoutItem^>(items->GetAt(0));
                VERIFY_IS_TRUE(deleteRepeated->Text == L"Delete repeated word");
                clickRegistration.Attach(deleteRepeated, ref new xaml::RoutedEventHandler([menuItemClickEvent](Platform::Object^ s, xaml::RoutedEventArgs^ e)
                {
                    menuItemClickEvent->Set();
                }));

                VERIFY_IS_TRUE(safe_cast<xaml_controls::MenuFlyoutItem^>(items->GetAt(1))->Text == L"Ignore");
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                proofingMenu->ShowAt(textBox);
            });

            openedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->Tap(deleteRepeated);
            menuItemClickEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Text: %s", textBox->Text->Data());
                VERIFY_IS_TRUE(textBox->Text == L"hello");
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void FloatieContextMenuTests::TextBoxProofingMenu_AutocorrectedWord()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::Button^ button = nullptr;

            auto rootLoadedEvent = std::make_shared<Event>();
            auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);

            auto misTypedText = ref new Platform::String(L"helpp");
            auto correctedText = ref new Platform::String(L"help ");
            auto misTypedTextEvent = std::make_shared<Event>();
            auto correctedTextEvent = std::make_shared<Event>();
            auto textBoxTextChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, TextChanged);

            auto openedEvent = std::make_shared<Event>();
            auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <TextBox x:Name='textBox' SelectionFlyout='{x:Null}' FontSize='20' Width='180' Margin ='20,5,20,0'/>"
                    L"</StackPanel>"));
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                button = safe_cast<xaml_controls::Button^>(rootPanel->FindName("button"));
                VERIFY_IS_NOT_NULL(button);

                TestServices::WindowHelper->WindowContent = rootPanel;
                rootLoadedRegistration.Attach(
                    rootPanel,
                    ref new xaml::RoutedEventHandler(
                        [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Root loaded handler.");
                    rootLoadedEvent->Set();
                }));

                textBoxTextChangedRegistration.Attach(
                    textBox,
                    ref new xaml_controls::TextChangedEventHandler(
                        [&](Platform::Object^, xaml_controls::TextChangedEventArgs^)
                {
                    LOG_OUTPUT(L"Current TextInput:[%s]", textBox->Text->Data());
                    if (textBox->Text == misTypedText)
                    {
                        misTypedTextEvent->Set();
                    }
                    if (textBox->Text == correctedText)
                    {
                        correctedTextEvent->Set();
                    }
                }));
            });

            LOG_OUTPUT(L"Waiting for root loaded event.");
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
            rootLoadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            FocusTestHelper::EnsureFocus(textBox, FocusState::Keyboard);
            TestServices::WindowHelper->WaitForIdle();

            // Type a misspelled word
            TestServices::KeyboardHelper->PressKeySequence(misTypedText);
            misTypedTextEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            //Type space
            TestServices::KeyboardHelper->PressKeySequence(" ");
            TestServices::WindowHelper->WaitForIdle();

            // got "help "
            correctedTextEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            // Press left key to set cursor on top of autocorrected word
            TestServices::KeyboardHelper->Left();
            TestServices::WindowHelper->WaitForIdle();

            xaml_controls::MenuFlyout^ proofingMenu = nullptr;

            RunOnUIThread([&]()
            {
                proofingMenu = safe_cast<xaml_controls::MenuFlyout^>(textBox->ProofingMenuFlyout);
                VERIFY_IS_NOT_NULL(proofingMenu);

                openedRegistration.Attach(proofingMenu, [&]() { openedEvent->Set(); });

                auto items = proofingMenu->Items;
                auto size = items->Size;
                VERIFY_IS_TRUE(size == 1);

                auto autocorrected = safe_cast<xaml_controls::MenuFlyoutItem^>(items->GetAt(0));
                auto autocorrectedText = autocorrected->Text;
                VERIFY_IS_TRUE(autocorrectedText == L"Stop correcting helpp");
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void FloatieContextMenuTests::RichEditBoxProofingMenu_NoErrors()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::RichEditBox^ reb = nullptr;
            xaml_controls::Button^ button = nullptr;

            auto rootLoadedEvent = std::make_shared<Event>();
            auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <RichEditBox x:Name='reb' SelectionFlyout='{x:Null}' FontSize='20' Width='180' Margin ='20,5,20,0'/>"
                    L"</StackPanel>"));
                reb = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"reb"));
                VERIFY_IS_NOT_NULL(reb);
                button = safe_cast<xaml_controls::Button^>(rootPanel->FindName("button"));
                VERIFY_IS_NOT_NULL(button);

                TestServices::WindowHelper->WindowContent = rootPanel;
                rootLoadedRegistration.Attach(
                    rootPanel,
                    ref new xaml::RoutedEventHandler(
                        [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Root loaded handler.");
                    rootLoadedEvent->Set();
                }));

            });

            LOG_OUTPUT(L"Waiting for root loaded event.");
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
            rootLoadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            FocusTestHelper::EnsureFocus(reb, FocusState::Keyboard);
            TestServices::WindowHelper->WaitForIdle();

            // Type a correctly spelled word
            TestServices::KeyboardHelper->PressKeySequence("Hello ");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto selection = reb->Document->Selection;
                VERIFY_IS_NOT_NULL(selection);

                // Move cursor left twice to be on misspelled word
                selection->MoveLeft(Microsoft::UI::Text::TextRangeUnit::Character, 2, false);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto proofingMenu = safe_cast<xaml_controls::MenuFlyout^>(reb->ProofingMenuFlyout);
                VERIFY_IS_NOT_NULL(proofingMenu);
                VERIFY_IS_TRUE(proofingMenu->Items->Size == 0);
            });
        }

        void FloatieContextMenuTests::RichEditBoxProofingMenu_MisspelledWord()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::RichEditBox^ reb = nullptr;
            xaml_controls::Button^ button = nullptr;

            auto rootLoadedEvent = std::make_shared<Event>();
            auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);

            auto menuItemClickEvent = std::make_shared<Event>();
            auto clickRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutItem, Click);

            auto openedEvent = std::make_shared<Event>();
            auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <RichEditBox x:Name='reb' SelectionFlyout='{x:Null}' FontSize='20' Width='180' Margin ='20,5,20,0'/>"
                    L"</StackPanel>"));
                reb = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"reb"));
                VERIFY_IS_NOT_NULL(reb);
                button = safe_cast<xaml_controls::Button^>(rootPanel->FindName("button"));
                VERIFY_IS_NOT_NULL(button);

                TestServices::WindowHelper->WindowContent = rootPanel;
                rootLoadedRegistration.Attach(
                    rootPanel,
                    ref new xaml::RoutedEventHandler(
                        [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Root loaded handler.");
                    rootLoadedEvent->Set();
                }));
            });

            LOG_OUTPUT(L"Waiting for root loaded event.");
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
            rootLoadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            FocusTestHelper::EnsureFocus(reb, FocusState::Keyboard);
            TestServices::WindowHelper->WaitForIdle();

            // Type a misspelled word
            TestServices::KeyboardHelper->PressKeySequence("Hellloo ");
            TestServices::WindowHelper->WaitForIdle();

            // Press backspace to delete the space at the end
            TestServices::KeyboardHelper->Backspace();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto selection = reb->Document->Selection;
                VERIFY_IS_NOT_NULL(selection);

                // Move cursor left twice to be on misspelled word
                selection->MoveLeft(Microsoft::UI::Text::TextRangeUnit::Character, 2, false);
            });

            TestServices::WindowHelper->WaitForIdle();

            xaml_controls::MenuFlyout^ proofingMenu = nullptr;
            xaml_controls::MenuFlyoutItem^ firstSuggestion = nullptr;
            Platform::String^ firstSuggestionText = nullptr;

            RunOnUIThread([&]()
            {
                proofingMenu = safe_cast<xaml_controls::MenuFlyout^>(reb->ProofingMenuFlyout);
                VERIFY_IS_NOT_NULL(proofingMenu);

                openedRegistration.Attach(proofingMenu, [&]() { openedEvent->Set(); });

                auto items = proofingMenu->Items;
                VERIFY_IS_TRUE(items->Size == 6);

                firstSuggestion = safe_cast<xaml_controls::MenuFlyoutItem^>(items->GetAt(0));
                firstSuggestionText = firstSuggestion->Text;
                LOG_OUTPUT(L"First suggestion: %s", firstSuggestionText->Data());
                clickRegistration.Attach(firstSuggestion, ref new xaml::RoutedEventHandler([menuItemClickEvent](Platform::Object^ s, xaml::RoutedEventArgs^ e)
                {
                    menuItemClickEvent->Set();
                }));

                VERIFY_IS_TRUE(safe_cast<xaml_controls::MenuFlyoutItem^>(items->GetAt(4))->Text == L"Add to dictionary");
                VERIFY_IS_TRUE(safe_cast<xaml_controls::MenuFlyoutItem^>(items->GetAt(5))->Text == L"Ignore");
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                proofingMenu->ShowAt(reb);
            });

            openedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->Tap(firstSuggestion);
            menuItemClickEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                Platform::String^ text;
                reb->Document->GetText(Microsoft::UI::Text::TextGetOptions::AdjustCrlf, &text);

                LOG_OUTPUT(L"Text: %s", text->Data());
                VERIFY_IS_TRUE(text == firstSuggestionText);
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void FloatieContextMenuTests::RichEditBoxProofingMenu_RepeatedWord()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::RichEditBox^ reb = nullptr;
            xaml_controls::Button^ button = nullptr;

            auto rootLoadedEvent = std::make_shared<Event>();
            auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);

            auto menuItemClickEvent = std::make_shared<Event>();
            auto clickRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutItem, Click);

            auto openedEvent = std::make_shared<Event>();
            auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <RichEditBox x:Name='reb' SelectionFlyout='{x:Null}' FontSize='20' Width='180' Margin ='20,5,20,0'/>"
                    L"</StackPanel>"));
                reb = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"reb"));
                VERIFY_IS_NOT_NULL(reb);
                button = safe_cast<xaml_controls::Button^>(rootPanel->FindName("button"));
                VERIFY_IS_NOT_NULL(button);

                TestServices::WindowHelper->WindowContent = rootPanel;
                rootLoadedRegistration.Attach(
                    rootPanel,
                    ref new xaml::RoutedEventHandler(
                        [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Root loaded handler.");
                    rootLoadedEvent->Set();
                }));
            });

            LOG_OUTPUT(L"Waiting for root loaded event.");
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
            rootLoadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            FocusTestHelper::EnsureFocus(reb, FocusState::Keyboard);
            TestServices::WindowHelper->WaitForIdle();

            // Type a misspelled word
            TestServices::KeyboardHelper->PressKeySequence("hello hello ");
            TestServices::WindowHelper->WaitForIdle();

            // Press backspace to delete the space at the end
            TestServices::KeyboardHelper->Backspace();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto selection = reb->Document->Selection;
                VERIFY_IS_NOT_NULL(selection);

                // Move cursor left to be on repeated word
                selection->MoveLeft(Microsoft::UI::Text::TextRangeUnit::Character, 1, false);
            });

            TestServices::WindowHelper->WaitForIdle();

            xaml_controls::MenuFlyout^ proofingMenu = nullptr;
            xaml_controls::MenuFlyoutItem^ deleteRepeated = nullptr;

            RunOnUIThread([&]()
            {
                proofingMenu = safe_cast<xaml_controls::MenuFlyout^>(reb->ProofingMenuFlyout);
                VERIFY_IS_NOT_NULL(proofingMenu);

                openedRegistration.Attach(proofingMenu, [&]() { openedEvent->Set(); });

                auto items = proofingMenu->Items;
                VERIFY_IS_TRUE(items->Size == 2);

                deleteRepeated = safe_cast<xaml_controls::MenuFlyoutItem^>(items->GetAt(0));
                VERIFY_IS_TRUE(deleteRepeated->Text == L"Delete repeated word");
                clickRegistration.Attach(deleteRepeated, ref new xaml::RoutedEventHandler([menuItemClickEvent](Platform::Object^ s, xaml::RoutedEventArgs^ e)
                {
                    menuItemClickEvent->Set();
                }));

                VERIFY_IS_TRUE(safe_cast<xaml_controls::MenuFlyoutItem^>(items->GetAt(1))->Text == L"Ignore");
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                proofingMenu->ShowAt(reb);
            });

            openedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->Tap(deleteRepeated);
            menuItemClickEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                Platform::String^ text;
                reb->Document->GetText(Microsoft::UI::Text::TextGetOptions::AdjustCrlf, &text);

                LOG_OUTPUT(L"Text: %s", text->Data());
                VERIFY_IS_TRUE(text == L"hello");
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void FloatieContextMenuTests::RichEditBoxProofingMenu_AutocorrectedWord()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::RichEditBox^ reb = nullptr;
            xaml_controls::Button^ button = nullptr;

            auto rootLoadedEvent = std::make_shared<Event>();
            auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);

            auto menuItemClickEvent = std::make_shared<Event>();
            auto clickRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutItem, Click);

            auto openedEvent = std::make_shared<Event>();
            auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);

            auto misTypedText = ref new Platform::String(L"helpp");
            auto correctedText = ref new Platform::String(L"help ");
            auto misTypedTextEvent = std::make_shared<Event>();
            auto correctedTextEvent = std::make_shared<Event>();
            auto richEditBoxTextChangedRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, TextChanged);


            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <RichEditBox x:Name='reb' SelectionFlyout='{x:Null}' FontSize='20' Width='180' Margin ='20,5,20,0'/>"
                    L"</StackPanel>"));
                reb = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"reb"));
                VERIFY_IS_NOT_NULL(reb);
                button = safe_cast<xaml_controls::Button^>(rootPanel->FindName("button"));
                VERIFY_IS_NOT_NULL(button);

                TestServices::WindowHelper->WindowContent = rootPanel;
                rootLoadedRegistration.Attach(
                    rootPanel,
                    ref new xaml::RoutedEventHandler(
                        [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Root loaded handler.");
                    rootLoadedEvent->Set();
                }));

                richEditBoxTextChangedRegistration.Attach(
                    reb,
                    ref new xaml::RoutedEventHandler(
                        [&](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    Platform::String^ text;
                    reb->Document->GetText(mut::TextGetOptions::AdjustCrlf, &text);
                    LOG_OUTPUT(L"Current TextInput:[%s]", text->Data());
                    if (text == misTypedText)
                    {
                        misTypedTextEvent->Set();
                    }
                    if (text == correctedText)
                    {
                        correctedTextEvent->Set();
                    }
                }));
            });

            LOG_OUTPUT(L"Waiting for root loaded event.");
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
            rootLoadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            FocusTestHelper::EnsureFocus(reb, FocusState::Keyboard);
            TestServices::WindowHelper->WaitForIdle();


            // Type a misspelled word
            TestServices::KeyboardHelper->PressKeySequence(misTypedText);
            misTypedTextEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            //Type space
            TestServices::KeyboardHelper->PressKeySequence(" ");
            TestServices::WindowHelper->WaitForIdle();

            // got "help "
            correctedTextEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();


            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto selection = reb->Document->Selection;
                VERIFY_IS_NOT_NULL(selection);

                // Move cursor left to be on autocorrected word
                selection->MoveLeft(Microsoft::UI::Text::TextRangeUnit::Character, 2, false);
            });

            TestServices::WindowHelper->WaitForIdle();

            xaml_controls::MenuFlyout^ proofingMenu = nullptr;
            xaml_controls::MenuFlyoutItem^ deleteRepeated = nullptr;

            RunOnUIThread([&]()
            {
                proofingMenu = safe_cast<xaml_controls::MenuFlyout^>(reb->ProofingMenuFlyout);
                VERIFY_IS_NOT_NULL(proofingMenu);

                openedRegistration.Attach(proofingMenu, [&]() { openedEvent->Set(); });

                auto items = proofingMenu->Items;
                VERIFY_IS_TRUE(items->Size == 1);

                auto autocorrected = safe_cast<xaml_controls::MenuFlyoutItem^>(items->GetAt(0));
                auto autocorrectedText = autocorrected->Text;
                VERIFY_IS_TRUE(autocorrectedText == L"Stop correcting helpp");
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        template <typename TClassUnderTest>
        static void KeyInputContextMenuOpeningEventWhenFloatieEnabledTestHelper(TClassUnderTest^ testControl)
        {
            auto contextMenuOpeningEvent = std::make_shared<Event>();
            auto contextMenuOpeningRegistration = CreateSafeEventRegistration(TClassUnderTest, ContextMenuOpening);
            bool handleContextMenuEvent = false;
            xaml_primitives::FlyoutBase^ flyoutBase = nullptr;
            auto contextFlyoutOpenedEvent = std::make_shared<Event>();
            auto contextFlyoutOpenedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Opened);
            auto contextFlyoutClosedEvent = std::make_shared<Event>();
            auto closedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Closed);

            RunOnUIThread([&]()
            {
                flyoutBase = testControl->ContextFlyout;
                VERIFY_IS_NOT_NULL(flyoutBase);

                contextFlyoutOpenedRegistration.Attach(
                    flyoutBase,
                    ref new wf::EventHandler<Platform::Object^>(
                        [&](Platform::Object^, Platform::Object^)
                {
                    LOG_OUTPUT(L"ContextFlyout opened...");
                    contextFlyoutOpenedEvent->Set();
                }));
                closedRegistration.Attach(
                    flyoutBase,
                    ref new wf::EventHandler<Platform::Object^>(
                        [&](Platform::Object^, Platform::Object^)
                {
                    LOG_OUTPUT(L"ContextFlyout closed..");
                    contextFlyoutClosedEvent->Set();
                }));
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                contextMenuOpeningRegistration.Attach(
                    testControl,
                    ref new xaml_controls::ContextMenuOpeningEventHandler(
                        [&](Platform::Object^, xaml_controls::ContextMenuEventArgs^ args)
                {
                    LOG_OUTPUT(L"ContextMenuOpening event fired.");
                    contextMenuOpeningEvent->Set();
                    VERIFY_IS_FALSE(contextFlyoutOpenedEvent->HasFired());
                    args->Handled = handleContextMenuEvent;
                }));
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Test VK_APPS key.");
            handleContextMenuEvent = false;
            TestServices::KeyboardHelper->PressKeySequence("$d$_apps#$u$_apps");
            contextMenuOpeningEvent->WaitForDefault();
            contextFlyoutOpenedEvent->WaitForDefault();
            TestServices::KeyboardHelper->Escape();
            contextFlyoutClosedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Test GamePad Menu key.");
            contextMenuOpeningEvent->Reset();
            contextFlyoutOpenedEvent->Reset();
            TestServices::KeyboardHelper->PressKeySequence("$d$_GamePadMenu#$u$_GamePadMenu");
            contextMenuOpeningEvent->WaitForDefault();
            contextFlyoutOpenedEvent->WaitForDefault();
            TestServices::KeyboardHelper->Escape();
            contextFlyoutClosedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Test Shift-F10 key.");
            contextMenuOpeningEvent->Reset();
            contextFlyoutOpenedEvent->Reset();
            TestServices::KeyboardHelper->PressKeySequence("$d$_shift#$d$_f10#$u$_f10#$u$_shift");
            contextMenuOpeningEvent->WaitForDefault();
            contextFlyoutOpenedEvent->WaitForDefault();
            TestServices::KeyboardHelper->Escape();
            contextFlyoutClosedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            handleContextMenuEvent = true;

            LOG_OUTPUT(L"Test App handled ContextMenu opening event for VK_APPS key.");
            contextMenuOpeningEvent->Reset();
            contextFlyoutOpenedEvent->Reset();
            TestServices::KeyboardHelper->PressKeySequence("$d$_apps#$u$_apps");
            contextMenuOpeningEvent->WaitForDefault();
            contextFlyoutOpenedEvent->WaitForNoThrow(std::chrono::milliseconds(100));
            VERIFY_IS_FALSE(contextFlyoutOpenedEvent->HasFired());
            TestServices::KeyboardHelper->Escape();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Test App handled ContextMenu opening event for  GamePad Menu key.");
            contextMenuOpeningEvent->Reset();
            contextFlyoutOpenedEvent->Reset();
            TestServices::KeyboardHelper->PressKeySequence("$d$_GamePadMenu#$u$_GamePadMenu");
            contextMenuOpeningEvent->WaitForDefault();
            contextFlyoutOpenedEvent->WaitForNoThrow(std::chrono::milliseconds(100));
            VERIFY_IS_FALSE(contextFlyoutOpenedEvent->HasFired());
            TestServices::KeyboardHelper->Escape();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Test App handled ContextMenu opening event for Shift-F10 key.");
            contextMenuOpeningEvent->Reset();
            contextFlyoutOpenedEvent->Reset();
            TestServices::KeyboardHelper->PressKeySequence("$d$_shift#$d$_f10#$u$_f10#$u$_shift");
            contextMenuOpeningEvent->WaitForDefault();
            contextFlyoutOpenedEvent->WaitForNoThrow(std::chrono::milliseconds(100));
            VERIFY_IS_FALSE(contextFlyoutOpenedEvent->HasFired());
            TestServices::KeyboardHelper->Escape();
            TestServices::WindowHelper->WaitForIdle();
        }

        template <typename TClassUnderTest>
        static void MouseInputContextMenuOpeningEventWhenFloatieEnabledTestHelper(TClassUnderTest^ testControl)
        {
            auto contextMenuOpeningEvent = std::make_shared<Event>();
            auto contextMenuOpeningRegistration = CreateSafeEventRegistration(TClassUnderTest, ContextMenuOpening);
            bool handleContextMenuEvent = false;
            xaml_primitives::FlyoutBase^ flyoutBase = nullptr;
            auto contextFlyoutOpenedEvent = std::make_shared<Event>();
            auto contextFlyoutOpenedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Opening);
            auto contextFlyoutClosedEvent = std::make_shared<Event>();
            auto closedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Closed);

            RunOnUIThread([&]()
            {
                testControl->SelectionFlyout = nullptr;
                flyoutBase = testControl->ContextFlyout;
                VERIFY_IS_NOT_NULL(flyoutBase);

                contextFlyoutOpenedRegistration.Attach(
                    flyoutBase,
                    ref new wf::EventHandler<Platform::Object^>(
                        [&](Platform::Object^, Platform::Object^)
                {
                    LOG_OUTPUT(L"ContextFlyout opened...");
                    contextFlyoutOpenedEvent->Set();
                }));
                closedRegistration.Attach(
                    flyoutBase,
                    ref new wf::EventHandler<Platform::Object^>(
                        [&](Platform::Object^, Platform::Object^)
                {
                    LOG_OUTPUT(L"ContextFlyout closed..");
                    contextFlyoutClosedEvent->Set();
                }));
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                contextMenuOpeningRegistration.Attach(
                    testControl,
                    ref new xaml_controls::ContextMenuOpeningEventHandler(
                        [&](Platform::Object^, xaml_controls::ContextMenuEventArgs^ args)
                {
                    LOG_OUTPUT(L"ContextMenuOpening event fired.");
                    contextMenuOpeningEvent->Set();
                    VERIFY_IS_FALSE(contextFlyoutOpenedEvent->HasFired());
                    args->Handled = handleContextMenuEvent;
                }));
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Mouse right click...");
            wf::Rect textBoxBounds = {};

            TestServices::InputHelper->MoveMouse(testControl);
            TestServices::WindowHelper->WaitForIdle();

            // Right-click inside WebView and check that AppBar opened
            textBoxBounds = GetBounds(safe_cast<xaml::FrameworkElement^>(testControl));
            TestServices::InputHelper->ClickMouseButton(
                MouseButton::Right,
                wf::Point(textBoxBounds.Left + textBoxBounds.Width / 2, textBoxBounds.Top + textBoxBounds.Height / 2));

            contextMenuOpeningEvent->WaitForDefault();
            contextFlyoutOpenedEvent->WaitForDefault();
            TestServices::KeyboardHelper->Escape();
            contextFlyoutClosedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Test App handled ContextMenu opening event for mouse right click.");
            handleContextMenuEvent = true;
            contextMenuOpeningEvent->Reset();
            contextFlyoutOpenedEvent->Reset();
            TestServices::InputHelper->MoveMouse(testControl);
            TestServices::WindowHelper->WaitForIdle();

            textBoxBounds = GetBounds(safe_cast<xaml::FrameworkElement^>(testControl));
            TestServices::InputHelper->ClickMouseButton(
                MouseButton::Right,
                wf::Point(textBoxBounds.Left + textBoxBounds.Width / 2, textBoxBounds.Top + textBoxBounds.Height / 2));
            contextMenuOpeningEvent->WaitForDefault();
            contextFlyoutOpenedEvent->WaitForNoThrow(std::chrono::milliseconds(100));
            VERIFY_IS_FALSE(contextFlyoutOpenedEvent->HasFired());
        }

        template <typename TClassUnderTest>
        static void TouchInputContextMenuOpeningEventWhenFloatieEnabledTestHelper(TClassUnderTest^ testControl)
        {
            auto contextMenuOpeningEvent = std::make_shared<Event>();
            auto contextMenuOpeningRegistration = CreateSafeEventRegistration(TClassUnderTest, ContextMenuOpening);
            bool handleContextMenuEvent = false;
            xaml_primitives::FlyoutBase^ flyoutBase = nullptr;
            auto contextFlyoutOpenedEvent = std::make_shared<Event>();
            auto contextFlyoutOpenedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Opening);
            auto contextFlyoutClosedEvent = std::make_shared<Event>();
            auto closedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Closed);

            RunOnUIThread([&]()
            {
                testControl->SelectionFlyout = nullptr;
                flyoutBase = testControl->ContextFlyout;
                VERIFY_IS_NOT_NULL(flyoutBase);

                contextFlyoutOpenedRegistration.Attach(
                    flyoutBase,
                    ref new wf::EventHandler<Platform::Object^>(
                        [&](Platform::Object^, Platform::Object^)
                {
                    LOG_OUTPUT(L"ContextFlyout opened...");
                    contextFlyoutOpenedEvent->Set();
                }));
                closedRegistration.Attach(
                    flyoutBase,
                    ref new wf::EventHandler<Platform::Object^>(
                        [&](Platform::Object^, Platform::Object^)
                {
                    LOG_OUTPUT(L"ContextFlyout closed..");
                    contextFlyoutClosedEvent->Set();
                }));
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                contextMenuOpeningRegistration.Attach(
                    testControl,
                    ref new xaml_controls::ContextMenuOpeningEventHandler(
                        [&](Platform::Object^, xaml_controls::ContextMenuEventArgs^ args)
                {
                    LOG_OUTPUT(L"ContextMenuOpening event fired.");
                    contextMenuOpeningEvent->Set();
                    VERIFY_IS_FALSE(contextFlyoutOpenedEvent->HasFired());
                    args->Handled = handleContextMenuEvent;
                }));
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Hold...");
            contextMenuOpeningEvent->Reset();
            contextFlyoutOpenedEvent->Reset();
            TestServices::InputHelper->Hold(testControl);
            contextMenuOpeningEvent->WaitForDefault();
            contextFlyoutOpenedEvent->WaitForDefault();
            TestServices::KeyboardHelper->Escape();
            contextFlyoutClosedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            handleContextMenuEvent = true;

            LOG_OUTPUT(L"Test App handled ContextMenu opening event for hold.");
            contextMenuOpeningEvent->Reset();
            contextFlyoutOpenedEvent->Reset();
            TestServices::InputHelper->Hold(testControl);
            contextMenuOpeningEvent->WaitForDefault();
            contextFlyoutOpenedEvent->WaitForNoThrow(std::chrono::milliseconds(100));
            VERIFY_IS_FALSE(contextFlyoutOpenedEvent->HasFired());
        }

        void FloatieContextMenuTests::TextBoxKeyInputContextMenuOpeningEventWhenFloatieEnabled()
        {
            TestCleanupWrapper cleanup;
            TextBox^ testControl = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <TextBox x:Name='textBox' Text= 'Test Test Test Test Test' FontSize='40' Width='180' Margin ='20,5,20,0'/>"
                    L"</StackPanel>"));
                testControl = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(testControl);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(testControl, FocusState::Keyboard);

            KeyInputContextMenuOpeningEventWhenFloatieEnabledTestHelper<xaml_controls::TextBox>(testControl);

            TestServices::WindowHelper->WaitForIdle();
        }

        void FloatieContextMenuTests::RichEditBoxKeyInputContextMenuOpeningEventWhenFloatieEnabled()
        {
            TestCleanupWrapper cleanup;
            RichEditBox^ testControl = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <RichEditBox x:Name='richEditBox' FontSize='40' Width='180' Margin ='20,5,20,0'/>"
                    L"</StackPanel>"));
                testControl = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"richEditBox"));
                VERIFY_IS_NOT_NULL(testControl);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(testControl, FocusState::Keyboard);
            RunOnUIThread([&]()
            {
                testControl->Document->SetText(Microsoft::UI::Text::TextSetOptions::None, L"01234");
            });
            TestServices::WindowHelper->WaitForIdle();
            KeyInputContextMenuOpeningEventWhenFloatieEnabledTestHelper<xaml_controls::RichEditBox>(testControl);

            TestServices::WindowHelper->WaitForIdle();
        }

        void FloatieContextMenuTests::PasswordBoxKeyInputContextMenuOpeningEventWhenFloatieEnabled()
        {
            TestCleanupWrapper cleanup;
            PasswordBox^ testControl = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <PasswordBox x:Name='passwordBox' Password='test' FontSize='40' Width='180' Margin ='20,5,20,0'/>"
                    L"</StackPanel>"));
                testControl = safe_cast<xaml_controls::PasswordBox^>(rootPanel->FindName(L"passwordBox"));
                VERIFY_IS_NOT_NULL(testControl);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(testControl, FocusState::Keyboard);

            KeyInputContextMenuOpeningEventWhenFloatieEnabledTestHelper<xaml_controls::PasswordBox>(testControl);

            TestServices::WindowHelper->WaitForIdle();
        }

        void FloatieContextMenuTests::TextBlockKeyInputContextMenuOpeningEventWhenFloatieEnabled()
        {
            TestCleanupWrapper cleanup;
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

            TextBlock^ testControl = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                L"  <TextBlock x:Name='textBlock' Text= 'Test Test Test Test Test' IsTextSelectionEnabled='True' FontSize='20' Width='180' Margin ='20,5,20,0'/>"
                L"</StackPanel>"));
                testControl = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"textBlock"));
                VERIFY_IS_NOT_NULL(testControl);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->LeftMouseClick(testControl);

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                testControl->SelectAll();
            });
            TestServices::WindowHelper->WaitForIdle();

            KeyInputContextMenuOpeningEventWhenFloatieEnabledTestHelper<xaml_controls::TextBlock>(testControl);

            TestServices::WindowHelper->WaitForIdle();
        }

        void FloatieContextMenuTests::RichTextBlockKeyInputContextMenuOpeningEventWhenFloatieEnabled()
        {
            TestCleanupWrapper cleanup;
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

            RichTextBlock^ testControl = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                L"  <RichTextBlock x:Name='richTextBlock' IsTextSelectionEnabled='True' FontSize='20' Width='180' Margin ='20,5,20,0'> <Paragraph>Hello World</Paragraph> </RichTextBlock>"
                L"</StackPanel>"));
                testControl = safe_cast<xaml_controls::RichTextBlock^>(rootPanel->FindName(L"richTextBlock"));
                VERIFY_IS_NOT_NULL(testControl);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->LeftMouseClick(testControl);

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                testControl->SelectAll();
            });
            TestServices::WindowHelper->WaitForIdle();

            KeyInputContextMenuOpeningEventWhenFloatieEnabledTestHelper<xaml_controls::RichTextBlock>(testControl);

            TestServices::WindowHelper->WaitForIdle();
        }

        void FloatieContextMenuTests::TextBoxMouseInputContextMenuOpeningEventWhenFloatieEnabled()
        {
            TestCleanupWrapper cleanup;
            TextBox^ testControl = nullptr;
            Button^ button = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <TextBox x:Name='textBox' Text= 'Test Test Test Test Test' FontSize='40' Width='180' Margin ='20,5,20,0' SelectionFlyout='{x:Null}' />"
                    L"</StackPanel>"));
                testControl = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(testControl);
                button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
                VERIFY_IS_NOT_NULL(button);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(testControl, FocusState::Keyboard);
            MouseInputContextMenuOpeningEventWhenFloatieEnabledTestHelper<xaml_controls::TextBox>(testControl);
        }

        void FloatieContextMenuTests::RichEditBoxMouseInputContextMenuOpeningEventWhenFloatieEnabled()
        {
            TestCleanupWrapper cleanup;
            RichEditBox^ testControl = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <RichEditBox x:Name='richEditBox' FontSize='40' Width='180' Margin ='20,5,20,0'/>"
                    L"</StackPanel>"));
                testControl = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"richEditBox"));
                VERIFY_IS_NOT_NULL(testControl);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(testControl, FocusState::Keyboard);
            RunOnUIThread([&]()
            {
                testControl->Document->SetText(Microsoft::UI::Text::TextSetOptions::None, L"01234");
            });
            TestServices::WindowHelper->WaitForIdle();
            MouseInputContextMenuOpeningEventWhenFloatieEnabledTestHelper<xaml_controls::RichEditBox>(testControl);

            TestServices::WindowHelper->WaitForIdle();
        }

        void FloatieContextMenuTests::PasswordBoxMouseInputContextMenuOpeningEventWhenFloatieEnabled()
        {
            TestCleanupWrapper cleanup;
            PasswordBox^ testControl = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <PasswordBox x:Name='passwordBox' Password='test' FontSize='40' Width='180' Margin ='20,5,20,0'/>"
                    L"</StackPanel>"));
                testControl = safe_cast<xaml_controls::PasswordBox^>(rootPanel->FindName(L"passwordBox"));
                VERIFY_IS_NOT_NULL(testControl);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(testControl, FocusState::Keyboard);

            MouseInputContextMenuOpeningEventWhenFloatieEnabledTestHelper<xaml_controls::PasswordBox>(testControl);

            TestServices::WindowHelper->WaitForIdle();
        }

        void FloatieContextMenuTests::TextBlockMouseInputContextMenuOpeningEventWhenFloatieEnabled()
        {
            TestCleanupWrapper cleanup;
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

            TextBlock^ testControl = nullptr;
            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <TextBlock x:Name='textBlock' Text= 'Test Test Test Test Test' IsTextSelectionEnabled='True' FontSize='20' Width='180' Margin ='20,5,20,0'/>"
                    L"</StackPanel>"));
                testControl = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"textBlock"));
                VERIFY_IS_NOT_NULL(testControl);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->LeftMouseClick(testControl);

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                testControl->SelectAll();
            });
            TestServices::WindowHelper->WaitForIdle();

            MouseInputContextMenuOpeningEventWhenFloatieEnabledTestHelper<xaml_controls::TextBlock>(testControl);
        }

        void FloatieContextMenuTests::RichTextBlockMouseInputContextMenuOpeningEventWhenFloatieEnabled()
        {
            TestCleanupWrapper cleanup;
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

            RichTextBlock^ testControl = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <RichTextBlock x:Name='richTextBlock' IsTextSelectionEnabled='True' FontSize='20' Width='180' Margin ='20,5,20,0'> <Paragraph>Hello World</Paragraph> </RichTextBlock>"
                    L"</StackPanel>"));
                testControl = safe_cast<xaml_controls::RichTextBlock^>(rootPanel->FindName(L"richTextBlock"));
                VERIFY_IS_NOT_NULL(testControl);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->LeftMouseClick(testControl);

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                testControl->SelectAll();
            });
            TestServices::WindowHelper->WaitForIdle();

            MouseInputContextMenuOpeningEventWhenFloatieEnabledTestHelper<xaml_controls::RichTextBlock>(testControl);

            TestServices::WindowHelper->WaitForIdle();
        }

        void FloatieContextMenuTests::TextBoxTouchInputContextMenuOpeningEventWhenFloatieEnabled()
        {
            TestCleanupWrapper cleanup;
            TextBox^ testControl = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <TextBox x:Name='textBox' Text= 'Test Test Test Test Test' FontSize='40' Width='180' Margin ='20,5,20,0' SelectionFlyout='{x:Null}' />"
                    L"</StackPanel>"));
                testControl = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(testControl);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(testControl, FocusState::Keyboard);
            TouchInputContextMenuOpeningEventWhenFloatieEnabledTestHelper<xaml_controls::TextBox>(testControl);
            TestServices::WindowHelper->WaitForIdle();
        }

        void FloatieContextMenuTests::RichEditBoxTouchInputContextMenuOpeningEventWhenFloatieEnabled()
        {
            TestCleanupWrapper cleanup;
            RichEditBox^ testControl = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <RichEditBox x:Name='richEditBox' FontSize='40' Width='180' Margin ='20,5,20,0'/>"
                    L"</StackPanel>"));
                testControl = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"richEditBox"));
                VERIFY_IS_NOT_NULL(testControl);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(testControl, FocusState::Keyboard);
            RunOnUIThread([&]()
            {
                testControl->Document->SetText(Microsoft::UI::Text::TextSetOptions::None, L"01234");
            });
            TestServices::WindowHelper->WaitForIdle();
            TouchInputContextMenuOpeningEventWhenFloatieEnabledTestHelper<xaml_controls::RichEditBox>(testControl);

            TestServices::WindowHelper->WaitForIdle();
        }

        void FloatieContextMenuTests::PasswordBoxTouchInputContextMenuOpeningEventWhenFloatieEnabled()
        {
            TestCleanupWrapper cleanup;
            PasswordBox^ testControl = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <PasswordBox x:Name='passwordBox' Password='test' FontSize='40' Width='180' Margin ='20,5,20,0'/>"
                    L"</StackPanel>"));
                testControl = safe_cast<xaml_controls::PasswordBox^>(rootPanel->FindName(L"passwordBox"));
                VERIFY_IS_NOT_NULL(testControl);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(testControl, FocusState::Keyboard);

            TouchInputContextMenuOpeningEventWhenFloatieEnabledTestHelper<xaml_controls::PasswordBox>(testControl);

            TestServices::WindowHelper->WaitForIdle();
        }

        void FloatieContextMenuTests::TextBlockTouchInputContextMenuOpeningEventWhenFloatieEnabled()
        {
            TestCleanupWrapper cleanup;
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

            TextBlock^ testControl = nullptr;
            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <TextBlock x:Name='textBlock' Text= 'Test Test Test Test Test' IsTextSelectionEnabled='True' FontSize='20' Width='180' Margin ='20,5,20,0'/>"
                    L"</StackPanel>"));
                testControl = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"textBlock"));
                VERIFY_IS_NOT_NULL(testControl);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->LeftMouseClick(testControl);

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                testControl->SelectAll();
            });
            TestServices::WindowHelper->WaitForIdle();

            TouchInputContextMenuOpeningEventWhenFloatieEnabledTestHelper<xaml_controls::TextBlock>(testControl);
            TestServices::WindowHelper->WaitForIdle();
        }

        void FloatieContextMenuTests::RichTextBlockTouchInputContextMenuOpeningEventWhenFloatieEnabled()
        {
            TestCleanupWrapper cleanup;
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

            RichTextBlock^ testControl = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <RichTextBlock x:Name='richTextBlock' IsTextSelectionEnabled='True' FontSize='20' Width='180' Margin ='20,5,20,0'> <Paragraph>Hello World</Paragraph> </RichTextBlock>"
                    L"</StackPanel>"));
                testControl = safe_cast<xaml_controls::RichTextBlock^>(rootPanel->FindName(L"richTextBlock"));
                VERIFY_IS_NOT_NULL(testControl);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->LeftMouseClick(testControl);

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                testControl->SelectAll();
            });
            TestServices::WindowHelper->WaitForIdle();

            TouchInputContextMenuOpeningEventWhenFloatieEnabledTestHelper<xaml_controls::RichTextBlock>(testControl);

            TestServices::WindowHelper->WaitForIdle();
        }

        void FloatieContextMenuTests::RichEditBoxKeyDownDismissFloatie()
        {
            TestCleanupWrapper cleanup;
            RichEditBox^ testControl = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <RichEditBox x:Name='richEditBox' FontSize='40' Width='180' Margin ='20,5,20,0' />"
                    L"</StackPanel>"));
                testControl = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"richEditBox"));
                testControl->Document->SetText(Microsoft::UI::Text::TextSetOptions::None, L"01234");
                VERIFY_IS_NOT_NULL(testControl);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(testControl, FocusState::Keyboard);

            xaml_primitives::FlyoutBase^ flyoutBase = nullptr;
            auto selectionFlyoutOpenedEvent = std::make_shared<Event>();
            auto selectionFlyoutOpenedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Opened);
            auto selectionFlyoutClosedEvent = std::make_shared<Event>();
            auto closedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Closed);
            auto richEditBoxTextChangedEvent = std::make_shared<Event>();
            auto richEditBoxTextChangedRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, TextChanged);

            RunOnUIThread([&]()
            {
                flyoutBase = testControl->SelectionFlyout;
                VERIFY_IS_NOT_NULL(flyoutBase);

                selectionFlyoutOpenedRegistration.Attach(
                    flyoutBase,
                    ref new wf::EventHandler<Platform::Object^>(
                        [&](Platform::Object^, Platform::Object^)
                {
                    LOG_OUTPUT(L"SelectionFlyout opened.");
                    selectionFlyoutOpenedEvent->Set();
                }));
                closedRegistration.Attach(
                    flyoutBase,
                    ref new wf::EventHandler<Platform::Object^>(
                        [&](Platform::Object^, Platform::Object^)
                {
                    LOG_OUTPUT(L"SelectionFlyout closed.");
                    selectionFlyoutClosedEvent->Set();
                }));
                richEditBoxTextChangedRegistration.Attach(
                    testControl,
                    ref new xaml::RoutedEventHandler(
                        [&](Platform::Object^, Platform::Object^)
                {
                    richEditBoxTextChangedEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Invoke floatie by double-tapping to select text.");
            TestServices::InputHelper->Tap(testControl);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::InputHelper->Tap(testControl);

            selectionFlyoutOpenedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            richEditBoxTextChangedEvent->Reset();
            LOG_OUTPUT(L"Press A key.");
            TestServices::KeyboardHelper->PressKeySequence("a");
            selectionFlyoutClosedEvent->WaitForDefault();
            richEditBoxTextChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
        }
    } }
} } } }
