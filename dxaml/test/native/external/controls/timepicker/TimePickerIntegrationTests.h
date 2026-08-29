// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace TimePicker {

    class TimePickerIntegrationTests : public WEX::TestClass<TimePickerIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(TimePickerIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"1bb20c90-a558-491b-b76d-55bdb9a46911;57e0de30-efb3-4001-9ccc-b38032fd1974;bdc5c68b-03c1-4217-832c-4c09f99946f4")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a TimePicker.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyDefaultProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the TimePicker's default properties.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanFireTimeChangedEvent)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the DatePicker fires TimeChanged event.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIElementTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of TimePicker in various visual states ")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFootprint)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the ActualWidth and ActualHeight of TimePicker.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFlyoutPositioningAndSizing)
            TEST_METHOD_PROPERTY(L"Description", L"Validate that the TimePickerFlyout is positioned and sized correctly")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateMinuteIncrementProperty)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that the MinuteIncrement property works correctly.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateClockIdentifierProperty)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that the ClockIdentifier property works correctly.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanOpenAndCloseUsingKeyboardProjectedShadow)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the TimePicker can be opened and closed with the keyboard, projected shadow mode.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore,Santorini")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // We're on drop shadows now
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanOpenAndCloseUsingKeyboardDropShadow)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the TimePicker can be opened and closed with the keyboard, drop shadow mode.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore,Santorini")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"Ignore", L"True") // Disabled due to #36633792
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(HasPlaceholderTextByDefault)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the TimePicker has placeholder text and SelectedTime = null by default.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SelectingTimeSetsSelectedTime)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that selecting a time from the TimePicker sets SelectedTime to a non-null value.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSelectedTimePropagatesToTime)
            TEST_METHOD_PROPERTY(L"Description", L"Validates setting SelectedTime propagates that value to Time, and vice-versa when Time is set.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanProgrammaticallyClearSelectedTime)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting SelectedTime to null brings back the placeholder text.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateAutomationNameWhenTimeIsNull)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that the TimePicker's Automation Name is empty when no time is selected")
        END_TEST_METHOD()

    private:
        Microsoft::UI::Xaml::Controls::Grid^ CreateRootGrid();
        Microsoft::UI::Xaml::Controls::TimePicker^ SetupTimePickerTest();

        static xaml_controls::Button^ GetFlyoutButtonFromTimePicker(xaml_controls::TimePicker^ timePicker);

        static wg::Calendar^ CreateTime(int hours, int minutes, int period = 1)
        {
            auto time = ref new wg::Calendar();
            time->Hour = hours;
            time->Minute = minutes;
            time->Period = period;
            time->Nanosecond = 0;
            time->Month = 1;
            time->Day = 1;
            time->Year = 2018;
            return time;
        }

        static wf::TimeSpan CreateTimeSpan(int hours, int minutes, int period = 1)
        {
            return CreateTimeSpan(hours, minutes, 0, period);
        }

        static wf::TimeSpan CreateTimeSpan(int hours, int minutes, int seconds, int period);

        static wf::TimeSpan CalendarToTimeSpan(wg::Calendar^ calendar)
        {
            return CreateTimeSpan(calendar->Hour, calendar->Minute, calendar->Second, calendar->Period);
        }

        static bool AreClose(long long x, long long y, long long tolerance)
        {
            auto diff = x - y;
            return -tolerance < diff && diff < tolerance;
        }
        
        static void VerifyHasPlaceholder(xaml_controls::TimePicker^ timePicker);
        
        static void VerifyDoesNotHavePlaceholder(xaml_controls::TimePicker^ timePicker);

        static void SetTime(xaml_controls::TimePicker^ timePicker, wf::TimeSpan time);

        void CanOpenAndCloseUsingKeyboard();
};

} } } } } }

