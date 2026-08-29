// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "AllowFocusOnInteractionTests.h"
#include <XamlTailored.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <FocusTestHelper.h>
#include <TestEvent.h>

using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Input;
using namespace ::Windows::UI::ViewManagement;
using namespace ::Windows::Foundation;

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Focus {

        bool AllowFocusOnInteractionIntegrationTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool AllowFocusOnInteractionIntegrationTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool AllowFocusOnInteractionIntegrationTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void AllowFocusOnInteractionIntegrationTests::FocusAndSIPDoesNotChangeWhenAllowFocusDisabled()
        {
            wf::EventRegistrationToken inputPaneShowingToken = {};
            wf::EventRegistrationToken inputPaneHidingToken = {};

            // Since InputPane is not agile, we can't use SafeEventRegistration. We need to manage the SIP events manually.
            TestCleanupWrapper cleanup([&inputPaneShowingToken, &inputPaneHidingToken]()
            {
                RunOnUIThread([&inputPaneShowingToken, &inputPaneHidingToken]()
                {
                    InputPane::GetForCurrentView()->Showing -= inputPaneShowingToken;
                    InputPane::GetForCurrentView()->Hiding -= inputPaneHidingToken;
                    inputPaneShowingToken = {};
                    inputPaneHidingToken = {};
                });

                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            StackPanel^ rootPanel;
            xaml_controls::TextBox^ textBox;
            xaml_controls::Button^ btn;
            InputPane^ inputPane;

            auto textboxGotFocusEvent = std::make_shared<Event>();
            auto textboxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);
            auto textboxLostFocusEvent = std::make_shared<Event>();
            auto textboxLostFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, LostFocus);
            auto SIPShowingEvent = std::make_shared<Event>();
            auto SIPHidingEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"    <Button x:Name='button1' Height='20' Width='200' Content='Button' />"
                    L"    <TextBox x:Name='textBox' Height='80' Width='200'/>"
                    L"</StackPanel>"));

                TestServices::WindowHelper->WindowContent = rootPanel;
                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                btn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));

                btn->AllowFocusOnInteraction = false;

                textboxGotFocusRegistration.Attach(
                    textBox,
                    [textboxGotFocusEvent]()
                {
                    LOG_OUTPUT(L"Textbox GotFocus.");
                    textboxGotFocusEvent->Set();
                });
            });

            RunOnUIThread([&]()
            {
                inputPane = InputPane::GetForCurrentView();
                inputPaneShowingToken = inputPane->Showing += ref new TypedEventHandler<InputPane^, InputPaneVisibilityEventArgs^>([&](InputPane^ pane, InputPaneVisibilityEventArgs^ e)
                {
                    LOG_OUTPUT(L"SIP Showing...");
                    SIPShowingEvent->Set();
                });
                inputPaneHidingToken = inputPane->Hiding += ref new TypedEventHandler<InputPane^, InputPaneVisibilityEventArgs^>([&](InputPane^ pane, InputPaneVisibilityEventArgs^ e)
                {
                    LOG_OUTPUT(L"SIP Hiding...");
                    SIPHidingEvent->Set();
                });
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Focus TextBox to bringup SIP");
            RunOnUIThread([&]()
            {
                textBox->Focus(xaml::FocusState::Pointer);
            });

            textboxGotFocusEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            SIPShowingEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                //Attempting to set focus on button
                btn->Focus(xaml::FocusState::Pointer);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(textBox));
                VERIFY_IS_FALSE(SIPHidingEvent->HasFired());
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void AllowFocusOnInteractionIntegrationTests::ClickFiresButFocusDoesNotChange()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel;
            xaml_controls::Button^ btn;
            xaml_controls::Button^ btn2;

            auto buttonClickEvent = std::make_shared<Event>();
            auto buttonOnClickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"    <Button Height='20' Width='200' Content='DummyButton' />"
                    L"    <Button x:Name='button1' Height='20' Width='200' Content='Button' />"
                    L"    <Button x:Name='button2' Height='80' Width='200' Content='Button2'/>"
                    L"</StackPanel>"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                btn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
                btn2 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button2"));

                btn2->AllowFocusOnInteraction = false;

                buttonOnClickRegistration.Attach(
                    btn2,
                    [buttonClickEvent]()
                {
                    LOG_OUTPUT(L"Button has been clicked.");
                    buttonClickEvent->Set();
                });
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Setting Focus on Button 1.");
            FocusTestHelper::EnsureFocus(btn, FocusState::Pointer);

            //Attempting to set focus on button
            TestServices::InputHelper->LeftMouseClick(btn2);
            TestServices::WindowHelper->WaitForIdle();

            buttonClickEvent->WaitForDefault();
            VERIFY_IS_TRUE(buttonClickEvent->HasFired());

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(btn));
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void AllowFocusOnInteractionIntegrationTests::ControlReceivesAllowFocusOnInteractionThroughInheritance()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel;
            StackPanel^ innerPanel;
            xaml_controls::Button^ outsideBtn;
            xaml_controls::Button^ btn;
            xaml_controls::Button^ btn2;

            auto buttonGotFocusEvent = std::make_shared<Event>();
            auto buttonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name='button' Height='80' Width='200' Content='Button1'/>"
                    L"  <StackPanel x:Name='concontrol'>"
                    L"      <Button x:Name='button1' Height='80' Width='200' Content='Button1'/>"
                    L"  </StackPanel>"
                    L"</StackPanel>"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                innerPanel = safe_cast<xaml_controls::StackPanel^>(rootPanel->FindName(L"concontrol"));
                outsideBtn = safe_cast<xaml_controls::Button^>(innerPanel->FindName(L"button"));
                btn = safe_cast<xaml_controls::Button^>(innerPanel->FindName(L"button1"));

                innerPanel->AllowFocusOnInteraction = false;

                buttonGotFocusRegistration.Attach(
                    btn,
                    [buttonGotFocusEvent]()
                {
                    LOG_OUTPUT(L"Button GotFocus.");
                    buttonGotFocusEvent->Set();
                });
            });

            FocusTestHelper::EnsureFocus(outsideBtn, FocusState::Pointer);

            RunOnUIThread([&]()
            {
                //Attempting to set focus on button
                LOG_OUTPUT(L"Setting Focus on Button 1.");
                btn->Focus(FocusState::Pointer);
            });

            TestServices::WindowHelper->WaitForIdle();
            VERIFY_IS_FALSE(buttonGotFocusEvent->HasFired());

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(outsideBtn));
            });
        }

        void AllowFocusOnInteractionIntegrationTests::ControlOverridesAllowFocusEvenWhenInherited()
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
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                innerPanel = safe_cast<xaml_controls::StackPanel^>(rootPanel->FindName(L"concontrol"));
                btn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
                btn2 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button2"));
                btn3 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button3"));

                innerPanel->AllowFocusOnInteraction = false;
                btn3->AllowFocusOnInteraction = true;

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

                LOG_OUTPUT(L"Setting Focus on Button 1.");
            });

            FocusTestHelper::EnsureFocus(btn, FocusState::Pointer);

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

        void AllowFocusOnInteractionIntegrationTests::CanStillSetFocusThroughKeyboardWhenAllowFocusFalse()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel;
            xaml_controls::Button^ btn;
            xaml_controls::Button^ btn2;

            auto buttonGotFocusEvent = std::make_shared<Event>();
            auto buttonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto buttonBGotFocusEvent = std::make_shared<Event>();
            auto buttonBGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name='button1' Height='80' Width='200' Content='Button1'/>"
                    L"  <Button x:Name='button2' Height='80' Width='200' Content='Button2'/>"
                    L"</StackPanel>"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                btn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
                btn2 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button2"));

                btn2->AllowFocusOnInteraction = false;

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
                    LOG_OUTPUT(L"ButtonB has gotten Focus.");
                    buttonBGotFocusEvent->Set();
                });
            });

            FocusTestHelper::EnsureFocus(btn, FocusState::Pointer);

            RunOnUIThread([&]()
            {
                btn2->Focus(FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();
            VERIFY_IS_TRUE(buttonBGotFocusEvent->HasFired());

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(btn2));
            });
        }

        void AllowFocusOnInteractionIntegrationTests::AppBarButtonDenyFocusByDefault()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel;
            xaml_controls::Button^ btn;
            xaml_controls::AppBarButton^ appBarButton;
            xaml_controls::AppBarToggleButton^ toggle;

            auto buttonGotFocusEvent = std::make_shared<Event>();
            auto buttonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::AppBarButton, GotFocus);

            auto toggleGotFocusEvent = std::make_shared<Event>();
            auto toggleGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::AppBarToggleButton, GotFocus);

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <Button x:Name='button' Content='Button'/>"
                    L"  <CommandBar>"
                    L"      <AppBarButton Label='Test' x:Name='appBarButton'/>"
                    L"      <AppBarToggleButton Label='Toggle' x:Name='appBarToggle'/>"
                    L"  </CommandBar>"
                    L"</StackPanel>"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                btn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
                btn->Focus(FocusState::Pointer);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                appBarButton = safe_cast<xaml_controls::AppBarButton^>(rootPanel->FindName(L"appBarButton"));
                toggle = safe_cast<xaml_controls::AppBarToggleButton^>(rootPanel->FindName(L"appBarToggle"));

                buttonGotFocusRegistration.Attach(
                    appBarButton,
                    [buttonGotFocusEvent]()
                {
                    LOG_OUTPUT(L"AppBarButton GotFocus.");
                    buttonGotFocusEvent->Set();
                });

                toggleGotFocusRegistration.Attach(
                    toggle,
                    [toggleGotFocusEvent]()
                {
                    LOG_OUTPUT(L"ToggleButton has gotten Focus.");
                    toggleGotFocusEvent->Set();
                });

                appBarButton->Focus(FocusState::Pointer);

            });

            TestServices::WindowHelper->WaitForIdle();

            VERIFY_IS_FALSE(buttonGotFocusEvent->HasFired());

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(btn));
                toggle->Focus(FocusState::Pointer);
            });

            TestServices::WindowHelper->WaitForIdle();
            VERIFY_IS_FALSE(toggleGotFocusEvent->HasFired());

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(btn));
            });
        }

        void AllowFocusOnInteractionIntegrationTests::AllowFocusCorrectlyLeavesTree()
        {
            TestCleanupWrapper cleanup;

            Button^ btn1 = nullptr;
            Button^ btn2 = nullptr;

            const int buttonIndex = 0;

            auto gotFocusButtonEvent = std::make_shared<Event>();
            auto gotFocusButtonRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto gotFocusButton2Event = std::make_shared<Event>();
            auto gotFocusButton2Registration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            StackPanel^ rootPanel = nullptr;

            RunOnUIThread([&]
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                    L"  <Button x:Name='btn1' Content='Button 1'/>"
                    L"  <Button x:Name='btn2' Content='Button 2'/>"
                    L"</StackPanel>"));

                VERIFY_IS_NOT_NULL(rootPanel);

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]
            {
                btn1 = safe_cast<Button^>(rootPanel->FindName(L"btn1"));
                VERIFY_IS_NOT_NULL(btn1);

                btn2 = safe_cast<Button^>(rootPanel->FindName(L"btn2"));
                VERIFY_IS_NOT_NULL(btn2);

                btn2->AllowFocusOnInteraction = false;

                gotFocusButton2Registration.Attach(btn2, [gotFocusButton2Event]()
                {
                    LOG_OUTPUT(L"Button2 got focus");
                    gotFocusButton2Event->Set();
                });

                btn1->Focus(FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]
            {
                //We want to remove btn1 (the currently focused element) from the tree. This will force a selection
                //of a new focusable element, which should fire FocusedElementRemoved
                VERIFY_IS_TRUE(rootPanel->Children->GetAt(buttonIndex)->Equals(btn1));
                rootPanel->Children->RemoveAt(buttonIndex);
            });

            gotFocusButton2Event->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusButton2Event->HasFired());
        }

        void AllowFocusOnInteractionIntegrationTests::MenuFlyoutCorrectlyPropagatesProperty()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel = nullptr;
            Button^ outsideBtn = nullptr;
            AppBarButton^ button = nullptr;
            Flyout^ flyout = nullptr;
            MenuFlyoutItem^ flyoutItem = nullptr;

            auto flyOutOpenedEvent = std::make_shared<Event>();
            auto flyOutOpenedRegistration = CreateSafeEventRegistration(Flyout, Opened);

            auto buttonClickEvent = std::make_shared<Event>();
            auto buttonOnClickRegistration = CreateSafeEventRegistration(MenuFlyoutItem, Click);

            auto outsideButtonLostFocusEvent = std::make_shared<Event>();
            auto outsideButtonOnLostFocusRegistration = CreateSafeEventRegistration(Button, LostFocus);

            RunOnUIThread([&]
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                    L"  <Button x:Name='btn' Content='Outside Button'/>"
                    L"  <CommandBar IsOpen='True'>"
                    L"      <AppBarButton Content='AppBarButton' x:Name='appBarButton'>"
                    L"          <AppBarButton.Flyout>"
                    L"              <Flyout x:Name='fly'>"
                    L"                   <MenuFlyoutItem x:Name='flyoutItem'>Test</MenuFlyoutItem>"
                    L"              </Flyout>"
                    L"           </AppBarButton.Flyout>"
                    L"       </AppBarButton>"
                    L"  </CommandBar>"
                    L"</StackPanel>"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]
            {
                outsideBtn = safe_cast<Button^>(rootPanel->FindName(L"btn"));
                VERIFY_IS_NOT_NULL(outsideBtn);

                button = safe_cast<AppBarButton^>(rootPanel->FindName(L"appBarButton"));
                VERIFY_IS_NOT_NULL(button);

                flyout = safe_cast<Flyout^>(rootPanel->FindName("fly"));
                VERIFY_IS_NOT_NULL(flyout);

                flyoutItem = safe_cast<MenuFlyoutItem^>(rootPanel->FindName("flyoutItem"));
                VERIFY_IS_NOT_NULL(flyoutItem);

                flyOutOpenedRegistration.Attach(flyout, [flyOutOpenedEvent]()
                {
                    LOG_OUTPUT(L"Flyout opened");
                    flyOutOpenedEvent->Set();
                });

                buttonOnClickRegistration.Attach(flyoutItem, [buttonClickEvent]()
                {
                    LOG_OUTPUT(L"Button has been clicked.");
                    buttonClickEvent->Set();
                });

                outsideButtonOnLostFocusRegistration.Attach(outsideBtn, [outsideButtonLostFocusEvent]()
                {
                    LOG_OUTPUT(L"Button has lost focus.");
                    outsideButtonLostFocusEvent->Set();
                });
            });

            FocusTestHelper::EnsureFocus(outsideBtn, FocusState::Pointer);

            //Attempting to set focus on button
            TestServices::InputHelper->Tap(button);

            TestServices::WindowHelper->WaitForIdle();

            flyOutOpenedEvent->WaitForDefault();
            VERIFY_IS_TRUE(flyOutOpenedEvent->HasFired());

            TestServices::WindowHelper->WaitForIdle();

            //Attempting to set focus on flyout item
            TestServices::InputHelper->Tap(flyoutItem);

            buttonClickEvent->WaitForDefault();
            VERIFY_IS_TRUE(buttonClickEvent->HasFired());

            VERIFY_IS_FALSE(outsideButtonLostFocusEvent->HasFired());

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(outsideBtn));
                flyout->Hide();
            });
        }

        void AllowFocusOnInteractionIntegrationTests::AppBarButtonBehavesNormallyOnKeyboard()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel;
            xaml_controls::AppBarButton^ appBarButton;
            xaml_controls::MenuFlyoutItem^ item;
            Flyout^ flyout = nullptr;

            auto buttonGotFocusEvent = std::make_shared<Event>();
            auto buttonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::AppBarButton, GotFocus);

            auto itemGotFocusEvent = std::make_shared<Event>();
            auto itemGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutItem, GotFocus);

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                        <AppBar IsOpen="True">
                            <AppBarButton x:Name="button">
                                <AppBarButton.Flyout>
                                    <Flyout x:Name='flyout'>
                                        <MenuFlyoutItem x:Name="flyoutItem">ff</MenuFlyoutItem>
                                    </Flyout>
                                </AppBarButton.Flyout>
                            </AppBarButton>
                        </AppBar>
                    </StackPanel>)"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                appBarButton = safe_cast<xaml_controls::AppBarButton^>(rootPanel->FindName(L"button"));
                item = safe_cast<xaml_controls::MenuFlyoutItem^>(rootPanel->FindName(L"flyoutItem"));
                flyout = safe_cast<Flyout^>(rootPanel->FindName("flyout"));
                VERIFY_IS_NOT_NULL(flyout);

                buttonGotFocusRegistration.Attach(
                    appBarButton,
                    [buttonGotFocusEvent]()
                {
                    LOG_OUTPUT(L"AppBarButton GotFocus.");
                    buttonGotFocusEvent->Set();
                });

                itemGotFocusRegistration.Attach(
                    item,
                    [itemGotFocusEvent]()
                {
                    LOG_OUTPUT(L"MenuSubItem GotFocus.");
                    itemGotFocusEvent->Set();
                });
            });

            FocusTestHelper::EnsureFocus(appBarButton, FocusState::Keyboard);

            TestServices::KeyboardHelper->Enter();
            TestServices::WindowHelper->WaitForIdle();

            itemGotFocusEvent->WaitForDefault();
            VERIFY_IS_TRUE(itemGotFocusEvent->HasFired());

            RunOnUIThread([&]()
            {
                flyout->Hide();
            });
        }

        void AllowFocusOnInteractionIntegrationTests::PopupLightDismissBehaviorAllowFocusOnInteractionOff()
        {
            PopupLightDismissBehaviorHelper(false);
        }

        void AllowFocusOnInteractionIntegrationTests::PopupLightDismissBehaviorAllowFocusOnInteractionOn()
        {
            PopupLightDismissBehaviorHelper(true);
        }

        void AllowFocusOnInteractionIntegrationTests::PopupLightDismissBehaviorHelper(bool allowFocusOnInteraction)
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_primitives::Popup^ popup = nullptr;
            xaml_controls::Button^ button1 = nullptr;

            auto popupOpenedEvent = std::make_shared<Event>();
            auto popupClosedEvent = std::make_shared<Event>();
            auto popupOpenedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Opened);
            auto popupClosedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Closed);

            RunOnUIThread([&]()
            {
                auto rootPanel = dynamic_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                    L"      Width='400' Height='400' VerticalAlignment='Top' HorizontalAlignment='Left'> "
                    L"    <StackPanel>"
                    L""
                    L"        <Popup x:Name='popup' IsLightDismissEnabled='True' >"
                    L"            <Border Margin='5' Background='Orange'>"
                    L"                <TextBlock Text='In Popup' />"
                    L"            </Border>"
                    L"        </Popup>"
                    L""
                    L"        <Button  x:Name='button1' Margin='0, 200, 0, 10' Content='button1' AllowFocusOnInteraction='False' />"
                    L"    </StackPanel>"
                    L"</Grid>"));

                popup = safe_cast<xaml_primitives::Popup^>(rootPanel->FindName(L"popup"));
                button1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
                button1->AllowFocusOnInteraction = allowFocusOnInteraction;

                popupOpenedRegistration.Attach(popup, [&]()
                {
                    popupOpenedEvent->Set();
                });

                popupClosedRegistration.Attach(popup, [&]()
                {
                    popupClosedEvent->Set();
                });

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            // Open the Popup
            RunOnUIThread([&]()
            {
                popup->IsOpen = true;
            });
            popupOpenedEvent->WaitForDefault();

            TestServices::InputHelper->Tap(button1);
            popupClosedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            xaml::UIElement^ focusedElement;
            RunOnUIThread([&]()
            {
                focusedElement = safe_cast<xaml::UIElement^>(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
                if (allowFocusOnInteraction)
                {
                    VERIFY_IS_TRUE(focusedElement->Equals(button1));
                    VERIFY_ARE_EQUAL(button1->FocusState, xaml::FocusState::Pointer);
                }
                else
                {
                    if (focusedElement)
                    {
                        VERIFY_IS_FALSE(focusedElement->Equals(button1));
                    }
                }
            });

            TestServices::WindowHelper->WaitForIdle();
        }
    }
}}}}
