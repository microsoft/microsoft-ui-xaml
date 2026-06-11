// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "RepeatButtonIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>
#include <generic\ButtonBaseTests.h>
#include "KeyboardInjectionOverride.h"

#include <ControlHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Primitives { namespace RepeatButton {

    bool RepeatButtonIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool RepeatButtonIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool RepeatButtonIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void RepeatButtonIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_primitives::RepeatButton>::CanInstantiate();
    }

    void RepeatButtonIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_primitives::RepeatButton>::CanEnterAndLeaveLiveTree();
    }

    void RepeatButtonIntegrationTests::CanClickUsingTap()
    {
        TestCleanupWrapper cleanup;

        xaml_primitives::RepeatButton^ repeatButton = nullptr;
        auto clickEvent = std::make_shared<Event>();
        auto clickRegistration = CreateSafeEventRegistration(xaml_primitives::RepeatButton, Click);

        RunOnUIThread([&]()
        {
            repeatButton = ref new xaml_primitives::RepeatButton();
            repeatButton->Delay = c_repeatButtonDelay;
            repeatButton->Interval = c_repeatButtonInterval;

            clickRegistration.Attach(repeatButton, [clickEvent]() {clickEvent->Set();} );

            TestServices::WindowHelper->WindowContent = repeatButton;
        });

        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->Hold(repeatButton, c_holdDuration);
        clickEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(repeatButton->FocusState == FocusState::Pointer);
        });
    }

    void RepeatButtonIntegrationTests::UIETree()
    {
        ControlHelper::ValidateUIElementTree(
            wf::Size(400, 600),
            1.f,
            []()
            {
                xaml_primitives::RepeatButton^ restRepeatButton = nullptr;
                xaml_primitives::RepeatButton^ pointerOverRepeatButton = nullptr;
                xaml_primitives::RepeatButton^ pressedRepeatButton = nullptr;
                xaml_primitives::RepeatButton^ disabledRepeatButton = nullptr;

                xaml_primitives::RepeatButton^ focusedRestRepeatButton = nullptr;
                xaml_primitives::RepeatButton^ focusedpointerOverRepeatButton = nullptr;
                xaml_primitives::RepeatButton^ focusedPressedRepeatButton = nullptr;
                xaml_primitives::RepeatButton^ pointerFocusedRepeatButton = nullptr;

                xaml_controls::StackPanel^ rootPanel = nullptr;

                RunOnUIThread([&]()
                {
                    rootPanel = ref new xaml_controls::StackPanel();

                    restRepeatButton = ref new xaml_primitives::RepeatButton();
                    restRepeatButton->Content = "RepeatButton";
                    rootPanel->Children->Append(restRepeatButton);

                    pointerOverRepeatButton = ref new xaml_primitives::RepeatButton();
                    pointerOverRepeatButton->Content = "PointerOver RepeatButton";
                    rootPanel->Children->Append(pointerOverRepeatButton);

                    pressedRepeatButton = ref new xaml_primitives::RepeatButton();
                    pressedRepeatButton->Content = "Pressed RepeatButton";
                    rootPanel->Children->Append(pressedRepeatButton);

                    disabledRepeatButton = ref new xaml_primitives::RepeatButton();
                    disabledRepeatButton->Content = "Disabled RepeatButton";
                    disabledRepeatButton->IsEnabled = false;
                    rootPanel->Children->Append(disabledRepeatButton);

                    focusedRestRepeatButton = ref new xaml_primitives::RepeatButton();
                    focusedRestRepeatButton->Content = "Focused Rest RepeatButton";
                    rootPanel->Children->Append(focusedRestRepeatButton);

                    focusedpointerOverRepeatButton = ref new xaml_primitives::RepeatButton();
                    focusedpointerOverRepeatButton->Content = "Focused PointerOver RepeatButton";
                    rootPanel->Children->Append(focusedpointerOverRepeatButton);

                    focusedPressedRepeatButton = ref new xaml_primitives::RepeatButton();
                    focusedPressedRepeatButton->Content = "Focused Pressed RepeatButton";
                    rootPanel->Children->Append(focusedPressedRepeatButton);

                    pointerFocusedRepeatButton = ref new xaml_primitives::RepeatButton();
                    pointerFocusedRepeatButton->Content = "Pointer Focused RepeatButton";
                    rootPanel->Children->Append(pointerFocusedRepeatButton);

                    TestServices::WindowHelper->WindowContent = rootPanel;
                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    VisualStateManager::GoToState(pointerOverRepeatButton, "PointerOver", false);

                    VisualStateManager::GoToState(pressedRepeatButton, "Pressed", false);

                    VisualStateManager::GoToState(focusedRestRepeatButton, "Focused", false);

                    VisualStateManager::GoToState(focusedpointerOverRepeatButton, "Focused", false);
                    VisualStateManager::GoToState(focusedpointerOverRepeatButton, "PointerOver", false);

                    VisualStateManager::GoToState(focusedPressedRepeatButton, "Focused", false);
                    VisualStateManager::GoToState(focusedPressedRepeatButton, "Pressed", false);

                    VisualStateManager::GoToState(pointerFocusedRepeatButton, "PointerFocused", false);
                });
                TestServices::WindowHelper->WaitForIdle();

                return rootPanel;
            });
    }

    void RepeatButtonIntegrationTests::ValidateFootprint()
    {
        TestCleanupWrapper cleanup;

        double const expectedRepeatButtonWidth_WithTextContent = 66;
        double const expectedRepeatButtonWidth_WithNoContent = 24;
        double const expectedRepeatButtonWidth_WithLargeContent = 100 + expectedRepeatButtonWidth_WithNoContent;

        double const expectedRepeatButtonHeight_WithTextContent = 32;
        double const expectedRepeatButtonHeight_WithNoContent = 13;
        double const expectedRepeatButtonHeight_WithLargeContent = 100 + expectedRepeatButtonHeight_WithNoContent;

        xaml_primitives::RepeatButton^ repeatButtonWithTextContent;
        xaml_primitives::RepeatButton^ repeatButtonWithNoContent;
        xaml_primitives::RepeatButton^ repeatButtonWithLargeContent;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <RepeatButton x:Name="repeatButtonWithTextContent" Content="Button" />
                        <RepeatButton x:Name="repeatButtonWithNoContent" />
                        <RepeatButton x:Name="repeatButtonWithLargeContent" >
                            <Rectangle Height="100" Width="100" Fill="Red" />
                        </RepeatButton>
                    </StackPanel>)"));

            repeatButtonWithTextContent = safe_cast<xaml_primitives::RepeatButton^>(rootPanel->FindName(L"repeatButtonWithTextContent"));
            repeatButtonWithNoContent = safe_cast<xaml_primitives::RepeatButton^>(rootPanel->FindName(L"repeatButtonWithNoContent"));
            repeatButtonWithLargeContent = safe_cast<xaml_primitives::RepeatButton^>(rootPanel->FindName(L"repeatButtonWithLargeContent"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            // Verify Footprint of RepeatButton with text content:
            VERIFY_ARE_EQUAL(expectedRepeatButtonWidth_WithTextContent, repeatButtonWithTextContent->ActualWidth);
            VERIFY_ARE_EQUAL(expectedRepeatButtonHeight_WithTextContent, repeatButtonWithTextContent->ActualHeight);

            // Verify Footprint of RepeatButton with no content:
            VERIFY_ARE_EQUAL(expectedRepeatButtonWidth_WithNoContent, repeatButtonWithNoContent->ActualWidth);
            VERIFY_ARE_EQUAL(expectedRepeatButtonHeight_WithNoContent, repeatButtonWithNoContent->ActualHeight);

            // Verify Footprint of RepeatButton with large content:
            VERIFY_ARE_EQUAL(expectedRepeatButtonWidth_WithLargeContent, repeatButtonWithLargeContent->ActualWidth);
            VERIFY_ARE_EQUAL(expectedRepeatButtonHeight_WithLargeContent, repeatButtonWithLargeContent->ActualHeight);
        });
    }

    xaml_primitives::RepeatButton^ RepeatButtonIntegrationTests::SetupButtonTestUI(
        UINT delay,
        UINT interval,
        xaml_controls::ClickMode mode,
        SafeEventRegistrationType(xaml_primitives::RepeatButton, Click)& clickRegistration,
        std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> clickEvent)
    {
        xaml_primitives::RepeatButton^ repeatButton = nullptr;

        RunOnUIThread([&]()
        {
            repeatButton = ref new xaml_primitives::RepeatButton();
            repeatButton->Content = "RepeatContent";
            repeatButton->Delay = delay;
            repeatButton->Interval = interval;
            repeatButton->ClickMode = mode;

            clickRegistration.Attach(repeatButton, [clickEvent]()
            {
                LOG_OUTPUT(L"RepeatButton click event fired!");
                clickEvent->Set();
            });

            TestServices::WindowHelper->WindowContent = repeatButton;
        });
        return repeatButton;
    }

    void RepeatButtonIntegrationTests::CanChangeRepeatDelay()
    {
        TestCleanupWrapper cleanup;

        auto clickEvent = std::make_shared<Event>();
        auto clickRegistration = CreateSafeEventRegistration(xaml_primitives::RepeatButton, Click);
        xaml_primitives::RepeatButton^ repeatButton = SetupButtonTestUI(c_longRepeatButtonDelay, c_longRepeatButtonInterval, xaml_controls::ClickMode::Press, clickRegistration, clickEvent);

        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->Hold(repeatButton, c_holdDuration);
        clickEvent->WaitForDefault();

        // Try to change the repeat button delay
        RunOnUIThread([&]()
        {
            repeatButton->Delay = c_repeatButtonDelay;
            repeatButton->Interval = c_repeatButtonInterval;
        });

        TestServices::WindowHelper->WaitForIdle();

        // Hold down the button with the new repeatdelay and repeatinterval of 10ms each for hold duration of 200ms
        // In this scenario, the click event should fire ~20 times.  To prevent races, I'll verify it's fired at least twice.
        clickEvent->Reset();
        TestServices::InputHelper->Hold(repeatButton, c_holdDuration);
        clickEvent->WaitForDefault();
        VERIFY_IS_TRUE(clickEvent->TimesFired() > 2);

        // Let's set the delay to the lower limit e.g. 0
        RunOnUIThread([&]()
        {
            repeatButton->Delay = 0;
        });

        TestServices::WindowHelper->WaitForIdle();

        clickEvent->Reset();
        TestServices::InputHelper->Hold(repeatButton, c_holdDuration);
        clickEvent->WaitForDefault();
        VERIFY_IS_TRUE(clickEvent->TimesFired() > 2);
    }

    void RepeatButtonIntegrationTests::CanActivateWithSpaceKeyInput()
    {
        TestCleanupWrapper cleanup;
        KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride;

        auto clickEvent = std::make_shared<Event>();
        auto clickRegistration = CreateSafeEventRegistration(xaml_primitives::RepeatButton, Click);

        xaml_primitives::RepeatButton^ repeatButton = SetupButtonTestUI(c_repeatButtonDelay, c_repeatButtonInterval, xaml_controls::ClickMode::Press, clickRegistration, clickEvent);

        TestServices::WindowHelper->WaitForIdle();

        // Press down the space key
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ ");
        clickEvent->WaitForDefault();

        // Keeping space key pressed should cause the button to refire the click event, let's wait for the next event
        clickEvent->WaitForDefault();

        TestServices::KeyboardHelper->PressKeySequence(L"$u$_ ");
        TestServices::WindowHelper->WaitForIdle();
    }

    void RepeatButtonIntegrationTests::CanActivateWithClickModeHover()
    {
        TestCleanupWrapper cleanup;

        auto clickEvent = std::make_shared<Event>();
        auto clickRegistration = CreateSafeEventRegistration(xaml_primitives::RepeatButton, Click);

        xaml_primitives::RepeatButton^ repeatButton = SetupButtonTestUI(c_repeatButtonDelay, c_repeatButtonInterval, xaml_controls::ClickMode::Hover, clickRegistration, clickEvent);

        TestServices::WindowHelper->WaitForIdle();

        // Move the mouse off of the repeat button initialy.  Otherwise sometimes the system will not
        // correctly detect the mouse is over the button.
        TestServices::InputHelper->MoveMouse(wf::Point(0, 0));

        TestServices::InputHelper->MoveMouse(repeatButton);
        clickEvent->WaitForDefault();

        // Keeping the pointer over the button should cause the button to refire the click event, let's wait for the next event
        clickEvent->WaitForDefault();
    }

} } } } } } } // Microsoft::UI::Xaml::Tests::Controls::Primitives::RepeatButton
