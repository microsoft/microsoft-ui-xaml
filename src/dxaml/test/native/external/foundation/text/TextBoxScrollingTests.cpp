// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TextBoxScrollingTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include "TestCleanupWrapper.h"
#include "SafeEventRegistration.h"
#include <TreeHelper.h>
#include <WUCRenderingScopeGuard.h>
#include <FocusTestHelper.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Text {

        bool TextBoxScrollingTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool TextBoxScrollingTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool TextBoxScrollingTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void TextBoxScrollingTests::ScrollWithoutWrappingText()
        {
            // We expect the scroll to go to the parent ScrollViewer, thus scrolling the entire stackpanel.
            TextBoxScrollingTestsHelper(false);
        }

        void TextBoxScrollingTests::ScrollWithWrappingText()
        {
            // We expect the scroll to go to the textbox, thus only scrolling that individual box.
            TextBoxScrollingTestsHelper(true);
        }

        void TextBoxScrollingTests::TextBoxVisualStatePanInsideScrollViewer()
        {
            TextBoxScrollingTestsHelper(false, true);
        }

        void TextBoxScrollingTests::TextBoxScrollingTestsHelper(bool doesTextWrap, bool testVisualStatePanFromTextBox)
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(500, 500));

            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);
            auto loadedEvent = std::make_shared<Event>();
            auto renderedEvent = std::make_shared<Event>();

            xaml_controls::ScrollViewer ^scrollViewer;
            xaml_controls::TextBox ^panBox;

            RunOnUIThread([&] ()
            {
                xaml_controls::Grid ^grid = ref new xaml_controls::Grid();

                scrollViewer = ref new xaml_controls::ScrollViewer();
                scrollViewer->Width = 400;
                scrollViewer->Height = 400;

                xaml_controls::StackPanel ^stackPanel = ref new xaml_controls::StackPanel();

                for (int i = 0; i < 10; i++)
                {
                    xaml_controls::TextBox ^textBox = ref new xaml_controls::TextBox();
                    VERIFY_IS_NOT_NULL(textBox);
                    textBox->Height = 60;
                    textBox->Text = L"Here is some prepopulated text.";

                    // The third box will be the one we attempt to scroll.
                    if (i == 2)
                    {
                        panBox = textBox;
                        if (doesTextWrap)
                        {
                            panBox->TextWrapping = Microsoft::UI::Xaml::TextWrapping::Wrap;
                            panBox->Text = L"Here is some prepopulated text that is very long and I think it should wrap because it is very long. This text is going to be very very long just to ensure that it eventually wraps, because sometimes it is hard to tell how much is too much.";
                        }
                    }

                    stackPanel->Children->Append(textBox);
                }

                scrollViewer->Content = stackPanel;

                grid->Children->Append(scrollViewer);

                TestServices::WindowHelper->WindowContent = grid;

                loadedRegistration.Attach(grid, ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    loadedEvent->Set();
                }));
            });
            loadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            if (!testVisualStatePanFromTextBox)
            {
                // Give the textbox focus so we can test scrolling (either within or chained to the parent)
                RunOnUIThread([&]()
                {
                    panBox->Focus(xaml::FocusState::Keyboard);
                });
                TestServices::WindowHelper->WaitForIdle();
            }

            LOG_OUTPUT(L"Attempting to pan the TextBox");
            TestServices::InputHelper->PanFromCenter(panBox, 50 /*relX*/, -50 /*relY*/, 1.0 /*velocityFactor*/);
            TestServices::WindowHelper->WaitForIdle();

            if (!testVisualStatePanFromTextBox)
            {
                RunOnUIThread([&]()
                {
                    if (doesTextWrap)
                    {
                        LOG_OUTPUT(L"We expect the textbox scrollviewer to scroll.");
                        LOG_OUTPUT(L"Current vertical offset: %f", scrollViewer->VerticalOffset);
                        VERIFY_IS_TRUE(scrollViewer->VerticalOffset == 0.0);
                    }
                    else
                    {
                        LOG_OUTPUT(L"We expect the parent scrollViewer to scroll.");
                        LOG_OUTPUT(L"Current vertical offset: %f", scrollViewer->VerticalOffset);
                        VERIFY_IS_TRUE(scrollViewer->VerticalOffset != 0.0);
                    }
                });
            }
            else
            {
                bool normalStateFound = false;
                RunOnUIThread([&]()
                {
                    auto templateRoot = safe_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(panBox, 0));
                    auto groups = xaml::VisualStateManager::GetVisualStateGroups(templateRoot);
                    VERIFY_IS_TRUE(groups->Size > 0);

                    // enumerate current state of all VSG and check if contains "Disable"
                    for (unsigned int i = 0; i < groups->Size; ++i)
                    {
                        auto currentVisualStateGroup = groups->GetAt(i);
                        LOG_OUTPUT(L"VSG %s current state:%s", currentVisualStateGroup->Name->Data(), currentVisualStateGroup->CurrentState->Name->Data());
                        if (currentVisualStateGroup->CurrentState->Name == L"Normal")
                        {
                            normalStateFound = true;
                            break;
                        }
                    }

                    VERIFY_IS_TRUE(normalStateFound);
                });
            }

            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBoxScrollingTests::ScrollWithMouse()
        {
            TestCleanupWrapper cleanup;

            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(500, 500));

            xaml_controls::TextBox ^textBox;
            xaml_controls::ScrollViewer^ textBoxScrollViewer;

            auto textBoxGotFocusEvent = std::make_shared<Event>();
            auto textBoxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);
            auto scrollViewerViewChangedEvent = std::make_shared<Event>();
            auto scrollViewerViewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <TextBox x:Name='textBox'  IsSpellCheckEnabled='false' AcceptsReturn='true' FontSize='20' Width='200' Height='26' Margin ='20,5,20,0'>"
                    L"  </TextBox>"
                    L"</StackPanel>"));
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                TestServices::WindowHelper->WindowContent = rootPanel;

                textBoxGotFocusRegistration.Attach(
                    textBox,
                    ref new xaml::RoutedEventHandler(
                        [textBoxGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"TextBox control GotFocus handler.");
                    textBoxGotFocusEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                textBox->Focus(xaml::FocusState::Pointer);
                textBoxScrollViewer = TreeHelper::GetVisualChildByType<xaml_controls::ScrollViewer>(textBox);
                VERIFY_IS_NOT_NULL(textBoxScrollViewer);
                scrollViewerViewChangedRegistration.Attach(textBoxScrollViewer,
                    ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                        [&](Platform::Object^ sender, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
                {
                    if (args->IsIntermediate == false)
                    {
                        scrollViewerViewChangedEvent->Set();
                    }
                }));
            });

            TestServices::WindowHelper->WaitForIdle();
            textBoxGotFocusEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                textBox->Text += "Line 1\r\n";
                textBox->Text += "Line 2\r\n";
                textBox->Text += "Line 3";
                textBox->SelectionStart = 0;
            });
            TestServices::WindowHelper->WaitForIdle();

            // mouse wheel scrolling down
            LOG_OUTPUT(L"Scroll down text using mouse...");
            TestServices::InputHelper->ScrollMouseWheel(textBox, -3);
            LOG_OUTPUT(L"Wait for ScrollViewerViewChanged Event");
            scrollViewerViewChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"TextBox scrollviewer vertical offset after scroll down: %f", textBoxScrollViewer->VerticalOffset);
                VERIFY_IS_TRUE(textBoxScrollViewer->VerticalOffset > 0.0);
            });

            double bottomOffset = 0.0;
            RunOnUIThread([&]()
            {
                textBox->SelectionStart = textBox->Text->Length() -1;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"TextBox scrollviewer vertical offset at bottom: %f", textBoxScrollViewer->VerticalOffset);
                bottomOffset = textBoxScrollViewer->VerticalOffset;
            });
            TestServices::WindowHelper->WaitForIdle();

            scrollViewerViewChangedEvent->Reset();
            // mouse wheel scrolling up
            LOG_OUTPUT(L"Scroll up text using mouse...");
            TestServices::InputHelper->ScrollMouseWheel(textBox, 5);
            LOG_OUTPUT(L"Wait for ScrollViewerViewChanged Event");
            scrollViewerViewChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"TextBox scrollviewer vertical offset after scroll up: %f", textBoxScrollViewer->VerticalOffset);
                VERIFY_IS_TRUE(textBoxScrollViewer->VerticalOffset < bottomOffset);
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBoxScrollingTests::ScrollWithKeyboard()
        {
            TestCleanupWrapper cleanup;

            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(500, 500));

            xaml_controls::TextBox ^textBox;
            xaml_controls::ScrollViewer^ textBoxScrollViewer;

            auto textBoxGotFocusEvent = std::make_shared<Event>();
            auto textBoxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name='button' Content='Focus' Margin='20,40,20,0' />"
                    L"  <TextBox x:Name='textBox' IsSpellCheckEnabled='false' AcceptsReturn='true' FontSize='20' Width='200' Height='26' Margin='20,5,20,0' Padding='10,3,6,5'>"
                    L"  </TextBox>"
                    L"</StackPanel>"));
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                TestServices::WindowHelper->WindowContent = rootPanel;

                textBoxGotFocusRegistration.Attach(
                    textBox,
                    ref new xaml::RoutedEventHandler(
                        [textBoxGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"TextBox control GotFocus handler.");
                    textBoxGotFocusEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                textBox->Focus(xaml::FocusState::Pointer);
            });

            TestServices::WindowHelper->WaitForIdle();
            textBoxGotFocusEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                textBox->Text += "Line 1\r\n";
                textBox->Text += "Line 2\r\n";
                textBox->Text += "Line 3";
                textBox->SelectionStart = 0;
            });

            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                textBoxScrollViewer = TreeHelper::GetVisualChildByType<xaml_controls::ScrollViewer>(textBox);
                VERIFY_IS_NOT_NULL(textBoxScrollViewer);
            });

            TestServices::KeyboardHelper->Down();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->Down();
            TestServices::WindowHelper->WaitForIdle();

            const int line3VerticalOffset = 53;
            const int line3FirstCharCP = 14;
            const int line2VerticalOffset = 27;
            const int line2FirstCharCP = 7;
            const int line2LastCharCP = 13;
            const int line3LastCharCP = 20;

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"CP after down key press twice: %d", textBox->SelectionStart);
                LOG_OUTPUT(L"TextBox scrollviewer vertical offset: %f", textBoxScrollViewer->VerticalOffset);
                VERIFY_ARE_EQUAL(line3VerticalOffset, (int)textBoxScrollViewer->VerticalOffset);
                VERIFY_ARE_EQUAL(line3FirstCharCP, textBox->SelectionStart);
            });

            TestServices::KeyboardHelper->Up();
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"CP after up key press: %d", textBox->SelectionStart);
                LOG_OUTPUT(L"TextBox scrollviewer vertical offset: %f", textBoxScrollViewer->VerticalOffset);
                VERIFY_ARE_EQUAL(line2VerticalOffset, (int)textBoxScrollViewer->VerticalOffset);
                VERIFY_ARE_EQUAL(line2FirstCharCP, textBox->SelectionStart);
            });

            TestServices::KeyboardHelper->Right();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->Right();
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"CP after right key press twice: %d", textBox->SelectionStart);
                LOG_OUTPUT(L"TextBox scrollviewer vertical offset: %f", textBoxScrollViewer->VerticalOffset);
                VERIFY_ARE_EQUAL(line2VerticalOffset, (int)textBoxScrollViewer->VerticalOffset);
                VERIFY_ARE_EQUAL(line2FirstCharCP+2, textBox->SelectionStart);
            });

            TestServices::KeyboardHelper->Left();
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"CP after left key press: %d", textBox->SelectionStart);
                LOG_OUTPUT(L"TextBox scrollviewer vertical offset: %f", textBoxScrollViewer->VerticalOffset);
                VERIFY_ARE_EQUAL(line2VerticalOffset, (int)textBoxScrollViewer->VerticalOffset);
                VERIFY_ARE_EQUAL(line2FirstCharCP+1, textBox->SelectionStart);
            });

            TestServices::KeyboardHelper->Home();
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"CP after home key press: %d", textBox->SelectionStart);
                LOG_OUTPUT(L"TextBox scrollviewer vertical offset: %f", textBoxScrollViewer->VerticalOffset);
                VERIFY_ARE_EQUAL(line2VerticalOffset, (int)textBoxScrollViewer->VerticalOffset);
                VERIFY_ARE_EQUAL(line2FirstCharCP, textBox->SelectionStart);
            });

            TestServices::KeyboardHelper->End();
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"CP after end key press: %d", textBox->SelectionStart);
                LOG_OUTPUT(L"TextBox scrollviewer vertical offset: %f", textBoxScrollViewer->VerticalOffset);
                VERIFY_ARE_EQUAL(line2VerticalOffset, (int)textBoxScrollViewer->VerticalOffset);
                VERIFY_ARE_EQUAL(line2LastCharCP, textBox->SelectionStart);
            });

            TestServices::KeyboardHelper->PageDown();
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"CP after page down key press: %d", textBox->SelectionStart);
                LOG_OUTPUT(L"TextBox scrollviewer vertical offset: %f", textBoxScrollViewer->VerticalOffset);
                VERIFY_ARE_EQUAL(line3VerticalOffset, (int)textBoxScrollViewer->VerticalOffset);
                VERIFY_ARE_EQUAL(line3LastCharCP, textBox->SelectionStart);
            });

            TestServices::KeyboardHelper->PageUp();
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"CP after page up key press: %d", textBox->SelectionStart);
                LOG_OUTPUT(L"TextBox scrollviewer vertical offset: %f", textBoxScrollViewer->VerticalOffset);
                VERIFY_ARE_EQUAL(line2VerticalOffset, (int)textBoxScrollViewer->VerticalOffset);
                VERIFY_ARE_EQUAL(line2LastCharCP, textBox->SelectionStart);
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBoxScrollingTests::KeyboardScrollingInScrollViewerKeepsCaretInView()
        {
            TestCleanupWrapper cleanup;

            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(500, 500));

            xaml_controls::TextBox ^textBox;
            xaml_controls::ScrollViewer^ scrollViewer;

            const float textBoxTopPosition = 5.0;
            const float textBoxBottomPosition = 150.0;
            auto scrollViewerViewChangedEvent = std::make_shared<Event>();
            auto scrollViewerViewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"<ScrollViewer x:Name='scrollViewer' Height='50'>"
                    L"   <StackPanel>"
                    L"    <TextBox x:Name='textBox'  AcceptsReturn='true' FontSize='20' Width='400' Height='200'>"
                    L"    </TextBox>"
                    L"    </StackPanel>"
                    L"</ScrollViewer>"
                    L"</StackPanel>"));
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(rootPanel->FindName(L"scrollViewer"));
                VERIFY_IS_NOT_NULL(textBox);


                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            FocusTestHelper::EnsureFocus(textBox, FocusState::Keyboard);

            RunOnUIThread([&]()
            {
                textBox->Text += "Line 1\r\n";
                textBox->Text += "Line 2\r\n";
                textBox->Text += "Line 3\r\n";
                textBox->Text += "Line 4\r\n";
                textBox->Text += "Line 5\r\n";
                textBox->Text += "Line 6\r\n";
                textBox->Text += "Line 7";
                textBox->SelectionStart = 0;

                scrollViewerViewChangedRegistration.Attach(scrollViewer,
                    ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                        [&](Platform::Object^ sender, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
                {
                    if (args->IsIntermediate == false)
                    {
                        scrollViewerViewChangedEvent->Set();
                    }
                }));

            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Scroll down text using keyboard...");
            TestServices::KeyboardHelper->PageDown();
            LOG_OUTPUT(L"Wait for ScrollViewerViewChanged Event");
            scrollViewerViewChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"TextBox scrollviewer vertical offset after scroll down: %f", scrollViewer->VerticalOffset);
                VERIFY_ARE_EQUAL(42, textBox->SelectionStart);
                VERIFY_ARE_EQUAL(textBoxBottomPosition, scrollViewer->VerticalOffset);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                scrollViewerViewChangedEvent->Reset();
            });

            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Scroll up text using keyboard...");
            TestServices::KeyboardHelper->PageUp();
            LOG_OUTPUT(L"Wait for ScrollViewerViewChanged Event");
            scrollViewerViewChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"TextBox scrollviewer vertical offset after scroll up: %f", scrollViewer->VerticalOffset);
                VERIFY_ARE_EQUAL(0, textBox->SelectionStart);
                VERIFY_IS_TRUE(textBoxTopPosition > scrollViewer->VerticalOffset);
            });

        }

    } }
} } } }
