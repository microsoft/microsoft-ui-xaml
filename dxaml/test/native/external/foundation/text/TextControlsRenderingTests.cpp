// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TextControlsRenderingTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <ppltasks.h>
#include "FileLoader.h"
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <WUCRenderingScopeGuard.h>
#include <FocusTestHelper.h>
#include "TraceConsumerSession.h"
#include "MUX-ETWEvents.h"

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace MockDComp;
using namespace Microsoft::UI::Xaml::Media;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Text {
        bool TextControlsRenderingTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool TextControlsRenderingTests::ClassCleanup()
        {
            return true;
        }

        bool TextControlsRenderingTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool TextControlsRenderingTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        Platform::String^ TextControlsRenderingTests::GetPathToFiles() const
        {
            // Get the deployment directory, and then append our test's directory to the end
            auto deploymentDir = GetTestDeploymentDir();
            return ref new Platform::String(deploymentDir + L"resources\\native\\foundation\\text\\");
        }

        //------------------------------------------------------------------------
        // Test case: Validates various text controls rendering.
        //------------------------------------------------------------------------
        void TextControlsRenderingTests::ValidateTextControlsRendering()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            ::Windows::Foundation::Size size(800, 600);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto rootLoadedEvent = std::make_shared<Event>();
            auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);

            xaml_controls::Grid^ rootGrid = safe_cast<xaml_controls::Grid^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"TextControlsRenderingTests1.xaml"));
            VERIFY_IS_NOT_NULL(rootGrid);

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Listening to root FrameworkElement.Loaded.");
                rootLoadedRegistration.Attach(
                    rootGrid,
                    ref new xaml::RoutedEventHandler(
                    [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Root FE Loaded handler.");
                    rootLoadedEvent->Set();
                }));
                TestServices::WindowHelper->WindowContent = rootGrid;
            });

            LOG_OUTPUT(L"Waiting for root FE Loaded event.");
            rootLoadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Recording final DComp tree.");
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

            rootLoadedEvent->Reset();
            rootGrid = safe_cast<xaml_controls::Grid^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"TextControlsRenderingTests2.xaml"));
            VERIFY_IS_NOT_NULL(rootGrid);

            RunOnUIThread([&]()
            {
                rootLoadedRegistration.Detach();
                TestServices::WindowHelper->WindowContent = rootGrid;
                LOG_OUTPUT(L"Listening to root FrameworkElement.Loaded.");
                rootLoadedRegistration.Attach(
                    rootGrid,
                    ref new xaml::RoutedEventHandler(
                    [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Root FE Loaded handler.");
                    rootLoadedEvent->Set();
                }));
            });

            LOG_OUTPUT(L"Waiting for root FE Loaded event.");
            rootLoadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Recording final DComp tree.");
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");
        }

        void TextControlsRenderingTests::ValidateTextControlsWithLargeFontSize()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::TextBox^ textBox = nullptr;
            // Based on the font size (>130 in pts, >174 in font size), we turn on/off RichEdit's ideal rendering mode (subpixel rendering).
            // This test makes sure rendering ok for various font sizes.
            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button Content='Focus' Margin='20,0,20,0' />"
                    L"  <TextBox x:Name='textBox' Text='A' IsSpellCheckEnabled='False' FontSize='20' Width='200' Padding='10,3,6,5'/>"
                    L"</StackPanel>"));
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Changing TextBox's font size to 174...");
                textBox->FontSize = 174;
            });
            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Recording DComp tree.");
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Changing TextBox's font size to 175...");
                textBox->FontSize = 175;
            });
            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Recording DComp tree.");
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"3");

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Changing TextBox's font size to 300...");
                textBox->FontSize = 300;
            });
            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Recording DComp tree.");
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"4");

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Changing TextBox's font size back to 20...");
                textBox->FontSize = 20;
            });
            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Recording DComp tree.");
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1"); // same dump as step #1

        }

        void TextControlsRenderingTests::ValidateRichEditBoxTOMLargeFontSize()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            ::Windows::Foundation::Size size(400, 800);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::RichEditBox^ richEditBox1 = nullptr;
            xaml_controls::RichEditBox^ richEditBox2 = nullptr;
            Microsoft::UI::Text::ITextRange ^range1;
            Microsoft::UI::Text::ITextCharacterFormat ^format1;
            Microsoft::UI::Text::ITextRange ^range2;
            Microsoft::UI::Text::ITextCharacterFormat ^format2;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='800' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <RichEditBox x:Name='richEditBox1' IsSpellCheckEnabled='False' FontSize='20' Width='200'/>"
                    L"  <RichEditBox x:Name='richEditBox2' IsSpellCheckEnabled='False' FontSize='20' Width='200'/>"
                    L"</StackPanel>"));
                richEditBox1 = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"richEditBox1"));
                VERIFY_IS_NOT_NULL(richEditBox1);
                richEditBox2 = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"richEditBox2"));
                VERIFY_IS_NOT_NULL(richEditBox2);
                TestServices::WindowHelper->WindowContent = rootPanel;
                richEditBox1->Document->SetText(Microsoft::UI::Text::TextSetOptions::None, L"A");
                range1 = richEditBox1->Document->GetRange(0, 0);
                range1->Expand(Microsoft::UI::Text::TextRangeUnit::Story);
                format1 = range1->CharacterFormat;
                format1->Size = 15; // equals font size 20

                richEditBox2->Document->SetText(Microsoft::UI::Text::TextSetOptions::None, L"B");
                range2 = richEditBox2->Document->GetRange(0, 0);
                range2->Expand(Microsoft::UI::Text::TextRangeUnit::Story);
                format2 = range2->CharacterFormat;
                format2->Size = 15; // equals font size 20
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Changing RichEditBox1's font size to 131.25 PTS(175 in font size) through TOM ITextCharacterFormat->Size...");
                format1->Size = 131.25;

                LOG_OUTPUT(L"Changing RichEditBox2's font size to 131.25 PTS(175 in font size) through TOM cloned ITextCharacterFormat...");
                Microsoft::UI::Text::ITextCharacterFormat ^formatClone = format2->GetClone();
                formatClone->Size = 131.25;
                range2->CharacterFormat = formatClone;
            });
            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Recording DComp tree.");
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Changing RichEditBox's font size to 225 PTS(300 in font size) through TOM ITextCharacterFormat->Size...");
                format1->Size = 225;

                LOG_OUTPUT(L"Changing RichEditBox2's font size to 225 PTS(300 in font size) through TOM cloned ITextCharacterFormat...");
                Microsoft::UI::Text::ITextCharacterFormat ^formatClone = format2->GetClone();
                formatClone->Size = 225;
                range2->CharacterFormat = formatClone;
            });
            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Recording DComp tree.");
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");
        }

        void TextControlsRenderingTests::ValidateTextControlsWithAlgerianFont()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <StackPanel.Resources>"
                    L"    <Style TargetType='TextBox'>"
                    L"      <Setter Property='Padding' Value='10,3,6,5'/>"
                    L"    </Style>"
                    L"  </StackPanel.Resources>"
                    L"  <TextBox Text='Algerian' IsSpellCheckEnabled='False' FontFamily='Algerian' FontSize='20' Width='200' IsEnabled='False'/>"
                    L"  <TextBox Text='Algerian Bold' IsSpellCheckEnabled='False' FontFamily='Algerian' FontWeight='Bold' FontSize='20' Width='200' IsEnabled='False'/>"
                    L"  <TextBlock Text='Algerian' FontFamily='Algerian' FontSize='20' Width='200'/>"
                    L"  <TextBlock Text='Algerian Bold' FontFamily='Algerian' FontWeight='Bold' FontSize='20' Width='200'/>"
                    L"</StackPanel>"));
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::ReferencedOnly);
        }

        void TextControlsRenderingTests::ValidateTextControlForegroundUpdate()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::Button^ button = nullptr;
            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name='button' Content = 'Focus' Margin = '20,0,20,0' />"
                    L"  <TextBox x:Name='textBox' Text='A' IsSpellCheckEnabled='False' FontSize='20' Width='200'/>"
                    L"</StackPanel>"));
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
                VERIFY_IS_NOT_NULL(button);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            auto redColor = Microsoft::UI::Colors::Red;

            RunOnUIThread([&]()
            {
                textBox->Foreground = ref new Microsoft::UI::Xaml::Media::SolidColorBrush(redColor);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto brush = safe_cast<SolidColorBrush^>(textBox->Foreground);
                VERIFY_IS_NOT_NULL(brush);
                VERIFY_IS_TRUE(IsSameColor(brush->Color, redColor));
            });

            LOG_OUTPUT(L"Focus on the TextBox, the focused visual state will set foreground color to black");
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(textBox->Focus(FocusState::Programmatic));
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Focus on the button to remove focus from TextBox.");
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(button->Focus(FocusState::Programmatic));
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Verify same foreground color restored after focus is lost.");
                auto brush = safe_cast<SolidColorBrush^>(textBox->Foreground);
                VERIFY_IS_NOT_NULL(brush);
                VERIFY_IS_TRUE(IsSameColor(brush->Color, redColor));
            });

            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");
            LOG_OUTPUT(L"Change color to green and make a dump to be sure.");
            RunOnUIThread([&]()
            {
                textBox->Foreground = ref new Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Green);
            });
            // Workaround for 29353526: TestServices::WindowHelper::WaitForIdle() ineffective when TextBox::Foreground is changed
            // injecting few ticks for color change to propagate.
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");
        }

        void TextControlsRenderingTests::TextControlSupportAlphaForegroundColor()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::Button^ button = nullptr;
            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name='button' Content = 'Focus' Margin = '20,0,20,0' />"
                    L"  <TextBox x:Name='textBox' Text='A' IsSpellCheckEnabled='False' FontSize='20' Width='200'/>"
                    L"</StackPanel>"));
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
                VERIFY_IS_NOT_NULL(button);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                ::Windows::UI::Color clrRedTransparant = { 0x80, 0xFF, 0x00, 0x00 };
                textBox->Foreground = ref new Microsoft::UI::Xaml::Media::SolidColorBrush(clrRedTransparant);
            });
            // Workaround for 29353526: TestServices::WindowHelper::WaitForIdle() ineffective when TextBox::Foreground is changed
            // injecting few ticks for color change to propagate.
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);
            TestServices::WindowHelper->WaitForIdle();
        }

        void TextControlsRenderingTests::ValidateEmojiRendering()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::Button^ button = nullptr;
            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name='button' Content = 'Focus' Margin = '20,0,20,0' />"
                    L"  <TextBox x:Name='textBox' Text='A&#x1F601;B&#x1F35E;C' IsSpellCheckEnabled='False' PreventKeyboardDisplayOnProgrammaticFocus ='True' FontSize='20' Width='300'/>"
                    L"</StackPanel>"));
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
                VERIFY_IS_NOT_NULL(button);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            LOG_OUTPUT(L"Switching focus between textbox and button and dump to make sure emoji symbols can still be rendered fine after losing focus");
            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(textBox, FocusState::Programmatic);
            TestServices::WindowHelper->WaitForIdle();

            FocusTestHelper::EnsureFocus(button, FocusState::Programmatic);
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);
            TestServices::WindowHelper->WaitForIdle();
        }

        void TextControlsRenderingTests::ValidateTextFormattersLowMemoryRelease()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            //Create an ETW event to track TextFormatter creation
            TraceConsumerSession traceSession_FormatterCreation(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER);

            //Enable tracing event
            TraceConsumer::EnableTracingByEventId(TextFormatterCreatedInfo_value);
            TraceConsumer::EnableTracingByEventId(UnusedTextFormatterDeletedInfo_value);

            //Create a page with several RichTextBlocks
            LOG_OUTPUT(L"Creating a StackPanel with RichTextBlocks");
            xaml_controls::Panel^ panel = nullptr;
            int NumOfTextBlock;
            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <RichTextBlock Height='80' Width='200'> <Paragraph>Hello World</Paragraph></RichTextBlock>"
                    L"  <RichTextBlock Height='80' Width='200'> <Paragraph>Hello World</Paragraph></RichTextBlock>"
                    L"  <RichTextBlock Height='80' Width='200'> <Paragraph>Hello World</Paragraph></RichTextBlock>"
                    L"  <RichTextBlock Height='80' Width='200'> <Paragraph>Hello World</Paragraph></RichTextBlock>"
                    L"  <RichTextBlock Height='80' Width='200'> <Paragraph>Hello World</Paragraph></RichTextBlock>"
                    L"</StackPanel>"));
                panel = safe_cast<xaml_controls::Panel^>(rootPanel);
                NumOfTextBlock = panel->Children->Size;
                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"A panel with %d RichTextBlocks is created", NumOfTextBlock);

            //suspend and disable rendering
            LOG_OUTPUT(L"Triggering Suspend and disabling rendering");
            TestServices::WindowHelper->TriggerSuspend(true, true);
            TestServices::WindowHelper->SetIsRenderEnabled(false);

            //stop the session
            traceSession_FormatterCreation.Stop();

            //verify the counter value
            VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(TextFormatterCreatedInfo_value, NumOfTextBlock));
            VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(UnusedTextFormatterDeletedInfo_value, 0));

            //create ETW event to track TextFormatter release, resume rendering, and delete half of textblocks
            LOG_OUTPUT(L"Resume and delete textBlocks");
            TraceConsumerSession traceSession_FormatterRelease(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER);
            TraceConsumer::EnableTracingByEventId(TextFormatterCreatedInfo_value);
            TraceConsumer::EnableTracingByEventId(UnusedTextFormatterDeletedInfo_value);

            TestServices::WindowHelper->SetIsRenderEnabled(true);
            TestServices::WindowHelper->TriggerResume();

            int NumOfTextBlockToDelete = NumOfTextBlock / 2;
            if (NumOfTextBlockToDelete > 0) {
                RunOnUIThread([&]()
                {
                    for (int i = 0; i < NumOfTextBlockToDelete; i++) {
                        panel->Children->RemoveAt(0);
                    }
                });
                TestServices::WindowHelper->WaitForIdle();
            }
            LOG_OUTPUT(L"Number of TextBlocks remaining: %d", NumOfTextBlock - NumOfTextBlockToDelete);

            //suspend and disable rendering
            LOG_OUTPUT(L"Triggering Suspend and disabling rendering");
            TestServices::WindowHelper->TriggerSuspend(true, true);
            TestServices::WindowHelper->SetIsRenderEnabled(false);

            //stop the session
            traceSession_FormatterRelease.Stop();

            //verify the counter value
            VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(TextFormatterCreatedInfo_value, 0));
            VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(UnusedTextFormatterDeletedInfo_value, 0));

            //create ETW event to track unused TextFormatter deletion
            LOG_OUTPUT(L"Resume tracking again");
            TraceConsumerSession traceSession_LowMemory(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER);
            TraceConsumer::EnableTracingByEventId(TextFormatterCreatedInfo_value);
            TraceConsumer::EnableTracingByEventId(UnusedTextFormatterDeletedInfo_value);

            //trigger low memory condition
            LOG_OUTPUT(L"Triggering low memory condition");
            TestServices::WindowHelper->TriggerLowMemory();
            TestServices::WindowHelper->WaitForIdle();

            //stop the session
            traceSession_LowMemory.Stop();

            //verify the counter value
            VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(UnusedTextFormatterDeletedInfo_value, NumOfTextBlockToDelete));

            //resume rendering
            TestServices::WindowHelper->SetIsRenderEnabled(true);
            TestServices::WindowHelper->TriggerResume();

            //clear all the textBlocks and textFormatters
            LOG_OUTPUT(L"Clear all the remaining textBlocks and textFormatters");
            RunOnUIThread([&]()
            {
                panel->Children->Clear();
                LOG_OUTPUT(L"Number of elements in panel: %d", panel->Children->Size);
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::WindowHelper->TriggerLowMemory();

            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        }
    } }
} } } }
