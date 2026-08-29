// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace DatePickerFlyout {

    class DatePickerFlyoutIntegrationTests : public WEX::TestClass<DatePickerFlyoutIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(DatePickerFlyoutIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e9192bce-1f8a-48ea-8327-14058db070f2;a69ddfa4-5142-4bed-887d-6d0ca14a3473;bdc5c68b-03c1-4217-832c-4c09f99946f4")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a DatePickerFlyout.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyDefaultProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Validates DatePickerFlyout's default properties.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSelectDate)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the a Date can be selected using the DatePickerFlyout")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSelectDateWithShowAtAsync)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the a Date can be selected using DatePickerFlyout->ShowAtAsync")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesFlyoutOnDatePickerClicked)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the launching flyout on the DatePicker.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DismissWithCancelButton)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that the DatePickerFlyout can be dismissed with the Cancel button.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDayMonthYearOrderForDifferentLocales)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the order of Day/Month/Year and FlowDirection for a number of different locales. ")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateCalendarIdentifierProperty)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that the DatePickerFlyout.CalendarIdentifier property works correctly.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDayMonthYearFormatProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that the DayFormat, MonthFormat and YearFormat properties work correctly.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDayMonthYearVisibleProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that the DatePickerFlyout's DayVisible, MonthVisible and YearVisible properties work correctly.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyFlyoutResizesWithSmallHeight)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that the flyout resizes to a smaller height when a smaller height is available.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"Desktop") // Windowed popups make this test moot.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIElementTree_Dark)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of DatePickerFlyout in the dark theme visual states.")
            // TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIElementTree_Light)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of DatePickerFlyout in the light theme visual states.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIElementTree_HC)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of DatePickerFlyout in the HighContrast theme visual states.")
            // TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIElementTree_Full)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of DatePickerFlyout in the full placement mode visual states.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateMinYearAndMaxYearProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that the MinYear and MaxYear properties work correctly.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangingSelectedMonthShouldRecomputeDaysColumn)
            TEST_METHOD_PROPERTY(L"Description", L"Changing the selected month should should cause the days available to be recalculated.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(HorizontalArrowKeysInvertedInRTL)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that using the left and right arrow keys is inverted and correctly handled when FlowDirection for Grid is RightToLeft.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanMoveBetweenColumnsWithHorizontalArrowKeys)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that using the left and right arrow keys can shift focus between the columns.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateAcceptDismissButtonsAreHiddenWithGamepad)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that opening a DatePickerFlyout with a gamepad causes the accept and dismiss buttons to become hidden.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // When opening the flyout with gamepad input, AcceptDismissHostGrid is still visible. 
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOverlayBrush)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the brush used for the Overlay matches the 'DatePickerLightDismissOverlayBackground' resource.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyChangingDayOnClosedDatePickerFlyoutUpdatesCorrectly)
            TEST_METHOD_PROPERTY(L"Description", L"After programmatically changing the day on a closed DatePickerFlyout, the updated date should be displayed on reopening ")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDateSelectionFiresAutomationEvent)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that when a date is changed in DatePickerFlyout, an automation focus event is raised.")
        END_TEST_METHOD()

    private:
        // Create date and set to 1990/12/31.
        // This is just an arbitary value to be used by tests when they just need a date value to test with.
        static wg::Calendar^ CreateDate()
        {
            return CreateDate(12, 31, 1990);
        }

        static wg::Calendar^ CreateDate(int month, int day, int year)
        {
            auto date = ref new wg::Calendar();
            date->Hour = 1;
            date->Minute = 1;
            date->Period = 1;
            date->Nanosecond = 0;
            date->Month = month;
            date->Day = day;
            date->Year = year;

            return date;
        }

        template <class TDatePickerControl>
        void VerifyDateChange(TDatePickerControl datePickerControl, ::Windows::Globalization::Calendar^ originalDate);

        void VerifyDayMonthYearOrderAndFlowDirection(Platform::String^ locale, int expectedDayTextBlockColumn, int expectedMonthTextBlockColumn, int expectedYearTextBlockColumn, xaml::FlowDirection expectedFlowDirection);

        void SetupDatePickerFlyoutTest(xaml_controls::Button^& target, xaml_controls::DatePickerFlyout^& datePickerFlyout);

        xaml_controls::DatePickerFlyoutPresenter^ GetOpenDatePickerFlyoutPresenter();

        void FindSelectedDatePickerFlyoutItemsFromOpenFlyout(
            xaml_controls::IDatePickerFlyoutItem^& daySelectedItem,
            xaml_controls::IDatePickerFlyoutItem^& monthSelectedItem,
            xaml_controls::IDatePickerFlyoutItem^& yearSelectedItem);

        void HideDatePickerFlyout(xaml_controls::DatePickerFlyout^ datePickerFlyout);
        void ShowDatePickerFlyout(xaml_controls::DatePickerFlyout^ datePickerFlyout, xaml_controls::Button^ target, std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event>& flyoutOpenedEvent);

        enum class Theme
        {
            Dark = 0,
            Light = 1,
            HighContrast = 2
        };

        void ValidateUIETreeWorker(xaml_primitives::FlyoutPlacementMode placementMode, Theme theme, Platform::String^ variation);

        void VerifyYearLoopingSelectorItems(xaml_primitives::LoopingSelector^ yearLoopingSelector, unsigned int minYear, unsigned int maxYear);

        void VerifyDayLoopingSelectorItems(xaml_primitives::LoopingSelector^ dayLoopingSelector, int expectedSelectedDay, unsigned int expectedNumberOfDays);

        void VerifyDatesAreEqual(wg::Calendar^ expected, wf::DateTime actual);
    };


} } } } } } // Microsoft::UI::Xaml::Tests::Controls::DatePickerFlyout

