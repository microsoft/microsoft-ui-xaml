// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlTailored.h>
#include "calendarHelper.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common { 

    class CalendarDatePickerHelper
    {
    public:
        CalendarDatePickerHelper()
            : m_loadedEvent(new Event())
            , m_selectedDatesChangedEvent(new Event())
            , m_cicEvent(new Event())
            , m_openedEvent(new Event())
            , m_closedEvent(new Event())
            , m_addedDates(ref new Microsoft::UI::Xaml::Tests::Common::CalendarHelper::DateCollection())
            , m_removedDates(ref new Microsoft::UI::Xaml::Tests::Common::CalendarHelper::DateCollection())
            , m_loadedRegistration(CreateSafeEventRegistration(Microsoft::UI::Xaml::Controls::CalendarDatePicker, Loaded))
            , m_selectedDatesChangedRegistration(CreateSafeEventRegistration(Microsoft::UI::Xaml::Controls::CalendarDatePicker, DateChanged))
            , m_cicRegistration(CreateSafeEventRegistration(Microsoft::UI::Xaml::Controls::CalendarDatePicker, CalendarViewDayItemChanging))
            , m_openedRegistration(CreateSafeEventRegistration(xaml_controls::CalendarDatePicker, Opened))
            , m_closedRegistration(CreateSafeEventRegistration(xaml_controls::CalendarDatePicker, Closed))
        {
            RunOnUIThread([&]()
            {
                m_cp = ref new Microsoft::UI::Xaml::Controls::CalendarDatePicker();
                VERIFY_IS_NOT_NULL(m_cp);
            });
        }

        Microsoft::UI::Xaml::Controls::CalendarDatePicker^ GetCalendarDatePicker()
        {
            return m_cp;
        }

        xaml::DependencyObject^ GetTemplateChild(Platform::String^ childName)
        {
            return GetTemplateChild(m_cp, childName);
        }

        xaml::DependencyObject^ GetTemplateChild(xaml::DependencyObject^ root, Platform::String^ childName)
        {
            return CalendarHelper::GetTemplateChild(root, childName);
        }


        void PrepareLoadedEvent()
        {
            RunOnUIThread([&]()
            {
                m_loadedRegistration.Attach(
                    m_cp,
                    ref new xaml::RoutedEventHandler([this](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
                {
                    return OnLoaded(sender, e);
                }));
            });
        }

        void PrepareOpenedEvent()
        {
            RunOnUIThread([&]()
            {
                m_openedRegistration.Attach(
                    m_cp,
                    ref new ::Windows::Foundation::EventHandler<Platform::Object^>([this](Platform::Object ^sender, Platform::Object ^e)
                {
                    return OnOpened(sender, e);
                }));
            });
        }

        void PrepareClosedEvent()
        {
            RunOnUIThread([&]()
            {
                m_closedRegistration.Attach(
                    m_cp,
                    ref new ::Windows::Foundation::EventHandler<Platform::Object^>([this](Platform::Object ^sender, Platform::Object ^e)
                {
                    return OnClosed(sender, e);
                }));
            });
        }

        void PrepareSelectedDateChangedEvent()
        {
            m_addedDates->Clear();
            m_removedDates->Clear();

            RunOnUIThread([&]()
            {
                m_selectedDatesChangedRegistration.Attach(
                    m_cp,
                    ref new ::Windows::Foundation::TypedEventHandler<Microsoft::UI::Xaml::Controls::CalendarDatePicker^, Microsoft::UI::Xaml::Controls::CalendarDatePickerDateChangedEventArgs^>(
                    [this](Microsoft::UI::Xaml::Controls::CalendarDatePicker ^sender, Microsoft::UI::Xaml::Controls::CalendarDatePickerDateChangedEventArgs ^e)
                {
                    return OnCalendarViewSelectedDateChanged(sender, e);
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
        // the best position is before CalendarDatePicker enters visual tree.
        void PrepareCICEvent()
        {
            RunOnUIThread([&]()
            {
                m_cicRegistration.Attach(
                    m_cp,
                    ref new Microsoft::UI::Xaml::Controls::CalendarViewDayItemChangingEventHandler(
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

        void WaitForOpened()
        {
            m_openedEvent->WaitForDefault();
            VERIFY_IS_TRUE(m_openedEvent->HasFired());
            m_openedEvent->Reset();
            m_openedRegistration.Detach();
        }

        void WaitForClosed()
        {
            m_closedEvent->WaitForDefault();
            VERIFY_IS_TRUE(m_closedEvent->HasFired());
            m_closedEvent->Reset();
            m_closedRegistration.Detach();
        }

        void WaitForSelectedDatesChanged()
        {
            m_selectedDatesChangedEvent->WaitForDefault();
            VERIFY_IS_TRUE(m_selectedDatesChangedEvent->HasFired());
            m_selectedDatesChangedEvent->Reset();
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
            LOG_OUTPUT(L"CalendarDatePickerIntegrationTests: CalendarDatePicker Loaded.");
            m_loadedEvent->Set();
        }

        void OnOpened(Platform::Object ^sender, Platform::Object ^e)
        {
            LOG_OUTPUT(L"CalendarDatePickerIntegrationTests: CalendarDatePicker CalendarView flyout Opened.");
            m_openedEvent->Set();
        }

        void OnClosed(Platform::Object ^sender, Platform::Object ^e)
        {
            LOG_OUTPUT(L"CalendarDatePickerIntegrationTests: CalendarDatePicker CalendarView flyout Closed.");
            m_closedEvent->Set();
        }

        void OnCalendarViewSelectedDateChanged(Microsoft::UI::Xaml::Controls::CalendarDatePicker ^sender, Microsoft::UI::Xaml::Controls::CalendarDatePickerDateChangedEventArgs ^e)
        {
            unsigned addedSize = e->NewDate ? 1 : 0;
            unsigned removedSize = e->OldDate ? 1 : 0;
            VERIFY_ARE_EQUAL(m_addedDates->Size, addedSize);
            VERIFY_ARE_EQUAL(m_removedDates->Size, removedSize);

            if (e->NewDate)
            {
                VERIFY_ARE_EQUAL(e->NewDate->Value.UniversalTime, m_addedDates->GetAt(0).UniversalTime);
            }

            if (e->OldDate)
            {
                VERIFY_ARE_EQUAL(e->OldDate->Value.UniversalTime, m_removedDates->GetAt(0).UniversalTime);
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
                Microsoft::UI::Xaml::Tests::Common::CalendarHelper::ColorCollection^ colors = ref new Microsoft::UI::Xaml::Tests::Common::CalendarHelper::ColorCollection();
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



    private:
        Microsoft::UI::Xaml::Controls::CalendarDatePicker^ m_cp;

        SafeEventRegistration<Microsoft::UI::Xaml::Controls::CalendarDatePicker, xaml::RoutedEventHandler> m_loadedRegistration;
        SafeEventRegistration<Microsoft::UI::Xaml::Controls::CalendarDatePicker, ::Windows::Foundation::TypedEventHandler<Microsoft::UI::Xaml::Controls::CalendarDatePicker^, Microsoft::UI::Xaml::Controls::CalendarDatePickerDateChangedEventArgs^>> m_selectedDatesChangedRegistration;
        SafeEventRegistration<Microsoft::UI::Xaml::Controls::CalendarDatePicker, Microsoft::UI::Xaml::Controls::CalendarViewDayItemChangingEventHandler> m_cicRegistration;
        SafeEventRegistration<Microsoft::UI::Xaml::Controls::CalendarDatePicker, ::Windows::Foundation::EventHandler<Platform::Object^>> m_openedRegistration;
        SafeEventRegistration<Microsoft::UI::Xaml::Controls::CalendarDatePicker, ::Windows::Foundation::EventHandler<Platform::Object^>> m_closedRegistration;
        std::unique_ptr<Microsoft::UI::Xaml::Tests::Common::Event> m_loadedEvent;
        std::unique_ptr<Microsoft::UI::Xaml::Tests::Common::Event> m_selectedDatesChangedEvent;
        std::unique_ptr<Microsoft::UI::Xaml::Tests::Common::Event> m_cicEvent;
        std::unique_ptr<Microsoft::UI::Xaml::Tests::Common::Event> m_openedEvent;
        std::unique_ptr<Microsoft::UI::Xaml::Tests::Common::Event> m_closedEvent;
        Microsoft::UI::Xaml::Tests::Common::CalendarHelper::DateCollection^ m_addedDates;
        Microsoft::UI::Xaml::Tests::Common::CalendarHelper::DateCollection^ m_removedDates;
    };

} } } } }
