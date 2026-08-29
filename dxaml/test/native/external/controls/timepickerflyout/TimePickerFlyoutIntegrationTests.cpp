// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TimePickerFlyoutIntegrationTests.h"

#include <generic\DependencyObjectTests.h>

#include <XamlTailored.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>

#include <TreeHelper.h>
#include <ControlHelper.h>
#include <Collection.h>
#include <DateTimePickerHelper.h>
#include <PopupHelper.h>


using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace TimePickerFlyout {

    bool TimePickerFlyoutIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool TimePickerFlyoutIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool TimePickerFlyoutIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void TimePickerFlyoutIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::TimePickerFlyout>::CanInstantiate();
    }

    void TimePickerFlyoutIntegrationTests::DoesFlyoutOnTimePickerClicked()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::TimePicker^ timePicker = nullptr;
        xaml_controls::Button^ button = nullptr;
        wf::TimeSpan timeSpanOriginal = {};

        auto timeChangedEvent = std::make_shared<Event>();
        auto timeChangedRegistration = CreateSafeEventRegistration(xaml_controls::TimePicker, TimeChanged);

        timePicker = SetupTimePickerTest();

        RunOnUIThread([&]()
        {
            // Set the time to 6:30AM.
            timeSpanOriginal.Duration = (int64)10000000 * (6 * 60 + 30) * 60;
            timePicker->Time = timeSpanOriginal;

            timeChangedRegistration.Attach(
                timePicker,
                ref new wf::EventHandler<xaml_controls::TimePickerValueChangedEventArgs^>(
                    [timeChangedEvent, &timeSpanOriginal](Platform::Object^ sender, xaml_controls::TimePickerValueChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"DoesFlyoutOnTimePickerClicked: TimePickerValueChanged event fired.");
                VERIFY_ARE_EQUAL((int)(args->OldTime.Duration), (int)(timeSpanOriginal.Duration));
                VERIFY_IS_TRUE((int)(args->NewTime.Duration) != (int)(timeSpanOriginal.Duration));
                timeChangedEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            button = dynamic_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(timePicker, "FlyoutButton"));
            VERIFY_IS_NOT_NULL(button);
        });

        LOG_OUTPUT(L"DoesFlyoutOnTimePickerClicked: launch the time picker flyout by using Tap.");
        ControlHelper::DoClickUsingTap(button);

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"DoesFlyoutOnTimePickerClicked: Pan the looping selector.");
        LoopingSelectorHelper::PanDateTimeLoopingSelector();

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"DoesFlyoutOnTimePickerClicked: Close the picker flyout.");
        ControlHelper::ClickFlyoutCloseButton(button, true /* isAccept */);

        timeChangedEvent->WaitForDefault();

        LOG_OUTPUT(L"DoesFlyoutOnTimePickerClicked: New Time is %s", FormatTime(timePicker)->Data());

        TestServices::WindowHelper->WaitForIdle();
    }

    void TimePickerFlyoutIntegrationTests::CanSelectTime()
    {
        TestCleanupWrapper cleanup;
        // Verifies that the TimePickerFlyout can be used to select a Time.
        // A TimePickerFlyout is shown and a Time is selected using the keyboard up/down arrows.
        // Verifies:
        //    1. The TimePicked event is fired and the TimePickedEventArgs OldTime and NewTime are correct.
        //    2. TimePickerFlyout->Time gets updated to the new value.

        xaml_controls::TimePickerFlyout^ timePickerFlyout;
        xaml_controls::Button^ button;
        SetupTimePickerFlyoutTest(button, timePickerFlyout);

        auto startTime = CreateCalendarTime(4, 30, 1); //4:30 AM
        auto timeToSelect = CreateCalendarTime(5, 27, 2); //5:27 PM

        RunOnUIThread([&]()
        {
            timePickerFlyout->Time = CalendarToTimeSpan(startTime);
        });
        TestServices::WindowHelper->WaitForIdle();

        auto timePickedEvent = std::make_shared<Event>();
        auto timePickedRegistration = CreateSafeEventRegistration(xaml_controls::TimePickerFlyout, TimePicked);

        timePickedRegistration.Attach(timePickerFlyout,
            ref new wf::TypedEventHandler<xaml_controls::TimePickerFlyout^, xaml_controls::TimePickedEventArgs^>(
            [&](xaml_controls::TimePickerFlyout^ s, xaml_controls::TimePickedEventArgs^ args)
        {
            VERIFY_ARE_EQUAL(CalendarToTimeSpan(startTime).Duration, args->OldTime.Duration);
            VERIFY_ARE_EQUAL(CalendarToTimeSpan(timeToSelect).Duration, args->NewTime.Duration);
            timePickedEvent->Set();
        }));

        FlyoutHelper::OpenFlyout(timePickerFlyout, button, FlyoutOpenMethod::Touch);

        DateTimePickerHelper::SelectTimeInOpenTimePickerFlyout(timeToSelect, LoopingSelectorHelper::SelectionMode::Keyboard);
        timePickedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(CalendarToTimeSpan(timeToSelect).Duration, timePickerFlyout->Time.Duration);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void TimePickerFlyoutIntegrationTests::CanSelectTimeWithShowAtAsync()
    {
        // Leak: TimePickerFlyout�ShowAtAsync
        TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

        // Tests the functionality of TimePickerFlyout->ShowAtAsync.
        // Verifies:
        //   1. The IAsyncOperation.Completed handler gets called when a time is selected in the UI.
        //   2. The result passed to the completed handler is the correct TimeSpan value.
        //   3. TimePickerFlyout->Time gets updated to the correct value.
        TestCleanupWrapper cleanup;

        xaml_controls::TimePickerFlyout^ timePickerFlyout;
        xaml_controls::Button^ button;
        SetupTimePickerFlyoutTest(button, timePickerFlyout);

        auto startTime = CreateCalendarTime(4, 30, 1);
        auto timeToSelect = CreateCalendarTime(5, 27, 2);

        RunOnUIThread([&]()
        {
            timePickerFlyout->Time = CalendarToTimeSpan(startTime);
        });
        TestServices::WindowHelper->WaitForIdle();

        using ResultType = Platform::IBox<wf::TimeSpan>^;

        wf::IAsyncOperation<ResultType>^ asyncOperation;
        RunOnUIThread([&]()
        {
            asyncOperation = timePickerFlyout->ShowAtAsync(button);
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
                auto timePicked = result->Value;
                VERIFY_ARE_EQUAL(CalendarToTimeSpan(timeToSelect).Duration, timePicked.Duration);
            }

            showAtAsyncCompleted->Set();
        });

        TestServices::WindowHelper->WaitForIdle();
        DateTimePickerHelper::SelectTimeInOpenTimePickerFlyout(timeToSelect, LoopingSelectorHelper::SelectionMode::Keyboard);
        showAtAsyncCompleted->WaitForDefault();

        auto result = asyncOperation->GetResults();
        
        if (result == nullptr)
        {
           VERIFY_FAIL(L"IAsyncOperation::GetResults() returns null");
        }
        else
        {
           VERIFY_ARE_EQUAL(CalendarToTimeSpan(timeToSelect).Duration, result->Value.Duration);
        }
        TestServices::WindowHelper->WaitForIdle();
    }

    void TimePickerFlyoutIntegrationTests::VerifyBackButtonClosesFlyout()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::TimePickerFlyout^ timePickerFlyout = nullptr;
        xaml_controls::Button^ button = nullptr;

        SetupTimePickerFlyoutTest(button, timePickerFlyout);

        auto closedEvent = std::make_shared<Event>();
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::TimePickerFlyout, Closed);
        closedRegistration.Attach(timePickerFlyout, [&](){ closedEvent->Set(); });

        ShowTimePickerFlyout(timePickerFlyout, button);

        LOG_OUTPUT(L"Close the TimtePickerFlyout using the Back button.");
        bool backButtonPressHandled = false;
        TestServices::Utilities->InjectBackButtonPress(&backButtonPressHandled);
        VERIFY_IS_TRUE(backButtonPressHandled);
        closedEvent->WaitForDefault();

        LOG_OUTPUT(L"After closing a TimePickerFlyout, further back button presses should not get handled");
        TestServices::Utilities->InjectBackButtonPress(&backButtonPressHandled);
        VERIFY_IS_FALSE(backButtonPressHandled);
    }

    void TimePickerFlyoutIntegrationTests::DismissWithCancelButton()
    {
        // Verifies that the TimePickerFlyout can be closed with the Cancel button.
        // Even though we use the UI to change the selection, this should have no effect:
        //    1. The TimePicked event should NOT be fired.
        //    2. The TimePicker->Time property should NOT get changed.
        TestCleanupWrapper cleanup;

        xaml_controls::TimePickerFlyout^ timePickerFlyout;
        xaml_controls::Button^ button;
        SetupTimePickerFlyoutTest(button, timePickerFlyout);

        auto originalTime = CreateTimeSpan(5, 15, 1);

        RunOnUIThread([&]()
        {
            timePickerFlyout->Time = originalTime;
        });
        TestServices::WindowHelper->WaitForIdle();

        // The TimePicked event should not fire when we use the Cancel button.
        auto timePickedEvent = std::make_shared<Event>();
        auto timePickedRegistration = CreateSafeEventRegistration(xaml_controls::TimePickerFlyout, TimePicked);
        timePickedRegistration.Attach(timePickerFlyout, [&]()
        {
            VERIFY_FAIL(L"TimePicked event should not fire");
        });

        auto flyoutClosedEvent = std::make_shared<Event>();
        auto flyoutClosedRegistration = CreateSafeEventRegistration(xaml_controls::TimePickerFlyout, Closed);
        flyoutClosedRegistration.Attach(timePickerFlyout, [&]() { flyoutClosedEvent->Set(); });

        FlyoutHelper::OpenFlyout(timePickerFlyout, button, FlyoutOpenMethod::Touch);
        TestServices::WindowHelper->WaitForIdle();

        // Change the selected time in the open TimePickerFlyout:
        LoopingSelectorHelper::PanDateTimeLoopingSelector();
        TestServices::WindowHelper->WaitForIdle();

        // Dismiss the flyout with the Cancel button.
        ControlHelper::ClickFlyoutCloseButton(button, false /*isAccept*/);
        flyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // Verify that the Time property did not change:
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(originalTime.Duration, timePickerFlyout->Time.Duration);
        });
    }

    void TimePickerFlyoutIntegrationTests::VerifyFlyoutResizesWithSmallHeight()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::TimePickerFlyout^ timePickerFlyout = nullptr;
        xaml_controls::Button^ button = nullptr;

        double originalPresenterHeight = 0.0;
        double originalBackgroundHeight = 0.0;
        double newWindowHeight = 300.0;

        SetupTimePickerFlyoutTest(button, timePickerFlyout);

        ShowTimePickerFlyout(timePickerFlyout, button);

        RunOnUIThread([&]()
        {
            auto timePickerFlyoutPresenter = GetOpenTimePickerFlyoutPresenter();
            auto timePickerFlyoutPresenterBorder = TreeHelper::GetVisualChildByName(timePickerFlyoutPresenter, "Background");

            originalPresenterHeight = timePickerFlyoutPresenter->ActualHeight;
            originalBackgroundHeight = timePickerFlyoutPresenterBorder->ActualHeight;

            LOG_OUTPUT(L"Original presenter and background heights: %f, %f", originalPresenterHeight, originalBackgroundHeight);
        });

        HideTimePickerFlyout(timePickerFlyout);

        LOG_OUTPUT(L"Changing window size to 400x%d.", (int)newWindowHeight);
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, (float)newWindowHeight));
        TestServices::WindowHelper->WaitForIdle();

        ShowTimePickerFlyout(timePickerFlyout, button);

        RunOnUIThread([&]()
        {
            auto timePickerFlyoutPresenter = GetOpenTimePickerFlyoutPresenter();
            auto timePickerFlyoutPresenterBorder = TreeHelper::GetVisualChildByName(timePickerFlyoutPresenter, "Background");

            auto newPresenterHeight = timePickerFlyoutPresenter->ActualHeight;
            auto newBackgroundHeight = timePickerFlyoutPresenterBorder->ActualHeight;

            LOG_OUTPUT(L"New presenter and background heights: %f, %f", newPresenterHeight, newBackgroundHeight);
            LOG_OUTPUT(L"Should both be less than original heights and equal to new window height = %f.", newWindowHeight);

            VERIFY_IS_LESS_THAN(newPresenterHeight, originalPresenterHeight);
            VERIFY_IS_LESS_THAN(newBackgroundHeight, originalBackgroundHeight);

            VERIFY_ARE_EQUAL(newPresenterHeight, newWindowHeight);
            VERIFY_ARE_EQUAL(newBackgroundHeight, newWindowHeight);
        });

        HideTimePickerFlyout(timePickerFlyout);
    }

    void TimePickerFlyoutIntegrationTests::CanMoveBetweenColumnsWithHorizontalArrowKeys()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::TimePickerFlyout^ timePickerFlyout = nullptr;
        xaml_controls::Button^ button = nullptr;
        xaml_controls::Control^ firstPicker = nullptr;
        xaml_controls::Control^ secondPicker = nullptr;
        xaml_controls::Control^ thirdPicker = nullptr;

        SetupTimePickerFlyoutTest(button, timePickerFlyout);

        LOG_OUTPUT(L"Arrow between the three columns and ensure that focus changes.");

        FlyoutHelper::OpenFlyout(timePickerFlyout, button, FlyoutOpenMethod::Keyboard);

        RunOnUIThread([&]()
        {
            xaml_controls::Border^ firstPickerHost = static_cast<xaml_controls::Border^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"FirstPickerHost", button));
            xaml_controls::Border^ secondPickerHost = static_cast<xaml_controls::Border^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"SecondPickerHost", button));
            xaml_controls::Border^ thirdPickerHost = static_cast<xaml_controls::Border^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"ThirdPickerHost", button));
            firstPicker = static_cast<xaml_controls::Control^>(firstPickerHost->Child);
            secondPicker = static_cast<xaml_controls::Control^>(secondPickerHost->Child);
            thirdPicker = static_cast<xaml_controls::Control^>(thirdPickerHost->Child);

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

        FlyoutHelper::HideFlyout(timePickerFlyout);
    }

    // In 24HourClock, Period is not visible. but it is assigned to FirstPickerHost
    void TimePickerFlyoutIntegrationTests::CanMoveBetweenColumnsWithHorizontalArrowKeysNoPeriodPicker()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::TimePickerFlyout^ timePickerFlyout = nullptr;
        xaml_controls::Button^ button = nullptr;
        xaml_controls::Control^ secondPicker = nullptr;
        xaml_controls::Control^ thirdPicker = nullptr;

        SetupTimePickerFlyoutTest(button, timePickerFlyout);
        RunOnUIThread([&]()
        {
            timePickerFlyout->ClockIdentifier = L"24HourClock";
        });

        LOG_OUTPUT(L"Arrow between the two columns and ensure that focus changes.");

        FlyoutHelper::OpenFlyout(timePickerFlyout, button, FlyoutOpenMethod::Keyboard);

        RunOnUIThread([&]()
        {
            xaml_controls::Border^ secondPickerHost = static_cast<xaml_controls::Border^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"SecondPickerHost", button));
            xaml_controls::Border^ thirdPickerHost = static_cast<xaml_controls::Border^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"ThirdPickerHost", button));
            secondPicker = static_cast<xaml_controls::Control^>(secondPickerHost->Child);
            thirdPicker = static_cast<xaml_controls::Control^>(thirdPickerHost->Child);

            VERIFY_ARE_NOT_EQUAL(xaml::FocusState::Unfocused, secondPicker->FocusState);
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, thirdPicker->FocusState);
        });

        TestServices::KeyboardHelper->Right();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, secondPicker->FocusState);
            VERIFY_ARE_NOT_EQUAL(xaml::FocusState::Unfocused, thirdPicker->FocusState);
        });

        TestServices::KeyboardHelper->Left();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_NOT_EQUAL(xaml::FocusState::Unfocused, secondPicker->FocusState);
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, thirdPicker->FocusState);
        });

        FlyoutHelper::HideFlyout(timePickerFlyout);
    }

    void TimePickerFlyoutIntegrationTests::ValidateAcceptDismissButtonsAreHiddenWithGamepad()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::TimePickerFlyout^ timePickerFlyout = nullptr;
        xaml_controls::Button^ button = nullptr;

        SetupTimePickerFlyoutTest(button, timePickerFlyout);

        LOG_OUTPUT(L"Opening the flyout with the keyboard.  The buttons should be visible.");
        FlyoutHelper::OpenFlyout(timePickerFlyout, button, FlyoutOpenMethod::Keyboard);
        RunOnUIThread([&]()
        {
            xaml::UIElement^ acceptDismissHostGrid = TreeHelper::GetVisualChildByNameFromOpenPopups(L"AcceptDismissHostGrid", button);
            VERIFY_ARE_EQUAL(xaml::Visibility::Visible, acceptDismissHostGrid->Visibility);
        });
        FlyoutHelper::HideFlyout(timePickerFlyout);

        LOG_OUTPUT(L"Opening the flyout with  touch.  The buttons should be visible.");
        FlyoutHelper::OpenFlyout(timePickerFlyout, button, FlyoutOpenMethod::Touch);
        RunOnUIThread([&]()
        {
            xaml::UIElement^ acceptDismissHostGrid = TreeHelper::GetVisualChildByNameFromOpenPopups(L"AcceptDismissHostGrid", button);
            VERIFY_ARE_EQUAL(xaml::Visibility::Visible, acceptDismissHostGrid->Visibility);
        });
        FlyoutHelper::HideFlyout(timePickerFlyout);

        LOG_OUTPUT(L"Opening the flyout with the gamepad.  The buttons should not be visible.");
        FlyoutHelper::OpenFlyout(timePickerFlyout, button, FlyoutOpenMethod::Gamepad);
        RunOnUIThread([&]()
        {
            xaml::UIElement^ acceptDismissHostGrid = TreeHelper::GetVisualChildByNameFromOpenPopups(L"AcceptDismissHostGrid", button);
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, acceptDismissHostGrid->Visibility);
        });
        FlyoutHelper::HideFlyout(timePickerFlyout);
    }

    void TimePickerFlyoutIntegrationTests::MinuteIncrementZero()
    {
        TestCleanupWrapper cleanup;

        // 0 is treated as a special case for MinuteIncrement.
        // It has two effects:
        //  - The only item in the Minute LoopingSelector is 00.
        //  - The Minute LoopingSelector does not loop.

        xaml_controls::TimePickerFlyout^ timePickerFlyout;
        xaml_controls::Button^ button;
        SetupTimePickerFlyoutTest(button, timePickerFlyout);

        RunOnUIThread([&]()
        {
            timePickerFlyout->MinuteIncrement = 0;
        });

        ShowTimePickerFlyout(timePickerFlyout, button);

        xaml_primitives::LoopingSelector^ hourLoopingSelector;
        xaml_primitives::LoopingSelector^ minuteLoopingSelector;
        xaml_primitives::LoopingSelector^ periodLoopingSelector;
        DateTimePickerHelper::GetHourMinutePeriodLoopingSelectorsFromOpenFlyout(hourLoopingSelector, minuteLoopingSelector, periodLoopingSelector);

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(minuteLoopingSelector->ShouldLoop);
            VERIFY_ARE_EQUAL(1u, minuteLoopingSelector->Items->Size);
            auto item = safe_cast<xaml_controls::DatePickerFlyoutItem^>(minuteLoopingSelector->Items->GetAt(0));
            VERIFY_ARE_EQUAL(ref new Platform::String(L"00"), item->PrimaryText);
        });
        HideTimePickerFlyout(timePickerFlyout);

        // We also test that changing the MinuteIncrement back to something other than zero, returns the TimePickerFlyout
        // to the expected behavior:

        RunOnUIThread([&]()
        {
            timePickerFlyout->MinuteIncrement = 15;
        });

        ShowTimePickerFlyout(timePickerFlyout, button);
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(minuteLoopingSelector->ShouldLoop);
            VERIFY_ARE_EQUAL(4u, minuteLoopingSelector->Items->Size);
        });
        HideTimePickerFlyout(timePickerFlyout);
    }

    xaml_controls::TimePicker^ TimePickerFlyoutIntegrationTests::SetupTimePickerTest()
    {
        xaml_controls::TimePicker^ timePicker = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::TimePicker, Loaded);

        RunOnUIThread([&]()
        {
            auto rootGrid = ref new xaml_controls::Grid();
            TestServices::WindowHelper->WindowContent = rootGrid;

            timePicker = ref new xaml_controls::TimePicker();
            timePicker->Header = L"TimePickerTest";

            loadedRegistration.Attach(
                timePicker,
                ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
            {
                loadedEvent->Set();
            }));

            rootGrid->Children->Append(timePicker);
        });

        loadedEvent->WaitForDefault();

        return timePicker;
    }

    template <class TTimePickerControl>
    Platform::String^ TimePickerFlyoutIntegrationTests::FormatTime(TTimePickerControl timePickerControl)
    {
        Platform::String^ formattedTime = "";

        RunOnUIThread([&]()
        {
            auto formatter = ref new wg::DateTimeFormatting::DateTimeFormatter("shorttime");
            auto calendar = ref new wg::Calendar();
            calendar->SetToNow();

            int64 timePickerTotalSeconds = timePickerControl->Time.Duration / 10000000;
            int timePickerHours = (int)timePickerTotalSeconds / 3600;
            int timePickerMinutes = (int)(timePickerTotalSeconds % 3600) / 60;

            int timePickerHours12HourClock = timePickerHours % 12;
            if (timePickerHours12HourClock == 0)
            {
                timePickerHours12HourClock = 12;
            }

            calendar->Hour = timePickerHours12HourClock;
            calendar->Minute = timePickerMinutes;
            calendar->Period = (timePickerHours <= 12) ? 1 : 2;

            formattedTime = formatter->Format(calendar->GetDateTime());
        });

        return formattedTime;
    }

    void TimePickerFlyoutIntegrationTests::SetupTimePickerFlyoutTest(xaml_controls::Button^& target, xaml_controls::TimePickerFlyout^& timePickerFlyout)
    {
        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                L"  <Button x:Name='button' Content='Test TimePickerFlyout' > "
                L"    <Button.Flyout> "
                L"      <TimePickerFlyout />"
                L"    </Button.Flyout> "
                L"  </Button> "
                L"</Grid>"));

            VERIFY_IS_NOT_NULL(rootPanel);

            loadedRegistration.Attach(rootPanel, [loadedEvent]()
            {
                loadedEvent->Set();
            });

            target = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            VERIFY_IS_NOT_NULL(target);

            timePickerFlyout = dynamic_cast<xaml_controls::TimePickerFlyout^>(target->Flyout);
            VERIFY_IS_NOT_NULL(timePickerFlyout);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        LOG_OUTPUT(L"Waiting for rootPanel to be loaded...");
        loadedEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();
    }

    xaml_controls::TimePickerFlyoutPresenter^ TimePickerFlyoutIntegrationTests::GetOpenTimePickerFlyoutPresenter()
    {
        xaml_controls::TimePickerFlyoutPresenter^ timePickerFlyoutPresenter = nullptr;

        RunOnUIThread([&]()
        {
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(TestServices::WindowHelper->WindowContent->XamlRoot);
            VERIFY_IS_TRUE(popups->Size == 1);
            auto popup = popups->GetAt(0);
            timePickerFlyoutPresenter = dynamic_cast<xaml_controls::TimePickerFlyoutPresenter^>(popup->Child);
        });

        return timePickerFlyoutPresenter;
    }

    void TimePickerFlyoutIntegrationTests::ShowTimePickerFlyout(xaml_controls::TimePickerFlyout^ timePickerFlyout, xaml_controls::Button^ target)
    {
        auto openedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::TimePickerFlyout, Opened);
        openedRegistration.Attach(timePickerFlyout, [&](){ openedEvent->Set(); });

        RunOnUIThread([&]()
        {
            timePickerFlyout->ShowAt(target);
        });

        openedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void TimePickerFlyoutIntegrationTests::HideTimePickerFlyout(xaml_controls::TimePickerFlyout^ timePickerFlyout)
    {
        RunOnUIThread([&]()
        {
            timePickerFlyout->Hide();
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void TimePickerFlyoutIntegrationTests::ValidateUIElementTree()
    {
        // We do UIElement tree validation for:
        //  1. Dark Theme
        //  2. Light Theme
        //  3. HighContrast Theme
        //  4. PlacementMode=Full
        // We do not do "PlacementMode=Full in themes other than dark" as there is nothing unique in that scenario that is not covered by the other three cases.
        ValidateUIETreeWorker(xaml_primitives::FlyoutPlacementMode::Right, Theme::Dark, "Dark");
        ValidateUIETreeWorker(xaml_primitives::FlyoutPlacementMode::Right, Theme::Light, "Light");
        ValidateUIETreeWorker(xaml_primitives::FlyoutPlacementMode::Right, Theme::HighContrast, "HC");
        ValidateUIETreeWorker(xaml_primitives::FlyoutPlacementMode::Full,  Theme::Dark, "Full");
    }

    void TimePickerFlyoutIntegrationTests::ValidateUIETreeWorker(xaml_primitives::FlyoutPlacementMode placementMode, Theme theme, Platform::String^ variation)
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Panel^ rootPanel = nullptr;
        xaml_controls::TimePickerFlyout^ timePickerFlyout = nullptr;
        xaml_controls::Button^ button = nullptr;

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

        SetupTimePickerFlyoutTest(button, timePickerFlyout);
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
            timePickerFlyout->Time = CreateTimeSpan(14, 30);    // 2:30 PM
            timePickerFlyout->Placement = placementMode;
            timePickerFlyout->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;
        });
        ShowTimePickerFlyout(timePickerFlyout, button);
        validator();

        HideTimePickerFlyout(timePickerFlyout);
    }

    wf::TimeSpan TimePickerFlyoutIntegrationTests::CreateTimeSpan(int hours, int minutes, int period)
    {
        WEX::Common::Throw::If(period != 1 && period != 2, E_FAIL, L"period must be 1 (AM) or 2 (PM)");
        wf::TimeSpan timeSpan = {};
        if (period == 2)
        {
            hours += 12;
        }
        timeSpan.Duration = (int64)10000000 * (hours * 60 + minutes) * 60;
        return timeSpan;
    }

    void TimePickerFlyoutIntegrationTests::ValidateOverlayBrush()
    {
        TestCleanupWrapper cleanup;
        FlyoutHelper::ValidateOverlayBrush<xaml_controls::TimePickerFlyout>(L"TimePickerLightDismissOverlayBackground");
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::TimePickerFlyout
