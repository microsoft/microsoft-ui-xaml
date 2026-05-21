// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "SliderIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include "FeatureFlags.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>
#include <generic\RangeBaseTests.h>

#include <ControlHelper.h>
#include <CommonInputHelper.h>
#include <TreeHelper.h>
#include <WUCRenderingScopeGuard.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Slider {

    bool SliderIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool SliderIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool SliderIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void SliderIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::Slider>::CanInstantiate();
    }

    void SliderIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::Slider>::CanEnterAndLeaveLiveTree();
    }

    void SliderIntegrationTests::DoesFireRangeValueChangedEvent()
    {
        Generic::RangeBaseTests<xaml_controls::Slider>::DoesFireRangeValueChangedEvent();
    }

    void SliderIntegrationTests::IsRangeValueKeptBetweenMaxAndMin()
    {
        Generic::RangeBaseTests<xaml_controls::Slider>::IsRangeValueKeptBetweenMaxAndMin();
    }

    void SliderIntegrationTests::ValidateUIETreeHorizontal()
    {
        ControlHelper::ValidateUIElementTree(
            wf::Size(400, 700),
            1.f,
            []()
            {
                return ValidateUIETreeTestSetup(xaml_controls::Orientation::Horizontal);
            });
    }

    void SliderIntegrationTests::ValidateUIETreeVertical()
    {
        ControlHelper::ValidateUIElementTree(
            wf::Size(400, 700),
            1.f,
            []()
            {
                return ValidateUIETreeTestSetup(xaml_controls::Orientation::Vertical);
            });
    }

    xaml_controls::Panel^ SliderIntegrationTests::ValidateUIETreeTestSetup(xaml_controls::Orientation orientation)
    {
        xaml_controls::Slider^ restSlider = nullptr;
        xaml_controls::Slider^ hoverSlider = nullptr;
        xaml_controls::Slider^ pressedSlider = nullptr;
        xaml_controls::Slider^ disabledSlider = nullptr;

        xaml_controls::Slider^ tickInlineSlider = nullptr;
        xaml_controls::Slider^ tickOutsideSlider = nullptr;
        xaml_controls::Slider^ topLeftTicksSlider = nullptr;
        xaml_controls::Slider^ bottomRightTicksSlider = nullptr;

#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
        xaml_controls::Slider^ leftHeaderSlider = nullptr;
#endif

        xaml_controls::StackPanel^ rootPanel = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = ref new xaml_controls::StackPanel();
            rootPanel->IsHitTestVisible = false;

            // We split the Sliders across multiple panel so that we can get two rows
            // of vertical Sliders to fit on screen.
            auto commonStatesPanel = ref new xaml_controls::StackPanel();
            auto ticksPlacementPanel = ref new xaml_controls::StackPanel();
#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
            auto headerPlacementPanel = ref new xaml_controls::StackPanel();
#endif
            rootPanel->Children->Append(commonStatesPanel);
            rootPanel->Children->Append(ticksPlacementPanel);
#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
            rootPanel->Children->Append(headerPlacementPanel);
#endif

            if (orientation == xaml_controls::Orientation::Vertical)
            {
                commonStatesPanel->Orientation = xaml_controls::Orientation::Horizontal;
                commonStatesPanel->Height = 300;
                ticksPlacementPanel->Orientation = xaml_controls::Orientation::Horizontal;
                ticksPlacementPanel->Height = 300;
#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
                headerPlacementPanel->Orientation = xaml_controls::Orientation::Horizontal;
                headerPlacementPanel->Height = 100;
#endif
            }

            restSlider = ref new xaml_controls::Slider();
            restSlider->Header = "Rest Slider";
            restSlider->Value = 50;
            restSlider->Orientation = orientation;
            commonStatesPanel->Children->Append(restSlider);

            hoverSlider = ref new xaml_controls::Slider();
            hoverSlider->Header = "Hover Slider";
            hoverSlider->Value = 50;
            hoverSlider->Orientation = orientation;
            commonStatesPanel->Children->Append(hoverSlider);

            pressedSlider = ref new xaml_controls::Slider();
            pressedSlider->Header = "Pressed Slider";
            pressedSlider->Value = 50;
            pressedSlider->Orientation = orientation;
            commonStatesPanel->Children->Append(pressedSlider);

            disabledSlider = ref new xaml_controls::Slider();
            disabledSlider->Header = "Disabled Slider";
            disabledSlider->Value = 50;
            disabledSlider->IsEnabled = false;
            disabledSlider->Orientation = orientation;
            commonStatesPanel->Children->Append(disabledSlider);

            // Tick placement

            tickInlineSlider = ref new xaml_controls::Slider();
            tickInlineSlider->Header = "Tick Inline";
            tickInlineSlider->Value = 50;
            tickInlineSlider->TickPlacement = xaml_primitives::TickPlacement::Inline;
            tickInlineSlider->TickFrequency = 10;
            tickInlineSlider->Orientation = orientation;
            ticksPlacementPanel->Children->Append(tickInlineSlider);

            tickOutsideSlider = ref new xaml_controls::Slider();
            tickOutsideSlider->Header = "Tick Outside";
            tickOutsideSlider->Value = 50;
            tickOutsideSlider->TickPlacement = xaml_primitives::TickPlacement::Outside;
            tickOutsideSlider->TickFrequency = 10;
            tickOutsideSlider->Orientation = orientation;
            ticksPlacementPanel->Children->Append(tickOutsideSlider);

            topLeftTicksSlider = ref new xaml_controls::Slider();
            topLeftTicksSlider->Header = "Ticks TopLeft";
            topLeftTicksSlider->Value = 0;
            topLeftTicksSlider->TickPlacement = xaml_primitives::TickPlacement::TopLeft;
            topLeftTicksSlider->TickFrequency = 10;
            topLeftTicksSlider->Orientation = orientation;
            ticksPlacementPanel->Children->Append(topLeftTicksSlider);

            bottomRightTicksSlider = ref new xaml_controls::Slider();
            bottomRightTicksSlider->Header = "Ticks BottomRight";
            bottomRightTicksSlider->Value = 100;
            bottomRightTicksSlider->TickPlacement = xaml_primitives::TickPlacement::BottomRight;
            bottomRightTicksSlider->TickFrequency = 10;
            bottomRightTicksSlider->Orientation = orientation;
            ticksPlacementPanel->Children->Append(bottomRightTicksSlider);

#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
            // Header placement
            leftHeaderSlider = ref new xaml_controls::Slider();
            leftHeaderSlider->Header = "Left Header";
            leftHeaderSlider->HeaderPlacement = xaml_controls::ControlHeaderPlacement::Left;
            leftHeaderSlider->Value = 50;
            leftHeaderSlider->Orientation = orientation;
            headerPlacementPanel->Children->Append(leftHeaderSlider);
#endif

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VisualStateManager::GoToState(hoverSlider, "PointerOver", false);
            VisualStateManager::GoToState(pressedSlider, "Pressed", false);
        });
        TestServices::WindowHelper->WaitForIdle();

        return rootPanel;
    }

    void SliderIntegrationTests::ValidateFocusRectangleMovesToThumbWhenEngaged()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Slider^ horizontalSlider = nullptr;
        xaml_controls::Slider^ verticalSlider = nullptr;

        xaml::FrameworkElement^ horizontalSliderFocusBorder = nullptr;
        xaml::FrameworkElement^ horizontalSliderHorizontalThumb = nullptr;
        xaml::FrameworkElement^ horizontalSliderVerticalThumb = nullptr;
        xaml::FrameworkElement^ verticalSliderFocusBorder = nullptr;
        xaml::FrameworkElement^ verticalSliderHorizontalThumb = nullptr;
        xaml::FrameworkElement^ verticalSliderVerticalThumb = nullptr;

        SetupEngagementTest(&horizontalSlider, &verticalSlider);

        RunOnUIThread([&]()
        {
            horizontalSliderFocusBorder = TreeHelper::GetVisualChildByName(horizontalSlider, L"FocusBorder");
            horizontalSliderHorizontalThumb = TreeHelper::GetVisualChildByName(horizontalSlider, L"HorizontalThumb");
            horizontalSliderVerticalThumb = TreeHelper::GetVisualChildByName(horizontalSlider, L"VerticalThumb");
            verticalSliderFocusBorder = TreeHelper::GetVisualChildByName(verticalSlider, L"FocusBorder");
            verticalSliderHorizontalThumb = TreeHelper::GetVisualChildByName(verticalSlider, L"HorizontalThumb");
            verticalSliderVerticalThumb = TreeHelper::GetVisualChildByName(verticalSlider, L"VerticalThumb");

            VERIFY_IS_TRUE(xaml_controls::Control::GetIsTemplateFocusTarget(horizontalSliderFocusBorder));
            VERIFY_IS_FALSE(xaml_controls::Control::GetIsTemplateFocusTarget(horizontalSliderHorizontalThumb));
            VERIFY_IS_FALSE(xaml_controls::Control::GetIsTemplateFocusTarget(horizontalSliderVerticalThumb));
        });

        MoveDownAndEngageSlider(horizontalSlider);

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(xaml_controls::Control::GetIsTemplateFocusTarget(horizontalSliderFocusBorder));
            VERIFY_IS_TRUE(xaml_controls::Control::GetIsTemplateFocusTarget(horizontalSliderHorizontalThumb));
            VERIFY_IS_FALSE(xaml_controls::Control::GetIsTemplateFocusTarget(horizontalSliderVerticalThumb));
        });

        DisengageSlider(horizontalSlider, InputDevice::Gamepad);

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(xaml_controls::Control::GetIsTemplateFocusTarget(horizontalSliderFocusBorder));
            VERIFY_IS_FALSE(xaml_controls::Control::GetIsTemplateFocusTarget(horizontalSliderHorizontalThumb));
            VERIFY_IS_FALSE(xaml_controls::Control::GetIsTemplateFocusTarget(horizontalSliderVerticalThumb));

            VERIFY_IS_TRUE(xaml_controls::Control::GetIsTemplateFocusTarget(verticalSliderFocusBorder));
            VERIFY_IS_FALSE(xaml_controls::Control::GetIsTemplateFocusTarget(verticalSliderHorizontalThumb));
            VERIFY_IS_FALSE(xaml_controls::Control::GetIsTemplateFocusTarget(verticalSliderVerticalThumb));
        });

        MoveDownAndEngageSlider(verticalSlider);

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(xaml_controls::Control::GetIsTemplateFocusTarget(verticalSliderFocusBorder));
            VERIFY_IS_FALSE(xaml_controls::Control::GetIsTemplateFocusTarget(verticalSliderHorizontalThumb));
            VERIFY_IS_TRUE(xaml_controls::Control::GetIsTemplateFocusTarget(verticalSliderVerticalThumb));
        });

        DisengageSlider(verticalSlider, InputDevice::Gamepad);

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(xaml_controls::Control::GetIsTemplateFocusTarget(verticalSliderFocusBorder));
            VERIFY_IS_FALSE(xaml_controls::Control::GetIsTemplateFocusTarget(verticalSliderHorizontalThumb));
            VERIFY_IS_FALSE(xaml_controls::Control::GetIsTemplateFocusTarget(verticalSliderVerticalThumb));
        });
    }

    void SliderIntegrationTests::ValidateDCompTreeWhenEngaged()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

        xaml_controls::Slider^ horizontalSlider = nullptr;
        xaml_controls::Slider^ verticalSlider = nullptr;

        SetupEngagementTest(&horizontalSlider, &verticalSlider);

        MoveDownAndEngageSlider(horizontalSlider);

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "Horizontal");

        DisengageSlider(horizontalSlider, InputDevice::Gamepad);
        MoveDownAndEngageSlider(verticalSlider);

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "Vertical");

        DisengageSlider(verticalSlider, InputDevice::Gamepad);
    }

    void SliderIntegrationTests::ValidateFootprint()
    {
        TestCleanupWrapper cleanup;

        double const expectedSliderWidth = 200;
        double const expectedSliderWidth_Vertical = 32;
        double const expectedSliderWidth_VerticalWithWideHeader = expectedSliderWidth_Vertical + 43;

        double const expectedSliderHeight = 32;
        double const expectedSliderHeight_WithHeader = 19 + 4 + expectedSliderHeight;
        double const expectedSliderHeight_Vertical = 200;

        xaml_controls::Slider^ slider;
        xaml_controls::Slider^ verticalSlider;
        xaml_controls::Slider^ sliderWithHeader;
        xaml_controls::Slider^ verticalSliderWithWideHeader;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Width="200" >
                        <Slider x:Name="slider" />
                        <Slider x:Name="sliderWithHeader" Header="Header" />
                        <StackPanel Orientation="Horizontal" Height="200" >
                            <Slider x:Name="verticalSlider" Orientation="Vertical" />
                            <Slider x:Name="verticalSliderWithWideHeader" Orientation="Vertical" >
                                <Slider.Header>
                                    <Rectangle Fill="Red" Height="20" Width="75" />
                                </Slider.Header>
                            </Slider>
                        </StackPanel>
                    </StackPanel>)"));

            slider = safe_cast<xaml_controls::Slider^>(rootPanel->FindName(L"slider"));
            verticalSlider = safe_cast<xaml_controls::Slider^>(rootPanel->FindName(L"verticalSlider"));
            sliderWithHeader = safe_cast<xaml_controls::Slider^>(rootPanel->FindName(L"sliderWithHeader"));
            verticalSliderWithWideHeader = safe_cast<xaml_controls::Slider^>(rootPanel->FindName(L"verticalSliderWithWideHeader"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            // Verify Footprint of Slider:
            VERIFY_ARE_EQUAL(expectedSliderWidth, slider->ActualWidth);
            VERIFY_ARE_EQUAL(expectedSliderHeight, slider->ActualHeight);

            // Verify Footprint of Vertical Slider:
            VERIFY_ARE_EQUAL(expectedSliderWidth_Vertical, verticalSlider->ActualWidth);
            VERIFY_ARE_EQUAL(expectedSliderHeight_Vertical, verticalSlider->ActualHeight);

            // Verify Footprint of Slider with Header:
            VERIFY_ARE_EQUAL(expectedSliderWidth, sliderWithHeader->ActualWidth);
            VERIFY_ARE_EQUAL(expectedSliderHeight_WithHeader, sliderWithHeader->ActualHeight);

            // Verify Footprint of Vertical Slider with Header:
            VERIFY_ARE_EQUAL(expectedSliderWidth_VerticalWithWideHeader, verticalSliderWithWideHeader->ActualWidth);
            VERIFY_ARE_EQUAL(expectedSliderHeight_Vertical, verticalSliderWithWideHeader->ActualHeight);
        });
    }

    void SliderIntegrationTests::ValidateThumbTooltip()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Slider^ slider;
        xaml_primitives::Thumb^ thumb;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Width="200" HorizontalAlignment="Center" VerticalAlignment="Center">
                        <Slider x:Name="slider" Orientation="Vertical" Maximum="20" Minimum="0" Value="10" Width="200" Height="200"/>
                </StackPanel>)"
            ));

            slider = safe_cast<xaml_controls::Slider^>(rootPanel->FindName(L"slider"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            thumb = safe_cast<xaml_primitives::Thumb^>(TreeHelper::GetVisualChildByName(slider, L"VerticalThumb"));
        });

        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->MouseButtonDown(thumb, 0, 0, MouseButton::Left);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(slider->XamlRoot);
            auto popup = popups->GetAt(0);
            auto toolTip = popup->Child;
            wf::Rect toolTipBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(toolTip));

            VERIFY_IS_TRUE(toolTipBounds.Left > 0);
            VERIFY_IS_TRUE(slider->IsThumbToolTipEnabled);
        });

        TestServices::InputHelper->MouseButtonUp(thumb, 0, 0, MouseButton::Left);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            slider->IsThumbToolTipEnabled = false;
        });

        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->MouseButtonDown(thumb, 0, 0, MouseButton::Left);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(slider->XamlRoot);
            auto popup = popups->GetAt(0);
            auto toolTip = popup->Child;
            wf::Rect toolTipBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(toolTip));

            VERIFY_IS_TRUE(toolTipBounds.Left == 0);
            VERIFY_IS_FALSE(slider->IsThumbToolTipEnabled);
        });

        TestServices::InputHelper->MouseButtonUp(thumb, 0, 0, MouseButton::Left);
        TestServices::WindowHelper->WaitForIdle();
    }

    void SliderIntegrationTests::VerifyToolTipShowAndHideForKeyboardAndGamePad()
    {
        TestCleanupWrapper cleanup;

        DoVerifyToolTipShowAndHide(InputDevice::Gamepad,  true  /*isFocusEngagementEnabledOnSlider*/, true  /*toolTipShouldOnlyShowWhenEngaged*/);
        DoVerifyToolTipShowAndHide(InputDevice::Keyboard, true  /*isFocusEngagementEnabledOnSlider*/, false /*toolTipShouldOnlyShowWhenEngaged*/);
        DoVerifyToolTipShowAndHide(InputDevice::Gamepad,  false /*isFocusEngagementEnabledOnSlider*/, false /*toolTipShouldOnlyShowWhenEngaged*/);
        DoVerifyToolTipShowAndHide(InputDevice::Keyboard, false /*isFocusEngagementEnabledOnSlider*/, false /*toolTipShouldOnlyShowWhenEngaged*/);
    }

    void SliderIntegrationTests::VerifySmallRange()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Slider^ slider = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Width="200" HorizontalAlignment="Center" VerticalAlignment="Center">
                        <Slider x:Name="slider" Orientation="Horizontal" Maximum="0.9" Minimum="0.1" Value="0.1" Width="200" Height="200"/>
                </StackPanel>)"
            ));

            slider = safe_cast<xaml_controls::Slider^>(rootPanel->FindName(L"slider"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        xaml::FrameworkElement^ decreaseRect = nullptr;
        RunOnUIThread([&]()
        {
            decreaseRect = TreeHelper::GetVisualChildByName(slider, L"HorizontalDecreaseRect");
            VERIFY_ARE_EQUAL(decreaseRect->ActualWidth, 0);
        });
    }

    void SliderIntegrationTests::VerifyGamepadEngagementModel()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Button^ buttonForFocus;
        xaml_controls::Slider^ slider;
        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' HorizontalAlignment='Center' VerticalAlignment='Center'>
                        <Button x:Name='buttonForFocus' Content='Initial focus here'/>
                        <Slider x:Name="slider" Orientation="Horizontal" IsFocusEngagementEnabled="True" Maximum="20" Minimum="0" Value="10" Width="200" Height="200"/>
                </StackPanel>)"
            ));
            buttonForFocus = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"buttonForFocus"));
            slider = safe_cast<xaml_controls::Slider^>(rootPanel->FindName(L"slider"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();
        ControlHelper::EnsureFocused(buttonForFocus);

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(slider->Value, 10);
        });

        LOG_OUTPUT(L"Going down and engaging slider"); // test output due to a KeyUp bug is really verbose right now, so I have a lot of markers of test progress
        MoveDownAndEngageSlider(slider);

        LOG_OUTPUT(L"Going to press right on engaged slider");
        CommonInputHelper::Right(InputDevice::Gamepad);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(slider->Value, 11);
        });

        LOG_OUTPUT(L"Going to disengage slider");
        DisengageSlider(slider, InputDevice::Gamepad);

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(slider->Value, 10);
        });

        LOG_OUTPUT(L"Press right on an unengaged slider");

        CommonInputHelper::Right(InputDevice::Gamepad);
        TestServices::WindowHelper->WaitForIdle();

        // The above right shouldn't commit the value
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(slider->Value, 10);
        });

        LOG_OUTPUT(L"Returning focus to slider");

        // Return focus back to slider
        ControlHelper::EnsureFocused(slider);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Going to engage slider [2]");

        EngageSlider(slider, InputDevice::Gamepad);

        LOG_OUTPUT(L"Pressing right on an engaged slider");

        CommonInputHelper::Right(InputDevice::Gamepad);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(slider->Value, 11);
        });

        LOG_OUTPUT(L"Going to positively 'disengage' with slider to commit the value");

        CommitSlider(slider, InputDevice::Gamepad);

        // Ensure value committed
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(slider->Value, 11);
        });

        LOG_OUTPUT(L"Going to engage slider [3]");

        EngageSlider(slider, InputDevice::Gamepad);

        CommonInputHelper::Right(InputDevice::Gamepad);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            slider->RemoveFocusEngagement();
        });

        TestServices::WindowHelper->WaitForIdle();

        // Ensure value discarded
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(slider->Value, 11);
        });
    }

    void SliderIntegrationTests::DoVerifyToolTipShowAndHide(InputDevice inputDevice, bool isFocusEngagementEnabledOnSlider, bool toolTipShouldOnlyShowWhenEngaged)
    {
        xaml_controls::Button^ buttonForFocus;
        xaml_controls::Slider^ slider;
        std::shared_ptr<Event> gotFocusEvent = std::make_shared<Event>();
        auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Slider, GotFocus);

        LOG_OUTPUT(L"DoVerifyToolTipShowAndHide inputDevice=%s, isFocusEngagementEnabledOnSlider=%d, toolTipShouldOnlyShowWhenEngaged=%d", inputDevice.ToString()->Data(), isFocusEngagementEnabledOnSlider, toolTipShouldOnlyShowWhenEngaged);

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' HorizontalAlignment='Center' VerticalAlignment='Center'>
                        <Button x:Name='buttonForFocus' Content='Initial focus here'/>
                        <Slider x:Name='slider' />
                    </StackPanel>)"));
            buttonForFocus = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"buttonForFocus"));
            slider = safe_cast<xaml_controls::Slider^>(rootPanel->FindName(L"slider"));

            slider->IsFocusEngagementEnabled = isFocusEngagementEnabledOnSlider;
            gotFocusRegistration.Attach(slider, [&](){ gotFocusEvent->Set(); });

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();
        ControlHelper::EnsureFocused(buttonForFocus);

        // Move focus to slider using input device:
        if (inputDevice == InputDevice::Keyboard)
        {
            TestServices::KeyboardHelper->Tab();
        }
        else
        {
            CommonInputHelper::Down(inputDevice);
        }
        gotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        if (toolTipShouldOnlyShowWhenEngaged)
        {
            VerifyNoOpenToolTips(slider);
            EngageSlider(slider, inputDevice);
            VerifyToolTipOpen(slider);
            DisengageSlider(slider, inputDevice);
            VerifyNoOpenToolTips(slider);
        }
        else
        {
            VerifyToolTipOpen(slider);
        }

        // Move focus away from Slider:
        ControlHelper::EnsureFocused(buttonForFocus);

        VerifyNoOpenToolTips(slider);
    }

    void SliderIntegrationTests::ValidateCanMoveSliderUsingKeyboardVertical()
    {
        ValidateCanMoveSliderUsingKeyboardHelper(xaml_controls::Orientation::Vertical);
    }

    void SliderIntegrationTests::ValidateCanMoveSliderUsingKeyboardHorizontal()
    {
        ValidateCanMoveSliderUsingKeyboardHelper(xaml_controls::Orientation::Horizontal);
    }

    void SliderIntegrationTests::ValidateCanMoveSliderUsingKeyboardHelper(xaml_controls::Orientation orientation)
    {
        TestCleanupWrapper cleanup;

        xaml_controls::StackPanel^ rootPanel;
        xaml_controls::Slider^ slider;
        xaml_controls::Button^ button;

        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
               LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Width="200" HorizontalAlignment="Center" VerticalAlignment="Center">
                        <Slider x:Name="slider" Maximum="20" Minimum="0" Value="10" Width="200" Height="200" StepFrequency="1"/>
                        <Button x:Name="button" Content="Focus Me"/>
                </StackPanel>)"
            ));

            slider = safe_cast<xaml_controls::Slider^>(rootPanel->FindName(L"slider"));
            button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            slider->Orientation = orientation;
            slider->Focus(FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Increment the slider using the keyboard.
        if (orientation == xaml_controls::Orientation::Vertical)
        {
            TestServices::KeyboardHelper->Up();
        }
        else
        {
            TestServices::KeyboardHelper->Right();
        }
        TestServices::WindowHelper->WaitForIdle();

        // Verify that the slider has been incremented.
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(slider->Value, 11);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Undo the increment by doing the inverse operation
        if (orientation == xaml_controls::Orientation::Vertical)
        {
            TestServices::KeyboardHelper->Down();
        }
        else
        {
            TestServices::KeyboardHelper->Left();
        }
        TestServices::WindowHelper->WaitForIdle();

        // Verify that the value reset.
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(slider->Value, 10);
        });

        // Unfocus the slider to remove the tooltip popup. If we don't unfocus, the popup will remain up and crater TAEF.
        RunOnUIThread([&]()
        {
            button->Focus(FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void SliderIntegrationTests::ValidateFireRightTappedEvent()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Slider^ slider;
        auto rightTappedEvent = std::make_shared<Event>();
        auto rightTappedRegistration = CreateSafeEventRegistration(xaml_controls::Slider, RightTapped);

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Width="200" HorizontalAlignment="Center" VerticalAlignment="Center">
                        <Slider x:Name="slider" Orientation="Vertical" Maximum="20" Minimum="0" Value="10" Width="200" Height="200"/>
                </StackPanel>)"
            ));

            slider = safe_cast<xaml_controls::Slider^>(rootPanel->FindName(L"slider"));

            TestServices::WindowHelper->WindowContent = rootPanel;

            rightTappedRegistration.Attach(slider, ref new xaml::Input::RightTappedEventHandler([rightTappedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e) {
                LOG_OUTPUT(L"Slider has been Right Tapped");
                rightTappedEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Right-tapping the Slider.");
        TestServices::InputHelper->Hold(slider);
        rightTappedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void SliderIntegrationTests::MoveDownAndEngageSlider(xaml_controls::Slider^ slider)
    {
        std::shared_ptr<Event> gotFocusEvent = std::make_shared<Event>();
        auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Slider, GotFocus);
        gotFocusRegistration.Attach(slider, [&](){ gotFocusEvent->Set(); });

        std::shared_ptr<Event> focusEngagedEvent = std::make_shared<Event>();
        auto focusEngagedRegistration = CreateSafeEventRegistration(xaml_controls::Slider, FocusEngaged);
        focusEngagedRegistration.Attach(slider, [&](){ focusEngagedEvent->Set(); });

        CommonInputHelper::Down(InputDevice::Gamepad);

        gotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        CommonInputHelper::Accept(InputDevice::Gamepad);

        focusEngagedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void SliderIntegrationTests::DisengageSlider(xaml_controls::Slider^ slider, InputDevice inputDevice)
    {
        std::shared_ptr<Event> focusDisengagedEvent = std::make_shared<Event>();
        auto focusDisngagedRegistration = CreateSafeEventRegistration(xaml_controls::Slider, FocusDisengaged);
        focusDisngagedRegistration.Attach(slider, [&](){ focusDisengagedEvent->Set(); });

        CommonInputHelper::Cancel(inputDevice);

        focusDisengagedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void SliderIntegrationTests::CommitSlider(xaml_controls::Slider^ slider, InputDevice inputDevice)
    {
        std::shared_ptr<Event> focusDisengagedEvent = std::make_shared<Event>();
        auto focusDisngagedRegistration = CreateSafeEventRegistration(xaml_controls::Slider, FocusDisengaged);
        focusDisngagedRegistration.Attach(slider, [&]() { focusDisengagedEvent->Set(); });

        CommonInputHelper::Accept(inputDevice);

        focusDisengagedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }
    void SliderIntegrationTests::SetupEngagementTest(xaml_controls::Slider^* horizontalSlider, xaml_controls::Slider^* verticalSlider)
    {
        xaml_controls::Button^ buttonForFocus = nullptr;
        xaml_controls::Slider^ horizontalSliderElement = nullptr;
        xaml_controls::Slider^ verticalSliderElement = nullptr;

        std::shared_ptr<Event> loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
               LR"(<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' HorizontalAlignment='Center' VerticalAlignment='Center'>
                        <Button x:Name='buttonForFocus' Content='Initial focus here'/>
                        <Slider x:Name='horizontalSlider' Maximum='20' Minimum='0' Value='10' Width='100' Height='100' StepFrequency='1'/>
                        <Slider x:Name='verticalSlider' Orientation='Vertical' Maximum='20' Minimum='0' Value='10' Width='100' Height='100' StepFrequency='1'/>
                </StackPanel>)"
            ));

            loadedRegistration.Attach(rootPanel, [&](){ loadedEvent->Set(); });

            buttonForFocus = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"buttonForFocus"));
            horizontalSliderElement = safe_cast<xaml_controls::Slider^>(rootPanel->FindName(L"horizontalSlider"));
            verticalSliderElement = safe_cast<xaml_controls::Slider^>(rootPanel->FindName(L"verticalSlider"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();
        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            buttonForFocus->Focus(xaml::FocusState::Programmatic);
        });

        TestServices::WindowHelper->WaitForIdle();

        *horizontalSlider = horizontalSliderElement;
        *verticalSlider = verticalSliderElement;
    }

    void SliderIntegrationTests::EngageSlider(xaml_controls::Slider^ slider, InputDevice inputDevice)
    {
        std::shared_ptr<Event> focusEngagedEvent = std::make_shared<Event>();
        auto focusEngagedRegistration = CreateSafeEventRegistration(xaml_controls::Slider, FocusEngaged);
        focusEngagedRegistration.Attach(slider, [&](){ focusEngagedEvent->Set(); });

        CommonInputHelper::Accept(inputDevice);

        focusEngagedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void SliderIntegrationTests::VerifyNoOpenToolTips(xaml_controls::Slider^ slider)
    {
        RunOnUIThread([&]()
        {
            auto currentToolTip = TreeHelper::GetVisualChildByTypeFromOpenPopups<xaml_controls::ToolTip>(slider);
            VERIFY_IS_TRUE(currentToolTip == nullptr, L"There should be no open ToolTips");
        });
    }

    void SliderIntegrationTests::VerifyToolTipOpen(xaml_controls::Slider^ slider)
    {
        RunOnUIThread([&]()
        {
            auto openPopups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(slider->XamlRoot);
            VERIFY_ARE_EQUAL(1u, openPopups->Size, L"There should be exactly one open popup");
            auto currentToolTip = TreeHelper::GetVisualChildByTypeFromOpenPopups<xaml_controls::ToolTip>(slider);
            VERIFY_IS_TRUE(currentToolTip != nullptr, L"There should be an open ToolTip");
        });
    }

    void SliderIntegrationTests::VerifySliderLeavePointOverStateOnPointerCaptureLost()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Slider^ slider;
        xaml_primitives::Thumb^ thumb;

        TestServices::InputHelper->MoveMouse(wf::Point(0, 0));

        auto pointerEnteredEvent = std::make_shared<Event>();
        auto pointerEnteredRegistration = CreateSafeEventRegistration(xaml_controls::Slider, PointerEntered);

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Width="200" HorizontalAlignment="Center" VerticalAlignment="Center">
                        <Button Content="PlaceHolder" Width="100" Height="100"/>
                        <Slider x:Name="slider" Orientation="Vertical" Maximum="20" Minimum="0" Value="10" Width="100" Height="100"/>
                </StackPanel>)"
            ));

            slider = safe_cast<xaml_controls::Slider^>(rootPanel->FindName(L"slider"));
            pointerEnteredRegistration.Attach(slider, [&]() { pointerEnteredEvent->Set(); });

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            thumb = safe_cast<xaml_primitives::Thumb^>(TreeHelper::GetVisualChildByName(slider, L"VerticalThumb"));
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Move mouse to slider.");
        TestServices::InputHelper->MoveMouse(slider);
        pointerEnteredEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_TRUE(ControlHelper::IsInVisualState(slider, L"CommonStates", L"PointerOver"));

        LOG_OUTPUT(L"Dragging thumb out of the control.");
        TestServices::InputHelper->DragFromCenter(thumb, 0 /* relX */, -150 /* relY */, 0.5 /* velocityFactor */);

        TestServices::WindowHelper->WaitForIdle();

        // Not in PointerOver anymore
        VERIFY_IS_FALSE(ControlHelper::IsInVisualState(slider, L"CommonStates", L"PointerOver"));
    }

    void SliderIntegrationTests::MinMaxValueSetThroughMarkupWork()
    {
        Generic::RangeBaseTests<xaml_controls::Slider>::MinMaxValueSetThroughMarkupWork(L"Slider");
    }

    void SliderIntegrationTests::VerifyIsThumbToolTipEnabledProperty()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ buttonForFocus;
        xaml_controls::Slider^ slider;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' HorizontalAlignment='Center' VerticalAlignment='Center'>
                        <Button x:Name='buttonForFocus' Content='Initial focus here'/>
                        <Slider x:Name='slider' />
                    </StackPanel>)"));
            buttonForFocus = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"buttonForFocus"));
            slider = safe_cast<xaml_controls::Slider^>(rootPanel->FindName(L"slider"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();
        ControlHelper::EnsureFocused(buttonForFocus);

        LOG_OUTPUT(L"Move focus to slider");
        //We need to use tab key to change focus here because ControlHelper::EnsureFocused is using xaml::FocusState::Programmatic to set focus, which doesn't trigger the event to popup tooltip.
        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Verify ThumbToolTip is enabled by default");
        VerifyToolTipOpen(slider);

        LOG_OUTPUT(L"Move focus to button");
        ControlHelper::EnsureFocused(buttonForFocus);
        LOG_OUTPUT(L"Verify ThumbToolTip is closed when out of focus");
        VerifyNoOpenToolTips(slider);

        RunOnUIThread([&]()
        {
            slider->IsThumbToolTipEnabled = false;
        });

        LOG_OUTPUT(L"Move focus to slider");
        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Verify ThumbToolTip is disabled");
        VerifyNoOpenToolTips(slider);
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::Slider
