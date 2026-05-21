// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "HighContrastAdjustmentTests.h"
#include <AutomationClient\AutomationClientManager.h>
#include <AutomationClient\AutomationEventHandler.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include <SafeEventRegistration.h>
#include <UIAutomationCore.h>
#include <WUCRenderingScopeGuard.h>
#include <RuntimeEnabledFeatureOverride.h>

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

        bool HighContrastAdjustmentTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool HighContrastAdjustmentTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool HighContrastAdjustmentTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void HighContrastAdjustmentTests::VerifyTextBlockDCompTree()
        {
            Platform::String^ xamlString =
                L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' Background='Orange' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                L"  <StackPanel x:Name='controlParent'> "
                L"    <TextBlock IsTextSelectionEnabled='True' Text='TextBlock1' Foreground='Brown' /> "
                L"    <TextBlock Text='TextBlock2' Foreground='Brown' Opacity='0.5' CompositeMode='SourceOver' /> "
                L"    <TextBlock Text='TextBlock3' Foreground='Brown' IsTextSelectionEnabled='True' Opacity='0.5' /> "
                L"    <TextBlock Foreground='Brown'>TextBlock4</TextBlock> "
                L"    <TextBlock Foreground='Brown' IsTextSelectionEnabled='True'>TextBlock5</TextBlock> "
                L"    <TextBlock> <Italic>Multi - line</Italic> <Bold>text</Bold> <LineBreak/>given as content</TextBlock> "
                L"    <TextBlock> <Span Foreground='HotPink'>Some</Span> <Span Foreground='Red'>multi - colored</Span> <Span Foreground='Green'>text</Span></TextBlock> "
                L"    <TextBlock CacheMode='BitmapCache' Text='BitmapCache1' /> "
                L"    <TextBlock CacheMode='BitmapCache'>BitmapCache2</TextBlock> "
                L"    <TextBlock CacheMode='BitmapCache' Opacity='0.5' Text='BitmapCache3' CompositeMode='SourceOver' /> "
                L"    <TextBlock CacheMode='BitmapCache' Opacity='0.5' CompositeMode='SourceOver'>BitmapCache4</TextBlock> "
                L"    <TextBlock CacheMode='BitmapCache' Text='BitmapCache5' IsTextSelectionEnabled='True' />"
                L"    <TextBlock CacheMode='BitmapCache' IsTextSelectionEnabled='True'>BitmapCache6</TextBlock> "
                L"    <TextBlock CacheMode='BitmapCache' Opacity='0.5' Text='BitmapCache7' IsTextSelectionEnabled='True' /> "
                L"    <TextBlock CacheMode='BitmapCache' Opacity='0.5' IsTextSelectionEnabled='True'>BitmapCache8</TextBlock> "
                L"    <Button IsEnabled='False'> "
                L"      <TextBlock CacheMode='BitmapCache' Opacity='0.5' IsTextSelectionEnabled='True'>BitmapCache9</TextBlock> "
                L"    </Button> "
                L"  </StackPanel> "
                L"</StackPanel> ";

            ValidateControlDCompTree(xamlString);
        }

        void HighContrastAdjustmentTests::VerifyRichTextBlockDCompTree()
        {
            Platform::String^ xamlString =
                L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' Background='Orange' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                L"  <StackPanel x:Name='controlParent'> "
                L"    <RichTextBlock IsTextSelectionEnabled='True' TextIndent='12'> "
                L"      <Paragraph>RichTextBlock1</Paragraph>"
                L"      <Paragraph TextIndent='24'>First paragraph.</Paragraph> "
                L"      <Paragraph>Second paragraph.</Paragraph> "
                L"    </RichTextBlock> "
                L"    <RichTextBlock IsTextSelectionEnabled='False' TextIndent='12'> "
                L"      <Paragraph>RichTextBlock2</Paragraph>"
                L"      <Paragraph TextIndent='24'>First paragraph.</Paragraph> "
                L"      <Paragraph>Second paragraph.</Paragraph> "
                L"    </RichTextBlock> "
                L"    <RichTextBlock IsTextSelectionEnabled='True' TextIndent='12' Opacity='0.5' CompositeMode='SourceOver'> "
                L"      <Paragraph>RichTextBlock3</Paragraph>"
                L"      <Paragraph TextIndent='24'>First paragraph.</Paragraph> "
                L"      <Paragraph>Second paragraph.</Paragraph> "
                L"    </RichTextBlock> "
                L"    <RichTextBlock IsTextSelectionEnabled='False' TextIndent='12' Opacity='0.5'> "
                L"      <Paragraph>RichTextBlock4</Paragraph>"
                L"      <Paragraph TextIndent='24'>First paragraph.</Paragraph> "
                L"      <Paragraph>Second paragraph.</Paragraph> "
                L"    </RichTextBlock> "
                L"    <RichTextBlock CacheMode='BitmapCache' IsTextSelectionEnabled='True' TextIndent='12' Opacity='0.5'> "
                L"      <Paragraph>RichTextBlock5</Paragraph>"
                L"      <Paragraph TextIndent='24'>First paragraph.</Paragraph> "
                L"      <Paragraph>Second paragraph.</Paragraph> "
                L"    </RichTextBlock> "
                L"    <RichTextBlock CacheMode='BitmapCache' IsTextSelectionEnabled='False' TextIndent='12' Opacity='0.5'> "
                L"      <Paragraph>RichTextBlock6</Paragraph>"
                L"      <Paragraph TextIndent='24'>First paragraph.</Paragraph> "
                L"      <Paragraph>Second paragraph.</Paragraph> "
                L"    </RichTextBlock> "
                L"    <Button IsEnabled='False'> "
                L"      <RichTextBlock CacheMode='BitmapCache' IsTextSelectionEnabled='False' TextIndent='12' Opacity='0.5'> "
                L"        <Paragraph>RichTextBlock7</Paragraph>"
                L"        <Paragraph TextIndent='24'>First paragraph.cache mode</Paragraph> "
                L"      </RichTextBlock> "
                L"    </Button> "
                L"  </StackPanel> "
                L"</StackPanel>";

            ValidateControlDCompTree(xamlString);
        }

        void HighContrastAdjustmentTests::VerifyTextBoxDCompTree()
        {
            Platform::String^ xamlString =
                L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' Background='Orange' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                L"  <StackPanel x:Name='controlParent'> "
                L"    <TextBox PlaceholderText='TextBox1' Background='Red' SelectionHighlightColor='Green' Foreground='Pink' Opacity='0.5' Header='This is the header' /> "
                L"    <TextBox Text='TextBox2' Background='Red' SelectionHighlightColor='Green' Foreground='Pink' Header='This is the header' Opacity='0.1' CompositeMode='SourceOver' /> "
                L"    <TextBox Background='Red' SelectionHighlightColor='Green' Foreground='Pink' Header='This is the header' Text='TextBox3'/> "
                L"  </StackPanel> "
                L"</StackPanel> ";

            ValidateControlDCompTree(xamlString);
        }

        void HighContrastAdjustmentTests::VerifyRichEditBoxDCompTree()
        {
            Platform::String^ xamlString =
                L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' Background='Orange' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                L"  <StackPanel x:Name='controlParent'> "
                L"    <RichEditBox PlaceholderText='RichEditBox1 PlaceholderText' Background='Red' SelectionHighlightColor='Green' Foreground='Pink' Header='RichEditBox1 Header' Opacity='0.5' /> "
                L"    <RichEditBox PlaceholderText='RichEditBox2 PlaceholderText' Background='Red' SelectionHighlightColor='Green' Foreground='Pink' Header='RichEditBox2 Header' /> "
                L"  </StackPanel> "
                L"</StackPanel> ";

            ValidateControlDCompTree(xamlString);
        }

        void HighContrastAdjustmentTests::VerifyPasswordBoxDCompTree()
        {
            Platform::String^ xamlString =
                L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' Background='Orange' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                L"  <StackPanel x:Name='controlParent'> "
                L"    <PasswordBox Header='PasswordBox1 Header' PlaceholderText='PasswordBox1 PlaceholderText' Background='Red' SelectionHighlightColor='Green' Foreground='Pink' /> "
                L"  </StackPanel> "
                L"</StackPanel> ";

            ValidateControlDCompTree(xamlString);
        }

        void HighContrastAdjustmentTests::VerifyHyperlinkDCompTree()
        {
            Platform::String^ xamlString =
                L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' Background='Orange' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                L"  <StackPanel x:Name='controlParent'> "
                L"    <TextBlock Foreground='Pink'>Hyperlink: <Hyperlink NavigateUri='http://www.microsoft.com'>Hyperlink1</Hyperlink></TextBlock> "
                L"  </StackPanel> "
                L"</StackPanel> ";

            ValidateControlDCompTree(xamlString);
        }

        void HighContrastAdjustmentTests::VerifyHyperlinkButtonDCompTree()
        {
            Platform::String^ xamlString =
                L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' Background='Orange' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                L"  <StackPanel x:Name='controlParent'> "
                L"    <HyperlinkButton NavigateUri='http://www.microsoft.com'>HyperlinkButton1</HyperlinkButton> "
                L"    <HyperlinkButton NavigateUri='http://www.microsoft.com'> "
                L"      <TextBlock Text='HyperlinkButton2' /> "
                L"    </HyperlinkButton> "
                L"    <HyperlinkButton NavigateUri='http://www.microsoft.com'> "
                L"      <Grid>"
                L"          <TextBlock Text='HyperlinkButton2' /> "
                L"      </Grid>"
                L"    </HyperlinkButton> "
                L"    <HyperlinkButton Style='{ThemeResource TextBlockButtonStyle}' NavigateUri='http://www.microsoft.com'>HyperlinkButton3</HyperlinkButton> "
                L"    <HyperlinkButton Foreground='Red' NavigateUri='http://www.microsoft.com'>HyperlinkButton4</HyperlinkButton> "
                L"    <HyperlinkButton Foreground='Red' NavigateUri='http://www.microsoft.com' IsEnabled='false'>HyperlinkButton5</HyperlinkButton> "
                L"  </StackPanel> "
                L"</StackPanel> ";

            ValidateControlDCompTree(xamlString);
        }

        void HighContrastAdjustmentTests::ValidateControlDCompTree(Platform::String^ xamlString)
        {
            TestCleanupWrapper cleanup;

            ValidateControlDCompTree(xamlString, ElementHighContrastAdjustment::None, ApplicationHighContrastAdjustment::None, HighContrastTheme::None, "None");
            ValidateControlDCompTree(xamlString, ElementHighContrastAdjustment::None, ApplicationHighContrastAdjustment::None, HighContrastTheme::Black, "None_HC");
            ValidateControlDCompTree(xamlString, ElementHighContrastAdjustment::None, ApplicationHighContrastAdjustment::Auto, HighContrastTheme::None, "None");
            ValidateControlDCompTree(xamlString, ElementHighContrastAdjustment::None, ApplicationHighContrastAdjustment::Auto, HighContrastTheme::Black, "None_HC");

            ValidateControlDCompTree(xamlString, ElementHighContrastAdjustment::Auto, ApplicationHighContrastAdjustment::None, HighContrastTheme::None, "None");
            ValidateControlDCompTree(xamlString, ElementHighContrastAdjustment::Auto, ApplicationHighContrastAdjustment::None, HighContrastTheme::Black, "HC");
            ValidateControlDCompTree(xamlString, ElementHighContrastAdjustment::Auto, ApplicationHighContrastAdjustment::Auto, HighContrastTheme::None, "None");
            ValidateControlDCompTree(xamlString, ElementHighContrastAdjustment::Auto, ApplicationHighContrastAdjustment::Auto, HighContrastTheme::Black, "HC");

            ValidateControlDCompTree(xamlString, ElementHighContrastAdjustment::Application, ApplicationHighContrastAdjustment::None, HighContrastTheme::None, "None");
            ValidateControlDCompTree(xamlString, ElementHighContrastAdjustment::Application, ApplicationHighContrastAdjustment::None, HighContrastTheme::Black, "None_HC");
            ValidateControlDCompTree(xamlString, ElementHighContrastAdjustment::Application, ApplicationHighContrastAdjustment::Auto, HighContrastTheme::None, "None");
            ValidateControlDCompTree(xamlString, ElementHighContrastAdjustment::Application, ApplicationHighContrastAdjustment::Auto, HighContrastTheme::Black, "HC");
        }

        void HighContrastAdjustmentTests::ValidateControlDCompTree(
            Platform::String^ xamlString, ElementHighContrastAdjustment elementHCAdj,
            ApplicationHighContrastAdjustment appHCAdj, HighContrastTheme hcTheme, Platform::String^ variation)
        {
            // Hide the textbox caret so it doesn't interfere with UIElement tree comparison
            RuntimeEnabledFeatureOverride disableTextBoxCaret(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableTextBoxCaret, true);

            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            TestServices::WindowHelper->WaitForIdle();

            xaml_controls::StackPanel^ rootPanel = nullptr;
            xaml_controls::StackPanel^ controlParent = nullptr;

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(xamlString));
                VERIFY_IS_NOT_NULL(rootPanel);

                controlParent = safe_cast<xaml_controls::StackPanel^>(rootPanel->FindName(L"controlParent"));
                VERIFY_IS_NOT_NULL(controlParent);

                TestServices::WindowHelper->WindowContent = rootPanel;

            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                controlParent->HighContrastAdjustment = elementHCAdj;
                Application::Current->HighContrastAdjustment = appHCAdj;
                TestServices::ThemingHelper->HighContrastTheme = hcTheme;
            });

            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Recording DComp tree to make sure HighContrastAdjustment backplate is shown when appropriate.");
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, variation);
        }

        void HighContrastAdjustmentTests::VerifySelectedTextBlockDCompTree()
        {
            TestCleanupWrapper cleanup([]()
            {
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            // Hide the textbox caret so it doesn't interfere with UIElement tree comparison
            RuntimeEnabledFeatureOverride disableTextBoxCaret(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableTextBoxCaret, true);

            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

            TextBlock^ textBlock = nullptr;
            xaml_controls::StackPanel^ rootPanel = nullptr;

            auto textBlockGotFocusEvent = std::make_shared<Event>();
            auto textBlockGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBlock, GotFocus);

            auto textBlockSelectionChangedEvent = std::make_shared<Event>();
            auto textBlockSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBlock, SelectionChanged);

            Platform::String^ xamlString =
                L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' Background='Orange' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                L"  <TextBlock x:Name='textBlock' Margin='25' FontFamily='Consolas' FontSize='25' Height='30' Text='THIS IS SOME TEXT' IsTextSelectionEnabled='True' /> "
                L"</StackPanel> ";

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(xamlString));
                VERIFY_IS_NOT_NULL(rootPanel);

                textBlock = safe_cast<TextBlock^>(rootPanel->FindName(L"textBlock"));
                VERIFY_IS_NOT_NULL(textBlock);

                TestServices::WindowHelper->WindowContent = rootPanel;

                textBlockGotFocusRegistration.Attach(textBlock, [textBlockGotFocusEvent]() { textBlockGotFocusEvent->Set(); });
                textBlockSelectionChangedRegistration.Attach(textBlock, [textBlockSelectionChangedEvent]() { textBlockSelectionChangedEvent->Set(); });
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                rootPanel->HighContrastAdjustment = ElementHighContrastAdjustment::Auto;
                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::Black;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Give the TextBlock focus and select some text.
                textBlock->Focus(xaml::FocusState::Keyboard);

                TextPointer^ startPosition = textBlock->ContentStart->GetPositionAtOffset(5, LogicalDirection::Backward);
                TextPointer^ endPosition = textBlock->ContentStart->GetPositionAtOffset(10, LogicalDirection::Forward);

                textBlock->Select(startPosition, endPosition);
            });

            // Verify that the TextBlock was focused and text was selected.
            textBlockGotFocusEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
            textBlockSelectionChangedEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
            VERIFY_IS_TRUE(textBlockGotFocusEvent->HasFired());
            VERIFY_IS_TRUE(textBlockSelectionChangedEvent->HasFired());
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Recording DComp tree to make sure HighContrastAdjustment backplate is shown when appropriate.");
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, "HC_Black");
        }

        void HighContrastAdjustmentTests::VerifySelectedTextBoxDCompTree()
        {
            TestCleanupWrapper cleanup([]()
            {
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            // Hide the textbox caret so it doesn't interfere with UIElement tree comparison
            RuntimeEnabledFeatureOverride disableTextBoxCaret(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableTextBoxCaret, true);

            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

            TextBox^ textBox = nullptr;
            xaml_controls::StackPanel^ rootPanel = nullptr;

            auto textBoxGotFocusEvent = std::make_shared<Event>();
            auto textBoxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);

            auto textBoxSelectionChangedEvent = std::make_shared<Event>();
            auto textBoxSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, SelectionChanged);

            Platform::String^ xamlString =
                L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' Background='Orange' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                L"  <TextBox x:Name='textBox' Margin='25' FontFamily='Consolas' FontSize='25' Height='30' Text='THIS IS SOME TEXT' /> "
                L"</StackPanel> ";

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(xamlString));
                VERIFY_IS_NOT_NULL(rootPanel);

                textBox = safe_cast<TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);

                TestServices::WindowHelper->WindowContent = rootPanel;

                textBoxGotFocusRegistration.Attach(textBox, [textBoxGotFocusEvent]() { textBoxGotFocusEvent->Set(); });
                textBoxSelectionChangedRegistration.Attach(textBox, [textBoxSelectionChangedEvent]() { textBoxSelectionChangedEvent->Set(); });
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                rootPanel->HighContrastAdjustment = ElementHighContrastAdjustment::Auto;
                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::Black;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Give the TextBox focus and select some text.
                textBox->Focus(xaml::FocusState::Keyboard);
                textBox->Select(5, 10);
            });

            // Verify that the TextBox was focused and text was selected.
            textBoxGotFocusEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
            textBoxSelectionChangedEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
            VERIFY_IS_TRUE(textBoxGotFocusEvent->HasFired());
            VERIFY_IS_TRUE(textBoxSelectionChangedEvent->HasFired());
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Recording DComp tree to make sure HighContrastAdjustment backplate is shown when appropriate.");
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, "HC_Black");
        }

        void HighContrastAdjustmentTests::VerifySelectedRichTextBlockDCompTree()
        {
            TestCleanupWrapper cleanup([]()
            {
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            // Hide the textbox caret so it doesn't interfere with UIElement tree comparison
            RuntimeEnabledFeatureOverride disableTextBoxCaret(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableTextBoxCaret, true);

            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

            RichTextBlock^ richTextBlock = nullptr;
            xaml_controls::StackPanel^ rootPanel = nullptr;

            auto richTextBlockGotFocusEvent = std::make_shared<Event>();
            auto richTextBlockGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::RichTextBlock, GotFocus);

            auto richTextBlockSelectionChangedEvent = std::make_shared<Event>();
            auto richTextBlockSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::RichTextBlock, SelectionChanged);

            Platform::String^ xamlString =
                L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' Background='Orange' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                L"  <RichTextBlock ContextFlyout='{x:Null}' SelectionFlyout='{x:Null}' x:Name='richTextBlock' IsTextSelectionEnabled='True' TextIndent='12'> "
                L"    <Paragraph>RichTextBlock1</Paragraph>"
                L"    <Paragraph TextIndent='24'>First paragraph.</Paragraph> "
                L"    <Paragraph>Second paragraph.</Paragraph> "
                L"    <Paragraph>Third paragraph.</Paragraph> "
                L"  </RichTextBlock> "
                L"</StackPanel> ";

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(xamlString));
                VERIFY_IS_NOT_NULL(rootPanel);

                richTextBlock = safe_cast<RichTextBlock^>(rootPanel->FindName(L"richTextBlock"));
                VERIFY_IS_NOT_NULL(richTextBlock);

                TestServices::WindowHelper->WindowContent = rootPanel;

                richTextBlockGotFocusRegistration.Attach(richTextBlock, [richTextBlockGotFocusEvent]() { richTextBlockGotFocusEvent->Set(); });
                richTextBlockSelectionChangedRegistration.Attach(richTextBlock, [richTextBlockSelectionChangedEvent]() { richTextBlockSelectionChangedEvent->Set(); });
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                rootPanel->HighContrastAdjustment = ElementHighContrastAdjustment::Auto;
                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::Black;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Give the RichTextBlock focus and select some text.
                richTextBlock->Focus(xaml::FocusState::Keyboard);

                TextPointer^ startPosition = richTextBlock->ContentStart->GetPositionAtOffset(5, LogicalDirection::Backward);
                TextPointer^ endPosition = richTextBlock->ContentStart->GetPositionAtOffset(10, LogicalDirection::Forward);

                richTextBlock->Select(startPosition, endPosition);
            });

            // Verify that the RichTextBlock was focused and text was selected.
            richTextBlockGotFocusEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
            richTextBlockSelectionChangedEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
            VERIFY_IS_TRUE(richTextBlockGotFocusEvent->HasFired());
            VERIFY_IS_TRUE(richTextBlockSelectionChangedEvent->HasFired());
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Recording DComp tree to make sure HighContrastAdjustment backplate is shown when appropriate.");
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, "HC_Black");
        }

        void HighContrastAdjustmentTests::VerifyCheckBoxDCompTree()
        {
            // By default every CheckBox with the element CheckGlyph will have the element HighContrastAdjustment set to None to avoid hiding the CheckBox rectangle behind a BackPlate.
            // Setting the HighContrastAdjustment in CheckGlyph for a re-templated CheckBox should prevent the CheckBox OnApplyTemplate logic from replacing the value to None.
            Platform::String^ xamlString =
                L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' Background='Orange' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                L"  <StackPanel x:Name='controlParent'> "
                L"      <CheckBox>CheckBox 1</CheckBox> "
                L"      <CheckBox IsChecked='True'>CheckBox 2</CheckBox> "
                L"      <CheckBox Content = 'CheckBox 3' IsChecked = 'True'>"
                L"          <CheckBox.Template>"
                L"              <ControlTemplate TargetType = 'CheckBox'>"
                L"                  <Grid x:Name = 'RootGrid'"
                L"                      Background = '{TemplateBinding Background}'"
                L"                      BorderBrush = '{TemplateBinding BorderBrush}'"
                L"                      BorderThickness = '{TemplateBinding BorderThickness}'>"
                L"                      <VisualStateManager.VisualStateGroups>"
                L"                          <VisualStateGroup x:Name = 'CombinedStates'>"
                L"                              <VisualState x:Name = 'CheckedNormal'>"
                L"                                  <Storyboard>"
                L"                                      <ObjectAnimationUsingKeyFrames Storyboard.TargetName = 'ContentPresenter' Storyboard.TargetProperty = 'Foreground'>"
                L"                                          <DiscreteObjectKeyFrame KeyTime = '0' Value = '{ThemeResource CheckBoxForegroundChecked}'/>"
                L"                                      </ObjectAnimationUsingKeyFrames>"
                L"                                      <ObjectAnimationUsingKeyFrames Storyboard.TargetName = 'RootGrid' Storyboard.TargetProperty = 'Background'>"
                L"                                          <DiscreteObjectKeyFrame KeyTime = '0' Value = '{ThemeResource CheckBoxBackgroundChecked}'/>"
                L"                                      </ObjectAnimationUsingKeyFrames>"
                L"                                      <ObjectAnimationUsingKeyFrames Storyboard.TargetName = 'RootGrid' Storyboard.TargetProperty = 'BorderBrush'>"
                L"                                          <DiscreteObjectKeyFrame KeyTime = '0' Value = '{ThemeResource CheckBoxBorderBrushChecked}'/>"
                L"                                      </ObjectAnimationUsingKeyFrames>"
                L"                                      <ObjectAnimationUsingKeyFrames Storyboard.TargetName = 'NormalRectangle' Storyboard.TargetProperty = 'Stroke'>"
                L"                                          <DiscreteObjectKeyFrame KeyTime = '0' Value = '{ThemeResource CheckBoxCheckBackgroundStrokeChecked}'/>"
                L"                                      </ObjectAnimationUsingKeyFrames>"
                L"                                      <ObjectAnimationUsingKeyFrames Storyboard.TargetName = 'NormalRectangle' Storyboard.TargetProperty = 'Fill'>"
                L"                                          <DiscreteObjectKeyFrame KeyTime = '0' Value = '{ThemeResource CheckBoxCheckBackgroundFillChecked}'/>"
                L"                                      </ObjectAnimationUsingKeyFrames>"
                L"                                      <ObjectAnimationUsingKeyFrames Storyboard.TargetName = 'CheckGlyph' Storyboard.TargetProperty = 'Foreground'>"
                L"                                          <DiscreteObjectKeyFrame KeyTime = '0' Value = '{ThemeResource CheckBoxCheckGlyphForegroundChecked}'/>"
                L"                                      </ObjectAnimationUsingKeyFrames>"
                L"                                          <DoubleAnimation Storyboard.TargetName = 'NormalRectangle'"
                L"                                              Storyboard.TargetProperty = 'StrokeThickness'"
                L"                                              To = '{ThemeResource CheckBoxCheckedStrokeThickness}'"
                L"                                              Duration = '0'/>"
                L"                                          <DoubleAnimation Storyboard.TargetName = 'CheckGlyph'"
                L"                                              Storyboard.TargetProperty = 'Opacity'"
                L"                                              To = '1'"
                L"                                              Duration = '0' />"
                L"                                  </Storyboard>"
                L"                              </VisualState>"
                L"                          </VisualStateGroup>"
                L"                      </VisualStateManager.VisualStateGroups>"
                L"                      <Grid.ColumnDefinitions>"
                L"                          <ColumnDefinition Width = '20' />"
                L"                          <ColumnDefinition Width = '*' />"
                L"                      </Grid.ColumnDefinitions>"
                L"                      <Grid VerticalAlignment = 'Top' Height = '32'>"
                L"                          <Rectangle x:Name = 'NormalRectangle'"
                L"                              Fill = '{ThemeResource CheckBoxCheckBackgroundFillUnchecked}'"
                L"                              Stroke = '{ThemeResource CheckBoxCheckBackgroundStrokeUnchecked}'"
                L"                              StrokeThickness = '{ThemeResource CheckBoxBorderThemeThickness}'"
                L"                              UseLayoutRounding = 'False'"
                L"                              Height = '20'"
                L"                              Width = '20' />"
                L"                          <FontIcon x:Name = 'CheckGlyph'"
                L"                              FontFamily = '{ThemeResource SymbolThemeFontFamily}'"
                L"                              Glyph = '&#xE001;'"
                L"                              FontSize = '20'"
                L"                              Foreground = '{ThemeResource CheckBoxCheckGlyphForegroundUnchecked}'"
                L"                              Opacity = '0'"
                L"                              HighContrastAdjustment = 'Auto' />"
                L"                      </Grid>"
                L"                      <ContentPresenter x:Name = 'ContentPresenter'"
                L"                          ContentTemplate = '{TemplateBinding ContentTemplate}'"
                L"                          ContentTransitions = '{TemplateBinding ContentTransitions}'"
                L"                          Content = '{TemplateBinding Content}'"
                L"                          Margin = '{TemplateBinding Padding}'"
                L"                          HorizontalAlignment = '{TemplateBinding HorizontalContentAlignment}'"
                L"                          VerticalAlignment = '{TemplateBinding VerticalContentAlignment}'"
                L"                          Grid.Column = '1'"
                L"                          AutomationProperties.AccessibilityView = 'Raw'"
                L"                          TextWrapping = 'Wrap' />"
                L"                  </Grid>"
                L"              </ControlTemplate>"
                L"          </CheckBox.Template>"
                L"      </CheckBox>"
                L"  </StackPanel> "
                L"</StackPanel> ";

            ValidateControlDCompTree(xamlString);
        }

        void HighContrastAdjustmentTests::VerifyBackPlateUpdatesOnTextAlignment()
        {
            TestCleanupWrapper cleanup([]()
            {
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

            xaml_controls::StackPanel^ rootPanel = nullptr;

            Platform::String^ xamlString =
                L"<StackPanel Width='200' Height='400' VerticalAlignment='Top' Background='Orange' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                L"  <TextBlock Text = 'Left aligned' TextAlignment = 'Left'/>"
                L"  <TextBlock Text = 'Right aligned' TextAlignment = 'Right'/>"
                L"  <TextBlock Text = 'Center aligned' TextAlignment = 'Center'/>"
                L"  <TextBlock Text = 'Justified' TextAlignment = 'Justify'/>"
                L"</StackPanel> ";

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(xamlString));
                VERIFY_IS_NOT_NULL(rootPanel);

                TestServices::WindowHelper->WindowContent = rootPanel;

                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::Black;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                rootPanel->Width = 400;
            });

            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Recording DComp tree to make sure HighContrastAdjustment backplate updates appropriately.");
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, "HC_Black");
        }

        void HighContrastAdjustmentTests::VerifyHighContrastAdjustmentOpacityOverride()
        {
            TestCleanupWrapper cleanup([]()
            {
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

            xaml_controls::StackPanel^ rootPanel = nullptr;

            Platform::String^ xamlString =
                L"<StackPanel Width = '400' Height = '500' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'> "
                L"  <TextBlock>1 Opaque</TextBlock>"
                L"  <Rectangle Width = '30' Height = '30' Fill = 'Red'></Rectangle>"
                L"  <Rectangle Width = '30' Height = '30' Fill = 'Red' Opacity = '0.5'></Rectangle>"
                L"  <Grid Opacity = '0.5'>"
                L"      <Rectangle Width = '30' Height = '30' Fill = 'Red' Opacity = '0.5'></Rectangle>"
                L"  </Grid>"
                L"  <Grid CompositeMode = 'SourceOver' Opacity = '0.5'>"
                L"      <Rectangle Width = '30' Height = '30' Fill = 'Red' Opacity = '0.5'></Rectangle>"
                L"  </Grid>"
                L"  <Grid Opacity = '0.5'>"
                L"      <Grid Opacity = '0.5'>"
                L"          <Rectangle Width = '30' Height = '30' Fill = 'Red' Opacity = '0.5'></Rectangle>"
                L"      </Grid>"
                L"  </Grid>"
                L"  <TextBlock>0.5 Opacity</TextBlock>"
                L"  <Rectangle HighContrastAdjustment = 'None' Width = '30' Height = '30' Fill = 'Red' Opacity = '0.5'></Rectangle>"
                L"  <Grid Opacity = '0.5'>"
                L"      <Rectangle HighContrastAdjustment = 'None' Width = '30' Height = '30' Fill = 'Red' Opacity = '0.5'></Rectangle>"
                L"  </Grid>"
                L"  <Grid Opacity = '0.5'>"
                L"      <Grid Opacity = '0.5'>"
                L"          <Rectangle HighContrastAdjustment = 'None' Width = '30' Height = '30' Fill = 'Red' Opacity = '0.5'></Rectangle>"
                L"      </Grid>"
                L"  </Grid>"
                L"  <Grid HighContrastAdjustment = 'None' Opacity = '0.5'>"
                L"      <Grid HighContrastAdjustment = 'Auto' Opacity = '0.5'>"
                L"          <Rectangle HighContrastAdjustment = 'None' Width = '30' Height = '30' Fill = 'Red' Opacity = '0.5'></Rectangle>"
                L"      </Grid>"
                L"  </Grid>"
                L"  <Grid Opacity = '0.5'>"
                L"      <Rectangle HighContrastAdjustment = 'None' CompositeMode = 'SourceOver' Width = '30' Height = '30' Fill = 'Red' Opacity = '0.5'></Rectangle>"
                L"  </Grid>"
                L"  <TextBlock>0.25 Opacity</TextBlock>"
                L"  <Rectangle HighContrastAdjustment = 'None' Width = '30' Height = '30' Fill = 'Red' Opacity = '0.25'></Rectangle>"
                L"  <Grid HighContrastAdjustment = 'None' Opacity = '0.5'>"
                L"      <Rectangle Width = '30' Height = '30' Fill = 'Red' Opacity = '0.5'></Rectangle>"
                L"  </Grid>"
                L"  <Grid Opacity = '0.5'>"
                L"      <Grid HighContrastAdjustment = 'None' Opacity = '0.5'>"
                L"          <Rectangle Width = '30' Height = '30' Fill = 'Red' Opacity = '0.5'></Rectangle>"
                L"      </Grid>"
                L"  </Grid>"
                L"  <Grid HighContrastAdjustment = 'None' Opacity = '0.5'>"
                L"      <Grid HighContrastAdjustment = 'Auto' Opacity = '0.5'>"
                L"          <Grid HighContrastAdjustment = 'None' Opacity = '0.5'>"
                L"              <Rectangle Width = '30' Height = '30' Fill = 'Red' Opacity = '0.5'></Rectangle>"
                L"          </Grid>"
                L"      </Grid>"
                L"  </Grid>"
                L"  <Grid HighContrastAdjustment = 'None' CompositeMode = 'SourceOver' Opacity = '0.5'>"
                L"      <Grid CompositeMode = 'SourceOver' Opacity = '0.5'>"
                L"          <Rectangle HighContrastAdjustment = 'Auto' Width = '30' Height = '30' Fill = 'Red' Opacity = '0.5'></Rectangle>"
                L"      </Grid>"
                L"  </Grid>"
                L"</StackPanel>";

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(xamlString));
                VERIFY_IS_NOT_NULL(rootPanel);

                TestServices::WindowHelper->WindowContent = rootPanel;

                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::Black;
            });

            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Recording DComp tree to make sure opacity updates appropriately.");
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, "Black");
        }

        void HighContrastAdjustmentTests::VerifyHCAFontOverrideUpdatesWhenControlEnabledChanges()
        {
            TestCleanupWrapper cleanup([]()
            {
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

            xaml_controls::StackPanel^ rootPanel = nullptr;
            xaml_controls::ContentControl^ contentControl = nullptr;

            Platform::String^ xamlString =
                    L"<StackPanel Orientation='Horizontal' HorizontalAlignment='Center' VerticalAlignment='Center' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <ContentControl x:Name='ContentControl1'>"
                    L"        <StackPanel>"
                    L"            <FontIcon Glyph='&#xE11D;' Foreground='Red' />"
                    L"            <TextBlock Foreground='Red'>A TextBlock</TextBlock>"
                    L"            <RichTextBlock Foreground='Red'>"
                    L"                <Paragraph>"
                    L"                    A rich"
                    L"                </Paragraph>"
                    L"                <Paragraph>"
                    L"                    Textblock"
                    L"                </Paragraph>"
                    L"            </RichTextBlock>"
                    L"            <HyperlinkButton Foreground='Red'>HyperlinkButton</HyperlinkButton>"
                    L"            <TextBlock><Hyperlink Foreground='Red'>Hyperlink</Hyperlink></TextBlock>"
                    L"            <TextBox PlaceholderText='Placeholder' Foreground='Red'></TextBox>"
                    L"            <TextBox Text='TextBox' Foreground='Red'></TextBox>"
                    L"        </StackPanel>"
                    L"    </ContentControl>"
                    L"</StackPanel>";

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(xamlString));
                VERIFY_IS_NOT_NULL(rootPanel);

                TestServices::WindowHelper->WindowContent = rootPanel;

                contentControl = safe_cast<xaml_controls::ContentControl^>(rootPanel->FindName(L"ContentControl1"));

                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::Black;
            });

            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Recording DComp tree to make sure overridden font color is correct.");
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, "ControlEnabled");

            RunOnUIThread([&]()
            {
                contentControl->IsEnabled = false;
            });

            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Recording DComp tree to make sure overridden font color is correct after Control is disabled.");
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, "ControlDisabled");
        }

    } }
} } } }
