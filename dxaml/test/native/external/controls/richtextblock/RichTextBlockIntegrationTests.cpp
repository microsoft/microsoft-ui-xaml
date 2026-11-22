// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "RichTextBlockIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TreeHelper.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <TextControlHelper.h>

#include <algorithm>
#include <cmath>
#include <cwctype>
#include <string>
#include <vector>

using namespace Platform;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace RichTextBlock {

    bool RichTextBlockIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool RichTextBlockIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool RichTextBlockIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void RichTextBlockIntegrationTests::VerifySelectingTextWithTouchShowsSelectionFlyout()
    {
        TextControlHelper::VerifySelectingTextWithTouchShowsSelectionFlyout<xaml_controls::RichTextBlock>(
            [](xaml_controls::RichTextBlock^ richTextBlock)
            {
                auto run = ref new xaml_docs::Run();
                run->Text = "aaaaaaaaaa";

                auto paragraph = ref new xaml_docs::Paragraph();
                paragraph->Inlines->Append(run);

                richTextBlock->Blocks->Append(paragraph);
                richTextBlock->IsTextSelectionEnabled = true;
            });
    }

    namespace
    {
        // Pixel tolerance for layout values that the product computes in fixed-point text units.
        const float c_layoutTolerance = 0.5f;

        bool AreSameObject(Platform::Object^ first, Platform::Object^ second)
        {
            if (first == nullptr || second == nullptr)
            {
                return first == second;
            }

            wrl::ComPtr<IUnknown> firstIdentity;
            wrl::ComPtr<IUnknown> secondIdentity;
            VERIFY_SUCCEEDED(reinterpret_cast<IUnknown*>(first)->QueryInterface(IID_PPV_ARGS(&firstIdentity)));
            VERIFY_SUCCEEDED(reinterpret_cast<IUnknown*>(second)->QueryInterface(IID_PPV_ARGS(&secondIdentity)));
            return firstIdentity.Get() == secondIdentity.Get();
        }

        void VerifyAreClose(float expected, float actual, const wchar_t* what)
        {
            LOG_OUTPUT(L"%s: expected %f, actual %f", what, expected, actual);
            VERIFY_IS_TRUE(std::abs(expected - actual) <= c_layoutTolerance, what);
        }

        xaml_controls::StackPanel^ CreateTopLeftPanel()
        {
            auto panel = ref new xaml_controls::StackPanel();
            panel->HorizontalAlignment = xaml::HorizontalAlignment::Left;
            panel->VerticalAlignment = xaml::VerticalAlignment::Top;
            return panel;
        }

        xaml_controls::RichTextBlock^ CreateRichTextBlock(double width, double fontSize, Platform::String^ fontFamily)
        {
            auto richTextBlock = ref new xaml_controls::RichTextBlock();
            richTextBlock->HorizontalAlignment = xaml::HorizontalAlignment::Left;
            richTextBlock->Width = width;
            richTextBlock->FontSize = fontSize;
            richTextBlock->FontFamily = ref new xaml_media::FontFamily(fontFamily);
            return richTextBlock;
        }

        xaml_docs::Run^ AppendParagraphWithRun(xaml_controls::RichTextBlock^ richTextBlock, Platform::String^ text)
        {
            auto run = ref new xaml_docs::Run();
            run->Text = text;
            auto paragraph = ref new xaml_docs::Paragraph();
            paragraph->Inlines->Append(run);
            richTextBlock->Blocks->Append(paragraph);
            return run;
        }

        xaml_docs::TextPointer^ PositionAt(xaml_docs::TextPointer^ start, int offset)
        {
            auto position = start->GetPositionAtOffset(offset, xaml_docs::LogicalDirection::Forward);
            VERIFY_IS_NOT_NULL(position);
            return position;
        }

        wf::Rect CharacterRect(xaml_docs::TextPointer^ start, int offset, xaml_docs::LogicalDirection direction)
        {
            return PositionAt(start, offset)->GetCharacterRect(direction);
        }

        void VerifySelection(xaml_controls::RichTextBlock^ richTextBlock, int expectedStart, int expectedEnd, Platform::String^ expectedText)
        {
            auto selectionStart = richTextBlock->SelectionStart;
            auto selectionEnd = richTextBlock->SelectionEnd;
            VERIFY_IS_NOT_NULL(selectionStart);
            VERIFY_IS_NOT_NULL(selectionEnd);
            VERIFY_ARE_EQUAL(expectedStart, selectionStart->Offset);
            VERIFY_ARE_EQUAL(expectedEnd, selectionEnd->Offset);
            VERIFY_ARE_EQUAL(expectedText, richTextBlock->SelectedText);
        }

        xaml_automation::Provider::ITextProvider^ GetTextProvider(xaml_controls::RichTextBlock^ richTextBlock)
        {
            auto peer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(richTextBlock);
            VERIFY_IS_NOT_NULL(peer);
            auto textPattern = peer->GetPattern(xaml_automation_peers::PatternInterface::Text);
            VERIFY_IS_NOT_NULL(textPattern);
            return safe_cast<xaml_automation::Provider::ITextProvider^>(textPattern);
        }

        std::vector<wf::Rect> GetBoundingRectangles(xaml_automation::Provider::ITextRangeProvider^ range)
        {
            Platform::Array<double>^ flatRectangles = nullptr;
            range->GetBoundingRectangles(&flatRectangles);
            VERIFY_IS_NOT_NULL(flatRectangles);
            VERIFY_ARE_EQUAL(0u, flatRectangles->Length % 4);

            std::vector<wf::Rect> rectangles;
            for (unsigned int i = 0; i < flatRectangles->Length; i += 4)
            {
                rectangles.push_back(wf::Rect(
                    static_cast<float>(flatRectangles[i]),
                    static_cast<float>(flatRectangles[i + 1]),
                    static_cast<float>(flatRectangles[i + 2]),
                    static_cast<float>(flatRectangles[i + 3])));
            }
            return rectangles;
        }

        // Selects [start, end) of the run and returns the bounding rectangles of the text pattern's selection range.
        std::vector<wf::Rect> GetRunRangeBoundingRectangles(
            xaml_controls::RichTextBlock^ richTextBlock,
            xaml_automation::Provider::ITextProvider^ textProvider,
            xaml_docs::Run^ run,
            int start,
            int end)
        {
            richTextBlock->Select(PositionAt(run->ContentStart, start), PositionAt(run->ContentStart, end));

            auto selection = textProvider->GetSelection();
            VERIFY_IS_NOT_NULL(selection);
            VERIFY_ARE_EQUAL(1u, selection->Length);

            Platform::String^ expectedText = ref new Platform::String(run->Text->Data() + start, end - start);
            VERIFY_ARE_EQUAL(expectedText, selection[0]->GetText(-1));

            return GetBoundingRectangles(selection[0]);
        }

        bool IsPointInsideAny(const std::vector<wf::Rect>& rectangles, float x, float y)
        {
            for (const auto& rectangle : rectangles)
            {
                if (x > rectangle.X && x < rectangle.X + rectangle.Width &&
                    y > rectangle.Y && y < rectangle.Y + rectangle.Height)
                {
                    return true;
                }
            }
            return false;
        }

        // Every character of the run in [start, end) must be covered by the range rectangles and every other
        // character must not be; the reference rectangle of each character comes from a single-character range.
        void VerifyRangeCoverage(
            const std::vector<wf::Rect>& rangeRectangles,
            const std::vector<wf::Rect>& characterRectangles,
            int start,
            int end,
            const wchar_t* rangeName)
        {
            VERIFY_IS_FALSE(rangeRectangles.empty());

            for (const auto& rectangle : rangeRectangles)
            {
                LOG_OUTPUT(L"%s rectangle: %f,%f %fx%f", rangeName, rectangle.X, rectangle.Y, rectangle.Width, rectangle.Height);
                VERIFY_IS_GREATER_THAN(rectangle.Width, 0.0f);
                VerifyAreClose(characterRectangles[0].Height, rectangle.Height, L"Range rectangle has the line height");
            }

            for (size_t i = 0; i < rangeRectangles.size(); i++)
            {
                for (size_t j = i + 1; j < rangeRectangles.size(); j++)
                {
                    const float overlapLeft = (std::max)(rangeRectangles[i].X, rangeRectangles[j].X);
                    const float overlapRight = (std::min)(rangeRectangles[i].X + rangeRectangles[i].Width, rangeRectangles[j].X + rangeRectangles[j].Width);
                    VERIFY_IS_LESS_THAN_OR_EQUAL(overlapRight - overlapLeft, c_layoutTolerance, L"Range rectangles must not overlap");
                }
            }

            for (int i = 0; i < static_cast<int>(characterRectangles.size()); i++)
            {
                const auto& characterRectangle = characterRectangles[i];
                const bool isSelected = (i >= start && i < end);
                const bool isCovered = IsPointInsideAny(
                    rangeRectangles,
                    characterRectangle.X + characterRectangle.Width / 2,
                    characterRectangle.Y + characterRectangle.Height / 2);
                LOG_OUTPUT(L"%s: character %d selected=%d covered=%d", rangeName, i, isSelected, isCovered);
                VERIFY_ARE_EQUAL(isSelected, isCovered);
            }
        }

        std::wstring LettersOf(Platform::String^ text)
        {
            std::wstring letters;
            if (text != nullptr)
            {
                for (unsigned int i = 0; i < text->Length(); i++)
                {
                    if (iswalpha(text->Data()[i]))
                    {
                        letters.push_back(text->Data()[i]);
                    }
                }
            }
            return letters;
        }
    }

    void RichTextBlockIntegrationTests::VerifySelectionStartAndEndTrackSelection()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::RichTextBlock^ richTextBlock = nullptr;
        xaml_docs::Run^ run = nullptr;

        RunOnUIThread([&]()
        {
            richTextBlock = CreateRichTextBlock(400, 20, L"Segoe UI");
            richTextBlock->IsTextSelectionEnabled = false;
            run = AppendParagraphWithRun(richTextBlock, L"abcdefgh");

            auto root = CreateTopLeftPanel();
            root->Children->Append(richTextBlock);
            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_NULL(richTextBlock->SelectionStart);
            VERIFY_IS_NULL(richTextBlock->SelectionEnd);

            // Select is ignored while selection is disabled.
            richTextBlock->Select(PositionAt(run->ContentStart, 2), PositionAt(run->ContentStart, 6));
            VERIFY_IS_NULL(richTextBlock->SelectionStart);
            VERIFY_IS_NULL(richTextBlock->SelectionEnd);

            richTextBlock->IsTextSelectionEnabled = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto start = run->ContentStart;

            richTextBlock->Select(PositionAt(start, 2), PositionAt(start, 6));
            VerifySelection(richTextBlock, PositionAt(start, 2)->Offset, PositionAt(start, 6)->Offset, L"cdef");
            VERIFY_IS_TRUE(AreSameObject(run, richTextBlock->SelectionStart->Parent));
            VERIFY_IS_TRUE(AreSameObject(richTextBlock, richTextBlock->SelectionEnd->VisualParent));

            // Reversed arguments are rejected (the selection manager requires start <= end) and the current
            // selection is kept.
            VERIFY_THROWS_SPECIFIC_WINRT(
                richTextBlock->Select(PositionAt(start, 7), PositionAt(start, 3)),
                Platform::Exception^,
                [](Platform::Exception^ ex) { return ex->HResult == E_UNEXPECTED; });
            VerifySelection(richTextBlock, PositionAt(start, 2)->Offset, PositionAt(start, 6)->Offset, L"cdef");

            richTextBlock->Select(PositionAt(start, 3), PositionAt(start, 7));
            VerifySelection(richTextBlock, PositionAt(start, 3)->Offset, PositionAt(start, 7)->Offset, L"defg");

            // An empty selection reports the caret position as both edges.
            richTextBlock->Select(PositionAt(start, 4), PositionAt(start, 4));
            VerifySelection(richTextBlock, PositionAt(start, 4)->Offset, PositionAt(start, 4)->Offset, nullptr);

            // First and last character edges.
            richTextBlock->Select(PositionAt(start, 0), PositionAt(start, 1));
            VerifySelection(richTextBlock, start->Offset, PositionAt(start, 1)->Offset, L"a");
            richTextBlock->Select(PositionAt(start, 7), PositionAt(start, 8));
            VerifySelection(richTextBlock, PositionAt(start, 7)->Offset, run->ContentEnd->Offset, L"h");

            richTextBlock->SelectAll();
            auto selectionStart = richTextBlock->SelectionStart;
            auto selectionEnd = richTextBlock->SelectionEnd;
            VERIFY_IS_NOT_NULL(selectionStart);
            VERIFY_IS_NOT_NULL(selectionEnd);
            VERIFY_IS_LESS_THAN_OR_EQUAL(selectionStart->Offset, start->Offset);
            VERIFY_IS_GREATER_THAN_OR_EQUAL(selectionEnd->Offset, run->ContentEnd->Offset);

            std::wstring selectedText(richTextBlock->SelectedText->Data());
            while (!selectedText.empty() && (selectedText.back() == L'\r' || selectedText.back() == L'\n'))
            {
                selectedText.pop_back();
            }
            VERIFY_ARE_EQUAL(std::wstring(L"abcdefgh"), selectedText);
        });
    }

    void RichTextBlockIntegrationTests::VerifyGetPositionFromPointMapsToCharacterEdges()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::RichTextBlock^ richTextBlock = nullptr;
        xaml_docs::Run^ firstRun = nullptr;
        xaml_docs::Run^ secondRun = nullptr;

        RunOnUIThread([&]()
        {
            richTextBlock = CreateRichTextBlock(400, 20, L"Consolas");
            firstRun = AppendParagraphWithRun(richTextBlock, L"abcdef");
            secondRun = AppendParagraphWithRun(richTextBlock, L"ghij");

            auto root = CreateTopLeftPanel();
            root->Children->Append(richTextBlock);
            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto firstStart = firstRun->ContentStart;
            auto offsetAtPoint = [&](float x, float y)
            {
                auto position = richTextBlock->GetPositionFromPoint(wf::Point(x, y));
                VERIFY_IS_NOT_NULL(position);
                VERIFY_IS_TRUE(AreSameObject(richTextBlock, position->VisualParent));
                return position->Offset;
            };

            VERIFY_ARE_EQUAL(firstStart->Offset, offsetAtPoint(0, 0));

            for (int i = 0; i < 6; i++)
            {
                const wf::Rect leadingEdge = CharacterRect(firstStart, i, xaml_docs::LogicalDirection::Forward);
                const wf::Rect trailingEdge = CharacterRect(firstStart, i + 1, xaml_docs::LogicalDirection::Backward);
                const float width = trailingEdge.X - leadingEdge.X;
                const float y = leadingEdge.Y + leadingEdge.Height / 2;
                LOG_OUTPUT(L"Character %d: left %f, right %f, line %f+%f", i, leadingEdge.X, trailingEdge.X, leadingEdge.Y, leadingEdge.Height);
                VERIFY_IS_GREATER_THAN(width, 4.0f);
                VERIFY_IS_GREATER_THAN(leadingEdge.Height, 0.0f);

                const int leadingOffset = PositionAt(firstStart, i)->Offset;
                const int trailingOffset = PositionAt(firstStart, i + 1)->Offset;

                VERIFY_ARE_EQUAL(leadingOffset, offsetAtPoint(leadingEdge.X, y));
                VERIFY_ARE_EQUAL(leadingOffset, offsetAtPoint(leadingEdge.X + 1, y));
                VERIFY_ARE_EQUAL(leadingOffset, offsetAtPoint(leadingEdge.X + width * 0.25f, y));
                VERIFY_ARE_EQUAL(trailingOffset, offsetAtPoint(leadingEdge.X + width * 0.75f, y));
                VERIFY_ARE_EQUAL(trailingOffset, offsetAtPoint(trailingEdge.X - 1, y));
            }

            // Right of the end of the first line maps to the end of that line, not to the next paragraph.
            const wf::Rect lastCharacterTrailingEdge = CharacterRect(firstStart, 6, xaml_docs::LogicalDirection::Backward);
            VERIFY_ARE_EQUAL(
                firstRun->ContentEnd->Offset,
                offsetAtPoint(lastCharacterTrailingEdge.X + 100, lastCharacterTrailingEdge.Y + lastCharacterTrailingEdge.Height / 2));

            auto secondStart = secondRun->ContentStart;
            const wf::Rect h = CharacterRect(secondStart, 1, xaml_docs::LogicalDirection::Forward);
            const wf::Rect i = CharacterRect(secondStart, 2, xaml_docs::LogicalDirection::Forward);
            const wf::Rect j = CharacterRect(secondStart, 3, xaml_docs::LogicalDirection::Forward);
            VERIFY_IS_GREATER_THAN(h.Y, lastCharacterTrailingEdge.Y);

            VERIFY_ARE_EQUAL(PositionAt(secondStart, 1)->Offset, offsetAtPoint(h.X + (i.X - h.X) * 0.25f, h.Y + h.Height / 2));

            // Below the last line maps to the closest position on the last line.
            VERIFY_ARE_EQUAL(PositionAt(secondStart, 2)->Offset, offsetAtPoint(i.X + (j.X - i.X) * 0.25f, i.Y + i.Height + 30));
        });
    }

    void RichTextBlockIntegrationTests::VerifyBaselineOffsetReflectsPaddingAndFontSize()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::RichTextBlock^ unpadded = nullptr;
        xaml_controls::RichTextBlock^ padded = nullptr;
        xaml_docs::Run^ unpaddedRun = nullptr;
        xaml_controls::RichTextBlock^ linked = nullptr;
        xaml_controls::RichTextBlockOverflow^ overflow = nullptr;
        xaml_docs::Run^ overflowRun = nullptr;

        RunOnUIThread([&]()
        {
            unpadded = CreateRichTextBlock(400, 20, L"Segoe UI");
            unpaddedRun = AppendParagraphWithRun(unpadded, L"Baseline");

            padded = CreateRichTextBlock(400, 20, L"Segoe UI");
            padded->Padding = xaml::ThicknessHelper::FromLengths(0, 13, 0, 0);
            AppendParagraphWithRun(padded, L"Baseline");

            // Same first line as 'unpadded'; its second paragraph flows into the overflow element once the height is
            // limited to one line below.
            linked = CreateRichTextBlock(400, 20, L"Segoe UI");
            AppendParagraphWithRun(linked, L"Baseline");
            overflowRun = AppendParagraphWithRun(linked, L"Baseline");
            overflow = ref new xaml_controls::RichTextBlockOverflow();
            overflow->HorizontalAlignment = xaml::HorizontalAlignment::Left;
            overflow->Width = 400;
            linked->OverflowContentTarget = overflow;

            auto root = CreateTopLeftPanel();
            root->Children->Append(unpadded);
            root->Children->Append(padded);
            root->Children->Append(linked);
            root->Children->Append(overflow);
            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        double smallFontBaseline = 0;
        RunOnUIThread([&]()
        {
            smallFontBaseline = unpadded->BaselineOffset;
            const wf::Rect firstLine = unpaddedRun->ContentStart->GetCharacterRect(xaml_docs::LogicalDirection::Forward);
            LOG_OUTPUT(L"Baseline %f, padded baseline %f, line height %f", smallFontBaseline, padded->BaselineOffset, firstLine.Height);

            VERIFY_IS_GREATER_THAN(smallFontBaseline, 0.0);
            VERIFY_IS_LESS_THAN(smallFontBaseline, static_cast<double>(firstLine.Height));
            VerifyAreClose(static_cast<float>(smallFontBaseline + 13), static_cast<float>(padded->BaselineOffset), L"Top padding moves the baseline down");

            // Room for the first line only: the second paragraph moves to the overflow element.
            linked->Height = firstLine.Height * 1.5;
            unpadded->FontSize = 40;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Baseline after FontSize 40: %f", unpadded->BaselineOffset);
            VERIFY_IS_GREATER_THAN(unpadded->BaselineOffset, smallFontBaseline + 1);

            // RichTextBlock and RichTextBlockOverflow both report the distance from their top edge to the baseline of
            // their first line. Both first lines hold the same text and font, so the values must agree.
            VERIFY_IS_TRUE(linked->HasOverflowContent);
            VERIFY_IS_TRUE(overflow->ContentStart->Offset <= overflowRun->ContentStart->Offset);
            LOG_OUTPUT(L"Linked baseline %f, overflow baseline %f", linked->BaselineOffset, overflow->BaselineOffset);
            VerifyAreClose(static_cast<float>(smallFontBaseline), static_cast<float>(linked->BaselineOffset), L"Same first line, same baseline");
            VerifyAreClose(static_cast<float>(overflow->BaselineOffset), static_cast<float>(linked->BaselineOffset), L"RichTextBlock and its overflow report the same first-line baseline");
        });
    }

    void RichTextBlockIntegrationTests::VerifyDisconnectChildrenRecursiveClearsRichTextBlockContent()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::StackPanel^ disconnectedPanel = nullptr;
        xaml_controls::RichTextBlock^ richTextBlock = nullptr;
        xaml_docs::Paragraph^ firstParagraph = nullptr;
        xaml_docs::Paragraph^ secondParagraph = nullptr;
        xaml_docs::Span^ span = nullptr;
        xaml_docs::InlineUIContainer^ container = nullptr;
        xaml_controls::Button^ button = nullptr;
        xaml_controls::RichTextBlock^ untouchedRichTextBlock = nullptr;
        Platform::String^ disconnectedTag = L"disconnectedTag";
        Platform::String^ disconnectedContext = L"disconnectedContext";
        Platform::String^ untouchedTag = L"untouchedTag";
        Platform::String^ untouchedContext = L"untouchedContext";

        RunOnUIThread([&]()
        {
            richTextBlock = CreateRichTextBlock(400, 20, L"Segoe UI");

            auto run = ref new xaml_docs::Run();
            run->Text = L"first ";
            auto spanRun = ref new xaml_docs::Run();
            spanRun->Text = L"span";
            span = ref new xaml_docs::Span();
            span->Inlines->Append(spanRun);
            button = ref new xaml_controls::Button();
            button->Content = L"button";
            container = ref new xaml_docs::InlineUIContainer();
            container->Child = button;

            firstParagraph = ref new xaml_docs::Paragraph();
            firstParagraph->Inlines->Append(run);
            firstParagraph->Inlines->Append(span);
            firstParagraph->Inlines->Append(container);
            richTextBlock->Blocks->Append(firstParagraph);

            AppendParagraphWithRun(richTextBlock, L"second");
            secondParagraph = safe_cast<xaml_docs::Paragraph^>(richTextBlock->Blocks->GetAt(1));

            untouchedRichTextBlock = CreateRichTextBlock(400, 20, L"Segoe UI");
            AppendParagraphWithRun(untouchedRichTextBlock, L"untouched");

            // FrameworkElement state that a disconnect clears on every element it visits.
            richTextBlock->Tag = disconnectedTag;
            richTextBlock->DataContext = disconnectedContext;
            untouchedRichTextBlock->Tag = untouchedTag;
            untouchedRichTextBlock->DataContext = untouchedContext;

            disconnectedPanel = CreateTopLeftPanel();
            disconnectedPanel->Children->Append(richTextBlock);
            auto untouchedPanel = CreateTopLeftPanel();
            untouchedPanel->Children->Append(untouchedRichTextBlock);

            auto root = CreateTopLeftPanel();
            root->Children->Append(disconnectedPanel);
            root->Children->Append(untouchedPanel);
            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(2u, richTextBlock->Blocks->Size);
            VERIFY_ARE_EQUAL(3u, firstParagraph->Inlines->Size);
            VERIFY_ARE_EQUAL(1u, span->Inlines->Size);
            VERIFY_IS_TRUE(AreSameObject(button, container->Child));
            VERIFY_IS_NOT_NULL(xaml_media::VisualTreeHelper::GetParent(button));
            VERIFY_ARE_EQUAL(disconnectedTag, safe_cast<Platform::String^>(richTextBlock->Tag));
            VERIFY_ARE_EQUAL(disconnectedContext, safe_cast<Platform::String^>(richTextBlock->DataContext));

            xaml_media::VisualTreeHelper::DisconnectChildrenRecursive(disconnectedPanel);

            VERIFY_ARE_EQUAL(0u, richTextBlock->Blocks->Size);
            VERIFY_ARE_EQUAL(0u, firstParagraph->Inlines->Size);
            VERIFY_ARE_EQUAL(0u, secondParagraph->Inlines->Size);
            VERIFY_ARE_EQUAL(0u, span->Inlines->Size);
            VERIFY_IS_NULL(container->Child);
            VERIFY_IS_NULL(xaml_media::VisualTreeHelper::GetParent(button));

            // The RichTextBlock's own FrameworkElement state is cleared as well.
            VERIFY_IS_NULL(richTextBlock->Tag);
            VERIFY_IS_NULL(richTextBlock->DataContext);

            // Content outside the disconnected subtree is unchanged.
            VERIFY_ARE_EQUAL(1u, untouchedRichTextBlock->Blocks->Size);
            VERIFY_ARE_EQUAL(1u, safe_cast<xaml_docs::Paragraph^>(untouchedRichTextBlock->Blocks->GetAt(0))->Inlines->Size);
            VERIFY_ARE_EQUAL(untouchedTag, safe_cast<Platform::String^>(untouchedRichTextBlock->Tag));
            VERIFY_ARE_EQUAL(untouchedContext, safe_cast<Platform::String^>(untouchedRichTextBlock->DataContext));
        });
    }

    void RichTextBlockIntegrationTests::VerifySelectNormalizesToClusterBoundaries()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::RichTextBlock^ richTextBlock = nullptr;
        xaml_docs::Run^ firstRun = nullptr;
        xaml_docs::Bold^ bold = nullptr;
        xaml_docs::Run^ lastRun = nullptr;

        RunOnUIThread([&]()
        {
            richTextBlock = CreateRichTextBlock(400, 20, L"Segoe UI");

            // "a" + U+1F600 (surrogate pair) | Bold "b" | "e" + U+0301 (combining acute) + U+1F600 + "c"
            firstRun = ref new xaml_docs::Run();
            firstRun->Text = L"a\U0001F600";
            auto boldRun = ref new xaml_docs::Run();
            boldRun->Text = L"b";
            bold = ref new xaml_docs::Bold();
            bold->Inlines->Append(boldRun);
            lastRun = ref new xaml_docs::Run();
            lastRun->Text = L"e\u0301\U0001F600c";

            auto paragraph = ref new xaml_docs::Paragraph();
            paragraph->Inlines->Append(firstRun);
            paragraph->Inlines->Append(bold);
            paragraph->Inlines->Append(lastRun);
            richTextBlock->Blocks->Append(paragraph);

            auto root = CreateTopLeftPanel();
            root->Children->Append(richTextBlock);
            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto first = firstRun->ContentStart;
            auto last = lastRun->ContentStart;
            const int firstEmojiStart = PositionAt(first, 1)->Offset;
            const int lastEmojiEnd = PositionAt(last, 4)->Offset;

            // Positions already on cluster boundaries are kept.
            richTextBlock->Select(PositionAt(first, 1), PositionAt(last, 2));
            VerifySelection(richTextBlock, firstEmojiStart, PositionAt(last, 2)->Offset, L"\U0001F600be\u0301");

            // A caret inside a surrogate pair moves back to the start of the pair.
            richTextBlock->Select(PositionAt(first, 2), PositionAt(first, 2));
            VerifySelection(richTextBlock, firstEmojiStart, firstEmojiStart, nullptr);

            // A range expands outwards: the start moves back, the end moves forward.
            richTextBlock->Select(PositionAt(first, 2), PositionAt(last, 3));
            VerifySelection(richTextBlock, firstEmojiStart, lastEmojiEnd, L"\U0001F600be\u0301\U0001F600");

            // The same range given in reverse order is rejected and the normalized selection is kept.
            VERIFY_THROWS_SPECIFIC_WINRT(
                richTextBlock->Select(PositionAt(last, 3), PositionAt(first, 2)),
                Platform::Exception^,
                [](Platform::Exception^ ex) { return ex->HResult == E_UNEXPECTED; });
            VerifySelection(richTextBlock, firstEmojiStart, lastEmojiEnd, L"\U0001F600be\u0301\U0001F600");

            // A base character and its combining mark are never split.
            richTextBlock->Select(PositionAt(last, 0), PositionAt(last, 1));
            VerifySelection(richTextBlock, last->Offset, PositionAt(last, 2)->Offset, L"e\u0301");
            richTextBlock->Select(PositionAt(last, 1), PositionAt(last, 1));
            VerifySelection(richTextBlock, last->Offset, last->Offset, nullptr);

            // Starting at the Bold element edge: the start never moves forward past the requested position and
            // never lands inside a cluster; the already normalized end is kept.
            richTextBlock->Select(bold->ElementStart, PositionAt(last, 4));
            auto selectionStart = richTextBlock->SelectionStart;
            VERIFY_IS_NOT_NULL(selectionStart);
            VERIFY_IS_LESS_THAN_OR_EQUAL(selectionStart->Offset, bold->ElementStart->Offset);
            VERIFY_ARE_EQUAL(lastEmojiEnd, richTextBlock->SelectionEnd->Offset);
            std::wstring selectedText(richTextBlock->SelectedText->Data());
            LOG_OUTPUT(L"Selected text length from the Bold edge: %u", static_cast<unsigned int>(selectedText.length()));
            const std::wstring expectedSuffix(L"be\u0301\U0001F600");
            VERIFY_IS_GREATER_THAN_OR_EQUAL(selectedText.length(), expectedSuffix.length());
            VERIFY_ARE_EQUAL(expectedSuffix, selectedText.substr(selectedText.length() - expectedSuffix.length()));
            VERIFY_IS_FALSE(IS_LOW_SURROGATE(selectedText.front()));
        });
    }

    void RichTextBlockIntegrationTests::VerifyCharacterRectsFollowBidiVisualOrder()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::RichTextBlock^ singleLine = nullptr;
        xaml_controls::RichTextBlock^ wrapped = nullptr;
        xaml_docs::Run^ singleLineRun = nullptr;
        xaml_docs::Run^ wrappedRun = nullptr;

        // Hebrew words: shalom olam gadol veyafe meod hayom.
        const std::wstring hebrewSentence(
            L"\u05E9\u05DC\u05D5\u05DD \u05E2\u05D5\u05DC\u05DD \u05D2\u05D3\u05D5\u05DC "
            L"\u05D5\u05D9\u05E4\u05D4 \u05DE\u05D0\u05D5\u05D3 \u05D4\u05D9\u05D5\u05DD");
        const int wrappedHebrewStart = 3;
        const int wrappedHebrewEnd = wrappedHebrewStart + static_cast<int>(hebrewSentence.length());

        RunOnUIThread([&]()
        {
            singleLine = CreateRichTextBlock(600, 20, L"Segoe UI");
            singleLine->FlowDirection = xaml::FlowDirection::LeftToRight;
            singleLineRun = AppendParagraphWithRun(singleLine, L"abc \u05D0\u05D1\u05D2 xyz");

            wrapped = CreateRichTextBlock(150, 20, L"Segoe UI");
            wrapped->FlowDirection = xaml::FlowDirection::LeftToRight;
            wrapped->TextWrapping = xaml::TextWrapping::Wrap;
            wrappedRun = AppendParagraphWithRun(wrapped, ref new Platform::String((L"ab " + hebrewSentence + L" cd").c_str()));

            auto root = CreateTopLeftPanel();
            root->Children->Append(singleLine);
            root->Children->Append(wrapped);
            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto start = singleLineRun->ContentStart;
            std::vector<wf::Rect> rects;
            for (int i = 0; i < 11; i++)
            {
                rects.push_back(CharacterRect(start, i, xaml_docs::LogicalDirection::Forward));
                LOG_OUTPUT(L"Character %d: x %f, y %f, height %f", i, rects[i].X, rects[i].Y, rects[i].Height);
                VERIFY_IS_GREATER_THAN(rects[i].Height, 0.0f);
                VerifyAreClose(rects[0].Y, rects[i].Y, L"All characters are on the first line");
            }

            // Latin characters run left to right.
            VERIFY_IS_LESS_THAN(rects[0].X, rects[1].X);
            VERIFY_IS_LESS_THAN(rects[1].X, rects[2].X);

            // Hebrew characters run right to left inside the left-to-right paragraph.
            VERIFY_IS_GREATER_THAN(rects[4].X, rects[5].X);
            VERIFY_IS_GREATER_THAN(rects[5].X, rects[6].X);

            // The Hebrew run sits between the Latin words.
            VERIFY_IS_LESS_THAN(rects[2].X, rects[6].X);
            VERIFY_IS_LESS_THAN(rects[4].X, rects[8].X);
            VERIFY_IS_LESS_THAN(rects[8].X, rects[9].X);
            VERIFY_IS_LESS_THAN(rects[9].X, rects[10].X);

            auto wrappedStart = wrappedRun->ContentStart;
            const wf::Rect firstLatin = CharacterRect(wrappedStart, 0, xaml_docs::LogicalDirection::Forward);
            const wf::Rect firstHebrew = CharacterRect(wrappedStart, wrappedHebrewStart, xaml_docs::LogicalDirection::Forward);
            const wf::Rect secondHebrew = CharacterRect(wrappedStart, wrappedHebrewStart + 1, xaml_docs::LogicalDirection::Forward);
            const wf::Rect lastHebrew = CharacterRect(wrappedStart, wrappedHebrewEnd - 1, xaml_docs::LogicalDirection::Forward);
            const wf::Rect trailingLatin = CharacterRect(wrappedStart, wrappedHebrewEnd + 1, xaml_docs::LogicalDirection::Forward);
            LOG_OUTPUT(L"Wrapped: first Hebrew y %f, last Hebrew y %f, trailing Latin y %f", firstHebrew.Y, lastHebrew.Y, trailingLatin.Y);

            VerifyAreClose(firstLatin.Y, firstHebrew.Y, L"The Hebrew run starts on the first line");
            VERIFY_IS_GREATER_THAN(firstHebrew.X, secondHebrew.X);
            VERIFY_IS_GREATER_THAN(lastHebrew.Y, firstHebrew.Y + c_layoutTolerance);
            VERIFY_IS_GREATER_THAN(trailingLatin.Y, firstLatin.Y + c_layoutTolerance);
        });
    }

    void RichTextBlockIntegrationTests::VerifyBoundingRectanglesForRangesCrossingBidiRuns()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

        xaml_controls::RichTextBlock^ richTextBlock = nullptr;
        xaml_docs::Run^ run = nullptr;

        RunOnUIThread([&]()
        {
            richTextBlock = CreateRichTextBlock(400, 20, L"Segoe UI");
            richTextBlock->FlowDirection = xaml::FlowDirection::LeftToRight;
            // Indexes: abc=0-2, Hebrew dalet he vav=4-6, digits 1 2 3=8-10 (nested inside the Hebrew run),
            // Hebrew zayin het=12-13, xyz=15-17.
            run = AppendParagraphWithRun(richTextBlock, L"abc \u05D3\u05D4\u05D5 123 \u05D6\u05D7 xyz");

            auto root = CreateTopLeftPanel();
            root->Children->Append(richTextBlock);
            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto peer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(richTextBlock);
            VERIFY_IS_NOT_NULL(peer);
            VERIFY_IS_NULL(peer->GetPattern(xaml_automation_peers::PatternInterface::Invoke));
            VERIFY_IS_NULL(peer->GetPattern(xaml_automation_peers::PatternInterface::Value));
            auto textPattern = peer->GetPattern(xaml_automation_peers::PatternInterface::Text);
            VERIFY_IS_NOT_NULL(textPattern);
            VERIFY_IS_TRUE(AreSameObject(textPattern, peer->GetPattern(xaml_automation_peers::PatternInterface::Text)));
            auto textProvider = GetTextProvider(richTextBlock);

            const int length = static_cast<int>(run->Text->Length());
            std::vector<wf::Rect> characterRectangles;
            for (int i = 0; i < length; i++)
            {
                auto rectangles = GetRunRangeBoundingRectangles(richTextBlock, textProvider, run, i, i + 1);
                VERIFY_ARE_EQUAL(static_cast<size_t>(1), rectangles.size());
                VERIFY_IS_GREATER_THAN(rectangles[0].Width, 0.0f);
                characterRectangles.push_back(rectangles[0]);
            }

            // Visual order check of the reference rectangles: dalet is right of he, digits run left to right.
            VERIFY_IS_GREATER_THAN(characterRectangles[4].X, characterRectangles[5].X);
            VERIFY_IS_LESS_THAN(characterRectangles[8].X, characterRectangles[9].X);

            // Latin into the right-to-left run.
            auto latinIntoHebrew = GetRunRangeBoundingRectangles(richTextBlock, textProvider, run, 1, 6);
            VERIFY_IS_GREATER_THAN_OR_EQUAL(latinIntoHebrew.size(), static_cast<size_t>(2));
            VerifyRangeCoverage(latinIntoHebrew, characterRectangles, 1, 6, L"Latin into Hebrew");

            // Digits nested in the right-to-left run out to the trailing Latin text.
            auto nestedDigitsToLatin = GetRunRangeBoundingRectangles(richTextBlock, textProvider, run, 9, 17);
            VERIFY_IS_GREATER_THAN_OR_EQUAL(nestedDigitsToLatin.size(), static_cast<size_t>(2));
            VerifyRangeCoverage(nestedDigitsToLatin, characterRectangles, 9, 17, L"Nested digits to Latin");

            // Latin into the digits nested in the right-to-left run.
            auto latinIntoNestedDigits = GetRunRangeBoundingRectangles(richTextBlock, textProvider, run, 1, 10);
            VERIFY_IS_GREATER_THAN_OR_EQUAL(latinIntoNestedDigits.size(), static_cast<size_t>(2));
            VerifyRangeCoverage(latinIntoNestedDigits, characterRectangles, 1, 10, L"Latin into nested digits");

            // Ranges that end exactly at, or one character past, the end of the nested digits and of the enclosing
            // right-to-left run, and ranges that end inside the second Hebrew word of that run.
            const std::vector<std::pair<int, int>> boundaryRanges = {
                { 9, 11 }, { 9, 12 }, { 9, 13 }, { 9, 14 }, { 9, 15 },
                { 5, 11 }, { 5, 12 }, { 5, 14 }, { 5, 15 },
                { 1, 13 }, { 1, 14 }, { 1, 15 } };
            for (const auto& boundaryRange : boundaryRanges)
            {
                const std::wstring rangeName =
                    L"Range [" + std::to_wstring(boundaryRange.first) + L"," + std::to_wstring(boundaryRange.second) + L")";
                auto rectangles = GetRunRangeBoundingRectangles(richTextBlock, textProvider, run, boundaryRange.first, boundaryRange.second);
                VerifyRangeCoverage(rectangles, characterRectangles, boundaryRange.first, boundaryRange.second, rangeName.c_str());
            }
        });
    }

    void RichTextBlockIntegrationTests::VerifyWrapWholeWordsKeepsOverlongWordOnOneLine()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::RichTextBlock^ wrapWholeWords = nullptr;
        xaml_controls::RichTextBlock^ wrap = nullptr;
        xaml_docs::Run^ wrapWholeWordsRun = nullptr;
        xaml_docs::Run^ wrapRun = nullptr;

        const std::wstring longWord(L"Pneumonoultramicroscopicsilicovolcanoconiosis");
        const int longWordStart = 3;
        const int longWordLast = longWordStart + static_cast<int>(longWord.length()) - 1;
        const int trailingWordStart = longWordLast + 2;
        Platform::String^ text = ref new Platform::String((L"aa " + longWord + L" bb").c_str());

        RunOnUIThread([&]()
        {
            wrapWholeWords = CreateRichTextBlock(120, 20, L"Segoe UI");
            wrapWholeWords->TextWrapping = xaml::TextWrapping::WrapWholeWords;
            wrapWholeWordsRun = AppendParagraphWithRun(wrapWholeWords, text);

            wrap = CreateRichTextBlock(120, 20, L"Segoe UI");
            wrap->TextWrapping = xaml::TextWrapping::Wrap;
            wrapRun = AppendParagraphWithRun(wrap, text);

            auto root = CreateTopLeftPanel();
            root->Children->Append(wrapWholeWords);
            root->Children->Append(wrap);
            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto start = wrapWholeWordsRun->ContentStart;
            const wf::Rect leadingWord = CharacterRect(start, 0, xaml_docs::LogicalDirection::Forward);
            const wf::Rect longWordFirst = CharacterRect(start, longWordStart, xaml_docs::LogicalDirection::Forward);
            const wf::Rect longWordEnd = CharacterRect(start, longWordLast, xaml_docs::LogicalDirection::Forward);
            const wf::Rect trailingWord = CharacterRect(start, trailingWordStart, xaml_docs::LogicalDirection::Forward);
            LOG_OUTPUT(L"WrapWholeWords: aa y %f, long word y %f..%f (last x %f), bb y %f", leadingWord.Y, longWordFirst.Y, longWordEnd.Y, longWordEnd.X, trailingWord.Y);

            VerifyAreClose(longWordFirst.Y, longWordEnd.Y, L"The over-long word stays on one line");
            VERIFY_IS_GREATER_THAN(longWordEnd.X, 120.0f);
            VERIFY_IS_GREATER_THAN(longWordFirst.Y, leadingWord.Y + c_layoutTolerance);
            VERIFY_IS_GREATER_THAN(trailingWord.Y, longWordFirst.Y + c_layoutTolerance);

            // Control: with Wrap the same word is broken because it does not fit.
            auto wrapStart = wrapRun->ContentStart;
            const wf::Rect wrapLongWordFirst = CharacterRect(wrapStart, longWordStart, xaml_docs::LogicalDirection::Forward);
            const wf::Rect wrapLongWordEnd = CharacterRect(wrapStart, longWordLast, xaml_docs::LogicalDirection::Forward);
            LOG_OUTPUT(L"Wrap: long word y %f..%f", wrapLongWordFirst.Y, wrapLongWordEnd.Y);
            VERIFY_IS_GREATER_THAN(wrapLongWordEnd.Y, wrapLongWordFirst.Y + c_layoutTolerance);
            VERIFY_IS_LESS_THAN_OR_EQUAL(wrapLongWordEnd.X, 120.0f);
        });
    }

    void RichTextBlockIntegrationTests::VerifyCharacterRectOfEmptyParagraph()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::RichTextBlock^ richTextBlock = nullptr;
        xaml_docs::Run^ firstRun = nullptr;
        xaml_docs::Paragraph^ emptyParagraph = nullptr;
        xaml_docs::Run^ lastRun = nullptr;

        RunOnUIThread([&]()
        {
            richTextBlock = CreateRichTextBlock(400, 20, L"Segoe UI");
            richTextBlock->Padding = xaml::ThicknessHelper::FromLengths(7, 0, 0, 0);
            firstRun = AppendParagraphWithRun(richTextBlock, L"abc");
            emptyParagraph = ref new xaml_docs::Paragraph();
            richTextBlock->Blocks->Append(emptyParagraph);
            lastRun = AppendParagraphWithRun(richTextBlock, L"def");

            auto root = CreateTopLeftPanel();
            root->Children->Append(richTextBlock);
            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            const wf::Rect first = firstRun->ContentStart->GetCharacterRect(xaml_docs::LogicalDirection::Forward);
            const wf::Rect empty = emptyParagraph->ContentStart->GetCharacterRect(xaml_docs::LogicalDirection::Forward);
            const wf::Rect last = lastRun->ContentStart->GetCharacterRect(xaml_docs::LogicalDirection::Forward);
            LOG_OUTPUT(L"First %f,%f h%f; empty %f,%f h%f; last %f,%f h%f", first.X, first.Y, first.Height, empty.X, empty.Y, empty.Height, last.X, last.Y, last.Height);

            VERIFY_IS_GREATER_THAN(first.Height, 0.0f);
            VerifyAreClose(7.0f, first.X, L"The first paragraph starts at the padded content origin");
            VerifyAreClose(first.X, empty.X, L"The empty paragraph starts at the content origin");
            VerifyAreClose(first.Height, empty.Height, L"The empty paragraph has a full line height");
            VerifyAreClose(first.Y + first.Height, empty.Y, L"The empty paragraph is on the line after the first paragraph");
            VerifyAreClose(empty.Y + empty.Height, last.Y, L"The last paragraph is on the line after the empty paragraph");
        });
    }

    void RichTextBlockIntegrationTests::VerifyWordNavigationAcrossInlineElementBoundaries()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::RichTextBlock^ richTextBlock = nullptr;
        xaml_docs::Run^ twoRun = nullptr;
        xaml_docs::InlineUIContainer^ container = nullptr;

        RunOnUIThread([&]()
        {
            richTextBlock = CreateRichTextBlock(600, 20, L"Segoe UI");

            auto makeRun = [](Platform::String^ text)
            {
                auto run = ref new xaml_docs::Run();
                run->Text = text;
                return run;
            };

            auto bold = ref new xaml_docs::Bold();
            twoRun = makeRun(L"two");
            bold->Inlines->Append(twoRun);
            auto italic = ref new xaml_docs::Italic();
            italic->Inlines->Append(makeRun(L"three"));
            auto border = ref new xaml_controls::Border();
            border->Width = 10;
            border->Height = 10;
            container = ref new xaml_docs::InlineUIContainer();
            container->Child = border;

            auto paragraph = ref new xaml_docs::Paragraph();
            paragraph->Inlines->Append(makeRun(L"one "));
            paragraph->Inlines->Append(bold);
            paragraph->Inlines->Append(makeRun(L" "));
            paragraph->Inlines->Append(italic);
            paragraph->Inlines->Append(makeRun(L" four "));
            paragraph->Inlines->Append(container);
            paragraph->Inlines->Append(makeRun(L" five"));
            richTextBlock->Blocks->Append(paragraph);

            auto root = CreateTopLeftPanel();
            root->Children->Append(richTextBlock);
            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            const std::vector<std::wstring> expectedWords = { L"one", L"two", L"three", L"four", L"five" };
            const int maxSteps = 20;

            auto range = GetTextProvider(richTextBlock)->DocumentRange->Clone();
            range->MoveEndpointByRange(
                xaml_automation::Text::TextPatternRangeEndpoint::End,
                range,
                xaml_automation::Text::TextPatternRangeEndpoint::Start);
            range->ExpandToEnclosingUnit(xaml_automation::Text::TextUnit::Word);

            // Walk forward word by word; each visited range holds at most one alphabetic word.
            std::vector<std::wstring> forwardWords;
            int steps = 0;
            for (; steps < maxSteps; steps++)
            {
                const std::wstring letters = LettersOf(range->GetText(-1));
                LOG_OUTPUT(L"Forward step %d: '%s'", steps, letters.c_str());
                if (!letters.empty())
                {
                    forwardWords.push_back(letters);
                }

                // Exact word ends at inline element edges, read back through the selection: the Bold element edges
                // count as word separators (the word "one " ends at the first letter of "two"), and the
                // InlineUIContainer start edge ends the preceding word.
                if (letters == L"one" || letters == L"four")
                {
                    range->Select();
                    VERIFY_IS_NOT_NULL(richTextBlock->SelectionEnd);
                    const int expectedEnd = (letters == L"one") ? twoRun->ContentStart->Offset : container->ElementStart->Offset;
                    LOG_OUTPUT(L"Word '%s' ends at %d, expected %d", letters.c_str(), richTextBlock->SelectionEnd->Offset, expectedEnd);
                    VERIFY_ARE_EQUAL(expectedEnd, richTextBlock->SelectionEnd->Offset);
                }

                const int moved = range->Move(xaml_automation::Text::TextUnit::Word, 1);
                if (moved == 0)
                {
                    break;
                }
                VERIFY_ARE_EQUAL(1, moved);
            }
            VERIFY_IS_LESS_THAN(steps, maxSteps);
            VERIFY_IS_TRUE(expectedWords == forwardWords);

            // Walking back visits the same words in reverse order.
            std::vector<std::wstring> backwardWords;
            for (steps = 0; steps < maxSteps; steps++)
            {
                const std::wstring letters = LettersOf(range->GetText(-1));
                LOG_OUTPUT(L"Backward step %d: '%s'", steps, letters.c_str());
                if (!letters.empty())
                {
                    backwardWords.push_back(letters);
                }

                if (range->Move(xaml_automation::Text::TextUnit::Word, -1) == 0)
                {
                    break;
                }
            }
            VERIFY_IS_LESS_THAN(steps, maxSteps);
            VERIFY_IS_TRUE(std::vector<std::wstring>(expectedWords.rbegin(), expectedWords.rend()) == backwardWords);
        });
    }

    void RichTextBlockIntegrationTests::VerifyRichTextBlockPropertiesRoundTripThroughProjection()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto richTextBlock = ref new xaml_controls::RichTextBlock();

            // Documented defaults.
            VERIFY_IS_TRUE(richTextBlock->IsTextSelectionEnabled);
            VERIFY_IS_TRUE(richTextBlock->IsColorFontEnabled);
            VERIFY_IS_TRUE(richTextBlock->IsTextScaleFactorEnabled);
            VERIFY_ARE_EQUAL(0, richTextBlock->MaxLines);
            VERIFY_ARE_EQUAL(0, richTextBlock->CharacterSpacing);
            VERIFY_ARE_EQUAL(0.0, richTextBlock->LineHeight);
            VERIFY_ARE_EQUAL(0.0, richTextBlock->TextIndent);
            VERIFY_ARE_EQUAL(xaml::LineStackingStrategy::MaxHeight, richTextBlock->LineStackingStrategy);
            VERIFY_ARE_EQUAL(xaml::TextLineBounds::Full, richTextBlock->TextLineBounds);
            VERIFY_ARE_EQUAL(xaml::OpticalMarginAlignment::None, richTextBlock->OpticalMarginAlignment);
            VERIFY_ARE_EQUAL(xaml::TextAlignment::Left, richTextBlock->TextAlignment);
            VERIFY_ARE_EQUAL(::Windows::UI::Text::TextDecorations::None, richTextBlock->TextDecorations);
            VERIFY_ARE_EQUAL(::Windows::UI::Text::FontStyle::Normal, richTextBlock->FontStyle);
            VERIFY_ARE_EQUAL(::Windows::UI::Text::FontStretch::Normal, richTextBlock->FontStretch);
            VERIFY_ARE_EQUAL(400, static_cast<int>(richTextBlock->FontWeight.Weight));
            VERIFY_ARE_EQUAL(0.0, richTextBlock->Padding.Left);
            VERIFY_ARE_EQUAL(0.0, richTextBlock->Padding.Top);
            VERIFY_ARE_EQUAL(0.0, richTextBlock->Padding.Right);
            VERIFY_ARE_EQUAL(0.0, richTextBlock->Padding.Bottom);
            VERIFY_IS_FALSE(richTextBlock->HasOverflowContent);
            VERIFY_IS_FALSE(richTextBlock->IsTextTrimmed);

            // Read-only properties agree with their dependency properties.
            VERIFY_ARE_EQUAL(richTextBlock->HasOverflowContent, safe_cast<bool>(richTextBlock->GetValue(xaml_controls::RichTextBlock::HasOverflowContentProperty)));
            VERIFY_ARE_EQUAL(richTextBlock->IsTextTrimmed, safe_cast<bool>(richTextBlock->GetValue(xaml_controls::RichTextBlock::IsTextTrimmedProperty)));
            VERIFY_ARE_EQUAL(richTextBlock->SelectedText, safe_cast<Platform::String^>(richTextBlock->GetValue(xaml_controls::RichTextBlock::SelectedTextProperty)));

            // Each property returns the value just set, both from the property and from its dependency property.
            // The values differ between properties so that a getter reading another property is detected.
            richTextBlock->FontSize = 23;
            VERIFY_ARE_EQUAL(23.0, richTextBlock->FontSize);
            VERIFY_ARE_EQUAL(23.0, safe_cast<double>(richTextBlock->GetValue(xaml_controls::RichTextBlock::FontSizeProperty)));

            auto fontFamily = ref new xaml_media::FontFamily(L"Consolas");
            richTextBlock->FontFamily = fontFamily;
            VERIFY_IS_TRUE(AreSameObject(fontFamily, richTextBlock->FontFamily));
            VERIFY_IS_TRUE(AreSameObject(fontFamily, richTextBlock->GetValue(xaml_controls::RichTextBlock::FontFamilyProperty)));

            ::Windows::UI::Text::FontWeight bold;
            bold.Weight = 700;
            richTextBlock->FontWeight = bold;
            VERIFY_ARE_EQUAL(700, static_cast<int>(richTextBlock->FontWeight.Weight));
            VERIFY_ARE_EQUAL(700, static_cast<int>(safe_cast<::Windows::UI::Text::FontWeight>(richTextBlock->GetValue(xaml_controls::RichTextBlock::FontWeightProperty)).Weight));

            richTextBlock->FontStyle = ::Windows::UI::Text::FontStyle::Italic;
            VERIFY_ARE_EQUAL(::Windows::UI::Text::FontStyle::Italic, richTextBlock->FontStyle);
            VERIFY_ARE_EQUAL(::Windows::UI::Text::FontStyle::Italic, safe_cast<::Windows::UI::Text::FontStyle>(richTextBlock->GetValue(xaml_controls::RichTextBlock::FontStyleProperty)));

            richTextBlock->FontStretch = ::Windows::UI::Text::FontStretch::Condensed;
            VERIFY_ARE_EQUAL(::Windows::UI::Text::FontStretch::Condensed, richTextBlock->FontStretch);
            VERIFY_ARE_EQUAL(::Windows::UI::Text::FontStretch::Condensed, safe_cast<::Windows::UI::Text::FontStretch>(richTextBlock->GetValue(xaml_controls::RichTextBlock::FontStretchProperty)));

            auto foreground = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
            richTextBlock->Foreground = foreground;
            VERIFY_IS_TRUE(AreSameObject(foreground, richTextBlock->Foreground));
            VERIFY_IS_TRUE(AreSameObject(foreground, richTextBlock->GetValue(xaml_controls::RichTextBlock::ForegroundProperty)));

            richTextBlock->Padding = xaml::ThicknessHelper::FromLengths(1, 2, 3, 4);
            const xaml::Thickness padding = safe_cast<xaml::Thickness>(richTextBlock->GetValue(xaml_controls::RichTextBlock::PaddingProperty));
            VERIFY_ARE_EQUAL(1.0, richTextBlock->Padding.Left);
            VERIFY_ARE_EQUAL(2.0, richTextBlock->Padding.Top);
            VERIFY_ARE_EQUAL(3.0, richTextBlock->Padding.Right);
            VERIFY_ARE_EQUAL(4.0, richTextBlock->Padding.Bottom);
            VERIFY_ARE_EQUAL(1.0, padding.Left);
            VERIFY_ARE_EQUAL(2.0, padding.Top);
            VERIFY_ARE_EQUAL(3.0, padding.Right);
            VERIFY_ARE_EQUAL(4.0, padding.Bottom);

            richTextBlock->LineHeight = 31;
            VERIFY_ARE_EQUAL(31.0, richTextBlock->LineHeight);
            VERIFY_ARE_EQUAL(31.0, safe_cast<double>(richTextBlock->GetValue(xaml_controls::RichTextBlock::LineHeightProperty)));

            richTextBlock->LineStackingStrategy = xaml::LineStackingStrategy::BlockLineHeight;
            VERIFY_ARE_EQUAL(xaml::LineStackingStrategy::BlockLineHeight, richTextBlock->LineStackingStrategy);
            VERIFY_ARE_EQUAL(xaml::LineStackingStrategy::BlockLineHeight, safe_cast<xaml::LineStackingStrategy>(richTextBlock->GetValue(xaml_controls::RichTextBlock::LineStackingStrategyProperty)));

            richTextBlock->CharacterSpacing = 120;
            VERIFY_ARE_EQUAL(120, richTextBlock->CharacterSpacing);
            VERIFY_ARE_EQUAL(120, safe_cast<int>(richTextBlock->GetValue(xaml_controls::RichTextBlock::CharacterSpacingProperty)));

            richTextBlock->MaxLines = 3;
            VERIFY_ARE_EQUAL(3, richTextBlock->MaxLines);
            VERIFY_ARE_EQUAL(3, safe_cast<int>(richTextBlock->GetValue(xaml_controls::RichTextBlock::MaxLinesProperty)));

            richTextBlock->TextAlignment = xaml::TextAlignment::Right;
            VERIFY_ARE_EQUAL(xaml::TextAlignment::Right, richTextBlock->TextAlignment);
            VERIFY_ARE_EQUAL(xaml::TextAlignment::Right, safe_cast<xaml::TextAlignment>(richTextBlock->GetValue(xaml_controls::RichTextBlock::TextAlignmentProperty)));

            richTextBlock->HorizontalTextAlignment = xaml::TextAlignment::Center;
            VERIFY_ARE_EQUAL(xaml::TextAlignment::Center, richTextBlock->HorizontalTextAlignment);
            VERIFY_ARE_EQUAL(xaml::TextAlignment::Center, safe_cast<xaml::TextAlignment>(richTextBlock->GetValue(xaml_controls::RichTextBlock::HorizontalTextAlignmentProperty)));

            richTextBlock->TextIndent = 17;
            VERIFY_ARE_EQUAL(17.0, richTextBlock->TextIndent);
            VERIFY_ARE_EQUAL(17.0, safe_cast<double>(richTextBlock->GetValue(xaml_controls::RichTextBlock::TextIndentProperty)));

            richTextBlock->TextLineBounds = xaml::TextLineBounds::Tight;
            VERIFY_ARE_EQUAL(xaml::TextLineBounds::Tight, richTextBlock->TextLineBounds);
            VERIFY_ARE_EQUAL(xaml::TextLineBounds::Tight, safe_cast<xaml::TextLineBounds>(richTextBlock->GetValue(xaml_controls::RichTextBlock::TextLineBoundsProperty)));

            richTextBlock->TextReadingOrder = xaml::TextReadingOrder::UseFlowDirection;
            VERIFY_ARE_EQUAL(xaml::TextReadingOrder::UseFlowDirection, richTextBlock->TextReadingOrder);
            VERIFY_ARE_EQUAL(xaml::TextReadingOrder::UseFlowDirection, safe_cast<xaml::TextReadingOrder>(richTextBlock->GetValue(xaml_controls::RichTextBlock::TextReadingOrderProperty)));
            richTextBlock->TextReadingOrder = xaml::TextReadingOrder::DetectFromContent;
            VERIFY_ARE_EQUAL(xaml::TextReadingOrder::DetectFromContent, richTextBlock->TextReadingOrder);
            VERIFY_ARE_EQUAL(xaml::TextReadingOrder::DetectFromContent, safe_cast<xaml::TextReadingOrder>(richTextBlock->GetValue(xaml_controls::RichTextBlock::TextReadingOrderProperty)));

            richTextBlock->TextDecorations = ::Windows::UI::Text::TextDecorations::Underline;
            VERIFY_ARE_EQUAL(::Windows::UI::Text::TextDecorations::Underline, richTextBlock->TextDecorations);
            VERIFY_ARE_EQUAL(::Windows::UI::Text::TextDecorations::Underline, safe_cast<::Windows::UI::Text::TextDecorations>(richTextBlock->GetValue(xaml_controls::RichTextBlock::TextDecorationsProperty)));

            richTextBlock->OpticalMarginAlignment = xaml::OpticalMarginAlignment::TrimSideBearings;
            VERIFY_ARE_EQUAL(xaml::OpticalMarginAlignment::TrimSideBearings, richTextBlock->OpticalMarginAlignment);
            VERIFY_ARE_EQUAL(xaml::OpticalMarginAlignment::TrimSideBearings, safe_cast<xaml::OpticalMarginAlignment>(richTextBlock->GetValue(xaml_controls::RichTextBlock::OpticalMarginAlignmentProperty)));

            richTextBlock->IsColorFontEnabled = false;
            VERIFY_IS_FALSE(richTextBlock->IsColorFontEnabled);
            VERIFY_IS_FALSE(safe_cast<bool>(richTextBlock->GetValue(xaml_controls::RichTextBlock::IsColorFontEnabledProperty)));

            richTextBlock->IsTextScaleFactorEnabled = false;
            VERIFY_IS_FALSE(richTextBlock->IsTextScaleFactorEnabled);
            VERIFY_IS_FALSE(safe_cast<bool>(richTextBlock->GetValue(xaml_controls::RichTextBlock::IsTextScaleFactorEnabledProperty)));

            auto highlight = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Blue);
            richTextBlock->SelectionHighlightColor = highlight;
            VERIFY_IS_TRUE(AreSameObject(highlight, richTextBlock->SelectionHighlightColor));
            VERIFY_IS_TRUE(AreSameObject(highlight, richTextBlock->GetValue(xaml_controls::RichTextBlock::SelectionHighlightColorProperty)));

            // Every dependency property identifier exists and identifies a different property.
            std::vector<xaml::DependencyProperty^> properties = {
                xaml_controls::RichTextBlock::FontSizeProperty,
                xaml_controls::RichTextBlock::FontFamilyProperty,
                xaml_controls::RichTextBlock::FontWeightProperty,
                xaml_controls::RichTextBlock::FontStyleProperty,
                xaml_controls::RichTextBlock::FontStretchProperty,
                xaml_controls::RichTextBlock::ForegroundProperty,
                xaml_controls::RichTextBlock::TextWrappingProperty,
                xaml_controls::RichTextBlock::TextTrimmingProperty,
                xaml_controls::RichTextBlock::TextAlignmentProperty,
                xaml_controls::RichTextBlock::PaddingProperty,
                xaml_controls::RichTextBlock::LineHeightProperty,
                xaml_controls::RichTextBlock::LineStackingStrategyProperty,
                xaml_controls::RichTextBlock::CharacterSpacingProperty,
                xaml_controls::RichTextBlock::OverflowContentTargetProperty,
                xaml_controls::RichTextBlock::IsTextSelectionEnabledProperty,
                xaml_controls::RichTextBlock::HasOverflowContentProperty,
                xaml_controls::RichTextBlock::SelectedTextProperty,
                xaml_controls::RichTextBlock::MaxLinesProperty,
                xaml_controls::RichTextBlock::TextLineBoundsProperty,
                xaml_controls::RichTextBlock::SelectionHighlightColorProperty,
                xaml_controls::RichTextBlock::OpticalMarginAlignmentProperty,
                xaml_controls::RichTextBlock::IsColorFontEnabledProperty,
                xaml_controls::RichTextBlock::TextReadingOrderProperty,
                xaml_controls::RichTextBlock::IsTextScaleFactorEnabledProperty,
                xaml_controls::RichTextBlock::TextDecorationsProperty,
                xaml_controls::RichTextBlock::IsTextTrimmedProperty,
                xaml_controls::RichTextBlock::HorizontalTextAlignmentProperty,
                xaml_controls::RichTextBlock::SelectionFlyoutProperty,
                xaml_controls::RichTextBlock::TextIndentProperty,
            };
            for (size_t i = 0; i < properties.size(); i++)
            {
                VERIFY_IS_NOT_NULL(properties[i]);
                for (size_t j = i + 1; j < properties.size(); j++)
                {
                    VERIFY_IS_FALSE(AreSameObject(properties[i], properties[j]));
                }
            }
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::RichTextBlock
