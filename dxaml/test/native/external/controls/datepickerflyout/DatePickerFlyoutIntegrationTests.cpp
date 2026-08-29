// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "DatePickerFlyoutIntegrationTests.h"

#include <generic\DependencyObjectTests.h>

#include <XamlTailored.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <Collection.h>

#include <TreeHelper.h>
#include <ControlHelper.h>
#include <DateTimePickerHelper.h>
#include <LoopingSelectorHelper.h>
#include <PopupHelper.h>

#include <AutomationClient\AutomationClientManager.h>
#include <AutomationClient\AutomationEventHandler.h>

#include "KeyboardInjectionOverride.h"

using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace DatePickerFlyout {

    bool DatePickerFlyoutIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool DatePickerFlyoutIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool DatePickerFlyoutIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void DatePickerFlyoutIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::DatePickerFlyout>::CanInstantiate();
    }

    void DatePickerFlyoutIntegrationTests::VerifyDefaultProperties()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::DatePickerFlyout^ datePickerFlyout = nullptr;

        RunOnUIThread([&]()
        {
            datePickerFlyout = ref new xaml_controls::DatePickerFlyout();

            VERIFY_ARE_EQUAL(wg::CalendarIdentifiers::Gregorian, datePickerFlyout->CalendarIdentifier);
            VERIFY_ARE_EQUAL(Platform::StringReference(L"day"), datePickerFlyout->DayFormat);
            VERIFY_ARE_EQUAL(Platform::StringReference(L"{month.full}"), datePickerFlyout->MonthFormat);
            VERIFY_ARE_EQUAL(Platform::StringReference(L"year.full"), datePickerFlyout->YearFormat);
            VERIFY_IS_TRUE(datePickerFlyout->DayVisible);
            VERIFY_IS_TRUE(datePickerFlyout->MonthVisible);
            VERIFY_IS_TRUE(datePickerFlyout->YearVisible);

            auto today = ref new wg::Calendar();
            today->SetToNow();

            // Default value of Date should be equal to today's date.
            VerifyDatesAreEqual(today, datePickerFlyout->Date);

            // Default value of MinYear should be 50 years ago.
            auto datePickerFlyoutMinYear = ref new wg::Calendar();
            datePickerFlyoutMinYear->SetDateTime(datePickerFlyout->MinYear);
            VERIFY_ARE_EQUAL(today->Year - 50, datePickerFlyoutMinYear->Year);

            // Default value of MinYear should be 50 years from now.
            auto datePickerFlyoutMaxYear = ref new wg::Calendar();
            datePickerFlyoutMaxYear->SetDateTime(datePickerFlyout->MaxYear);
            VERIFY_ARE_EQUAL(today->Year + 50, datePickerFlyoutMaxYear->Year);
        });

        xaml_controls::Button^ button = nullptr;
        SetupDatePickerFlyoutTest(button, datePickerFlyout);
        FlyoutHelper::OpenFlyout(datePickerFlyout, button, FlyoutOpenMethod::Programmatic_ShowAt);

        RunOnUIThread([&]()
        {
            auto datepickerFlyoutPresenter = DateTimePickerHelper::GetOpenDatePickerFlyoutPresenter();
            VERIFY_IS_NOT_NULL(datepickerFlyoutPresenter);

            VERIFY_ARE_EQUAL(true, datepickerFlyoutPresenter->IsDefaultShadowEnabled);
        });
    }

    void DatePickerFlyoutIntegrationTests::DoesFlyoutOnDatePickerClicked()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::DatePicker^ datePicker = nullptr;
        xaml_controls::Button^ button = nullptr;
        wg::Calendar^ originalDate = nullptr;

        auto dateChangedEvent = std::make_shared<Event>();
        auto dateChangedRegistration = CreateSafeEventRegistration(xaml_controls::DatePicker, DateChanged);

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();
            VERIFY_IS_NOT_NULL(rootPanel);

            datePicker = ref new xaml_controls::DatePicker();
            VERIFY_IS_NOT_NULL(datePicker);

            // Create date and set 1990/12/31.
            originalDate = CreateDate();
            datePicker->Date = originalDate->GetDateTime();

            dateChangedRegistration.Attach(
                datePicker,
                ref new wf::EventHandler<xaml_controls::DatePickerValueChangedEventArgs^>(
                [dateChangedEvent](Platform::Object^ sender, xaml_controls::DatePickerValueChangedEventArgs^ args)
            {
                auto calendarNew = ref new wg::Calendar();
                calendarNew->SetDateTime(args->NewDate);
                LOG_OUTPUT(L"DoesFlyoutOnDatePickerClicked: Changed Year=%d Month=%d Day=%d.", calendarNew->Year, calendarNew->Month, calendarNew->Day);
                dateChangedEvent->Set();
            }));

            rootPanel->Children->Append(datePicker);
            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            button = dynamic_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(datePicker, "FlyoutButton"));
            VERIFY_IS_NOT_NULL(button);
        });

        LOG_OUTPUT(L"DoesFlyoutOnDatePickerClicked: launch the date picker flyout by using Tap.");
        ControlHelper::DoClickUsingTap(button);

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"DoesFlyoutOnDatePickerClicked: Pan the looping selector.");
        LoopingSelectorHelper::PanDateTimeLoopingSelector();

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"DoesFlyoutOnDatePickerClicked: Close the picker flyout.");
        ControlHelper::ClickFlyoutCloseButton(button, true /* isAccept */);

        dateChangedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VerifyDateChange(datePicker, originalDate);
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void DatePickerFlyoutIntegrationTests::CanSelectDate()
    {
        TestCleanupWrapper cleanup;
        // Verifies that the DatePickerFlyout can be used to select a Date.
        // A DatePickerFlyout is shown and a Date is selected using the keyboard up/down arrows.
        // Verifies:
        //    1. The DatePicked event is fired and the DatePickedEventArgs OldDate and NewDate are correct.
        //    2. DatePickerFlyout->Date gets updated to the new value.

        KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride(KeyboardWaitKind::None);

        xaml_controls::DatePickerFlyout^ datePickerFlyout;
        xaml_controls::Button^ button;
        SetupDatePickerFlyoutTest(button, datePickerFlyout);

        auto originalDate = CreateDate(6, 12, 2015);
        auto dateToSelect = CreateDate(4, 15, 2012);
        auto minYear = CreateDate(1, 1, 1950);

        RunOnUIThread([&]()
        {
            datePickerFlyout->Date = originalDate->GetDateTime();
            datePickerFlyout->MinYear = minYear->GetDateTime();
        });

        auto datePickedEvent = std::make_shared<Event>();
        auto datePickedRegistration = CreateSafeEventRegistration(xaml_controls::DatePickerFlyout, DatePicked);
        datePickedRegistration.Attach(datePickerFlyout,
            ref new wf::TypedEventHandler<xaml_controls::DatePickerFlyout^, xaml_controls::DatePickedEventArgs^>(
            [&](xaml_controls::DatePickerFlyout^ s, xaml_controls::DatePickedEventArgs^ args)
        {
            VERIFY_ARE_EQUAL(originalDate->GetDateTime(), args->OldDate);
            VERIFY_ARE_EQUAL(dateToSelect->GetDateTime(), args->NewDate);
            datePickedEvent->Set();
        }));

        FlyoutHelper::OpenFlyout(datePickerFlyout, button, FlyoutOpenMethod::Programmatic_ShowAt);

        DateTimePickerHelper::SelectDateInOpenDatePickerFlyout(dateToSelect, minYear->Year, LoopingSelectorHelper::SelectionMode::Keyboard);
        datePickedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(dateToSelect->GetDateTime(), datePickerFlyout->Date);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void DatePickerFlyoutIntegrationTests::CanSelectDateWithShowAtAsync()
    {
        // Leak: TimePickerFlyout::ShowAtAsync
        TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

        KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride(KeyboardWaitKind::None);

        TestCleanupWrapper cleanup;
        // Tests the functionality of DatePickerFlyout->ShowAtAsync.
        // Verifies:
        //   1. The IAsyncOperation.Completed handler gets called when a date is selected in the UI.
        //   2. The result passed to the completed handler is the correct DateTime value.
        //   3. DatePickerFlyout->Date gets updated to the correct value.

        xaml_controls::DatePickerFlyout^ datePickerFlyout;
        xaml_controls::Button^ button;
        SetupDatePickerFlyoutTest(button, datePickerFlyout);

        auto originalDate = CreateDate(6, 12, 2015);
        auto dateToSelect = CreateDate(4, 15, 2012);
        auto minYear = CreateDate(1, 1, 1950);

        RunOnUIThread([&]()
        {
            datePickerFlyout->Date = originalDate->GetDateTime();
            datePickerFlyout->MinYear = minYear->GetDateTime();
        });
        TestServices::WindowHelper->WaitForIdle();

        using ResultType = Platform::IBox<wf::DateTime>^;

        wf::IAsyncOperation<ResultType>^ asyncOperation;
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Showing DatePickerFlyout...");
            asyncOperation = datePickerFlyout->ShowAtAsync(button);
        });

        auto showAtAsyncCompleted = std::make_shared<Event>();

        asyncOperation->Completed = ref new wf::AsyncOperationCompletedHandler<ResultType>(
            [&](wf::IAsyncOperation<ResultType>^ asyncInfo, wf::AsyncStatus asyncStatus)
        {
            LOG_OUTPUT(L"IAsyncOperation::Completed handler - asyncStatus: %d", asyncStatus);
            VERIFY_ARE_EQUAL(wf::AsyncStatus::Completed, asyncStatus);

            auto result = asyncInfo->GetResults();

            if (result == nullptr)
            {
                LOG_OUTPUT(L"IAsyncOperation::Completed handler - IAsyncOperation::GetResults() returns null");
            }
            else
            {
                auto datePicked = result->Value;
                VERIFY_ARE_EQUAL(dateToSelect->GetDateTime(), datePicked);
            }

            showAtAsyncCompleted->Set();
        });

        TestServices::WindowHelper->WaitForIdle();
        DateTimePickerHelper::SelectDateInOpenDatePickerFlyout(dateToSelect, minYear->Year, LoopingSelectorHelper::SelectionMode::Keyboard);
        showAtAsyncCompleted->WaitForDefault();

        auto result = asyncOperation->GetResults();
        if (result == nullptr)
        {
            VERIFY_FAIL(L"IAsyncOperation::GetResults() returns null");
        }
        else
        {
            VERIFY_ARE_EQUAL(dateToSelect->GetDateTime(), result->Value);
        }
        TestServices::WindowHelper->WaitForIdle();
    }

    void DatePickerFlyoutIntegrationTests::DismissWithCancelButton()
    {
        // Verifies that the DatePickerFlyout can be closed with the Cancel button.
        // Even though we use the UI to change the selection, this should have no effect:
        //    1. The DatePicked event should NOT be fired.
        //    2. The DatePicker->Date property should NOT get changed.
        TestCleanupWrapper cleanup;

        xaml_controls::DatePickerFlyout^ datePickerFlyout;
        xaml_controls::Button^ button;
        SetupDatePickerFlyoutTest(button, datePickerFlyout);

        auto originalDate = CreateDate(6, 12, 2015);

        RunOnUIThread([&]()
        {
            datePickerFlyout->Date = originalDate->GetDateTime();
        });
        TestServices::WindowHelper->WaitForIdle();

        // The DatePicked event should not fire when we use the Cancel button.
        auto datePickedEvent = std::make_shared<Event>();
        auto datePickedRegistration = CreateSafeEventRegistration(xaml_controls::DatePickerFlyout, DatePicked);
        datePickedRegistration.Attach(datePickerFlyout, [&]()
        {
            VERIFY_FAIL(L"DatePicked event should not fire");
        });

        auto flyoutClosedEvent = std::make_shared<Event>();
        auto flyoutClosedRegistration = CreateSafeEventRegistration(xaml_controls::DatePickerFlyout, Closed);
        flyoutClosedRegistration.Attach(datePickerFlyout, [&]() { flyoutClosedEvent->Set(); });

        FlyoutHelper::OpenFlyout(datePickerFlyout, button, FlyoutOpenMethod::Touch);
        TestServices::WindowHelper->WaitForIdle();

        // Change the selected date in the open DatePickerFlyout:
        LoopingSelectorHelper::PanDateTimeLoopingSelector();
        TestServices::WindowHelper->WaitForIdle();

        // Dismiss the flyout with the Cancel button.
        ControlHelper::ClickFlyoutCloseButton(button, false /*isAccept*/);
        flyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // Verify that the Date property did not change:
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(originalDate->GetDateTime(), datePickerFlyout->Date);
        });
    }

    // Verify the date change with the original date.
    template <class TDatePickerControl>
    void DatePickerFlyoutIntegrationTests::VerifyDateChange(TDatePickerControl datePickerControl, wg::Calendar^ originalDate)
    {
        auto newDate = ref new wg::Calendar();
        newDate->SetDateTime(datePickerControl->Date);

        VERIFY_IS_TRUE(originalDate->Year != newDate->Year);
        VERIFY_IS_TRUE(originalDate->Month != newDate->Month);
        VERIFY_IS_TRUE(originalDate->Day != newDate->Day);
    }

    void DatePickerFlyoutIntegrationTests::ValidateDayMonthYearOrderForDifferentLocales()
    {
        TestCleanupWrapper cleanup([]()
        {
            LOG_OUTPUT(L"Resetting ApplicationLanguages::PrimaryLanguageOverride");
            wg::ApplicationLanguages::PrimaryLanguageOverride = nullptr;
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        // In this test, we force the app into a specifed locale, and verify that the DatePicker lays out correctly.
        // We validate:
        // 1. The Day/Month/Year Columns are in the appropriate order.
        // 2. The Day/Month/Year LoopingSelectors are in the appropriate order.
        // 3. The FlowDirection is set correctly.
        // Note: In the DatePickerFlyoutPresenter template's Grid the columns 1 and 3 are used by the cell dividers, so they are skipped.
        // That is why the expected column positions for day/month/year are {0, 2, 4} and not {0, 1, 2}.

        VerifyDayMonthYearOrderAndFlowDirection(L"en-US", 2, 0, 4, xaml::FlowDirection::LeftToRight);
        VerifyDayMonthYearOrderAndFlowDirection(L"en-GB", 0, 2, 4, xaml::FlowDirection::LeftToRight);
        VerifyDayMonthYearOrderAndFlowDirection(L"ar",    0, 2, 4, xaml::FlowDirection::RightToLeft);
        VerifyDayMonthYearOrderAndFlowDirection(L"ts-ZA", 4, 2, 0, xaml::FlowDirection::LeftToRight);
    }

    void DatePickerFlyoutIntegrationTests::VerifyDayMonthYearOrderAndFlowDirection(Platform::String^ locale, int expectedDaySelectorColumn, int expectedMonthSelectorColumn, int expectedYearSelectorColumn, xaml::FlowDirection expectedFlowDirection)
    {
        // This forces the app to use the specified language/locale:
        LOG_OUTPUT(L"VerifyDayMonthYearOrder: Setting ApplicationLanguages::PrimaryLanguageOverride to %s", locale->Begin());
        wg::ApplicationLanguages::PrimaryLanguageOverride = locale;

        xaml_controls::DatePickerFlyout^ datePickerFlyout = nullptr;
        xaml_controls::Button^ button = nullptr;

        SetupDatePickerFlyoutTest(button, datePickerFlyout);

        FlyoutHelper::OpenFlyout(datePickerFlyout, button, FlyoutOpenMethod::Programmatic_ShowAt);

        RunOnUIThread([&]()
        {
            auto datepickerFlyoutPresenter = DateTimePickerHelper::GetOpenDatePickerFlyoutPresenter();
            VERIFY_IS_NOT_NULL(datepickerFlyoutPresenter);

            auto templateRoot = dynamic_cast<xaml::FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(datepickerFlyoutPresenter, 0));
            VERIFY_IS_NOT_NULL(templateRoot);

            auto dayLoopingSelector = dynamic_cast<xaml_primitives::LoopingSelector^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"DayLoopingSelector", templateRoot));
            auto monthLoopingSelector = dynamic_cast<xaml_primitives::LoopingSelector^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"MonthLoopingSelector", templateRoot));
            auto yearLoopingSelector = dynamic_cast<xaml_primitives::LoopingSelector^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"YearLoopingSelector", templateRoot));
            VERIFY_IS_NOT_NULL(dayLoopingSelector);
            VERIFY_IS_NOT_NULL(monthLoopingSelector);
            VERIFY_IS_NOT_NULL(yearLoopingSelector);

            auto dayColumn = dynamic_cast<xaml_controls::ColumnDefinition^>(templateRoot->FindName("DayColumn"));
            auto monthColumn = dynamic_cast<xaml_controls::ColumnDefinition^>(templateRoot->FindName("MonthColumn"));
            auto yearColumn = dynamic_cast<xaml_controls::ColumnDefinition^>(templateRoot->FindName("YearColumn"));
            VERIFY_IS_NOT_NULL(dayColumn);
            VERIFY_IS_NOT_NULL(monthColumn);
            VERIFY_IS_NOT_NULL(yearColumn);

            auto pickerHostGrid = dynamic_cast<xaml_controls::Grid^>(templateRoot->FindName("PickerHostGrid"));
            VERIFY_IS_NOT_NULL(pickerHostGrid);

            VERIFY_ARE_EQUAL(expectedFlowDirection, pickerHostGrid->FlowDirection);

            VERIFY_ARE_EQUAL(expectedDaySelectorColumn, xaml_controls::Grid::GetColumn(dayLoopingSelector));
            VERIFY_ARE_EQUAL(expectedMonthSelectorColumn, xaml_controls::Grid::GetColumn(monthLoopingSelector));
            VERIFY_ARE_EQUAL(expectedYearSelectorColumn, xaml_controls::Grid::GetColumn(yearLoopingSelector));

            bool isFound = false;
            unsigned int indexOfDayColumn = 0;
            unsigned int indexOfMonthColumn = 0;
            unsigned int indexOfYearColumn = 0;

            isFound = pickerHostGrid->ColumnDefinitions->IndexOf(dayColumn, &indexOfDayColumn);
            VERIFY_IS_TRUE(isFound /*dayColumn*/);
            VERIFY_ARE_EQUAL(expectedDaySelectorColumn, (int)indexOfDayColumn);

            isFound = pickerHostGrid->ColumnDefinitions->IndexOf(monthColumn, &indexOfMonthColumn);
            VERIFY_IS_TRUE(isFound /*monthColumn*/);
            VERIFY_ARE_EQUAL(expectedMonthSelectorColumn, (int)indexOfMonthColumn);

            isFound = pickerHostGrid->ColumnDefinitions->IndexOf(yearColumn, &indexOfYearColumn);
            VERIFY_IS_TRUE(isFound /*yearColumn*/);
            VERIFY_ARE_EQUAL(expectedYearSelectorColumn, (int)indexOfYearColumn);
        });
        FlyoutHelper::HideFlyout(datePickerFlyout);
    }

    void DatePickerFlyoutIntegrationTests::ValidateCalendarIdentifierProperty()
    {
        TestCleanupWrapper cleanup;

        // Validate that the DatePickerFlyout.CalendarIdentifier property causes the date to be displayed in format for that Calendar
        // We are not attempting to validate the correctness of what gets displayed. Only that it matches DateTimeFormatter returns for that calendar.

        // For each supported calendar, we perform the following:
        // Create a date in that calendar.
        // Using DateTimeFormatter, construct an expected day/month/year string for that date in that calendar.
        // Create a DatePickerFlyout and set Date and CalendarIdentifier
        // Show the DatePickerFlyout and search the visual tree for the displayed Day, Month and Year.
        // Verify that the Day/Month/Year strings match the expected values.

        std::array<Platform::String^, 9> calendarIdentifiers{ {
            L"GregorianCalendar",
            L"HebrewCalendar",
            L"HijriCalendar",
            L"JapaneseCalendar",
            L"JulianCalendar",
            L"KoreanCalendar",
            L"TaiwanCalendar",
            L"ThaiCalendar",
            L"UmAlQuraCalendar"
                } };

        for (auto& cid : calendarIdentifiers)
        {
            LOG_OUTPUT(L"Testing DatePicker with Calendar: %s", cid->Begin());

            xaml_controls::DatePickerFlyout^ datePickerFlyout = nullptr;
            xaml_controls::Button^ button = nullptr;
            Platform::String^ dayFormat;
            Platform::String^ monthFormat;
            Platform::String^ yearFormat;

            SetupDatePickerFlyoutTest(button, datePickerFlyout);

            // Get the values of Day/Month/YearFormat used by DatePickerFlyout.
            // These are used below to construct the "expected" values for Day/Month/Year strings.
            RunOnUIThread([&]()
            {
                dayFormat = datePickerFlyout->DayFormat;
                monthFormat = datePickerFlyout->MonthFormat;
                yearFormat = datePickerFlyout->YearFormat;
            });

            auto calendar = ref new wg::Calendar();
            calendar->Year = 2015;
            calendar->Month = 2;
            calendar->Day = 11;
            calendar->ChangeCalendarSystem(cid);

            auto dtf = ref new wg::DateTimeFormatting::DateTimeFormatter(L"{month.full}");
            dtf = ref new wg::DateTimeFormatting::DateTimeFormatter(dayFormat, dtf->Languages, dtf->GeographicRegion, cid, dtf->Clock);
            auto expectedDayString = dtf->Format(calendar->GetDateTime());
            dtf = ref new wg::DateTimeFormatting::DateTimeFormatter(monthFormat, dtf->Languages, dtf->GeographicRegion, cid, dtf->Clock);
            auto expectedMonthString = dtf->Format(calendar->GetDateTime());
            dtf = ref new wg::DateTimeFormatting::DateTimeFormatter(yearFormat, dtf->Languages, dtf->GeographicRegion, cid, dtf->Clock);
            auto expectedYearString = dtf->Format(calendar->GetDateTime());

            RunOnUIThread([&]()
            {
                datePickerFlyout->Date = calendar->GetDateTime();
                datePickerFlyout->CalendarIdentifier = cid;
            });

            FlyoutHelper::OpenFlyout(datePickerFlyout, button, FlyoutOpenMethod::Programmatic_ShowAt);

            xaml_controls::IDatePickerFlyoutItem^ daySelectedItem;
            xaml_controls::IDatePickerFlyoutItem^ monthSelectedItem;
            xaml_controls::IDatePickerFlyoutItem^ yearSelectedItem;
            FindSelectedDatePickerFlyoutItemsFromOpenFlyout(daySelectedItem, monthSelectedItem, yearSelectedItem);

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(daySelectedItem->PrimaryText, expectedDayString);
                VERIFY_ARE_EQUAL(monthSelectedItem->PrimaryText, expectedMonthString);
                VERIFY_ARE_EQUAL(yearSelectedItem->PrimaryText, expectedYearString);

                datePickerFlyout->Hide();
            });
        }
    }

    void DatePickerFlyoutIntegrationTests::ValidateDayMonthYearFormatProperties()
    {
        TestCleanupWrapper cleanup;

        Platform::String^ dayFormat = L"{day.integer(2)}";
        Platform::String^ monthFormat = L"{month.abbreviated}";
        Platform::String^ yearFormat = L"{year.abbreviated(2)}";

        auto date = CreateDate()->GetDateTime();

        auto expectedDayString = (ref new wg::DateTimeFormatting::DateTimeFormatter(dayFormat))->Format(date);
        auto expectedMonthString = (ref new wg::DateTimeFormatting::DateTimeFormatter(monthFormat))->Format(date);
        auto expectedYearString = (ref new wg::DateTimeFormatting::DateTimeFormatter(yearFormat))->Format(date);

        xaml_controls::DatePickerFlyout^ datePickerFlyout = nullptr;
        xaml_controls::Button^ button = nullptr;

        SetupDatePickerFlyoutTest(button, datePickerFlyout);

        RunOnUIThread([&]()
        {
            datePickerFlyout->DayFormat = dayFormat;
            datePickerFlyout->MonthFormat = monthFormat;
            datePickerFlyout->YearFormat = yearFormat;
            datePickerFlyout->Date = date;
        });

        FlyoutHelper::OpenFlyout(datePickerFlyout, button, FlyoutOpenMethod::Programmatic_ShowAt);

        xaml_controls::IDatePickerFlyoutItem^ daySelectedItem = nullptr;
        xaml_controls::IDatePickerFlyoutItem^ monthSelectedItem = nullptr;
        xaml_controls::IDatePickerFlyoutItem^ yearSelectedItem = nullptr;
        FindSelectedDatePickerFlyoutItemsFromOpenFlyout(daySelectedItem, monthSelectedItem, yearSelectedItem);

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(daySelectedItem->PrimaryText, expectedDayString);
            VERIFY_ARE_EQUAL(monthSelectedItem->PrimaryText, expectedMonthString);
            VERIFY_ARE_EQUAL(yearSelectedItem->PrimaryText, expectedYearString);
        });

        FlyoutHelper::HideFlyout(datePickerFlyout);

        // Verify that we can update the properties of an already used DatePickerFlyout
        // We want to make sure that the updated values get applied correctly and the old values don't end up getting used.
        RunOnUIThread([&]()
        {
            dayFormat = L"dayofweek";
            monthFormat = L"month.numeric";
            yearFormat = L"year";
            expectedDayString = (ref new wg::DateTimeFormatting::DateTimeFormatter(dayFormat))->Format(date);
            expectedMonthString = (ref new wg::DateTimeFormatting::DateTimeFormatter(monthFormat))->Format(date);
            expectedYearString = (ref new wg::DateTimeFormatting::DateTimeFormatter(yearFormat))->Format(date);
            datePickerFlyout->DayFormat = dayFormat;
            datePickerFlyout->MonthFormat = monthFormat;
            datePickerFlyout->YearFormat = yearFormat;
        });

        FlyoutHelper::OpenFlyout(datePickerFlyout, button, FlyoutOpenMethod::Programmatic_ShowAt);

        FindSelectedDatePickerFlyoutItemsFromOpenFlyout(daySelectedItem, monthSelectedItem, yearSelectedItem);

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(daySelectedItem->PrimaryText, expectedDayString);
            VERIFY_ARE_EQUAL(monthSelectedItem->PrimaryText, expectedMonthString);
            VERIFY_ARE_EQUAL(yearSelectedItem->PrimaryText, expectedYearString);

            datePickerFlyout->Hide();
        });
    }

    void DatePickerFlyoutIntegrationTests::ValidateDayMonthYearVisibleProperties()
    {
        TestCleanupWrapper cleanup;

        // Verifies that DayVisibile, MonthVisible and YearVisible properties have the appropriate effect.
        // We set these properties and verify:
        //   The corresponding LoopingSelector gets Collapsed.
        //   The corresponding ColumnDefinition gets removed from the PickerHostGrid.ColumnDefinitions

        xaml_controls::DatePickerFlyout^ datePickerFlyout = nullptr;
        xaml_controls::Button^ button = nullptr;

        SetupDatePickerFlyoutTest(button, datePickerFlyout);

        FlyoutHelper::OpenFlyout(datePickerFlyout, button, FlyoutOpenMethod::Programmatic_ShowAt);

        xaml_primitives::LoopingSelector^ dayLoopingSelector = nullptr;
        xaml_primitives::LoopingSelector^ monthLoopingSelector = nullptr;
        xaml_primitives::LoopingSelector^ yearLoopingSelector = nullptr;
        xaml_controls::ColumnDefinition^ dayColumn = nullptr;
        xaml_controls::ColumnDefinition^ monthColumn = nullptr;
        xaml_controls::ColumnDefinition^ yearColumn = nullptr;
        xaml_controls::Grid^ pickerHostGrid = nullptr;
        unsigned int columnIndex = 0;

        RunOnUIThread([&]()
        {
            auto datepickerFlyoutPresenter = DateTimePickerHelper::GetOpenDatePickerFlyoutPresenter();
            VERIFY_IS_NOT_NULL(datepickerFlyoutPresenter);

            auto templateRoot = safe_cast<xaml::FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(datepickerFlyoutPresenter, 0));
            VERIFY_IS_NOT_NULL(templateRoot);

            dayLoopingSelector = dynamic_cast<xaml_primitives::LoopingSelector^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"DayLoopingSelector", button));
            monthLoopingSelector = dynamic_cast<xaml_primitives::LoopingSelector^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"MonthLoopingSelector", button));
            yearLoopingSelector = dynamic_cast<xaml_primitives::LoopingSelector^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"YearLoopingSelector", button));
            VERIFY_IS_NOT_NULL(dayLoopingSelector);
            VERIFY_IS_NOT_NULL(monthLoopingSelector);
            VERIFY_IS_NOT_NULL(yearLoopingSelector);

            dayColumn = dynamic_cast<xaml_controls::ColumnDefinition^>(templateRoot->FindName("DayColumn"));
            monthColumn = dynamic_cast<xaml_controls::ColumnDefinition^>(templateRoot->FindName("MonthColumn"));
            yearColumn = dynamic_cast<xaml_controls::ColumnDefinition^>(templateRoot->FindName("YearColumn"));
            VERIFY_IS_NOT_NULL(dayColumn);
            VERIFY_IS_NOT_NULL(monthColumn);
            VERIFY_IS_NOT_NULL(yearColumn);

            pickerHostGrid = dynamic_cast<xaml_controls::Grid^>(templateRoot->FindName("PickerHostGrid"));
            VERIFY_IS_NOT_NULL(pickerHostGrid);
        });

        FlyoutHelper::HideFlyout(datePickerFlyout);

        RunOnUIThread([&]()
        {
            datePickerFlyout->DayVisible = false;
        });
        FlyoutHelper::OpenFlyout(datePickerFlyout, button, FlyoutOpenMethod::Programmatic_ShowAt);
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(dayLoopingSelector->Visibility == xaml::Visibility::Collapsed);
            VERIFY_IS_FALSE(pickerHostGrid->ColumnDefinitions->IndexOf(dayColumn, &columnIndex));

            datePickerFlyout->DayVisible = true;
            datePickerFlyout->MonthVisible = false;
        });
        FlyoutHelper::HideFlyout(datePickerFlyout);
        FlyoutHelper::OpenFlyout(datePickerFlyout, button, FlyoutOpenMethod::Programmatic_ShowAt);
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(dayLoopingSelector->Visibility == xaml::Visibility::Visible);
            VERIFY_IS_TRUE(pickerHostGrid->ColumnDefinitions->IndexOf(dayColumn, &columnIndex));

            VERIFY_IS_TRUE(monthLoopingSelector->Visibility == xaml::Visibility::Collapsed);
            VERIFY_IS_FALSE(pickerHostGrid->ColumnDefinitions->IndexOf(monthColumn, &columnIndex));

            datePickerFlyout->MonthVisible = true;
            datePickerFlyout->YearVisible = false;
        });
        FlyoutHelper::HideFlyout(datePickerFlyout);
        FlyoutHelper::OpenFlyout(datePickerFlyout, button, FlyoutOpenMethod::Programmatic_ShowAt);
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(monthLoopingSelector->Visibility == xaml::Visibility::Visible);
            VERIFY_IS_TRUE(pickerHostGrid->ColumnDefinitions->IndexOf(monthColumn, &columnIndex));

            VERIFY_IS_TRUE(yearLoopingSelector->Visibility == xaml::Visibility::Collapsed);
            VERIFY_IS_FALSE(pickerHostGrid->ColumnDefinitions->IndexOf(yearColumn, &columnIndex));
        });
        FlyoutHelper::HideFlyout(datePickerFlyout);
    }

    void DatePickerFlyoutIntegrationTests::VerifyFlyoutResizesWithSmallHeight()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::DatePickerFlyout^ datePickerFlyout = nullptr;
        xaml_controls::Button^ button = nullptr;

        double originalPresenterHeight = 0.0;
        double originalBackgroundHeight = 0.0;
        double newWindowHeight = 300.0;

        SetupDatePickerFlyoutTest(button, datePickerFlyout);

        FlyoutHelper::OpenFlyout(datePickerFlyout, button, FlyoutOpenMethod::Programmatic_ShowAt);

        RunOnUIThread([&]()
        {
            auto datepickerFlyoutPresenter = DateTimePickerHelper::GetOpenDatePickerFlyoutPresenter();
            auto datePickerFlyoutPresenterBorder = TreeHelper::GetVisualChildByName(datepickerFlyoutPresenter, "Background");

            originalPresenterHeight = datepickerFlyoutPresenter->ActualHeight;
            originalBackgroundHeight = datePickerFlyoutPresenterBorder->ActualHeight;

            LOG_OUTPUT(L"Original presenter and background heights: %f, %f", originalPresenterHeight, originalBackgroundHeight);
        });

        FlyoutHelper::HideFlyout(datePickerFlyout);

        LOG_OUTPUT(L"Changing window size to 400x%d.", (int)newWindowHeight);
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, (float)newWindowHeight));
        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::OpenFlyout(datePickerFlyout, button, FlyoutOpenMethod::Programmatic_ShowAt);

        RunOnUIThread([&]()
        {
            auto datepickerFlyoutPresenter = DateTimePickerHelper::GetOpenDatePickerFlyoutPresenter();
            auto datePickerFlyoutPresenterBorder = TreeHelper::GetVisualChildByName(datepickerFlyoutPresenter, "Background");

            auto newPresenterHeight = datepickerFlyoutPresenter->ActualHeight;
            auto newBackgroundHeight = datePickerFlyoutPresenterBorder->ActualHeight;

            LOG_OUTPUT(L"New presenter and background heights: %f, %f", newPresenterHeight, newBackgroundHeight);
            LOG_OUTPUT(L"Should both be less than original heights and equal to new window height = %f.", newWindowHeight);

            VERIFY_IS_LESS_THAN(newPresenterHeight, originalPresenterHeight);
            VERIFY_IS_LESS_THAN(newBackgroundHeight, originalBackgroundHeight);

            VERIFY_ARE_EQUAL(newPresenterHeight, newWindowHeight);
            VERIFY_ARE_EQUAL(newBackgroundHeight, newWindowHeight);
        });

        FlyoutHelper::HideFlyout(datePickerFlyout);
    }

    void DatePickerFlyoutIntegrationTests::ValidateUIElementTree_Dark()
    {
        ValidateUIETreeWorker(xaml_primitives::FlyoutPlacementMode::Right, Theme::Dark, "Dark");
    }

    void DatePickerFlyoutIntegrationTests::ValidateUIElementTree_Light()
    {
        ValidateUIETreeWorker(xaml_primitives::FlyoutPlacementMode::Right, Theme::Light, "Light");
    }

    void DatePickerFlyoutIntegrationTests::ValidateUIElementTree_HC()
    {
        ValidateUIETreeWorker(xaml_primitives::FlyoutPlacementMode::Right, Theme::HighContrast, "HC");
    }

    void DatePickerFlyoutIntegrationTests::ValidateUIElementTree_Full()
    {
        // We do not do "PlacementMode=Full in themes other than dark" as there is nothing unique in that scenario that is not covered by the other three cases.
        ValidateUIETreeWorker(xaml_primitives::FlyoutPlacementMode::Full, Theme::Dark, "Full");
    }

    void DatePickerFlyoutIntegrationTests::ValidateUIETreeWorker(xaml_primitives::FlyoutPlacementMode placementMode, Theme theme, Platform::String^ variation)
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Panel^ rootPanel = nullptr;
        xaml_controls::DatePickerFlyout^ datePickerFlyout = nullptr;
        xaml_controls::Button^ button = nullptr;

        LOG_OUTPUT(L"ValidateUIETreeWorker: Started.");

        if (PopupHelper::AreWindowedPopupsEnabled())
        {
            variation += ref new Platform::String(L".Windowed");
        }
        else
        {
            variation += ref new Platform::String(L".Unwindowed");
        }

        auto validationRules = ref new Platform::String(DefaultUIElementTreeValidationRules);

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));

        SetupDatePickerFlyoutTest(button, datePickerFlyout);
        RunOnUIThread([&]() { rootPanel = safe_cast<xaml_controls::Panel^>(TestServices::WindowHelper->WindowContent); });

        std::function<void()> validator = [&]() { TestServices::Utilities->VerifyUIElementTreeWithRulesInline(variation, validationRules); };

        if (theme == Theme::Light)
        {
            RunOnUIThread([&]()
            {
                rootPanel->RequestedTheme = xaml::ElementTheme::Light;
                rootPanel->Background = ref new xaml_media::SolidColorBrush(mu::Colors::White);
            });
        }
        else if (theme == Theme::HighContrast)
        {
            // Use a different validator for high-contrast, it'll put us in high-contrast mode as well
            // as do some extra validation.
            validator = [&]() { ControlHelper::ValidateUIElementTreeForHighContrast(variation, rootPanel, validationRules); };
        }

        RunOnUIThread([&]()
        {
            button->IsEnabled = false;    // Disabled so it is not Focusable and IsPressed is set to False to ensure same dumps for Desktop and Phone.
            datePickerFlyout->Date = CreateDate()->GetDateTime();
            datePickerFlyout->Placement = placementMode;
            datePickerFlyout->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;
            // Default values are set to -/+ 50 years from the current year.
            // To make this test consistent from year to year, we use 2016 +/-50 as the min/max year values.
            datePickerFlyout->MinYear = CreateDate(1, 1, 1966)->GetDateTime();
            datePickerFlyout->MaxYear = CreateDate(1, 1, 2066)->GetDateTime();
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"ValidateUIETreeWorker: Open Flyout.");

        FlyoutHelper::OpenFlyout(datePickerFlyout, button, FlyoutOpenMethod::Programmatic_ShowAt);

        LOG_OUTPUT(L"ValidateUIETreeWorker: Validate UIE Tree.");

        validator();

        LOG_OUTPUT(L"ValidateUIETreeWorker: Hide Flyout.");

        FlyoutHelper::HideFlyout(datePickerFlyout);

        LOG_OUTPUT(L"ValidateUIETreeWorker: Completed.");
    }

    void DatePickerFlyoutIntegrationTests::FindSelectedDatePickerFlyoutItemsFromOpenFlyout(
        xaml_controls::IDatePickerFlyoutItem^& daySelectedItem,
        xaml_controls::IDatePickerFlyoutItem^& monthSelectedItem,
        xaml_controls::IDatePickerFlyoutItem^& yearSelectedItem)
    {
        RunOnUIThread([&]()
        {
            auto datepickerFlyoutPresenter = DateTimePickerHelper::GetOpenDatePickerFlyoutPresenter();
            VERIFY_IS_NOT_NULL(datepickerFlyoutPresenter);

            auto templateRoot = dynamic_cast<xaml::FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(datepickerFlyoutPresenter, 0));
            VERIFY_IS_NOT_NULL(templateRoot);

            auto dayLoopingSelector = dynamic_cast<xaml_primitives::LoopingSelector^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"DayLoopingSelector", templateRoot));
            auto monthLoopingSelector = dynamic_cast<xaml_primitives::LoopingSelector^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"MonthLoopingSelector", templateRoot));
            auto yearLoopingSelector = dynamic_cast<xaml_primitives::LoopingSelector^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"YearLoopingSelector", templateRoot));
            VERIFY_IS_NOT_NULL(dayLoopingSelector);
            VERIFY_IS_NOT_NULL(monthLoopingSelector);
            VERIFY_IS_NOT_NULL(yearLoopingSelector);

            daySelectedItem = dynamic_cast<xaml_controls::IDatePickerFlyoutItem^>(dayLoopingSelector->SelectedItem);
            monthSelectedItem = dynamic_cast<xaml_controls::IDatePickerFlyoutItem^>(monthLoopingSelector->SelectedItem);
            yearSelectedItem = dynamic_cast<xaml_controls::IDatePickerFlyoutItem^>(yearLoopingSelector->SelectedItem);
            VERIFY_IS_NOT_NULL(daySelectedItem);
            VERIFY_IS_NOT_NULL(monthSelectedItem);
            VERIFY_IS_NOT_NULL(yearSelectedItem);
        });
    }

    void DatePickerFlyoutIntegrationTests::HorizontalArrowKeysInvertedInRTL()
    {
        // In RTL, Grid column 0 is on the right side, so the visual effect from left to right is thirdPicker-secondPicker-firstPicker
        // Verify that key is inverted and correctly moving among first, second and third picker.
        // the initial focus is firstPicker. Compared to test case CanMoveBetweenColumnsWithHorizontalArrowKeys, we move left first, then move right.

        TestCleanupWrapper cleanup;

        xaml_controls::DatePickerFlyout^ datePickerFlyout = nullptr;
        xaml_controls::Button^ button = nullptr;
        xaml_controls::Control^ firstPicker = nullptr;
        xaml_controls::Control^ secondPicker = nullptr;
        xaml_controls::Control^ thirdPicker = nullptr;

        SetupDatePickerFlyoutTest(button, datePickerFlyout);

        LOG_OUTPUT(L"Arrow between the three columns and ensure that focus changes.");

        RunOnUIThread([&]()
        {
            datePickerFlyout->CalendarIdentifier = L"HebrewCalendar"; // make Grid's FlowDirection to RTL
        });
        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::OpenFlyout(datePickerFlyout, button, FlyoutOpenMethod::Keyboard);

        xaml_primitives::LoopingSelector^ dayLoopingSelector;
        xaml_primitives::LoopingSelector^ monthLoopingSelector;
        xaml_primitives::LoopingSelector^ yearLoopingSelector;
        DateTimePickerHelper::GetDayMonthYearLoopingSelectorsFromOpenFlyout(dayLoopingSelector, monthLoopingSelector, yearLoopingSelector);

        firstPicker = dayLoopingSelector;
        secondPicker = monthLoopingSelector;
        thirdPicker = yearLoopingSelector;

        RunOnUIThread([&]()
        {
            VERIFY_ARE_NOT_EQUAL(xaml::FocusState::Unfocused, firstPicker->FocusState);
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, secondPicker->FocusState);
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, thirdPicker->FocusState);
        });

        TestServices::KeyboardHelper->Left();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, firstPicker->FocusState);
            VERIFY_ARE_NOT_EQUAL(xaml::FocusState::Unfocused, secondPicker->FocusState);
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, thirdPicker->FocusState);
        });

        TestServices::KeyboardHelper->Left();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, firstPicker->FocusState);
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, secondPicker->FocusState);
            VERIFY_ARE_NOT_EQUAL(xaml::FocusState::Unfocused, thirdPicker->FocusState);
        });

        TestServices::KeyboardHelper->Right();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, firstPicker->FocusState);
            VERIFY_ARE_NOT_EQUAL(xaml::FocusState::Unfocused, secondPicker->FocusState);
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, thirdPicker->FocusState);
        });

        TestServices::KeyboardHelper->Right();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_NOT_EQUAL(xaml::FocusState::Unfocused, firstPicker->FocusState);
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, secondPicker->FocusState);
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, thirdPicker->FocusState);
        });

        FlyoutHelper::HideFlyout(datePickerFlyout);
    }

    void DatePickerFlyoutIntegrationTests::CanMoveBetweenColumnsWithHorizontalArrowKeys()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::DatePickerFlyout^ datePickerFlyout = nullptr;
        xaml_controls::Button^ button = nullptr;
        xaml_controls::Control^ firstPicker = nullptr;
        xaml_controls::Control^ secondPicker = nullptr;
        xaml_controls::Control^ thirdPicker = nullptr;

        SetupDatePickerFlyoutTest(button, datePickerFlyout);

        LOG_OUTPUT(L"Arrow between the three columns and ensure that focus changes.");

        FlyoutHelper::OpenFlyout(datePickerFlyout, button, FlyoutOpenMethod::Keyboard);

        RunOnUIThread([&]()
        {
            // We don't have direct access to the pickers themselves, but we know
            // that they're the last three items in the picker host grid, so
            // we'll retrieve them that way.
            xaml_controls::Grid^ pickerHostGrid = static_cast<xaml_controls::Grid^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"PickerHostGrid", button));
            firstPicker = static_cast<xaml_controls::Control^>(pickerHostGrid->Children->GetAt(pickerHostGrid->Children->Size - 3));
            secondPicker = static_cast<xaml_controls::Control^>(pickerHostGrid->Children->GetAt(pickerHostGrid->Children->Size - 2));
            thirdPicker = static_cast<xaml_controls::Control^>(pickerHostGrid->Children->GetAt(pickerHostGrid->Children->Size - 1));

            VERIFY_ARE_NOT_EQUAL(xaml::FocusState::Unfocused, firstPicker->FocusState);
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, secondPicker->FocusState);
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, thirdPicker->FocusState);
        });

        TestServices::KeyboardHelper->Right();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, firstPicker->FocusState);
            VERIFY_ARE_NOT_EQUAL(xaml::FocusState::Unfocused, secondPicker->FocusState);
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, thirdPicker->FocusState);
        });

        TestServices::KeyboardHelper->Right();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, firstPicker->FocusState);
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, secondPicker->FocusState);
            VERIFY_ARE_NOT_EQUAL(xaml::FocusState::Unfocused, thirdPicker->FocusState);
        });

        TestServices::KeyboardHelper->Left();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, firstPicker->FocusState);
            VERIFY_ARE_NOT_EQUAL(xaml::FocusState::Unfocused, secondPicker->FocusState);
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, thirdPicker->FocusState);
        });

        TestServices::KeyboardHelper->Left();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_NOT_EQUAL(xaml::FocusState::Unfocused, firstPicker->FocusState);
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, secondPicker->FocusState);
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, thirdPicker->FocusState);
        });

        FlyoutHelper::HideFlyout(datePickerFlyout);

        LOG_OUTPUT(L"Now hide the day column (i.e., the center column) and make sure that arrowing between the three columns skips that column.");

        RunOnUIThread([&]()
        {
            datePickerFlyout->DayVisible = false;
        });

        FlyoutHelper::OpenFlyout(datePickerFlyout, button, FlyoutOpenMethod::Keyboard);

        RunOnUIThread([&]()
        {
            VERIFY_ARE_NOT_EQUAL(xaml::FocusState::Unfocused, firstPicker->FocusState);
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, secondPicker->FocusState);
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, thirdPicker->FocusState);
        });

        TestServices::KeyboardHelper->Right();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, firstPicker->FocusState);
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, secondPicker->FocusState);
            VERIFY_ARE_NOT_EQUAL(xaml::FocusState::Unfocused, thirdPicker->FocusState);
        });

        TestServices::KeyboardHelper->Left();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_NOT_EQUAL(xaml::FocusState::Unfocused, firstPicker->FocusState);
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, secondPicker->FocusState);
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, thirdPicker->FocusState);
        });

        FlyoutHelper::HideFlyout(datePickerFlyout);
    }

    void DatePickerFlyoutIntegrationTests::ValidateAcceptDismissButtonsAreHiddenWithGamepad()
    {
        TestCleanupWrapper cleanup;

        // There is a reliability issue for keyboard injecting that waits for the event from the InputManager
        // after sending the input injection. This is the work around to disable WaitForEvent.
        KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride(KeyboardWaitKind::None);

        xaml_controls::DatePickerFlyout^ datePickerFlyout = nullptr;
        xaml_controls::Button^ button = nullptr;

        SetupDatePickerFlyoutTest(button, datePickerFlyout);

        LOG_OUTPUT(L"Opening the flyout with the keyboard.  The buttons should be visible.");
        FlyoutHelper::OpenFlyout(datePickerFlyout, button, FlyoutOpenMethod::Keyboard);
        RunOnUIThread([&]()
        {
            xaml::UIElement^ acceptDismissHostGrid = TreeHelper::GetVisualChildByNameFromOpenPopups(L"AcceptDismissHostGrid", button);
            VERIFY_ARE_EQUAL(xaml::Visibility::Visible, acceptDismissHostGrid->Visibility);
        });
        FlyoutHelper::HideFlyout(datePickerFlyout);

        LOG_OUTPUT(L"Opening the flyout with touch.  The buttons should be visible.");
        FlyoutHelper::OpenFlyout(datePickerFlyout, button, FlyoutOpenMethod::Touch);
        RunOnUIThread([&]()
        {
            xaml::UIElement^ acceptDismissHostGrid = TreeHelper::GetVisualChildByNameFromOpenPopups(L"AcceptDismissHostGrid", button);
            VERIFY_ARE_EQUAL(xaml::Visibility::Visible, acceptDismissHostGrid->Visibility);
        });
        FlyoutHelper::HideFlyout(datePickerFlyout);

        LOG_OUTPUT(L"Opening the flyout with the gamepad.  The buttons should not be visible.");
        FlyoutHelper::OpenFlyout(datePickerFlyout, button, FlyoutOpenMethod::Gamepad);
        RunOnUIThread([&]()
        {
            xaml::UIElement^ acceptDismissHostGrid = TreeHelper::GetVisualChildByNameFromOpenPopups(L"AcceptDismissHostGrid", button);
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, acceptDismissHostGrid->Visibility);
        });
        FlyoutHelper::HideFlyout(datePickerFlyout);
    }

    void DatePickerFlyoutIntegrationTests::SetupDatePickerFlyoutTest(xaml_controls::Button^& target, xaml_controls::DatePickerFlyout^& datePickerFlyout)
    {
        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                      <Button x:Name="button" Content="Test DatePickerFlyout" >
                        <Button.Flyout>
                          <DatePickerFlyout />
                        </Button.Flyout>
                      </Button>
                    </Grid>)"));
            TestServices::WindowHelper->WindowContent = rootPanel;

            target = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            THROW_IF_NULL(target);

            datePickerFlyout = safe_cast<xaml_controls::DatePickerFlyout^>(target->Flyout);
            THROW_IF_NULL(datePickerFlyout);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void DatePickerFlyoutIntegrationTests::ValidateMinYearAndMaxYearProperties()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::DatePickerFlyout^ datePickerFlyout;
        xaml_controls::Button^ button;
        xaml_primitives::LoopingSelector^ dayLoopingSelector;
        xaml_primitives::LoopingSelector^ monthLoopingSelector;
        xaml_primitives::LoopingSelector^ yearLoopingSelector;
        wg::Calendar^ expectedDate;

        SetupDatePickerFlyoutTest(button, datePickerFlyout);

        RunOnUIThread([&]()
        {
            datePickerFlyout->MinYear = CreateDate(7, 15, 1980)->GetDateTime(); //Only the Year component should have any effect.
            datePickerFlyout->MaxYear = CreateDate(3, 21, 2010)->GetDateTime();

            // Try to set a Date that is too large, it should get clamped by MaxYear.
            datePickerFlyout->Date = CreateDate(6, 6, 2020)->GetDateTime();

            // The date should get clamped to the last day of MaxYear.
            // Note: DatePickerFlyout does not update its Date property immediately. The MinYear/MaxYear properties are only used
            // to constrain the range that can be chosen via the UI. So in order to see the effect of MinYear/MaxYear we
            // need to open and close the DatePickerFlyout.
            expectedDate = CreateDate(12, 31, 2010);
        });

        TestServices::WindowHelper->WaitForIdle();
        FlyoutHelper::OpenFlyout(datePickerFlyout, button, FlyoutOpenMethod::Touch);
        DateTimePickerHelper::GetDayMonthYearLoopingSelectorsFromOpenFlyout(dayLoopingSelector, monthLoopingSelector, yearLoopingSelector);
        VerifyYearLoopingSelectorItems(yearLoopingSelector, 1980, 2010);
        ControlHelper::ClickFlyoutCloseButton(button, true /*isAccept*/);

        RunOnUIThread([&]()
        {
            VerifyDatesAreEqual(expectedDate, datePickerFlyout->Date);

            // Try to set a Date that is too small, it should get clamped by MinYear.
            datePickerFlyout->Date = CreateDate(8, 12, 1970)->GetDateTime();

            // The date should get clamped to the first day of MinYear
            expectedDate = CreateDate(1, 1, 1980);
        });

        TestServices::WindowHelper->WaitForIdle();
        FlyoutHelper::OpenFlyout(datePickerFlyout, button, FlyoutOpenMethod::Touch);
        ControlHelper::ClickFlyoutCloseButton(button, true /*isAccept*/);

        RunOnUIThread([&]()
        {
            VerifyDatesAreEqual(expectedDate, datePickerFlyout->Date);

            // Try to set a date between min and max. It should not get changed.
            auto dateToSet = CreateDate(1, 9, 1984);
            datePickerFlyout->Date = dateToSet->GetDateTime();
            expectedDate = dateToSet;
        });

        TestServices::WindowHelper->WaitForIdle();
        FlyoutHelper::OpenFlyout(datePickerFlyout, button, FlyoutOpenMethod::Touch);
        ControlHelper::ClickFlyoutCloseButton(button, true /*isAccept*/);

        RunOnUIThread([&]()
        {
            VerifyDatesAreEqual(expectedDate, datePickerFlyout->Date);

            // Setting MaxYear to below the currently set date should cause the date to get clamped:
            datePickerFlyout->MaxYear = CreateDate(1, 1, 1982)->GetDateTime();
            expectedDate = CreateDate(12, 31, 1982);
        });

        TestServices::WindowHelper->WaitForIdle();
        FlyoutHelper::OpenFlyout(datePickerFlyout, button, FlyoutOpenMethod::Touch);
        VerifyYearLoopingSelectorItems(yearLoopingSelector, 1980, 1982);
        ControlHelper::ClickFlyoutCloseButton(button, true /*isAccept*/);

        RunOnUIThread([&]()
        {
            VerifyDatesAreEqual(expectedDate, datePickerFlyout->Date);

            datePickerFlyout->MaxYear = CreateDate(1, 1, 2050)->GetDateTime();
            datePickerFlyout->Date = CreateDate(6, 15, 1991)->GetDateTime();

            // Setting MinYear to above the currently set date should cause the date to get clamped:
            datePickerFlyout->MinYear = CreateDate(1, 1, 1995)->GetDateTime();
            expectedDate = CreateDate(1, 1, 1995);
        });

        TestServices::WindowHelper->WaitForIdle();
        FlyoutHelper::OpenFlyout(datePickerFlyout, button, FlyoutOpenMethod::Touch);
        VerifyYearLoopingSelectorItems(yearLoopingSelector, 1995, 2050);
        ControlHelper::ClickFlyoutCloseButton(button, true /*isAccept*/);

        RunOnUIThread([&]()
        {
            VerifyDatesAreEqual(expectedDate, datePickerFlyout->Date);
        });
    }

    void DatePickerFlyoutIntegrationTests::VerifyYearLoopingSelectorItems(xaml_primitives::LoopingSelector^ yearLoopingSelector, unsigned int minYear, unsigned int maxYear)
    {
        // Verifies the Items in the LoopingSelector based on the value of minYear and maxYear.
        //   1. Items should contain the correct number of years
        //   2. The first item string should equal the MinYear
        //   3. The last item string should equal the MaxYear
        //
        // Note:
        // The year items that get created by the DatePickerFlyout include the Left-To-Right marker in the string.

        RunOnUIThread([&]()
        {
            unsigned int expectedNumberOfYears = maxYear - minYear + 1;
            VERIFY_ARE_EQUAL(expectedNumberOfYears, yearLoopingSelector->Items->Size);

            auto minYearItem = safe_cast<xaml_controls::DatePickerFlyoutItem^>(yearLoopingSelector->Items->GetAt(0));
            auto expectedMinYearString = ref new Platform::String(WEX::Common::String().Format(L"\u200E%d", minYear));
            VERIFY_ARE_EQUAL(expectedMinYearString, minYearItem->PrimaryText);

            auto maxYearItem = safe_cast<xaml_controls::DatePickerFlyoutItem^>(yearLoopingSelector->Items->GetAt(expectedNumberOfYears - 1));
            auto expectedMaxYearString = ref new Platform::String(WEX::Common::String().Format(L"\u200E%d", maxYear));
            VERIFY_ARE_EQUAL(expectedMaxYearString, maxYearItem->PrimaryText);
        });
    }

    void DatePickerFlyoutIntegrationTests::ChangingSelectedMonthShouldRecomputeDaysColumn()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::DatePickerFlyout^ datePickerFlyout;
        xaml_controls::Button^ button;
        xaml_primitives::LoopingSelector^ dayLoopingSelector;
        xaml_primitives::LoopingSelector^ monthLoopingSelector;
        xaml_primitives::LoopingSelector^ yearLoopingSelector;

        SetupDatePickerFlyoutTest(button, datePickerFlyout);

        RunOnUIThread([&]()
        {
            // Initial date is in January:
            datePickerFlyout->Date = CreateDate(1, 30, 2015)->GetDateTime();
        });
        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::OpenFlyout(datePickerFlyout, button, FlyoutOpenMethod::Touch);

        DateTimePickerHelper::GetDayMonthYearLoopingSelectorsFromOpenFlyout(dayLoopingSelector, monthLoopingSelector, yearLoopingSelector);

        // January has 31 days:
        VerifyDayLoopingSelectorItems(dayLoopingSelector, 30 /*expectedSelectedDay*/, 31 /*expectedNumberOfDays*/);

        // Change Month to February (index = 1):
        LoopingSelectorHelper::SelectItemByIndex(monthLoopingSelector, 1, LoopingSelectorHelper::SelectionMode::Keyboard);
        TestServices::WindowHelper->WaitForIdle();

        // February has 28 days.
        // Selected day index should get clamped from 30 to 28.
        // Day items collection should get updated to be 1 to 28.
        VerifyDayLoopingSelectorItems(dayLoopingSelector, 28 /*expectedSelectedDay*/, 28 /*expectedNumberOfDays*/);

        FlyoutHelper::HideFlyout(datePickerFlyout);
    }

    void DatePickerFlyoutIntegrationTests::VerifyChangingDayOnClosedDatePickerFlyoutUpdatesCorrectly()
    {
        TestCleanupWrapper cleanup;

        // Regression coverage for:
        // [GitHub] DatePickerFlyout : Selected date does not receive updates from source.

        // The scenario is:
        // 1. Open and Close a DatePickerFlyout
        // 2. Programmatically set a new date that differs only in the 'day' component
        // 3. Open the DatePickerFlyout

        // Due to the bug, the DatePickerFlyout would display the previously selected date instead of the newly assigned date.

        xaml_controls::DatePickerFlyout^ datePickerFlyout;
        xaml_controls::Button^ button;
        SetupDatePickerFlyoutTest(button, datePickerFlyout);

        RunOnUIThread([&]()
        {
            // Jan 1st 2015
            datePickerFlyout->Date = CreateDate(1, 1, 2015)->GetDateTime();
        });
        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::OpenFlyout(datePickerFlyout, button, FlyoutOpenMethod::Programmatic_ShowAt);

        xaml_primitives::LoopingSelector^ dayLoopingSelector;
        xaml_primitives::LoopingSelector^ monthLoopingSelector;
        xaml_primitives::LoopingSelector^ yearLoopingSelector;
        DateTimePickerHelper::GetDayMonthYearLoopingSelectorsFromOpenFlyout(dayLoopingSelector, monthLoopingSelector, yearLoopingSelector);

        VerifyDayLoopingSelectorItems(dayLoopingSelector, 1 /*expectedSelectedDay*/, 31 /*expectedNumberOfDays*/);

        FlyoutHelper::HideFlyout(datePickerFlyout);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Jan 2nd 2015
            datePickerFlyout->Date = CreateDate(1, 2, 2015)->GetDateTime();
        });
        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::OpenFlyout(datePickerFlyout, button, FlyoutOpenMethod::Programmatic_ShowAt);
        TestServices::WindowHelper->WaitForIdle();

        VerifyDayLoopingSelectorItems(dayLoopingSelector, 2 /*expectedSelectedDay*/, 31 /*expectedNumberOfDays*/);
        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::HideFlyout(datePickerFlyout);
    }

    void DatePickerFlyoutIntegrationTests::VerifyDayLoopingSelectorItems(xaml_primitives::LoopingSelector^ dayLoopingSelector, int expectedSelectedDay, unsigned int expectedNumberOfDays)
    {
        // Verifies:
        // 1. The number of items equals expectedNumberOfDays
        // 2. The selected index is correct of expectedSelectedDay
        // 3. The PrimaryText strings of the items matches the expected set of days
        //     - Note, the day items created by the DatePickerFlyout include the Left-To-Right marker (\u200E) in their strings.
        RunOnUIThread([&]()
        {
            auto dayItems = dayLoopingSelector->Items;
            VERIFY_ARE_EQUAL(expectedNumberOfDays, dayItems->Size);

            VERIFY_ARE_EQUAL(expectedSelectedDay, dayLoopingSelector->SelectedIndex + 1);

            for (unsigned int i = 0; i < dayItems->Size; i++)
            {
                auto dayItem = safe_cast<xaml_controls::DatePickerFlyoutItem^>(dayItems->GetAt(i));
                VERIFY_IS_NOT_NULL(dayItem);

                int day = i + 1;
                auto expectedDayString = ref new Platform::String(WEX::Common::String().Format(L"\u200E%d", day));
                VERIFY_ARE_EQUAL(expectedDayString, dayItem->PrimaryText);
            }
        });
    }

    void DatePickerFlyoutIntegrationTests::VerifyDatesAreEqual(wg::Calendar^ expected, wf::DateTime actual)
    {
        // Note: We only compare the date component of the DateTime. We ignore the time component.
        auto actualCalendar = ref new wg::Calendar();
        actualCalendar->SetDateTime(actual);

        VERIFY_ARE_EQUAL(expected->Year, actualCalendar->Year);
        VERIFY_ARE_EQUAL(expected->Month, actualCalendar->Month);
        VERIFY_ARE_EQUAL(expected->Day, actualCalendar->Day);
    }

    void DatePickerFlyoutIntegrationTests::ValidateOverlayBrush()
    {
        TestCleanupWrapper cleanup;
        FlyoutHelper::ValidateOverlayBrush<xaml_controls::DatePickerFlyout>(L"DatePickerLightDismissOverlayBackground");
    }

    void DatePickerFlyoutIntegrationTests::ValidateDateSelectionFiresAutomationEvent()
    {
        TestCleanupWrapper cleanup;
        // Verifies that the DatePickerFlyout can raises a focus change event to the UIA client.
        // A DatePickerFlyout is shown and a Date is selected using the keyboard up/down arrows.

        KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride(KeyboardWaitKind::None);

        xaml_controls::DatePickerFlyout^ datePickerFlyout;
        xaml_controls::Button^ button;
        SetupDatePickerFlyoutTest(button, datePickerFlyout);

        auto originalDate = CreateDate(6, 12, 2015);
        auto dateToSelect = CreateDate(4, 15, 2012);
        auto minYear = CreateDate(1, 1, 1950);

        RunOnUIThread([&]()
        {
            datePickerFlyout->Date = originalDate->GetDateTime();
            datePickerFlyout->MinYear = minYear->GetDateTime();
        });

        FlyoutHelper::OpenFlyout(datePickerFlyout, button, FlyoutOpenMethod::Programmatic_ShowAt);
        TestServices::WindowHelper->WaitForIdle();

        auto focusChangeEvent = std::make_shared<Event>();
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"FlyoutName";
        auto automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
        wrl::ComPtr<AutomationClient::AutomationFocusChangeHandler> automationFocusChangeHandler;
        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            automationFocusChangeHandler.Attach(new AutomationClient::AutomationFocusChangeHandler(
                automationClientManager, focusChangeEvent, TreeScope_Subtree));
            automationFocusChangeHandler->Init();
            automationFocusChangeHandler->AttachEventHandler();
        });

        auto scopeExit1 = wil::scope_exit([&](){
            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                automationFocusChangeHandler->RemoveEventHandler();
            });
        });

        xaml_primitives::LoopingSelector^ loopingSelector[3];
        DateTimePickerHelper::GetDayMonthYearLoopingSelectorsFromOpenFlyout(loopingSelector[0], loopingSelector[1], loopingSelector[2]);

        for (size_t loopingSelectorIdx = 0; loopingSelectorIdx < _countof(loopingSelector); ++loopingSelectorIdx)
        {
            focusChangeEvent->Reset();
            ControlHelper::EnsureFocused(loopingSelector[loopingSelectorIdx]);
            TestServices::WindowHelper->WaitForIdle();
            focusChangeEvent->WaitForDefault();

            for (size_t i=0; i<3; ++i)
            {
                LOG_OUTPUT(L"Press down and wait for focus change");
                focusChangeEvent->Reset();
                TestServices::KeyboardHelper->PressKeySequence(L"$d$_down#$u$_down");
                focusChangeEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();
            }
        }

        TestServices::WindowHelper->WaitForIdle();

        ControlHelper::ClickFlyoutCloseButton(loopingSelector[0], true /* isAccept */);
        loopingSelector[0] = nullptr;
        loopingSelector[1] = nullptr;
        loopingSelector[2] = nullptr;
        TestServices::WindowHelper->WaitForIdle();
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::DatePickerFlyout
