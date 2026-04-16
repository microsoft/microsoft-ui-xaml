// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "DatePickerIntegrationTests.h"

#include <generic\DependencyObjectTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <ControlHelper.h>
#include <LoopingSelectorHelper.h>
#include <DateTimePickerHelper.h>
#include <ComboBoxHelper.h>
#include <FlyoutHelper.h>
#include <TreeHelper.h>
#include "FeatureFlags.h"
#include "FocusTestHelper.h"
#include <RuntimeEnabledFeatureOverride.h>
#include <WUCRenderingScopeGuard.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace Microsoft::UI::Xaml::Media;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace DatePicker {

    bool DatePickerIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool DatePickerIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool DatePickerIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void DatePickerIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::DatePicker>::CanInstantiate();
    }

    void DatePickerIntegrationTests::VerifyDefaultProperties()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::DatePicker^ datePicker = nullptr;

        RunOnUIThread([&]()
        {
            datePicker = ref new xaml_controls::DatePicker();
            VERIFY_IS_NOT_NULL(datePicker);

            datePicker->Header = L"DatePicker P0";

            TestServices::WindowHelper->WindowContent = datePicker;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(wg::CalendarIdentifiers::Gregorian, datePicker->CalendarIdentifier);
            VERIFY_ARE_EQUAL(xaml_controls::Orientation::Horizontal, datePicker->Orientation);
            VERIFY_ARE_EQUAL(Platform::StringReference(L"day"), datePicker->DayFormat);
            VERIFY_ARE_EQUAL(Platform::StringReference(L"{month.full}"), datePicker->MonthFormat);
            VERIFY_ARE_EQUAL(Platform::StringReference(L"year.full"), datePicker->YearFormat);
            VERIFY_IS_TRUE(datePicker->DayVisible);
            VERIFY_IS_TRUE(datePicker->MonthVisible);
            VERIFY_IS_TRUE(datePicker->YearVisible);

            auto today = ref new wg::Calendar();
            today->SetToNow();

            // Default value of Date should be the null sentinel value.
            VERIFY_ARE_EQUAL(0, datePicker->Date.UniversalTime);
            VERIFY_IS_NULL(datePicker->SelectedDate);

            // Default value of MinYear should be 100 years ago.
            auto datePickerMinYear = ref new wg::Calendar();
            datePickerMinYear->SetDateTime(datePicker->MinYear);
            VERIFY_ARE_EQUAL(today->Year - 100, datePickerMinYear->Year);

            // Default value of MaxYear should be 100 years from now.
            auto datePickerMaxYear = ref new wg::Calendar();
            datePickerMaxYear->SetDateTime(datePicker->MaxYear);
            VERIFY_ARE_EQUAL(today->Year + 100, datePickerMaxYear->Year);
        });
    }

    void DatePickerIntegrationTests::CanFireDateChangedEvent()
    {
        TestCleanupWrapper cleanup;

        auto datePickerValueChangedEvent = std::make_shared<Event>();

        auto datePicker = SetupDatePickerTest();

        RunOnUIThread([&]()
        {
            auto calendarNow = ref new wg::Calendar();
            calendarNow->SetToNow();

            auto selectedDateChangedRegistration = CreateSafeEventRegistration(xaml_controls::DatePicker, SelectedDateChanged);
            selectedDateChangedRegistration.Attach(datePicker, ref new wf::TypedEventHandler<xaml_controls::DatePicker^, xaml_controls::DatePickerSelectedValueChangedEventArgs^>(
                [datePickerValueChangedEvent, &calendarNow]
            (xaml_controls::DatePicker^ sender, xaml_controls::DatePickerSelectedValueChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"CanFireDateChangedEvent: SelectedDateChanged event fired.");

                VERIFY_IS_NULL(args->OldDate);

                auto calendarNew = ref new wg::Calendar();
                calendarNew->SetDateTime(args->NewDate->Value);

                VERIFY_ARE_EQUAL(calendarNow->Year, calendarNew->Year);
                VERIFY_ARE_EQUAL(calendarNow->Month, calendarNew->Month);
                VERIFY_ARE_EQUAL(calendarNow->Day, calendarNew->Day);
            }));

            LOG_OUTPUT(L"CanFireDateChangedEvent: Execute date change from null to today.");
            datePicker->SelectedDate = ref new Platform::Box<wf::DateTime>(calendarNow->GetDateTime());
            selectedDateChangedRegistration.Detach();

            auto calendarChanged = ref new wg::Calendar();
            calendarChanged->Year = 1990;
            calendarChanged->Month = 12;
            calendarChanged->Day = 31;

            auto dateChangedRegistration = CreateSafeEventRegistration(xaml_controls::DatePicker, DateChanged);
            dateChangedRegistration.Attach(datePicker, ref new wf::EventHandler<xaml_controls::DatePickerValueChangedEventArgs^>(
                [datePickerValueChangedEvent, &calendarNow]
            (Platform::Object^ sender, xaml_controls::DatePickerValueChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"CanFireDateChangedEvent: DateChanged event fired.");

                auto calendarOld = ref new wg::Calendar();
                calendarOld->SetDateTime(args->OldDate);

                auto calendarNew = ref new wg::Calendar();
                calendarNew->SetDateTime(args->NewDate);

                VERIFY_ARE_EQUAL(calendarNow->Year, calendarOld->Year);
                VERIFY_ARE_EQUAL(calendarNow->Month, calendarOld->Month);
                VERIFY_ARE_EQUAL(calendarNow->Day, calendarOld->Day);
                VERIFY_ARE_EQUAL(1990, calendarNew->Year);
                VERIFY_ARE_EQUAL(12, calendarNew->Month);
                VERIFY_ARE_EQUAL(31, calendarNew->Day);

                datePickerValueChangedEvent->Set();
            }));

            LOG_OUTPUT(L"CanFireDateChangedEvent: Execute date change from today to 12/31/1990.");
            datePicker->SelectedDate = ref new Platform::Box<wf::DateTime>(calendarChanged->GetDateTime());
            dateChangedRegistration.Detach();
        });

        datePickerValueChangedEvent->WaitForDefault();
    }

    void DatePickerIntegrationTests::CanChooseDateProjectedShadow()
    {
        RuntimeEnabledFeatureOverride featureUseDropShadows(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceProjectedShadowsOnByDefault, true);
        CanChooseDate();
    }

    void DatePickerIntegrationTests::CanChooseDateDropShadow()
    {
        CanChooseDate();
    }

    void DatePickerIntegrationTests::CanChooseDate()
    {
        // Verify that the DatePicker control can be used to choose a Date.
        // We verify:
        //   1. The DateChanged event is fired.
        //         The DatePickerValueChangedEventArgs OldDate and NewDate properties should be correct.
        //   2. DatePicker->Date property gets updated as appropriate.
        WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

        auto datePicker = SetupDatePickerTest();

        auto initialDate = CreateDate(10, 15, 2015);
        auto dateToSelect = CreateDate(8, 18, 2013);
        auto minYear = CreateDate(1, 1, 1950);

        RunOnUIThread([&]()
        {
            datePicker->SelectedDate = ref new Platform::Box<wf::DateTime>(initialDate->GetDateTime());
            datePicker->MinYear = minYear->GetDateTime();
        });

        auto dateChangedEvent = std::make_shared<Event>();
        auto dateChangedRegistration = CreateSafeEventRegistration(xaml_controls::DatePicker, DateChanged);

        dateChangedRegistration.Attach(datePicker,
            ref new wf::EventHandler<xaml_controls::DatePickerValueChangedEventArgs^>(
            [&](Platform::Object^ sender, xaml_controls::DatePickerValueChangedEventArgs^ args)
        {
            VERIFY_ARE_EQUAL(initialDate->GetDateTime(), args->OldDate);
            VERIFY_ARE_EQUAL(dateToSelect->GetDateTime(), args->NewDate);

            dateChangedEvent->Set();
        }));

        DateTimePickerHelper::OpenDateTimePicker(datePicker);
        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

        DateTimePickerHelper::SelectDateInOpenDatePickerFlyout(dateToSelect, minYear->Year, LoopingSelectorHelper::SelectionMode::Keyboard);
        dateChangedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(dateToSelect->GetDateTime(), datePicker->Date);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    xaml_controls::DatePicker^ DatePickerIntegrationTests::SetupDatePickerTest()
    {
        xaml_controls::DatePicker^ datePicker = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::DatePicker, Loaded);

        RunOnUIThread([&]()
        {
            auto rootGrid = ref new xaml_controls::Grid();
            rootGrid->Background = ref new SolidColorBrush(Microsoft::UI::Colors::White);
            TestServices::WindowHelper->WindowContent = rootGrid;

            datePicker = ref new xaml_controls::DatePicker();

            loadedRegistration.Attach(datePicker, [&]() { loadedEvent->Set(); });

            rootGrid->Children->Append(datePicker);
        });

        loadedEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();

        return datePicker;
    }

    void DatePickerIntegrationTests::ValidateDayMonthYearOrderForDifferentLocales()
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
        // 2. The Day/Month/Year TextBlocks are in the appropriate order.
        // 3. The FlowDirection is set correctly.
        // Note: In the DatePicker template's Grid the columns 1 and 3 are used by the cell dividers, so they are skipped.
        // That is why the expected column positions for day/month/year are {0, 2, 4} and not {0, 1, 2}.

        VerifyDayMonthYearOrderAndFlowDirection(L"en-US", 2, 0, 4, xaml::FlowDirection::LeftToRight);
        VerifyDayMonthYearOrderAndFlowDirection(L"en-GB", 0, 2, 4, xaml::FlowDirection::LeftToRight);
        VerifyDayMonthYearOrderAndFlowDirection(L"ar"   , 0, 2, 4, xaml::FlowDirection::RightToLeft);
        VerifyDayMonthYearOrderAndFlowDirection(L"ts-ZA", 4, 2, 0, xaml::FlowDirection::LeftToRight);
    }

    void DatePickerIntegrationTests::VerifyDayMonthYearOrderAndFlowDirection(Platform::String^ locale, int expectedDayTextBlockColumn, int expectedMonthTextBlockColumn, int expectedYearTextBlockColumn, xaml::FlowDirection expectedFlowDirection)
    {
        // This forces the app to use the specified language/locale:
        LOG_OUTPUT(L"VerifyDayMonthYearOrder: Setting ApplicationLanguages::PrimaryLanguageOverride to %s", locale->Begin());
        ::Windows::Globalization::ApplicationLanguages::PrimaryLanguageOverride = locale;

        auto datePicker = SetupDatePickerTest();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto templateRoot = dynamic_cast<xaml::FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(datePicker, 0));
            VERIFY_IS_NOT_NULL(templateRoot);

            auto dayTextBlock = dynamic_cast<xaml_controls::TextBlock^>(templateRoot->FindName("DayTextBlock"));
            auto monthTextBlock = dynamic_cast<xaml_controls::TextBlock^>(templateRoot->FindName("MonthTextBlock"));
            auto yearTextBlock = dynamic_cast<xaml_controls::TextBlock^>(templateRoot->FindName("YearTextBlock"));
            VERIFY_IS_NOT_NULL(dayTextBlock);
            VERIFY_IS_NOT_NULL(monthTextBlock);
            VERIFY_IS_NOT_NULL(yearTextBlock);

            auto dayColumn = dynamic_cast<xaml_controls::ColumnDefinition^>(templateRoot->FindName("DayColumn"));
            auto monthColumn = dynamic_cast<xaml_controls::ColumnDefinition^>(templateRoot->FindName("MonthColumn"));
            auto yearColumn = dynamic_cast<xaml_controls::ColumnDefinition^>(templateRoot->FindName("YearColumn"));
            VERIFY_IS_NOT_NULL(dayColumn);
            VERIFY_IS_NOT_NULL(monthColumn);
            VERIFY_IS_NOT_NULL(yearColumn);

            auto flyoutButtonContentGrid = dynamic_cast<xaml_controls::Grid^>(templateRoot->FindName("FlyoutButtonContentGrid"));
            VERIFY_IS_NOT_NULL(flyoutButtonContentGrid);

            VERIFY_ARE_EQUAL(expectedFlowDirection, flyoutButtonContentGrid->FlowDirection);

            VERIFY_ARE_EQUAL(expectedDayTextBlockColumn, xaml_controls::Grid::GetColumn(dayTextBlock));
            VERIFY_ARE_EQUAL(expectedMonthTextBlockColumn, xaml_controls::Grid::GetColumn(monthTextBlock));
            VERIFY_ARE_EQUAL(expectedYearTextBlockColumn, xaml_controls::Grid::GetColumn(yearTextBlock));

            bool isFound = false;
            unsigned int indexOfDayColumn = 0;
            unsigned int indexOfMonthColumn = 0;
            unsigned int indexOfYearColumn = 0;

            isFound = flyoutButtonContentGrid->ColumnDefinitions->IndexOf(dayColumn, &indexOfDayColumn);
            VERIFY_IS_TRUE(isFound /*dayColumn*/);
            VERIFY_ARE_EQUAL(expectedDayTextBlockColumn, (int)indexOfDayColumn);

            isFound = flyoutButtonContentGrid->ColumnDefinitions->IndexOf(monthColumn, &indexOfMonthColumn);
            VERIFY_IS_TRUE(isFound /*monthColumn*/);
            VERIFY_ARE_EQUAL(expectedMonthTextBlockColumn, (int)indexOfMonthColumn);

            isFound = flyoutButtonContentGrid->ColumnDefinitions->IndexOf(yearColumn, &indexOfYearColumn);
            VERIFY_IS_TRUE(isFound /*yearColumn*/);
            VERIFY_ARE_EQUAL(expectedYearTextBlockColumn, (int)indexOfYearColumn);
        });
    }

    void DatePickerIntegrationTests::ValidateUIElementTree()
    {
        ControlHelper::ValidateUIElementTree(
            wf::Size(500, 600),
            1.f,
            []()
            {
                auto calendar = ref new wg::Calendar();
                calendar->Year = 1990;
                calendar->Month = 12;
                calendar->Day = 31;
                auto testDateTime = calendar->GetDateTime();

                xaml_controls::DatePicker^ restDatePicker = nullptr;
                xaml_controls::DatePicker^ pointerOverDatePicker = nullptr;
                xaml_controls::DatePicker^ pressedDatePicker = nullptr;
                xaml_controls::DatePicker^ disabledDatePicker = nullptr;

                xaml_controls::DatePicker^ focusedRestDatePicker = nullptr;
                xaml_controls::DatePicker^ focusedPointerOverDatePicker = nullptr;

#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
                xaml_controls::DatePicker^ leftHeaderDatePicker = nullptr;
#endif

                xaml_controls::StackPanel^ rootPanel = nullptr;

                RunOnUIThread([&]()
                {
                    rootPanel = ref new xaml_controls::StackPanel();

                    restDatePicker = ref new xaml_controls::DatePicker();
                    restDatePicker->Header = "Rest";
                    restDatePicker->SelectedDate = ref new Platform::Box<wf::DateTime>(testDateTime);
                    restDatePicker->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;
                    rootPanel->Children->Append(restDatePicker);

                    pointerOverDatePicker = ref new xaml_controls::DatePicker();
                    pointerOverDatePicker->Header = "Hover";
                    pointerOverDatePicker->SelectedDate = ref new Platform::Box<wf::DateTime>(testDateTime);
                    pointerOverDatePicker->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;
                    rootPanel->Children->Append(pointerOverDatePicker);

                    pressedDatePicker = ref new xaml_controls::DatePicker();
                    pressedDatePicker->Header = "Pressed";
                    pressedDatePicker->SelectedDate = ref new Platform::Box<wf::DateTime>(testDateTime);
                    pressedDatePicker->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;
                    rootPanel->Children->Append(pressedDatePicker);

                    disabledDatePicker = ref new xaml_controls::DatePicker();
                    disabledDatePicker->Header = "Disabled";
                    disabledDatePicker->SelectedDate = ref new Platform::Box<wf::DateTime>(testDateTime);
                    disabledDatePicker->IsEnabled = false;
                    disabledDatePicker->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;
                    rootPanel->Children->Append(disabledDatePicker);

                    focusedRestDatePicker = ref new xaml_controls::DatePicker();
                    focusedRestDatePicker->Header = "Focused";
                    focusedRestDatePicker->SelectedDate = ref new Platform::Box<wf::DateTime>(testDateTime);
                    focusedRestDatePicker->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;
                    rootPanel->Children->Append(focusedRestDatePicker);

                    focusedPointerOverDatePicker = ref new xaml_controls::DatePicker();
                    focusedPointerOverDatePicker->Header = "Focused Hover";
                    focusedPointerOverDatePicker->SelectedDate = ref new Platform::Box<wf::DateTime>(testDateTime);
                    focusedPointerOverDatePicker->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;
                    rootPanel->Children->Append(focusedPointerOverDatePicker);

#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
                    leftHeaderDatePicker = ref new xaml_controls::DatePicker();
                    leftHeaderDatePicker->Header = "Left Header";
                    leftHeaderDatePicker->HeaderPlacement = xaml_controls::ControlHeaderPlacement::Left;
                    leftHeaderDatePicker->SelectedDate = ref new Platform::Box<wf::DateTime>(testDateTime);
                    leftHeaderDatePicker->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;
                    rootPanel->Children->Append(leftHeaderDatePicker);
#endif

                    TestServices::WindowHelper->WindowContent = rootPanel;
                });

                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    auto pointerOverFlyoutButton = GetFlyoutButtonFromDatePicker(pointerOverDatePicker);
                    VisualStateManager::GoToState(pointerOverFlyoutButton, "PointerOver", false);

                    auto pressedFlyoutButton = GetFlyoutButtonFromDatePicker(pressedDatePicker);
                    VisualStateManager::GoToState(pressedFlyoutButton, "Pressed", false);

                    auto focusedRestFlyoutButton = GetFlyoutButtonFromDatePicker(focusedRestDatePicker);
                    VisualStateManager::GoToState(focusedRestFlyoutButton, "Focused", false);

                    auto focusedPointerOverFlyoutButton = GetFlyoutButtonFromDatePicker(focusedPointerOverDatePicker);
                    VisualStateManager::GoToState(focusedPointerOverFlyoutButton, "PointerOver", false);
                    VisualStateManager::GoToState(focusedPointerOverFlyoutButton, "Focused", false);
                });
                TestServices::WindowHelper->WaitForIdle();

                return rootPanel;
            }
        );
    }

    void DatePickerIntegrationTests::ValidateFlyoutPositioningAndSizing()
    {
        TestCleanupWrapper cleanup;
        DateTimePickerHelper::ValidateDateTimePickerFlyoutPositioningAndSizing<xaml_controls::DatePicker>();
    }

    void DatePickerIntegrationTests::CanOpenAndCloseUsingKeyboard()
    {
        TestCleanupWrapper cleanup;

        auto datePicker = SetupDatePickerTest();

        auto dateChangedEvent = std::make_shared<Event>();
        auto dateChangedRegistration = CreateSafeEventRegistration(xaml_controls::DatePicker, DateChanged);
        dateChangedRegistration.Attach(datePicker, [&]() { dateChangedEvent->Set(); });

        LOG_OUTPUT(L"Ensuring datePicker has focus.");
        FocusTestHelper::EnsureFocus(datePicker, FocusState::Keyboard);

        LOG_OUTPUT(L"Try to open and close DatePicker using space key press");
        DateTimePickerHelper::OpenAndCloseDateTimePickerUsingKeyboard(L" ", L" ", dateChangedEvent);

        LOG_OUTPUT(L"Try to open and close DatePicker using enter");
        DateTimePickerHelper::OpenAndCloseDateTimePickerUsingKeyboard(L"$d$_enter#$u$_enter", L"$d$_enter#$u$_enter", dateChangedEvent);

        LOG_OUTPUT(L"Try to open and close DatePicker using Alt+Down");
        DateTimePickerHelper::OpenAndCloseDateTimePickerUsingKeyboard(L"$d$_alt#$d$_down#$u$_down#$u$_alt", L"$d$_alt#$d$_down#$u$_down#$u$_alt", dateChangedEvent);

        LOG_OUTPUT(L"Try to open and close DatePicker using Alt+Up");
        DateTimePickerHelper::OpenAndCloseDateTimePickerUsingKeyboard(L"$d$_alt#$d$_up#$u$_up#$u$_alt", L"$d$_alt#$d$_up#$u$_up#$u$_alt", dateChangedEvent);
    }

    void DatePickerIntegrationTests::ValidateCalendarIdentifierProperty()
    {
        TestCleanupWrapper cleanup;

        // Validate that the DatePicker.CalendarIdentifier property causes the date to be displayed in format for that Calendar
        // We are not attempting to validate the correctness of what gets displayed. Only that it matches DateTimeFormatter returns for that calendar.

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

            auto calendar = ref new wg::Calendar();
            calendar->Year = 2015;
            calendar->Month = 2;
            calendar->Day = 11;
            calendar->Hour = 1;
            calendar->ChangeCalendarSystem(cid);

            auto dtf = ref new wg::DateTimeFormatting::DateTimeFormatter(L"{month.full}");

            dtf = ref new wg::DateTimeFormatting::DateTimeFormatter(L"{month.full}", dtf->Languages, dtf->GeographicRegion, cid, dtf->Clock);
            auto expectedMonthString = dtf->Format(calendar->GetDateTime());
            dtf = ref new wg::DateTimeFormatting::DateTimeFormatter(L"day", dtf->Languages, dtf->GeographicRegion, cid, dtf->Clock);
            auto expectedDayString = dtf->Format(calendar->GetDateTime());
            dtf = ref new wg::DateTimeFormatting::DateTimeFormatter(L"year.full", dtf->Languages, dtf->GeographicRegion, cid, dtf->Clock);
            auto expectedYearString = dtf->Format(calendar->GetDateTime());


            auto datePicker = SetupDatePickerTest();

            RunOnUIThread([&]()
            {
                datePicker->SelectedDate = ref new Platform::Box<wf::DateTime>(calendar->GetDateTime());
                datePicker->CalendarIdentifier = cid;
            });
            TestServices::WindowHelper->WaitForIdle();

            xaml_controls::TextBlock^ dayTextBlock;
            xaml_controls::TextBlock^ monthTextBlock;
            xaml_controls::TextBlock^ yearTextBlock;
            GetDayMonthYearTextBlocksFromDatePicker(datePicker, dayTextBlock, monthTextBlock, yearTextBlock);

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(dayTextBlock->Text, expectedDayString);
                VERIFY_ARE_EQUAL(monthTextBlock->Text, expectedMonthString);
                VERIFY_ARE_EQUAL(yearTextBlock->Text, expectedYearString);
            });
        }
    }

    void DatePickerIntegrationTests::ValidateDayMonthYearFormatProperties()
    {
        TestCleanupWrapper cleanup;

        Platform::String^ dayFormat = L"{day.integer(2)}";
        Platform::String^ monthFormat = L"{month.abbreviated}";
        Platform::String^ yearFormat = L"{year.abbreviated(2)}";

        auto date = CreateDate()->GetDateTime();

        auto expectedDayString = (ref new wg::DateTimeFormatting::DateTimeFormatter(dayFormat))->Format(date);
        auto expectedMonthString = (ref new wg::DateTimeFormatting::DateTimeFormatter(monthFormat))->Format(date);
        auto expectedYearString = (ref new wg::DateTimeFormatting::DateTimeFormatter(yearFormat))->Format(date);

        auto datePicker = SetupDatePickerTest();

        RunOnUIThread([&]()
        {
            datePicker->DayFormat = dayFormat;
            datePicker->MonthFormat = monthFormat;
            datePicker->YearFormat = yearFormat;
            datePicker->SelectedDate = ref new Platform::Box<wf::DateTime>(date);
        });

        TestServices::WindowHelper->WaitForIdle();

        xaml_controls::TextBlock^ dayTextBlock;
        xaml_controls::TextBlock^ monthTextBlock;
        xaml_controls::TextBlock^ yearTextBlock;
        GetDayMonthYearTextBlocksFromDatePicker(datePicker, dayTextBlock, monthTextBlock, yearTextBlock);

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(dayTextBlock->Text, expectedDayString);
            VERIFY_ARE_EQUAL(monthTextBlock->Text, expectedMonthString);
            VERIFY_ARE_EQUAL(yearTextBlock->Text, expectedYearString);
        });

        // Verify that we can update the properties of DatePicker
        // We want to make sure that the updated values get applied correctly and the old values don't end up getting used.
        RunOnUIThread([&]()
        {
            dayFormat = L"dayofweek";
            monthFormat = L"month.numeric";
            yearFormat = L"year";
            expectedDayString = (ref new wg::DateTimeFormatting::DateTimeFormatter(dayFormat))->Format(date);
            expectedMonthString = (ref new wg::DateTimeFormatting::DateTimeFormatter(monthFormat))->Format(date);
            expectedYearString = (ref new wg::DateTimeFormatting::DateTimeFormatter(yearFormat))->Format(date);
            datePicker->DayFormat = dayFormat;
            datePicker->MonthFormat = monthFormat;
            datePicker->YearFormat = yearFormat;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(dayTextBlock->Text, expectedDayString);
            VERIFY_ARE_EQUAL(monthTextBlock->Text, expectedMonthString);
            VERIFY_ARE_EQUAL(yearTextBlock->Text, expectedYearString);
        });
    }

    void DatePickerIntegrationTests::ValidateDayMonthYearVisibleProperties()
    {
        TestCleanupWrapper cleanup;

        // Verifies that DayVisibile, MonthVisible and YearVisible properties have the appropriate effect.
        // We set these properties and verify:
        //   The corresponding TextBlock gets Collapsed.
        //   The corresponding ColumnDefinition gets removed from the FlyoutButtonContentGrid.ColumnDefinitions

        auto datePicker = SetupDatePickerTest();

        xaml_controls::TextBlock^ dayTextBlock;
        xaml_controls::TextBlock^ monthTextBlock;
        xaml_controls::TextBlock^ yearTextBlock;
        xaml_controls::ColumnDefinition^ dayColumn;
        xaml_controls::ColumnDefinition^ monthColumn;
        xaml_controls::ColumnDefinition^ yearColumn;
        xaml_controls::Grid^ flyoutButtonContentGrid;
        unsigned int columnIndex = 0;

        RunOnUIThread([&]()
        {
            auto templateRoot = dynamic_cast<xaml::FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(datePicker, 0));
            VERIFY_IS_NOT_NULL(templateRoot);

            dayTextBlock = dynamic_cast<xaml_controls::TextBlock^>(templateRoot->FindName("DayTextBlock"));
            monthTextBlock = dynamic_cast<xaml_controls::TextBlock^>(templateRoot->FindName("MonthTextBlock"));
            yearTextBlock = dynamic_cast<xaml_controls::TextBlock^>(templateRoot->FindName("YearTextBlock"));
            VERIFY_IS_NOT_NULL(dayTextBlock);
            VERIFY_IS_NOT_NULL(monthTextBlock);
            VERIFY_IS_NOT_NULL(yearTextBlock);

            dayColumn = dynamic_cast<xaml_controls::ColumnDefinition^>(templateRoot->FindName("DayColumn"));
            monthColumn = dynamic_cast<xaml_controls::ColumnDefinition^>(templateRoot->FindName("MonthColumn"));
            yearColumn = dynamic_cast<xaml_controls::ColumnDefinition^>(templateRoot->FindName("YearColumn"));
            VERIFY_IS_NOT_NULL(dayColumn);
            VERIFY_IS_NOT_NULL(monthColumn);
            VERIFY_IS_NOT_NULL(yearColumn);

            flyoutButtonContentGrid = dynamic_cast<xaml_controls::Grid^>(templateRoot->FindName("FlyoutButtonContentGrid"));
            VERIFY_IS_NOT_NULL(flyoutButtonContentGrid);
        });

        RunOnUIThread([&]()
        {
            datePicker->DayVisible = false;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(dayTextBlock->Visibility == xaml::Visibility::Collapsed);
            VERIFY_IS_FALSE(flyoutButtonContentGrid->ColumnDefinitions->IndexOf(dayColumn, &columnIndex));

            datePicker->DayVisible = true;
            datePicker->MonthVisible = false;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(dayTextBlock->Visibility == xaml::Visibility::Visible);
            VERIFY_IS_TRUE(flyoutButtonContentGrid->ColumnDefinitions->IndexOf(dayColumn, &columnIndex));

            VERIFY_IS_TRUE(monthTextBlock->Visibility == xaml::Visibility::Collapsed);
            VERIFY_IS_FALSE(flyoutButtonContentGrid->ColumnDefinitions->IndexOf(monthColumn, &columnIndex));

            datePicker->MonthVisible = true;
            datePicker->YearVisible = false;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(monthTextBlock->Visibility == xaml::Visibility::Visible);
            VERIFY_IS_TRUE(flyoutButtonContentGrid->ColumnDefinitions->IndexOf(monthColumn, &columnIndex));

            VERIFY_IS_TRUE(yearTextBlock->Visibility == xaml::Visibility::Collapsed);
            VERIFY_IS_FALSE(flyoutButtonContentGrid->ColumnDefinitions->IndexOf(yearColumn, &columnIndex));
        });
    }

    void DatePickerIntegrationTests::ValidateMinYearAndMaxYearProperties()
    {
        TestCleanupWrapper cleanup;

        auto datePicker = SetupDatePickerTest();

        RunOnUIThread([&]()
        {
            datePicker->MinYear = CreateDate(7, 15, 1980)->GetDateTime(); //Only the Year component should have any effect.
            datePicker->MaxYear = CreateDate(3, 21, 2010)->GetDateTime();

            // Try to set a Date that is too large, it should get clamped by MaxYear.
            datePicker->SelectedDate = ref new Platform::Box<wf::DateTime>(CreateDate(6, 6, 2020)->GetDateTime());

            // The date should get clamped to the last day of MaxYear:
            VerifyDatesAreEqual(CreateDate(12, 31, 2010), datePicker->Date);

            // Try to set a Date that is too small, it should get clamped by MinYear.
            datePicker->SelectedDate = ref new Platform::Box<wf::DateTime>(CreateDate(8, 12, 1970)->GetDateTime());

            // The date should get clamped to the first day of MinYear
            VerifyDatesAreEqual(CreateDate(1, 1, 1980), datePicker->Date);

            // Try to set a date between min and max. It should not get changed.
            auto dateToSet = CreateDate(1, 9, 1984);
            datePicker->SelectedDate = ref new Platform::Box<wf::DateTime>(dateToSet->GetDateTime());
            VerifyDatesAreEqual(dateToSet, datePicker->Date);

            // Setting MaxYear to below the currently set date should cause the date to get clamped:
            datePicker->MaxYear = CreateDate(1, 1, 1982)->GetDateTime();
            VerifyDatesAreEqual(CreateDate(12, 31, 1982), datePicker->Date);

            datePicker->MaxYear = CreateDate(1, 1, 2050)->GetDateTime();
            datePicker->SelectedDate = ref new Platform::Box<wf::DateTime>(CreateDate(6, 15, 1991)->GetDateTime());

            // Setting MinYear to above the currently set date should cause the date to get clamped:
            datePicker->MinYear = CreateDate(1, 1, 1995)->GetDateTime();
            VerifyDatesAreEqual(CreateDate(1, 1, 1995), datePicker->Date);
        });
    }

    void DatePickerIntegrationTests::ValidateFlyoutPositioningForRightToLeftCalendar()
    {
        TestCleanupWrapper cleanup;

        // We want to validate that a DatePickerFlyout opens in the correct location for a DatePicker
        // that is displaying an RTL date.

        xaml_controls::DatePicker^ datePicker = nullptr;
        xaml_controls::Button^ button = nullptr;
        xaml::Thickness datePickerMargin = { 5, 0, 10, 0 };

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();

            datePicker = ref new xaml_controls::DatePicker();
            datePicker->Margin = datePickerMargin;

            // The Hewbrew calendar is RTL.
            datePicker->CalendarIdentifier = wg::CalendarIdentifiers::Hebrew;

            rootPanel->Children->Append(datePicker);
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        DateTimePickerHelper::OpenDateTimePicker(datePicker);

        RunOnUIThread([&]()
        {
            auto button = GetFlyoutButtonFromDatePicker(datePicker);
            auto flyoutPopup = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(datePicker->XamlRoot)->GetAt(0);
            auto datepickerFlyoutPresenter = GetDatePickerFlyoutPresenter();

            // The flyout popup, the flyout presenter and the button should have an RTL flow direction.
            // The DatePicker itself should remain in LTR flow direction.
            VERIFY_ARE_EQUAL(datePicker->FlowDirection, xaml::FlowDirection::LeftToRight);
            VERIFY_ARE_EQUAL(flyoutPopup->FlowDirection, xaml::FlowDirection::RightToLeft);
            VERIFY_ARE_EQUAL(datepickerFlyoutPresenter->FlowDirection, xaml::FlowDirection::RightToLeft);
            VERIFY_ARE_EQUAL(button->FlowDirection, xaml::FlowDirection::RightToLeft);

            // The flyout presenter should be the same width as the datepicker.
            VERIFY_IS_TRUE(datepickerFlyoutPresenter->ActualWidth == datePicker->ActualWidth);

            // For a Popup with RTL flowdirection, HorizontalOffset represents the Popup's RIGHT most edge from the LEFT most edge of the screen.
            // For a DatePickerFlyout, we expect its right-most edge to line up with the right-most edge of the DatePicker.
            // The right edge of the DatePicker is equal to its left edge + its width.
            double rightEdgeOfDatePicker = datePickerMargin.Left + datePicker->ActualWidth;
            VERIFY_IS_TRUE(AreClose(flyoutPopup->HorizontalOffset, rightEdgeOfDatePicker, 1.0));
        });

        ControlHelper::ClickFlyoutCloseButton(datePicker, true /* isAccept */);
    }

    void DatePickerIntegrationTests::DatePickerShouldMaintainTime()
    {
        TestCleanupWrapper cleanup;

        // Even though the DatePicker only deals with dates, the Date property is of type DateTime and so has a time component
        // DatePicker should leave the time component of the DateTime intact.

        xaml_controls::DatePicker^ datePicker = nullptr;

        auto calendar = ref new wg::Calendar();
        calendar->ChangeClock(wg::ClockIdentifiers::TwelveHour);
        calendar->Year = 1990;
        calendar->Month = 12;
        calendar->Day = 31;
        calendar->Hour = 6;
        calendar->Minute = 45;
        calendar->Second = 30;
        calendar->Period = 1;

        auto dateChangedEvent = std::make_shared<Event>();
        auto dateChangedRegistration = CreateSafeEventRegistration(xaml_controls::DatePicker, DateChanged);

        RunOnUIThread([&]()
        {
            auto rootGrid = ref new xaml_controls::Grid();
            TestServices::WindowHelper->WindowContent = rootGrid;

            datePicker = ref new xaml_controls::DatePicker();
            datePicker->SelectedDate = ref new Platform::Box<wf::DateTime>(calendar->GetDateTime());

            dateChangedRegistration.Attach(datePicker, [&]() { dateChangedEvent->Set(); });

            rootGrid->Children->Append(datePicker);
        });

        TestServices::WindowHelper->WaitForIdle();

        //We verify that the DateTime was not changed by the DatePicker:
        RunOnUIThread([&]()
        {
            auto calendarNew = ref new wg::Calendar();
            calendarNew->SetDateTime(datePicker->Date);

            VERIFY_ARE_EQUAL(calendarNew->Year, calendar->Year);
            VERIFY_ARE_EQUAL(calendarNew->Month, calendar->Month);
            VERIFY_ARE_EQUAL(calendarNew->Day, calendar->Day);
            VERIFY_ARE_EQUAL(calendarNew->Hour, calendar->Hour);
            VERIFY_ARE_EQUAL(calendarNew->Minute, calendar->Minute);
            VERIFY_ARE_EQUAL(calendarNew->Second, calendar->Second);
            VERIFY_ARE_EQUAL(calendarNew->Period, calendar->Period);
        });

        // We also verify that if the Date is changed by interacting with the DatePicker
        // the time component of the DateTime remains unaffected.
        // This also covers testing the DatePickerFlyout to ensure it handles the time component of
        // the DateTime correctly.

        LOG_OUTPUT(L"Launch the date picker flyout by using Tap.");
        ControlHelper::DoClickUsingTap(GetFlyoutButtonFromDatePicker(datePicker));

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Pan the looping selectors.");
        LoopingSelectorHelper::PanDateTimeLoopingSelector();

        TestServices::WindowHelper->WaitForIdle();

        dateChangedEvent->Reset();
        LOG_OUTPUT(L"Close the picker flyout.");
        ControlHelper::ClickFlyoutCloseButton(datePicker, true /* isAccept */);
        dateChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // After changing the Date, the time component should remain unaffected:
        RunOnUIThread([&]()
        {
            auto calendarNew = ref new wg::Calendar();
            calendarNew->SetDateTime(datePicker->Date);

            VERIFY_ARE_EQUAL(calendarNew->Hour, calendar->Hour);
            VERIFY_ARE_EQUAL(calendarNew->Minute, calendar->Minute);
            VERIFY_ARE_EQUAL(calendarNew->Second, calendar->Second);
            VERIFY_ARE_EQUAL(calendarNew->Period, calendar->Period);
        });
    }

    void DatePickerIntegrationTests::ValidateFootprint()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(500, 600));

        double const expectedDatePickerWidth = 296;
        double const expectedDatePickerWidth_WithWideHeader = 350;

        double const expectedDatePickerHeight = 30;
        double const expectedDatePickerHeight_WithHeader = 19 + 4 + expectedDatePickerHeight;

        xaml_controls::DatePicker^ datePicker;
        xaml_controls::DatePicker^ datePickerWithHeader;
        xaml_controls::DatePicker^ datePickerWithWideHeader;
        xaml_controls::DatePicker^ datePickerStretched;
        xaml_controls::DatePicker^ datePicker_NoYear;
        xaml_controls::DatePicker^ datePicker_NoMonth;
        xaml_controls::DatePicker^ datePicker_NoDay;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <DatePicker x:Name="datePicker" />
                        <DatePicker x:Name="datePickerWithHeader" Header="H" />
                        <DatePicker x:Name="datePickerWithWideHeader" >
                            <DatePicker.Header>
                                <Rectangle Height="19" Width="350" Fill="Red" />
                            </DatePicker.Header>
                        </DatePicker>
                        <DatePicker x:Name="datePickerStretched" HorizontalAlignment="Stretch" />
                        <DatePicker x:Name="datePicker_NoYear" YearVisible="False" />
                        <DatePicker x:Name="datePicker_NoMonth" MonthVisible="False" />
                        <DatePicker x:Name="datePicker_NoDay" DayVisible="False" />
                    </StackPanel>)"));

            datePicker = safe_cast<xaml_controls::DatePicker^>(rootPanel->FindName(L"datePicker"));
            datePickerWithHeader = safe_cast<xaml_controls::DatePicker^>(rootPanel->FindName(L"datePickerWithHeader"));
            datePickerWithWideHeader = safe_cast<xaml_controls::DatePicker^>(rootPanel->FindName(L"datePickerWithWideHeader"));
            datePickerStretched = safe_cast<xaml_controls::DatePicker^>(rootPanel->FindName(L"datePickerStretched"));
            datePicker_NoYear = safe_cast<xaml_controls::DatePicker^>(rootPanel->FindName(L"datePicker_NoYear"));
            datePicker_NoMonth = safe_cast<xaml_controls::DatePicker^>(rootPanel->FindName(L"datePicker_NoMonth"));
            datePicker_NoDay = safe_cast<xaml_controls::DatePicker^>(rootPanel->FindName(L"datePicker_NoDay"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            // Verify Footprint of DatePicker:
            VERIFY_ARE_EQUAL(expectedDatePickerWidth, datePicker->ActualWidth);
            VERIFY_ARE_EQUAL(expectedDatePickerHeight, datePicker->ActualHeight);

            // Verify Footprint of DatePicker with Header:
            VERIFY_ARE_EQUAL(expectedDatePickerWidth, datePickerWithHeader->ActualWidth);
            VERIFY_ARE_EQUAL(expectedDatePickerHeight_WithHeader, datePickerWithHeader->ActualHeight);

            // Verify Footprint of DatePicker with wide Header:
            VERIFY_ARE_EQUAL(expectedDatePickerWidth_WithWideHeader, datePickerWithWideHeader->ActualWidth);
            VERIFY_ARE_EQUAL(expectedDatePickerHeight_WithHeader, datePickerWithWideHeader->ActualHeight);

            // Verify Footprint of DatePicker with no Year/Month/Day:
            VERIFY_ARE_EQUAL(expectedDatePickerWidth, datePicker_NoYear->ActualWidth);
            VERIFY_ARE_EQUAL(expectedDatePickerHeight, datePicker_NoYear->ActualHeight);
            VERIFY_ARE_EQUAL(expectedDatePickerWidth, datePicker_NoMonth->ActualWidth);
            VERIFY_ARE_EQUAL(expectedDatePickerHeight, datePicker_NoMonth->ActualHeight);
            VERIFY_ARE_EQUAL(expectedDatePickerWidth, datePicker_NoDay->ActualWidth);
            VERIFY_ARE_EQUAL(expectedDatePickerHeight, datePicker_NoDay->ActualHeight);
        });
    }

    void DatePickerIntegrationTests::HasPlaceholderTextByDefault()
    {
        TestCleanupWrapper cleanup;

        auto datePicker = SetupDatePickerTest();

        RunOnUIThread([&]()
        {
            VERIFY_IS_NULL(datePicker->SelectedDate);
        });

        VerifyHasPlaceholder(datePicker);
    }

    void DatePickerIntegrationTests::SelectingDateSetsSelectedDate()
    {
        TestCleanupWrapper cleanup;

        auto datePicker = SetupDatePickerTest();
        auto targetDate = CreateDate(1, 1, 2018);

        RunOnUIThread([&]()
        {
            datePicker->MinYear = targetDate->GetDateTime();
        });

        LOG_OUTPUT(L"Selecting January 1, 2018.");
        DateTimePickerHelper::OpenDateTimePicker(datePicker);
        TestServices::WindowHelper->WaitForIdle();

        DateTimePickerHelper::SelectDateInOpenDatePickerFlyout(targetDate, targetDate->Year, LoopingSelectorHelper::SelectionMode::Keyboard);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Date and SelectedDate should now refer to the same date.");
            VerifyDatesAreEqual(targetDate, datePicker->Date);
            VERIFY_IS_NOT_NULL(datePicker->SelectedDate);
            VerifyDatesAreEqual(targetDate, datePicker->SelectedDate->Value);
        });
    }

    void DatePickerIntegrationTests::ValidateSelectedDatePropagatesToDate()
    {
        TestCleanupWrapper cleanup;

        auto datePicker = SetupDatePickerTest();
        auto date = CreateDate(1, 1, 2018);
        auto date2 = CreateDate(2, 2, 2018);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Setting SelectedDate to null.  Date should be the null sentinel value.");
            datePicker->SelectedDate = nullptr;

            VERIFY_ARE_EQUAL(0, datePicker->Date.UniversalTime);

            LOG_OUTPUT(L"Setting SelectedDate to February 2, 2018. Date should change to this value.");
            datePicker->SelectedDate = ref new Platform::Box<wf::DateTime>(date2->GetDateTime());

            VerifyDatesAreEqual(date2, datePicker->Date);
            VERIFY_IS_NOT_NULL(datePicker->SelectedDate);
            VerifyDatesAreEqual(date2, datePicker->SelectedDate->Value);

            LOG_OUTPUT(L"Setting Date to January 1, 2018. SelectedDate should change to this value.");
            datePicker->Date = date->GetDateTime();

            VerifyDatesAreEqual(date, datePicker->Date);
            VERIFY_IS_NOT_NULL(datePicker->SelectedDate);
            VerifyDatesAreEqual(date, datePicker->SelectedDate->Value);

            LOG_OUTPUT(L"Setting Date to the null sentinel value. SelectedDate should become null.");
            wf::DateTime nullDate;
            nullDate.UniversalTime = 0;
            datePicker->Date = nullDate;

            VERIFY_IS_NULL(datePicker->SelectedDate);
        });
    }

    void DatePickerIntegrationTests::CanProgrammaticallyClearSelectedDate()
    {
        TestCleanupWrapper cleanup;

        auto datePicker = SetupDatePickerTest();

        VerifyHasPlaceholder(datePicker);

        RunOnUIThread([&]()
        {
            datePicker->SelectedDate = ref new Platform::Box<wf::DateTime>(CreateDate(1, 1, 2018)->GetDateTime());
        });

        VerifyDoesNotHavePlaceholder(datePicker);

        RunOnUIThread([&]()
        {
            datePicker->SelectedDate = nullptr;
        });

        VerifyHasPlaceholder(datePicker);
    }

    xaml_controls::Button^ DatePickerIntegrationTests::GetFlyoutButtonFromDatePicker(xaml_controls::DatePicker^ datePicker)
    {
        xaml_controls::Button^ flyoutButton = nullptr;

        RunOnUIThread([&]()
        {
            auto templateRoot = dynamic_cast<xaml::FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(datePicker, 0));
            flyoutButton = dynamic_cast<xaml_controls::Button^>(templateRoot->FindName("FlyoutButton"));
        });

        return flyoutButton;
    }

    void DatePickerIntegrationTests::GetDayMonthYearTextBlocksFromDatePicker(xaml_controls::DatePicker^ datePicker, xaml_controls::TextBlock^& dayTextBlock, xaml_controls::TextBlock^& monthTextBlock, xaml_controls::TextBlock^& yearTextBlock)
    {
        RunOnUIThread([&]()
        {
            auto templateRoot = dynamic_cast<xaml::FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(datePicker, 0));
            VERIFY_IS_NOT_NULL(templateRoot);

            dayTextBlock = dynamic_cast<xaml_controls::TextBlock^>(templateRoot->FindName("DayTextBlock"));
            monthTextBlock = dynamic_cast<xaml_controls::TextBlock^>(templateRoot->FindName("MonthTextBlock"));
            yearTextBlock = dynamic_cast<xaml_controls::TextBlock^>(templateRoot->FindName("YearTextBlock"));
            VERIFY_IS_NOT_NULL(dayTextBlock);
            VERIFY_IS_NOT_NULL(monthTextBlock);
            VERIFY_IS_NOT_NULL(yearTextBlock);
        });
    }

    void DatePickerIntegrationTests::CanSelectDateInJapaneseCalendar()
    {
        TestCleanupWrapper cleanup;

        auto datePicker = SetupDatePickerTest();
        auto date = CreateDate(1, 9, 2019);

        RunOnUIThread([&]()
        {
            datePicker->CalendarIdentifier = L"JapaneseCalendar";

            VERIFY_ARE_EQUAL(0, datePicker->Date.UniversalTime);

            LOG_OUTPUT(L"Setting SelectedDate to January 9, 2019. Date should change to this value.");
            datePicker->SelectedDate = ref new Platform::Box<wf::DateTime>(date->GetDateTime());

            VerifyDatesAreEqual(date, datePicker->Date);
            VERIFY_IS_NOT_NULL(datePicker->SelectedDate);
            VerifyDatesAreEqual(date, datePicker->SelectedDate->Value);

            LOG_OUTPUT(L"Setting Date back to the null sentinel value. SelectedDate should become null.");
            wf::DateTime nullDate;
            nullDate.UniversalTime = 0;
            datePicker->Date = nullDate;

            VERIFY_IS_NULL(datePicker->SelectedDate);
        });
    }

    xaml_controls::DatePickerFlyoutPresenter^ DatePickerIntegrationTests::GetDatePickerFlyoutPresenter()
    {
        return safe_cast<xaml_controls::DatePickerFlyoutPresenter^>(FlyoutHelper::GetOpenFlyoutPresenter());
    }

    void DatePickerIntegrationTests::VerifyDatesAreEqual(wg::Calendar^ expected, wf::DateTime actual)
    {
        auto actualCalendar = ref new wg::Calendar();
        actualCalendar->SetDateTime(actual);

        VERIFY_ARE_EQUAL(expected->Year, actualCalendar->Year);
        VERIFY_ARE_EQUAL(expected->Month, actualCalendar->Month);
        VERIFY_ARE_EQUAL(expected->Day, actualCalendar->Day);
    }

    void DatePickerIntegrationTests::VerifyHasPlaceholder(xaml_controls::DatePicker^ datePicker)
    {
        RunOnUIThread([&]()
        {
            auto dayTextBlock = safe_cast<xaml_controls::TextBlock^>(TreeHelper::GetVisualChildByName(datePicker, L"DayTextBlock"));
            auto monthTextBlock = safe_cast<xaml_controls::TextBlock^>(TreeHelper::GetVisualChildByName(datePicker, L"MonthTextBlock"));
            auto yearTextBlock = safe_cast<xaml_controls::TextBlock^>(TreeHelper::GetVisualChildByName(datePicker, L"YearTextBlock"));

            auto validatePlaceholder = [](xaml_controls::TextBlock^ textBlock, Platform::String^ expectedPlaceholder)
            {
                LOG_OUTPUT(L"Expected placeholder: \"%s\"", expectedPlaceholder->Data());
                LOG_OUTPUT(L"Actual text: \"%s\"", textBlock->Text->Data());

                VERIFY_IS_TRUE(Platform::String::CompareOrdinal(expectedPlaceholder, textBlock->Text) == 0);
            };

            validatePlaceholder(dayTextBlock, L"day");
            validatePlaceholder(monthTextBlock, L"month");
            validatePlaceholder(yearTextBlock, L"year");
        });
    }

    void DatePickerIntegrationTests::VerifyDoesNotHavePlaceholder(xaml_controls::DatePicker^ datePicker)
    {
        RunOnUIThread([&]()
        {
            auto dayTextBlock = safe_cast<xaml_controls::TextBlock^>(TreeHelper::GetVisualChildByName(datePicker, L"DayTextBlock"));
            auto monthTextBlock = safe_cast<xaml_controls::TextBlock^>(TreeHelper::GetVisualChildByName(datePicker, L"MonthTextBlock"));
            auto yearTextBlock = safe_cast<xaml_controls::TextBlock^>(TreeHelper::GetVisualChildByName(datePicker, L"YearTextBlock"));

            auto validateValue = [](xaml_controls::TextBlock^ textBlock, Platform::String^ placeholder)
            {
                LOG_OUTPUT(L"Placeholder: \"%s\"", placeholder->Data());
                LOG_OUTPUT(L"Actual text: \"%s\"", textBlock->Text->Data());

                VERIFY_IS_TRUE(Platform::String::CompareOrdinal(placeholder, textBlock->Text) != 0);
            };

            validateValue(dayTextBlock, L"day");
            validateValue(monthTextBlock, L"month");
            validateValue(yearTextBlock, L"year");
        });
    }

    void DatePickerIntegrationTests::ValidateAutomationNameWhenDateIsNull()
    {
        TestCleanupWrapper cleanup;

        auto datePicker = SetupDatePickerTest();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Setting SelectedDate to null.");
            datePicker->SelectedDate = nullptr;

            auto automationPeer = Microsoft::UI::Xaml::Automation::Peers::FrameworkElementAutomationPeer::CreatePeerForElement(datePicker);
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal("date picker", automationPeer->GetName()) == 0);
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::DatePicker
