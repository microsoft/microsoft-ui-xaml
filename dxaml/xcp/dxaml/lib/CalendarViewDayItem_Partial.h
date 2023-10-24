// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "calendarviewdayitem.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(CalendarViewDayItem)
    {
    public:

        // Handle the custom property changed event and call the OnPropertyChanged2 methods.
        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        _Check_return_ HRESULT GetBuildTreeArgs(_Out_ ctl::ComPtr<xaml_controls::ICalendarViewDayItemChangingEventArgs>* pspArgs);

        // Called when a pointer makes a tap gesture on a CalendarViewBaseItem.
        IFACEMETHOD(OnTapped)(_In_ xaml_input::ITappedRoutedEventArgs* pArgs) override;

        // Handles when a key is pressed down on the CalendarView.
        IFACEMETHOD(OnKeyDown)(_In_ xaml_input::IKeyRoutedEventArgs* pArgs) override;

#if DBG
        _Check_return_ HRESULT put_Date(_In_ wf::DateTime value) override;
#endif

        _Check_return_ HRESULT GetDate(_Out_ wf::DateTime* pDate) override { return get_Date(pDate); }

        virtual _Check_return_ HRESULT SetDensityColorsImpl(_In_opt_ wfc::IIterable<wu::Color>* pColors);
    protected:
        // Override OnCreateAutomationPeer()
        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_result_maybenull_ xaml_automation_peers::IAutomationPeer** returnValue);

    private:
        // Returns False because when rounded styling is applied, day items do not use hard-coded margins but a Style for the CalendarViewDayItem type.
        bool HasStaticRoundedItemMargin() const override { return false; }
        // Not expected to be invoked since HasStaticRoundedItemMargin() returns False.
        xaml::Thickness GetStaticRoundedItemMargin() const override { ASSERT(false); return {}; }

        TrackerPtr<xaml_controls::ICalendarViewDayItemChangingEventArgs> m_tpBuildTreeArgs;
    };
}
