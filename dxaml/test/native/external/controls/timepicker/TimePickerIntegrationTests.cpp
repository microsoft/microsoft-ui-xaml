// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TimePickerIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>

#include <generic\DependencyObjectTests.h>

#include <ControlHelper.h>
#include <DateTimePickerHelper.h>
#include <ComboBoxHelper.h>
#include "FeatureFlags.h"
#include "FocusTestHelper.h"
#include "KeyboardInjectionOverride.h"
#include <RuntimeEnabledFeatureOverride.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Media;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace TimePicker {

    bool TimePickerIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool TimePickerIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool TimePickerIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void TimePickerIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::TimePicker>::CanInstantiate();
    }

    void TimePickerIntegrationTests::VerifyDefaultProperties()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::TimePicker^ timePicker = nullptr;

        RunOnUIThread([&]()
        {
            timePicker = ref new xaml_controls::TimePicker();
            VERIFY_IS_NOT_NULL(timePicker);

            VERIFY_ARE_EQUAL(-1, timePicker->Time.Duration);

            timePicker->Header = L"TimePickerTest P0";

            TestServices::WindowHelper->WindowContent = timePicker;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(wg::ClockIdentifiers::TwelveHour, timePicker->ClockIdentifier);
            VERIFY_ARE_EQUAL(1, timePicker->MinuteIncrement);
        });

        timePicker = SetupTimePickerTest();
        DateTimePickerHelper::OpenDateTimePicker(timePicker);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto timePickerFlyoutPresenter = TreeHelper::GetVisualChildByTypeFromOpenPopups<xaml_controls::TimePickerFlyoutPresenter>(timePicker);
            VERIFY_IS_NOT_NULL(timePickerFlyoutPresenter);
            VERIFY_ARE_EQUAL(true, timePickerFlyoutPresenter->IsDefaultShadowEnabled);
        });
    }

    void TimePickerIntegrationTests::CanFireTimeChangedEvent()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::TimePicker^ timePicker = nullptr;

        auto timePickerValueChangedEvent = std::make_shared<Event>();

        wf::TimeSpan timeSpanOriginal = {};
        wf::TimeSpan timeSpanNew = {};

        timePicker = SetupTimePickerTest();

        RunOnUIThread([&]()
        {
            auto selectedTimeChangedRegistration = CreateSafeEventRegistration(xaml_controls::TimePicker, SelectedTimeChanged);
            selectedTimeChangedRegistration.Attach(timePicker, ref new wf::TypedEventHandler<xaml_controls::TimePicker^, xaml_controls::TimePickerSelectedValueChangedEventArgs^>(
                [timePickerValueChangedEvent, &timeSpanOriginal, &timeSpanNew]
            (xaml_controls::TimePicker^ sender, xaml_controls::TimePickerSelectedValueChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"CanFireTimeChangedEvent: TimePickerSelectedValueChanged event fired.");

                VERIFY_IS_NULL(args->OldTime);
                VERIFY_ARE_EQUAL((int)(args->NewTime->Value.Duration), (int)(timeSpanNew.Duration));

                timePickerValueChangedEvent->Set();
            }));

            LOG_OUTPUT(L"CanFireTimeChangedEvent: Execute time change from null to 2:15.");
            timeSpanOriginal = timeSpanNew = CreateTimeSpan(2, 15);
            timePicker->SelectedTime = ref new Platform::Box<wf::TimeSpan>(timeSpanOriginal);
            selectedTimeChangedRegistration.Detach();

            auto timeChangedRegistration = CreateSafeEventRegistration(xaml_controls::TimePicker, TimeChanged);
            timeChangedRegistration.Attach(timePicker, ref new wf::EventHandler<xaml_controls::TimePickerValueChangedEventArgs^>(
                [timePickerValueChangedEvent, &timeSpanOriginal, &timeSpanNew]
                    (Platform::Object^ sender, xaml_controls::TimePickerValueChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"CanFireTimeChangedEvent: TimePickerValueChanged event fired.");

                VERIFY_ARE_EQUAL((int)(args->OldTime.Duration), (int)(timeSpanOriginal.Duration));
                VERIFY_ARE_EQUAL((int)(args->NewTime.Duration), (int)(timeSpanNew.Duration));

                timePickerValueChangedEvent->Set();
            }));

            // Set the time to 4:30AM.
            timeSpanNew = CreateTimeSpan(4, 30);

            LOG_OUTPUT(L"CanFireTimeChangedEvent: Execute time change from 2:15 to 4:30.");
            timePicker->SelectedTime = ref new Platform::Box<wf::TimeSpan>(timeSpanNew);
            timeChangedRegistration.Detach();
        });

        timePickerValueChangedEvent->WaitForDefault();
    }

    xaml_controls::TimePicker^ TimePickerIntegrationTests::SetupTimePickerTest()
    {
        xaml_controls::TimePicker^ timePicker = nullptr;

        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::TimePicker, Loaded);
        auto loadedEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            auto rootGrid = ref new xaml_controls::Grid();
            rootGrid->Background = ref new SolidColorBrush(Microsoft::UI::Colors::White);
            TestServices::WindowHelper->WindowContent = rootGrid;

            timePicker = ref new xaml_controls::TimePicker();
            timePicker->Header = L"TimePickerTest";

            loadedRegistration.Attach(timePicker, ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"TimePickerIntegrationTests: Loaded TimePicker.");
                loadedEvent->Set();
            }));

            rootGrid->Children->Append(timePicker);
        });

        loadedEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();

        return timePicker;
    }

    void TimePickerIntegrationTests::ValidateUIElementTree()
    {
        ControlHelper::ValidateUIElementTree(
            wf::Size(400, 600),
            1.f,
            []()
            {
                wf::TimeSpan testTimeSpan = CreateTimeSpan(4, 30); // 4:30 AM

                xaml_controls::TimePicker^ restTimePicker = nullptr;
                xaml_controls::TimePicker^ pointerOverTimePicker = nullptr;
                xaml_controls::TimePicker^ pressedTimePicker = nullptr;
                xaml_controls::TimePicker^ disabledTimePicker = nullptr;

                xaml_controls::TimePicker^ focusedRestTimePicker = nullptr;
                xaml_controls::TimePicker^ focusedPointerOverTimePicker = nullptr;

#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
                xaml_controls::TimePicker^ leftHeaderTimePicker = nullptr;
#endif

                xaml_controls::StackPanel^ rootPanel = nullptr;

                RunOnUIThread([&]()
                {
                    rootPanel = ref new xaml_controls::StackPanel();

                    restTimePicker = ref new xaml_controls::TimePicker();
                    restTimePicker->Header = "Rest";
                    restTimePicker->SelectedTime = ref new Platform::Box<wf::TimeSpan>(testTimeSpan);
                    restTimePicker->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;
                    rootPanel->Children->Append(restTimePicker);

                    pointerOverTimePicker = ref new xaml_controls::TimePicker();
                    pointerOverTimePicker->Header = "Hover";
                    pointerOverTimePicker->SelectedTime = ref new Platform::Box<wf::TimeSpan>(testTimeSpan);
                    pointerOverTimePicker->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;
                    rootPanel->Children->Append(pointerOverTimePicker);

                    pressedTimePicker = ref new xaml_controls::TimePicker();
                    pressedTimePicker->Header = "Pressed";
                    pressedTimePicker->SelectedTime = ref new Platform::Box<wf::TimeSpan>(testTimeSpan);
                    pressedTimePicker->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;
                    rootPanel->Children->Append(pressedTimePicker);

                    disabledTimePicker = ref new xaml_controls::TimePicker();
                    disabledTimePicker->Header = "Disabled";
                    disabledTimePicker->SelectedTime = ref new Platform::Box<wf::TimeSpan>(testTimeSpan);
                    disabledTimePicker->IsEnabled = false;
                    disabledTimePicker->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;
                    rootPanel->Children->Append(disabledTimePicker);

                    focusedRestTimePicker = ref new xaml_controls::TimePicker();
                    focusedRestTimePicker->Header = "Focused";
                    focusedRestTimePicker->SelectedTime = ref new Platform::Box<wf::TimeSpan>(testTimeSpan);
                    focusedRestTimePicker->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;
                    rootPanel->Children->Append(focusedRestTimePicker);

                    focusedPointerOverTimePicker = ref new xaml_controls::TimePicker();
                    focusedPointerOverTimePicker->Header = "Focused Hover";
                    focusedPointerOverTimePicker->SelectedTime = ref new Platform::Box<wf::TimeSpan>(testTimeSpan);
                    focusedPointerOverTimePicker->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;
                    rootPanel->Children->Append(focusedPointerOverTimePicker);

#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
                    leftHeaderTimePicker = ref new xaml_controls::TimePicker();
                    leftHeaderTimePicker->Header = "Left Header";
                    leftHeaderTimePicker->HeaderPlacement = xaml_controls::ControlHeaderPlacement::Left;
                    leftHeaderTimePicker->SelectedTime = ref new Platform::Box<wf::TimeSpan>(testTimeSpan);
                    leftHeaderTimePicker->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;
                    rootPanel->Children->Append(leftHeaderTimePicker);
#endif

                    TestServices::WindowHelper->WindowContent = rootPanel;
                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    auto pointerOverFlyoutButton = GetFlyoutButtonFromTimePicker(pointerOverTimePicker);
                    VisualStateManager::GoToState(pointerOverFlyoutButton, "PointerOver", false);

                    auto pressedFlyoutButton = GetFlyoutButtonFromTimePicker(pressedTimePicker);
                    VisualStateManager::GoToState(pressedFlyoutButton, "Pressed", false);

                    auto focusedRestFlyoutButton = GetFlyoutButtonFromTimePicker(focusedRestTimePicker);
                    VisualStateManager::GoToState(focusedRestFlyoutButton, "Focused", false);

                    auto focusedPointerOverFlyoutButton = GetFlyoutButtonFromTimePicker(focusedPointerOverTimePicker);
                    VisualStateManager::GoToState(focusedPointerOverFlyoutButton, "PointerOver", false);
                    VisualStateManager::GoToState(focusedPointerOverFlyoutButton, "Focused", false);
                });
                TestServices::WindowHelper->WaitForIdle();

                return rootPanel;
            });
    }

    void TimePickerIntegrationTests::CanOpenAndCloseUsingKeyboardProjectedShadow()
    {
        RuntimeEnabledFeatureOverride featureUseDropShadows(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceProjectedShadowsOnByDefault, true);
        CanOpenAndCloseUsingKeyboard();
    }

    void TimePickerIntegrationTests::CanOpenAndCloseUsingKeyboardDropShadow()
    {
        CanOpenAndCloseUsingKeyboard();
    }

    void TimePickerIntegrationTests::CanOpenAndCloseUsingKeyboard()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

        KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride(KeyboardWaitKind::None);

        auto timePicker = SetupTimePickerTest();

        RunOnUIThread([&]()
        {
            auto time = CreateTimeSpan(5, 45, 1);
            timePicker->SelectedTime = ref new Platform::Box<wf::TimeSpan>(time);
        });
        TestServices::WindowHelper->WaitForIdle();

        auto timeChangedEvent = std::make_shared<Event>();
        auto timeChangedRegistration = CreateSafeEventRegistration(xaml_controls::TimePicker, TimeChanged);
        timeChangedRegistration.Attach(timePicker, [timeChangedEvent]()
        {
            LOG_OUTPUT(L"TimePicker.TimeChanged event fired");
            timeChangedEvent->Set();
        });

        LOG_OUTPUT(L"Ensuring timePicker has focus.");
        FocusTestHelper::EnsureFocus(timePicker, FocusState::Keyboard);

        LOG_OUTPUT(L"Try to open and close TimePicker using space key press");
        DateTimePickerHelper::OpenAndCloseDateTimePickerUsingKeyboard(L" ", L" ", timeChangedEvent, true);

        LOG_OUTPUT(L"Try to open and close TimePicker using enter");
        DateTimePickerHelper::OpenAndCloseDateTimePickerUsingKeyboard(L"$d$_enter#$u$_enter", L"$d$_enter#$u$_enter", timeChangedEvent);

        LOG_OUTPUT(L"Try to open and close TimePicker using Alt+Down");
        DateTimePickerHelper::OpenAndCloseDateTimePickerUsingKeyboard(L"$d$_alt#$d$_down#$u$_down#$u$_alt", L"$d$_alt#$d$_down#$u$_down#$u$_alt", timeChangedEvent);

        LOG_OUTPUT(L"Try to open and close TimePicker using Alt+Up");
        DateTimePickerHelper::OpenAndCloseDateTimePickerUsingKeyboard(L"$d$_alt#$d$_up#$u$_up#$u$_alt", L"$d$_alt#$d$_up#$u$_up#$u$_alt", timeChangedEvent);
    }

    void TimePickerIntegrationTests::ValidateFootprint()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(500, 600));

        double const expectedTimePickerWidth = 242;
        double const expectedTimePickerWidth_WithWideHeader = 350;

        double const expectedTimePickerHeight = 30;
        double const expectedTimePickerHeight_WithHeader = 19 + 4 + expectedTimePickerHeight;

        xaml_controls::TimePicker^ timePicker;
        xaml_controls::TimePicker^ timePickerWithHeader;
        xaml_controls::TimePicker^ timePickerWithWideHeader;
        xaml_controls::TimePicker^ timePickerStretched;
        xaml_controls::TimePicker^ timePicker24Hour;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <TimePicker x:Name="timePicker" />
                        <TimePicker x:Name="timePickerWithHeader" Header="H" />
                        <TimePicker x:Name="timePickerWithWideHeader" >
                            <TimePicker.Header>
                                <Rectangle Height="19" Width="350" Fill="Red" />
                            </TimePicker.Header>
                        </TimePicker>
                        <TimePicker x:Name="timePickerStretched" HorizontalAlignment="Stretch" />
                        <TimePicker x:Name="timePicker24Hour" ClockIdentifier="24HourClock" />
                    </StackPanel>)"));

            timePicker = safe_cast<xaml_controls::TimePicker^>(rootPanel->FindName(L"timePicker"));
            timePickerWithHeader = safe_cast<xaml_controls::TimePicker^>(rootPanel->FindName(L"timePickerWithHeader"));
            timePickerWithWideHeader = safe_cast<xaml_controls::TimePicker^>(rootPanel->FindName(L"timePickerWithWideHeader"));
            timePickerStretched = safe_cast<xaml_controls::TimePicker^>(rootPanel->FindName(L"timePickerStretched"));
            timePicker24Hour = safe_cast<xaml_controls::TimePicker^>(rootPanel->FindName(L"timePicker24Hour"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            // Verify Footprint of TimePicker:
            VERIFY_ARE_EQUAL(expectedTimePickerWidth, timePicker->ActualWidth);
            VERIFY_ARE_EQUAL(expectedTimePickerHeight, timePicker->ActualHeight);

            // Verify Footprint of TimePicker with Header:
            VERIFY_ARE_EQUAL(expectedTimePickerWidth, timePickerWithHeader->ActualWidth);
            VERIFY_ARE_EQUAL(expectedTimePickerHeight_WithHeader, timePickerWithHeader->ActualHeight);

            // Verify Footprint of TimePicker with wide Header:
            VERIFY_ARE_EQUAL(expectedTimePickerWidth_WithWideHeader, timePickerWithWideHeader->ActualWidth);
            VERIFY_ARE_EQUAL(expectedTimePickerHeight_WithHeader, timePickerWithWideHeader->ActualHeight);

            // Verify Footprint of TimePicker with 24Hour Clock:
            VERIFY_ARE_EQUAL(expectedTimePickerWidth, timePicker24Hour->ActualWidth);
            VERIFY_ARE_EQUAL(expectedTimePickerHeight, timePicker24Hour->ActualHeight);
        });
    }

    void TimePickerIntegrationTests::ValidateFlyoutPositioningAndSizing()
    {
        TestCleanupWrapper cleanup;
        DateTimePickerHelper::ValidateDateTimePickerFlyoutPositioningAndSizing<xaml_controls::TimePicker>();
    }

    void TimePickerIntegrationTests::HasPlaceholderTextByDefault()
    {
        TestCleanupWrapper cleanup;

        auto timePicker = SetupTimePickerTest();

        RunOnUIThread([&]()
        {
            VERIFY_IS_NULL(timePicker->SelectedTime);
        });

        VerifyHasPlaceholder(timePicker);
    }

    void TimePickerIntegrationTests::SelectingTimeSetsSelectedTime()
    {
        TestCleanupWrapper cleanup;

        auto timePicker = SetupTimePickerTest();
        auto targetTime = CreateTime(4, 30, 2);
        targetTime->Second = 0;

        LOG_OUTPUT(L"Selecting 4:30 PM.");
        DateTimePickerHelper::OpenDateTimePicker(timePicker);
        TestServices::WindowHelper->WaitForIdle();

        DateTimePickerHelper::SelectTimeInOpenTimePickerFlyout(targetTime, LoopingSelectorHelper::SelectionMode::Keyboard);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto targetTimeSpan = CalendarToTimeSpan(targetTime);

            LOG_OUTPUT(L"Time and SelectedTime should now refer to the same date.");
            VERIFY_ARE_EQUAL(targetTimeSpan.Duration, timePicker->Time.Duration);
            VERIFY_IS_NOT_NULL(timePicker->SelectedTime);
            VERIFY_ARE_EQUAL(targetTimeSpan.Duration, timePicker->SelectedTime->Value.Duration);
        });
    }

    void TimePickerIntegrationTests::ValidateSelectedTimePropagatesToTime()
    {
        TestCleanupWrapper cleanup;

        auto timePicker = SetupTimePickerTest();
        auto time = CreateTimeSpan(4, 30, 2);
        auto time2 = CreateTimeSpan(5, 45, 1);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Setting SelectedTime to null. Time should be the null sentinel value.");
            timePicker->SelectedTime = nullptr;

            VERIFY_ARE_EQUAL(-1, timePicker->Time.Duration);

            LOG_OUTPUT(L"Setting SelectedTime to 5:45 AM. Time should change to this value.");
            timePicker->SelectedTime = ref new Platform::Box<wf::TimeSpan>(time2);

            VERIFY_ARE_EQUAL(time2.Duration, timePicker->Time.Duration);
            VERIFY_IS_NOT_NULL(timePicker->SelectedTime);
            VERIFY_ARE_EQUAL(time2.Duration, timePicker->SelectedTime->Value.Duration);

            LOG_OUTPUT(L"Setting Time to February 4:30 PM. SelectedTime should change to this value.");
            timePicker->Time = time;

            VERIFY_ARE_EQUAL(time.Duration, timePicker->Time.Duration);
            VERIFY_IS_NOT_NULL(timePicker->SelectedTime);
            VERIFY_ARE_EQUAL(time.Duration, timePicker->SelectedTime->Value.Duration);

            LOG_OUTPUT(L"Setting Time to the null sentinel value. SelectedTime should revert to null.");
            wf::TimeSpan nullTime;
            nullTime.Duration = -1;
            timePicker->Time = nullTime;

            VERIFY_IS_NULL(timePicker->SelectedTime);
        });
    }

    void TimePickerIntegrationTests::CanProgrammaticallyClearSelectedTime()
    {
        TestCleanupWrapper cleanup;

        auto timePicker = SetupTimePickerTest();

        VerifyHasPlaceholder(timePicker);

        RunOnUIThread([&]()
        {
            timePicker->SelectedTime = ref new Platform::Box<wf::TimeSpan>(CreateTimeSpan(4, 30, 2));
        });

        VerifyDoesNotHavePlaceholder(timePicker);

        RunOnUIThread([&]()
        {
            timePicker->SelectedTime = nullptr;
        });

        VerifyHasPlaceholder(timePicker);
    }

    void TimePickerIntegrationTests::ValidateMinuteIncrementProperty()
    {
        TestCleanupWrapper cleanup;

        auto timePicker = SetupTimePickerTest();

        auto timeChangedEvent = std::make_shared<Event>();
        auto timeChangedRegistration = CreateSafeEventRegistration(xaml_controls::TimePicker, TimeChanged);
        timeChangedRegistration.Attach(timePicker, [&](){ timeChangedEvent->Set(); });

        auto initialTime = CreateTimeSpan(5, 12, 2); //5:12 PM
        auto initialTimeRounded = CreateTimeSpan(5, 10, 2); //5:10 PM

        auto timeToSet = CreateTimeSpan(7, 47, 1); //7:47 PM
        auto timeToSetRounded = CreateTimeSpan(7, 45, 1); //7:45 PM

        RunOnUIThread([&]()
        {
            timePicker->SelectedTime = ref new Platform::Box<wf::TimeSpan>(initialTime);
        });
        TestServices::WindowHelper->WaitForIdle();

        timeChangedEvent->Reset();
        RunOnUIThread([&]()
        {
            timePicker->MinuteIncrement = 5;
            VERIFY_ARE_EQUAL(initialTimeRounded.Duration, timePicker->Time.Duration, L"Setting TimePicker->MinuteIncrement should cause TimePicker->Time to get rounded");
        });

        // Setting MinuteIncrement should result in TimeChanged being raised:
        timeChangedEvent->WaitForDefault();

        // Setting Time should result in the value being rounded based on MinuteIncrement:
        RunOnUIThread([&]()
        {
            timePicker->SelectedTime = ref new Platform::Box<wf::TimeSpan>(timeToSet);
            VERIFY_ARE_EQUAL(timeToSetRounded.Duration, timePicker->Time.Duration, L"TimePicker->Time should get rounded based on TimePicker->MinuteIncrement");
        });

        DateTimePickerHelper::OpenDateTimePicker(timePicker);
        TestServices::WindowHelper->WaitForIdle();

        // The TimePickerFlyout should generate Minute items based on MinuteIncrement:
        RunOnUIThread([&]()
        {
            xaml_primitives::LoopingSelector^ hourLoopingSelector;
            xaml_primitives::LoopingSelector^ minuteLoopingSelector;
            xaml_primitives::LoopingSelector^ periodLoopingSelector;
            DateTimePickerHelper::GetHourMinutePeriodLoopingSelectorsFromOpenFlyout(hourLoopingSelector, minuteLoopingSelector, periodLoopingSelector);

            auto minuteItems = minuteLoopingSelector->Items;

            VERIFY_ARE_EQUAL(12u, minuteItems->Size);
            for (unsigned int i = 0; i < minuteItems->Size; i++)
            {
                auto dayItem = safe_cast<xaml_controls::DatePickerFlyoutItem^>(minuteItems->GetAt(i));
                VERIFY_IS_NOT_NULL(dayItem);

                // The item's PrimaryText string should be the minute value as a 2 digit number (with a leading 0 as necessary).
                unsigned int minuteValue = i * 5;
                auto expectedMinuteString = ref new Platform::String(WEX::Common::String().Format(L"%02u", minuteValue));
                VERIFY_ARE_EQUAL(expectedMinuteString, dayItem->PrimaryText);
            }
        });

        ControlHelper::ClickFlyoutCloseButton(timePicker, true /* isAccept */);
        TestServices::WindowHelper->WaitForIdle();
    }

    void TimePickerIntegrationTests::ValidateClockIdentifierProperty()
    {
        TestCleanupWrapper cleanup;

        auto timePicker = SetupTimePickerTest();

        auto initialTime = CreateTimeSpan(5, 15, 2); //5:15 PM

        RunOnUIThread([&]()
        {
            timePicker->SelectedTime = ref new Platform::Box<wf::TimeSpan>(initialTime);

        });
        TestServices::WindowHelper->WaitForIdle();

        xaml_controls::TextBlock^ hourTextBlock;
        xaml_controls::TextBlock^ minuteTextBlock;
        xaml_controls::TextBlock^ periodTextBlock;
        RunOnUIThread([&]()
        {
            hourTextBlock = safe_cast<xaml_controls::TextBlock^>(TreeHelper::GetVisualChildByName(timePicker, L"HourTextBlock"));
            minuteTextBlock = safe_cast<xaml_controls::TextBlock^>(TreeHelper::GetVisualChildByName(timePicker, L"MinuteTextBlock"));
            periodTextBlock = safe_cast<xaml_controls::TextBlock^>(TreeHelper::GetVisualChildByName(timePicker, L"PeriodTextBlock"));
            THROW_IF_NULL(hourTextBlock);
            THROW_IF_NULL(minuteTextBlock);
            THROW_IF_NULL(periodTextBlock);

            VERIFY_ARE_EQUAL(ref new Platform::String(L"5"), hourTextBlock->Text);
            VERIFY_ARE_EQUAL(ref new Platform::String(L"15"), minuteTextBlock->Text);
            VERIFY_ARE_EQUAL(ref new Platform::String(L"PM"), periodTextBlock->Text);

            timePicker->ClockIdentifier = wg::ClockIdentifiers::TwentyFourHour;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // periodTextBlock should get removed from the tree when using a 24Hour clock.
            VERIFY_IS_TRUE(periodTextBlock->Parent == nullptr);

            VERIFY_ARE_EQUAL(ref new Platform::String(L"17"), hourTextBlock->Text);
            VERIFY_ARE_EQUAL(ref new Platform::String(L"15"), minuteTextBlock->Text);
        });
    }

    xaml_controls::Button^ TimePickerIntegrationTests::GetFlyoutButtonFromTimePicker(xaml_controls::TimePicker^ timePicker)
    {
        auto templateRoot = dynamic_cast<xaml::FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(timePicker, 0));
        auto flyoutButton = dynamic_cast<xaml_controls::Button^>(templateRoot->FindName("FlyoutButton"));
        return flyoutButton;
    }

    wf::TimeSpan TimePickerIntegrationTests::CreateTimeSpan(int hours, int minutes, int seconds, int period)
    {
        WEX::Common::Throw::If(period != 1 && period != 2, E_FAIL, L"period must be 1 (AM) or 2 (PM)");
        wf::TimeSpan timeSpan = {};

        // Conver to 24 hours
        if (hours == 12)
        {
            if (period == 1)
            {
                hours = 0;
            }
            else
            {
                hours = 12;
            }
        }
        else if (period == 2)
        {
            hours += 12;
        }
        timeSpan.Duration = (int64)10000000 * ((hours * 60 + minutes) * 60 + seconds);

        LOG_OUTPUT(L"CreateTimeSpan period=%d hours=%d minutes=%d seconds=%d timeDuration=%llu", period, hours, minutes, seconds, timeSpan.Duration);

        return timeSpan;
    }

    void TimePickerIntegrationTests::VerifyHasPlaceholder(xaml_controls::TimePicker^ timePicker)
    {
        RunOnUIThread([&]()
        {
            auto hourTextBlock = safe_cast<xaml_controls::TextBlock^>(TreeHelper::GetVisualChildByName(timePicker, L"HourTextBlock"));
            auto minuteTextBlock = safe_cast<xaml_controls::TextBlock^>(TreeHelper::GetVisualChildByName(timePicker, L"MinuteTextBlock"));
            auto periodTextBlock = safe_cast<xaml_controls::TextBlock^>(TreeHelper::GetVisualChildByName(timePicker, L"PeriodTextBlock"));

            auto validatePlaceholder = [](xaml_controls::TextBlock^ textBlock, Platform::String^ placeholder)
            {
                LOG_OUTPUT(L"Expected placeholder: \"%s\"", placeholder->Data());
                LOG_OUTPUT(L"Actual text: \"%s\"", textBlock->Text->Data());

                VERIFY_IS_TRUE(Platform::String::CompareOrdinal(placeholder, textBlock->Text) == 0);
            };

            validatePlaceholder(hourTextBlock, L"hour");
            validatePlaceholder(minuteTextBlock, L"minute");
            validatePlaceholder(periodTextBlock, L"AM");
        });
    }

    void TimePickerIntegrationTests::VerifyDoesNotHavePlaceholder(xaml_controls::TimePicker^ timePicker)
    {
        RunOnUIThread([&]()
        {
            auto hourTextBlock = safe_cast<xaml_controls::TextBlock^>(TreeHelper::GetVisualChildByName(timePicker, L"HourTextBlock"));
            auto minuteTextBlock = safe_cast<xaml_controls::TextBlock^>(TreeHelper::GetVisualChildByName(timePicker, L"MinuteTextBlock"));
            auto periodTextBlock = safe_cast<xaml_controls::TextBlock^>(TreeHelper::GetVisualChildByName(timePicker, L"PeriodTextBlock"));

            auto validateValue = [](xaml_controls::TextBlock^ textBlock, Platform::String^ placeholder)
            {
                LOG_OUTPUT(L"Placeholder: \"%s\"", placeholder->Data());
                LOG_OUTPUT(L"Actual text: \"%s\"", textBlock->Text->Data());

                VERIFY_IS_TRUE(Platform::String::CompareOrdinal(placeholder, textBlock->Text) != 0);
            };

            validateValue(hourTextBlock, L"hour");
            validateValue(minuteTextBlock, L"minute");
            validateValue(periodTextBlock, L"AM");
        });
    }

    void TimePickerIntegrationTests::ValidateAutomationNameWhenTimeIsNull()
    {
        TestCleanupWrapper cleanup;

        auto timePicker = SetupTimePickerTest();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Setting SelectedTime to null.");
            timePicker->SelectedTime = nullptr;

            auto automationPeer = Microsoft::UI::Xaml::Automation::Peers::FrameworkElementAutomationPeer::CreatePeerForElement(timePicker);
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal("TimePickerTest", automationPeer->GetName()) == 0);     
        });
    }

    void TimePickerIntegrationTests::SetTime(xaml_controls::TimePicker^ timePicker, wf::TimeSpan time)
    {
#if WI_IS_FEATURE_PRESENT(Feature_DateTimePickerNullVisualization)
        timePicker->SelectedTime = ref new Platform::Box<wf::TimeSpan>(time);
#else
        timePicker->Time = time;
#endif
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::TimePicker
