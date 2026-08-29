// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "CalendarDatePickerIntegrationTests.h"

#include <XamlTailored.h>
#include <Utils.h>
#include <TestEvent.h>
#include <collection.h>
#include <TestCleanupWrapper.h>
#include <ControlHelper.h>
#include <FlyoutHelper.h>
#include <SafeEventRegistration.h>
#include <RuntimeEnabledFeaturesEnum.h>
#include "CalendarDatePickerHelper.h"
#include <WUCRenderingScopeGuard.h>
#include "FeatureFlags.h"

using namespace std;
using namespace std::placeholders;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace Microsoft::UI::Xaml::Tests::Common::CalendarHelper;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace CalendarDatePicker {


    bool CalendarDatePickerIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }


    bool CalendarDatePickerIntegrationTests::TestCleanup()
    {
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //

    void CalendarDatePickerIntegrationTests::ValidateDefaultPropertyValues()
    {
        RunOnUIThread([&]()
        {
            auto cp = ref new Microsoft::UI::Xaml::Controls::CalendarDatePicker();
            VERIFY_ARE_EQUAL(cp->FirstDayOfWeek, ::Windows::Globalization::DayOfWeek::Sunday);
            VERIFY_ARE_EQUAL(cp->DisplayMode, Microsoft::UI::Xaml::Controls::CalendarViewDisplayMode::Month);
            VERIFY_ARE_EQUAL(cp->IsTodayHighlighted, true);
            VERIFY_ARE_EQUAL(cp->IsOutOfScopeEnabled, true);
            VERIFY_ARE_EQUAL(cp->IsGroupLabelVisible, false);

            ::Windows::Globalization::Calendar^ calendar = ref new ::Windows::Globalization::Calendar();
            calendar->SetToNow();

            calendar->AddYears(-100);

            calendar->Month = calendar->FirstMonthInThisYear;
            calendar->Day = calendar->FirstDayInThisMonth;

            auto minDate = calendar->GetDateTime();

            calendar->AddYears(200);

            calendar->Month = calendar->LastMonthInThisYear;
            calendar->Day = calendar->LastDayInThisMonth;

            auto maxDate = calendar->GetDateTime();

            CompareDate comparer;
            VERIFY_IS_TRUE(comparer(cp->MinDate, minDate));
            VERIFY_IS_TRUE(comparer(cp->MaxDate, maxDate));
        });
    }

    void CalendarDatePickerIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        // Note: CalendarDatePicker can't use below commented line to test "CanEnterAndLeaveLiveTree"
        // the problem is in below helper, we did these:
        //  1. create CalendarDatePicker
        //  2. added into visual tree
        //  3. test loaded and unloaded event
        //  4. remove CalendarDatePicker from visual tree
        //  5. destroy CalendarDatePicker
        //  .....

        // because we destroy CalendarDatePicker after we remove it from visual tree, so if there are any left work in build tree services, they can't be cleaned up correctly.
        // this should happens on ListView and GridView, however for default ListView and GridView (especially in below helper method) are empty and there is no buildtree work.
        // But for default CalendarDatePicker, we have! because default CalendarDatePicker will show the dates in 3 years.

        //Generic::FrameworkElementTests<Microsoft::UI::Xaml::Controls::CalendarDatePicker>::CanEnterAndLeaveLiveTree();

        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;

        CalendarDatePickerHelper helper;
        helper.PrepareLoadedEvent();
        Microsoft::UI::Xaml::Controls::CalendarDatePicker^ cp = helper.GetCalendarDatePicker();

        CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cp);
        });

        helper.WaitForLoaded();

        // remove from visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Clear();
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void CalendarDatePickerIntegrationTests::CanOpenFlyoutByTapping()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::Grid^ root = nullptr;
        xaml_controls::TextBlock^ dateText = nullptr;
        xaml_controls::Primitives::FlyoutBase^ flyout = nullptr;
        CalendarDatePickerHelper helper;
        helper.PrepareLoadedEvent();
        Microsoft::UI::Xaml::Controls::CalendarDatePicker^ cp = helper.GetCalendarDatePicker();

        CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cp);
        });

        helper.WaitForLoaded();

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            root = safe_cast<xaml_controls::Grid^>(helper.GetTemplateChild(L"Root"));
            dateText = safe_cast<xaml_controls::TextBlock^>(helper.GetTemplateChild(L"DateText"));

            VERIFY_IS_NOT_NULL(root);
            VERIFY_IS_NOT_NULL(dateText);

            flyout = xaml_controls::Primitives::FlyoutBase::GetAttachedFlyout(root);
            VERIFY_IS_NOT_NULL(flyout);
        });

        helper.PrepareOpenedEvent();

        TestServices::InputHelper->Tap(dateText);

        helper.WaitForOpened();

        TestServices::WindowHelper->WaitForIdle();

        helper.PrepareClosedEvent();

        RunOnUIThread([&]()
        {
            // close the flyout before exiting.
            flyout->Hide();
        });
        helper.WaitForClosed();

        TestServices::WindowHelper->WaitForIdle();
    }


    void CalendarDatePickerIntegrationTests::CanOpenFlyoutByKeyboard()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;

        CalendarDatePickerHelper helper;
        helper.PrepareLoadedEvent();
        Microsoft::UI::Xaml::Controls::CalendarDatePicker^ cp = helper.GetCalendarDatePicker();

        CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cp);
        });

        helper.WaitForLoaded();

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            cp->Focus(Microsoft::UI::Xaml::FocusState::Programmatic);
        });

        helper.PrepareOpenedEvent();
        TestServices::WindowHelper->WaitForIdle();

        // press enter to open flyout
        TestServices::KeyboardHelper->Enter();

        helper.WaitForOpened();

        // escape to close the flyout
        TestServices::KeyboardHelper->Escape();

        TestServices::WindowHelper->WaitForIdle();
        helper.PrepareOpenedEvent();

        RunOnUIThread([&]()
        {
            cp->Focus(Microsoft::UI::Xaml::FocusState::Programmatic);
        });
        TestServices::WindowHelper->WaitForIdle();

        // press space to open flyout
        TestServices::KeyboardHelper->PressKeySequence("$d$_ #$u$_ ");

        helper.WaitForOpened();

        // escape to close the flyout
        TestServices::KeyboardHelper->Escape();
        TestServices::WindowHelper->WaitForIdle();
    }


    void CalendarDatePickerIntegrationTests::CanOpenCloseFlyoutBySettingIsCalendarOpen()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;
        CalendarDatePickerHelper helper;
        helper.PrepareLoadedEvent();
        Microsoft::UI::Xaml::Controls::CalendarDatePicker^ cp = helper.GetCalendarDatePicker();

        CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cp);
        });

        helper.WaitForLoaded();

        TestServices::WindowHelper->WaitForIdle();

        helper.PrepareOpenedEvent();

        RunOnUIThread([&]()
        {
            cp->IsCalendarOpen = true;
        });
        helper.WaitForOpened();

        helper.PrepareClosedEvent();

        RunOnUIThread([&]()
        {
            cp->IsCalendarOpen = false;
        });

        helper.WaitForClosed();

        RunOnUIThread([&]()
        {
            // disable CP to make sure input pane is not open during clean up.
            cp->IsEnabled = false;
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void CalendarDatePickerIntegrationTests::CanCloseFlyoutBySelectingADate()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::TextBlock^ dateText = nullptr;
        xaml_controls::Grid^ root = nullptr;
        xaml_controls::Primitives::FlyoutBase^ flyout = nullptr;
        xaml_controls::CalendarView^ calendarView = nullptr;
        CalendarDatePickerHelper helper;
        helper.PrepareLoadedEvent();
        Microsoft::UI::Xaml::Controls::CalendarDatePicker^ cp = helper.GetCalendarDatePicker();

        CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            cp->IsOutOfScopeEnabled = false;
            rootPanel->Children->Append(cp);
        });

        helper.WaitForLoaded();

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            dateText = safe_cast<xaml_controls::TextBlock^>(helper.GetTemplateChild(L"DateText"));
            VERIFY_IS_NOT_NULL(dateText);

            root = safe_cast<xaml_controls::Grid^>(helper.GetTemplateChild(L"Root"));
            VERIFY_IS_NOT_NULL(root);

            flyout = xaml_controls::Primitives::FlyoutBase::GetAttachedFlyout(root);
            VERIFY_IS_NOT_NULL(flyout);

            auto content = safe_cast<xaml_controls::Flyout^>(flyout)->Content;
            calendarView = safe_cast<xaml_controls::CalendarView^>(content);
            VERIFY_IS_NOT_NULL(calendarView);


            calendarView->MinDate = ConvertToDateTime(1, 2000, 1, 1);
            calendarView->MaxDate = ConvertToDateTime(1, 2001, 1, 1);
            calendarView->UpdateLayout();
        });


        TestServices::InputHelper->Tap(dateText);

        TestServices::WindowHelper->WaitForIdle();

        helper.PrepareClosedEvent();

        RunOnUIThread([&]()
        {
            calendarView->SelectedDates->Append(ConvertToDateTime(1, 2000, 10, 21));
        });

        helper.WaitForClosed();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"actual text: %s.", dateText->Text->Data());
            
            // U+200E is the left-to-right mark Unicode character.
            VERIFY_IS_TRUE(dateText->Text == L"\u200E10\u200E/\u200E21\u200E/\u200E2000");
        });

        RunOnUIThread([&]()
        {
            // disable CP to make sure input pane is not open during clean up.
            cp->IsEnabled = false;
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void CalendarDatePickerIntegrationTests::ValidateDateIsCoerced()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;
        CalendarDatePickerHelper helper;
        helper.PrepareLoadedEvent();
        Microsoft::UI::Xaml::Controls::CalendarDatePicker^ cp = helper.GetCalendarDatePicker();

        CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cp);
            cp->MinDate = ConvertToDateTime(1, 2000, 1, 1);
            cp->MaxDate = ConvertToDateTime(1, 2002, 1, 1);
            cp->Date = ConvertToDateTime(1, 2001, 1, 1);
        });

        helper.WaitForLoaded();

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_NOT_NULL(cp->Date);
            VERIFY_ARE_EQUAL(cp->Date->Value.UniversalTime, ConvertToDateTime(1, 2001, 1, 1).UniversalTime);

            // make date beyond the range.
            // it should be coerced to min/max
            cp->Date = ConvertToDateTime(1, 2010, 1, 1);
            cp->UpdateLayout();
            VERIFY_IS_NOT_NULL(cp->Date);
            VERIFY_ARE_EQUAL(cp->Date->Value.UniversalTime, cp->MaxDate.UniversalTime);

            cp->Date = ConvertToDateTime(1, 1999, 1, 1);
            cp->UpdateLayout();
            VERIFY_IS_NOT_NULL(cp->Date);
            VERIFY_ARE_EQUAL(cp->Date->Value.UniversalTime, cp->MinDate.UniversalTime);
        });
    }

    void CalendarDatePickerIntegrationTests::CanFormatDate()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::TextBlock^ dateText = nullptr;

        CalendarDatePickerHelper helper;
        helper.PrepareLoadedEvent();
        Microsoft::UI::Xaml::Controls::CalendarDatePicker^ cp = helper.GetCalendarDatePicker();

        CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cp);
            cp->MinDate = ConvertToDateTime(1, 2000, 1, 1);
            cp->MaxDate = ConvertToDateTime(1, 2002, 1, 1);
            cp->Date = ConvertToDateTime(1, 2001, 1, 1);
        });

        helper.WaitForLoaded();

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            dateText = safe_cast<xaml_controls::TextBlock^>(helper.GetTemplateChild(L"DateText"));
            VERIFY_IS_NOT_NULL(dateText);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            cp->DateFormat = "{dayofweek.full}, {month.full} {day.integer}, {year.full}";   // equivalent to "longdate"
            cp->UpdateLayout();

            LOG_OUTPUT(L"actual text: %s.", dateText->Text->Data());
            VERIFY_IS_TRUE(dateText->Text == L"Monday, January 1, 2001");
        });
    }

    void CalendarDatePickerIntegrationTests::SettingCalendarIdentifierChangesDateFormat()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::TextBlock^ dateText = nullptr;

        CalendarDatePickerHelper helper;
        helper.PrepareLoadedEvent();
        Microsoft::UI::Xaml::Controls::CalendarDatePicker^ cp = helper.GetCalendarDatePicker();

        CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cp);
            cp->MinDate = ConvertToDateTime(1, 2000, 1, 1);
            cp->MaxDate = ConvertToDateTime(1, 2002, 1, 1);
            cp->Date = ConvertToDateTime(1, 2001, 1, 1);
        });

        helper.WaitForLoaded();

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            dateText = safe_cast<xaml_controls::TextBlock^>(helper.GetTemplateChild(L"DateText"));
            VERIFY_IS_NOT_NULL(dateText);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            cp->DateFormat = "{dayofweek.full}, {month.full} {day.integer}, {year.full}";   // equivalent to "longdate"
            cp->UpdateLayout();

            cp->CalendarIdentifier = ::Windows::Globalization::CalendarIdentifiers::Taiwan;

            LOG_OUTPUT(L"actual text: %s.", dateText->Text->Data());
            VERIFY_IS_TRUE(dateText->Text == L"Monday, January 1, 90");
        });

    }

    void CalendarDatePickerIntegrationTests::PressingDoesNotOpenMenuFlyout()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::Grid^ root = nullptr;
        xaml_controls::TextBlock^ dateText = nullptr;
        xaml_controls::Primitives::FlyoutBase^ flyout = nullptr;

        CalendarDatePickerHelper helper;
        helper.PrepareLoadedEvent();
        Microsoft::UI::Xaml::Controls::CalendarDatePicker^ cp = helper.GetCalendarDatePicker();

        auto gridPointerPressedEvent = std::make_shared<Event>();
        auto gridPointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);

        CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cp);
            cp->MinDate = ConvertToDateTime(1, 2000, 1, 1);
            cp->MaxDate = ConvertToDateTime(1, 2002, 1, 1);
            cp->Date = ConvertToDateTime(1, 2001, 1, 1);

            gridPointerPressedRegistration.Attach(rootPanel, ref new Microsoft::UI::Xaml::Input::PointerEventHandler(
                [gridPointerPressedEvent]
            (Platform::Object^ sender, Microsoft::UI::Xaml::Input::PointerRoutedEventArgs^ args)
            {
                gridPointerPressedEvent->Set();
            }));
        });

        helper.WaitForLoaded();

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            root = safe_cast<xaml_controls::Grid^>(helper.GetTemplateChild(L"Root"));
            dateText = safe_cast<xaml_controls::TextBlock^>(helper.GetTemplateChild(L"DateText"));

            VERIFY_IS_NOT_NULL(root);
            VERIFY_IS_NOT_NULL(dateText);

            flyout = xaml_controls::Primitives::FlyoutBase::GetAttachedFlyout(root);
            VERIFY_IS_NOT_NULL(flyout);
        });
        TestServices::WindowHelper->WaitForIdle();

        helper.PrepareOpenedEvent();

        TestServices::InputHelper->Tap(dateText);

        helper.WaitForOpened();

        TestServices::WindowHelper->WaitForIdle();
        helper.PrepareClosedEvent();

        RunOnUIThread([&]()
        {
            // close the flyout before exiting.
            flyout->Hide();
        });
        helper.WaitForClosed();

        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_FALSE(gridPointerPressedEvent->HasFired());
    }

    void CalendarDatePickerIntegrationTests::ValidateUIElementTree()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::StackPanel^ rootPanel = nullptr;

        CalendarDatePickerHelper helper;

        helper.PrepareLoadedEvent();
        Microsoft::UI::Xaml::Controls::CalendarDatePicker^ cdp = helper.GetCalendarDatePicker();

        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      Width='400' Height='400' VerticalAlignment='Top' HorizontalAlignment='Left' Background='Black'/> "));

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });

        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cdp);
        });

        helper.WaitForLoaded();

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            cdp->Focus(Microsoft::UI::Xaml::FocusState::Pointer);
        });

        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyUIElementTree();
    }

    void CalendarDatePickerIntegrationTests::ValidateVisualStates()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        xaml_controls::StackPanel^ rootPanel = nullptr;

        CalendarDatePickerHelper helper;
        helper.PrepareLoadedEvent();
        Microsoft::UI::Xaml::Controls::CalendarDatePicker^ cpNormal = helper.GetCalendarDatePicker();
#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
        Microsoft::UI::Xaml::Controls::CalendarDatePicker^ cpLeftHeader = nullptr;
#endif
        Microsoft::UI::Xaml::Controls::CalendarDatePicker^ cpPressed = nullptr;
        Microsoft::UI::Xaml::Controls::CalendarDatePicker^ cpPointerOver = nullptr;
        Microsoft::UI::Xaml::Controls::CalendarDatePicker^ cpDisabled = nullptr;
        Microsoft::UI::Xaml::Controls::CalendarDatePicker^ cpFocused = nullptr;
        Microsoft::UI::Xaml::Controls::CalendarDatePicker^ cpSelected = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      Width='400' Height='400' VerticalAlignment='Top' HorizontalAlignment='Left' Background='Black'/> "));

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });



        // load into visual tree
        RunOnUIThread([&]()
        {
#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
            cpLeftHeader = ref new xaml_controls::CalendarDatePicker();
#endif
            cpPressed = ref new xaml_controls::CalendarDatePicker();
            cpPointerOver = ref new xaml_controls::CalendarDatePicker();
            cpDisabled = ref new xaml_controls::CalendarDatePicker();
            cpFocused = ref new xaml_controls::CalendarDatePicker();
            cpSelected = ref new xaml_controls::CalendarDatePicker();

            rootPanel->Children->Append(cpNormal);
#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
            rootPanel->Children->Append(cpLeftHeader);
#endif
            rootPanel->Children->Append(cpPressed);
            rootPanel->Children->Append(cpPointerOver);
            rootPanel->Children->Append(cpDisabled);
            rootPanel->Children->Append(cpFocused);
            rootPanel->Children->Append(cpSelected);

            cpNormal->Header = L"Normal";
#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
            cpLeftHeader->Header = L"Left Header";
            cpLeftHeader->HeaderPlacement = xaml_controls::ControlHeaderPlacement::Left;
#endif
            cpPressed->Header = L"Pressed";
            cpPointerOver->Header = L"PointerOver";
            cpDisabled->Header = L"Disabled";
            cpFocused->Header = L"Focused";
            cpSelected->Header = L"Selected";
        });

        helper.WaitForLoaded();

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            //cpNormal stays in common state
            VisualStateManager::GoToState(cpPressed, "Pressed", true);
            VisualStateManager::GoToState(cpPointerOver, "PointerOver", true);
            VisualStateManager::GoToState(cpDisabled, "Disabled", true);
            VisualStateManager::GoToState(cpFocused, "Focused", true);
            VisualStateManager::GoToState(cpSelected, "Selected", true);
        });

        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }

    void CalendarDatePickerIntegrationTests::DonotResizeCalendarView()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        xaml_controls::Grid^ rootPanel = nullptr;

        CalendarDatePickerHelper helper;
        Microsoft::UI::Xaml::Controls::CalendarDatePicker^ cp = helper.GetCalendarDatePicker();

        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      Width='200' Height='200' VerticalAlignment='Top' HorizontalAlignment='Left' Background='Black'/> "));
            rootPanel->Children->Append(cp);
            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
            cp->IsCalendarOpen = true;
            // there is not enough space to show the flyout, before this fix, the flyoutpresenter's content will be clipped
            cp->HorizontalAlignment = xaml::HorizontalAlignment::Center;
            cp->VerticalAlignment = xaml::VerticalAlignment::Center;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(rootPanel->XamlRoot);
            VERIFY_ARE_EQUAL(popups->Size, 1u);
            auto popup = popups->GetAt(0);
            auto presenter = safe_cast<xaml::FrameworkElement^>(popup->Child);
            LOG_OUTPUT(L"actual height: %lf. expected height: 332.", presenter->ActualHeight);
            // was 284 before this fix, the calendarview is clipped.
            VERIFY_ARE_EQUAL(presenter->ActualHeight, 352);

            cp->IsCalendarOpen = false;
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void CalendarDatePickerIntegrationTests::CanPresetDate()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        xaml_controls::Grid^ rootPanel = nullptr;

        CalendarDatePickerHelper helper;
        Microsoft::UI::Xaml::Controls::CalendarDatePicker^ cp = helper.GetCalendarDatePicker();
        xaml_controls::CalendarView^ calendarView = nullptr;
        xaml_controls::TextBlock^ dateText = nullptr;

        CreateTestResources(rootPanel);
        auto date1 = ConvertToDateTime(1, 2000, 10, 21);
        auto date2 = ConvertToDateTime(1, 2003, 1, 1);
        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cp);
            cp->Date = date1;
            cp->IsCalendarOpen = true;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto root = safe_cast<xaml_controls::Grid^>(helper.GetTemplateChild(L"Root"));
            VERIFY_IS_NOT_NULL(root);

            auto flyout = xaml_controls::Primitives::FlyoutBase::GetAttachedFlyout(root);
            VERIFY_IS_NOT_NULL(flyout);

            auto content = safe_cast<xaml_controls::Flyout^>(flyout)->Content;
            calendarView = safe_cast<xaml_controls::CalendarView^>(content);
            VERIFY_IS_NOT_NULL(calendarView);

            dateText = safe_cast<xaml_controls::TextBlock^>(helper.GetTemplateChild(L"DateText"));

            VERIFY_IS_NOT_NULL(dateText);
        });

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"actual text: %s.", dateText->Text->Data());
            
            // U+200E is the left-to-right mark Unicode character.
            VERIFY_IS_TRUE(dateText->Text == L"\u200E10\u200E/\u200E21\u200E/\u200E2000");

            VERIFY_ARE_EQUAL(calendarView->SelectedDates->Size, 1u);
            VERIFY_ARE_EQUAL(calendarView->SelectedDates->GetAt(0).UniversalTime, date1.UniversalTime);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            cp->Date = nullptr;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"actual text: %s.", dateText->Text->Data());
            // clear the Date property will display placehoder text.
            VERIFY_IS_TRUE(dateText->Text == cp->PlaceholderText);

            VERIFY_ARE_EQUAL(calendarView->SelectedDates->Size, 0u);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            cp->Date = date2;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"actual text: %s.", dateText->Text->Data());
            
            // U+200E is the left-to-right mark Unicode character.
            VERIFY_IS_TRUE(dateText->Text == L"\u200E1\u200E/\u200E1\u200E/\u200E2003");

            VERIFY_ARE_EQUAL(calendarView->SelectedDates->Size, 1u);
            VERIFY_ARE_EQUAL(calendarView->SelectedDates->GetAt(0).UniversalTime, date2.UniversalTime);

            cp->IsCalendarOpen = false;
        });


        TestServices::WindowHelper->WaitForIdle();
    }

    void CalendarDatePickerIntegrationTests::VerifyTwoWayBinding()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::StackPanel^ rootPanel = nullptr;
        Microsoft::UI::Xaml::Controls::CalendarDatePicker^ cdp1 = nullptr;
        Microsoft::UI::Xaml::Controls::CalendarDatePicker^ cdp2 = nullptr;
        auto date1 = ConvertToDateTime(1, 2000, 1, 1);
        auto date2 = ConvertToDateTime(1, 2000, 1, 2);

        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      Width='400' Height='400' VerticalAlignment='Top' HorizontalAlignment='Left' Background='Black'> "
                L"    <CalendarDatePicker x:Name='cdp1'/>"
                L"    <CalendarDatePicker x:Name='cdp2' Date='{Binding ElementName=cdp1, Path=Date, Mode=TwoWay}'/>"
                L"</StackPanel>"
                ));

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        // load into visual tree
        RunOnUIThread([&]()
        {
            cdp1 = safe_cast<xaml_controls::CalendarDatePicker^>(rootPanel->Children->GetAt(0));
            cdp2 = safe_cast<xaml_controls::CalendarDatePicker^>(rootPanel->Children->GetAt(1));
            cdp1->Date = date1;
            // due to a known issue:{Binding} doesn't work on nullable properties when source is null
            // we can't test the scenario that cdp1->Date is null
            CalendarHelper::DumpDate(cdp1->Date->Value, L"Changing cdp1.Date to");
            CalendarHelper::DumpDate(cdp2->Date->Value, L"Now cdp2.Date is");
            VERIFY_ARE_EQUAL(cdp1->Date->Value.UniversalTime, cdp2->Date->Value.UniversalTime);

            cdp2->Date = date2;
            CalendarHelper::DumpDate(cdp2->Date->Value, L"Changing cdp2.Date to");
            CalendarHelper::DumpDate(cdp1->Date->Value, L"Now cdp1.Date is");
            VERIFY_ARE_EQUAL(cdp1->Date->Value.UniversalTime, cdp2->Date->Value.UniversalTime);
        });

    }

    void CalendarDatePickerIntegrationTests::TestDateChangedEventWhenAssignDateToSameValue()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Grid^ rootPanel = nullptr;
        Microsoft::UI::Xaml::Controls::CalendarDatePicker^ cp = nullptr;

        CreateTestResources(rootPanel);
        auto date = ConvertToDateTime(1, 2000, 1, 1);

        // load into visual tree
        RunOnUIThread([&]()
        {
            cp = ref new Microsoft::UI::Xaml::Controls::CalendarDatePicker();
            cp->Date = date;

            rootPanel->Children->Append(cp);
        });

        TestServices::WindowHelper->WaitForIdle();

        auto dateChangedEvent = std::make_shared<Event>();
        auto dateChangedRegistration = CreateSafeEventRegistration(xaml_controls::CalendarDatePicker, DateChanged);

        dateChangedRegistration.Attach(cp, ref new wf::TypedEventHandler<Microsoft::UI::Xaml::Controls::CalendarDatePicker^, xaml_controls::CalendarDatePickerDateChangedEventArgs^>(
            [dateChangedEvent](xaml_controls::CalendarDatePicker^ sender, xaml_controls::CalendarDatePickerDateChangedEventArgs^ args)
            {
                dateChangedEvent->Set();
            }));

        RunOnUIThread([&]()
        {
            cp->Date = date;
        });

        TestServices::WindowHelper->WaitForIdle();

        // should not raise DateChanged event because we set the same date to cp->Date.
        VERIFY_IS_FALSE(dateChangedEvent->HasFired());
    }

    void CalendarDatePickerIntegrationTests::ValidateOverlayBrush()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::CalendarDatePicker^ calendarDatePicker = nullptr;

        RunOnUIThread([&]()
        {
            calendarDatePicker = ref new xaml_controls::CalendarDatePicker();
            calendarDatePicker->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::On;

            TestServices::WindowHelper->WindowContent = calendarDatePicker;
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->Tap(calendarDatePicker);
        TestServices::WindowHelper->WaitForIdle();

        xaml_primitives::FlyoutBase^ flyout = nullptr;
        RunOnUIThread([&]()
        {
            auto root = safe_cast<xaml::FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(calendarDatePicker, 0));
            flyout = xaml_primitives::FlyoutBase::GetAttachedFlyout(root);
            THROW_IF_NULL_WITH_MSG(flyout, L"An overlay element should exist for the flyout.");
        });

        FlyoutHelper::ValidateOpenFlyoutOverlayBrush(L"CalendarDatePickerLightDismissOverlayBackground");

        FlyoutHelper::HideFlyout(flyout);
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Enterprise::CalendarDatePicker






