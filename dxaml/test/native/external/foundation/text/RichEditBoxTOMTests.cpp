// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "RichEditBoxTOMTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <ppltasks.h>
#include "FileLoader.h"
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <MUX-ETWEvents.h>
#include "ETWWaiterProxy.h"
#include "TextBoxGenericTests.h"
#include "KeyboardInjectionOverride.h"
#include <WUCRenderingScopeGuard.h>
#include <FocusTestHelper.h>

using namespace concurrency;
using namespace ::Windows::Storage;
using namespace ::Windows::Foundation;
using namespace ::Windows::Storage::Streams;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace MockDComp;
using namespace Microsoft::UI::Xaml::Input;
using namespace ::Windows::ApplicationModel;
using namespace ::Windows::ApplicationModel::DataTransfer;
using namespace Microsoft::UI::Text;


namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Text {

        bool RichEditBoxTOMTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool RichEditBoxTOMTests::ClassCleanup()
        {
            return true;
        }

        bool RichEditBoxTOMTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool RichEditBoxTOMTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        Platform::String^ RichEditBoxTOMTests::GetResourcesPath() const
        {
            return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\image\\";
        }

        void RichEditBoxTOMTests::TestSelection()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto selectionChangedEvent = std::make_shared<Event>();
            auto richEditBoxSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, SelectionChanged);
            auto reBoxGotFocusEvent = std::make_shared<Event>();
            auto reBoxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, GotFocus);

            xaml_controls::RichEditBox^ rebx = nullptr;
            xaml_controls::Button^ button = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <RichEditBox x:Name = 'rebx' Width='200' Height='60' Margin = '20,40,20,0' />"
                    L"</StackPanel>"));
                rebx = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx"));
                VERIFY_IS_NOT_NULL(rebx);
                TestServices::WindowHelper->WindowContent = rootPanel;

                richEditBoxSelectionChangedRegistration.Attach(
                    rebx,
                    ref new xaml::RoutedEventHandler(
                    [selectionChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"RichEditBox SelectionChanged handler.");
                    selectionChangedEvent->Set();
                }));

                reBoxGotFocusRegistration.Attach(
                    rebx,
                    ref new xaml::RoutedEventHandler(
                    [reBoxGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"RichEditBox control GotFocus handler.");
                    reBoxGotFocusEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                rebx->Focus(FocusState::Pointer);
            });
            reBoxGotFocusEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                rebx->Document->SetText(Microsoft::UI::Text::TextSetOptions::None, L"hello world");
            });
            TestServices::WindowHelper->WaitForIdle();

            selectionChangedEvent->Reset();
            RunOnUIThread([&]()
            {
                rebx->Document->Selection->StartPosition = 1;
                rebx->Document->Selection->EndPosition = 4;
            });
            selectionChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            // verify selection changed
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(rebx->Document->Selection->StartPosition == 1);
                VERIFY_IS_TRUE(rebx->Document->Selection->EndPosition == 4);
            });

            selectionChangedEvent->Reset();
            RunOnUIThread([&]()
            {
                Microsoft::UI::Text::ITextRange^ Range1 = rebx->Document->GetRange(4, 8);
                Microsoft::UI::Text::ITextRange^ Range2 = Range1->GetClone();
                Range2->MatchSelection();
            });
            selectionChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            // verify selection changed
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(rebx->Document->Selection->StartPosition == 4);
                VERIFY_IS_TRUE(rebx->Document->Selection->EndPosition == 8);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                rebx->IsEnabled = false; // disable textbox in case SIP is opened.
            });
        }

        void RichEditBoxTOMTests::VerifyClearUndoRedoHistory()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto tb1GotFocusEvent = std::make_shared<Event>();
            auto tb1GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, GotFocus);

            auto tb2GotFocusEvent = std::make_shared<Event>();
            auto tb2GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, GotFocus);

            auto tb1TextChangedEvent = std::make_shared<Event>();
            auto tb1TextChangedRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, TextChanged);

            xaml_controls::Grid^ rootGrid = nullptr;
            xaml_controls::RichEditBox^ tb1 = nullptr;
            xaml_controls::RichEditBox^ tb2 = nullptr;
            xaml_controls::StackPanel^ stackPanel = nullptr;

            RunOnUIThread([&]()
            {
                rootGrid = ref new xaml_controls::Grid;
                rootGrid->Width = 400;
                TestServices::WindowHelper->WindowContent = rootGrid;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                stackPanel = ref new xaml_controls::StackPanel();
                tb1 = ref new xaml_controls::RichEditBox();
                tb2 = ref new xaml_controls::RichEditBox();
                tb1->Document->SetText(Microsoft::UI::Text::TextSetOptions::None, "Text goes here");
                stackPanel->Children->Append(tb1);
                stackPanel->Children->Append(tb2);
                rootGrid->Children->Append(stackPanel);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                tb1GotFocusRegistration.Attach(
                    tb1,
                    ref new xaml::RoutedEventHandler(
                        [tb1GotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"tb1 GotFocus handler");
                    tb1GotFocusEvent->Set();
                }));

                tb1TextChangedRegistration.Attach(
                    tb1,
                    ref new xaml::RoutedEventHandler(
                        [tb1TextChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"tb1 TextChanged handler");
                    tb1TextChangedEvent->Set();
                }));

                tb2GotFocusRegistration.Attach(
                    tb2,
                    ref new xaml::RoutedEventHandler(
                        [tb2GotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"tb2 GotFocus handler");
                    tb2GotFocusEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                tb2->Focus(FocusState::Pointer);
            });

            tb2GotFocusEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                try
                {
                    // Calling Undo on untouched/default textbox
                    LOG_OUTPUT(L"tb2->TextDocument->ClearUndoRedoHistory()");
                    tb2->TextDocument->ClearUndoRedoHistory();
                }
                catch (...)
                {
                    VERIFY_IS_TRUE(false, L"ClearUndoRedoHistory on default textbox threw an exception.");
                }
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"tb1->SetText(empty)");
                tb1->Document->SetText(Microsoft::UI::Text::TextSetOptions::None, "");
            });

            tb1TextChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // setting the text to empty should have cleared tb1's text
                Platform::String^ text;
                tb1->Document->GetText(mut::TextGetOptions::AdjustCrlf, &text);
                VERIFY_IS_TRUE(text == nullptr);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"tb1->TextDocument->ClearUndoRedoHistory()");
                tb1->TextDocument->ClearUndoRedoHistory();
                tb1TextChangedEvent->Reset();
                LOG_OUTPUT(L"tb1->Undo()");
                tb1->Document->Undo();
            });

            TestServices::WindowHelper->WaitForIdle();
            VERIFY_IS_FALSE(tb1TextChangedEvent->HasFired());

            RunOnUIThread([&]()
            {
                // tb1's text should NOT have been restored if ClearUndoRedoHistory was successful
                Platform::String^ text;
                tb1->Document->GetText(mut::TextGetOptions::AdjustCrlf, &text);
                VERIFY_IS_TRUE(text == nullptr);
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void RichEditBoxTOMTests::TestClipboardCopyFormats()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto reBoxGotFocusEvent = std::make_shared<Event>();
            auto targetTextChangedEvent0 = std::make_shared<Event>();
            auto targetTextChangedEvent1 = std::make_shared<Event>();
            auto targetGotFocusEvent0 = std::make_shared<Event>();
            auto targetGotFocusEvent1 = std::make_shared<Event>();
            auto reBoxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, GotFocus);
            auto targetGotFocusRegistration0 = CreateSafeEventRegistration(xaml_controls::RichEditBox, GotFocus);
            auto targetGotFocusRegistration1 = CreateSafeEventRegistration(xaml_controls::RichEditBox, GotFocus);
            auto targetTextChangedRegistration0 = CreateSafeEventRegistration(xaml_controls::RichEditBox, TextChanged);
            auto targetTextChangedRegistration1 = CreateSafeEventRegistration(xaml_controls::RichEditBox, TextChanged);

            xaml_controls::RichEditBox^ rebx;
            xaml_controls::RichEditBox^ target0;
            xaml_controls::RichEditBox^ target1;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(LR"(
                    <StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >
                      <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />
                      <RichEditBox x:Name = 'rebx' Width='200' Height='60' Margin = '20,40,20,0' ContextFlyout='{x:Null}' SelectionFlyout='{x:Null}' />
                      <RichEditBox x:Name = 'target0' Width='200' Height='60' ContextFlyout='{x:Null}' SelectionFlyout='{x:Null}' />
                      <RichEditBox x:Name = 'target1' Width='200' Height='60' ContextFlyout='{x:Null}' SelectionFlyout='{x:Null}' />
                    </StackPanel>
                )"));
                rebx = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx"));
                target0 = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"target0"));
                target1 = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"target1"));
                VERIFY_IS_NOT_NULL(rebx);
                VERIFY_IS_NOT_NULL(target0);
                VERIFY_IS_NOT_NULL(target1);

                reBoxGotFocusRegistration.Attach(
                    rebx,
                    ref new xaml::RoutedEventHandler(
                    [reBoxGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"RichEditBox control GotFocus handler.");
                    reBoxGotFocusEvent->Set();
                }));

                targetGotFocusRegistration0.Attach(
                    target0,
                    ref new xaml::RoutedEventHandler(
                    [targetGotFocusEvent0](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"RichEditBox 'target0' GotFocus handler.");
                    targetGotFocusEvent0->Set();
                }));

                targetGotFocusRegistration1.Attach(
                    target1,
                    ref new xaml::RoutedEventHandler(
                    [targetGotFocusEvent1](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"RichEditBox 'target1' GotFocus handler.");
                    targetGotFocusEvent1->Set();
                }));
                
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Setting focus to the RichEditBox control by tapping.");
            TestServices::InputHelper->Tap(rebx);
            reBoxGotFocusEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                rebx->Document->SetText(mut::TextSetOptions::None, L"world hello");
                rebx->Document->Selection->StartPosition = 0;
                rebx->Document->Selection->EndPosition = 11;
                rebx->Document->Selection->CharacterFormat->Italic = mut::FormatEffect::On;
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Setting focus to the 'target0' RichEditBox control by tapping.");
            TestServices::InputHelper->Tap(target0);
            targetGotFocusEvent0->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            targetTextChangedRegistration0.Attach(
                target0,
                ref new xaml::RoutedEventHandler(
                [targetTextChangedEvent0](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"RichEditBox 'target0' TextChanged handler.");
                targetTextChangedEvent0->Set();
            }));

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Copy&Paste to 'target0' with formatting preserved");
                rebx->Document->Selection->Copy();
            });
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_v#$u$_v#$u$_ctrl"); // Ctrl+V

            LOG_OUTPUT(L"Waiting for 'target0' TextChanged.");
            targetTextChangedEvent0->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(mut::FormatEffect::On, target0->Document->Selection->CharacterFormat->Italic);
            });


            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Setting focus to the 'target1' RichEditBox control by tapping.");
            TestServices::InputHelper->Tap(target1);
            targetGotFocusEvent1->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            targetTextChangedRegistration1.Attach(
                target1,
                ref new xaml::RoutedEventHandler(
                [targetTextChangedEvent1](Platform::Object^, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"RichEditBox 'target1' TextChanged handler.");
                targetTextChangedEvent1->Set();
            }));

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Copy&Paste to 'target1' as plain text");
                rebx->ClipboardCopyFormat = xaml_controls::RichEditClipboardFormat::PlainText;
                rebx->Document->Selection->Copy();
            });
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_v#$u$_v#$u$_ctrl"); // Ctrl+V

            LOG_OUTPUT(L"Waiting for 'target1' TextChanged.");
            targetTextChangedEvent1->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(mut::FormatEffect::Off, target1->Document->Selection->CharacterFormat->Italic);
            });

            RunOnUIThread([&]()
            {
                // disable textboxes in case SIP is opened.
                rebx->IsEnabled = false;
                target0->IsEnabled = false;
                target1->IsEnabled = false;
            });
        }

        void RichEditBoxTOMTests::TestLongStringPaste()
        {
            RuntimeEnabledFeatureOverride disableTextBoxCaret(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableTextBoxCaret, true);
            KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride;

            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);
            Platform::String ^toReplaceWith;
            for (int i = 0; i < 256; i++)
            {
                toReplaceWith += L'A';
            }
            auto tb2TextChangedEvent = std::make_shared<Event>();
            auto tb2TextChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, TextChanged);

            xaml_controls::Grid^ rootGrid = nullptr;
            xaml_controls::TextBox^ tb1 = nullptr;
            xaml_controls::TextBox^ tb2 = nullptr;
            xaml_controls::StackPanel^ stackPanel = nullptr;

            RunOnUIThread([&]()
            {
                rootGrid = ref new xaml_controls::Grid;
                rootGrid->Width = 400;
                TestServices::WindowHelper->WindowContent = rootGrid;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                stackPanel = ref new xaml_controls::StackPanel();
                tb1 = ref new xaml_controls::TextBox();
                tb1->Text = toReplaceWith;
                tb1->PreventKeyboardDisplayOnProgrammaticFocus = true;
                tb2 = ref new xaml_controls::TextBox();
                tb2->PreventKeyboardDisplayOnProgrammaticFocus = true;
                stackPanel->Children->Append(tb1);
                stackPanel->Children->Append(tb2);
                rootGrid->Children->Append(stackPanel);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                tb2TextChangedRegistration.Attach(
                    tb2,
                    ref new xaml_controls::TextChangedEventHandler(
                        [tb2TextChangedEvent](Platform::Object^, xaml_controls::TextChangedEventArgs^)
                {
                    LOG_OUTPUT(L"Tb2 TextChanged handler.");
                    tb2TextChangedEvent->Set();
                }));
            });
            TestServices::WindowHelper->WaitForIdle();

            FocusTestHelper::EnsureFocus(tb1, FocusState::Programmatic);
            RunOnUIThread([&]()
            {
                tb1->SelectAll();
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_c#$u$_c#$u$_ctrlscan"); // Ctrl+C
            TestServices::WindowHelper->WaitForIdle();

            FocusTestHelper::EnsureFocus(tb2, FocusState::Programmatic);
            TestServices::WindowHelper->WaitForIdle();

            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_v#$u$_v#$u$_ctrlscan"); // Ctrl+V
            TestServices::WindowHelper->WaitForIdle();
            tb2TextChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                // tb2 should have the tb1 Text now if copy/paste are successful
                VERIFY_ARE_EQUAL(toReplaceWith, tb2->Text);
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }

        void RichEditBoxTOMTests::DocumentDraw()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::RichEditBox^ rebxBitmap;
            xaml_controls::RichEditBox^ rebxLink;
            auto textChangedEvent = std::make_shared<Event>();
            auto textChangedRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, TextChanged);


            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(LR"(
                    <StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >
                      <Button x:Name='button' Content='Focus' Margin='20,40,20,0' />
                      <RichEditBox x:Name='rebxBitmap' Width='200' Height='60' Margin='20,40,20,0' FontSize='15'/>
                      <RichEditBox x:Name='rebxLink' Width='200' Height='60' Margin='20,40,20,0' FontSize='15'/>
                    </StackPanel>
                )"));

                rebxBitmap = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebxBitmap"));
                VERIFY_IS_NOT_NULL(rebxBitmap);
                rebxLink = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebxLink"));
                VERIFY_IS_NOT_NULL(rebxLink);
                TestServices::WindowHelper->WindowContent = rootPanel;
                textChangedRegistration.Attach(
                    rebxBitmap,
                    ref new xaml::RoutedEventHandler(
                        [textChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"RichEditBox TextChanged handler.");
                    textChangedEvent->Set();
                }));

            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                rebxBitmap->Document->SetText(mut::TextSetOptions::None, L"Test Bitmap");
                rebxBitmap->Document->Selection->StartPosition = 5;
                rebxBitmap->Document->Selection->EndPosition = 5;
                rebxLink->Document->SetText(mut::TextSetOptions::None, L"http://msn.com");
                rebxLink->Document->Selection->StartPosition = 0;
                rebxLink->Document->Selection->EndPosition = 14;
            });
            TestServices::WindowHelper->WaitForIdle();
            textChangedEvent->WaitForDefault(); // make sure text changed event is fired by calling SetText on Document property

            RunOnUIThread([&]()
            {
                auto cf = rebxBitmap->Document->GetDefaultCharacterFormat();
                cf->Bold = mut::FormatEffect::On;
                cf->Underline = mut::UnderlineType::Single;
                cf->Strikethrough = mut::FormatEffect::On;
                rebxLink->Document->SetDefaultCharacterFormat(cf);
                rebxLink->Document->Selection->Link = "\"http://msn.com\"";
            });

            TestServices::WindowHelper->WaitForIdle();

            auto pBitmapInsertedEvent = std::make_shared<Event>();
            create_task(StorageFile::GetFileFromPathAsync(GetResourcesPath() + L"rain.png"))
            .then([=](StorageFile^ pFile)
            {
                VERIFY_IS_NOT_NULL(pFile);

                create_task(pFile->OpenAsync(::Windows::Storage::FileAccessMode::Read))
                    .then([=](IRandomAccessStream^ pFileStream)
                {
                    VERIFY_IS_NOT_NULL(pFileStream);

                    textChangedEvent->Reset();
                    RunOnUIThread([=]()
                    {
                        rebxBitmap->Document->Selection->InsertImage(50, 50, 10, Microsoft::UI::Text::VerticalCharacterAlignment::Baseline, "stuff", pFileStream);
                        pBitmapInsertedEvent->Set();
                    });
                    textChangedEvent->WaitForDefault(); // inserting image also causing text changed event to fire
                });
            });

            pBitmapInsertedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
        }

        void RichEditBoxTOMTests::TestLinkNavigation()
        {
            KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride;
            TestCleanupWrapper cleanup;

            ETWWaiterProxy etwWaiter;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto reBoxGotFocusEvent = std::make_shared<Event>();
            auto reBoxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, GotFocus);

            xaml_controls::RichEditBox^ rebx = nullptr;
            xaml_controls::Button^ button = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <RichEditBox x:Name = 'rebx' Width='100' Height='25' Margin = '20,40,20,0' />"
                    L"</StackPanel>"));
                rebx = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx"));
                VERIFY_IS_NOT_NULL(rebx);
                button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
                VERIFY_IS_NOT_NULL(button);
                TestServices::WindowHelper->WindowContent = rootPanel;

                reBoxGotFocusRegistration.Attach(
                    rebx,
                    ref new xaml::RoutedEventHandler(
                        [reBoxGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"RichEditBox control GotFocus handler.");
                    reBoxGotFocusEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();

            Platform::String^ etwFilterString = L"@InvokeResult=0";
            etwWaiter.Start(
                WINDOWS_UI_XAML_ETW_PROVIDER,
                CoreWebViewHostInvokeInfo_value,
                etwFilterString);

            RunOnUIThread([&]()
            {
                rebx->Document->SetText(Microsoft::UI::Text::TextSetOptions::None, "Click me to bing.com");
                rebx->Document->GetRange(0, 20)->Link = "\"http://bing.com\"";
            });

            // setting focus on the button to make sure app goes foreground
            FocusTestHelper::EnsureFocus(button, FocusState::Keyboard, 3 /* Attempts */);
            TestServices::WindowHelper->WaitForIdle();

            // Mouse left button click to set focus and cursor inside the link text
            TestServices::InputHelper->ClickMouseButton(MouseButton::Left, rebx);
            reBoxGotFocusEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            // Press enter key should bring up link navigation
            TestServices::KeyboardHelper->Enter();
            TestServices::WindowHelper->WaitForIdle();

            // Using ETW event to check if webViewHost launched navigation successfully
            etwWaiter.WaitForDefault();

            RunOnUIThread([&]()
            {
                rebx->IsEnabled = false; // disable TextBox in case SIP is opened.
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::WindowHelper->RestoreForegroundWindow();

            FocusTestHelper::EnsureFocus(button, FocusState::Keyboard, 3 /* Attempts */);
            TestServices::WindowHelper->WaitForIdle();
        }

        void RichEditBoxTOMTests::CheckFiresManipulationEvents()
        {
            TestCleanupWrapper cleanup;
            TextBoxGenericTests<xaml_controls::RichEditBox>::CanFireManipulationEvents();
        }

        void RichEditBoxTOMTests::RangeFormat()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::RichEditBox^ rebx = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <RichEditBox x:Name = 'rebx' Width='350' Height='25' Margin = '20,40,20,0' IsSpellCheckEnabled='False' PreventKeyboardDisplayOnProgrammaticFocus='True' />"
                    L"</StackPanel>"));
                rebx = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx"));
                VERIFY_IS_NOT_NULL(rebx);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(rebx, FocusState::Programmatic);

            RunOnUIThread([&]()
            {
                rebx->Document->SetText(Microsoft::UI::Text::TextSetOptions::None, "Range1 Range2 Range3");
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Update part of the Text to different format other that default
                Microsoft::UI::Text::ITextRange^ Range2 = rebx->Document->GetRange(7, 12);
                Range2->CharacterFormat->Bold = Microsoft::UI::Text::FormatEffect::On;
                Range2->CharacterFormat->Underline = Microsoft::UI::Text::UnderlineType::Double;
                Range2->CharacterFormat->ForegroundColor = Microsoft::UI::Colors::Red;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Selection(Cursor) moved to end of the Text Range where it has special format applied
                rebx->Document->Selection->SetRange(12, 12);
            });
            TestServices::WindowHelper->WaitForIdle();
            // verify newly typed text uses the special format of the range.
            TestServices::KeyboardHelper->PressKeySequence("Typed Text");
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

            RunOnUIThread([&]()
            {
                rebx->IsEnabled = false; // disable textbox in case SIP is opened.
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void RichEditBoxTOMTests::RangeFormatSizeAndSpacing()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::RichEditBox^ rebx = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <RichEditBox x:Name = 'rebx' Width='200' Height='350' Margin = '20,40,20,0' IsSpellCheckEnabled='False' />"
                    L"</StackPanel>"));
                rebx = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx"));
                VERIFY_IS_NOT_NULL(rebx);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                rebx->Document->SetText(Microsoft::UI::Text::TextSetOptions::None, "123 567 90");
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {

                Microsoft::UI::Text::ITextRange^ rangeTotal = rebx->Document->GetRange(0, 10);
                rangeTotal->CharacterFormat->Size = 20;
                rangeTotal->CharacterFormat->Spacing = 0;

                Microsoft::UI::Text::ITextRange^ rangePartial1 = rebx->Document->GetRange(4, 7);
                rangePartial1->CharacterFormat->Size = 30;
                rangePartial1->CharacterFormat->Spacing = 50;

                Microsoft::UI::Text::ITextRange^ rangePartial2 = rebx->Document->GetRange(7,10);
                rangePartial2->CharacterFormat->Size = 40;
                rangePartial2->CharacterFormat->Spacing = 70;

            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

        }

        void RichEditBoxTOMTests::SelectionFormat()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::RichEditBox^ rebx = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <RichEditBox x:Name = 'rebx' Width='350' Height='25' Margin = '20,40,20,0' IsSpellCheckEnabled='False' PreventKeyboardDisplayOnProgrammaticFocus='True'/>"
                    L"</StackPanel>"));
                rebx = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx"));
                VERIFY_IS_NOT_NULL(rebx);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            FocusTestHelper::EnsureFocus(rebx, FocusState::Programmatic);

            RunOnUIThread([&]()
            {
                // Update Text Selection format to different from default
                rebx->Document->Selection->CharacterFormat->Bold = Microsoft::UI::Text::FormatEffect::On;
            });

            TestServices::KeyboardHelper->PressKeySequence("12");
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                // Update color for the Text Selection, "12" should be in black, "34" should be in red and all text bold
                rebx->Document->Selection->CharacterFormat->ForegroundColor = Microsoft::UI::Colors::Red;
            });
            TestServices::KeyboardHelper->PressKeySequence("34");
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"1");

            RunOnUIThread([&]()
            {
                // select all Text and change underline style, it should also pick up foreground color from "12", so "56" should be bold, black and underlined
                rebx->Document->Selection->SetRange(0, 4);
                rebx->Document->Selection->CharacterFormat->Underline = Microsoft::UI::Text::UnderlineType::Double;
            });
            TestServices::KeyboardHelper->PressKeySequence("56");
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

            RunOnUIThread([&]()
            {
                rebx->IsEnabled = false; // disable textbox in case SIP is opened.
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void RichEditBoxTOMTests::PasteImage()
        {
            KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride;
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::Button^ button;
            xaml_controls::RichEditBox^ rebxPaste;

            auto richeditBoxPasteEvent = std::make_shared<Event>();
            auto richeditBoxPasteRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, Paste);
            bool handlePasteEvent = true;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(LR"(
                    <StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >
                      <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />
                      <RichEditBox x:Name = 'rebxPaste' Width='200' Height='250' Margin = '20,5,20,0' />
                    </StackPanel>
                )"));

                rebxPaste = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebxPaste"));
                VERIFY_IS_NOT_NULL(rebxPaste);
                button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
                VERIFY_IS_NOT_NULL(button);
                TestServices::WindowHelper->WindowContent = rootPanel;

                richeditBoxPasteRegistration.Attach(
                    rebxPaste,
                    ref new xaml_controls::TextControlPasteEventHandler(
                        [&](Platform::Object^, xaml_controls::TextControlPasteEventArgs^ e)
                {
                    if (handlePasteEvent)
                    {
                        LOG_OUTPUT(L"RichEditBox paste event handled.");
                        e->Handled = true;
                    }
                    else
                    {
                        LOG_OUTPUT(L"RichEditBox paste event not handled.");
                    }
                    richeditBoxPasteEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();

            auto clipboardBitmapSet = std::make_shared<Event>();
            create_task(StorageFile::GetFileFromPathAsync(GetResourcesPath() + L"Smiley.bmp"))
                .then([=](StorageFile^ pFile)
            {
                VERIFY_IS_NOT_NULL(pFile);
                auto imgStreamRef = RandomAccessStreamReference::CreateFromFile(pFile);
                auto dataPackage = ref new DataPackage();
                dataPackage->SetBitmap(imgStreamRef);

                RunOnUIThread([=]()
                {
                    DataTransfer::Clipboard::SetContent(dataPackage);
                    clipboardBitmapSet->Set();
                });
            });

            clipboardBitmapSet->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            // putting focus on the RichEditBox and do a Ctrl-V to paste the Bitmap from Clipboard
            FocusTestHelper::EnsureFocus(rebxPaste, FocusState::Keyboard);
            TestServices::WindowHelper->WaitForIdle();
            auto pasteTextChangedEvent = std::make_shared<Event>();
            auto pasteTextChangedRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, TextChanged);
            RunOnUIThread([&]()
            {
                pasteTextChangedRegistration.Attach(
                    rebxPaste,
                    [pasteTextChangedEvent]()
                    {
                        LOG_OUTPUT(L"Paste RichEditBox TextChanged handler.");
                        pasteTextChangedEvent->Set();
                    });
            });

            LOG_OUTPUT(L"Testing bitmap paste when paste event is handled.");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_v#$u$_ctrlscan#$u$_v");
            // verify paste event fired
            richeditBoxPasteEvent->WaitForDefault();

            // text change event should not fire since paste event is handled by test
            pasteTextChangedEvent->WaitForNoThrow(std::chrono::milliseconds(100));
            VERIFY_IS_FALSE(pasteTextChangedEvent->HasFired());

            LOG_OUTPUT(L"Testing bitmap paste when paste event is not handled.");
            handlePasteEvent = false;
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_v#$u$_ctrlscan#$u$_v");
            // verify paste event fired
            richeditBoxPasteEvent->WaitForDefault();
            pasteTextChangedEvent->WaitForDefault();

            // putting focus on button to bring down SIP
            FocusTestHelper::EnsureFocus(button, FocusState::Pointer);
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Dcomp dump to verify the bitmap paste.");
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
            TestServices::WindowHelper->WaitForIdle();
        }

        void RichEditBoxTOMTests::PasteAndSelectImage()
        {
            KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride;
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::Button^ button;
            xaml_controls::RichEditBox^ rebxPaste;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(LR"(
                    <StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >
                      <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />
                      <RichEditBox x:Name = 'rebxPaste' Width='200' Height='250' Margin = '20,5,20,0' />
                    </StackPanel>
                )"));

                rebxPaste = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebxPaste"));
                VERIFY_IS_NOT_NULL(rebxPaste);
                button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
                VERIFY_IS_NOT_NULL(button);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            auto clipboardBitmapSet = std::make_shared<Event>();
            create_task(StorageFile::GetFileFromPathAsync(GetResourcesPath() + L"Smiley.bmp"))
                .then([=](StorageFile^ pFile)
            {
                VERIFY_IS_NOT_NULL(pFile);
                auto imgStreamRef = RandomAccessStreamReference::CreateFromFile(pFile);
                auto dataPackage = ref new DataPackage();
                dataPackage->SetBitmap(imgStreamRef);

                RunOnUIThread([=]()
                {
                    DataTransfer::Clipboard::SetContent(dataPackage);
                    clipboardBitmapSet->Set();
                });
            });

            clipboardBitmapSet->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            // putting focus on the RichEditBox and do a Ctrl-V to paste the Bitmap from Clipboard
            FocusTestHelper::EnsureFocus(rebxPaste, FocusState::Keyboard);
            TestServices::WindowHelper->WaitForIdle();
            auto pasteTextChangedEvent = std::make_shared<Event>();
            auto pasteTextChangedRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, TextChanged);
            RunOnUIThread([&]()
            {
                pasteTextChangedRegistration.Attach(
                    rebxPaste,
                    [pasteTextChangedEvent]()
                {
                    LOG_OUTPUT(L"Paste RichEditBox TextChanged handler.");
                    pasteTextChangedEvent->Set();
                });
            });

            RunOnUIThread([&]()
            {
                rebxPaste->Document->SetText(Microsoft::UI::Text::TextSetOptions::None, L"hello world");
            });
            pasteTextChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            auto selectionChangedEvent = std::make_shared<Event>();
            auto richEditBoxSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, SelectionChanged);
            RunOnUIThread([&]()
            {
                richEditBoxSelectionChangedRegistration.Attach(
                    rebxPaste,
                    ref new xaml::RoutedEventHandler(
                        [selectionChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"RichEditBox SelectionChanged handler.");
                    selectionChangedEvent->Set();
                }));
            });

            RunOnUIThread([&]()
            {
                rebxPaste->Document->Selection->StartPosition = 1;
                rebxPaste->Document->Selection->EndPosition = 4;
            });
            selectionChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Testing bitmap paste and replace the current selection.");
            pasteTextChangedEvent->Reset();
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_v#$u$_ctrlscan#$u$_v");
            pasteTextChangedEvent->WaitForDefault();
            selectionChangedEvent->WaitForDefault();

            LOG_OUTPUT(L"Select all using Ctrl-A, testing inverted bitmap image.");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_a#$u$_ctrlscan#$u$_a");
            selectionChangedEvent->WaitForDefault();

            LOG_OUTPUT(L"Dcomp dump to verify the bitmap paste and selection.");
            TestServices::WindowHelper->SynchronouslyTickUIThread(5);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }

        void RichEditBoxTOMTests::RichEditBoxMaxLength()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto richEditBoxTextChangedEvent = std::make_shared<Event>();
            auto richEditBoxTextChangedRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, TextChanged);

            xaml_controls::RichEditBox^ richEditBox = nullptr;
            xaml_controls::Button^ button = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <RichEditBox x:Name='richEditBox' IsSpellCheckEnabled='False' FontSize='20' Width='200' Height='80' Margin ='20,5,20,0' AcceptsReturn='True' TextWrapping='Wrap' MaxLength='0'>"
                    L"  </RichEditBox>"
                    L"</StackPanel>"));

                richEditBox = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"richEditBox"));
                TestServices::WindowHelper->WindowContent = rootPanel;

                richEditBoxTextChangedRegistration.Attach(
                    richEditBox,
                    [richEditBoxTextChangedEvent]()
                {
                    LOG_OUTPUT(L"RichEditBox control TextChanged handler.");
                    richEditBoxTextChangedEvent->Set();
                });
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Setting focus to the RichEditBox control.");
            FocusTestHelper::EnsureFocus(richEditBox, FocusState::Keyboard);
            TestServices::WindowHelper->WaitForIdle();
            FocusMonitor focusMonitor(richEditBox);

            Platform::String ^strToType = "ABCDEFGH";
            Platform::String ^strToCompareFirstTwoChar = "AB";
            TestServices::KeyboardHelper->PressKeySequence(strToType);
            TestServices::WindowHelper->WaitForIdle();
            richEditBoxTextChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                Platform::String^ text;
                richEditBox->Document->GetText(mut::TextGetOptions::AdjustCrlf, &text);
                LOG_OUTPUT(L"TextInput:%s", text->Data());
                VERIFY_IS_TRUE(text == strToType);
            });
            TestServices::WindowHelper->WaitForIdle();

            // change textbox's MaxLength to 2
            RunOnUIThread([&]()
            {
                richEditBox->Document->SetText(mut::TextSetOptions::None, L"");
                richEditBox->MaxLength = 2;
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->PressKeySequence(strToType);
            TestServices::WindowHelper->WaitForIdle();
            richEditBoxTextChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                Platform::String^ text;
                richEditBox->Document->GetText(mut::TextGetOptions::AdjustCrlf, &text);
                LOG_OUTPUT(L"TextInput:%s", text->Data());
                VERIFY_IS_TRUE(text == strToCompareFirstTwoChar);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Change MaxLength to 3 and input AB then press enter.");
                richEditBox->Document->SetText(mut::TextSetOptions::None, L"");
                richEditBox->MaxLength = 3;
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->PressKeySequence(L"ab");
            TestServices::KeyboardHelper->Enter();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                Platform::String^ text;
                richEditBox->Document->GetText(mut::TextGetOptions::AdjustCrlf, &text);
                LOG_OUTPUT(L"TextInput:%s", text->Data());
                VERIFY_ARE_EQUAL(wcslen(text->Data()), static_cast<size_t>(3));
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Change MaxLength to 4 and try to set Text to AB\\r\\n, richedit will combine \\r\\n to \\r.");
                richEditBox->MaxLength = 4;
                richEditBox->Document->SetText(mut::TextSetOptions::None, L"AB\r\n");
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                Platform::String^ text;
                richEditBox->Document->GetText(mut::TextGetOptions::AdjustCrlf, &text);
                LOG_OUTPUT(L"Current TextBox.Text:%s", text->Data());
                VERIFY_ARE_EQUAL(wcslen(text->Data()), static_cast<size_t>(3));
            });

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Set Text to A\\rB\\n, richedit will change it to A\\rB\\r.");
                richEditBox->MaxLength = 4;
                richEditBox->Document->SetText(mut::TextSetOptions::None, L"A\rB\n");
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                Platform::String^ text;
                richEditBox->Document->GetText(mut::TextGetOptions::AdjustCrlf, &text);
                LOG_OUTPUT(L"Current TextBox.Text:%s", text->Data());
                VERIFY_ARE_EQUAL(wcslen(text->Data()), static_cast<size_t>(4));
            });

            RunOnUIThread([&]()
            {
                richEditBox->IsEnabled = false;
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void RichEditBoxTOMTests::SetTextAdheresToMaxLength()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto richEditBoxTextChangedEvent = std::make_shared<Event>();
            auto richEditBoxTextChangedRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, TextChanged);

            xaml_controls::RichEditBox^ richEditBox = nullptr;
            xaml_controls::Button^ button = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <RichEditBox x:Name='richEditBox' IsSpellCheckEnabled='False' FontSize='20' Width='200' Height='80' Margin ='20,5,20,0' AcceptsReturn='True' TextWrapping='Wrap' MaxLength='2'>"
                    L"  </RichEditBox>"
                    L"</StackPanel>"));

                richEditBox = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"richEditBox"));
                TestServices::WindowHelper->WindowContent = rootPanel;

                richEditBoxTextChangedRegistration.Attach(
                    richEditBox,
                    [richEditBoxTextChangedEvent]()
                {
                    LOG_OUTPUT(L"RichEditBox control TextChanged handler.");
                    richEditBoxTextChangedEvent->Set();
                });
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Setting focus to the RichEditBox control.");
            FocusTestHelper::EnsureFocus(richEditBox, FocusState::Keyboard);
            TestServices::WindowHelper->WaitForIdle();
            FocusMonitor focusMonitor(richEditBox);

            RunOnUIThread([&]()
            {
                richEditBox->Document->SetText(Microsoft::UI::Text::TextSetOptions::None, L"hello world");
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                Platform::String^ text;
                richEditBox->Document->GetText(mut::TextGetOptions::AdjustCrlf, &text);
                VERIFY_ARE_EQUAL(wcslen(text->Data()), static_cast<size_t>(2));
            });
        }

        void RichEditBoxTOMTests::PasteImageAdheresToMaxLength()
        {
            KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride;
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::Button^ button;
            xaml_controls::RichEditBox^ rebxPaste;

            auto richeditBoxPasteEvent = std::make_shared<Event>();
            auto richeditBoxPasteRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, Paste);

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(LR"(
                    <StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >
                      <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />
                      <RichEditBox x:Name = 'rebxPaste' MaxLength='1' Width='200' Height='250' Margin = '20,5,20,0' />
                    </StackPanel>
                )"));

                rebxPaste = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebxPaste"));
                VERIFY_IS_NOT_NULL(rebxPaste);
                button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
                VERIFY_IS_NOT_NULL(button);
                TestServices::WindowHelper->WindowContent = rootPanel;

                richeditBoxPasteRegistration.Attach(
                    rebxPaste,
                    ref new xaml_controls::TextControlPasteEventHandler(
                        [&](Platform::Object^, xaml_controls::TextControlPasteEventArgs^ e)
                {
                    richeditBoxPasteEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();

            auto clipboardBitmapSet = std::make_shared<Event>();
            create_task(StorageFile::GetFileFromPathAsync(GetResourcesPath() + L"Smiley.bmp"))
                .then([=](StorageFile^ pFile)
            {
                VERIFY_IS_NOT_NULL(pFile);
                auto imgStreamRef = RandomAccessStreamReference::CreateFromFile(pFile);
                auto dataPackage = ref new DataPackage();
                dataPackage->SetBitmap(imgStreamRef);

                RunOnUIThread([=]()
                {
                    DataTransfer::Clipboard::SetContent(dataPackage);
                    clipboardBitmapSet->Set();
                });
            });

            clipboardBitmapSet->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            // putting focus on the RichEditBox and do a Ctrl-V to paste the Bitmap from Clipboard
            FocusTestHelper::EnsureFocus(rebxPaste, FocusState::Keyboard);
            TestServices::WindowHelper->WaitForIdle();
            auto pasteTextChangedEvent = std::make_shared<Event>();
            auto pasteTextChangedRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, TextChanged);
            RunOnUIThread([&]()
            {
                pasteTextChangedRegistration.Attach(
                    rebxPaste,
                    [pasteTextChangedEvent]()
                {
                    LOG_OUTPUT(L"Paste RichEditBox TextChanged handler.");
                    pasteTextChangedEvent->Set();
                });
            });

            LOG_OUTPUT(L"Testing bitmap paste when paste event is handled.");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_v#$u$_ctrlscan#$u$_v");
            // verify paste event fired
            richeditBoxPasteEvent->WaitForDefault();
            pasteTextChangedEvent->WaitForDefault();

            richeditBoxPasteEvent->Reset();
            pasteTextChangedEvent->Reset();

            LOG_OUTPUT(L"The second paste should not work");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_v#$u$_ctrlscan#$u$_v");

            VERIFY_IS_TRUE(richeditBoxPasteEvent->HasFired());
            pasteTextChangedEvent->WaitForNoThrow(std::chrono::milliseconds(100));
            VERIFY_IS_FALSE(pasteTextChangedEvent->HasFired());

            // putting focus on button to bring down SIP
            FocusTestHelper::EnsureFocus(button, FocusState::Pointer);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                Platform::String^ text;
                rebxPaste->Document->GetText(mut::TextGetOptions::AdjustCrlf, &text);
                VERIFY_ARE_EQUAL(wcslen(text->Data()), static_cast<size_t>(1));
            });

            LOG_OUTPUT(L"Dcomp dump to verify the bitmap paste.");
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
            TestServices::WindowHelper->WaitForIdle();
        }

        void RichEditBoxTOMTests::PasteTextAdheresToMaxLength()
        {
            KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride;

            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);
            Platform::String ^toReplaceWith = L"Hello, World";

            auto re2TextChangedEvent = std::make_shared<Event>();
            auto re2TextChangedRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, TextChanged);

            xaml_controls::Grid^ rootGrid = nullptr;
            xaml_controls::TextBox^ tb1 = nullptr;
            xaml_controls::RichEditBox^ re2 = nullptr;
            xaml_controls::StackPanel^ stackPanel = nullptr;

            RunOnUIThread([&]()
            {
                rootGrid = ref new xaml_controls::Grid;
                rootGrid->Width = 400;
                TestServices::WindowHelper->WindowContent = rootGrid;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                stackPanel = ref new xaml_controls::StackPanel();

                tb1 = ref new xaml_controls::TextBox();
                tb1->Text = toReplaceWith;

                re2 = ref new xaml_controls::RichEditBox();
                re2->MaxLength = 2;
                stackPanel->Children->Append(tb1);
                stackPanel->Children->Append(re2);
                rootGrid->Children->Append(stackPanel);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                re2TextChangedRegistration.Attach(
                    re2, [re2TextChangedEvent]()
                {
                    LOG_OUTPUT(L"re2 TextChanged handler.");
                    re2TextChangedEvent->Set();
                });
            });
            TestServices::WindowHelper->WaitForIdle();

            FocusTestHelper::EnsureFocus(tb1, FocusState::Pointer);

            RunOnUIThread([&]()
            {
                tb1->SelectAll();
            });

            TestServices::WindowHelper->WaitForIdle();

            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_c#$u$_c#$u$_ctrlscan"); // Ctrl+C
            TestServices::WindowHelper->WaitForIdle();

            FocusTestHelper::EnsureFocus(re2, FocusState::Pointer);

            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_v#$u$_v#$u$_ctrlscan"); // Ctrl+V
            TestServices::WindowHelper->WaitForIdle();
            re2TextChangedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                Platform::String^ text;
                re2->Document->GetText(mut::TextGetOptions::AdjustCrlf, &text);
                VERIFY_ARE_EQUAL(wcslen(text->Data()), static_cast<size_t>(2));
            });
        }

        void RichEditBoxTOMTests::AlignmentIncludesTrailingWhitespace()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::RichEditBox^ rebx = nullptr;
            xaml_controls::Button^ button = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <RichEditBox x:Name = 'rebx' Width='360' Height='200' Margin = '20,40,20,0' />"
                    L"</StackPanel>"));
                rebx = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx"));
                VERIFY_IS_NOT_NULL(rebx);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                RichEditTextDocument^ document = rebx->TextDocument;
                LOG_OUTPUT(L"Default value for AlignmentIncludesTrailingWhitespace should be false.");
                VERIFY_IS_FALSE(document->AlignmentIncludesTrailingWhitespace);
                LOG_OUTPUT(L"Set AlignmentIncludesTrailingWhitespace to true.");
                document->AlignmentIncludesTrailingWhitespace = TRUE;
                rebx->Document->SetText(Microsoft::UI::Text::TextSetOptions::None,
                    L"String with trailing spaces                           \r"
                    L"                            String with leading spaces\r"
                    L"String with spaces                       in the middle\r"
                );
                rebx->TextAlignment = TextAlignment::Left;
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"AlignmentIncludesTrailingWhitespace:TRUE, TextAlignment::Left");
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"SpaceTrueLeft");

            RunOnUIThread([&]()
            {
                rebx->TextAlignment = TextAlignment::Center;
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"AlignmentIncludesTrailingWhitespace:TRUE, TextAlignment::Center");
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"SpaceTrueCenter");

            RunOnUIThread([&]()
            {
                rebx->TextAlignment = TextAlignment::Right;
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"AlignmentIncludesTrailingWhitespace:TRUE, TextAlignment::Right");
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"SpaceTrueRight");

            RunOnUIThread([&]()
            {
                RichEditTextDocument^ document = rebx->TextDocument;
                document->AlignmentIncludesTrailingWhitespace = FALSE;
                rebx->TextAlignment = TextAlignment::Left;
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"AlignmentIncludesTrailingWhitespace:FALSE, TextAlignment::Left");
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"SpaceFlaseLeft");

            RunOnUIThread([&]()
            {
                rebx->TextAlignment = TextAlignment::Center;
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"AlignmentIncludesTrailingWhitespace:FALSE, TextAlignment::Center");
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"SpaceFlaseCenter");

            RunOnUIThread([&]()
            {
                rebx->TextAlignment = TextAlignment::Right;
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"AlignmentIncludesTrailingWhitespace:FALSE, TextAlignment::Right");
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"SpaceFlaseRight");
        }

        void RichEditBoxTOMTests::UseLfAndUseCrLf()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::RichEditBox^ rebx = nullptr;
            xaml_controls::Button^ button = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <RichEditBox x:Name = 'rebx' Width='360' Height='200' Margin = '20,40,20,0' />"
                    L"</StackPanel>"));
                rebx = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx"));
                VERIFY_IS_NOT_NULL(rebx);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Setting Text with \r and \n.");
            Platform::String^ stringSet = L"Text";
            Platform::String^ stringExpectedUseLf = L"Text\n";
            Platform::String^ stringExpectedUseCrlf = L"Text\r\n";
            Platform::String^ stringExpectedUseDefault = L"Text\r";

            RunOnUIThread([&]()
            {
                rebx->Document->SetText(Microsoft::UI::Text::TextSetOptions::None, stringSet);
            });

            TestServices::WindowHelper->WaitForIdle();

            Platform::String ^text;
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Getting Text with TextGetOptions::None.");
                rebx->Document->GetText(Microsoft::UI::Text::TextGetOptions::None, &text);
            });
            TestServices::WindowHelper->WaitForIdle();
            VERIFY_ARE_EQUAL(text, stringExpectedUseDefault);

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Getting Text with TextGetOptions::UseCrlf.");
                rebx->Document->GetText(Microsoft::UI::Text::TextGetOptions::UseCrlf | Microsoft::UI::Text::TextGetOptions::AllowFinalEop, &text);
            });
            TestServices::WindowHelper->WaitForIdle();
            VERIFY_ARE_EQUAL(text, stringExpectedUseCrlf);

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Getting Text with TextGetOptions::UseLf.");
                rebx->Document->GetText(Microsoft::UI::Text::TextGetOptions::UseLf | Microsoft::UI::Text::TextGetOptions::AllowFinalEop, &text);
            });
            TestServices::WindowHelper->WaitForIdle();
            VERIFY_ARE_EQUAL(text, stringExpectedUseLf);

            RunOnUIThread([&]()
            {
                VERIFY_THROWS_WINRT(
                    (rebx->Document->GetText(Microsoft::UI::Text::TextGetOptions::UseLf | Microsoft::UI::Text::TextGetOptions::UseCrlf, &text)),
                    Platform::InvalidArgumentException^,
                    L"Exception thrown by invalid TextGetOptions.");
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void RichEditBoxTOMTests::PrepRichEditBox(
            xaml_controls::RichEditBox^ rebx,
            bool ignoreTrailingCharacterSpacing,
            Xaml::TextAlignment alignment,
            bool underline,
            bool strikethrough,
            bool partial,
            bool multiline)
        {
            RunOnUIThread([&]()
            {
                RichEditTextDocument^ document = rebx->TextDocument;
                LOG_OUTPUT(L"Default value for IgnoreTrailingCharacterSpacing should be false.");
                VERIFY_IS_FALSE(document->IgnoreTrailingCharacterSpacing);
                document->IgnoreTrailingCharacterSpacing = ignoreTrailingCharacterSpacing;

                ITextCharacterFormat^ characterFormat = document->GetDefaultCharacterFormat();
                characterFormat->Spacing = 20;

                if (underline)
                {
                    characterFormat->Underline = Microsoft::UI::Text::UnderlineType::Single;
                }

                if (strikethrough)
                {
                    characterFormat->Strikethrough = Microsoft::UI::Text::FormatEffect::On;
                }

                document->SetDefaultCharacterFormat(characterFormat);

                rebx->TextAlignment = alignment;

                if (!multiline)
                {
                    rebx->Document->SetText(Microsoft::UI::Text::TextSetOptions::None, L"AAAA");
                }
                else
                {
                    rebx->Document->SetText(Microsoft::UI::Text::TextSetOptions::None, L"AAAA\rbbbb");
                }

                if (partial)
                {
                    Microsoft::UI::Text::ITextRange^ Range = rebx->Document->GetRange(2, 4);
                    Range->CharacterFormat->Underline = Microsoft::UI::Text::UnderlineType::None;
                    Range->CharacterFormat->Strikethrough = Microsoft::UI::Text::FormatEffect::Off;
                }
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void RichEditBoxTOMTests::IgnoreTrailingCharacterSpacing()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 800);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::RichEditBox^ rebx1 = nullptr;
            xaml_controls::RichEditBox^ rebx2 = nullptr;
            xaml_controls::RichEditBox^ rebx3 = nullptr;
            xaml_controls::RichEditBox^ rebx4 = nullptr;
            xaml_controls::RichEditBox^ rebx5 = nullptr;
            xaml_controls::RichEditBox^ rebx6 = nullptr;
            xaml_controls::RichEditBox^ rebx7 = nullptr;
            xaml_controls::RichEditBox^ rebx8 = nullptr;
            xaml_controls::RichEditBox^ rebx9 = nullptr;
            xaml_controls::RichEditBox^ rebx10 = nullptr;
            xaml_controls::RichEditBox^ rebx11 = nullptr;
            xaml_controls::RichEditBox^ rebx12 = nullptr;
            xaml_controls::RichEditBox^ rebx13 = nullptr;
            xaml_controls::RichEditBox^ rebx14 = nullptr;
            xaml_controls::RichEditBox^ rebx15 = nullptr;
            xaml_controls::RichEditBox^ rebx16 = nullptr;
            xaml_controls::RichEditBox^ rebx17 = nullptr;
            xaml_controls::RichEditBox^ rebx18 = nullptr;
            xaml_controls::RichEditBox^ rebx19 = nullptr;
            xaml_controls::Button^ button = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='800' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,0,20,0' />"
                    L"  <RichEditBox x:Name = 'rebx1' Height='20' MinWidth='40' HorizontalAlignment='Left' TextWrapping='NoWrap' />"
                    L"  <RichEditBox x:Name = 'rebx2' Height='20' MinWidth='40' HorizontalAlignment='Left' TextWrapping='NoWrap' />"
                    L"  <RichEditBox x:Name = 'rebx3' Height='20' MinWidth='40' HorizontalAlignment='Left' TextWrapping='NoWrap' />"
                    L"  <RichEditBox x:Name = 'rebx4' Height='20' MinWidth='40' HorizontalAlignment='Left' TextWrapping='NoWrap' />"
                    L"  <RichEditBox x:Name = 'rebx5' Height='20' MinWidth='40' HorizontalAlignment='Left' TextWrapping='NoWrap' />"
                    L"  <RichEditBox x:Name = 'rebx6' Height='20' MinWidth='40' HorizontalAlignment='Left' TextWrapping='NoWrap' />"
                    L"  <RichEditBox x:Name = 'rebx7' Height='20' MinWidth='40' HorizontalAlignment='Left' TextWrapping='NoWrap' />"
                    L"  <RichEditBox x:Name = 'rebx8' Height='20' MinWidth='40' HorizontalAlignment='Left' TextWrapping='NoWrap' />"
                    L"  <RichEditBox x:Name = 'rebx9' Height='20' MinWidth='40' HorizontalAlignment='Left' TextWrapping='NoWrap' />"
                    L"  <RichEditBox x:Name = 'rebx10' Height='20' MinWidth='40' HorizontalAlignment='Left' TextWrapping='NoWrap' />"
                    L"  <RichEditBox x:Name = 'rebx11' Height='20' MinWidth='40' HorizontalAlignment='Left' TextWrapping='NoWrap' />"
                    L"  <RichEditBox x:Name = 'rebx12' Height='20' MinWidth='40' HorizontalAlignment='Left' TextWrapping='NoWrap' />"
                    L"  <RichEditBox x:Name = 'rebx13' Height='20' MinWidth='40' HorizontalAlignment='Left' TextWrapping='NoWrap' />"
                    L"  <RichEditBox x:Name = 'rebx14' Height='20' MinWidth='40' HorizontalAlignment='Left' TextWrapping='NoWrap' />"
                    L"  <RichEditBox x:Name = 'rebx15' Height='20' MinWidth='40' HorizontalAlignment='Left' TextWrapping='NoWrap' />"
                    L"  <RichEditBox x:Name = 'rebx16' Height='20' MinWidth='40' HorizontalAlignment='Left' TextWrapping='NoWrap' />"
                    L"  <RichEditBox x:Name = 'rebx17' Height='20' MinWidth='40' HorizontalAlignment='Left' TextWrapping='NoWrap' />"
                    L"  <RichEditBox x:Name = 'rebx18' Height='20' MinWidth='40' HorizontalAlignment='Left' TextWrapping='NoWrap' />"
                    L"  <RichEditBox x:Name = 'rebx19' Height='50' MinWidth='40' HorizontalAlignment='Left' TextWrapping='NoWrap' />"
                    L"</StackPanel>"));
                rebx1 = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx1"));
                rebx2 = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx2"));
                rebx3 = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx3"));
                rebx4 = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx4"));
                rebx5 = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx5"));
                rebx6 = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx6"));
                rebx7 = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx7"));
                rebx8 = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx8"));
                rebx9 = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx9"));
                rebx10 = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx10"));
                rebx11 = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx11"));
                rebx12 = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx12"));
                rebx13 = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx13"));
                rebx14 = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx14"));
                rebx15 = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx15"));
                rebx16 = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx16"));
                rebx17 = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx17"));
                rebx18 = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx18"));
                rebx19 = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx19"));
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();
            PrepRichEditBox(rebx1, false /*IgnoreTrailingCharacterSpacing*/, Xaml::TextAlignment::Left, false /*underline*/, false /*strikethrough*/, false /*partial*/, false /*multiline*/);
            PrepRichEditBox(rebx2, false /*IgnoreTrailingCharacterSpacing*/, Xaml::TextAlignment::Center, false /*underline*/, false /*strikethrough*/, false /*partial*/, false /*multiline*/);
            PrepRichEditBox(rebx3, false /*IgnoreTrailingCharacterSpacing*/, Xaml::TextAlignment::Right, false /*underline*/, false /*strikethrough*/, false /*partial*/, false /*multiline*/);
            PrepRichEditBox(rebx4, true /*IgnoreTrailingCharacterSpacing*/, Xaml::TextAlignment::Left, false /*underline*/, false /*strikethrough*/, false /*partial*/, false /*multiline*/);
            PrepRichEditBox(rebx5, true /*IgnoreTrailingCharacterSpacing*/, Xaml::TextAlignment::Center, false /*underline*/, false /*strikethrough*/, false /*partial*/, false /*multiline*/);
            PrepRichEditBox(rebx6, true /*IgnoreTrailingCharacterSpacing*/, Xaml::TextAlignment::Right, false /*underline*/, false /*strikethrough*/, false /*partial*/, false /*multiline*/);
            PrepRichEditBox(rebx7, false /*IgnoreTrailingCharacterSpacing*/, Xaml::TextAlignment::Left, true /*underline*/, false /*strikethrough*/, false /*partial*/, false /*multiline*/);
            PrepRichEditBox(rebx8, false /*IgnoreTrailingCharacterSpacing*/, Xaml::TextAlignment::Center, true /*underline*/, false /*strikethrough*/, false /*partial*/, false /*multiline*/);
            PrepRichEditBox(rebx9, false /*IgnoreTrailingCharacterSpacing*/, Xaml::TextAlignment::Right, true /*underline*/, false /*strikethrough*/, false /*partial*/, false /*multiline*/);
            PrepRichEditBox(rebx10, true /*IgnoreTrailingCharacterSpacing*/, Xaml::TextAlignment::Left, true /*underline*/, false /*strikethrough*/, false /*partial*/, false /*multiline*/);
            PrepRichEditBox(rebx11, true /*IgnoreTrailingCharacterSpacing*/, Xaml::TextAlignment::Center, true /*underline*/, false /*strikethrough*/, false /*partial*/, false /*multiline*/);
            PrepRichEditBox(rebx12, true /*IgnoreTrailingCharacterSpacing*/, Xaml::TextAlignment::Right, true /*underline*/, false /*strikethrough*/, false /*partial*/, false /*multiline*/);
            PrepRichEditBox(rebx13, false /*IgnoreTrailingCharacterSpacing*/, Xaml::TextAlignment::Right, false /*underline*/, true /*strikethrough*/, false /*partial*/, false /*multiline*/);
            PrepRichEditBox(rebx14, true /*IgnoreTrailingCharacterSpacing*/, Xaml::TextAlignment::Left, false /*underline*/, true /*strikethrough*/, false /*partial*/, false /*multiline*/);
            PrepRichEditBox(rebx15, true /*IgnoreTrailingCharacterSpacing*/, Xaml::TextAlignment::Center, false /*underline*/, true /*strikethrough*/, false /*partial*/, false /*multiline*/);
            PrepRichEditBox(rebx16, true /*IgnoreTrailingCharacterSpacing*/, Xaml::TextAlignment::Right, false /*underline*/, true /*strikethrough*/, false /*partial*/, false /*multiline*/);
            PrepRichEditBox(rebx17, true /*IgnoreTrailingCharacterSpacing*/, Xaml::TextAlignment::Right, true /*underline*/, true /*strikethrough*/, false /*partial*/, false /*multiline*/);
            PrepRichEditBox(rebx18, true /*IgnoreTrailingCharacterSpacing*/, Xaml::TextAlignment::Right, true /*underline*/, true /*strikethrough*/, true /*partial*/, false /*multiline*/);
            PrepRichEditBox(rebx19, true /*IgnoreTrailingCharacterSpacing*/, Xaml::TextAlignment::Right, false /*underline*/, false /*strikethrough*/, false /*partial*/, true /*multiline*/);

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }


        void RichEditBoxTOMTests::FormatFontWeight()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);
            xaml_controls::RichEditBox^ rebx = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <RichEditBox x:Name = 'rebx' Width='350' Height='25' Margin = '20,40,20,0' IsSpellCheckEnabled='False' />"
                    L"</StackPanel>"));
                rebx = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx"));
                VERIFY_IS_NOT_NULL(rebx);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto cf = rebx->Document->GetDefaultCharacterFormat();
                LOG_OUTPUT(L"Set and get to verify all possible Weights.");
                for (int WeightIn = 0; WeightIn <= 950; WeightIn += 50)
                {
                    cf->Weight = WeightIn;
                    int WeightOut = cf->Weight;
                    VERIFY_ARE_EQUAL(WeightIn, WeightOut);
                }
                rebx->Document->SetDefaultCharacterFormat(cf);
                rebx->Document->SetText(Microsoft::UI::Text::TextSetOptions::None, "ABCD");

                LOG_OUTPUT(L"Update part of the Text to different font weight.");
                Microsoft::UI::Text::ITextRange^ Range = rebx->Document->GetRange(2, 4);
                Range->CharacterFormat->Weight = 900;
            });
            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Dcomp dump to verify font weight");
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
            TestServices::WindowHelper->WaitForIdle();
        }

        void RichEditBoxTOMTests::StartEndHorizontalTextAlignment()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::RichEditBox^ richEditBox = nullptr;
            xaml_controls::Panel^ root = safe_cast<xaml_controls::Panel^>(LoadXamlFileOnUIThread(
                    GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\rendering\\TextBoxTests.xaml"));

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = root;

                richEditBox = safe_cast<xaml_controls::RichEditBox^>(root->FindName(L"myRichEditBox"));
                VERIFY_IS_NOT_NULL(richEditBox);

                // HorizontalTextAlignment::Start should be the same as TextAlignment::Left
                richEditBox->HorizontalTextAlignment = TextAlignment::Start;
                VERIFY_IS_TRUE(richEditBox->TextAlignment == TextAlignment::Left);

                // HorizontalTextAlignment::End should be the same as TextAlignment::Right
                richEditBox->HorizontalTextAlignment = TextAlignment::End;
                VERIFY_IS_TRUE(richEditBox->TextAlignment == TextAlignment::Right);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void RichEditBoxTOMTests::VerifyProgrammaticSelectionCutRaisesRichEditBoxEvent()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::RichEditBox^ rebx = nullptr;

            auto cutEvent = std::make_shared<Event>();
            auto copyEvent = std::make_shared<Event>();
            auto cutEventRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, CuttingToClipboard);
            auto copyEventRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, CopyingToClipboard);

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel x:Name = 'stack' Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <RichEditBox x:Name = 'rebx' Width='400' Height='300' Margin = '20,40,20,0' SelectionHighlightColor='Red' />"
                    L"</StackPanel>"));
                rebx = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx"));
                VERIFY_IS_NOT_NULL(rebx);

                cutEventRegistration.Attach(rebx, [cutEvent]()
                {
                    LOG_OUTPUT(L"RichEditBox Cut event called.");
                    cutEvent->Set();
                });

                copyEventRegistration.Attach(rebx, [copyEvent]()
                {
                    LOG_OUTPUT(L"RichEditBox Copy event called.");
                    copyEvent->Set();
                });

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                rebx->Document->SetText(Microsoft::UI::Text::TextSetOptions::None, "text to cut");
                rebx->Document->Selection->Expand(Microsoft::UI::Text::TextRangeUnit::Story);

                LOG_OUTPUT(L"Cut the selected text programmatically. The Cut event should be raised.");
                rebx->Document->Selection->Cut();
            });

            cutEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"The Copy event should not be raised, even though internally Cut uses the same functionality as Copy.");
            VERIFY_IS_FALSE(copyEvent->HasFired());

            cutEvent->Reset();

            RunOnUIThread([&]()
            {
                rebx->Document->SetText(Microsoft::UI::Text::TextSetOptions::None, "text to cut");
                auto range = rebx->Document->GetRange(0, 5);

                LOG_OUTPUT(L"Cut non-selected text programmatically. The Cut event should not be raised.");
                range->Cut();
            });

            TestServices::WindowHelper->WaitForIdle();

            VERIFY_IS_FALSE(cutEvent->HasFired());
        }

        void RichEditBoxTOMTests::VerifyProgrammaticSelectionCopyRaisesRichEditBoxEvent()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::RichEditBox^ rebx = nullptr;

            auto cutEvent = std::make_shared<Event>();
            auto copyEvent = std::make_shared<Event>();
            auto cutEventRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, CuttingToClipboard);
            auto copyEventRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, CopyingToClipboard);

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel x:Name = 'stack' Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <RichEditBox x:Name = 'rebx' Width='400' Height='300' Margin = '20,40,20,0' SelectionHighlightColor='Red' />"
                    L"</StackPanel>"));
                rebx = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx"));
                VERIFY_IS_NOT_NULL(rebx);

                cutEventRegistration.Attach(rebx, [cutEvent]()
                {
                    LOG_OUTPUT(L"RichEditBox Cut event called.");
                    cutEvent->Set();
                });

                copyEventRegistration.Attach(rebx, [copyEvent]()
                {
                    LOG_OUTPUT(L"RichEditBox Copy event called.");
                    copyEvent->Set();
                });

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                rebx->Document->SetText(Microsoft::UI::Text::TextSetOptions::None, "text to copy");
                rebx->Document->Selection->Expand(Microsoft::UI::Text::TextRangeUnit::Story);

                LOG_OUTPUT(L"Copy the selected text programmatically. The Copy event should be raised.");
                rebx->Document->Selection->Copy();
            });

            copyEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"The Cut event should not be raised, even though internally Copy uses the same functionality as Cut.");
            VERIFY_IS_FALSE(cutEvent->HasFired());

            copyEvent->Reset();

            RunOnUIThread([&]()
            {
                rebx->Document->SetText(Microsoft::UI::Text::TextSetOptions::None, "text to copy");
                auto range = rebx->Document->GetRange(0, 5);

                LOG_OUTPUT(L"Copy non-selected text programmatically. The Copy event should not be raised.");
                range->Copy();
            });

            TestServices::WindowHelper->WaitForIdle();

            VERIFY_IS_FALSE(copyEvent->HasFired());
        }

        void RichEditBoxTOMTests::VerifyProgrammaticSelectionPasteRaisesRichEditBoxEvent()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::RichEditBox^ rebx = nullptr;

            auto pasteEvent = std::make_shared<Event>();
            auto pasteEventRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, Paste);

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel x:Name = 'stack' Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <RichEditBox x:Name = 'rebx' Width='400' Height='300' Margin = '20,40,20,0' SelectionHighlightColor='Red' />"
                    L"</StackPanel>"));
                rebx = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx"));
                VERIFY_IS_NOT_NULL(rebx);

                pasteEventRegistration.Attach(rebx, [pasteEvent]()
                {
                    LOG_OUTPUT(L"RichEditBox Paste event called.");
                    pasteEvent->Set();
                });

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Initialize the clipboard with text so we'll have something to paste.");
                auto dataPackage = ref new DataPackage();
                dataPackage->SetText("text to paste");

                DataTransfer::Clipboard::SetContent(dataPackage);

                LOG_OUTPUT(L"Paste text to the selection programmatically. The Paste event should be raised.");
                rebx->Document->Selection->Paste(0);
            });

            pasteEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            pasteEvent->Reset();

            RunOnUIThread([&]()
            {
                auto range = rebx->Document->GetRange(0, 0);

                LOG_OUTPUT(L"Paste to somewhere other than the selection. The Paste event should not be raised.");
                range->Paste(0);
            });

            TestServices::WindowHelper->WaitForIdle();

            VERIFY_IS_FALSE(pasteEvent->HasFired());
        }
    } }
} } } }
