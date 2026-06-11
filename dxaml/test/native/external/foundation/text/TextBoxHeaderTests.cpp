// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TextBoxHeaderTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <ppltasks.h>
#include "FileLoader.h"
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <WUCRenderingScopeGuard.h>
#include <FocusTestHelper.h>
#include <RuntimeEnabledFeatureOverride.h>
#include "FeatureFlags.h"

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace MockDComp;
using namespace Microsoft::UI::Xaml::Input;


namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation {
        namespace Text {

            bool TextBoxHeaderTests::ClassSetup()
            {
                CommonTestSetupHelper::CommonTestClassSetup();
                 return true;
            }

            bool TextBoxHeaderTests::ClassCleanup()
            {
                 return true;
            }

            bool TextBoxHeaderTests::TestSetup()
            {
                test_infra::TestServices::WindowHelper->InitializeXaml();
                return true;
            }

            bool TextBoxHeaderTests::TestCleanup()
            {
                test_infra::TestServices::WindowHelper->ShutdownXaml();
                TestServices::WindowHelper->VerifyTestCleanup();
                return true;
            }

            void TextBoxHeaderTests::UpdateTextBoxHeader()
            {
                WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
                ::Windows::Foundation::Size size(400, 400);
                TestServices::WindowHelper->SetWindowSizeOverride(size);

                xaml_controls::TextBox^ textBox = nullptr;
                xaml_controls::TextBlock^ headerBlock = nullptr;

                auto rootLoadedEvent = std::make_shared<Event>();
                auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);

                RunOnUIThread([&]()
                {
                    auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                        L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                        // button here is purely for taking the focus when test is initialized, otherwise caret may cause DComp dump visual ID mismatch
                        L"  <Button x:Name = 'button' Content = 'Discard SIP' Margin = '20,40,20,0' />"
                        L"  <TextBox x:Name='textBox' FontSize='20' Width='200' Margin ='20,5,20,0'>"
                        L"     <TextBox.Header>"
                        L"         <TextBlock x:Name='textBlock' Text = 'TextBlock Header' />"
                        L"     </TextBox.Header>"
                        L"  </TextBox>"
                        L"</StackPanel>"));
                    textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                    VERIFY_IS_NOT_NULL(textBox);
                    headerBlock = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"textBlock"));
                    VERIFY_IS_NOT_NULL(headerBlock);
                    
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
                rootLoadedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");
                //update text of textbox's header textblock
                RunOnUIThread([&]()
                {
                    headerBlock->Text = L"Updated Block Text";
                });
                TestServices::WindowHelper->WaitForIdle();
                TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

                //Use header property
                RunOnUIThread([&]()
                {
                    textBox->Header = L"Header";
                });
                TestServices::WindowHelper->WaitForIdle();
                TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, "3");

#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
                //Set header placement to left
                RunOnUIThread([&]()
                {
                    textBox->HeaderPlacement = xaml_controls::ControlHeaderPlacement::Left;
                });
                TestServices::WindowHelper->WaitForIdle();
                TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, "4");
#endif

                //Set header property to blank string
                RunOnUIThread([&]()
                {
                    textBox->Header = L"";
                });
                TestServices::WindowHelper->WaitForIdle();
                TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, "5");

                RunOnUIThread([&]()
                {
                    textBox->IsEnabled = false;
                });
                TestServices::WindowHelper->WaitForIdle();
            }

        void TextBoxHeaderTests::TextBoxHeaderScrollsIntoView()
        {
            // Hide the textbox caret so it doesn't interfere with UIElement tree comparison
            RuntimeEnabledFeatureOverride disableTextBoxCaret(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableTextBoxCaret, true);
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::TextBox^ txb1 = nullptr;
            xaml_controls::TextBox^ txb2 = nullptr;
            xaml_controls::TextBox^ txb3 = nullptr;

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Loading rootpanel");

                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Margin='20,20,20,20' Width='200' Height='400' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <ScrollViewer Height='200' Width='200' VerticalScrollBarVisibility='Hidden'>"
                    L"    <StackPanel>"
                    L"      <TextBox x:Name='txb1' Height='100' Width='200' Header='TextBox 1'/>"
                    L"      <TextBox x:Name='txb2' Height='100' Width='200' Header='TextBox 2'/>"
                    L"      <TextBox x:Name='txb3' Height='100' Width='200' Header='TextBox 3'/>"
                    L"    </StackPanel>"
                    L"  </ScrollViewer> "
                    L"</StackPanel>"));
                txb1 = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"txb1"));
                txb2 = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"txb2"));
                txb3 = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"txb3"));

                VERIFY_IS_NOT_NULL(txb1);
                VERIFY_IS_NOT_NULL(txb2);
                VERIFY_IS_NOT_NULL(txb3);

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Setting Focus on txb3");
            FocusTestHelper::EnsureFocus(txb3, FocusState::Keyboard);
            LOG_OUTPUT(L"Moving focus to txb2");
            TestServices::KeyboardHelper->ShiftTab();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(txb2));
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::KeyboardHelper->ShiftTab();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(txb1));
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyUIElementTree();
        }

        void TextBoxHeaderTests::TextBoxHeaderDoesNotScrollIntoViewWhenViewPortIsSmall()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::TextBox^ txb1 = nullptr;
            xaml_controls::TextBox^ txb2 = nullptr;
            xaml_controls::TextBox^ txb3 = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Margin='20,20,20,20' Width='200' Height='400' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <ScrollViewer Height='35' Width='200' VerticalScrollBarVisibility='Hidden'>"
                    L"    <StackPanel>"
                    L"      <TextBox x:Name='txb1' Height='100' Width='200' Header='TextBox 1'/>"
                    L"      <TextBox x:Name='txb2' Height='100' Width='200' Header='TextBox 2'/>"
                    L"      <TextBox x:Name='txb3' Height='100' Width='200' Header='TextBox 3'/>"
                    L"    </StackPanel>"
                    L"  </ScrollViewer> "
                    L"</StackPanel>"));
                txb1 = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"txb1"));
                txb2 = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"txb2"));
                txb3 = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"txb3"));

                VERIFY_IS_NOT_NULL(txb1);
                VERIFY_IS_NOT_NULL(txb2);
                VERIFY_IS_NOT_NULL(txb3);

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Setting Focus on txb3");
            FocusTestHelper::EnsureFocus(txb3, FocusState::Keyboard);
            LOG_OUTPUT(L"Moving focus to txb2");
            TestServices::KeyboardHelper->ShiftTab();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(txb2));
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::KeyboardHelper->ShiftTab();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(txb1));
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyUIElementTree();
        }

        void TextBoxHeaderTests::TextBoxHeaderDoesNotScrollIntoViewIfNoGotFocus()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::TextBox^ txb1 = nullptr;
            auto gotFocusRegistration = CreateSafeEventRegistration(UIElement, GotFocus);
            auto gotFocusEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Loading rootpanel");

                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Margin='20,20,20,20' Width='200' Height='400' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <ScrollViewer Height='100' Width='200' VerticalScrollBarVisibility='Hidden'>"
                    L"    <StackPanel>"
                    L"      <TextBox x:Name='txb1' Height='800' Width='200' Header='TextBox 1'/>"
                    L"    </StackPanel>"
                    L"  </ScrollViewer> "
                    L"</StackPanel>"));
                txb1 = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"txb1"));

                txb1->Text = L"a \n b \n c \\n d \n e \n f \n g \n h \n i \n j \n k \n l \n l \n m \n n \n o \n p \n";

                VERIFY_IS_NOT_NULL(txb1);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Setting Focus on txb1");
            FocusTestHelper::EnsureFocus(txb1, FocusState::Pointer);

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Moving cursor.");
            TestServices::KeyboardHelper->PageDown();
            TestServices::KeyboardHelper->PageDown();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                gotFocusRegistration.Attach(txb1, ref new RoutedEventHandler([gotFocusEvent](Platform::Object^ sender, RoutedEventArgs^ e)
                {
                    LOG_OUTPUT(L"Got Focus Event Fired");
                    VERIFY_FAIL();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->PanFromCenter(txb1, 0 /*relX*/, -60 /*relY*/, 0.1 /*velocityFactor*/);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyUIElementTree();
        }
      }
    }
} } } }
