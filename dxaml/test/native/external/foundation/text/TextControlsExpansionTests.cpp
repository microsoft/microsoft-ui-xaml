// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TextControlsExpansionTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <ppltasks.h>
#include "FileLoader.h"
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include "KeyboardInjectionOverride.h"

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Text {

        bool TextControlsExpansionTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool TextControlsExpansionTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool TextControlsExpansionTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        Platform::String^ TextControlsExpansionTests::GetPathToFiles() const
        {
            // Get the deployment directory, and then append our test's directory to the end
            auto deploymentDir = GetTestDeploymentDir();
            return ref new Platform::String(deploymentDir + L"resources\\native\\foundation\\text\\");
        }

        void TextControlsExpansionTests::AnimateTextControlHeight(
            _In_ xaml::FrameworkElement^ root,
            _In_ bool addNewLine)
        {
            KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride(KeyboardWaitKind::WaitForIdleBeforeAndAfter);

            xaml::FrameworkElement^ fe = nullptr;
            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::RichEditBox^ richEditBox = nullptr;
            xaml_controls::Button^ button = nullptr;

            int sizeChangedCount = 0;

            auto rootLoadedEvent = std::make_shared<Event>();
            auto btnGotFocusEvent = std::make_shared<Event>();
            auto feGotFocusEvent = std::make_shared<Event>();
            auto selectionChangedEvent = std::make_shared<Event>();
            auto sizeChangedEvent = std::make_shared<Event>();
            auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);
            auto feGotFocusRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, GotFocus);
            auto btnGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
            auto sizeChangedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, SizeChanged);
            auto textBoxSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, SelectionChanged);
            auto richEditBoxSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, SelectionChanged);

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Listening to root FrameworkElement.Loaded.");
                rootLoadedRegistration.Attach(
                    root,
                    ref new xaml::RoutedEventHandler(
                    [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Root FE Loaded handler.");
                    rootLoadedEvent->Set();
                }));

                fe = safe_cast<xaml::FrameworkElement^>(root->FindName("textControl"));
                VERIFY_IS_NOT_NULL(fe);

                textBox = dynamic_cast<xaml_controls::TextBox^>(fe);
                if (textBox == nullptr)
                {
                    richEditBox = dynamic_cast<xaml_controls::RichEditBox^>(fe);
                    VERIFY_IS_NOT_NULL(richEditBox);
                    richEditBox->Header = "Header";
                    richEditBox->Document->SetText(Microsoft::UI::Text::TextSetOptions::None, L"One Two Three");
                }
                else
                {
                    textBox->Header = "Header";
                }

                LOG_OUTPUT(L"Listening to FrameworkElement.GotFocus.");
                feGotFocusRegistration.Attach(
                    fe,
                    ref new xaml::RoutedEventHandler(
                    [feGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Text Control GotFocus handler.");
                    feGotFocusEvent->Set();
                }));
                TestServices::WindowHelper->WindowContent = root;
            });

            LOG_OUTPUT(L"Waiting for root FE Loaded event.");
            rootLoadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            if (addNewLine)
            {
                LOG_OUTPUT(L"Setting focus to the Text Control by tapping.");
                TestServices::InputHelper->Tap(fe);
            }
            else
            {
                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Setting focus to the Text Control programmatically.");
                    xaml_controls::Control^ textControl = dynamic_cast<xaml_controls::Control^>(fe);
                    textControl->Focus(xaml::FocusState::Programmatic);
                });
            }

            LOG_OUTPUT(L"Waiting for GotFocus event.");
            feGotFocusEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            // move cusor to the front of the text
            RunOnUIThread([&]()
            {
                if (textBox == nullptr)
                {
                    LOG_OUTPUT(L"Moving cursor to the front of the RichEditBox.");
                    richEditBox->Document->Selection->StartPosition = 0;
                    richEditBox->Document->Selection->EndPosition = 0;
                }
                else
                {
                    LOG_OUTPUT(L"Moving cursor to the front of the TextBox.");
                    textBox->Select(0, 0);
                }
                LOG_OUTPUT(L"Done.");
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Listening to FrameworkElement.SizeChanged.");
                sizeChangedRegistration.Attach(
                    fe,
                    ref new xaml::SizeChangedEventHandler(
                    [sizeChangedEvent, &sizeChangedCount, fe](Platform::Object^, xaml::ISizeChangedEventArgs^)
                {
                    sizeChangedCount++;
                    // During the height animation, Text Control Height is not expected to be NaN
                    VERIFY_ARE_EQUAL(_isnan(fe->Height), 0);
                    LOG_OUTPUT(L"Text Control Height in FrameworkElement.SizeChanged: %f.", fe->Height);
                    if (sizeChangedCount >= 2)
                    {
                        sizeChangedEvent->Set();
                    }
                }));

                if (addNewLine)
                {
                    if (richEditBox != nullptr)
                    {
                        // The Enter keystroke is expected to raise the RichEditBox.SelectionChanged event.
                        LOG_OUTPUT(L"Listening to RichEditBox.SelectionChanged.");
                        richEditBoxSelectionChangedRegistration.Attach(
                            richEditBox,
                            ref new xaml::RoutedEventHandler(
                            [selectionChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                        {
                            LOG_OUTPUT(L"RichEditBox.SelectionChanged handler.");
                            selectionChangedEvent->Set();
                        }));
                    }
                    else
                    {
                        // The Enter keystroke is expected to raise the TextBox.SelectionChanged event.
                        LOG_OUTPUT(L"Listening to TextBox.SelectionChanged.");
                        textBoxSelectionChangedRegistration.Attach(
                            textBox,
                            ref new xaml::RoutedEventHandler(
                            [selectionChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                        {
                            LOG_OUTPUT(L"TextBox.SelectionChanged handler.");
                            selectionChangedEvent->Set();
                        }));
                    }
                }

                button = safe_cast<xaml_controls::Button^>(root->FindName("button"));
                VERIFY_IS_NOT_NULL(button);

                LOG_OUTPUT(L"Listening to Button.GotFocus.");
                btnGotFocusRegistration.Attach(
                    button,
                    ref new xaml::RoutedEventHandler(
                    [btnGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Button.GotFocus handler.");
                    btnGotFocusEvent->Set();
                }));
            });

            if (addNewLine)
            {
                // Add a new line to the text control
                LOG_OUTPUT(L"Injecting Enter keystroke.");
                selectionChangedEvent->Reset();
                TestServices::KeyboardHelper->Enter();
                TestServices::WindowHelper->WaitForIdle();
                LOG_OUTPUT(L"Waiting for SelectionChanged event.");
                selectionChangedEvent->WaitForDefault();
            }
            else
            {
                RunOnUIThread([&]()
                {
                    if (richEditBox != nullptr)
                    {
                        LOG_OUTPUT(L"Changing RichEditBox's text to expand control.");
                        richEditBox->Document->SetText(Microsoft::UI::Text::TextSetOptions::None, L"One Two Three Four Five");
                    }
                    else
                    {
                        LOG_OUTPUT(L"Changing TextBox's text to expand control.");
                        textBox->Text = L"One Two Three Four Five";
                    }
                });
            }

            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Waiting for SizeChanged event.");
            sizeChangedEvent->WaitForDefault();

            // Expect to receive multiple SizeChanged events.
            LOG_OUTPUT(L"Text Control SizeChanged occurrences: %d.", sizeChangedCount);
            VERIFY_IS_TRUE(sizeChangedCount >= 2);

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Focus the Button to discard the input pane.");
                VERIFY_IS_NOT_NULL(button);
                button->Focus(FocusState::Pointer);
            });

            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Waiting for GotFocus event.");
            btnGotFocusEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                // Prevent the text control from getting focus again so the input pane remains hidden.
                if (textBox != nullptr)
                {
                    textBox->IsEnabled = false;
                    LOG_OUTPUT(L"TextBox disabled.");
                }
                if (richEditBox != nullptr)
                {
                    richEditBox->IsEnabled = false;
                    LOG_OUTPUT(L"RichEditBox disabled.");
                }
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void TextControlsExpansionTests::ChangeTextControlHeightWithoutAnimation(
            _In_ xaml::FrameworkElement^ root,
            _In_ bool setFocus,
            _In_ bool canWrap,
            _In_ bool acceptsReturn,
            _In_ bool isHeightNaN)
        {
            xaml::FrameworkElement^ fe = nullptr;
            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::RichEditBox^ richEditBox = nullptr;
            xaml_controls::Button^ button = nullptr;

            int sizeChangedCount = 0;

            auto btnGotFocusEvent = std::make_shared<Event>();
            auto feGotFocusEvent = std::make_shared<Event>();
            auto feGotFocusRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, GotFocus);
            auto btnGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
            auto sizeChangedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, SizeChanged);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = root;

                fe = safe_cast<xaml::FrameworkElement^>(root->FindName("textControl"));
                VERIFY_IS_NOT_NULL(fe);

                textBox = dynamic_cast<xaml_controls::TextBox^>(fe);
                if (textBox == nullptr)
                {
                    richEditBox = dynamic_cast<xaml_controls::RichEditBox^>(fe);
                    VERIFY_IS_NOT_NULL(richEditBox);
                    richEditBox->Document->SetText(Microsoft::UI::Text::TextSetOptions::None, L"One Two Three");
                    if (!canWrap)
                    {
                        richEditBox->TextWrapping = Microsoft::UI::Xaml::TextWrapping::NoWrap;
                    }
                    if (!acceptsReturn)
                    {
                        richEditBox->AcceptsReturn = false;
                    }
                }
                else
                {
                    if (!canWrap)
                    {
                        textBox->TextWrapping = Microsoft::UI::Xaml::TextWrapping::NoWrap;
                    }
                    if (!acceptsReturn)
                    {
                        textBox->AcceptsReturn = false;
                    }
                }

                if (!isHeightNaN)
                {
                    fe->Height = 60;
                }

                feGotFocusRegistration.Attach(
                    fe,
                    ref new xaml::RoutedEventHandler(
                    [feGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Text control GotFocus handler.");
                    feGotFocusEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();

            if (setFocus)
            {
                LOG_OUTPUT(L"Setting focus to the text control by tapping.");
                // Tap near the top (0.1f) of the textbox to ensure that the tap isn't adjusted
                // by the touch-hittesting code to be on the (not visible) horizontal scrollbar.
                TestServices::InputHelper->Tap(fe, 0.5f, 0.1f);
                feGotFocusEvent->WaitForDefault();
            }

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE((_isnan(fe->Height) != 0) == isHeightNaN);

                sizeChangedRegistration.Attach(
                    fe,
                    ref new xaml::SizeChangedEventHandler(
                    [&sizeChangedCount, fe](Platform::Object^, xaml::ISizeChangedEventArgs^)
                {
                    sizeChangedCount++;
                    LOG_OUTPUT(L"Text control Height in FrameworkElement.SizeChanged: %f.", fe->Height);
                }));

                button = safe_cast<xaml_controls::Button^>(root->FindName("button"));
                VERIFY_IS_NOT_NULL(button);

                btnGotFocusRegistration.Attach(
                    button,
                    ref new xaml::RoutedEventHandler(
                    [btnGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Button.GotFocus handler.");
                    btnGotFocusEvent->Set();
                }));

                if (isHeightNaN && (canWrap || acceptsReturn))
                {
                    // Change the text control's height by adding text to it.
                    if (richEditBox != nullptr)
                    {
                        LOG_OUTPUT(L"Changing RichEditBox's text to expand control.");
                        richEditBox->Document->SetText(Microsoft::UI::Text::TextSetOptions::None, L"One Two Three Four Five");
                    }
                    else
                    {
                        LOG_OUTPUT(L"Changing RichEditBox's text to expand control.");
                        textBox->Text = L"One Two Three Four Five";
                    }
                }
                else
                {
                    // Adding text would not result in a height change here. So change the height directly.
                    fe->Height = 70;
                }
            });

            TestServices::WindowHelper->WaitForIdle();

            // Expect a single SizeChanged event.
            LOG_OUTPUT(L"Text Control SizeChanged occurrences: %d.", sizeChangedCount);
            VERIFY_IS_TRUE(sizeChangedCount == 1);

            if (setFocus)
            {
                // Discard the input pane since focus was previously given to the text control.
                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Focus the Button to discard the input pane.");
                    VERIFY_IS_NOT_NULL(button);
                    button->Focus(FocusState::Pointer);
                });
                btnGotFocusEvent->WaitForDefault();

                RunOnUIThread([&]()
                {
                    // Prevent the text control from getting focus again so the input pane remains hidden.
                    if (textBox != nullptr)
                    {
                        textBox->IsEnabled = false;
                        LOG_OUTPUT(L"TextBox disabled.");
                    }
                    if (richEditBox != nullptr)
                    {
                        richEditBox->IsEnabled = false;
                        LOG_OUTPUT(L"RichEditBox disabled.");
                    }
                });
            }

            TestServices::WindowHelper->WaitForIdle();
        }

        //------------------------------------------------------------------------
        // Test case: Validates animated growth of a TextBox height when new line
        //            is added with keyboard.
        //------------------------------------------------------------------------
        void TextControlsExpansionTests::AnimateTextBoxHeightByAddingNewLine()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::StackPanel^ rootStackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"TextBoxExpansionTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            AnimateTextControlHeight(rootStackPanel, true /*addNewLine*/);
        }

        //------------------------------------------------------------------------
        // Test case: Validates animated growth of a RichEditBox height when new
        //            line is added with keyboard.
        //------------------------------------------------------------------------
        void TextControlsExpansionTests::AnimateRichEditBoxHeightByAddingNewLine()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::StackPanel^ rootStackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"RichEditBoxExpansionTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            AnimateTextControlHeight(rootStackPanel, true /*addNewLine*/);
        }

        //------------------------------------------------------------------------
        // Test case: Validates animated growth of a TextBox height when text is
        // added programmatically while it has focus.
        //------------------------------------------------------------------------
        void TextControlsExpansionTests::AnimateTextBoxHeightByAddingText()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::StackPanel^ rootStackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"TextBoxExpansionTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            AnimateTextControlHeight(rootStackPanel, false /*addNewLine*/);
        }

        //------------------------------------------------------------------------
        // Test case: Validates animated growth of a RichEditBox height when text
        // is added programmatically while it has focus.
        //------------------------------------------------------------------------
        void TextControlsExpansionTests::AnimateRichEditBoxHeightByAddingText()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::StackPanel^ rootStackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"RichEditBoxExpansionTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            AnimateTextControlHeight(rootStackPanel, false /*addNewLine*/);
        }

        //------------------------------------------------------------------------
        // Test case: Changes the height of a TextBox control by adding text
        // while it does not have focus. No animation is expected.
        //------------------------------------------------------------------------
        void TextControlsExpansionTests::ChangeTextBoxHeightWithoutAnimationNoFocus()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::StackPanel^ rootStackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"TextBoxExpansionTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            ChangeTextControlHeightWithoutAnimation(rootStackPanel, false /*setFocus*/, true /*canWrap*/, true /*acceptsReturn*/, true /*isHeightNaN*/);
        }

        //------------------------------------------------------------------------
        // Test case: Changes the height of a RichEditBox control by adding text
        // while it does not have focus. No animation is expected.
        //------------------------------------------------------------------------
        void TextControlsExpansionTests::ChangeRichEditBoxHeightWithoutAnimationNoFocus()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::StackPanel^ rootStackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"RichEditBoxExpansionTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            ChangeTextControlHeightWithoutAnimation(rootStackPanel, false /*setFocus*/, true /*canWrap*/, true /*acceptsReturn*/, true /*isHeightNaN*/);
        }

        //------------------------------------------------------------------------
        // Test case: Changes the height of a TextBox control while wrapping
        // is off and carriage returns are not accepted. No animation is expected.
        //------------------------------------------------------------------------
        void TextControlsExpansionTests::ChangeTextBoxHeightWithoutAnimationNoWrapAndNoReturn()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::StackPanel^ rootStackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"TextBoxExpansionTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            ChangeTextControlHeightWithoutAnimation(rootStackPanel, true /*setFocus*/, false /*canWrap*/, false /*acceptsReturn*/, true /*isHeightNaN*/);
        }

        //------------------------------------------------------------------------
        // Test case: Changes the height of a RichEditBox control while wrapping
        // is off and carriage returns are not accepted. No animation is expected.
        //------------------------------------------------------------------------
        void TextControlsExpansionTests::ChangeRichEditBoxHeightWithoutAnimationNoWrapAndNoReturn()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::StackPanel^ rootStackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"RichEditBoxExpansionTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            ChangeTextControlHeightWithoutAnimation(rootStackPanel, true /*setFocus*/, false /*canWrap*/, false /*acceptsReturn*/, true /*isHeightNaN*/);
        }

        //------------------------------------------------------------------------
        // Test case: Changes the height of a TextBox control while original
        // height is different from NaN. No animation is expected.
        //------------------------------------------------------------------------
        void TextControlsExpansionTests::ChangeTextBoxHeightWithoutAnimationNoVariableHeight()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::StackPanel^ rootStackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"TextBoxExpansionTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            ChangeTextControlHeightWithoutAnimation(rootStackPanel, true /*setFocus*/, true /*canWrap*/, true /*acceptsReturn*/, false /*isHeightNaN*/);
        }

        //------------------------------------------------------------------------
        // Test case: Changes the height of a RichEditBox control while original
        // height is different from NaN. No animation is expected.
        //------------------------------------------------------------------------
        void TextControlsExpansionTests::ChangeRichEditBoxHeightWithoutAnimationNoVariableHeight()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::StackPanel^ rootStackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"RichEditBoxExpansionTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            ChangeTextControlHeightWithoutAnimation(rootStackPanel, true /*setFocus*/, true /*canWrap*/, true /*acceptsReturn*/, false /*isHeightNaN*/);
        }
    } }
} } } }
