// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TextBoxKeyInput.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <ppltasks.h>
#include "FileLoader.h"
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include "KeyboardInjectionOverride.h"
#include "FocusTestHelper.h"
#include <WUCRenderingScopeGuard.h>
#include "ClipboardHelper.h"
#include "FocusTestHelper.h"

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace MockDComp;
using namespace Microsoft::UI::Xaml::Input;
using namespace concurrency;
using namespace ::Windows::Storage;
using namespace ::Windows::Storage::Streams;
using namespace Microsoft::UI::Text;
using namespace ::Windows::ApplicationModel;
using namespace ::Windows::ApplicationModel::DataTransfer;


namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Text {

        Platform::String^ TextBoxKeyInputTests::GetResourcesPath() const
        {
            return GetPackageFolder() + L"resources\\native\\foundation\\Text\\";
        }

        bool TextBoxKeyInputTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool TextBoxKeyInputTests::ClassCleanup()
        {
            return true;
        }

        bool TextBoxKeyInputTests::TestCleanup()
        {
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void TextBoxKeyInputTests::CheckTextBoxBasicKeyInputHelper(xaml_controls::TextBox^ textBox)
        {
            auto textBoxTextChangedEvent = std::make_shared<Event>();
            auto textBoxTextChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, TextChanged);

            Platform::String ^strToType = "Simple text input...";
            UINT numberOfTextChangedEvents = 0;

            RunOnUIThread([&]()
            {
                textBoxTextChangedRegistration.Attach(
                    textBox,
                    ref new xaml_controls::TextChangedEventHandler(
                        [&](Platform::Object^, xaml_controls::TextChangedEventArgs^)
                {
                    numberOfTextChangedEvents++;
                    LOG_OUTPUT(L"TextBox control TextChanged handler, occurrence=%d", numberOfTextChangedEvents);
                    LOG_OUTPUT(L"Current TextInput:%s", textBox->Text->Data());
                    if (numberOfTextChangedEvents >= strToType->Length()) // Wait for all key strokes, each key stroke will fire TextChanged event once
                    {
                        textBoxTextChangedEvent->Set();
                    }
                }));
            });

            FocusTestHelper::EnsureFocus(textBox, FocusState::Keyboard);
            TestServices::WindowHelper->WaitForIdle();

            TestServices::KeyboardHelper->PressKeySequence(strToType);
            TestServices::WindowHelper->WaitForIdle();
            textBoxTextChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"TextInput:%s", textBox->Text->Data());
                VERIFY_ARE_EQUAL(textBox->Text, strToType);
                textBox->IsEnabled = false; // disable textbox in case SIP is opened.
            });
        }

        void TextBoxKeyInputTests::CheckTextBoxBasicKeyInputTestHelper()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::TextBox^ textBox = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <TextBox x:Name='textBox' FontSize='20' Width='200' Height='80' Margin ='20,5,20,0'>"
                    L"  </TextBox>"
                    L"</StackPanel>"));
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();
            CheckTextBoxBasicKeyInputHelper(textBox);
        }

        void TextBoxKeyInputTests::CheckTextBoxBasicKeyInput()
        {
            CheckTextBoxBasicKeyInputTestHelper();
        }

        void TextBoxKeyInputTests::CheckTextBoxBasicKeyInputInWindowedPopup()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_primitives::Popup^ windowedPopup;
            xaml_controls::Canvas^ root;
            RunOnUIThread([&]()
            {
                root = ref new xaml_controls::Canvas();
                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                textBox = ref new xaml_controls::TextBox();
                textBox->Width = 200;
                textBox->Height = 40;
                windowedPopup = ref new xaml_primitives::Popup();
                windowedPopup->ShouldConstrainToRootBounds = false;
                windowedPopup->XamlRoot = root->XamlRoot;
                windowedPopup->Child = textBox;
                windowedPopup->IsOpen = true;
            });
            TestServices::WindowHelper->WaitForIdle();

            CheckTextBoxBasicKeyInputHelper(textBox);
        }

        void TextBoxKeyInputTests::MultiLineTextInput()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto textBoxTextChangedEvent = std::make_shared<Event>();
            auto textBoxTextChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, TextChanged);
            auto textBoxGotFocusEvent = std::make_shared<Event>();
            auto textBoxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::Button^ button = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <TextBox x:Name='textBox'  FontSize='20' Width='200' Height='80' Margin ='20,5,20,0' AcceptsReturn='True' TextWrapping='Wrap' MaxLength='0'>"
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

                textBoxTextChangedRegistration.Attach(
                    textBox,
                    ref new xaml_controls::TextChangedEventHandler(
                        [textBoxTextChangedEvent](Platform::Object^, xaml_controls::TextChangedEventArgs^)
                {
                    LOG_OUTPUT(L"TextBox control TextChanged handler.");
                    textBoxTextChangedEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Setting focus to the TextBox control.");
            RunOnUIThread([&]()
            {
                textBox->Focus(FocusState::Pointer);
            });
            textBoxGotFocusEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            Platform::String ^strToType = "ABCDEFGH";
            Platform::String ^strToCompareFirstTwoChar = "AB";
            TestServices::KeyboardHelper->PressKeySequence(strToType);
            TestServices::WindowHelper->WaitForIdle();
            textBoxTextChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"TextInput:%s", textBox->Text->Data());
                VERIFY_IS_TRUE(textBox->Text == strToType);
            });
            TestServices::WindowHelper->WaitForIdle();

            // change textbox's MaxLength to 2
            RunOnUIThread([&]()
            {
                textBox->Text = "";
                textBox->MaxLength = 2;
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->PressKeySequence(strToType);
            TestServices::WindowHelper->WaitForIdle();
            textBoxTextChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"TextInput:%s", textBox->Text->Data());
                VERIFY_IS_TRUE(textBox->Text == strToCompareFirstTwoChar);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Change MaxLength to 3 and input AB then press enter.");
                textBox->Text = "";
                textBox->MaxLength = 3;
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->PressKeySequence(L"ab");
            TestServices::KeyboardHelper->Enter();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"TextInput:%s", textBox->Text->Data());
                VERIFY_ARE_EQUAL(wcslen(textBox->Text->Data()), static_cast<size_t>(3));
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Setting Text to something from APP...");
                textBox->MaxLength = 4;
                textBox->Text = L"A\rB\n";
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Current TextBox.Text:%s", textBox->Text->Data());
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                textBox->IsEnabled = false;
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBoxKeyInputTests::CheckTextBoxMaxLength()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto textBoxTextChangedEvent = std::make_shared<Event>();
            auto textBoxTextChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, TextChanged);
            auto textBoxGotFocusEvent = std::make_shared<Event>();
            auto textBoxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::Button^ button = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <TextBox x:Name='textBox' IsTextPredictionEnabled ='False' FontSize='20' Width='200' Height='80' Margin ='20,5,20,0' AcceptsReturn='True' TextWrapping='Wrap' MaxLength='0'>"
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

                textBoxTextChangedRegistration.Attach(
                    textBox,
                    ref new xaml_controls::TextChangedEventHandler(
                        [textBoxTextChangedEvent](Platform::Object^, xaml_controls::TextChangedEventArgs^)
                {
                    LOG_OUTPUT(L"TextBox control TextChanged handler.");
                    textBoxTextChangedEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Setting focus to the TextBox control.");
            RunOnUIThread([&]()
            {
                textBox->Focus(FocusState::Pointer);
            });
            textBoxGotFocusEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            Platform::String ^strToType = "ABCDEFGH";
            Platform::String ^strToCompareFirstTwoChar = "AB";
            TestServices::KeyboardHelper->PressKeySequence(strToType);
            TestServices::WindowHelper->WaitForIdle();
            textBoxTextChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"TextInput:%s", textBox->Text->Data());
                VERIFY_IS_TRUE(textBox->Text == strToType);
            });
            TestServices::WindowHelper->WaitForIdle();

            // change textbox's MaxLength to 2
            RunOnUIThread([&]()
            {
                textBox->Text = "";
                textBox->MaxLength = 2;
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->PressKeySequence(strToType);
            TestServices::WindowHelper->WaitForIdle();
            textBoxTextChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"TextInput:%s", textBox->Text->Data());
                VERIFY_IS_TRUE(textBox->Text == strToCompareFirstTwoChar);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Change MaxLength to 3 and input AB then press enter.");
                textBox->Text = "";
                textBox->MaxLength = 3;
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->PressKeySequence(L"ab");
            TestServices::KeyboardHelper->Enter();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"TextInput:%s", textBox->Text->Data());
                VERIFY_ARE_EQUAL(wcslen(textBox->Text->Data()), static_cast<size_t>(3));
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Change MaxLength to 4 and try to set Text to AB\\r\\n, richedit will combine \\r\\n to \\r.");
                textBox->MaxLength = 4;
                textBox->Text = L"AB\r\n";
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Current TextBox.Text:%s", textBox->Text->Data());
                VERIFY_ARE_EQUAL(wcslen(textBox->Text->Data()), static_cast<size_t>(3));
            });

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Set Text to A\\rB\\n, richedit will change it to A\\rB\\r.");
                textBox->MaxLength = 4;
                textBox->Text = L"A\rB\n";
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Current TextBox.Text:%s", textBox->Text->Data());
                VERIFY_ARE_EQUAL(wcslen(textBox->Text->Data()), static_cast<size_t>(4));
            });

            // Check that setting text programmatically adheres to MaxLength
            if (false) // a known issue
            {
                RunOnUIThread([&]()
                {
                    textBox->MaxLength = 4;
                    textBox->Text = L"Hello";
                });

                RunOnUIThread([&]()
                {
                    VERIFY_ARE_EQUAL(wcslen(textBox->Text->Data()), static_cast<size_t>(4));
                });
            }

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                textBox->IsEnabled = false;
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBoxKeyInputTests::CheckTextBoxTextChanging()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto textChangingEvent = std::make_shared<Event>();
            auto textChangingRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, TextChanging);
            auto textChangedEvent = std::make_shared<Event>();
            auto textChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, TextChanged);

            xaml_controls::TextBox^ textBox = nullptr;
            Platform::String ^toType = "X";
            Platform::String ^toReplaceWith = "Y";

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <TextBox x:Name='textBox' FontSize='20' Width='200' />"
                    L"</StackPanel>"));
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                TestServices::WindowHelper->WindowContent = rootPanel;

                textChangingRegistration.Attach(
                    textBox,
                    ref new wf::TypedEventHandler<xaml_controls::TextBox^, xaml_controls::TextBoxTextChangingEventArgs^>(
                    [=](xaml_controls::TextBox ^sender, xaml_controls::TextBoxTextChangingEventArgs^)
                {
                    LOG_OUTPUT(L"TextBox control TextChanging handler.");
                    VERIFY_IS_FALSE(textChangingEvent->HasFired(), L"TextChanging is only expected to fire once per original change");
                    VERIFY_IS_FALSE(textChangedEvent->HasFired(), L"TextChanged should not fire before TextChanging");
                    textChangingEvent->Set();

                    // replace text from within the handler, should not trigger additional events
                    VERIFY_ARE_EQUAL(toType, sender->Text, L"Text is expected to be set when TextChanging fires");
                    sender->Text = toReplaceWith;
                }));

                textChangedRegistration.Attach(
                    textBox,
                    ref new xaml_controls::TextChangedEventHandler(
                    [=](Platform::Object^, xaml_controls::TextChangedEventArgs^)
                {
                    LOG_OUTPUT(L"TextBox control TextChanged handler.");
                    VERIFY_IS_TRUE(textChangingEvent->HasFired(), L"TextChanging is expected to fire before TextChanged");
                    VERIFY_IS_FALSE(textChangedEvent->HasFired(), L"TextChanged is only expected to fire once per original change");
                    VERIFY_ARE_EQUAL(toReplaceWith, textBox->Text, L"Text is expected to be replaced when TextChanged fires");
                    textChangedEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();

            FocusTestHelper::EnsureFocus(textBox, FocusState::Keyboard);

            TestServices::KeyboardHelper->PressKeySequence(toType);
            textChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([=]()
            {
                VERIFY_ARE_EQUAL(toReplaceWith, textBox->Text);
            });


            LOG_OUTPUT(L"Repeat the sequence to make sure TextChanging can fire again.");
            textChangingEvent->Reset();
            textChangedEvent->Reset();
            RunOnUIThread([=]()
            {
                LOG_OUTPUT(L"This time replace text instead of typing.");
                textBox->Text = toType;
            });
            textChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([=]()
            {
                VERIFY_ARE_EQUAL(toReplaceWith, textBox->Text);
                textBox->IsEnabled = false; // disable textbox in case SIP is opened.
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBoxKeyInputTests::TextBoxTextChangingReentrancyCheck()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto textChangingEvent = std::make_shared<Event>();
            auto textChangingRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, TextChanging);

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::TextBlock^ textBlock = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <TextBlock x:Name='textBlock' FontSize='20' Width='200' Text='TextBlock' />"
                    L"  <TextBox x:Name='textBox' FontSize='20' Width='200' Text='{Binding Text, ElementName = textBlock}' />"
                    L"</StackPanel>"));
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                textBlock = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"textBlock"));
                VERIFY_IS_NOT_NULL(textBlock);
                TestServices::WindowHelper->WindowContent = rootPanel;

                textChangingRegistration.Attach(
                    textBox,
                    ref new wf::TypedEventHandler<xaml_controls::TextBox^, xaml_controls::TextBoxTextChangingEventArgs^>(
                        [=](xaml_controls::TextBox ^sender, xaml_controls::TextBoxTextChangingEventArgs^)
                {
                    LOG_OUTPUT(L"TextBox control TextChanging handler.");
                    textChangingEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(textBox, FocusState::Keyboard);

            textChangingEvent->Reset();
            RunOnUIThread([=]()
            {
                textBlock->Text = "XYZ";
            });

            TestServices::WindowHelper->WaitForIdle();
            textChangingEvent->WaitForDefault();
        }

        void TextBoxKeyInputTests::TextBoxTextChangingEventContentChanged()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto textChangingEvent = std::make_shared<Event>();
            auto realTextChangingEvent = std::make_shared<Event>();
            auto textChangingRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, TextChanging);

            xaml_controls::TextBox^ textBox = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <TextBox x:Name='textBox' FontSize='20' Width='200' Text='1234' />"
                    L"</StackPanel>"));
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                TestServices::WindowHelper->WindowContent = rootPanel;

                textChangingRegistration.Attach(
                    textBox,
                    ref new wf::TypedEventHandler<xaml_controls::TextBox^, xaml_controls::TextBoxTextChangingEventArgs^>(
                        [=](xaml_controls::TextBox ^sender, xaml_controls::TextBoxTextChangingEventArgs^ textChangingEventArgs)
                {
                    LOG_OUTPUT(L"TextBox control TextChanging handler.");
                    if (textChangingEventArgs->IsContentChanging)
                    {
                        LOG_OUTPUT(L"Text content changed.");
                        realTextChangingEvent->Set();
                    }
                    textChangingEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(textBox, FocusState::Keyboard);

            textChangingEvent->Reset();
            realTextChangingEvent->Reset();
            RunOnUIThread([=]()
            {
                textBox->Text = "XYZ";
            });

            TestServices::WindowHelper->WaitForIdle();
            textChangingEvent->WaitForDefault();
            realTextChangingEvent->WaitForDefault();

            textChangingEvent->Reset();
            RunOnUIThread([=]()
            {
                LOG_OUTPUT(L"Changing text format, there should be no textChanging event at all for TextBox.");
                textBox->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
                textBox->FontSize = 25;
            });

            TestServices::WindowHelper->WaitForIdle();
            textChangingEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
            VERIFY_IS_FALSE(textChangingEvent->HasFired());
        }

        void TextBoxKeyInputTests::TextBoxNoTextChangingEventForTyping()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto textChangingEventNotContent = std::make_shared<Event>();
            auto textChangingRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, TextChanging);

            xaml_controls::TextBox^ textBox = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <TextBox x:Name='textBox' FontSize='20' Width='200' Text='1234' />"
                    L"</StackPanel>"));
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                TestServices::WindowHelper->WindowContent = rootPanel;

                textChangingRegistration.Attach(
                    textBox,
                    ref new wf::TypedEventHandler<xaml_controls::TextBox^, xaml_controls::TextBoxTextChangingEventArgs^>(
                        [=](xaml_controls::TextBox ^sender, xaml_controls::TextBoxTextChangingEventArgs^ textChangingEventArgs)
                {
                    LOG_OUTPUT(L"TextBox control TextChanging handler.");
                    if (!textChangingEventArgs->IsContentChanging)
                    {
                        LOG_OUTPUT(L"Text changing event is not for content change.");
                        textChangingEventNotContent->Set();
                    }
                }));
            });

            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(textBox, FocusState::Keyboard);

            TestServices::KeyboardHelper->PressKeySequence(L"0123456789");

            TestServices::WindowHelper->WaitForIdle();
            textChangingEventNotContent->WaitForNoThrow(std::chrono::milliseconds(1000));
            VERIFY_IS_FALSE(textChangingEventNotContent->HasFired());
        }

        void TextBoxKeyInputTests::RichEditBoxTextChangingEventContentChanged()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto textChangingEvent = std::make_shared<Event>();
            auto realTextChangingEvent = std::make_shared<Event>();
            auto textChangingRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, TextChanging);

            xaml_controls::RichEditBox^ rebx = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <RichEditBox x:Name = 'rebx' Width='200' Height='60' Margin = '20,40,20,0' />"
                    L"</StackPanel>"));
                rebx = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx"));
                VERIFY_IS_NOT_NULL(rebx);
                TestServices::WindowHelper->WindowContent = rootPanel;

                textChangingRegistration.Attach(
                    rebx,
                    ref new wf::TypedEventHandler<xaml_controls::RichEditBox^, xaml_controls::RichEditBoxTextChangingEventArgs^>(
                        [=](xaml_controls::RichEditBox ^sender, xaml_controls::RichEditBoxTextChangingEventArgs^ textChangingEventArgs)
                {
                    LOG_OUTPUT(L"RichEditBox control TextChanging handler.");
                    if (textChangingEventArgs->IsContentChanging)
                    {
                        LOG_OUTPUT(L"Text content changed.");
                        realTextChangingEvent->Set();
                    }
                    textChangingEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(rebx, FocusState::Keyboard);

            textChangingEvent->Reset();
            realTextChangingEvent->Reset();
            RunOnUIThread([=]()
            {
                rebx->Document->SetText(Microsoft::UI::Text::TextSetOptions::None, L"XYZ");
            });

            TestServices::WindowHelper->WaitForIdle();
            textChangingEvent->WaitForDefault();
            realTextChangingEvent->WaitForDefault();

            textChangingEvent->Reset();
            realTextChangingEvent->Reset();
            RunOnUIThread([=]()
            {
                LOG_OUTPUT(L"Changing text format, there should be a textChanging event with no content change for RichEditBox.");
                rebx->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
                rebx->FontSize = 25;
            });

            TestServices::WindowHelper->WaitForIdle();
            textChangingEvent->WaitForDefault();
            realTextChangingEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
            VERIFY_IS_FALSE(realTextChangingEvent->HasFired());
        }

        void TextBoxKeyInputTests::CheckRichEditBoxTextChanging()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto gotFocusEvent = std::make_shared<Event>();
            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, GotFocus);
            auto textChangingEvent = std::make_shared<Event>();
            auto textChangingRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, TextChanging);
            auto textChangedEvent = std::make_shared<Event>();
            auto textChangedRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, TextChanged);

            xaml_controls::RichEditBox^ richEditBox = nullptr;
            Platform::String ^toType = "X";
            Platform::String ^toReplaceWith = "Y";

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <RichEditBox x:Name='richEditBox' FontSize='20' Width='200' />"
                    L"</StackPanel>"));
                richEditBox = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"richEditBox"));
                VERIFY_IS_NOT_NULL(richEditBox);
                TestServices::WindowHelper->WindowContent = rootPanel;

                gotFocusRegistration.Attach(
                    richEditBox,
                    ref new xaml::RoutedEventHandler(
                    [gotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"RichEditBox control GotFocus handler.");
                    gotFocusEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Set keyboard focus on the richedit box.");
                richEditBox->Focus(xaml::FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Wait focus to be set on the RichEditBox control");
            gotFocusEvent->WaitForDefault();

            // // Workaround: Only listen for TextChanged/TextChanging events during key press. Otherwise
            //             these events would fire many times for unrelated reasons such as on set focus
            textChangingRegistration.Attach(
                richEditBox,
                ref new wf::TypedEventHandler<xaml_controls::RichEditBox^, xaml_controls::RichEditBoxTextChangingEventArgs^>(
                [=](xaml_controls::RichEditBox ^sender, xaml_controls::RichEditBoxTextChangingEventArgs^)
            {
                Platform::String ^text;
                richEditBox->Document->GetText(mut::TextGetOptions::AdjustCrlf, &text);
                LOG_OUTPUT(L"RichEditBox control TextChanging (Text = '%s')", text->Data());

                VERIFY_IS_FALSE(textChangingEvent->HasFired(), L"TextChanging is only expected to fire once per original change");
                VERIFY_IS_FALSE(textChangedEvent->HasFired(), L"TextChanged should not fire before TextChanging");
                VERIFY_ARE_EQUAL(toType, text, L"Document text is expected to be set when TextChanging fires");

                textChangingEvent->Set();

                // replace the document text from within the handler, should not trigger additional events
                sender->Document->SetText(mut::TextSetOptions::None, toReplaceWith);
            }));
            textChangedRegistration.Attach(
                richEditBox,
                ref new xaml::RoutedEventHandler(
                [=](Platform::Object^, xaml::RoutedEventArgs^)
            {
                Platform::String ^text;
                richEditBox->Document->GetText(mut::TextGetOptions::AdjustCrlf, &text);
                LOG_OUTPUT(L"RichEditBox control TextChanged (Text = '%s')", text->Data());

                VERIFY_IS_TRUE(textChangingEvent->HasFired(), L"TextChanging is expected to fire before TextChanged");
                VERIFY_IS_FALSE(textChangedEvent->HasFired(), L"TextChanged is only expected to fire once per original change");
                textChangedEvent->Set();
            }));
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Type the following text: '%s'", toType->Data());
            TestServices::KeyboardHelper->PressKeySequence(toType);
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Wait for TextChanged event");
            textChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            // // Workaround: These events would fire when IsEnabled changed
            textChangedRegistration.Detach();
            textChangingRegistration.Detach();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([=]()
            {
                Platform::String ^text;
                richEditBox->Document->GetText(mut::TextGetOptions::AdjustCrlf, &text);
                VERIFY_ARE_EQUAL(toReplaceWith, text);

                richEditBox->IsEnabled = false; // disable textbox in case SIP is opened.
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBoxKeyInputTests::PasswordBoxPasswordChangingEventContentChanged()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto passwordChangingEvent = std::make_shared<Event>();
            auto realPasswordChangingEvent = std::make_shared<Event>();
            auto passwordChangingRegistration = CreateSafeEventRegistration(xaml_controls::PasswordBox, PasswordChanging);

            xaml_controls::PasswordBox^ passwordBox = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <PasswordBox x:Name='passwordBox' FontSize='20' Width='200' Password='1234' />"
                    L"</StackPanel>"));
                passwordBox = safe_cast<xaml_controls::PasswordBox^>(rootPanel->FindName(L"passwordBox"));
                VERIFY_IS_NOT_NULL(passwordBox);
                TestServices::WindowHelper->WindowContent = rootPanel;

                passwordChangingRegistration.Attach(
                    passwordBox,
                    ref new wf::TypedEventHandler<xaml_controls::PasswordBox^, xaml_controls::PasswordBoxPasswordChangingEventArgs^>(
                        [=](xaml_controls::PasswordBox ^sender, xaml_controls::PasswordBoxPasswordChangingEventArgs^ passwordChangingEventArgs)
                {
                    LOG_OUTPUT(L"PasswordBox control PasswordChanging handler.");
                    if (passwordChangingEventArgs->IsContentChanging)
                    {
                        LOG_OUTPUT(L"Password content changed.");
                        realPasswordChangingEvent->Set();
                    }
                    passwordChangingEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(passwordBox, FocusState::Keyboard);

            passwordChangingEvent->Reset();
            realPasswordChangingEvent->Reset();
            RunOnUIThread([=]()
            {
                passwordBox->Password = "XYZ";
            });

            passwordChangingEvent->WaitForDefault();
            realPasswordChangingEvent->WaitForDefault();

            passwordChangingEvent->Reset();
            RunOnUIThread([=]()
            {
                LOG_OUTPUT(L"Changing text format, there should be no passwordChanging event at all for PasswordBox.");
                passwordBox->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
                passwordBox->FontSize = 25;
            });

            TestServices::WindowHelper->WaitForIdle();
            passwordChangingEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
            VERIFY_IS_FALSE(passwordChangingEvent->HasFired());
        }

        void TextBoxKeyInputTests::CheckPasswordBoxPasswordChanging()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto passwordChangingEvent = std::make_shared<Event>();
            auto passwordChangingRegistration = CreateSafeEventRegistration(xaml_controls::PasswordBox, PasswordChanging);
            auto passwordChangedEvent = std::make_shared<Event>();
            auto passwordChangedRegistration = CreateSafeEventRegistration(xaml_controls::PasswordBox, PasswordChanged);

            xaml_controls::PasswordBox^ passwordBox = nullptr;
            Platform::String ^toType = "X";
            Platform::String ^toReplaceWith = "Y";

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <PasswordBox x:Name='passwordBox' FontSize='20' Width='200' />"
                    L"</StackPanel>"));
                passwordBox = safe_cast<xaml_controls::PasswordBox^>(rootPanel->FindName(L"passwordBox"));
                VERIFY_IS_NOT_NULL(passwordBox);
                TestServices::WindowHelper->WindowContent = rootPanel;

                passwordChangingRegistration.Attach(
                    passwordBox,
                    ref new wf::TypedEventHandler<xaml_controls::PasswordBox^, xaml_controls::PasswordBoxPasswordChangingEventArgs^>(
                    [=](xaml_controls::PasswordBox ^sender, xaml_controls::PasswordBoxPasswordChangingEventArgs^)
                {
                    LOG_OUTPUT(L"PasswordBox control PasswordChanging handler.");
                    VERIFY_IS_FALSE(passwordChangingEvent->HasFired(), L"PasswordChanging is only expected to fire once per original change");
                    VERIFY_IS_FALSE(passwordChangedEvent->HasFired(), L"PasswordChanged should not fire before PasswordChanging");
                    passwordChangingEvent->Set();

                    // replace text from within the handler, should not trigger additional events
                    VERIFY_ARE_EQUAL(toType, sender->Password, L"Text is expected to be set when PasswordChanging fires");
                    sender->Password = toReplaceWith;
                }));

                passwordChangedRegistration.Attach(
                    passwordBox,
                    ref new xaml::RoutedEventHandler(
                    [=](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"PasswordBox control PasswordChanged handler.");
                    VERIFY_IS_TRUE(passwordChangingEvent->HasFired(), L"PasswordChanging is expected to fire before PasswordChanged");
                    VERIFY_IS_FALSE(passwordChangedEvent->HasFired(), L"PasswordChanged is only expected to fire once per original change");
                    VERIFY_ARE_EQUAL(toReplaceWith, passwordBox->Password, L"Text is expected to be replaced when PasswordChanged fires");
                    passwordChangedEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(passwordBox, FocusState::Keyboard);

            TestServices::KeyboardHelper->PressKeySequence(toType);
            passwordChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([=]()
            {
                VERIFY_ARE_EQUAL(toReplaceWith, passwordBox->Password);
            });


            LOG_OUTPUT(L"Repeat the sequence to make sure PasswordChanging can fire again.");
            passwordChangingEvent->Reset();
            passwordChangedEvent->Reset();
            RunOnUIThread([=]()
            {
                LOG_OUTPUT(L"This time replace text instead of typing.");
                passwordBox->Password = toType;
            });
            passwordChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([=]()
            {
                VERIFY_ARE_EQUAL(toReplaceWith, passwordBox->Password);
                passwordBox->IsEnabled = false; // disable passwordbox in case SIP is opened.
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBoxKeyInputTests::CheckTextBoxControlAltKeyInput()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto textBoxSelectionChangedEvent = std::make_shared<Event>();
            auto textBoxSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, SelectionChanged);
            auto textBoxGotFocusEvent = std::make_shared<Event>();
            auto textBoxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::Button^ button = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <TextBox x:Name='textBox' Text='Ctrl key test' FontSize='20' Width='200' Margin ='20,5,20,0'>"
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

                textBoxSelectionChangedRegistration.Attach(
                    textBox,
                    ref new xaml::RoutedEventHandler(
                    [textBoxSelectionChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"TextBox.SelectionChanged handler.");
                    textBoxSelectionChangedEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Setting focus to the TextBox control.");
            RunOnUIThread([&]()
            {
                textBox->Focus(FocusState::Pointer);
            });
            textBoxGotFocusEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Ctrl+A--->Select All...");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_a#$u$_a#$u$_ctrl");
            textBoxSelectionChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"SelectionStart=%d, SelectionLength=%d", textBox->SelectionStart, textBox->SelectionLength);
                VERIFY_IS_TRUE(textBox->SelectionLength == 13);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                textBox->Select(0, 0); // move the cursor to front
            });
            TestServices::WindowHelper->WaitForIdle();
            textBoxSelectionChangedEvent->Reset();

            LOG_OUTPUT(L"Ctrl+Right--->Jump one word...");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_right#$u$_right#$u$_ctrl");
            TestServices::WindowHelper->WaitForIdle();
            textBoxSelectionChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"SelectionStart=%d, SelectionLength=%d", textBox->SelectionStart, textBox->SelectionLength);
                VERIFY_IS_TRUE(textBox->SelectionStart == 5);
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Ctrl+Left--->Back one word...");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_left#$u$_left#$u$_ctrl");
            TestServices::WindowHelper->WaitForIdle();
            textBoxSelectionChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"SelectionStart=%d, SelectionLength=%d", textBox->SelectionStart, textBox->SelectionLength);
                VERIFY_IS_TRUE(textBox->SelectionStart == 0);
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Shift+Right--->Select one char...");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_right#$u$_right#$u$_shift");
            TestServices::WindowHelper->WaitForIdle();
            textBoxSelectionChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"SelectionStart=%d, SelectionLength=%d", textBox->SelectionStart, textBox->SelectionLength);
                VERIFY_IS_TRUE(textBox->SelectionLength == 1);
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Shift+Left--->Unselect one char...");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_left#$u$_left#$u$_shift");
            TestServices::WindowHelper->WaitForIdle();
            textBoxSelectionChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"SelectionStart=%d, SelectionLength=%d", textBox->SelectionStart, textBox->SelectionLength);
                VERIFY_IS_TRUE(textBox->SelectionLength == 0);
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Ctrl+Shift+Right--->Select one word...");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_shift#$d$_right#$u$_right#$u$_shift#$u$_ctrl"); // press ctl+shift-right
            TestServices::WindowHelper->WaitForIdle();
            textBoxSelectionChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"SelectionStart=%d, SelectionLength=%d", textBox->SelectionStart, textBox->SelectionLength);
                VERIFY_IS_TRUE(textBox->SelectionLength == 5);
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Ctrl+Shift+Left--->Unselect one word...");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_shift#$d$_left#$u$_left#$u$_shift#$u$_ctrl"); // press ctl+shift-left
            TestServices::WindowHelper->WaitForIdle();
            textBoxSelectionChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"SelectionStart=%d, SelectionLength=%d", textBox->SelectionStart, textBox->SelectionLength);
                VERIFY_IS_TRUE(textBox->SelectionLength == 0);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                textBox->IsEnabled = false; // disable textbox in case SIP is opened.
            });
        }

        void TextBoxKeyInputTests::CheckTextBoxURLTabbing()
        {
            TestCleanupWrapper cleanup;
            KeyboardInjectionIgnoreEventWaitOverride keyboardWaitOverride;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto textBoxSelectionChangedEvent = std::make_shared<Event>();
            auto textBoxSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, SelectionChanged);
            auto textBoxGotFocusEvent = std::make_shared<Event>();
            auto textBoxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::Button^ button = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <TextBox x:Name='textBox' FontSize='20' Width='300' Height='80' Margin ='20,5,20,0' />"
                    L"</StackPanel>"));

                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                TestServices::WindowHelper->WindowContent = rootPanel;

                textBoxGotFocusRegistration.Attach(
                    textBox,
                    ref new xaml::RoutedEventHandler(
                        [textBoxGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"TextBox GotFocus handler.");
                    textBoxGotFocusEvent->Set();
                }));

                textBoxSelectionChangedRegistration.Attach(
                    textBox,
                    ref new xaml::RoutedEventHandler(
                        [textBoxSelectionChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"TextBox SelectionChanged handler.");
                    textBoxSelectionChangedEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Setting focus to the TextBox control.");
            RunOnUIThread([&]()
            {
                textBox->Text = "http://www.bing.com";
                textBox->Focus(xaml::FocusState::Pointer);
            });
            TestServices::WindowHelper->WaitForIdle();
            textBoxGotFocusEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                textBox->Select(0,0); // move cursor to beginning of text
            });
            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"PressKeySequence($d$_ctrl#$d$_right#$u$_right#$u$_ctrl)");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_right#$u$_right#$u$_ctrl");
            textBoxSelectionChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Selection Start %d, Length:%d", textBox->SelectionStart, textBox->SelectionLength);
                VERIFY_IS_TRUE(textBox->SelectionStart == 4);
            });

            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_right#$u$_right#$u$_ctrl");
            textBoxSelectionChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Selection Start %d, Length:%d", textBox->SelectionStart, textBox->SelectionLength);
                VERIFY_IS_TRUE(textBox->SelectionStart == 7);
            });

            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_right#$u$_right#$u$_ctrl");
            textBoxSelectionChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Selection Start %d, Length:%d", textBox->SelectionStart, textBox->SelectionLength);
                VERIFY_IS_TRUE(textBox->SelectionStart == 10);
            });

            LOG_OUTPUT(L"Switch input scope to URL.");
            RunOnUIThread([&]()
            {
                xaml_input::InputScope ^scope = ref new xaml_input::InputScope();
                xaml_input::InputScopeName ^scopeName = ref new xaml_input::InputScopeName(Xaml::Input::InputScopeNameValue::Url);
                scope->Names->Append(scopeName);
                textBox->InputScope = scope;
            });
            RunOnUIThread([&]()
            {
                textBox->Select(0, 0); // move cursor to beginning of text
            });
            TestServices::WindowHelper->WaitForIdle();

            //Ctrl+-> should skip those symbols such as "//" in the URL string
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_right#$u$_right#$u$_ctrl");
            textBoxSelectionChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Selection Start %d, Length:%d", textBox->SelectionStart, textBox->SelectionLength);
                VERIFY_IS_TRUE(textBox->SelectionStart == 7);
            });

            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_right#$u$_right#$u$_ctrl");
            textBoxSelectionChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Selection Start %d, Length:%d", textBox->SelectionStart, textBox->SelectionLength);
                VERIFY_IS_TRUE(textBox->SelectionStart == 11);
            });

            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_right#$u$_right#$u$_ctrl");
            textBoxSelectionChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Selection Start %d, Length:%d", textBox->SelectionStart, textBox->SelectionLength);
                VERIFY_IS_TRUE(textBox->SelectionStart == 16);
            });

            // testing UNC path tab navigation
            RunOnUIThread([&]()
            {
                textBox->Text = "\\\\Test";
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                textBox->Select(6, 0); // move cursor to the end
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_left#$u$_left#$u$_ctrl");
            TestServices::WindowHelper->WaitForIdle();
            textBoxSelectionChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Selection Start %d, Length:%d", textBox->SelectionStart, textBox->SelectionLength);
                VERIFY_IS_TRUE(textBox->SelectionStart == 2);
            });

            // verify no hang when navigating to the front of UNC path '\\' 
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_left#$u$_left#$u$_ctrl");
            TestServices::WindowHelper->WaitForIdle();
            textBoxSelectionChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Selection Start %d, Length:%d", textBox->SelectionStart, textBox->SelectionLength);
                VERIFY_IS_TRUE(textBox->SelectionStart == 0);
            });

            RunOnUIThread([&]()
            {
                textBox->IsEnabled = false; // disable textbox in case SIP is opened.
            });
        }

        void TextBoxKeyInputTests::CheckTextBoxPasteEvent()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto textBoxPasteEvent = std::make_shared<Event>();
            auto textBoxPasteRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, Paste);
            auto textBoxGotFocusEvent = std::make_shared<Event>();
            auto textBoxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::Button^ button = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <TextBox x:Name='textBox' Text='Paste event test' FontSize='20' Width='200' Margin ='20,5,20,0'>"
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

                textBoxPasteRegistration.Attach(
                    textBox,
                    ref new xaml_controls::TextControlPasteEventHandler(
                        [textBoxPasteEvent](Platform::Object^, xaml_controls::TextControlPasteEventArgs^)
                {
                    LOG_OUTPUT(L"TextBox paste event handler.");
                    textBoxPasteEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                textBox->Focus(xaml::FocusState::Pointer);
            });

            textBoxGotFocusEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Ctrl+C--->Copy...");
            TestServices::KeyboardHelper->Copy();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Ctrl+V--->Paste...");
            TestServices::KeyboardHelper->Paste();
            TestServices::WindowHelper->WaitForIdle();
            textBoxPasteEvent->WaitForDefault();

            textBoxPasteEvent->Reset();
            LOG_OUTPUT(L"Ctrl+Shift+V--->Paste...");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_ctrl#$d$_v#$u$_v#$u$_ctrl#$u$_shift");
            TestServices::WindowHelper->WaitForIdle();
            textBoxPasteEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                textBox->IsEnabled = false; // disable textbox in case SIP is opened.
            });
        }

        void TextBoxKeyInputTests::CheckTextBoxCopyEvent()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto textBoxCopyEvent = std::make_shared<Event>();
            auto textBoxCopyRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, CopyingToClipboard);

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::Button^ button = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <TextBox x:Name='textBox' Text='Copy event test' FontSize='20' Width='200' Margin ='20,5,20,0'>"
                    L"  </TextBox>"
                    L"</StackPanel>"));
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                TestServices::WindowHelper->WindowContent = rootPanel;

                textBoxCopyRegistration.Attach(
                    textBox,
                    ref new wf::TypedEventHandler<xaml_controls::TextBox^, xaml_controls::TextControlCopyingToClipboardEventArgs^>(
                        [textBoxCopyEvent](xaml_controls::TextBox^ sender, xaml_controls::TextControlCopyingToClipboardEventArgs^ args)
                {
                    LOG_OUTPUT(L"TextBox copy event handler.");
                    textBoxCopyEvent->Set();
                }));

            });

            TestServices::WindowHelper->WaitForIdle();

            //Focus textBox3
            FocusTestHelper::EnsureFocus(textBox, FocusState::Keyboard);

            RunOnUIThread([&]()
            {
                textBox->SelectAll();
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Ctrl+C--->Copy...");
            TestServices::KeyboardHelper->Copy();
            TestServices::WindowHelper->WaitForIdle();
            textBoxCopyEvent->WaitForDefault();
        }

        void TextBoxKeyInputTests::CheckTextBoxCopyEventCanBeHandled()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto textBoxCopyEvent = std::make_shared<Event>();
            auto textBoxCopyRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, CopyingToClipboard);

            auto textBox3CopyEvent = std::make_shared<Event>();
            auto textBox3CopyRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, CopyingToClipboard);

            auto textBox2PasteEvent = std::make_shared<Event>();
            auto textBox2PasteRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, Paste);

            auto clipboardContentChanged = std::make_shared<Event>();

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::TextBox^ textBox2 = nullptr;
            xaml_controls::TextBox^ textBox3 = nullptr;
            xaml_controls::Button^ button = nullptr;

            Platform::String^ textBox3Text = "Lorem";

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <TextBox x:Name='textBox' Text='Copy event test' FontSize='20' Width='200' Margin ='20,5,20,0'>"
                    L"  </TextBox>"
                    L"  <TextBox x:Name='textBox2' FontSize='20' Width='200' Margin ='20,5,20,0'>"
                    L"  </TextBox>"
                    L"  <TextBox x:Name='textBox3' FontSize='20' Width='200' Margin ='20,5,20,0'>"
                    L"  </TextBox>"
                    L"</StackPanel>"));
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                textBox2 = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox2"));
                textBox3 = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox3"));
                VERIFY_IS_NOT_NULL(textBox);
                VERIFY_IS_NOT_NULL(textBox2);
                VERIFY_IS_NOT_NULL(textBox3);

                textBox3->Text = textBox3Text;

                TestServices::WindowHelper->WindowContent = rootPanel;

                textBoxCopyRegistration.Attach(
                    textBox,
                    ref new wf::TypedEventHandler<xaml_controls::TextBox^, xaml_controls::TextControlCopyingToClipboardEventArgs^>(
                        [textBoxCopyEvent](xaml_controls::TextBox^ sender, xaml_controls::TextControlCopyingToClipboardEventArgs^ args)
                {
                    LOG_OUTPUT(L"In textBox copy event, setting Handled to true.");
                    args->Handled = true;
                    textBoxCopyEvent->Set();
                }));

                textBox3CopyRegistration.Attach(
                    textBox3,
                    ref new wf::TypedEventHandler<xaml_controls::TextBox^, xaml_controls::TextControlCopyingToClipboardEventArgs^>(
                        [textBox3CopyEvent](xaml_controls::TextBox^ sender, xaml_controls::TextControlCopyingToClipboardEventArgs^ args)
                {
                    LOG_OUTPUT(L"In TextBox3 Copy event.");
                    textBox3CopyEvent->Set();
                }));

                textBox2PasteRegistration.Attach(
                    textBox2,
                    ref new xaml_controls::TextControlPasteEventHandler(
                        [textBox2PasteEvent](Platform::Object^, xaml_controls::TextControlPasteEventArgs^)
                {
                    LOG_OUTPUT(L"TextBox2 paste event handler.");
                    textBox2PasteEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();
            ClipboardHelper clipboardHelper;
            //Focus textBox3
            FocusTestHelper::EnsureFocus(textBox3, FocusState::Keyboard);

            RunOnUIThread([&]()
            {
                textBox3->SelectAll();
            });
            TestServices::WindowHelper->WaitForIdle();


            LOG_OUTPUT(L"Ctrl+C--->Copy...");
            TestServices::KeyboardHelper->Copy();
            TestServices::WindowHelper->WaitForIdle();
            textBox3CopyEvent->WaitForDefault();

            clipboardHelper.WaitForContentChangedEvent();
            TestServices::WindowHelper->WaitForIdle();
            clipboardHelper.VerifyClipboardText(textBox3Text);
            clipboardHelper.ResetContentChangedEvent();

            //Focus textBox
            FocusTestHelper::EnsureFocus(textBox, FocusState::Keyboard);

            RunOnUIThread([&]()
            {
                textBox->SelectAll();
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Ctrl+C--->Copy...");
            TestServices::KeyboardHelper->Copy();
            TestServices::WindowHelper->WaitForIdle();
            textBoxCopyEvent->WaitForDefault();

            VERIFY_IS_FALSE(clipboardHelper.ContentChangedEventHasFired());
            TestServices::WindowHelper->WaitForIdle();
            clipboardHelper.VerifyClipboardText(textBox3Text);

            //Focus textBox2
            FocusTestHelper::EnsureFocus(textBox2, FocusState::Keyboard);

            LOG_OUTPUT(L"Ctrl+V--->Paste...");
            TestServices::KeyboardHelper->Paste();
            TestServices::WindowHelper->WaitForIdle();
            textBox2PasteEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(textBox3Text, textBox2->Text);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBoxKeyInputTests::CheckTextBoxCuttingToClipboardEventCanBeHandled()
        {
            TestCleanupWrapper cleanup;
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto textBoxCuttingEvent = std::make_shared<Event>();
            auto textBoxCuttingRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, CuttingToClipboard);

            auto textBox3CuttingEvent = std::make_shared<Event>();
            auto textBox3CuttingRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, CuttingToClipboard);

            auto textBox2PasteEvent = std::make_shared<Event>();
            auto textBox2PasteRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, Paste);

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::TextBox^ textBox2 = nullptr;
            xaml_controls::TextBox^ textBox3 = nullptr;

            Platform::String^ textBox3Text = "Lorem";
            Platform::String^ textBoxText = "Cut event test";

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <TextBox x:Name='textBox' FontSize='20' Width='200' Margin ='20,5,20,0'>"
                    L"  </TextBox>"
                    L"  <TextBox x:Name='textBox2' FontSize='20' Width='200' Margin ='20,5,20,0'>"
                    L"  </TextBox>"
                    L"  <TextBox x:Name='textBox3' FontSize='20' Width='200' Margin ='20,5,20,0'>"
                    L"  </TextBox>"
                    L"</StackPanel>"));
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                textBox2 = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox2"));
                textBox3 = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox3"));
                VERIFY_IS_NOT_NULL(textBox);
                VERIFY_IS_NOT_NULL(textBox2);
                VERIFY_IS_NOT_NULL(textBox3);

                textBox3->Text = textBox3Text;
                textBox->Text = textBoxText;

                TestServices::WindowHelper->WindowContent = rootPanel;

                textBoxCuttingRegistration.Attach(
                    textBox,
                    ref new wf::TypedEventHandler<xaml_controls::TextBox^, xaml_controls::TextControlCuttingToClipboardEventArgs^>(
                        [textBoxCuttingEvent](xaml_controls::TextBox^ sender, xaml_controls::TextControlCuttingToClipboardEventArgs^ args)
                {
                    LOG_OUTPUT(L"In textBox Cutting event, setting Handled to true.");
                    args->Handled = true;
                    textBoxCuttingEvent->Set();
                }));

                textBox3CuttingRegistration.Attach(
                    textBox3,
                    ref new wf::TypedEventHandler<xaml_controls::TextBox^, xaml_controls::TextControlCuttingToClipboardEventArgs^>(
                        [textBox3CuttingEvent](xaml_controls::TextBox^ sender, xaml_controls::TextControlCuttingToClipboardEventArgs^ args)
                {
                    LOG_OUTPUT(L"In TextBox3 Cutting event.");
                    textBox3CuttingEvent->Set();
                }));

                textBox2PasteRegistration.Attach(
                    textBox2,
                    ref new xaml_controls::TextControlPasteEventHandler(
                        [textBox2PasteEvent](Platform::Object^, xaml_controls::TextControlPasteEventArgs^)
                {
                    LOG_OUTPUT(L"TextBox2 paste event handler.");
                    textBox2PasteEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();
            ClipboardHelper clipboardHelper;

            //Focus textBox3
            FocusTestHelper::EnsureFocus(textBox3, FocusState::Keyboard);

            RunOnUIThread([&]()
            {
                textBox3->SelectAll();
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Ctrl+X--->Cut...");
            TestServices::KeyboardHelper->Cut();
            TestServices::WindowHelper->WaitForIdle();
            textBox3CuttingEvent->WaitForDefault();

            clipboardHelper.WaitForContentChangedEvent();
            TestServices::WindowHelper->WaitForIdle();
            clipboardHelper.VerifyClipboardText(textBox3Text);
            clipboardHelper.ResetContentChangedEvent();


            //Focus textBox
            FocusTestHelper::EnsureFocus(textBox, FocusState::Keyboard);

            RunOnUIThread([&]()
            {
                textBox->SelectAll();
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Ctrl+X--->Cut...");
            TestServices::KeyboardHelper->Cut();
            TestServices::WindowHelper->WaitForIdle();
            textBoxCuttingEvent->WaitForDefault();

            VERIFY_IS_FALSE(clipboardHelper.ContentChangedEventHasFired());
            TestServices::WindowHelper->WaitForIdle();
            clipboardHelper.VerifyClipboardText(textBox3Text);


            //Focus textBox2
            FocusTestHelper::EnsureFocus(textBox2, FocusState::Keyboard);

            LOG_OUTPUT(L"Ctrl+V--->Paste...");
            TestServices::KeyboardHelper->Paste();
            TestServices::WindowHelper->WaitForIdle();
            textBox2PasteEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(textBox3->Text->IsEmpty());
                //Verify text was not cut
                VERIFY_ARE_EQUAL(textBoxText, textBox->Text);
                VERIFY_ARE_EQUAL(textBox3Text, textBox2->Text);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBoxKeyInputTests::CheckEuroSignInput()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto textBoxTextChangedEvent = std::make_shared<Event>();
            auto textBoxTextChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, TextChanged);
            auto textBoxGotFocusEvent = std::make_shared<Event>();
            auto textBoxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);
            auto textBox2GotFocusEvent = std::make_shared<Event>();
            auto textBox2GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::TextBox^ textBox2 = nullptr;
            xaml_controls::Button^ button = nullptr;
            xaml::XamlRoot^ xamlRoot = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <TextBox x:Name='textBox' FontSize='20' Width='200' Height='80' Margin ='20,5,20,0'/>"
                    L"  <TextBox x:Name='textBox2' FontSize='20' Width='200' Height='80' Margin ='20,5,20,0'/>"
                    L"</StackPanel>"));
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                textBox2 = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox2"));
                VERIFY_IS_NOT_NULL(textBox2);
                TestServices::WindowHelper->WindowContent = rootPanel;

                textBoxGotFocusRegistration.Attach(
                    textBox,
                    [textBoxGotFocusEvent]()
                {
                    LOG_OUTPUT(L"TextBox control GotFocus handler.");
                    textBoxGotFocusEvent->Set();
                });

                textBox2GotFocusRegistration.Attach(
                    textBox2,
                    [textBox2GotFocusEvent]()
                {
                    LOG_OUTPUT(L"TextBox2 control GotFocus handler.");
                    textBox2GotFocusEvent->Set();
                });

                textBoxTextChangedRegistration.Attach(
                    textBox,
                    [textBoxTextChangedEvent]()
                {
                    LOG_OUTPUT(L"TextBox control TextChanged handler.");
                    textBoxTextChangedEvent->Set();
                });
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Setting focus to the TextBox control.");
            RunOnUIThread([&]()
            {
                xamlRoot = textBox->XamlRoot;
                textBox->Focus(FocusState::Pointer);
            });
            textBoxGotFocusEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Pressing Ctrl+ALT+E while input lanauge is English");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_alt#$d$_e#$u$_e#$u$_alt#$u$_ctrl");
            TestServices::WindowHelper->WaitForIdle();
            textBoxTextChangedEvent->WaitForDefault();

            Platform::String ^strEuro = L"???";

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Input Language English, TextInput:%ws", textBox->Text->Data());
                // Expecting Euro sign for CTRL_ALT_E when current input language is English
                VERIFY_ARE_EQUAL(textBox->Text, strEuro);
                textBox->Text = "";
            });

            TestServices::WindowHelper->WaitForIdle();
            textBoxTextChangedEvent->Reset();

            //Inject WM_INPUTLANGCHANGE message to simulate user switching to Polish input
            TestServices::WindowHelper->InjectWindowMessage(WM_INPUTLANGCHANGE, 0, 0x0415, xamlRoot);
            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Pressing Ctrl+ALT+E while input lanauge is Polish");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_lctrl#$d$_lalt#$d$_e#$u$_e#$u$_lalt#$u$_lctrl");
            TestServices::WindowHelper->WaitForIdle();
            textBoxTextChangedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Input Language Polish, TextInput:%ws", textBox->Text->Data());
                // Not expecting Euro sign for CTRL_ALT_E when current input language is Polish
                VERIFY_ARE_NOT_EQUAL(textBox->Text, strEuro);
                textBox->Text = "";
            });

            TestServices::WindowHelper->WaitForIdle();

            // Testing scenarios when input language is changed while editing textbox out of focus
            LOG_OUTPUT(L"Setting focus to the TextBox2.");
            RunOnUIThread([&]()
            {
                textBox2->Focus(FocusState::Pointer);
            });
            TestServices::WindowHelper->WaitForIdle();
            textBox2GotFocusEvent->WaitForDefault();

            textBoxGotFocusEvent->Reset();
            LOG_OUTPUT(L"Setting focus back to the TextBox.");
            RunOnUIThread([&]()
            {
                textBox->Focus(FocusState::Pointer);
            });
            textBoxGotFocusEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            textBoxTextChangedEvent->Reset();
            // Even though we injected Polish input language earlier, TextBox will query the current input language in its ::GotFocus().
            // This achieves same effect as input language changed when it is out of focus
            LOG_OUTPUT(L"Pressing Ctrl+ALT+E while input language is English");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_alt#$d$_e#$u$_e#$u$_alt#$u$_ctrl");
            TestServices::WindowHelper->WaitForIdle();
            textBoxTextChangedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Input Language English, TextInput:%ws", textBox->Text->Data());
                VERIFY_ARE_EQUAL(textBox->Text, strEuro);
                textBox->Text = "";
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                textBox->IsEnabled = false; // disable textbox in case SIP is opened.
            });
        }

        void TextBoxKeyInputTests::CheckMarlettFont()
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
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <TextBox x:Name='textBox' FontSize='50' Width='400' IsSpellCheckEnabled='False' TextWrapping='NoWrap' FontFamily='Marlett' Margin ='20,5,20,0'/>"
                    L"</StackPanel>"));
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(textBox, FocusState::Pointer);
            TestServices::WindowHelper->WaitForIdle();

            TestServices::KeyboardHelper->PressKeySequence(L"efgh");
            TestServices::WindowHelper->WaitForIdle();

            TestServices::KeyboardHelper->Backspace(); // press backspace to delete a char
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                textBox->IsEnabled = false;
            });

            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Recording DComp tree to make sure Marlett glyph can be deleted after backspace key press.");
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);
        }

        void TextBoxKeyInputTests::CheckCtrlAltV()
        {
            TestCleanupWrapper cleanup;
            KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto textBoxTextChangedEvent = std::make_shared<Event>();
            auto textBoxTextChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, TextChanged);
            auto textBoxGotFocusEvent = std::make_shared<Event>();
            auto textBoxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);
            auto reBoxGotFocusEvent = std::make_shared<Event>();
            auto reBoxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, GotFocus);

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::RichEditBox^ reBox = nullptr;
            xaml_controls::Button^ button = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <TextBox x:Name='textBox' FontSize='20' Width='200' Height='80' Margin ='20,5,20,0' ContextFlyout='{x:Null}' SelectionFlyout='{x:Null}'/>"
                    L"  <RichEditBox x:Name='reBox' Width='200' Height='60' Margin='20,40,20,0' ContextFlyout='{x:Null}' SelectionFlyout='{x:Null}'/>"
                    L"</StackPanel>"));

                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                reBox = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"reBox"));
                VERIFY_IS_NOT_NULL(reBox);
                TestServices::WindowHelper->WindowContent = rootPanel;

                textBoxGotFocusRegistration.Attach(
                    textBox,
                    [textBoxGotFocusEvent]()
                {
                    LOG_OUTPUT(L"TextBox control GotFocus handler.");
                    textBoxGotFocusEvent->Set();
                });

                reBoxGotFocusRegistration.Attach(
                    reBox,
                    [reBoxGotFocusEvent]()
                {
                    LOG_OUTPUT(L"reBox control GotFocus handler.");
                    reBoxGotFocusEvent->Set();
                });

                textBoxTextChangedRegistration.Attach(
                    textBox,
                    [textBoxTextChangedEvent]()
                {
                    LOG_OUTPUT(L"TextBox control TextChanged handler.");
                    textBoxTextChangedEvent->Set();
                });
            });

            TestServices::WindowHelper->WaitForIdle();

            // Switch focus to RichEditBox and perform copy to make sure clipboard is filled with some contents

            LOG_OUTPUT(L"Setting focus to the reBox by tapping.");
            TestServices::InputHelper->Tap(reBox);
            TestServices::WindowHelper->WaitForIdle();
            reBoxGotFocusEvent->WaitForDefault();
            Platform::String ^strToType = "CtrlAltVTest";

            RunOnUIThread([&]()
            {
                reBox->Document->SetText(mut::TextSetOptions::None, strToType);
                reBox->Document->Selection->StartPosition = 0;
                reBox->Document->Selection->EndPosition = 12;
                reBox->Document->Selection->Copy();
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Setting focus back to the TextBox by tapping.");
            TestServices::InputHelper->Tap(textBox);
            textBoxGotFocusEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Pressing Ctrl+ALT+V and it should not trigger paste");
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_altscan#$d$_v#$u$_v#$u$_altscan#$u$_ctrlscan");
            TestServices::WindowHelper->WaitForIdle();
            textBoxTextChangedEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
            VERIFY_IS_FALSE(textBoxTextChangedEvent->HasFired());

            LOG_OUTPUT(L"Pressing Ctrl+ALT+V but release alt key before v key");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_altscan#$u$_altscan#$d$_v#$u$_v#$u$_ctrlscan");
            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Effectively it is ctrl-v and should trigger paste.");
            textBoxTextChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"TextInput:%s", textBox->Text->Data());
                VERIFY_IS_TRUE(textBox->Text == strToType);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBoxKeyInputTests::IsReadOnlyCheck()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto textBoxTextChangedEvent = std::make_shared<Event>();
            auto textBoxTextChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, TextChanged);
            auto textBoxGotFocusEvent = std::make_shared<Event>();
            auto textBoxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::Button^ button = nullptr;
            Platform::String ^strToType = "Simple text input...";

            UINT numberOfTextChangedEvents = 0;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <TextBox x:Name='textBox' FontSize='20' Width='200' Height='80' Margin ='20,5,20,0'>"
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

                textBoxTextChangedRegistration.Attach(
                    textBox,
                    ref new xaml_controls::TextChangedEventHandler(
                        [&](Platform::Object^, xaml_controls::TextChangedEventArgs^)
                {
                    numberOfTextChangedEvents++;
                    LOG_OUTPUT(L"TextBox control TextChanged handler, occurrence=%d", numberOfTextChangedEvents);
                    LOG_OUTPUT(L"Current TextInput:%s", textBox->Text->Data());
                    if (numberOfTextChangedEvents >= strToType->Length()) // Wait for all key strokes, each key stroke will fire TextChanged event once
                    {
                        textBoxTextChangedEvent->Set();
                    }
                }));
            });

            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                textBox->IsReadOnly = true;
                textBox->Focus(FocusState::Keyboard);
            });

            textBoxGotFocusEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            TestServices::KeyboardHelper->PressKeySequence(strToType);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"TextInput:%s", textBox->Text->Data());
                VERIFY_IS_TRUE(textBox->Text->Length() == 0);
                textBox->IsReadOnly = false; // switch IsReadOnly back to false
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::KeyboardHelper->PressKeySequence(strToType);
            TestServices::WindowHelper->WaitForIdle();
            textBoxTextChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"TextInput:%s", textBox->Text->Data());
                VERIFY_ARE_EQUAL(textBox->Text, strToType);
                textBox->IsReadOnly = true;
            });
            TestServices::WindowHelper->WaitForIdle();

            //Try cut, copy and paste short cut key to make sure no crash
            TestServices::KeyboardHelper->Cut();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->Copy();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->Paste();
            TestServices::WindowHelper->WaitForIdle();

        }

        void TextBoxKeyInputTests::HomeAndEndKeyInput()
        {
            TestCleanupWrapper cleanup;
            KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto textBoxSelectionChangedEvent = std::make_shared<Event>();
            auto textBoxSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, SelectionChanged);

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::Button^ button = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <TextBox x:Name='textBox' TextWrapping='Wrap' AcceptsReturn='True' FontSize='20' Width='200' Height='200' Margin ='20,5,20,0'>"
                    L"  </TextBox>"
                    L"</StackPanel>"));
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                TestServices::WindowHelper->WindowContent = rootPanel;

                textBoxSelectionChangedRegistration.Attach(
                    textBox,
                    ref new xaml::RoutedEventHandler(
                        [textBoxSelectionChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"TextBox.SelectionChanged handler.");
                    textBoxSelectionChangedEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(textBox, FocusState::Keyboard);

            const int lines = 100;

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"\r\nGenerating first half %d lines of text...", lines / 2);
                for (int line = 1; line <= lines / 2; line++)
                {
                    textBox->Text += L"Line" + line + "\r\n";
                }
            });

            int middleOfLines = 0;
            RunOnUIThread([&]()
            {
                middleOfLines = textBox->Text->Length();
                LOG_OUTPUT(L"\r\nGenerating second half  %d lines of text...", lines/2);
                for (int line = lines/2+1; line < lines; line++)
                {
                    textBox->Text += L"Line" + line + "\r\n";
                }
                LOG_OUTPUT(L"\r\nMove cursor to the middle of text:%d.", middleOfLines);
                textBox->SelectionStart = middleOfLines;
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"\r\nEnd key, cursor should move to the end of the line...");
            textBoxSelectionChangedEvent->Reset();
            TestServices::KeyboardHelper->End();
            textBoxSelectionChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"SelectionStart=%d, SelectionLength=%d", textBox->SelectionStart, textBox->SelectionLength);
                VERIFY_ARE_EQUAL(textBox->SelectionStart, middleOfLines + 6);
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"\r\nHome key, cursor should move to the beginning of the line...");
            TestServices::KeyboardHelper->Home();
            textBoxSelectionChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"SelectionStart=%d, SelectionLength=%d", textBox->SelectionStart, textBox->SelectionLength);
                VERIFY_ARE_EQUAL(textBox->SelectionStart, middleOfLines);
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"\r\nCtrl-End key, cursor should move to the end of the document...");
            TestServices::KeyboardHelper->PressKeySequence("$d$_ctrlscan#$d$_end#$u$_end#$u$_ctrlscan");
            textBoxSelectionChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"SelectionStart=%d, SelectionLength=%d", textBox->SelectionStart, textBox->SelectionLength);
                VERIFY_ARE_EQUAL(textBox->SelectionStart, static_cast<int>(textBox->Text->Length()));
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"\r\nCtrl-Home key, cursor should move to the beginning of the document...");
            TestServices::KeyboardHelper->PressKeySequence("$d$_ctrlscan#$d$_home#$u$_home#$u$_ctrlscan");
            textBoxSelectionChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"SelectionStart=%d, SelectionLength=%d", textBox->SelectionStart, textBox->SelectionLength);
                VERIFY_ARE_EQUAL(textBox->SelectionStart, 0);
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"\r\nCtrl-End key with ctrl key released first, cursor should move to the end of the document...");
            TestServices::KeyboardHelper->PressKeySequence("$d$_ctrlscan#$d$_end#$u$_ctrlscan#$u$_end");
            textBoxSelectionChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"SelectionStart=%d, SelectionLength=%d", textBox->SelectionStart, textBox->SelectionLength);
                VERIFY_ARE_EQUAL(textBox->SelectionStart, static_cast<int>(textBox->Text->Length()));
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"\r\nCtrl-Home key with ctrl key released first, cursor should move to the beginning of the document...");
            TestServices::KeyboardHelper->PressKeySequence("$d$_ctrlscan#$d$_home#$u$_ctrlscan#$u$_home");
            textBoxSelectionChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"SelectionStart=%d, SelectionLength=%d", textBox->SelectionStart, textBox->SelectionLength);
                VERIFY_ARE_EQUAL(textBox->SelectionStart, 0);
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"\r\nCtrl-Shift-End from top of the document should select everything...");
            TestServices::KeyboardHelper->PressKeySequence("$d$_ctrlscan#$d$_shift#$d$_end#$u$_end#$u$_shift#$u$_ctrlscan");
            textBoxSelectionChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"SelectionStart=%d, SelectionLength=%d", textBox->SelectionStart, textBox->SelectionLength);
                VERIFY_ARE_EQUAL(textBox->SelectionStart, 0);
                VERIFY_ARE_EQUAL(textBox->SelectionLength, static_cast<int>(textBox->Text->Length()));
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"\r\nPressing down arrow should remove selection and have the cursor at bottom of document...");
            TestServices::KeyboardHelper->Down();
            TestServices::WindowHelper->WaitForIdle();
            textBoxSelectionChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"SelectionStart=%d, SelectionLength=%d", textBox->SelectionStart, textBox->SelectionLength);
                VERIFY_ARE_EQUAL(textBox->SelectionLength, 0);
                VERIFY_ARE_EQUAL(textBox->SelectionStart, static_cast<int>(textBox->Text->Length()));
            });

            LOG_OUTPUT(L"\r\nCtrl-Shift-Home from bottom of the document should select everything...");
            TestServices::KeyboardHelper->PressKeySequence("$d$_ctrlscan#$d$_shift#$d$_home#$u$_home#$u$_shift#$u$_ctrlscan");
            textBoxSelectionChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"SelectionStart=%d, SelectionLength=%d", textBox->SelectionStart, textBox->SelectionLength);
                VERIFY_ARE_EQUAL(textBox->SelectionStart, 0);
                VERIFY_ARE_EQUAL(textBox->SelectionLength, static_cast<int>(textBox->Text->Length()));
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"\r\nPressing up arrow should remove selection and have the cursor at beginning of document...");
            TestServices::KeyboardHelper->Up();
            TestServices::WindowHelper->WaitForIdle();
            textBoxSelectionChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"SelectionStart=%d, SelectionLength=%d", textBox->SelectionStart, textBox->SelectionLength);
                VERIFY_ARE_EQUAL(textBox->SelectionLength, 0);
                VERIFY_ARE_EQUAL(textBox->SelectionStart, 0);
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"\r\nShift-End should select the line...");
            TestServices::KeyboardHelper->PressKeySequence("$d$_shift#$d$_end#$u$_end#$u$_shift");
            textBoxSelectionChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"SelectionStart=%d, SelectionLength=%d", textBox->SelectionStart, textBox->SelectionLength);
                VERIFY_ARE_EQUAL(textBox->SelectionStart, 0);
                VERIFY_ARE_EQUAL(textBox->SelectionLength, 6);
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"\r\nMove cursor to end of the line and Shift-Home should select the line...");
            RunOnUIThread([&]()
            {
                textBox->SelectionStart = 5;
                textBox->SelectionLength = 0;
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->PressKeySequence("$d$_shift#$d$_home#$u$_home#$u$_shift");
            textBoxSelectionChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"SelectionStart=%d, SelectionLength=%d", textBox->SelectionStart, textBox->SelectionLength);
                VERIFY_ARE_EQUAL(textBox->SelectionStart, 0);
                VERIFY_ARE_EQUAL(textBox->SelectionLength, 5);
            });
            TestServices::WindowHelper->WaitForIdle();

        }

        void TextBoxKeyInputTests::UndoRedoTextInput()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto textBoxTextChangedEvent = std::make_shared<Event>();
            auto textBoxTextChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, TextChanged);

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::Button^ button = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <TextBox x:Name='textBox' FontSize='20' Width='200' Height='80' Margin ='20,5,20,0'>"
                    L"  </TextBox>"
                    L"</StackPanel>"));
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                TestServices::WindowHelper->WindowContent = rootPanel;

                textBoxTextChangedRegistration.Attach(
                    textBox,
                    ref new xaml_controls::TextChangedEventHandler(
                        [&](Platform::Object^, xaml_controls::TextChangedEventArgs^)
                {
                    LOG_OUTPUT(L"Current TextInput:%s", textBox->Text->Data());
                    textBoxTextChangedEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();

            FocusTestHelper::EnsureFocus(textBox, FocusState::Keyboard);

            LOG_OUTPUT(L"Input a.");
            TestServices::KeyboardHelper->PressKeySequence(L"a");
            textBoxTextChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Input b.");
            TestServices::KeyboardHelper->PressKeySequence(L"b");
            textBoxTextChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Press Ctrl-Z to undo ab.");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_z#$u$_z#$u$_ctrl");
            textBoxTextChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(textBox->Text->Length() == 0);
            });

            LOG_OUTPUT(L"Press Ctrl-Y to redo ab.");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_y#$u$_y#$u$_ctrl");
            textBoxTextChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(textBox->Text, ref new Platform::String(L"ab"));
            });

            LOG_OUTPUT(L"Press Ctrl-Z to undo ab.");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_z#$u$_z#$u$_ctrl");
            textBoxTextChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(textBox->Text->Length() == 0);
            });

            LOG_OUTPUT(L"Input a.");
            TestServices::KeyboardHelper->PressKeySequence(L"a");
            textBoxTextChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Press Ctrl-Z to undo a.");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_z#$u$_z#$u$_ctrl");
            textBoxTextChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(textBox->Text->Length() == 0);
            });

            LOG_OUTPUT(L"Input b.");
            TestServices::KeyboardHelper->PressKeySequence(L"b");
            textBoxTextChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Press Alt-Back to undo b.");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_alt#$d$_backspace#$u$_backspace#$u$_alt");
            textBoxTextChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(textBox->Text->Length() == 0);
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBoxKeyInputTests::InsertMode()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto textBoxTextChangedEvent = std::make_shared<Event>();
            auto textBoxTextChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, TextChanged);
            auto textBoxSelectionChangedEvent = std::make_shared<Event>();
            auto textBoxSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, SelectionChanged);

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::Button^ button = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <TextBox x:Name='textBox' FontSize='20' Width='200' Height='80' Margin ='20,5,20,0'>"
                    L"  </TextBox>"
                    L"</StackPanel>"));
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                TestServices::WindowHelper->WindowContent = rootPanel;

                textBoxTextChangedRegistration.Attach(
                    textBox,
                    ref new xaml_controls::TextChangedEventHandler(
                        [&](Platform::Object^, xaml_controls::TextChangedEventArgs^)
                {
                    LOG_OUTPUT(L"Current TextInput:%s", textBox->Text->Data());
                    textBoxTextChangedEvent->Set();
                }));

                textBoxSelectionChangedRegistration.Attach(
                    textBox,
                    ref new xaml::RoutedEventHandler(
                        [&](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"TextBox.SelectionChanged handler.");
                    textBoxSelectionChangedEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();

            FocusTestHelper::EnsureFocus(textBox, FocusState::Keyboard);

            LOG_OUTPUT(L"Input a.");
            TestServices::KeyboardHelper->PressKeySequence(L"a");
            textBoxTextChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Turn on insert mode.");
            TestServices::KeyboardHelper->Insert();
            TestServices::WindowHelper->WaitForIdle();

            textBoxSelectionChangedEvent->Reset();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Move cursor to the beginning.");
                textBox->Select(0, 0);
            });
            textBoxSelectionChangedEvent->WaitForDefault();

            textBoxTextChangedEvent->Reset();
            LOG_OUTPUT(L"Input b.");
            TestServices::KeyboardHelper->PressKeySequence(L"b");
            textBoxTextChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(textBox->Text, ref new Platform::String(L"b"));
            });

            LOG_OUTPUT(L"Turn off insert mode.");
            TestServices::KeyboardHelper->Insert();
            TestServices::WindowHelper->WaitForIdle();

            textBoxSelectionChangedEvent->Reset();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Move cursor to the beginning.");
                textBox->Select(0, 0);
            });
            textBoxSelectionChangedEvent->WaitForDefault();

            textBoxTextChangedEvent->Reset();
            LOG_OUTPUT(L"Input c.");
            TestServices::KeyboardHelper->PressKeySequence(L"c");
            textBoxTextChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(textBox->Text, ref new Platform::String(L"cb"));
            });
        }

        void TextBoxKeyInputTests::UndoRedoGrouping()
        {
            TestCleanupWrapper cleanup;
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
            FocusTestHelper::EnsureFocus(rebx, FocusState::Keyboard);

            RunOnUIThread([&]()
            {
                rebx->Document->SetText(Microsoft::UI::Text::TextSetOptions::None, "Test");
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Begin undo grouping.");
                rebx->Document->BeginUndoGroup();
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Perform some format change on selection object.");
                rebx->Document->Selection->CharacterFormat->Bold = Microsoft::UI::Text::FormatEffect::On;
                rebx->Document->Selection->CharacterFormat->ForegroundColor = Microsoft::UI::Colors::Red;
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Do some text editing.");
            TestServices::KeyboardHelper->PressKeySequence("abcd");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                rebx->Document->EndUndoGroup();
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Undo and it should back out the all changes include formatting and text editing.");
                rebx->Document->Undo();
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_NOT_EQUAL(rebx->Document->Selection->CharacterFormat->Bold, Microsoft::UI::Text::FormatEffect::On);
                VERIFY_IS_FALSE(IsSameColor(rebx->Document->Selection->CharacterFormat->ForegroundColor, Microsoft::UI::Colors::Red));
                Platform::String^ text;
                rebx->Document->GetText(Microsoft::UI::Text::TextGetOptions::None, &text);
                VERIFY_ARE_EQUAL(text, ref new Platform::String(L"Test\r"));
            });

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Redo and it should replay all changes include formatting and text editing.");
                rebx->Document->Redo();
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(rebx->Document->Selection->CharacterFormat->Bold, Microsoft::UI::Text::FormatEffect::On);
                VERIFY_IS_TRUE(IsSameColor(rebx->Document->Selection->CharacterFormat->ForegroundColor, Microsoft::UI::Colors::Red));
                Platform::String^ text;
                rebx->Document->GetText(Microsoft::UI::Text::TextGetOptions::None, &text);
                VERIFY_ARE_EQUAL(text, ref new Platform::String(L"abcdTest\r"));
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Undo with Ctrl-Z it should back out the all changes include formatting and text editing.");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_z#$u$_z#$u$_ctrl");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_NOT_EQUAL(rebx->Document->Selection->CharacterFormat->Bold, Microsoft::UI::Text::FormatEffect::On);
                VERIFY_IS_FALSE(IsSameColor(rebx->Document->Selection->CharacterFormat->ForegroundColor, Microsoft::UI::Colors::Red));
                Platform::String^ text;
                rebx->Document->GetText(Microsoft::UI::Text::TextGetOptions::None, &text);
                VERIFY_ARE_EQUAL(text, ref new Platform::String(L"Test\r"));
            });

            LOG_OUTPUT(L"Redo with Ctrl-Y and it should replay all changes include formatting and text editing.");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_y#$u$_y#$u$_ctrl");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(rebx->Document->Selection->CharacterFormat->Bold, Microsoft::UI::Text::FormatEffect::On);
                VERIFY_IS_TRUE(IsSameColor(rebx->Document->Selection->CharacterFormat->ForegroundColor, Microsoft::UI::Colors::Red));
                Platform::String^ text;
                rebx->Document->GetText(Microsoft::UI::Text::TextGetOptions::None, &text);
                VERIFY_ARE_EQUAL(text, ref new Platform::String(L"abcdTest\r"));
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBoxKeyInputTests::InsertKeyInput()
        {
            TestCleanupWrapper cleanup;
            KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto textBoxTextChangedEvent = std::make_shared<Event>();
            auto textBoxTextChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, TextChanged);

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::Button^ button = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <TextBox x:Name='textBox' Text='aaa' FontSize='20' Width='200' Height='80' Margin ='20,5,20,0'>"
                    L"  </TextBox>"
                    L"</StackPanel>"));
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                TestServices::WindowHelper->WindowContent = rootPanel;

                textBoxTextChangedRegistration.Attach(
                    textBox,
                    ref new xaml_controls::TextChangedEventHandler(
                        [&](Platform::Object^, xaml_controls::TextChangedEventArgs^)
                {
                    LOG_OUTPUT(L"Current TextInput:%s", textBox->Text->Data());
                    textBoxTextChangedEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();

            FocusTestHelper::EnsureFocus(textBox, FocusState::Keyboard);

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"move cursor to front.");
                textBox->SelectionStart = 0;
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Press Insert key, textbox should now be in the overtype mode.");
            TestServices::KeyboardHelper->Insert();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Input b.");
            textBoxTextChangedEvent->Reset();
            TestServices::KeyboardHelper->PressKeySequence(L"b");
            textBoxTextChangedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(textBox->Text, ref new Platform::String(L"baa"));
            });

            LOG_OUTPUT(L"Press Insert key again, textbox should now be in the insert mode.");
            TestServices::KeyboardHelper->Insert();
            LOG_OUTPUT(L"Input b.");
            TestServices::KeyboardHelper->PressKeySequence(L"b");
            textBoxTextChangedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(textBox->Text, ref new Platform::String(L"bbaa"));
            });

            LOG_OUTPUT(L"Press Insert key again,textbox should now be in the overtype mode.");
            TestServices::KeyboardHelper->Insert();
            LOG_OUTPUT(L"Input b.");
            TestServices::KeyboardHelper->PressKeySequence(L"b");
            textBoxTextChangedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(textBox->Text, ref new Platform::String(L"bbba"));
                LOG_OUTPUT(L"Select all text.");
                textBox->SelectAll();
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Input b.");
            TestServices::KeyboardHelper->PressKeySequence(L"b");
            textBoxTextChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(textBox->Text, ref new Platform::String(L"b"));
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBoxKeyInputTests::BackspaceKeyInput()
        {
            TestCleanupWrapper cleanup;
            KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto textBoxTextChangedEvent = std::make_shared<Event>();
            auto textBoxTextChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, TextChanged);
            auto textBoxSelectionChangedEvent = std::make_shared<Event>();
            auto textBoxSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, SelectionChanged);

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::Button^ button = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <TextBox x:Name='textBox' Text='aaa bbb' FontSize='20' Width='200' Height='80' Margin ='20,5,20,0'>"
                    L"  </TextBox>"
                    L"</StackPanel>"));
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                TestServices::WindowHelper->WindowContent = rootPanel;

                textBoxTextChangedRegistration.Attach(
                    textBox,
                    ref new xaml_controls::TextChangedEventHandler(
                        [&](Platform::Object^, xaml_controls::TextChangedEventArgs^)
                {
                    LOG_OUTPUT(L"Current TextInput:%s", textBox->Text->Data());
                    textBoxTextChangedEvent->Set();
                }));

                textBoxSelectionChangedRegistration.Attach(
                    textBox,
                    ref new xaml::RoutedEventHandler(
                        [textBoxSelectionChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"TextBox.SelectionChanged handler.");
                    textBoxSelectionChangedEvent->Set();
                }));

            });

            TestServices::WindowHelper->WaitForIdle();

            FocusTestHelper::EnsureFocus(textBox, FocusState::Keyboard);

            textBoxSelectionChangedEvent->Reset();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"move cursor to front.");
                textBox->SelectionStart = 0;
            });
            TestServices::WindowHelper->WaitForIdle();

            textBoxTextChangedEvent->Reset();
            LOG_OUTPUT(L"Press Backspace key.");
            TestServices::KeyboardHelper->Backspace();
            textBoxTextChangedEvent->WaitForNoThrow(std::chrono::milliseconds(200));
            VERIFY_IS_FALSE(textBoxTextChangedEvent->HasFired());

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Nothing should be removed from Text.");
                VERIFY_ARE_EQUAL(textBox->Text, ref new Platform::String(L"aaa bbb"));
                LOG_OUTPUT(L"move cursor to end.");
                textBox->SelectionStart = textBox->Text->Length();
            });
            textBoxSelectionChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Press Backspace key again.");
            textBoxTextChangedEvent->Reset();
            TestServices::KeyboardHelper->Backspace();
            textBoxTextChangedEvent->WaitForDefault();
            textBoxSelectionChangedEvent->Reset();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(textBox->Text, ref new Platform::String(L"aaa bb"));
                LOG_OUTPUT(L"move cursor to middle.");
                textBox->SelectionStart = 4;
            });
            textBoxSelectionChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Press Backspace key again.");
            textBoxTextChangedEvent->Reset();
            TestServices::KeyboardHelper->Backspace();
            textBoxTextChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(textBox->Text, ref new Platform::String(L"aaabb"));
                LOG_OUTPUT(L"Select all text.");
                textBox->SelectAll();
            });
            textBoxSelectionChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Press Backspace key again.");
            textBoxTextChangedEvent->Reset();
            TestServices::KeyboardHelper->Backspace();
            textBoxTextChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(textBox->Text, ref new Platform::String(L""));
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBoxKeyInputTests::DeleteKeyInput()
        {
            TestCleanupWrapper cleanup;
            KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto textBoxTextChangedEvent = std::make_shared<Event>();
            auto textBoxTextChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, TextChanged);
            auto textBoxSelectionChangedEvent = std::make_shared<Event>();
            auto textBoxSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, SelectionChanged);

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::Button^ button = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <TextBox x:Name='textBox' Text='aaa bbb' FontSize='20' Width='200' Height='80' Margin ='20,5,20,0'>"
                    L"  </TextBox>"
                    L"</StackPanel>"));
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                TestServices::WindowHelper->WindowContent = rootPanel;

                textBoxTextChangedRegistration.Attach(
                    textBox,
                    ref new xaml_controls::TextChangedEventHandler(
                        [&](Platform::Object^, xaml_controls::TextChangedEventArgs^)
                {
                    LOG_OUTPUT(L"Current TextInput:%s", textBox->Text->Data());
                    textBoxTextChangedEvent->Set();
                }));

                textBoxSelectionChangedRegistration.Attach(
                    textBox,
                    ref new xaml::RoutedEventHandler(
                        [textBoxSelectionChangedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"TextBox.SelectionChanged handler.");
                    textBoxSelectionChangedEvent->Set();
                }));

            });

            TestServices::WindowHelper->WaitForIdle();

            FocusTestHelper::EnsureFocus(textBox, FocusState::Keyboard);

            textBoxSelectionChangedEvent->Reset();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"move cursor to end.");
                textBox->SelectionStart = textBox->Text->Length();
            });

            textBoxSelectionChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Press Delete key.");
            TestServices::KeyboardHelper->Delete();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Nothing should be removed from Text.");
                VERIFY_ARE_EQUAL(textBox->Text, ref new Platform::String(L"aaa bbb"));
                LOG_OUTPUT(L"move cursor to front.");
                textBox->SelectionStart = 0;
            });
            textBoxSelectionChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Press Delete key again.");
            textBoxTextChangedEvent->Reset();
            TestServices::KeyboardHelper->Delete();
            textBoxTextChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(textBox->Text, ref new Platform::String(L"aa bbb"));
                LOG_OUTPUT(L"move cursor to middle.");
                textBox->SelectionStart = 3;
            });
            textBoxSelectionChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Press Delete key again.");
            textBoxTextChangedEvent->Reset();
            TestServices::KeyboardHelper->Delete();
            textBoxTextChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(textBox->Text, ref new Platform::String(L"aa bb"));
                LOG_OUTPUT(L"Select all text.");
                textBox->SelectAll();
            });
            textBoxSelectionChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Press Delete key again.");
            textBoxTextChangedEvent->Reset();
            TestServices::KeyboardHelper->Delete();
            textBoxTextChangedEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(textBox->Text, ref new Platform::String(L""));
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBoxKeyInputTests::FamilyEmoji()
        {
            EmojiTestHelper(L"FamilyEmoji.rtf");
        }

        void TextBoxKeyInputTests::DetectiveEmoji()
        {
            EmojiTestHelper(L"DetectiveEmoji.rtf");
        }

        void TextBoxKeyInputTests::EmojiTestHelper(Platform::String^ emojiXamlFile)
        {
            TestCleanupWrapper cleanup;
            KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            auto richEditBoxTextChangedEvent = std::make_shared<Event>();
            auto richEditBoxTextChangedRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, TextChanged);

            xaml_controls::RichEditBox^ richEditBox = nullptr;
            xaml_controls::Button^ button = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <RichEditBox x:Name='richEditBox' Width='380' Height='120' />"
                    L"</StackPanel>"));
                richEditBox = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"richEditBox"));
                VERIFY_IS_NOT_NULL(richEditBox);
                TestServices::WindowHelper->WindowContent = rootPanel;

                richEditBoxTextChangedRegistration.Attach(
                    richEditBox,
                    ref new xaml::RoutedEventHandler(
                        [&](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    richEditBoxTextChangedEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();

            auto pRtfLoadedEvent = std::make_shared<Event>();
            create_task(StorageFile::GetFileFromPathAsync(GetResourcesPath() + emojiXamlFile))
                .then([=](StorageFile^ pFile)
            {
                VERIFY_IS_NOT_NULL(pFile);

                richEditBoxTextChangedEvent->Reset();
                create_task(pFile->OpenAsync(::Windows::Storage::FileAccessMode::Read))
                    .then([=](IRandomAccessStream^ pFileStream)
                {
                    VERIFY_IS_NOT_NULL(pFileStream);

                    RunOnUIThread([=]()
                    {
                        richEditBox->Document->LoadFromStream(TextSetOptions::FormatRtf, pFileStream);
                        pRtfLoadedEvent->Set();
                    });
                    richEditBoxTextChangedEvent->WaitForDefault(); // Wait until textChanged event fired
                });
            });

            pRtfLoadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Verify emojis are rendered correctly!");
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

            FocusTestHelper::EnsureFocus(richEditBox, FocusState::Keyboard);

            LOG_OUTPUT(L"RichEditBox should now display two family emojis");
            LOG_OUTPUT(L"Press right arrow key, it should put the cursor between two emojis.");
            TestServices::KeyboardHelper->Right();
            TestServices::WindowHelper->WaitForIdle();

            richEditBoxTextChangedEvent->Reset();
            LOG_OUTPUT(L"Press delete key, it should delete the second emoji.");
            TestServices::KeyboardHelper->Delete();
            TestServices::WindowHelper->WaitForIdle();
            richEditBoxTextChangedEvent->WaitForDefault();

            richEditBoxTextChangedEvent->Reset();
            LOG_OUTPUT(L"Press backspace key, it should delete the frst emoji.");
            TestServices::KeyboardHelper->Backspace();
            TestServices::WindowHelper->WaitForIdle();
            richEditBoxTextChangedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                Platform::String ^text;
                Platform::String ^emptyStringWithReturn = "\r";
                richEditBox->Document->GetText(mut::TextGetOptions::None, &text);
                LOG_OUTPUT(L"RichEditBox control TextChanging (Text = '%s')", text->Data());
                VERIFY_ARE_EQUAL(emptyStringWithReturn, text, L"Expecting blank string after emoji symbols are all removed!");
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBoxKeyInputTests::VerifyCharacterCasing()
        {
            TestCleanupWrapper cleanup;
            KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride;

            unsigned textChangingCount = 0;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            auto textBoxTextChangedEvent = std::make_shared<Event>();
            auto textBoxTextChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, TextChanged);

            auto textBoxTextChangingEvent = std::make_shared<Event>();
            auto textBoxTextChangingRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, TextChanging);


            Platform::String^ expectedUpper = "A";
            Platform::String^ expectedLower = "Ab";
            Platform::String^ expectedFinal = "AbaB";

            Platform::String^ expectedText[4] = { expectedUpper, expectedLower, "Aba", expectedFinal };

            xaml_controls::TextBox^ textBox = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <TextBox x:Name='textBox' CharacterCasing='Upper' Width='380' Height='120' />"
                    L"</StackPanel>"));
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                TestServices::WindowHelper->WindowContent = rootPanel;

                textBoxTextChangedRegistration.Attach(
                    textBox,
                    ref new xaml_controls::TextChangedEventHandler(
                        [&](Platform::Object^, xaml_controls::TextChangedEventArgs^)
                {
                    textBoxTextChangedEvent->Set();
                }));

                textBoxTextChangingRegistration.Attach(
                    textBox,
                    ref new wf::TypedEventHandler<xaml_controls::TextBox^, xaml_controls::TextBoxTextChangingEventArgs^>(
                        [&](xaml_controls::TextBox ^sender, xaml_controls::TextBoxTextChangingEventArgs^)
                {
                    VERIFY_IS_TRUE(textChangingCount < 4);
                    VERIFY_ARE_EQUAL(textBox->Text, expectedText[textChangingCount]);
                    textChangingCount++;

                    textBoxTextChangingEvent->Set();
                }));

            });

            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(textBox, FocusState::Keyboard);

            LOG_OUTPUT(L"Input a");
            TestServices::KeyboardHelper->PressKeySequence(L"a");
            textBoxTextChangingEvent->WaitForDefault();
            textBoxTextChangedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(textBox->Text, expectedUpper);
            });
            textBoxTextChangingEvent->Reset();
            textBoxTextChangedEvent->Reset();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Switching casing to Lower");
                textBox->CharacterCasing = Microsoft::UI::Xaml::Controls::CharacterCasing::Lower;
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Input B");
            TestServices::KeyboardHelper->PressKeySequence(L"B");
            textBoxTextChangingEvent->WaitForDefault();
            textBoxTextChangedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(textBox->Text, expectedLower);
            });
            textBoxTextChangingEvent->Reset();
            textBoxTextChangedEvent->Reset();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Switching casing to Normal");
                textBox->CharacterCasing = Microsoft::UI::Xaml::Controls::CharacterCasing::Normal;
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Input a");
            TestServices::KeyboardHelper->PressKeySequence(L"a");
            textBoxTextChangingEvent->WaitForDefault();
            textBoxTextChangedEvent->WaitForDefault();
            textBoxTextChangedEvent->Reset();
            textBoxTextChangingEvent->Reset();

            LOG_OUTPUT(L"Input B");
            TestServices::KeyboardHelper->PressKeySequence(L"B");
            textBoxTextChangingEvent->WaitForDefault();
            textBoxTextChangedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(textBox->Text, expectedFinal);
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBoxKeyInputTests::VerifyCharacterCasingWorksWithPaste()
        {
            TestCleanupWrapper cleanup;
            KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            auto textBoxTextChangedEvent = std::make_shared<Event>();
            auto textBoxTextChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, TextChanged);

            Platform::String^ testString = "Test String";
            Platform::String^ testStringUpper = "TEST STRING";
            Platform::String^ testStringLower = "test string";
            xaml_controls::TextBox^ textBox = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <TextBox x:Name='textBox' Width='380' Height='120' />"
                    L"</StackPanel>"));
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);

                textBox->Text = testString;

                TestServices::WindowHelper->WindowContent = rootPanel;

                textBoxTextChangedRegistration.Attach(
                    textBox,
                    ref new xaml_controls::TextChangedEventHandler(
                        [&](Platform::Object^, xaml_controls::TextChangedEventArgs^)
                {
                    textBoxTextChangedEvent->Set();
                }));

            });

            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(textBox, FocusState::Keyboard);

            ClipboardHelper clipboardHelper;

            RunOnUIThread([&]()
            {
                textBox->SelectAll();
            });
            TestServices::WindowHelper->WaitForIdle();

            //Cut content
            TestServices::KeyboardHelper->Cut();
            textBoxTextChangedEvent->WaitForDefault();
            textBoxTextChangedEvent->Reset();

            //Check clipboard
            clipboardHelper.WaitForContentChangedEvent();
            clipboardHelper.VerifyClipboardText(testString);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Switching casing to Upper");
                VERIFY_IS_TRUE(textBox->Text->IsEmpty());
                textBox->CharacterCasing = Microsoft::UI::Xaml::Controls::CharacterCasing::Upper;
            });
            TestServices::WindowHelper->WaitForIdle();

            //Paste content
            TestServices::KeyboardHelper->Paste();
            textBoxTextChangedEvent->WaitForDefault();
            textBoxTextChangedEvent->Reset();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(textBox->Text, testStringUpper);
            });

            clipboardHelper.ResetContentChangedEvent();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                textBox->SelectAll();
            });
            TestServices::WindowHelper->WaitForIdle();

            //Cut content
            TestServices::KeyboardHelper->Cut();
            textBoxTextChangedEvent->WaitForDefault();
            textBoxTextChangedEvent->Reset();

            //Check clipboard
            clipboardHelper.WaitForContentChangedEvent();
            clipboardHelper.VerifyClipboardText(testStringUpper);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Switching casing to Lower");
                textBox->CharacterCasing = Microsoft::UI::Xaml::Controls::CharacterCasing::Lower;
            });
            TestServices::WindowHelper->WaitForIdle();

            //Paste content
            TestServices::KeyboardHelper->Paste();
            textBoxTextChangedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(textBox->Text, testStringLower);
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBoxKeyInputTests::VerifyCharacterCasingWorksWithPasteInRightToLeftScenarios()
        {
            TestCleanupWrapper cleanup;
            KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            auto textBoxTextChangedEvent = std::make_shared<Event>();
            auto textBoxTextChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, TextChanged);

            Platform::String^ testString = "Test String";
            Platform::String^ testStringUpper = "TEST STRING";
            Platform::String^ testStringLower = "test string";

            xaml_controls::TextBox^ textBox = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <TextBox FlowDirection='RightToLeft' x:Name='textBox' Width='380' Height='120' />"
                    L"</StackPanel>"));
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);

                textBox->Text = testString;

                TestServices::WindowHelper->WindowContent = rootPanel;

                textBoxTextChangedRegistration.Attach(
                    textBox,
                    ref new xaml_controls::TextChangedEventHandler(
                        [&](Platform::Object^, xaml_controls::TextChangedEventArgs^)
                {
                    textBoxTextChangedEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(textBox, FocusState::Keyboard);

            ClipboardHelper clipboardHelper;

            RunOnUIThread([&]()
            {
                textBox->SelectAll();
            });
            TestServices::WindowHelper->WaitForIdle();

            //Cut content
            TestServices::KeyboardHelper->Cut();
            textBoxTextChangedEvent->WaitForDefault();
            textBoxTextChangedEvent->Reset();

            //Check clipboard
            clipboardHelper.WaitForContentChangedEvent();
            clipboardHelper.VerifyClipboardText(testString);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Switching casing to Upper");
                VERIFY_IS_TRUE(textBox->Text->IsEmpty());
                textBox->CharacterCasing = Microsoft::UI::Xaml::Controls::CharacterCasing::Upper;
            });
            TestServices::WindowHelper->WaitForIdle();

            //Paste content
            TestServices::KeyboardHelper->Paste();
            textBoxTextChangedEvent->WaitForDefault();
            textBoxTextChangedEvent->Reset();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(textBox->Text, testStringUpper);
            });

            clipboardHelper.ResetContentChangedEvent();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                textBox->SelectAll();
            });
            TestServices::WindowHelper->WaitForIdle();

            //Cut content
            TestServices::KeyboardHelper->Cut();
            textBoxTextChangedEvent->WaitForDefault();
            textBoxTextChangedEvent->Reset();

            //Check clipboard
            clipboardHelper.WaitForContentChangedEvent();
            clipboardHelper.VerifyClipboardText(testStringUpper);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Switching casing to Lower");
                textBox->CharacterCasing = Microsoft::UI::Xaml::Controls::CharacterCasing::Lower;
            });
            TestServices::WindowHelper->WaitForIdle();

            //Paste content
            TestServices::KeyboardHelper->Paste();
            textBoxTextChangedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(textBox->Text, testStringLower);
            });

            TestServices::WindowHelper->WaitForIdle();
        }


        void TextBoxKeyInputTests::DisabledFormattingAccelerators()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::RichEditBox^ rebx1;
            xaml_controls::RichEditBox^ rebx2;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name = 'button' Content = 'Focus' Margin = '20,40,20,0' />"
                    L"  <RichEditBox x:Name = 'rebx1' Width='200' Height='60' Margin = '20,40,20,0' DisabledFormattingAccelerators='Bold, Italic, Underline' />"
                    L"  <RichEditBox x:Name = 'rebx2' Width='200' Height='60' Margin = '20,40,20,0'/>"
                    L"</StackPanel>"));
                rebx1 = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx1"));
                VERIFY_IS_NOT_NULL(rebx1);
                rebx2 = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName(L"rebx2"));
                VERIFY_IS_NOT_NULL(rebx2);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(rebx1, FocusState::Keyboard);
            LOG_OUTPUT(L"1.Press Ctrl-i/u/b on the formatting accelerator disabled RichEditBox, it should not flip the selection format.");

            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_i#$u$_i#$u$_ctrlscan");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_u#$u$_u#$u$_ctrlscan");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_b#$u$_b#$u$_ctrlscan");
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(rebx1->Document->Selection->CharacterFormat->Italic, mut::FormatEffect::Off);
                VERIFY_ARE_EQUAL(rebx1->Document->Selection->CharacterFormat->Bold, mut::FormatEffect::Off);
                VERIFY_ARE_EQUAL(rebx1->Document->Selection->CharacterFormat->Underline, mut::UnderlineType::None);
            });
            TestServices::WindowHelper->WaitForIdle();

            FocusTestHelper::EnsureFocus(rebx2, FocusState::Keyboard);
            LOG_OUTPUT(L"\r\n2.Press Ctrl-i/u/b on the formatting accelerator enabled RichEditBox, it should flip the selection format.");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_i#$u$_i#$u$_ctrlscan");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_u#$u$_u#$u$_ctrlscan");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_b#$u$_b#$u$_ctrlscan");
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(rebx2->Document->Selection->CharacterFormat->Italic, mut::FormatEffect::On);
                VERIFY_ARE_EQUAL(rebx2->Document->Selection->CharacterFormat->Bold, mut::FormatEffect::On);
                VERIFY_ARE_EQUAL(rebx2->Document->Selection->CharacterFormat->Underline, mut::UnderlineType::Single);
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"\r\n3.Press Ctrl-i/u/b again on the formatting accelerator enabled RichEditBox, it should flip the selection format.");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_i#$u$_i#$u$_ctrlscan");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_u#$u$_u#$u$_ctrlscan");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_b#$u$_b#$u$_ctrlscan");
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(rebx2->Document->Selection->CharacterFormat->Italic, mut::FormatEffect::Off);
                VERIFY_ARE_EQUAL(rebx2->Document->Selection->CharacterFormat->Bold, mut::FormatEffect::Off);
                VERIFY_ARE_EQUAL(rebx2->Document->Selection->CharacterFormat->Underline, mut::UnderlineType::None);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                rebx2->DisabledFormattingAccelerators = xaml_controls::DisabledFormattingAccelerators::Bold | xaml_controls::DisabledFormattingAccelerators::Italic | xaml_controls::DisabledFormattingAccelerators::Underline;
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"\r\n4.Press Ctrl-i/u/b again after disabling the accelerators, it should not flip the selection format.");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_i#$u$_i#$u$_ctrlscan");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_u#$u$_u#$u$_ctrlscan");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_b#$u$_b#$u$_ctrlscan");
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(rebx2->Document->Selection->CharacterFormat->Italic, mut::FormatEffect::Off);
                VERIFY_ARE_EQUAL(rebx2->Document->Selection->CharacterFormat->Bold, mut::FormatEffect::Off);
                VERIFY_ARE_EQUAL(rebx2->Document->Selection->CharacterFormat->Underline, mut::UnderlineType::None);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                rebx2->DisabledFormattingAccelerators = xaml_controls::DisabledFormattingAccelerators::None;
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"\r\n5.Press Ctrl-i/u/b again after enabling the accelerators, it should flip the selection format.");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_i#$u$_i#$u$_ctrlscan");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_u#$u$_u#$u$_ctrlscan");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_b#$u$_b#$u$_ctrlscan");
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(rebx2->Document->Selection->CharacterFormat->Italic, mut::FormatEffect::On);
                VERIFY_ARE_EQUAL(rebx2->Document->Selection->CharacterFormat->Bold, mut::FormatEffect::On);
                VERIFY_ARE_EQUAL(rebx2->Document->Selection->CharacterFormat->Underline, mut::UnderlineType::Single);
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"\r\n6.Press  Ctrl-i/u/b again after disabling the accelerators for italic only, it should only flip the selection format for bold and underline.");
            RunOnUIThread([&]()
            {
                rebx2->DisabledFormattingAccelerators = xaml_controls::DisabledFormattingAccelerators::Italic;
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_i#$u$_i#$u$_ctrlscan");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_u#$u$_u#$u$_ctrlscan");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_b#$u$_b#$u$_ctrlscan");
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(rebx2->Document->Selection->CharacterFormat->Italic, mut::FormatEffect::On);
                VERIFY_ARE_EQUAL(rebx2->Document->Selection->CharacterFormat->Bold, mut::FormatEffect::Off);
                VERIFY_ARE_EQUAL(rebx2->Document->Selection->CharacterFormat->Underline, mut::UnderlineType::None);
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"\r\n7.Press  Ctrl-i/u/b again after disabling the accelerators for bold only, it should only flip the selection format for italic and underline.");
            RunOnUIThread([&]()
            {
                rebx2->DisabledFormattingAccelerators = xaml_controls::DisabledFormattingAccelerators::Bold;
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_i#$u$_i#$u$_ctrlscan");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_u#$u$_u#$u$_ctrlscan");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_b#$u$_b#$u$_ctrlscan");
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(rebx2->Document->Selection->CharacterFormat->Italic, mut::FormatEffect::Off);
                VERIFY_ARE_EQUAL(rebx2->Document->Selection->CharacterFormat->Bold, mut::FormatEffect::Off);
                VERIFY_ARE_EQUAL(rebx2->Document->Selection->CharacterFormat->Underline, mut::UnderlineType::Single);
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"\r\n8.Press  Ctrl-i/u/b again after disabling the accelerators for underline only, it should only flip the selection format for italic and bold.");
            RunOnUIThread([&]()
            {
                rebx2->DisabledFormattingAccelerators = xaml_controls::DisabledFormattingAccelerators::Underline;
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_i#$u$_i#$u$_ctrlscan");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_u#$u$_u#$u$_ctrlscan");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_b#$u$_b#$u$_ctrlscan");
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(rebx2->Document->Selection->CharacterFormat->Italic, mut::FormatEffect::On);
                VERIFY_ARE_EQUAL(rebx2->Document->Selection->CharacterFormat->Bold, mut::FormatEffect::On);
                VERIFY_ARE_EQUAL(rebx2->Document->Selection->CharacterFormat->Underline, mut::UnderlineType::Single);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                rebx2->DisabledFormattingAccelerators = xaml_controls::DisabledFormattingAccelerators::All;
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"\r\n9.Press Ctrl-i/u/b again after disabling the accelerators, it should not flip the selection format.");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_i#$u$_i#$u$_ctrlscan");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_u#$u$_u#$u$_ctrlscan");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_b#$u$_b#$u$_ctrlscan");
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(rebx2->Document->Selection->CharacterFormat->Italic, mut::FormatEffect::On);
                VERIFY_ARE_EQUAL(rebx2->Document->Selection->CharacterFormat->Bold, mut::FormatEffect::On);
                VERIFY_ARE_EQUAL(rebx2->Document->Selection->CharacterFormat->Underline, mut::UnderlineType::Single);
            });
            TestServices::WindowHelper->WaitForIdle();
        }
    } }
} } } }
