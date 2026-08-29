// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace TimePickerFlyout {

    class TimePickerFlyoutIntegrationTests : public WEX::TestClass<TimePickerFlyoutIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(TimePickerFlyoutIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"a62e3c8d-69d4-44de-95b5-a62be5062286;57e0de30-efb3-4001-9ccc-b38032fd1974;bdc5c68b-03c1-4217-832c-4c09f99946f4")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a TimePickerFlyout.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSelectTime)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that the TimePickerFlyout can be used to select a Time.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSelectTimeWithShowAtAsync)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that the ShowAtAsync method can be used to select a Time.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DismissWithCancelButton)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that the TimePickerFlyout can be dismissed with the Cancel button.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(DoesFlyoutOnTimePickerClicked)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the launching flyout on the TimePicker.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyBackButtonClosesFlyout)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that pressing the Back button closes the flyout.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // BackButton only available in UWP
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyFlyoutResizesWithSmallHeight)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that the flyout resizes to a smaller height when a smaller height is available.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"Desktop") // Windowed popups make this test moot.
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIElementTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of TimePickerFlyout in various visual states.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanMoveBetweenColumnsWithHorizontalArrowKeys)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that using the left and right arrow keys can shift focus between the columns.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanMoveBetweenColumnsWithHorizontalArrowKeysNoPeriodPicker)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that using the left and right arrow keys can shift focus between the columns when PeriodPicker is not visible.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateAcceptDismissButtonsAreHiddenWithGamepad)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that opening a TimePickerFlyout with a gamepad causes the accept and dismiss buttons to become hidden.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOverlayBrush)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the brush used for the Overlay matches the 'TimePickerLightDismissOverlayBackground' resource.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MinuteIncrementZero)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that setting MinuteIncrement to 0 results in the expected behavior.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

    private:
        Microsoft::UI::Xaml::Controls::TimePicker^ SetupTimePickerTest();

        static wg::Calendar^ CreateCalendarTime(int hours, int minutes, int period)
        {
            auto calendar = ref new wg::Calendar();
            calendar->Hour = hours;
            calendar->Minute = minutes;
            calendar->Period = period;
            return calendar;
        }

        template <class TTimePickerControl>
        Platform::String^ FormatTime(TTimePickerControl timePickerControl);

        void SetupTimePickerFlyoutTest(xaml_controls::Button^& target, xaml_controls::TimePickerFlyout^& timePickerFlyout);

        xaml_controls::TimePickerFlyoutPresenter^ GetOpenTimePickerFlyoutPresenter();

        void ShowTimePickerFlyout(xaml_controls::TimePickerFlyout^ timePickerFlyout, xaml_controls::Button^ target);

        void HideTimePickerFlyout(xaml_controls::TimePickerFlyout^ timePickerFlyout);

        enum class Theme
        {
            Dark = 0,
            Light = 1,
            HighContrast = 2
        };
        void ValidateUIETreeWorker(xaml_primitives::FlyoutPlacementMode placementMode, Theme theme, Platform::String^ variation);

        static wf::TimeSpan CreateTimeSpan(int hours, int minutes, int period = 1);

        static wf::TimeSpan CalendarToTimeSpan(wg::Calendar^ calendar)
        {
            return CreateTimeSpan(calendar->Hour, calendar->Minute, calendar->Period);
        }
    };

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::TimePickerFlyout

