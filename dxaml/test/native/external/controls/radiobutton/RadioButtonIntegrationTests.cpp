// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <algorithm>
#include <random>
#include "RadioButtonIntegrationTests.h"

#include <XamlTailored.h>
#include <TestCleanupWrapper.h>

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>
#include <generic\ButtonBaseTests.h>

#include <CommonInputHelper.h>
#include <ControlHelper.h>
#include <FocusTestHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace RadioButton {

    bool RadioButtonIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool RadioButtonIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool RadioButtonIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void RadioButtonIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::RadioButton>::CanInstantiate();
    }

    void RadioButtonIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::RadioButton>::CanEnterAndLeaveLiveTree();
    }

    void RadioButtonIntegrationTests::CanClickUsingTap()
    {
        Generic::ButtonBaseTests<xaml_controls::RadioButton>::CanClickUsingTap();
    }

    xaml_controls::StackPanel^ RadioButtonIntegrationTests::AddRadioButtonsToPanel(
        bool isNamedGroup,
        std::vector<xaml_controls::RadioButton^>& radioButtons,
        xaml_controls::StackPanel^ panel)
    {
        if (!panel)
        {
            panel = ref new xaml_controls::StackPanel();
            // Set VerticalAlignment to "Center" so that the RadioButtons are never rendered in the Status Bar region at the Top.
            panel->VerticalAlignment = xaml::VerticalAlignment::Center;
        }


        for (unsigned int i = 0; i < radioButtons.size(); i++)
        {
            radioButtons[i] = ref new xaml_controls::RadioButton();
            radioButtons[i]->Content = "Radio Button " + i;
        }

        // Randomly shuffle the radioButtons so that the visual order is different from initialization order.
        // Note that the "Content" indices refer to initialization order and would not necessarily be the same
        // as "Tag" indices which refer to visual order.
        auto rng = std::default_random_engine{};
        std::shuffle(radioButtons.begin(), radioButtons.end(), rng);

        for (unsigned int i = 0; i < radioButtons.size(); i++)
        {
            radioButtons[i]->Tag = "RB" + i;
            radioButtons[i]->IsChecked = false;
            if (isNamedGroup)
            {
                radioButtons[i]->GroupName = "radioGroup";
            }

            panel->Children->Append(radioButtons[i]);
        }

        return panel;
    }

    xaml_controls::StackPanel^ RadioButtonIntegrationTests::AddRadioButtonsToPanelWithCheckedEvents(
        bool isNamedGroup,
        std::vector<xaml_controls::RadioButton^> & radioButtons,
        std::vector<SafeEventRegistrationType(xaml_controls::RadioButton, Checked)>& checkedRegistrations,
        std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> radioButtonCheckedEvent,
        xaml_controls::StackPanel^ panel)
    {
        panel = AddRadioButtonsToPanel(isNamedGroup, radioButtons, panel);
        for (unsigned int i = 0; i < radioButtons.size(); i++)

        {
            auto checkedRegistration = CreateSafeEventRegistration(xaml_controls::RadioButton, Checked);
            checkedRegistration.Attach(radioButtons[i], [radioButtonCheckedEvent]() { radioButtonCheckedEvent->Set(); });
            checkedRegistrations.push_back(std::move(checkedRegistration));
        }

        return panel;
    }

    xaml_controls::StackPanel^ RadioButtonIntegrationTests::AddRadioButtonsToPanelWithFocusedHandler(
        bool isNamedGroup,
        std::vector<xaml_controls::RadioButton^> & radioButtons,
        std::vector<SafeEventRegistrationType(xaml_controls::RadioButton, GotFocus)>& focusedRegistrations,
        xaml::RoutedEventHandler^ gotFocusHandler,
        xaml_controls::StackPanel^ panel)
    {
        panel = AddRadioButtonsToPanel(isNamedGroup, radioButtons, panel);

        for (unsigned int i = 0; i < radioButtons.size(); i++)
        {
            auto focusedRegistration = CreateSafeEventRegistration(xaml_controls::RadioButton, GotFocus);
            focusedRegistration.Attach(radioButtons[i], gotFocusHandler);
            focusedRegistrations.push_back(std::move(focusedRegistration));
        }

        return panel;
    }


    void RadioButtonIntegrationTests::DoesGroupingWork()
    {
        TestCleanupWrapper cleanup;

        // In order to test various combinations of groups we create four "sets" of radio buttons
        // spread across two different container panels.
        //
        //  Set 1.   No group name (Left Panel)
        //  Set 2.   no group name (Right Panel)
        //  set 3.   GroupNameA (Split between Left and Right Panels)
        //  set 4.   GroupNameB: (Left Panel)
        std::shared_ptr<Event> radioButtonCheckedEvent = std::make_shared<Event>();
        std::vector<SafeEventRegistrationType(xaml_controls::RadioButton, Checked)> checkedRegistrations;
        std::vector<xaml_controls::RadioButton^> leftPanelNoGroupButtons(3);
        std::vector<xaml_controls::RadioButton^> rightPanelNoGroupButtons(3);
        std::vector<xaml_controls::RadioButton^> leftPanelGroupAButtons(2);
        std::vector<xaml_controls::RadioButton^> rightPanelGroupAButtons(2);
        std::vector<xaml_controls::RadioButton^> leftPanelGroupBButtons(3);

        RunOnUIThread([&]()
            {
                auto panel = ref new xaml_controls::StackPanel();
                panel->Orientation = xaml_controls::Orientation::Horizontal;
                TestServices::WindowHelper->WindowContent = panel;

                auto leftPanel = ref new xaml_controls::StackPanel();
                auto rightPanel = ref new xaml_controls::StackPanel();
                panel->Children->Append(leftPanel);
                panel->Children->Append(rightPanel);

                xaml_controls::TextBlock^ tb;
                tb = ref new xaml_controls::TextBlock();
                tb->Text = "No Group Name";
                leftPanel->Children->Append(tb);
                AddRadioButtonsToPanelWithCheckedEvents(false, leftPanelNoGroupButtons, checkedRegistrations, radioButtonCheckedEvent, leftPanel);

                tb = ref new xaml_controls::TextBlock();
                tb->Text = "No Group Name";
                leftPanel->Children->Append(tb);
                AddRadioButtonsToPanelWithCheckedEvents(false, rightPanelNoGroupButtons, checkedRegistrations, radioButtonCheckedEvent, rightPanel);

                tb = ref new xaml_controls::TextBlock();
                tb->Text = "Group A";
                leftPanel->Children->Append(tb);
                AddRadioButtonsToPanelWithCheckedEvents(false, leftPanelGroupAButtons, checkedRegistrations, radioButtonCheckedEvent, leftPanel);
                leftPanelGroupAButtons[0]->GroupName = "GroupA";
                leftPanelGroupAButtons[1]->GroupName = "GroupA";

                tb = ref new xaml_controls::TextBlock();
                tb->Text = "Group A";
                leftPanel->Children->Append(tb);
                AddRadioButtonsToPanelWithCheckedEvents(false, rightPanelGroupAButtons, checkedRegistrations, radioButtonCheckedEvent, rightPanel);
                rightPanelGroupAButtons[0]->GroupName = "GroupA";
                rightPanelGroupAButtons[1]->GroupName = "GroupA";

                tb = ref new xaml_controls::TextBlock();
                tb->Text = "Group B";
                leftPanel->Children->Append(tb);
                AddRadioButtonsToPanelWithCheckedEvents(false, leftPanelGroupBButtons, checkedRegistrations, radioButtonCheckedEvent, leftPanel);
                leftPanelGroupBButtons[0]->GroupName = "GroupB";
                leftPanelGroupBButtons[1]->GroupName = "GroupB";
                leftPanelGroupBButtons[2]->GroupName = "GroupB";
        });

        LOG_OUTPUT(L"Seting intital states of the groups");
        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->Tap(leftPanelNoGroupButtons[0]);
        radioButtonCheckedEvent->WaitForDefault();
        TestServices::InputHelper->Tap(rightPanelNoGroupButtons[0]);
        radioButtonCheckedEvent->WaitForDefault();
        TestServices::InputHelper->Tap(leftPanelGroupAButtons[0]);
        radioButtonCheckedEvent->WaitForDefault();
        TestServices::InputHelper->Tap(leftPanelGroupBButtons[0]);
        radioButtonCheckedEvent->WaitForDefault();

        LOG_OUTPUT(L"Validating Initial State");
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(leftPanelNoGroupButtons[0]->IsChecked->Value);
            VERIFY_IS_FALSE(leftPanelNoGroupButtons[1]->IsChecked->Value);
            VERIFY_IS_FALSE(leftPanelNoGroupButtons[2]->IsChecked->Value);

            VERIFY_IS_TRUE(rightPanelNoGroupButtons[0]->IsChecked->Value);
            VERIFY_IS_FALSE(rightPanelNoGroupButtons[1]->IsChecked->Value);
            VERIFY_IS_FALSE(rightPanelNoGroupButtons[2]->IsChecked->Value);

            VERIFY_IS_TRUE(leftPanelGroupAButtons[0]->IsChecked->Value);
            VERIFY_IS_FALSE(leftPanelGroupAButtons[1]->IsChecked->Value);
            VERIFY_IS_FALSE(rightPanelGroupAButtons[0]->IsChecked->Value);
            VERIFY_IS_FALSE(rightPanelGroupAButtons[1]->IsChecked->Value);
            
            VERIFY_IS_TRUE(leftPanelGroupBButtons[0]->IsChecked->Value);
            VERIFY_IS_FALSE(leftPanelGroupBButtons[1]->IsChecked->Value);
            VERIFY_IS_FALSE(leftPanelGroupBButtons[2]->IsChecked->Value);
            });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Validating First Group with no Group Name selecting second button");
        TestServices::InputHelper->Tap(leftPanelNoGroupButtons[1]);
        radioButtonCheckedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(leftPanelNoGroupButtons[0]->IsChecked->Value);
            VERIFY_IS_TRUE(leftPanelNoGroupButtons[1]->IsChecked->Value);
            VERIFY_IS_FALSE(leftPanelNoGroupButtons[2]->IsChecked->Value);

            // We only need to check that the other groups haven't changed their selected button
            VERIFY_IS_TRUE(rightPanelNoGroupButtons[0]->IsChecked->Value);
            VERIFY_IS_TRUE(leftPanelGroupAButtons[0]->IsChecked->Value);
            VERIFY_IS_TRUE(leftPanelGroupBButtons[0]->IsChecked->Value);
            });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Validating First Group with no Group Name selecting third button");
        TestServices::InputHelper->Tap(leftPanelNoGroupButtons[2]);
        radioButtonCheckedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(leftPanelNoGroupButtons[0]->IsChecked->Value);
            VERIFY_IS_FALSE(leftPanelNoGroupButtons[1]->IsChecked->Value);
            VERIFY_IS_TRUE(leftPanelNoGroupButtons[2]->IsChecked->Value);

            // We only need to check that the other groups haven't changed their selected button
            VERIFY_IS_TRUE(rightPanelNoGroupButtons[0]->IsChecked->Value);
            VERIFY_IS_TRUE(leftPanelGroupAButtons[0]->IsChecked->Value);
            VERIFY_IS_TRUE(leftPanelGroupBButtons[0]->IsChecked->Value);
            });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Validating Split Group (GroupA) selecting second button on left");

        TestServices::InputHelper->Tap(leftPanelGroupAButtons[1]);
        radioButtonCheckedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
            {
                VERIFY_IS_FALSE(leftPanelGroupAButtons[0]->IsChecked->Value);
                VERIFY_IS_TRUE(leftPanelGroupAButtons[1]->IsChecked->Value);
                VERIFY_IS_FALSE(rightPanelGroupAButtons[0]->IsChecked->Value);
                VERIFY_IS_FALSE(rightPanelGroupAButtons[1]->IsChecked->Value);

                // We only need to check that the other groups haven't changed their selected button
                VERIFY_IS_TRUE(leftPanelNoGroupButtons[2]->IsChecked->Value);
                VERIFY_IS_TRUE(rightPanelNoGroupButtons[0]->IsChecked->Value);
                VERIFY_IS_TRUE(leftPanelGroupBButtons[0]->IsChecked->Value);
            });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Validating Split Group (GroupA) selecting first button on right");

        TestServices::InputHelper->Tap(rightPanelGroupAButtons[0]);
        radioButtonCheckedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
            {
                VERIFY_IS_FALSE(leftPanelGroupAButtons[0]->IsChecked->Value);
                VERIFY_IS_FALSE(leftPanelGroupAButtons[1]->IsChecked->Value);
                VERIFY_IS_TRUE(rightPanelGroupAButtons[0]->IsChecked->Value);
                VERIFY_IS_FALSE(rightPanelGroupAButtons[1]->IsChecked->Value);

                // We only need to check that the other groups haven't changed their selected button
                VERIFY_IS_TRUE(leftPanelNoGroupButtons[2]->IsChecked->Value);
                VERIFY_IS_TRUE(rightPanelNoGroupButtons[0]->IsChecked->Value);
                VERIFY_IS_TRUE(leftPanelGroupBButtons[0]->IsChecked->Value);
            });
        TestServices::WindowHelper->WaitForIdle();
    }

    void RadioButtonIntegrationTests::CreateMultipleChecked()
    {
        TestCleanupWrapper cleanup;

        std::vector<xaml_controls::RadioButton^> radioButtonsWithNoGroup(3);
        std::vector<xaml_controls::RadioButton^> radioButtonsWithGroup(3);

        xaml_controls::StackPanel^ panel;

        RunOnUIThread([&]()
            {
                panel = ref new xaml_controls::StackPanel();
                TestServices::WindowHelper->WindowContent = panel;

                //  Create the buttons independently of the tree so that that will have
                //  retain their checked values.
                for (unsigned int i = 0; i < radioButtonsWithNoGroup.size(); i++)
                {
                    radioButtonsWithNoGroup[i] = ref new xaml_controls::RadioButton();
                    radioButtonsWithNoGroup[i]->Content = "Radio Button " + i;
                    radioButtonsWithNoGroup[i]->IsChecked = true;
                }
                for (unsigned int i = 0; i < radioButtonsWithGroup.size(); i++)
                {
                    radioButtonsWithGroup[i] = ref new xaml_controls::RadioButton();
                    radioButtonsWithGroup[i]->Content = "Grouped Radio Button " + i;
                    radioButtonsWithGroup[i]->GroupName = "ButtonGroup";
                    radioButtonsWithGroup[i]->IsChecked = true;
                }
            });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Verifying unparented radio buttons with no group name are checked");
                VERIFY_IS_TRUE(radioButtonsWithNoGroup[0]->IsChecked->Value);
                VERIFY_IS_TRUE(radioButtonsWithNoGroup[1]->IsChecked->Value);
                VERIFY_IS_TRUE(radioButtonsWithNoGroup[2]->IsChecked->Value);
                LOG_OUTPUT(L"Verifying unparented radio buttons with with group name are checked");
                VERIFY_IS_TRUE(radioButtonsWithGroup[0]->IsChecked->Value);
                VERIFY_IS_TRUE(radioButtonsWithGroup[1]->IsChecked->Value);
                VERIFY_IS_TRUE(radioButtonsWithGroup[2]->IsChecked->Value);

                LOG_OUTPUT(L"Adding radio buttons into tree");
                for (unsigned int i = 0; i < radioButtonsWithNoGroup.size(); i++)
                {
                    panel->Children->Append(radioButtonsWithNoGroup[i]);
                }
                for (unsigned int i = 0; i < radioButtonsWithGroup.size(); i++)
                {
                    panel->Children->Append(radioButtonsWithGroup[i]);
                }
            });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Verifying radio buttons with no group name only have once checked button");
                VERIFY_IS_FALSE(radioButtonsWithNoGroup[0]->IsChecked->Value);
                VERIFY_IS_FALSE(radioButtonsWithNoGroup[1]->IsChecked->Value);
                VERIFY_IS_TRUE(radioButtonsWithNoGroup[2]->IsChecked->Value);
                LOG_OUTPUT(L"Verifying radio buttons with with group name only have once checked button");
                VERIFY_IS_FALSE(radioButtonsWithGroup[0]->IsChecked->Value);
                VERIFY_IS_FALSE(radioButtonsWithGroup[1]->IsChecked->Value);
                VERIFY_IS_TRUE(radioButtonsWithGroup[2]->IsChecked->Value);
            });
        TestServices::WindowHelper->WaitForIdle();
    }

    void RadioButtonIntegrationTests::CanChangeGroupName()
    {
        TestCleanupWrapper cleanup;

        const int radioButtonCount = 3;

        std::shared_ptr<Event> radioButtonCheckedEvent = std::make_shared<Event>();
        std::vector<SafeEventRegistrationType(xaml_controls::RadioButton, Checked)> checkedRegistrations;
        std::vector<xaml_controls::RadioButton^> radioButtons(radioButtonCount);

        RunOnUIThread([&]()
        {
            auto panel = AddRadioButtonsToPanelWithCheckedEvents(true, radioButtons, checkedRegistrations, radioButtonCheckedEvent);
            TestServices::WindowHelper->WindowContent = panel;
        });

        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->Tap(radioButtons[0]);
        radioButtonCheckedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(radioButtons[0]->IsChecked->Value);
            VERIFY_IS_FALSE(radioButtons[1]->IsChecked->Value);
            VERIFY_IS_FALSE(radioButtons[2]->IsChecked->Value);
            radioButtons[1]->GroupName = "radioGroup2";
        });

        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->Tap(radioButtons[1]);
        radioButtonCheckedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(radioButtons[0]->IsChecked->Value);
            VERIFY_IS_TRUE(radioButtons[1]->IsChecked->Value);
            VERIFY_IS_FALSE(radioButtons[2]->IsChecked->Value);
        });

        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->Tap(radioButtons[2]);
        radioButtonCheckedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(radioButtons[0]->IsChecked->Value);
            VERIFY_IS_TRUE(radioButtons[1]->IsChecked->Value);
            VERIFY_IS_TRUE(radioButtons[2]->IsChecked->Value);
            radioButtons[1]->IsChecked = false;
            radioButtons[1]->GroupName = "radioGroup";
        });


        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->Tap(radioButtons[0]);
        radioButtonCheckedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(radioButtons[0]->IsChecked->Value);
            VERIFY_IS_FALSE(radioButtons[1]->IsChecked->Value);
            VERIFY_IS_FALSE(radioButtons[2]->IsChecked->Value);
        });

    }

    void RadioButtonIntegrationTests::CanSetIntermediateState()
    {
        TestCleanupWrapper cleanup;

        const int radioButtonCount = 3;
        std::vector<xaml_controls::RadioButton^> radioButtons(radioButtonCount);

        std::shared_ptr<Event> radioButtonCheckedEvent = std::make_shared<Event>();
        std::shared_ptr<Event> radioButtonIndeterminateEvent = std::make_shared<Event>();

        std::vector<SafeEventRegistrationType(xaml_controls::RadioButton, Checked)> checkedRegistrations;
        std::vector<SafeEventRegistrationType(xaml_controls::RadioButton, Indeterminate)> indeterminateRegistrations;

        RunOnUIThread([&]()
        {
            auto panel = ref new xaml_controls::StackPanel();
            // Set VerticalAlignment to "Center" so that the RadioButtons are never rendered in the Status Bar region at the Top.
            panel->VerticalAlignment = xaml::VerticalAlignment::Center;

            for (int i = 0; i < radioButtonCount; i++)
            {
                radioButtons[i] = ref new xaml_controls::RadioButton();
                radioButtons[i]->Content = "Radio Button " + i;
                radioButtons[i]->IsThreeState = true;
                radioButtons[i]->IsChecked = false;

                auto checkedRegistration = CreateSafeEventRegistration(xaml_controls::RadioButton, Checked);
                auto indeterminateRegistration = CreateSafeEventRegistration(xaml_controls::RadioButton, Indeterminate);

                checkedRegistration.Attach(radioButtons[i], [&]() {radioButtonCheckedEvent->Set(); });

                indeterminateRegistration.Attach(radioButtons[i], [&]() {radioButtonIndeterminateEvent->Set(); });

                checkedRegistrations.push_back(std::move(checkedRegistration));
                indeterminateRegistrations.push_back(std::move(indeterminateRegistration));

                panel->Children->Append(radioButtons[i]);
            }

            TestServices::WindowHelper->WindowContent = panel;

            // Test false-> null transition
            radioButtons[0]->IsChecked = nullptr;
        });

        TestServices::WindowHelper->WaitForIdle();
        radioButtonIndeterminateEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_NULL(radioButtons[0]->IsChecked);
            VERIFY_IS_FALSE(radioButtons[1]->IsChecked->Value);
            VERIFY_IS_FALSE(radioButtons[2]->IsChecked->Value);
        });

        // Test null->true transition
        TestServices::InputHelper->Tap(radioButtons[0]);
        radioButtonCheckedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(radioButtons[0]->IsChecked->Value);
            VERIFY_IS_FALSE(radioButtons[1]->IsChecked->Value);
            VERIFY_IS_FALSE(radioButtons[2]->IsChecked->Value);

            // Test true -> null transition
            radioButtons[0]->IsChecked = nullptr;
        });

        radioButtonIndeterminateEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_NULL(radioButtons[0]->IsChecked);
            VERIFY_IS_FALSE(radioButtons[1]->IsChecked->Value);
            VERIFY_IS_FALSE(radioButtons[2]->IsChecked->Value);
        });

        TestServices::WindowHelper->WaitForIdle();
        // Test null->false transition
        TestServices::InputHelper->Tap(radioButtons[1]);
        radioButtonCheckedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(radioButtons[0]->IsChecked->Value);
            VERIFY_IS_TRUE(radioButtons[1]->IsChecked->Value);
            VERIFY_IS_FALSE(radioButtons[2]->IsChecked->Value);
        });
    }

    void RadioButtonIntegrationTests::ValidateUIElementTree()
    {
        ControlHelper::ValidateUIElementTree(
            wf::Size(400, 600),
            1.f,
            []()
            {
                xaml_controls::RadioButton^ restUnselectedRadioButton = nullptr;
                xaml_controls::RadioButton^ restSelectedRadioButton = nullptr;
                xaml_controls::RadioButton^ hoverSelectedRadioButton = nullptr;
                xaml_controls::RadioButton^ hoverUnselectedRadioButton = nullptr;
                xaml_controls::RadioButton^ disabledSelectedRadioButton = nullptr;
                xaml_controls::RadioButton^ disabledUnselectedRadioButton = nullptr;
                xaml_controls::RadioButton^ pressedRadioButton = nullptr;

                xaml_controls::RadioButton^ focusedRestUnselectedRadioButton = nullptr;
                xaml_controls::RadioButton^ focusedRestSelectedRadioButton = nullptr;
                xaml_controls::RadioButton^ focusedHoverSelectedRadioButton = nullptr;
                xaml_controls::RadioButton^ focusedHoverUnselectedRadioButton = nullptr;
                xaml_controls::RadioButton^ focusedPressedRadioButton = nullptr;

                xaml_controls::StackPanel^ rootPanel = nullptr;

                RunOnUIThread([&]()
                {
                    rootPanel = ref new xaml_controls::StackPanel();

                    restUnselectedRadioButton = ref new xaml_controls::RadioButton();
                    restUnselectedRadioButton->Content = "Rest Unselected Radio Button";
                    restUnselectedRadioButton->GroupName = "restUnselectedRadioButtonGroup";
                    rootPanel->Children->Append(restUnselectedRadioButton);

                    restSelectedRadioButton = ref new xaml_controls::RadioButton();
                    restSelectedRadioButton->Content = "Rest Selected Radio Button";
                    restSelectedRadioButton->GroupName = "restSelectedRadioButtonGroup";
                    restSelectedRadioButton->IsChecked = true;
                    rootPanel->Children->Append(restSelectedRadioButton);

                    hoverUnselectedRadioButton = ref new xaml_controls::RadioButton();
                    hoverUnselectedRadioButton->Content = "Hover Unselected Radio Button";
                    hoverUnselectedRadioButton->GroupName = "hoverUnselectedRadioButtonGroup";
                    rootPanel->Children->Append(hoverUnselectedRadioButton);

                    hoverSelectedRadioButton = ref new xaml_controls::RadioButton();
                    hoverSelectedRadioButton->Content = "Hover Selected Radio Button";
                    hoverSelectedRadioButton->GroupName = "hoverSelectedRadioButtonGroup";
                    hoverSelectedRadioButton->IsChecked = true;
                    rootPanel->Children->Append(hoverSelectedRadioButton);

                    disabledUnselectedRadioButton = ref new xaml_controls::RadioButton();
                    disabledUnselectedRadioButton->Content = "Disabled Unselected Radio Button";
                    disabledUnselectedRadioButton->GroupName = "disabledUnselectedRadioButtonGroup";
                    disabledUnselectedRadioButton->IsEnabled = false;
                    rootPanel->Children->Append(disabledUnselectedRadioButton);

                    disabledSelectedRadioButton = ref new xaml_controls::RadioButton();
                    disabledSelectedRadioButton->Content = "Disabled Selected Radio Button";
                    disabledSelectedRadioButton->GroupName = "disabledSelectedRadioButtonGroup";
                    disabledSelectedRadioButton->IsChecked = true;
                    disabledSelectedRadioButton->IsEnabled = false;
                    rootPanel->Children->Append(disabledSelectedRadioButton);

                    pressedRadioButton = ref new xaml_controls::RadioButton();
                    pressedRadioButton->Content = "Pressed Radio Button";
                    pressedRadioButton->GroupName = "pressedRadioButtonGroup";
                    pressedRadioButton->IsChecked = true;
                    rootPanel->Children->Append(pressedRadioButton);

                    focusedRestUnselectedRadioButton = ref new xaml_controls::RadioButton();
                    focusedRestUnselectedRadioButton->Content = "Keyboard Focused Rest Unselected Radio Button";
                    focusedRestUnselectedRadioButton->GroupName = "focusRestUnselectedRadioButtonGroup";
                    rootPanel->Children->Append(focusedRestUnselectedRadioButton);

                    focusedRestSelectedRadioButton = ref new xaml_controls::RadioButton();
                    focusedRestSelectedRadioButton->Content = "Keyboard Focused Rest Selected Radio Button";
                    focusedRestSelectedRadioButton->GroupName = "focusRestSelectedRadioButtonGroup";
                    focusedRestSelectedRadioButton->IsChecked = true;
                    rootPanel->Children->Append(focusedRestSelectedRadioButton);

                    focusedHoverUnselectedRadioButton = ref new xaml_controls::RadioButton();
                    focusedHoverUnselectedRadioButton->Content = "Keyboard Focused Hover Unselected Radio Button";
                    focusedHoverUnselectedRadioButton->GroupName = "focusedHoverUnselectedRadioButtonGroup";
                    rootPanel->Children->Append(focusedHoverUnselectedRadioButton);

                    focusedHoverSelectedRadioButton = ref new xaml_controls::RadioButton();
                    focusedHoverSelectedRadioButton->Content = "Keyboard Focused Hover Selected Radio Button";
                    focusedHoverSelectedRadioButton->GroupName = "focusedHoverSelectedRadioButtonGroup";
                    focusedHoverSelectedRadioButton->IsChecked = true;
                    rootPanel->Children->Append(focusedHoverSelectedRadioButton);

                    focusedPressedRadioButton = ref new xaml_controls::RadioButton();
                    focusedPressedRadioButton->Content = "Keyboard Focused Pressed Radio Button";
                    focusedPressedRadioButton->GroupName = "focusedPressedRadioButtonGroup";
                    focusedPressedRadioButton->IsChecked = true;
                    rootPanel->Children->Append(focusedPressedRadioButton);

                    TestServices::WindowHelper->WindowContent = rootPanel;
                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    VisualStateManager::GoToState(hoverUnselectedRadioButton, "PointerOver", false);
                    VisualStateManager::GoToState(hoverSelectedRadioButton, "PointerOver", false);
                    VisualStateManager::GoToState(pressedRadioButton, "Pressed", false);
                    VisualStateManager::GoToState(focusedPressedRadioButton, "Pressed", false);
                    VisualStateManager::GoToState(focusedRestUnselectedRadioButton, "Focused", false);
                    VisualStateManager::GoToState(focusedRestSelectedRadioButton, "Focused", false);
                    VisualStateManager::GoToState(focusedPressedRadioButton, "Focused", false);

                    VisualStateManager::GoToState(focusedHoverSelectedRadioButton, "PointerOver", false);
                    VisualStateManager::GoToState(focusedHoverSelectedRadioButton, "Focused", false);

                    VisualStateManager::GoToState(focusedHoverUnselectedRadioButton, "PointerOver", false);
                    VisualStateManager::GoToState(focusedHoverUnselectedRadioButton, "Focused", false);
                });
                TestServices::WindowHelper->WaitForIdle();

                return rootPanel;
            });
    }

    void RadioButtonIntegrationTests::ValidateFootprint()
    {
        TestCleanupWrapper cleanup;

        double const expectedRadioButtonWidth = 120;
        double const expectedRadioButtonWidth_WithLargeContent = 200 + 28;

        double const expectedRadioButtonHeight = 32;
        double const expectedRadioButtonHeight_WithLargeContent = 200 + 6;

        xaml_controls::RadioButton^ radioButton;
        xaml_controls::RadioButton^ radioButtonWithTextContent;
        xaml_controls::RadioButton^ radioButtonWithLargeContent;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <RadioButton x:Name="radioButton" />
                        <RadioButton x:Name="radioButtonWithTextContent" Content="RadioButton" />
                        <RadioButton x:Name="radioButtonWithLargeContent" >
                            <Rectangle Height="200" Width="200" Fill="Red" />
                        </RadioButton>
                    </StackPanel>)"));

            radioButton = safe_cast<xaml_controls::RadioButton^>(rootPanel->FindName(L"radioButton"));
            radioButtonWithTextContent = safe_cast<xaml_controls::RadioButton^>(rootPanel->FindName(L"radioButtonWithTextContent"));
            radioButtonWithLargeContent = safe_cast<xaml_controls::RadioButton^>(rootPanel->FindName(L"radioButtonWithLargeContent"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            // Verify Footprint of RadioButton:
            VERIFY_ARE_EQUAL(expectedRadioButtonWidth, radioButton->ActualWidth);
            VERIFY_ARE_EQUAL(expectedRadioButtonHeight, radioButton->ActualHeight);

            // Verify Footprint of RadioButton with text content:
            VERIFY_ARE_EQUAL(expectedRadioButtonWidth, radioButtonWithTextContent->ActualWidth);
            VERIFY_ARE_EQUAL(expectedRadioButtonHeight, radioButtonWithTextContent->ActualHeight);

            // Verify Footprint of RadioButton with large content:
            VERIFY_ARE_EQUAL(expectedRadioButtonWidth_WithLargeContent, radioButtonWithLargeContent->ActualWidth);
            VERIFY_ARE_EQUAL(expectedRadioButtonHeight_WithLargeContent, radioButtonWithLargeContent->ActualHeight);
        });
    }

    void RadioButtonIntegrationTests::ValidateFocusDoesNotShiftWhenFocusedRadioButtonGroupNameIsChanged()
    {
        PerformValidateFocusShift(/* shouldRenameGroup */ true, /* shouldRemove */ false, /* focusShifts */ false);
    }

    void RadioButtonIntegrationTests::ValidateFocusShiftsWhenFocusedRadioButtonIsRemoved()
    {
        PerformValidateFocusShift(/* shouldRenameGroup */ false, /* shouldRemove */ true, /* focusShifts */ true);
    }

    void RadioButtonIntegrationTests::PerformValidateFocusShift(bool shouldRenameGroup, bool shouldRemove, bool focusShifts)
    {
        TestCleanupWrapper cleanup;

        const int radioButtonCount = 5;

        std::vector<SafeEventRegistrationType(xaml_controls::RadioButton, GotFocus)> focusedRegistrations;
        std::vector<xaml_controls::RadioButton^> radioButtons(radioButtonCount);
        xaml::RoutedEventHandler^ gotFocusHandler = nullptr;

        xaml_controls::Panel^ panel = nullptr;
        xaml_controls::Button^ button = nullptr;

        // The focus starts from the fourth RadioButton in the group and when that RadioButton is removed,
        // focus shifts to the first RadioButton in the group.
        Platform::String^ expectedFocusSequence = "[RB3]";
        Platform::String^ focusSequence = "";

        RunOnUIThread([&]()
        {
            gotFocusHandler = ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
            {
                focusSequence += "[" + safe_cast<xaml::FrameworkElement^>(sender)->Tag + "]";
            });

            panel = AddRadioButtonsToPanelWithFocusedHandler(true, radioButtons, focusedRegistrations, gotFocusHandler);

            // Add a button to be the FirstFocusableElement (FFE) on the page and disable the first RadioButton to make sure that when
            // a RadioButton in the group is removed or has its group name changed, focus does not jump to the FFE on the page or to
            // the first RadioButton in the group but to the first focusable RadioButton (i.e. second Radio Button) in the group.
            button = ref new xaml_controls::Button();
            button->Content = "Button";
            button->Tag = "Button";
            panel->Children->InsertAt(0, button);

            radioButtons[0]->IsEnabled = false;

            TestServices::WindowHelper->WindowContent = panel;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Ensure initial focus:
        ControlHelper::EnsureFocused(button);

        RunOnUIThread([&]()
        {
            // Start by focusing the fourth RadioButton in the group.
            radioButtons[3]->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Depending on the boolean flags passed, rename GroupName for this RadioButton or remove it from the panel.
            if (shouldRenameGroup)
            {
                radioButtons[3]->GroupName = "radioButtonGroup2";
            }
            if (shouldRemove)
            {
                panel->Children->RemoveAt(4);
            }
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(focusShifts ? xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(button) : xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(radioButtons[3]));
        });

        LOG_OUTPUT(L"Expected focus sequence: %s", expectedFocusSequence->Data());
        LOG_OUTPUT(L"Actual focus sequence: %s", focusSequence->Data());
        VERIFY_ARE_EQUAL(focusSequence, expectedFocusSequence);
    }

    void RadioButtonIntegrationTests::ValidateTraverseDefaultRadioButtonGroupByGamepad()
    {
        PerformValidateTraverseRadioButtonGroup(/* isNamedGroup */ false, InputDevice::Gamepad, /* moveForwardFirst */ true, /* useLeftRightkeys */ false);
        PerformValidateTraverseRadioButtonGroup(/* isNamedGroup */ false, InputDevice::Gamepad, /* moveForwardFirst */ false, /* useLeftRightkeys */ false);
    }

    void RadioButtonIntegrationTests::ValidateTraverseDefaultRadioButtonGroupByKeyboard()
    {
        PerformValidateTraverseRadioButtonGroup(/* isNamedGroup */ false, InputDevice::Keyboard, /* moveForwardFirst */ true, /* useLeftRightkeys */ false);
        PerformValidateTraverseRadioButtonGroup(/* isNamedGroup */ false, InputDevice::Keyboard, /* moveForwardFirst */ false, /* useLeftRightkeys */ false);

        // For Keyboard, we have the option of traversing the RadioButton "Group" using Left/Right keys.
        PerformValidateTraverseRadioButtonGroup(/* isNamedGroup */ false, InputDevice::Keyboard, /* moveForwardFirst */ true, /* useLeftRightkeys */ true);
        PerformValidateTraverseRadioButtonGroup(/* isNamedGroup */ false, InputDevice::Keyboard, /* moveForwardFirst */ false, /* useLeftRightkeys */ true);
    }

    void RadioButtonIntegrationTests::ValidateTraverseRadioButtonGroupByGamepad()
    {
        PerformValidateTraverseRadioButtonGroup(/* isNamedGroup */ true, InputDevice::Gamepad, /* moveForwardFirst */ true, /* useLeftRightkeys */ false);
        PerformValidateTraverseRadioButtonGroup(/* isNamedGroup */ true, InputDevice::Gamepad, /* moveForwardFirst */ false, /* useLeftRightkeys */ false);
    }

    void RadioButtonIntegrationTests::ValidateTraverseRadioButtonGroupByKeyboard()
    {
        PerformValidateTraverseRadioButtonGroup(/* isNamedGroup */ true, InputDevice::Keyboard, /* moveForwardFirst */ true, /* useLeftRightkeys */ false);
        PerformValidateTraverseRadioButtonGroup(/* isNamedGroup */ true, InputDevice::Keyboard, /* moveForwardFirst */ false, /* useLeftRightkeys */ false);

        // For Keyboard, we have the option of traversing the RadioButton "Group" using Left/Right keys.
        PerformValidateTraverseRadioButtonGroup(/* isNamedGroup */ true, InputDevice::Keyboard, /* moveForwardFirst */ true, /* useLeftRightkeys */ true);
        PerformValidateTraverseRadioButtonGroup(/* isNamedGroup */ true, InputDevice::Keyboard, /* moveForwardFirst */ false, /* useLeftRightkeys */ true);
    }

    void RadioButtonIntegrationTests::PerformValidateTraverseRadioButtonGroup(bool isNamedGroup, InputDevice inputDevice, bool moveForwardFirst, bool useLeftRightkeys)
    {
        TestCleanupWrapper cleanup;

        const int radioButtonCount = 7;

        std::vector<SafeEventRegistrationType(xaml_controls::RadioButton, GotFocus)> focusedRegistrations;
        std::vector<xaml_controls::RadioButton^> radioButtons(radioButtonCount);
        xaml::RoutedEventHandler^ gotFocusHandler = nullptr;

        // There is no looping when we traverse the RadioButton "Group". For a group of size n, we keep going in one direction for n+1 times
        // and then we go in the opposite direction for n+1 times, but the expectedFocusSequence includes tags from fewer then n controls.
        Platform::String^ expectedFocusSequence = "";
        switch (inputDevice)
        {
            // For Gamepad, we don't care about RadioButton "groups" so we go through the number of focusable (not disabled) Controls which is 6.
            case InputDevice::Gamepad:
                expectedFocusSequence = moveForwardFirst ? L"[RB0][RB2][RB3][RB4][RB5][RB6][RB5][RB4][RB3][RB2][RB0]" : L"[RB6][RB5][RB4][RB3][RB2][RB0][RB2][RB3][RB4][RB5][RB6]";
                break;
            // For Keyboard, we go through the number of focusable (not disabled) RadioButtons in the RadioButton "Group".
            // If the end of the "Group" is reached, we loop around.
            case InputDevice::Keyboard:
                expectedFocusSequence = moveForwardFirst ?
                    L"[RB0][RB2][RB3][RB6][RB0][RB2][RB3][RB6][RB0][RB6][RB3][RB2][RB0][RB6][RB3][RB2][RB0]" :
                    L"[RB6][RB3][RB2][RB0][RB6][RB3][RB2][RB0][RB6][RB0][RB2][RB3][RB6][RB0][RB2][RB3][RB6]";
                break;
        }
        Platform::String^ focusSequence = "";

        RunOnUIThread([&]()
        {
            gotFocusHandler = ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
            {
                Platform::String^ focusStr = "[" + safe_cast<xaml::FrameworkElement^>(sender)->Tag + "]";
                LOG_OUTPUT(L"GotFocus: %s", focusStr->Data());
                focusSequence += focusStr;
            });

            auto panel = AddRadioButtonsToPanelWithFocusedHandler(isNamedGroup, radioButtons, focusedRegistrations, gotFocusHandler);

            // Disable a RadioButton and check a RadioButton to get a good mix of cases.
            radioButtons[1]->IsEnabled = false;
            radioButtons[2]->IsChecked = true;

            // Change the GroupName for two of the RadioButtons from the middle of the "Group" so that these are excluded from the traversal.
            radioButtons[4]->GroupName = "radioButtonGroup2";
            radioButtons[5]->GroupName = "radioButtonGroup2";

            TestServices::WindowHelper->WindowContent = panel;

        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Programmatically attempt to move focus to the RadioButton 'Group', if it is not already focused.");
            if (!radioButtons[0]->Equals(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)))
            {
                auto options = ref new xaml_input::FindNextElementOptions();
                options->SearchRoot = TestServices::WindowHelper->WindowContent;
                bool resultTryMoveFocus = xaml_input::FocusManager::TryMoveFocus(moveForwardFirst ? xaml_input::FocusNavigationDirection::Next : xaml_input::FocusNavigationDirection::Previous, options);
                VERIFY_ARE_EQUAL(resultTryMoveFocus, true);
            }
        });
        TestServices::WindowHelper->WaitForIdle();

        // Go in one direction, for the length of the group + 1.
        for (unsigned int i = 0; i < radioButtonCount + 1; i++)
        {
            if (moveForwardFirst)
            {
                if (useLeftRightkeys)
                {
                    CommonInputHelper::Right(inputDevice);
                }
                else
                {
                    CommonInputHelper::Down(inputDevice);
                }
            }
            else
            {
                if (useLeftRightkeys)
                {
                    CommonInputHelper::Left(inputDevice);
                }
                else
                {
                    CommonInputHelper::Up(inputDevice);
                }
            }
            TestServices::WindowHelper->WaitForIdle();
        }

        // Go in the opposite direction, for the length of the group + 1.
        for (unsigned int i = 0; i < radioButtonCount + 1; i++)
        {
            if (moveForwardFirst)
            {
                if (useLeftRightkeys)
                {
                    CommonInputHelper::Left(inputDevice);
                }
                else
                {
                    CommonInputHelper::Up(inputDevice);
                }
            }
            else
            {
                if (useLeftRightkeys)
                {
                    CommonInputHelper::Right(inputDevice);
                }
                else
                {
                    CommonInputHelper::Down(inputDevice);
                }
            }
            TestServices::WindowHelper->WaitForIdle();
        }
        LOG_OUTPUT(L"Expected focus sequence: %s", expectedFocusSequence->Data());
        LOG_OUTPUT(L"Actual focus sequence: %s", focusSequence->Data());
        VERIFY_ARE_EQUAL(focusSequence, expectedFocusSequence);
    }

    void RadioButtonIntegrationTests::CanInteractWithRadioButtonsOnAPage()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Page^ page1 = nullptr;
        xaml_controls::Page^ page2 = nullptr;
        xaml_controls::StackPanel^ panel = nullptr;

        const int radioButtonCount = 3;

        std::shared_ptr<Event> radioButtonCheckedEvent = std::make_shared<Event>();
        std::vector<SafeEventRegistrationType(xaml_controls::RadioButton, Checked)> checkedRegistrations;
        std::vector<xaml_controls::RadioButton^> radioButtons(radioButtonCount);
        std::vector<xaml_controls::RadioButton^> radioButtons2(radioButtonCount);
        std::vector<xaml_controls::RadioButton^> namedGroupRadioButtons(radioButtonCount);
        std::vector<xaml_controls::RadioButton^> namedGroupRadioButtons2(radioButtonCount);

        RunOnUIThread([&]()
        {
            // We do three groups of radio buttons to ensure only the correct group is updated
            page1 = TestServices::WindowHelper->SetupSimulatedAppPage();
            auto stackPanel = ref new xaml_controls::StackPanel();
            panel = AddRadioButtonsToPanelWithCheckedEvents(false, radioButtons, checkedRegistrations, radioButtonCheckedEvent);
            stackPanel->Children->Append(panel);
            panel = AddRadioButtonsToPanelWithCheckedEvents(false, radioButtons2, checkedRegistrations, radioButtonCheckedEvent);
            stackPanel->Children->Append(panel);
            panel = AddRadioButtonsToPanelWithCheckedEvents(true, namedGroupRadioButtons, checkedRegistrations, radioButtonCheckedEvent);
            stackPanel->Children->Append(panel);

            page1->Content = stackPanel;

            radioButtons[0]->IsChecked = true;
            radioButtons2[0]->IsChecked = true;
            namedGroupRadioButtons[0]->IsChecked = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Validate that 'Checked' state is preserved on a page.");
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(radioButtons[0]->IsChecked->Value);
            VERIFY_IS_FALSE(radioButtons[1]->IsChecked->Value);
            VERIFY_IS_FALSE(radioButtons[2]->IsChecked->Value);
            VERIFY_IS_TRUE(radioButtons2[0]->IsChecked->Value);
            VERIFY_IS_FALSE(radioButtons2[1]->IsChecked->Value);
            VERIFY_IS_FALSE(radioButtons2[2]->IsChecked->Value);
            VERIFY_IS_TRUE(namedGroupRadioButtons[0]->IsChecked->Value);
            VERIFY_IS_FALSE(namedGroupRadioButtons[1]->IsChecked->Value);
            VERIFY_IS_FALSE(namedGroupRadioButtons[2]->IsChecked->Value);
            });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->Tap(radioButtons[1]);

        radioButtonCheckedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Validate that 'Checked' state is changed when the second RadioButton is tapped.");
        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(radioButtons[0]->IsChecked->Value);
            VERIFY_IS_TRUE(radioButtons[1]->IsChecked->Value);
            VERIFY_IS_FALSE(radioButtons[2]->IsChecked->Value);
            VERIFY_IS_TRUE(radioButtons2[0]->IsChecked->Value);
            VERIFY_IS_FALSE(radioButtons2[1]->IsChecked->Value);
            VERIFY_IS_FALSE(radioButtons2[2]->IsChecked->Value);
            VERIFY_IS_TRUE(namedGroupRadioButtons[0]->IsChecked->Value);
            VERIFY_IS_FALSE(namedGroupRadioButtons[1]->IsChecked->Value);
            VERIFY_IS_FALSE(namedGroupRadioButtons[2]->IsChecked->Value);
            });
        TestServices::WindowHelper->WaitForIdle();


        LOG_OUTPUT(L"Navigate to the second page.");
        RunOnUIThread([&]()
        {

            page2 = TestServices::WindowHelper->SetupSimulatedAppPage();
            panel = AddRadioButtonsToPanelWithCheckedEvents(true, namedGroupRadioButtons2, checkedRegistrations, radioButtonCheckedEvent);
            page2->Content = panel;

            page1->Frame->Navigate(::Windows::UI::Xaml::Interop::TypeName(page2->GetType()));
            namedGroupRadioButtons2[0]->IsChecked = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Validate that 'Checked' state is preserved on the first page, even though we have navigated away from it.");
        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(radioButtons[0]->IsChecked->Value);
            VERIFY_IS_TRUE(radioButtons[1]->IsChecked->Value);
            VERIFY_IS_FALSE(radioButtons[2]->IsChecked->Value);
            VERIFY_IS_TRUE(radioButtons2[0]->IsChecked->Value);
            VERIFY_IS_FALSE(radioButtons2[1]->IsChecked->Value);
            VERIFY_IS_FALSE(radioButtons2[2]->IsChecked->Value);
            VERIFY_IS_TRUE(namedGroupRadioButtons[0]->IsChecked->Value);
            VERIFY_IS_FALSE(namedGroupRadioButtons[1]->IsChecked->Value);
            VERIFY_IS_FALSE(namedGroupRadioButtons[2]->IsChecked->Value);
            VERIFY_IS_TRUE(namedGroupRadioButtons2[0]->IsChecked->Value);
            VERIFY_IS_FALSE(namedGroupRadioButtons2[1]->IsChecked->Value);
            VERIFY_IS_FALSE(namedGroupRadioButtons2[2]->IsChecked->Value);

            radioButtons[2]->IsChecked = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Validate that 'Checked' state is changed on the first page, even though we have navigated away from it.");
        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(radioButtons[0]->IsChecked->Value);
            VERIFY_IS_FALSE(radioButtons[1]->IsChecked->Value);
            VERIFY_IS_TRUE(radioButtons[2]->IsChecked->Value);
            VERIFY_IS_TRUE(radioButtons2[0]->IsChecked->Value);
            VERIFY_IS_FALSE(radioButtons2[1]->IsChecked->Value);
            VERIFY_IS_FALSE(radioButtons2[2]->IsChecked->Value);
            VERIFY_IS_TRUE(namedGroupRadioButtons[0]->IsChecked->Value);
            VERIFY_IS_FALSE(namedGroupRadioButtons[1]->IsChecked->Value);
            VERIFY_IS_FALSE(namedGroupRadioButtons[2]->IsChecked->Value);
            VERIFY_IS_TRUE(namedGroupRadioButtons2[0]->IsChecked->Value);
            VERIFY_IS_FALSE(namedGroupRadioButtons2[1]->IsChecked->Value);
            VERIFY_IS_FALSE(namedGroupRadioButtons2[2]->IsChecked->Value);
            });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Validate that a change to group with the same name on the second page does not affect the group on the first page.");
        RunOnUIThread([&]()
            {
                namedGroupRadioButtons2[2]->IsChecked = true;
            });
        RunOnUIThread([&]()
            {
                VERIFY_IS_FALSE(radioButtons[0]->IsChecked->Value);
                VERIFY_IS_FALSE(radioButtons[1]->IsChecked->Value);
                VERIFY_IS_TRUE(radioButtons[2]->IsChecked->Value);
                VERIFY_IS_TRUE(radioButtons2[0]->IsChecked->Value);
                VERIFY_IS_FALSE(radioButtons2[1]->IsChecked->Value);
                VERIFY_IS_FALSE(radioButtons2[2]->IsChecked->Value);
                VERIFY_IS_TRUE(namedGroupRadioButtons[0]->IsChecked->Value);
                VERIFY_IS_FALSE(namedGroupRadioButtons[1]->IsChecked->Value);
                VERIFY_IS_FALSE(namedGroupRadioButtons[2]->IsChecked->Value);
                VERIFY_IS_FALSE(namedGroupRadioButtons2[0]->IsChecked->Value);
                VERIFY_IS_FALSE(namedGroupRadioButtons2[1]->IsChecked->Value);
                VERIFY_IS_TRUE(namedGroupRadioButtons2[2]->IsChecked->Value);
            });
        TestServices::WindowHelper->WaitForIdle();


        LOG_OUTPUT(L"Navigate back to the first page.");
        RunOnUIThread([&]()
        {
            page2->Frame->Navigate(::Windows::UI::Xaml::Interop::TypeName(page1->GetType()));
        });
        TestServices::WindowHelper->WaitForIdle();


        LOG_OUTPUT(L"Validate that 'Checked' state is preserved on the first page, when we navigate back to it.");
        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(radioButtons[0]->IsChecked->Value);
            VERIFY_IS_FALSE(radioButtons[1]->IsChecked->Value);
            VERIFY_IS_TRUE(radioButtons[2]->IsChecked->Value);
            VERIFY_IS_TRUE(radioButtons2[0]->IsChecked->Value);
            VERIFY_IS_FALSE(radioButtons2[1]->IsChecked->Value);
            VERIFY_IS_FALSE(radioButtons2[2]->IsChecked->Value);
            VERIFY_IS_TRUE(namedGroupRadioButtons[0]->IsChecked->Value);
            VERIFY_IS_FALSE(namedGroupRadioButtons[1]->IsChecked->Value);
            VERIFY_IS_FALSE(namedGroupRadioButtons[2]->IsChecked->Value);
            VERIFY_IS_FALSE(namedGroupRadioButtons2[0]->IsChecked->Value);
            VERIFY_IS_FALSE(namedGroupRadioButtons2[1]->IsChecked->Value);
            VERIFY_IS_TRUE(namedGroupRadioButtons2[2]->IsChecked->Value);

            radioButtons[0]->IsChecked = true;
        });
        TestServices::WindowHelper->WaitForIdle();


        LOG_OUTPUT(L"Validate that 'Checked' state is changed on the first page, even we have navigate back to it.");
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(radioButtons[0]->IsChecked->Value);
            VERIFY_IS_FALSE(radioButtons[1]->IsChecked->Value);
            VERIFY_IS_FALSE(radioButtons[2]->IsChecked->Value);
            VERIFY_IS_TRUE(radioButtons2[0]->IsChecked->Value);
            VERIFY_IS_FALSE(radioButtons2[1]->IsChecked->Value);
            VERIFY_IS_FALSE(radioButtons2[2]->IsChecked->Value);
            VERIFY_IS_TRUE(namedGroupRadioButtons[0]->IsChecked->Value);
            VERIFY_IS_FALSE(namedGroupRadioButtons[1]->IsChecked->Value);
            VERIFY_IS_FALSE(namedGroupRadioButtons[2]->IsChecked->Value);
            VERIFY_IS_FALSE(namedGroupRadioButtons2[0]->IsChecked->Value);
            VERIFY_IS_FALSE(namedGroupRadioButtons2[1]->IsChecked->Value);
            VERIFY_IS_TRUE(namedGroupRadioButtons2[2]->IsChecked->Value);
        });
    }

    void RadioButtonIntegrationTests::CanTapRadioButtonInAPopup()
    {
        TestCleanupWrapper cleanup;

        const int radioButtonCount = 3;

        xaml_controls::StackPanel^ rootPanel = nullptr;

        std::shared_ptr<Event> radioButtonCheckedEvent = std::make_shared<Event>();
        std::vector<SafeEventRegistrationType(xaml_controls::RadioButton, Checked)> checkedRegistrations;
        std::vector<xaml_controls::RadioButton^> radioButtons(radioButtonCount);

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                        <Popup x:Name="popup" IsOpen="True"/>
                    </StackPanel>)"));
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto radioButtonPanel = AddRadioButtonsToPanelWithCheckedEvents(true, radioButtons, checkedRegistrations, radioButtonCheckedEvent);
            auto popup = dynamic_cast<xaml_primitives::Popup^>(rootPanel->FindName(L"popup"));
            popup->Child = radioButtonPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->Tap(radioButtons[1]);
        radioButtonCheckedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(radioButtons[0]->IsChecked->Value);
            VERIFY_IS_TRUE(radioButtons[1]->IsChecked->Value);
            VERIFY_IS_FALSE(radioButtons[2]->IsChecked->Value);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void RadioButtonIntegrationTests::SelectionPatternShouldInvokeClick()
    {
        TestCleanupWrapper cleanup;

        // Regression coverage for:
        // Narrator Desktop: CAPS+Enter on a RadioButton doesn't invoke Command or Click handler
        // It might make logical sense that ISelectionItem.Select on a RadioButton AutomationPeer would be equivalent to 
        // programatically setting IsChecked=true. 
        // However, it is usually expected that interacting with an application via narrator will produce the same results
        // as interacting with it via mouse and keyboard. As such, ISelectionItem.Select should invoke the Click operation on 
        // the RadioButton (which will in turn set IsChecked). 

        xaml_controls::RadioButton^ radioButton1;
        Event clickEvent;
        auto clickRegistration = CreateSafeEventRegistration(xaml_controls::RadioButton, Click);
        Event checkedEvent;
        auto checkedRegistration = CreateSafeEventRegistration(xaml_controls::RadioButton, Checked);

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <RadioButton x:Name="radioButton1" />
                        <RadioButton x:Name="radioButton2" IsChecked="True" />
                    </StackPanel>)"));

            radioButton1 = safe_cast<xaml_controls::RadioButton^>(rootPanel->FindName(L"radioButton1"));
            
            clickRegistration.Attach(radioButton1, [&](){
                clickEvent.Set();
            });
            checkedRegistration.Attach(radioButton1, [&](){
                checkedEvent.Set();
            });

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        xaml_automation_peers::RadioButtonAutomationPeer^ radioButtonPeer;

        RunOnUIThread([&]()
        {
            auto peer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(radioButton1);
            radioButtonPeer = safe_cast<xaml_automation_peers::RadioButtonAutomationPeer^>(peer);
            radioButtonPeer->Select();
        });

        clickEvent.WaitForDefault();
        checkedEvent.WaitForDefault();
    }

    void RadioButtonIntegrationTests::RadioButtonInCycleTabFocusDoesNotCauseInfiniteLoop()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::RadioButton^ radioButton;
        xaml_controls::Button^ buttonOutsideCycle;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                        <StackPanel TabFocusNavigation="Cycle">
                            <RadioButton x:Name="RadioButton" IsTabStop="False">
                                <Button />
                            </RadioButton>
                        </StackPanel>
                        <Button x:Name="ButtonOutsideCycle" />
                    </StackPanel>)"));

            radioButton = safe_cast<xaml_controls::RadioButton^>(rootPanel->FindName(L"RadioButton"));
            buttonOutsideCycle = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"ButtonOutsideCycle"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Giving focus to the radio button.");
        FocusTestHelper::EnsureFocus(radioButton, xaml::FocusState::Keyboard);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Pressing the down key to attempt to find the next radio button. We should not get in an infinite loop.");
        TestServices::KeyboardHelper->Down();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Giving focus to another element, which will only succeed if we are not in an infinite loop.");
        FocusTestHelper::EnsureFocus(buttonOutsideCycle, xaml::FocusState::Keyboard);
        TestServices::WindowHelper->WaitForIdle();
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::RadioButton
