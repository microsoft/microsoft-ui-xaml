// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "RichTextBlockTests.h"
#include <AutomationClient\AutomationClientManager.h>
#include <AutomationClient\AutomationEventHandler.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include <SafeEventRegistration.h>
#include <UIAutomationCore.h>
#include <WUCRenderingScopeGuard.h>
#include <ClipboardHelper.h>
#include <FocusTestHelper.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Documents;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Media;
using namespace ::Windows::Foundation;
using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;
using namespace Microsoft::UI::Xaml::Automation::Peers;
using namespace Microsoft::UI::Xaml::Tests::Automation;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics {


        Platform::String^ RichTextBlockTests::GetResourcesPath() const
        {
            return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\rendering\\";
        }
        bool RichTextBlockTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool RichTextBlockTests::TestSetup()
        {
            TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool RichTextBlockTests::TestCleanup()
        {
            TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void RichTextBlockTests::SpanWithEmptyInlineCollection()
        {
            TestCleanupWrapper cleanup;
            RichTextBlock^ rtb = nullptr;
            Paragraph^ p = nullptr;
            Span^ s = nullptr;
            TextPointer^ tp = nullptr;
            InlineCollection^ inlines = nullptr;
            RunOnUIThread([&]()
             {
                 rtb = ref new RichTextBlock();
                 p = ref new Paragraph();
                 s = ref new Span();
                 p->Inlines->Append(s);
                 rtb->Blocks->Append(p);
                 // Span will create its InlineCollection Lazily here, the collection is empty.
                 inlines = s->Inlines;
                 tp = s->ContentStart;
                 VERIFY_IS_NOT_NULL(tp, L"TextPointer should be able to be created without problem");
             });
        }

        void RichTextBlockTests::ParagraphWithEmptyInlineCollection()
        {
            TestCleanupWrapper cleanup;
            RichTextBlock^ rtb = nullptr;
            Paragraph^ p = nullptr;
            TextPointer^ tp = nullptr;
            InlineCollection^ inlines = nullptr;
            RunOnUIThread([&]()
             {
                 rtb = ref new RichTextBlock();
                 p = ref new Paragraph();
                 rtb->Blocks->Append(p);
                 // Paragraph will create its InlineCollection Lazily here, the collection is empty.
                 inlines = p->Inlines;
                 tp = p->ContentStart;
                 VERIFY_IS_NOT_NULL(tp, L"TextPointer should be able to be created without problem");
             });
        }

        void RichTextBlockTests::EmbeddedInlineElementPlacement()
        {
            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = nullptr;
            });
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(500, 300));

            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "RichTextBlockInlineUI.xaml"));
            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = root;
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "1");

            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(300, 500));

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "2");
        }

        void RichTextBlockTests::EmbeddedInlineElementPlacementBidi()
        {
              RunOnUIThread([&]()
              {
                  TestServices::WindowHelper->WindowContent = nullptr;
              });
              WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
              TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

              Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "RTBInlineUIBidi.xaml"));
              RunOnUIThread([&]()
              {
                  TestServices::WindowHelper->WindowContent = root;
              });

              TestServices::WindowHelper->WaitForIdle();
              TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

              TestServices::WindowHelper->WaitForIdle();
          }

        void RichTextBlockTests::NWRenderWUCFull()
        {
            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = nullptr;
            });
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(500, 300));

            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "RichTextBlockHyperlink.xaml"));
            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = root;
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }

        void RichTextBlockTests::DoubleTap()
        {
            RichTextBlock^ richTextBlock = nullptr;
            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = nullptr;
            });
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(500, 300));

            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "RichTextBlockHyperlink.xaml"));
            RunOnUIThread([&]()
            {
                richTextBlock = safe_cast<RichTextBlock^>(root->FindName(L"myRTB"));
                VERIFY_IS_NOT_NULL(richTextBlock);
                TestServices::WindowHelper->WindowContent = root;
            });

            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->DoubleTap(richTextBlock);

            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }

        void RichTextBlockTests::CopySelection_Keyboard()
        {
            TestCleanupWrapper cleanup;

            TextBlock^ textBlock = nullptr;
            RichTextBlock^ richTextBlock = nullptr;
            RichTextBlock^ richTextBlockWithOverflow = nullptr;
            TextBox^ textBox1 = nullptr;
            TextBox^ textBox2 = nullptr;
            TextBox^ textBox3 = nullptr;

            Platform::String ^textString1 = "I'm a TextBlock";
            Platform::String ^textString2 = "I'm a RichTextBlock";
            Platform::String ^textString3 = "I'm a RichTextBlockOverflow";

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = nullptr;
            });

            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(500, 300));

            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "TextCopyTests.xaml"));
            RunOnUIThread([&]()
            {
                textBlock = safe_cast<TextBlock^>(root->FindName(L"textBlock"));
                richTextBlock = safe_cast<RichTextBlock^>(root->FindName(L"richTextBlock"));
                richTextBlockWithOverflow = safe_cast<RichTextBlock^>(root->FindName(L"richTextBlockWithOverflow"));
                textBox1 = safe_cast<TextBox^>(root->FindName(L"textBox1"));
                textBox2 = safe_cast<TextBox^>(root->FindName(L"textBox2"));
                textBox3 = safe_cast<TextBox^>(root->FindName(L"textBox3"));
                VERIFY_IS_NOT_NULL(textBlock);
                VERIFY_IS_NOT_NULL(richTextBlock);
                VERIFY_IS_NOT_NULL(richTextBlockWithOverflow);
                VERIFY_IS_NOT_NULL(textBox1);
                VERIFY_IS_NOT_NULL(textBox2);
                VERIFY_IS_NOT_NULL(textBox3);
                TestServices::WindowHelper->WindowContent = root;
            });

            TestServices::WindowHelper->WaitForIdle();

            // Then select a range.
            RunOnUIThread([&]()
            {
                TextPointer^ start = textBlock->ContentStart;
                TextPointer^ end = textBlock->ContentEnd;
                VERIFY_IS_TRUE(start->LogicalDirection == LogicalDirection::Backward, L"default tb.ContentStart.LogicalDirection is not Backward.");
                VERIFY_IS_TRUE(end->LogicalDirection == LogicalDirection::Forward, L"default tb.ContentEnd.LogicalDirection is not Forward.");

                LOG_OUTPUT(L"textBlock - Focus and Select");
                textBlock->Focus(FocusState::Pointer);
                textBlock->Select(start, end);
            });

            TestServices::WindowHelper->WaitForIdle();

            // Copy to Clipboard by pressing control + c;
            LOG_OUTPUT(L"Press Ctrl+C");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_c#$u$_c#$u$_ctrl");

            LOG_OUTPUT(L"Focus textBox1");
            FocusTestHelper::EnsureFocus(textBox1, FocusState::Programmatic);

            // Paste to TextBox by pressing control + v;
            LOG_OUTPUT(L"Press Ctrl+V");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_v#$u$_v#$u$_ctrl");

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(textBox1->Text, textBlock->Text);
            });

            TestServices::WindowHelper->WaitForIdle();

            // Then select all.
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"richTextBlock - Focus and SelectAll");
                richTextBlock->Focus(FocusState::Pointer);
                richTextBlock->SelectAll();
            });

            TestServices::WindowHelper->WaitForIdle();

            // Copy to Clipboard by pressing control + c;
            LOG_OUTPUT(L"Press Ctrl+C");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_c#$u$_c#$u$_ctrl");

            LOG_OUTPUT(L"Focus textBox2");
            FocusTestHelper::EnsureFocus(textBox2, FocusState::Programmatic);

            // Paste to TextBox by pressing control + v;
            LOG_OUTPUT(L"Press Ctrl+C");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_v#$u$_v#$u$_ctrl");

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(textBox2->Text, textString2);
            });

            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
            TestServices::WindowHelper->WaitForIdle();

            // Then select all.
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"richTextBlockWithOverflow - Focus and SelectAll");
                richTextBlockWithOverflow->Focus(FocusState::Pointer);
                richTextBlockWithOverflow->SelectAll();
            });

            TestServices::WindowHelper->WaitForIdle();

            // Copy to Clipboard by pressing control + c;
            LOG_OUTPUT(L"Press Ctrl+C");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_c#$u$_c#$u$_ctrl");

            LOG_OUTPUT(L"Focus textBox3");
            FocusTestHelper::EnsureFocus(textBox3, FocusState::Programmatic);

            // Paste to TextBox by pressing control + v;
            LOG_OUTPUT(L"Press Ctrl+V");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_v#$u$_v#$u$_ctrl");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(textBox3->Text, textString3);
            });

            TestServices::WindowHelper->SynchronouslyTickUIThread(2);

            TestServices::WindowHelper->WaitForIdle();
        }

        void RichTextBlockTests::CopySelection_API()
        {
            TestCleanupWrapper cleanup;

            TextBlock^ textBlock = nullptr;
            RichTextBlock^ richTextBlock = nullptr;
            RichTextBlock^ richTextBlockWithOverflow = nullptr;
            TextBox^ textBox1 = nullptr;
            TextBox^ textBox2 = nullptr;
            TextBox^ textBox3 = nullptr;

            Platform::String^ textString1 = "I'm a TextBlock";
            Platform::String^ textString2 = "I'm a RichTextBlock";
            Platform::String^ textString3 = "I'm a RichTextBlockOverflow";

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = nullptr;
            });

            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(500, 300));

            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "TextCopyTests.xaml"));

            RunOnUIThread([&]()
            {
                textBlock = safe_cast<TextBlock^>(root->FindName(L"textBlock"));
                richTextBlock = safe_cast<RichTextBlock^>(root->FindName(L"richTextBlock"));
                richTextBlockWithOverflow = safe_cast<RichTextBlock^>(root->FindName(L"richTextBlockWithOverflow"));
                textBox1 = safe_cast<TextBox^>(root->FindName(L"textBox1"));
                textBox2 = safe_cast<TextBox^>(root->FindName(L"textBox2"));
                textBox3 = safe_cast<TextBox^>(root->FindName(L"textBox3"));
                VERIFY_IS_NOT_NULL(textBlock);
                VERIFY_IS_NOT_NULL(richTextBlock);
                VERIFY_IS_NOT_NULL(richTextBlockWithOverflow);
                VERIFY_IS_NOT_NULL(textBox1);
                VERIFY_IS_NOT_NULL(textBox2);
                VERIFY_IS_NOT_NULL(textBox3);
                TestServices::WindowHelper->WindowContent = root;
            });

            TestServices::WindowHelper->WaitForIdle();

            // Then select a range.
            RunOnUIThread([&]()
            {
                TextPointer^ start = textBlock->ContentStart;
                TextPointer^ end = textBlock->ContentEnd;
                VERIFY_IS_TRUE(start->LogicalDirection == LogicalDirection::Backward, L"default tb.ContentStart.LogicalDirection is not Backward.");
                VERIFY_IS_TRUE(end->LogicalDirection == LogicalDirection::Forward, L"default tb.ContentEnd.LogicalDirection is not Forward.");

                LOG_OUTPUT(L"textBlock - Focus and Select");
                textBlock->Focus(FocusState::Pointer);
                textBlock->Select(start, end);
            });

            TestServices::WindowHelper->WaitForIdle();

            ClipboardHelper clipboardHelper;

            TestServices::WindowHelper->WaitForIdle();

            // Copy to Clipboard by calling CopySelectionToClipboard()
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"textBlock->CopySelectionToClipboard()");
                textBlock->CopySelectionToClipboard();
            });

            TestServices::WindowHelper->WaitForIdle();

            clipboardHelper.WaitForContentChangedEvent();

            TestServices::WindowHelper->WaitForIdle();

            clipboardHelper.VerifyClipboardText(textString1);
            clipboardHelper.ResetContentChangedEvent();

            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Focus textBox1");
            FocusTestHelper::EnsureFocus(textBox1, FocusState::Programmatic);

            // Paste to TextBox by pressing control + v;
            LOG_OUTPUT(L"Press Ctrl+V");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_v#$u$_v#$u$_ctrl");

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(textBox1->Text, textBlock->Text);
            });

            TestServices::WindowHelper->WaitForIdle();

            // Then select all.
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"richTextBlock - Focus and SelectAll");
                richTextBlock->Focus(FocusState::Pointer);
                richTextBlock->SelectAll();
            });

            // Copy to Clipboard by calling CopySelectionToClipboard()
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"richTextBlock->CopySelectionToClipboard()");
                richTextBlock->CopySelectionToClipboard();
            });

            TestServices::WindowHelper->WaitForIdle();

            clipboardHelper.VerifyClipboardText(textString2);
            clipboardHelper.ResetContentChangedEvent();

            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Focus textBox2");
            FocusTestHelper::EnsureFocus(textBox2, FocusState::Programmatic);

            // Paste to TextBox by pressing control + v;
            LOG_OUTPUT(L"Press Ctrl+V");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_v#$u$_v#$u$_ctrl");

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(textBox2->Text, textString2);
            });

            TestServices::WindowHelper->SynchronouslyTickUIThread(2);

            TestServices::WindowHelper->WaitForIdle();

            // Then select all.
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"richTextBlockWithOverflow - Focus and SelectAll");
                richTextBlockWithOverflow->Focus(FocusState::Pointer);
                richTextBlockWithOverflow->SelectAll();
            });

            TestServices::WindowHelper->WaitForIdle();

            // Copy to Clipboard by calling CopySelectionToClipboard()
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"richTextBlockWithOverflow->CopySelectionToClipboard()");
                richTextBlockWithOverflow->CopySelectionToClipboard();
            });

            TestServices::WindowHelper->WaitForIdle();

            clipboardHelper.VerifyClipboardText(textString3 + "\r\n");
            clipboardHelper.ResetContentChangedEvent();

            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Focus textBox13");
            FocusTestHelper::EnsureFocus(textBox3, FocusState::Programmatic);

            // Paste to TextBox by pressing control + v;
            LOG_OUTPUT(L"Press Ctrl+V");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_v#$u$_v#$u$_ctrl");

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(textBox3->Text, textString3);
            });

            TestServices::WindowHelper->SynchronouslyTickUIThread(2);

            TestServices::WindowHelper->WaitForIdle();
        }

        void RichTextBlockTests::SelectionChangedEvent()
        {
            TestCleanupWrapper cleanup;

            RichTextBlock^ richTextBlock = nullptr;

            auto textSelectedChangedEvent = std::make_shared<Event>();
            auto textSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::RichTextBlock, SelectionChanged);

            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "TextBlockEventTests.xaml"));
            RunOnUIThread([&]()
            {
                richTextBlock = safe_cast<RichTextBlock^>(root->FindName(L"richTextBlock"));
                VERIFY_IS_NOT_NULL(richTextBlock);

                TestServices::WindowHelper->WindowContent = root;

                textSelectionChangedRegistration.Attach(
                    richTextBlock,
                    ref new xaml::RoutedEventHandler(
                    [textSelectedChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    textSelectedChangedEvent->Set();
                }));
            });
            TestServices::WindowHelper->WaitForIdle();

            // Select some text by click and dragging
            TestServices::InputHelper->DragToCenter(richTextBlock, -100 /*relX*/, 0 /*relY*/, 0.5 /*velocityFactor*/);

            // Verify some text was selected
            textSelectedChangedEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
            VERIFY_IS_TRUE(textSelectedChangedEvent->HasFired());

            RunOnUIThread([&]()
            {
                // take richTextBlock of the tree
                UINT index;
                root->Children->IndexOf(richTextBlock, &index);
                root->Children->RemoveAt(index);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                root->Children->Append(richTextBlock);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                richTextBlock->SelectAll();
            });
            // Verify some text was selected
            textSelectedChangedEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
            VERIFY_IS_TRUE(textSelectedChangedEvent->HasFired());
        }

        void RichTextBlockTests::TextUpdatesWithFocus()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(500, 500));

            RichTextBlock^ richTextBlock = nullptr;
            RichTextBlockOverflow^ richTextBlockOverflow = nullptr;

            // Helper function to add text
            auto AddText = [](RichTextBlock^ richTextBlock, Platform::String^ text) {
                LOG_OUTPUT(L"Adding Text: %s", text->Data());
                RunOnUIThread([&]()
                {
                    Paragraph^ p = ref new Paragraph();
                    Run^ r = ref new Run();
                    r->Text = text;
                    p->Inlines->Append(r);
                    richTextBlock->Blocks->Append(p);
                });
                TestServices::WindowHelper->WaitForIdle();
            };

            // Helper function to change Character Spacing
            auto ChangeSpacing = [](RichTextBlock^ richTextBlock, int spacing) {
                LOG_OUTPUT(L"Changing character spacing to Adding Text: %d", spacing);
                RunOnUIThread([&]()
                {
                    richTextBlock->CharacterSpacing = spacing;
                });
                TestServices::WindowHelper->WaitForIdle();
            };

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <RichTextBlock x:Name='myrtb' OverflowContentTarget='{Binding ElementName=myrtbo}' MaxLines='3' IsTextSelectionEnabled='True'>"
                    L"    <Paragraph>Test Text</Paragraph>"
                    L"  </RichTextBlock>"
                    L"  <RichTextBlockOverflow x:Name='myrtbo' Margin='10,10,0,0'/>"
                    L"</StackPanel>"));
                richTextBlock = safe_cast<RichTextBlock^>(rootPanel->FindName(L"myrtb"));
                richTextBlockOverflow = safe_cast<RichTextBlockOverflow^>(rootPanel->FindName(L"myrtbo"));
                VERIFY_IS_NOT_NULL(richTextBlock);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Setting focus to text block and selecting text");
            RunOnUIThread([&]()
            {
                TextPointer^ start = richTextBlock->ContentStart;
                TextPointer^ end = richTextBlock->ContentEnd;
                richTextBlock->Focus(FocusState::Pointer);
                richTextBlock->Select(start, end);
            });
            TestServices::WindowHelper->WaitForIdle();

            // Change the text formatting to get a new arrange
            ChangeSpacing(richTextBlock, 1);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"FormatChange");

            // First addition of text will verify that we render correctly if the selection changes
            // since adding text will collapse the selection to the start.
            AddText(richTextBlock, L"More Test Text");
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"SelectionChange");

            // Second addition will verify that we render correctly if the selection doesn't change
            // since it will still be collapsed at the start from the previous add.
            AddText(richTextBlock, L"Even More Test Text");
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"NoSelectionChange");

            // Add a fourth line that will run into the overflow area.
            AddText(richTextBlock, L"Overflow Text");
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Setting focus to the overflow text block and selecting text");
            RunOnUIThread([&]()
            {
                TextPointer^ start = richTextBlockOverflow->ContentStart;
                TextPointer^ end = richTextBlockOverflow->ContentEnd;
                richTextBlockOverflow->Focus(FocusState::Pointer);
                richTextBlock->Select(start, end);
            });
            TestServices::WindowHelper->WaitForIdle();

            // Change the text formatting to get a new arrange
            ChangeSpacing(richTextBlock, 2);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"OverflowFormatChange");

            // First addition of text will verify that we render correctly if the selection changes
            // since adding text will collapse the selection to the start.
            AddText(richTextBlock, L"More Overflow Text");
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"OverflowSelectionChange");

            // Second addition will verify that we render correctly if the selection doesn't change
            // since it will still be collapsed at the start from the previous add.
            AddText(richTextBlock, L"Even More Overflow Text");
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"OverflowNoSelectionChange");

        }

        void RichTextBlockTests::ClearEmbeddedElements()
        {
            TestCleanupWrapper cleanup;
            RichTextBlock^ rtb = nullptr;
            Paragraph^ p = nullptr;
            Span^ s = nullptr;
            InlineUIContainer^ container = nullptr;

            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));
            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "TextBlockEventTests.xaml"));

            RunOnUIThread([&]()
             {
                 // The RichTextBlock is intentionally not added to the visual tree.
                 rtb = ref new RichTextBlock();
                 p = ref new Paragraph();
                 s = ref new Span();
                 container = ref new InlineUIContainer();

                 rtb->Blocks->Append(p);
                 p->Inlines->Append(s);
                 s->Inlines->Append(container);

                 TextBlock^ tb = ref new TextBlock();
                 tb->Text = "Hello World";

                 container->Child = tb;

                 Size limit{ 100, 100 };
                 rtb->Measure(limit);

                 TestServices::WindowHelper->WindowContent = root;
                 // The InlineUIContainer is released, it should detach itself from its host (RichTextBlock).
                 s->Inlines->Clear();
             });

            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
             {
                 // Now release the RichTextBlock, the previous InlineUIContainer should not present in its embedded element list.
                 rtb = nullptr;
             });
            TestServices::WindowHelper->WaitForIdle();
        }

        void RichTextBlockTests::GetArrtributeFromDocumentRange()
        {
            TestCleanupWrapper cleanup;
            RichTextBlock^ richTextBlock = nullptr;
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"myrtb";
            uiaInfo.m_AutomationID = L"myrtb";
            uiaInfo.m_cType = UIA_TextControlTypeId;
            wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;
            wrl::ComPtr<IUIAutomationTextPattern> spTextPattern;
            POINT point = { 0, 0 };
            AutoVariant varData;
            Platform::Object^ prop;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <RichTextBlock x:Name='myrtb'>"
                    L"    <Paragraph>"
                    L"        Test"
                    L"    </Paragraph>"
                    L"  </RichTextBlock>"
                    L"</StackPanel>"));
                richTextBlock = safe_cast<RichTextBlock^>(rootPanel->FindName(L"myrtb"));
                VERIFY_IS_NOT_NULL(richTextBlock);
                xaml_automation::AutomationProperties::SetName(richTextBlock, ref new Platform::String(uiaInfo.m_Name));
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
                auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
                spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
                VERIFY_IS_NOT_NULL(spUIAutomationElement.Get());

                VERIFY_SUCCEEDED(spUIAutomationElement->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &spTextPattern),
                    L"TextIntegrationTests::VerifyRangeFromPoint: Failed in retreiving Text Pattern.");
                WEX::Common::Throw::IfNull(spTextPattern.Get(), L"TextIntegrationTests::VerifyRangeFromPoint: This TextBlock doesn't support Text Pattern which is required.");

                VERIFY_SUCCEEDED(spTextPattern->get_DocumentRange(&spUIAutomationTextRange)); // get the document range.
                VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetAttributeValue(UIA_IsReadOnlyAttributeId, varData.ReleaseAndGetAddressOf()));
                VERIFY_ARE_EQUAL(varData.Storage()->vt, VT_BOOL);
                VERIFY_IS_TRUE(varData.Storage()->boolVal == VARIANT_TRUE);
            });
        }

        void RichTextBlockTests::StartEndHorizontalTextAlignment()
        {
            TestCleanupWrapper cleanup;

            RichTextBlock^ richTextBlock = nullptr;
            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "TextBlockEventTests.xaml"));

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = root;

                richTextBlock = safe_cast<RichTextBlock^>(root->FindName(L"richTextBlock"));
                VERIFY_IS_NOT_NULL(richTextBlock);

                // HorizontalTextAlignment::Start should be the same as TextAlignment::Left
                richTextBlock->HorizontalTextAlignment = TextAlignment::Start;
                VERIFY_IS_TRUE(richTextBlock->TextAlignment == TextAlignment::Left);

                // HorizontalTextAlignment::End should be the same as TextAlignment::Right
                richTextBlock->HorizontalTextAlignment = TextAlignment::End;
                VERIFY_IS_TRUE(richTextBlock->TextAlignment == TextAlignment::Right);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void RichTextBlockTests::IsTextTrimmedRichTextBlock()
        {
            IsTextTrimmedPropertyAndEventHelper(L"richTextBlock", L"THIS IS SOME TEXT THIS IS SOME TEXT");
            IsTextTrimmedBindingHelper(L"richTextBlock", L"THIS IS SOME TEXT THIS IS SOME TEXT");
        }

        void RichTextBlockTests::IsTextTrimmedRichTextBlockOverflow()
        {
            IsTextTrimmedPropertyAndEventHelper(L"richTextBlockOverflow", L"THIS IS SOME TEXT THIS IS SOME TEXT THIS IS SOME TEXT");
            IsTextTrimmedBindingHelper(L"richTextBlockOverflow", L"THIS IS SOME TEXT THIS IS SOME TEXT THIS IS SOME TEXT");
        }

        void RichTextBlockTests::IsTextTrimmedPropertyAndEventHelper(Platform::String^ textBlockName, Platform::String^ textContent)
        {
            TestCleanupWrapper cleanup;

            RichTextBlock^ richTextBlock = nullptr;
            RichTextBlockOverflow^ richTextBlockOverflow = nullptr;
            double originalWidth = 0;
            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "TextBlockEventTests.xaml"));

            auto textBlockIsTextTrimmedChangedEvent = std::make_shared<Event>();
            auto richTextBlockIsTextTrimmedChangedRegistration = CreateSafeEventRegistration(xaml_controls::RichTextBlock, IsTextTrimmedChanged);
            auto richTextBlockOverflowIsTextTrimmedChangedRegistration = CreateSafeEventRegistration(xaml_controls::RichTextBlockOverflow, IsTextTrimmedChanged);

            RunOnUIThread([&]()
            {
                richTextBlock = safe_cast<RichTextBlock^>(root->FindName(L"richTextBlock"));
                richTextBlockOverflow = safe_cast<RichTextBlockOverflow^>(root->FindName(L"richTextBlockOverflow"));
                VERIFY_IS_NOT_NULL(richTextBlock);
                VERIFY_IS_NOT_NULL(richTextBlockOverflow);

                // Turn on text trimming
                richTextBlock->TextTrimming = TextTrimming::CharacterEllipsis;
                TestServices::WindowHelper->WindowContent = root;

                if (textBlockName->Equals(L"richTextBlock")) {
                    richTextBlockIsTextTrimmedChangedRegistration.Attach(
                        richTextBlock,
                        ref new ::Windows::Foundation::TypedEventHandler<RichTextBlock^, IsTextTrimmedChangedEventArgs^>(
                            [textBlockIsTextTrimmedChangedEvent](Platform::Object^ sender, xaml_controls::IsTextTrimmedChangedEventArgs^ args)
                    {
                        // Event has been triggered
                        textBlockIsTextTrimmedChangedEvent->Set();
                    }));
                }
                else
                {
                    richTextBlockOverflowIsTextTrimmedChangedRegistration.Attach(
                        richTextBlockOverflow,
                        ref new ::Windows::Foundation::TypedEventHandler<RichTextBlockOverflow^, IsTextTrimmedChangedEventArgs^>(
                            [textBlockIsTextTrimmedChangedEvent](Platform::Object^ sender, xaml_controls::IsTextTrimmedChangedEventArgs^ args)
                    {
                        // Event has been triggered
                        textBlockIsTextTrimmedChangedEvent->Set();
                    }));
                }

            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Original text should not be trimmed
                if (textBlockName->Equals(L"richTextBlock"))
                {
                    VERIFY_IS_TRUE(!(richTextBlock->IsTextTrimmed));
                }
                else
                {
                    VERIFY_IS_TRUE(!(richTextBlockOverflow->IsTextTrimmed));
                }
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Change the trimming state to true
                originalWidth = richTextBlock->Width;
                richTextBlock->Width = 100.0;
                richTextBlockOverflow->Width = 100.0;

                richTextBlock->Blocks->Clear();
                Paragraph^ p = ref new Paragraph();
                Run^ r = ref new Run();
                r->Text = textContent;
                p->Inlines->Append(r);
                richTextBlock->Blocks->Append(p);
            });
            TestServices::WindowHelper->WaitForIdle();

            // Verify the event fired the first time
            textBlockIsTextTrimmedChangedEvent->WaitForDefault();
            VERIFY_IS_TRUE(textBlockIsTextTrimmedChangedEvent->HasFired());
            textBlockIsTextTrimmedChangedEvent->Reset();

            RunOnUIThread([&]()
            {
                // Text now should be trimmed
                if (textBlockName->Equals(L"richTextBlock"))
                {
                    VERIFY_IS_TRUE(richTextBlock->IsTextTrimmed);
                }
                else
                {
                    VERIFY_IS_TRUE(richTextBlockOverflow->IsTextTrimmed);
                }
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Change the trimming back to false
                richTextBlock->Width = originalWidth;
                richTextBlockOverflow->Width = originalWidth;

                richTextBlock->Blocks->Clear();
                Paragraph^ p = ref new Paragraph();
                Run^ r = ref new Run();
                r->Text = "short text";
                p->Inlines->Append(r);
                richTextBlock->Blocks->Append(p);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Text should not be trimmed again
                if (textBlockName->Equals(L"richTextBlock"))
                {
                    VERIFY_IS_TRUE(!(richTextBlock->IsTextTrimmed));
                }
                else
                {
                    VERIFY_IS_TRUE(!(richTextBlockOverflow->IsTextTrimmed));
                }
            });
            TestServices::WindowHelper->WaitForIdle();

            // Verify the event fired again
            textBlockIsTextTrimmedChangedEvent->WaitForDefault();
            VERIFY_IS_TRUE(textBlockIsTextTrimmedChangedEvent->HasFired());

        }

        void RichTextBlockTests::IsTextTrimmedBindingHelper(Platform::String^ textBlockName, Platform::String^ textContent)
        {
            TestCleanupWrapper cleanup;

            RichTextBlock^ richTextBlock = nullptr;
            RichTextBlockOverflow^ richTextBlockOverflow = nullptr;
            Microsoft::UI::Xaml::Data::Binding^ binding = nullptr;
            double originalWidth = 0;
            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "TextBlockEventTests.xaml"));

            RunOnUIThread([&]()
            {
                richTextBlock = safe_cast<RichTextBlock^>(root->FindName(L"richTextBlock"));
                richTextBlockOverflow = safe_cast<RichTextBlockOverflow^>(root->FindName(L"richTextBlockOverflow"));
                VERIFY_IS_NOT_NULL(richTextBlock);
                VERIFY_IS_NOT_NULL(richTextBlockOverflow);

                // Turn on text trimming
                richTextBlock->TextTrimming = TextTrimming::CharacterEllipsis;
                // Bind IsTextSelectionEnabled to IsTextTrimmed
                binding = ref new Microsoft::UI::Xaml::Data::Binding();
                binding->Path = ref new PropertyPath(L"IsTextTrimmed");
                binding->ElementName = textBlockName;
                richTextBlock->SetBinding(RichTextBlock::IsTextSelectionEnabledProperty, binding);

                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Original text should not be trimmed
                if (textBlockName->Equals(L"richTextBlock"))
                {
                    VERIFY_IS_TRUE(!(richTextBlock->IsTextTrimmed));
                }
                else
                {
                    VERIFY_IS_TRUE(!(richTextBlockOverflow->IsTextTrimmed));
                }
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Change the trimming state to true
                originalWidth = richTextBlock->Width;
                richTextBlock->Width = 100;
                richTextBlockOverflow->Width = 100;
                Paragraph^ p = ref new Paragraph();
                Run^ r = ref new Run();
                r->Text = textContent;
                p->Inlines->Append(r);
                richTextBlock->Blocks->Clear();
                richTextBlock->Blocks->Append(p);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Text now should be trimmed
                if (textBlockName->Equals(L"richTextBlock"))
                {
                    VERIFY_IS_TRUE(richTextBlock->IsTextTrimmed);
                }
                else
                {
                    VERIFY_IS_TRUE(richTextBlockOverflow->IsTextTrimmed);
                }
                // IsTextSelectionEnabled should have changed via binding
                VERIFY_IS_TRUE(richTextBlock->IsTextSelectionEnabled);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Change the trimming back to false
                richTextBlock->Width = originalWidth;
                richTextBlockOverflow->Width = originalWidth;
                Paragraph^ p = ref new Paragraph();
                Run^ r = ref new Run();
                r->Text = "short text";
                p->Inlines->Append(r);
                richTextBlock->Blocks->Clear();
                richTextBlock->Blocks->Append(p);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Text should not be trimmed again
                if (textBlockName->Equals(L"richTextBlock"))
                {
                    VERIFY_IS_TRUE(!(richTextBlock->IsTextTrimmed));
                }
                else
                {
                    VERIFY_IS_TRUE(!(richTextBlockOverflow->IsTextTrimmed));
                }
                // IsTextSelectionEnabled should have changed via binding
                VERIFY_IS_TRUE(!(richTextBlock->IsTextSelectionEnabled));
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void RichTextBlockTests::DoNotRenderTooManyDiacritics()
        {
            TestCleanupWrapper cleanup;

            RichTextBlock^ richTextBlock = nullptr;
            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "RichTextBlockDiacritics.xaml"));

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = root;

                richTextBlock = safe_cast<RichTextBlock^>(root->FindName(L"richTextBlock"));
                VERIFY_IS_NOT_NULL(richTextBlock);

            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void RichTextBlockTests::JupRtblXamlP0()
        {
            TestCleanupWrapper cleanup;

            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(500, 500));

            xaml_controls::RichTextBlock^ richTextBlock;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::Canvas^> (xaml_markup::XamlReader::Load(
                    L"<Canvas x:Name='parentCanvas'"
                    L"        xmlns='http://schemas.microsoft.com/client/2007' "
                    L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                    L"        Width='480' Height='600' Background='Orange'>"
                    L"    <Grid x:Name='LayoutRoot' Background='DarkOrchid' Width='480' Height='500'>"
                    L"         <Grid.ColumnDefinitions>"
                    L"             <ColumnDefinition Width='*' />"
                    L"             <ColumnDefinition Width='*' />"
                    L"         </Grid.ColumnDefinitions>"
                    L"         <Grid.RowDefinitions>"
                    L"             <RowDefinition Height='*' />"
                    L"            <RowDefinition Height='*' />"
                    L"        </Grid.RowDefinitions>"
                    L"        <RichTextBlock Name='rtbl' Grid.Row='0' Foreground='Black' TextWrapping='Wrap' OverflowContentTarget='{Binding ElementName=rtblo1}' FontFamily='Times New Roman' FontSize='14'>"
                    L"             <Paragraph >"
                    L"                 <Run TextDecorations='Underline'>Hello</Run>some text here. and more test testing line height property. Add more text to produce linking, and more, and more, and more."
                    L"                 Testing RichTextBlock overflow linking control in this project."
                    L"                 <InlineUIContainer>"
                    L"                     <Button Height='36' Width='110' x:Name='InlineUIButton'>InlineUI Button</Button>"
                    L"                 </InlineUIContainer>"
                    L"                 some text here. and"
                    L"                 <Bold>more test testing line height property. Add more</Bold> text to produce linking, and more, and more, and more."
                    L"                 Testing RichTextBlock overflow linking control in this project."
                    L"                 some text here. and more test testing line height property."
                    L"                 <Hyperlink Foreground='Blue' NavigateUri='http://www.bing.com'>Add more text to produce linking.</Hyperlink>"
                    L"                 Testing RichTextBlock overflow linking control in this project."
                    L"             </Paragraph>"
                    L"             <Paragraph>"
                    L"                 Testing RichTextBlock overflow linking control in this project."
                    L"                 some text here. and more test testing line height property."
                    L"                 Additional content here."
                    L"             </Paragraph>"
                    L"         </RichTextBlock>"
                    L"        <RichTextBlockOverflow Name='rtblo1' Height='10' Grid.Row='1'  Grid.Column='0' OverflowContentTarget='{Binding ElementName=rtblo2}'/>"
                    L"         <RichTextBlockOverflow Name='rtblo2' Grid.Column='1'/>"
                    L"    </Grid>"
                    L"</Canvas>"));
                richTextBlock = safe_cast<xaml_controls::RichTextBlock^>(rootPanel->FindName(L"rtbl"));
                VERIFY_IS_NOT_NULL(richTextBlock);

                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

            RunOnUIThread([&]()
            {
                richTextBlock->FontSize = 25;
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "FontSize");
        }

        void RichTextBlockTests::EmptyOverflowHasNullTextPatternAndCanNavigatePast()
        {
            TestCleanupWrapper cleanup;

            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"rtbo";
            uiaInfo.m_AutomationID = L"rtbo";
            uiaInfo.m_cType = UIA_TextControlTypeId;
            wrl::ComPtr<IUIAutomationTextPattern> spTextPattern;
            RichTextBlockOverflow^ overflow = nullptr;

            wrl::ComPtr<IUIAutomationElement> secondTextBlock;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <TextBlock x:Name='tb1' Text='Test1' /> "
                    L"  <RichTextBlockOverflow x:Name='rtbo' /> "
                    L"  <TextBlock x:Name='tb2' Text='Test2' /> "
                    L"</StackPanel>"));
                overflow = safe_cast<RichTextBlockOverflow^>(rootPanel->FindName(L"rtbo"));
                VERIFY_IS_NOT_NULL(overflow);
                xaml_automation::AutomationProperties::SetName(overflow, ref new Platform::String(uiaInfo.m_Name));
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                // Get automation element for the emtpty RTBO
                wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
                auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
                spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
                VERIFY_IS_NOT_NULL(spUIAutomationElement.Get());

                // Verify that the text pattern is null for the empty element
                VERIFY_SUCCEEDED(spUIAutomationElement->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &spTextPattern),
                    L"RichTextBlockTests::EmptyOverflowHasNullTextPattern: Failed in retreiving Text Pattern.");
                VERIFY_ARE_EQUAL(spTextPattern.Get(), nullptr);

                // Navigate from the empty RTBO to the next TextBlock
                wrl::ComPtr<IUIAutomation> spUIAutomation;
                spAutomationClientManager->GetAutomation(&spUIAutomation);

                wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
                wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
                LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition),
                    L"RichTextBlockTests::EmptyOverflowHasNullTextPattern: Failed in creating True PropertyCondition.");
                LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker),
                    L"RichTextBlockTests::EmptyOverflowHasNullTextPattern: Failed in creating TreeWalker.");

                spUIAutomationTreeWalker->GetNextSiblingElement(spUIAutomationElement.Get(), &secondTextBlock);
                Common::AutoVariant autoVar;
                secondTextBlock->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
                VERIFY_IS_TRUE(!wcscmp(L"Test2", (autoVar.Storage())->bstrVal));
            });
        }

        void RichTextBlockTests::EmptyRichTextBlockWithOverflowHasNoText()
        {
            TestCleanupWrapper cleanup;

            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"rtb";
            uiaInfo.m_AutomationID = L"rtb";
            uiaInfo.m_cType = UIA_TextControlTypeId;
            wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;
            wrl::ComPtr<IUIAutomationTextPattern> spTextPattern;
            RichTextBlock^ rtb = nullptr;
            RichTextBlockOverflow^ overflow = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <TextBlock x:Name='tb1' Text='Test1' /> "
                    L"  <RichTextBlock x:Name='rtb' OverflowContentTarget='{Binding ElementName=rtbo}' /> "
                    L"  <RichTextBlockOverflow x:Name='rtbo' /> "
                    L"  <TextBlock x:Name='tb2' Text='Test2' /> "
                    L"</StackPanel>"));
                rtb = safe_cast<RichTextBlock^>(rootPanel->FindName(L"rtb"));
                VERIFY_IS_NOT_NULL(rtb);
                overflow = safe_cast<RichTextBlockOverflow^>(rootPanel->FindName(L"rtbo"));
                VERIFY_IS_NOT_NULL(overflow);
                xaml_automation::AutomationProperties::SetName(rtb, ref new Platform::String(uiaInfo.m_Name));
                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
                auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
                spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
                VERIFY_IS_NOT_NULL(spUIAutomationElement.Get());

                VERIFY_SUCCEEDED(spUIAutomationElement->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &spTextPattern),
                    L"EmptyRichTextBlockWithOverflowHasNoText: Failed in retreiving Text Pattern.");
                VERIFY_IS_NOT_NULL(spTextPattern.Get());

                VERIFY_SUCCEEDED(spTextPattern->get_DocumentRange(&spUIAutomationTextRange)); // get the document range.
                VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());

                AutoBSTR textFromRange;
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromRange.ReleaseAndGetAddressOf()));

                bool textIsNull = textFromRange == nullptr; // True on Desktop
                LOG_OUTPUT(L"Is text null? (Should be true on Desktop): %d.", textIsNull);

                bool textIsEmpty = false;
                if (textFromRange != nullptr)
                {
                    textIsEmpty = !wcscmp(L"", textFromRange); // True on some SKUs
                    LOG_OUTPUT(L"Is text empty? (Should be true on some SKUs): %d.", textIsEmpty);
                }

                // Null and empty string both produce the desired outcome (nothing is read, Narrator continues) so accept both.
                VERIFY_IS_TRUE(textIsNull || textIsEmpty);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void RichTextBlockTests::InlineUIContainerDirections()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "InlineUIContainerDirections.xaml"));

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }

        // This test was added because when ExpandToLine() was being called on an InlineUIContainer, the wrong line's text range was being returned and messing up UIA.
        // This wasn't because of a problem in ExpandToLine, but rather because PageNode::GetElementsWithinRange was returning the wrong ranges for child elements,
        // and those incorrect ranges would affect other UIA calls, like ExpandToLine, or GetChildren.
        // This test verifies that each InlineUIContainer is the child of the correct text range, so any other UIA calls made on the IUC's range will be getting the correct range.
        void RichTextBlockTests::VerifyInlineUIContainerIsChildOfCorrectRange()
        {
            TestCleanupWrapper cleanup;

            RichTextBlock^ richTextBlock = nullptr;
            xaml_controls::StackPanel^ rootPanel = nullptr;
            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <RichTextBlock x:Name='myrtb'>"
                    L"  </RichTextBlock>"
                    L"</StackPanel>"));
                richTextBlock = safe_cast<RichTextBlock^>(rootPanel->FindName(L"myrtb"));
                VERIFY_IS_NOT_NULL(richTextBlock);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Fill in the RichTextBlock like a dictionary
                // Specifically the dictionary that comes up in edge when you "Define <word>"
                for (int i = 0; i < 3; i++)
                {
                    Paragraph^ p1 = ref new Paragraph();
                    Run^ r1 = ref new Run();
                    r1->Text = "word";
                    p1->Inlines->Append(r1);

                    Paragraph^ p2 = ref new Paragraph();
                    Run^ r2 = ref new Run();
                    r2->Text = "pronunciation";

                    // Button
                    Button^ button = ref new Button();
                    button->Name = "button" + i;
                    button->Content = "button" + i;
                    button->FontSize = 20;

                    // Put the button in a stack panel
                    StackPanel^ sp = ref new StackPanel();
                    sp->Children->Append(button);

                    // Put the stack Panel in the InlineUIContainer
                    InlineUIContainer^ iuc = ref new InlineUIContainer();
                    iuc->Child = sp;

                    p2->Inlines->Append(r2);
                    p2->Inlines->Append(iuc);

                    Paragraph^ p3 = ref new Paragraph();
                    Run^ r3 = ref new Run();
                    r3->Text = "definition";
                    p3->Inlines->Append(r3);

                    richTextBlock->Blocks->Append(p1);
                    richTextBlock->Blocks->Append(p2);
                    richTextBlock->Blocks->Append(p3);
                }
            });
            TestServices::WindowHelper->WaitForIdle();

            Automation::AutomationClient::UIAElementInfo rtbInfo;
            rtbInfo.m_Name = L"RTB";
            rtbInfo.m_AutomationID = L"RTB";

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
                wrl::ComPtr<IUIAutomationTextPattern> spTextPattern;
                wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;
                wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRangeFound;
                wrl::ComPtr<IUIAutomationElementArray> children;
                wrl::ComPtr<IUIAutomationElement> firstChild;
                wrl::ComPtr<IUIAutomationElement> secondChild;
                wrl::ComPtr<IUIAutomationElement> thirdChild;

                wrl::ComPtr<IUIAutomationTextRangeArray> spVisibleRangesArray;
                wrl::ComPtr<IUIAutomationTextRange> spRange;
                wrl::ComPtr<IUIAutomationElement> childElement;
                AutoBSTR textFromTextRange;

                RunOnUIThread([&]()
                {
                    xaml_automation::AutomationProperties::SetName(richTextBlock, ref new Platform::String(rtbInfo.m_Name));
                });
                auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(rtbInfo);

                spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
                VERIFY_IS_NOT_NULL(spUIAutomationElement.Get());
                VERIFY_SUCCEEDED(spUIAutomationElement->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &spTextPattern));
                VERIFY_IS_NOT_NULL(spTextPattern.Get());

                VERIFY_SUCCEEDED(spTextPattern->get_DocumentRange(spUIAutomationTextRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());

                // Get the three children of the document - this will be the three ButtonAutomationPeers
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetChildren(&children));
                int childCount;
                VERIFY_SUCCEEDED(children->get_Length(&childCount));
                VERIFY_IS_TRUE(childCount == 3);

                VERIFY_SUCCEEDED(children->GetElement(0, &firstChild));
                VERIFY_SUCCEEDED(children->GetElement(1, &secondChild));
                VERIFY_SUCCEEDED(children->GetElement(2, &thirdChild));

                // Get all visible ranges - There will be one range for each line
                VERIFY_SUCCEEDED(spTextPattern->GetVisibleRanges(spVisibleRangesArray.ReleaseAndGetAddressOf()));
                VERIFY_IS_NOT_NULL(spVisibleRangesArray.Get());

                // The second range (range 1) should hold the first child
                VERIFY_SUCCEEDED(spVisibleRangesArray->GetElement(1, spRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_NOT_NULL(spRange.Get());

                VERIFY_SUCCEEDED(spRange->GetChildren(&children));
                VERIFY_SUCCEEDED(children->get_Length(&childCount));
                VERIFY_IS_TRUE(childCount == 1);

                // The fifth range (range 4) should hold the second child
                VERIFY_SUCCEEDED(spVisibleRangesArray->GetElement(4, spRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_NOT_NULL(spRange.Get());

                VERIFY_SUCCEEDED(spRange->GetChildren(&children));
                VERIFY_SUCCEEDED(children->get_Length(&childCount));
                VERIFY_IS_TRUE(childCount == 1);

                // The eighth range (range 7) should hold the third child
                VERIFY_SUCCEEDED(spVisibleRangesArray->GetElement(7, spRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_NOT_NULL(spRange.Get());

                VERIFY_SUCCEEDED(spRange->GetChildren(&children));
                VERIFY_SUCCEEDED(children->get_Length(&childCount));
                VERIFY_IS_TRUE(childCount == 1);

                // Other ranges should NOT have children - before the fix, these ranges
                // would have incorrectly thought that they were holding the children
                VERIFY_SUCCEEDED(spVisibleRangesArray->GetElement(2, spRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_NOT_NULL(spRange.Get());
                VERIFY_SUCCEEDED(spRange->GetChildren(&children));
                VERIFY_SUCCEEDED(children->get_Length(&childCount));
                VERIFY_IS_TRUE(childCount == 0);

                VERIFY_SUCCEEDED(spVisibleRangesArray->GetElement(5, spRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_NOT_NULL(spRange.Get());
                VERIFY_SUCCEEDED(spRange->GetChildren(&children));
                VERIFY_SUCCEEDED(children->get_Length(&childCount));
                VERIFY_IS_TRUE(childCount == 0);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void RichTextBlockTests::RichTextBlockWordBreakerNavigation()
        {
            TestCleanupWrapper cleanup;

            RichTextBlock^ richTextBlock = nullptr;
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"RichTextBlock";

            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "WordBreakerTests.xaml"));

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = root;
                richTextBlock = safe_cast<RichTextBlock^>(root->FindName("richTextBlock"));
                VERIFY_IS_NOT_NULL(richTextBlock);

                xaml_automation::AutomationProperties::SetName(richTextBlock, ref new Platform::String(uiaInfo.m_Name));
            });
            TestServices::WindowHelper->WaitForIdle();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
                wrl::ComPtr<IUIAutomationTextPattern> spTextPattern;
                wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;
                AutoBSTR textFromTextRange;

                auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

                // Get TextPattern
                spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
                VERIFY_IS_NOT_NULL(spUIAutomationElement.Get());
                VERIFY_SUCCEEDED(spUIAutomationElement->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &spTextPattern));
                VERIFY_IS_NOT_NULL(spTextPattern.Get());

                // Get DocumentRange
                VERIFY_SUCCEEDED(spTextPattern->get_DocumentRange(spUIAutomationTextRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_TRUE(!wcscmp(L"This is some text. !!! In a *rich text block?\r\n", textFromTextRange));

                // Move the end point to match the start
                VERIFY_SUCCEEDED(spUIAutomationTextRange->MoveEndpointByRange(TextPatternRangeEndpoint_End, spUIAutomationTextRange.Get(), TextPatternRangeEndpoint_Start));
                // Move the end point to the end of the word
                BOOL moved = FALSE;
                VERIFY_SUCCEEDED(spUIAutomationTextRange->MoveEndpointByUnit(TextPatternRangeEndpoint_End, TextUnit_Word, 1, &moved));
                VERIFY_IS_TRUE(!!moved);
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_TRUE(!wcscmp(L"This ", textFromTextRange));

                // Move both
                VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Word, 4, &moved));
                VERIFY_IS_TRUE(!!moved);
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_TRUE(!wcscmp(L"!!! ", textFromTextRange));

                // Test punctuation at the start of a word
                VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Word, 3, &moved));
                VERIFY_IS_TRUE(!!moved);
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_TRUE(!wcscmp(L"*rich ", textFromTextRange));

                VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Word, 2, &moved));
                VERIFY_IS_TRUE(!!moved);
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_TRUE(!wcscmp(L"block?", textFromTextRange));

                // Move backwards
                VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Word, -2, &moved));
                VERIFY_IS_TRUE(!!moved);
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_TRUE(!wcscmp(L"*rich ", textFromTextRange));

                VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Word, -6, &moved));
                VERIFY_IS_TRUE(!!moved);
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_TRUE(!wcscmp(L"is ", textFromTextRange));
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void RichTextBlockTests::RichTextBlockOverflowWordBreakerNavigation()
        {
            TestCleanupWrapper cleanup;

            RichTextBlockOverflow^ richTextBlockOverflow = nullptr;
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"RichTextBlockOverflow";

            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + "WordBreakerTests.xaml"));

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = root;
                richTextBlockOverflow = safe_cast<RichTextBlockOverflow^>(root->FindName("richTextBlockOverflow"));
                VERIFY_IS_NOT_NULL(richTextBlockOverflow);

                xaml_automation::AutomationProperties::SetName(richTextBlockOverflow, ref new Platform::String(uiaInfo.m_Name));
            });
            TestServices::WindowHelper->WaitForIdle();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
                wrl::ComPtr<IUIAutomationTextPattern> spTextPattern;
                wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;
                AutoBSTR textFromTextRange;

                auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

                // Get TextPattern
                spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
                VERIFY_IS_NOT_NULL(spUIAutomationElement.Get());
                VERIFY_SUCCEEDED(spUIAutomationElement->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &spTextPattern));
                VERIFY_IS_NOT_NULL(spTextPattern.Get());

                // Get DocumentRange
                VERIFY_SUCCEEDED(spTextPattern->get_DocumentRange(spUIAutomationTextRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_TRUE(!wcscmp(L"Here's the next line in an overflow.\r\nAnd here's a second paragraph.\r\n", textFromTextRange));

                // Move the end point to match the start
                VERIFY_SUCCEEDED(spUIAutomationTextRange->MoveEndpointByRange(TextPatternRangeEndpoint_End, spUIAutomationTextRange.Get(), TextPatternRangeEndpoint_Start));
                // Move the end point to the end of the word
                BOOL moved = FALSE;
                VERIFY_SUCCEEDED(spUIAutomationTextRange->MoveEndpointByUnit(TextPatternRangeEndpoint_End, TextUnit_Word, 1, &moved));
                VERIFY_IS_TRUE(!!moved);
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_TRUE(!wcscmp(L"Here's ", textFromTextRange));

                // Move both
                VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Word, 7, &moved));
                VERIFY_IS_TRUE(!!moved);
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_TRUE(!wcscmp(L"\r\n", textFromTextRange));

                VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Word, 6, &moved));
                VERIFY_IS_TRUE(!!moved);
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_TRUE(!wcscmp(L"paragraph.", textFromTextRange));

                // Move backwards
                VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Word, -12, &moved));
                VERIFY_IS_TRUE(!!moved);
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_TRUE(!wcscmp(L"the ", textFromTextRange));
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void RichTextBlockTests::VerifyNarratorSelectionOnRichTextBlockOverflow()
        {
            TestCleanupWrapper cleanup;

            RichTextBlock^ rtb = nullptr;
            RichTextBlockOverflow^ rtbo = nullptr;
            xaml_controls::StackPanel^ rootPanel = nullptr;

            Automation::AutomationClient::UIAElementInfo uiaRTB;
            uiaRTB.m_Name = L"rtb";
            Automation::AutomationClient::UIAElementInfo uiaRTBO;
            uiaRTBO.m_Name = L"rtbo";
            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Grid Height='100'> "
                    L"    <Grid.ColumnDefinitions> <ColumnDefinition/> <ColumnDefinition/> <ColumnDefinition/> </Grid.ColumnDefinitions> "
                    L"    <RichTextBlock Grid.Column='0' OverflowContentTarget='{Binding ElementName=firstOverflowContainer}' TextAlignment='Justify' Margin='12,0' x:Name='rtb' > "
                    L"      <Paragraph>Linked text containers allow text which does not fit in one element to overflow into a different element on the page. "
                    L"           Creative use of linked text containers enables basic multicolumn support and other advanced page layouts. </Paragraph> "
                    L"    </RichTextBlock>"
                    L"    <RichTextBlockOverflow x:Name='firstOverflowContainer' Grid.Column='1' OverflowContentTarget='{Binding ElementName=secondOverflowContainer}' Margin='12,0' />"
                    L"    <RichTextBlockOverflow x:Name='secondOverflowContainer' Grid.Column='2' Margin='12,0' /> "
                    L"  </Grid> "
                    L"</StackPanel>"));
                rtb = safe_cast<RichTextBlock^>(rootPanel->FindName(L"rtb"));
                VERIFY_IS_NOT_NULL(rtb);
                xaml_automation::AutomationProperties::SetName(rtb, ref new Platform::String(uiaRTB.m_Name));
                rtbo = safe_cast<RichTextBlockOverflow^>(rootPanel->FindName(L"firstOverflowContainer"));
                VERIFY_IS_NOT_NULL(rtbo);
                xaml_automation::AutomationProperties::SetName(rtbo, ref new Platform::String(uiaRTBO.m_Name));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                wrl::ComPtr<IUIAutomationElement> rtbAutomationElement;
                wrl::ComPtr<IUIAutomationTextRange> rtbAutomationTextRange;
                wrl::ComPtr<IUIAutomationTextPattern> rtbTextPattern;
                wrl::ComPtr<IUIAutomationTextRangeArray> spVisibleRangesArray;

                wrl::ComPtr<IUIAutomationElement> overflowAutomationElement;
                wrl::ComPtr<IUIAutomationTextRange> overflowAutomationTextRange;
                wrl::ComPtr<IUIAutomationTextPattern> overflowTextPattern;
                wrl::ComPtr<IUIAutomationTextRangeArray> spUITextArrangeArray;


                // Get RTB
                auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaRTB);
                spAutomationClientManager->GetCurrentUIAutomationElement(&rtbAutomationElement);
                VERIFY_IS_NOT_NULL(rtbAutomationElement.Get());

                // Get text pattern for the RTB element
                VERIFY_SUCCEEDED(rtbAutomationElement->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &rtbTextPattern),
                    L"RichTextBlockTests::EmptyOverflowHasNullTextPattern: Failed in retreiving Text Pattern.");
                VERIFY_IS_NOT_NULL(rtbTextPattern.Get());

                // Select an RTB text range - Here we select the first line, positions 2-14
                VERIFY_SUCCEEDED(rtbTextPattern->GetVisibleRanges(spVisibleRangesArray.ReleaseAndGetAddressOf()));
                VERIFY_IS_NOT_NULL(spVisibleRangesArray.Get());
                VERIFY_SUCCEEDED(spVisibleRangesArray->GetElement(0, rtbAutomationTextRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_NOT_NULL(rtbAutomationTextRange.Get());
                VERIFY_SUCCEEDED(rtbAutomationTextRange->Select());

                // Get RTBO
                spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaRTBO);
                spAutomationClientManager->GetCurrentUIAutomationElement(&overflowAutomationElement);
                VERIFY_IS_NOT_NULL(overflowAutomationElement.Get());

                // Get text pattern for the RTBO element
                VERIFY_SUCCEEDED(overflowAutomationElement->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &overflowTextPattern),
                    L"RichTextBlockTests::EmptyOverflowHasNullTextPattern: Failed in retreiving Text Pattern.");
                VERIFY_IS_NOT_NULL(overflowTextPattern.Get());

                // Should not crash, even though inside the GetSelection call, selectionEndTextPointer is less than
                // selectionStartTextPointer and the two pointers get swapped.
                // TODO: Are the two pointers's offsets in that function correct? They are 14 and 73 (as in, the rest of the RTB content).
                // Instead, should they be 73 and 73? (As in, there is no selection in the RTBOverflow.)
                // RTB contains positions 0-73, RTBO contains positions 73-148.
                VERIFY_SUCCEEDED(overflowTextPattern->GetSelection(spUITextArrangeArray.ReleaseAndGetAddressOf()));
                VERIFY_IS_NOT_NULL(spUITextArrangeArray.Get());

            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void RichTextBlockTests::RichTextBlockOverflowTapOnHyperlink()
        {
            TestCleanupWrapper cleanup;

            RichTextBlock^ richTextBlock = nullptr;
            RichTextBlockOverflow^ richTextBlockOverflow = nullptr;
            Hyperlink^ hyperlink = nullptr;

            auto spClickEvent = std::make_shared<Event>();
            auto clickRegistration = CreateSafeEventRegistration(Hyperlink, Click);

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"            <RichTextBlock x:Name='myrtb' MaxLines='1' Width='300' OverflowContentTarget='{Binding ElementName=myrtbo}'>"
                    L"                <Paragraph><Hyperlink x:Name='hyperlink'><Run Text='1231111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111' /></Hyperlink></Paragraph>"
                    L"            </RichTextBlock>"
                    L"            <RichTextBlockOverflow x:Name='myrtbo' Width='600' Height='100'></RichTextBlockOverflow>"
                    L"</StackPanel>"));
                richTextBlock = safe_cast<RichTextBlock^>(rootPanel->FindName(L"myrtb"));
                richTextBlockOverflow = safe_cast<RichTextBlockOverflow^>(rootPanel->FindName(L"myrtbo"));
                hyperlink = safe_cast<xaml_docs::Hyperlink^>(rootPanel->FindName(L"hyperlink"));
                VERIFY_IS_NOT_NULL(richTextBlock);
                VERIFY_IS_NOT_NULL(richTextBlockOverflow);
                VERIFY_IS_NOT_NULL(hyperlink);
                TestServices::WindowHelper->WindowContent = rootPanel;

                clickRegistration.Attach(hyperlink,
                    ref new wf::TypedEventHandler<xaml_docs::Hyperlink^, xaml_docs::HyperlinkClickEventArgs^>([spClickEvent](xaml_docs::Hyperlink^ sender, xaml_docs::HyperlinkClickEventArgs^ e)
                    {
                        spClickEvent->Set();
                    }));
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->Tap(richTextBlockOverflow, 0.5f, 0.1f);
            spClickEvent->WaitForDefault();

            // Click a second time, which may crash if refcount is mishandled on the first tap.
            TestServices::InputHelper->Tap(richTextBlockOverflow, 0.5f, 0.1f);
            spClickEvent->WaitForDefault();
        }
    } }
} } } }
