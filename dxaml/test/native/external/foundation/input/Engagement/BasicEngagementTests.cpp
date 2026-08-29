// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "BasicEngagementTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <ppltasks.h>
#include "FileLoader.h"
#include <TestCleanupWrapper.h>
#include <SafeEventRegistration.h>
#include <RuntimeEnabledFeaturesEnum.h>

#include <FocusTestHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Input;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace Engagement {

        bool BasicEngagementTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool BasicEngagementTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool BasicEngagementTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        Platform::String^ BasicEngagementTests::GetPathToFiles() const
        {
            // Get the deployment directory, and then append our test's directory to the end
            auto deploymentDir = GetTestDeploymentDir();
            return ref new Platform::String(deploymentDir + L"resources\\native\\foundation\\input\\Engagement\\");
        }

        void BasicEngagementTests::CheckFocusStatusForAllControls(
            _In_ std::vector<Control^>* pControls,
            _In_opt_ Control^ expectedToBeFocused,
            FocusState expectedFocusState) const
        {
                for (Control^ control : *pControls)
                {
                    if (control == expectedToBeFocused && control->FocusState != expectedFocusState)
                    {
                        LOG_OUTPUT(L"CheckFocusStatusForAllControls - error: expected FocusState=%d, actual FocusState=%d.", expectedFocusState, control->FocusState);
                        VERIFY_ARE_EQUAL(control->FocusState, expectedFocusState);
                    }
                    else if (control != expectedToBeFocused && control->FocusState != FocusState::Unfocused)
                    {
                        LOG_OUTPUT(L"CheckFocusStatusForAllControls - error: expected FocusState=Unfocused, actual FocusState=%d.", control->FocusState);
                        VERIFY_ARE_EQUAL(control->FocusState, FocusState::Unfocused);
                    }
                }
        }

        void BasicEngagementTests::IsFocusEngagementEnabledProperty()
        {
            TestCleanupWrapper cleanup;

            InputDevice device = InputDevice::Gamepad;

            Slider^ slider1 = nullptr;
            ListView^ lv1 = nullptr;
            StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"EngagementTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            auto slider1FocusEngaged = std::make_shared<Event>();
            auto slider1FocusEngagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusEngaged);

            auto slider1FocusDisengaged = std::make_shared<Event>();
            auto slider1FocusDisengagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusDisengaged);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                lv1 = dynamic_cast<ListView^>(rootStackPanel->FindName(L"lv1"));
                slider1 = dynamic_cast<Slider^>(rootStackPanel->FindName(L"slider1"));

                slider1->IsFocusEngagementEnabled = true;
                lv1->IsFocusEngagementEnabled = true;

                slider1FocusEngagedRegistration.Attach(slider1, [&]() {slider1FocusEngaged->Set(); });
                slider1FocusDisengagedRegistration.Attach(slider1, [&]() {slider1FocusDisengaged->Set(); });

                slider1->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(slider1->IsFocusEngagementEnabled, true);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, false);
            });
            CommonInputHelper::Accept(device);
            slider1FocusEngaged->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(lv1->IsFocusEngaged, false);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, true);
                LOG_OUTPUT(L"IsFocusEngagedEnabledPropertyTest: slider1, IsFocusEngaged settable with accept");
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                slider1->IsFocusEngagementEnabled = false;
            });
            slider1FocusDisengaged->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(lv1->IsFocusEngaged, false);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, false);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngagementEnabled, false);
            });
        }

        void BasicEngagementTests::IsFocusEngagedSet()
        {
            TestCleanupWrapper cleanup;

            InputDevice device = InputDevice::Gamepad;

            Slider^ slider1 = nullptr;
            ListView^ lv1 = nullptr;

            StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"EngagementTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            auto slider1FocusEngaged = std::make_shared<Event>();
            auto slider1FocusEngagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusEngaged);

            auto slider1FocusDisengaged = std::make_shared<Event>();
            auto slider1FocusDisengagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusDisengaged);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                lv1 = dynamic_cast<ListView^>(rootStackPanel->FindName(L"lv1"));
                slider1 = dynamic_cast<Slider^>(rootStackPanel->FindName(L"slider1"));

                slider1->IsFocusEngagementEnabled = true;
                lv1->IsFocusEngagementEnabled = true;

                slider1FocusEngagedRegistration.Attach(slider1, [&]() {slider1FocusEngaged->Set(); });
                slider1FocusDisengagedRegistration.Attach(slider1, [&]() {slider1FocusDisengaged->Set(); });

                slider1->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(slider1->IsFocusEngagementEnabled, true);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, false);
            });
            CommonInputHelper::Accept(device);
            slider1FocusEngaged->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(lv1->IsFocusEngaged, false);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, true);
                LOG_OUTPUT(L"IsFocusEngagedSetTest: IsFocusEngaged settable with accept");
            });

            TestServices::WindowHelper->WaitForIdle();

            CommonInputHelper::Cancel(device);
            slider1FocusDisengaged->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(lv1->IsFocusEngaged, false);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, false);
                LOG_OUTPUT(L"IsFocusEngagedSetTest: IsFocusEngaged unsettable with cancel");
            });
        }

        void BasicEngagementTests::UnengagedSliderInteraction()
        {
            TestCleanupWrapper cleanup;
            std::vector<Control^> controlsVect;
            Button^ btnB = nullptr;
            Button^ btnA = nullptr;
            Slider^ slider1 = nullptr;

            InputDevice device = InputDevice::Gamepad;

            StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"EngagementTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                btnA = dynamic_cast<Button^>(rootStackPanel->FindName(L"btnA"));
                btnB = dynamic_cast<Button^>(rootStackPanel->FindName(L"btnB"));
                slider1 = dynamic_cast<Slider^>(rootStackPanel->FindName(L"slider1"));

                VERIFY_ARE_EQUAL(slider1->IsFocusEngagementEnabled, true);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, false);
                controlsVect.push_back(btnA);
                controlsVect.push_back(btnB);
                controlsVect.push_back(slider1);

                btnA->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(slider1->IsFocusEngagementEnabled, true);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, false);
                LOG_OUTPUT(L"UnengagedSliderInteractionTest: slider1 state correctly set in XAML");
            });
            CommonInputHelper::Right(device);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    slider1 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
                LOG_OUTPUT(L"UnengagedSliderInteractionTest: slider1 has focus");
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(slider1->IsFocusEngagementEnabled, true);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, false);
            });
            CommonInputHelper::Right(device);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    btnB /*expectedToBeFocused*/,
                    FocusState::Keyboard);
                LOG_OUTPUT(L"UnengagedSliderInteractionTest: slider1 navigation correct when not engaged");
            });
        }

        void BasicEngagementTests::SliderInteractionAfterEngagement()
        {
            TestCleanupWrapper cleanup;
            std::vector<Control^> controlsVect;
            Button^ btnB = nullptr;
            Button^ btnA = nullptr;
            Slider^ slider1 = nullptr;
            double oldSliderValue;

            InputDevice device = InputDevice::Gamepad;

            StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"EngagementTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            auto slider1FocusEngaged = std::make_shared<Event>();
            auto slider1FocusEngagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusEngaged);

            auto slider1FocusDisengaged = std::make_shared<Event>();
            auto slider1FocusDisengagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusDisengaged);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                btnA = dynamic_cast<Button^>(rootStackPanel->FindName(L"btnA"));
                btnB = dynamic_cast<Button^>(rootStackPanel->FindName(L"btnB"));
                slider1 = dynamic_cast<Slider^>(rootStackPanel->FindName(L"slider1"));

                controlsVect.push_back(btnA);
                controlsVect.push_back(btnB);
                controlsVect.push_back(slider1);
                slider1FocusEngagedRegistration.Attach(slider1, [&]() {slider1FocusEngaged->Set(); });
                slider1FocusDisengagedRegistration.Attach(slider1, [&]() {slider1FocusDisengaged->Set(); });

                btnA->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(slider1->IsFocusEngagementEnabled, true);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, false);
                LOG_OUTPUT(L"SliderInteractionAfterEngagementTest: slider1 state correctly set in XAML");
            });
            CommonInputHelper::Right(device);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    slider1 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
                LOG_OUTPUT(L"SliderInteractiontAfterEngagementTest: slider1 has focus");
            });
            TestServices::WindowHelper->WaitForIdle();

            CommonInputHelper::Accept(device);
            LOG_OUTPUT(L"SlideInteractionAfterEngagementTest: Injected accept");
            slider1FocusEngaged->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                oldSliderValue = slider1->Value;
                VERIFY_ARE_EQUAL(slider1->IsFocusEngagementEnabled, true);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, true);
            });
            CommonInputHelper::Right(device);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    slider1 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
                VERIFY_ARE_NOT_EQUAL(slider1->Value, oldSliderValue);
                LOG_OUTPUT(L"SliderInteractionAfterEngagementTest: slider1 navigation correct when engaged");
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                slider1->RemoveFocusEngagement();
            });
            slider1FocusDisengaged->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, false);
                LOG_OUTPUT(L"SliderInteractiontAfterEngagementTest: Disengaged from slider1");
            });
            TestServices::WindowHelper->WaitForIdle();

            CommonInputHelper::Right(device);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    btnB /*expectedToBeFocused*/,
                    FocusState::Keyboard);
            });
        }

        void BasicEngagementTests::DisengageOnlyIfCancelNotHandled()
        {
            TestCleanupWrapper cleanup;
            Slider^ slider1 = nullptr;

            InputDevice device = InputDevice::Gamepad;

            StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"EngagementTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            auto slider1FocusEngaged = std::make_shared<Event>();
            auto slider1FocusEngagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusEngaged);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                slider1 = ref new StickyEngagementSlider();
                rootStackPanel->Children->Append(slider1);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                slider1FocusEngagedRegistration.Attach(slider1, [&]() {slider1FocusEngaged->Set(); });

                slider1->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(slider1->IsFocusEngagementEnabled, true);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, false);
            });
            CommonInputHelper::Accept(device);
            slider1FocusEngaged->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(slider1->IsFocusEngagementEnabled, true);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, true);
                LOG_OUTPUT(L"DisengageOnlyIfCancelNotHandled: Injecting cancel");
            });
            CommonInputHelper::Cancel(device);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, true);
                LOG_OUTPUT(L"DisengageOnlyIfCancelNotHandled: slider1 still engaged");
            });
        }

        void BasicEngagementTests::ForceDisengage()
        {
            TestCleanupWrapper cleanup;
            std::vector<Control^> controlsVect;
            Slider^ slider1 = nullptr;

            InputDevice device = InputDevice::Gamepad;

            SafeEventRegistration<Microsoft::UI::Xaml::Controls::Slider, xaml_input::KeyEventHandler> slider1handler = CreateSafeEventRegistration(Microsoft::UI::Xaml::Controls::Slider, KeyDown);

            StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"EngagementTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            auto slider1FocusEngaged = std::make_shared<Event>();
            auto slider1FocusEngagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusEngaged);

            auto slider1FocusDisengaged = std::make_shared<Event>();
            auto slider1FocusDisengagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusDisengaged);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                slider1 = dynamic_cast<Slider^>(rootStackPanel->FindName(L"slider1"));

                slider1handler.Attach(
                    slider1,
                    ref new xaml_input::KeyEventHandler([this, slider1](Platform::Object^ sender, xaml_input::KeyRoutedEventArgs^ e)
                {
                    LOG_OUTPUT(L"ForceDisengageTest: Handled cancel");
                    e->Handled = true;

                }));
                slider1FocusEngagedRegistration.Attach(slider1, [&]() {slider1FocusEngaged->Set(); });
                slider1FocusDisengagedRegistration.Attach(slider1, [&]() {slider1FocusDisengaged->Set(); });

                slider1->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(slider1->IsFocusEngagementEnabled, true);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, false);
            });
            CommonInputHelper::Accept(device);
            slider1FocusEngaged->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                slider1->RemoveFocusEngagement();
                LOG_OUTPUT(L"ForceDisengageTest: Removing Focus Engagement");
            });
            slider1FocusDisengaged->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, false);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngagementEnabled, true);
                LOG_OUTPUT(L"ForceDisengageTest: slider1 not engaged");
            });
        }

        void BasicEngagementTests::DisengageAfterFocusChange()
        {
            TestCleanupWrapper cleanup;
            std::vector<Control^> controlsVect;
            Button^ btnB = nullptr;
            Button^ btnA = nullptr;
            Slider^ slider1 = nullptr;

            InputDevice device = InputDevice::Gamepad;

            auto slider1FocusEngaged = std::make_shared<Event>();
            auto slider1FocusEngagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusEngaged);

            auto slider1FocusDisengaged = std::make_shared<Event>();
            auto slider1FocusDisengagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusDisengaged);

            StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"EngagementTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                btnA = dynamic_cast<Button^>(rootStackPanel->FindName(L"btnA"));
                btnB = dynamic_cast<Button^>(rootStackPanel->FindName(L"btnB"));
                slider1 = dynamic_cast<Slider^>(rootStackPanel->FindName(L"slider1"));

                controlsVect.push_back(btnA);
                controlsVect.push_back(btnB);
                controlsVect.push_back(slider1);

                slider1FocusEngagedRegistration.Attach(slider1, [&]() {slider1FocusEngaged->Set(); });
                slider1FocusDisengagedRegistration.Attach(slider1, [&]() {slider1FocusDisengaged->Set(); });
                btnA->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(slider1->IsFocusEngagementEnabled, true);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, false);
            });
            CommonInputHelper::Right(device);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    slider1 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            CommonInputHelper::Accept(device);
            slider1FocusEngaged->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(slider1->IsFocusEngagementEnabled, true);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, true);
                btnA->Focus(FocusState::Keyboard);
            });
            slider1FocusDisengaged->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    btnA /*expectedToBeFocused*/,
                    FocusState::Keyboard);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngagementEnabled, true);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, false);
                LOG_OUTPUT(L"DisengageAfterFocusChangeTest: btnA now has focus, slider1 no longer engaged");
            });
        }

        void BasicEngagementTests::EngagedAutofocus()
        {
            TestCleanupWrapper cleanup;
            std::vector<Control^> controlsVect;
            FlipView^ fv1 = nullptr;
            ListView^ lv1 = nullptr;
            Slider^ slider1 = nullptr;
            Button^ btnC = nullptr;

            InputDevice device = InputDevice::Gamepad;

            auto slider1FocusEngaged = std::make_shared<Event>();
            auto slider1FocusEngagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusEngaged);

            auto slider1FocusDisengaged = std::make_shared<Event>();
            auto slider1FocusDisengagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusDisengaged);

            StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"EngagementTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                fv1 = dynamic_cast<FlipView^>(rootStackPanel->FindName(L"fv1"));
                lv1 = dynamic_cast<ListView^>(rootStackPanel->FindName(L"lv1"));
                slider1 = dynamic_cast<Slider^>(rootStackPanel->FindName(L"slider1"));
                btnC = dynamic_cast<Button^>(rootStackPanel->FindName(L"btnC"));
                slider1FocusEngagedRegistration.Attach(slider1, [&]() {slider1FocusEngaged->Set(); });
                slider1FocusDisengagedRegistration.Attach(slider1, [&]() {slider1FocusDisengaged->Set(); });

                controlsVect.push_back(fv1);
                controlsVect.push_back(lv1);
                controlsVect.push_back(slider1);
                controlsVect.push_back(btnC);

                slider1->IsFocusEngagementEnabled = true;

                slider1->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(slider1->IsFocusEngagementEnabled, true);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, false);
            });
            CommonInputHelper::Accept(device);
            slider1FocusEngaged->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(slider1->IsFocusEngagementEnabled, true);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, true);
            });
            CommonInputHelper::Down(device);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    slider1 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, true);
                LOG_OUTPUT(L"EngagedAutofocusTest: slider1 still has focus, autofocus disabled when engaged");
            });
            TestServices::WindowHelper->WaitForIdle();

            CommonInputHelper::Cancel(device);
            slider1FocusDisengaged->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(slider1->IsFocusEngagementEnabled, true);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, false);
            });
            CommonInputHelper::Down(device);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    btnC /*expectedToBeFocused*/,
                    FocusState::Keyboard);
                LOG_OUTPUT(L"EngagedAutofocusTest: btnC has focus, autofocus works when not engaged");
            });
        }

        void BasicEngagementTests::EngagementWhenNotEnabled()
        {
            TestCleanupWrapper cleanup;

            Slider^ slider1 = nullptr;
            ListView^ lv1 = nullptr;

            InputDevice device = InputDevice::Gamepad;

            auto slider1FocusEngaged = std::make_shared<Event>();
            auto slider1FocusEngagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusEngaged);

            auto slider1FocusDisengaged = std::make_shared<Event>();
            auto slider1FocusDisengagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusDisengaged);

            StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"EngagementTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                lv1 = dynamic_cast<ListView^>(rootStackPanel->FindName(L"lv1"));
                slider1 = dynamic_cast<Slider^>(rootStackPanel->FindName(L"slider1"));

                slider1->IsFocusEngagementEnabled = true;
                lv1->IsFocusEngagementEnabled = true;
                slider1FocusEngagedRegistration.Attach(slider1, [&]() {slider1FocusEngaged->Set(); });
                slider1FocusDisengagedRegistration.Attach(slider1, [&]() {slider1FocusDisengaged->Set(); });

                slider1->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(slider1->IsFocusEngagementEnabled, true);
            });
            CommonInputHelper::Accept(device);
            slider1FocusEngaged->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(lv1->IsFocusEngaged, false);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, true);
                LOG_OUTPUT(L"EngagementWhenNotEnabledTest: IsFocusEngaged settable with A");
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                slider1->IsEnabled = false;
            });
            slider1FocusDisengaged->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, false);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngagementEnabled, true);
                LOG_OUTPUT(L"EngagementWhenNotEnabledTest: slider1 disengaged by changing is Enabled");
            });
        }

        void BasicEngagementTests::EngagementWhenNotTabStop()
        {
            TestCleanupWrapper cleanup;

            Slider^ slider1 = nullptr;
            ListView^ lv1 = nullptr;

            InputDevice device = InputDevice::Gamepad;

            auto slider1FocusEngaged = std::make_shared<Event>();
            auto slider1FocusEngagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusEngaged);

            StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"EngagementTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                lv1 = dynamic_cast<ListView^>(rootStackPanel->FindName(L"lv1"));
                slider1 = dynamic_cast<Slider^>(rootStackPanel->FindName(L"slider1"));

                slider1->IsFocusEngagementEnabled = true;
                lv1->IsFocusEngagementEnabled = true;
                slider1FocusEngagedRegistration.Attach(slider1, [&]() {slider1FocusEngaged->Set(); });

                slider1->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(slider1->IsFocusEngagementEnabled, true);
            });
            CommonInputHelper::Accept(device);
            slider1FocusEngaged->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(lv1->IsFocusEngaged, false);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, true);
                LOG_OUTPUT(L"EngagementWhenNotTabStopTest: IsFocusEngaged settable with A");
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                slider1->IsTabStop = false;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(FocusState::Keyboard, slider1->FocusState);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, true);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngagementEnabled, true);
                LOG_OUTPUT(L"EngagementWhenNotTabStopTest: Setting IsTabStop to false does not change Focus or Engagement immediately");
            });
        }

        void BasicEngagementTests::ParentChildEngagementWhenNotEnabled()
        {
            TestCleanupWrapper cleanup;

            ListView^ lv1 = nullptr;

            InputDevice device = InputDevice::Gamepad;

            auto lv1FocusEngaged = std::make_shared<Event>();
            auto lv1FocusEngagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusEngaged);

            auto lv1FocusDisengaged = std::make_shared<Event>();
            auto lv1FocusDisengagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusDisengaged);

            StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"EngagementTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                lv1 = dynamic_cast<ListView^>(rootStackPanel->FindName(L"lv1"));

                lv1FocusEngagedRegistration.Attach(lv1, [&]() {lv1FocusEngaged->Set(); });
                lv1FocusDisengagedRegistration.Attach(lv1, [&]() {lv1FocusDisengaged->Set(); });

                lv1->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(lv1->IsFocusEngagementEnabled, true);
            });
            CommonInputHelper::Accept(device);
            lv1FocusEngaged->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(lv1->IsFocusEngaged, true);
                lv1->IsEnabled = false;
            });
            lv1FocusDisengaged->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(lv1->IsFocusEngaged, false);
                VERIFY_ARE_EQUAL(lv1->IsFocusEngagementEnabled, true);
                LOG_OUTPUT(L"ParentChildEngagementWhenNotEnabledTest: lv1 disengaged by changing IsEnabled");
            });
        }

        void BasicEngagementTests::ListViewEngagementInteractionAndDisengagement()
        {
            TestCleanupWrapper cleanup;

            ListView^ lv1 = nullptr;
            ListViewItem^ item1 = nullptr;
            ListViewItem^ item2 = nullptr;
            std::vector<Control^> controlsVect;

            InputDevice device = InputDevice::Gamepad;

            auto lv1FocusEngaged = std::make_shared<Event>();
            auto lv1FocusEngagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusEngaged);

            auto lv1FocusDisengaged = std::make_shared<Event>();
            auto lv1FocusDisengagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusDisengaged);

            StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"EngagementTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                lv1 = dynamic_cast<ListView^>(rootStackPanel->FindName(L"lv1"));
                item1 = dynamic_cast<ListViewItem^>(rootStackPanel->FindName(L"item1"));
                item2 = dynamic_cast<ListViewItem^>(rootStackPanel->FindName(L"item2"));

                controlsVect.push_back(lv1);
                controlsVect.push_back(item1);
                controlsVect.push_back(item2);

                lv1FocusEngagedRegistration.Attach(lv1, [&]() {lv1FocusEngaged->Set(); });
                lv1FocusDisengagedRegistration.Attach(lv1, [&]() {lv1FocusDisengaged->Set(); });

                lv1->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(lv1->IsFocusEngagementEnabled, true);
                VERIFY_ARE_EQUAL(lv1->IsFocusEngaged, false);
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    lv1 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
                LOG_OUTPUT(L"ListViewEngagementInteractionAndDisengagementTest: item1 has focus");
            });
            CommonInputHelper::Accept(device);
            lv1FocusEngaged->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(lv1->IsFocusEngaged, true);
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    item1 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
                LOG_OUTPUT(L"ListViewEngagementInteractionAndDisengagementTest: item1 has focus");
            });
            CommonInputHelper::Down(device);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    item2 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
                LOG_OUTPUT(L"ListViewEngagementInteractionAndDisengagementTest: item2 has focus");
            });
            CommonInputHelper::Cancel(device);
            lv1FocusDisengaged->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(lv1->IsFocusEngaged, false);
                VERIFY_ARE_EQUAL(lv1->IsFocusEngagementEnabled, true);
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    lv1 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
                LOG_OUTPUT(L"ListViewEngagementInteractionAndDisengagementTest: ListView has focus, is disengaged");
            });
            CommonInputHelper::Accept(device);
            lv1FocusEngaged->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(lv1->IsFocusEngaged, true);
                lv1->IsFocusEngagementEnabled = false;
                VERIFY_ARE_EQUAL(lv1->IsFocusEngaged, false);

            });
        }

        void BasicEngagementTests::FlipViewEngagementInteractionAndDisengagement()
        {
            TestCleanupWrapper cleanup;

            FlipView^ fv1 = nullptr;
            FlipViewItem^ fvItem1 = nullptr;
            FlipViewItem^ fvItem2 = nullptr;
            std::vector<Control^> controlsVect;

            InputDevice device = InputDevice::Gamepad;

            auto fv1FocusEngaged = std::make_shared<Event>();
            auto fv1FocusEngagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusEngaged);

            auto fv1FocusDisengaged = std::make_shared<Event>();
            auto fv1FocusDisengagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusDisengaged);

            StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"EngagementTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                fv1 = dynamic_cast<FlipView^>(rootStackPanel->FindName(L"fv1"));
                fvItem1 = dynamic_cast<FlipViewItem^>(rootStackPanel->FindName(L"fvItem1"));
                fvItem2 = dynamic_cast<FlipViewItem^>(rootStackPanel->FindName(L"fvItem2"));

                controlsVect.push_back(fv1);
                controlsVect.push_back(fvItem1);
                controlsVect.push_back(fvItem2);

                fv1FocusEngagedRegistration.Attach(fv1, [&]() {fv1FocusEngaged->Set(); });
                fv1FocusDisengagedRegistration.Attach(fv1, [&]() {fv1FocusDisengaged->Set(); });

                fv1->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(fv1->IsFocusEngagementEnabled, true);
                VERIFY_ARE_EQUAL(fv1->IsFocusEngaged, false);
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    fv1 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
            });
            CommonInputHelper::Accept(device);
            fv1FocusEngaged->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(fv1->IsFocusEngaged, true);
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    fvItem1 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
                LOG_OUTPUT(L"FlipViewEngagementInteractionAndDisengagementTest: fvItem1 has focus");
            });
            CommonInputHelper::Right(device);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    fvItem2 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
                LOG_OUTPUT(L"FlipViewEngagementInteractionAndDisengagementTest: fvItem2 has focus");
            });
            CommonInputHelper::Cancel(device);
            fv1FocusDisengaged->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(fv1->IsFocusEngaged, false);
                VERIFY_ARE_EQUAL(fv1->IsFocusEngagementEnabled, true);
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    fv1 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
                LOG_OUTPUT(L"FlipViewEngagementInteractionAndDisengagementTest: fv1 has focus, is disengaged");
            });
        }

        void BasicEngagementTests::ListBoxEngagementInteractionAndDisengagement()
        {
            TestCleanupWrapper cleanup;

            ListBox^ lb1 = nullptr;
            ListBoxItem^ lbItem1 = nullptr;
            ListBoxItem^ lbItem2 = nullptr;
            std::vector<Control^> controlsVect;

            InputDevice device = InputDevice::Gamepad;

            auto lb1FocusEngaged = std::make_shared<Event>();
            auto lb1FocusEngagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusEngaged);

            auto lb1FocusDisengaged = std::make_shared<Event>();
            auto lb1FocusDisengagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusDisengaged);

            StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"EngagementTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                lb1 = dynamic_cast<ListBox^>(rootStackPanel->FindName(L"lb1"));
                lbItem1 = dynamic_cast<ListBoxItem^>(rootStackPanel->FindName(L"lbItem1"));
                lbItem2 = dynamic_cast<ListBoxItem^>(rootStackPanel->FindName(L"lbItem2"));

                controlsVect.push_back(lb1);
                controlsVect.push_back(lbItem1);
                controlsVect.push_back(lbItem2);

                lb1FocusEngagedRegistration.Attach(lb1, [&]() {lb1FocusEngaged->Set(); });
                lb1FocusDisengagedRegistration.Attach(lb1, [&]() {lb1FocusDisengaged->Set(); });

                lb1->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(lb1->IsFocusEngagementEnabled, true);
                VERIFY_ARE_EQUAL(lb1->IsFocusEngaged, false);
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    lb1 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
            });
            CommonInputHelper::Accept(device);
            lb1FocusEngaged->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(lb1->IsFocusEngaged, true);
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    lbItem1 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
                LOG_OUTPUT(L"ListBoxEngagementInteractionAndDisengagementTest: lbItem1 has focus");
            });
            CommonInputHelper::Down(device);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    lbItem2 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
                LOG_OUTPUT(L"ListBoxEngagementInteractionAndDisengagementTest: lbItem2 has focus");
            });
            CommonInputHelper::Cancel(device);
            lb1FocusDisengaged->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(lb1->IsFocusEngaged, false);
                VERIFY_ARE_EQUAL(lb1->IsFocusEngagementEnabled, true);
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    lb1 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
                LOG_OUTPUT(L"ListBoxEngagementInteractionAndDisengagementTest: lb1 has focus, is disengaged");
            });
        }

        void BasicEngagementTests::ComboBoxDropdownEngagement()
        {
            TestCleanupWrapper cleanup;

            ComboBox^ combobox1 = nullptr;

            InputDevice device = InputDevice::Gamepad;

            auto combobox1FocusEngaged = std::make_shared<Event>();
            auto combobox1FocusEngagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusEngaged);

            auto combobox1FocusDisengaged = std::make_shared<Event>();
            auto combobox1FocusDisengagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusDisengaged);

            StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"EngagementTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                combobox1 = dynamic_cast<ComboBox^>(rootStackPanel->FindName(L"combobox1"));
                combobox1FocusEngagedRegistration.Attach(combobox1, [&]() {combobox1FocusEngaged->Set(); });
                combobox1FocusDisengagedRegistration.Attach(combobox1, [&]() {combobox1FocusDisengaged->Set(); });

                combobox1->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(combobox1->FocusState, FocusState::Keyboard);
                VERIFY_ARE_EQUAL(combobox1->IsFocusEngagementEnabled, true);
                VERIFY_ARE_EQUAL(combobox1->IsFocusEngaged, false);
            });
            CommonInputHelper::Accept(device);
            combobox1FocusEngaged->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(combobox1->IsFocusEngaged, true);
                VERIFY_ARE_EQUAL(combobox1->IsDropDownOpen, false);
            });
            CommonInputHelper::Accept(device);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(combobox1->IsFocusEngaged, true);
                VERIFY_ARE_EQUAL(combobox1->IsDropDownOpen, true);
                LOG_OUTPUT(L"ComboBoxDropdownEngagementTest: Dropdown opened");
            });
            CommonInputHelper::Down(device);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(combobox1->IsFocusEngaged, true);
                VERIFY_ARE_EQUAL(combobox1->IsDropDownOpen, true);
            });
            CommonInputHelper::Cancel(device);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(combobox1->IsFocusEngaged, true);
                VERIFY_ARE_EQUAL(combobox1->IsDropDownOpen, false);
                LOG_OUTPUT(L"ComboBoxDropdownEngagementTest: Dropdown closed");
            });
            CommonInputHelper::Cancel(device);
            combobox1FocusDisengaged->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(combobox1->IsFocusEngaged, false);
            });
        }

        void BasicEngagementTests::UnparentedPopupAutofocusOnEngagement()
        {
            TestCleanupWrapper cleanup;

            Button^ btnA = nullptr;
            Button^ popupBtn1 = nullptr;
            Button^ popupBtn2 = nullptr;
            StackPanel^ stackPanel = nullptr;
            xaml_primitives::Popup^ popup1 = nullptr;
            std::vector<Control^> controlsVect;
            auto btnFocusEngaged = std::make_shared<Event>();
            auto btnFocusEngagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusEngaged);

            InputDevice device = InputDevice::Gamepad;

            auto btnAClickedRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);

            StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"EngagementTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                popupBtn1 = ref new Button();
                popupBtn1->Height = 100;
                popupBtn1->Width = 100;
                popupBtn1->Content = "Button1";
                popupBtn2 = ref new Button();
                popupBtn2->Height = 100;
                popupBtn2->Width = 100;
                popupBtn2->Content = "Button2";
                stackPanel = ref new StackPanel();
                popup1 = ref new xaml_primitives::Popup();
                popup1->Child = stackPanel;
                stackPanel->Children->Append(popupBtn1);
                stackPanel->Children->Append(popupBtn2);
                btnA = safe_cast<Button^>(rootStackPanel->FindName(L"btnA"));
                controlsVect.push_back(btnA);
                controlsVect.push_back(popupBtn1);
                controlsVect.push_back(popupBtn2);

                btnA->IsFocusEngagementEnabled = true;
                popup1->IsOpen = false;
                popup1->HorizontalOffset = 50;
                popup1->VerticalOffset = 300;
                btnA->Focus(FocusState::Keyboard);

                btnAClickedRegistration.Attach(btnA, ref new xaml::RoutedEventHandler([popup1](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
                {
                    popup1->IsOpen = !popup1->IsOpen;
                }));

                btnFocusEngagedRegistration.Attach(btnA, [&]() {btnFocusEngaged->Set(); });
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(btnA->IsFocusEngagementEnabled, true);
                VERIFY_ARE_EQUAL(btnA->IsFocusEngaged, false);
            });
            CommonInputHelper::Accept(device);
            btnFocusEngaged->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(btnA->IsFocusEngaged, true);
            });
            CommonInputHelper::Accept(device);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(popup1->IsOpen);
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    btnA /*expectedToBeFocused*/,
                    FocusState::Keyboard);
            });
            CommonInputHelper::Down(device);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    popupBtn1 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
            });
            CommonInputHelper::Down(device);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(popup1->IsOpen);
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    popupBtn2 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
            });
            CommonInputHelper::Down(device);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(popup1->IsOpen);
                CheckFocusStatusForAllControls(
                    &controlsVect,
                    popupBtn2 /*expectedToBeFocused*/,
                    FocusState::Keyboard);
                LOG_OUTPUT(L"UnparentedPopupAutofocusOnEngagementTest: Popup closed");
            });
        }

        void BasicEngagementTests::CanFocusOnFocusEngageableItem()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::StackPanel ^ rootPanel;

            std::vector<xaml_controls::Control^> buttons;

            RunOnUIThread([&]
            {
                rootPanel = ref new xaml_controls::StackPanel();

                for (unsigned int i = 0; i < 3; i++)
                {
                    xaml_controls::Button^ button = ref new xaml_controls::Button();
                    button->Content = "Button" + i;
                    button->Margin = xaml::ThicknessHelper::FromUniformLength(50);
                    button->IsTabStop = true;
                    buttons.push_back(button);
                    rootPanel->Children->Append(button);
                }
                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            // Wait for UI to be drawn before moving focus.
            RunOnUIThread([&]
            {
                buttons[0]->Focus(FocusState::Keyboard);
                buttons[1]->IsTabStop = false;
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::KeyboardHelper->GamepadDpadDown();
            TestServices::WindowHelper->WaitForIdle();

            // Verify the focus skipped the midde button
            RunOnUIThread([&]
            {
                CheckFocusStatusForAllControls(
                    &buttons,
                    buttons[2] /*expectedToBeFocused*/,
                    FocusState::Keyboard);
                // This should allow this button to now be focusable
                buttons[1]->IsFocusEngagementEnabled = true;
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::KeyboardHelper->GamepadDpadUp();
            TestServices::WindowHelper->WaitForIdle();

            // Verify focus should now on the middle button.
            RunOnUIThread([&]
            {
                CheckFocusStatusForAllControls(
                    &buttons,
                    buttons[1] /*expectedToBeFocused*/,
                    FocusState::Keyboard);
            });
        }

        void BasicEngagementTests::KeyDownEventIsHandledPastEngagedElement()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel = nullptr;
            StackPanel^ stackPanel = nullptr;
            Button^ btn = nullptr;
            Button^ btn2 = nullptr;

            auto gotFocusEvent = std::make_shared<Event>();
            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto keyDownEvent = std::make_shared<Event>();
            auto keyDownRegistration = CreateSafeEventRegistration(xaml_controls::Button, KeyDown);

            auto keyDownPanelRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, KeyDown);

            auto keyDownPanelEvent = std::make_shared<Event>();
            auto handledKeyDownRegistration = CreateSafeEventRegistrationForHandledEvents(xaml_controls::StackPanel, KeyDownEvent);

            auto btn2FocusEngaged = std::make_shared<Event>();
            auto btn2FocusEngagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusEngaged);

            RunOnUIThread([&]()
            {
                rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    LR"(<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
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

                btn2->IsFocusEngagementEnabled = true;

                gotFocusRegistration.Attach(btn, [&]()
                {
                    LOG_OUTPUT(L"btn gained focus");
                    gotFocusEvent->Set();
                });

                btn2FocusEngagedRegistration.Attach(btn2, [&]() { btn2FocusEngaged->Set(); });
                btn->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            gotFocusEvent->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusEvent->HasFired());


            CommonInputHelper::Down(InputDevice::Gamepad);
            TestServices::WindowHelper->WaitForIdle();
            CommonInputHelper::Accept(InputDevice::Gamepad);
            TestServices::WindowHelper->WaitForIdle();
            btn2FocusEngaged->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(btn2->IsFocusEngaged, true);

                keyDownRegistration.Attach(btn2, [&]()
                {
                    LOG_OUTPUT(L"btn2 received keydown");
                    keyDownEvent->Set();
                });

                keyDownPanelRegistration.Attach(stackPanel, [&]()
                {
                    VERIFY_FAIL(L"The stackpanel should not have received a keydown because an element within it is engaged.");
                });

                handledKeyDownRegistration.Attach(stackPanel, ref new xaml_input::KeyEventHandler([&](Platform::Object^ sender, xaml_input::KeyRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"The stackpanel received a HANDLED keydown");
                    VERIFY_IS_TRUE(args->Handled);
                    keyDownPanelEvent->Set();
                }));

                LOG_OUTPUT(L"Pressing Down");
            });
            CommonInputHelper::Down(InputDevice::Gamepad);

            TestServices::WindowHelper->WaitForIdle();
            keyDownEvent->WaitForDefault();
            keyDownPanelEvent->WaitForDefault();
            VERIFY_IS_TRUE(keyDownEvent->HasFired());
            VERIFY_IS_TRUE(keyDownPanelEvent->HasFired());
        }

        void BasicEngagementTests::NonKeyDownEventHandledCorrectlyPastEngagedElement()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel = nullptr;
            StackPanel^ stackPanel = nullptr;
            Button^ btn = nullptr;
            Button^ btn2 = nullptr;

            auto gotFocusEvent = std::make_shared<Event>();
            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto tappedEvent = std::make_shared<Event>();
            auto tappedRegistration = CreateSafeEventRegistration(xaml_controls::Button, Tapped);

            auto tappedPanelEvent = std::make_shared<Event>();
            auto tappedPanelRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Tapped);

            auto btn2FocusEngaged = std::make_shared<Event>();
            auto btn2FocusEngagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusEngaged);

            RunOnUIThread([&]()
            {
                rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    LR"(<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
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

                btn2->IsFocusEngagementEnabled = true;
                btn2FocusEngagedRegistration.Attach(btn2, [&]() { btn2FocusEngaged->Set(); });

                tappedRegistration.Attach(btn2, [&]()
                {
                    LOG_OUTPUT(L"btn2 was tapped");
                    tappedEvent->Set();
                });

                tappedPanelRegistration.Attach(stackPanel, [&]()
                {
                    LOG_OUTPUT(L"stackpanel was tapped");
                    tappedPanelEvent->Set();
                });

                gotFocusRegistration.Attach(btn, [&]()
                {
                    LOG_OUTPUT(L"btn gained focus");
                    gotFocusEvent->Set();
                });

                btn->Focus(FocusState::Keyboard);
            });

            gotFocusEvent->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusEvent->HasFired());

            CommonInputHelper::Down(InputDevice::Gamepad);
            TestServices::WindowHelper->WaitForIdle();

            CommonInputHelper::Accept(InputDevice::Gamepad);
            TestServices::WindowHelper->WaitForIdle();

            btn2FocusEngaged->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(btn2->IsFocusEngaged, true);
            });

            TestServices::InputHelper->Tap(btn2);
            tappedEvent->WaitForDefault();
            tappedPanelEvent->WaitForDefault();

            VERIFY_IS_TRUE(tappedEvent->HasFired());
            VERIFY_IS_TRUE(tappedPanelEvent->HasFired());
        }

        void BasicEngagementTests::FocusingHyperlinkDoesNotDisengage()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel = nullptr;
            ContentControl^ contentControl = nullptr;
            Button^ btn1 = nullptr;
            xaml_docs::Hyperlink^ hyperlink = nullptr;

            auto gotFocusEvent = std::make_shared<Event>();
            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            RunOnUIThread([&]()
            {
                rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    LR"(<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <ContentControl x:Name="contentControl">
                                <StackPanel>
                                    <Button x:Name="btn1" Content="Button 1"/>
                                    <TextBlock Name="TxB1">
                                        <Run>Pre-hyperlink:</Run>
                                        <Hyperlink  x:Name="hyperlink" NavigateUri="www.bing.com">Bing</Hyperlink>
                                        <Run>Post-hyperlink</Run>
                                    </TextBlock>
                                </StackPanel>
                            </ContentControl>
                        </StackPanel>)"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                btn1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"btn1"));
                hyperlink = safe_cast<xaml_docs::Hyperlink^>(rootPanel->FindName(L"hyperlink"));
                contentControl = safe_cast<ContentControl^>(rootPanel->FindName(L"contentControl"));

                contentControl->IsFocusEngagementEnabled = true;

                gotFocusRegistration.Attach(btn1, [&]()
                {
                    LOG_OUTPUT(L"btn gained focus");
                    gotFocusEvent->Set();
                });
            });

            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(contentControl, FocusState::Keyboard);
            CommonInputHelper::Accept(InputDevice::Gamepad);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(contentControl->IsFocusEngaged);
                VERIFY_IS_TRUE(gotFocusEvent->HasFired());
            });

            CommonInputHelper::Down(InputDevice::Gamepad);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(hyperlink));
                VERIFY_IS_TRUE(contentControl->IsFocusEngaged);
            });
        }

        void BasicEngagementTests::ValidateIsFocusEngagedProgrammatically()
        {
            TestCleanupWrapper cleanup;

            Slider^ slider1 = nullptr;
            StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"EngagementTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            auto slider1FocusEngaged = std::make_shared<Event>();
            auto slider1FocusEngagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusEngaged);

            auto slider1FocusDisengaged = std::make_shared<Event>();
            auto slider1FocusDisengagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusDisengaged);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                slider1 = dynamic_cast<Slider^>(rootStackPanel->FindName(L"slider1"));
                slider1->IsFocusEngagementEnabled = true;

                slider1FocusEngagedRegistration.Attach(slider1, [&]() {slider1FocusEngaged->Set(); });
                slider1FocusDisengagedRegistration.Attach(slider1, [&]() {slider1FocusDisengaged->Set(); });

                slider1->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(slider1->IsFocusEngagementEnabled, true);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, false);

                slider1->IsFocusEngaged = true;
            });
            slider1FocusEngaged->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, true);
                slider1->IsFocusEngaged = false;
            });
            slider1FocusDisengaged->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, false);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void BasicEngagementTests::ValidateExceptionsForIsFocusEngagedSetter()
        {
            TestCleanupWrapper cleanup;

            Slider^ slider1 = nullptr;
            ListView^ lv1 = nullptr;

            StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"EngagementTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            auto slider1FocusEngaged = std::make_shared<Event>();
            auto slider1FocusEngagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusEngaged);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                slider1 = dynamic_cast<Slider^>(rootStackPanel->FindName(L"slider1"));
                lv1 = dynamic_cast<ListView^>(rootStackPanel->FindName(L"lv1"));

                slider1->IsFocusEngagementEnabled = true;

                slider1FocusEngagedRegistration.Attach(slider1, [&]() {slider1FocusEngaged->Set(); });

                lv1->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(slider1->IsFocusEngagementEnabled, true);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, false);

                bool caughtInvalidOperationException = false;
                try
                {
                    slider1->IsFocusEngaged = true;
                }
                catch (Platform::Exception^)
                {
                    caughtInvalidOperationException = true;
                }
                VERIFY_IS_TRUE(caughtInvalidOperationException, L"Expected an InvalidOperationException to be thrown when setting IsFocusEngaged without setting focus first");

                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, false);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                slider1->IsFocusEngagementEnabled = false;

                slider1->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(slider1->IsFocusEngagementEnabled, false);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, false);

                bool caughtInvalidOperationException = false;
                try
                {
                    slider1->IsFocusEngaged = true;
                }
                catch (Platform::Exception^)
                {
                    caughtInvalidOperationException = true;
                }
                VERIFY_IS_TRUE(caughtInvalidOperationException, L"Expected an InvalidOperationException to be thrown when setting IsFocusEngaged without setting IsFocusEngagementEnabled first");

                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, false);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void BasicEngagementTests::ValidateEngagementIsRemovedWithUnexpectedInput()
        {
            TestCleanupWrapper cleanup;
            // Leaky test, leak detection disabled 
            TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

            Slider^ slider1 = nullptr;
            StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"EngagementTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            auto slider1FocusEngaged = std::make_shared<Event>();
            auto slider1FocusEngagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusEngaged);

            auto slider1FocusDisengaged = std::make_shared<Event>();
            auto slider1FocusDisengagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusDisengaged);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                slider1 = dynamic_cast<Slider^>(rootStackPanel->FindName(L"slider1"));
                slider1->IsFocusEngagementEnabled = true;

                slider1FocusEngagedRegistration.Attach(slider1, [&]() {slider1FocusEngaged->Set(); });
                slider1FocusDisengagedRegistration.Attach(slider1, [&]() {slider1FocusDisengaged->Set(); });

                slider1->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(slider1->IsFocusEngagementEnabled, true);
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, false);

                slider1->IsFocusEngaged = true;
            });
            slider1FocusEngaged->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, true);
            });
            TestServices::KeyboardHelper->Enter();
            slider1FocusDisengaged->WaitForDefault();
            slider1FocusDisengaged->Reset();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                slider1->IsFocusEngaged = true;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(slider1->IsFocusEngaged, true);
            });
            TestServices::InputHelper->Tap(slider1);
            slider1FocusDisengaged->WaitForDefault();
        }

        void BasicEngagementTests::ValidateIsFocusEngagedWithChildContainingFocus()
        {
            TestCleanupWrapper cleanup;

            AppBar^ appBar = nullptr;
            AppBarButton^ btn1 = nullptr;
            AppBarButton^ btn2 = nullptr;
            StackPanel^ rootPanel = nullptr;

            RunOnUIThread([&]()
            {
                rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    LR"(<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <AppBar x:Name="appBar" IsOpen="true">
                                <StackPanel>
                                    <AppBarButton x:Name="btn1" Content="Button 1"/>
                                    <AppBarButton x:Name="btn2" Content="Button 2"/>
                                </StackPanel>
                            </AppBar>
                        </StackPanel>)"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                appBar = safe_cast<xaml_controls::AppBar^>(rootPanel->FindName(L"appBar"));
                btn1 = safe_cast<xaml_controls::AppBarButton^>(rootPanel->FindName(L"btn1"));
                btn2 = safe_cast<xaml_controls::AppBarButton^>(rootPanel->FindName(L"btn2"));

                appBar->IsFocusEngagementEnabled = true;
                //Set Focus on the Descendant Control
                btn1->Focus(xaml::FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(btn1->FocusState == xaml::FocusState::Keyboard);
                VERIFY_IS_FALSE(appBar->IsFocusEngaged);

                appBar->IsFocusEngaged = true;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(appBar->IsFocusEngaged);

                //Set focus on another descandant under the same "engaged" control
                btn2->Focus(xaml::FocusState::Keyboard);
            });
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(btn1->FocusState == xaml::FocusState::Unfocused);
                VERIFY_IS_TRUE(btn2->FocusState == xaml::FocusState::Keyboard);
                VERIFY_IS_TRUE(appBar->IsFocusEngaged);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void BasicEngagementTests::ElementWithinEngagedControlFocusedAfterEngagement()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel = nullptr;
            Button^ innerButton = nullptr;
            UserControl^ userControl = nullptr;
            XamlRoot^ xamlRoot = nullptr;

            auto controlEngagedEvent = std::make_shared<Event>();
            auto controlEngagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusEngaged);

            auto gotFocusEvent = std::make_shared<Event>();
            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            RunOnUIThread([&]()
            {
                rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    LR"(<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <UserControl x:Name='userControl' IsFocusEngagementEnabled="True">
                                <StackPanel>
                                    <Button x:Name="innerButton" Content="Button"/>
                                    <Button Content="Button" />
                                    <Button Content="Button" />
                                    <Button Content="Button" />
                                </StackPanel>
                            </UserControl>
                        </StackPanel>)"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                xamlRoot = rootPanel->XamlRoot;
            });

            TestServices::WindowHelper->SetLastInputMethod(test_infra::LastInputDeviceType::GamepadOrRemote, xamlRoot);

            RunOnUIThread([&]()
            {
                innerButton = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"innerButton"));
                userControl = safe_cast<xaml_controls::UserControl^>(rootPanel->FindName(L"userControl"));

                controlEngagedRegistration.Attach(userControl, [controlEngagedEvent]()
                {
                    LOG_OUTPUT(L"UserControl has been engaged");
                    controlEngagedEvent->Set();
                });

                gotFocusRegistration.Attach(innerButton, [gotFocusEvent]()
                {
                    LOG_OUTPUT(L"InnerButton has gained focus");
                    gotFocusEvent->Set();
                });
            });

            TestServices::WindowHelper->WaitForIdle();
            gotFocusEvent->Reset();
            FocusTestHelper::EnsureFocus(userControl, FocusState::Keyboard);
            CommonInputHelper::Accept(InputDevice::Gamepad);
            controlEngagedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(userControl->IsFocusEngaged);
                VERIFY_IS_TRUE(gotFocusEvent->HasFired());
            });
        }

        void BasicEngagementTests::NonTabbableElementWithinEngagedControlSkippedAfterEngagement()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel = nullptr;
            Button^ innerButton = nullptr;
            UserControl^ userControl = nullptr;

            auto controlEngagedEvent = std::make_shared<Event>();
            auto controlEngagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusEngaged);

            auto gotFocusEvent = std::make_shared<Event>();
            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            RunOnUIThread([&]()
            {
                rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    LR"(<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <UserControl x:Name='userControl' IsFocusEngagementEnabled="True">
                                <StackPanel>
                                    <Button IsTabStop="False" Content="Button"/>
                                    <Button x:Name="innerButton" Content="Button" />
                                </StackPanel>
                            </UserControl>
                        </StackPanel>)"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            XamlRoot^ xamlRoot = nullptr;
            RunOnUIThread([&]()
            {
                xamlRoot = rootPanel->XamlRoot;
            });

            TestServices::WindowHelper->SetLastInputMethod(test_infra::LastInputDeviceType::GamepadOrRemote, xamlRoot);

            RunOnUIThread([&]()
            {
                innerButton = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"innerButton"));
                userControl = safe_cast<xaml_controls::UserControl^>(rootPanel->FindName(L"userControl"));

                controlEngagedRegistration.Attach(userControl, [controlEngagedEvent]()
                {
                    LOG_OUTPUT(L"UserControl has been engaged");
                    controlEngagedEvent->Set();
                });

                gotFocusRegistration.Attach(innerButton, [gotFocusEvent]()
                {
                    LOG_OUTPUT(L"InnerButton has gained focus");
                    gotFocusEvent->Set();
                });
            });

            TestServices::WindowHelper->WaitForIdle();
            gotFocusEvent->Reset();
            FocusTestHelper::EnsureFocus(userControl, FocusState::Keyboard);
            CommonInputHelper::Accept(InputDevice::Gamepad);
            controlEngagedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(userControl->IsFocusEngaged);
                VERIFY_IS_TRUE(gotFocusEvent->HasFired());
            });
        }

        void BasicEngagementTests::ValidateFocusStaysOnEngagedControlIfNoCandidatesAfterEngagement()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel = nullptr;
            Button^ innerButton = nullptr;
            UserControl^ userControl = nullptr;

            auto controlEngagedEvent = std::make_shared<Event>();
            auto controlEngagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusEngaged);

            auto gotFocusEvent = std::make_shared<Event>();
            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            RunOnUIThread([&]()
            {
                rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    LR"(<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <UserControl x:Name='userControl' IsFocusEngagementEnabled="True">
                                <StackPanel>
                                    <Button IsTabStop="False" x:Name="innerButton" Content="Button"/>
                                </StackPanel>
                            </UserControl>
                        </StackPanel>)"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            XamlRoot^ xamlRoot = nullptr;
            RunOnUIThread([&]()
            {
                xamlRoot = rootPanel->XamlRoot;
            });

            TestServices::WindowHelper->SetLastInputMethod(test_infra::LastInputDeviceType::GamepadOrRemote, xamlRoot);

            RunOnUIThread([&]()
            {
                innerButton = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"innerButton"));
                userControl = safe_cast<xaml_controls::UserControl^>(rootPanel->FindName(L"userControl"));

                controlEngagedRegistration.Attach(userControl, [controlEngagedEvent]()
                {
                    LOG_OUTPUT(L"UserControl has been engaged");
                    controlEngagedEvent->Set();
                });

                gotFocusRegistration.Attach(innerButton, [gotFocusEvent]()
                {
                    LOG_OUTPUT(L"InnerButton has gained focus");
                    gotFocusEvent->Set();
                });
            });

            TestServices::WindowHelper->WaitForIdle();
            gotFocusEvent->Reset();
            FocusTestHelper::EnsureFocus(userControl, FocusState::Keyboard);
            CommonInputHelper::Accept(InputDevice::Gamepad);
            controlEngagedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(userControl->IsFocusEngaged);
                VERIFY_IS_FALSE(gotFocusEvent->HasFired());
                VERIFY_IS_TRUE(FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(userControl));
            });
        }

        void BasicEngagementTests::EngagedElementWithinPopupShouldDisengageWhenLosingFocus()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel = nullptr;
            Button^ button = nullptr;
            Slider^ slider = nullptr;

            auto sliderEngagedEvent = std::make_shared<Event>();
            auto sliderEngagedRegistration = CreateSafeEventRegistration(xaml_controls::Slider, FocusEngaged);

            auto sliderDisengagedEvent = std::make_shared<Event>();
            auto sliderDisengagedRegistration = CreateSafeEventRegistration(xaml_controls::Slider, FocusDisengaged);

            RunOnUIThread([&]()
            {
                rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    LR"(<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Popup IsOpen="True">
                                <StackPanel>
                                    <Slider x:Name="slider"/>
                                    <Button x:Name="btn"/>
                                </StackPanel>
                            </Popup>
                        </StackPanel>)"));

                button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"btn"));
                slider = safe_cast<xaml_controls::Slider^>(rootPanel->FindName(L"slider"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            XamlRoot^ xamlRoot = nullptr;
            RunOnUIThread([&]()
            {
                xamlRoot = rootPanel->XamlRoot;
            });

            TestServices::WindowHelper->SetLastInputMethod(test_infra::LastInputDeviceType::GamepadOrRemote, xamlRoot);

            FocusTestHelper::EnsureFocus(slider, FocusState::Keyboard);

            RunOnUIThread([&]()
            {
                sliderEngagedRegistration.Attach(slider, [sliderEngagedEvent]()
                {
                    LOG_OUTPUT(L"Slider has been engaged");
                    sliderEngagedEvent->Set();
                });

                sliderDisengagedRegistration.Attach(slider, [sliderDisengagedEvent]()
                {
                    LOG_OUTPUT(L"Slider has been disengaged");
                    sliderDisengagedEvent->Set();
                });

                slider->IsFocusEngaged = true;
            });

            sliderEngagedEvent->WaitForDefault();
            VERIFY_IS_TRUE(sliderEngagedEvent->HasFired());
            FocusTestHelper::EnsureFocus(button, FocusState::Keyboard);

            sliderDisengagedEvent->WaitForDefault();
            VERIFY_IS_TRUE(sliderDisengagedEvent->HasFired());
        }
    }
} } } } } }
