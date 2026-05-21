// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ToggleSwitchIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <TreeHelper.h>
#include "FeatureFlags.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <ControlHelper.h>
#include <Utils.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ToggleSwitch {

    bool ToggleSwitchIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ToggleSwitchIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ToggleSwitchIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void ToggleSwitchIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::ToggleSwitch>::CanInstantiate();
    }

    void ToggleSwitchIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::ToggleSwitch>::CanEnterAndLeaveLiveTree();
    }

    void ToggleSwitchIntegrationTests::DoesFireToggleEvent()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ToggleSwitch^ toggleSwitch = nullptr;

        auto toggleSwitchToggledEventRegistration = CreateSafeEventRegistration(xaml_controls::ToggleSwitch, Toggled);
        auto toggleSwitchToggledEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            toggleSwitch = ref new xaml_controls::ToggleSwitch();
            toggleSwitch->IsOn = false;

            toggleSwitchToggledEventRegistration.Attach(toggleSwitch, ref new RoutedEventHandler([&](Platform::Object^, RoutedEventArgs^)
            {
                toggleSwitchToggledEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = toggleSwitch;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(toggleSwitch->IsOn);
            toggleSwitch->IsOn = true;
        });

        toggleSwitchToggledEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(toggleSwitch->IsOn);
            toggleSwitch->IsOn = false;
        });

        toggleSwitchToggledEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(toggleSwitch->IsOn);
        });
    }

    void ToggleSwitchIntegrationTests::ValidateUIElementTree()
    {
        ControlHelper::ValidateUIElementTree(
            wf::Size(400, 600),
            1.f,
            []()
            {
                xaml_controls::ToggleSwitch^ restOffSwitch = nullptr;
                xaml_controls::ToggleSwitch^ hoverOffSwitch = nullptr;
                xaml_controls::ToggleSwitch^ pressedOffSwitch = nullptr;
                xaml_controls::ToggleSwitch^ disabledOffSwitch = nullptr;

                xaml_controls::ToggleSwitch^ restOnSwitch = nullptr;
                xaml_controls::ToggleSwitch^ hoverOnSwitch = nullptr;
                xaml_controls::ToggleSwitch^ pressedOnSwitch = nullptr;
                xaml_controls::ToggleSwitch^ disabledOnSwitch = nullptr;

                xaml_controls::ToggleSwitch^ focusedRestOffSwitch = nullptr;
                xaml_controls::ToggleSwitch^ focusedHoverOffSwitch = nullptr;
                xaml_controls::ToggleSwitch^ focusedPressedOffSwitch = nullptr;
                xaml_controls::ToggleSwitch^ focusedRestOnSwitch = nullptr;
                xaml_controls::ToggleSwitch^ focusedHoverOnSwitch = nullptr;
                xaml_controls::ToggleSwitch^ focusedPressedOnSwitch = nullptr;

#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
                xaml_controls::ToggleSwitch^ leftHeaderSwitch = nullptr;
#endif

                xaml_controls::StackPanel^ rootPanel = nullptr;

                RunOnUIThread([&]()
                {
                    rootPanel = ref new xaml_controls::StackPanel();
                    rootPanel->Orientation = xaml_controls::Orientation::Horizontal;
                    xaml_controls::StackPanel^ sp1 = ref new xaml_controls::StackPanel();
                    xaml_controls::StackPanel^ sp2 = ref new xaml_controls::StackPanel();
                    rootPanel->Children->Append(sp1);
                    rootPanel->Children->Append(sp2);

                    restOffSwitch = ref new xaml_controls::ToggleSwitch();
                    restOffSwitch->Header = "Rest Off";
                    sp1->Children->Append(restOffSwitch);

                    hoverOffSwitch = ref new xaml_controls::ToggleSwitch();
                    hoverOffSwitch->Header = "Hover Off";
                    sp1->Children->Append(hoverOffSwitch);

                    pressedOffSwitch = ref new xaml_controls::ToggleSwitch();
                    pressedOffSwitch->Header = "Pressed Off";
                    sp1->Children->Append(pressedOffSwitch);

                    disabledOffSwitch = ref new xaml_controls::ToggleSwitch();
                    disabledOffSwitch->Header = "Disabled Off";
                    disabledOffSwitch->IsEnabled = false;
                    sp1->Children->Append(disabledOffSwitch);

                    restOnSwitch = ref new xaml_controls::ToggleSwitch();
                    restOnSwitch->Header = "Rest On";
                    restOnSwitch->IsOn = true;
                    sp1->Children->Append(restOnSwitch);

                    hoverOnSwitch = ref new xaml_controls::ToggleSwitch();
                    hoverOnSwitch->Header = "Hover On";
                    hoverOnSwitch->IsOn = true;
                    sp1->Children->Append(hoverOnSwitch);

                    pressedOnSwitch = ref new xaml_controls::ToggleSwitch();
                    pressedOnSwitch->Header = "Pressed On";
                    pressedOnSwitch->IsOn = true;
                    sp1->Children->Append(pressedOnSwitch);

                    disabledOnSwitch = ref new xaml_controls::ToggleSwitch();
                    disabledOnSwitch->Header = "Disabled On";
                    disabledOnSwitch->IsEnabled = false;
                    disabledOnSwitch->IsOn = true;
                    sp1->Children->Append(disabledOnSwitch);

#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
                    leftHeaderSwitch = ref new xaml_controls::ToggleSwitch();
                    leftHeaderSwitch->Header = "Left Header";
                    leftHeaderSwitch->HeaderPlacement = xaml_controls::ControlHeaderPlacement::Left;
                    sp1->Children->Append(leftHeaderSwitch);
#endif

                    focusedRestOffSwitch = ref new xaml_controls::ToggleSwitch();
                    focusedRestOffSwitch->Header = "Rest Off";
                    sp2->Children->Append(focusedRestOffSwitch);

                    focusedHoverOffSwitch = ref new xaml_controls::ToggleSwitch();
                    focusedHoverOffSwitch->Header = "Hover Off";
                    sp2->Children->Append(focusedHoverOffSwitch);

                    focusedPressedOffSwitch = ref new xaml_controls::ToggleSwitch();
                    focusedPressedOffSwitch->Header = "Pressed Off";
                    sp2->Children->Append(focusedPressedOffSwitch);

                    focusedRestOnSwitch = ref new xaml_controls::ToggleSwitch();
                    focusedRestOnSwitch->Header = "Rest On";
                    focusedRestOnSwitch->IsOn = true;
                    sp2->Children->Append(focusedRestOnSwitch);

                    focusedHoverOnSwitch = ref new xaml_controls::ToggleSwitch();
                    focusedHoverOnSwitch->Header = "Hover On";
                    focusedHoverOnSwitch->IsOn = true;
                    sp2->Children->Append(focusedHoverOnSwitch);

                    focusedPressedOnSwitch = ref new xaml_controls::ToggleSwitch();
                    focusedPressedOnSwitch->Header = "Pressed On";
                    focusedPressedOnSwitch->IsOn = true;
                    sp2->Children->Append(focusedPressedOnSwitch);

                    TestServices::WindowHelper->WindowContent = rootPanel;
                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    VisualStateManager::GoToState(hoverOffSwitch, "PointerOver", false);
                    VisualStateManager::GoToState(hoverOnSwitch, "PointerOver", false);
                    VisualStateManager::GoToState(focusedHoverOffSwitch, "PointerOver", false);
                    VisualStateManager::GoToState(focusedHoverOnSwitch, "PointerOver", false);

                    VisualStateManager::GoToState(pressedOffSwitch, "Pressed", false);
                    VisualStateManager::GoToState(pressedOnSwitch, "Pressed", false);
                    VisualStateManager::GoToState(focusedPressedOffSwitch, "Pressed", false);
                    VisualStateManager::GoToState(focusedPressedOnSwitch, "Pressed", false);

                    VisualStateManager::GoToState(focusedRestOffSwitch, "Focused", false);
                    VisualStateManager::GoToState(focusedHoverOffSwitch, "Focused", false);
                    VisualStateManager::GoToState(focusedPressedOffSwitch, "Focused", false);
                    VisualStateManager::GoToState(focusedRestOnSwitch, "Focused", false);
                    VisualStateManager::GoToState(focusedHoverOnSwitch, "Focused", false);
                    VisualStateManager::GoToState(focusedPressedOnSwitch, "Focused", false);
                });
                TestServices::WindowHelper->WaitForIdle();

                return rootPanel;
            });
    }

    void ToggleSwitchIntegrationTests::CanPanVerticallyOverToggleSwitchToScroll()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ToggleSwitch^ toggleSwitch = nullptr;

        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);
        auto viewChangedEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            auto scrollViewer = ref new xaml_controls::ScrollViewer();
            scrollViewer->Height = 300;

            viewChangedRegistration.Attach(scrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [viewChangedEvent](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                if (args->IsIntermediate == false)
                {
                    viewChangedEvent->Set();
                }
            }));

            auto stackPanel = ref new xaml_controls::StackPanel();
            stackPanel->Height = 500;
            stackPanel->Orientation = xaml_controls::Orientation::Horizontal;
            scrollViewer->Content = stackPanel;

            toggleSwitch = ref new xaml_controls::ToggleSwitch();
            toggleSwitch->IsOn = false;
            toggleSwitch->FontSize = 20; // Make sure the caption crosses the middle of the control for the PanFromCenter operation below.
            stackPanel->Children->Append(toggleSwitch);

            TestServices::WindowHelper->WindowContent = scrollViewer;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Pan to scroll Down. We use a small relX to make sure we forgive little imprecisions in the horizontal direction.
        TestServices::InputHelper->PanFromCenter(toggleSwitch, 30 /*relX*/, -150 /*relY*/, 1.0 /*velocityFactor*/);
        viewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // Verify the ToggleSwitch state has not changed.
        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(toggleSwitch->IsOn);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Pan to scroll Up. We use a small relX to make sure we forgive little imprecisions in the horizontal direction.
        TestServices::InputHelper->PanFromCenter(toggleSwitch, 30 /*relX*/, 150 /*relY*/, 1.0 /*velocityFactor*/);
        viewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // Verify the ToggleSwitch state has not changed.
        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(toggleSwitch->IsOn);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void ToggleSwitchIntegrationTests::CanPanHorizontallyOverToggleSwitchToSelect()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ToggleSwitch^ toggleSwitch = nullptr;

        auto toggleSwitchToggledEventRegistration = CreateSafeEventRegistration(xaml_controls::ToggleSwitch, Toggled);
        auto toggleSwitchToggledEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            toggleSwitch = ref new xaml_controls::ToggleSwitch();
            toggleSwitch->IsOn = false;
            toggleSwitch->Margin = xaml::Thickness({ 75, 75, 75, 75 });
            toggleSwitch->FontSize = 20; // Make sure the caption crosses the middle of the control for the PanFromCenter operation below.
            toggleSwitch->HorizontalAlignment = xaml::HorizontalAlignment::Center;

            toggleSwitchToggledEventRegistration.Attach(toggleSwitch, ref new RoutedEventHandler([&](Platform::Object^, RoutedEventArgs^)
            {
                toggleSwitchToggledEvent->Set();
            }));

            auto root = ref new xaml_controls::Grid();
            root->Children->Append(toggleSwitch);

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Pan to toggle state. We use a small relY to make sure we forgive little imprecisions in the vertical direction.
        TestServices::InputHelper->PanFromCenter(toggleSwitch, 150 /*relX*/, 30 /*relY*/, 1.0 /*velocityFactor*/);
        toggleSwitchToggledEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // Verify the ToggleSwitch state has changed to "On".
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(toggleSwitch->IsOn);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Pan to toggle state. We use a small relY to make sure we forgive little imprecisions in the vertical direction.
        TestServices::InputHelper->PanFromCenter(toggleSwitch, -150 /*relX*/, 30 /*relY*/, 1.0 /*velocityFactor*/);
        toggleSwitchToggledEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // Verify the ToggleSwitch state has changed to "Off".
        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(toggleSwitch->IsOn);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void ToggleSwitchIntegrationTests::CanDragHorizontallyOverToggleSwitchToSelect()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ToggleSwitch^ toggleSwitch = nullptr;
        xaml::FrameworkElement^ knob = nullptr;

        auto toggleSwitchToggledEventRegistration = CreateSafeEventRegistration(xaml_controls::ToggleSwitch, Toggled);
        auto toggleSwitchToggledEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            toggleSwitch = ref new xaml_controls::ToggleSwitch();
            toggleSwitch->IsOn = false;

            toggleSwitchToggledEventRegistration.Attach(toggleSwitch, ref new RoutedEventHandler([&](Platform::Object^, RoutedEventArgs^)
            {
                LOG_OUTPUT(L"ToggleSwitch.Toggled event raised.");
                toggleSwitchToggledEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = toggleSwitch;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Accessing knob element for drag gestures.");
            knob = TreeHelper::GetVisualChildByName(toggleSwitch, L"SwitchKnob");
            VERIFY_IS_NOT_NULL(knob);
        });

        LOG_OUTPUT(L"Dragging toggle diagonally to switch state On.");
        // Drag to toggle state. We use a small relY to make sure we forgive little imprecisions in the vertical direction.
        TestServices::InputHelper->DragFromCenter(knob, 20 /*relX*/, 5 /*relY*/, 1.0 /*velocityFactor*/);
        toggleSwitchToggledEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // Verify the ToggleSwitch state has changed to "On".
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(toggleSwitch->IsOn);
        });

        LOG_OUTPUT(L"Dragging toggle diagonally to switch state Off.");
        // Drag to toggle state. We use a small relY to make sure we forgive little imprecisions in the vertical direction.
        TestServices::InputHelper->DragFromCenter(knob, -20 /*relX*/, 5 /*relY*/, 1.0 /*velocityFactor*/);
        toggleSwitchToggledEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // Verify the ToggleSwitch state has changed to "Off".
        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(toggleSwitch->IsOn);
        });
    }

    void ToggleSwitchIntegrationTests::ValidateFootprint()
    {
        TestCleanupWrapper cleanup;

        double const expectedToggleSwitchWidth = 154;
        double const expectedToggleSwitchWidth_WithWideHeader = 200;
        double const expectedToggleSwitchWidth_WithWideOnOffContent = 200 + 40 + 12; //content width + switch width + gap width
        double const expectedToggleSwitchTouchableWidth = 72;

        double const expectedToggleSwitchHeight_NoHeader = 40;
        double const expectedToggleSwitchHeight_WithHeader = 23 + expectedToggleSwitchHeight_NoHeader;

        xaml_controls::ToggleSwitch^ toggleSwitch;
        xaml_controls::ToggleSwitch^ toggleSwitchWithHeader;
        xaml_controls::ToggleSwitch^ toggleSwitchWithWideHeader;
        xaml_controls::ToggleSwitch^ toggleSwitchWithWideOnContent;
        xaml_controls::ToggleSwitch^ toggleSwitchWithWideOffContent;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <ToggleSwitch x:Name="toggleSwitch" />
                        <ToggleSwitch x:Name="toggleSwitchWithHeader" Header="H" />
                        <ToggleSwitch x:Name="toggleSwitchWithWideHeader">
                            <ToggleSwitch.Header>
                                <Rectangle Height="19" Width="200" Fill="Red" />
                            </ToggleSwitch.Header>
                        </ToggleSwitch>
                        <ToggleSwitch x:Name="toggleSwitchWithWideOnContent" >
                            <ToggleSwitch.OnContent>
                                <Rectangle Height="20" Width="200" Fill="Red" />
                            </ToggleSwitch.OnContent>
                        </ToggleSwitch>
                        <ToggleSwitch x:Name="toggleSwitchWithWideOffContent" >
                            <ToggleSwitch.OffContent>
                                <Rectangle Height="20" Width="200" Fill="Red" />
                            </ToggleSwitch.OffContent>
                        </ToggleSwitch>
                    </StackPanel>)"));

            toggleSwitch = safe_cast<xaml_controls::ToggleSwitch^>(rootPanel->FindName(L"toggleSwitch"));
            toggleSwitchWithHeader = safe_cast<xaml_controls::ToggleSwitch^>(rootPanel->FindName(L"toggleSwitchWithHeader"));
            toggleSwitchWithWideHeader = safe_cast<xaml_controls::ToggleSwitch^>(rootPanel->FindName(L"toggleSwitchWithWideHeader"));
            toggleSwitchWithWideOnContent = safe_cast<xaml_controls::ToggleSwitch^>(rootPanel->FindName(L"toggleSwitchWithWideOnContent"));
            toggleSwitchWithWideOffContent = safe_cast<xaml_controls::ToggleSwitch^>(rootPanel->FindName(L"toggleSwitchWithWideOffContent"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            // Verify Footprint of ToggleSwitch:
            VERIFY_ARE_EQUAL(expectedToggleSwitchWidth, toggleSwitch->ActualWidth);
            VERIFY_ARE_EQUAL(expectedToggleSwitchHeight_NoHeader, toggleSwitch->ActualHeight);

            // Verify the touchable area of the ToggleSwitch:
            auto thumb = TreeHelper::GetVisualChildByName(toggleSwitch, L"SwitchThumb");
            VERIFY_ARE_EQUAL(expectedToggleSwitchHeight_NoHeader, thumb->ActualHeight);
            VERIFY_ARE_EQUAL(expectedToggleSwitchTouchableWidth, thumb->ActualWidth);

            // Verify Footprint of Headered ToggleSwitch:
            VERIFY_ARE_EQUAL(expectedToggleSwitchWidth, toggleSwitchWithHeader->ActualWidth);
            VERIFY_ARE_EQUAL(expectedToggleSwitchHeight_WithHeader, toggleSwitchWithHeader->ActualHeight);

            // Verify the touchable area of the Headered ToggleSwitch:
            thumb = TreeHelper::GetVisualChildByName(toggleSwitchWithHeader, L"SwitchThumb");
            VERIFY_ARE_EQUAL(expectedToggleSwitchTouchableWidth, thumb->ActualWidth);
            VERIFY_ARE_EQUAL(expectedToggleSwitchHeight_NoHeader, thumb->ActualHeight);

            // Verify footprint of ToggleSwitch with a wide header:
            VERIFY_ARE_EQUAL(expectedToggleSwitchWidth_WithWideHeader, toggleSwitchWithWideHeader->ActualWidth);
            VERIFY_ARE_EQUAL(expectedToggleSwitchHeight_WithHeader, toggleSwitchWithWideHeader->ActualHeight);

            // Verify the footprint of a ToggleSwitch with wide OnContent
            // Note: Even though the ToggleSwitch is OFF, it should still size to fit the invisible OnContent
            VERIFY_ARE_EQUAL(expectedToggleSwitchWidth_WithWideOnOffContent, toggleSwitchWithWideOnContent->ActualWidth);
            VERIFY_ARE_EQUAL(expectedToggleSwitchHeight_NoHeader, toggleSwitchWithWideOnContent->ActualHeight);

            // Verify the footprint of a ToggleSwitch with wide OffContent
            VERIFY_ARE_EQUAL(expectedToggleSwitchWidth_WithWideOnOffContent, toggleSwitchWithWideOffContent->ActualWidth);
            VERIFY_ARE_EQUAL(expectedToggleSwitchHeight_NoHeader, toggleSwitchWithWideOffContent->ActualHeight);
        });
    }

    void ToggleSwitchIntegrationTests::CanToggleUsingTapInput()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ToggleSwitch^ toggleSwitch = nullptr;

        // Create a toggle switch
        RunOnUIThread([&]()
        {
            toggleSwitch = ref new xaml_controls::ToggleSwitch();
            toggleSwitch->FontSize = 20; // Make sure the caption crosses the middle of the control for the Tap operation below.
            TestServices::WindowHelper->WindowContent = toggleSwitch;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Tap and verify control gets toggled
        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(toggleSwitch->IsOn);
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->Tap(toggleSwitch);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(toggleSwitch->IsOn);
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->Tap(toggleSwitch);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(toggleSwitch->IsOn);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void ToggleSwitchIntegrationTests::CanToggleUsingSpace()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ToggleSwitch^ toggleSwitch = nullptr;

        Event toggledEvent;
        auto toggledEventRegistration = CreateSafeEventRegistration(xaml_controls::ToggleSwitch, Toggled);

        RunOnUIThread([&]()
        {
            toggleSwitch = ref new xaml_controls::ToggleSwitch();

            toggledEventRegistration.Attach(toggleSwitch, [&]() {toggledEvent.Set(); });

            TestServices::WindowHelper->WindowContent = toggleSwitch;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            toggleSwitch->Focus(xaml::FocusState::Keyboard);
        });

        LOG_OUTPUT(L"Press Space to toggle the switch On.");
        TestServices::KeyboardHelper->Space();
        TestServices::WindowHelper->WaitForIdle();
        toggledEvent.WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(toggleSwitch->IsOn);
        });

        LOG_OUTPUT(L"Press Space to toggle the switch Off.");
        TestServices::KeyboardHelper->Space();
        TestServices::WindowHelper->WaitForIdle();
        toggledEvent.WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(toggleSwitch->IsOn);
        });
    }

    void ToggleSwitchIntegrationTests::DoesNotToggleUsingDirectionalKeys()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ToggleSwitch^ toggleSwitch = nullptr;

        Event toggledEvent;
        auto toggledEventRegistration = CreateSafeEventRegistration(xaml_controls::ToggleSwitch, Toggled);

        RunOnUIThread([&]()
        {
            toggleSwitch = ref new xaml_controls::ToggleSwitch();

            toggledEventRegistration.Attach(toggleSwitch, [&]() {toggledEvent.Set(); });

            TestServices::WindowHelper->WindowContent = toggleSwitch;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            toggleSwitch->Focus(xaml::FocusState::Keyboard);
        });

        LOG_OUTPUT(L"Press Home key and validate that the switch has not toggled.");
        RunOnUIThread([&]() { toggleSwitch->IsOn = true; });
        toggledEvent.Reset();
        TestServices::KeyboardHelper->Home();
        TestServices::WindowHelper->WaitForIdle();
        VERIFY_IS_FALSE(toggledEvent.HasFired());
        RunOnUIThread([&]() { VERIFY_IS_TRUE(toggleSwitch->IsOn); });

        LOG_OUTPUT(L"Press End key and validate that the switch has not toggled.");
        RunOnUIThread([&]() { toggleSwitch->IsOn = false; });
        toggledEvent.Reset();
        TestServices::KeyboardHelper->End();
        TestServices::WindowHelper->WaitForIdle();
        VERIFY_IS_FALSE(toggledEvent.HasFired());
        RunOnUIThread([&]() { VERIFY_IS_FALSE(toggleSwitch->IsOn); });

        LOG_OUTPUT(L"Press Up key and validate that the switch has not toggled.");
        RunOnUIThread([&]() { toggleSwitch->IsOn = true; });
        toggledEvent.Reset();
        TestServices::KeyboardHelper->Up();
        TestServices::WindowHelper->WaitForIdle();
        VERIFY_IS_FALSE(toggledEvent.HasFired());
        RunOnUIThread([&]() { VERIFY_IS_TRUE(toggleSwitch->IsOn); });

        LOG_OUTPUT(L"Press Down key and validate that the switch has not toggled.");
        RunOnUIThread([&]() { toggleSwitch->IsOn = false; });
        toggledEvent.Reset();
        TestServices::KeyboardHelper->Down();
        TestServices::WindowHelper->WaitForIdle();
        VERIFY_IS_FALSE(toggledEvent.HasFired());
        RunOnUIThread([&]() { VERIFY_IS_FALSE(toggleSwitch->IsOn); });

        LOG_OUTPUT(L"Press Left key and validate that the switch has not toggled.");
        RunOnUIThread([&]() { toggleSwitch->IsOn = true; });
        toggledEvent.Reset();
        TestServices::KeyboardHelper->Left();
        TestServices::WindowHelper->WaitForIdle();
        VERIFY_IS_FALSE(toggledEvent.HasFired());
        RunOnUIThread([&]() { VERIFY_IS_TRUE(toggleSwitch->IsOn); });

        LOG_OUTPUT(L"Press Right key and validate that the switch has not toggled.");
        RunOnUIThread([&]() { toggleSwitch->IsOn = false; });
        toggledEvent.Reset();
        TestServices::KeyboardHelper->Right();
        TestServices::WindowHelper->WaitForIdle();
        VERIFY_IS_FALSE(toggledEvent.HasFired());
        RunOnUIThread([&]() { VERIFY_IS_FALSE(toggleSwitch->IsOn); });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::ToggleSwitch
