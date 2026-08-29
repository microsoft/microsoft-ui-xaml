// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ToggleButtonIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>
#include <generic\ToggleButtonTests.h>

#include <ControlHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Primitives { namespace ToggleButton {

    bool ToggleButtonIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ToggleButtonIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ToggleButtonIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void ToggleButtonIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_primitives::ToggleButton>::CanInstantiate();
    }

    void ToggleButtonIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_primitives::ToggleButton>::CanEnterAndLeaveLiveTree();
    }

    void ToggleButtonIntegrationTests::CanToggle()
    {
        Generic::ToggleButtonTests<xaml_primitives::ToggleButton>::CanToggle();
    }

    void ToggleButtonIntegrationTests::UIETree()
    {
        ControlHelper::ValidateUIElementTree(
            wf::Size(400, 600),
            1.f,
            []()
            {
                xaml_primitives::ToggleButton^ restToggleButton = nullptr;
                xaml_primitives::ToggleButton^ pointerOverToggleButton = nullptr;
                xaml_primitives::ToggleButton^ pressedToggleButton = nullptr;
                xaml_primitives::ToggleButton^ disabledToggleButton = nullptr;
                xaml_primitives::ToggleButton^ checkedToggleButton = nullptr;
                xaml_primitives::ToggleButton^ checkedPointerOverToggleButton = nullptr;
                xaml_primitives::ToggleButton^ checkedPressedToggleButton = nullptr;
                xaml_primitives::ToggleButton^ checkedDisabledToggleButton = nullptr;
                xaml_primitives::ToggleButton^ indeterminateToggleButton = nullptr;
                xaml_primitives::ToggleButton^ indeterminatePointerOverToggleButton = nullptr;
                xaml_primitives::ToggleButton^ indeterminatePressedToggleButton = nullptr;
                xaml_primitives::ToggleButton^ indeterminateDisabledToggleButton = nullptr;

                xaml_primitives::ToggleButton^ focusedRestToggleButton = nullptr;
                xaml_primitives::ToggleButton^ focusedPointerOverToggleButton = nullptr;
                xaml_primitives::ToggleButton^ focusedPressedToggleButton = nullptr;
                xaml_primitives::ToggleButton^ focusedCheckedToggleButton = nullptr;
                xaml_primitives::ToggleButton^ focusedCheckedPointerOverToggleButton = nullptr;
                xaml_primitives::ToggleButton^ focusedCheckedPressedToggleButton = nullptr;
                xaml_primitives::ToggleButton^ focusedIndeterminateToggleButton = nullptr;
                xaml_primitives::ToggleButton^ focusedIndeterminatePointerOverToggleButton = nullptr;
                xaml_primitives::ToggleButton^ focusedIndeterminatePressedToggleButton = nullptr;

                xaml_controls::StackPanel^ rootPanel = nullptr;

                RunOnUIThread([&]()
                {
                    rootPanel = ref new xaml_controls::StackPanel();

                    // Ensure that mouse movements over the scene don't cause unexpected failures.
                    rootPanel->IsHitTestVisible = false;

                    restToggleButton = ref new xaml_primitives::ToggleButton();
                    restToggleButton->Content = "Rest Toggle Button";
                    rootPanel->Children->Append(restToggleButton);

                    checkedToggleButton = ref new xaml_primitives::ToggleButton();
                    checkedToggleButton->Content = "Checked Toggle Button";
                    rootPanel->Children->Append(checkedToggleButton);

                    indeterminateToggleButton = ref new xaml_primitives::ToggleButton();
                    indeterminateToggleButton->Content = "Indeterminate Toggle Button";
                    rootPanel->Children->Append(indeterminateToggleButton);

                    pointerOverToggleButton = ref new xaml_primitives::ToggleButton();
                    pointerOverToggleButton->Content = "PointerOver Toggle Button";
                    rootPanel->Children->Append(pointerOverToggleButton);

                    checkedPointerOverToggleButton = ref new xaml_primitives::ToggleButton();
                    checkedPointerOverToggleButton->Content = "Checked PointerOver Toggle Button";
                    rootPanel->Children->Append(checkedPointerOverToggleButton);

                    indeterminatePointerOverToggleButton = ref new xaml_primitives::ToggleButton();
                    indeterminatePointerOverToggleButton->Content = "Indeterminate PointerOver Toggle Button";
                    rootPanel->Children->Append(indeterminatePointerOverToggleButton);

                    pressedToggleButton = ref new xaml_primitives::ToggleButton();
                    pressedToggleButton->Content = "Pressed Toggle Button";
                    rootPanel->Children->Append(pressedToggleButton);

                    checkedPressedToggleButton = ref new xaml_primitives::ToggleButton();
                    checkedPressedToggleButton->Content = "Checked Pressed Toggle Button";
                    rootPanel->Children->Append(checkedPressedToggleButton);

                    indeterminatePressedToggleButton = ref new xaml_primitives::ToggleButton();
                    indeterminatePressedToggleButton->Content = "Indeterminate Pressed Toggle Button";
                    rootPanel->Children->Append(indeterminatePressedToggleButton);

                    disabledToggleButton = ref new xaml_primitives::ToggleButton();
                    disabledToggleButton->Content = "Disabled Toggle Button";
                    rootPanel->Children->Append(disabledToggleButton);

                    checkedDisabledToggleButton = ref new xaml_primitives::ToggleButton();
                    checkedDisabledToggleButton->Content = "Checked Disabled Toggle Button";
                    rootPanel->Children->Append(checkedDisabledToggleButton);

                    indeterminateDisabledToggleButton = ref new xaml_primitives::ToggleButton();
                    indeterminateDisabledToggleButton->Content = "Indeterminate Disabled Toggle Button";
                    rootPanel->Children->Append(indeterminateDisabledToggleButton);

                    focusedRestToggleButton = ref new xaml_primitives::ToggleButton();
                    focusedRestToggleButton->Content = "Focused Rest Toggle Button";
                    rootPanel->Children->Append(focusedRestToggleButton);

                    focusedCheckedToggleButton = ref new xaml_primitives::ToggleButton();
                    focusedCheckedToggleButton->Content = "Focused Checked Toggle Button";
                    rootPanel->Children->Append(focusedCheckedToggleButton);

                    focusedIndeterminateToggleButton = ref new xaml_primitives::ToggleButton();
                    focusedIndeterminateToggleButton->Content = "Focused Indeterminate Toggle Button";
                    rootPanel->Children->Append(focusedIndeterminateToggleButton);

                    focusedPointerOverToggleButton = ref new xaml_primitives::ToggleButton();
                    focusedPointerOverToggleButton->Content = "Focused PointerOver Toggle Button";
                    rootPanel->Children->Append(focusedPointerOverToggleButton);

                    focusedCheckedPointerOverToggleButton = ref new xaml_primitives::ToggleButton();
                    focusedCheckedPointerOverToggleButton->Content = "Focused Checked PointerOver Toggle Button";
                    rootPanel->Children->Append(focusedCheckedPointerOverToggleButton);

                    focusedIndeterminatePointerOverToggleButton = ref new xaml_primitives::ToggleButton();
                    focusedIndeterminatePointerOverToggleButton->Content = "Focused Indeterminate PointerOver Toggle Button";
                    rootPanel->Children->Append(focusedIndeterminatePointerOverToggleButton);

                    focusedPressedToggleButton = ref new xaml_primitives::ToggleButton();
                    focusedPressedToggleButton->Content = "Focused Pressed Toggle Button";
                    rootPanel->Children->Append(focusedPressedToggleButton);

                    focusedCheckedPressedToggleButton = ref new xaml_primitives::ToggleButton();
                    focusedCheckedPressedToggleButton->Content = "Focused Checked Pressed Toggle Button";
                    rootPanel->Children->Append(focusedCheckedPressedToggleButton);

                    focusedIndeterminatePressedToggleButton = ref new xaml_primitives::ToggleButton();
                    focusedIndeterminatePressedToggleButton->Content = "Focused Indeterminate Pressed Toggle Button";
                    rootPanel->Children->Append(focusedIndeterminatePressedToggleButton);

                    TestServices::WindowHelper->WindowContent = rootPanel;
                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    VisualStateManager::GoToState(pointerOverToggleButton, "PointerOver", false);
                    VisualStateManager::GoToState(pressedToggleButton, "Pressed", false);
                    VisualStateManager::GoToState(disabledToggleButton, "Disabled", false);
                    VisualStateManager::GoToState(checkedToggleButton, "Checked", false);
                    VisualStateManager::GoToState(checkedPointerOverToggleButton, "CheckedPointerOver", false);
                    VisualStateManager::GoToState(checkedPressedToggleButton, "CheckedPressed", false);
                    VisualStateManager::GoToState(checkedDisabledToggleButton, "CheckedDisabled", false);
                    VisualStateManager::GoToState(indeterminateToggleButton, "Indeterminate", false);
                    VisualStateManager::GoToState(indeterminatePointerOverToggleButton, "IndeterminatePointerOver", false);
                    VisualStateManager::GoToState(indeterminatePressedToggleButton, "IndeterminatePressed", false);
                    VisualStateManager::GoToState(indeterminateDisabledToggleButton, "IndeterminateDisabled", false);

                    VisualStateManager::GoToState(focusedRestToggleButton, "Focused", false);
                    VisualStateManager::GoToState(focusedPointerOverToggleButton, "Focused", false);
                    VisualStateManager::GoToState(focusedPointerOverToggleButton, "PointerOver", false);
                    VisualStateManager::GoToState(focusedPressedToggleButton, "Focused", false);
                    VisualStateManager::GoToState(focusedPressedToggleButton, "Pressed", false);
                    VisualStateManager::GoToState(focusedCheckedToggleButton, "Focused", false);
                    VisualStateManager::GoToState(focusedCheckedToggleButton, "Checked", false);
                    VisualStateManager::GoToState(focusedCheckedPointerOverToggleButton, "Focused", false);
                    VisualStateManager::GoToState(focusedCheckedPointerOverToggleButton, "CheckedPointerOver", false);
                    VisualStateManager::GoToState(focusedCheckedPressedToggleButton, "Focused", false);
                    VisualStateManager::GoToState(focusedCheckedPressedToggleButton, "CheckedPressed", false);
                    VisualStateManager::GoToState(focusedIndeterminateToggleButton, "Focused", false);
                    VisualStateManager::GoToState(focusedIndeterminateToggleButton, "Indeterminate", false);
                    VisualStateManager::GoToState(focusedIndeterminatePointerOverToggleButton, "Focused", false);
                    VisualStateManager::GoToState(focusedIndeterminatePointerOverToggleButton, "IndeterminatePointerOver", false);
                    VisualStateManager::GoToState(focusedIndeterminatePressedToggleButton, "Focused", false);
                    VisualStateManager::GoToState(focusedIndeterminatePressedToggleButton, "IndeterminatePressed", false);
                });
                TestServices::WindowHelper->WaitForIdle();

                return rootPanel;
            });
    }

    void ToggleButtonIntegrationTests::ValidateFootprint()
    {
        TestCleanupWrapper cleanup;

        double const expectedToggleButtonWidth_WithTextContent = 66;
        double const expectedToggleButtonWidth_WithNoContent = 24;
        double const expectedToggleButtonWidth_WithLargeContent = 100 + expectedToggleButtonWidth_WithNoContent;

        double const expectedToggleButtonHeight_WithTextContent = 32;
        double const expectedToggleButtonHeight_WithNoContent = 13;
        double const expectedToggleButtonHeight_WithLargeContent = 100 + expectedToggleButtonHeight_WithNoContent;

        xaml_primitives::ToggleButton^ toggleButtonWithTextContent;
        xaml_primitives::ToggleButton^ toggleButtonWithNoContent;
        xaml_primitives::ToggleButton^ toggleButtonWithLargeContent;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <ToggleButton x:Name="toggleButtonWithTextContent" Content="Button" />
                        <ToggleButton x:Name="toggleButtonWithNoContent" />
                        <ToggleButton x:Name="toggleButtonWithLargeContent" >
                            <Rectangle Height="100" Width="100" Fill="Red" />
                        </ToggleButton>
                    </StackPanel>)"));

            toggleButtonWithTextContent = safe_cast<xaml_primitives::ToggleButton^>(rootPanel->FindName(L"toggleButtonWithTextContent"));
            toggleButtonWithNoContent = safe_cast<xaml_primitives::ToggleButton^>(rootPanel->FindName(L"toggleButtonWithNoContent"));
            toggleButtonWithLargeContent = safe_cast<xaml_primitives::ToggleButton^>(rootPanel->FindName(L"toggleButtonWithLargeContent"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            // Verify Footprint of ToggleButton with text content:
            VERIFY_ARE_EQUAL(expectedToggleButtonWidth_WithTextContent, toggleButtonWithTextContent->ActualWidth);
            VERIFY_ARE_EQUAL(expectedToggleButtonHeight_WithTextContent, toggleButtonWithTextContent->ActualHeight);

            // Verify Footprint of ToggleButton with no content:
            VERIFY_ARE_EQUAL(expectedToggleButtonWidth_WithNoContent, toggleButtonWithNoContent->ActualWidth);
            VERIFY_ARE_EQUAL(expectedToggleButtonHeight_WithNoContent, toggleButtonWithNoContent->ActualHeight);

            // Verify Footprint of ToggleButton with content:
            VERIFY_ARE_EQUAL(expectedToggleButtonWidth_WithLargeContent, toggleButtonWithLargeContent->ActualWidth);
            VERIFY_ARE_EQUAL(expectedToggleButtonHeight_WithLargeContent, toggleButtonWithLargeContent->ActualHeight);
        });
    }

    void ToggleButtonIntegrationTests::CanToggleThroughThreeStates()
    {
        TestCleanupWrapper cleanup;

        xaml_primitives::ToggleButton^ toggleButton;

        auto clickedEvent = std::make_shared<Event>();
        auto indeterminateEvent = std::make_shared<Event>();

        auto clickedRegistration = CreateSafeEventRegistration(xaml_primitives::ToggleButton, Click);
        auto indeterminateRegistration = CreateSafeEventRegistration(xaml_primitives::ToggleButton, Indeterminate);

        RunOnUIThread([&]()
        {
            toggleButton = ref new xaml_primitives::ToggleButton();
            toggleButton->Content = "Toggle Button";
            toggleButton->IsThreeState = true;
            toggleButton->IsChecked = false;

            clickedRegistration.Attach(toggleButton, [clickedEvent]() {clickedEvent->Set(); });

            indeterminateRegistration.Attach(toggleButton, [indeterminateEvent]() {indeterminateEvent->Set(); });

            TestServices::WindowHelper->WindowContent = toggleButton;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Test false->true transition
        TestServices::InputHelper->Tap(toggleButton);
        clickedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(toggleButton->IsChecked->Value);
        });
        TestServices::WindowHelper->WaitForIdle();

        // After a click on a Clicked toggle button (IsClick = true) state should move to indeterminate when the toggle is a 3 state toggle.
        // In this case 2 events fire --- Indeterminate and Click
        TestServices::InputHelper->Tap(toggleButton);
        clickedEvent->WaitForDefault();
        indeterminateEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_NULL(toggleButton->IsChecked);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Test after indeterminate, a tap causes the button to move to the IsClicked = false state
        TestServices::InputHelper->Tap(toggleButton);
        clickedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(toggleButton->IsChecked->Value);
        });
    }

} } } } } } } // Microsoft::UI::Xaml::Tests::Controls::Primitives::ToggleButton
