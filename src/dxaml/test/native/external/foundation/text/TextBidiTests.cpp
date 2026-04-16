// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TextBidiTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include <SafeEventRegistration.h>
#include <WUCRenderingScopeGuard.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Documents;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Text {

        Platform::String^ TextBidiTests::GetResourcesPath() const
        {
            return GetPackageFolder() + L"resources\\native\\foundation\\Text\\";
        }

        bool TextBidiTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool TextBidiTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool TextBidiTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        //------------------------------------------------------------------------
        // Test case: Verify TextBlock with combination of text attributes
        // FlowDirection(LTR), TextReadingOrder and Alignment on RTL test string
        //------------------------------------------------------------------------
        void TextBidiTests::TextBlockBidiFlowLTR()
        {
            TextBidiTestHelper(L"TextBlockBidiTests-LTR.xaml");
        }

        //------------------------------------------------------------------------
        // Test case: Verify TextBlock with combination of text attributes
        // FlowDirection(RTL), TextReadingOrder and Alignment on RTL test string
        //------------------------------------------------------------------------
        void TextBidiTests::TextBlockBidiFlowRTL()
        {
            TextBidiTestHelper(L"TextBlockBidiTests-RTL.xaml");
        }

        //------------------------------------------------------------------------
        // Test case: Verify TextBlock with combination of text attributes
        // FlowDirection(LTR), TextReadingOrder and Alignment on LTR test string
        //------------------------------------------------------------------------
        void TextBidiTests::TextBlockBidiFlowLTREnglish()
        {
            TextBidiTestHelper(L"TextBlockBidiTests-LTR-English.xaml");
        }

        //------------------------------------------------------------------------
        // Test case: Verify TextBlock with combination of text attributes
        // FlowDirection(RTL), TextReadingOrder and Alignment on LTR test string
        //------------------------------------------------------------------------
        void TextBidiTests::TextBlockBidiFlowRTLEnglish()
        {
            TextBidiTestHelper(L"TextBlockBidiTests-RTL-English.xaml");
        }

        //------------------------------------------------------------------------
        // Test case: Verify RichTextBlock with combination of text attributes
        // FlowDirection(LTR), TextReadingOrder and Alignment on RTL test string
        //------------------------------------------------------------------------
        void TextBidiTests::RichTextBlockBidiFlowLTR()
        {
            TextBidiTestHelper(L"RichTextBlockBidiTests-LTR.xaml");
        }

        //------------------------------------------------------------------------
        // Test case: Verify RichTextBlock with combination of text attributes
        // FlowDirection(RTL), TextReadingOrder and Alignment on RTL test string
        //------------------------------------------------------------------------
        void TextBidiTests::RichTextBlockBidiFlowRTL()
        {
            TextBidiTestHelper(L"RichTextBlockBidiTests-RTL.xaml");
        }

        //------------------------------------------------------------------------
        // Test case: Verify RichTextBlock with combination of text attributes
        // FlowDirection(LTR), TextReadingOrder and Alignment on LTR test string
        //------------------------------------------------------------------------
        void TextBidiTests::RichTextBlockBidiFlowLTREnglish()
        {
            TextBidiTestHelper(L"RichTextBlockBidiTests-LTR-English.xaml");
        }

        //------------------------------------------------------------------------
        // Test case: Verify RichTextBlock with combination of text attributes
        // FlowDirection(RTL), TextReadingOrder and Alignment on LTR test string
        //------------------------------------------------------------------------
        void TextBidiTests::RichTextBlockBidiFlowRTLEnglish()
        {
            TextBidiTestHelper(L"RichTextBlockBidiTests-RTL-English.xaml");
        }

        //------------------------------------------------------------------------
        // Test case: Verify TextBox with combination of text attributes
        // FlowDirection(LTR), TextReadingOrder and Alignment on RTL test string
        //------------------------------------------------------------------------
        void TextBidiTests::TextBoxBidiFlowLTR()
        {
            TextBidiTestHelper(L"TextBoxBidiTests-LTR.xaml");
        }

        //------------------------------------------------------------------------
        // Test case: Verify TextBox with combination of text attributes
        // FlowDirection(RTL), TextReadingOrder and Alignment on RTL test string
        //------------------------------------------------------------------------
        void TextBidiTests::TextBoxBidiFlowRTL()
        {
            TextBidiTestHelper(L"TextBoxBidiTests-RTL.xaml");
        }

        //------------------------------------------------------------------------
        // Test case: Verify TextBox with combination of text attributes
        // FlowDirection(LTR), TextReadingOrder and Alignment on LTR test string
        //------------------------------------------------------------------------
        void TextBidiTests::TextBoxBidiFlowLTREnglish()
        {
            TextBidiTestHelper(L"TextBoxBidiTests-LTR-English.xaml");
        }

        //------------------------------------------------------------------------
        // Test case: Verify TextBox with combination of text attributes
        // FlowDirection(RTL), TextReadingOrder and Alignment on LTR test string
        //------------------------------------------------------------------------
        void TextBidiTests::TextBoxBidiFlowRTLEnglish()
        {
            TextBidiTestHelper(L"TextBoxBidiTests-RTL-English.xaml");
        }

        void TextBidiTests::PasswordBoxBidiFlowLTREnglish()
        {
            GenerateAndTestPasswordBoxUIText(true, FlowDirection::LeftToRight);
        }

        void TextBidiTests::PasswordBoxBidiFlowRTLEnglish()
        {
            GenerateAndTestPasswordBoxUIText(true, FlowDirection::RightToLeft);
        }

        void TextBidiTests::PasswordBoxBidiFlowLTR()
        {
            GenerateAndTestPasswordBoxUIText(false, FlowDirection::LeftToRight);
        }

        void TextBidiTests::PasswordBoxBidiFlowRTL()
        {
            GenerateAndTestPasswordBoxUIText(false, FlowDirection::RightToLeft);
        }

        void TextBidiTests::TextBidiTestHelper(Platform::String^ filename)
        {
             // Clear out the current window content before injecting MockDComp, to
            // MockDComp doesn't capture an image for anything currently in the content,
            // since that will interfere with the expected surface counts.
            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = nullptr;
            });

            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));
            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + filename));
            auto rootLoadedEvent = std::make_shared<Event>();
            auto rootLoadedRegistration = CreateSafeEventRegistration(Panel, Loaded);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = root;
                rootLoadedRegistration.Attach(
                    root,
                    ref new xaml::RoutedEventHandler(
                    [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Root FE Loaded handler.");
                    rootLoadedEvent->Set();
                }));
            });

            LOG_OUTPUT(L"Waiting for root Loaded event.");
            rootLoadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            // dump all surfaces to verify Bidi text displayed correctly
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
        }

        //---------------------------------------------------------------------------
        // Test case: Verify TextBox initial cursor placement based on input language
        //            when TextAlignment is set to DTC
        //---------------------------------------------------------------------------
        void TextBidiTests::TextBoxBidiCursorPlacement()
        {
             // Clear out the current window content before injecting MockDComp, to
            // MockDComp doesn't capture an image for anything currently in the content,
            // since that will interfere with the expected surface counts.
            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = nullptr;
            });

            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));
            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"TextBoxBidiTests-FlowRTL.xaml"));
            xaml_controls::TextBox^ textBoxAlignLeft = nullptr;
            xaml_controls::TextBox^ textBoxAlignDTC = nullptr;
            xaml_controls::Button^ button = nullptr;

            auto rootLoadedEvent = std::make_shared<Event>();
            auto rootLoadedRegistration = CreateSafeEventRegistration(Panel, Loaded);
            auto textBoxLeftGotFocusEvent = std::make_shared<Event>();
            auto textBoxLeftGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);
            auto textBoxDTCGotFocusEvent = std::make_shared<Event>();
            auto textBoxDTCGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);
            auto buttonGotFocusEvent = std::make_shared<Event>();
            auto buttonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            RunOnUIThread([&]()
            {
                rootLoadedRegistration.Attach(
                    root,
                    ref new xaml::RoutedEventHandler(
                    [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Root FE Loaded handler.");
                    rootLoadedEvent->Set();
                }));

                textBoxAlignLeft = safe_cast<xaml_controls::TextBox^>(root->FindName(L"textBoxAlignLeft"));
                VERIFY_IS_NOT_NULL(textBoxAlignLeft);
                textBoxAlignDTC = safe_cast<xaml_controls::TextBox^>(root->FindName(L"textBoxAlignDTC"));
                VERIFY_IS_NOT_NULL(textBoxAlignDTC);
                button = safe_cast<xaml_controls::Button^>(root->FindName("button"));
                VERIFY_IS_NOT_NULL(button);

                textBoxLeftGotFocusRegistration.Attach(
                    textBoxAlignLeft,
                    ref new xaml::RoutedEventHandler(
                    [textBoxLeftGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"TextBoxLeft control GotFocus handler.");
                    textBoxLeftGotFocusEvent->Set();
                }));

                textBoxDTCGotFocusRegistration.Attach(
                    textBoxAlignDTC,
                    ref new xaml::RoutedEventHandler(
                    [textBoxDTCGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"TextBoxDTC control GotFocus handler.");
                    textBoxDTCGotFocusEvent->Set();
                }));

                buttonGotFocusRegistration.Attach(
                    button,
                    ref new xaml::RoutedEventHandler(
                    [buttonGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Button control GotFocus handler.");
                    buttonGotFocusEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = root;
            });

            LOG_OUTPUT(L"Waiting for root Loaded event.");
            rootLoadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Setting focus to the textBoxAlignLeft control by tapping.");
            TestServices::InputHelper->Tap(textBoxAlignLeft);
            textBoxLeftGotFocusEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->PressKeySequence(L"a"); // Text should start typing from right, mirror grid's RTL flow direction
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Setting focus to the textBoxAlignDTC control by tapping.");
            TestServices::InputHelper->Tap(textBoxAlignDTC);
            textBoxDTCGotFocusEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->PressKeySequence(L"a"); // Cursor placement determined by input language, text should start typing from left
            TestServices::WindowHelper->WaitForIdle();

            // tap button to remove focus on TextBox
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Focus the Button to discard the input pane.");
                button->Focus(FocusState::Pointer);
            });
            buttonGotFocusEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            // DComp dump to verify initial cursor placement: right for first TextBox and left for second
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }

        void TextBidiTests::GenerateAndTestPasswordBoxUIText(bool useEnglish, FlowDirection flowDirection)
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

            StackPanel^ sp = nullptr;
            std::vector<PasswordBox^> passwordBoxes;
            xaml_controls::Button^ button = nullptr;

            auto panelLoadedEvent = std::make_shared<Event>();
            auto panelLoadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

            RunOnUIThread([&]()
            {
                sp = ref new StackPanel();

                wf::Rect visibleBounds = TestServices::WindowHelper->VisibleBounds;
                sp->Margin = xaml::Thickness({visibleBounds.X, visibleBounds.Y, 0, 0});

                // Insert a button to take focus.
                button = ref new xaml_controls::Button();
                sp->Children->Append(button);

                for (int i = 0; i < 2; i++)
                {
                    passwordBoxes.push_back(ref new PasswordBox());
                }

                passwordBoxes[0]->TextReadingOrder = TextReadingOrder::UseFlowDirection;
                passwordBoxes[1]->TextReadingOrder = TextReadingOrder::DetectFromContent;

                for each (PasswordBox ^ passwordBox in  passwordBoxes)
                {
                    passwordBox->Width = 200;
                    passwordBox->Height = 35;

                    passwordBox->FontFamily = ref new xaml_media::FontFamily("Segoe UI");
                    passwordBox->FontSize = 15;
                    passwordBox->FlowDirection = flowDirection;
                    passwordBox->Margin = xaml::Thickness({20, 5, 10, 0});
                    passwordBox->Padding = xaml::Thickness({10, 3, 6, 5}); // Restore old TextControlThemePadding padding value when TextBox defaulted to 15 font size instead of 14

                    useEnglish ? passwordBox->Password = "English" : passwordBox->Password = L"نسيايب123 !";
                    passwordBox->PasswordRevealMode = PasswordRevealMode::Visible;

                    sp->Children->Append(passwordBox);
                }
                panelLoadedRegistration.Attach(sp, [&]() { panelLoadedEvent->Set(); });

                TestServices::WindowHelper->WindowContent = sp;
            });

            panelLoadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            // Verify password rendered correctly
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
        }
    } }
} } } }
