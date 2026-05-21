// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "CaretBrowsingTests.h"
#include <AutomationClient\AutomationClientManager.h>
#include <AutomationClient\AutomationEventHandler.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include <RuntimeEnabledFeatureOverride.h>
#include <CustomSystemFontCollectionOverride.h>
#include <SafeEventRegistration.h>
#include <UIAutomationCore.h>
#include <WUCRenderingScopeGuard.h>
#include <TreeHelper.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Text;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Documents;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Automation::Peers;
using namespace Microsoft::UI::Xaml::Input;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

#define HOME_KEY L"$d$_home#$u$_home"
#define END_KEY  L"$d$_end#$u$_end"
#define CTRL_HOME_KEY L"$d$_ctrl#$d$_home#$u$_home#$u$_ctrl"
#define CTRL_END_KEY  L"$d$_ctrl#$d$_end#$u$_end#$u$_ctrl"

#define SHIFT_HOME_KEY L"$d$_shift#$d$_home#$u$_home#$u$_shift"
#define SHIFT_END_KEY  L"$d$_shift#$d$_end#$u$_end#$u$_shift"
#define SHIFT_CONTROL_HOME_KEY L"$d$_shift#$d$_ctrl#$d$_home#$u$_home#$u$_ctrl#$u$_shift"
#define SHIFT_CONTROL_END_KEY  L"$d$_shift#$d$_ctrl#$d$_end#$u$_end#$u$_ctrl#$u$_shift"

#define F7_KEY L"$d$_f7#$u$_f7"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics {

        bool CaretBrowsingTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool CaretBrowsingTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool CaretBrowsingTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void CaretBrowsingTests::TurnCaretBrowsingOffAndClicking()
        {
            TestCleanupWrapper cleanup;
            TextBlock^ textBlock;
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));
            CreateSingleLineTextBlock(textBlock);
            TestServices::InputHelper->LeftMouseClick(textBlock);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                //  checking for the cursor Rectangle child
                VERIFY_ARE_EQUAL(VisualTreeHelper::GetChildrenCount(textBlock), 0);
                VERIFY_IS_TRUE(FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(textBlock));
            });
        }

        void CaretBrowsingTests::TurnCaretBrowsingOffAndTabbing()
        {
            TestCleanupWrapper cleanup;
            TextBlock^ textBlock;
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));
            CreateSingleLineTextBlock(textBlock);
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(VisualTreeHelper::GetChildrenCount(textBlock), 0);
            });
        }

        void CaretBrowsingTests::TurnCaretBrowsingOnAndClicking()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            auto testGuard = wil::scope_exit([] {
                TestServices::WindowHelper->SetCaretBrowsingModeGlobal(false, false);
            });
            TextBlock^ textBlock;
            RichTextBlock^ richTextBlock;
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));
            CreateTextBlockAndRichTextBlock(richTextBlock, textBlock);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::WindowHelper->SetCaretBrowsingModeGlobal(true, false);
            TestServices::InputHelper->LeftMouseClick(textBlock);
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 8);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

            TestServices::InputHelper->LeftMouseClick(richTextBlock);
            TestServices::WindowHelper->WaitForIdle();
            VerifyRichTextBlockCaretPosition(richTextBlock, 14);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
        }

        static void VerifyElementHasCaretChild(UIElement^ element)
        {
            VERIFY_ARE_EQUAL(VisualTreeHelper::GetChildrenCount(element), 1);
            Panel^ caret = safe_cast<Panel^>(VisualTreeHelper::GetChild(element, 0));
            VERIFY_IS_NOT_NULL(caret);
        }

        void CaretBrowsingTests::TurnCaretBrowsingOnAndTabbing()
        {
            TestCleanupWrapper cleanup;
            auto testGuard = wil::scope_exit([] {
                TestServices::WindowHelper->SetCaretBrowsingModeGlobal(false, false);
            });
            TextBlock^ textBlock;
            RichTextBlock^ richTextBlock;
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));
            CreateTextBlockAndRichTextBlock(richTextBlock, textBlock);
            TestServices::WindowHelper->SetCaretBrowsingModeGlobal(true, false);
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();

            // For an unknown reason, Tab sometimes ends up with focus on the richTextBlock rather
            // than the textBlock, just 4% of the time in CatGates confirmation runs.  If we detect
            // this has happened, just do an extra Tab to get focus in the right starting place.
            bool isFocusOnTextBlock = false;
            RunOnUIThread([&]()
            {
                isFocusOnTextBlock = (VisualTreeHelper::GetChildrenCount(textBlock) == 1);
            });
            if (!isFocusOnTextBlock)
            {
                TestServices::KeyboardHelper->Tab();
                TestServices::WindowHelper->WaitForIdle();
            }

            RunOnUIThread([&]()
            {
                VerifyElementHasCaretChild(textBlock);
                VERIFY_ARE_EQUAL(VisualTreeHelper::GetChildrenCount(richTextBlock), 0);
            });

            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(VisualTreeHelper::GetChildrenCount(textBlock), 0);
                VerifyElementHasCaretChild(richTextBlock);
            });
        }

        void CaretBrowsingTests::TurnCaretBrowsingOnAndClickingNonSelectable()
        {
            TestCleanupWrapper cleanup;
            auto testGuard = wil::scope_exit([] {
                TestServices::WindowHelper->SetCaretBrowsingModeGlobal(false, false);
            });
            TextBlock^ textBlock;
            RichTextBlock^ richTextBlock;
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));
            CreateNonSelectableTBAndSelectableRTB(richTextBlock, textBlock);
            TestServices::WindowHelper->SetCaretBrowsingModeGlobal(true, false);
            TestServices::InputHelper->LeftMouseClick(textBlock);
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(VisualTreeHelper::GetChildrenCount(textBlock), 0);
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::InputHelper->LeftMouseClick(richTextBlock);
            TestServices::WindowHelper->WaitForIdle();
            VerifyRichTextBlockCaretPosition(richTextBlock, 14);
        }

        void CaretBrowsingTests::TurnCaretBrowsingOnAndTabbingNonSelectable()
        {
            TestCleanupWrapper cleanup;
            auto testGuard = wil::scope_exit([] {
                TestServices::WindowHelper->SetCaretBrowsingModeGlobal(false, false);
            });
            TextBlock^ textBlock;
            RichTextBlock^ richTextBlock;
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));
            CreateNonSelectableTBAndSelectableRTB(richTextBlock, textBlock);
            TestServices::WindowHelper->SetCaretBrowsingModeGlobal(true, false);
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(VisualTreeHelper::GetChildrenCount(textBlock), 0);
                VerifyElementHasCaretChild(richTextBlock);
            });
        }

        void CaretBrowsingTests::ShowCaretBrowsingDialogViaF7()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            auto testGuard = wil::scope_exit([] {
                TestServices::WindowHelper->SetCaretBrowsingModeGlobal(false, false);
            });
            TextBlock^ textBlock;
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));
            RunOnUIThread([&]()
            {
                auto rootCanvas = safe_cast<xaml_controls::Canvas^>(xaml_markup::XamlReader::Load(
                    L"<Canvas Background='Black' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"<StackPanel VerticalAlignment='Top'> "
                    L"  <Button x:Name='button' Content='Button'/>"
                    L"  <TextBlock x:Name='mytb' IsTextSelectionEnabled='True' Text='Hello World' FontSize='15'/>"
                    L"</StackPanel>"
                    L"</Canvas>"));
                textBlock = safe_cast<TextBlock^>(rootCanvas->FindName(L"mytb"));
                VERIFY_IS_NOT_NULL(textBlock);
                TestServices::WindowHelper->WindowContent = rootCanvas;

                // Give focus to the button, so keyboard input will go to this UI in WPF hosting mode.
                safe_cast<Button^>(rootCanvas->FindName(L"button"))->Focus(FocusState::Programmatic);
            });
            TestServices::WindowHelper->WaitForIdle();

            // F7 to show the dialog.
            TestServices::KeyboardHelper->PressKeySequence(F7_KEY);
            TestServices::WindowHelper->WaitForIdle();

            // Focus should be on the checkbox
            RunOnUIThread([&]()
            {
                auto focus = FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot);
                VERIFY_IS_TRUE(safe_cast<CheckBox^>(focus) != nullptr);
            });

            // Tab to the "Yes" button
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                auto button = safe_cast<Button^>(FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
                VERIFY_IS_TRUE(button != nullptr);
                VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(button->Content), ref new Platform::String(L"Yes"));
            });

            // Press F7 again (while the dialog is still up) to verify no crash .
            TestServices::KeyboardHelper->PressKeySequence(F7_KEY);
            TestServices::WindowHelper->WaitForIdle();

            // Press Space to turn on caret browsing, click in the textblock, and verify the caret
            TestServices::KeyboardHelper->Space();
            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->LeftMouseClick(textBlock);
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 8);
        }

        void CaretBrowsingTests::CaretBrowsingPermDisableViaF7Checkbox()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            auto testGuard = wil::scope_exit([] {
                TestServices::WindowHelper->SetCaretBrowsingModeGlobal(false, false);
            });
            TextBlock^ textBlock;
            Button^ button;
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));
            RunOnUIThread([&]()
            {
                auto rootCanvas = safe_cast<xaml_controls::Canvas^>(xaml_markup::XamlReader::Load(
                    L"<Canvas Background='Black' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"<StackPanel VerticalAlignment='Top'> "
                    L"  <Button x:Name='button' Content='Button'/>"
                    L"  <TextBlock x:Name='mytb' IsTextSelectionEnabled='True' Text='Hello World' FontSize='15'/>"
                    L"</StackPanel>"
                    L"</Canvas>"));
                textBlock = safe_cast<TextBlock^>(rootCanvas->FindName(L"mytb"));
                VERIFY_IS_NOT_NULL(textBlock);
                TestServices::WindowHelper->WindowContent = rootCanvas;

                // Give focus to the button, so keyboard input will go to this UI in WPF hosting mode.
                button = safe_cast<Button^>(rootCanvas->FindName(L"button"));
                button->Focus(FocusState::Programmatic);
            });
            TestServices::WindowHelper->WaitForIdle();

            // F7 to show the dialog.
            TestServices::KeyboardHelper->PressKeySequence(F7_KEY);
            TestServices::WindowHelper->WaitForIdle();

            // Focus should be on the checkbox. Press Space to check the checkbox
            TestServices::KeyboardHelper->Space();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto focus = FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot);
                VERIFY_IS_TRUE(safe_cast<CheckBox^>(focus) != nullptr);
                VERIFY_IS_TRUE(safe_cast<CheckBox^>(focus)->IsChecked->Value);
            });

            // Tab to the "No" button
            TestServices::KeyboardHelper->Tab();
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                auto button = safe_cast<Button^>(FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
                VERIFY_IS_TRUE(button != nullptr);
                VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(button->Content), ref new Platform::String(L"No"));
            });

            // Press Space to permanently disable caret browsing for this process
            TestServices::KeyboardHelper->Space();
            TestServices::WindowHelper->WaitForIdle();

            // Ensure caret browsing is disabled (no cursor child)
            TestServices::InputHelper->LeftMouseClick(textBlock);
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(VisualTreeHelper::GetChildrenCount(textBlock), 0);

                // Move focus back to the button
                button->Focus(FocusState::Programmatic);
            });

            // Press F7 again, and verify no dialog and still no caret browsing.
            TestServices::KeyboardHelper->PressKeySequence(F7_KEY);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // focus should be on the button in the main UI, not in a dialog
                VERIFY_IS_TRUE(FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(button));
            });

            // Ensure caret browsing is disabled (no cursor child)
            TestServices::InputHelper->LeftMouseClick(textBlock);
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(VisualTreeHelper::GetChildrenCount(textBlock), 0);
            });
        }

        void CaretBrowsingTests::CaretMovingByChar()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            auto testGuard = wil::scope_exit([] {
                TestServices::WindowHelper->SetCaretBrowsingModeGlobal(false, false);
            });
            TextBlock^ textBlock;
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));
            CreateSingleLineTextBlock(textBlock);
            TestServices::WindowHelper->SetCaretBrowsingModeGlobal(true, false);
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();

            VerifyTextBlockCaretPosition(textBlock, 2);
            TestServices::KeyboardHelper->Left();   //move left, make sure it doesn't go to hidden character
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);
            TestServices::KeyboardHelper->Right();
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 3);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

            for (int i = 0; i < 10; i++){  //move to the end of content
                TestServices::KeyboardHelper->Right();
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    VerifyTextBlockCaretPosition(textBlock, 4 + i);
                });
            }
            VerifyTextBlockCaretPosition(textBlock, 13);
            TestServices::KeyboardHelper->Right();   //move to the end and try to pass it
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 13);
            TestServices::KeyboardHelper->Left();   //move back
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 12);
            for (int i = 0; i < 10; i++){  //move to beginning
                TestServices::KeyboardHelper->Left();
                TestServices::WindowHelper->WaitForIdle();
                VerifyTextBlockCaretPosition(textBlock, 11 - i);
            }
        }

        void CaretBrowsingTests::SelectionMovingByChar()
        {
            TestCleanupWrapper cleanup;
            auto testGuard = wil::scope_exit([] {
                TestServices::WindowHelper->SetCaretBrowsingModeGlobal(false, false);
            });
            TextBlock^ textBlock;
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));
            CreateSingleLineTextBlock(textBlock);
            TestServices::WindowHelper->SetCaretBrowsingModeGlobal(true, false);
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VerifyElementHasCaretChild(textBlock);
            });

            auto rootKeyDownRegistration = CreateSafeEventRegistration(xaml_controls::TextBlock, KeyDown);
            rootKeyDownRegistration.Attach(textBlock, ref new KeyEventHandler([&](Platform::Object^, KeyRoutedEventArgs^ args)
            {
                LOG_OUTPUT(L"The key pressed was %i", args->Key);
            }));

            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_right#$u$_right#$u$_shift");
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockSelection(textBlock, 2, 3);
            for (int i = 0; i < 10; i++){  //move to the end of content
                TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_right#$u$_right#$u$_shift");
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    VerifyTextBlockSelection(textBlock, 2, 4 + i);
                });
            }
            VerifyTextBlockSelection(textBlock, 2, 13);
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_right#$u$_right#$u$_shift");  //move to the end and try to pass it
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockSelection(textBlock, 2, 13);
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_left#$u$_left#$u$_shift");  //move back
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockSelection(textBlock, 2, 12);
            for (int i = 0; i < 10; i++){  //move to beginning
                TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_left#$u$_left#$u$_shift");
                TestServices::WindowHelper->WaitForIdle();
                VerifyTextBlockSelection(textBlock, 2, 11 - i);
            }
        }

        void CaretBrowsingTests::CaretMovingByWord()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            auto testGuard = wil::scope_exit([] {
                TestServices::WindowHelper->SetCaretBrowsingModeGlobal(false, false);
            });
            TextBlock^ textBlock;
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));
            TestServices::WindowHelper->SetCaretBrowsingModeGlobal(true, false);

            // First try a simple single-line TextBlock
            LOG_OUTPUT(L"Testing single-line TextBlock");
            CreateSingleLineTextBlock(textBlock);
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_left#$u$_left#$u$_ctrl");   //move left, make sure it doesn't go to hidden character
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_right#$u$_right#$u$_ctrl");
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 8);
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_right#$u$_right#$u$_ctrl");
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 13);
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_right#$u$_right#$u$_ctrl");    //move to the end and try to pass it
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 13);
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_left#$u$_left#$u$_ctrl");    //move back
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 8);

            // Now try a TextBlock which is explicitly multi-line
            LOG_OUTPUT(L"Testing explicitly multi-line TextBlock");
            CreateMultiLineTextBlock(textBlock);
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_left#$u$_left#$u$_ctrl");   // move left, make sure it doesn't go to hidden character
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_right#$u$_right#$u$_ctrl");
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 7);
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_right#$u$_right#$u$_ctrl");
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 11);
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_right#$u$_right#$u$_ctrl");
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 16);
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_right#$u$_right#$u$_ctrl");  // try to go past the end -- shouldn't move
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 16);
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_left#$u$_left#$u$_ctrl");    // move back
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 11);
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_left#$u$_left#$u$_ctrl");
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 9); // note: not the same as the 7 we got when going forward, but visually the same
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_left#$u$_left#$u$_ctrl");
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);
        }

        void CaretBrowsingTests::SelectionMovingByWord()
        {
            TestCleanupWrapper cleanup;
            auto testGuard = wil::scope_exit([] {
                TestServices::WindowHelper->SetCaretBrowsingModeGlobal(false, false);
            });
            TextBlock^ textBlock;
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));
            CreateSingleLineTextBlock(textBlock);
            TestServices::WindowHelper->SetCaretBrowsingModeGlobal(true, false);
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);

            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_ctrl#$d$_left#$u$_left#$u$_ctrl#$u$_shift");   //move left, make sure it doesn't go to hidden character
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockSelection(textBlock, 2, 2);
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_ctrl#$d$_right#$u$_right#$u$_ctrl#$u$_shift");
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockSelection(textBlock, 2, 8);
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_ctrl#$d$_right#$u$_right#$u$_ctrl#$u$_shift");
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockSelection(textBlock, 2, 13);
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_ctrl#$d$_right#$u$_right#$u$_ctrl#$u$_shift");    //move to the end and try to pass it
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockSelection(textBlock, 2, 13);
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_ctrl#$d$_left#$u$_left#$u$_ctrl#$u$_shift");   //move left, make sure it doesn't go to hidden character
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockSelection(textBlock, 2, 8);    //move back
        }


        void CaretBrowsingTests::CaretMovingByLine()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            auto testGuard = wil::scope_exit([] {
                TestServices::WindowHelper->SetCaretBrowsingModeGlobal(false, false);
            });
            TextBlock^ textBlock;
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));
            TestServices::WindowHelper->SetCaretBrowsingModeGlobal(true, false);

            // First try a TextBlock which is explicitly multi-line
            LOG_OUTPUT(L"Testing explicitly multi-line TextBlock");
            CreateMultiLineTextBlock(textBlock);
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);
            TestServices::KeyboardHelper->PressKeySequence(HOME_KEY);   // move left, make sure it doesn't go to hidden character
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);
            TestServices::KeyboardHelper->PressKeySequence(END_KEY);
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 7);
            TestServices::KeyboardHelper->PressKeySequence(END_KEY);    // move to the end and try to pass it
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 7);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->PressKeySequence(HOME_KEY);   // move back
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);

            // Now try a TextBlock which is wrapping to be multi-line
            LOG_OUTPUT(L"Testing wrapping TextBlock");
            CreatWrappingTextBlock(textBlock);
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);
            TestServices::KeyboardHelper->PressKeySequence(HOME_KEY);   // move left, make sure it doesn't go to hidden character
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);
            TestServices::KeyboardHelper->PressKeySequence(END_KEY);
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 45);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"45end"); // verify caret rendering at 45 end of line
            TestServices::KeyboardHelper->PressKeySequence(END_KEY);    // go to end again and ensure nothing changed
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 45);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"45end"); // verify caret rendering at 45 end of line
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->PressKeySequence(HOME_KEY);   // move back
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);
            TestServices::KeyboardHelper->Down();                       // move down
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 45);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"45start"); // verify caret rendering at 45 start of line
            TestServices::KeyboardHelper->PressKeySequence(END_KEY);
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 86);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"86end"); // verify caret rendering at 86 end of line
            TestServices::KeyboardHelper->PressKeySequence(END_KEY);    // go to end again and ensure nothing changed
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 86);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"86end"); // verify caret rendering at 86 end of line
            TestServices::KeyboardHelper->Left();                       // move left
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 85);
            TestServices::KeyboardHelper->PressKeySequence(END_KEY);    // go back to end
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 86);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"86end"); // verify caret rendering at 86 end of line
            TestServices::KeyboardHelper->PressKeySequence(HOME_KEY);   // go to start of line
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 45);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"45start"); // verify caret rendering at 45 start of line
            TestServices::KeyboardHelper->PressKeySequence(HOME_KEY);   // go to start of line and ensure nothing changed
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 45);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"45start"); // verify caret rendering at 45 start of line
        }

        void CaretBrowsingTests::SelectionMovingByLine()
        {
            TestCleanupWrapper cleanup;
            auto testGuard = wil::scope_exit([] {
                TestServices::WindowHelper->SetCaretBrowsingModeGlobal(false, false);
            });
            TextBlock^ textBlock;
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));
            CreateMultiLineTextBlock(textBlock);
            TestServices::WindowHelper->SetCaretBrowsingModeGlobal(true, false);
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);
            TestServices::KeyboardHelper->PressKeySequence(SHIFT_HOME_KEY);   // move left, make sure it doesn't go to hidden character
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);
            TestServices::KeyboardHelper->PressKeySequence(SHIFT_END_KEY);
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockSelection(textBlock, 2, 7);
            TestServices::KeyboardHelper->PressKeySequence(SHIFT_END_KEY);    // move to the end and try to pass it
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockSelection(textBlock, 2, 7);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->PressKeySequence(SHIFT_HOME_KEY);   // move back
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);
        }

        void CaretBrowsingTests::CaretMovingByContent()
        {
            TestCleanupWrapper cleanup;
            auto testGuard = wil::scope_exit([] {
                TestServices::WindowHelper->SetCaretBrowsingModeGlobal(false, false);
            });
            TextBlock^ textBlock;
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));
            CreateMultiLineTextBlock(textBlock);
            TestServices::WindowHelper->SetCaretBrowsingModeGlobal(true, false);
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);
            TestServices::KeyboardHelper->PressKeySequence(CTRL_HOME_KEY);   // move left, make sure it doesn't go to hidden character
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);
            TestServices::KeyboardHelper->PressKeySequence(CTRL_END_KEY);
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 16);
            TestServices::KeyboardHelper->PressKeySequence(CTRL_END_KEY);    // move to the end and try to pass it
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 16);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->PressKeySequence(CTRL_HOME_KEY);   // move back
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);
        }

        void CaretBrowsingTests::SelectionMovingByContent()
        {
            TestCleanupWrapper cleanup;
            auto testGuard = wil::scope_exit([] {
                TestServices::WindowHelper->SetCaretBrowsingModeGlobal(false, false);
            });
            TextBlock^ textBlock;
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));
            CreateMultiLineTextBlock(textBlock);
            TestServices::WindowHelper->SetCaretBrowsingModeGlobal(true, false);
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();

            VerifyTextBlockCaretPosition(textBlock, 2);
            TestServices::KeyboardHelper->PressKeySequence(SHIFT_CONTROL_HOME_KEY);   // move left, make sure it doesn't go to hidden character
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);
            TestServices::KeyboardHelper->PressKeySequence(SHIFT_CONTROL_END_KEY);
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockSelection(textBlock, 2, 16);
            TestServices::KeyboardHelper->PressKeySequence(SHIFT_CONTROL_END_KEY);    // move to the end and try to pass it
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockSelection(textBlock, 2, 16);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->PressKeySequence(SHIFT_CONTROL_HOME_KEY);   // move back
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);
        }

        void CaretBrowsingTests::CaretMovingUpAndDown()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            auto testGuard = wil::scope_exit([] {
                TestServices::WindowHelper->SetCaretBrowsingModeGlobal(false, false);
            });
            TextBlock^ textBlock;
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));
            TestServices::WindowHelper->SetCaretBrowsingModeGlobal(true, false);

            // First try a TextBlock which is explicitly multi-line
            CreateMultiLineTextBlock(textBlock);
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);
            TestServices::KeyboardHelper->Up();   //move up, make sure it doesn't go to hidden character
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);
            TestServices::KeyboardHelper->Down();
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 11);
            TestServices::KeyboardHelper->Down();     //move to the end and try to pass it
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 11);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->Up();    //move back
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);

            // Now try a TextBlock which is wrapping to be multi-line
            LOG_OUTPUT(L"Testing wrapping TextBlock");
            CreatWrappingTextBlock(textBlock);
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);
            TestServices::KeyboardHelper->Down();           // move down
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 45);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"45start"); // verify caret rendering at 45 start of line
            TestServices::KeyboardHelper->Down();           // move down
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 86);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"86start"); // verify caret rendering at 86 start of line
            TestServices::KeyboardHelper->Down();           // move down
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 134);
            TestServices::KeyboardHelper->Up();             // move back up
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 86);
            TestServices::KeyboardHelper->Up();             // move back up
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 45);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"45start"); // verify caret rendering at 45 start of line
            TestServices::KeyboardHelper->Up();             // move back up
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);

            // Test moving down from a long line to a shorter line.
            LOG_OUTPUT(L"Testing wrapping TextBlock, moving down from long line to shorter lines");
            TestServices::KeyboardHelper->Down();           // move down
            TestServices::KeyboardHelper->Down();           // move down
            TestServices::KeyboardHelper->Down();           // move down
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 134);
            TestServices::KeyboardHelper->PressKeySequence(END_KEY);     // go to the end of the line
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 177);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"177end"); // verify caret rendering at 177 end of line
            TestServices::KeyboardHelper->Down();           // move down
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 218);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"218end"); // verify caret rendering at 218 end of line
            TestServices::KeyboardHelper->Down();           // move down
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 248);

            // Test moving up from a long line to a shorter line.
            LOG_OUTPUT(L"Testing wrapping TextBlock, moving up from long line to shorter lines");
            TestServices::KeyboardHelper->PressKeySequence(CTRL_HOME_KEY);   // move back to the beginning
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);
            TestServices::KeyboardHelper->Down();           // move down
            TestServices::KeyboardHelper->Down();           // move down
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 86);
            TestServices::KeyboardHelper->PressKeySequence(END_KEY);     // go to the end of the line
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 134);
            TestServices::KeyboardHelper->Up();             // move up
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 86);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"86end"); // verify caret rendering at 86 end of line
            TestServices::KeyboardHelper->Up();             // move up
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 45);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"45end"); // verify caret rendering at 45 end of line
            TestServices::KeyboardHelper->Up();             // move up, should do nothing since we're already at the top line
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 45);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"45end"); // verify caret rendering at 45 end of line
        }

        void CaretBrowsingTests::SelectionMovingUpAndDown()
        {
            TestCleanupWrapper cleanup;
            auto testGuard = wil::scope_exit([] {
                TestServices::WindowHelper->SetCaretBrowsingModeGlobal(false, false);
            });
            TextBlock^ textBlock;
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));
            CreateMultiLineTextBlock(textBlock);
            TestServices::WindowHelper->SetCaretBrowsingModeGlobal(true, false);
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();

            VerifyTextBlockCaretPosition(textBlock, 2);
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_up#$u$_up#$u$_shift");   //move left, make sure it doesn't go to hidden character
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_down#$u$_down#$u$_shift");
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockSelection(textBlock, 2, 11);
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_down#$u$_down#$u$_shift");     //move to the end and try to pass it
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockSelection(textBlock, 2, 11);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_up#$u$_up#$u$_shift");    //move back
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);
        }

        void CaretBrowsingTests::RichTextBlockCaretMovingUpAndDown()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            auto testGuard = wil::scope_exit([] {
                TestServices::WindowHelper->SetCaretBrowsingModeGlobal(false, false);
            });
            RichTextBlock^ richTextBlock;
            RichTextBlockOverflow^ rtbo;
            RichTextBlockOverflow^ rtbo2;
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));
            TestServices::WindowHelper->SetCaretBrowsingModeGlobal(true, false);

            // Create RichTextBlock with some RichTextBlockOverflows
            CreateRichTextBlockWithOverflows(richTextBlock, rtbo, rtbo2);
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
            VerifyRichTextBlockCaretPosition(richTextBlock, 3);
            TestServices::KeyboardHelper->Down();           // move down
            TestServices::WindowHelper->WaitForIdle();
            VerifyRichTextBlockCaretPosition(richTextBlock, 33);
            TestServices::KeyboardHelper->PressKeySequence(END_KEY);   // move to end of line
            TestServices::WindowHelper->WaitForIdle();
            VerifyRichTextBlockCaretPosition(richTextBlock, 65);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"65end"); // verify caret rendering at 65 end of line
            TestServices::KeyboardHelper->PressKeySequence(HOME_KEY);   // move back to start of line
            TestServices::KeyboardHelper->Down();           // move down
            TestServices::WindowHelper->WaitForIdle();
            VerifyRichTextBlockCaretPosition(richTextBlock, 65);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"65start"); // verify caret rendering at 65 start of line
            TestServices::KeyboardHelper->Down();           // move down
            TestServices::WindowHelper->WaitForIdle();
            VerifyRichTextBlockCaretPosition(richTextBlock, 94);
            TestServices::KeyboardHelper->Down();           // move down
            TestServices::WindowHelper->WaitForIdle();
            VerifyRichTextBlockCaretPosition(richTextBlock, 119);

            // Move down into overflow 1
            TestServices::KeyboardHelper->Down();           // move down
            TestServices::WindowHelper->WaitForIdle();
            VerifyRichTextBlockCaretPosition(rtbo, 150);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"150start"); // verify caret rendering at 150 start of line
            TestServices::KeyboardHelper->Down();           // move down
            TestServices::WindowHelper->WaitForIdle();
            VerifyRichTextBlockCaretPosition(rtbo, 164);

            // Move down into overflow 2
            TestServices::KeyboardHelper->Down();           // move down
            TestServices::WindowHelper->WaitForIdle();
            VerifyRichTextBlockCaretPosition(rtbo2, 186);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"186start"); // verify caret rendering at 186 start of line

            // Move back up into overflow 1
            TestServices::KeyboardHelper->Up();             // move up
            TestServices::WindowHelper->WaitForIdle();
            VerifyRichTextBlockCaretPosition(rtbo, 164);
            TestServices::KeyboardHelper->Up();             // move up
            TestServices::WindowHelper->WaitForIdle();
            VerifyRichTextBlockCaretPosition(rtbo, 150);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"150start"); // verify caret rendering at 150 start of line

            // Move up into the RichTextBlock
            TestServices::KeyboardHelper->Up();             // move up
            TestServices::WindowHelper->WaitForIdle();
            VerifyRichTextBlockCaretPosition(richTextBlock, 119);

            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_a");  // Select All
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"selectall");
            TestServices::KeyboardHelper->Up();             // move up
            TestServices::WindowHelper->WaitForIdle();
            VerifyRichTextBlockCaretPosition(rtbo2, 199);
        }

        void CaretBrowsingTests::RichTextBlockCaretMoving()
        {
            TestCleanupWrapper cleanup;
            auto testGuard = wil::scope_exit([] {
                TestServices::WindowHelper->SetCaretBrowsingModeGlobal(false, false);
            });
            RichTextBlock^ richTextBlock;
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));
            CreateRichTextBlock(richTextBlock);
            TestServices::WindowHelper->SetCaretBrowsingModeGlobal(true, false);
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();

            VerifyRichTextBlockCaretPosition(richTextBlock, 2);
            TestServices::KeyboardHelper->Left();   //move left, make sure it doesn't go to hidden character
            TestServices::WindowHelper->WaitForIdle();
            VerifyRichTextBlockCaretPosition(richTextBlock, 2);

            for (int i = 0; i < 11; i++){  //move to the end of content
                TestServices::KeyboardHelper->Right();
                TestServices::WindowHelper->WaitForIdle();
                VerifyRichTextBlockCaretPosition(richTextBlock, i + 3);
            }
            VerifyRichTextBlockCaretPosition(richTextBlock, 13);
            TestServices::KeyboardHelper->Right();   //move to nextline
            TestServices::WindowHelper->WaitForIdle();
            VerifyRichTextBlockCaretPosition(richTextBlock, 15);
            for (int i = 0; i < 13; i++){  //move over UIContainner
                TestServices::KeyboardHelper->Right();
                TestServices::WindowHelper->WaitForIdle();
            }
            VerifyRichTextBlockCaretPosition(richTextBlock, 31);
            TestServices::KeyboardHelper->Right();   //move to nextline
            TestServices::WindowHelper->WaitForIdle();
            VerifyRichTextBlockCaretPosition(richTextBlock, 35);

        }


        void CaretBrowsingTests::RemoveSelectionShowCaret()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            auto testGuard = wil::scope_exit([] {
                TestServices::WindowHelper->SetCaretBrowsingModeGlobal(false, false);
            });
            TextBlock^ textBlock;
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));
            CreateSingleLineTextBlock(textBlock);
            TestServices::WindowHelper->SetCaretBrowsingModeGlobal(true, false);
            TestServices::InputHelper->LeftMouseClick(textBlock);
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VerifyElementHasCaretChild(textBlock);
            });

            int savedOffset;
            RunOnUIThread([&]()
            {
                savedOffset = textBlock->SelectionEnd->Offset;
            });
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_right#$u$_right#$u$_shift");
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_right#$u$_right#$u$_shift");
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->Left();
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, savedOffset);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_right#$u$_right#$u$_shift");
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_right#$u$_right#$u$_shift");
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->Right();
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, savedOffset + 2);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }

        void CaretBrowsingTests::CaretMovingInForceWrappedTextBlock()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            auto testGuard = wil::scope_exit([] {
                TestServices::WindowHelper->SetCaretBrowsingModeGlobal(false, false);
            });
            TextBlock^ textBlock;
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));
            TestServices::WindowHelper->SetCaretBrowsingModeGlobal(true, false);

            CreatForceWrappedTextBlock(textBlock);
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);
            TestServices::KeyboardHelper->PressKeySequence(END_KEY);
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 18);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"18end"); // verify caret rendering at 18 end of line
            TestServices::KeyboardHelper->PressKeySequence(END_KEY);     // go to end again and ensure nothing changed
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 18);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"18end"); // verify caret rendering at 18 end of line
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->PressKeySequence(HOME_KEY);   // move back
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);
            TestServices::KeyboardHelper->Down();           // move down
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 18);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"18start"); // verify caret rendering at 18 start of line
            TestServices::KeyboardHelper->PressKeySequence(END_KEY);
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 33);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"33end"); // verify caret rendering at 33 end of line
            TestServices::KeyboardHelper->Down();           // move down
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 36);

            LOG_OUTPUT(L"Go back to the start, and test arrowing around the end of line");
            TestServices::KeyboardHelper->PressKeySequence(CTRL_HOME_KEY);   // go back to the start
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);
            TestServices::KeyboardHelper->PressKeySequence(END_KEY);
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 18);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"18end"); // verify caret rendering at 18 end of line
            TestServices::KeyboardHelper->Right();           // move right
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 19);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"19"); // verify caret rendering at 19 (no ambiguity)
            TestServices::KeyboardHelper->Right();           // move right
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 20);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"20"); // verify caret rendering at 20 (no ambiguity)
            TestServices::KeyboardHelper->Left();            // move back left
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 19);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"19"); // verify caret rendering at 19 (no ambiguity)
            TestServices::KeyboardHelper->Left();            // move back left
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 18);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"18start"); // verify caret rendering at 18 start of line
            TestServices::KeyboardHelper->Left();            // move back left
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 17);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"17"); // verify caret rendering at 17 (no ambiguity)
            TestServices::KeyboardHelper->Right();           // move right, back to the start of next line
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 18);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"18start"); // verify caret rendering at 18 start of line
        }

        void CaretBrowsingTests::CaretMovingInTextBlockHyperLink()
        {
            TestCleanupWrapper cleanup;
            auto testGuard = wil::scope_exit([] {
                TestServices::WindowHelper->SetCaretBrowsingModeGlobal(false, false);
            });
            TextBlock^ textBlock;
            Hyperlink^ hyperlink;
            Hyperlink^ hyperlink2;
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));
            TestServices::WindowHelper->SetCaretBrowsingModeGlobal(true, false);
            RunOnUIThread([&]()
            {
                auto rootCanvas = safe_cast<xaml_controls::Canvas^>(xaml_markup::XamlReader::Load(
                    L"<Canvas Background='Black' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"<StackPanel VerticalAlignment='Top'> "
                    L"  <TextBlock x:Name='mytb' FontFamily='Segoe UI' IsTextSelectionEnabled='True'>This is text block with "
                    L"     <Hyperlink x:Name='myhl' NavigateUri='http://www.bing.com' FontFamily='Segoe UI'>Hyperlink1</Hyperlink> <LineBreak/>and "
                    L"     <Hyperlink x:Name='myhl2' NavigateUri='http://www.microsoft.com'>Hyperlink2</Hyperlink></TextBlock>"
                    L"</StackPanel>"
                    L"</Canvas>"));
                textBlock = safe_cast<TextBlock^>(rootCanvas->FindName(L"mytb"));
                hyperlink = safe_cast<Hyperlink^>(rootCanvas->FindName(L"myhl"));
                hyperlink2 = safe_cast<Hyperlink^>(rootCanvas->FindName(L"myhl2"));
                VERIFY_IS_NOT_NULL(textBlock);
                VERIFY_IS_NOT_NULL(hyperlink);
                VERIFY_IS_NOT_NULL(hyperlink2);
                TestServices::WindowHelper->WindowContent = rootCanvas;
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::InputHelper->LeftMouseClick(textBlock);
            TestServices::WindowHelper->WaitForIdle();

            VerifyTextBlockCaretPosition(textBlock, 20);
            TestServices::KeyboardHelper->PressKeySequence(CTRL_HOME_KEY);   // move to home
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 2);

            for (int i = 0; i < 10; i++){  //moving
                TestServices::KeyboardHelper->Right();
                TestServices::WindowHelper->WaitForIdle();
                VerifyTextBlockCaretPosition(textBlock, i + 3);
            }
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(textBlock->SelectionStart->Offset, 28);
                VERIFY_ARE_EQUAL(textBlock->SelectionEnd->Offset, 28);
                VerifyElementHasCaretChild(textBlock);
                VERIFY_IS_TRUE(FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(hyperlink));
            });
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(textBlock->SelectionStart->Offset, 50);
                VERIFY_ARE_EQUAL(textBlock->SelectionEnd->Offset, 50);
                VerifyElementHasCaretChild(textBlock);
                VERIFY_IS_TRUE(FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(hyperlink2));
            });
            TestServices::KeyboardHelper->Right();
            TestServices::WindowHelper->WaitForIdle();
            VerifyTextBlockCaretPosition(textBlock, 52);
        }

        void CaretBrowsingTests::PopContentDialogTurnOnCaretBrowsing()
        {
            TestCleanupWrapper cleanup;
            auto testGuard = wil::scope_exit([] {
                TestServices::WindowHelper->SetCaretBrowsingModeGlobal(false, false);
            });
            TextBlock^ textBlock;
            RichTextBlock^ richTextBlock;
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));
            CreateTextBlockAndRichTextBlock(richTextBlock, textBlock);
            TestServices::KeyboardHelper->PressKeySequence(F7_KEY);    //pop the dialog

            xaml_primitives::ButtonBase^ primaryButton;

            RunOnUIThread([&]()
            {
                auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(textBlock->XamlRoot);
                UIElement ^ uielement = (popups->GetAt(0)->Child);
                ContentDialog^ contentDialog = safe_cast<ContentDialog^>(uielement);
                primaryButton = safe_cast<xaml_primitives::ButtonBase^>(TreeHelper::GetVisualChildByName(contentDialog, L"PrimaryButton"));
            });

            TestServices::InputHelper->LeftMouseClick(primaryButton);
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                //  checking for the cursor child
                VerifyElementHasCaretChild(textBlock);
                VERIFY_ARE_EQUAL(VisualTreeHelper::GetChildrenCount(richTextBlock), 0);
            });

            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(VisualTreeHelper::GetChildrenCount(textBlock), 0);
                VerifyElementHasCaretChild(richTextBlock);
            });
        }

        void CaretBrowsingTests::PopContentDialogTurnOffCaretBrowsing()
        {
            TestCleanupWrapper cleanup;
            auto testGuard = wil::scope_exit([] {
                TestServices::WindowHelper->SetCaretBrowsingModeGlobal(false, false);
            });
            TextBlock^ textBlock;
            RichTextBlock^ richTextBlock;
            TestServices::WindowHelper->SetCaretBrowsingModeGlobal(true, false);
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));
            CreateTextBlockAndRichTextBlock(richTextBlock, textBlock);
            TestServices::KeyboardHelper->PressKeySequence(F7_KEY);    //pop the dialog

            xaml_primitives::ButtonBase^ primaryButton;

            RunOnUIThread([&]()
            {
                auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(textBlock->XamlRoot);
                UIElement ^ uielement = (popups->GetAt(0)->Child);
                ContentDialog^ contentDialog = safe_cast<ContentDialog^>(uielement);
                primaryButton = safe_cast<xaml_primitives::ButtonBase^>(TreeHelper::GetVisualChildByName(contentDialog, L"PrimaryButton"));
            });

            TestServices::InputHelper->LeftMouseClick(primaryButton);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(VisualTreeHelper::GetChildrenCount(textBlock), 0);
                VERIFY_ARE_EQUAL(VisualTreeHelper::GetChildrenCount(richTextBlock), 0);
            });
        }

        void CaretBrowsingTests::PopContentDialogTurnOnCaretBrowsingWithCheckBox()
        {
            TestCleanupWrapper cleanup;
            auto testGuard = wil::scope_exit([] {
                TestServices::WindowHelper->SetCaretBrowsingModeGlobal(false, false);
            });
            TextBlock^ textBlock;
            RichTextBlock^ richTextBlock;
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));
            CreateTextBlockAndRichTextBlock(richTextBlock, textBlock);
            TestServices::KeyboardHelper->PressKeySequence(F7_KEY);    //pop the dialog
            TestServices::WindowHelper->WaitForIdle();

            xaml_primitives::ButtonBase^ primaryButton;
            xaml_controls::CheckBox^ chkBox;

            RunOnUIThread([&]()
            {
                auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(textBlock->XamlRoot);
                UIElement ^ uielement = (popups->GetAt(0)->Child);
                ContentDialog^ contentDialog = safe_cast<ContentDialog^>(uielement);
                primaryButton = safe_cast<xaml_primitives::ButtonBase^>(TreeHelper::GetVisualChildByName(contentDialog, L"PrimaryButton"));
                chkBox = safe_cast<xaml_controls::CheckBox^>(TreeHelper::GetVisualChildByName(contentDialog, L"notPopAgainCheckBox"));
            });
            TestServices::InputHelper->LeftMouseClick(chkBox);
            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->LeftMouseClick(primaryButton);
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VerifyElementHasCaretChild(textBlock);
                VERIFY_ARE_EQUAL(VisualTreeHelper::GetChildrenCount(richTextBlock), 0);
            });

            TestServices::KeyboardHelper->PressKeySequence(F7_KEY);    //Turn It off
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(VisualTreeHelper::GetChildrenCount(textBlock), 0);
                VERIFY_ARE_EQUAL(VisualTreeHelper::GetChildrenCount(richTextBlock), 0);
            });
        }

        void CaretBrowsingTests::CreateSingleLineTextBlock(TextBlock^& textBlock)
        {
            RunOnUIThread([&]()
            {
                auto rootCanvas = safe_cast<xaml_controls::Canvas^>(xaml_markup::XamlReader::Load(
                    L"<Canvas Background='Black' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"<StackPanel VerticalAlignment='Top'> "
                    L"  <TextBlock x:Name='mytb' IsTextSelectionEnabled='True' Text='Hello World' FontSize='15'/>"
                    L"</StackPanel>"
                    L"</Canvas>"));
                textBlock = safe_cast<TextBlock^>(rootCanvas->FindName(L"mytb"));
                VERIFY_IS_NOT_NULL(textBlock);
                TestServices::WindowHelper->WindowContent = rootCanvas;
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void CaretBrowsingTests::CreateMultiLineTextBlock(TextBlock^& textBlock)
        {
            RunOnUIThread([&]()
            {
                auto rootCanvas = safe_cast<xaml_controls::Canvas^>(xaml_markup::XamlReader::Load(
                    L"<Canvas Background='Black' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"<StackPanel VerticalAlignment='Top'> "
                    L"  <TextBlock x:Name='mytb' IsTextSelectionEnabled='True' FontSize='15'>Hello<LineBreak/> World</TextBlock>"
                    L"</StackPanel>"
                    L"</Canvas>"));
                textBlock = safe_cast<TextBlock^>(rootCanvas->FindName(L"mytb"));
                VERIFY_IS_NOT_NULL(textBlock);
                TestServices::WindowHelper->WindowContent = rootCanvas;
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void CaretBrowsingTests::CreatWrappingTextBlock(TextBlock^& textBlock)
        {
            RunOnUIThread([&]()
            {
                auto rootCanvas = safe_cast<xaml_controls::Canvas^>(xaml_markup::XamlReader::Load(
                    L"<Canvas Background='Black' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"<StackPanel VerticalAlignment='Top'> "
                    L"  <TextBlock x:Name='mytb' IsTextSelectionEnabled='True' FontSize='15' TextWrapping='Wrap' Width='300' Text='This project sounds easy, but it is not. I spend lots of time learning source code, design the feature before start writing code. I conquered lots of unexpected problems, and changed my initial design to make better performance and simplier code.'/>"
                    L"</StackPanel>"
                    L"</Canvas>"));
                textBlock = safe_cast<TextBlock^>(rootCanvas->FindName(L"mytb"));
                VERIFY_IS_NOT_NULL(textBlock);
                TestServices::WindowHelper->WindowContent = rootCanvas;
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void CaretBrowsingTests::CreatForceWrappedTextBlock(TextBlock^& textBlock)
        {
            RunOnUIThread([&]()
            {
                auto rootCanvas = safe_cast<xaml_controls::Canvas^>(xaml_markup::XamlReader::Load(
                    L"<Canvas Background='Black' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"<StackPanel VerticalAlignment='Top'> "
                    L"  <TextBlock x:Name='mytb' IsTextSelectionEnabled='True' FontSize='15' TextWrapping='Wrap' Width='100' Text='supercalifragilisticexpialidocious'/>"
                    L"</StackPanel>"
                    L"</Canvas>"));
                textBlock = safe_cast<TextBlock^>(rootCanvas->FindName(L"mytb"));
                VERIFY_IS_NOT_NULL(textBlock);
                TestServices::WindowHelper->WindowContent = rootCanvas;
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void CaretBrowsingTests::CreateRichTextBlock(RichTextBlock^& richTextBlock)
        {
            RunOnUIThread([&]()
            {
                auto rootCanvas = safe_cast<xaml_controls::Canvas^>(xaml_markup::XamlReader::Load(
                    L"<Canvas Background='Black' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"<StackPanel VerticalAlignment='Top'> "
                    L"  <RichTextBlock x:Name='myrtb' IsTextSelectionEnabled='True' FontSize='15'>"
                    L"      <Paragraph>Hello World</Paragraph>"
                    L"      <Paragraph>Hello World2</Paragraph>"
                    L"      <Paragraph>"
                    L"          <InlineUIContainer>"
                    L"              <Border Background='Red'>"
                    L"                  <Button Content='btnHello in rich'></Button>"
                    L"              </Border>"
                    L"          </InlineUIContainer>"
                    L"      </Paragraph>"
                    L"      <Paragraph>Hello World3</Paragraph>"
                    L"  </RichTextBlock>"
                    L"</StackPanel>"
                    L"</Canvas>"));
                richTextBlock = safe_cast<RichTextBlock^>(rootCanvas->FindName(L"myrtb"));
                VERIFY_IS_NOT_NULL(richTextBlock);
                TestServices::WindowHelper->WindowContent = rootCanvas;
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void CaretBrowsingTests::CreateRichTextBlockWithOverflows(RichTextBlock^& richTextBlock, RichTextBlockOverflow^& rtbo, RichTextBlockOverflow^& rtbo2)
        {
            RunOnUIThread([&]()
            {
                auto rootCanvas = safe_cast<xaml_controls::Canvas^>(xaml_markup::XamlReader::Load(
                    L"<Canvas Background='Black' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"<StackPanel>"
                    L"  <RichTextBlock x:Name='myrtb' IsTextSelectionEnabled='True' FontSize='15' Width='200' Height='100' HorizontalAlignment='Left' OverflowContentTarget='{Binding ElementName = rtbo}'>"
                    L"      <Paragraph><Span>This project sounds easy, but it is not. I spend lots of time learning source code, design the feature before start writing code. I conquered lots of unexpected problems, and changed my initial design to make better performance and simplier code. Right now is two third of the whole internship, and the last two week is for internship ending activities such as presentation and intern event, so my schedule is tight.</Span></Paragraph>"
                    L"  </RichTextBlock>"
                    L"  <RichTextBlockOverflow x:Name='rtbo' Width='160' Height='40' HorizontalAlignment='Left' Margin='10,10,0,0' OverflowContentTarget='{Binding ElementName=rtbo2}' />"
                    L"  <RichTextBlockOverflow x:Name='rtbo2' Width='160' Height='50' HorizontalAlignment='Left' Margin='25,10,0,0' />"
                    L"</StackPanel>"
                    L"</Canvas>"));
                richTextBlock = safe_cast<RichTextBlock^>(rootCanvas->FindName(L"myrtb"));
                rtbo = safe_cast<RichTextBlockOverflow^>(rootCanvas->FindName(L"rtbo"));
                rtbo2 = safe_cast<RichTextBlockOverflow^>(rootCanvas->FindName(L"rtbo2"));
                VERIFY_IS_NOT_NULL(richTextBlock);
                VERIFY_IS_NOT_NULL(rtbo);
                VERIFY_IS_NOT_NULL(rtbo2);
                TestServices::WindowHelper->WindowContent = rootCanvas;
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void CaretBrowsingTests::CreateTextBlockAndRichTextBlock(RichTextBlock^& richTextBlock, TextBlock^& textBlock)
        {
            RunOnUIThread([&]()
            {
                auto rootCanvas = safe_cast<xaml_controls::Canvas^>(xaml_markup::XamlReader::Load(
                    L"<Canvas Background='Black' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"<StackPanel VerticalAlignment='Top'> "
                    L"  <TextBlock x:Name='mytb' IsTextSelectionEnabled='True' Text='Hello World' FontSize='15'/>"
                    L"  <RichTextBlock x:Name='myrtb' IsTextSelectionEnabled='True' FontSize='15'>"
                    L"      <Paragraph>this is some sample text</Paragraph>"
                    L"  </RichTextBlock>"
                    L"</StackPanel>"
                    L"</Canvas>"));
                textBlock = safe_cast<TextBlock^>(rootCanvas->FindName(L"mytb"));
                VERIFY_IS_NOT_NULL(textBlock);
                richTextBlock = safe_cast<RichTextBlock^>(rootCanvas->FindName(L"myrtb"));
                VERIFY_IS_NOT_NULL(richTextBlock);
                TestServices::WindowHelper->WindowContent = rootCanvas;
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void CaretBrowsingTests::CreateNonSelectableTBAndSelectableRTB(RichTextBlock^& richTextBlock, TextBlock^& textBlock)
        {
            RunOnUIThread([&]()
            {
                auto rootCanvas = safe_cast<xaml_controls::Canvas^>(xaml_markup::XamlReader::Load(
                    L"<Canvas Background='Black' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"<StackPanel VerticalAlignment='Top'> "
                    L"  <TextBlock x:Name='mytb' IsTextSelectionEnabled='False' Text='Hello World' FontSize='15'/>"
                    L"  <RichTextBlock x:Name='myrtb' IsTextSelectionEnabled='True' FontSize='15'>"
                    L"      <Paragraph>this is some sample text</Paragraph>"
                    L"  </RichTextBlock>"
                    L"</StackPanel>"
                    L"</Canvas>"));
                textBlock = safe_cast<TextBlock^>(rootCanvas->FindName(L"mytb"));
                VERIFY_IS_NOT_NULL(textBlock);
                richTextBlock = safe_cast<RichTextBlock^>(rootCanvas->FindName(L"myrtb"));
                VERIFY_IS_NOT_NULL(richTextBlock);
                TestServices::WindowHelper->WindowContent = rootCanvas;
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void CaretBrowsingTests::VerifyTextBlockCaretPosition(xaml_controls::TextBlock^ textBlock, int offset)
        {
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(textBlock->SelectionStart->Offset, offset);
                VERIFY_ARE_EQUAL(textBlock->SelectionEnd->Offset, offset);
                //  checking for the cursor child
                VerifyElementHasCaretChild(textBlock);
                VERIFY_IS_TRUE(FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(textBlock));

                // Note: You might think this code could/should also check the SelectionStart->LogicalDirection
                // to see if it has the expected Backward/Forward direction, but for empty selections
                // CTextSelection::GetStartTextPosition/GetEndTextPosition always returns m_anchorPosition, which
                // CTextSelection::Select() always sets to LineForwardCharacterBackward gravity.  So the result
                // is that LogicalDirection is always Backward when there is just a caret.
            });
        }

        void CaretBrowsingTests::VerifyRichTextBlockCaretPosition(xaml_controls::RichTextBlock^ richTextBlock, int offset)
        {
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(richTextBlock->SelectionStart->Offset, offset);
                VERIFY_ARE_EQUAL(richTextBlock->SelectionEnd->Offset, offset);
                VERIFY_IS_TRUE(FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(richTextBlock));
            });
        }

        void CaretBrowsingTests::VerifyRichTextBlockCaretPosition(xaml_controls::RichTextBlockOverflow^ richTextBlockOverflow, int offset)
        {
            RunOnUIThread([&]()
            {
                RichTextBlock^ richTextBlock = richTextBlockOverflow->ContentSource;
                VERIFY_ARE_EQUAL(richTextBlock->SelectionStart->Offset, offset);
                VERIFY_ARE_EQUAL(richTextBlock->SelectionEnd->Offset, offset);
                VERIFY_IS_TRUE(FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(richTextBlockOverflow));
            });
        }


        void CaretBrowsingTests::VerifyTextBlockSelection(xaml_controls::TextBlock^ textBlock, int startOffset, int endOffset)
        {
            if (startOffset == endOffset)
            {
                VerifyTextBlockCaretPosition(textBlock, startOffset);
            }
            else
            {
                RunOnUIThread([&]()
                {
                    VERIFY_ARE_EQUAL(textBlock->SelectionStart->Offset, startOffset);
                    VERIFY_ARE_EQUAL(textBlock->SelectionEnd->Offset, endOffset);
                    //  checking for the cursor Rectangle child
                    VERIFY_ARE_EQUAL(VisualTreeHelper::GetChildrenCount(textBlock), 0);
                    VERIFY_IS_TRUE(FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(textBlock));
                });
            }

        }

        void CaretBrowsingTests::VerifyRichTextBlockSelection(xaml_controls::RichTextBlock^ richTextBlock, int startOffset, int endOffset)
        {
            if (startOffset == endOffset)
            {
                VerifyRichTextBlockCaretPosition(richTextBlock, startOffset);
            }
            else
            {
                RunOnUIThread([&]()
                {
                    VERIFY_ARE_EQUAL(richTextBlock->SelectionStart->Offset, startOffset);
                    VERIFY_ARE_EQUAL(richTextBlock->SelectionEnd->Offset, endOffset);
                    VERIFY_IS_TRUE(FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(richTextBlock));
                });
            }
        }

    } }
} } } }
