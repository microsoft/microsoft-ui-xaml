// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <FileLoader.h>
#include <XamlFileTestEngine.h>
#include <TestCleanupWrapper.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <SafeEventRegistration.h>
#include <WUCRenderingScopeGuard.h>
#include "TextHighlighterTests.h"

using namespace ::Windows::UI;
using namespace Microsoft::UI::Text;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Documents;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Media;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics {

        Platform::String^ TextHighlighterTests::GetRenderingResourcesPath() const
        {
            return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\rendering\\";
        }

        bool TextHighlighterTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool TextHighlighterTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool TextHighlighterTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void TextHighlighterTests::TextRangeStruct()
        {
            xaml_docs::TextRange textRangeStruct = {};

            VERIFY_ARE_EQUAL(textRangeStruct.StartIndex, 0);
            VERIFY_ARE_EQUAL(textRangeStruct.Length, 0);

            textRangeStruct.StartIndex = 10;
            textRangeStruct.Length = 20;

            VERIFY_ARE_EQUAL(textRangeStruct.StartIndex, 10);
            VERIFY_ARE_EQUAL(textRangeStruct.Length, 20);
        }

        void TextHighlighterTests::TextHighlighterObject()
        {
            TestCleanupWrapper cleanup;

            const int ElementCount = 10;

            RunOnUIThread([&]()
            {
                auto textHighlighter = ref new xaml_docs::TextHighlighter();

                LOG_OUTPUT(L"Testing default Foreground/Background value");
                VERIFY_IS_NULL(textHighlighter->Foreground);
                VERIFY_IS_NULL(textHighlighter->Background);

                LOG_OUTPUT(L"Testing Foreground/Background get/set");
                textHighlighter->Foreground = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Black);
                textHighlighter->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Yellow);

                VERIFY_ARE_EQUAL(safe_cast<xaml_media::SolidColorBrush^>(textHighlighter->Foreground)->Color, Microsoft::UI::Colors::Black);
                VERIFY_ARE_EQUAL(safe_cast<xaml_media::SolidColorBrush^>(textHighlighter->Background)->Color, Microsoft::UI::Colors::Yellow);

                // Test by copy
                LOG_OUTPUT(L"Testing Ranges by copy");
                for (int i = 0; i < ElementCount; i++)
                {
                    xaml_docs::TextRange textRangeIn = { i, i + ElementCount };
                    textHighlighter->Ranges->Append(textRangeIn);

                    auto textRangeOut = textHighlighter->Ranges->GetAt(i);
                    VERIFY_ARE_EQUAL(textRangeOut.StartIndex, i);
                    VERIFY_ARE_EQUAL(textRangeOut.Length, i + ElementCount);
                }

                // Test IndexOf
                {
                    xaml_docs::TextRange textRangeIn = { 1, 11 };
                    uint32_t foundIndex = 0;
                    auto found = textHighlighter->Ranges->IndexOf(textRangeIn, &foundIndex);
                    VERIFY_ARE_EQUAL(foundIndex, 1u);
                    VERIFY_IS_TRUE(found);
                }

                textHighlighter->Ranges->Clear();

                // Test using move and inserting at 0
                LOG_OUTPUT(L"Testing Ranges by move");
                for (int i = 0; i < ElementCount; i++)
                {
                    xaml_docs::TextRange textRangeIn = { i, i + ElementCount };
                    textHighlighter->Ranges->InsertAt(0, std::move(textRangeIn));

                    auto textRangeOut = textHighlighter->Ranges->GetAt(0);
                    VERIFY_ARE_EQUAL(textRangeOut.StartIndex, i);
                    VERIFY_ARE_EQUAL(textRangeOut.Length, i + ElementCount);
                }

                textHighlighter->Ranges->Clear();
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void TextHighlighterTests::TextHighlighterCollection()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]()
            {
                auto textBlock = ref new xaml_controls::TextBlock();

                auto textHighlighter1 = ref new xaml_docs::TextHighlighter();
                textHighlighter1->Foreground = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Black);
                textHighlighter1->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Yellow);

                auto textHighlighter2 = ref new xaml_docs::TextHighlighter();
                textHighlighter2->Foreground = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
                textHighlighter2->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Green);

                textBlock->TextHighlighters->Append(textHighlighter1);
                textBlock->TextHighlighters->Append(textHighlighter2);

                TestServices::WindowHelper->WindowContent = textBlock;
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void TextHighlighterTests::SimpleTextBlockHighlight()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
            TestCleanupWrapper cleanup;

            RunOnUIThread([&] ()
            {
                auto textBlock = ref new xaml_controls::TextBlock();
                textBlock->Text = L"The quick brown fox jumps over the lazy dog.";

                auto textHighlighter = ref new xaml_docs::TextHighlighter();
                textHighlighter->Ranges->Append({ 4, 5 }); // Highlight "quick"

                textBlock->TextHighlighters->Append(textHighlighter);

                TestServices::WindowHelper->WindowContent = textBlock;
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }

        void TextHighlighterTests::SimpleRichTextBlockHighlight()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
            TestCleanupWrapper cleanup;

            RunOnUIThread([&] ()
            {
                auto run = ref new xaml_docs::Run();
                run->Text = L"The quick brown fox jumps over the lazy dog.";

                auto paragraph = ref new xaml_docs::Paragraph();
                paragraph->Inlines->Append(run);

                auto richTextBlock = ref new xaml_controls::RichTextBlock();
                richTextBlock->Blocks->Append(paragraph);

                auto textHighlighter = ref new xaml_docs::TextHighlighter();
                textHighlighter->Ranges->Append({ 4, 5 }); // Highlight "quick"

                richTextBlock->TextHighlighters->Append(textHighlighter);

                TestServices::WindowHelper->WindowContent = richTextBlock;
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }

        void TextHighlighterTests::TextBlockMultiHighlight()
        {
            // Tests multiple overlapping highlights with different colors in addition to
            // multi-line highlight.
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
            TestCleanupWrapper cleanup;

            RunOnUIThread([&] ()
            {
                auto textBlock = ref new xaml_controls::TextBlock();
                textBlock->Width = 200;
                textBlock->TextWrapping = xaml::TextWrapping::Wrap;
                textBlock->Text = L"The quick brown fox jumps over the lazy dog.";

                auto textHighlighter1 = ref new xaml_docs::TextHighlighter();
                textHighlighter1->Foreground = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
                textHighlighter1->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Green);
                textHighlighter1->Ranges->Append({ 4, 6 }); // Highlight "quick "
                textHighlighter1->Ranges->Append({ 16, 3 }); // Highlight "fox"
                textHighlighter1->Ranges->Append({ 20, 10 }); // Highlight "jumps over"
                textBlock->TextHighlighters->Append(textHighlighter1);

                auto textHighlighter2 = ref new xaml_docs::TextHighlighter();
                textHighlighter2->Foreground = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Orange);
                textHighlighter2->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Blue);
                textHighlighter2->Ranges->Append({ 4, 5 }); // Highlight "quick"
                textHighlighter2->Ranges->Append({ 17, 9 }); // Highlight some text intersecting previous highlights
                textBlock->TextHighlighters->Append(textHighlighter2);

                auto textHighlighter3 = ref new xaml_docs::TextHighlighter();
                textHighlighter3->Foreground = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Cyan);
                textHighlighter3->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::DarkBlue);
                textHighlighter3->Ranges->Append({ 0, 3 }); // Highlight "The"
                textBlock->TextHighlighters->Append(textHighlighter3);

                TestServices::WindowHelper->WindowContent = textBlock;
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }

        void TextHighlighterTests::TextBlockRemoveHighlight()
        {
            // Tests multiple overlapping highlights with different colors in addition to
            // multi-line highlight.
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
            TestCleanupWrapper cleanup;

            xaml_controls::TextBlock^ textBlock;
            xaml_docs::TextHighlighter^ textHighlighter1;
            xaml_docs::TextHighlighter^ textHighlighter2;

            RunOnUIThread([&] ()
            {
                textBlock = ref new xaml_controls::TextBlock();
                textBlock->Text = L"The quick brown fox jumps over the lazy dog.";

                textHighlighter1 = ref new xaml_docs::TextHighlighter();
                textHighlighter1->Foreground = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
                textHighlighter1->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Green);
                textHighlighter1->Ranges->Append({ 4, 11 }); // Highlight "quick brown"
                textHighlighter1->Ranges->Append({ 15, 3 }); // Highlight "fox"
                textHighlighter1->Ranges->Append({ 20, 23 }); // Highlight "jumps over the lazy dog"
                textBlock->TextHighlighters->Append(textHighlighter1);

                textHighlighter2 = ref new xaml_docs::TextHighlighter();
                textHighlighter2->Foreground = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Yellow);
                textHighlighter2->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Blue);
                textHighlighter2->Ranges->Append({ 0, 44 }); // Highlight all the text
                textBlock->TextHighlighters->Append(textHighlighter2);

                TestServices::WindowHelper->WindowContent = textBlock;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&] ()
            {
                textHighlighter1->Ranges->RemoveAt(1); // Remove "fox" highlight
                textBlock->TextHighlighters->RemoveAt(1); // Remove highlighter textHighlighter2
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }

        void TextHighlighterTests::TextBlockRTL()
        {
            TestCleanupWrapper cleanup;

            XamlFileTestEngine engine;
            engine.SetXamlFilePath(GetRenderingResourcesPath() + L"TextBlockSelectionTests.xaml");
            engine.SetWindowSize(wf::Size(800, 600));
            engine.SetDCompRenderingMode(DCompRendering::WUCCompleteSynchronousCompTree);
            engine.SetMockDCompVerification(MockDComp::SurfaceComparison::NoComparison);
            engine.SetPostInitCallback([&] (xaml::FrameworkElement^ rootElement)
            {
                auto tb5 = safe_cast<xaml_controls::TextBlock^>(rootElement->FindName(L"tb5"));

                auto textHighlighter1 = ref new xaml_docs::TextHighlighter();
                textHighlighter1->Ranges->Append({ 3, 5 }); // Highlight some english and arabic text
                tb5->TextHighlighters->Append(textHighlighter1);
            });
            engine.Execute();
        }

        void TextHighlighterTests::TextBlockFullLineHighlight()
        {
            TestCleanupWrapper cleanup;

            XamlFileTestEngine engine;
            engine.SetXamlFilePath(GetRenderingResourcesPath() + L"TextBlockSelectionTests.xaml");
            engine.SetWindowSize(wf::Size(800, 600));
            engine.SetDCompRenderingMode(DCompRendering::WUCCompleteSynchronousCompTree);
            engine.SetMockDCompVerification(MockDComp::SurfaceComparison::NoComparison);
            engine.SetPostInitCallback([&] (xaml::FrameworkElement^ rootElement)
            {
                auto tb1 = safe_cast<xaml_controls::TextBlock^>(rootElement->FindName(L"tb1"));

                auto textHighlighter1 = ref new xaml_docs::TextHighlighter();
                textHighlighter1->Ranges->Append({ 0, 3 }); // Highlight abc
                tb1->TextHighlighters->Append(textHighlighter1);
            });
            engine.Execute();
        }

        void TextHighlighterTests::TextBlockOutOfRange()
        {
            TestCleanupWrapper cleanup;

            XamlFileTestEngine engine;
            engine.SetXamlFilePath(GetRenderingResourcesPath() + L"TextBlockSelectionTests.xaml");
            engine.SetWindowSize(wf::Size(800, 600));
            engine.SetDCompRenderingMode(DCompRendering::WUCCompleteSynchronousCompTree);
            engine.SetMockDCompVerification(MockDComp::SurfaceComparison::NoComparison);
            engine.SetPostInitCallback([&] (xaml::FrameworkElement^ rootElement)
            {
                auto tb5 = safe_cast<xaml_controls::TextBlock^>(rootElement->FindName(L"tb1"));

                auto textHighlighter1 = ref new xaml_docs::TextHighlighter();
                textHighlighter1->Foreground = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Yellow);
                textHighlighter1->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Blue);
                textHighlighter1->Ranges->Append({ -10, 3 }); // Completely out of range on the left
                textHighlighter1->Ranges->Append({ 10, 3 }); // Completely out of range on the right
                tb5->TextHighlighters->Append(textHighlighter1);

                auto textHighlighter2 = ref new xaml_docs::TextHighlighter();
                textHighlighter2->Foreground = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Green);
                textHighlighter2->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
                textHighlighter2->Ranges->Append({ -2, 3 }); // Partially out of range on the left
                textHighlighter2->Ranges->Append({ 2, 3 }); // Partially out of range on the right
                tb5->TextHighlighters->Append(textHighlighter2);
            });
            engine.Execute();
        }

        void TextHighlighterTests::TextBlockMultipleRuns()
        {
            TestCleanupWrapper cleanup;

            XamlFileTestEngine engine;
            engine.SetXamlFilePath(GetRenderingResourcesPath() + L"TextBlockBatching.xaml");
            engine.SetDCompRenderingMode(DCompRendering::WUCCompleteSynchronousCompTree);
            engine.SetMockDCompVerification(MockDComp::SurfaceComparison::NoComparison);
            engine.SetPostInitCallback([&] (xaml::FrameworkElement^ rootElement)
            {
                auto tb1 = safe_cast<xaml_controls::TextBlock^>(rootElement->FindName(L"tb1"));

                auto textHighlighter1 = ref new xaml_docs::TextHighlighter();
                textHighlighter1->Ranges->Append({ 1, 4 }); // Highlight 2345, spanning multiple runs
                tb1->TextHighlighters->Append(textHighlighter1);
            });
            engine.Execute();
        }

        void TextHighlighterTests::TextBlockLightTheme()
        {
            TestCleanupWrapper cleanup;
            TestServices::ThemingHelper->SystemTheme = xaml::ApplicationTheme::Light;

            XamlFileTestEngine engine;
            engine.SetXamlFilePath(GetRenderingResourcesPath() + L"TextBlockSelectionTests.xaml");
            engine.SetWindowSize(wf::Size(800, 600));
            engine.SetDCompRenderingMode(DCompRendering::WUCCompleteSynchronousCompTree);
            engine.SetMockDCompVerification(MockDComp::SurfaceComparison::NoComparison);
            engine.SetPostInitCallback([&] (xaml::FrameworkElement^ rootElement)
            {
                auto tb1 = safe_cast<xaml_controls::TextBlock^>(rootElement->FindName(L"tb1"));

                auto textHighlighter1 = ref new xaml_docs::TextHighlighter();
                textHighlighter1->Ranges->Append({ 0, 3 }); // Highlight abc
                tb1->TextHighlighters->Append(textHighlighter1);
            });
            engine.Execute();
        }

        void TextHighlighterTests::TextBlockHighContrastTheme()
        {
            TestCleanupWrapper cleanup;
            TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::Default;

            XamlFileTestEngine engine;
            engine.SetXamlFilePath(GetRenderingResourcesPath() + L"TextBlockSelectionTests.xaml");
            engine.SetWindowSize(wf::Size(800, 600));
            engine.SetDCompRenderingMode(DCompRendering::WUCCompleteSynchronousCompTree);
            engine.SetMockDCompVerification(MockDComp::SurfaceComparison::NoComparison);
            engine.SetPostInitCallback([&] (xaml::FrameworkElement^ rootElement)
            {
                auto tb1 = safe_cast<xaml_controls::TextBlock^>(rootElement->FindName(L"tb1"));

                auto textHighlighter1 = ref new xaml_docs::TextHighlighter();
                textHighlighter1->Ranges->Append({ 0, 3 }); // Highlight abc
                tb1->TextHighlighters->Append(textHighlighter1);
            });
            engine.Execute();
        }

        void TextHighlighterTests::RichTextBlockMultiParagraph()
        {
            TestCleanupWrapper cleanup;

            XamlFileTestEngine engine;
            engine.SetXamlFilePath(GetRenderingResourcesPath() + L"RichTextBlockMultiParagraph.xaml");
            engine.SetDCompRenderingMode(DCompRendering::WUCCompleteSynchronousCompTree);
            engine.SetMockDCompVerification(MockDComp::SurfaceComparison::NoComparison);
            engine.SetPostInitCallback([&] (xaml::FrameworkElement^ rootElement)
            {
                auto rtb = safe_cast<xaml_controls::RichTextBlock^>(rootElement->FindName(L"richTextBlock"));

                auto textHighlighter1 = ref new xaml_docs::TextHighlighter();
                textHighlighter1->Ranges->Append({ 10, 20 }); // Highlight a lot of text spanning multiple paragraphs
                rtb->TextHighlighters->Append(textHighlighter1);
            });
            engine.Execute();
        }

        void TextHighlighterTests::RichTextBlockOverflow()
        {
            TestCleanupWrapper cleanup;

            XamlFileTestEngine engine;
            engine.SetXamlFilePath(GetRenderingResourcesPath() + L"TextCopyTests.xaml");
            engine.SetDCompRenderingMode(DCompRendering::WUCCompleteSynchronousCompTree);
            engine.SetMockDCompVerification(MockDComp::SurfaceComparison::NoComparison);
            engine.SetPostInitCallback([&] (xaml::FrameworkElement^ rootElement)
            {
                auto richTextBlockWithOverflow = safe_cast<xaml_controls::RichTextBlock^>(rootElement->FindName(L"richTextBlockWithOverflow"));

                auto textHighlighter1 = ref new xaml_docs::TextHighlighter();
                textHighlighter1->Ranges->Append({ 3, 5 }); // Highlight some text
                richTextBlockWithOverflow->TextHighlighters->Append(textHighlighter1);
            });
            engine.Execute();
        }

        // Highlight + Selection, selection should take priority
        void TextHighlighterTests::SelectOverHighlight()
        {
            TestCleanupWrapper cleanup;
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

            TextBlock^ tb1 = nullptr;
            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetRenderingResourcesPath() + L"TextBlockBatching.xaml"));
            RunOnUIThread([&]()
            {
                tb1 = safe_cast<xaml_controls::TextBlock^>(root->FindName(L"tb1"));
                VERIFY_IS_NOT_NULL(tb1);

                TestServices::WindowHelper->WindowContent = root;

                TextHighlighter^ textHighlighter1 = ref new xaml_docs::TextHighlighter();
                textHighlighter1->Ranges->Append({ 1, 4 }); // Highlight 2345, spanning multiple runs
                tb1->TextHighlighters->Append(textHighlighter1);

                tb1->Focus(FocusState::Pointer);
            });
            TestServices::WindowHelper->WaitForIdle();

            // Select some highlighted and non-highlighted text by click and dragging
            TestServices::InputHelper->DragFromCenter(tb1, -100 /*relX*/, 0 /*relY*/, 0.5 /*velocityFactor*/);
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }

        void TextHighlighterTests::BackplatesAndHighlightsAndSelection()
        {
            TestCleanupWrapper cleanup;
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
            TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::Default;

            xaml_controls::TextBlock^ tb = nullptr;
            xaml_controls::RichTextBlock^ rtb = nullptr;
            xaml_controls::RichTextBlock^ rtbWithO = nullptr;
            xaml_controls::RichTextBlockOverflow^ rtbo = nullptr;
            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetRenderingResourcesPath() + L"TextCopyTests.xaml"));
            RunOnUIThread([&]()
            {
                tb = safe_cast<xaml_controls::TextBlock^>(root->FindName(L"textBlock"));
                rtb = safe_cast<xaml_controls::RichTextBlock^>(root->FindName(L"richTextBlock"));
                rtbWithO = safe_cast<xaml_controls::RichTextBlock^>(root->FindName(L"richTextBlockWithOverflow"));
                rtbo = safe_cast<xaml_controls::RichTextBlockOverflow^>(root->FindName(L"firstOverflowContainer"));
                VERIFY_IS_NOT_NULL(tb);
                VERIFY_IS_NOT_NULL(rtb);
                VERIFY_IS_NOT_NULL(rtbWithO);
                VERIFY_IS_NOT_NULL(rtbo);

                TestServices::WindowHelper->WindowContent = root;

                root->HighContrastAdjustment = ElementHighContrastAdjustment::Auto;
                Application::Current->HighContrastAdjustment = ApplicationHighContrastAdjustment::Auto;

                // Add a highlight to each type of element
                TextHighlighter^ textHighlighter1 = ref new xaml_docs::TextHighlighter();
                textHighlighter1->Ranges->Append({ 2, 3 });
                tb->TextHighlighters->Append(textHighlighter1);

                TextHighlighter^ textHighlighter2 = ref new xaml_docs::TextHighlighter();
                textHighlighter2->Ranges->Append({ 2, 3 });
                rtb->TextHighlighters->Append(textHighlighter2);

                TextHighlighter^ textHighlighter3 = ref new xaml_docs::TextHighlighter();
                textHighlighter3->Ranges->Append({ 8, 4 });
                rtbWithO->TextHighlighters->Append(textHighlighter3);

                tb->Focus(FocusState::Pointer);
            });
            TestServices::WindowHelper->WaitForIdle();

            // Select some highlighted and non-highlighted text by click and dragging
            TestServices::InputHelper->DragFromCenter(tb, -100 /*relX*/, 0 /*relY*/, 0.5 /*velocityFactor*/);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "TextBlock");

            TestServices::InputHelper->DragFromCenter(rtb, -100 /*relX*/, 0 /*relY*/, 0.5 /*velocityFactor*/);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "RichTextBlock");

            TestServices::InputHelper->DragFromCenter(rtbo, -100 /*relX*/, 0 /*relY*/, 0.5 /*velocityFactor*/);
            TestServices::WindowHelper->WaitForIdle();
            // Overflow backplating is broken
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "RichTextBlockOverflow");
        }

        void TextHighlighterTests::HighlightAroundInlineUIContainer()
        {
            TestCleanupWrapper cleanup;
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

            xaml_controls::TextBlock^ tb = nullptr;
            xaml_controls::RichTextBlock^ rtb = nullptr;
            Grid^ rootGrid = nullptr;

            std::function<void(void)> ClearHighlights;

            RunOnUIThread([&]()
            {
                rootGrid = ref new Grid;
                TestServices::WindowHelper->WindowContent = rootGrid;

                // Set up the text blocks
                StackPanel^ stackPanel = ref new StackPanel();
                rtb = ref new RichTextBlock();
                rtb->Width = 100;
                Paragraph^ p = ref new Paragraph();

                Run^ run1 = ref new Run();
                run1->Text = "012345678";
                Run^ run2 = ref new Run();
                run2->Text = "012345678";
                Run^ run3 = ref new Run();
                run3->Text = "01234";
                Run^ run4 = ref new Run();
                run4->Text = "56789";
                Run^ run5 = ref new Run();
                run5->Text = "this is a really long line that will wrap around";

                tb = ref new TextBlock();
                tb->Text = "abc";
                InlineUIContainer^ uic = ref new InlineUIContainer();
                uic->Child = tb;

                auto symbolIcon = ref new xaml_controls::SymbolIcon();
                VERIFY_ARE_EQUAL(symbolIcon->Symbol, xaml_controls::Symbol::Emoji);

                InlineUIContainer^ emojiContainer = ref new InlineUIContainer();
                emojiContainer->Child = symbolIcon;

                p->Inlines->Append(run1);
                p->Inlines->Append(uic);
                p->Inlines->Append(run2);
                p->Inlines->Append(ref new LineBreak());
                p->Inlines->Append(run3);
                p->Inlines->Append(emojiContainer);
                p->Inlines->Append(run4);
                p->Inlines->Append(ref new LineBreak());
                p->Inlines->Append(run5);

                rtb->Blocks->Append(p);
                stackPanel->Children->Append(rtb);
                rootGrid->Children->Append(stackPanel);

                ClearHighlights = [&]()
                {
                    tb->TextHighlighters->Clear();
                    rtb->TextHighlighters->Clear();
                };
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                ClearHighlights();

                // Highlight around a IUC
                TextHighlighter^ textHighlighter1 = ref new xaml_docs::TextHighlighter();
                textHighlighter1->Ranges->Append({ 3, 5 }); // highlight "34567"
                rtb->TextHighlighters->Append(textHighlighter1);

                TextHighlighter^ textHighlighter2 = ref new xaml_docs::TextHighlighter();
                textHighlighter2->Ranges->Append({ 14, 2 }); // highlight "56"
                rtb->TextHighlighters->Append(textHighlighter2);

            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"1");

            RunOnUIThread([&]()
            {
                ClearHighlights();

                // Highlight inside a IUC
                TextHighlighter^ textHighlighter = ref new xaml_docs::TextHighlighter();
                textHighlighter->Ranges->Append({ 1, 1 }); // highlight "b"
                tb->TextHighlighters->Append(textHighlighter);

            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

            RunOnUIThread([&]()
            {
                ClearHighlights();

                // Highlight over (and including) an IUC
                TextHighlighter^ textHighlighter = ref new xaml_docs::TextHighlighter();
                textHighlighter->Ranges->Append({ 7, 4 }); // highlight "78abc01"
                rtb->TextHighlighters->Append(textHighlighter);

            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

            RunOnUIThread([&]()
            {
                ClearHighlights();

                // Highlight after a line break, right before IUC
                TextHighlighter^ textHighlighter = ref new xaml_docs::TextHighlighter();
                textHighlighter->Ranges->Append({ 23, 1 }); // highlight "4" (and the IUC right after)
                rtb->TextHighlighters->Append(textHighlighter);

            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"4");

            RunOnUIThread([&]()
            {
                ClearHighlights();

                // Highlight right after IUC
                TextHighlighter^ textHighlighter = ref new xaml_docs::TextHighlighter();
                textHighlighter->Ranges->Append({ 24, 1 }); // highlight "5"
                rtb->TextHighlighters->Append(textHighlighter);

            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"5");

            RunOnUIThread([&]()
            {
                ClearHighlights();

                // Highlight only a line break
                TextHighlighter^ textHighlighter = ref new xaml_docs::TextHighlighter();
                textHighlighter->Ranges->Append({ 18, 1 }); // highlight the linebreak (nothing seen)
                rtb->TextHighlighters->Append(textHighlighter);

            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"6");

            RunOnUIThread([&]()
            {
                ClearHighlights();

                // Highlight over a line break
                TextHighlighter^ textHighlighter = ref new xaml_docs::TextHighlighter();
                textHighlighter->Ranges->Append({ 17, 3 }); // highlight "8<LB>0"
                rtb->TextHighlighters->Append(textHighlighter);

            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"7");

            RunOnUIThread([&]()
            {
                ClearHighlights();

                // Highlight over a non-explicit line break
                TextHighlighter^ textHighlighter = ref new xaml_docs::TextHighlighter();
                textHighlighter->Ranges->Append({ 45, 3 }); // highlight "y l" in "really long"
                textHighlighter->Ranges->Append({ 60, 3 }); // highlight "t w" in "that will"
                rtb->TextHighlighters->Append(textHighlighter);

            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"8");
        }

        void TextHighlighterTests::HighlightAroundSpans()
        {
            TestCleanupWrapper cleanup;
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

            xaml_controls::RichTextBlock^ rtb = nullptr;
            Grid^ rootGrid = nullptr;

            std::function<void(void)> ClearHighlights;

            RunOnUIThread([&]()
            {
                rootGrid = ref new Grid;
                TestServices::WindowHelper->WindowContent = rootGrid;

                // Set up the text blocks
                StackPanel^ stackPanel = ref new StackPanel();
                rtb = ref new RichTextBlock();
                Paragraph^ p = ref new Paragraph();

                Span^ span1 = ref new Span();
                Span^ span2 = ref new Span();
                span1->Foreground = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                span2->Foreground = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);

                Run^ run1 = ref new Run();
                run1->Text = "012345678";
                Run^ run2 = ref new Run();
                run2->Text = "012345678";
                Run^ run3 = ref new Run();
                run3->Text = "01234";
                Run^ run4 = ref new Run();
                run4->Text = "56789";
                LineBreak^ lb = ref new LineBreak();

                span1->Inlines->Append(run2);
                span2->Inlines->Append(run4);

                p->Inlines->Append(run1);
                p->Inlines->Append(span1);
                p->Inlines->Append(lb);
                p->Inlines->Append(run3);
                p->Inlines->Append(span2);

                rtb->Blocks->Append(p);
                stackPanel->Children->Append(rtb);
                rootGrid->Children->Append(stackPanel);

                ClearHighlights = [&]()
                {
                    rtb->TextHighlighters->Clear();
                };
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                ClearHighlights();

                // Highlight in normal run
                TextHighlighter^ textHighlighter1 = ref new xaml_docs::TextHighlighter();
                textHighlighter1->Ranges->Append({ 4, 2 }); // highlight "45"
                rtb->TextHighlighters->Append(textHighlighter1);

                // Highlight in span
                TextHighlighter^ textHighlighter2 = ref new xaml_docs::TextHighlighter();
                textHighlighter2->Ranges->Append({ 14, 2 }); // highlight "56"
                rtb->TextHighlighters->Append(textHighlighter2);

                // Highlight across run and span
                TextHighlighter^ textHighlighter3 = ref new xaml_docs::TextHighlighter();
                textHighlighter3->Ranges->Append({ 23, 3 }); // highlight "456"
                rtb->TextHighlighters->Append(textHighlighter3);

            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }

        void TextHighlighterTests::HighlightAroundLinks()
        {
            TestCleanupWrapper cleanup;
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

            xaml_controls::RichTextBlock^ rtb = nullptr;
            Grid^ rootGrid = nullptr;

            std::function<void(void)> ClearHighlights;

            RunOnUIThread([&]()
            {
                rootGrid = ref new Grid;
                TestServices::WindowHelper->WindowContent = rootGrid;

                // Set up the text blocks
                StackPanel^ stackPanel = ref new StackPanel();
                rtb = ref new RichTextBlock();
                Paragraph^ p = ref new Paragraph();

                Run^ run1 = ref new Run();
                run1->Text = "01234";
                Run^ run2 = ref new Run();
                run2->Text = "56789";
                Run^ run3 = ref new Run();
                run3->Text = "01234";

                Hyperlink^ hyperlink = ref new Hyperlink();
                Run^ run = ref new Run();
                run->Text = "Hyperlink";
                hyperlink->Inlines->Append(run);

                p->Inlines->Append(run1);
                p->Inlines->Append(run2);
                p->Inlines->Append(hyperlink);
                p->Inlines->Append(run3);

                rtb->Blocks->Append(p);
                stackPanel->Children->Append(rtb);
                rootGrid->Children->Append(stackPanel);

                ClearHighlights = [&]()
                {
                    rtb->TextHighlighters->Clear();
                };
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                ClearHighlights();

                TextHighlighter^ textHighlighter1 = ref new xaml_docs::TextHighlighter();
                rtb->TextHighlighters->Append(textHighlighter1);

                TextHighlighter^ textHighlighter2 = ref new xaml_docs::TextHighlighter();
                textHighlighter2->Ranges->Append({ 26, 5 }); // highlight "link0" (in "hyperlink")
                rtb->TextHighlighters->Append(textHighlighter2);

            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }

    } }
} } } }
