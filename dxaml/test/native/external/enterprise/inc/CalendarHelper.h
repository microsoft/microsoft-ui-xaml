// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlTailored.h>
#include <tuple>

using namespace std;
using namespace std::placeholders;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common { namespace CalendarHelper {

    static std::tuple<int, int, int, int> GetDateFromDateTime(::Windows::Foundation::DateTime datetime)
    {
        std::tuple<int, int, int, int> date;
        ::Windows::Globalization::Calendar^ calendar = ref new ::Windows::Globalization::Calendar();
        calendar->SetDateTime(datetime);
        std::get<0>(date) = calendar->Era;
        std::get<1>(date) = calendar->Year;
        std::get<2>(date) = calendar->Month;
        std::get<3>(date) = calendar->Day;
        return date;
    }

    struct CompareDate
    {
        bool operator ()(const ::Windows::Foundation::DateTime& lhs, const ::Windows::Foundation::DateTime& rhs)
        {
            auto ldate = GetDateFromDateTime(lhs);
            auto rdate = GetDateFromDateTime(rhs);

            return ldate == rdate;
        }
    };

    struct CompareColor
    {
        bool operator ()(const ::Windows::UI::Color& lhs, const ::Windows::UI::Color& rhs)
        {
            return lhs.A == rhs.A
                && lhs.R == rhs.R
                && lhs.G == rhs.G
                && lhs.B == rhs.B;
        }
    };

    typedef Platform::Collections::Vector<::Windows::Foundation::DateTime, CompareDate> DateCollection;
    typedef Platform::Collections::Vector<::Windows::UI::Color, CompareColor> ColorCollection;


    static xaml::DependencyObject^ GetTemplateChild(xaml::DependencyObject^ root, Platform::String^ childName)
    {

        int count = xaml_media::VisualTreeHelper::GetChildrenCount(root);
        for (int i = 0; i < count; i++)
        {
            auto child = xaml_media::VisualTreeHelper::GetChild(root, i);
            xaml::FrameworkElement^ childAsFE = safe_cast<xaml::FrameworkElement^>(child);
            if (childAsFE->Name == childName)
                return child;
            auto result = GetTemplateChild(child, childName);
            if (result != nullptr)
                return result;
        }
        return nullptr;
    }

    static ::Windows::Foundation::DateTime ConvertToDateTime(int era, int year, int month, int day, int period = 1, int hour = 12, int minute = 0, int second = 0, int nanosecond = 0)
    {
        ::Windows::Globalization::Calendar^ calendar = ref new ::Windows::Globalization::Calendar();
        calendar->Era = era;
        calendar->Year = year;
        calendar->Month = month;
        calendar->Day = day;
        calendar->Period = period;
        calendar->Hour = hour;
        calendar->Minute = minute;
        calendar->Second = second;
        calendar->Nanosecond = nanosecond;
        return calendar->GetDateTime();
    }

    static void CreateTestResources(xaml_controls::Grid^& rootPanel)
    {
        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      Width='400' Height='400' VerticalAlignment='Top' HorizontalAlignment='Left' Background='Navy'/> "));

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });
    }

    struct CompareDateTime
    {
        bool operator ()(const ::Windows::Foundation::DateTime& lhs, const ::Windows::Foundation::DateTime& rhs)
        {
            return lhs.UniversalTime == rhs.UniversalTime;
        }
    };

    class CalendarViewHelper
    {
    public:
        CalendarViewHelper()
            : m_loadedEvent(new Event())
            , m_selectedDatesChangedEvent(new Event())
            , m_cicEvent(new Event())
            , m_addedDates(ref new DateCollection())
            , m_removedDates(ref new DateCollection())
            , m_loadedRegistration(CreateSafeEventRegistration(Microsoft::UI::Xaml::Controls::CalendarView, Loaded))
            , m_selectedDatesChangedRegistration(CreateSafeEventRegistration(Microsoft::UI::Xaml::Controls::CalendarView, SelectedDatesChanged))
            , m_cicRegistration(CreateSafeEventRegistration(Microsoft::UI::Xaml::Controls::CalendarView, CalendarViewDayItemChanging))
        {
            RunOnUIThread([&]()
            {
                m_cv = ref new Microsoft::UI::Xaml::Controls::CalendarView();
                VERIFY_IS_NOT_NULL(m_cv);
            });
        }

        Microsoft::UI::Xaml::Controls::CalendarView^ GetCalendarView()
        {
            return m_cv;
        }

        xaml::DependencyObject^ GetTemplateChild(Platform::String^ childName)
        {
            return GetTemplateChild(m_cv, childName);
        }

        void PrepareLoadedEvent()
        {
            RunOnUIThread([&]()
            {
                m_loadedRegistration.Attach(
                    m_cv,
                    ref new xaml::RoutedEventHandler([this](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
                {
                    return OnLoaded(sender, e);
                }));
            });
        }

        void PrepareSelectedDatesChangedEvent()
        {
            m_addedDates->Clear();
            m_removedDates->Clear();

            RunOnUIThread([&]()
            {
                m_selectedDatesChangedRegistration.Attach(
                    m_cv,
                    ref new ::Windows::Foundation::TypedEventHandler<Microsoft::UI::Xaml::Controls::CalendarView^, Microsoft::UI::Xaml::Controls::CalendarViewSelectedDatesChangedEventArgs^>(
                    [this](Microsoft::UI::Xaml::Controls::CalendarView ^sender, Microsoft::UI::Xaml::Controls::CalendarViewSelectedDatesChangedEventArgs ^e)
                {
                    return OnSelectedDatesChanged(sender, e);
                }));
            });
        }

        void ExpectAddedDate(::Windows::Foundation::DateTime date)
        {
            m_addedDates->Append(date);
        }

        void ExpectRemovedDate(::Windows::Foundation::DateTime date)
        {
            m_removedDates->Append(date);
        }

        // note: PrepareCICEvent must be called before item get realized
        // the best position is before CalendarView enters visual tree.
        void PrepareCICEvent()
        {
            RunOnUIThread([&]()
            {
                m_cicRegistration.Attach(
                    m_cv,
                    ref new ::Windows::Foundation::TypedEventHandler<Microsoft::UI::Xaml::Controls::CalendarView^, Microsoft::UI::Xaml::Controls::CalendarViewDayItemChangingEventArgs^>(
                    [this](Microsoft::UI::Xaml::Controls::CalendarView^ sender, Microsoft::UI::Xaml::Controls::CalendarViewDayItemChangingEventArgs^ e)
                {
                    return OnCalendarViewDayItemChanging(sender, e);
                }));
            });
        }

        void WaitForLoaded()
        {
            m_loadedEvent->WaitForDefault();
            VERIFY_IS_TRUE(m_loadedEvent->HasFired());
            m_loadedEvent->Reset();
            m_loadedRegistration.Detach();
        }

        void WaitForSelectedDatesChanged()
        {
            m_selectedDatesChangedEvent->WaitForDefault();
            VERIFY_IS_TRUE(m_selectedDatesChangedEvent->HasFired());
            m_selectedDatesChangedEvent->Reset();
            m_selectedDatesChangedRegistration.Detach();
        }

        void VerifyNoSelectedDatesChanged()
        {
            // we expect no event here, so below statement will timeout and throw WEX::Common::Exception.
            VERIFY_THROWS_WINRT(m_selectedDatesChangedEvent->WaitFor(std::chrono::milliseconds(5000), true /*enforceUnderDebugger*/),
                WEX::Common::Exception,
                L"SelectedDatesChanged event should not raise!");
            VERIFY_IS_FALSE(m_selectedDatesChangedEvent->HasFired());
            m_selectedDatesChangedRegistration.Detach();
        }

        void WaitForCICEvent()
        {
            m_cicEvent->WaitForDefault();
            VERIFY_IS_TRUE(m_cicEvent->HasFired());
            m_cicEvent->Reset();
            m_cicRegistration.Detach();
        }

    private:
        void OnLoaded(Platform::Object ^sender, Microsoft::UI::Xaml::RoutedEventArgs ^e)
        {
            LOG_OUTPUT(L"CalendarViewIntegrationTests: CalendarView Loaded.");
            m_loadedEvent->Set();
        }

        void OnSelectedDatesChanged(Microsoft::UI::Xaml::Controls::CalendarView ^sender, Microsoft::UI::Xaml::Controls::CalendarViewSelectedDatesChangedEventArgs ^e)
        {
            VERIFY_ARE_EQUAL(e->AddedDates->Size, m_addedDates->Size);
            VERIFY_ARE_EQUAL(e->RemovedDates->Size, m_removedDates->Size);

            for (unsigned i = 0; i < e->AddedDates->Size; ++i)
            {
                VERIFY_ARE_EQUAL(e->AddedDates->GetAt(i).UniversalTime, m_addedDates->GetAt(i).UniversalTime);
            }

            for (unsigned i = 0; i < e->RemovedDates->Size; ++i)
            {
                VERIFY_ARE_EQUAL(e->RemovedDates->GetAt(i).UniversalTime, m_removedDates->GetAt(i).UniversalTime);
            }
            m_selectedDatesChangedEvent->Set();
        }

        void OnCalendarViewDayItemChanging(Microsoft::UI::Xaml::Controls::CalendarView ^sender, Microsoft::UI::Xaml::Controls::CalendarViewDayItemChangingEventArgs ^e)
        {
            // phase 2: set density bar
            // phase 5: blackout
            // phase 7: end cic event

            if (e->Phase == 2)
            {
                ColorCollection^ colors = ref new ColorCollection();
                colors->Append(Microsoft::UI::Colors::Red);
                colors->Append(Microsoft::UI::Colors::Green);
                colors->Append(Microsoft::UI::Colors::Blue);
                colors->Append(Microsoft::UI::Colors::Yellow);
                e->Item->SetDensityColors(colors);
            }
            else if (e->Phase == 5)
            {
                e->Item->IsBlackout = true;
            }
            else if (e->Phase == 7)
            {
                m_cicEvent->Set();
            }

            // keep subscribing cic event until phase 7.
            if (e->Phase < 7)
            {
                e->RegisterUpdateCallback(
                    ref new ::Windows::Foundation::TypedEventHandler<Microsoft::UI::Xaml::Controls::CalendarView^, Microsoft::UI::Xaml::Controls::CalendarViewDayItemChangingEventArgs^>(
                    [this](Microsoft::UI::Xaml::Controls::CalendarView^ sender, Microsoft::UI::Xaml::Controls::CalendarViewDayItemChangingEventArgs^ e)
                {
                    return OnCalendarViewDayItemChanging(sender, e);
                }));
            }
        }

        xaml::DependencyObject^ GetTemplateChild(xaml::DependencyObject^ root, Platform::String^ childName)
        {

            int count = xaml_media::VisualTreeHelper::GetChildrenCount(root);
            for (int i = 0; i < count; i++)
            {
                auto child = xaml_media::VisualTreeHelper::GetChild(root, i);
                xaml::FrameworkElement^ childAsFE = safe_cast<xaml::FrameworkElement^>(child);
                if (childAsFE->Name == childName)
                    return child;
                auto result = GetTemplateChild(child, childName);
                if (result != nullptr)
                    return result;
            }
            return nullptr;
        }

    private:
        Microsoft::UI::Xaml::Controls::CalendarView^ m_cv;

        SafeEventRegistration<Microsoft::UI::Xaml::Controls::CalendarView, xaml::RoutedEventHandler> m_loadedRegistration;
        SafeEventRegistration<Microsoft::UI::Xaml::Controls::CalendarView, ::Windows::Foundation::TypedEventHandler<Microsoft::UI::Xaml::Controls::CalendarView^, Microsoft::UI::Xaml::Controls::CalendarViewSelectedDatesChangedEventArgs^>> m_selectedDatesChangedRegistration;
        SafeEventRegistration<Microsoft::UI::Xaml::Controls::CalendarView, ::Windows::Foundation::TypedEventHandler<Microsoft::UI::Xaml::Controls::CalendarView^, Microsoft::UI::Xaml::Controls::CalendarViewDayItemChangingEventArgs^>> m_cicRegistration;
        unique_ptr<Event> m_loadedEvent;
        unique_ptr<Event> m_selectedDatesChangedEvent;
        unique_ptr<Event> m_cicEvent;
        DateCollection^ m_addedDates;
        DateCollection^ m_removedDates;
    };


    static void DumpDate(::Windows::Foundation::DateTime date, Platform::String^ prefix)
    {
        ::Windows::Globalization::Calendar^ calendar = ref new ::Windows::Globalization::Calendar();
        calendar->SetDateTime(date);
        LOG_OUTPUT(L"%s: %d/%d/%d (dd/mm/yy)", prefix->Data(), calendar->Year, calendar->Month, calendar->Day);
    }

    static void VerifyDateTimesAreEqual(::Windows::Foundation::DateTime date1, ::Windows::Foundation::DateTime date2)
    {
        if (date1.UniversalTime != date2.UniversalTime)
        {
            ::Windows::Globalization::Calendar^ calendar = ref new ::Windows::Globalization::Calendar();
            calendar->SetDateTime(date1);
            LOG_OUTPUT(L"UTC %lld: %d/%d/%d %d:%d:%d %d", date1.UniversalTime, calendar->Year, calendar->Month, calendar->Day, calendar->Hour, calendar->Minute, calendar->Second, calendar->Nanosecond);
            calendar->SetDateTime(date2);
            LOG_OUTPUT(L"UTC %lld: %d/%d/%d %d:%d:%d %d", date2.UniversalTime, calendar->Year, calendar->Month, calendar->Day, calendar->Hour, calendar->Minute, calendar->Second, calendar->Nanosecond);
            VERIFY_ARE_EQUAL(date1.UniversalTime, date2.UniversalTime);
        }
    }

    static void CheckFocusedItem()
    {
        auto item = Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot);
        LOG_OUTPUT(L"Type of focused item is: %s", item->GetType()->FullName->Data());
        Microsoft::UI::Xaml::FrameworkElement^ itemAsFE = dynamic_cast<Microsoft::UI::Xaml::FrameworkElement^>(item);
        if (itemAsFE)
        {
            auto point = itemAsFE->TransformToVisual(nullptr)->TransformPoint(::Windows::Foundation::Point(0, 0));
            LOG_OUTPUT(L"Focused item position. x %f, y %f, width %f, height %f", point.X, point.Y, itemAsFE->ActualWidth, itemAsFE->ActualHeight);
        }
        Microsoft::UI::Xaml::Controls::CalendarViewDayItem^ itemAsDayItem = dynamic_cast<Microsoft::UI::Xaml::Controls::CalendarViewDayItem^>(item);
        if (itemAsDayItem)
        {
            LOG_OUTPUT(L"Focused item is a day item, date is %s", ::Windows::Globalization::DateTimeFormatting::DateTimeFormatter::ShortDate->Format(itemAsDayItem->Date)->Data());
        }
    };

    static bool AreClose(double a, double b, double threshold = 0.1)
    {
        LOG_OUTPUT(L"AreClose? %lf, %lf", a, b);
        return abs(a - b) <= threshold;
    }

    static void ReplaceAccentColorForTesting(Microsoft::UI::Xaml::Controls::CalendarView^ cv)
    {
        // replace the accent colors to some color constants to make sure test results are not affected by the accent color.
        cv->TodayForeground = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
        cv->SelectedBorderBrush = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
        cv->SelectedPressedBorderBrush = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Green);
        cv->SelectedHoverBorderBrush = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Blue);
    }


    static std::array<Platform::String^, 10> GetAllSupportedCalendarIdentifiers()
    {
        return std::array < Platform::String^, 10 > { {
                    L"PersianCalendar",
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
    }

} } } } } }
