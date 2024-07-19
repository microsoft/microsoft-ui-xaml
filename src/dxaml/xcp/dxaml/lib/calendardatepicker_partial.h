// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CalendarDatePicker.g.h"
#include <fwd/windows.globalization.h>

namespace DirectUI
{
    PARTIAL_CLASS(CalendarDatePicker)
    {
    protected:
        CalendarDatePicker();

        IFACEMETHOD(OnApplyTemplate)() override;

        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        _Check_return_ HRESULT PrepareState() override;

        _Check_return_ HRESULT ChangeVisualState(_In_ bool bUseTransitions) override;

        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue) override;

    public:
        _Check_return_ HRESULT GetCurrentFormattedDate(_Out_ HSTRING* value);

        _Check_return_ HRESULT SetYearDecadeDisplayDimensionsImpl(_In_ INT columns, _In_ INT rows);
        _Check_return_ HRESULT SetDisplayDateImpl(_In_ wf::DateTime date);

        IFACEMETHOD(OnKeyDown)(_In_ xaml_input::IKeyRoutedEventArgs* pArgs) override;

        IFACEMETHOD(OnPointerPressed)(_In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        IFACEMETHOD(OnPointerReleased)(_In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        IFACEMETHOD(OnPointerEntered)(_In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        IFACEMETHOD(OnPointerMoved)(_In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        IFACEMETHOD(OnPointerExited)(_In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        IFACEMETHOD(OnGotFocus)(_In_ xaml::IRoutedEventArgs* pArgs) override;

        IFACEMETHOD(OnLostFocus)(_In_ xaml::IRoutedEventArgs* pArgs) override;

        IFACEMETHOD(OnPointerCaptureLost)(_In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        // Handles the case where right tapped is raised by unhandled.
        _Check_return_ HRESULT OnRightTappedUnhandled(_In_ xaml_input::IRightTappedRoutedEventArgs* pArgs) override;

        // IsEnabled property changed handler.
        _Check_return_ HRESULT OnIsEnabledChanged(_In_ DirectUI::IsEnabledChangedEventArgs* pArgs) override;

        _Check_return_ HRESULT GetPlainText(_Out_ HSTRING* strPlainText) override;

    private:

        _Check_return_ HRESULT OnSelectedDatesChanged(
            _In_ xaml_controls::ICalendarView* pSender,
            _In_ xaml_controls::ICalendarViewSelectedDatesChangedEventArgs* pArgs);

        _Check_return_ HRESULT OnDateChanged(
            _In_ wf::IReference<wf::DateTime>* pOldDateReference,
            _In_ wf::IReference<wf::DateTime>* pNewDateReference);

        _Check_return_ HRESULT RaiseDateChanged(
            _In_ wf::IReference<wf::DateTime>* pOldDateReference,
            _In_ wf::IReference<wf::DateTime>* pNewDateReference);

        _Check_return_ HRESULT OnDateFormatChanged();

        _Check_return_ HRESULT FormatDate();

        _Check_return_ HRESULT UpdateCalendarVisibility();

        _Check_return_ HRESULT UpdateHeaderVisibility();

        _Check_return_ HRESULT IsEventSourceTarget(
            _In_ xaml::IRoutedEventArgs* pArgs,
            _Out_ bool* pIsEventSourceChildOfTarget);

        // The "target" area of a CalendarDatePicker excludes the header. Pointer and focus events will only
        // have effect if they originate from an element within the target area. This is a
        // helper function to make that determination.
        _Check_return_ HRESULT IsChildOfTarget(
            _In_opt_ xaml::IDependencyObject* pChild,
            _In_ bool doCacheResult,
            _Out_ bool* pIsChildOfTarget);

        // Contains the logic to be employed if we decide to handle pointer released.
        _Check_return_ HRESULT PerformPointerUpAction();

        // keep CalendarView.SelectedDates in sync with CalendarDatePicker.Date property
        _Check_return_ HRESULT SyncDate();

    private:
        TrackerPtr<xaml_controls::ICalendarView> m_tpCalendarView;
        TrackerPtr<xaml_controls::IContentPresenter> m_tpHeaderContentPresenter;
        TrackerPtr<xaml_controls::ITextBlock> m_tpDateText;
        TrackerPtr<xaml_controls::IGrid> m_tpRoot;
        TrackerPtr<xaml_primitives::IFlyoutBase> m_tpFlyout;

        ctl::EventPtr<FlyoutBaseOpenedEventCallback> m_epFlyoutOpenedHandler;
        ctl::EventPtr<FlyoutBaseClosedEventCallback> m_epFlyoutClosedHandler;
        ctl::EventPtr<CalendarViewCalendarViewDayItemChangingCallback> m_epCalendarViewCalendarViewDayItemChangingHandler;
        ctl::EventPtr<CalendarViewSelectedDatesChangedCallback> m_epCalendarViewSelectedDatesChangedHandler;

        int m_colsInYearDecadeView;
        int m_rowsInYearDecadeView;

        wf::DateTime m_displayDate;

        TrackerPtr<wg::DateTimeFormatting::IDateTimeFormatter> m_tpDateFormatter;


        // if CalendView template part is not ready, we delay this request.
        bool m_isYearDecadeViewDimensionRequested : 1;

        // if CalendView template part is not ready, we delay this request.
        bool m_isSetDisplayDateRequested : 1;

        // true when pointer is over the control but not on the header.
        bool m_isPointerOverMain : 1;

        // true when pointer is pressed on the control but not on the header.
        bool m_isPressedOnMain : 1;

        // On pointer released we perform some actions depending on control. We decide to whether to perform them
        // depending on some parameters including but not limited to whether released is followed by a pressed, which
        // mouse button is pressed, what type of pointer is it etc. This boolean keeps our decision.
        bool m_shouldPerformActions : 1;

        bool m_isSelectedDatesChangingInternally : 1;
    };
}
