// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TextBoxSelectionTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <ppltasks.h>
#include "FileLoader.h"
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <TreeHelper.h>
#include <WUCRenderingScopeGuard.h>
#include <FocusTestHelper.h>
#include "RuntimeEnabledFeatureOverride.h"
#include <ClipboardHelper.h>
#include <TreeHelper.h>
#include <TestComparisonGuards.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace MockDComp;
using namespace Microsoft::UI::Xaml::Input;
using namespace ::Windows::UI::ViewManagement;
using namespace ::Windows::Foundation;
using namespace ::Windows::Storage;
using namespace ::Windows::Storage::Streams;
using namespace ::Windows::ApplicationModel;
using namespace ::Windows::ApplicationModel::DataTransfer;
using namespace concurrency;
using namespace Microsoft::UI::Text;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation {
        namespace Text {

            bool TextBoxSelectionTests::ClassSetup()
            {
                CommonTestSetupHelper::CommonTestClassSetup();
                return true;
            }

            bool TextBoxSelectionTests::ClassCleanup()
            {
                return true;
            }

            bool TextBoxSelectionTests::TestSetup()
            {
                test_infra::TestServices::WindowHelper->InitializeXaml();
                return true;
            }

            bool TextBoxSelectionTests::TestCleanup()
            {
                test_infra::TestServices::WindowHelper->ShutdownXaml();
                TestServices::WindowHelper->VerifyTestCleanup();
                return true;
            }

            void TextBoxSelectionTests::TextBoxSelection()
            {
                WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

                ::Windows::Foundation::Size size(400, 400);
                TestServices::WindowHelper->SetWindowSizeOverride(size);

                xaml_controls::TextBox^ textBox = nullptr;
                xaml_controls::Button^ button = nullptr;

                auto rootLoadedEvent = std::make_shared<Event>();
                auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);

                auto textBoxFocusedEvent = std::make_shared<Event>();
                auto textBoxFocusedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);

                auto textSelectionChangedEvent = std::make_shared<Event>();
                auto textSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, SelectionChanged);

                RunOnUIThread([&]()
                {
                    auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                        L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                        // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                        L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                        L"  <TextBox x:Name='textBox' Text= '0123456789 X' FontSize='20' Width='200' Margin ='20,5,20,0' ContextFlyout='{x:Null}' SelectionFlyout='{x:Null}'/>"
                        L"</StackPanel>"));
                    textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                    VERIFY_IS_NOT_NULL(textBox);
                    button = safe_cast<xaml_controls::Button^>(rootPanel->FindName("button"));
                    VERIFY_IS_NOT_NULL(button);

                    rootLoadedRegistration.Attach(
                        rootPanel,
                        ref new xaml::RoutedEventHandler(
                        [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"Root loaded handler.");
                        rootLoadedEvent->Set();
                    }));

                    textSelectionChangedRegistration.Attach(
                        textBox,
                        ref new xaml::RoutedEventHandler(
                        [textSelectionChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"TextBox selection changed.");
                        textSelectionChangedEvent->Set();
                    }));

                    textBoxFocusedRegistration.Attach(
                        textBox,
                        ref new xaml::RoutedEventHandler(
                        [textBoxFocusedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"TextBox got focus.");
                        textBoxFocusedEvent->Set();
                    }));

                    TestServices::WindowHelper->WindowContent = rootPanel;
                });

                LOG_OUTPUT(L"Waiting for root loaded event.");
                TestServices::WindowHelper->SynchronouslyTickUIThread(2);
                rootLoadedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                LOG_OUTPUT(L"First Tap TextBox.");
                TestServices::InputHelper->Tap(textBox);
                textBoxFocusedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                textSelectionChangedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Selection Start %d, Length:%d", textBox->SelectionStart, textBox->SelectionLength);
                    VERIFY_IS_TRUE(textBox->SelectionLength == 0); // first tap places the cursor
                    VERIFY_IS_TRUE(textBox->SelectionStart == 10);
                });

                LOG_OUTPUT(L"Second TAP on the TextBox.");
                // Avoid a DoubleTap by tapping in a slightly different screen location
                TestServices::InputHelper->Tap(textBox, 0.2f, 0.2f);
                textSelectionChangedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Selection Start %d, Length:%d", textBox->SelectionStart, textBox->SelectionLength);
                    VERIFY_IS_TRUE(textBox->SelectionLength == 10);
                    VERIFY_IS_TRUE(textBox->SelectionStart == 0);
                });

                TestServices::WindowHelper->WaitForIdle();

                textSelectionChangedEvent->Reset();
                LOG_OUTPUT(L"Third TAP on the TextBox.");
                TestServices::InputHelper->Tap(textBox);
                textSelectionChangedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Selection Start %d, Length:%d", textBox->SelectionStart, textBox->SelectionLength);
                    VERIFY_IS_TRUE(textBox->SelectionLength == 0);
                    VERIFY_IS_TRUE(textBox->SelectionStart == 10);
                });

                // programmatically change focus
                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Focus the Button to discard the input pane.");
                    button->Focus(FocusState::Pointer);
                });

                RunOnUIThread([&]()
                {
                    // Prevent the text control from getting focus again so input pane remains hidden.
                    textBox->IsEnabled = false;
                });

                TestServices::WindowHelper->WaitForIdle();
            }

            void TextBoxSelectionTests::TextBoxThemeChangeInternal()
            {
                // Hide the textbox caret so it doesn't interfere with UIElement tree comparison
                RuntimeEnabledFeatureOverride disableTextBoxCaret(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableTextBoxCaret, true);
                ::Windows::Foundation::Size size(400, 400);
                TestServices::WindowHelper->SetWindowSizeOverride(size);

                xaml_controls::TextBox^ textBox = nullptr;
                xaml_controls::Button^ button = nullptr;

                auto rootLoadedEvent = std::make_shared<Event>();
                auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);

                RunOnUIThread([&]()
                {
                    auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                        L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                        // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                        L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                        L"  <TextBox x:Name='textBox' SelectionFlyout='{x:Null}' IsSpellCheckEnabled='false' Text= '0123456789' FontSize='20' Width='180' Margin ='20,5,20,0'/>"
                        L"</StackPanel>"));
                    textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                    VERIFY_IS_NOT_NULL(textBox);
                    button = safe_cast<xaml_controls::Button^>(rootPanel->FindName("button"));
                    VERIFY_IS_NOT_NULL(button);

                    rootLoadedRegistration.Attach(
                        rootPanel,
                        ref new xaml::RoutedEventHandler(
                            [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"Root loaded handler.");
                        rootLoadedEvent->Set();
                    }));

                    TestServices::WindowHelper->WindowContent = rootPanel;
                });

                LOG_OUTPUT(L"Waiting for root loaded event.");
                TestServices::WindowHelper->SynchronouslyTickUIThread(2);
                rootLoadedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                FocusTestHelper::EnsureFocus(textBox, FocusState::Keyboard);
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Update textbox theme to dark");
                    textBox->RequestedTheme = ElementTheme::Dark;
                });

                TestServices::WindowHelper->WaitForIdle();
                TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Focus the Button to discard the input pane.");
                    button->Focus(FocusState::Pointer);
                });

                RunOnUIThread([&]()
                {
                    // Prevent the text control from getting focus again so input pane remains hidden.
                    textBox->IsEnabled = false;
                });

                TestServices::WindowHelper->WaitForIdle();
            }

            void TextBoxSelectionTests::TextBoxThemeChangeWUC()
            {
                WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
                TextBoxThemeChangeInternal();
            }

            void TextBoxSelectionTests::NewTextBoxGripperTest()
            {
                TestCleanupWrapper cleanup;
                ::Windows::Foundation::Size size(400, 400);
                TestServices::WindowHelper->SetWindowSizeOverride(size);

                xaml_controls::TextBox^ textBox = nullptr;
                xaml_controls::TextBox^ textBox2 = nullptr;
                xaml_controls::Button^ button = nullptr;
                xaml_controls::StackPanel^ rootPanel = nullptr;

                auto rootLoadedEvent = std::make_shared<Event>();
                auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);

                auto textSelectionChangedEvent = std::make_shared<Event>();
                auto textSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, SelectionChanged);

                RunOnUIThread([&]()
                {
                    rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                        L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                        // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                        L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                        L"  <TextBox x:Name='textBox' Text= '0123456789 X' FontSize='20' Width='180' Margin ='20,5,20,0' ContextFlyout='{x:Null}' SelectionFlyout='{x:Null}'/>"
                        L"</StackPanel>"));
                    textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                    VERIFY_IS_NOT_NULL(textBox);
                    button = safe_cast<xaml_controls::Button^>(rootPanel->FindName("button"));
                    VERIFY_IS_NOT_NULL(button);

                    rootLoadedRegistration.Attach(
                        rootPanel,
                        ref new xaml::RoutedEventHandler(
                            [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"Root loaded handler.");
                        rootLoadedEvent->Set();
                    }));

                    textSelectionChangedRegistration.Attach(
                        textBox,
                        ref new xaml::RoutedEventHandler(
                            [textSelectionChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"TextBox selection changed.");
                        textSelectionChangedEvent->Set();
                    }));

                    TestServices::WindowHelper->WindowContent = rootPanel;
                });

                LOG_OUTPUT(L"Waiting for root loaded event.");
                TestServices::WindowHelper->SynchronouslyTickUIThread(2);
                rootLoadedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                FocusTestHelper::EnsureFocus(textBox, FocusState::Keyboard);
                TestServices::WindowHelper->WaitForIdle();

                LOG_OUTPUT(L"First Tap TextBox.");
                TestServices::InputHelper->Tap(textBox);
                textSelectionChangedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();
                textSelectionChangedEvent->Reset();

                LOG_OUTPUT(L"Second TAP on the TextBox."); // both grippers should be showing after second tap
                TestServices::InputHelper->Tap(textBox);
                textSelectionChangedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                auto textBox2FocusedEvent = std::make_shared<Event>();
                auto textBox2FocusedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);

                // create second textbox, switch focus to the second textbox and check if grippers associated with the first textbox can be hidden
                RunOnUIThread([&]()
                {
                    textBox2 = ref new  xaml_controls::TextBox();
                    rootPanel->Children->Append(textBox2);
                    textBox2FocusedRegistration.Attach(
                        textBox2,
                        ref new xaml::RoutedEventHandler(
                            [textBox2FocusedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"TextBox2 got focus.");
                        textBox2FocusedEvent->Set();
                    }));
                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    textBox2->Focus(xaml::FocusState::Programmatic);
                });

                textBox2FocusedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                LOG_OUTPUT(L"Grippers should be hidden after focus moved to different textbox.");
                RunOnUIThread([&]()
                {
                    auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(textBox->XamlRoot);
                    VERIFY_ARE_EQUAL(popups->Size, static_cast<UINT>(0));
                });

                TestServices::WindowHelper->WaitForIdle();

                // tap button to remove focus on textbox
                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Focus the Button to discard the input pane.");
                    button->Focus(FocusState::Pointer);
                });

                RunOnUIThread([&]()
                {
                    // Prevent the text control from getting focus again so input pane remains hidden.
                    textBox->IsEnabled = false;
                });

                TestServices::WindowHelper->WaitForIdle();
            }

            void TextBoxSelectionTests::TextBoxBringIntoViewByAPP()
            {
                TextBoxBringIntoView(true);
            }

            void TextBoxSelectionTests::TextBoxBringIntoViewByXAML()
            {
                TextBoxBringIntoView(false);
            }

            void TextBoxSelectionTests::TextBoxBringIntoView(bool appHandleSIPShowing)
            {
                WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

                ::Windows::Foundation::Size size(400, 800);
                TestServices::WindowHelper->SetWindowSizeOverride(size);
                InputPane^ inputPane;
                wf::EventRegistrationToken inputPaneShowToken;

                xaml_controls::TextBox^ textBox = nullptr;
                xaml_controls::Button^ button = nullptr;
                xaml_controls::StackPanel ^rootPanel = nullptr;

                auto rootLoadedEvent = std::make_shared<Event>();
                auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);

                auto textBoxFocusedEvent = std::make_shared<Event>();
                auto textBoxFocusedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);

                RunOnUIThread([&]()
                {
                    rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                        L"<StackPanel Width='400' Height='800' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                        // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                        L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                        L"  <TextBox x:Name='textBox' IsSpellCheckEnabled='false' AcceptsReturn='true' FontSize='20' Width='180' Margin ='20,5,20,0' ContextFlyout='{x:Null}' SelectionFlyout='{x:Null}'/>"
                        L"</StackPanel>"));
                    VERIFY_IS_NOT_NULL(rootPanel);
                    textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                    VERIFY_IS_NOT_NULL(textBox);
                    button = safe_cast<xaml_controls::Button^>(rootPanel->FindName("button"));
                    VERIFY_IS_NOT_NULL(button);

                    TestServices::WindowHelper->WindowContent = rootPanel;
                    rootLoadedRegistration.Attach(
                        rootPanel,
                        ref new xaml::RoutedEventHandler(
                            [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"Root FE Loaded handler.");
                        rootLoadedEvent->Set();
                    }));

                    textBoxFocusedRegistration.Attach(
                        textBox,
                        ref new xaml::RoutedEventHandler(
                        [textBoxFocusedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"TextBox got focus.");
                        textBoxFocusedEvent->Set();
                    }));
                });

                RunOnUIThread([&]()
                {
                    inputPane = test_infra::TestServices::WindowHelper->GetInputPaneForMainView();
                    inputPaneShowToken = inputPane->Showing += ref new TypedEventHandler<InputPane^, InputPaneVisibilityEventArgs^>([&](InputPane^ pane, InputPaneVisibilityEventArgs^ e)
                    {
                        if (appHandleSIPShowing)
                        {
                            textBox->MaxHeight = 100; // simulate APP adjusting textbox screen position on its own.
                            e->EnsuredFocusedElementInView = true; // set EnsuredFocusedElementInView to true, indicating to framework that APP has handled the SIP Showing event.
                        }
                    });
                });

                LOG_OUTPUT(L"Waiting for root loaded event.");
                TestServices::WindowHelper->SynchronouslyTickUIThread(2);
                rootLoadedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    for (UINT lines = 0; lines < 20; lines++) // append 20 lines of text
                    {
                        for (UINT chars = 0; chars <= lines; chars++)
                        {
                            textBox->Text += "A";
                        }
                        textBox->Text += "\r\n";
                    }
                });
                TestServices::WindowHelper->WaitForIdle();

                LOG_OUTPUT(L"TAP txtbox to bring up SIP.");
                TestServices::InputHelper->Tap(textBox);
                textBoxFocusedEvent->WaitForDefault();
                TestServices::WindowHelper->SynchronouslyTickUIThread(20);
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Move caret to bottom of textbox."); // Test bring caret into view
                    UINT textLength = textBox->Text->Length();
                    textBox->Select(textLength, 0);
                });

                TestServices::WindowHelper->SynchronouslyTickUIThread(10);
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    textBox->IsEnabled = false; // disable textbox so to remove caret
                });

                TestServices::WindowHelper->WaitForIdle();
                // verify textbox size and caret can be brought into view.
                TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

                // tap button to remove focus on textbox
                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Focus the Button to discard the input pane.");
                    button->Focus(FocusState::Pointer);
                });

                RunOnUIThread([&]()
                {
                    // remove showing event callback
                    inputPane->Showing -= inputPaneShowToken;
                });

                TestServices::WindowHelper->WaitForIdle();
            }

            void TextBoxSelectionTests::TextBoxBringIntoViewPivot()
            {
                WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

                ::Windows::Foundation::Size size(400, 800);
                TestServices::WindowHelper->SetWindowSizeOverride(size);

                xaml_controls::TextBox^ textBox = nullptr;
                xaml_controls::Button^ button = nullptr;
                xaml_controls::StackPanel ^rootPanel = nullptr;

                auto rootLoadedEvent = std::make_shared<Event>();
                auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);

                auto textBoxFocusedEvent = std::make_shared<Event>();
                auto textBoxFocusedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);

                RunOnUIThread([&]()
                {
                    // testing textbox inside a pivot header. Problematic bringintoview logic can cause pivot to start moving
                    rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                        L"<StackPanel Width='400' Height='800' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                        L"<Pivot Title = 'Hello'> "
                        L"<PivotItem Header = 'textBox'> "
                        L"<StackPanel> "
                        L"<Button x:Name = 'button' Content = 'Focus' Margin = '30,40,20,0' /> "
                        L"<TextBox x:Name = 'textBox' HorizontalAlignment = 'Left' TextWrapping = 'NoWrap' FontSize = '20' Width = '200' Margin = '30,20,0,0' /> "
                        L"</StackPanel> "
                        L"</PivotItem> "
                        L"<PivotItem Header = 'textBlock' > "
                        L"<TextBlock Text = 'block' /> "
                        L"</PivotItem> "
                        L"</Pivot> "
                        L"</StackPanel>"));

                    VERIFY_IS_NOT_NULL(rootPanel);
                    textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                    VERIFY_IS_NOT_NULL(textBox);

                    // Disable SelectionFlyout since it can cause the test to fail due to leaving open popups on the screen.
                    textBox->SelectionFlyout = nullptr;

                    TestServices::WindowHelper->WindowContent = rootPanel;
                    rootLoadedRegistration.Attach(
                        rootPanel,
                        ref new xaml::RoutedEventHandler(
                            [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"Root FE Loaded handler.");
                        rootLoadedEvent->Set();
                    }));

                    textBoxFocusedRegistration.Attach(
                        textBox,
                        ref new xaml::RoutedEventHandler(
                            [textBoxFocusedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"TextBox got focus.");
                        textBoxFocusedEvent->Set();
                    }));
                });

                LOG_OUTPUT(L"Waiting for root loaded event.");
                TestServices::WindowHelper->SynchronouslyTickUIThread(2);
                rootLoadedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    // extra long text to force textbox to scroll the text inside
                    textBox->Text = L"1 LONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONGLONG";
                });
                TestServices::WindowHelper->WaitForIdle();

                LOG_OUTPUT(L"TAP txtbox to bring up SIP.");
                TestServices::InputHelper->Tap(textBox);
                textBoxFocusedEvent->WaitForDefault();
                TestServices::WindowHelper->SynchronouslyTickUIThread(10);
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    //moving caret to begin and end may cause Pivot header to scroll TextBox's position if there is a bug in BringIntoView logic
                    LOG_OUTPUT(L"Move caret to first char.");
                    UINT textLength = textBox->Text->Length();
                    textBox->Select(1, 0);
                    LOG_OUTPUT(L"Move caret to end of textbox."); // Test bring caret into view
                    textBox->Select(textLength, 0);
                });

                // additional ticks to have the BringIntoView async events to take effect
                TestServices::WindowHelper->SynchronouslyTickUIThread(10);
                TestServices::WindowHelper->WaitForIdle();

                // verify textbox pivot header does not scroll using DComp dump
                TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

                RunOnUIThread([&]()
                {
                    textBox->IsEnabled = false; // disable textbox to hide the input pane
                });
                TestServices::WindowHelper->WaitForIdle();
            }

            void TextBoxSelectionTests::TextBoxBringIntoViewMultiLine()
            {
                RuntimeEnabledFeatureOverride disableTextBoxCaret(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableTextBoxCaret, true);
                WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

                ::Windows::Foundation::Size size(400, 800);
                TestServices::WindowHelper->SetWindowSizeOverride(size);

                xaml_controls::TextBox^ textBox = nullptr;
                xaml_controls::StackPanel ^rootPanel = nullptr;

                auto rootLoadedEvent = std::make_shared<Event>();
                auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);

                auto textBoxFocusedEvent = std::make_shared<Event>();
                auto textBoxFocusedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);

                RunOnUIThread([&]()
                {
                    // multiline textbox has height < 3 lines, verifying caret scroll into view when setting to first and last chars of three lines of text
                    rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                        L"<StackPanel Width='400' Height='800' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                        L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                        L"<TextBox x:Name = 'textBox' SelectionFlyout='{x:Null}' HorizontalAlignment = 'Left' TextWrapping = 'Wrap' FontSize = '20' Height = '50' Width = '200' Margin = '30,20,0,0' AcceptsReturn = 'True' /> "
                        L"</StackPanel>"));

                    VERIFY_IS_NOT_NULL(rootPanel);
                    textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                    VERIFY_IS_NOT_NULL(textBox);

                    rootLoadedRegistration.Attach(
                        rootPanel,
                        ref new xaml::RoutedEventHandler(
                            [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"Root FE Loaded handler.");
                        rootLoadedEvent->Set();
                    }));

                    textBoxFocusedRegistration.Attach(
                        textBox,
                        ref new xaml::RoutedEventHandler(
                            [textBoxFocusedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"TextBox got focus.");
                        textBoxFocusedEvent->Set();
                    }));

                    TestServices::WindowHelper->WindowContent = rootPanel;
                });

                LOG_OUTPUT(L"Waiting for root loaded event.");
                TestServices::WindowHelper->SynchronouslyTickUIThread(2);
                rootLoadedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                LOG_OUTPUT(L"TAP txtbox to bring up SIP.");
                TestServices::InputHelper->Tap(textBox);
                textBoxFocusedEvent->WaitForDefault();
                TestServices::WindowHelper->SynchronouslyTickUIThread(10);
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    textBox->Text = L"Line 1 \r\n";
                    textBox->Text += L"Line 2 \r\n";
                    textBox->Text += L"Line 3";
                });

                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    //moving caret to end of the text
                    LOG_OUTPUT(L"Move caret to last char.");
                    textBox->Select(textBox->Text->Length(), 0);
                });

                // additional ticks to have the BringIntoView async events to take effect
                TestServices::WindowHelper->SynchronouslyTickUIThread(10);
                TestServices::WindowHelper->WaitForIdle();
                TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"1");

                RunOnUIThread([&]()
                {
                    //moving caret to begin of the text
                    LOG_OUTPUT(L"Move caret to first char.");
                    textBox->Select(0, 0);
                });

                // additional ticks to have the BringIntoView async events to take effect
                TestServices::WindowHelper->SynchronouslyTickUIThread(10);
                TestServices::WindowHelper->WaitForIdle();
                TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

                RunOnUIThread([&]()
                {
                    textBox->IsEnabled = false; // disable textbox to hide the input pane
                });
                TestServices::WindowHelper->WaitForIdle();
            }

            void TextBoxSelectionTests::TextBoxMouseSelection()
            {
                TestCleanupWrapper cleanup;
                ::Windows::Foundation::Size size(400, 800);
                TestServices::WindowHelper->SetWindowSizeOverride(size);

                xaml_controls::TextBox^ textBox = nullptr;
                xaml_controls::StackPanel ^rootPanel = nullptr;

                auto rootLoadedEvent = std::make_shared<Event>();
                auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);

                auto textBoxFocusedEvent = std::make_shared<Event>();
                auto textBoxFocusedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);

                RunOnUIThread([&]()
                {
                    rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                        L"<StackPanel Width='400' Height='800' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                        L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                        L"<TextBox x:Name = 'textBox' HorizontalAlignment = 'Left' TextWrapping = 'NoWrap' FontSize = '20' Height = '30' Width = '200' Margin = '30,40,0,0' Text = '01234' /> "
                        L"</StackPanel>"));

                    VERIFY_IS_NOT_NULL(rootPanel);
                    textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                    VERIFY_IS_NOT_NULL(textBox);

                    rootLoadedRegistration.Attach(
                        rootPanel,
                        ref new xaml::RoutedEventHandler(
                            [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"Root FE Loaded handler.");
                        rootLoadedEvent->Set();
                    }));

                    textBoxFocusedRegistration.Attach(
                        textBox,
                        ref new xaml::RoutedEventHandler(
                            [textBoxFocusedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"TextBox got focus.");
                        textBoxFocusedEvent->Set();
                    }));

                    TestServices::WindowHelper->WindowContent = rootPanel;
                });

                LOG_OUTPUT(L"Waiting for root loaded event.");
                TestServices::WindowHelper->SynchronouslyTickUIThread(2);
                rootLoadedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Focus on txtbox.");
                    textBox->Focus(FocusState::Pointer);
                });

                textBoxFocusedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();


                // Select text using mouse: select should still work if user moves mouse outside outside of textbox
                TestServices::InputHelper->DragFromCenter(textBox, -100/*relX*/, -60 /*relY*/, 0.1 /*velocityFactor*/);
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Selection Start %d, Length:%d", textBox->SelectionStart, textBox->SelectionLength);
                    VERIFY_IS_TRUE(textBox->SelectionLength == 5);
                    VERIFY_IS_TRUE(textBox->SelectionStart == 0);
                });

                RunOnUIThread([&]()
                {
                    textBox->IsEnabled = false; // disable textbox to hide the input pane
                });
                TestServices::WindowHelper->WaitForIdle();
            }

            void TextBoxSelectionTests::TextBoxSelectionHighlightColor()
            {
                TestCleanupWrapper cleanup;
                WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
                ::Windows::Foundation::Size size(400, 800);
                TestServices::WindowHelper->SetWindowSizeOverride(size);

                auto textSelectionChangedEvent = std::make_shared<Event>();
                auto textSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, SelectionChanged);

                xaml_controls::TextBox^ textBox = nullptr;
                xaml_controls::StackPanel ^rootPanel = nullptr;
                xaml_controls::Button ^button = nullptr;

                RunOnUIThread([&]()
                {
                    rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                        L"<StackPanel Width='400' Height='800' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                        L"<StackPanel.Resources>"
                        L"<ResourceDictionary>"
                        L"<ResourceDictionary.ThemeDictionaries>"
                        L"<ResourceDictionary x:Key = 'Default'>"
                        L"<SolidColorBrush x:Key = 'GreenBrush' Color = 'Green'/>"
                        L"</ResourceDictionary>"
                        L"</ResourceDictionary.ThemeDictionaries>"
                        L"</ResourceDictionary>"
                        L"</StackPanel.Resources>"
                        L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                        L"  <TextBox x:Name = 'textBox' HorizontalAlignment = 'Left' TextWrapping = 'NoWrap' SelectionHighlightColor='Red' SelectionHighlightColorWhenNotFocused='{ThemeResource GreenBrush}' FontSize = '20' Height = '30' Width = '200' Margin = '30,40,0,0' Text = '01234' PreventKeyboardDisplayOnProgrammaticFocus = 'True' /> "
                        L"</StackPanel>"));

                    VERIFY_IS_NOT_NULL(rootPanel);
                    textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                    VERIFY_IS_NOT_NULL(textBox);
                    button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
                    VERIFY_IS_NOT_NULL(button);
                    TestServices::WindowHelper->WindowContent = rootPanel;

                    textSelectionChangedRegistration.Attach(
                        textBox,
                        ref new xaml::RoutedEventHandler(
                            [textSelectionChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"TextBox selection changed.");
                        textSelectionChangedEvent->Set();
                    }));
                });
                TestServices::WindowHelper->WaitForIdle();

                FocusTestHelper::EnsureFocus(textBox, FocusState::Programmatic);
                textSelectionChangedEvent->Reset();

                RunOnUIThread([&]()
                {
                    textBox->SelectAll();
                });

                textSelectionChangedEvent->WaitForDefault();
                LOG_OUTPUT(L"1. Dump Dcomp to verify focused red SelectionHighlightColor.");
                TestServices::WindowHelper->SynchronouslyTickUIThread(5);
                TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Focused-Red");

                FocusTestHelper::EnsureFocus(button, FocusState::Programmatic);
                LOG_OUTPUT(L"2. Dump Dcomp to verify unfocused green SelectionHighlightColorWhenNotFocused.");
                TestServices::WindowHelper->SynchronouslyTickUIThread(5);
                TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Unfocused-Green");

                LOG_OUTPUT(L"3. Update SelectionHighlightColor to Yellow and SelectionHighlightColorWhenNotFocused to Blue.");
                RunOnUIThread([&]()
                {
                    textBox->SelectionHighlightColorWhenNotFocused = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Blue);
                    textBox->SelectionHighlightColor = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Yellow);
                });
                TestServices::WindowHelper->WaitForIdle();
                LOG_OUTPUT(L"4. Dump Dcomp to verify unfocused blue SelectionHighlightColorWhenNotFocused.");
                TestServices::WindowHelper->SynchronouslyTickUIThread(5);
                TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"UnFocused-Blue");

                FocusTestHelper::EnsureFocus(textBox, FocusState::Programmatic);
                LOG_OUTPUT(L"5. Dump Dcomp to verify focused yellow SelectionHighlightColor.");
                TestServices::WindowHelper->SynchronouslyTickUIThread(5);
                TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Focused-Yellow");

                LOG_OUTPUT(L"6. Update SelectionHighlightColor to Red.");
                RunOnUIThread([&]()
                {
                    textBox->SelectionHighlightColor = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
                });
                TestServices::WindowHelper->WaitForIdle();
                LOG_OUTPUT(L"7. Dump Dcomp to verify focused red SelectionHighlightColor.");
                TestServices::WindowHelper->SynchronouslyTickUIThread(5);
                TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Focused-Red");

                FocusTestHelper::EnsureFocus(button, FocusState::Programmatic);
                LOG_OUTPUT(L"8. Dump Dcomp to verify unfocused blue SelectionHighlightColorWhenNotFocused again.");
                TestServices::WindowHelper->SynchronouslyTickUIThread(5);
                TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Unfocused-Blue");

                TestServices::WindowHelper->WaitForIdle();
            }

            void TextBoxSelectionTests::RichEditBoxSelectionHighlightColor()
            {
                TestCleanupWrapper cleanup;
                WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
                ::Windows::Foundation::Size size(400, 800);
                TestServices::WindowHelper->SetWindowSizeOverride(size);

                auto textSelectionChangedEvent = std::make_shared<Event>();
                auto textSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, SelectionChanged);

                xaml_controls::RichEditBox^ richEditBox = nullptr;
                xaml_controls::StackPanel ^rootPanel = nullptr;
                xaml_controls::Button ^button = nullptr;

                RunOnUIThread([&]()
                {
                    rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                        L"<StackPanel Width='400' Height='800' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                        L"<StackPanel.Resources>"
                        L"<ResourceDictionary>"
                        L"  <ResourceDictionary.ThemeDictionaries>"
                        L"    <ResourceDictionary x:Key = 'Default'>"
                        L"      <SolidColorBrush x:Key = 'GreenBrush' Color = 'Green'/>"
                        L"    </ResourceDictionary>"
                        L"  </ResourceDictionary.ThemeDictionaries>"
                        L"</ResourceDictionary>"
                        L"</StackPanel.Resources>"
                        L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                        L"  <RichEditBox x:Name = 'richEditBox' HorizontalAlignment = 'Left' TextWrapping = 'NoWrap' SelectionHighlightColor='Red' SelectionHighlightColorWhenNotFocused='{ThemeResource GreenBrush}' FontSize = '20' Height = '30' Width = '200' Margin = '30,40,0,0' PreventKeyboardDisplayOnProgrammaticFocus = 'True' /> "
                        L"</StackPanel>"));

                    VERIFY_IS_NOT_NULL(rootPanel);
                    richEditBox = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"richEditBox"));
                    VERIFY_IS_NOT_NULL(richEditBox);
                    button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
                    VERIFY_IS_NOT_NULL(button);
                    TestServices::WindowHelper->WindowContent = rootPanel;

                    textSelectionChangedRegistration.Attach(
                        richEditBox,
                        ref new xaml::RoutedEventHandler(
                            [textSelectionChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"RichEditBox selection changed.");
                        textSelectionChangedEvent->Set();
                    }));
                });
                TestServices::WindowHelper->WaitForIdle();

                FocusTestHelper::EnsureFocus(richEditBox, FocusState::Programmatic);
                textSelectionChangedEvent->Reset();

                RunOnUIThread([&]()
                {
                    richEditBox->Document->SetText(TextSetOptions::None, L"01234");
                    richEditBox->Document->Selection->StartPosition = 1;
                    richEditBox->Document->Selection->EndPosition = 4;
                });

                textSelectionChangedEvent->WaitForDefault();
                LOG_OUTPUT(L"1. Dump Dcomp to verify focused red SelectionHighlightColor.");
                TestServices::WindowHelper->SynchronouslyTickUIThread(5);
                TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Focused-Red");

                FocusTestHelper::EnsureFocus(button, FocusState::Programmatic);
                LOG_OUTPUT(L"2. Dump Dcomp to verify unfocused green SelectionHighlightColorWhenNotFocused.");
                TestServices::WindowHelper->SynchronouslyTickUIThread(5);
                TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Unfocused-Green");

                LOG_OUTPUT(L"3. Update SelectionHighlightColor to Yellow and SelectionHighlightColorWhenNotFocused to Blue.");
                RunOnUIThread([&]()
                {
                    richEditBox->SelectionHighlightColorWhenNotFocused = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Blue);
                    richEditBox->SelectionHighlightColor = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Yellow);
                });
                TestServices::WindowHelper->WaitForIdle();
                LOG_OUTPUT(L"4. Dump Dcomp to verify unfocused blue SelectionHighlightColorWhenNotFocused.");
                TestServices::WindowHelper->SynchronouslyTickUIThread(5);
                TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"UnFocused-Blue");

                FocusTestHelper::EnsureFocus(richEditBox, FocusState::Programmatic);
                LOG_OUTPUT(L"5. Dump Dcomp to verify focused yellow SelectionHighlightColor.");
                TestServices::WindowHelper->SynchronouslyTickUIThread(5);
                TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Focused-Yellow");

                LOG_OUTPUT(L"6. Update SelectionHighlightColor to Red.");
                RunOnUIThread([&]()
                {
                    richEditBox->SelectionHighlightColor = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
                });
                TestServices::WindowHelper->WaitForIdle();
                LOG_OUTPUT(L"7. Dump Dcomp to verify focused red SelectionHighlightColor.");
                TestServices::WindowHelper->SynchronouslyTickUIThread(5);
                TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Focused-Red");

                FocusTestHelper::EnsureFocus(button, FocusState::Programmatic);
                LOG_OUTPUT(L"8. Dump Dcomp to verify unfocused blue SelectionHighlightColorWhenNotFocused again.");
                TestServices::WindowHelper->SynchronouslyTickUIThread(5);
                TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Unfocused-Blue");

                TestServices::WindowHelper->WaitForIdle();
            }

            void TextBoxSelectionTests::VerifyTextBoxSelectionGrippers()
            {
                // Win11 25H2 changed the default accent color from #0078D7 to #0078D4.
                if (IsOSBuildAtLeast(26200))
                {
                    TestServices::Utilities->SetDCompXmlVariable(L"AccentBlue", L"rgb {0, 0.4706, 0.8314}");
                }
                else
                {
                    TestServices::Utilities->SetDCompXmlVariable(L"AccentBlue", L"rgb {0, 0.4706, 0.8431}");
                }

                TestCleanupWrapper cleanup;
                WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

                ::Windows::Foundation::Size size(400, 400);
                TestServices::WindowHelper->SetWindowSizeOverride(size);

                xaml_controls::TextBox^ textBox = nullptr;
                xaml_controls::Button^ button = nullptr;

                auto rootLoadedEvent = std::make_shared<Event>();
                auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);

                auto textBoxFocusedEvent = std::make_shared<Event>();
                auto textBoxFocusedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);

                auto textSelectionChangedEvent = std::make_shared<Event>();
                auto textSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, SelectionChanged);

                RunOnUIThread([&]()
                {
                    auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                        L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                        // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                        L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                        L"  <TextBox x:Name='textBox' SelectionFlyout='{x:Null}' Text= '0123456789 X' FontSize='20' Width='200' Margin ='20,5,20,0'/>"
                        L"</StackPanel>"));
                    textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                    VERIFY_IS_NOT_NULL(textBox);
                    button = safe_cast<xaml_controls::Button^>(rootPanel->FindName("button"));
                    VERIFY_IS_NOT_NULL(button);

                    rootLoadedRegistration.Attach(
                        rootPanel,
                        ref new xaml::RoutedEventHandler(
                            [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"Root loaded handler.");
                        rootLoadedEvent->Set();
                    }));

                    textSelectionChangedRegistration.Attach(
                        textBox,
                        ref new xaml::RoutedEventHandler(
                            [textSelectionChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"TextBox selection changed.");
                        textSelectionChangedEvent->Set();
                    }));

                    textBoxFocusedRegistration.Attach(
                        textBox,
                        ref new xaml::RoutedEventHandler(
                            [textBoxFocusedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"TextBox got focus.");
                        textBoxFocusedEvent->Set();
                    }));

                    TestServices::WindowHelper->WindowContent = rootPanel;
                });

                LOG_OUTPUT(L"Waiting for root loaded event.");
                TestServices::WindowHelper->SynchronouslyTickUIThread(2);
                rootLoadedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                LOG_OUTPUT(L"First Tap TextBox.");
                TestServices::InputHelper->Tap(textBox);

                textBoxFocusedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                textSelectionChangedEvent->Reset();
                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Selection Start %d, Length:%d", textBox->SelectionStart, textBox->SelectionLength);
                    VERIFY_IS_TRUE(textBox->SelectionLength == 0); // first tap places the cursor
                    VERIFY_IS_TRUE(textBox->SelectionStart == 10);
                });

                LOG_OUTPUT(L"Second TAP on the TextBox.");
                // Avoid a DoubleTap by tapping in a slightly different screen location
                TestServices::InputHelper->Tap(textBox, 0.2f, 0.2f);
                textSelectionChangedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Selection Start %d, Length:%d", textBox->SelectionStart, textBox->SelectionLength);
                    VERIFY_IS_TRUE(textBox->SelectionLength == 10);
                    VERIFY_IS_TRUE(textBox->SelectionStart == 0);
                });

                TestServices::WindowHelper->WaitForIdle();
                TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);
                TestServices::WindowHelper->WaitForIdle();
            }

            void TextBoxSelectionTests::VerifyTextBlockSelectionGrippers()
            {
                TestCleanupWrapper cleanup;

                ::Windows::Foundation::Size size(400, 400);
                TestServices::WindowHelper->SetWindowSizeOverride(size);

                xaml_controls::TextBlock^ textBlock;
                xaml_controls::Button^ button;

                auto textSelectionChangedEvent = std::make_shared<Event>();
                auto textSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBlock, SelectionChanged);

                RunOnUIThread([&]()
                {
                    auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                        L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                        // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                        L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                        L"  <TextBlock x:Name='textBlock' Height='200' Width='200' IsTextSelectionEnabled='True' Text='1234567890' ContextFlyout='{x:Null}' SelectionFlyout='{x:Null}' />"
                        L"</StackPanel>"));
                    textBlock = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"textBlock"));
                    VERIFY_IS_NOT_NULL(textBlock);
                    button = safe_cast<xaml_controls::Button^>(rootPanel->FindName("button"));
                    VERIFY_IS_NOT_NULL(button);
                    TestServices::WindowHelper->WindowContent = rootPanel;

                    textSelectionChangedRegistration.Attach(
                        textBlock,
                        ref new xaml::RoutedEventHandler(
                            [textSelectionChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"TextBlock selection changed.");
                        textSelectionChangedEvent->Set();
                    }));

                });

                TestServices::WindowHelper->WaitForIdle();
                FocusTestHelper::EnsureFocus(button, FocusState::Pointer);
                TestServices::WindowHelper->WaitForIdle();

                TestServices::InputHelper->Tap(textBlock);
                TestServices::WindowHelper->WaitForIdle();

                textSelectionChangedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();
                xaml::Shapes::Rectangle^ gripperRect = GetGripperRect(textBlock);

                VERIFY_IS_NOT_NULL(gripperRect);
                TestServices::WindowHelper->WaitForIdle();
            }

            // The TextCommandBarFlyout has an associated light-dismiss layer, and light-dismiss layers
            // usually capture and eat any pointer input that they receive.  The TextCommandBarFlyout's
            // light-dismiss layer should not do that for any control, however.
            // A bug that was fixed surrounded popups not being interactable through the TextCommandBarFlyout's
            // light-dismiss layer, and this test verifies that that has not regressed.
            void TextBoxSelectionTests::VerifyTextBoxSelectionGrippersAreInteractibleWithTextCommandBarFlyout()
            {
                TestCleanupWrapper cleanup;

                xaml_controls::TextBox^ textBox = nullptr;
                xaml_controls::Button^ button = nullptr;

                auto rootLoadedEvent = std::make_shared<Event>();
                auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);

                auto textSelectionChangedEvent = std::make_shared<Event>();
                auto textSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, SelectionChanged);

                RunOnUIThread([&]()
                {
                    auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                        L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                        L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                        L"  <TextBox x:Name='textBox' Text= '0123456789 X' FontSize='20' Width='200' Margin ='20,5,20,0'/>"
                        L"</StackPanel>"));
                    textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                    VERIFY_IS_NOT_NULL(textBox);
                    button = safe_cast<xaml_controls::Button^>(rootPanel->FindName("button"));
                    VERIFY_IS_NOT_NULL(button);

                    rootLoadedRegistration.Attach(rootPanel, [rootLoadedEvent]() { rootLoadedEvent->Set(); });
                    textSelectionChangedRegistration.Attach(textBox,
                        [textBox, textSelectionChangedEvent]()
                        {
                            LOG_OUTPUT(L"Selection changed. Selection start = %d, length = %d", textBox->SelectionStart, textBox->SelectionLength);
                            textSelectionChangedEvent->Set();
                        });

                    TestServices::WindowHelper->WindowContent = rootPanel;
                });

                rootLoadedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                FocusTestHelper::EnsureFocus(button, FocusState::Pointer);
                TestServices::WindowHelper->WaitForIdle();

                LOG_OUTPUT(L"Tap on the TextBox to set the caret initially.");
                TestServices::InputHelper->Tap(textBox);
                
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    VERIFY_ARE_EQUAL(0, textBox->SelectionLength);
                    VERIFY_ARE_EQUAL(10, textBox->SelectionStart);
                });

                LOG_OUTPUT(L"Tap on the TextBox again to select the text.");
                TestServices::InputHelper->Tap(textBox, 0.2f, 0.2f);
                textSelectionChangedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    VERIFY_ARE_EQUAL(10, textBox->SelectionLength);
                    VERIFY_ARE_EQUAL(0, textBox->SelectionStart);
                });

                TestServices::WindowHelper->WaitForIdle();

                xaml::Shapes::Rectangle^ gripperRect = GetGripperRect(textBox);

                LOG_OUTPUT(L"Pan the left gripper to the right. This gripper interaction should show the TextCommandBarFlyout.");
                TestServices::InputHelper->PanFromCenter(gripperRect, 100 /*relX*/, 0 /*relY*/, 0.1 /*velocityFactor*/);

                textSelectionChangedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                // We'll check to make sure that the TextCommandBarFlyout is open at this point
                // to make sure that we're verifying the correct scenario.
                bool isTextCommandBarFlyoutOpen = false;
                auto checkedFlyoutState = std::make_shared<Event>();

                RunOnUIThread([&]()
                {
                    auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(textBox->XamlRoot);

                    for (auto popup : popups)
                    {
                        if (TreeHelper::GetVisualChildByType<xaml_controls::CommandBar>(safe_cast<xaml::FrameworkElement^>(popup->Child)) != nullptr)
                        {
                            isTextCommandBarFlyoutOpen = true;
                        }
                    }

                    VERIFY_IS_TRUE(isTextCommandBarFlyoutOpen);
                    checkedFlyoutState->Set();
                });

                checkedFlyoutState->WaitForDefault();

                LOG_OUTPUT(L"Now pan the left gripper to the right. This should be possible even with the TextCommandBarFlyout showing.");
                TestServices::InputHelper->PanFromCenter(gripperRect, -100 /*relX*/, 0 /*relY*/, 0.1 /*velocityFactor*/);

                textSelectionChangedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();
            }

            void TextBoxSelectionTests::VerifyTextBoxFocusAndSelectAll()
            {
                TestCleanupWrapper cleanup;
                WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

                // Hide the textbox caret so it doesn't interfere with UIElement tree comparison
                RuntimeEnabledFeatureOverride disableTextBoxCaret(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableTextBoxCaret, true);

                ::Windows::Foundation::Size size(400, 400);
                TestServices::WindowHelper->SetWindowSizeOverride(size);

                xaml_controls::TextBox^ textBox = nullptr;
                xaml_controls::Button^ button = nullptr;

                auto textBoxFocusedEvent = std::make_shared<Event>();
                auto textBoxFocusedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);

                auto textSelectionChangedEvent = std::make_shared<Event>();
                auto textSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, SelectionChanged);

                RunOnUIThread([&]()
                {
                    auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                        L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                        L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                        L"  <TextBox x:Name='textBox' Text= '0123456789012345678901234567890123456789012345678901234567890123456789' FontSize='20' Width='180' Margin ='20,5,20,0'/>"
                        L"</StackPanel>"));

                    TestServices::WindowHelper->WindowContent = rootPanel;
                    textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                    VERIFY_IS_NOT_NULL(textBox);
                    button = safe_cast<xaml_controls::Button^>(rootPanel->FindName("button"));
                    VERIFY_IS_NOT_NULL(button);

                    textSelectionChangedRegistration.Attach(
                        textBox,
                        ref new xaml::RoutedEventHandler(
                            [textSelectionChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"TextBox selection changed.");
                        textSelectionChangedEvent->Set();
                    }));

                    textBoxFocusedRegistration.Attach(
                        textBox,
                        ref new xaml::RoutedEventHandler(
                            [&](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"TextBox got focus.");
                        textSelectionChangedEvent->Reset();
                        textBox->SelectAll();
                        textBoxFocusedEvent->Set();
                    }));
                });

                TestServices::WindowHelper->WaitForIdle();
                FocusTestHelper::EnsureFocus(button, FocusState::Programmatic);
                TestServices::WindowHelper->WaitForIdle();
                FocusTestHelper::EnsureFocus(textBox, FocusState::Programmatic);

                textBoxFocusedEvent->WaitForDefault();
                textSelectionChangedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Selection Start %d, Length:%d", textBox->SelectionStart, textBox->SelectionLength);
                    VERIFY_IS_TRUE(textBox->SelectionLength == 70);
                    VERIFY_IS_TRUE(textBox->SelectionStart == 0);
                });
                TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);
                TestServices::WindowHelper->WaitForIdle();
            }

            void TextBoxSelectionTests::KeyboardSelectionChangingEvent()
            {
                TestCleanupWrapper cleanup;
                WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

                ::Windows::Foundation::Size size(400, 400);
                TestServices::WindowHelper->SetWindowSizeOverride(size);

                xaml_controls::TextBox^ textBox = nullptr;

                auto textSelectionChangedEvent = std::make_shared<Event>();
                auto textSelectionChangingEvent = std::make_shared<Event>();
                auto textSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, SelectionChanged);
                auto textSelectionChangingRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, SelectionChanging);

                RunOnUIThread([&]()
                {
                    auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                        L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                        L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                        L"  <TextBox x:Name='textBox' Text= 'Hello World' FontSize='20' Width='180' Margin ='20,5,20,0'/>"
                        L"</StackPanel>"));

                    TestServices::WindowHelper->WindowContent = rootPanel;
                    textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                    VERIFY_IS_NOT_NULL(textBox);
                });

                TestServices::WindowHelper->WaitForIdle();
                FocusTestHelper::EnsureFocus(textBox, FocusState::Programmatic);

                bool cancelSelectionChanging = true;
                int expectedSelectionStart = 11;
                int expectedSelectionLength = 0;

                LOG_OUTPUT(L"Test cancelling the SelectionChanging event with End key press.");
                RunOnUIThread([&]()
                {
                    textSelectionChangedRegistration.Attach(
                        textBox,
                        ref new xaml::RoutedEventHandler(
                            [textSelectionChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"TextBox selection changed.");
                        textSelectionChangedEvent->Set();
                    }));

                    textSelectionChangingRegistration.Attach(
                        textBox,
                        ref new wf::TypedEventHandler<xaml_controls::TextBox^, xaml_controls::TextBoxSelectionChangingEventArgs^>(
                            [&](xaml_controls::TextBox ^sender, xaml_controls::TextBoxSelectionChangingEventArgs^ args)
                    {
                        LOG_OUTPUT(L"TextBox selection changing: start:%d length:%d", args->SelectionStart, args->SelectionLength);
                        VERIFY_ARE_EQUAL(args->SelectionStart, expectedSelectionStart);
                        VERIFY_ARE_EQUAL(args->SelectionLength, expectedSelectionLength);
                        textSelectionChangingEvent->Set();
                        if (cancelSelectionChanging)
                        {
                            args->Cancel = true;
                        }
                    }));
                });

                TestServices::KeyboardHelper->End();
                textSelectionChangingEvent->WaitForDefault();
                VERIFY_IS_FALSE(textSelectionChangedEvent->HasFired());

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Selection Start %d, Length:%d", textBox->SelectionStart, textBox->SelectionLength);
                    VERIFY_ARE_EQUAL(textBox->SelectionStart, 0);
                    VERIFY_ARE_EQUAL(textBox->SelectionLength, 0);
                });

                LOG_OUTPUT(L"Test not cancelling the SelectionChanging event with End key press.");
                cancelSelectionChanging = false;
                expectedSelectionStart = 11;
                expectedSelectionLength = 0;

                textSelectionChangingEvent->Reset();
                TestServices::KeyboardHelper->End();
                textSelectionChangingEvent->WaitForDefault();
                textSelectionChangedEvent->WaitForDefault();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Selection Start %d, Length:%d", textBox->SelectionStart, textBox->SelectionLength);
                    VERIFY_ARE_EQUAL(textBox->SelectionStart, 11);
                    VERIFY_ARE_EQUAL(textBox->SelectionLength, 0);
                });

                LOG_OUTPUT(L"Test cancelling the SelectionChanging event with Ctrl-A key press.");
                cancelSelectionChanging = true;
                expectedSelectionStart = 0;
                expectedSelectionLength = 11;
                textSelectionChangingEvent->Reset();
                textSelectionChangedEvent->Reset();

                TestServices::KeyboardHelper->PressKeySequence("$d$_ctrlscan#$d$_a#$u$_a#$u$_ctrlscan");

                textSelectionChangingEvent->WaitForDefault();
                VERIFY_IS_FALSE(textSelectionChangedEvent->HasFired());

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Selection Start %d, Length:%d", textBox->SelectionStart, textBox->SelectionLength);
                    VERIFY_ARE_EQUAL(textBox->SelectionStart, 11);
                    VERIFY_ARE_EQUAL(textBox->SelectionLength, 0);
                });
            }

            void TextBoxSelectionTests::ProgrammaticSelectionChangingEvent()
            {
                TestCleanupWrapper cleanup;
                WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

                ::Windows::Foundation::Size size(400, 400);
                TestServices::WindowHelper->SetWindowSizeOverride(size);

                xaml_controls::TextBox^ textBox = nullptr;

                auto textSelectionChangedEvent = std::make_shared<Event>();
                auto textSelectionChangingEvent = std::make_shared<Event>();
                auto textSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, SelectionChanged);
                auto textSelectionChangingRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, SelectionChanging);

                RunOnUIThread([&]()
                {
                    auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                        L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                        L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                        L"  <TextBox x:Name='textBox' Text= 'Hello World' FontSize='20' Width='180' Margin ='20,5,20,0'/>"
                        L"</StackPanel>"));

                    TestServices::WindowHelper->WindowContent = rootPanel;
                    textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                    VERIFY_IS_NOT_NULL(textBox);
                });

                TestServices::WindowHelper->WaitForIdle();
                FocusTestHelper::EnsureFocus(textBox, FocusState::Programmatic);

                bool cancelSelectionChanging = true;
                int expectedSelectionStart = 11;
                int expectedSelectionLength = 0;
                bool appChangeSelection = false;

                LOG_OUTPUT(L"Test cancelling the SelectionChanging event from programmatic selection change.");
                RunOnUIThread([&]()
                {
                    textSelectionChangedRegistration.Attach(
                        textBox,
                        ref new xaml::RoutedEventHandler(
                            [textSelectionChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"TextBox selection changed.");
                        textSelectionChangedEvent->Set();
                    }));

                    textSelectionChangingRegistration.Attach(
                        textBox,
                        ref new wf::TypedEventHandler<xaml_controls::TextBox^, xaml_controls::TextBoxSelectionChangingEventArgs^>(
                            [&](xaml_controls::TextBox ^sender, xaml_controls::TextBoxSelectionChangingEventArgs^ args)
                    {
                        LOG_OUTPUT(L"TextBox selection changing: start:%d length:%d", args->SelectionStart, args->SelectionLength);
                        VERIFY_ARE_EQUAL(args->SelectionStart, expectedSelectionStart);
                        VERIFY_ARE_EQUAL(args->SelectionLength, expectedSelectionLength);
                        textSelectionChangingEvent->Set();
                        if (cancelSelectionChanging)
                        {
                            args->Cancel = true;
                        }
                        if (appChangeSelection)
                        {
                            textBox->Select(5, 5);
                        }
                    }));
                });

                RunOnUIThread([&]()
                {
                    textBox->Select(11, 0);
                });

                textSelectionChangingEvent->WaitForDefault();
                VERIFY_IS_FALSE(textSelectionChangedEvent->HasFired());

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Selection Start %d, Length:%d", textBox->SelectionStart, textBox->SelectionLength);
                    VERIFY_ARE_EQUAL(textBox->SelectionStart, 0);
                    VERIFY_ARE_EQUAL(textBox->SelectionLength, 0);
                });

                LOG_OUTPUT(L"Test not cancelling the SelectionChanging event from programmatic selection change.");
                cancelSelectionChanging = false;
                expectedSelectionStart = 11;
                expectedSelectionLength = 0;

                textSelectionChangingEvent->Reset();
                RunOnUIThread([&]()
                {
                    textBox->Select(11, 0);
                });

                textSelectionChangingEvent->WaitForDefault();
                textSelectionChangedEvent->WaitForDefault();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Selection Start %d, Length:%d", textBox->SelectionStart, textBox->SelectionLength);
                    VERIFY_ARE_EQUAL(textBox->SelectionStart, 11);
                    VERIFY_ARE_EQUAL(textBox->SelectionLength, 0);
                });

                LOG_OUTPUT(L"Test app changes selection in SelectionChanging event handler.");
                cancelSelectionChanging = true;
                appChangeSelection = true;
                expectedSelectionStart = 0;
                expectedSelectionLength = 11;
                textSelectionChangingEvent->Reset();
                textSelectionChangedEvent->Reset();

                RunOnUIThread([&]()
                {
                    textBox->Select(0, 11);
                });

                textSelectionChangingEvent->WaitForDefault();
                textSelectionChangedEvent->WaitForDefault();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Selection Start %d, Length:%d", textBox->SelectionStart, textBox->SelectionLength);
                    VERIFY_ARE_EQUAL(textBox->SelectionStart, 5);
                    VERIFY_ARE_EQUAL(textBox->SelectionLength, 5);
                });
            }

            void TextBoxSelectionTests::UndoSelectionChangingEvent()
            {
                TestCleanupWrapper cleanup;
                WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

                ::Windows::Foundation::Size size(400, 400);
                TestServices::WindowHelper->SetWindowSizeOverride(size);

                xaml_controls::TextBox^ textBox = nullptr;

                auto textSelectionChangedEvent = std::make_shared<Event>();
                auto textSelectionChangingEvent = std::make_shared<Event>();
                auto textSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, SelectionChanged);
                auto textSelectionChangingRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, SelectionChanging);

                Platform::String ^strHello = "Hello World";
                Platform::String ^strToType = "a";

                RunOnUIThread([&]()
                {
                    auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                        L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                        L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                        L"  <TextBox x:Name='textBox' FontSize='20' Width='180' Margin ='20,5,20,0'/>"
                        L"</StackPanel>"));

                    TestServices::WindowHelper->WindowContent = rootPanel;
                    textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                    VERIFY_IS_NOT_NULL(textBox);
                    textBox->Text = strHello;
                });

                TestServices::WindowHelper->WaitForIdle();
                FocusTestHelper::EnsureFocus(textBox, FocusState::Programmatic);

                LOG_OUTPUT(L"Select All");
                RunOnUIThread([&]()
                {
                    textBox->SelectAll();
                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    textSelectionChangedRegistration.Attach(
                        textBox,
                        ref new xaml::RoutedEventHandler(
                            [textSelectionChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"TextBox selection changed.");
                        textSelectionChangedEvent->Set();
                    }));

                    textSelectionChangingRegistration.Attach(
                        textBox,
                        ref new wf::TypedEventHandler<xaml_controls::TextBox^, xaml_controls::TextBoxSelectionChangingEventArgs^>(
                            [&](xaml_controls::TextBox ^sender, xaml_controls::TextBoxSelectionChangingEventArgs^ args)
                    {
                        LOG_OUTPUT(L"TextBox selection changing: start:%d length:%d", args->SelectionStart, args->SelectionLength);
                        LOG_OUTPUT(L"        SelectedText:%s", textBox->SelectedText->Data());
                        textSelectionChangingEvent->Set();
                        args->Cancel = true;
                    }));
                });

                LOG_OUTPUT(L"Replace the selected text with %s", strToType->Data());
                TestServices::KeyboardHelper->PressKeySequence(strToType);
                textSelectionChangingEvent->WaitForDefault();
                textSelectionChangedEvent->WaitForDefault();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Selection Start %d, Length:%d", textBox->SelectionStart, textBox->SelectionLength);
                    VERIFY_ARE_EQUAL(textBox->SelectionStart, 1);
                    VERIFY_ARE_EQUAL(textBox->SelectionLength, 0);
                });

                LOG_OUTPUT(L"Ctrl+Z...");
                TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_z#$u$_z#$u$_ctrl");
                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Selection Start %d, Length:%d", textBox->SelectionStart, textBox->SelectionLength);
                    LOG_OUTPUT(L"Text:%s", textBox->Text->Data());
                    VERIFY_IS_TRUE(textBox->Text == strHello);
                });

                LOG_OUTPUT(L"Ctrl+Y...");
                TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_y#$u$_y#$u$_ctrl");
                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Selection Start %d, Length:%d", textBox->SelectionStart, textBox->SelectionLength);
                    LOG_OUTPUT(L"Text:%s", textBox->Text->Data());
                    VERIFY_IS_TRUE(textBox->Text == strToType);
                });
            }

            void TextBoxSelectionTests::MouseSelectionChangingEvent()
            {
                TestCleanupWrapper cleanup;
                WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

                ::Windows::Foundation::Size size(400, 400);
                TestServices::WindowHelper->SetWindowSizeOverride(size);

                xaml_controls::TextBox^ textBox = nullptr;
                xaml_controls::Button^ button = nullptr;

                auto textSelectionChangingEvent = std::make_shared<Event>();
                auto textSelectionChangingRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, SelectionChanging);

                RunOnUIThread([&]()
                {
                    auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                        L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                        L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                        L"  <TextBox x:Name='textBox' Text= 'Hello 1234567890' FontSize='20' Width='180' Margin ='20,5,20,0' ContextFlyout='{x:Null}' SelectionFlyout='{x:Null}'/>"
                        L"</StackPanel>"));

                    TestServices::WindowHelper->WindowContent = rootPanel;
                    textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                    button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
                    VERIFY_IS_NOT_NULL(textBox);
                });

                TestServices::WindowHelper->WaitForIdle();
                TestServices::InputHelper->Tap(button);
                TestServices::WindowHelper->WaitForIdle();
                FocusTestHelper::EnsureFocus(textBox, FocusState::Programmatic);

                RunOnUIThread([&]()
                {
                    textBox->Select(1, 2);

                    textSelectionChangingRegistration.Attach(
                        textBox,
                        ref new wf::TypedEventHandler<xaml_controls::TextBox^, xaml_controls::TextBoxSelectionChangingEventArgs^>(
                            [&](xaml_controls::TextBox ^sender, xaml_controls::TextBoxSelectionChangingEventArgs^ args)
                    {
                        LOG_OUTPUT(L"TextBox selection changing: start:%d length:%d", args->SelectionStart, args->SelectionLength);
                        textSelectionChangingEvent->Set();
                        args->Cancel = true;
                    }));
                });

                TestServices::WindowHelper->WaitForIdle();
                TestServices::InputHelper->LeftMouseClick(textBox);
                TestServices::WindowHelper->WaitForIdle();
                TestServices::InputHelper->DragFromCenter(textBox, -100/*relX*/, -60 /*relY*/, 0.1 /*velocityFactor*/);
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Selection Start %d, Length:%d", textBox->SelectionStart, textBox->SelectionLength);
                    VERIFY_ARE_EQUAL(textBox->SelectionStart, 1);
                    VERIFY_ARE_EQUAL(textBox->SelectionLength, 2);
                });
            }

            void TextBoxSelectionTests::TouchSelectionChangingEvent()
            {
                TestCleanupWrapper cleanup;
                WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

                ::Windows::Foundation::Size size(400, 400);
                TestServices::WindowHelper->SetWindowSizeOverride(size);

                xaml_controls::TextBox^ textBox = nullptr;

                auto textSelectionChangingEvent = std::make_shared<Event>();
                auto textSelectionChangingRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, SelectionChanging);

                RunOnUIThread([&]()
                {
                    auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                        L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                        L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                        L"  <TextBox x:Name='textBox' Text= '123 456 789 abc def' FontSize='20' Width='180' Margin ='20,5,20,0' ContextFlyout='{x:Null}' SelectionFlyout='{x:Null}'/>"
                        L"</StackPanel>"));

                    TestServices::WindowHelper->WindowContent = rootPanel;
                    textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                    VERIFY_IS_NOT_NULL(textBox);
                });

                TestServices::WindowHelper->WaitForIdle();
                FocusTestHelper::EnsureFocus(textBox, FocusState::Programmatic);
                bool cancelEvent = true;
                RunOnUIThread([&]()
                {
                    textSelectionChangingRegistration.Attach(
                        textBox,
                        ref new wf::TypedEventHandler<xaml_controls::TextBox^, xaml_controls::TextBoxSelectionChangingEventArgs^>(
                            [&](xaml_controls::TextBox ^sender, xaml_controls::TextBoxSelectionChangingEventArgs^ args)
                    {
                        LOG_OUTPUT(L"TextBox selection changing: start:%d length:%d", args->SelectionStart, args->SelectionLength);
                        textSelectionChangingEvent->Set();
                        if (cancelEvent)
                        {
                            args->Cancel = true;
                        }
                    }));
                });

                TestServices::WindowHelper->WaitForIdle();
                LOG_OUTPUT(L"Testing selectionChanging cancellation following gripper tap.");
                TestServices::InputHelper->Tap(textBox);
                textSelectionChangingEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                cancelEvent = false;
                textSelectionChangingEvent->Reset();
                RunOnUIThread([&]()
                {
                    textBox->Select(5, 3);
                });
                textSelectionChangingEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                cancelEvent = true;
                xaml::Shapes::Rectangle^ gripperRect = GetGripperRect(textBox);
                LOG_OUTPUT(L"Testing selectionChanging cancellation following gripper manipulation.");
                TestServices::InputHelper->PanFromCenter(gripperRect, 100 /*relX*/, 0 /*relY*/, 0.1 /*velocityFactor*/);
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"At end of test: Selection Start %d, Length:%d", textBox->SelectionStart, textBox->SelectionLength);
                    VERIFY_ARE_EQUAL(textBox->SelectionStart, 5);
                    VERIFY_ARE_EQUAL(textBox->SelectionLength, 3);
                });
            }

            void TextBoxSelectionTests::AppendOnlyTextBoxUsingSelectionChangingEvent()
            {
                TestCleanupWrapper cleanup;
                WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

                ::Windows::Foundation::Size size(400, 400);
                TestServices::WindowHelper->SetWindowSizeOverride(size);

                xaml_controls::TextBox^ textBox = nullptr;

                auto textSelectionChangingRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, SelectionChanging);

                RunOnUIThread([&]()
                {
                    auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                        L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                        L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                        L"  <TextBox x:Name='textBox' Text= 'Hello' FontSize='20' Width='180' Margin ='20,5,20,0' ContextFlyout='{x:Null}' SelectionFlyout='{x:Null}'/>"
                        L"</StackPanel>"));

                    TestServices::WindowHelper->WindowContent = rootPanel;
                    textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                    VERIFY_IS_NOT_NULL(textBox);
                });

                TestServices::WindowHelper->WaitForIdle();
                FocusTestHelper::EnsureFocus(textBox, FocusState::Programmatic);
                RunOnUIThread([&]()
                {
                    textBox->Select(textBox->Text->Length(), 0);
                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    textSelectionChangingRegistration.Attach(
                        textBox,
                        ref new wf::TypedEventHandler<xaml_controls::TextBox^, xaml_controls::TextBoxSelectionChangingEventArgs^>(
                            [&](xaml_controls::TextBox ^sender, xaml_controls::TextBoxSelectionChangingEventArgs^ args)
                    {
                        LOG_OUTPUT(L"TextBox selection changing: start:%d length:%d", args->SelectionStart, args->SelectionLength);
                        LOG_OUTPUT(L"TextBox Text:[%s] length:%d", textBox->Text->Data(), textBox->Text->Length());

                        // Note: For UWP / TSF3, the comparison below was originally SelectionStart != Text->Length()
                        // But in Win32 / TSF1, during text enry SelectionStart can actually be larger than Text->Length(),
                        // so we do a < comparison instead.
                        if (static_cast<UINT>(args->SelectionStart) < textBox->Text->Length() || args->SelectionLength != 0)
                        {
                            LOG_OUTPUT(L"Cancelling SelectionChanging event.");
                            args->Cancel = true;
                        }
                    }));
                });

                TestServices::WindowHelper->WaitForIdle();
                TestServices::KeyboardHelper->PressKeySequence(" world");
                TestServices::WindowHelper->WaitForIdle();

                TestServices::KeyboardHelper->Left();
                TestServices::KeyboardHelper->Left();
                TestServices::KeyboardHelper->Left();
                TestServices::KeyboardHelper->Left();
                TestServices::WindowHelper->WaitForIdle();

                TestServices::KeyboardHelper->Right();
                TestServices::KeyboardHelper->Right();
                TestServices::WindowHelper->WaitForIdle();

                TestServices::KeyboardHelper->Backspace();
                TestServices::WindowHelper->WaitForIdle();

                TestServices::InputHelper->LeftMouseClick(textBox);
                TestServices::WindowHelper->WaitForIdle();
                TestServices::KeyboardHelper->PressKeySequence("XYZ");

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"At end of test: Selection Start %d, Length:%d", textBox->SelectionStart, textBox->SelectionLength);
                    LOG_OUTPUT(L"TextBox Text:[%s] length:%d", textBox->Text->Data(), textBox->Text->Length());
                    VERIFY_ARE_EQUAL(static_cast<int>(textBox->Text->Length()), 13);
                    VERIFY_ARE_EQUAL(textBox->SelectionStart, 13);
                    VERIFY_ARE_EQUAL(textBox->SelectionLength, 0);
                });
            }

            void TextBoxSelectionTests::AppendOnlyRichEditBoxUsingSelectionChangingEvent()
            {
                TestCleanupWrapper cleanup;
                WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

                ::Windows::Foundation::Size size(400, 400);
                TestServices::WindowHelper->SetWindowSizeOverride(size);

                xaml_controls::RichEditBox^ reBox = nullptr;

                auto textSelectionChangingRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, SelectionChanging);

                RunOnUIThread([&]()
                {
                    auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                        L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                        L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                        L"  <RichEditBox x:Name='reBox' FontSize='20' Width='180' Margin ='20,5,20,0' ContextFlyout='{x:Null}' SelectionFlyout='{x:Null}'/>"
                        L"</StackPanel>"));

                    TestServices::WindowHelper->WindowContent = rootPanel;
                    reBox = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"reBox"));
                    VERIFY_IS_NOT_NULL(reBox);
                });

                TestServices::WindowHelper->WaitForIdle();
                FocusTestHelper::EnsureFocus(reBox, FocusState::Programmatic);
                RunOnUIThread([&]()
                {
                    reBox->Document->SetText(mut::TextSetOptions::None, L"Hello");
                    reBox->Document->Selection->StartPosition = 5;
                    reBox->Document->Selection->EndPosition = 5;
                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    textSelectionChangingRegistration.Attach(
                        reBox,
                        ref new wf::TypedEventHandler<xaml_controls::RichEditBox^, xaml_controls::RichEditBoxSelectionChangingEventArgs^>(
                            [&](xaml_controls::RichEditBox ^sender, xaml_controls::RichEditBoxSelectionChangingEventArgs^ args)
                    {
                        Platform::String^ text;
                        reBox->Document->GetText(mut::TextGetOptions::None, &text);

                        LOG_OUTPUT(L"RichEditBox selection changing: start:%d length:%d", args->SelectionStart, args->SelectionLength);
                        LOG_OUTPUT(L"RichEditBox Text:[%s] length:%d", text->Data(), text->Length());

                        if (static_cast<UINT>(args->SelectionStart) != text->Length() - 1 || args->SelectionLength != 0)
                        {
                            LOG_OUTPUT(L"Cancelling SelectionChanging event.");
                            args->Cancel = true;
                        }
                    }));
                });

                TestServices::WindowHelper->WaitForIdle();
                TestServices::KeyboardHelper->PressKeySequence(" world");
                TestServices::WindowHelper->WaitForIdle();

                TestServices::KeyboardHelper->Left();
                TestServices::KeyboardHelper->Left();
                TestServices::KeyboardHelper->Left();
                TestServices::KeyboardHelper->Left();
                TestServices::WindowHelper->WaitForIdle();

                TestServices::KeyboardHelper->Right();
                TestServices::KeyboardHelper->Right();
                TestServices::WindowHelper->WaitForIdle();

                TestServices::KeyboardHelper->Backspace();
                TestServices::WindowHelper->WaitForIdle();

                TestServices::InputHelper->LeftMouseClick(reBox);
                TestServices::WindowHelper->WaitForIdle();
                TestServices::KeyboardHelper->PressKeySequence("XYZ");

                RunOnUIThread([&]()
                {
                    Platform::String^ text;
                    reBox->Document->GetText(mut::TextGetOptions::None, &text);

                    LOG_OUTPUT(L"At end of test: Selection Start %d, End:%d", reBox->Document->Selection->StartPosition, reBox->Document->Selection->EndPosition);
                    LOG_OUTPUT(L"RichEditBox Text:[%s] length:%d", text->Data(), text->Length());
                    VERIFY_ARE_EQUAL(reBox->Document->Selection->StartPosition, 13);
                    VERIFY_ARE_EQUAL(reBox->Document->Selection->EndPosition, 13);
                });
            }

            void TextBoxSelectionTests::VerifyTextBoxCanPasteClipboardContent()
            {
                TestCleanupWrapper cleanup;

                ::Windows::Foundation::Size size(400, 400);
                TestServices::WindowHelper->SetWindowSizeOverride(size);

                Platform::String ^toReplaceWith = "PastedFromClipboard";
                auto tb1GotFocusEvent = std::make_shared<Event>();
                auto tb1GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);
                auto tb2GotFocusEvent = std::make_shared<Event>();
                auto tb2GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);
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
                    tb2 = ref new xaml_controls::TextBox();
                    tb2->Text = "Random";
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

                    tb2GotFocusRegistration.Attach(
                        tb2,
                        ref new xaml::RoutedEventHandler(
                            [tb2GotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"tb2 GotFocus handler");
                        tb2GotFocusEvent->Set();
                    }));

                    tb2TextChangedRegistration.Attach(
                        tb2,
                        ref new xaml_controls::TextChangedEventHandler(
                            [tb2TextChangedEvent](Platform::Object^, xaml_controls::TextChangedEventArgs^)
                    {
                        LOG_OUTPUT(L"tb2 TextChanged handler");
                        tb2TextChangedEvent->Set();
                    }));
                });

                TestServices::WindowHelper->WaitForIdle();

                auto clipboardBitmapSet = std::make_shared<Event>();
                auto resourcesPath = GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\image\\";
                create_task(StorageFile::GetFileFromPathAsync(resourcesPath + L"Smiley.bmp"))
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

                RunOnUIThread([&]()
                {
                    tb2->Focus(FocusState::Pointer);
                });

                tb2GotFocusEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"tb2->CanPasteClipboardContent");
                    VERIFY_IS_FALSE(tb2->CanPasteClipboardContent);
                });

                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    tb1->Focus(FocusState::Pointer);
                    LOG_OUTPUT(L"tb1->SelectAll()");
                    tb1->SelectAll();
                });

                tb1GotFocusEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                LOG_OUTPUT(L"Press Ctrl+C");
                TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_c#$u$_c#$u$_ctrl"); // Ctrl+C
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    tb2->Focus(FocusState::Pointer);
                });

                tb2GotFocusEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"tb2->CanPasteClipboardContent");
                    VERIFY_IS_TRUE(tb2->CanPasteClipboardContent);
                });

                TestServices::WindowHelper->WaitForIdle();
            }

            void TextBoxSelectionTests::VerifyTextBoxPasteFromClipboard()
            {
                TestCleanupWrapper cleanup;

                ::Windows::Foundation::Size size(400, 400);
                TestServices::WindowHelper->SetWindowSizeOverride(size);

                Platform::String ^toReplaceWith = "PastedFromClipboard";
                auto tb1GotFocusEvent = std::make_shared<Event>();
                auto tb1GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);
                auto tb2GotFocusEvent = std::make_shared<Event>();
                auto tb2GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);
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
                    tb2 = ref new xaml_controls::TextBox();
                    tb2->Text = "Random";
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

                    tb2GotFocusRegistration.Attach(
                        tb2,
                        ref new xaml::RoutedEventHandler(
                            [tb2GotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"tb2 GotFocus handler");
                        tb2GotFocusEvent->Set();
                    }));

                    tb2TextChangedRegistration.Attach(
                        tb2,
                        ref new xaml_controls::TextChangedEventHandler(
                            [tb2TextChangedEvent](Platform::Object^, xaml_controls::TextChangedEventArgs^)
                    {
                        LOG_OUTPUT(L"tb2 TextChanged handler");
                        tb2TextChangedEvent->Set();
                    }));
                });

                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    tb1->Focus(FocusState::Pointer);
                    LOG_OUTPUT(L"tb1->SelectAll()");
                    tb1->SelectAll();
                });

                tb1GotFocusEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                LOG_OUTPUT(L"Press Ctrl+C");
                TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_c#$u$_c#$u$_ctrl"); // Ctrl+C
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    tb2->Focus(FocusState::Pointer);
                    LOG_OUTPUT(L"tb2->SelectAll()");
                    tb2->SelectAll();
                });

                tb2GotFocusEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"tb2->PasteFromClipboard()");
                    tb2->PasteFromClipboard();
                });

                tb2TextChangedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    // tb2 should have the tb1 Text now if copy/paste are successful
                    VERIFY_ARE_EQUAL(toReplaceWith, tb2->Text);
                });

                TestServices::WindowHelper->WaitForIdle();
            }

            void TextBoxSelectionTests::VerifyTextBoxCopySelectionToClipboard()
            {
                TestCleanupWrapper cleanup;

                ::Windows::Foundation::Size size(400, 400);
                TestServices::WindowHelper->SetWindowSizeOverride(size);

                Platform::String^ toReplaceWith = "PastedFromClipboard";
                auto tb1GotFocusEvent = std::make_shared<Event>();
                auto tb1GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);
                auto tb2GotFocusEvent = std::make_shared<Event>();
                auto tb2GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);
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
                    tb2 = ref new xaml_controls::TextBox();
                    tb2->Text = "Random";
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

                    tb2GotFocusRegistration.Attach(
                        tb2,
                        ref new xaml::RoutedEventHandler(
                            [tb2GotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"tb2 GotFocus handler");
                        tb2GotFocusEvent->Set();
                    }));

                    tb2TextChangedRegistration.Attach(
                        tb2,
                        ref new xaml_controls::TextChangedEventHandler(
                            [tb2TextChangedEvent](Platform::Object^, xaml_controls::TextChangedEventArgs^)
                    {
                        LOG_OUTPUT(L"tb2 TextChanged handler");
                        tb2TextChangedEvent->Set();
                    }));
                });

                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    tb1->Focus(FocusState::Pointer);
                    LOG_OUTPUT(L"tb1->SelectAll()");
                    tb1->SelectAll();
                });

                tb1GotFocusEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                ClipboardHelper clipboardHelper;

                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"tb1->CopySelectionToClipboard()");
                    tb1->CopySelectionToClipboard();
                });

                TestServices::WindowHelper->WaitForIdle();

                clipboardHelper.VerifyClipboardText(toReplaceWith);
                clipboardHelper.ResetContentChangedEvent();

                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    // tb1 text shouldn't have changed on copy
                    VERIFY_ARE_EQUAL(toReplaceWith, tb1->Text);
                });

                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    tb2->Focus(FocusState::Pointer);
                    LOG_OUTPUT(L"tb2->SelectAll()");
                    tb2->SelectAll();
                });

                tb2GotFocusEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"tb2->PasteFromClipboard()");
                    tb2->PasteFromClipboard();
                });

                tb2TextChangedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    // tb2 should have the tb1 Text now if copy/paste are successful
                    VERIFY_ARE_EQUAL(toReplaceWith, tb2->Text);
                });

                TestServices::WindowHelper->WaitForIdle();
            }

            void TextBoxSelectionTests::VerifyTextBoxCutSelectionToClipboard()
            {
                TestCleanupWrapper cleanup;

                ::Windows::Foundation::Size size(400, 400);
                TestServices::WindowHelper->SetWindowSizeOverride(size);

                Platform::String ^toReplaceWith = "PastedFromClipboard";
                auto tb1GotFocusEvent = std::make_shared<Event>();
                auto tb1GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);
                auto tb1TextChangedEvent = std::make_shared<Event>();
                auto tb1TextChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, TextChanged);

                auto tb2GotFocusEvent = std::make_shared<Event>();
                auto tb2GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);
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
                    tb2 = ref new xaml_controls::TextBox();
                    tb2->Text = "Random";
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

                    tb2GotFocusRegistration.Attach(
                        tb2,
                        ref new xaml::RoutedEventHandler(
                            [tb2GotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"tb2 GotFocus handler");
                        tb2GotFocusEvent->Set();
                    }));

                    tb1TextChangedRegistration.Attach(
                        tb1,
                        ref new xaml_controls::TextChangedEventHandler(
                            [tb1TextChangedEvent](Platform::Object^, xaml_controls::TextChangedEventArgs^)
                    {
                        LOG_OUTPUT(L"tb1 TextChanged handler");
                        tb1TextChangedEvent->Set();
                    }));

                    tb2TextChangedRegistration.Attach(
                        tb2,
                        ref new xaml_controls::TextChangedEventHandler(
                            [tb2TextChangedEvent](Platform::Object^, xaml_controls::TextChangedEventArgs^)
                    {
                        LOG_OUTPUT(L"tb2 TextChanged handler");
                        tb2TextChangedEvent->Set();
                    }));
                });

                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    tb1->Focus(FocusState::Pointer);
                    LOG_OUTPUT(L"tb1->SelectAll()");
                    tb1->SelectAll();
                });

                tb1GotFocusEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                ClipboardHelper clipboardHelper;

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"tb1->CutSelectionToClipboard()");
                    tb1->CutSelectionToClipboard();
                });

                tb1TextChangedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                clipboardHelper.VerifyClipboardText(toReplaceWith);
                clipboardHelper.ResetContentChangedEvent();

                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    // cut should have cleared tb1's text
                    VERIFY_IS_TRUE(tb1->Text->IsEmpty());
                });

                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    tb2->Focus(FocusState::Pointer);
                    LOG_OUTPUT(L"tb2->SelectAll()");
                    tb2->SelectAll();
                });

                tb2GotFocusEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"tb2->PasteFromClipboard()");
                    tb2->PasteFromClipboard();
                });

                tb2TextChangedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    // tb2 should have the tb1 Text now if copy/paste are successful
                    VERIFY_ARE_EQUAL(toReplaceWith, tb2->Text);
                });

                TestServices::WindowHelper->WaitForIdle();
            }

            void TextBoxSelectionTests::VerifyTextBoxUndo()
            {
                TestCleanupWrapper cleanup;

                ::Windows::Foundation::Size size(400, 400);
                TestServices::WindowHelper->SetWindowSizeOverride(size);

                Platform::String ^toReplaceWith = "Text goes here";
                auto tb1GotFocusEvent = std::make_shared<Event>();
                auto tb1GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);

                auto tb2GotFocusEvent = std::make_shared<Event>();
                auto tb2GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);

                auto tb1TextChangedEvent = std::make_shared<Event>();
                auto tb1TextChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, TextChanged);

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
                    tb2 = ref new xaml_controls::TextBox();
                    tb1->Text = toReplaceWith;
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
                        ref new xaml_controls::TextChangedEventHandler(
                            [tb1TextChangedEvent](Platform::Object^, xaml_controls::TextChangedEventArgs^)
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
                        LOG_OUTPUT(L"tb2->Undo()");
                        tb2->Undo();
                    }
                    catch (...)
                    {
                        VERIFY_IS_TRUE(false, L"Undo on default textbox threw an exception.");
                    }
                });

                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    tb1->Focus(FocusState::Pointer);
                    LOG_OUTPUT(L"tb1->SelectAll()");
                    tb1->SelectAll();
                });

                tb1GotFocusEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"tb1->CutSelectionToClipboard()");
                    tb1->CutSelectionToClipboard();
                });

                tb1TextChangedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    // cut should have cleared tb1's text
                    VERIFY_IS_TRUE(tb1->Text->IsEmpty());
                });

                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"tb1->Undo()");
                    tb1->Undo();
                });

                tb1TextChangedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    // tb1's text should have been restored if undo was successful
                    VERIFY_ARE_EQUAL(toReplaceWith, tb1->Text);
                });

                TestServices::WindowHelper->WaitForIdle();

                // Redo by pressing control + y;
                LOG_OUTPUT(L"Press Ctrl+Y (Redo)");
                TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_y#$u$_y#$u$_ctrl");

                tb1TextChangedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    // Redo should have cleared tb1's text
                    VERIFY_IS_TRUE(tb1->Text->IsEmpty());
                });

                TestServices::WindowHelper->WaitForIdle();

                // Paste by pressing control + v;
                LOG_OUTPUT(L"Press Ctrl+V (Paste)");
                TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_v#$u$_v#$u$_ctrl");

                TestServices::WindowHelper->WaitForIdle();

                tb1TextChangedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    // tb1's text should have been restored if paste was successful
                    VERIFY_ARE_EQUAL(toReplaceWith, tb1->Text);
                });

                TestServices::WindowHelper->WaitForIdle();

                TestServices::KeyboardHelper->PressKeySequence(" abc");

                tb1TextChangedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    // tb1's text should have been restored if paste was successful
                    VERIFY_ARE_EQUAL(ref new Platform::String(L"Text goes here abc"), tb1->Text);
                });

                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"tb1->Undo()");
                    tb1->Undo();
                });

                tb1TextChangedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    // tb1's text should have been restored if KeyPress Undo was successful
                    VERIFY_ARE_EQUAL(toReplaceWith, tb1->Text);
                });

                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"tb1->Undo()");
                    tb1->Undo();
                });

                tb1TextChangedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    // Undo should have cleared tb1's text
                    VERIFY_IS_TRUE(tb1->Text->IsEmpty());
                });

                TestServices::WindowHelper->WaitForIdle();
            }

            void TextBoxSelectionTests::VerifyTextBoxRedo()
            {
                TestCleanupWrapper cleanup;

                ::Windows::Foundation::Size size(400, 400);
                TestServices::WindowHelper->SetWindowSizeOverride(size);

                Platform::String ^toReplaceWith = "Text goes here";
                auto tb1GotFocusEvent = std::make_shared<Event>();
                auto tb1GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);

                auto tb2GotFocusEvent = std::make_shared<Event>();
                auto tb2GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);

                auto tb1TextChangedEvent = std::make_shared<Event>();
                auto tb1TextChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, TextChanged);

                auto tb1SelectedChangedEvent = std::make_shared<Event>();
                auto tb1SelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, SelectionChanged);

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
                    tb2 = ref new xaml_controls::TextBox();
                    tb1->Text = toReplaceWith;
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
                        ref new xaml_controls::TextChangedEventHandler(
                            [tb1TextChangedEvent](Platform::Object^, xaml_controls::TextChangedEventArgs^)
                    {
                        LOG_OUTPUT(L"tb1 TextChanged handler");
                        tb1TextChangedEvent->Set();
                    }));

                    tb1SelectionChangedRegistration.Attach(
                        tb1,
                        ref new xaml::RoutedEventHandler(
                            [tb1SelectedChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"TextBox selection changed.");
                        tb1SelectedChangedEvent->Set();
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
                        // Calling Redo on unaltered/default textbox
                        LOG_OUTPUT(L"tb2->Redo()");
                        tb2->Redo();
                    }
                    catch (...)
                    {
                        VERIFY_IS_TRUE(false, L"Redo on default textbox threw an exception.");
                    }
                });

                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    tb1->Focus(FocusState::Pointer);
                    LOG_OUTPUT(L"tb1->SelectAll()");
                    tb1->SelectAll();
                });

                tb1GotFocusEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"tb1->CutSelectionToClipboard()");
                    tb1->CutSelectionToClipboard();
                });

                tb1TextChangedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    // cut should have cleared tb1's text
                    VERIFY_IS_TRUE(tb1->Text->IsEmpty());
                });

                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"tb1->Undo()");
                    tb1->Undo();
                });

                tb1TextChangedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    // tb1's should have the restored if undo was successful
                    VERIFY_ARE_EQUAL(toReplaceWith, tb1->Text);
                });

                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"tb1->Redo()");
                    tb1->Redo();
                });

                tb1TextChangedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    // Redo should have cleared tb1's text
                    VERIFY_IS_TRUE(tb1->Text->IsEmpty());
                });

                TestServices::WindowHelper->WaitForIdle();

                // Undo by pressing control + z;
                LOG_OUTPUT(L"Press Ctrl+Z (Undo)");
                TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_z#$u$_z#$u$_ctrl");

                tb1TextChangedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    // tb1's should have the restored if undo was successful
                    VERIFY_ARE_EQUAL(toReplaceWith, tb1->Text);
                });

                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"tb1->Redo()");
                    tb1->Redo();
                });

                tb1TextChangedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    // Redo should have cleared tb1's text
                    VERIFY_IS_TRUE(tb1->Text->IsEmpty());
                });

                TestServices::WindowHelper->WaitForIdle();
            }

            void TextBoxSelectionTests::VerifyTextBoxClearUndoRedoHistory()
            {
                TestCleanupWrapper cleanup;

                ::Windows::Foundation::Size size(400, 400);
                TestServices::WindowHelper->SetWindowSizeOverride(size);

                Platform::String ^text  = "Text goes here";
                auto tb1GotFocusEvent = std::make_shared<Event>();
                auto tb1GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);

                auto tb2GotFocusEvent = std::make_shared<Event>();
                auto tb2GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);

                auto tb1TextChangedEvent = std::make_shared<Event>();
                auto tb1TextChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, TextChanged);

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
                    tb2 = ref new xaml_controls::TextBox();
                    tb1->Text = text;
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
                        ref new xaml_controls::TextChangedEventHandler(
                            [tb1TextChangedEvent](Platform::Object^, xaml_controls::TextChangedEventArgs^)
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
                        // Calling ClearUndoRedoHistory on untouched/default textbox
                        LOG_OUTPUT(L"tb2->ClearUndoRedoHistory()");
                        tb2->ClearUndoRedoHistory();
                    }
                    catch (...)
                    {
                        VERIFY_IS_TRUE(false, L"ClearUndoRedoHistory on default textbox threw an exception.");
                    }
                });

                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    tb1->Focus(FocusState::Pointer);
                    LOG_OUTPUT(L"tb1->SelectAll()");
                    tb1->SelectAll();
                });

                tb1GotFocusEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"tb1->CutSelectionToClipboard()");
                    tb1->CutSelectionToClipboard();
                });

                tb1TextChangedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    // cut should have cleared tb1's text
                    VERIFY_IS_TRUE(tb1->Text->IsEmpty());
                });

                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"tb1->ClearUndoRedoHistory()");
                    tb1->ClearUndoRedoHistory();
                    tb1TextChangedEvent->Reset();
                    LOG_OUTPUT(L"tb1->Undo()");
                    tb1->Undo();
                });

                TestServices::WindowHelper->WaitForIdle();
                VERIFY_IS_FALSE(tb1TextChangedEvent->HasFired());

                RunOnUIThread([&]()
                {
                    // tb1's text should NOT have been restored if ClearUndoRedoHistory was successful
                    VERIFY_IS_TRUE(tb1->Text->IsEmpty());
                });

                TestServices::WindowHelper->WaitForIdle();
            }

            xaml::Shapes::Rectangle^ TextBoxSelectionTests::GetGripperRect(UIElement^ textElement)
            {
                xaml::Shapes::Rectangle^ gripperRect;
                const int waitforGripperRetry = 10;
                for (int retry = 0; retry < waitforGripperRetry && gripperRect == nullptr; retry++)
                {
                    RunOnUIThread([&]()
                    {
                        auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(textElement->XamlRoot);

                        for (auto popup : popups)
                        {
                            auto popupChild = safe_cast<xaml::FrameworkElement^>(popup->Child);
                            gripperRect = TreeHelper::GetVisualChildByType<xaml::Shapes::Rectangle>(popupChild);
                            if (gripperRect != nullptr)
                            {
                                break;
                            }
                        }
                    });
                    TestServices::WindowHelper->WaitForIdle();
                }

                VERIFY_IS_NOT_NULL(gripperRect);
                return gripperRect;
            }
        }
        }


} } } }
