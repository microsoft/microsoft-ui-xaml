// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "BasicFocusTests.h"
#include <XamlTailored.h>
#include <SafeEventRegistration.h>
#include <ppltasks.h>
#include "FileLoader.h"
#include <TestCleanupWrapper.h>
#include <TestEvent.h>
#include <FocusTestHelper.h>
#include <WUCRenderingScopeGuard.h>

using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Input;
using namespace ::Windows::UI::ViewManagement;
using namespace ::Windows::Foundation;

using namespace test_infra;

namespace Microsoft::UI::Xaml::Tests {
    namespace Foundation::Input::Focus {

        bool BasicFocusTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool BasicFocusTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool BasicFocusTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        Platform::String^ BasicFocusTests::GetPathToFiles() const
        {
            // Get the deployment directory, and then append our test's directory to the end
            auto deploymentDir = GetTestDeploymentDir();
            return ref new Platform::String(deploymentDir + L"resources\\native\\foundation\\input\\Focus\\");
        }

        void BasicFocusTests::CheckFocusStatusForAllControls(
            _In_ std::vector<Control^>* pControlsVect,
            _In_opt_ Control^ expectedToBeFocused,
            FocusState expectedFocusState) const
        {
            for (Control^ control : *pControlsVect)
            {
                if (control == expectedToBeFocused && control->FocusState != expectedFocusState)
                {
                    LOG_OUTPUT(L"CheckFocusStatusForAllControls - error: control=%s, expected FocusState=%d, actual FocusState=%d.", control->Name->Data(), expectedFocusState, control->FocusState);
                    VERIFY_ARE_EQUAL(control->FocusState, expectedFocusState);
                }
                else if (control != expectedToBeFocused && control->FocusState != FocusState::Unfocused)
                {
                    LOG_OUTPUT(L"CheckFocusStatusForAllControls - error: control=%s, expected FocusState=Unfocused (0), actual FocusState=%d.", control->Name->Data(), control->FocusState);
                    VERIFY_ARE_EQUAL(control->FocusState, FocusState::Unfocused);
                }
            }
        }

        //------------------------------------------------------------------------
        // Test case: Click a Button and a TextBox with the left mouse button,
        //            and record the resulting PointerEntered, PointerCaptureLost,
        //            Click, GotFocus & LostFocus events.
        //------------------------------------------------------------------------
        void BasicFocusTests::LeftMouseClickAButtonAndTextBox()
        {
            TestCleanupWrapper cleanup;

            std::shared_ptr<Event> buttonGotFocusEvent = std::make_shared<Event>();
            std::shared_ptr<Event> buttonClickEvent = std::make_shared<Event>();

            Button^ btn = nullptr;
            wf::EventRegistrationToken buttonPointerEnteredToken = {};
            wf::EventRegistrationToken buttonPointerCaptureLostToken = {};
            wf::EventRegistrationToken buttonClickToken = {};
            wf::EventRegistrationToken buttonGotFocusToken = {};
            wf::EventRegistrationToken buttonLostFocusToken = {};

            TextBox^ txt = nullptr;
            wf::EventRegistrationToken textBoxPointerEnteredToken = {};
            wf::EventRegistrationToken textBoxPointerCaptureLostToken = {};
            wf::EventRegistrationToken textBoxGotFocusToken = {};
            wf::EventRegistrationToken textBoxLostFocusToken = {};

            int buttonPointerEnteredCount = 0;
            int buttonPointerCaptureLostCount = 0;
            int buttonClickCount = 0;
            int buttonGotFocusCount = 0;
            int buttonLostFocusCount = 0;
            int textBoxPointerEnteredCount = 0;
            int textBoxPointerCaptureLostCount = 0;
            int textBoxGotFocusCount = 0;
            int textBoxLostFocusCount = 0;

            Button^ dummyBtn = nullptr;

            RunOnUIThread([&]()
            {
                StackPanel^ mainStackPanel = ref new StackPanel();

                dummyBtn = ref new Button();
                dummyBtn->Width = 150;
                dummyBtn->Height = 50;
                dummyBtn->Content = "Button";
                dummyBtn->HorizontalAlignment = HorizontalAlignment::Center;

                btn = ref new Button();
                btn->Width = 150;
                btn->Height = 50;
                btn->Content = "Button";
                btn->HorizontalAlignment = HorizontalAlignment::Center;
                buttonPointerEnteredToken = btn->PointerEntered +=
                    ref new PointerEventHandler([&buttonPointerEnteredCount](Platform::Object^, PointerRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Button PointerEntered event");
                    VERIFY_ARE_EQUAL(args->Pointer->PointerDeviceType, Microsoft::UI::Input::PointerDeviceType::Mouse);
                    buttonPointerEnteredCount++;
                });
                buttonPointerCaptureLostToken = btn->PointerCaptureLost +=
                    ref new PointerEventHandler([&buttonPointerCaptureLostCount](Platform::Object^, PointerRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Button PointerCaptureLost event");
                    VERIFY_ARE_EQUAL(args->Pointer->PointerDeviceType, Microsoft::UI::Input::PointerDeviceType::Mouse);
                    buttonPointerCaptureLostCount++;
                });
                buttonClickToken = btn->Click +=
                    ref new RoutedEventHandler([buttonClickEvent, &buttonClickCount](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Button Click event");
                    buttonClickCount++;
                    buttonClickEvent->Set();
                });
                buttonGotFocusToken = btn->GotFocus +=
                    ref new RoutedEventHandler([buttonGotFocusEvent, &buttonGotFocusCount](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Button GotFocus event");
                    buttonGotFocusCount++;
                    buttonGotFocusEvent->Set();
                });
                buttonLostFocusToken = btn->LostFocus +=
                    ref new RoutedEventHandler([&buttonLostFocusCount](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Button LostFocus event");
                    buttonLostFocusCount++;
                });

                txt = ref new TextBox();
                txt->Width = 150;
                txt->Height = 50;
                txt->Text = "TextBox";
                textBoxPointerEnteredToken = txt->PointerEntered +=
                    ref new PointerEventHandler([&textBoxPointerEnteredCount](Platform::Object^, PointerRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"TextBox PointerEntered event");
                    textBoxPointerEnteredCount++;
                });
                textBoxPointerCaptureLostToken = txt->PointerCaptureLost +=
                    ref new PointerEventHandler([&textBoxPointerCaptureLostCount](Platform::Object^, PointerRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"TextBox PointerCaptureLost event");
                    textBoxPointerCaptureLostCount++;
                });
                textBoxGotFocusToken = txt->GotFocus +=
                    ref new RoutedEventHandler([&textBoxGotFocusCount](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"TextBox GotFocus event");
                    textBoxGotFocusCount++;
                });
                textBoxLostFocusToken = txt->LostFocus +=
                    ref new RoutedEventHandler([&textBoxLostFocusCount](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"TextBox LostFocus event");
                    textBoxLostFocusCount++;
                });

                mainStackPanel->Children->Append(dummyBtn);
                mainStackPanel->Children->Append(btn);
                mainStackPanel->Children->Append(txt);

                TestServices::WindowHelper->WindowContent = mainStackPanel;
            });
            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(dummyBtn, FocusState::Pointer);
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Clicking Button.");
            TestServices::InputHelper->LeftMouseClick(btn);
            buttonClickEvent->WaitForDefault();
            buttonClickEvent->Reset();
            LOG_OUTPUT(L"Clicking TextBox.");
            TestServices::InputHelper->LeftMouseClick(txt);
            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Clicking Button again.");
            TestServices::InputHelper->LeftMouseClick(btn);
            TestServices::WindowHelper->WaitForIdle();

            buttonGotFocusEvent->WaitForDefault();
            buttonClickEvent->WaitForDefault();

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                btn->PointerEntered -= buttonPointerEnteredToken;
                btn->PointerCaptureLost -= buttonPointerCaptureLostToken;
                btn->Click -= buttonClickToken;
                btn->GotFocus -= buttonGotFocusToken;
                btn->LostFocus -= buttonLostFocusToken;
                txt->GotFocus -= textBoxGotFocusToken;
                txt->LostFocus -= textBoxLostFocusToken;
                txt->PointerEntered -= textBoxPointerEnteredToken;
                txt->PointerCaptureLost -= textBoxPointerCaptureLostToken;

                VERIFY_IS_TRUE(buttonPointerEnteredCount == 1 || buttonPointerEnteredCount == 2);
                VERIFY_ARE_EQUAL(buttonPointerCaptureLostCount, 2);
                VERIFY_ARE_EQUAL(buttonClickCount, 2);
                VERIFY_ARE_EQUAL(buttonGotFocusCount, 2);
                VERIFY_ARE_EQUAL(buttonLostFocusCount, 1);
                VERIFY_ARE_EQUAL(textBoxPointerEnteredCount, 1);
                VERIFY_ARE_EQUAL(textBoxPointerCaptureLostCount, 1);
                VERIFY_ARE_EQUAL(textBoxGotFocusCount, 1);
                VERIFY_ARE_EQUAL(textBoxLostFocusCount, 1);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        //------------------------------------------------------------------------
        // Test case: Use keyboard to navigate from control to control.
        //------------------------------------------------------------------------
        void BasicFocusTests::CanChangeFocusWithKeyboard()
        {
            TestCleanupWrapper cleanup;

            auto gotFocusEvent = std::make_shared<Event>();
            auto loadedEvent = std::make_shared<Event>();

            wf::EventRegistrationToken gotFocusToken[13] = {};
            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

            Button^ btn1 = nullptr;
            Button^ btn2 = nullptr;
            Button^ btn3 = nullptr;
            ListBox^ lb1 = nullptr;
            ListBoxItem^ lbi1 = nullptr;
            ListBoxItem^ lbi2 = nullptr;
            xaml_primitives::RepeatButton^ rbtn1 = nullptr;
            ComboBox^ cb1 = nullptr;
            ComboBoxItem^ cbi1 = nullptr;
            ComboBoxItem^ cbi2 = nullptr;
            ComboBoxItem^ cbi3 = nullptr;
            RadioButton^ radio1 = nullptr;
            Button^ btnEnd = nullptr;

            std::vector<Control^> controlsVect;

            StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"FocusTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            RunOnUIThread([&]()
            {
                loadedRegistration.Attach(rootStackPanel, [&]()
                {
                    loadedEvent->Set();
                });

                TestServices::WindowHelper->WindowContent = rootStackPanel;

                btn1 = dynamic_cast<Button^>(rootStackPanel->FindName(L"btn1"));
                VERIFY_IS_NOT_NULL(btn1);

                btn2 = dynamic_cast<Button^>(rootStackPanel->FindName(L"btn2"));
                VERIFY_IS_NOT_NULL(btn2);

                btn3 = dynamic_cast<Button^>(rootStackPanel->FindName(L"btn3"));
                VERIFY_IS_NOT_NULL(btn3);

                lb1 = dynamic_cast<ListBox^>(rootStackPanel->FindName(L"lb1"));
                VERIFY_IS_NOT_NULL(lb1);

                rbtn1 = dynamic_cast<xaml_primitives::RepeatButton^>(rootStackPanel->FindName(L"rbtn1"));
                VERIFY_IS_NOT_NULL(rbtn1);

                lbi1 = dynamic_cast<ListBoxItem^>(rootStackPanel->FindName(L"lbi1"));
                VERIFY_IS_NOT_NULL(lbi1);

                lbi2 = dynamic_cast<ListBoxItem^>(rootStackPanel->FindName(L"lbi2"));
                VERIFY_IS_NOT_NULL(lbi2);

                cb1 = dynamic_cast<ComboBox^>(rootStackPanel->FindName(L"cb1"));
                VERIFY_IS_NOT_NULL(cb1);

                cbi1 = dynamic_cast<ComboBoxItem^>(rootStackPanel->FindName(L"cbi1"));
                VERIFY_IS_NOT_NULL(cbi1);

                cbi2 = dynamic_cast<ComboBoxItem^>(rootStackPanel->FindName(L"cbi2"));
                VERIFY_IS_NOT_NULL(cbi2);

                cbi3 = dynamic_cast<ComboBoxItem^>(rootStackPanel->FindName(L"cbi3"));
                VERIFY_IS_NOT_NULL(cbi3);

                radio1 = dynamic_cast<RadioButton^>(rootStackPanel->FindName(L"radio1"));
                VERIFY_IS_NOT_NULL(radio1);

                btnEnd = dynamic_cast<Button^>(rootStackPanel->FindName(L"btnEnd"));
                VERIFY_IS_NOT_NULL(btnEnd);

                controlsVect.push_back(btn1);
                controlsVect.push_back(btn2);
                controlsVect.push_back(btn3);
                controlsVect.push_back(lb1);
                controlsVect.push_back(lbi1);
                controlsVect.push_back(lbi2);
                controlsVect.push_back(rbtn1);
                controlsVect.push_back(cb1);
                controlsVect.push_back(cbi1);
                controlsVect.push_back(cbi2);
                controlsVect.push_back(cbi3);
                controlsVect.push_back(radio1);
                controlsVect.push_back(btnEnd);

                UINT ctrlIndex = 0u;
                for (Control^ control : controlsVect)
                {
                    gotFocusToken[ctrlIndex] = control->GotFocus +=
                        ref new xaml::RoutedEventHandler(
                        [gotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^) {
                        gotFocusEvent->Set();
                    });
                    ctrlIndex++;
                }
            });
            loadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            auto scopeGuard = wil::scope_exit([&]
            {
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Unsubscribing events.");
                    UINT ctrlIndex = 0u;
                    for (Control^ control : controlsVect)
                    {
                        control->GotFocus -= gotFocusToken[ctrlIndex];
                        ctrlIndex++;
                    }
                });
                TestServices::WindowHelper->WaitForIdle();
            });

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Setting focus on the Button.");
                VERIFY_IS_TRUE(btn1->Focus(FocusState::Keyboard));
            });

            gotFocusEvent->WaitForDefault();
            gotFocusEvent->Reset();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    btn1 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Tab forward to second Button.");
            TestServices::KeyboardHelper->Tab();
            gotFocusEvent->WaitForDefault();
            gotFocusEvent->Reset();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    btn2 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Tab backward to first Button.");
            TestServices::KeyboardHelper->ShiftTab();
            gotFocusEvent->WaitForDefault();
            gotFocusEvent->Reset();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    btn1 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Tab backward to wrap around to last Button.");
            TestServices::KeyboardHelper->ShiftTab();
            gotFocusEvent->WaitForDefault();
            gotFocusEvent->Reset();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    btnEnd /*expectedToBeFocused*/,
                    FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Tab forward to first Button.");
            TestServices::KeyboardHelper->Tab();
            gotFocusEvent->WaitForDefault();
            gotFocusEvent->Reset();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    btn1 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Tab forward to second Button.");
            TestServices::KeyboardHelper->Tab();
            gotFocusEvent->WaitForDefault();
            gotFocusEvent->Reset();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    btn2 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Tab forward to TextBox.");
            TestServices::KeyboardHelper->Tab();
            gotFocusEvent->WaitForDefault();
            gotFocusEvent->Reset();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    btn3 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Tab forward to ListBox.");
            TestServices::KeyboardHelper->Tab();
            gotFocusEvent->WaitForDefault();
            gotFocusEvent->Reset();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    lbi1 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Arrow down to second ListBox item.");
            TestServices::KeyboardHelper->Down();
            gotFocusEvent->WaitForDefault();
            gotFocusEvent->Reset();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    lbi2 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Arrow up to first ListBox item.");
            TestServices::KeyboardHelper->Up();
            gotFocusEvent->WaitForDefault();
            gotFocusEvent->Reset();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    lbi1 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Tab out of ListBox, forward to RepeatButton.");
            TestServices::KeyboardHelper->Tab();
            gotFocusEvent->WaitForDefault();
            gotFocusEvent->Reset();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    rbtn1 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Tab forward to ComboBox.");
            TestServices::KeyboardHelper->Tab();
            gotFocusEvent->WaitForDefault();
            gotFocusEvent->Reset();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    cb1 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Open ComboBox.");
            TestServices::KeyboardHelper->Enter();
            TestServices::WindowHelper->WaitForIdle();
            gotFocusEvent->WaitForDefault();
            gotFocusEvent->Reset();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    cbi1 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
            });
            
            LOG_OUTPUT(L"Arrow down to second ComboBox item.");
            TestServices::KeyboardHelper->Down();
            gotFocusEvent->WaitForDefault();
            gotFocusEvent->Reset();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    cbi2 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Arrow down to third ComboBox item.");
            TestServices::KeyboardHelper->Down();
            gotFocusEvent->WaitForDefault();
            gotFocusEvent->Reset();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    cbi3 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Arrow up to second ComboBox item.");
            TestServices::KeyboardHelper->Up();
            gotFocusEvent->WaitForDefault();
            gotFocusEvent->Reset();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    cbi2 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Arrow up to first ComboBox item.");
            TestServices::KeyboardHelper->Up();
            gotFocusEvent->WaitForDefault();
            gotFocusEvent->Reset();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    cbi1 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Close ComboBox.");
            TestServices::KeyboardHelper->Escape();
            gotFocusEvent->WaitForDefault();
            gotFocusEvent->Reset();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    cb1 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Tab forward to RadioButton.");
            TestServices::KeyboardHelper->Tab();
            gotFocusEvent->WaitForDefault();
            gotFocusEvent->Reset();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    radio1 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Tab forward to last Button.");
            TestServices::KeyboardHelper->Tab();
            gotFocusEvent->WaitForDefault();
            gotFocusEvent->Reset();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    btnEnd /*expectedToBeFocused*/,
                    FocusState::Keyboard);
            });
        }

        // Validates inability to focus control with the FocusManager::TryMoveFocus method when IsTabStop is always false.
        void BasicFocusTests::CannotChangeFocusWithoutTabStop()
        {
            ChangeFocusBasedOnTabStop(false /*hasATabStop*/);
        }

        // Validates ability to focus control with the FocusManager::TryMoveFocus method when IsTabStop is true for one control.
        void BasicFocusTests::CanChangeFocusWithOneTabStop()
        {
            ChangeFocusBasedOnTabStop(true /*hasATabStop*/);
        }

        void BasicFocusTests::ChangeFocusBasedOnTabStop(_In_ bool hasATabStop)
        {
            TestCleanupWrapper cleanup;

            auto loadedEvent = std::make_shared<Event>();
            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

            Button^ btn1 = nullptr;
            Button^ btn2 = nullptr;
            Button^ btn3 = nullptr;
            ListBox^ lb1 = nullptr;
            ListBoxItem^ lbi1 = nullptr;
            ListBoxItem^ lbi2 = nullptr;
            ListBoxItem^ lbi3 = nullptr;
            RepeatButton^ rbtn1 = nullptr;
            ComboBox^ cb1 = nullptr;
            ComboBoxItem^ cbi1 = nullptr;
            ComboBoxItem^ cbi2 = nullptr;
            ComboBoxItem^ cbi3 = nullptr;
            RadioButton^ radio1 = nullptr;
            Button^ btnEnd = nullptr;

            std::vector<Control^> controlsVect;

            StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"FocusTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            RunOnUIThread([&]()
            {
                loadedRegistration.Attach(rootStackPanel, [&]()
                {
                    loadedEvent->Set();
                });

                rootStackPanel->TabFocusNavigation = KeyboardNavigationMode::Cycle;
                TestServices::WindowHelper->WindowContent = rootStackPanel;
                btn1 = safe_cast<Button^>(rootStackPanel->FindName(L"btn1"));
            });
            loadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Setting focus on btn1");
            FocusTestHelper::EnsureFocus(btn1, FocusState::Pointer);

            RunOnUIThread([&]()
            {
                btn2 = safe_cast<Button^>(rootStackPanel->FindName(L"btn2"));
                btn3 = safe_cast<Button^>(rootStackPanel->FindName(L"btn3"));
                lb1 = safe_cast<ListBox^>(rootStackPanel->FindName(L"lb1"));
                lbi1 = safe_cast<ListBoxItem^>(rootStackPanel->FindName(L"lbi1"));
                lbi2 = safe_cast<ListBoxItem^>(rootStackPanel->FindName(L"lbi2"));
                lbi3 = safe_cast<ListBoxItem^>(rootStackPanel->FindName(L"lbi3"));
                rbtn1 = safe_cast<RepeatButton^>(rootStackPanel->FindName(L"rbtn1"));
                cb1 = safe_cast<ComboBox^>(rootStackPanel->FindName(L"cb1"));
                cbi1 = safe_cast<ComboBoxItem^>(rootStackPanel->FindName(L"cbi1"));
                cbi2 = safe_cast<ComboBoxItem^>(rootStackPanel->FindName(L"cbi2"));
                cbi3 = safe_cast<ComboBoxItem^>(rootStackPanel->FindName(L"cbi3"));
                radio1 = safe_cast<RadioButton^>(rootStackPanel->FindName(L"radio1"));
                btnEnd = safe_cast<Button^>(rootStackPanel->FindName(L"btnEnd"));

                controlsVect.push_back(btn1);
                controlsVect.push_back(btn2);
                controlsVect.push_back(btn3);
                controlsVect.push_back(lb1);
                controlsVect.push_back(lbi1);
                controlsVect.push_back(lbi2);
                controlsVect.push_back(lbi3);
                controlsVect.push_back(rbtn1);
                controlsVect.push_back(cb1);
                controlsVect.push_back(cbi1);
                controlsVect.push_back(cbi2);
                controlsVect.push_back(cbi3);
                controlsVect.push_back(radio1);
                controlsVect.push_back(btnEnd);

                UINT ctrlIndex = 0u;
                for (Control^ control : controlsVect)
                {
                    control->IsTabStop = false;
                    ctrlIndex++;
                }

                btn3->IsTabStop = hasATabStop;
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Tapping the first Button.");
            TestServices::InputHelper->Tap(btn1);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Even though btn1.IsTabStop was set to False, it kept input focus.
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    btn1 /*expectedToBeFocused*/,
                    FocusState::Pointer /*expectedFocusState*/);

                LOG_OUTPUT(L"Programmatically attempt to focus the next control.");
                xaml_input::FindNextElementOptions^ options = ref new xaml_input::FindNextElementOptions;
                options->SearchRoot = TestServices::WindowHelper->WindowContent;
                bool resultTryMoveFocus = xaml_input::FocusManager::TryMoveFocus(xaml_input::FocusNavigationDirection::Next, options);
                VERIFY_ARE_EQUAL(resultTryMoveFocus, hasATabStop);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    hasATabStop ? btn3 : btn1 /*expectedToBeFocused*/,
                    FocusState::Pointer /*expectedFocusState*/);

                LOG_OUTPUT(L"Programmatically attempt to focus the previous control.");
                xaml_input::FindNextElementOptions^ options = ref new xaml_input::FindNextElementOptions;
                options->SearchRoot = TestServices::WindowHelper->WindowContent;
                bool resultTryMoveFocus = xaml_input::FocusManager::TryMoveFocus(xaml_input::FocusNavigationDirection::Previous, options);
                VERIFY_ARE_EQUAL(resultTryMoveFocus, hasATabStop);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    hasATabStop ? btn3 : btn1 /*expectedToBeFocused*/,
                    FocusState::Pointer /*expectedFocusState*/);
            });
        }

        // Validates ability to navigate from control to control with the FocusManager::TryMoveFocus method.
        void BasicFocusTests::ChangeFocusWithTryMoveFocus()
        {
            TestCleanupWrapper cleanup;

            auto gotFocusEvent = std::make_shared<Event>();
            auto loadedEvent = std::make_shared<Event>();

            wf::EventRegistrationToken gotFocusToken[3] = {};
            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

            Button^ btn1 = nullptr;
            Button^ btn2 = nullptr;
            Button^ btnEnd = nullptr;

            std::vector<Control^> controlsVect;

            StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"FocusTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            RunOnUIThread([&]()
            {
                loadedRegistration.Attach(rootStackPanel, [&]()
                {
                    loadedEvent->Set();
                });

                rootStackPanel->TabFocusNavigation = KeyboardNavigationMode::Cycle;
                TestServices::WindowHelper->WindowContent = rootStackPanel;

                btn1 = safe_cast<Button^>(rootStackPanel->FindName(L"btn1"));
                btn2 = safe_cast<Button^>(rootStackPanel->FindName(L"btn2"));
                btnEnd = safe_cast<Button^>(rootStackPanel->FindName(L"btnEnd"));

                controlsVect.push_back(btn1);
                controlsVect.push_back(btn2);
                controlsVect.push_back(btnEnd);

                UINT ctrlIndex = 0u;
                for (Control^ control : controlsVect)
                {
                    gotFocusToken[ctrlIndex] = control->GotFocus +=
                        ref new xaml::RoutedEventHandler(
                        [gotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^) {
                        gotFocusEvent->Set();
                    });
                    ctrlIndex++;
                }
            });
            loadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Tapping the first Button.");
            TestServices::InputHelper->Tap(btn1);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Programmatically focus the next Button.");
                xaml_input::FindNextElementOptions^ options = ref new xaml_input::FindNextElementOptions;
                options->SearchRoot = TestServices::WindowHelper->WindowContent;
                bool resultTryMoveFocus = xaml_input::FocusManager::TryMoveFocus(xaml_input::FocusNavigationDirection::Next, options);
                VERIFY_IS_TRUE(resultTryMoveFocus);
            });

            TestServices::WindowHelper->WaitForIdle();
            gotFocusEvent->WaitForDefault();
            gotFocusEvent->Reset();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    btn2 /*expectedToBeFocused*/,
                    FocusState::Pointer);

                LOG_OUTPUT(L"Programmatically focus the previous Button.");
                xaml_input::FindNextElementOptions^ options = ref new xaml_input::FindNextElementOptions;
                options->SearchRoot = TestServices::WindowHelper->WindowContent;
                bool resultTryMoveFocus = xaml_input::FocusManager::TryMoveFocus(xaml_input::FocusNavigationDirection::Previous, options);
                VERIFY_IS_TRUE(resultTryMoveFocus);
            });

            TestServices::WindowHelper->WaitForIdle();
            gotFocusEvent->WaitForDefault();
            gotFocusEvent->Reset();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    btn1 /*expectedToBeFocused*/,
                    FocusState::Pointer);

                LOG_OUTPUT(L"Programmatically focus the last focusable control.");
                xaml_input::FindNextElementOptions^ options = ref new xaml_input::FindNextElementOptions;
                options->SearchRoot = TestServices::WindowHelper->WindowContent;
                bool resultTryMoveFocus = xaml_input::FocusManager::TryMoveFocus(xaml_input::FocusNavigationDirection::Previous, options);
                VERIFY_IS_TRUE(resultTryMoveFocus);
            });

            TestServices::WindowHelper->WaitForIdle();
            gotFocusEvent->WaitForDefault();
            gotFocusEvent->Reset();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    btnEnd /*expectedToBeFocused*/,
                    FocusState::Pointer);

                LOG_OUTPUT(L"Programmatically focus the first focusable control.");
                xaml_input::FindNextElementOptions^ options = ref new xaml_input::FindNextElementOptions;
                options->SearchRoot = TestServices::WindowHelper->WindowContent;
                bool resultTryMoveFocus = xaml_input::FocusManager::TryMoveFocus(xaml_input::FocusNavigationDirection::Next, options);
                VERIFY_IS_TRUE(resultTryMoveFocus);
            });

            TestServices::WindowHelper->WaitForIdle();
            gotFocusEvent->WaitForDefault();
            gotFocusEvent->Reset();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    btn1 /*expectedToBeFocused*/,
                    FocusState::Pointer);

                UINT ctrlIndex = 0u;
                for (Control^ control : controlsVect)
                {
                    control->GotFocus -= gotFocusToken[ctrlIndex];
                    ctrlIndex++;
                }
            });
        }

        //------------------------------------------------------------------------
        // Test case: Change focus programmatically to navigate from control to
        //            control.
        //------------------------------------------------------------------------
        void BasicFocusTests::CanChangeFocusProgrammatically()
        {
            TestCleanupWrapper cleanup;

            auto gotFocusEvent = std::make_shared<Event>();
            auto loadedEvent = std::make_shared<Event>();

            wf::EventRegistrationToken gotFocusToken[10] = {};
            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

            Button^ btn1 = nullptr;
            Button^ btn2 = nullptr;
            Button^ btn3 = nullptr;
            ListBox^ lb1 = nullptr;
            ListBoxItem^ lbi1 = nullptr;
            ListBoxItem^ lbi2 = nullptr;
            ComboBox^ cb1 = nullptr;
            ComboBoxItem^ cbi1 = nullptr;
            ComboBoxItem^ cbi2 = nullptr;
            Button^ btnEnd = nullptr;

            std::vector<Control^> controlsVect;

            StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"FocusTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            RunOnUIThread([&]()
            {
                loadedRegistration.Attach(rootStackPanel, [&]()
                {
                    loadedEvent->Set();
                });

                TestServices::WindowHelper->WindowContent = rootStackPanel;

                btn1 = dynamic_cast<Button^>(rootStackPanel->FindName(L"btn1"));
                btn2 = dynamic_cast<Button^>(rootStackPanel->FindName(L"btn2"));
                btn3 = dynamic_cast<Button^>(rootStackPanel->FindName(L"btn3"));
                lb1 = dynamic_cast<ListBox^>(rootStackPanel->FindName(L"lb1"));
                lbi1 = dynamic_cast<ListBoxItem^>(rootStackPanel->FindName(L"lbi1"));
                lbi2 = dynamic_cast<ListBoxItem^>(rootStackPanel->FindName(L"lbi2"));
                cb1 = dynamic_cast<ComboBox^>(rootStackPanel->FindName(L"cb1"));
                cbi1 = dynamic_cast<ComboBoxItem^>(rootStackPanel->FindName(L"cbi1"));
                cbi2 = dynamic_cast<ComboBoxItem^>(rootStackPanel->FindName(L"cbi2"));
                btnEnd = dynamic_cast<Button^>(rootStackPanel->FindName(L"btnEnd"));

                controlsVect.push_back(btn1);
                controlsVect.push_back(btn2);
                controlsVect.push_back(btn3);
                controlsVect.push_back(lb1);
                controlsVect.push_back(lbi1);
                controlsVect.push_back(lbi2);
                controlsVect.push_back(cb1);
                controlsVect.push_back(cbi1);
                controlsVect.push_back(cbi2);
                controlsVect.push_back(btnEnd);

                UINT ctrlIndex = 0u;
                for (Control^ control : controlsVect)
                {
                    gotFocusToken[ctrlIndex] = control->GotFocus +=
                        ref new xaml::RoutedEventHandler(
                        [gotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^) {
                        gotFocusEvent->Set();
                    });
                    ctrlIndex++;
                }
            });
            loadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            auto scopeGuard = wil::scope_exit([&]
            {
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Unsubscribing events.");
                    UINT ctrlIndex = 0u;
                    for (Control^ control : controlsVect)
                    {
                        control->GotFocus -= gotFocusToken[ctrlIndex];
                        ctrlIndex++;
                    }
                });
                TestServices::WindowHelper->WaitForIdle();
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(btn1->IsTapEnabled);
            });

            LOG_OUTPUT(L"Tapping the first Button.");
            TestServices::InputHelper->Tap(btn1); // Input mode set to pointer
            FocusState currentFocusState = FocusState::Pointer;

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Programmatically focus the second Button.");
                btn2->Focus(FocusState::Programmatic);
            });

            gotFocusEvent->WaitForDefault();
            gotFocusEvent->Reset();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    btn2 /*expectedToBeFocused*/,
                    currentFocusState);

                LOG_OUTPUT(L"Programmatically focus the third Button.");
                btn3->Focus(FocusState::Programmatic);
            });

            gotFocusEvent->WaitForDefault();
            gotFocusEvent->Reset();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    btn3 /*expectedToBeFocused*/,
                    currentFocusState);

                LOG_OUTPUT(L"Programmatically focus the last Button with FocusState::Keyboard.");
                currentFocusState = FocusState::Keyboard;
                btnEnd->Focus(currentFocusState);
            });

            gotFocusEvent->WaitForDefault();
            gotFocusEvent->Reset();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    btnEnd /*expectedToBeFocused*/,
                    currentFocusState);

                LOG_OUTPUT(L"Programmatically focus the first Button with FocusState::Pointer.");
                currentFocusState = FocusState::Pointer;
                btn1->Focus(currentFocusState);
            });

            gotFocusEvent->WaitForDefault();
            gotFocusEvent->Reset();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    btn1 /*expectedToBeFocused*/,
                    currentFocusState);

                LOG_OUTPUT(L"Programmatically focus the ComboBox with FocusState::Pointer.");
                currentFocusState = FocusState::Pointer;
                cb1->Focus(currentFocusState);
            });

            gotFocusEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    cb1 /*expectedToBeFocused*/,
                    currentFocusState);

                LOG_OUTPUT(L"Programmatically open the ComboBox.");
                cb1->IsDropDownOpen = true;
            });

            TestServices::WindowHelper->WaitForIdle();
            gotFocusEvent->Reset();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    cbi1 /*expectedToBeFocused*/,
                    currentFocusState);

                LOG_OUTPUT(L"Programmatically focus the second ComboBoxItem with FocusState::Pointer.");
                currentFocusState = FocusState::Pointer;
                cbi2->Focus(currentFocusState);
            });

            gotFocusEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    cbi2 /*expectedToBeFocused*/,
                    currentFocusState);

                LOG_OUTPUT(L"Programmatically close the ComboBox.");
                cb1->IsDropDownOpen = false;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    cb1 /*expectedToBeFocused*/,
                    currentFocusState);

                LOG_OUTPUT(L"Programmatically focus the ListBox.");
                lb1->Focus(FocusState::Programmatic);
            });

            TestServices::WindowHelper->WaitForIdle();
            gotFocusEvent->Reset();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    lbi1 /*expectedToBeFocused*/,
                    currentFocusState);

                LOG_OUTPUT(L"Programmatically focus the second ListBoxItem.");
                lbi2->Focus(FocusState::Programmatic);
            });

            gotFocusEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    lbi2 /*expectedToBeFocused*/,
                    currentFocusState);
            });
        }

        //------------------------------------------------------------------------
        // Test case: Change focus programmatically to FocusState::Unfocused.
        //    Verify that an InvalidArgumentException is thrown. We also verify
        //    that doing this does not change the currently focused element.
        //------------------------------------------------------------------------
        void BasicFocusTests::ProgramaticallySettingFocusStateToUnfocusedShouldThrowInvalidArgumentException()
        {
            TestCleanupWrapper cleanup;

            Button^ btn1 = nullptr;
            Button^ btn2 = nullptr;
            std::vector<Control^> controlsVect;

            auto gotFocusEvent = std::make_shared<Event>();
            auto loadedEvent = std::make_shared<Event>();

            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

            StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"FocusTests.xaml"));

            RunOnUIThread([&]()
            {
                loadedRegistration.Attach(rootStackPanel, [&]()
                {
                    loadedEvent->Set();
                });

                TestServices::WindowHelper->WindowContent = rootStackPanel;

                btn1 = safe_cast<Button^>(rootStackPanel->FindName(L"btn1"));
                btn2 = safe_cast<Button^>(rootStackPanel->FindName(L"btn2"));
                controlsVect.push_back(btn1);
                controlsVect.push_back(btn2);

                gotFocusRegistration.Attach(btn1, ref new xaml::RoutedEventHandler([&](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    gotFocusEvent->Set();
                }));
            });
            loadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Tapping the first Button.");
            TestServices::InputHelper->Tap(btn1);
            gotFocusEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(&controlsVect, btn1, FocusState::Pointer);

                LOG_OUTPUT(L"Programmatically focus btn2 with FocusState::Unfocused - ArgumentException expected");
                bool caughtArgumentException = false;
                try
                {
                    btn2->Focus(FocusState::Unfocused);
                }
                catch (Platform::InvalidArgumentException^)
                {
                    caughtArgumentException = true;
                }
                VERIFY_IS_TRUE(caughtArgumentException, L"Expected an InvalidArgumentException to be thrown when setting programatically setting FocusState::Unfocused");

                // Make sure focus is still Pointer focus on btn1.
                CheckFocusStatusForAllControls(&controlsVect, btn1, FocusState::Pointer);
            });
        }

        void BasicFocusTests::ClearCollectionWithFocusedElement()
        {
            TestCleanupWrapper cleanup;

            auto gotFocusEvent = std::make_shared<Event>();
            auto loadedEvent = std::make_shared<Event>();

            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

            Button^ btn2 = nullptr;

            StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"FocusTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            RunOnUIThread([&]()
            {
                loadedRegistration.Attach(rootStackPanel, [&]()
                {
                    loadedEvent->Set();
                });

                TestServices::WindowHelper->WindowContent = rootStackPanel;

                btn2 = dynamic_cast<Button^>(rootStackPanel->FindName(L"btn2"));

                btn2->GotFocus += ref new xaml::RoutedEventHandler(
                    [gotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^) {
                    gotFocusEvent->Set();
                });
            });
            loadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                btn2->Focus(FocusState::Programmatic);
            });

            gotFocusEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_NOT_EQUAL(FocusState::Unfocused, btn2->FocusState);

                // Clear the collection with the element that has focus, and make sure the focus manager is able to move the focus without AV'ing.
                rootStackPanel->Children->Clear();
            });
        }

        void BasicFocusTests::ClearCollectionWithFocusedElementUsingKeyboard()
        {
            TestCleanupWrapper cleanup;

            auto clickEvent = std::make_shared<Event>();
            auto gotFocusEvent = std::make_shared<Event>();
            auto loadedEvent = std::make_shared<Event>();

            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

            Button^ btn2 = nullptr;

            StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"FocusTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            RunOnUIThread([&]()
            {
                loadedRegistration.Attach(rootStackPanel, [&]()
                {
                    loadedEvent->Set();
                });

                TestServices::WindowHelper->WindowContent = rootStackPanel;

                btn2 = dynamic_cast<Button^>(rootStackPanel->FindName(L"btn2"));

                btn2->Click += ref new xaml::RoutedEventHandler(
                    [clickEvent, rootStackPanel](Platform::Object^, xaml::IRoutedEventArgs^) {
                    // Clear the collection with the element that has focus.
                    rootStackPanel->Children->Clear();
                    clickEvent->Set();
                });
                btn2->GotFocus += ref new xaml::RoutedEventHandler(
                    [gotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^) {
                    gotFocusEvent->Set();
                });
            });
            loadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                btn2->Focus(FocusState::Programmatic);
            });

            gotFocusEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_NOT_EQUAL(FocusState::Unfocused, btn2->FocusState);
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->Enter();
            clickEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(0u, rootStackPanel->Children->Size);
            });
        }

        void BasicFocusTests::TabStopWrapsFocusForListView()
        {
            TestCleanupWrapper cleanup;

            auto loadedEvent = std::make_shared<Event>();
            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

            ListViewItem^ lvItem = nullptr;
            ListView^ lv1 = nullptr;

            std::vector<Control^> controlsVect;

            StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"FocusTestsList.xaml"));

            RunOnUIThread([&]()
            {
                loadedRegistration.Attach(rootStackPanel, [&]()
                {
                    loadedEvent->Set();
                });

                TestServices::WindowHelper->WindowContent = rootStackPanel;

                lvItem = safe_cast<ListViewItem^>(rootStackPanel->FindName(L"lvItem1"));
                VERIFY_IS_NOT_NULL(lvItem);

                lv1 = safe_cast<ListView^>(rootStackPanel->FindName(L"lv1"));
                VERIFY_IS_NOT_NULL(lv1);

                controlsVect.push_back(lvItem);
                controlsVect.push_back(lv1);
            });
            loadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(lv1, FocusState::Keyboard);

            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    lvItem /*expectedToBeFocused*/,
                    FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void BasicFocusTests::FireEventWhenFocusedElementLeavesTree()
        {
            TestCleanupWrapper cleanup;

            Button^ btn1 = nullptr;
            Button^ btn2 = nullptr;
            Button^ btn3 = nullptr;
            IFocusManagerStaticsPrivate^ focusManagerPrivate = nullptr;

            const int button2Index = 1;

            auto focusElementRemovedEvent = std::make_shared<Event>();
            auto focusElementRemovedRegistration = CreateSafeEventRegistration(IFocusManagerStaticsPrivate, FocusedElementRemoved);

            auto loadedEvent = std::make_shared<Event>();
            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

            auto unloadedEvent = std::make_shared<Event>();
            auto unloadedRegistration = CreateSafeEventRegistration(xaml_controls::Button, Unloaded);

            auto gotFocusButton2Event = std::make_shared<Event>();
            auto gotFocusButton2Registration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto gotFocusButton3Event = std::make_shared<Event>();
            auto gotFocusButton3Registration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            StackPanel^ rootPanel = nullptr;

            RunOnUIThread([&]
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                    L"  <Button x:Name='btn1' Content='Button 1'/>"
                    L"  <Button x:Name='btn2' Content='Button 2'/>"
                    L"  <Button x:Name='btn3' Content='Button 3'/>"
                    L"</StackPanel>"));

                VERIFY_IS_NOT_NULL(rootPanel);

                loadedRegistration.Attach(rootPanel, [&]()
                {
                    loadedEvent->Set();
                });

                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            loadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]
            {
                Platform::Object^ focusManager;

                btn1 = safe_cast<Button^>(rootPanel->FindName(L"btn1"));
                VERIFY_IS_NOT_NULL(btn1);

                btn2 = safe_cast<Button^>(rootPanel->FindName(L"btn2"));
                VERIFY_IS_NOT_NULL(btn2);

                btn3 = safe_cast<Button^>(rootPanel->FindName(L"btn3"));
                VERIFY_IS_NOT_NULL(btn3);

                unloadedRegistration.Attach(btn2, [&]()
                {
                    //focusElementRemovedEvent should have fired before the unloaded event has fired
                    VERIFY_IS_TRUE(focusElementRemovedEvent->HasFired());
                    VERIFY_IS_TRUE(gotFocusButton3Event->HasFired());

                    LOG_OUTPUT(L"Button has been unloaded");
                    unloadedEvent->Set();
                });

                gotFocusButton2Registration.Attach(btn2, [gotFocusButton2Event]()
                {
                    LOG_OUTPUT(L"Button2 got focus");
                    gotFocusButton2Event->Set();
                });

                gotFocusButton3Registration.Attach(btn3, [gotFocusButton3Event, focusElementRemovedEvent]()
                {
                    //focusElementRemovedEvent should have fired before the got focus event has fired
                    VERIFY_IS_TRUE(focusElementRemovedEvent->HasFired());

                    LOG_OUTPUT(L"Button3 got focus");
                    gotFocusButton3Event->Set();
                });

                VERIFY_SUCCEEDED(::Windows::Foundation::GetActivationFactory(wrl::Wrappers::HStringReference(L"Microsoft.UI.Xaml.Input.FocusManager").Get(),
                    reinterpret_cast<IInspectable**>(&focusManager)));
                VERIFY_IS_NOT_NULL(focusManager);

                IFocusManagerStaticsPrivate^ focusManagerPrivate = dynamic_cast<IFocusManagerStaticsPrivate^>(focusManager);
                VERIFY_IS_NOT_NULL(focusManagerPrivate);

                focusElementRemovedRegistration.Attach(focusManagerPrivate, ref new xaml_input::FocusedElementRemovedEventHandler(
                    [&](Platform::Object^, Platform::Object^ e)
                {
                    LOG_OUTPUT(L"[FocusedElementRemoved]: Event Fired");

                    IFocusedElementRemovedEventArgs^ args = dynamic_cast<IFocusedElementRemovedEventArgs^>(e);

                    VERIFY_ARE_EQUAL(btn2, args->OldFocusedElement);
                    VERIFY_ARE_EQUAL(btn1, args->NewFocusedElement);

                    args->NewFocusedElement = btn3;

                    focusElementRemovedEvent->Set();
                }));

                LOG_OUTPUT(L"Setting focus on Button2");
                btn2->Focus(Microsoft::UI::Xaml::FocusState::Keyboard);
            });

            gotFocusButton2Event->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusButton2Event->HasFired());

            RunOnUIThread([&]
            {
                //We want to remove btn2 (the currently focused element) from the tree. This will force a selection
                //of a new focusable element, which should fire FocusedElementRemoved
                VERIFY_IS_TRUE(rootPanel->Children->GetAt(button2Index)->Equals(btn2));
                rootPanel->Children->RemoveAt(button2Index);
            });

            //Once the element has been unloaded, we know all the work has been done
            unloadedEvent->WaitForDefault();

            VERIFY_IS_TRUE(unloadedEvent->HasFired());
            VERIFY_IS_TRUE(focusElementRemovedEvent->HasFired());
        }

        void BasicFocusTests::FireEventWhenFocusedElementChangesState()
        {
            TestCleanupWrapper cleanup;

            Button^ btn1 = nullptr;
            Button^ btn2 = nullptr;
            Button^ btn3 = nullptr;
            IFocusManagerStaticsPrivate^ focusManagerPrivate = nullptr;

            auto focusElementRemovedEvent = std::make_shared<Event>();
            auto focusElementRemovedRegistration = CreateSafeEventRegistration(IFocusManagerStaticsPrivate, FocusedElementRemoved);

            auto gotFocusButton1Event = std::make_shared<Event>();
            auto gotFocusButton1Registration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto gotFocusButton2Event = std::make_shared<Event>();
            auto gotFocusButton2Registration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto gotFocusButton3Event = std::make_shared<Event>();
            auto gotFocusButton3Registration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            StackPanel^ rootPanel = nullptr;

            RunOnUIThread([&]
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                    L"  <Button x:Name='btn1' Content='Button 1'/>"
                    L"  <Button x:Name='btn2' Content='Button 2'/>"
                    L"  <Button x:Name='btn3' Content='Button 3'/>"
                    L"</StackPanel>"));

                VERIFY_IS_NOT_NULL(rootPanel);

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]
            {
                Platform::Object^ focusManager;

                btn1 = safe_cast<Button^>(rootPanel->FindName(L"btn1"));
                VERIFY_IS_NOT_NULL(btn1);

                btn2 = safe_cast<Button^>(rootPanel->FindName(L"btn2"));
                VERIFY_IS_NOT_NULL(btn2);

                btn3 = safe_cast<Button^>(rootPanel->FindName(L"btn3"));
                VERIFY_IS_NOT_NULL(btn3);

                gotFocusButton1Registration.Attach(btn1, [gotFocusButton1Event]()
                {
                    LOG_OUTPUT(L"Button1 got focus");
                    gotFocusButton1Event->Set();
                });

                gotFocusButton2Registration.Attach(btn2, [gotFocusButton2Event]()
                {
                    LOG_OUTPUT(L"Button2 got focus");
                    gotFocusButton2Event->Set();
                });

                gotFocusButton3Registration.Attach(btn3, [gotFocusButton3Event, focusElementRemovedEvent]()
                {
                    LOG_OUTPUT(L"Button3 got focus");
                    gotFocusButton3Event->Set();
                });

                VERIFY_SUCCEEDED(::Windows::Foundation::GetActivationFactory(wrl::Wrappers::HStringReference(L"Microsoft.UI.Xaml.Input.FocusManager").Get(),
                    reinterpret_cast<IInspectable**>(&focusManager)));
                VERIFY_IS_NOT_NULL(focusManager);

                IFocusManagerStaticsPrivate^ focusManagerPrivate = dynamic_cast<IFocusManagerStaticsPrivate^>(focusManager);
                VERIFY_IS_NOT_NULL(focusManagerPrivate);

                focusElementRemovedRegistration.Attach(focusManagerPrivate, ref new xaml_input::FocusedElementRemovedEventHandler(
                    [&](Platform::Object^, Platform::Object^ e)
                {
                    LOG_OUTPUT(L"[FocusedElementRemoved]: Event Fired");

                    IFocusedElementRemovedEventArgs^ args = dynamic_cast<IFocusedElementRemovedEventArgs^>(e);

                    if (btn2->Equals(args->OldFocusedElement))
                    {
                        VERIFY_ARE_EQUAL(btn3, args->NewFocusedElement);
                        args->NewFocusedElement = btn1;
                    }
                    else if (btn1->Equals(args->OldFocusedElement))
                    {
                        VERIFY_ARE_EQUAL(btn2, args->NewFocusedElement);
                        args->NewFocusedElement = btn3;
                    }

                    focusElementRemovedEvent->Set();
                }));

                LOG_OUTPUT(L"Setting focus on Button2");
                btn2->Focus(Microsoft::UI::Xaml::FocusState::Keyboard);
            });

            gotFocusButton2Event->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusButton2Event->HasFired());

            RunOnUIThread([&]
            {
                btn2->Visibility = Visibility::Collapsed;
            });

            focusElementRemovedEvent->WaitForDefault();
            VERIFY_IS_TRUE(focusElementRemovedEvent->HasFired());

            gotFocusButton1Event->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusButton1Event->HasFired());
            VERIFY_IS_FALSE(gotFocusButton3Event->HasFired());

            //Reset events
            focusElementRemovedEvent->Reset();
            gotFocusButton1Event->Reset();
            gotFocusButton2Event->Reset();
            gotFocusButton3Event->Reset();

            RunOnUIThread([&]
            {
                btn2->Visibility = Visibility::Visible;
            });

            LOG_OUTPUT(L"Setting focus on Button1");
            FocusTestHelper::EnsureFocus(btn1, FocusState::Keyboard);

            RunOnUIThread([&]
            {
                btn1->IsEnabled = false;
            });

            focusElementRemovedEvent->WaitForDefault();
            VERIFY_IS_TRUE(focusElementRemovedEvent->HasFired());

            gotFocusButton3Event->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusButton3Event->HasFired());
            VERIFY_IS_FALSE(gotFocusButton2Event->HasFired());
        }

        void BasicFocusTests::FindNextFocusableElementReturnsCorrectElement()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel = nullptr;
            Button^ btn1 = nullptr;
            Button^ btn2 = nullptr;
            Button^ btn3 = nullptr;

            auto gotFocusEvent = std::make_shared<Event>();
            auto loadedEvent = std::make_shared<Event>();

            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

            RunOnUIThread([&]
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                    L"  <Button x:Name='btn1' TabIndex='0' Height='50' Content='Button 1'/>"
                    L"  <Button x:Name='btn2' TabIndex='2' Height='50' Content='Button 2'/>"
                    L"  <Button x:Name='btn3' TabIndex='1' Height='50' Content='Button 3'/>"
                    L"</StackPanel>"));

                VERIFY_IS_NOT_NULL(rootPanel);

                loadedRegistration.Attach(rootPanel, [&]()
                {
                    loadedEvent->Set();
                });

                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            loadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]
            {
                Platform::Object^ focusManager;

                btn1 = safe_cast<Button^>(rootPanel->FindName(L"btn1"));
                VERIFY_IS_NOT_NULL(btn1);

                btn2 = safe_cast<Button^>(rootPanel->FindName(L"btn2"));
                VERIFY_IS_NOT_NULL(btn2);

                btn3 = safe_cast<Button^>(rootPanel->FindName(L"btn3"));
                VERIFY_IS_NOT_NULL(btn3);

                gotFocusRegistration.Attach(btn1, [gotFocusEvent]()
                {
                    LOG_OUTPUT(L"Button1 got focus");
                    gotFocusEvent->Set();
                });

                btn3->Focus(FocusState::Keyboard);
                btn1->Focus(FocusState::Keyboard);
            });

            gotFocusEvent->WaitForDefault();

            RunOnUIThread([&]
            {
                Button^ button = safe_cast<Button^>(FocusManager::FindNextFocusableElement(FocusNavigationDirection::Down));
                VERIFY_IS_TRUE(button->Equals(btn2));

                //Verify that using hint also works
                ::Windows::Foundation::Rect rect(0, 50, 50, 50);
                button = safe_cast<Button^>(FocusManager::FindNextFocusableElement(FocusNavigationDirection::Down, rect));
                VERIFY_IS_TRUE(button->Equals(btn3));

                //Verify that next/prev works
                button = safe_cast<Button^>(FocusManager::FindNextFocusableElement(FocusNavigationDirection::Next));
                VERIFY_IS_TRUE(button->Equals(btn3));

                button = safe_cast<Button^>(FocusManager::FindNextFocusableElement(FocusNavigationDirection::Previous));
                VERIFY_IS_TRUE(button->Equals(btn2));
            });
        }

        void BasicFocusTests::ValidateFocusApisWithInvalidSearchRootInUAP()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel = nullptr;
            Button^ btn1 = nullptr;
            Button^ btn2 = nullptr;
            Button^ btn3 = nullptr;

            auto gotFocusEvent = std::make_shared<Event>();
            auto loadedEvent = std::make_shared<Event>();

            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

            RunOnUIThread([&]
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                    L"  <Button x:Name='btn1' TabIndex='0' Height='50' Content='Button 1'/>"
                    L"  <Button x:Name='btn2' TabIndex='2' Height='50' Content='Button 2'/>"
                    L"  <Button x:Name='btn3' TabIndex='1' Height='50' Content='Button 3'/>"
                    L"</StackPanel>"));

                VERIFY_IS_NOT_NULL(rootPanel);

                loadedRegistration.Attach(rootPanel, [&]()
                {
                    loadedEvent->Set();
                });

                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            loadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]
            {
                Platform::Object^ focusManager;

                btn1 = safe_cast<Button^>(rootPanel->FindName(L"btn1"));
                VERIFY_IS_NOT_NULL(btn1);

                btn2 = safe_cast<Button^>(rootPanel->FindName(L"btn2"));
                VERIFY_IS_NOT_NULL(btn2);

                btn3 = safe_cast<Button^>(rootPanel->FindName(L"btn3"));
                VERIFY_IS_NOT_NULL(btn3);

                gotFocusRegistration.Attach(btn1, [gotFocusEvent]()
                {
                    LOG_OUTPUT(L"Button1 got focus");
                    gotFocusEvent->Set();
                });

                btn3->Focus(FocusState::Keyboard);
                btn1->Focus(FocusState::Keyboard);
            });

            gotFocusEvent->WaitForDefault();

            RunOnUIThread([&]
            {
                xaml_input::FindNextElementOptions^ options = ref new xaml_input::FindNextElementOptions;
                options->SearchRoot = nullptr;
                
                LOG_OUTPUT(L"Validating TryMoveFocus");
                bool resultTryMoveFocus = xaml_input::FocusManager::TryMoveFocus(xaml_input::FocusNavigationDirection::Down, options);
                VERIFY_IS_TRUE(resultTryMoveFocus);
                
                LOG_OUTPUT(L"Validating FindNextElement");
                DependencyObject^ object = FocusManager::FindNextElement(FocusNavigationDirection::Down, options);
                VERIFY_IS_TRUE(btn3->Equals(object));
            });
        }

        void BasicFocusTests::FindNextFocusableElementForUIElement()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel;
            StackPanel^ sp1;
            StackPanel^ sp2;
            StackPanel^ sp3;
            TextBlock^ tb1;
            TextBlock^ tb2;
            RichTextBlock^ rtb;

            auto gotFocusEvent = std::make_shared<Event>();
            auto loadedEvent = std::make_shared<Event>();

            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, GotFocus);
            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

            RunOnUIThread([&]
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                    L"  <StackPanel x:Name='sp1' HorizontalAlignment = 'Left' Width='50' Height='50' Background='Red' IsTabStop='True' TabIndex='0' UseSystemFocusVisuals='True'/>"
                    L"  <StackPanel x:Name='sp2' HorizontalAlignment = 'Left' Width='50' Height='50' Background='Green' IsTabStop='True' TabIndex='1' UseSystemFocusVisuals='True'/>"
                    L"  <StackPanel x:Name='sp3' HorizontalAlignment = 'Left' Width='50' Height='50' Background='Blue' IsTabStop='True' TabIndex='2' UseSystemFocusVisuals='True'/>"
                    L"  <TextBlock x:Name='tb1' IsTabStop='True' TabIndex='3' UseSystemFocusVisuals='True' Text='TextBlock'/>"
                    L"  <TextBlock x:Name='tb2' IsTabStop='True' TabIndex='4' UseSystemFocusVisuals='True' Text='TextBlock'/>"
                    L"  <RichTextBlock x:Name='rtb' IsTabStop='True' TabIndex='5' UseSystemFocusVisuals='True'>"
                    L"    <Paragraph>RichTextBlock</Paragraph>"
                    L"  </RichTextBlock>"
                    L"</StackPanel>"));

                VERIFY_IS_NOT_NULL(rootPanel);

                loadedRegistration.Attach(rootPanel, [&]()
                {
                    loadedEvent->Set();
                });

                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            loadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]
            {
                Platform::Object^ focusManager;

                sp1 = safe_cast<StackPanel^>(rootPanel->FindName(L"sp1"));
                VERIFY_IS_NOT_NULL(sp1);

                sp2 = safe_cast<StackPanel^>(rootPanel->FindName(L"sp2"));
                VERIFY_IS_NOT_NULL(sp2);

                sp3 = safe_cast<StackPanel^>(rootPanel->FindName(L"sp3"));
                VERIFY_IS_NOT_NULL(sp3);

                tb1 = safe_cast<TextBlock^>(rootPanel->FindName(L"tb1"));
                VERIFY_IS_NOT_NULL(tb1);

                tb2 = safe_cast<TextBlock^>(rootPanel->FindName(L"tb2"));
                VERIFY_IS_NOT_NULL(tb2);

                rtb = safe_cast<RichTextBlock^>(rootPanel->FindName(L"rtb"));
                VERIFY_IS_NOT_NULL(sp3);

                gotFocusRegistration.Attach(sp2, [gotFocusEvent]()
                {
                    LOG_OUTPUT(L"StackPanel2 got focus");
                    gotFocusEvent->Set();
                });

                FocusManager::TryFocusAsync(sp2, FocusState::Keyboard);
            });

            gotFocusEvent->WaitForDefault();

            RunOnUIThread([&]
            {
                StackPanel^ sp = safe_cast<StackPanel^>(FocusManager::FindNextFocusableElement(FocusNavigationDirection::Down));
                VERIFY_IS_TRUE(sp->Equals(sp3));

                //Verify that using hint also works
                ::Windows::Foundation::Rect rect(0, 50, 50, 50);
                sp = safe_cast<StackPanel^>(FocusManager::FindNextFocusableElement(FocusNavigationDirection::Down, rect));
                VERIFY_IS_TRUE(sp->Equals(sp3));

                //Verify that next/prev works
                sp = safe_cast<StackPanel^>(FocusManager::FindNextFocusableElement(FocusNavigationDirection::Next));
                VERIFY_IS_TRUE(sp->Equals(sp3));

                sp = safe_cast<StackPanel^>(FocusManager::FindNextFocusableElement(FocusNavigationDirection::Previous));
                VERIFY_IS_TRUE(sp->Equals(sp1));

                FocusManager::TryFocusAsync(tb2, FocusState::Keyboard);

                RichTextBlock^ rtbTest = safe_cast<RichTextBlock^>(FocusManager::FindNextFocusableElement(FocusNavigationDirection::Next));
                VERIFY_IS_TRUE(rtbTest->Equals(rtb));

                TextBlock^ tbTest = safe_cast<TextBlock^>(FocusManager::FindNextFocusableElement(FocusNavigationDirection::Previous));
                VERIFY_IS_TRUE(tbTest->Equals(tb1));
            });
        }

        void BasicFocusTests::CannotSetRichTextBlockOverflowAsTabStop()
        {            
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]
            {
                bool testPass = false;
                try
                {
                    RichTextBlockOverflow^ rtbo = ref new RichTextBlockOverflow();
                    LOG_OUTPUT(L"Trying to set RichTextBlock.IsTabStop, expect failure");
                    rtbo->IsTabStop = true;
                }
                catch (Platform::Exception^ e)
                {
                    VERIFY_IS_TRUE(e->HResult == E_INVALIDARG);
                    testPass = true;
                }
                VERIFY_IS_TRUE(testPass);
            });

            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]
            {
                bool testPass = false;
                try
                {
                    RichTextBlockOverflow^ rtbo = ref new RichTextBlockOverflow();
                    LOG_OUTPUT(L"Trying to set RichTextBlock.TabIndex, expect failure");
                    rtbo->TabIndex = 0;
                }
                catch (Platform::Exception^ e)
                {
                    VERIFY_IS_TRUE(e->HResult == E_INVALIDARG);
                    testPass = true;
                }
                VERIFY_IS_TRUE(testPass);
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void BasicFocusTests::FindNextElementReturnsDependencyObject()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel = nullptr;
            Button^ btn1 = nullptr;
            Button^ btn2 = nullptr;
            Button^ btn3 = nullptr;
            xaml::Documents::Hyperlink^ hyperlink = nullptr;

            auto gotFocusEvent = std::make_shared<Event>();
            auto loadedEvent = std::make_shared<Event>();

            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

            RunOnUIThread([&]
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                    L"  <Button x:Name='btn1' TabIndex='0' Height='50' Content='Button 1'/>"
                    L"  <TextBlock><Hyperlink x:Name='hyperlink'>hyperlink</Hyperlink></TextBlock>"
                    L"</StackPanel>"));

                VERIFY_IS_NOT_NULL(rootPanel);

                btn1 = safe_cast<Button^>(rootPanel->FindName(L"btn1"));
                VERIFY_IS_NOT_NULL(btn1);

                hyperlink = safe_cast<xaml_docs::Hyperlink^>(rootPanel->FindName(L"hyperlink"));
                VERIFY_IS_NOT_NULL(hyperlink);

                loadedRegistration.Attach(rootPanel, [&]()
                {
                    loadedEvent->Set();
                });

                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            loadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(btn1, FocusState::Keyboard);

            RunOnUIThread([&]
            {
                //Verify that FindNextFocusableElement cannot return the hyperlink
                UIElement^ element = FocusManager::FindNextFocusableElement(FocusNavigationDirection::Down);
                VERIFY_IS_NULL(element);

                // Verify that FindNextElement does return the hyperlink
                DependencyObject^ object = FocusManager::FindNextElement(FocusNavigationDirection::Down);
                VERIFY_IS_TRUE(hyperlink->Equals(object));
            });
        }

        void BasicFocusTests::VerifyCyclingWhenTabNavigationSet()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel = nullptr;
            Button^ btn = nullptr;

            Button^ btnA = nullptr;
            Button^ btnB = nullptr;
            Button^ btnC = nullptr;
            Button^ btnD = nullptr;

            const int NUM_TAB = 4;

            std::wstring eventOrder;
            const std::wstring btnAString = L"btnA";
            const std::wstring btnBString = L"btnB";
            const std::wstring btnCString = L"btnC";
            const std::wstring btnDString = L"btnD";

            const std::wstring successString = btnAString + btnBString + btnCString + btnDString
                + btnAString + btnDString + btnCString + btnBString + btnAString + btnDString;

            auto gotFocusEvent = std::make_shared<Event>();
            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto gotFocusEventB = std::make_shared<Event>();
            auto gotFocusRegistrationB = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto gotFocusEventC = std::make_shared<Event>();
            auto gotFocusRegistrationC = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto gotFocusEventD = std::make_shared<Event>();
            auto gotFocusRegistrationD = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto loadedEvent = std::make_shared<Event>();
            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

            RunOnUIThread([&]
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                    L"  <Button x:Name='btn' Height='50' Content='Button0'/>"
                    L"  <ContentControl IsTabStop='False' TabNavigation='Cycle'>"
                    L"      <StackPanel>"
                    L"          <StackPanel>"
                    L"              <Button x:Name='btnA' Content='Button0'/>"
                    L"              <Button x:Name='btnB' Content='Button1'/>"
                    L"              <Button x:Name='btnC' Content='Button2'/>"
                    L"          </StackPanel>"
                    L"          <Button x:Name='btnD' Content='Button3'/>"
                    L"      </StackPanel>"
                    L"  </ContentControl>"
                    L"</StackPanel>"));

                VERIFY_IS_NOT_NULL(rootPanel);

                loadedRegistration.Attach(rootPanel, [&]()
                {
                    loadedEvent->Set();
                });

                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            loadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]
            {
                btn = safe_cast<Button^>(rootPanel->FindName(L"btn"));
                VERIFY_IS_NOT_NULL(btn);

                btnA = safe_cast<Button^>(rootPanel->FindName(L"btnA"));
                VERIFY_IS_NOT_NULL(btnA);

                btnB = safe_cast<Button^>(rootPanel->FindName(L"btnB"));
                VERIFY_IS_NOT_NULL(btnB);

                btnC = safe_cast<Button^>(rootPanel->FindName(L"btnC"));
                VERIFY_IS_NOT_NULL(btnC);

                btnD = safe_cast<Button^>(rootPanel->FindName(L"btnD"));
                VERIFY_IS_NOT_NULL(btnD);

                gotFocusRegistration.Attach(btnA, [&]()
                {
                    LOG_OUTPUT(L"btnA got focus");
                    eventOrder.append(btnAString);
                    gotFocusEvent->Set();
                });

                gotFocusRegistrationB.Attach(btnB, [&]()
                {
                    LOG_OUTPUT(L"btnB got focus");
                    eventOrder.append(btnBString);
                    gotFocusEventB->Set();
                });

                gotFocusRegistrationC.Attach(btnC, [&]()
                {
                    LOG_OUTPUT(L"btnC got focus");
                    eventOrder.append(btnCString);
                    gotFocusEventC->Set();
                });

                gotFocusRegistrationD.Attach(btnD, [&]()
                {
                    LOG_OUTPUT(L"btnD got focus");
                    eventOrder.append(btnDString);
                    gotFocusEventD->Set();
                });

                btn->Focus(FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();

            for (int count = 0; count < NUM_TAB + 1; count++)
            {
                TestServices::KeyboardHelper->Tab();
                TestServices::WindowHelper->WaitForIdle();
            }

            //Go in reverse order
            for (int count = NUM_TAB - 1; count >= 0; count--)
            {
                TestServices::KeyboardHelper->ShiftTab();
                TestServices::WindowHelper->WaitForIdle();
            }

            TestServices::KeyboardHelper->ShiftTab();
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_ARE_EQUAL(successString, eventOrder);
        }

        void BasicFocusTests::VerifyCyclingWithTabIndexWhenTabNavigationSet()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel = nullptr;
            Button^ btn = nullptr;

            Button^ btnA = nullptr;
            Button^ btnB = nullptr;
            Button^ btnC = nullptr;
            Button^ btnD = nullptr;

            const int NUM_TAB = 4;

            std::wstring eventOrder;
            const std::wstring btnAString = L"btnA";
            const std::wstring btnBString = L"btnB";
            const std::wstring btnCString = L"btnC";
            const std::wstring btnDString = L"btnD";

            const std::wstring successString = btnDString + btnAString + btnCString + btnBString
                + btnDString + btnBString + btnCString + btnAString + btnDString + btnBString;

            auto gotFocusEvent = std::make_shared<Event>();
            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto gotFocusEventB = std::make_shared<Event>();
            auto gotFocusRegistrationB = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto gotFocusEventC = std::make_shared<Event>();
            auto gotFocusRegistrationC = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto gotFocusEventD = std::make_shared<Event>();
            auto gotFocusRegistrationD = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto loadedEvent = std::make_shared<Event>();
            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

            RunOnUIThread([&]
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                    L"  <Button x:Name='btn' Height='50' Content='Button0'/>"
                    L"  <ContentControl IsTabStop='False' TabNavigation='Cycle'>"
                    L"      <StackPanel>"
                    L"          <StackPanel>"
                    L"              <Button x:Name='btnA' TabIndex='1' Content='Button0'/>"
                    L"              <Button x:Name='btnB' TabIndex='3' Content='Button1'/>"
                    L"              <Button x:Name='btnC' TabIndex='2' Content='Button2'/>"
                    L"          </StackPanel>"
                    L"          <Button x:Name='btnD' TabIndex='0' Content='Button3'/>"
                    L"      </StackPanel>"
                    L"  </ContentControl>"
                    L"</StackPanel>"));

                VERIFY_IS_NOT_NULL(rootPanel);

                loadedRegistration.Attach(rootPanel, [&]()
                {
                    loadedEvent->Set();
                });

                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            loadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]
            {
                btn = safe_cast<Button^>(rootPanel->FindName(L"btn"));
                VERIFY_IS_NOT_NULL(btn);

                btnA = safe_cast<Button^>(rootPanel->FindName(L"btnA"));
                VERIFY_IS_NOT_NULL(btnA);

                btnB = safe_cast<Button^>(rootPanel->FindName(L"btnB"));
                VERIFY_IS_NOT_NULL(btnB);

                btnC = safe_cast<Button^>(rootPanel->FindName(L"btnC"));
                VERIFY_IS_NOT_NULL(btnC);

                btnD = safe_cast<Button^>(rootPanel->FindName(L"btnD"));
                VERIFY_IS_NOT_NULL(btnD);

                gotFocusRegistration.Attach(btnA, [&]()
                {
                    LOG_OUTPUT(L"btnA got focus");
                    eventOrder.append(btnAString);
                    gotFocusEvent->Set();
                });

                gotFocusRegistrationB.Attach(btnB, [&]()
                {
                    LOG_OUTPUT(L"btnB got focus");
                    eventOrder.append(btnBString);
                    gotFocusEventB->Set();
                });

                gotFocusRegistrationC.Attach(btnC, [&]()
                {
                    LOG_OUTPUT(L"btnC got focus");
                    eventOrder.append(btnCString);
                    gotFocusEventC->Set();
                });

                gotFocusRegistrationD.Attach(btnD, [&]()
                {
                    LOG_OUTPUT(L"btnD got focus");
                    eventOrder.append(btnDString);
                    gotFocusEventD->Set();
                });

                btn->Focus(FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();

            for (int count = 0; count < NUM_TAB + 1; count++)
            {
                TestServices::KeyboardHelper->Tab();
                TestServices::WindowHelper->WaitForIdle();
            }

            //Go in reverse order
            for (int count = NUM_TAB - 1; count >= 0; count--)
            {
                TestServices::KeyboardHelper->ShiftTab();
                TestServices::WindowHelper->WaitForIdle();
            }

            TestServices::KeyboardHelper->ShiftTab();
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_ARE_EQUAL(successString, eventOrder);
        }

        void BasicFocusTests::VerifyShiftTabWhenOnceTabNavigationSet()
        {
            VerifyTabNavigationWorker(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <Button x:Name='btn0' Height='50' Content='Button0'/>"
                L"  <ContentControl IsTabStop='False' TabNavigation='Once'>"
                L"      <StackPanel>"
                L"          <StackPanel>"
                L"              <Button x:Name='btnA' Content='ButtonA'/>"
                L"              <Button x:Name='btnB' Content='ButtonB'/>"
                L"              <Button x:Name='btnC' Content='ButtonC'/>"
                L"          </StackPanel>"
                L"          <Button x:Name='btnD' Content='ButtonD'/>"
                L"      </StackPanel>"
                L"  </ContentControl>"
                L"</StackPanel>",
                L"0;D;0;D;0;D;", // The order the buttons receive focus in.  We start at button 0 and then press shift-tab 5x
                TabType::ShiftTab);
        }

        void BasicFocusTests::VerifyShiftTabWithNavigationOnceWithNestedButtons()
        {
            VerifyTabNavigationWorker(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <Button x:Name='btn0' Height='50' Content='Button0'/>"
                L"  <StackPanel TabFocusNavigation='Once'>"
                L"      <StackPanel>"
                L"          <Button x:Name='btnA' Content='ButtonA'/>"
                L"      </StackPanel>"
                L"      <StackPanel>"
                L"          <Button x:Name='btnB' Content='ButtonB'/>"
                L"      </StackPanel>"  
                L"      <StackPanel>" 
                L"          <StackPanel>" 
                L"              <Button x:Name='btnC' Content='ButtonC'/>"
                L"          </StackPanel>"
                L"      </StackPanel>"
                L"  </StackPanel>"
                L"  <Button x:Name='btnD' Content='ButtonD'/>"
                L"</StackPanel>",
                L"0;D;C;0;D;C;" ,// The order the buttons receive focus in.  We start at button 0 and then press shift-tab 5x
                TabType::ShiftTab);
        }

        void BasicFocusTests::VerifyTabNavigationWorker(
            const wchar_t* xamlString,
            const wchar_t* expectedVisitationOrder,
            TabType tabType)
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel = nullptr;
            Button^ btn0 = nullptr;

            Button^ btnA = nullptr;
            Button^ btnB = nullptr;
            Button^ btnC = nullptr;
            Button^ btnD = nullptr;

            const int NUM_TAB = 5;

            std::wstring eventOrder;

            auto gotFocusEventButton = std::make_shared<Event>();
            auto gotFocusRegistrationButton = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto gotFocusEvent = std::make_shared<Event>();
            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto gotFocusEventB = std::make_shared<Event>();
            auto gotFocusRegistrationB = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto gotFocusEventC = std::make_shared<Event>();
            auto gotFocusRegistrationC = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto gotFocusEventD = std::make_shared<Event>();
            auto gotFocusRegistrationD = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            RunOnUIThread([&]
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(ref new Platform::String(xamlString)));

                VERIFY_IS_NOT_NULL(rootPanel);

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]
            {
                btn0 = safe_cast<Button^>(rootPanel->FindName(L"btn0"));
                VERIFY_IS_NOT_NULL(btn0);

                btnA = safe_cast<Button^>(rootPanel->FindName(L"btnA"));
                VERIFY_IS_NOT_NULL(btnA);

                btnB = safe_cast<Button^>(rootPanel->FindName(L"btnB"));
                VERIFY_IS_NOT_NULL(btnB);

                btnC = safe_cast<Button^>(rootPanel->FindName(L"btnC"));
                VERIFY_IS_NOT_NULL(btnC);

                btnD = safe_cast<Button^>(rootPanel->FindName(L"btnD"));
                VERIFY_IS_NOT_NULL(btnD);

                gotFocusRegistrationButton.Attach(btn0, [&]()
                {
                    LOG_OUTPUT(L"btn0 got focus");
                    eventOrder.append(L"0;");
                    gotFocusEventButton->Set();
                });

                gotFocusRegistration.Attach(btnA, [&]()
                {
                    LOG_OUTPUT(L"btnA got focus");
                    eventOrder.append(L"A;");
                    gotFocusEvent->Set();
                });

                gotFocusRegistrationB.Attach(btnB, [&]()
                {
                    LOG_OUTPUT(L"btnB got focus");
                    eventOrder.append(L"B;");
                    gotFocusEventB->Set();
                });

                gotFocusRegistrationC.Attach(btnC, [&]()
                {
                    LOG_OUTPUT(L"btnC got focus");
                    eventOrder.append(L"C;");
                    gotFocusEventC->Set();
                });

                gotFocusRegistrationD.Attach(btnD, [&]()
                {
                    LOG_OUTPUT(L"btnD got focus");
                    eventOrder.append(L"D;");
                    gotFocusEventD->Set();
                });

                btn0->Focus(FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();

            for (int count = 0; count < NUM_TAB; count++)
            {
                if (tabType == TabType::Tab)
                {
                    TestServices::KeyboardHelper->Tab();
                }
                else if (tabType == TabType::ShiftTab)
                {
                    TestServices::KeyboardHelper->ShiftTab();
                }
                TestServices::WindowHelper->WaitForIdle();
            }

            LOG_OUTPUT(L"Expected event order: %s", expectedVisitationOrder);
            LOG_OUTPUT(L"Observed event order: %s", eventOrder.c_str());
            VERIFY_ARE_EQUAL(std::wstring{expectedVisitationOrder}, eventOrder);
        }

        void BasicFocusTests::SetFocusOnTheFirstFocusableElement()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::TextBox^ textBox = nullptr;
            xaml::XamlRoot^ xamlRoot = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                    L"      x:Name='root' Background='SlateBlue' Width='400' Height='200' VerticalAlignment='Top' HorizontalAlignment='Left'> "
                    L"  <TextBox x:Name='textBox' Width='100' Height='20' HorizontalAlignment='Left' FontSize='25' /> "
                    L"</Grid>"));

                VERIFY_IS_NOT_NULL(rootPanel);
                TestServices::WindowHelper->WindowContent = rootPanel;

                textBox = dynamic_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textBox"));
                VERIFY_IS_NOT_NULL(textBox);

            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                xamlRoot = textBox->XamlRoot;
            });

            // Simulate app launch by injecting window messages.
            TestServices::WindowHelper->InjectWindowMessage(WM_KILLFOCUS, 0, 0, xamlRoot);
            TestServices::WindowHelper->InjectWindowMessage(WM_ACTIVATE, 0, 0, xamlRoot);
            TestServices::WindowHelper->InjectWindowMessage(WM_ACTIVATE, WA_ACTIVE, 0, xamlRoot);
            TestServices::WindowHelper->InjectWindowMessage(WM_SETFOCUS, 0, 0, xamlRoot);

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_NOT_NULL(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
                VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(textBox));
            });
        }

        void BasicFocusTests::FocusActivationAndSIP()
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
            XamlRoot^ xamlRoot;

            auto loadedEvent = std::make_shared<Event>();
            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);
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
                    L"    <Button x:Name='Button1' Height='20' Width='200' Content='Button' />"
                    L"    <TextBox x:Name='TextBox' Height='80' Width='200'/>"
                    L"</StackPanel>"));

                textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"TextBox"));
                btn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"Button"));

                loadedRegistration.Attach(rootPanel, [&]() {loadedEvent->Set(); });

                textboxGotFocusRegistration.Attach(
                    textBox,
                    [textboxGotFocusEvent]()
                {
                    LOG_OUTPUT(L"Textbox GotFocus.");
                    textboxGotFocusEvent->Set();
                });

                textboxLostFocusRegistration.Attach(
                    textBox,
                    [textboxLostFocusEvent]()
                {
                    LOG_OUTPUT(L"Textbox LostFocus.");
                    textboxLostFocusEvent->Set();
                });

                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            loadedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                xamlRoot = rootPanel->XamlRoot;
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

            LOG_OUTPUT(L"Simulate app deactivate...");

            // Simulate app deactivation
            TestServices::WindowHelper->InjectWindowMessage(WM_KILLFOCUS, 0, 0, xamlRoot);
            TestServices::WindowHelper->InjectWindowMessage(WM_ACTIVATE, WA_INACTIVE, 0, xamlRoot);
            TestServices::WindowHelper->WaitForIdle();
            textboxLostFocusEvent->WaitForDefault();
            SIPHidingEvent->WaitForDefault();

            LOG_OUTPUT(L"Simulate app resume...");
            // Simulate app resume
            SIPShowingEvent->Reset();
            TestServices::WindowHelper->InjectWindowMessage(WM_ACTIVATE, WA_ACTIVE, 0, xamlRoot);
            TestServices::WindowHelper->InjectWindowMessage(WM_SETFOCUS, 0, 0, xamlRoot);
            TestServices::WindowHelper->WaitForIdle();
            textboxGotFocusEvent->WaitForDefault();
            SIPShowingEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                textBox->IsEnabled = false;
            });
            TestServices::WindowHelper->WaitForIdle();

        }

        void BasicFocusTests::VerifyElementFocusIsSetAfterPluginFocus()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::Page^ page;
            xaml_controls::Button^ button = nullptr;

            auto pageLoadedRegistration = CreateSafeEventRegistration(xaml_controls::Page, Loaded);
            auto pageLoadedEvent = std::make_shared<Event>();
            auto buttonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
            auto buttonGotFocusEvent = std::make_shared<Event>();

            auto buttonLostFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, LostFocus);
            auto buttonLostFocusEvent = std::make_shared<Event>();
            RunOnUIThread([&]()
            {
                page = TestServices::WindowHelper->SetupSimulatedAppPage();
                button = ref new xaml_controls::Button();
                button->Width = 200;
                button->Height = 75;
                button->Content = "Click";

                page->Content = button;

                pageLoadedRegistration.Attach(page, [&]() { pageLoadedEvent->Set(); });

                buttonGotFocusRegistration.Attach(button, [&]()
                {
                    VERIFY_IS_TRUE(pageLoadedEvent->HasFired());
                    wuc::CoreWindowActivationMode status = xaml::Window::Current->CoreWindow->ActivationMode;
                    bool activatedStatus = ( status == wuc::CoreWindowActivationMode::ActivatedInForeground ||
                                                            status == wuc::CoreWindowActivationMode::ActivatedNotForeground ) ;
                    VERIFY_IS_TRUE( activatedStatus ); //Implicit verification that our plugin has focus
                    buttonGotFocusEvent->Set();
                });

                buttonLostFocusRegistration.Attach(button, [&]()
                {
                    buttonLostFocusEvent->Set();
                });
            });
            TestServices::WindowHelper->WaitForIdle();
            buttonGotFocusEvent->WaitForDefault();

            VERIFY_IS_FALSE(buttonLostFocusEvent->HasFired());
        }

        void BasicFocusTests::DoNotSetFocusOnElementWithCollapsedParent()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel = nullptr;
            StackPanel^ stackPanel = nullptr;
            Button^ btn = nullptr;
            Button^ btn2 = nullptr;

            auto gotFocusEvent = std::make_shared<Event>();
            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            RunOnUIThread([&]()
            {
                rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    LR"(<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Content="First Focusable Button"/>
                            <Button x:Name="btn1" Content="Button 1"/>
                            <StackPanel x:Name="panel">
                                <Button x:Name="btn2" Content="Button 2"/>
                            </StackPanel>
                        </StackPanel>)"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                stackPanel = dynamic_cast<xaml_controls::StackPanel^>(rootPanel->FindName(L"panel"));
                btn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"btn1"));
                btn2 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"btn2"));

                gotFocusRegistration.Attach(btn, [&]()
                {
                    LOG_OUTPUT(L"btn got focus");
                    gotFocusEvent->Set();
                });

                btn->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            gotFocusEvent->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusEvent->HasFired());

            gotFocusEvent->Reset();

            RunOnUIThread([&]()
            {
                stackPanel->Visibility = Visibility::Collapsed;
                btn2->Focus(FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(btn));
            });
        }

        void BasicFocusTests::VerifyFocusRectShowsOnXboxOnInitialLoad()
        {
            // Note:
            // Disable WUCRenderingScopeGuard setting of Window.Content via resetWindowContent = false,
            // as setting Window.Content multiple times apparently disables automatic focus setting, which is the purpose of this test.
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree,
                                        false,  // resizeWindow
                                        true,   // injectMockDComp
                                        true,   // resetDevice
                                        false   // resetWindowContent
                                        );

            TestServices::WindowHelper->SetLastInputMethod(test_infra::LastInputDeviceType::GamepadOrRemote, nullptr);
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

            StackPanel^ rootPanel = nullptr;
            Button^ btn = nullptr;

            auto gotFocusEvent = std::make_shared<Event>();
            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            RunOnUIThread([&]()
            {
                rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    LR"(<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        </StackPanel>)"));

                btn = ref new Button();
                btn->Content = L"Button 1";

                gotFocusRegistration.Attach(btn, [&]()
                {
                    LOG_OUTPUT(L"btn got focus");
                    gotFocusEvent->Set();
                });
                rootPanel->Children->Append(btn);

                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            gotFocusEvent->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusEvent->HasFired());

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(xaml::FocusState::Keyboard, btn->FocusState);
            });
            TestServices::WindowHelper->WaitForIdle();

            //Verifies Focus Rect
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }

        void BasicFocusTests::VerifyInitialFocusOnLoadOnPhone()
        {
            TestCleanupWrapper cleanup;

            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

            StackPanel^ rootPanel = nullptr;
            Button^ btn = nullptr;

            auto gotFocusEvent = std::make_shared<Event>();
            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            RunOnUIThread([&]()
            {
                rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    LR"(<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        </StackPanel>)"));

                btn = ref new Button();
                btn->Content = "btn 1";
                gotFocusRegistration.Attach(btn, [&]()
                {
                    LOG_OUTPUT(L"btn got focus");
                    gotFocusEvent->Set();
                });
                rootPanel->Children->Append(btn);

                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            gotFocusEvent->WaitForDefault();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(btn->FocusState, FocusState::Pointer);
            });
        }

        void BasicFocusTests::DoNotSetFocusOnHyperlinkWithDisabledParent()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel = nullptr;
            Button^ btn = nullptr;
            Frame^ frame = nullptr;

            RunOnUIThread([&]()
            {
                rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    LR"(<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button x:Name="btn" Content="Test Button"/>
                            <Frame x:Name="ContentRootPanel">
                                <StackPanel>
                                    <TextBlock>
                                            <Hyperlink>
                                                <Hyperlink.Inlines>
                                                    <Run Text="Hyperlink with Inlines in TextBlock"/>
                                                </Hyperlink.Inlines>
                                            </Hyperlink>
                                    </TextBlock>
                                    <HyperlinkButton Content="HyperlinkButton"/>
                                </StackPanel>
                            </Frame>
                        </StackPanel>)"));

                TestServices::WindowHelper->WindowContent = rootPanel;
                btn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"btn"));
                frame = safe_cast<xaml_controls::Frame^>(rootPanel->FindName(L"ContentRootPanel"));
            });

            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(btn, FocusState::Keyboard);

            RunOnUIThread([&]()
            {
                frame->IsEnabled = false;
            });

            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(btn));
            });
        }

        void BasicFocusTests::VerifyFocusMoveAfterLeavingTree()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel = nullptr;
            StackPanel^ innerPanel = nullptr;
            Button^ btn = nullptr;
            Button^ btnWithExpectedFocus = nullptr;

            RunOnUIThread([&]()
            {
                rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    LR"(<StackPanel
                            xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                            x:Name="rootPanel">
                            <Button x:Name="btn01" Content="Button01"/>
                            <Button x:Name="btn02" Content="Button02"/>
                            <StackPanel x:Name="innerPanel">
                                <Button x:Name="btn11" Content="Button11"/>
                                <Button x:Name="btn12" Content="Button12"/>
                                <Button x:Name="btn13" Content="Button13"/>
                            </StackPanel>
                            <Button x:Name="btn03" Content="Button03"/>
                            <Button x:Name="btn04" Content="Button04"/>
                        </StackPanel>)"));
                VERIFY_IS_NOT_NULL(rootPanel);

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                innerPanel = safe_cast<StackPanel^>(rootPanel->FindName(L"innerPanel"));
                VERIFY_IS_NOT_NULL(innerPanel);

                btnWithExpectedFocus = safe_cast<Button^>(innerPanel->FindName(L"btn03"));
                VERIFY_IS_NOT_NULL(btnWithExpectedFocus);

                btn = safe_cast<Button^>(innerPanel->FindName(L"btn11"));
                VERIFY_IS_NOT_NULL(btn);
            });

            LOG_OUTPUT(L"Focusing btn11");
            FocusTestHelper::EnsureFocus(btn, FocusState::Programmatic);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(btn));

                LOG_OUTPUT(L"Removing focused btn11");
                innerPanel->Children->RemoveAt(0);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // This is a case where the leaving element, btn11, is not the last focusable element among its siblings.
                // btn03 gets focus instead of btn12.
                auto focusedElement = xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot);
                VERIFY_IS_NOT_NULL(focusedElement);

                LOG_OUTPUT(L"Focused element: %s", focusedElement->ToString()->Data());
                if (auto fe = dynamic_cast<xaml::FrameworkElement^>(focusedElement))
                {
                    LOG_OUTPUT(L"Focused framework element name: %s", fe->Name->ToString()->Data());
                }

                VERIFY_IS_TRUE(focusedElement->Equals(btnWithExpectedFocus));

                btn = safe_cast<Button^>(innerPanel->FindName(L"btn13"));
                VERIFY_IS_NOT_NULL(btn);
            });

            LOG_OUTPUT(L"Focusing btn13");
            FocusTestHelper::EnsureFocus(btn, FocusState::Programmatic);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(btn));

                LOG_OUTPUT(L"Removing focused btn13");
                innerPanel->Children->RemoveAt(1);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto focusedElement = xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot);
                VERIFY_IS_NOT_NULL(focusedElement);

                LOG_OUTPUT(L"Focused element: %s", focusedElement->ToString()->Data());
                if (auto fe = dynamic_cast<xaml::FrameworkElement^>(focusedElement))
                {
                    LOG_OUTPUT(L"Focused framework element name: %s", fe->Name->ToString()->Data());
                }

                VERIFY_IS_TRUE(focusedElement->Equals(btnWithExpectedFocus));

                btn = safe_cast<Button^>(innerPanel->FindName(L"btn12"));
                VERIFY_IS_NOT_NULL(btn);
            });

            LOG_OUTPUT(L"Focusing btn12");
            FocusTestHelper::EnsureFocus(btn, FocusState::Programmatic);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(btn));

                LOG_OUTPUT(L"Removing focused btn12");
                innerPanel->Children->RemoveAt(0);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto focusedElement = xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot);
                VERIFY_IS_NOT_NULL(focusedElement);

                LOG_OUTPUT(L"Focused element: %s", focusedElement->ToString()->Data());
                if (auto fe = dynamic_cast<xaml::FrameworkElement^>(focusedElement))
                {
                    LOG_OUTPUT(L"Focused framework element name: %s", fe->Name->ToString()->Data());
                }

                VERIFY_IS_TRUE(focusedElement->Equals(btnWithExpectedFocus));
            });
        }
    }
}
