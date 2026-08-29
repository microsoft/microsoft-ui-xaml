// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace DatePicker {

    class DatePickerIntegrationTests : public WEX::TestClass<DatePickerIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(DatePickerIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e9192bce-1f8a-48ea-8327-14058db070f2;70a0f79e-de5f-46d3-bd45-a84dcb6e99df;cc5953d4-6553-42e5-8c02-80720aa9d842")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a DatePicker.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyDefaultProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the DatePicker's default properties.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanFireDateChangedEvent)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the DatePicker fires DateChanged event.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChooseDateProjectedShadow)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that the DatePicker can be used to choose a date, projected shadow mode.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore,Santorini")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // We're on drop shadows now
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChooseDateDropShadow)
            TEST_METHOD_PROPERTY(L"Ignore", L"True") // Disabled due to #36620754
            TEST_METHOD_PROPERTY(L"Description", L"Verify that the DatePicker can be used to choose a date, drop shadow mode.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore,Santorini")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIElementTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of DatePicker in various visual states ")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFlyoutPositioningAndSizing)
            TEST_METHOD_PROPERTY(L"Description", L"Validate that the DatePickerFlyout is positioned and sized correctly")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDayMonthYearOrderForDifferentLocales)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the order of Day/Month/Year and FlowDirection for a number of different locales.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateCalendarIdentifierProperty)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that the DatePicker.CalendarIdentifier property works correctly.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDayMonthYearFormatProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that the DatePicker's DayFormat, MonthFormat and YearFormat properties work correctly.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDayMonthYearVisibleProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that the DatePicker's DayVisible, MonthVisible and YearVisible properties work correctly.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateMinYearAndMaxYearProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that the MinYear and MaxYear properties work correctly.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFlyoutPositioningForRightToLeftCalendar)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that the DatePickerFlyout shows at the correct location for RTL calendars.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DatePickerShouldMaintainTime)
            TEST_METHOD_PROPERTY(L"Description", L"DatePicker should not stomp on the time portion of the DateTime property")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFootprint)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the ActualWidth and ActualHeight of DatePicker.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanOpenAndCloseUsingKeyboard)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the DatePicker can be opened and closed with the keyboard.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(HasPlaceholderTextByDefault)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the DatePicker has placeholder text and SelectedDate = null by default.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SelectingDateSetsSelectedDate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that selecting a date from the DatePicker sets SelectedDate to a non-null value.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSelectedDatePropagatesToDate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates setting SelectedDate propagates that value to Date, and vice-versa when Date is set.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanProgrammaticallyClearSelectedDate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting SelectedDate to null brings back the placeholder text.")
        END_TEST_METHOD()
        
        // Test: DateTime picker fails for calendars whose minimum date is not DateTime.MinValue.
        BEGIN_TEST_METHOD(CanSelectDateInJapaneseCalendar)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that the DatePicker can pick a date in Japanese calendar.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateAutomationNameWhenDateIsNull)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that the DatePicker's Automation Name is empty when no date is selected")
        END_TEST_METHOD()

    private:
        static Microsoft::UI::Xaml::Controls::DatePicker^ SetupDatePickerTest();

        // Create date and set to 1990/06/15.
        // This is just an arbitary value to be used by tests when they just need a date value to test with.
        static wg::Calendar^ CreateDate()
        {
            return CreateDate(6, 15, 1990);
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

        static xaml_controls::Button^ GetFlyoutButtonFromDatePicker(xaml_controls::DatePicker^ datePicker);

        static xaml_controls::DatePickerFlyoutPresenter^ GetDatePickerFlyoutPresenter();

        static void VerifyDayMonthYearOrderAndFlowDirection(Platform::String^ locale, int expectedDayTextBlockColumn, int expectedMonthTextBlockColumn, int expectedYearTextBlockColumn, xaml::FlowDirection expectedFlowDirection);

        static void GetDayMonthYearTextBlocksFromDatePicker(xaml_controls::DatePicker^ datePicker, xaml_controls::TextBlock^& dayTextBlock, xaml_controls::TextBlock^& monthTextBlock, xaml_controls::TextBlock^& yearTextBlock);

        static bool AreClose(double a, double b, double threshold)
        {
            return abs(a - b) <= threshold;
        }

        void VerifyDatesAreEqual(wg::Calendar^ expected, wf::DateTime actual);
        
        static void VerifyHasPlaceholder(xaml_controls::DatePicker^ datePicker);
        
        static void VerifyDoesNotHavePlaceholder(xaml_controls::DatePicker^ datePicker);
        
        static void SetDate(xaml_controls::DatePicker^ datePicker, wf::DateTime date);

        void CanChooseDate();
    };

} } } } } }

