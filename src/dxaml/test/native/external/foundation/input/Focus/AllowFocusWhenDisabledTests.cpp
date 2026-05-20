// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "AllowFocusWhenDisabledTests.h"
#include <XamlTailored.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <TestEvent.h>
#include <Collection.h>
#include <FileLoader.h>
#include <UIAutomationHelper.h>
#include <AutomationClient\AutomationClientManager.h>
#include <AutomationClient\AutomationEventHandler.h>

using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Input;
using namespace ::Windows::UI::ViewManagement;
using namespace ::Windows::Foundation;

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Focus {

        bool AllowFocusWhenDisabledIntegrationTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool AllowFocusWhenDisabledIntegrationTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool AllowFocusWhenDisabledIntegrationTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void AllowFocusWhenDisabledIntegrationTests::ControlReceivesAllowFocusWhenDisabledThroughInheritance()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel;
            StackPanel^ innerPanel;
            xaml_controls::Button^ outsideBtn;
            xaml_controls::Button^ btn;
            xaml_controls::Button^ btn2;
            xaml_controls::Button^ btn3;

            auto buttonGotFocusEvent = std::make_shared<Event>();
            auto buttonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
            auto button2GotFocusEvent = std::make_shared<Event>();
            auto button2GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto outsideBtnGotFocusEvent = std::make_shared<Event>();
            auto outsideBtnGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name='button' Height='80' Width='200' Content='OutButton'/>"
                    L"  <StackPanel x:Name='concontrol'>"
                    L"      <Button x:Name='button1' Height='80' Width='200' Content='Button1'/>"
                    L"      <Button x:Name='button2' Height='80' Width='200' Content='Button2'/>"
                    L"      <Button x:Name='button3' Height='80' Width='200' Content='Button3'/>"
                    L"  </StackPanel>"
                    L"</StackPanel>"));

                TestServices::WindowHelper->WindowContent = rootPanel;
                innerPanel = safe_cast<xaml_controls::StackPanel^>(rootPanel->FindName(L"concontrol"));
                outsideBtn = safe_cast<xaml_controls::Button^>(innerPanel->FindName(L"button"));
                btn = safe_cast<xaml_controls::Button^>(innerPanel->FindName(L"button1"));
                btn2 = safe_cast<xaml_controls::Button^>(innerPanel->FindName(L"button2"));
                btn3 = safe_cast<xaml_controls::Button^>(innerPanel->FindName(L"button3"));

                innerPanel->AllowFocusWhenDisabled = true;

                buttonGotFocusRegistration.Attach(
                    btn,
                    [buttonGotFocusEvent]()
                {
                    LOG_OUTPUT(L"Button GotFocus.");
                    buttonGotFocusEvent->Set();
                });

                button2GotFocusRegistration.Attach(
                    btn2,
                    [button2GotFocusEvent]()
                {
                    LOG_OUTPUT(L"Button2 GotFocus.");
                    button2GotFocusEvent->Set();
                });

                outsideBtnGotFocusRegistration.Attach(
                    outsideBtn,
                    [outsideBtnGotFocusEvent]()
                {
                    LOG_OUTPUT(L"Outside button has gotten Focus.");
                    outsideBtnGotFocusEvent->Set();
                });
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                outsideBtn->Focus(FocusState::Pointer);
            });
            outsideBtnGotFocusEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Disable Button 1 and 3.");
                btn->IsEnabled = false;
                btn3->IsEnabled = false;

                LOG_OUTPUT(L"Set AllowFocusWhenDisabled to false on Button3.");
                btn3->AllowFocusWhenDisabled = false;
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Tab Focus to Button1.");
            TestServices::KeyboardHelper->Tab();

            TestServices::WindowHelper->WaitForIdle();
            buttonGotFocusEvent->WaitForDefault();

            LOG_OUTPUT(L"Tab Focus to Button2.");
            TestServices::KeyboardHelper->Tab();

            TestServices::WindowHelper->WaitForIdle();
            button2GotFocusEvent->WaitForDefault();

            outsideBtnGotFocusEvent->Reset();

            LOG_OUTPUT(L"Tab again, focus should ignore Button3.");
            TestServices::KeyboardHelper->Tab();

            TestServices::WindowHelper->WaitForIdle();
            outsideBtnGotFocusEvent->WaitForDefault();
        }

        void AllowFocusWhenDisabledIntegrationTests::AllowFocusWhenDisabledPropertyDefault()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel;
            StackPanel^ innerPanel;
            xaml_controls::Button^ btn;
            xaml_controls::Button^ btn2;
            xaml_controls::Button^ btn3;

            auto buttonGotFocusEvent = std::make_shared<Event>();
            auto buttonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto buttonBGotFocusEvent = std::make_shared<Event>();
            auto buttonBGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto buttonCGotFocusEvent = std::make_shared<Event>();
            auto buttonCGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name='button1' Height='20' Width='200' Content='Button' />"
                    L"  <StackPanel x:Name='concontrol'>"
                    L"      <StackPanel>"
                    L"          <Button x:Name='button2' Height='80' Width='200' Content='Button2'/>"
                    L"          <Button x:Name='button3' Height='80' Width='200' Content='Button3'/>"
                    L"      </StackPanel>"
                    L"  </StackPanel>"
                    L"</StackPanel>"));

                TestServices::WindowHelper->WindowContent = rootPanel;
                innerPanel = safe_cast<xaml_controls::StackPanel^>(rootPanel->FindName(L"concontrol"));
                btn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
                btn2 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button2"));
                btn3 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button3"));

                btn3->AllowFocusWhenDisabled = true;

                buttonGotFocusRegistration.Attach(
                    btn,
                    [buttonGotFocusEvent]()
                {
                    LOG_OUTPUT(L"Button GotFocus.");
                    buttonGotFocusEvent->Set();
                });

                buttonBGotFocusRegistration.Attach(
                    btn2,
                    [buttonBGotFocusEvent]()
                {
                    LOG_OUTPUT(L"Button 2 has gotten Focus.");
                    buttonBGotFocusEvent->Set();
                });

                buttonCGotFocusRegistration.Attach(
                    btn3,
                    [buttonCGotFocusEvent]()
                {
                    LOG_OUTPUT(L"Button 3 has gotten Focus.");
                    buttonCGotFocusEvent->Set();
                });
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                btn->Focus(FocusState::Pointer);
            });

            buttonGotFocusEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_IS_TRUE(buttonGotFocusEvent->HasFired());

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Disable Button 2 and 3.");
                btn2->IsEnabled = false;
                btn3->IsEnabled = false;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                //Attempting to set focus on button
                btn2->Focus(FocusState::Pointer);
            });

            TestServices::WindowHelper->WaitForIdle();
            VERIFY_IS_FALSE(buttonBGotFocusEvent->HasFired());

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(btn));
            });

            RunOnUIThread([&]()
            {
                //Attempting to set focus on button
                LOG_OUTPUT(L"Setting Focus on Button 3.");
                btn3->Focus(FocusState::Pointer);
            });

            buttonCGotFocusEvent->WaitForDefault();
            VERIFY_IS_TRUE(buttonCGotFocusEvent->HasFired());

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(btn3));
            });
        }

        void AllowFocusWhenDisabledIntegrationTests::MenuFlyoutCorrectlyPropagatesProperty()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel = nullptr;
            Button^ outsideBtn = nullptr;
            AppBarButton^ appBarButton = nullptr;
            Flyout^ flyout = nullptr;
            Button^ btn1 = nullptr;
            Button^ btn2 = nullptr;
            Button^ btn3 = nullptr;

            auto flyOutOpenedEvent = std::make_shared<Event>();
            auto flyOutOpenedRegistration = CreateSafeEventRegistration(Flyout, Opened);

            auto button1GotFocusEvent = std::make_shared<Event>();
            auto button1GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto button3GotFocusEvent = std::make_shared<Event>();
            auto button3GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto outsideButtonLostFocusEvent = std::make_shared<Event>();
            auto outsideButtonOnLostFocusRegistration = CreateSafeEventRegistration(Button, LostFocus);

            RunOnUIThread([&]
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                    L"  <Button x:Name='btn' Content='Outside Button'/>"
                    L"  <CommandBar>"
                    L"      <AppBarButton  x:Name='appBarButton'>"
                    L"          <AppBarButton.Flyout>"
                    L"              <Flyout x:Name='fly' AllowFocusWhenDisabled='True'>"
                    L"                  <StackPanel>"
                    L"                      <Button x:Name='button1' Height='80' Width='200' Content='Button1'/>"
                    L"                      <Button x:Name='button2' AllowFocusWhenDisabled='False' Height='80' Width='200' Content='Button2'/>"
                    L"                      <Button x:Name='button3' Height='80' Width='200' Content='Button3'/>"
                    L"                  </StackPanel>"
                    L"              </Flyout>"
                    L"           </AppBarButton.Flyout>"
                    L"       </AppBarButton>"
                    L"  </CommandBar>"
                    L"</StackPanel>"));

                VERIFY_IS_NOT_NULL(rootPanel);

                outsideBtn = safe_cast<Button^>(rootPanel->FindName(L"btn"));
                VERIFY_IS_NOT_NULL(outsideBtn);

                appBarButton = safe_cast<AppBarButton^>(rootPanel->FindName(L"appBarButton"));
                VERIFY_IS_NOT_NULL(appBarButton);

                flyout = safe_cast<Flyout^>(rootPanel->FindName("fly"));
                VERIFY_IS_NOT_NULL(flyout);

                btn1 = safe_cast<Button^>(rootPanel->FindName("button1"));
                VERIFY_IS_NOT_NULL(btn1);

                btn2 = safe_cast<Button^>(rootPanel->FindName("button2"));
                VERIFY_IS_NOT_NULL(btn1);

                btn3 = safe_cast<Button^>(rootPanel->FindName("button3"));
                VERIFY_IS_NOT_NULL(btn1);

                flyOutOpenedRegistration.Attach(flyout, [flyOutOpenedEvent]()
                {
                    LOG_OUTPUT(L"Flyout opened");
                    flyOutOpenedEvent->Set();
                });

                button1GotFocusRegistration.Attach(btn1, [button1GotFocusEvent]()
                {
                    LOG_OUTPUT(L"Button1 has focus.");
                    button1GotFocusEvent->Set();
                });

                button3GotFocusRegistration.Attach(btn3, [button3GotFocusEvent]()
                {
                    LOG_OUTPUT(L"Button3 has focus.");
                    button3GotFocusEvent->Set();
                });

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->Tap(appBarButton);
            flyOutOpenedEvent->WaitForDefault();
            VERIFY_IS_TRUE(flyOutOpenedEvent->HasFired());

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                btn1->IsEnabled = false;
                btn2->IsEnabled = false;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                //Attempting to set focus on button
                btn1->Focus(FocusState::Keyboard);
            });
            button1GotFocusEvent->WaitForDefault();

            LOG_OUTPUT(L"Tab Focus to Button3.");
            TestServices::KeyboardHelper->Tab();

            button3GotFocusEvent->WaitForDefault();

            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                flyout->Hide();
                LOG_OUTPUT(L"Hiding Flyout.");
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void AllowFocusWhenDisabledIntegrationTests::SupportAllowFocusWhenDisabledOnToggleSwitch()
        {
            TestAllowFocusWhenDisabledVisualStates<xaml_controls::ToggleSwitch>();
        }

        void AllowFocusWhenDisabledIntegrationTests::SupportAllowFocusWhenDisabledOnButton()
        {
            TestAllowFocusWhenDisabledVisualStates<xaml_controls::Button>();
        }

        void AllowFocusWhenDisabledIntegrationTests::SupportAllowFocusWhenDisabledOnToggleButton()
        {
            TestAllowFocusWhenDisabledVisualStates<xaml_primitives::ToggleButton>();
        }

        void AllowFocusWhenDisabledIntegrationTests::SupportAllowFocusWhenDisabledOnSlider()
        {
            TestAllowFocusWhenDisabledVisualStates<xaml_controls::Slider>();
        }

        void AllowFocusWhenDisabledIntegrationTests::SupportAllowFocusWhenDisabledOnRepeatButton()
        {
            TestAllowFocusWhenDisabledVisualStates<xaml_primitives::RepeatButton>();
        }

        void AllowFocusWhenDisabledIntegrationTests::SupportAllowFocusWhenDisabledOnHyperlinkButton()
        {
            TestAllowFocusWhenDisabledVisualStates<xaml_controls::HyperlinkButton>();
        }

        void AllowFocusWhenDisabledIntegrationTests::SupportAllowFocusWhenDisabledOnAutoSuggestBox()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::AutoSuggestBox^ autoSuggestBox = nullptr;
            xaml_controls::Button^ button = nullptr;
            auto autoSuggestBoxGotFocusEvent = std::make_shared<Event>();
            auto autoSuggestBoxxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, GotFocus);

            RunOnUIThread([&]()
            {
                auto rootPanel = ref new xaml_controls::StackPanel();
                TestServices::WindowHelper->WindowContent = rootPanel;

                button = ref new xaml_controls::Button();
                button->Content = L"button";
                rootPanel->Children->Append(button);

                auto itemList = ref new Platform::Collections::Vector<Platform::String^>();
                itemList->Append("Red");
                itemList->Append("Blue");
                itemList->Append("Yellow");
                autoSuggestBox = ref new xaml_controls::AutoSuggestBox();
                autoSuggestBox->ItemsSource = itemList;
                autoSuggestBox->IsEnabled = false;
                autoSuggestBox->AllowFocusWhenDisabled = true;
                rootPanel->Children->Append(autoSuggestBox);
                autoSuggestBoxxGotFocusRegistration.Attach(
                    autoSuggestBox,
                    [autoSuggestBoxGotFocusEvent]()
                {
                    LOG_OUTPUT(L"AutoSuggestBox GotFocus.");
                    autoSuggestBoxGotFocusEvent->Set();
                });

            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Setting focus to button.");
                button->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Move focus to the AutoSuggestBox");
            TestServices::KeyboardHelper->Tab();

            autoSuggestBoxGotFocusEvent->WaitForDefault();
        }

        void AllowFocusWhenDisabledIntegrationTests::SupportAllowFocusWhenDisabledOnTextBoxWithGamePad()
        {
            SupportAllowFocusWhenDisabledOnTextBoxHelper(true /* useGamePad */);
        }

        void AllowFocusWhenDisabledIntegrationTests::SupportAllowFocusWhenDisabledOnTextBox()
        {
            SupportAllowFocusWhenDisabledOnTextBoxHelper(false /* useGamePad */);
        }

        void AllowFocusWhenDisabledIntegrationTests::SupportAllowFocusWhenDisabledOnButtonWithCustomizedTemplate()
        {
            TestCleanupWrapper cleanup;

            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"AllowFocusWhenDisabled.xaml"));
            VERIFY_IS_NOT_NULL(root);
            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = root;
            });

            xaml_controls::Button^ button = nullptr;
            RunOnUIThread([&]()
            {
                button = safe_cast<xaml_controls::Button^>(root->FindName("button"));
                VERIFY_IS_NOT_NULL(button);
            });

            auto buttonGotFocusEvent = std::make_shared<Event>();
            auto buttonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            RunOnUIThread([&]()
            {
                buttonGotFocusRegistration.Attach(
                    button,
                    [buttonGotFocusEvent]()
                {
                    LOG_OUTPUT(L"Button GotFocus.");
                    buttonGotFocusEvent->Set();
                });

            });
            TestServices::WindowHelper->WaitForIdle();

            // Set button's AllowFocusWhenDisabled to true and disable the button
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Disable button and set AllowFocusWhenDisabled to true.");
                button->AllowFocusWhenDisabled = true;
                button->IsEnabled = false;
            });
            TestServices::WindowHelper->WaitForIdle();
            buttonGotFocusEvent->Reset();
            // Focus on the button
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Setting focus to button.");
                button->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();
            buttonGotFocusEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                bool disabledStateFound = false;
                bool unFocusedStateFound = false;
                auto templateRoot = safe_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(button, 0));
                auto groups = xaml::VisualStateManager::GetVisualStateGroups(templateRoot);
                VERIFY_IS_TRUE(groups->Size > 0);

                // enumerate current state of all VSG and check if contains "Disable"
                for (unsigned int i = 0; i < groups->Size; ++i)
                {
                    auto currentVisualStateGroup = groups->GetAt(i);

                    LOG_OUTPUT(L"VSG %s current state:%s", currentVisualStateGroup->Name->Data(), currentVisualStateGroup->CurrentState->Name->Data());
                    if (currentVisualStateGroup->CurrentState->Name == L"Disabled")
                    {
                        disabledStateFound = true;
                    }
                    if (currentVisualStateGroup->CurrentState->Name == L"Unfocused")
                    {
                        unFocusedStateFound = true;
                    }
                }

                VERIFY_IS_TRUE(disabledStateFound);
                VERIFY_IS_TRUE(unFocusedStateFound);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Enable button control.");
                button->IsEnabled = true;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                bool focusedStateFound = false;
                bool normalStateFound = false;
                auto templateRoot = safe_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(button, 0));
                auto groups = xaml::VisualStateManager::GetVisualStateGroups(templateRoot);
                VERIFY_IS_TRUE(groups->Size > 0);

                // enumerate current state of all VSG and check if contains "Disable"
                for (unsigned int i = 0; i < groups->Size; ++i)
                {
                    auto currentVisualStateGroup = groups->GetAt(i);

                    LOG_OUTPUT(L"VSG %s current state:%s", currentVisualStateGroup->Name->Data(), currentVisualStateGroup->CurrentState->Name->Data());
                    if (currentVisualStateGroup->CurrentState->Name == L"Focused")
                    {
                        focusedStateFound = true;
                    }
                    if (currentVisualStateGroup->CurrentState->Name == L"Normal")
                    {
                        normalStateFound = true;
                    }
                }

                VERIFY_IS_TRUE(focusedStateFound);
                VERIFY_IS_TRUE(normalStateFound);
            });
            TestServices::WindowHelper->WaitForIdle();

        }

        void AllowFocusWhenDisabledIntegrationTests::SupportAllowFocusWhenDisabledOnTextBoxHelper(bool useGamePad)
        {
            TestCleanupWrapper cleanup;

            xaml_controls::Button^ button = nullptr;
            xaml_controls::TextBox^ disabledTextBox = nullptr;
            xaml_controls::TextBox^ disabledAndFocusAllowedTextBox = nullptr;

            InputPane^ inputPane;
            wf::EventRegistrationToken inputPaneShowToken;
            auto SIPShowingEvent = std::make_shared<Event>();
            auto textBoxTextChangedEvent = std::make_shared<Event>();
            auto textBoxTextChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, TextChanged);
            auto textBoxGotFocusEvent = std::make_shared<Event>();
            auto textBoxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);

            RunOnUIThread([&]()
            {
                auto rootPanel = ref new xaml_controls::StackPanel();
                TestServices::WindowHelper->WindowContent = rootPanel;
                button = ref new xaml_controls::Button();
                button->Content = L"button";
                disabledTextBox = ref new xaml_controls::TextBox();
                disabledTextBox->Text = L"disabledTextBox";
                disabledAndFocusAllowedTextBox = ref new xaml_controls::TextBox();
                disabledAndFocusAllowedTextBox->Text = L"disabledAndFocusAllowedTextBox";
                rootPanel->Children->Append(button);
                rootPanel->Children->Append(disabledTextBox);
                rootPanel->Children->Append(disabledAndFocusAllowedTextBox);
                disabledTextBox->IsEnabled = false;
                disabledAndFocusAllowedTextBox->IsEnabled = false;
                disabledAndFocusAllowedTextBox->AllowFocusWhenDisabled = true;

                inputPane = test_infra::TestServices::WindowHelper->GetInputPaneForMainView();

                inputPaneShowToken = inputPane->Showing += ref new TypedEventHandler<InputPane^, InputPaneVisibilityEventArgs^>([&](InputPane^ pane, InputPaneVisibilityEventArgs^ e)
                {
                    LOG_OUTPUT(L"SIP Showing...");
                    SIPShowingEvent->Set();
                });

                textBoxGotFocusRegistration.Attach(
                    disabledAndFocusAllowedTextBox,
                    [textBoxGotFocusEvent]()
                {
                    LOG_OUTPUT(L"TextBox GotFocus.");
                    textBoxGotFocusEvent->Set();
                });

                textBoxTextChangedRegistration.Attach(
                    disabledAndFocusAllowedTextBox,
                    ref new xaml_controls::TextChangedEventHandler(
                        [textBoxTextChangedEvent](Platform::Object^, xaml_controls::TextChangedEventArgs^)
                {
                    LOG_OUTPUT(L"TextBox TextChanged.");
                    textBoxTextChangedEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Setting focus to button.");
                button->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Move focus to the textbox");
            if (useGamePad)
            {
                TestServices::KeyboardHelper->GamepadDpadDown();
            }
            else
            {
                TestServices::KeyboardHelper->Tab();
            }
            textBoxGotFocusEvent->WaitForDefault();

            //SIP should not popup
            SIPShowingEvent->WaitForNoThrow(std::chrono::milliseconds(2000));
            VERIFY_IS_FALSE(SIPShowingEvent->HasFired());
            textBoxTextChangedEvent->Reset();

            // Text input should not work
            TestServices::KeyboardHelper->PressKeySequence(L"1234");
            textBoxTextChangedEvent->WaitForNoThrow(std::chrono::milliseconds(2000));
            VERIFY_IS_FALSE(textBoxTextChangedEvent->HasFired());

            RunOnUIThread([&]()
            {
                // enable the textbox and it should bring up the SIP.
                disabledAndFocusAllowedTextBox->IsEnabled = true;
            });

            TestServices::WindowHelper->WaitForIdle();

            // Text input should work now
            textBoxTextChangedEvent->Reset();
            TestServices::KeyboardHelper->PressKeySequence(L"1234");
            textBoxTextChangedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                inputPane->Showing -= inputPaneShowToken;
            });
        }

        void AllowFocusWhenDisabledIntegrationTests::FocusStayOnSameControlAfterReEnable()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::Button^ button0 = nullptr;
            xaml_controls::Button^ button1 = nullptr;
            xaml_controls::Button^ button2 = nullptr;

            auto button1GotFocusEvent = std::make_shared<Event>();
            auto button1GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
            auto button2GotFocusEvent = std::make_shared<Event>();
            auto button2GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            RunOnUIThread([&]()
            {
                auto rootPanel = ref new xaml_controls::StackPanel();
                TestServices::WindowHelper->WindowContent = rootPanel;
                button0 = ref new xaml_controls::Button();
                button0->Content = L"button0";
                button1 = ref new xaml_controls::Button();
                button1->Content = L"button1";
                button2 = ref new xaml_controls::Button();
                button2->Content = L"button2";
                rootPanel->Children->Append(button0);
                rootPanel->Children->Append(button1);
                rootPanel->Children->Append(button2);

                button1->AllowFocusWhenDisabled = true;

                button1GotFocusRegistration.Attach(
                    button1,
                    [button1GotFocusEvent]()
                {
                    LOG_OUTPUT(L"Button1 GotFocus.");
                    button1GotFocusEvent->Set();
                });

                button2GotFocusRegistration.Attach(
                    button2,
                    [button2GotFocusEvent]()
                {
                    LOG_OUTPUT(L"Button2 GotFocus.");
                    button2GotFocusEvent->Set();
                });
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Setting focus to button1.");
                button1->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();
            button1GotFocusEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Disable button1, focus should stay.");
                button1->IsEnabled = false;
            });
            TestServices::WindowHelper->WaitForIdle();
            button2GotFocusEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
            VERIFY_IS_FALSE(button2GotFocusEvent->HasFired());
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(button1));
            });

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Enable button1 again, focus should stay.");
                button1->IsEnabled = true;
            });
            TestServices::WindowHelper->WaitForIdle();
            button2GotFocusEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
            VERIFY_IS_FALSE(button2GotFocusEvent->HasFired());
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(button1));
            });

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Change button1's AllowFocusWhenDisabled to false, disable and focus should move.");
                button1->AllowFocusWhenDisabled = false;
                button1->IsEnabled = false;
            });
            button2GotFocusEvent->WaitForDefault();
        }

        void AllowFocusWhenDisabledIntegrationTests::AllowUiaFocusWhenElementDisabled()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel;
            StackPanel^ innerPanel;
            xaml_controls::Button^ outsideBtn;
            xaml_controls::Button^ btn;
            xaml_controls::Button^ btn2;
            xaml_controls::Button^ btn3;

            auto button1GotFocusEvent = std::make_shared<Event>();
            auto button1GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
            auto button2GotFocusEvent = std::make_shared<Event>();
            auto button2GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <StackPanel x:Name='concontrol'>"
                    L"      <Button x:Name='button1' Height='80' Width='200' Content='Button1' AutomationProperties.Name='button1' />"
                    L"      <Button x:Name='button2' Height='80' Width='200' Content='Button2' AutomationProperties.Name='button2' AllowFocusWhenDisabled='True' IsEnabled='False'/>"
                    L"      <Button x:Name='button3' Height='80' Width='200' Content='Button3' AutomationProperties.Name='button3' />"
                    L"  </StackPanel>"
                    L"</StackPanel>"));

                TestServices::WindowHelper->WindowContent = rootPanel;
                innerPanel = safe_cast<xaml_controls::StackPanel^>(rootPanel->FindName(L"concontrol"));
                btn = safe_cast<xaml_controls::Button^>(innerPanel->FindName(L"button1"));
                btn2 = safe_cast<xaml_controls::Button^>(innerPanel->FindName(L"button2"));
                btn3 = safe_cast<xaml_controls::Button^>(innerPanel->FindName(L"button3"));

                innerPanel->AllowFocusWhenDisabled = true;

                button1GotFocusRegistration.Attach(
                    btn,
                    [button1GotFocusEvent]()
                {
                    LOG_OUTPUT(L"Button GotFocus.");
                    button1GotFocusEvent->Set();
                });

                button2GotFocusRegistration.Attach(
                    btn2,
                    [button2GotFocusEvent]()
                {
                    LOG_OUTPUT(L"Button2 GotFocus.");
                    button2GotFocusEvent->Set();
                });
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                btn->Focus(FocusState::Pointer);
            });
            button1GotFocusEvent->WaitForDefault();

            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"button2";
            uiaInfo.m_AutomationID = L"button2";
            uiaInfo.m_cType = UIA_ButtonControlTypeId;
            auto automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            wrl::ComPtr<IUIAutomationElement> automationElement;
            automationClientManager->GetCurrentUIAutomationElement(&automationElement);

            #define VERIFY_AUTOMATION_PROP(__func, __expected) \
                { \
                    BOOL result##__func = FALSE; \
                    VERIFY_SUCCEEDED(automationElement->get_##__func(& result##__func)); \
                    if (__expected) { VERIFY_IS_TRUE(result##__func); } else { VERIFY_IS_FALSE(result##__func); } \
                }

            VERIFY_AUTOMATION_PROP(CurrentIsKeyboardFocusable, TRUE);
            VERIFY_AUTOMATION_PROP(CurrentHasKeyboardFocus, FALSE);
            VERIFY_AUTOMATION_PROP(CurrentIsEnabled, FALSE);

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Setting focus to button2");
                btn2->Focus(FocusState::Keyboard);
            });
            button2GotFocusEvent->WaitForDefault();

            VERIFY_AUTOMATION_PROP(CurrentIsKeyboardFocusable, TRUE);
            VERIFY_AUTOMATION_PROP(CurrentHasKeyboardFocus, TRUE);
            VERIFY_AUTOMATION_PROP(CurrentIsEnabled, FALSE);

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Setting focus back to button1");
                btn->Focus(FocusState::Keyboard);
            });
            button1GotFocusEvent->WaitForDefault();

            VERIFY_AUTOMATION_PROP(CurrentIsKeyboardFocusable, TRUE);
            VERIFY_AUTOMATION_PROP(CurrentHasKeyboardFocus, FALSE);
            VERIFY_AUTOMATION_PROP(CurrentIsEnabled, FALSE);

            LOG_OUTPUT(L"Set focus to button2 via UIA");
            VERIFY_SUCCEEDED(automationElement->SetFocus());
            button2GotFocusEvent->WaitForDefault();

            VERIFY_AUTOMATION_PROP(CurrentIsKeyboardFocusable, TRUE);
            VERIFY_AUTOMATION_PROP(CurrentHasKeyboardFocus, TRUE);
            VERIFY_AUTOMATION_PROP(CurrentIsEnabled, FALSE);

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Setting AllowFocusWhenDisabled to false");
                btn2->AllowFocusWhenDisabled = false;
            });
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_AUTOMATION_PROP(CurrentIsKeyboardFocusable, FALSE);
            VERIFY_AUTOMATION_PROP(CurrentHasKeyboardFocus, FALSE);
            VERIFY_AUTOMATION_PROP(CurrentIsEnabled, FALSE);

        }

        Platform::String^ AllowFocusWhenDisabledIntegrationTests::GetResourcesPath() const
        {
            return GetPackageFolder() + L"resources\\native\\foundation\\Input\\Focus\\";
        }

    }
}}}}
