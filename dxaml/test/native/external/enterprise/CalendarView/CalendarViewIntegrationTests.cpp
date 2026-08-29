// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "CalendarViewIntegrationTests.h"

#include <XamlTailored.h>
#include <Utils.h>
#include <TestEvent.h>
#include <collection.h>
#include <TestCleanupWrapper.h>
#include <ControlHelper.h>
#include <Timezoneapi.h>
#include <WUCRenderingScopeGuard.h>
#include "FlyoutHelper.h"
#include <CustomBrushes.h>
#include <ControlHelper.h>
#include <FileLoader.h>

using namespace std;
using namespace std::placeholders;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Common::CalendarHelper;
using namespace test_infra;
using namespace Microsoft::UI::Xaml::Tests::Foundation::Graphics;
using namespace Microsoft::UI::Xaml::Media;

namespace
{
    std::wstring GetCurrentTimeZoneId()
    {
        auto info = DYNAMIC_TIME_ZONE_INFORMATION{ 0 };
        auto const mode = GetDynamicTimeZoneInformation(&info);
        FAIL_FAST_IF(mode == TIME_ZONE_ID_INVALID);
        return std::wstring(info.TimeZoneKeyName);
    }

    bool ReadDwordAsBoolFromRegKey(HKEY regKey, const wchar_t* valueName)
    {
        DWORD type = REG_DWORD;
        unsigned value = 0;
        DWORD size = sizeof(value);
        LSTATUS result = RegQueryValueEx(regKey, valueName, nullptr, &type, (BYTE*)&value, &size);
        FAIL_FAST_IF(!(result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND));
        if (result == ERROR_FILE_NOT_FOUND)
        {
            return false;
        }
        else
        {
            return (value != 0);
        }
    }
}

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace CalendarView {
    class RoundedChromeDictionaryHelper
    {
    private:
        xaml::ResourceDictionary^ m_customDictionary = nullptr;

    public:
        RoundedChromeDictionaryHelper(bool useRoundedCorners) {

            // If we aren't using rounded chrome, we don't have to do anything.  This will just be a no-op.
            if (!useRoundedCorners) return;

            // Set up a dictionary and add it to the application.
            RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Creating custom resource dictionary for rounded chrome.");
                    m_customDictionary = safe_cast<xaml::ResourceDictionary^>(xaml_markup::XamlReader::Load(
                        LR"(
                        <ResourceDictionary
                            xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <x:Boolean x:Key="CalendarViewBaseItemRoundedChromeEnabled">True</x:Boolean>
                        </ResourceDictionary>)"));

                    Application::Current->Resources->MergedDictionaries->Append(m_customDictionary);
                });
        }

        ~RoundedChromeDictionaryHelper()
        {
            // If we didn't actually set the dictionary, no-op
            if (!m_customDictionary) return;

            RunOnUIThread([&]()
                {
                    auto mergedDictionaries = Application::Current->Resources->MergedDictionaries;

                    // Find the custom dictionary
                    uint32 index;
                    VERIFY_IS_TRUE(mergedDictionaries->IndexOf(m_customDictionary, &index));

                    // Then remove it and make sure we tell Xaml to reset any cached dictionaries they might have.
                    LOG_OUTPUT(L"Removing dictionary with CalendarViewBaseItemRoundedChromeEnabled=True.");
                    mergedDictionaries->RemoveAt(index);

                    // ListViewBaseItem will cache the value, so make sure to clear that cache
                    LOG_OUTPUT(L"Invoking Utilities.DeleteResourceDictionaryCaches.");
                    TestServices::Utilities->DeleteResourceDictionaryCaches();
                });
        }
    };

    bool CalendarViewIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool CalendarViewIntegrationTests::TestCleanup()
    {
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }


    //
    // Test Cases
    //

    void CalendarViewIntegrationTests::VerifyMultipleEraCalendar()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            /*
            JapaneseCalendar           Era 1: 01/01/1868 ~ 29/07/1912
            JapaneseCalendar           Era 2: 30/07/1912 ~ 24/12/1926
            JapaneseCalendar           Era 3: 25/12/1926 ~ 07/01/1989
            JapaneseCalendar           Era 4: 08/01/1989 ~ 31/12/9999
            */

            // below date range will cross Era 3 and Era 4.
            cv->CalendarIdentifier = L"JapaneseCalendar";
            cv->MinDate = ConvertToDateTime(1, 1988, 1, 1);
            cv->MaxDate = ConvertToDateTime(1, 2000, 1, 1);
            cv->DisplayMode = xaml_controls::CalendarViewDisplayMode::Decade;
            rootPanel->Children->Append(cv);
        });

        // no crash!
        TestServices::WindowHelper->WaitForIdle();
    }

    void CalendarViewIntegrationTests::ValidateDefaultPropertyValues()
    {
        RunOnUIThread([&]()
        {
            auto cv = ref new xaml_controls::CalendarView();
            VERIFY_ARE_EQUAL(cv->NumberOfWeeksInView, 6);
            VERIFY_ARE_EQUAL(cv->FirstDayOfWeek, ::Windows::Globalization::DayOfWeek::Sunday);
            VERIFY_ARE_EQUAL(cv->SelectionMode, xaml_controls::CalendarViewSelectionMode::Single);

            VERIFY_ARE_EQUAL(cv->DisplayMode, xaml_controls::CalendarViewDisplayMode::Month);
            VERIFY_ARE_EQUAL(cv->IsTodayHighlighted, true);
            VERIFY_ARE_EQUAL(cv->IsOutOfScopeEnabled, true);
            VERIFY_ARE_EQUAL(cv->IsGroupLabelVisible, false);
            VERIFY_ARE_EQUAL(cv->DayItemFontStyle, ::Windows::UI::Text::FontStyle::Normal);
            VERIFY_ARE_EQUAL(cv->DayItemFontWeight.Weight, Microsoft::UI::Text::FontWeights::Normal.Weight);
            VERIFY_ARE_EQUAL(cv->TodayFontWeight.Weight, Microsoft::UI::Text::FontWeights::SemiBold.Weight);
            VERIFY_ARE_EQUAL(cv->FirstOfMonthLabelFontSize, 12.0);
            VERIFY_ARE_EQUAL(cv->FirstOfMonthLabelFontStyle, ::Windows::UI::Text::FontStyle::Normal);
            VERIFY_ARE_EQUAL(cv->FirstOfMonthLabelFontWeight.Weight, Microsoft::UI::Text::FontWeights::Normal.Weight);
            VERIFY_ARE_EQUAL(cv->MonthYearItemFontSize, 20.0);
            VERIFY_ARE_EQUAL(cv->MonthYearItemFontStyle, ::Windows::UI::Text::FontStyle::Normal);
            VERIFY_ARE_EQUAL(cv->MonthYearItemFontWeight.Weight, Microsoft::UI::Text::FontWeights::Normal.Weight);
            VERIFY_ARE_EQUAL(cv->FirstOfYearDecadeLabelFontSize, 12.0);
            VERIFY_ARE_EQUAL(cv->FirstOfYearDecadeLabelFontStyle, ::Windows::UI::Text::FontStyle::Normal);
            VERIFY_ARE_EQUAL(cv->FirstOfYearDecadeLabelFontWeight.Weight, Microsoft::UI::Text::FontWeights::Normal.Weight);

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
            VERIFY_IS_TRUE(comparer(cv->MinDate, minDate));
            VERIFY_IS_TRUE(comparer(cv->MaxDate, maxDate));
        });
    }


    void CalendarViewIntegrationTests::TestSelection()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;

        ::Windows::Foundation::DateTime date = ConvertToDateTime(1, 2000, 1, 1, 1, 8, 30); // 1/1/2000 8:30 AM

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        CalendarHelper::CreateTestResources(rootPanel);

        // set mindate and maxdate
        RunOnUIThread([&]()
        {
            cv->MinDate = ConvertToDateTime(1, 1999, 1, 1);
            cv->MaxDate = ConvertToDateTime(1, 2020, 12, 31);
        });

        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        // select a single valid date
        {

            helper.PrepareSelectedDatesChangedEvent();
            helper.ExpectAddedDate(date);

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"CalendarViewIntegrationTests: select a date.");
                cv->SelectedDates->Append(date);
            });

            helper.WaitForSelectedDatesChanged();
        }

        // remove the date
        {
            helper.PrepareSelectedDatesChangedEvent();
            helper.ExpectRemovedDate(date);

            RunOnUIThread([&]()
            {
                cv->SelectedDates->RemoveAt(0);
            });

            helper.WaitForSelectedDatesChanged();

        }

        // multiple selection
        {
            TestServices::WindowHelper->WaitForIdle();

            ::Windows::Foundation::DateTime date1 = ConvertToDateTime(1, 2000, 1, 1);
            ::Windows::Foundation::DateTime date2 = ConvertToDateTime(1, 2000, 1, 2);
            ::Windows::Foundation::DateTime date3 = ConvertToDateTime(1, 2000, 1, 3);


            RunOnUIThread([&]()
            {
                cv->SelectionMode = xaml_controls::CalendarViewSelectionMode::Multiple;
                cv->SelectedDates->Clear();

                cv->SelectedDates->Append(date1);
                cv->SelectedDates->Append(date2);
                cv->SelectedDates->Append(date3);
            });

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(cv->SelectedDates->Size, 3u);
            });


            // switch selection mode to single, only the first selected date is kept.

            helper.PrepareSelectedDatesChangedEvent();
            helper.ExpectRemovedDate(date2);
            helper.ExpectRemovedDate(date3);


            RunOnUIThread([&]()
            {
                cv->SelectionMode = xaml_controls::CalendarViewSelectionMode::Single;
            });

            helper.WaitForSelectedDatesChanged();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(cv->SelectedDates->Size, 1u);
            });


            // switch selection mode to none, the first selected date will be clear

            helper.PrepareSelectedDatesChangedEvent();
            helper.ExpectRemovedDate(date1);

            RunOnUIThread([&]()
            {
                cv->SelectionMode = xaml_controls::CalendarViewSelectionMode::None;
            });

            helper.WaitForSelectedDatesChanged();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(cv->SelectedDates->Size, 0u);
            });
        }

        TestServices::WindowHelper->WaitForIdle();
    }

    void CalendarViewIntegrationTests::CanSelectOutOfRangeDate()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;

        ::Windows::Foundation::DateTime date = ConvertToDateTime(1, 2000, 1, 1, 1, 8, 30); // 1/1/2000 8:30 AM

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        CalendarHelper::CreateTestResources(rootPanel);



        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cv);
            // note: above "date" is not in this range.
            cv->MinDate = ConvertToDateTime(1, 2010, 1, 1);
            cv->MaxDate = ConvertToDateTime(1, 2010, 1, 1);
        });

        TestServices::WindowHelper->WaitForIdle();

        // select an invalid date (out of min/max range)
        {
            helper.PrepareSelectedDatesChangedEvent();
            helper.ExpectAddedDate(date);

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"select a date.");
                cv->SelectedDates->Append(date);
            });

            helper.WaitForSelectedDatesChanged();
        }

        // remove the date
        {
            helper.PrepareSelectedDatesChangedEvent();
            helper.ExpectRemovedDate(date);

            RunOnUIThread([&]()
            {
                cv->SelectedDates->RemoveAt(0);
            });

            helper.WaitForSelectedDatesChanged();
        }

        // change min/max to make this selected date valid
        {
            helper.PrepareSelectedDatesChangedEvent();
            helper.ExpectAddedDate(date);
            RunOnUIThread([&]()
            {
                cv->SelectedDates->Append(date);
            });
            helper.WaitForSelectedDatesChanged();

            // make it in range will not triggle additional SelectedDatesChangedEvent
            helper.PrepareSelectedDatesChangedEvent();
            RunOnUIThread([&]()
            {
                cv->MinDate = date;
                cv->MaxDate = date;
                cv->UpdateLayout();
            });
            helper.VerifyNoSelectedDatesChanged();
        }

        // remove the date
        {
            helper.PrepareSelectedDatesChangedEvent();
            helper.ExpectRemovedDate(date);

            RunOnUIThread([&]()
            {
                cv->SelectedDates->RemoveAt(0);
            });

            helper.WaitForSelectedDatesChanged();
        }
    }

    void CalendarViewIntegrationTests::CanSelectDuplicatedDates()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        xaml_controls::Grid^ rootPanel = nullptr;

        ::Windows::Foundation::DateTime date = ConvertToDateTime(1, 2010, 1, 1, 1, 8, 30); // 1/1/2000 8:30 AM

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        CalendarHelper::CreateTestResources(rootPanel);



        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cv);
            cv->SelectionMode = xaml_controls::CalendarViewSelectionMode::Multiple;
            CalendarHelper::ReplaceAccentColorForTesting(cv);
            cv->MinDate = date;
            cv->MaxDate = date;
        });

        TestServices::WindowHelper->WaitForIdle();

        const unsigned count = 2;

        // select the date for multiple times and remove the date programatically one by one (developer's behavior).
        // here are the result we expected:
        // 1. only the first time we add this date into SelectedDates, we get the SelectedDatesChanged event
        // 2. after the first time, the item is always marked as selected.
        // 3. when remove the date from SelectedDates, the item stays selected until all are removed.
        for (unsigned i = 0; i < count; i++)
        {
            helper.PrepareSelectedDatesChangedEvent();
            if (i == 0)
            {
                // only the first time we'll get selectedDatesChanged event
                helper.ExpectAddedDate(date);
            }

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"select a date (%d of %d).", i + 1, count);
                cv->SelectedDates->Append(date);
            });

            TestServices::WindowHelper->WaitForIdle();

            if (i == 0)
            {
                // only the first time we'll get selectedDatesChanged event
                helper.WaitForSelectedDatesChanged();
            }
            else
            {
                helper.VerifyNoSelectedDatesChanged();
            }

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "S");    //Selected
        }

        // remove the date one by one
        for (unsigned i = 0; i < count; i++)
        {
            helper.PrepareSelectedDatesChangedEvent();
            if (i == count - 1)
            {
                helper.ExpectRemovedDate(date);
            }

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"remove a date (%d of %d).", i + 1, count);
                cv->SelectedDates->RemoveAt(0);
            });

            TestServices::WindowHelper->WaitForIdle();

            if (i == count - 1)
            {
                // we get the SelectedDatesChanged event in the last time.
                helper.WaitForSelectedDatesChanged();
            }
            else
            {
                // otherwise, no selected dates chagned event.
                helper.VerifyNoSelectedDatesChanged();
            }

            // the item stays selected until the last time we remove the date from SelectedDates.
            if (i < count - 1)
            {
                TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "S");    // selected
            }
            else
            {
                TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "NS");   // not selected
            }
        }

        // select the date for multiple times and remove the date by tapping on the item (user's behavior).
        // in this scenario we'll expect all dates are removed together when user taps to deselect.

        RunOnUIThread([&]()
        {
            for (unsigned i = 0; i < count; i++)
            {

                LOG_OUTPUT(L"select a date (%d of %d).", i + 1, count);
                cv->SelectedDates->Append(date);
            }
        });

        TestServices::WindowHelper->WaitForIdle();
        helper.PrepareSelectedDatesChangedEvent();
        helper.ExpectRemovedDate(date);

        xaml_controls::CalendarViewDayItem^ firstItem = nullptr;
        RunOnUIThread([&]()
        {
            auto calendarPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(L"MonthViewPanel"));
            firstItem = safe_cast<xaml_controls::CalendarViewDayItem^>(calendarPanel->Children->GetAt(0));
            VERIFY_IS_TRUE(cv->SelectedDates->Size == count);
        });

        LOG_OUTPUT(L"Tap to deselect this item, all dates from SelectedDates will be cleared.");
        LOG_OUTPUT(L"Press center.");
        TestServices::InputHelper->DynamicPressCenter(firstItem, 0, 0, PointerFinger::Finger1);
        TestServices::WindowHelper->WaitForIdle();
        LOG_OUTPUT(L"Press successful.");

        LOG_OUTPUT(L"Release.");
        TestServices::InputHelper->DynamicRelease(PointerFinger::Finger1);

        // on desktop, move the mouse away to avoid hover state.
        LOG_OUTPUT(L"Moving mouse to prevent hover state on phone.");
        TestServices::InputHelper->MoveMouse(::Windows::Foundation::Point(0, 0));

        TestServices::WindowHelper->WaitForIdle();
        LOG_OUTPUT(L"Release successful.");

        LOG_OUTPUT(L"Waiting for SelectedDatesChangedEvent.");
        helper.WaitForSelectedDatesChanged();
        LOG_OUTPUT(L"SelectedDatesChangedEvent successful.");

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(cv->SelectedDates->Size == 0u);
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "NS2");   // not selected.

        // test we can avoid change a date to the save value
        RunOnUIThread([&]()
        {
            cv->SelectedDates->Append(date);
        });
        helper.PrepareSelectedDatesChangedEvent();
        RunOnUIThread([&]()
        {
            cv->SelectedDates->SetAt(0, date);  // no crash, no selected dates changed event.
        });
        helper.VerifyNoSelectedDatesChanged();
    }

    void CalendarViewIntegrationTests::CanNotSelectBlackoutDate()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;

        ::Windows::Foundation::DateTime blackoutDate = ConvertToDateTime(1, 2010, 1, 1, 1, 8, 30); // 1/1/2010 8:30 AM
        ::Windows::Foundation::DateTime normalDate = ConvertToDateTime(1, 2010, 1, 2, 1, 8, 30); // 1/2/2010 8:30 AM

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        CalendarHelper::CreateTestResources(rootPanel);



        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cv);
            cv->MinDate = blackoutDate;
            cv->MaxDate = normalDate;
        });

        TestServices::WindowHelper->WaitForIdle();

        xaml_controls::CalendarViewDayItem^ firstItem = nullptr;

        RunOnUIThread([&]()
        {
            auto calendarPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(L"MonthViewPanel"));
            firstItem = safe_cast<xaml_controls::CalendarViewDayItem^>(calendarPanel->Children->GetAt(0));
            firstItem->IsBlackout = true;
        });

        TestServices::WindowHelper->WaitForIdle();

        // tap will not select it (user's behavior).
        helper.PrepareSelectedDatesChangedEvent();
        LOG_OUTPUT(L"Tap the blackout item will not select it");
        LOG_OUTPUT(L"Press center.");
        TestServices::InputHelper->DynamicPressCenter(firstItem, 0, 0, PointerFinger::Finger1);
        TestServices::WindowHelper->WaitForIdle();
        LOG_OUTPUT(L"Release.");
        TestServices::InputHelper->DynamicRelease(PointerFinger::Finger1);
        TestServices::WindowHelper->WaitForIdle();
        helper.VerifyNoSelectedDatesChanged();
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(cv->SelectedDates->Size == 0u);
        });

        // mark the blackout date as selected will fail (developer's behavior)

        RunOnUIThread([&]()
        {
            VERIFY_THROWS_WINRT(cv->SelectedDates->Append(blackoutDate),
                Platform::Exception^,
                L"should not be able to select a blackout date");

            // SelectedDates stay unchanged.
            VERIFY_IS_TRUE(cv->SelectedDates->Size == 0u);
        });

        RunOnUIThread([&]()
        {
            cv->SelectedDates->Append(normalDate);
            VERIFY_IS_TRUE(cv->SelectedDates->Size == 1u);

            // change a normal date from SelectedDates to a blackout date will fail (developer's behavior)
            VERIFY_THROWS_WINRT(cv->SelectedDates->SetAt(0, blackoutDate),
                Platform::Exception^,
                L"should not be able to replace a selected date by a blackout date");

            // SelectedDates stay unchanged.
            VERIFY_IS_TRUE(cv->SelectedDates->Size == 1u);
        });

    }

    void CalendarViewIntegrationTests::CanNotSelectMoreDates()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;

        ::Windows::Foundation::DateTime date1 = ConvertToDateTime(1, 2005, 1, 1, 1, 8, 30); // 1/1/2005 8:30 AM
        ::Windows::Foundation::DateTime date2 = ConvertToDateTime(1, 2005, 1, 2, 1, 8, 30); // 1/2/2005 8:30 AM
        ::Windows::Foundation::DateTime minDate = ConvertToDateTime(1, 2000, 1, 1, 1, 8, 30); // 1/1/2000 8:30 AM
        ::Windows::Foundation::DateTime maxDate = ConvertToDateTime(1, 2010, 1, 1, 1, 8, 30); // 1/1/2010 8:30 AM

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cv);
            cv->MinDate = minDate;
            cv->MaxDate = maxDate;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            cv->SelectionMode = xaml_controls::CalendarViewSelectionMode::None;
            VERIFY_THROWS_WINRT(cv->SelectedDates->Append(date1),
                Platform::Exception^,
                L"should not be able to select any date when selection mode is None");

            // selectedDates stay unchanged
            VERIFY_IS_TRUE(cv->SelectedDates->Size == 0u);

            cv->SelectionMode = xaml_controls::CalendarViewSelectionMode::Single;

            cv->SelectedDates->Append(date1);

            VERIFY_IS_TRUE(cv->SelectedDates->Size == 1u);

            VERIFY_THROWS_WINRT(cv->SelectedDates->Append(date2),
                Platform::Exception^,
                L"should not be able to select more than one date when selection mode is Single");

            // selectedDates stay unchanged
            VERIFY_IS_TRUE(cv->SelectedDates->Size == 1u);
        });
    }

    void CalendarViewIntegrationTests::CanChangeSelectedDatesInsideSelectedDatesChangedEvent()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;

        ::Windows::Foundation::DateTime minDate = ConvertToDateTime(1, 2010, 1, 1); // 1/1/2010
        ::Windows::Foundation::DateTime maxDate = ConvertToDateTime(1, 2010, 1, 31); // 1/31/2010

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cv);
            cv->MinDate = minDate;
            cv->MaxDate = maxDate;
            cv->SelectionMode = xaml_controls::CalendarViewSelectionMode::Multiple;
        });

        TestServices::WindowHelper->WaitForIdle();
        int hitCounts = 0;
        auto firstSelectedDatesChanged = std::make_shared<Event>();
        auto secondSelectedDatesChanged = std::make_shared<Event>();
        auto selectedDatesChangedRegistration = CreateSafeEventRegistration(xaml_controls::CalendarView, SelectedDatesChanged);
        selectedDatesChangedRegistration.Attach(
            cv,
            ref new ::Windows::Foundation::TypedEventHandler<xaml_controls::CalendarView^, xaml_controls::CalendarViewSelectedDatesChangedEventArgs^>(
            [&](xaml_controls::CalendarView ^sender, xaml_controls::CalendarViewSelectedDatesChangedEventArgs ^e)
        {
            int number = ++hitCounts;
            LOG_OUTPUT(L"Entering CalendarViewSelectedDatesChanged event# %d", number);

            if (number == 1)
            {
                VERIFY_ARE_EQUAL(cv->SelectedDates->Size, 1u);
                VERIFY_ARE_EQUAL(e->AddedDates->Size, 1u);
                VERIFY_ARE_EQUAL(e->AddedDates->GetAt(0).UniversalTime, minDate.UniversalTime);

                LOG_OUTPUT(L"Try to select the 2nd date %ld inside the SelectedDatesChanged event", maxDate.UniversalTime);
                cv->SelectedDates->Append(maxDate);

                firstSelectedDatesChanged->Set();
            }
            else if (number == 2)
            {
                VERIFY_ARE_EQUAL(cv->SelectedDates->Size, 2u);
                VERIFY_ARE_EQUAL(e->AddedDates->Size, 1u);
                VERIFY_ARE_EQUAL(e->AddedDates->GetAt(0).UniversalTime, maxDate.UniversalTime);
                secondSelectedDatesChanged->Set();
            }
            LOG_OUTPUT(L"Leaving CalendarViewSelectedDatesChanged event# %d", number);
        }));

        xaml_controls::CalendarViewDayItem^ firstDayItem = nullptr;
        RunOnUIThread([&]()
        {
            auto monthPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(L"MonthViewPanel"));
            firstDayItem = safe_cast<xaml_controls::CalendarViewDayItem^>(monthPanel->Children->GetAt(0));
        });
        VERIFY_IS_NOT_NULL(firstDayItem);
        LOG_OUTPUT(L"select the 1st date %I64d by tap", minDate.UniversalTime);
        TestServices::InputHelper->Tap(firstDayItem);

        secondSelectedDatesChanged->WaitForDefault();
        VERIFY_IS_TRUE(secondSelectedDatesChanged->HasFired());

        firstSelectedDatesChanged->WaitForDefault();
        VERIFY_IS_TRUE(firstSelectedDatesChanged->HasFired());
    }

    void CalendarViewIntegrationTests::TestViewMode()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::Button^ headerButton = nullptr;

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        // find the header button and tap it
        {
            RunOnUIThread([&]()
            {
                headerButton = safe_cast<xaml_controls::Button^>(helper.GetTemplateChild(L"HeaderButton"));
                VERIFY_ARE_EQUAL(cv->DisplayMode, xaml_controls::CalendarViewDisplayMode::Month);
            });

            LOG_OUTPUT(L"CalendarViewIntegrationTests: changing viewmode to Year by using Tap.");

            ControlHelper::DoClickUsingAP(headerButton);

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(cv->DisplayMode, xaml_controls::CalendarViewDisplayMode::Year);
            });

            LOG_OUTPUT(L"CalendarViewIntegrationTests: changing viewmode to Decade by using Tap.");

            ControlHelper::DoClickUsingAP(headerButton);

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(cv->DisplayMode, xaml_controls::CalendarViewDisplayMode::Decade);
            });
        }
    }

    void CalendarViewIntegrationTests::TestCICEvents()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::Button^ headerButton = nullptr;

        CalendarHelper::CalendarViewHelper helper;
        helper.PrepareLoadedEvent();
        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        CalendarHelper::CreateTestResources(rootPanel);

        helper.PrepareCICEvent();

        auto date = ConvertToDateTime(1, 2000, 2, 1, 1, 12, 0, 0, 0);      //  2/1/2000 12:00:00 AM

        // set selections
        RunOnUIThread([&]()
        {
            // CIC event will be raised on all realized dayitem
            // to easily track the CIC event we let the calendarview have only one item
            cv->MinDate = date;
            cv->MaxDate = date;

            // select the only day
            cv->SelectedDates->Append(date);
        });

        // in CIC Event, we are going to blackout the selected date
        // so we expect the selectedDatesChanging event.
        helper.PrepareSelectedDatesChangedEvent();
        helper.ExpectRemovedDate(date);

        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cv);
        });

        helper.WaitForLoaded();

        helper.WaitForCICEvent();

        helper.WaitForSelectedDatesChanged();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(cv->SelectedDates->Size, 0u);
        });

        TestServices::WindowHelper->WaitForIdle();

        // verify density bars
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }

    void CalendarViewIntegrationTests::VerifyBlackoutProperty()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::Button^ headerButton = nullptr;

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        xaml_controls::CalendarViewDayItem^ firstContainer = nullptr;

        CalendarHelper::CreateTestResources(rootPanel);

        auto minDate = ConvertToDateTime(1, 2015, 1, 1);
        auto maxDate = ConvertToDateTime(1, 2020, 1, 1);

        // set selections
        RunOnUIThread([&]()
        {
            cv->MinDate = minDate;
            cv->MaxDate = maxDate;
            // display the first day, in CIC event we'll try to find the first container and set it as blackout
            cv->SetDisplayDate(minDate);
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        auto cicEvent = std::make_shared<Event>();
        auto cicRegistration = CreateSafeEventRegistration(xaml_controls::CalendarView, CalendarViewDayItemChanging);
        cicRegistration.Attach(
            cv,
            ref new ::Windows::Foundation::TypedEventHandler<xaml_controls::CalendarView^, xaml_controls::CalendarViewDayItemChangingEventArgs^>(
            [cv, minDate, &firstContainer, cicEvent](xaml_controls::CalendarView^ sender, xaml_controls::CalendarViewDayItemChangingEventArgs^ e)
        {
            if (e->Item->Date.UniversalTime == minDate.UniversalTime)
            {
                // the first container is being set up the first time, remember it and mark it as blackout
                e->Item->IsBlackout = true;
                firstContainer = e->Item;
            }
            else if (firstContainer != nullptr && e->Item == firstContainer)
            {
                // next time the firstContainer is being reused, the blackout flag should be gone
                // i.e. we don't keep the blackout flag on the container, developer should check
                // the date property and decide to set blackout flag or not.

                VERIFY_IS_FALSE(firstContainer->IsBlackout);
                cicEvent->Set();
            }

        }));

        RunOnUIThread([&]()
        {
            // scroll to maxDate, the first container will be recycled and reused.
            cv->SetDisplayDate(maxDate);
        });

        cicEvent->WaitForDefault();
        VERIFY_IS_TRUE(cicEvent->HasFired());
    }

    void CalendarViewIntegrationTests::VerifyCalendarItemCount()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;

        ::Windows::Globalization::Calendar^ calendar = nullptr;

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            // note, don't make the range too big.
            // to make sure this test works correctly, all items should be realized.
            // by default the monthview panel will realize about 120 day items.
            cv->MinDate = ConvertToDateTime(1, 2000, 2, 1, 1, 12, 0, 0, 0);      //  2/1/2000 12:00:00 AM
            cv->MaxDate = ConvertToDateTime(1, 2000, 3, 1, 2, 11, 59, 59, 0);    //  3/1/2000 11:59:59 PM
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        // find the Panels and verify the item numbers.
        {
            RunOnUIThread([&]()
            {
                xaml_controls::Primitives::CalendarPanel^ monthPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(L"MonthViewPanel"));
                VERIFY_ARE_EQUAL(monthPanel->Children->Size, 30u);  // 29 days in Feb + 1 day in Mar

                cv->DisplayMode = xaml_controls::CalendarViewDisplayMode::Year;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                xaml_controls::Primitives::CalendarPanel^ yearPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(L"YearViewPanel"));
                VERIFY_ARE_EQUAL(yearPanel->Children->Size, 2u);    // Feb + Mar

                cv->DisplayMode = xaml_controls::CalendarViewDisplayMode::Decade;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                xaml_controls::Primitives::CalendarPanel^ decadePanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(L"DecadeViewPanel"));
                VERIFY_ARE_EQUAL(decadePanel->Children->Size, 1u);  // year 2000
            });
        }
    }

    void CalendarViewIntegrationTests::ValidateNumberOfWeeksInViewRange()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::CalendarView^ calendarView = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

        RunOnUIThread([&]()
        {
            xaml_controls::StackPanel^ rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel Width="400" Height="400" xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
                        HorizontalAlignment="Center" VerticalAlignment="Center">
                        <CalendarView x:Name="calendarview" HorizontalAlignment="Stretch" VerticalAlignment="Stretch" Margin="50" NumberOfWeeksInView="2"/>
                    </StackPanel>)"));

            VERIFY_THROWS_WINRT(xaml_markup::XamlReader::Load(
                LR"(<CalendarView xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" NumberOfWeeksInView="1"/>)"),
                Platform::Exception^,
                L"Should not be able to set NumberOfWeeksInView to a value smaller than 2.");

            VERIFY_THROWS_WINRT(xaml_markup::XamlReader::Load(
                LR"(<CalendarView xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" NumberOfWeeksInView="9"/>)"),
                Platform::Exception^,
                L"Should not be able to set NumberOfWeeksInView to a value greater than 8.");

            calendarView = safe_cast<xaml_controls::CalendarView^>(rootPanel->FindName(L"calendarview"));
            VERIFY_IS_NOT_NULL(calendarView);

            loadedRegistration.Attach(rootPanel, [loadedEvent]()
            {
                LOG_OUTPUT(L"StackPanel.Loaded event raised.");
                loadedEvent->Set();
            });

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        LOG_OUTPUT(L"Waiting for StackPanel.Loaded event...");
        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Verifying NumberOfWeeksInView value set in markup.");
            VERIFY_ARE_EQUAL(calendarView->NumberOfWeeksInView, 2);

            LOG_OUTPUT(L"Setting NumberOfWeeksInView to valid value.");
            calendarView->NumberOfWeeksInView = 8;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Verifying valid NumberOfWeeksInView value.");
            VERIFY_ARE_EQUAL(calendarView->NumberOfWeeksInView, 8);

            LOG_OUTPUT(L"Verifying too small NumberOfWeeksInView value.");
            VERIFY_THROWS_WINRT(calendarView->NumberOfWeeksInView = 1,
                Platform::Exception^,
                L"Should not be able to set NumberOfWeeksInView to a value smaller than 2.");

            LOG_OUTPUT(L"Verifying too large NumberOfWeeksInView value.");
            VERIFY_THROWS_WINRT(calendarView->NumberOfWeeksInView = 9,
                Platform::Exception^,
                L"Should not be able to set NumberOfWeeksInView to a value greater than 8.");
        });
    }

    void CalendarViewIntegrationTests::VerifyButtonState()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;

        xaml_controls::Button^ headerButton = nullptr;
        xaml_controls::Button^ previousButton = nullptr;
        xaml_controls::Button^ nextButton = nullptr;
        ::Windows::Foundation::DateTime begin = ConvertToDateTime(1, 1800, 1, 1);
        ::Windows::Foundation::DateTime medium = ConvertToDateTime(1, 2200, 1, 1);
        ::Windows::Foundation::DateTime end = ConvertToDateTime(1, 2400, 1, 1);

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            cv->MinDate = begin;
            cv->MaxDate = end;
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        // find the buttons
        RunOnUIThread([&]()
        {
            headerButton = safe_cast<xaml_controls::Button^>(helper.GetTemplateChild(L"HeaderButton"));
            previousButton = safe_cast<xaml_controls::Button^>(helper.GetTemplateChild(L"PreviousButton"));
            nextButton = safe_cast<xaml_controls::Button^>(helper.GetTemplateChild(L"NextButton"));
        });

        // monthview mode:
        {
            // 1. go to the very beginning
            RunOnUIThread([&]()
            {
                cv->SetDisplayDate(begin);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(headerButton->IsEnabled);    // can switch to yearview
                VERIFY_IS_FALSE(previousButton->IsEnabled); // can't go backwards
                VERIFY_IS_TRUE(nextButton->IsEnabled);      // can go forward
            });

            // 2. go to the medium part
            RunOnUIThread([&]()
            {
                cv->SetDisplayDate(medium);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(headerButton->IsEnabled);    // can switch to yearview
                VERIFY_IS_TRUE(previousButton->IsEnabled);  // can go backward
                VERIFY_IS_TRUE(nextButton->IsEnabled);      // can go forward
            });

            // 3. go to the end
            RunOnUIThread([&]()
            {
                cv->SetDisplayDate(end);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(headerButton->IsEnabled);    // can switch to yearview
                VERIFY_IS_TRUE(previousButton->IsEnabled);  // can go backward
                VERIFY_IS_FALSE(nextButton->IsEnabled);     // can't go forward
            });
        }

        // yearview mode:
        {
            // 1. go to the very beginning
            RunOnUIThread([&]()
            {
                cv->DisplayMode = xaml_controls::CalendarViewDisplayMode::Year;
                cv->SetDisplayDate(begin);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(headerButton->IsEnabled);    // can switch to decadeview
                VERIFY_IS_FALSE(previousButton->IsEnabled); // can't go backwards
                VERIFY_IS_TRUE(nextButton->IsEnabled);      // can go forward
            });

            // 2. go to the medium part
            RunOnUIThread([&]()
            {
                cv->SetDisplayDate(medium);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(headerButton->IsEnabled);    // can switch to decadeview
                VERIFY_IS_TRUE(previousButton->IsEnabled);  // can go backward
                VERIFY_IS_TRUE(nextButton->IsEnabled);      // can go forward
            });

            // 3. go to the end
            RunOnUIThread([&]()
            {
                cv->SetDisplayDate(end);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(headerButton->IsEnabled);    // can switch to decadeview
                VERIFY_IS_TRUE(previousButton->IsEnabled);  // can go backward
                VERIFY_IS_FALSE(nextButton->IsEnabled);     // can't go forward
            });
        }

        // decadeview mode:
        {
            // 1. go to the very beginning
            RunOnUIThread([&]()
            {
                cv->DisplayMode = xaml_controls::CalendarViewDisplayMode::Decade;
                cv->SetDisplayDate(begin);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_FALSE(headerButton->IsEnabled);   // can't switch viewmode by tapping header button
                VERIFY_IS_FALSE(previousButton->IsEnabled); // can't go backwards
                VERIFY_IS_TRUE(nextButton->IsEnabled);      // can go forward
            });

            // 2. go to the medium part
            RunOnUIThread([&]()
            {
                cv->SetDisplayDate(medium);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_FALSE(headerButton->IsEnabled);   // can't switch viewmode by tapping header button
                VERIFY_IS_TRUE(previousButton->IsEnabled);  // can go backward
                VERIFY_IS_TRUE(nextButton->IsEnabled);      // can go forward
            });

            // 3. go to the end
            RunOnUIThread([&]()
            {
                cv->SetDisplayDate(end);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_FALSE(headerButton->IsEnabled);   // can't switch viewmode by tapping header button
                VERIFY_IS_TRUE(previousButton->IsEnabled);  // can go backward
                VERIFY_IS_FALSE(nextButton->IsEnabled);     // can't go forward
            });
        }
    }

    void CalendarViewIntegrationTests::VerifyNavigationButtonsBehavior()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;

        xaml_controls::Button^ headerButton = nullptr;
        xaml_controls::Button^ nextButton = nullptr;
        xaml_controls::ScrollViewer^ scrollViewer = nullptr;

        auto minDate = ConvertToDateTime(1, 2000, 1, 1);
        auto maxDate = ConvertToDateTime(1, 2100, 12, 31);

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            cv->MinDate = minDate;
            cv->MaxDate = maxDate;
            cv->SetDisplayDate(minDate);
            cv->NumberOfWeeksInView = 2;    // putting 2 rows in the view instead of the default 6, so a button tap will move by two rows instead of one scope.
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        // find the template parts
        RunOnUIThread([&]()
        {
            headerButton = safe_cast<xaml_controls::Button^>(helper.GetTemplateChild(L"HeaderButton"));
            nextButton = safe_cast<xaml_controls::Button^>(helper.GetTemplateChild(L"NextButton"));
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(helper.GetTemplateChild(L"MonthViewScrollViewer"));
        });

        auto viewChangedEvent = std::make_shared<Event>();
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        viewChangedRegistration.Attach(scrollViewer,
            ref new ::Windows::Foundation::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
            [viewChangedEvent](Platform::Object ^sender, xaml_controls::ScrollViewerViewChangedEventArgs ^e)
        {
            if (!e->IsIntermediate)
            {
                viewChangedEvent->Set();
            }
        }));

        // verify that when tapping the next button 3 times, we are still on the first month.
        for (int i = 0; i < 3; i++)
        {
            RunOnUIThread([&]()
            {
                auto headText = safe_cast<Platform::String^>(headerButton->Content);
                LOG_OUTPUT(L"Current month %s", headText->Data());
                VERIFY_IS_TRUE(headText == L"\u200eJanuary\u200e \u200e2000");
            });

            TestServices::InputHelper->Tap(nextButton);
            viewChangedEvent->WaitForDefault();
            VERIFY_IS_TRUE(viewChangedEvent->HasFired());
            viewChangedEvent->Reset();
        }
    }

    void CalendarViewIntegrationTests::VerifySelfAdaptivePanel()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        auto minDate = ConvertToDateTime(1, 2000, 1, 1);
        auto maxDate = ConvertToDateTime(1, 2100, 12, 31);

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            cv->MinDate = minDate;
            cv->MaxDate = maxDate;
            cv->SetDisplayDate(minDate);
            cv->Language = L"sms";  // long month name in this language
            cv->DisplayMode = xaml_controls::CalendarViewDisplayMode::Year;
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Validate Gregorian Calendar with sms language in Year mode (should be 1 x 4)");
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "1");

        RunOnUIThread([&]()
        {
            cv->Language = L"en-us";
            cv->MonthYearItemFontSize = 36;
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Validate Gregorian Calendar with big font size in Year mode (should be 3 x 4)");
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "2");

        RunOnUIThread([&]()
        {
            cv->SetYearDecadeDisplayDimensions(4, 4); // explicitly set the dimension, the panel will no longer adjust the dimension.
        });

        TestServices::WindowHelper->WaitForIdle();
        LOG_OUTPUT(L"Validate Gregorian Calendar with big font size but have dimension set explicitly in Year mode (should be 4 x 4, i.e. some items get clipped)");
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "3");
    }

    void CalendarViewIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        // Note: CalendarView can't use below commented line to test "CanEnterAndLeaveLiveTree"
        // the problem is in below helper, we did these:
        //  1. create CalendarView
        //  2. added into visual tree
        //  3. test loaded and unloaded event
        //  4. remove calendarview from visual tree
        //  5. destroy CalendarView
        //  .....

        // because we destroy CalendarView after we remove it from visual tree, so if there are any left work in build tree services, they can't be cleaned up correctly.
        // this should happens on ListView and GridView, however for default ListView and GridView (especially in below helper method) are empty and there is no buildtree work.
        // But for default CalendarView, we have! because default Calendarview will show the dates in 3 years.

        //Generic::FrameworkElementTests<xaml_controls::CalendarView>::CanEnterAndLeaveLiveTree();

        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        // remove from visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Clear();
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    static void VerifyItemPositionInPanel(Microsoft::UI::Xaml::UIElement^ item, xaml_controls::Primitives::CalendarPanel^ panel, int col, int row)
    {
        ::Windows::Foundation::Point origin = { 0, 0 };
        auto itemPos = item->TransformToVisual(panel)->TransformPoint(origin);
        auto itemWidth = safe_cast<Microsoft::UI::Xaml::FrameworkElement^>(item)->ActualWidth;
        auto itemHeight = safe_cast<Microsoft::UI::Xaml::FrameworkElement^>(item)->ActualHeight;
        auto margin = safe_cast<Microsoft::UI::Xaml::FrameworkElement^>(item)->Margin;

        itemWidth += margin.Left + margin.Right;
        itemHeight += margin.Top + margin.Bottom;

        itemPos.X -= static_cast<float>(margin.Left);
        itemPos.Y -= static_cast<float>(margin.Top);

        VERIFY_IS_TRUE(CalendarHelper::AreClose(itemPos.X, itemWidth * col, 1.0 /*rounding issue, to be fixed*/));
        VERIFY_IS_TRUE(CalendarHelper::AreClose(itemPos.Y, itemHeight * row, 1.0 /*rounding issue, to be fixed*/));
    }

    static void VerifyItemCountInViewport(xaml_controls::Primitives::CalendarPanel^ panel, xaml_controls::ScrollViewer^ scrollViewer, int col, int row)
    {
        auto item = panel->Children->GetAt(0);  // any item, we just want the size.
        auto itemWidth = safe_cast<Microsoft::UI::Xaml::FrameworkElement^>(item)->ActualWidth;
        auto itemHeight = safe_cast<Microsoft::UI::Xaml::FrameworkElement^>(item)->ActualHeight;
        auto viewportWidth = scrollViewer->ViewportWidth;
        auto viewportHeight = scrollViewer->ViewportHeight;

        auto margin = safe_cast<Microsoft::UI::Xaml::FrameworkElement^>(item)->Margin;

        itemWidth += margin.Left + margin.Right;
        itemHeight += margin.Top + margin.Bottom;

        VERIFY_IS_TRUE(CalendarHelper::AreClose(viewportWidth / col, itemWidth, 1.0 /*rounding issue, to be fixed*/));
        VERIFY_IS_TRUE(CalendarHelper::AreClose(viewportHeight / row, itemHeight, 1.0 /*rounding issue, to be fixed*/));
    }

    void CalendarViewIntegrationTests::CalendarPanelLayoutTestFirstItemPositonTest()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::Primitives::CalendarPanel^ monthPanel = nullptr;
        Microsoft::UI::Xaml::UIElement^ firstDayItem = nullptr;
        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        CalendarHelper::CreateTestResources(rootPanel);

        auto minDate = ConvertToDateTime(1, 2014, 8, 30); // Saturday
        auto maxDate = ConvertToDateTime(1, 2036, 1, 1);


        // load into visual tree
        RunOnUIThread([&]()
        {
            cv->MinDate = minDate;
            cv->MaxDate = maxDate;
            cv->SetDisplayDate(minDate);
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            monthPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(L"MonthViewPanel"));
            firstDayItem = monthPanel->Children->GetAt(0);
        });

        TestServices::WindowHelper->WaitForIdle();

        // the first day item is on Saturday, when we change FirstDayOfWeek from Sunday (0) to Saturday (6), the col of first item will be from 6 to 0
        for (int i = 0; i < 7; i++)
        {
            ::Windows::Globalization::DayOfWeek dayofWeek = static_cast<::Windows::Globalization::DayOfWeek>(static_cast<int>(::Windows::Globalization::DayOfWeek::Sunday) + i);

            RunOnUIThread([&]()
            {
                cv->FirstDayOfWeek = dayofWeek;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                VerifyItemPositionInPanel(firstDayItem, monthPanel, 6 - i /*col*/, 0 /*the first item is always stay in row 0.*/);
            });
        }

        TestServices::WindowHelper->WaitForIdle();

        // now change to year view, the item start at 0 always
        RunOnUIThread([&]()
        {

            cv->DisplayMode = xaml_controls::CalendarViewDisplayMode::Year;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto yearPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(L"YearViewPanel"));
            auto firstYearItem = yearPanel->Children->GetAt(0);
            VerifyItemPositionInPanel(firstYearItem, yearPanel, 0 /*col*/, 0 /*row*/);
        });

        // now change to decade view, the item start at 0 always
        RunOnUIThread([&]()
        {

            cv->DisplayMode = xaml_controls::CalendarViewDisplayMode::Decade;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto decadePanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(L"YearViewPanel"));
            auto firstDecadeItem = decadePanel->Children->GetAt(0);
            VerifyItemPositionInPanel(firstDecadeItem, decadePanel, 0 /*col*/, 0 /*row*/);
        });

    }

    void CalendarViewIntegrationTests::CalendarPanelLayoutTestRowsAndColsTest()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::Primitives::CalendarPanel^ calendarPanel = nullptr;
        xaml_controls::ScrollViewer^ scrollViewer = nullptr;
        Microsoft::UI::Xaml::UIElement^ calendarItem = nullptr;
        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        // month view, we change the NumberOfWeeksInView from 2 to 8 and then go back to 2
        int numberofWeeksInView[] = { 2, 3, 4, 5, 6, 7, 8, 6, 4, 2 };
        for (int i = 0; i < ARRAYSIZE(numberofWeeksInView); i++)
        {
            int numberOfWeeksInView = numberofWeeksInView[i];
            RunOnUIThread([&]()
            {
                cv->NumberOfWeeksInView = numberOfWeeksInView;
                calendarPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(L"MonthViewPanel"));
                scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(helper.GetTemplateChild(L"MonthViewScrollViewer"));
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VerifyItemCountInViewport(calendarPanel, scrollViewer, 7, numberOfWeeksInView);
            });
        }

        // now test year view and decacde view

        xaml_controls::CalendarViewDisplayMode modes[] = { xaml_controls::CalendarViewDisplayMode::Year, xaml_controls::CalendarViewDisplayMode::Decade };
        Platform::String^ panelNames[] = { L"YearViewPanel", L"DecadeViewPanel" };
        Platform::String^ scrollViewerNames[] = { L"YearViewScrollViewer", L"DecadeViewScrollViewer" };
        for (int i = 0; i < 2; i++)
        {
            // first let's change to the corresponding view.
            RunOnUIThread([&]()
            {
                cv->DisplayMode = modes[i];
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                calendarPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(panelNames[i]));
                scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(helper.GetTemplateChild(scrollViewerNames[i]));
            });

            // change the dimensions and verify size.
            for (int row = 1; row < 8; row += 2)
            {
                for (int col = 1; col < 8; col += 2)
                {
                    RunOnUIThread([&]()
                    {
                        cv->SetYearDecadeDisplayDimensions(col, row);
                    });

                    TestServices::WindowHelper->WaitForIdle();

                    RunOnUIThread([&]()
                    {
                        VerifyItemCountInViewport(calendarPanel, scrollViewer, col, row);
                    });
                }
            }

        }
    }

    void CalendarViewIntegrationTests::CalendarPanelLayoutTestStretchTest()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::Primitives::CalendarPanel^ calendarPanel = nullptr;
        xaml_controls::ScrollViewer^ scrollViewer = nullptr;
        Microsoft::UI::Xaml::UIElement^ calendarItem = nullptr;
        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        struct AlignmentHelper
        {
            Microsoft::UI::Xaml::HorizontalAlignment ha;
            Microsoft::UI::Xaml::HorizontalAlignment hca;
            Microsoft::UI::Xaml::VerticalAlignment va;
            Microsoft::UI::Xaml::VerticalAlignment vca;
        };

        // test when changing alignment from stretch to non-stretch mode, for both Horizontal and Vertical
        AlignmentHelper alignments[] = {
                { Microsoft::UI::Xaml::HorizontalAlignment::Center, Microsoft::UI::Xaml::HorizontalAlignment::Center, Microsoft::UI::Xaml::VerticalAlignment::Center, Microsoft::UI::Xaml::VerticalAlignment::Center },
                { Microsoft::UI::Xaml::HorizontalAlignment::Stretch, Microsoft::UI::Xaml::HorizontalAlignment::Stretch, Microsoft::UI::Xaml::VerticalAlignment::Center, Microsoft::UI::Xaml::VerticalAlignment::Center },
                { Microsoft::UI::Xaml::HorizontalAlignment::Stretch, Microsoft::UI::Xaml::HorizontalAlignment::Stretch, Microsoft::UI::Xaml::VerticalAlignment::Stretch, Microsoft::UI::Xaml::VerticalAlignment::Stretch },
                { Microsoft::UI::Xaml::HorizontalAlignment::Stretch, Microsoft::UI::Xaml::HorizontalAlignment::Center, Microsoft::UI::Xaml::VerticalAlignment::Stretch, Microsoft::UI::Xaml::VerticalAlignment::Center },
                { Microsoft::UI::Xaml::HorizontalAlignment::Center, Microsoft::UI::Xaml::HorizontalAlignment::Stretch, Microsoft::UI::Xaml::VerticalAlignment::Center, Microsoft::UI::Xaml::VerticalAlignment::Stretch },
                { Microsoft::UI::Xaml::HorizontalAlignment::Center, Microsoft::UI::Xaml::HorizontalAlignment::Center, Microsoft::UI::Xaml::VerticalAlignment::Center, Microsoft::UI::Xaml::VerticalAlignment::Center },
        };

        struct ModeHelper
        {
            xaml_controls::CalendarViewDisplayMode mode;
            Platform::String^ panelName;
            Platform::String^ scrollViewerName;
            int col;
            int row;
        };

        ModeHelper modes[] = {
                { xaml_controls::CalendarViewDisplayMode::Month, L"MonthViewPanel", L"MonthViewScrollViewer", 7, 6 },
                { xaml_controls::CalendarViewDisplayMode::Year, L"YearViewPanel", L"YearViewScrollViewer", 4, 4 },
                { xaml_controls::CalendarViewDisplayMode::Decade, L"DecadeViewPanel", L"DecadeViewScrollViewer", 4, 4 },
        };

        for (int j = 0; j < ARRAYSIZE(alignments); j++)
        {
            RunOnUIThread([&]()
            {
                cv->HorizontalAlignment = alignments[j].ha;
                cv->HorizontalContentAlignment = alignments[j].hca;
                cv->VerticalAlignment = alignments[j].va;
                cv->VerticalContentAlignment = alignments[j].vca;
            });

            TestServices::WindowHelper->WaitForIdle();

            for (int i = 0; i < ARRAYSIZE(modes); i++)
            {
                RunOnUIThread([&]()
                {
                    cv->DisplayMode = modes[i].mode;
                });

                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    calendarPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(modes[i].panelName));
                    scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(helper.GetTemplateChild(modes[i].scrollViewerName));
                });

                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    VerifyItemCountInViewport(calendarPanel, scrollViewer, modes[i].col, modes[i].row);
                });
            }
        }
    }

    void CalendarViewIntegrationTests::CanSwitchDisplayModeByCtrlUpAfterLoaded()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::Primitives::CalendarPanel^ calendarPanel = nullptr;
        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            calendarPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(L"MonthViewPanel"));
            auto firstItem = safe_cast<xaml_controls::CalendarViewDayItem^>(calendarPanel->Children->GetAt(0));
            firstItem->Focus(Microsoft::UI::Xaml::FocusState::Programmatic);
        });

        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_up#$u$_up#$u$_ctrl");
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(cv->DisplayMode == xaml_controls::CalendarViewDisplayMode::Year);
        });
    }

    void CalendarViewIntegrationTests::KeyboardNavigationTestNavigationKeyTest()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::Primitives::CalendarPanel^ calendarPanel = nullptr;
        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();
        CompareDate comparer;

        CalendarHelper::CreateTestResources(rootPanel);


        auto minDate = ConvertToDateTime(1, 2014, 9, 3); // Wednesday
        auto maxDate = ConvertToDateTime(1, 2015, 9, 3);

        // load into visual tree
        RunOnUIThread([&]()
        {
            cv->MinDate = minDate;
            cv->MaxDate = maxDate;
            cv->SetDisplayDate(minDate);
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            calendarPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(L"MonthViewPanel"));
            auto firstItem = safe_cast<xaml_controls::CalendarViewDayItem^>(calendarPanel->Children->GetAt(0));
            firstItem->Focus(Microsoft::UI::Xaml::FocusState::Programmatic);
        });

        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            auto focusedElement = safe_cast<xaml_controls::CalendarViewDayItem^>(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            CalendarHelper::DumpDate(focusedElement->Date, L"Focus starts from ");
            VERIFY_IS_TRUE(comparer(focusedElement->Date, minDate));
        });

        TestServices::KeyboardHelper->Down();
        TestServices::KeyboardHelper->Down();
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            auto focusedElement = safe_cast<xaml_controls::CalendarViewDayItem^>(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            CalendarHelper::DumpDate(focusedElement->Date, L"Pressed down twice, now focus is on ");
            VERIFY_IS_TRUE(comparer(focusedElement->Date, ConvertToDateTime(1, 2014, 9, 17)));
        });

        TestServices::KeyboardHelper->PressKeySequence(L"$d$_left#$u$_left#$d$_left#$u$_left#$d$_left#$u$_left#$d$_left#$u$_left");
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            auto focusedElement = safe_cast<xaml_controls::CalendarViewDayItem^>(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            CalendarHelper::DumpDate(focusedElement->Date, L"Pressed left 4 times, now focus is on ");
            VERIFY_IS_TRUE(comparer(focusedElement->Date, ConvertToDateTime(1, 2014, 9, 13)));
        });

        TestServices::KeyboardHelper->PressKeySequence(L"$d$_right#$u$_right");
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            auto focusedElement = safe_cast<xaml_controls::CalendarViewDayItem^>(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            CalendarHelper::DumpDate(focusedElement->Date, L"Pressed right once, now focus is on ");
            VERIFY_IS_TRUE(comparer(focusedElement->Date, ConvertToDateTime(1, 2014, 9, 14)));
        });

        TestServices::KeyboardHelper->Up();
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            auto focusedElement = safe_cast<xaml_controls::CalendarViewDayItem^>(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            CalendarHelper::DumpDate(focusedElement->Date, L"Pressed up once, now focus is on ");
            VERIFY_IS_TRUE(comparer(focusedElement->Date, ConvertToDateTime(1, 2014, 9, 7)));
        });

        TestServices::KeyboardHelper->PressKeySequence(L"$d$_home#$u$_home");
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            auto focusedElement = safe_cast<xaml_controls::CalendarViewDayItem^>(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            CalendarHelper::DumpDate(focusedElement->Date, L"Pressed home once, now focus is on ");
            // note we should be at 9/1 but the first day is 9/3.
            VERIFY_IS_TRUE(comparer(focusedElement->Date, ConvertToDateTime(1, 2014, 9, 3)));
        });

        TestServices::KeyboardHelper->PressKeySequence(L"$d$_home#$u$_home");
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            auto focusedElement = safe_cast<xaml_controls::CalendarViewDayItem^>(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            CalendarHelper::DumpDate(focusedElement->Date, L"Pressed home again, now focus is on ");
            VERIFY_IS_TRUE(comparer(focusedElement->Date, ConvertToDateTime(1, 2014, 9, 3)));
        });

        TestServices::KeyboardHelper->PressKeySequence(L"$d$_end#$u$_end");
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            auto focusedElement = safe_cast<xaml_controls::CalendarViewDayItem^>(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            CalendarHelper::DumpDate(focusedElement->Date, L"Pressed end once, now focus is on ");
            VERIFY_IS_TRUE(comparer(focusedElement->Date, ConvertToDateTime(1, 2014, 9, 30)));
        });

        TestServices::KeyboardHelper->PressKeySequence(L"$d$_end#$u$_end");
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            auto focusedElement = safe_cast<xaml_controls::CalendarViewDayItem^>(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            CalendarHelper::DumpDate(focusedElement->Date, L"Pressed end again, now focus is on ");
            VERIFY_IS_TRUE(comparer(focusedElement->Date, ConvertToDateTime(1, 2014, 9, 30)));
        });

        TestServices::KeyboardHelper->PressKeySequence(L"$d$_pagedown#$u$_pagedown");
        // sometime the following right key press event is not handled by App, can't repro this issue locally with or without stress.
        // try an additional UpdateLayout to see if it helps.
        RunOnUIThread([&]()
        {
            cv->UpdateLayout();
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_right#$u$_right");
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            auto focusedElement = safe_cast<xaml_controls::CalendarViewDayItem^>(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            CalendarHelper::DumpDate(focusedElement->Date, L"Pressed pagedown once followed by right once, now focus is on ");
            VERIFY_IS_TRUE(comparer(focusedElement->Date, ConvertToDateTime(1, 2014, 10, 31)));
        });

        TestServices::KeyboardHelper->PressKeySequence(L"$d$_pageup#$u$_pageup");
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            auto focusedElement = safe_cast<xaml_controls::CalendarViewDayItem^>(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            CalendarHelper::DumpDate(focusedElement->Date, L"Pressed pagedown once followed by right once, now focus is on ");
            VERIFY_IS_TRUE(comparer(focusedElement->Date, ConvertToDateTime(1, 2014, 9, 30)));
        });

        TestServices::KeyboardHelper->PressKeySequence(L"$d$_pagedown#$u$_pagedown");
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            auto focusedElement = safe_cast<xaml_controls::CalendarViewDayItem^>(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            CalendarHelper::DumpDate(focusedElement->Date, L"Pressed pagedown once followed by right once, now focus is on ");
            VERIFY_IS_TRUE(comparer(focusedElement->Date, ConvertToDateTime(1, 2014, 10, 30)));
        });
    }

    void CalendarViewIntegrationTests::ValidateNavigation()
    {
        TestCleanupWrapper cleanup;
        InputDevice device = InputDevice::Gamepad;

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::Primitives::CalendarPanel^ calendarPanel = nullptr;
        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();
        CompareDate comparer;

        CalendarHelper::CreateTestResources(rootPanel);

        auto minDate = ConvertToDateTime(1, 2014, 9, 3); // Wednesday
        auto maxDate = ConvertToDateTime(1, 2015, 9, 3);

        // load into visual tree
        RunOnUIThread([&]()
        {
            cv->MinDate = minDate;
            cv->MaxDate = maxDate;
            cv->SetDisplayDate(minDate);
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            calendarPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(L"MonthViewPanel"));
            auto firstItem = safe_cast<xaml_controls::CalendarViewDayItem^>(calendarPanel->Children->GetAt(0));
            firstItem->Focus(Microsoft::UI::Xaml::FocusState::Programmatic);
        });

        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            auto focusedElement = safe_cast<xaml_controls::CalendarViewDayItem^>(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            CalendarHelper::DumpDate(focusedElement->Date, L"Focus starts from ");
            VERIFY_IS_TRUE(comparer(focusedElement->Date, minDate));
        });

        CommonInputHelper::Down(device);
        CommonInputHelper::Down(device);
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            auto focusedElement = safe_cast<xaml_controls::CalendarViewDayItem^>(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            CalendarHelper::DumpDate(focusedElement->Date, L"Pressed down twice, now focus is on ");
            VERIFY_IS_TRUE(comparer(focusedElement->Date, ConvertToDateTime(1, 2014, 9, 17)));
        });

        CommonInputHelper::Left(device);
        CommonInputHelper::Left(device);
        CommonInputHelper::Left(device);
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            auto focusedElement = safe_cast<xaml_controls::CalendarViewDayItem^>(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            CalendarHelper::DumpDate(focusedElement->Date, L"Pressed left 3 times, now focus is on ");
            VERIFY_IS_TRUE(comparer(focusedElement->Date, ConvertToDateTime(1, 2014, 9, 14)));
        });

        CommonInputHelper::Left(device);
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            auto focusedElement = safe_cast<xaml_controls::CalendarViewDayItem^>(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            CalendarHelper::DumpDate(focusedElement->Date, L"Pressed left again, now focus is on ");
            VERIFY_IS_TRUE(comparer(focusedElement->Date, ConvertToDateTime(1, 2014, 9, 14)));
        });

        CommonInputHelper::Right(device);
        CommonInputHelper::Right(device);
        CommonInputHelper::Right(device);
        CommonInputHelper::Right(device);
        CommonInputHelper::Right(device);
        CommonInputHelper::Right(device);
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            auto focusedElement = safe_cast<xaml_controls::CalendarViewDayItem^>(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            CalendarHelper::DumpDate(focusedElement->Date, L"Pressed right 6 times, now focus is on ");
            VERIFY_IS_TRUE(comparer(focusedElement->Date, ConvertToDateTime(1, 2014, 9, 20)));
        });

        CommonInputHelper::Right(device);
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            auto focusedElement = safe_cast<xaml_controls::CalendarViewDayItem^>(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            CalendarHelper::DumpDate(focusedElement->Date, L"Pressed right again, now focus is on ");
            VERIFY_IS_TRUE(comparer(focusedElement->Date, ConvertToDateTime(1, 2014, 9, 20)));
        });

        CommonInputHelper::Up(device);
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            auto focusedElement = safe_cast<xaml_controls::CalendarViewDayItem^>(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            CalendarHelper::DumpDate(focusedElement->Date, L"Pressed up once, now focus is on ");
            VERIFY_IS_TRUE(comparer(focusedElement->Date, ConvertToDateTime(1, 2014, 9, 13)));
        });
    }

    void CalendarViewIntegrationTests::KeyboardNavigationTestCanTryToNavigateOutOfBoundary()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::Primitives::CalendarPanel^ calendarPanel = nullptr;
        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();
        CompareDate comparer;
        CalendarHelper::CreateTestResources(rootPanel);

        auto minDate = ConvertToDateTime(1, 2014, 9, 3); // Wednesday
        auto maxDate = minDate;

        // load into visual tree
        RunOnUIThread([&]()
        {
            cv->MinDate = minDate;
            cv->MaxDate = maxDate;
            cv->SetDisplayDate(minDate);
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            calendarPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(L"MonthViewPanel"));
            auto firstItem = safe_cast<xaml_controls::CalendarViewDayItem^>(calendarPanel->Children->GetAt(0));
            firstItem->Focus(Microsoft::UI::Xaml::FocusState::Programmatic);
        });


        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            CalendarHelper::CheckFocusedItem();
            auto focusedElement = safe_cast<xaml_controls::CalendarViewDayItem^>(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            CalendarHelper::DumpDate(focusedElement->Date, L"Focus starts from ");
            VERIFY_IS_TRUE(comparer(focusedElement->Date, minDate));
        });

        TestServices::KeyboardHelper->Down();
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            // Cinch fails here - the focusedElement is not a CalendarVIewDayItem, however the focus should be still on the calendarViewDayItem and it is true locally.
            // add a check to see where is the focus now.
            CalendarHelper::CheckFocusedItem();
            auto focusedElement = safe_cast<xaml_controls::CalendarViewDayItem^>(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            CalendarHelper::DumpDate(focusedElement->Date, L"Pressed down, now focus is on ");
            VERIFY_IS_TRUE(comparer(focusedElement->Date, minDate));
        });

        TestServices::KeyboardHelper->Up();
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            CalendarHelper::CheckFocusedItem();
            auto focusedElement = safe_cast<xaml_controls::CalendarViewDayItem^>(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            CalendarHelper::DumpDate(focusedElement->Date, L"Pressed up, now focus is on ");
            VERIFY_IS_TRUE(comparer(focusedElement->Date, minDate));
        });


        TestServices::KeyboardHelper->PressKeySequence(L"$d$_right#$u$_right");
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            CalendarHelper::CheckFocusedItem();
            auto focusedElement = safe_cast<xaml_controls::CalendarViewDayItem^>(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            CalendarHelper::DumpDate(focusedElement->Date, L"Pressed right, now focus is on ");
            VERIFY_IS_TRUE(comparer(focusedElement->Date, minDate));
        });

        TestServices::KeyboardHelper->PressKeySequence(L"$d$_left#$u$_left");
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            CalendarHelper::CheckFocusedItem();
            auto focusedElement = safe_cast<xaml_controls::CalendarViewDayItem^>(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            CalendarHelper::DumpDate(focusedElement->Date, L"Pressed left, now focus is on ");
            VERIFY_IS_TRUE(comparer(focusedElement->Date, minDate));
        });


    }

    static bool IsCalendarItem(Platform::Object^ object)
    {
        if (dynamic_cast<xaml_controls::CalendarViewDayItem^>(object) != nullptr)
            return true;
        else
        {
            // only CalendarViewDayItem is public, so for the item in YearView and DecadeView (they are CalendarViewItems, which are not public)
            // we have to check:
            //  1. It is a control
            //  2. It's parent is CalendarPanel

            // we still can't make sure that this item is a CalendarItem but above check is good enough in this test.
            if (dynamic_cast<xaml_controls::Control^>(object) != nullptr)
            {
                auto parent = xaml_media::VisualTreeHelper::GetParent(safe_cast<Microsoft::UI::Xaml::DependencyObject^>(object));
                return dynamic_cast<xaml_controls::Primitives::CalendarPanel^>(parent) != nullptr;
            }

        }
        return false;
    }

    void CalendarViewIntegrationTests::KeyboardNavigationTestTabTest()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::Primitives::CalendarPanel^ calendarPanel = nullptr;
        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            // make sure in decadeview, we have enough items so at least one navigation button is enabled
            // or we can't tab/shift-tab out from a focused item.
            cv->MinDate = ConvertToDateTime(1, 1900, 1, 1);
            cv->MaxDate = ConvertToDateTime(1, 2300, 12, 31);

            xaml_controls::Button^ btn = ref new xaml_controls::Button();
            btn->Content = L"Button to accept focus since the navigation buttons are invisible by default now.";

            rootPanel->Children->Append(cv);
            rootPanel->Children->Append(btn);
        });

        TestServices::WindowHelper->WaitForIdle();

        struct ModeHelper
        {
            Platform::String^ panelName;
            xaml_controls::CalendarViewDisplayMode displayMode;
        };

        std::array<ModeHelper, 3> modes{ {
                { L"MonthViewPanel", xaml_controls::CalendarViewDisplayMode::Month },
                { L"YearViewPanel", xaml_controls::CalendarViewDisplayMode::Year },
                { L"DecadeViewPanel", xaml_controls::CalendarViewDisplayMode::Decade },
                } };

        for (auto& mode : modes)
        {
            RunOnUIThread([&]()
            {
                cv->DisplayMode = mode.displayMode;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                calendarPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(mode.panelName));
                // let's start from not the first or last item.
                auto item = safe_cast<xaml_controls::Control^>(calendarPanel->Children->GetAt(1));
                item->Focus(Microsoft::UI::Xaml::FocusState::Programmatic);
            });

            TestServices::WindowHelper->WaitForIdle();

            // tab out
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                auto focusedElement = Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot);
                VERIFY_IS_TRUE(!IsCalendarItem(focusedElement));
            });

            // shift tab back
            TestServices::KeyboardHelper->ShiftTab();
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                auto focusedElement = Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot);
                VERIFY_IS_TRUE(IsCalendarItem(focusedElement));
            });

            // shift tab out
            TestServices::KeyboardHelper->ShiftTab();
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                auto focusedElement = Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot);
                VERIFY_IS_TRUE(!IsCalendarItem(focusedElement));
            });

            // tab back
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                auto focusedElement = Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot);
                VERIFY_IS_TRUE(IsCalendarItem(focusedElement));
            });
        }
    }

    void CalendarViewIntegrationTests::KeyboardNavigationTestSpaceEnterTest()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::Primitives::CalendarPanel^ calendarPanel = nullptr;
        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();
        CompareDate comparer;

        CalendarHelper::CreateTestResources(rootPanel);

        // in this test we'll focus on 2014/1/31, then switch to year 2015, month 2, we should see the day is adjusted to 28 (2/28/2015).
        auto minDate = ConvertToDateTime(1, 2014, 1, 31);
        auto maxDate = ConvertToDateTime(1, 2015, 9, 3);

        // load into visual tree
        RunOnUIThread([&]()
        {
            cv->MinDate = minDate;
            cv->MaxDate = maxDate;
            cv->SetDisplayDate(minDate);
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            calendarPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(L"MonthViewPanel"));
            auto firstItem = safe_cast<xaml_controls::CalendarViewDayItem^>(calendarPanel->Children->GetAt(0));
            firstItem->Focus(Microsoft::UI::Xaml::FocusState::Programmatic);
        });

        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            CalendarHelper::CheckFocusedItem();
            auto focusedElement = safe_cast<xaml_controls::CalendarViewDayItem^>(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            CalendarHelper::DumpDate(focusedElement->Date, L"Focus starts from ");
            VERIFY_IS_TRUE(comparer(focusedElement->Date, minDate));
        });

        // ctrl + up
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_up#$u$_up#$u$_ctrl");
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            CalendarHelper::CheckFocusedItem();
            // the focused element is calendar monthitem, which is not public, we can not verify the date of that item
            VERIFY_IS_TRUE(IsCalendarItem(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)));
            VERIFY_IS_TRUE(cv->DisplayMode == xaml_controls::CalendarViewDisplayMode::Year);
        });

        // ctrl + up
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_up#$u$_up#$u$_ctrl");
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            CalendarHelper::CheckFocusedItem();
            // the focused element is calendar yearitem, which is not public, we can not verify the date of that item
            VERIFY_IS_TRUE(IsCalendarItem(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)));
            VERIFY_IS_TRUE(cv->DisplayMode == xaml_controls::CalendarViewDisplayMode::Decade);
        });

        // ctrl + up
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_up#$u$_up#$u$_ctrl");
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            CalendarHelper::CheckFocusedItem();
            // nothing is changed this time.
            VERIFY_IS_TRUE(IsCalendarItem(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)));
            VERIFY_IS_TRUE(cv->DisplayMode == xaml_controls::CalendarViewDisplayMode::Decade);
        });

        // right, ctrl + down - > to select year 2015 and switch back to Year mode
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_right#$u$_right#$d$_ctrl#$d$_down#$u$_down#$u$_ctrl");
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            CalendarHelper::CheckFocusedItem();
            VERIFY_IS_TRUE(IsCalendarItem(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)));
            VERIFY_IS_TRUE(cv->DisplayMode == xaml_controls::CalendarViewDisplayMode::Year);
        });

        // right, ctrl + down - > to select month Feburary and switch back to month mode
        // we should on 2/28/2015
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_right#$u$_right#$d$_ctrl#$d$_down#$u$_down#$u$_ctrl");
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            CalendarHelper::CheckFocusedItem();
            auto focusedElement = safe_cast<xaml_controls::CalendarViewDayItem^>(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            CalendarHelper::DumpDate(focusedElement->Date, L"Pressed right, ctrl + down, now focus is on ");
            VERIFY_IS_TRUE(comparer(focusedElement->Date, ConvertToDateTime(1, 2015, 2, 28)));
            VERIFY_IS_TRUE(cv->DisplayMode == xaml_controls::CalendarViewDisplayMode::Month);
        });

        // ctrl + down -> nothing should happen
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_down#$u$_down#$u$_ctrl");
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            CalendarHelper::CheckFocusedItem();
            auto focusedElement = safe_cast<xaml_controls::CalendarViewDayItem^>(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            CalendarHelper::DumpDate(focusedElement->Date, L"Pressed ctrl + down, now focus is on ");
            VERIFY_IS_TRUE(comparer(focusedElement->Date, ConvertToDateTime(1, 2015, 2, 28)));
            VERIFY_IS_TRUE(cv->DisplayMode == xaml_controls::CalendarViewDisplayMode::Month);
        });

    }

    void CalendarViewIntegrationTests::SnapPointTest()
    {
         // Note: InputHelper->Flick does not like a flick, it is more like a pan. by default CalendarView
        // has optional snap point so we can't test the snap point with a "pan".

        // This test is designed to validate that CalendarView
        //  1. has a regular snap point on each row if the current viewport doesn't have enough space to show a full scope.
        //  2. has an irregular snap point on the first item of each scope if the current viewport has enough space to show a full scope.

        // So here we'll "hack" the snap point type to MandatorySingle so we can use InputHelper->ScrollMouseWheel to test this behavior.


        struct ModeHelper
        {
            Platform::String^ panelName;
            Platform::String^ scrollViewerName;
            xaml_controls::CalendarViewDisplayMode displayMode;
            struct TestDataHelper
            {
                int row;
                int col;
                std::array<int, 3> snapPoints;    // each mode we test the first 3 snap points
            };
            // in each mode we have two group of data to test: regular and irregular.
            std::array<TestDataHelper, 2> testData;
        };

        std::array<ModeHelper, 3> modes
        { {
                { L"MonthViewPanel", L"MonthViewScrollViewer", xaml_controls::CalendarViewDisplayMode::Month,
                { {
                    // dimension 6 x 7, irregular snap point (per scope).
                        { 6, 7, { { 5, 9, 13 } } },
                        // dimension 4 x 7, regular snap point (per row).
                        { 4, 7, { { 1, 2, 3 } } }
                    } },
                },

                { L"YearViewPanel", L"YearViewScrollViewer", xaml_controls::CalendarViewDisplayMode::Year,
                { {
                            // dimension 4 x 4, irregular snap point (per scope).
                        { 4, 4, { { 3, 6, 9 } } },
                        // dimension 2 x 4, regular snap point (per row).
                        { 2, 4, { { 1, 2, 3 } } }
                    } }
                },

                { L"DecadeViewPanel", L"DecadeViewScrollViewer", xaml_controls::CalendarViewDisplayMode::Decade,
                { {
                            // dimension 4 x 4, irregular snap point (per scope).
                        { 4, 4, { { 2, 5, 7 } } },
                        // dimension 2 x 4, regular snap point (per row).
                        { 2, 4, { { 1, 2, 3 } } }
                    } }
                }
            } };

        for (auto mode : modes)
        {
            for (auto testData : mode.testData)
            {
                TestCleanupWrapper cleanup;

                xaml_controls::Grid^ rootPanel = nullptr;
                xaml_controls::Primitives::CalendarPanel^ calendarPanel = nullptr;
                xaml_controls::ScrollViewer^ scrollViewer = nullptr;
                CalendarHelper::CalendarViewHelper helper;

                xaml_controls::CalendarView^ cv = helper.GetCalendarView();

                CalendarHelper::CreateTestResources(rootPanel);

                // load into visual tree
                RunOnUIThread([&]()
                {
                    cv->MinDate = ConvertToDateTime(1, 2000, 1, 1);
                    cv->MaxDate = ConvertToDateTime(1, 2300, 12, 31);
                    cv->SetDisplayDate(cv->MinDate);
                    rootPanel->Children->Append(cv);
                });

                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    cv->DisplayMode = mode.displayMode;
                    cv->UpdateLayout();
                    scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(helper.GetTemplateChild(mode.scrollViewerName));
                    calendarPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(mode.panelName));
                    // hack the snap point type.
                    scrollViewer->VerticalSnapPointsType = xaml_controls::SnapPointsType::MandatorySingle;

                    // setup dimension
                    if (mode.displayMode == xaml_controls::CalendarViewDisplayMode::Month)
                    {
                        cv->NumberOfWeeksInView = testData.row; // for month mode, just simply ignore the col (because it is hardcoded to 7)
                    }
                    else
                    {
                        cv->SetYearDecadeDisplayDimensions(testData.col, testData.row);
                    }
                });

                LOG_OUTPUT(L"Mode: %d (0 - Month, 1 - Year, 2 - Decade.), dimension %d (col) x %d (row).", mode.displayMode, testData.col, testData.row);

                TestServices::WindowHelper->WaitForIdle();

                auto viewChangedEvent = std::make_shared<Event>();
                auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

                viewChangedRegistration.Attach(scrollViewer,
                    ref new ::Windows::Foundation::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                    [viewChangedEvent](Platform::Object ^sender, xaml_controls::ScrollViewerViewChangedEventArgs ^e)
                {
                    if (!e->IsIntermediate)
                    {
                        viewChangedEvent->Set();
                    }
                }));

                double itemHeight = 0;  // we need the item height to determine which row the viewport snaps to.
                double itemTopMargin = 0;

                RunOnUIThread([&]()
                {
                    itemHeight = safe_cast<Microsoft::UI::Xaml::FrameworkElement^>(calendarPanel->Children->GetAt(0))->ActualHeight;
                    auto margin = safe_cast<Microsoft::UI::Xaml::FrameworkElement^>(calendarPanel->Children->GetAt(0))->Margin;
                    itemHeight += margin.Top + margin.Bottom;
                    itemTopMargin = margin.Top;
                });


                for (unsigned i = 0; i < testData.snapPoints.size(); i++)
                {
                    LOG_OUTPUT(L"scroll down to next snap point and wait for viewchanged event.");
                    TestServices::InputHelper->ScrollMouseWheel(cv, -1);

                    viewChangedEvent->WaitForDefault();
                    viewChangedEvent->Reset();

                    // check the relative distance between Panel and ScrollViewer, divide this distance by the itemHeight to get the
                    // row that we snapped to.
                    RunOnUIThread([&]()
                    {
                        auto distance = scrollViewer->TransformToVisual(calendarPanel)->TransformPoint(::Windows::Foundation::Point(0, 0));
                        distance.Y -= static_cast<float>(itemTopMargin);
                        LOG_OUTPUT(L"actual position %f, expected %f", distance.Y, itemHeight * testData.snapPoints[i]);
                        VERIFY_IS_TRUE(CalendarHelper::AreClose(distance.Y / testData.snapPoints[i], itemHeight, 1.0));
                    });
                }
            }
        }
    }

    void CalendarViewIntegrationTests::CanChangeDisplayModeBeforeLoaded()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;
        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            // change display mode before loaded.
            cv->DisplayMode = xaml_controls::CalendarViewDisplayMode::Year;
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            xaml_controls::Grid^ monthViewGrid = safe_cast<xaml_controls::Grid^>(helper.GetTemplateChild(L"MonthView"));
            xaml_controls::ScrollViewer^ yearScrollViewer = safe_cast<xaml_controls::ScrollViewer^>(helper.GetTemplateChild(L"YearViewScrollViewer"));

            VERIFY_IS_NOT_NULL(monthViewGrid);
            VERIFY_ARE_EQUAL(monthViewGrid->Opacity, 0);

            VERIFY_IS_NOT_NULL(yearScrollViewer);
            VERIFY_ARE_EQUAL(yearScrollViewer->Visibility, Microsoft::UI::Xaml::Visibility::Visible);
            VERIFY_ARE_EQUAL(yearScrollViewer->Opacity, 1.0);
        });
    }

    void CalendarViewIntegrationTests::VerifyTransitionAnimation()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;
        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();


        xaml_animation::Storyboard^ monthToYearTransitionStoryboard = nullptr;
        RunOnUIThread([&]()
        {
            // find the root border and find the Month to Year transition.
            auto rootBorder = safe_cast<xaml_controls::Border^>(xaml_media::VisualTreeHelper::GetChild(cv, 0));
            VERIFY_IS_NOT_NULL(rootBorder);
            auto vsgs = VisualStateManager::GetVisualStateGroups(rootBorder);

            VisualTransition^ monthToYearTransition = nullptr;

            for (auto vsg : vsgs)
            {
                if (vsg->Name == L"DisplayModeStates")
                {
                    for (auto transition : vsg->Transitions)
                    {
                        if (transition->From == L"Month" && transition->To == L"Year")
                        {
                            monthToYearTransition = transition;
                            break;
                        }
                    }
                    if (monthToYearTransition != nullptr)
                    {
                        break;
                    }
                }
            }

            VERIFY_IS_NOT_NULL(monthToYearTransition);
            monthToYearTransitionStoryboard = monthToYearTransition->Storyboard;
            VERIFY_IS_NOT_NULL(monthToYearTransitionStoryboard);
        });

        auto storyboardCompletedEvent = std::make_shared<Event>();
        auto storyboardCompletedRegistration = CreateSafeEventRegistration(xaml_animation::Storyboard, Completed);

        storyboardCompletedRegistration.Attach(monthToYearTransitionStoryboard,
            ref new ::Windows::Foundation::EventHandler<Platform::Object ^>(
            [storyboardCompletedEvent](Platform::Object ^sender, Platform::Object ^e)
        {
            storyboardCompletedEvent->Set();
        }));

        RunOnUIThread([&]()
        {
            cv->DisplayMode = xaml_controls::CalendarViewDisplayMode::Year;
        });

        storyboardCompletedEvent->WaitForDefault();
        VERIFY_IS_TRUE(storyboardCompletedEvent->HasFired());

        RunOnUIThread([&]()
        {
            xaml_controls::Grid^ monthViewGrid = safe_cast<xaml_controls::Grid^>(helper.GetTemplateChild(L"MonthView"));
            xaml_controls::ScrollViewer^ yearScrollViewer = safe_cast<xaml_controls::ScrollViewer^>(helper.GetTemplateChild(L"YearViewScrollViewer"));

            VERIFY_IS_NOT_NULL(monthViewGrid);
            VERIFY_ARE_EQUAL(monthViewGrid->Opacity, 0);

            VERIFY_IS_NOT_NULL(yearScrollViewer);
            VERIFY_ARE_EQUAL(yearScrollViewer->Visibility, Microsoft::UI::Xaml::Visibility::Visible);
            VERIFY_ARE_EQUAL(yearScrollViewer->Opacity, 1.0);

        });
    }

    void CalendarViewIntegrationTests::VerifyHeaderTransitionAnimation()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;
        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        xaml_animation::Storyboard^ headerTransitionStoryboard = nullptr;
        RunOnUIThread([&]()
        {
            // find the root border and find the Month to Year transition.
            auto rootBorder = safe_cast<xaml_controls::Border^>(xaml_media::VisualTreeHelper::GetChild(cv, 0));
            VERIFY_IS_NOT_NULL(rootBorder);
            auto vsgs = VisualStateManager::GetVisualStateGroups(rootBorder);

            VisualState^ changingState = nullptr;

            for (auto vsg : vsgs)
            {
                if (vsg->Name == L"HeaderButtonStates")
                {
                    for (auto state : vsg->States)
                    {
                        if (state->Name == L"ViewChanging")
                        {
                            changingState = state;
                            break;
                        }
                    }
                    if (changingState != nullptr)
                    {
                        break;
                    }
                }
            }

            VERIFY_IS_NOT_NULL(changingState);
            headerTransitionStoryboard = changingState->Storyboard;
            VERIFY_IS_NOT_NULL(headerTransitionStoryboard);
        });

        auto storyboardCompletedEvent = std::make_shared<Event>();
        auto storyboardCompletedRegistration = CreateSafeEventRegistration(xaml_animation::Storyboard, Completed);

        storyboardCompletedRegistration.Attach(headerTransitionStoryboard,
            ref new ::Windows::Foundation::EventHandler<Platform::Object ^>(
            [storyboardCompletedEvent](Platform::Object ^sender, Platform::Object ^e)
        {
            storyboardCompletedEvent->Set();
        }));

        RunOnUIThread([&]()
        {
            // switching view will trigger transition animation on Header.
            cv->DisplayMode = xaml_controls::CalendarViewDisplayMode::Year;
        });
        storyboardCompletedEvent->WaitForDefault();
        VERIFY_IS_TRUE(storyboardCompletedEvent->HasFired());
    }

    void CalendarViewIntegrationTests::CanChangeLanguage()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();


        RunOnUIThread([&]()
        {
            // We should check the item text after we changed the language.
            // However the item text is chrome and it doesn't exist in visual tree
            // so we can't validate item text. Instead we can validate the weekday names,
            // which is also effected by languages.

            xaml_controls::Grid^ weekDayNamesGrid = safe_cast<xaml_controls::Grid^>(helper.GetTemplateChild(L"WeekDayNames"));
            xaml_controls::TextBlock^ firstWeekDay = safe_cast<xaml_controls::TextBlock^>(weekDayNamesGrid->Children->GetAt(0));
            VERIFY_IS_TRUE(firstWeekDay->Text == L"Su");

            cv->Language = L"zh-CN";
            LOG_OUTPUT(L"Change languages to Chinese.");
            cv->UpdateLayout();

            // U+65E5 is the Chinese character for Sunday.
            VERIFY_IS_TRUE(firstWeekDay->Text == L"\u65E5");
        });

    }

    void CalendarViewIntegrationTests::ValidateDCompTreeWithPointerOverNavigationButton()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        TestCleanupWrapper cleanup;

        xaml_controls::StackPanel^ rootPanel = nullptr;
        xaml_controls::Button^ nextButton = nullptr;
        xaml_controls::CalendarView^ calendarView = nullptr;
        xaml_controls::Button^ button = nullptr;

        auto pointerEnteredEvent = std::make_shared<Event>();
        auto pointerEnteredRegistration = CreateSafeEventRegistration(xaml_controls::Button, PointerEntered);

        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel  Width="400" Height="400" xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
                     HorizontalAlignment="Center" VerticalAlignment="Center">
                        <Button x:Name="button" Content="Discard Button" Margin="20,40,20,0"/>
                        <CalendarView x:Name="calendarview"/>
                </StackPanel>)"
            ));

            calendarView = safe_cast<xaml_controls::CalendarView^>(rootPanel->FindName(L"calendarview"));

            button = safe_cast<xaml_controls::Button^>(rootPanel->FindName("button"));
            VERIFY_IS_NOT_NULL(button);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            nextButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(calendarView, L"NextButton"));
            VERIFY_IS_NOT_NULL(nextButton);
        });
        pointerEnteredRegistration.Attach(nextButton, [&]() { pointerEnteredEvent->Set(); });

        TestServices::WindowHelper->WaitForIdle();
        LOG_OUTPUT(L"Make sure mouse pointer is away from CalendarView...");
        TestServices::InputHelper->MoveMouse(button);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Move mouse to nextButton.");
        TestServices::InputHelper->MoveMouse(nextButton);
        pointerEnteredEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Validate the dark theme of the overlay.");
        RunOnUIThread([&]()
        {
            rootPanel->RequestedTheme = xaml::ElementTheme::Dark;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "Dark");

        LOG_OUTPUT(L"Validate the light theme of the overlay.");
        RunOnUIThread([&]()
        {
            rootPanel->RequestedTheme = xaml::ElementTheme::Light;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "Light");

        LOG_OUTPUT(L"Validate the high-contrast theme of the overlay.");
        RunOnUIThread([&]()
        {
            TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::Test;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "HC");
    }

    void CalendarViewIntegrationTests::ValidateDCompTree()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            cv->MinDate = ConvertToDateTime(1, 2014, 1, 1);
            cv->MaxDate = ConvertToDateTime(1, 2014, 1, 1);
            cv->SelectedDates->Append(ConvertToDateTime(1, 2014, 1, 1));
            CalendarHelper::ReplaceAccentColorForTesting(cv);
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Validate dark theme");
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "dark");

        RunOnUIThread([&]()
        {
            cv->DisplayMode = xaml_controls::CalendarViewDisplayMode::Year;
        });

        LOG_OUTPUT(L"Validate Year mode");
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "year");

        RunOnUIThread([&]()
        {
            cv->DisplayMode = xaml_controls::CalendarViewDisplayMode::Decade;
        });

        LOG_OUTPUT(L"Validate decade mode");
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "decade");

        std::array<Platform::String^, 9> calendarIdentifiers{ {
            L"PersianCalendar",
            //L"GregorianCalendar", // skip this. this is the default calendar and we've verified already.
            L"HebrewCalendar",
            L"HijriCalendar",
            L"JapaneseCalendar",
            L"JulianCalendar",
            L"KoreanCalendar",
            L"TaiwanCalendar",
            L"ThaiCalendar",
            L"UmAlQuraCalendar"
        } };

        RunOnUIThread([&]()
        {
            cv->DisplayMode = xaml_controls::CalendarViewDisplayMode::Month;
        });

        for (auto& cid : calendarIdentifiers)
        {
            RunOnUIThread([&]()
            {
                cv->CalendarIdentifier = cid;
            });
            TestServices::WindowHelper->WaitForIdle();
            wchar_t masterFileName[3];
            masterFileName[0] = cid->Data()[0];
            masterFileName[1] = cid->Data()[1];
            masterFileName[2] = L'\0';
            LOG_OUTPUT(L"Validate %s", cid->Data());

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(masterFileName));
        }

        RunOnUIThread([&]()
        {
            cv->DisplayMode = xaml_controls::CalendarViewDisplayMode::Month;
            cv->IsEnabled = false;
        });

        LOG_OUTPUT(L"Validate disabled + month mode");
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "disabled");

        /* Light Themed testing blocked
        //Change theme, validate again.
        RunOnUIThread([&]()
        {
            cv->RequestedTheme = xaml::ElementTheme::Light;
        });

        LOG_OUTPUT(L"Validate light theme");
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "light");
        */
    }

    void CalendarViewIntegrationTests::ValidateDCompTreeWithCompositionBrush()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        CalendarHelper::CalendarViewHelper helper;
        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::CalendarView^ cv = helper.GetCalendarView();
        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            cv->MinDate = ConvertToDateTime(1, 2014, 1, 1);
            cv->MaxDate = ConvertToDateTime(1, 2014, 1, 1);
            cv->SelectedDates->Append(ConvertToDateTime(1, 2014, 1, 1));

            cv->CalendarItemBorderBrush = ref new XcbPurpleBrush();
            cv->CalendarItemBackground = ref new XcbPurpleBrush();
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }

    void CalendarViewIntegrationTests::ValidateDCompTreeWithLinearGradientBrush()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        CalendarHelper::CalendarViewHelper helper;
        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::CalendarView^ cv = helper.GetCalendarView();
        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            cv->MinDate = ConvertToDateTime(1, 2014, 1, 1);
            cv->MaxDate = ConvertToDateTime(1, 2014, 1, 1);
            cv->SelectedDates->Append(ConvertToDateTime(1, 2014, 1, 1));

            cv->CalendarItemBorderBrush = MakeLinearGradientBrush(0.0f, 1.0f);
            cv->CalendarItemBackground = MakeLinearGradientBrush(0.0f, 1.0f);
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }

    void CalendarViewIntegrationTests::ValidateUIElementTree()
    {
        ControlHelper::ValidateUIElementTree(
            wf::Size(400, 600),
            1.f,
            // Test setup.
            []()
            {
                CalendarHelper::CalendarViewHelper helper;
                xaml_controls::CalendarView^ cv = helper.GetCalendarView();

                xaml_controls::Grid^ root = nullptr;
                CalendarHelper::CreateTestResources(root);

                // load into visual tree
                RunOnUIThread([&]()
                {
                    cv->MinDate = ConvertToDateTime(1, 2010, 1, 1);
                    cv->MaxDate = ConvertToDateTime(1, 2014, 1, 1);
                    root->Children->Append(cv);
                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    // In test environment sometime the CalendarView doesn't have focus.
                    // To get a consistent result, we move the focus to it forcedly.
                    cv->Focus(Microsoft::UI::Xaml::FocusState::Pointer);
                });
                TestServices::WindowHelper->WaitForIdle();

                return root;
            });
    }

    void CalendarViewIntegrationTests::ChangeStylePropsDynamically()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();


        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            // make sure no virutalization, or DComp comparison will fail.
            cv->MinDate = ConvertToDateTime(1, 2014, 1, 1);
            cv->MaxDate = ConvertToDateTime(1, 2014, 1, 1);
            cv->SelectedDates->Append(ConvertToDateTime(1, 2014, 1, 1));
            CalendarHelper::ReplaceAccentColorForTesting(cv);
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            cv->SelectedForeground = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
            cv->BorderThickness = ThicknessHelper::FromUniformLength(3);
            cv->HorizontalDayItemAlignment = xaml::HorizontalAlignment::Left;
            cv->HorizontalFirstOfMonthLabelAlignment = xaml::HorizontalAlignment::Right;
            cv->CalendarItemBorderBrush = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Yellow);
            cv->CalendarViewDayItemStyle = dynamic_cast<xaml::Style^>(xaml_markup::XamlReader::Load(
                L"<Style xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' TargetType='CalendarViewDayItem'>"
                L"  <Setter Property='MinWidth' Value='50' />"
                L"  <Setter Property='MinHeight' Value='50' />"
                // CalendarView: change dayitem's template will cause day text disappear
                // now changing the template will not remove the chrom textblock from visual tree.
                L"  <Setter Property='Template'>"
                L"    <Setter.Value>"
                L"      <ControlTemplate TargetType='CalendarViewDayItem'/>"
                L"    </Setter.Value>"
                L"  </Setter>"
                L"</Style>"
                ));
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }

    void CalendarViewIntegrationTests::ValidateScopeChange()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            // make sure no virtualization, or DComp comparison will fail.
            cv->MinDate = ConvertToDateTime(1, 2014, 9, 27);
            cv->MaxDate = ConvertToDateTime(1, 2014, 10, 20);
            CalendarHelper::ReplaceAccentColorForTesting(cv);
            rootPanel->Children->Append(cv);
        });


        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }

    void CalendarViewIntegrationTests::CanSetDayItemStyle()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();


        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            // make sure no virutalization, or DComp comparison will fail.
            cv->MinDate = ConvertToDateTime(1, 2014, 1, 1);
            cv->MaxDate = ConvertToDateTime(1, 2014, 1, 1);
            cv->SelectedDates->Append(ConvertToDateTime(1, 2014, 1, 1));
            CalendarHelper::ReplaceAccentColorForTesting(cv);

            cv->CalendarViewDayItemStyle = dynamic_cast<xaml::Style^>(xaml_markup::XamlReader::Load(
                L"<Style xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' TargetType='CalendarViewDayItem'>"
                L"  <Setter Property='MinWidth' Value='50' />"
                L"  <Setter Property='MinHeight' Value='50' />"
                L"</Style>"
                ));

            rootPanel->Children->Append(cv);
        });


        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }

    void VerifyChangingCalendarIdentifier(xaml_controls::CalendarViewDisplayMode displayMode)
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Grid^ rootPanel = nullptr;
        CalendarHelper::CalendarViewHelper helper;
        xaml_controls::CalendarView^ cv = helper.GetCalendarView();
        CalendarHelper::CreateTestResources(rootPanel);

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        auto cids = GetAllSupportedCalendarIdentifiers();

        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cv);
        });

        LOG_OUTPUT(L"  Change DisplayMode to %s", displayMode.ToString()->Data());

        RunOnUIThread([&]()
        {
            cv->DisplayMode = displayMode;
        });

        TestServices::WindowHelper->WaitForIdle();

        for (auto& cid : cids)
        {
            LOG_OUTPUT(L"Begin Testing CalendarIdentifier to %s", cid->Data());
            // change calendar identifier
            RunOnUIThread([&]()
            {
                cv->CalendarIdentifier = cid;
            });

            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"End Testing CalendarIdentifier to %s", cid->Data());
        }

    }

    void CalendarViewIntegrationTests::CanChangeMonthCalendarIdentifier()
    {
        VerifyChangingCalendarIdentifier(xaml_controls::CalendarViewDisplayMode::Month);
    }

    void CalendarViewIntegrationTests::CanChangeYearCalendarIdentifier()
    {
        VerifyChangingCalendarIdentifier(xaml_controls::CalendarViewDisplayMode::Year);
    }

    void CalendarViewIntegrationTests::CanChangeDecadeCalendarIdentifier()
    {
        VerifyChangingCalendarIdentifier(xaml_controls::CalendarViewDisplayMode::Decade);
    }

    void VerifyBoundaries(Platform::String^ cid, Platform::String^ panelName, xaml_controls::CalendarViewDisplayMode displayMode)
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Grid^ rootPanel = nullptr;
        CalendarHelper::CalendarViewHelper helper;
        xaml_controls::CalendarView^ cv = helper.GetCalendarView();
        CalendarHelper::CreateTestResources(rootPanel);
        auto defaultCalendar = ref new ::Windows::Globalization::Calendar();
        auto today = defaultCalendar->GetDateTime();

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        LOG_OUTPUT(L"Begin Testing CalendarIdentifier to %s", cid->Data());

        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cv);
            // set MinDate/MaxDate to the maximium range that calendar can support.
            cv->CalendarIdentifier = cid;
            auto cal = ref new ::Windows::Globalization::Calendar(defaultCalendar->Languages, cid, defaultCalendar->GetClock());
            cal->SetToMin();
            cv->MinDate = cal->GetDateTime();
            cal->SetToMax();
            cv->MaxDate = cal->GetDateTime();
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"  Change DisplayMode to %s", displayMode.ToString()->Data());
        RunOnUIThread([&]()
        {
            cv->DisplayMode = displayMode;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // move focus to the items so we can test keyboard navigations.
            auto calendarPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(panelName));
            auto firstItem = safe_cast<xaml_controls::Control^>(calendarPanel->Children->GetAt(0));
            firstItem->Focus(Microsoft::UI::Xaml::FocusState::Programmatic);

            cv->SetDisplayDate(cv->MinDate);
        });

        LOG_OUTPUT(L"    Jump to the beginning");
        TestServices::WindowHelper->WaitForIdle();

        // at the beginning, we try to cross the boundary.
        // press Down once, Up twice,
        // press Right once, Left twice,
        // press PgDn once, PgUp twice

        TestServices::KeyboardHelper->Down();
        TestServices::KeyboardHelper->Up();
        TestServices::KeyboardHelper->Up();
        TestServices::WindowHelper->WaitForIdle();
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_right#$u$_right#$d$_left#$u$_left#$d$_left#$u$_left");
        TestServices::WindowHelper->WaitForIdle();
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_pagedown#$u$_pagedown#$d$_pageup#$u$_pageup#$d$_pageup#$u$_pageup");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            cv->SetDisplayDate(today);
        });

        LOG_OUTPUT(L"    Jump to today");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            cv->SetDisplayDate(cv->MaxDate);
        });

        LOG_OUTPUT(L"    Jump to the end");
        TestServices::WindowHelper->WaitForIdle();

        // at the end, we try to cross the boundary.
        // press Up once, Down twice,
        // press Left once, Right twice,
        // press PgUp once, PgDown twice

        TestServices::KeyboardHelper->Up();
        TestServices::KeyboardHelper->Down();
        TestServices::KeyboardHelper->Down();
        TestServices::WindowHelper->WaitForIdle();
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_left#$u$_left#$d$_right#$u$_right#$d$_right#$u$_right");
        TestServices::WindowHelper->WaitForIdle();
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_pageup#$u$_pageup#$d$_pagedown#$u$_pagedown#$d$_pagedown#$u$_pagedown");
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"End Testing CalendarIdentifier to %s", cid->Data());
    }

    void CalendarViewIntegrationTests::VerifyPersianCalendarMonthBoundaries()
    {
        VerifyBoundaries("PersianCalendar", "MonthViewPanel", xaml_controls::CalendarViewDisplayMode::Month);
    }

    void CalendarViewIntegrationTests::VerifyPersianCalendarYearBoundaries()
    {
        VerifyBoundaries("PersianCalendar", "YearViewPanel", xaml_controls::CalendarViewDisplayMode::Year);
    }

    void CalendarViewIntegrationTests::VerifyPersianCalendarDecadeBoundaries()
    {
        VerifyBoundaries("PersianCalendar", "DecadeViewPanel", xaml_controls::CalendarViewDisplayMode::Decade);
    }

    void CalendarViewIntegrationTests::VerifyGregorianCalendarMonthBoundaries()
    {
        VerifyBoundaries("GregorianCalendar", "MonthViewPanel", xaml_controls::CalendarViewDisplayMode::Month);
    }

    void CalendarViewIntegrationTests::VerifyGregorianCalendarYearBoundaries()
    {
        VerifyBoundaries("GregorianCalendar", "YearViewPanel", xaml_controls::CalendarViewDisplayMode::Year);
    }

    void CalendarViewIntegrationTests::VerifyGregorianCalendarDecadeBoundaries()
    {
        VerifyBoundaries("GregorianCalendar", "DecadeViewPanel", xaml_controls::CalendarViewDisplayMode::Decade);
    }

    void CalendarViewIntegrationTests::VerifyHebrewCalendarMonthBoundaries()
    {
        VerifyBoundaries("HebrewCalendar", "MonthViewPanel", xaml_controls::CalendarViewDisplayMode::Month);
    }

    void CalendarViewIntegrationTests::VerifyHebrewCalendarYearBoundaries()
    {
        VerifyBoundaries("HebrewCalendar", "YearViewPanel", xaml_controls::CalendarViewDisplayMode::Year);
    }

    void CalendarViewIntegrationTests::VerifyHebrewCalendarDecadeBoundaries()
    {
        VerifyBoundaries("HebrewCalendar", "DecadeViewPanel", xaml_controls::CalendarViewDisplayMode::Decade);
    }

    void CalendarViewIntegrationTests::VerifyHijriCalendarMonthBoundaries()
    {
        VerifyBoundaries("HijriCalendar", "MonthViewPanel", xaml_controls::CalendarViewDisplayMode::Month);
    }

    void CalendarViewIntegrationTests::VerifyHijriCalendarYearBoundaries()
    {
        VerifyBoundaries("HijriCalendar", "YearViewPanel", xaml_controls::CalendarViewDisplayMode::Year);
    }

    void CalendarViewIntegrationTests::VerifyHijriCalendarDecadeBoundaries()
    {
        VerifyBoundaries("HijriCalendar", "DecadeViewPanel", xaml_controls::CalendarViewDisplayMode::Decade);
    }

    void CalendarViewIntegrationTests::VerifyJapaneseCalendarMonthBoundaries()
    {
        VerifyBoundaries("JapaneseCalendar", "MonthViewPanel", xaml_controls::CalendarViewDisplayMode::Month);
    }

    void CalendarViewIntegrationTests::VerifyJapaneseCalendarYearBoundaries()
    {
        VerifyBoundaries("JapaneseCalendar", "YearViewPanel", xaml_controls::CalendarViewDisplayMode::Year);
    }

    void CalendarViewIntegrationTests::VerifyJapaneseCalendarDecadeBoundaries()
    {
        VerifyBoundaries("JapaneseCalendar", "DecadeViewPanel", xaml_controls::CalendarViewDisplayMode::Decade);
    }

    void CalendarViewIntegrationTests::VerifyJulianCalendarMonthBoundaries()
    {
        VerifyBoundaries("JulianCalendar", "MonthViewPanel", xaml_controls::CalendarViewDisplayMode::Month);
    }

    void CalendarViewIntegrationTests::VerifyJulianCalendarYearBoundaries()
    {
        VerifyBoundaries("JulianCalendar", "YearViewPanel", xaml_controls::CalendarViewDisplayMode::Year);
    }

    void CalendarViewIntegrationTests::VerifyJulianCalendarDecadeBoundaries()
    {
        VerifyBoundaries("JulianCalendar", "DecadeViewPanel", xaml_controls::CalendarViewDisplayMode::Decade);
    }

    void CalendarViewIntegrationTests::VerifyKoreanCalendarMonthBoundaries()
    {
        VerifyBoundaries("KoreanCalendar", "MonthViewPanel", xaml_controls::CalendarViewDisplayMode::Month);
    }

    void CalendarViewIntegrationTests::VerifyKoreanCalendarYearBoundaries()
    {
        VerifyBoundaries("KoreanCalendar", "YearViewPanel", xaml_controls::CalendarViewDisplayMode::Year);
    }

    void CalendarViewIntegrationTests::VerifyKoreanCalendarDecadeBoundaries()
    {
        VerifyBoundaries("KoreanCalendar", "DecadeViewPanel", xaml_controls::CalendarViewDisplayMode::Decade);
    }

    void CalendarViewIntegrationTests::VerifyTaiwanCalendarMonthBoundaries()
    {
        VerifyBoundaries("TaiwanCalendar", "MonthViewPanel", xaml_controls::CalendarViewDisplayMode::Month);
    }

    void CalendarViewIntegrationTests::VerifyTaiwanCalendarYearBoundaries()
    {
        VerifyBoundaries("TaiwanCalendar", "YearViewPanel", xaml_controls::CalendarViewDisplayMode::Year);
    }

    void CalendarViewIntegrationTests::VerifyTaiwanCalendarDecadeBoundaries()
    {
        VerifyBoundaries("TaiwanCalendar", "DecadeViewPanel", xaml_controls::CalendarViewDisplayMode::Decade);
    }

    void CalendarViewIntegrationTests::VerifyThaiCalendarMonthBoundaries()
    {
        VerifyBoundaries("ThaiCalendar", "MonthViewPanel", xaml_controls::CalendarViewDisplayMode::Month);
    }

    void CalendarViewIntegrationTests::VerifyThaiCalendarYearBoundaries()
    {
        VerifyBoundaries("ThaiCalendar", "YearViewPanel", xaml_controls::CalendarViewDisplayMode::Year);
    }

    void CalendarViewIntegrationTests::VerifyThaiCalendarDecadeBoundaries()
    {
        VerifyBoundaries("ThaiCalendar", "DecadeViewPanel", xaml_controls::CalendarViewDisplayMode::Decade);
    }

    void CalendarViewIntegrationTests::VerifyUmAlQuraCalendarMonthBoundaries()
    {
        VerifyBoundaries("UmAlQuraCalendar", "MonthViewPanel", xaml_controls::CalendarViewDisplayMode::Month);
    }

    void CalendarViewIntegrationTests::VerifyUmAlQuraCalendarYearBoundaries()
    {
        VerifyBoundaries("UmAlQuraCalendar", "YearViewPanel", xaml_controls::CalendarViewDisplayMode::Year);
    }

    void CalendarViewIntegrationTests::VerifyUmAlQuraCalendarDecadeBoundaries()
    {
        VerifyBoundaries("UmAlQuraCalendar", "DecadeViewPanel", xaml_controls::CalendarViewDisplayMode::Decade);
    }

    void CalendarViewIntegrationTests::ValidateScopeStateAfterLoaded()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            cv->MinDate = ConvertToDateTime(1, 2013, 5, 18);
            cv->MaxDate = ConvertToDateTime(1, 2013, 6, 23);
            CalendarHelper::ReplaceAccentColorForTesting(cv);
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }

    void CalendarViewIntegrationTests::ValidateScopeInManipulation()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            cv->MinDate = ConvertToDateTime(1, 2013, 1, 1);
            cv->MaxDate = ConvertToDateTime(1, 2013, 12, 31);
            CalendarHelper::ReplaceAccentColorForTesting(cv);
            cv->SetDisplayDate(cv->MinDate);
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        xaml_controls::CalendarViewDayItem^ firstItem = nullptr;

        RunOnUIThread([&]()
        {
            auto calendarPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(L"MonthViewPanel"));
            firstItem = safe_cast<xaml_controls::CalendarViewDayItem^>(calendarPanel->Children->GetAt(0));
        });

        TestServices::InputHelper->DynamicPressCenter(firstItem, 0, 0, PointerFinger::Finger1);

        // Finger down, we don't change OutOfScope state
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

        // it is not easy to validate OutOfScope state during manipulation because OutOfScope state must be validated via DComp Comparison,
        // however during manipulation DComp comparison is not predictable.
        TestServices::InputHelper->DynamicRelease(PointerFinger::Finger1);
    }


    void CalendarViewIntegrationTests::ValidateScopeWithDST()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            cv->MinDate = ConvertToDateTime(1, 2014, 3, 8);
            cv->MaxDate = ConvertToDateTime(1, 2014, 4, 1);
            CalendarHelper::ReplaceAccentColorForTesting(cv);
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        // before the fix, 3/31/2014 is in OutOfScope state (gray by default).
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }


    void CalendarViewIntegrationTests::CanDisplayDateOnCorrectPositionWhenCallSetDisplayDate()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Primitives::CalendarPanel^ calendarPanel = nullptr;
        xaml_controls::ScrollViewer^ scrollViewer = nullptr;
        xaml_controls::CalendarViewDayItem^ firstItem = nullptr;
        xaml_controls::Grid^ rootPanel = nullptr;
        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        CalendarHelper::CreateTestResources(rootPanel);

        auto minDate = ConvertToDateTime(1, 2000, 1, 1);
        auto maxDate = ConvertToDateTime(1, 2010, 1, 1);

        // load into visual tree
        RunOnUIThread([&]()
        {
            cv->MinDate = minDate;
            cv->MaxDate = maxDate;
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            calendarPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(L"MonthViewPanel"));
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(helper.GetTemplateChild(L"MonthViewScrollViewer"));
            firstItem = safe_cast<xaml_controls::CalendarViewDayItem^>(calendarPanel->Children->GetAt(0));
        });

        // First we have 6 rows in month view so call displaydate will bring 1/1/2000 to the first row
        // hence the panel's offset is 0
        RunOnUIThread([&]()
        {
            cv->SetDisplayDate(ConvertToDateTime(1, 2000, 1, 20));
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            auto distance = scrollViewer->TransformToVisual(calendarPanel)->TransformPoint(::Windows::Foundation::Point(0, 0));
            VERIFY_IS_TRUE(CalendarHelper::AreClose(distance.Y, 0., 1.0));
        });

        // Second we set numberof weeks to 4, call displaydate will bring 1/20/2000 to the first row
        // hence the panel's offset is (-3) * itemHeight. 1/20/2000 is on the third row.
        RunOnUIThread([&]()
        {
            cv->NumberOfWeeksInView = 4;
            cv->SetDisplayDate(ConvertToDateTime(1, 2000, 1, 20));
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            auto distance = scrollViewer->TransformToVisual(calendarPanel)->TransformPoint(::Windows::Foundation::Point(0, 0));
            auto itemHeight = firstItem->ActualHeight;
            auto itemMargin = firstItem->Margin;
            itemHeight += itemMargin.Top + itemMargin.Bottom;
            VERIFY_IS_TRUE(CalendarHelper::AreClose(distance.Y, itemHeight * 3, 1.0));
        });
    }

    void CalendarViewIntegrationTests::CanDisplayDateOnCorrectPositionWhenSwitchView()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Primitives::CalendarPanel^ monthPanel = nullptr;
        xaml_controls::ScrollViewer^ monthScrollViewer = nullptr;
        xaml_controls::CalendarViewDayItem^ firstItem = nullptr;
        xaml_controls::CalendarViewDayItem^ itemFeb20 = nullptr;
        xaml_controls::Primitives::CalendarPanel^ yearPanel = nullptr;
        xaml_controls::Control^ firstYearItem = nullptr;
        xaml_controls::Grid^ rootPanel = nullptr;
        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        CalendarHelper::CreateTestResources(rootPanel);

        auto minDate = ConvertToDateTime(1, 2000, 1, 1);
        auto maxDate = ConvertToDateTime(1, 2010, 1, 1);

        // load into visual tree
        RunOnUIThread([&]()
        {
            cv->MinDate = minDate;
            cv->MaxDate = maxDate;
            cv->SetDisplayDate(minDate);
            rootPanel->Children->Append(cv);
        });


        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            monthPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(L"MonthViewPanel"));
            monthScrollViewer = safe_cast<xaml_controls::ScrollViewer^>(helper.GetTemplateChild(L"MonthViewScrollViewer"));
            firstItem = safe_cast<xaml_controls::CalendarViewDayItem^>(monthPanel->Children->GetAt(0));
            itemFeb20 = safe_cast<xaml_controls::CalendarViewDayItem^>(monthPanel->Children->GetAt(50));  // 20/2/2000
        });

        // In below test we'll do:
        //  1. Display and Focus Feb20
        //  2. Switch to YearView
        //  3. Tap January 2000
        // After above steps, we'll come back to month view and Jan20 will be focused and brought into view
        struct TestData
        {
            int numberOfWeeks;
            int distanceOfRows;
        };

        std::array<TestData, 2> testData{ {
                { 6, 0 },   // When MonthView has 6 weeks, the first row ( 1/1/2000) will be visible row after above steps.
                { 4, 3 }    // When MonthView has 4 weeks, the third row (1/20/2000) will be visible row after above steps.
        } };

        for (auto& data : testData)
        {
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Setting NumberOfWeeksInView to %d.", data.numberOfWeeks);
                cv->DisplayMode = xaml_controls::CalendarViewDisplayMode::Month;
                cv->NumberOfWeeksInView = data.numberOfWeeks;
                cv->SetDisplayDate(itemFeb20->Date);
                itemFeb20->Focus(Microsoft::UI::Xaml::FocusState::Programmatic);
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Dayitem 2/20/2000 should be focused.");
                CalendarHelper::CheckFocusedItem();
                cv->DisplayMode = xaml_controls::CalendarViewDisplayMode::Year;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Switched to Year mode, yearitem Feb/2000 should be focused.");
                CalendarHelper::CheckFocusedItem();
                yearPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(L"YearViewPanel"));
                firstYearItem = safe_cast<xaml_controls::Control^>(yearPanel->Children->GetAt(0));
            });
            TestServices::InputHelper->Tap(firstYearItem);
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Tapped on the first yearitem Jan/2000, we should go back to month mode and dayitem 1/20/2000 should be focused.");
                CalendarHelper::CheckFocusedItem();
                auto distance = monthScrollViewer->TransformToVisual(monthPanel)->TransformPoint(::Windows::Foundation::Point(0, 0));
                auto itemHeight = firstItem->ActualHeight;
                auto itemMargin = firstItem->Margin;
                itemHeight += itemMargin.Top + itemMargin.Bottom;

                VERIFY_IS_TRUE(CalendarHelper::AreClose(distance.Y, itemHeight * data.distanceOfRows, 1.0));
            });
        }
    }

    void CalendarViewIntegrationTests::CanDayItemFontPropertiesAffectMeasure()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Grid^ rootPanel = nullptr;
        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();
        xaml_controls::Primitives::CalendarPanel^ monthPanel = nullptr;
        xaml_controls::CalendarViewDayItem^ dayItem = nullptr;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        CalendarHelper::CreateTestResources(rootPanel);

        auto minDate = ConvertToDateTime(1, 2000, 1, 28);
        auto maxDate = ConvertToDateTime(1, 2000, 1, 28);

        // load into visual tree
        RunOnUIThread([&]()
        {
            cv->MinDate = minDate;
            cv->MaxDate = maxDate;
            cv->SetDisplayDate(minDate);
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        // in below test we'll change the dayitem font properties and expect a change on the dayitem's actual size.
        RunOnUIThread([&]()
        {
            monthPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(L"MonthViewPanel"));
            dayItem = safe_cast<xaml_controls::CalendarViewDayItem^>(monthPanel->Children->GetAt(0));

            double actualHeight = std::round(dayItem->ActualHeight);
            double actualWidth = std::round(dayItem->ActualWidth);
            double expectedHeight = 40.0;
            double expectedWidth = 40.0;
            LOG_OUTPUT(L"Expected size: (%.0f, %.0f)", expectedWidth, expectedHeight);
            LOG_OUTPUT(L"Actual size: (%.0f, %.0f)", actualWidth, actualHeight);
            VERIFY_IS_TRUE(CalendarHelper::AreClose(actualHeight, expectedHeight));
            VERIFY_IS_TRUE(CalendarHelper::AreClose(actualWidth, expectedWidth));

            cv->DayItemFontFamily = ref new Microsoft::UI::Xaml::Media::FontFamily(L"Segoe UI Symbol");
            cv->DayItemFontSize = 50;
            cv->DayItemFontStyle = ::Windows::UI::Text::FontStyle::Italic;
            cv->DayItemFontWeight = Microsoft::UI::Text::FontWeights::ExtraBold;
            cv->UpdateLayout();

            actualHeight = std::round(dayItem->ActualHeight);
            actualWidth = std::round(dayItem->ActualWidth);
            expectedHeight = 76.0;
            expectedWidth = 58.0;
            LOG_OUTPUT(L"Expected size: (%.0f, %.0f)", expectedWidth, expectedHeight);
            LOG_OUTPUT(L"Actual size: (%.0f, %.0f)", actualWidth, actualHeight);
            VERIFY_IS_TRUE(CalendarHelper::AreClose(actualHeight, expectedHeight));
            VERIFY_IS_TRUE(CalendarHelper::AreClose(actualWidth, expectedWidth));
        });
    }

    void CalendarViewIntegrationTests::VerifyHeaderTextChangesWhenCalendarIdentifierChanged()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Grid^ rootPanel = nullptr;

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        CalendarHelper::CreateTestResources(rootPanel);

        auto minDate = ConvertToDateTime(1, 2000, 1, 28);
        auto maxDate = ConvertToDateTime(1, 2000, 1, 28);

        // load into visual tree
        RunOnUIThread([&]()
        {
            cv->MinDate = minDate;
            cv->MaxDate = maxDate;
            rootPanel->Children->Append(cv);
        });


        TestServices::WindowHelper->WaitForIdle();


        RunOnUIThread([&]()
        {
            auto headerButton = safe_cast<xaml_controls::Button^>(helper.GetTemplateChild(L"HeaderButton"));
            auto headText = safe_cast<Platform::String^>(headerButton->Content);

            LOG_OUTPUT(L"header text in Gregorian calendar: %s", headText->Data());
            VERIFY_IS_TRUE(headText == L"\u200eJanuary\u200e \u200e2000");

            cv->CalendarIdentifier = L"HebrewCalendar";
            cv->UpdateLayout();

            auto headText2 = safe_cast<Platform::String^>(headerButton->Content);
            LOG_OUTPUT(L"header text in Hebrew calendar: %s", headText2->Data());

            VERIFY_IS_TRUE(headText2 == L"\u200f\u05e9\u05d1\u05d8\u200f \u200f\u05ea\u05e9\u05f4\u05e1");
        });

    }

    void CalendarViewIntegrationTests::VerifySetDisplayDateBeforeLoaded()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        xaml_controls::Grid^ rootPanel = nullptr;

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();
        xaml_controls::CalendarViewDayItem^ firstVisibleItem = nullptr;

        CalendarHelper::CreateTestResources(rootPanel);

        auto minDate = ConvertToDateTime(1, 2000, 1, 1);
        auto maxDate = ConvertToDateTime(1, 2020, 1, 2);
        auto displayDate = ConvertToDateTime(1, 2014, 6, 1); // by default this date will be on first column.

        // in below test we are going to find the first visible item and to verify if it is displayDate.
        // however CalendarPanel doesn't expose firstVisibleIndex API, so here we'll try to workaround it.
        // we use CalendarViewDayItemChanging event to find the item with displayDate, then check if this item is
        // at (0, 0) in the visible window.
        CalendarHelper::CompareDate dateComparer;
        auto cicEvent = std::make_shared<Event>();
        auto cicRegistration = CreateSafeEventRegistration(xaml_controls::CalendarView, CalendarViewDayItemChanging);
        cicRegistration.Attach(
            cv,
            ref new ::Windows::Foundation::TypedEventHandler<xaml_controls::CalendarView^, xaml_controls::CalendarViewDayItemChangingEventArgs^>(
            [&](xaml_controls::CalendarView^ sender, xaml_controls::CalendarViewDayItemChangingEventArgs^ e)
        {
            if (dateComparer(e->Item->Date, displayDate))
            {
                firstVisibleItem = e->Item;
                cicEvent->Set();
            }

        }));

        // load into visual tree
        RunOnUIThread([&]()
        {
            cv->MinDate = minDate;
            cv->MaxDate = maxDate;
            cv->SetDisplayDate(displayDate);
            rootPanel->Children->Append(cv);
        });


        TestServices::WindowHelper->WaitForIdle();
        cicEvent->WaitForDefault();
        VERIFY_IS_TRUE(cicEvent->HasFired());
        VERIFY_IS_NOT_NULL(firstVisibleItem);

        RunOnUIThread([&]()
        {
            auto scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(helper.GetTemplateChild(L"MonthViewScrollViewer"));
            ::Windows::Foundation::Point origin = { 0, 0 };
            auto itemPos = firstVisibleItem->TransformToVisual(scrollViewer)->TransformPoint(origin);
            LOG_OUTPUT(L"actual position of display date is (%f, %f)., expected (1, 1)", itemPos.X, itemPos.Y);

            //CalendarViewDayItem's margin is {1, 1, 1, 1}
            VERIFY_ARE_EQUAL(itemPos.X, 1.);
            VERIFY_ARE_EQUAL(itemPos.Y, 1.);
        });

    }

    void CalendarViewIntegrationTests::CanFocusOnCorrectItemWithTap()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::Primitives::CalendarPanel^ monthPanel;

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        CalendarHelper::CreateTestResources(rootPanel);

        auto minDate = ConvertToDateTime(1, 2000, 1, 1);
        auto maxDate = ConvertToDateTime(1, 2000, 1, 2);

        // load into visual tree
        RunOnUIThread([&]()
        {
            cv->MinDate = minDate;
            cv->MaxDate = maxDate;
            rootPanel->Children->Append(cv);
        });


        TestServices::WindowHelper->WaitForIdle();

        xaml_controls::CalendarViewDayItem^ firstItem = nullptr;
        xaml_controls::CalendarViewDayItem^ secondItem = nullptr;
        RunOnUIThread([&]()
        {
            monthPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(L"MonthViewPanel"));
            firstItem = safe_cast<xaml_controls::CalendarViewDayItem^>(monthPanel->Children->GetAt(0));
            secondItem = safe_cast<xaml_controls::CalendarViewDayItem^>(monthPanel->Children->GetAt(1));
            VERIFY_IS_NOT_NULL(firstItem);
            VERIFY_IS_NOT_NULL(secondItem);
        });

        std::shared_ptr<Event> gotFocusEvent = std::make_shared<Event>();
        auto gotFocusRegistration = CreateSafeEventRegistration(UIElement, GotFocus);

        gotFocusRegistration.Attach(secondItem, ref new xaml::RoutedEventHandler(
            [gotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^ args)
        {
            LOG_OUTPUT(L"CalendarViewDayItem got focus");
            gotFocusEvent->Set();
        }));

        TestServices::InputHelper->Tap(secondItem);
        gotFocusEvent->WaitForDefault();
        VERIFY_IS_TRUE(gotFocusEvent->HasFired());
        RunOnUIThread([&]()
        {
            CalendarHelper::CheckFocusedItem();
            auto focusedItem = safe_cast<xaml_controls::CalendarViewDayItem^>(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            VERIFY_IS_NOT_NULL(focusedItem);
            // before this fix, it will focus on first item.
            VERIFY_ARE_EQUAL(focusedItem, secondItem);
        });


        std::shared_ptr<Event> lostFocusEvent = std::make_shared<Event>();
        auto lostFocusRegistration = CreateSafeEventRegistration(UIElement, LostFocus);

        lostFocusRegistration.Attach(secondItem, ref new xaml::RoutedEventHandler(
            [lostFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^ args)
        {
            LOG_OUTPUT(L"CalendarViewDayItem lost focus");
            lostFocusEvent->Set();
        }));

        // tap the center of Panel (no item at center), we don't focus on any item.
        TestServices::InputHelper->Tap(monthPanel);
        lostFocusEvent->WaitForDefault();
        VERIFY_IS_TRUE(lostFocusEvent->HasFired());

        RunOnUIThread([&]()
        {
            auto focusedItem = dynamic_cast<xaml_controls::CalendarViewDayItem^>(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            VERIFY_IS_NULL(focusedItem);
        });

    }

    void CalendarViewIntegrationTests::VerifyRenderLayers()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            cv->MinDate = ConvertToDateTime(1, 1999, 12, 30);
            cv->MaxDate = ConvertToDateTime(1, 2000, 1, 5);
            cv->SelectedDates->Append(ConvertToDateTime(1, 2000, 1, 1));
            cv->HorizontalDayItemAlignment = HorizontalAlignment::Right;
            cv->VerticalDayItemAlignment = VerticalAlignment::Bottom;
            cv->HorizontalFirstOfMonthLabelAlignment = HorizontalAlignment::Left;
            cv->DayItemFontSize = 25;
            cv->FirstOfMonthLabelFontSize = 20;
            cv->IsGroupLabelVisible = true;
            cv->FirstDayOfWeek = ::Windows::Globalization::DayOfWeek::Thursday;
            cv->OutOfScopeBackground = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
            cv->CalendarItemBackground= ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Blue);
            cv->SelectedBorderBrush = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::White);
            cv->CalendarViewDayItemStyle = dynamic_cast<xaml::Style^>(xaml_markup::XamlReader::Load(
                L"<Style xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' TargetType='CalendarViewDayItem'>"
                L"  <Setter Property='MinWidth' Value='50' />"
                L"  <Setter Property='MinHeight' Value='50' />"
                L"  <Setter Property='Template'>"
                L"    <Setter.Value>"
                L"      <ControlTemplate TargetType='CalendarViewDayItem'>"
                // template child will be rendered before Number and Label textblocks, regardless ZIndex
                L"        <Border Background='Blue' Width='30' Height='30' Canvas.ZIndex='1000'/>"
                L"      </ControlTemplate>"
                L"    </Setter.Value>"
                L"  </Setter>"
                L"</Style>"
                ));
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto monthPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(L"MonthViewPanel"));
            auto dayDec31 = safe_cast<xaml_controls::CalendarViewDayItem^>(monthPanel->Children->GetAt(1));
            auto dayJan1 = safe_cast<xaml_controls::CalendarViewDayItem^>(monthPanel->Children->GetAt(2));

            // half transparent green background will be drawn on below Red background
            dayDec31->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(0x80, 0, 0xFF, 0));
            dayJan1->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(0x80, 0, 0xFF, 0));

            ColorCollection^ colors = ref new ColorCollection();
            for (int i = 0; i < 8; i++)
            {
                colors->Append(Microsoft::UI::Colors::Yellow);
            }
            dayDec31->SetDensityColors(colors);
            dayJan1->SetDensityColors(colors);
        });

        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

        RunOnUIThread([&]()
        {
            // remove the template child, density bar should be still there (and only drawn once).
            cv->CalendarViewDayItemStyle = dynamic_cast<xaml::Style^>(xaml_markup::XamlReader::Load(
                L"<Style xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' TargetType='CalendarViewDayItem'>"
                L"  <Setter Property='MinWidth' Value='50' />"
                L"  <Setter Property='MinHeight' Value='50' />"
                L"  <Setter Property='Template'>"
                L"    <Setter.Value>"
                // remove the template child, the density bar should be still there.
                L"      <ControlTemplate TargetType='CalendarViewDayItem'/>"
                L"    </Setter.Value>"
                L"  </Setter>"
                L"</Style>"
                ));

        });

        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "2");
    }

    void CalendarViewIntegrationTests::VerifyDisplayDate()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Grid^ rootPanel = nullptr;

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            cv->MinDate = ConvertToDateTime(1, 2000, 1, 1);
            cv->MaxDate = ConvertToDateTime(1, 2100, 1, 1);
            cv->SetDisplayDate(ConvertToDateTime(1, 2050, 1, 1));
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // switch to Year mode directly, we'll show year 2050 and panel offset will be at -11100
            cv->DisplayMode = xaml_controls::CalendarViewDisplayMode::Year;
            cv->UpdateLayout();

            auto yearPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(L"YearViewPanel"));
            auto yearScrollViewer = safe_cast<xaml_controls::ScrollViewer^>(helper.GetTemplateChild(L"YearViewScrollViewer"));
            auto headerButton = safe_cast<xaml_controls::Button^>(helper.GetTemplateChild(L"HeaderButton"));
            auto offset = yearPanel->TransformToVisual(yearScrollViewer)->TransformPoint(::Windows::Foundation::Point(0, 0));
            auto headText = safe_cast<Platform::String^>(headerButton->Content);
            LOG_OUTPUT(L"yearpanel offset is %f, header text is %s", offset.Y, headText->Data());
            VERIFY_IS_TRUE(headText == L"\u200e2050");
            VERIFY_ARE_EQUAL(offset.Y, -11100);

            // switch back to MonthView and call display the minDate (1/1/2000)
            // then switch back to yearView, we'll see year 2000 and panel offset will be at 0.
            cv->DisplayMode = xaml_controls::CalendarViewDisplayMode::Month;
            cv->SetDisplayDate(cv->MinDate);
            cv->UpdateLayout();
            cv->DisplayMode = xaml_controls::CalendarViewDisplayMode::Year;
            cv->UpdateLayout();
            offset = yearPanel->TransformToVisual(yearScrollViewer)->TransformPoint(::Windows::Foundation::Point(0, 0));
            headText = safe_cast<Platform::String^>(headerButton->Content);
            LOG_OUTPUT(L"yearpanel offset is %f, header text is %s", offset.Y, headText->Data());
            VERIFY_IS_TRUE(headText == L"\u200e2000");
            VERIFY_ARE_EQUAL(offset.Y, 0);
        });
    }

    void CalendarViewIntegrationTests::CanChangeStyle()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Grid^ rootPanel = nullptr;

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // changing the template will not crash the app
            cv->Style = dynamic_cast<xaml::Style^>(xaml_markup::XamlReader::Load(
                L"<Style xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' TargetType='CalendarView'>"
                L"  <Setter Property='Template'>"
                L"    <Setter.Value>"
                L"      <ControlTemplate TargetType='CalendarView'>"
                L"        <ScrollViewer x:Name='MonthViewScrollViewer'>"
                L"          <CalendarPanel x:Name='MonthViewPanel'/>"
                L"        </ScrollViewer>"
                L"      </ControlTemplate>"
                L"    </Setter.Value>"
                L"  </Setter>"
                L"</Style>"
                ));
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // even an empty template will not crash the app.
            cv->Style = dynamic_cast<xaml::Style^>(xaml_markup::XamlReader::Load(
                L"<Style xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' TargetType='CalendarView'>"
                L"  <Setter Property='Template'>"
                L"    <Setter.Value>"
                L"      <ControlTemplate TargetType='CalendarView'/>"
                L"    </Setter.Value>"
                L"  </Setter>"
                L"</Style>"
                ));
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void CalendarViewIntegrationTests::NavigationButtonShouldBeDisabledWhenThereIsNoMoreDates()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Grid^ rootPanel = nullptr;

        xaml_controls::Button^ previousButton = nullptr;
        xaml_controls::Button^ nextButton = nullptr;
        ::Windows::Foundation::DateTime minDate = ConvertToDateTime(1, 2000, 1, 1);
        ::Windows::Foundation::DateTime maxDate = ConvertToDateTime(1, 2000, 2, 6);

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            cv->MinDate = minDate;
            cv->MaxDate = maxDate;
            cv->SetDisplayDate(minDate);
            rootPanel->Children->Append(cv);
        });


        TestServices::WindowHelper->WaitForIdle();

        // find the buttons
        RunOnUIThread([&]()
        {
            previousButton = safe_cast<xaml_controls::Button^>(helper.GetTemplateChild(L"PreviousButton"));
            nextButton = safe_cast<xaml_controls::Button^>(helper.GetTemplateChild(L"NextButton"));

            LOG_OUTPUT(L"previous button's enable state is %d, next button's enable state is %d.", previousButton->IsEnabled, nextButton->IsEnabled);
            VERIFY_IS_FALSE(previousButton->IsEnabled); // can't go backwards
            VERIFY_IS_TRUE(nextButton->IsEnabled);      // can go forward
        });

        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);
        auto viewChangedEvent = std::make_shared<Event>();
        RunOnUIThread([&]()
        {
            auto scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(helper.GetTemplateChild(L"MonthViewScrollViewer"));
            viewChangedRegistration.Attach(scrollViewer,
                ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [viewChangedEvent](Platform::Object^ sender, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                if (args->IsIntermediate == false)
                {
                    viewChangedEvent->Set();
                }
            }));
        });

        TestServices::InputHelper->DynamicPressCenter(nextButton, 0, 0, PointerFinger::Finger1);
        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->DynamicRelease(PointerFinger::Finger1);
        TestServices::WindowHelper->WaitForIdle();

        viewChangedEvent->WaitForDefault();
        VERIFY_IS_TRUE(viewChangedEvent->HasFired());

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"previous button's enable state is %d, next button's enable state is %d.", previousButton->IsEnabled, nextButton->IsEnabled);
            VERIFY_IS_TRUE(previousButton->IsEnabled);  // can go backwards
            VERIFY_IS_FALSE(nextButton->IsEnabled);     // can't go forward
        });
    }

    void CalendarViewIntegrationTests::ValidateChromeFocus()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            cv->MinDate = ConvertToDateTime(1, 2014, 1, 1);
            cv->MaxDate = ConvertToDateTime(1, 2014, 1, 1);
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto monthPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(L"MonthViewPanel"));
            auto firstItem = safe_cast<xaml_controls::CalendarViewDayItem^>(monthPanel->Children->GetAt(0));
            firstItem->Focus(Microsoft::UI::Xaml::FocusState::Keyboard);
        });

        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }

    void CalendarViewIntegrationTests::ValidateChromeFocusYear()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;

        CalendarHelper::CalendarViewHelper helper;
        xaml_controls::Button^ headerButton = nullptr;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            cv->MinDate = ConvertToDateTime(1, 2014, 1, 1);
            cv->MaxDate = ConvertToDateTime(1, 2014, 1, 1);
            cv->DisplayMode = xaml_controls::CalendarViewDisplayMode::Year;
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto yearPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(L"YearViewPanel"));
            auto firstItem = safe_cast<xaml_controls::Control^>(yearPanel->Children->GetAt(0));
            firstItem->Focus(Microsoft::UI::Xaml::FocusState::Keyboard);
        });

        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }

    void CalendarViewIntegrationTests::ValidateChromeFocusDecade()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;

        CalendarHelper::CalendarViewHelper helper;
        xaml_controls::Button^ headerButton = nullptr;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            cv->MinDate = ConvertToDateTime(1, 2014, 1, 1);
            cv->MaxDate = ConvertToDateTime(1, 2014, 1, 1);
            cv->DisplayMode = xaml_controls::CalendarViewDisplayMode::Decade;
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto decadePanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(L"DecadeViewPanel"));
            auto firstItem = safe_cast<xaml_controls::Control^>(decadePanel->Children->GetAt(0));
            firstItem->Focus(Microsoft::UI::Xaml::FocusState::Keyboard);
        });

        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }

    void CalendarViewIntegrationTests::IgnoreBringIntoViewOnFocusChange()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        CalendarHelper::CreateTestResources(rootPanel);

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        // load into visual tree
        RunOnUIThread([&]()
        {
            cv->MinDate = ConvertToDateTime(1, 2000, 1, 1);
            cv->MaxDate = ConvertToDateTime(1, 2000, 2, 6);
            rootPanel->Children->Append(cv);
        });


        TestServices::WindowHelper->WaitForIdle();

        // We'll focus on the item (Jan 23) on last second row, then we press Down arrow key to move focus to the last row (Jan 30)
        // because Jan30 is on the last visible row, without this fix, ScrollViewer will bring this item into view and try to reserver 20 pixels
        // on the bottom, which cause bad visual effect. (by default viewport should land on item's edge, not half of item).

        RunOnUIThread([&]()
        {
            auto monthPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(L"MonthViewPanel"));
            auto itemJan23 = safe_cast<xaml_controls::CalendarViewDayItem^>(monthPanel->Children->GetAt(22));
            itemJan23->Focus(Microsoft::UI::Xaml::FocusState::Keyboard);
        });

        TestServices::WindowHelper->WaitForIdle();

        TestServices::KeyboardHelper->Down();

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            CheckFocusedItem();
            auto focusedElement = safe_cast<xaml_controls::CalendarViewDayItem^>(Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            auto monthPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(L"MonthViewPanel"));
            auto itemJan30 = safe_cast<xaml_controls::CalendarViewDayItem^>(monthPanel->Children->GetAt(29));
            VERIFY_ARE_EQUAL(focusedElement, itemJan30);

            auto scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(helper.GetTemplateChild(L"MonthViewScrollViewer"));
            ::Windows::Foundation::Point origin = { 0, 0 };
            auto itemPos = focusedElement->TransformToVisual(scrollViewer)->TransformPoint(origin);

            auto itemHeight = safe_cast<Microsoft::UI::Xaml::FrameworkElement^>(focusedElement)->ActualHeight;

            auto margin = safe_cast<Microsoft::UI::Xaml::FrameworkElement^>(focusedElement)->Margin;

            itemHeight += margin.Top + margin.Bottom;

            ::Windows::Foundation::Point expectedPos = { static_cast<float>(margin.Left), static_cast<float>(itemHeight * 5 + margin.Top) };
            LOG_OUTPUT(L"actual position of Jan30 is (%f, %f)., expected (%f, %f)", itemPos.X, itemPos.Y, expectedPos.X, expectedPos.Y);

            VERIFY_ARE_EQUAL(itemPos.X, expectedPos.X);
            VERIFY_ARE_EQUAL(itemPos.Y, expectedPos.Y);
        });
    }


    void CalendarViewIntegrationTests::SuspendBuildTreeWhileCollapsed()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        CalendarHelper::CreateTestResources(rootPanel);

        auto cicEvent = std::make_shared<Event>();
        auto cicRegistration = CreateSafeEventRegistration(xaml_controls::CalendarView, CalendarViewDayItemChanging);
        cicRegistration.Attach(
            cv,
            ref new ::Windows::Foundation::TypedEventHandler<xaml_controls::CalendarView^, xaml_controls::CalendarViewDayItemChangingEventArgs^>(
            [cicEvent](xaml_controls::CalendarView^ sender, xaml_controls::CalendarViewDayItemChangingEventArgs^ e)
        {
            if (!e->InRecycleQueue)
            {
                e->RegisterUpdateCallback(
                    ref new ::Windows::Foundation::TypedEventHandler<xaml_controls::CalendarView^, xaml_controls::CalendarViewDayItemChangingEventArgs^>(
                    [cicEvent](xaml_controls::CalendarView^ sender, xaml_controls::CalendarViewDayItemChangingEventArgs^ e)
                {
                    VERIFY_IS_TRUE(e->Phase == 1);
                    cicEvent->Set();
                }));
            }
        }));

        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Visibility = Microsoft::UI::Xaml::Visibility::Collapsed;
            cv->MinDate = ConvertToDateTime(1, 2000, 1, 1);
            cv->MaxDate = ConvertToDateTime(1, 2000, 1, 1);
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();
        VERIFY_IS_FALSE(cicEvent->HasFired());

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"measure to force it to register buildtree work.");
            cv->Measure(wf::Size(500.f, 500.f));
        });

        TestServices::WindowHelper->WaitForIdle();
        VERIFY_IS_FALSE(cicEvent->HasFired());

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"make parent visible");
            rootPanel->Visibility = Microsoft::UI::Xaml::Visibility::Visible;
        });

        cicEvent->WaitForDefault();
        VERIFY_IS_TRUE(cicEvent->HasFired());
        TestServices::WindowHelper->WaitForIdle();
    }

    // read all the timezone ids, return the one slice
    std::vector<Platform::String^> ReadTimeZoneIds(unsigned part, unsigned total)
    {
        std::vector<Platform::String^> tzids;

        wil::unique_hkey hKey = NULL;

        LSTATUS result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones", 0, KEY_READ, &hKey);
        FAIL_FAST_IF(result != ERROR_SUCCESS);

        DWORD cSubKeys;        //Used to store the number of Subkeys
        DWORD maxSubkeyLen;    //Longest Subkey name length

        result = RegQueryInfoKey(
            hKey.get(),
            nullptr,
            nullptr,
            nullptr,
            &cSubKeys,
            &maxSubkeyLen,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr);

        FAIL_FAST_IF(result != ERROR_SUCCESS);
        FAIL_FAST_IF(maxSubkeyLen >= MAX_PATH);
        FAIL_FAST_IF(part >= total);

        auto length = cSubKeys / total;
        auto begin = part * length;
        auto end = begin + length;
        if (part == cSubKeys - 1)
        {
            // the last part takes all remaining timezones.
            end = cSubKeys;
        }

        for (unsigned i = begin; i < end; i++)
        {
            wchar_t currentSubkey[MAX_PATH] = { 0 };
            DWORD currentSubLen = MAX_PATH;
            result = RegEnumKeyEx(
                hKey.get(),
                i,
                currentSubkey,
                &currentSubLen,
                nullptr,
                nullptr,
                nullptr,
                nullptr);
            FAIL_FAST_IF(result != ERROR_SUCCESS);

            std::wstring timeZoneKeyStr = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones\\" + std::wstring(currentSubkey);
            wil::unique_hkey timeZoneHKey;
            result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, timeZoneKeyStr.c_str(), 0, KEY_READ, &timeZoneHKey);
            FAIL_FAST_IF(result != ERROR_SUCCESS);

            bool isObsolete = ReadDwordAsBoolFromRegKey(timeZoneHKey.get(), L"IsObsolete");
            bool isCustom = ReadDwordAsBoolFromRegKey(timeZoneHKey.get(), L"IsCustom");

            // skip un official and obsolete timezones.
            if (isCustom || isObsolete)
                continue;

            tzids.emplace_back(Platform::StringReference(currentSubkey));
        }

        return tzids;
    }

    void VerifyTimeZones(unsigned part, unsigned total)
    {
        std::wstring currentTimeZoneId = GetCurrentTimeZoneId();
        auto tzids = ReadTimeZoneIds(part, total);
        TestCleanupWrapper cleanup([&currentTimeZoneId]()
        {
            // restore timezone information
            TestServices::Utilities->SetTimeZone(Platform::StringReference(currentTimeZoneId.c_str()));

            // restore the privilege
            TestServices::Utilities->EnableChangingTimeZone(false);

            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        // Enable the required privilege (to allow our broker process to adjust time zone information)
        TestServices::Utilities->EnableChangingTimeZone(true);

        xaml_controls::Grid^ rootPanel = nullptr;
        CalendarHelper::CreateTestResources(rootPanel);

        // Adjust the time zone information
        for (auto& tzid : tzids)
        {
            if (tzid == L"Samoa Standard Time")
            {
                // TODO: enable this time zone once below bug is fixed
                // Calling AddDays(N) followed by AddDays(-N) doesn't go back to same day in Samoa TimeZone when the range contains [12/30/2008, 1/2/2012]
                continue;
            }

            LOG_OUTPUT(L"\r\nSet TimeZone to \"%s\"", tzid->Data());
            TestServices::Utilities->SetTimeZone(tzid);

            CalendarHelper::CalendarViewHelper helper;
            xaml_controls::CalendarView^ cv = helper.GetCalendarView();
            xaml_controls::Primitives::CalendarPanel^ monthPanel = nullptr;
            RunOnUIThread([&]()
            {
                rootPanel->Children->Clear();
                rootPanel->Children->Append(cv);
                cv->MinDate = ConvertToDateTime(1, 1910, 1, 2, 1, 8, 0, 0, 0); // Monday
                cv->MaxDate = ConvertToDateTime(1, 2101, 1, 1, 1, 8, 0, 0, 0); // Sunday

            });

            TestServices::WindowHelper->WaitForIdle();

            // below tests will verify the first day and last day are in the correct positions.

            RunOnUIThread([&]()
            {
                monthPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(L"MonthViewPanel"));
                cv->SetDisplayDate(cv->MinDate);
            });

            RunOnUIThread([&]()
            {
                auto firstItem = safe_cast<xaml_controls::CalendarViewDayItem^>(monthPanel->Children->GetAt(0));
                CalendarHelper::VerifyDateTimesAreEqual(firstItem->Date, cv->MinDate);
                VerifyItemPositionInPanel(firstItem, monthPanel, 0, 0);  //Monday

                cv->SetDisplayDate(cv->MaxDate);

            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                xaml_controls::CalendarViewDayItem^ lastItem;
                ::Windows::Foundation::Rect layoutSlot;
                int lastIndex = monthPanel->Children->Size;

                // find the last realized item. The realized item's layout slot will
                // not have negative offset.
                do
                {
                    lastIndex--;
                    lastItem = safe_cast<xaml_controls::CalendarViewDayItem^>(monthPanel->Children->GetAt(lastIndex));
                    layoutSlot = xaml_primitives::LayoutInformation::GetLayoutSlot(lastItem);
                } while (lastIndex > 0 && (layoutSlot.X < 0 || layoutSlot.Y < 0));

                CalendarHelper::VerifyDateTimesAreEqual(lastItem->Date, cv->MaxDate);

                VerifyItemPositionInPanel(lastItem, monthPanel, 6, 9965); // Sunday
            });
        }
    }

    void CalendarViewIntegrationTests::VerifySkippedDaysInSamoa()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        std::wstring currentTimeZoneId = GetCurrentTimeZoneId();
        TestCleanupWrapper cleanup([&currentTimeZoneId]()
        {
            // restore timezone information
            TestServices::Utilities->SetTimeZone(Platform::StringReference(currentTimeZoneId.c_str()));

            // restore the privilege
            TestServices::Utilities->EnableChangingTimeZone(false);

            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        // Enable the required privilege (to allow our broker process to adjust time zone information)
        TestServices::Utilities->EnableChangingTimeZone(true);

        xaml_controls::Grid^ rootPanel = nullptr;
        CalendarHelper::CreateTestResources(rootPanel);

        // Adjust the time zone information
        Platform::String^ tzid = L"Samoa Standard Time";
        LOG_OUTPUT(L"\r\nSet TimeZone to \"%s\"", tzid->Data());
        TestServices::Utilities->SetTimeZone(tzid);

        CalendarHelper::CalendarViewHelper helper;
        xaml_controls::CalendarView^ cv = helper.GetCalendarView();
        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cv);
            cv->MinDate = ConvertToDateTime(1, 2011, 12, 31); // Saturday
            cv->MaxDate = ConvertToDateTime(1, 2012, 1, 2); // Monday
            cv->FirstDayOfWeek = ::Windows::Globalization::DayOfWeek::Saturday;
        });

        TestServices::WindowHelper->WaitForIdle();
        // Recording DComp dump to detect possible changes in days rendering.
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

        // below test will verify that we have a hole between 12/31/2011 and 1/2/2012
        // and also verify that days after 1/2/2012 are on the correct dayofweek.
        RunOnUIThread([&]()
        {
            auto monthPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(L"MonthViewPanel"));
            VERIFY_ARE_EQUAL(monthPanel->Children->Size, 2u);   // there are only two days in this view
            VerifyItemPositionInPanel(monthPanel->Children->GetAt(0), monthPanel, 0, 0);    // the first date (12/21/2011) is at (0, 0)
            VerifyItemPositionInPanel(monthPanel->Children->GetAt(1), monthPanel, 2, 0);    // the second date (1/2/2012) is at (0, 2)
        });

    }

    void CalendarViewIntegrationTests::TodayVisualTest()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        auto Jan12015 = ConvertToDateTime(1, 2015, 1, 1);

        xaml_controls::Grid^ rootPanel = nullptr;
        CalendarHelper::CreateTestResources(rootPanel);

        xaml_controls::CalendarView^ cv = nullptr;
        xaml_controls::CalendarViewDayItem^ todayItem = nullptr;

        {
            /* MOCK10_REMOVAL
            // change today to 1/1/2015
            auto mock = Mock10::Mock::Function(GetSystemTimePreciseAsFileTime, [](LPFILETIME lpFileTime)
            {
                SYSTEMTIME st = {};
                st.wYear = 2015;
                st.wMonth = 1;
                st.wDay = 1;
                st.wDayOfWeek = 4;
                st.wHour = 8;
                st.wMinute = 0;
                st.wSecond = 0;
                st.wMilliseconds = 0;
                SystemTimeToFileTime(&st, lpFileTime);
                LOG_OUTPUT(L"SystemTime is mocked to 1/1/2015 08:00:00.");
            });
            */

            RunOnUIThread([&]()
            {
                cv = ref new xaml_controls::CalendarView();

                rootPanel->Children->Append(cv);
                cv->MinDate = Jan12015;
                cv->MaxDate = Jan12015;
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "1");    //Today

        RunOnUIThread([&]()
        {
            cv->IsTodayHighlighted = false;
        });

        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "2");    //Today+without highlighted

        RunOnUIThread([&]()
        {
            cv->IsTodayHighlighted = true;
            cv->SelectedDates->Append(Jan12015);
        });

        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "3");    //Today+Selected

        RunOnUIThread([&]()
        {
            auto monthPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(CalendarHelper::GetTemplateChild(cv, L"MonthViewPanel"));
            todayItem = safe_cast<xaml_controls::CalendarViewDayItem^>(monthPanel->Children->GetAt(0));
            todayItem->IsBlackout = true;
        });

        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "4");    //Today+Blackout

        RunOnUIThread([&]()
        {
            todayItem->IsBlackout = false;
        });

        TestServices::InputHelper->MoveMouse(todayItem);

        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "5");    //Today+Hover
        // move mouse away to avoid any unexpected hover state.
        TestServices::InputHelper->MoveMouse(::Windows::Foundation::Point(0, 0));

        TestServices::InputHelper->DynamicPressCenter(todayItem, 0, 0, PointerFinger::Finger1);

        TestServices::WindowHelper->WaitForIdle();
        TestServices::WindowHelper->SynchronouslyTickUIThread(2);

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "6");    //Today+Pressed

        TestServices::InputHelper->DynamicRelease(PointerFinger::Finger1);
    }

    void CalendarViewIntegrationTests::ChangingViewHeaderTest()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::Button^ headerButton = nullptr;

        /* MOCK10_REMOVAL
        // change today to 12/9/2015
        auto mock = Mock10::Mock::Function(GetSystemTimePreciseAsFileTime, [](LPFILETIME lpFileTime)
        {
            SYSTEMTIME st = {};
            st.wYear = 2015;
            st.wMonth = 12;
            st.wDay = 9;
            st.wDayOfWeek = 3;
            st.wHour = 8;
            st.wMinute = 0;
            st.wSecond = 0;
            st.wMilliseconds = 0;
            SystemTimeToFileTime(&st, lpFileTime);
            LOG_OUTPUT(L"SystemTime is mocked to 12/9/2015 08:00:00.");
        });
        */

        CalendarHelper::CalendarViewHelper helper;
        xaml_controls::CalendarView^ cv = helper.GetCalendarView();
        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        // find the header button and tap it
        {
            RunOnUIThread([&]()
            {
                headerButton = safe_cast<xaml_controls::Button^>(helper.GetTemplateChild(L"HeaderButton"));
                auto headText = safe_cast<Platform::String^>(headerButton->Content);
                LOG_OUTPUT(L"Current month %s", headText->Data());
                VERIFY_IS_TRUE(headText == L"\u200eDecember\u200e \u200e2015");
            });

            LOG_OUTPUT(L"CalendarViewIntegrationTests: changing viewmode to Year by using Tap.");

            ControlHelper::DoClickUsingAP(headerButton);

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                headerButton = safe_cast<xaml_controls::Button^>(helper.GetTemplateChild(L"HeaderButton"));
                auto headText = safe_cast<Platform::String^>(headerButton->Content);
                LOG_OUTPUT(L"Current year %s", headText->Data());
                VERIFY_IS_TRUE(headText == L"\u200e2015");

            });

            LOG_OUTPUT(L"CalendarViewIntegrationTests: changing viewmode to Decade by using Tap.");

            ControlHelper::DoClickUsingAP(headerButton);

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                headerButton = safe_cast<xaml_controls::Button^>(helper.GetTemplateChild(L"HeaderButton"));
                auto headText = safe_cast<Platform::String^>(headerButton->Content);
                LOG_OUTPUT(L"Current decade %s", headText->Data());
                VERIFY_IS_TRUE(headText == L"2010 - 2019");
            });
        }
    }

    void CalendarViewIntegrationTests::ChangingViewHeaderWithCustomDimensionsTest()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::Button^ headerButton = nullptr;

        /* MOCK10_REMOVAL
        // change today to 12/9/2015
        auto mock = Mock10::Mock::Function(GetSystemTimePreciseAsFileTime, [](LPFILETIME lpFileTime)
        {
            SYSTEMTIME st = {};
            st.wYear = 2015;
            st.wMonth = 12;
            st.wDay = 9;
            st.wDayOfWeek = 3;
            st.wHour = 8;
            st.wMinute = 0;
            st.wSecond = 0;
            st.wMilliseconds = 0;
            SystemTimeToFileTime(&st, lpFileTime);
            LOG_OUTPUT(L"SystemTime is mocked to 12/9/2015 08:00:00.");
        });
        */

        CalendarHelper::CalendarViewHelper helper;
        xaml_controls::CalendarView^ cv = helper.GetCalendarView();
        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        // find the header button and tap it
        {
            RunOnUIThread([&]()
            {
                headerButton = safe_cast<xaml_controls::Button^>(helper.GetTemplateChild(L"HeaderButton"));
                auto headText = safe_cast<Platform::String^>(headerButton->Content);
                LOG_OUTPUT(L"Current month %s", headText->Data());
                VERIFY_IS_TRUE(headText == L"\u200eDecember\u200e \u200e2015");
            });

            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                cv->SetYearDecadeDisplayDimensions(4, 1);
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"CalendarViewIntegrationTests: changing viewmode to Year by using Tap.");

            ControlHelper::DoClickUsingAP(headerButton);

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                headerButton = safe_cast<xaml_controls::Button^>(helper.GetTemplateChild(L"HeaderButton"));
                auto headText = safe_cast<Platform::String^>(headerButton->Content);
                LOG_OUTPUT(L"Current year %s", headText->Data());
                VERIFY_IS_TRUE(headText == L"\u200e2015");

            });

            LOG_OUTPUT(L"CalendarViewIntegrationTests: changing viewmode to Decade by using Tap.");

            ControlHelper::DoClickUsingAP(headerButton);

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                headerButton = safe_cast<xaml_controls::Button^>(helper.GetTemplateChild(L"HeaderButton"));
                auto headText = safe_cast<Platform::String^>(headerButton->Content);
                LOG_OUTPUT(L"Current decade %s", headText->Data());
                VERIFY_IS_TRUE(headText == L"2010 - 2019");
            });
        }
    }

    void CalendarViewIntegrationTests::VerifyChangingHeightDoesNotScrollAwayFromItems()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::Button^ headerButton = nullptr;
        int changeCount = 0;

        CalendarHelper::CalendarViewHelper helper;

        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        CalendarHelper::CreateTestResources(rootPanel);

        // load into visual tree
        RunOnUIThread([&]()
        {
            cv->Height = 300;
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        auto cicEvent = std::make_shared<Event>();
        auto cicRegistration = CreateSafeEventRegistration(xaml_controls::CalendarView, CalendarViewDayItemChanging);
        cicRegistration.Attach(
            cv,
            ref new ::Windows::Foundation::TypedEventHandler<xaml_controls::CalendarView^, xaml_controls::CalendarViewDayItemChangingEventArgs^>(
                [cv, cicEvent, &changeCount](xaml_controls::CalendarView^ sender, xaml_controls::CalendarViewDayItemChangingEventArgs^ e)
        {
            changeCount = changeCount + 1;
        }));

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(changeCount == 0);

            cv->Height = 350;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(changeCount == 0);

            cv->Height = 300;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(changeCount == 0);
        });
    }

    void CalendarViewIntegrationTests::VerifyForegroundColorChangesPropagateWhenInPopup()
    {
        TestCleanupWrapper cleanup;
        CalendarHelper::CalendarViewHelper helper;
        ::Windows::UI::Color newColor = Microsoft::UI::Colors::Red;

        xaml_controls::Button^ flyoutButton = nullptr;
        xaml_controls::Flyout^ flyout = nullptr;
        xaml_controls::CalendarView^ calendarView = helper.GetCalendarView();

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::StackPanel();

            flyoutButton = ref new xaml_controls::Button();
            flyoutButton->Content = L"Click for CalendarView flyout";

            flyout = ref new xaml_controls::Flyout();
            flyout->Content = calendarView;

            flyoutButton->Flyout = flyout;
            rootPanel->Children->Append(flyoutButton);

            loadedRegistration.Attach(rootPanel, [loadedEvent]() { loadedEvent->Set(); });
            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Open the flyout to initially realize all of the CalendarView items.");
        FlyoutHelper::OpenFlyout<xaml_controls::Flyout>(flyout, flyoutButton, FlyoutOpenMethod::Programmatic_ShowAt);
        FlyoutHelper::HideFlyout<xaml_controls::Flyout>(flyout);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Now set CalendarItemForeground to change the intended foreground color of CalendarView items.");
            calendarView->CalendarItemForeground = ref new xaml_media::SolidColorBrush(newColor);
        });

        LOG_OUTPUT(L"Open the flyout again.  The items should have changed to the new foreground color.");
        FlyoutHelper::OpenFlyout<xaml_controls::Flyout>(flyout, flyoutButton, FlyoutOpenMethod::Programmatic_ShowAt);

        RunOnUIThread([&]()
        {
            auto calendarPanel = safe_cast<xaml_primitives::CalendarPanel^>(helper.GetTemplateChild("MonthViewPanel"));
            auto item = safe_cast<xaml_controls::Control^>(calendarPanel->Children->GetAt(0));
            auto textBlock = TreeHelper::GetVisualChildByType<xaml_controls::TextBlock>(item);
            auto textBlockColor = safe_cast<xaml_media::SolidColorBrush^>(textBlock->Foreground)->Color;

            VERIFY_ARE_EQUAL(newColor, textBlockColor);
        });

        FlyoutHelper::HideFlyout<xaml_controls::Flyout>(flyout);
    }

    // we have 128 timezones so far, split them into small parts so the test won't take too long.
#define VerifyAllTimeZonesPartImpl(i, j) \
    void CalendarViewIntegrationTests::VerifyAllTimeZonesPart##i()\
    {\
        VerifyTimeZones(i##u, j##u);\
    }

VerifyAllTimeZonesPartImpl(0, 20)
VerifyAllTimeZonesPartImpl(1, 20)
VerifyAllTimeZonesPartImpl(2, 20)
VerifyAllTimeZonesPartImpl(3, 20)
VerifyAllTimeZonesPartImpl(4, 20)
VerifyAllTimeZonesPartImpl(5, 20)
VerifyAllTimeZonesPartImpl(6, 20)
VerifyAllTimeZonesPartImpl(7, 20)
VerifyAllTimeZonesPartImpl(8, 20)
VerifyAllTimeZonesPartImpl(9, 20)
VerifyAllTimeZonesPartImpl(10, 20)
VerifyAllTimeZonesPartImpl(11, 20)
VerifyAllTimeZonesPartImpl(12, 20)
VerifyAllTimeZonesPartImpl(13, 20)
VerifyAllTimeZonesPartImpl(14, 20)
VerifyAllTimeZonesPartImpl(15, 20)
VerifyAllTimeZonesPartImpl(16, 20)
VerifyAllTimeZonesPartImpl(17, 20)
VerifyAllTimeZonesPartImpl(18, 20)
VerifyAllTimeZonesPartImpl(19, 20)


    void VerifyCalendarBoundariesByTappingByTappingNavigationButton(Platform::String^ cid)
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;
        auto defaultCalendar = ref new ::Windows::Globalization::Calendar();

        struct ModeHelper
        {
            xaml_controls::CalendarViewDisplayMode displayMode;
            Platform::String^ panelName;
            Platform::String^ scrollViewerName;
        };

        ModeHelper modes[] = {
            { xaml_controls::CalendarViewDisplayMode::Month, L"MonthViewPanel", L"MonthViewScrollViewer" },
            { xaml_controls::CalendarViewDisplayMode::Year, L"YearViewPanel", L"YearViewScrollViewer" },
            { xaml_controls::CalendarViewDisplayMode::Decade, L"DecadeViewPanel", L"DecadeViewScrollViewer" }
        };

        struct DimensionHelper
        {
            int numberOfWeeks;
            int rowInYearDecadeView;
            int colInYearDecadeView;
        };

        DimensionHelper dimensions[] = {
            {2, 2, 4},  // to test the code that Panel can not show a full scope
            {6, 4, 4}   // to test the code that Panel can show a full scope
        };

        CalendarHelper::CreateTestResources(rootPanel);

        LOG_OUTPUT(L"Begin Testing CalendarIdentifier: %s", cid->Data());

        for (auto& dimension : dimensions)
        {
            xaml_controls::Primitives::CalendarPanel^ calendarPanel = nullptr;
            CalendarHelper::CalendarViewHelper helper;

            xaml_controls::CalendarView^ cv = helper.GetCalendarView();
            xaml_controls::ScrollViewer^ scrollViewer = nullptr;
            xaml_controls::Primitives::CalendarPanel^ panel = nullptr;

            xaml_controls::Button^ previousButton = nullptr;
            xaml_controls::Button^ nextButton = nullptr;

            RunOnUIThread([&]()
            {
                rootPanel->Children->Clear();

                // set the limits from current calendaridentifier on CalendarView.
                cv->CalendarIdentifier = cid;
                auto cal = ref new ::Windows::Globalization::Calendar(defaultCalendar->Languages, cid, defaultCalendar->GetClock());

                cal->SetToMin();
                cv->MinDate = cal->GetDateTime();

                cal->SetToMax();
                cv->MaxDate = cal->GetDateTime();

                // load into visual tree
                rootPanel->Children->Append(cv);

                cv->UpdateLayout();

                cv->NumberOfWeeksInView = dimension.numberOfWeeks;
                cv->SetYearDecadeDisplayDimensions(dimension.colInYearDecadeView, dimension.rowInYearDecadeView);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                previousButton = safe_cast<xaml_controls::Button^>(helper.GetTemplateChild(L"PreviousButton"));
                nextButton = safe_cast<xaml_controls::Button^>(helper.GetTemplateChild(L"NextButton"));
            });

            for (auto& mode : modes)
            {
                LOG_OUTPUT(L"  Change DisplayMode to %s", mode.displayMode.ToString()->Data());

                double itemHeight = 0;

                RunOnUIThread([&]()
                {
                    cv->DisplayMode = mode.displayMode;
                    cv->UpdateLayout();

                    scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(helper.GetTemplateChild(mode.scrollViewerName));
                    panel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(mode.panelName));
                    auto firstItem = safe_cast<xaml_controls::Control^>(panel->Children->GetAt(0));
                    itemHeight = firstItem->ActualHeight;
                });

                auto viewChangedEvent = std::make_shared<Event>();
                auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

                viewChangedRegistration.Attach(scrollViewer,
                    ref new ::Windows::Foundation::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                    [viewChangedEvent](Platform::Object ^sender, xaml_controls::ScrollViewerViewChangedEventArgs ^e)
                {
                    if (!e->IsIntermediate)
                    {
                        LOG_OUTPUT(L"ViewChanged. vertical offset: %lf", static_cast<xaml_controls::ScrollViewer^>(sender)->VerticalOffset);
                        viewChangedEvent->Set();
                    }
                }));

                RunOnUIThread([&]()
                {
                    // jump to row#1.5, previous button is enabled
                    scrollViewer->ChangeView(nullptr/*horizontalOffset*/, itemHeight * 1.5, 1.0f, true /*disableAnimation*/);
                });

                viewChangedEvent->WaitForDefault();
                VERIFY_IS_TRUE(viewChangedEvent->HasFired());
                viewChangedEvent->Reset();

                RunOnUIThread([&]()
                {
                    VERIFY_IS_TRUE(previousButton->IsEnabled);
                });

                // tap the previous button, we should go to the very beginning without crash.
                TestServices::InputHelper->Tap(previousButton);

                viewChangedEvent->WaitForDefault();
                VERIFY_IS_TRUE(viewChangedEvent->HasFired());
                viewChangedEvent->Reset();

                RunOnUIThread([&]()
                {
                    VERIFY_ARE_EQUAL(scrollViewer->VerticalOffset, 0);

                    // jump to the last row#1.5, next button is enabled
                    scrollViewer->ChangeView(nullptr/*horizontalOffset*/, scrollViewer->ScrollableHeight - itemHeight * 1.5, 1.0f, true /*disableAnimation*/);
                });

                viewChangedEvent->WaitFor(std::chrono::milliseconds(10000));
                VERIFY_IS_TRUE(viewChangedEvent->HasFired());
                viewChangedEvent->Reset();

                RunOnUIThread([&]()
                {
                    VERIFY_IS_TRUE(nextButton->IsEnabled); // can go forward
                });

                // tap the next button, we should go to the very end without crash
                TestServices::InputHelper->Tap(nextButton);

                viewChangedEvent->WaitForDefault();
                VERIFY_IS_TRUE(viewChangedEvent->HasFired());
                viewChangedEvent->Reset();

                RunOnUIThread([&]()
                {
                    VERIFY_ARE_EQUAL(scrollViewer->VerticalOffset, scrollViewer->ScrollableHeight);
                });
            }
            LOG_OUTPUT(L"End Testing CalendarIdentifier: %s", cid->Data());
        }
    }

    void CalendarViewIntegrationTests::VerifyPersianCalendarBoundariesByTapping()
    {
        VerifyCalendarBoundariesByTappingByTappingNavigationButton("PersianCalendar");
    }

    void CalendarViewIntegrationTests::VerifyGregorianCalendarBoundariesByTapping()
    {
        VerifyCalendarBoundariesByTappingByTappingNavigationButton("GregorianCalendar");
    }

    void CalendarViewIntegrationTests::VerifyHebrewCalendarBoundariesByTapping()
    {
        VerifyCalendarBoundariesByTappingByTappingNavigationButton("HebrewCalendar");
    }

    void CalendarViewIntegrationTests::VerifyHijriCalendarBoundariesByTapping()
    {
        VerifyCalendarBoundariesByTappingByTappingNavigationButton("HijriCalendar");
    }

    void CalendarViewIntegrationTests::VerifyJapaneseCalendarBoundariesByTapping()
    {
        VerifyCalendarBoundariesByTappingByTappingNavigationButton("JapaneseCalendar");
    }

    void CalendarViewIntegrationTests::VerifyJulianCalendarBoundariesByTapping()
    {
        VerifyCalendarBoundariesByTappingByTappingNavigationButton("JulianCalendar");
    }

    void CalendarViewIntegrationTests::VerifyKoreanCalendarBoundariesByTapping()
    {
        VerifyCalendarBoundariesByTappingByTappingNavigationButton("KoreanCalendar");
    }

    void CalendarViewIntegrationTests::VerifyTaiwanCalendarBoundariesByTapping()
    {
        VerifyCalendarBoundariesByTappingByTappingNavigationButton("TaiwanCalendar");
    }

    void CalendarViewIntegrationTests::VerifyThaiCalendarBoundariesByTapping()
    {
        VerifyCalendarBoundariesByTappingByTappingNavigationButton("ThaiCalendar");
    }

    void CalendarViewIntegrationTests::VerifyUmAlQuraCalendarBoundariesByTapping()
    {
        VerifyCalendarBoundariesByTappingByTappingNavigationButton("UmAlQuraCalendar");
    }

    void CalendarViewIntegrationTests::VerifyFlowDirectionForCalendar()
    {
        TestCleanupWrapper cleanup;

        CalendarHelper::CalendarViewHelper helper;
        xaml_controls::CalendarView^ cv = helper.GetCalendarView();
        xaml_controls::Grid^ rootPanel;
        CalendarHelper::CreateTestResources(rootPanel);

        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cv);
        });
        TestServices::WindowHelper->WaitForIdle();

        // The expected FlowDirection for each calendar id:
        std::vector<std::pair<Platform::String^, xaml::FlowDirection>> calendarFlowDirections
        {
            { L"PersianCalendar", xaml::FlowDirection::RightToLeft },
            { L"GregorianCalendar", xaml::FlowDirection::LeftToRight },
            { L"HebrewCalendar", xaml::FlowDirection::RightToLeft },
            { L"HijriCalendar", xaml::FlowDirection::RightToLeft },
            { L"JapaneseCalendar", xaml::FlowDirection::LeftToRight },
            { L"JulianCalendar", xaml::FlowDirection::LeftToRight },
            { L"KoreanCalendar", xaml::FlowDirection::LeftToRight },
            { L"TaiwanCalendar", xaml::FlowDirection::LeftToRight },
            { L"ThaiCalendar", xaml::FlowDirection::LeftToRight },
            { L"UmAlQuraCalendar", xaml::FlowDirection::RightToLeft }
        };

        for (auto& calendarFlowDirection : calendarFlowDirections)
        {
            auto cid = calendarFlowDirection.first;
            auto expectedFlowDirection = calendarFlowDirection.second;

            LOG_OUTPUT(L"Verifying FlowDirection for Calendar '%s' is '%s'", cid->Begin(), expectedFlowDirection.ToString()->Begin());

            RunOnUIThread([&]()
            {
                cv->CalendarIdentifier = cid;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto viewsGrid = safe_cast<xaml_controls::Grid^>(helper.GetTemplateChild("Views"));
                auto actualFlowDirection = viewsGrid->FlowDirection;
                VERIFY_ARE_EQUAL(expectedFlowDirection, actualFlowDirection);
            });
        }
    }

    void CalendarViewIntegrationTests::CanScrollAcrossJapaneseEraOnMonthView()
    {
        TestCleanupWrapper cleanup;

        CalendarHelper::CalendarViewHelper helper;
        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        xaml_controls::Grid^ rootPanel = nullptr;
        CalendarHelper::CreateTestResources(rootPanel);

        RunOnUIThread([&]()
        {
            cv->CalendarIdentifier = L"JapaneseCalendar";
            cv->DisplayMode = xaml_controls::CalendarViewDisplayMode::Month;
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Verify Taisho - Showa boundary in year view");
        auto firstDayOfShowa = ConvertToDateTime(1, 1926, 12, 25);
        VerifyJapaneseEraBoundary(helper, xaml_controls::CalendarViewDisplayMode::Month, firstDayOfShowa);

        LOG_OUTPUT(L"Verify Showa - Heisei boundary in year view");
        auto firstDayOfHeisei = ConvertToDateTime(1, 1989, 1, 8);
        VerifyJapaneseEraBoundary(helper, xaml_controls::CalendarViewDisplayMode::Month, firstDayOfHeisei);

        LOG_OUTPUT(L"Verify Heisei - Reiwa boundary in year view");
        auto firstDayOfReiwa = ConvertToDateTime(1, 2019, 5, 1);
        VerifyJapaneseEraBoundary(helper, xaml_controls::CalendarViewDisplayMode::Month, firstDayOfReiwa);
    }

    void CalendarViewIntegrationTests::CanScrollAcrossJapaneseEraOnYearView()
    {
        TestCleanupWrapper cleanup;

        CalendarHelper::CalendarViewHelper helper;
        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        xaml_controls::Grid^ rootPanel = nullptr;
        CalendarHelper::CreateTestResources(rootPanel);

        RunOnUIThread([&]()
        {
            cv->CalendarIdentifier = L"JapaneseCalendar";
            cv->DisplayMode = xaml_controls::CalendarViewDisplayMode::Year;
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Verify Taisho - Showa boundary in year view");
        auto firstDayOfShowa = ConvertToDateTime(1, 1926, 12, 25);
        VerifyJapaneseEraBoundary(helper, xaml_controls::CalendarViewDisplayMode::Year, firstDayOfShowa);

        LOG_OUTPUT(L"Verify Showa - Heisei boundary in year view");
        auto firstDayOfHeisei = ConvertToDateTime(1, 1989, 1, 8);
        VerifyJapaneseEraBoundary(helper, xaml_controls::CalendarViewDisplayMode::Year, firstDayOfHeisei);

        LOG_OUTPUT(L"Verify Heisei - Reiwa boundary in year view");
        auto firstDayOfReiwa = ConvertToDateTime(1, 2019, 5, 1);
        VerifyJapaneseEraBoundary(helper, xaml_controls::CalendarViewDisplayMode::Year, firstDayOfReiwa);
    }

    void CalendarViewIntegrationTests::CanScrollAcrossJapaneseEraOnDecadeView()
    {
        TestCleanupWrapper cleanup;

        CalendarHelper::CalendarViewHelper helper;
        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        xaml_controls::Grid^ rootPanel = nullptr;
        CalendarHelper::CreateTestResources(rootPanel);

        RunOnUIThread([&]()
        {
            cv->MinDate = ConvertToDateTime(1, 1921, 1, 1); // Making sure there are enough Taisho dates to show the expected "10 - 15" header.
            cv->CalendarIdentifier = L"JapaneseCalendar";
            cv->DisplayMode = xaml_controls::CalendarViewDisplayMode::Decade;
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Verify Taisho - Showa boundary in decade view");
        auto firstDayOfShowa = ConvertToDateTime(1, 1926, 12, 25);
        VerifyJapaneseEraBoundary(helper, xaml_controls::CalendarViewDisplayMode::Decade, firstDayOfShowa, L"10 - 15", L"1 - 9");

        LOG_OUTPUT(L"Verify Showa - Heisei boundary in decade view");
        auto firstDayOfHeisei = ConvertToDateTime(1, 1989, 1, 8);
        VerifyJapaneseEraBoundary(helper, xaml_controls::CalendarViewDisplayMode::Decade, firstDayOfHeisei, L"60 - 64", L"1 - 9");

        LOG_OUTPUT(L"Verify Heisei - Reiwa boundary in decade view");
        auto firstDayOfReiwa = ConvertToDateTime(1, 2019, 5, 1);
        VerifyJapaneseEraBoundary(helper, xaml_controls::CalendarViewDisplayMode::Decade, firstDayOfReiwa, L"30 - 31", L"1 - 9");
    }

    void CalendarViewIntegrationTests::VerifyJapaneseEraBoundary(CalendarHelper::CalendarViewHelper& helper, xaml_controls::CalendarViewDisplayMode const& displayMode, ::Windows::Foundation::DateTime const& date, Platform::String^ oldEraHeaderText, Platform::String^ newEraHeaderText)
    {
        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        xaml_controls::Button^ headerButton = nullptr;
        xaml_controls::Button^ previousButton = nullptr;
        xaml_controls::Button^ nextButton = nullptr;
        xaml_controls::ScrollViewer^ scrollViewer = nullptr;
        RunOnUIThread([&]()
        {
            headerButton = safe_cast<xaml_controls::Button^>(helper.GetTemplateChild(L"HeaderButton"));
            previousButton = safe_cast<xaml_controls::Button^>(helper.GetTemplateChild(L"PreviousButton"));
            nextButton = safe_cast<xaml_controls::Button^>(helper.GetTemplateChild(L"NextButton"));
            switch (displayMode)
            {
            case xaml_controls::CalendarViewDisplayMode::Decade:
                scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(helper.GetTemplateChild(L"DecadeViewScrollViewer"));
                break;
            case xaml_controls::CalendarViewDisplayMode::Year:
                scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(helper.GetTemplateChild(L"YearViewScrollViewer"));
                break;
            case xaml_controls::CalendarViewDisplayMode::Month:
                scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(helper.GetTemplateChild(L"MonthViewScrollViewer"));
                break;
            }

            cv->SetDisplayDate(date);
        });

        auto viewChangedEvent = std::make_shared<Event>();
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);
        viewChangedRegistration.Attach(scrollViewer,
            ref new ::Windows::Foundation::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [viewChangedEvent](Platform::Object ^sender, xaml_controls::ScrollViewerViewChangedEventArgs ^e)
        {
            if (!e->IsIntermediate)
            {
                viewChangedEvent->Set();
            }
        }));

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto headText = safe_cast<Platform::String^>(headerButton->Content);
            if (newEraHeaderText)
            {
                LOG_OUTPUT(L"headText: %s, newEraHeaderText: %s", headText->Data(), newEraHeaderText->Data());
                VERIFY_IS_TRUE(headText == newEraHeaderText);
            }
            else
            {
                LOG_OUTPUT(L"headText: %s", headText->Data());
            }
        });

        TestServices::InputHelper->Tap(previousButton);

        viewChangedEvent->WaitForDefault();
        viewChangedEvent->Reset();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto headText = safe_cast<Platform::String^>(headerButton->Content);
            if (oldEraHeaderText)
            {
                LOG_OUTPUT(L"headText: %s, oldEraHeaderText: %s", headText->Data(), oldEraHeaderText->Data());
                VERIFY_IS_TRUE(headText == oldEraHeaderText);
            }
            else
            {
                LOG_OUTPUT(L"headText: %s", headText->Data());
            }
        });

        TestServices::InputHelper->Tap(nextButton);

        viewChangedEvent->WaitForDefault();
        viewChangedEvent->Reset();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto headText = safe_cast<Platform::String^>(headerButton->Content);
            if (newEraHeaderText)
            {
                LOG_OUTPUT(L"headText: %s, newEraHeaderText: %s", headText->Data(), newEraHeaderText->Data());
                VERIFY_IS_TRUE(headText == newEraHeaderText);
            }
            else
            {
                LOG_OUTPUT(L"headText: %s", headText->Data());
            }
        });
    }

    void CalendarViewIntegrationTests::CanSwitchDisplayMode()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::CalendarViewDisplayMode modes[] = {
            xaml_controls::CalendarViewDisplayMode::Month,
            xaml_controls::CalendarViewDisplayMode::Year,
            xaml_controls::CalendarViewDisplayMode::Decade,
            xaml_controls::CalendarViewDisplayMode::Year,
            xaml_controls::CalendarViewDisplayMode::Month
        };

        Platform::String^ panelNames[] = { L"MonthViewPanel", L"YearViewPanel", L"DecadeViewPanel", L"YearViewPanel", L"MonthViewPanel" };

        CalendarHelper::CalendarViewHelper helper;
        xaml_controls::CalendarView^ cv = helper.GetCalendarView();

        xaml_controls::Grid^ rootPanel = nullptr;
        CalendarHelper::CreateTestResources(rootPanel);

        RunOnUIThread([&]()
        {
            rootPanel->Children->Append(cv);
        });

        TestServices::WindowHelper->WaitForIdle();

        int count = _countof(modes);
        for (int i = 0; i < count; i++)
        {
            RunOnUIThread([&]()
            {
                cv->DisplayMode = modes[i];
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                xaml_controls::Primitives::CalendarPanel^ panel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(helper.GetTemplateChild(panelNames[i]));
                VERIFY_IS_NOT_NULL(panel);
            });
        }

    }

    LinearGradientBrush^ CalendarViewIntegrationTests::MakeLinearGradientBrush(float startX, float endX)
    {
        GradientStop^ stop1 = ref new GradientStop();
        stop1->Color = Microsoft::UI::Colors::Green;
        stop1->Offset = 0.2;

        GradientStop^ stop2 = ref new GradientStop();
        stop2->Color = Microsoft::UI::Colors::Red;
        stop2->Offset = 0.8;

        GradientStopCollection^ stopCollection = ref new GradientStopCollection();
        stopCollection->Append(stop1);
        stopCollection->Append(stop2);

        LinearGradientBrush^ brush = ref new LinearGradientBrush();
        brush->StartPoint = ::Windows::Foundation::Point(startX, 0);
        brush->EndPoint = ::Windows::Foundation::Point(endX, 0);
        brush->GradientStops = stopCollection;
        return brush;
    }

    void CalendarViewIntegrationTests::DisplayCalendarViewWithRoundedCornersStyle()
    {
        TestCleanupWrapper cleanup;

        DisplayCalendarView(
            6 /*numberOfWeeksInView*/, // Displaying default 6 weeks in the month view
            false /*isInteractive*/, // Use 'true' for manual interactive testing
            true  /*enableRoundedListViewBaseItemChrome*/);
    }

    void CalendarViewIntegrationTests::DisplayCalendarViewWithRoundedCornersStyleAndCustomNumberOfWeeksInView()
    {
        TestCleanupWrapper cleanup;

        DisplayCalendarView(
            2 /*numberOfWeeksInView*/,
            false /*isInteractive*/, // Use 'true' for manual interactive testing
            true  /*enableRoundedListViewBaseItemChrome*/);

        DisplayCalendarView(
            4 /*numberOfWeeksInView*/,
            false /*isInteractive*/, // Use 'true' for manual interactive testing
            true  /*enableRoundedListViewBaseItemChrome*/);

        DisplayCalendarView(
            8 /*numberOfWeeksInView*/,
            false /*isInteractive*/, // Use 'true' for manual interactive testing
            true  /*enableRoundedListViewBaseItemChrome*/);
    }

    void CalendarViewIntegrationTests::DisplayCalendarViewWithSquareCornersStyles()
    {
        TestCleanupWrapper cleanup;

        DisplayCalendarView(
            6 /*numberOfWeeksInView*/, // Displaying default 6 weeks in the month view
            false /*isInteractive*/, // Use 'true' for manual interactive testing
            false /*enableRoundedListViewBaseItemChrome*/);
    }

    void CalendarViewIntegrationTests::DisplayCalendarViewWithSquareCornersStylesAndCustomNumberOfWeeksInView()
    {
        TestCleanupWrapper cleanup;

        DisplayCalendarView(
            2 /*numberOfWeeksInView*/,
            false /*isInteractive*/, // Use 'true' for manual interactive testing
            false /*enableRoundedListViewBaseItemChrome*/);

        DisplayCalendarView(
            4 /*numberOfWeeksInView*/,
            false /*isInteractive*/, // Use 'true' for manual interactive testing
            false /*enableRoundedListViewBaseItemChrome*/);

        DisplayCalendarView(
            8 /*numberOfWeeksInView*/,
            false /*isInteractive*/, // Use 'true' for manual interactive testing
            false /*enableRoundedListViewBaseItemChrome*/);
    }

    void CalendarViewIntegrationTests::DisplayCalendarView(
        int numberOfWeeksInView,
        bool isInteractive,
        bool enableRoundedCalendarViewBaseItemChrome)
    {
        RoundedChromeDictionaryHelper dictionaryHelper(enableRoundedCalendarViewBaseItemChrome);

        xaml_controls::CheckBox^ chkExit = nullptr;
        xaml_controls::CheckBox^ chkEnableRoundedCalendarViewBaseItemChrome = nullptr;

        xaml_controls::CheckBox^ chkIsEnabled = nullptr;
        xaml_controls::CheckBox^ chkIsTodayHighlighted = nullptr;
        xaml_controls::CheckBox^ chkIsTodayBlackedOut = nullptr;
        xaml_controls::CheckBox^ chkIsSundayBlackedOut = nullptr;
        xaml_controls::CheckBox^ chkIsGroupLabelVisible = nullptr;
        xaml_controls::CheckBox^ chkIsOutOfScopeEnabled = nullptr;
        xaml_controls::CheckBox^ chkHasDensityBars = nullptr;

        xaml_controls::CalendarView^ calendarView = nullptr;

        auto cvDayItemChangingRegistration = CreateSafeEventRegistration(xaml_controls::CalendarView, CalendarViewDayItemChanging);
        auto chkIsEnabledCheckedRegistration = CreateSafeEventRegistration(xaml_controls::CheckBox, Checked);
        auto chkIsEnabledUncheckedRegistration = CreateSafeEventRegistration(xaml_controls::CheckBox, Unchecked);
        auto chkIsTodayHighlightedCheckedRegistration = CreateSafeEventRegistration(xaml_controls::CheckBox, Checked);
        auto chkIsTodayHighlightedUncheckedRegistration = CreateSafeEventRegistration(xaml_controls::CheckBox, Unchecked);
        auto chkIsTodayBlackedOutCheckedRegistration = CreateSafeEventRegistration(xaml_controls::CheckBox, Checked);
        auto chkIsTodayBlackedOutUncheckedRegistration = CreateSafeEventRegistration(xaml_controls::CheckBox, Unchecked);
        auto chkIsSundayBlackedOutCheckedRegistration = CreateSafeEventRegistration(xaml_controls::CheckBox, Checked);
        auto chkIsSundayBlackedOutUncheckedRegistration = CreateSafeEventRegistration(xaml_controls::CheckBox, Unchecked);
        auto chkIsGroupLabelVisibleCheckedRegistration = CreateSafeEventRegistration(xaml_controls::CheckBox, Checked);
        auto chkIsGroupLabelVisibleUncheckedRegistration = CreateSafeEventRegistration(xaml_controls::CheckBox, Unchecked);
        auto chkIsOutOfScopeEnabledCheckedRegistration = CreateSafeEventRegistration(xaml_controls::CheckBox, Checked);
        auto chkIsOutOfScopeEnabledUncheckedRegistration = CreateSafeEventRegistration(xaml_controls::CheckBox, Unchecked);
        auto chkHasDensityBarsCheckedRegistration = CreateSafeEventRegistration(xaml_controls::CheckBox, Checked);
        auto chkHasDensityBarsUncheckedRegistration = CreateSafeEventRegistration(xaml_controls::CheckBox, Unchecked);

        auto rootPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"DisplayCalendarView.xaml"));
        VERIFY_IS_NOT_NULL(rootPanel);

        RunOnUIThread([&]()
        {
            chkExit = safe_cast<xaml_controls::CheckBox^>(rootPanel->FindName(L"chkExit"));
            VERIFY_IS_NOT_NULL(chkExit);

            chkEnableRoundedCalendarViewBaseItemChrome = safe_cast<xaml_controls::CheckBox^>(rootPanel->FindName(L"chkEnableRoundedCalendarViewBaseItemChrome"));
            VERIFY_IS_NOT_NULL(chkEnableRoundedCalendarViewBaseItemChrome);

            chkIsEnabled = safe_cast<xaml_controls::CheckBox^>(rootPanel->FindName(L"chkIsEnabled"));
            VERIFY_IS_NOT_NULL(chkIsEnabled);

            chkIsTodayHighlighted = safe_cast<xaml_controls::CheckBox^>(rootPanel->FindName(L"chkIsTodayHighlighted"));
            VERIFY_IS_NOT_NULL(chkIsTodayHighlighted);

            chkIsTodayBlackedOut = safe_cast<xaml_controls::CheckBox^>(rootPanel->FindName(L"chkIsTodayBlackedOut"));
            VERIFY_IS_NOT_NULL(chkIsTodayBlackedOut);

            chkIsSundayBlackedOut = safe_cast<xaml_controls::CheckBox^>(rootPanel->FindName(L"chkIsSundayBlackedOut"));
            VERIFY_IS_NOT_NULL(chkIsSundayBlackedOut);

            chkIsGroupLabelVisible = safe_cast<xaml_controls::CheckBox^>(rootPanel->FindName(L"chkIsGroupLabelVisible"));
            VERIFY_IS_NOT_NULL(chkIsGroupLabelVisible);

            chkIsOutOfScopeEnabled = safe_cast<xaml_controls::CheckBox^>(rootPanel->FindName(L"chkIsOutOfScopeEnabled"));
            VERIFY_IS_NOT_NULL(chkIsOutOfScopeEnabled);

            chkHasDensityBars = safe_cast<xaml_controls::CheckBox^>(rootPanel->FindName(L"chkHasDensityBars"));
            VERIFY_IS_NOT_NULL(chkHasDensityBars);

            calendarView = safe_cast<xaml_controls::CalendarView^>(rootPanel->FindName(L"calendarView"));
            VERIFY_IS_NOT_NULL(calendarView);

            cvDayItemChangingRegistration.Attach(calendarView, ref new ::Windows::Foundation::TypedEventHandler<xaml_controls::CalendarView^, xaml_controls::CalendarViewDayItemChangingEventArgs^>(
                [&](xaml_controls::CalendarView^, xaml_controls::CalendarViewDayItemChangingEventArgs^ e)
                {
                    // Render basic day items.
                    if (e->Phase == 0 && (chkIsTodayBlackedOut->IsChecked->Value || chkIsSundayBlackedOut->IsChecked->Value || chkHasDensityBars->IsChecked->Value))
                    {
                        // Register callback for next phase.
                        e->RegisterUpdateCallback(
                            ref new ::Windows::Foundation::TypedEventHandler<xaml_controls::CalendarView^, xaml_controls::CalendarViewDayItemChangingEventArgs^>(
                            [&](xaml_controls::CalendarView^, xaml_controls::CalendarViewDayItemChangingEventArgs^ e)
                            {
                                // Set blackout dates.
                                VERIFY_IS_TRUE(e->Phase == 1);
                                SetBlackout(e->Item, chkIsTodayBlackedOut->IsChecked->Value, chkIsSundayBlackedOut->IsChecked->Value);

                                if (chkHasDensityBars->IsChecked->Value)
                                {
                                    // Register callback for next phase.
                                    e->RegisterUpdateCallback(
                                        ref new ::Windows::Foundation::TypedEventHandler<xaml_controls::CalendarView^, xaml_controls::CalendarViewDayItemChangingEventArgs^>(
                                        [&](xaml_controls::CalendarView^, xaml_controls::CalendarViewDayItemChangingEventArgs^ e)
                                        {
                                            // Set density bars.
                                            VERIFY_IS_TRUE(e->Phase == 2);
                                            SetDensityColors(e->Item, chkHasDensityBars->IsChecked->Value);
                                        }));
                                }
                            }));
                    }
                }));

            chkIsEnabledCheckedRegistration.Attach(chkIsEnabled, ref new xaml::RoutedEventHandler(
                [&](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    calendarView->IsEnabled = true;
                }));

            chkIsEnabledUncheckedRegistration.Attach(chkIsEnabled, ref new xaml::RoutedEventHandler(
                [&](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    calendarView->IsEnabled = false;
                }));

            chkIsTodayHighlightedCheckedRegistration.Attach(chkIsTodayHighlighted, ref new xaml::RoutedEventHandler(
                [&](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    calendarView->IsTodayHighlighted = true;
                }));

            chkIsTodayHighlightedUncheckedRegistration.Attach(chkIsTodayHighlighted, ref new xaml::RoutedEventHandler(
                [&](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    calendarView->IsTodayHighlighted = false;
                }));

            chkIsGroupLabelVisibleCheckedRegistration.Attach(chkIsGroupLabelVisible, ref new xaml::RoutedEventHandler(
                [&](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    calendarView->IsGroupLabelVisible = true;
                }));

            chkIsGroupLabelVisibleUncheckedRegistration.Attach(chkIsGroupLabelVisible, ref new xaml::RoutedEventHandler(
                [&](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    calendarView->IsGroupLabelVisible = false;
                }));

            chkIsOutOfScopeEnabledCheckedRegistration.Attach(chkIsOutOfScopeEnabled, ref new xaml::RoutedEventHandler(
                [&](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    calendarView->IsOutOfScopeEnabled = true;
                }));

            chkIsOutOfScopeEnabledUncheckedRegistration.Attach(chkIsOutOfScopeEnabled, ref new xaml::RoutedEventHandler(
                [&](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    calendarView->IsOutOfScopeEnabled = false;
                }));

            chkIsTodayBlackedOutCheckedRegistration.Attach(chkIsTodayBlackedOut, ref new xaml::RoutedEventHandler(
                [&](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    SetBlackouts(calendarView, true, chkIsSundayBlackedOut->IsChecked->Value);
                }));

            chkIsTodayBlackedOutUncheckedRegistration.Attach(chkIsTodayBlackedOut, ref new xaml::RoutedEventHandler(
                [&](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    SetBlackouts(calendarView, false, chkIsSundayBlackedOut->IsChecked->Value);
                }));

            chkIsSundayBlackedOutCheckedRegistration.Attach(chkIsSundayBlackedOut, ref new xaml::RoutedEventHandler(
                [&](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    SetBlackouts(calendarView, chkIsTodayBlackedOut->IsChecked->Value, true);
                }));

            chkIsSundayBlackedOutUncheckedRegistration.Attach(chkIsSundayBlackedOut, ref new xaml::RoutedEventHandler(
                [&](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    SetBlackouts(calendarView, chkIsTodayBlackedOut->IsChecked->Value, false);
                }));

            chkHasDensityBarsCheckedRegistration.Attach(chkHasDensityBars, ref new xaml::RoutedEventHandler(
                [&](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    SetDensityColors(calendarView, true);
                }));

            chkHasDensityBarsUncheckedRegistration.Attach(chkHasDensityBars, ref new xaml::RoutedEventHandler(
                [&](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    SetDensityColors(calendarView, false);
                }));

            chkEnableRoundedCalendarViewBaseItemChrome->IsChecked = enableRoundedCalendarViewBaseItemChrome;

            LOG_OUTPUT(L"WindowContent setter");
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Changing NumberOfWeeksInView to %d.", numberOfWeeksInView);
            calendarView->NumberOfWeeksInView = numberOfWeeksInView;
        });
        TestServices::WindowHelper->WaitForIdle();

        if (isInteractive)
        {
            LOG_OUTPUT(L"Waiting exit");
            bool exit = false;
            do
            {
                TestServices::WindowHelper->SynchronouslyTickUIThread(50);

                RunOnUIThread([&]()
                {
                    exit = chkExit->IsChecked->Value;
                });
            } while (!exit);

            TestServices::WindowHelper->WaitForIdle();
        }
        else
        {
            RunOnUIThread([&]()
            {
                calendarView->DisplayMode = xaml_controls::CalendarViewDisplayMode::Year;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                calendarView->DisplayMode = xaml_controls::CalendarViewDisplayMode::Decade;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                calendarView->DisplayMode = xaml_controls::CalendarViewDisplayMode::Month;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                calendarView->IsTodayHighlighted = false;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                calendarView->IsTodayHighlighted = true;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                calendarView->IsEnabled = false;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                calendarView->IsEnabled = true;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                calendarView->IsGroupLabelVisible = false;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                calendarView->IsGroupLabelVisible = true;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                calendarView->IsOutOfScopeEnabled = false;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                calendarView->IsOutOfScopeEnabled = true;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                SetBlackouts(calendarView, true, false);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                SetBlackouts(calendarView, false, true);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                SetDensityColors(calendarView, true);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                SetDensityColors(calendarView, false);
            });
            TestServices::WindowHelper->WaitForIdle();
        }
    }

    void CalendarViewIntegrationTests::SetBlackouts(xaml_controls::CalendarView^ calendarView, bool isTodayBlackedOut, bool isSundayBlackedOut)
    {
        VERIFY_IS_NOT_NULL(calendarView);

        auto calendarPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(CalendarHelper::GetTemplateChild(calendarView, L"MonthViewPanel"));
        VERIFY_IS_NOT_NULL(calendarPanel);

        for (unsigned child = 0; child < calendarPanel->Children->Size; child++)
        {
            auto dayItem = safe_cast<xaml_controls::CalendarViewDayItem^>(calendarPanel->Children->GetAt(child));
            SetBlackout(dayItem, isTodayBlackedOut, isSundayBlackedOut);
        }
    }

    void CalendarViewIntegrationTests::SetBlackout(xaml_controls::CalendarViewDayItem^ dayItem, bool isTodayBlackedOut, bool isSundayBlackedOut)
    {
        VERIFY_IS_NOT_NULL(dayItem);

        // Blackout Sundays and/or Today.

        auto itemCalendar = ref new wg::Calendar();
        itemCalendar->SetDateTime(dayItem->Date);

        dayItem->IsBlackout = isSundayBlackedOut && itemCalendar->DayOfWeek == ::Windows::Globalization::DayOfWeek::Sunday;

        if (!dayItem->IsBlackout && isTodayBlackedOut)
        {
            auto today = ref new wg::Calendar();
            today->SetToNow();

            dayItem->IsBlackout =
                itemCalendar->Era == today->Era &&
                itemCalendar->Year == today->Year &&
                itemCalendar->Month == today->Month &&
                itemCalendar->Day == today->Day;
        }
    }

    void CalendarViewIntegrationTests::SetDensityColors(xaml_controls::CalendarView^ calendarView, bool hasDensityBars)
    {
        VERIFY_IS_NOT_NULL(calendarView);

        auto calendarPanel = safe_cast<xaml_controls::Primitives::CalendarPanel^>(CalendarHelper::GetTemplateChild(calendarView, L"MonthViewPanel"));
        VERIFY_IS_NOT_NULL(calendarPanel);

        for (unsigned child = 0; child < calendarPanel->Children->Size; child++)
        {
            auto dayItem = safe_cast<xaml_controls::CalendarViewDayItem^>(calendarPanel->Children->GetAt(child));
            SetDensityColors(dayItem, hasDensityBars);
        }
    }

    void CalendarViewIntegrationTests::SetDensityColors(xaml_controls::CalendarViewDayItem^ dayItem, bool hasDensityBars)
    {
        VERIFY_IS_NOT_NULL(dayItem);

        if (hasDensityBars)
        {
            auto itemCalendar = ref new wg::Calendar();
            itemCalendar->SetDateTime(dayItem->Date);

            auto today = ref new wg::Calendar();
            today->SetToNow();

            bool isToday =
                itemCalendar->Era == today->Era &&
                itemCalendar->Year == today->Year &&
                itemCalendar->Month == today->Month &&
                itemCalendar->Day == today->Day;

            if (itemCalendar->Day % 6 == 0 || isToday)
            {
                CalendarHelper::ColorCollection^ densityColors = ref new CalendarHelper::ColorCollection();

                densityColors->Append(Microsoft::UI::Colors::Colors::Green);
                densityColors->Append(Microsoft::UI::Colors::Colors::Green);

                if (itemCalendar->Day % 4 == 0 || isToday)
                {
                    densityColors->Append(Microsoft::UI::Colors::Colors::Blue);
                    densityColors->Append(Microsoft::UI::Colors::Colors::Blue);
                }
                if (itemCalendar->Day % 9 == 0 || isToday)
                {
                    densityColors->Append(Microsoft::UI::Colors::Colors::Orange);
                }
                if (isToday)
                {
                    densityColors->Append(Microsoft::UI::Colors::Colors::Red);
                    densityColors->Append(Microsoft::UI::Colors::Colors::Yellow);
                }

                dayItem->SetDensityColors(densityColors);
            }
        }
        else
        {
            dayItem->SetDensityColors(nullptr);
        }
    }

    Platform::String^ CalendarViewIntegrationTests::GetResourcesPath() const
    {
        return GetPackageFolder() + L"resources\\native\\enterprise\\calendarview\\";
    }
} } } } } } // Microsoft::UI::Xaml::Tests::Enterprise::CalendarView






