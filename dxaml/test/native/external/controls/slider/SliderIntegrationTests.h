// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <CommonInputHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Slider {

    class SliderIntegrationTests : public WEX::TestClass<SliderIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(SliderIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"1bb20c90-a558-491b-b76d-55bdb9a46911;57e0de30-efb3-4001-9ccc-b38032fd1974;bdc5c68b-03c1-4217-832c-4c09f99946f4")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a Slider.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove a Slider from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesFireRangeValueChangedEvent)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that an event is fired whenever the value is changed")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(IsRangeValueKeptBetweenMaxAndMin)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the actual value is kept between previously set max and min values.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIETreeHorizontal)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of a Horizontal Slider in various visual states ")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIETreeVertical)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of a Vertical Slider in various visual states ")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFootprint)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the ActualWidth and ActualHeight of Slider.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateThumbTooltip)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the slider's tooltip goes away when the property value is changed.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateCanMoveSliderUsingKeyboardVertical)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a vertical slider's value changes as a response to up/down keyboard presses.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateCanMoveSliderUsingKeyboardHorizontal)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a Horizontal slider's value changes as a response to left/right keyboard presses.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFireRightTappedEvent)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that right tapping/clicking a slider is handled correctly.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFocusRectangleMovesToThumbWhenEngaged)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that engaging a slider with the gamepad causes the focus rectangle to move to the thumb.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDCompTreeWhenEngaged)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the DComp tree when sliders are engaged.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyToolTipShowAndHideForKeyboardAndGamePad)
            TEST_METHOD_PROPERTY(L"Description", L"ToolTip should show on focus with keyboard, but should only show on engagement with GamePad or Remote.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifySmallRange)
            TEST_METHOD_PROPERTY(L"Description", L"Ranges <1 should have the thumb at the minimum position when Value == Minimum.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyGamepadEngagementModel)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies slider obeys A confirm, B reject gamepad engagement model")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifySliderLeavePointOverStateOnPointerCaptureLost)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies when dragging the thumb out of the control and release the button, Slider leave PointOver state")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // TODO: - Mouse input helper doesn't work on OneCore
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyIsThumbToolTipEnabledProperty)
            TEST_METHOD_PROPERTY(L"Description", L"Slider's IsThumbToolTipEnabled is ignored by the narrator")
        END_TEST_METHOD()

        TEST_METHOD(MinMaxValueSetThroughMarkupWork)

    private:
        static xaml_controls::Panel^ ValidateUIETreeTestSetup(xaml_controls::Orientation orientation);

        void ValidateCanMoveSliderUsingKeyboardHelper(xaml_controls::Orientation orientation);

        void SetupEngagementTest(xaml_controls::Slider^* horizontalSlider, xaml_controls::Slider^* verticalSlider);
        void MoveDownAndEngageSlider(xaml_controls::Slider^ slider);

        // Both of these assume that the Slider has focus:
        void EngageSlider(xaml_controls::Slider^ slider, InputDevice inputDevice);
        void DisengageSlider(xaml_controls::Slider^ slider, InputDevice inputDevice);
        void CommitSlider(xaml_controls::Slider^ slider, InputDevice inputDevice);

        void VerifyNoOpenToolTips(xaml_controls::Slider^ slider);
        void VerifyToolTipOpen(xaml_controls::Slider^ slider);

        void DoVerifyToolTipShowAndHide(InputDevice inputDevice, bool isFocusEngagementEnabledOnSlider, bool toolTipShouldOnlyShowWhenEngaged);
    };

} } } } } }
