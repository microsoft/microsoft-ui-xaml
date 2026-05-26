// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>

#include <TreeHelper.h>
#include <ControlHelper.h>
#include <FlyoutHelper.h>
#include <LoopingSelectorHelper.h>
#include "WUCRenderingScopeGuard.h"

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    class DateTimePickerHelper
    {
    public:

        // Performs the following steps:
        //  1. Trys to open the Date/TimePicker using the specified key sequence
        //  2. Verifies that the Date/TimePickerFlyout opened by checking that there is exactly 1 open popup
        //     (we do not have a way to get a reference to the Date/TimePickerFlyout itself)
        //  3. Changes the date/time by panning one of the LoopingSelectors.
        //  4. Closes the Date/TimePickerFlyout using the specified key sequence.
        //  5. Verifies that the Date/TimeChanged event fires
        //  6. Verifies that the Date/TimePickerFlyout is closed by checking that there are no open popups.
        // Assumes that the DatePicker/TimePicker is already focused when this method is called.
        static void OpenAndCloseDateTimePickerUsingKeyboard(
            Platform::String^ keySequenceToOpenDateTimePicker,
            Platform::String^ keySequenceToCloseDateTimePicker,
            std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event>& dateTimeChangedEvent,
            bool validateDCompTree = false)
        {
            TestServices::KeyboardHelper->PressKeySequence(keySequenceToOpenDateTimePicker);
            TestServices::WindowHelper->WaitForIdle();
            if (validateDCompTree)
            {
                TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
            }

            RunOnUIThread([&]()
            {
                //There should be exactly one open popup:
                wfc::IVectorView<xaml_primitives::Popup^>^ popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(
                    TestServices::WindowHelper->WindowContent->XamlRoot);
                VERIFY_IS_TRUE(popups->Size == 1);
            });

            dateTimeChangedEvent->Reset();
            LoopingSelectorHelper::PanSingleDateTimeLoopingSelector();

            TestServices::KeyboardHelper->PressKeySequence(keySequenceToCloseDateTimePicker);
            TestServices::WindowHelper->WaitForIdle();
            
            LOG_OUTPUT(L"Waiting for DateChanged/TimeChanged event to fire.");
            dateTimeChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            
            RunOnUIThread([&]()
            {
                //There should be no open popups:
                wfc::IVectorView<xaml_primitives::Popup^>^ popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(
                    TestServices::WindowHelper->WindowContent->XamlRoot);
                VERIFY_IS_TRUE(popups->Size == 0);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // Validate the size and positioning of the flyout launched by tapping on a DatePicker or a TimePicker
        // The template parameter must be either DatePicker or TimePicker.
        template<typename DateTimePicker>
        static void ValidateDateTimePickerFlyoutPositioningAndSizing()
        {
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));

            DateTimePicker^ dateTimePicker = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = ref new xaml_controls::Grid();

                dateTimePicker = ref new DateTimePicker();
                dateTimePicker->VerticalAlignment = xaml::VerticalAlignment::Center;
                dateTimePicker->HorizontalAlignment = xaml::HorizontalAlignment::Center;
                dateTimePicker->Header = L"Header";

                rootPanel->Children->Append(dateTimePicker);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            OpenDateTimePicker(dateTimePicker);

            RunOnUIThread([&]()
            {
                // The flyout should be the same width as the datepicker.
                auto flyoutPresenter = FlyoutHelper::GetOpenFlyoutPresenter();
                VERIFY_ARE_EQUAL(flyoutPresenter->ActualWidth, dateTimePicker->ActualWidth);

                // We expect the HighlightRect to be centered vertically and horizontally over the button.
                auto highlightRect = TreeHelper::GetVisualChildByName(flyoutPresenter, L"HighlightRect");
                auto highlightRectCenter = ControlHelper::GetCenterOfElement(highlightRect);

                auto button = TreeHelper::GetVisualChildByName(dateTimePicker, "FlyoutButton");
                auto buttonCenter = ControlHelper::GetCenterOfElement(button);

                VERIFY_ARE_EQUAL(highlightRectCenter.X, buttonCenter.X);
                VERIFY_ARE_EQUAL(highlightRectCenter.Y, buttonCenter.Y);
            });

            ControlHelper::ClickFlyoutCloseButton(dateTimePicker, true /* isAccept */);
        }

        template<typename DateTimePicker>
        static void OpenDateTimePicker(DateTimePicker^ dateTimePicker)
        {
            xaml_controls::Button^ button = nullptr;

            RunOnUIThread([&]()
            {
                button = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(dateTimePicker, "FlyoutButton"));
            });

            ControlHelper::DoClickUsingTap(button);
            TestServices::WindowHelper->WaitForIdle();
        }

        template<typename FlyoutPresenterType>
        static FlyoutPresenterType^ GetOpenFlyoutPresenter()
        {
            FlyoutPresenterType^ flyoutPresenter = nullptr;

            RunOnUIThread([&]()
            {
                auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(
                    TestServices::WindowHelper->WindowContent->XamlRoot);
                VERIFY_IS_TRUE(popups->Size == 1);
                auto popup = popups->GetAt(0);
                flyoutPresenter = safe_cast<FlyoutPresenterType^>(popup->Child);
            });

            return flyoutPresenter;
        }

        static xaml_controls::DatePickerFlyoutPresenter^ GetOpenDatePickerFlyoutPresenter()
        {
            return GetOpenFlyoutPresenter<xaml_controls::DatePickerFlyoutPresenter>();
        }

        static xaml_controls::TimePickerFlyoutPresenter^ GetOpenTimePickerFlyoutPresenter()
        {
            return GetOpenFlyoutPresenter<xaml_controls::TimePickerFlyoutPresenter>();
        }

        static void GetDayMonthYearLoopingSelectorsFromOpenFlyout(
            xaml_primitives::LoopingSelector^& dayLoopingSelector,
            xaml_primitives::LoopingSelector^& monthLoopingSelector,
            xaml_primitives::LoopingSelector^& yearLoopingSelector)
        {
            RunOnUIThread([&]()
            {
                auto datepickerFlyoutPresenter = GetOpenDatePickerFlyoutPresenter();
                THROW_IF_NULL(datepickerFlyoutPresenter);

                dayLoopingSelector = safe_cast<xaml_primitives::LoopingSelector^>(TreeHelper::GetVisualChildByName(datepickerFlyoutPresenter, "DayLoopingSelector"));
                monthLoopingSelector = safe_cast<xaml_primitives::LoopingSelector^>(TreeHelper::GetVisualChildByName(datepickerFlyoutPresenter, "MonthLoopingSelector"));
                yearLoopingSelector = safe_cast<xaml_primitives::LoopingSelector^>(TreeHelper::GetVisualChildByName(datepickerFlyoutPresenter, "YearLoopingSelector"));
                THROW_IF_NULL(dayLoopingSelector);
                THROW_IF_NULL(monthLoopingSelector);
                THROW_IF_NULL(yearLoopingSelector);
            });
        }

        static void SelectDateInOpenDatePickerFlyout(wg::Calendar^ dateToSelect, int minYear, LoopingSelectorHelper::SelectionMode selectionMode)
        {
            xaml_primitives::LoopingSelector^ dayLoopingSelector;
            xaml_primitives::LoopingSelector^ monthLoopingSelector;
            xaml_primitives::LoopingSelector^ yearLoopingSelector;
            GetDayMonthYearLoopingSelectorsFromOpenFlyout(dayLoopingSelector, monthLoopingSelector, yearLoopingSelector);

            int dayToSelectIndex = dateToSelect->Day - 1; // index is zero based.
            int monthToSelectIndex = dateToSelect->Month - 1; // index is zero based
            int yearToSelectIndex = dateToSelect->Year - minYear; // index is based on value of min year

            LoopingSelectorHelper::SelectItemByIndex(dayLoopingSelector, dayToSelectIndex, selectionMode);
            LoopingSelectorHelper::SelectItemByIndex(monthLoopingSelector, monthToSelectIndex, selectionMode);
            LoopingSelectorHelper::SelectItemByIndex(yearLoopingSelector, yearToSelectIndex, selectionMode);

            ControlHelper::ClickFlyoutCloseButton(dayLoopingSelector, true /* isAccept */);
            RunOnUIThread([&]()
            {
                dayLoopingSelector = nullptr;
                monthLoopingSelector = nullptr;
                yearLoopingSelector = nullptr;
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        static void GetHourMinutePeriodLoopingSelectorsFromOpenFlyout(
            xaml_primitives::LoopingSelector^& hourLoopingSelector,
            xaml_primitives::LoopingSelector^& minuteLoopingSelector,
            xaml_primitives::LoopingSelector^& periodLoopingSelector)
        {
            RunOnUIThread([&]()
            {
                auto timepickerFlyoutPresenter = GetOpenTimePickerFlyoutPresenter();
                THROW_IF_NULL(timepickerFlyoutPresenter);

                // TimePicker does not name its LoopingSelectors, so we cannot find them by name.
                // The best we can do is find them by type, and rely on their order in the tree to distinguish them.
                auto loopingSelectors = ref new Platform::Collections::Vector<xaml_primitives::LoopingSelector^>();
                TreeHelper::GetVisualChildrenByType(timepickerFlyoutPresenter, loopingSelectors);
                WEX::Common::Throw::IfFalse(loopingSelectors->Size == 3, E_FAIL, L"Expected to find 3 LoopingSelectors");
                hourLoopingSelector = loopingSelectors->GetAt(0);
                minuteLoopingSelector = loopingSelectors->GetAt(1);
                periodLoopingSelector = loopingSelectors->GetAt(2);
            });
        }

        static void SelectTimeInOpenTimePickerFlyout(wg::Calendar^ timeToSelect, LoopingSelectorHelper::SelectionMode selectionMode)
        {
            xaml_primitives::LoopingSelector^ hourLoopingSelector;
            xaml_primitives::LoopingSelector^ minuteLoopingSelector;
            xaml_primitives::LoopingSelector^ periodLoopingSelector;
            GetHourMinutePeriodLoopingSelectorsFromOpenFlyout(hourLoopingSelector, minuteLoopingSelector, periodLoopingSelector);

            int hourIndexToSelect = timeToSelect->Hour;
            int minuteIndexToSelect = timeToSelect->Minute;
            int periodIndeexToSelect = timeToSelect->Period - 1; // AM = 1, PM = 2

            LoopingSelectorHelper::SelectItemByIndex(hourLoopingSelector, hourIndexToSelect, selectionMode);
            LoopingSelectorHelper::SelectItemByIndex(minuteLoopingSelector, minuteIndexToSelect, selectionMode);
            LoopingSelectorHelper::SelectItemByIndex(periodLoopingSelector, periodIndeexToSelect, selectionMode);

            ControlHelper::ClickFlyoutCloseButton(hourLoopingSelector, true /* isAccept */);
            TestServices::WindowHelper->WaitForIdle();
        }
    };
}}}}}
