// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "calendarview.g.h"
#include "CalendarViewHeaderAutomationPeer_Partial.h"
#include <fwd/windows.globalization.h>

namespace DirectUI
{

    class CalendarViewGeneratorHost;
    class CalendarViewDayItem;
    class CalendarViewItem;
    class CalendarViewBaseItem;
    class CalendarPanel;
    class DateComparer;
    enum class TrackableDateCollection_CollectionChanging;

    namespace CalendarConstants
    {
        const INT64 s_ticksPerDay = 864000000000L;       // 24 hours, the regular days
        const INT64 s_ticksPerHour = 36000000000L;       // 1 hour for day light saving
        const INT64 s_maxTicksPerDay = s_ticksPerDay + s_ticksPerHour;
        const INT64 s_maxTicksPerMonth = 31 * s_ticksPerDay + s_ticksPerHour;
        const INT64 s_maxTicksPerYear = 366 * s_ticksPerDay + s_ticksPerHour;
    }

    PARTIAL_CLASS(CalendarView)
    {
        friend class CalendarViewGeneratorHost;
        friend class CalendarViewGeneratorMonthViewHost;
        friend class CalendarViewGeneratorYearViewHost;
        friend class CalendarViewAutomationPeer;
        friend class CalendarViewBaseItemAutomationPeer;
        friend class CalendarViewDayItemAutomationPeer;
        friend class CalendarViewItemAutomationPeer;
        friend class CalendarScrollViewerAutomationPeer;

    protected:
        CalendarView();
        ~CalendarView() override;

        // Prepares object's state
        _Check_return_ HRESULT PrepareState() override;

        // Measure the content.
        IFACEMETHOD(MeasureOverride)(
            // Measurement constraints, a control cannot return a size
            // larger than the constraint.
            _In_ wf::Size pAvailableSize,
            // The desired size of the control.
            _Out_ wf::Size* pDesired) override;

        IFACEMETHOD(ArrangeOverride)(
            _In_ wf::Size arrangeSize,
            _Out_ wf::Size* pReturnValue)
            override;

        // Override the GetDefaultValue method to return the default values
        // for CalendarView dependency properties.
        _Check_return_ HRESULT GetDefaultValue2(
            _In_ const CDependencyProperty* pDP,
            _Out_ CValue* pValue) override;

        // Handle the custom property changed event and call the OnPropertyChanged2 methods.
        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        IFACEMETHOD(OnApplyTemplate)() override;

        // Handles when a key is pressed down on the CalendarView.
        IFACEMETHOD(OnKeyDown)(_In_ xaml_input::IKeyRoutedEventArgs* pArgs) override;

        // UIElement override for getting next tab stop on path from focus candidate element to root.
        _Check_return_ HRESULT ProcessCandidateTabStopOverride(
            _In_opt_ DependencyObject* pFocusedElement,
            _In_ DependencyObject* pCandidateTabStopElement,
            _In_opt_ DependencyObject* pOverriddenCandidateTabStopElement,
            const bool isBackward,
            _Outptr_ DependencyObject** ppNewTabStop,
            _Out_ BOOLEAN* pIsCandidateTabStopOverridden) override;

        _Check_return_ HRESULT ChangeVisualState(_In_ bool bUseTransitions) override;

        // Override OnCreateAutomationPeer()
        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_result_maybenull_ xaml_automation_peers::IAutomationPeer** returnValue);

        // IsEnabled property changed handler.
        _Check_return_ HRESULT OnIsEnabledChanged(_In_ DirectUI::IsEnabledChangedEventArgs* pArgs) override;

        _Check_return_ HRESULT GetRowHeaderForItemAutomationPeer(
            _In_ wf::DateTime itemDate,
            _In_ xaml_controls::CalendarViewDisplayMode displayMode,
            _Out_ UINT* pReturnValueCount,
            _Out_writes_to_ptr_(*pReturnValueCount) xaml_automation::Provider::IIRawElementProviderSimple*** ppReturnValue);

    public:
        wf::DateTime GetToday() { return m_today; }
        wf::DateTime GetMaxDate() { return m_maxDate; }
        wf::DateTime GetMinDate() { return m_minDate; }

        wg::ICalendar* GetCalendar() { return m_tpCalendar.Get(); }

        DateComparer* GetDateComparer() { return m_dateComparer.get(); }

        _Check_return_ HRESULT OnItemFocused(_In_ CalendarViewBaseItem* pItem);

    public: //Selection related

        _Check_return_ HRESULT OnSelectDayItem(_In_ CalendarViewDayItem* pItem);
        _Check_return_ HRESULT OnDayItemBlackoutChanged(_In_ CalendarViewDayItem* pItem, _In_ bool isBlackout);

        _Check_return_ HRESULT OnSelectMonthYearItem(_In_ CalendarViewItem* pItem, _In_ xaml::FocusState focusState);

        _Check_return_ HRESULT IsSelected(_In_ wf::DateTime date, _Out_ bool *pIsSelected);

        _Check_return_ HRESULT OnSelectionModeChanged();

        _Check_return_ HRESULT RaiseSelectionChangedEventIfChanged();

        _Check_return_ HRESULT OnSelectedDatesChanged(
            _In_ wfc::IObservableVector<wf::DateTime>* pSender,
            _In_ wfc::IVectorChangedEventArgs* e);

        _Check_return_ HRESULT ValidateSelectingDateIsNotBlackout(_In_ wf::DateTime date);

        _Check_return_ HRESULT OnSelectedDatesChanging(
            _In_ TrackableDateCollection_CollectionChanging action,
            _In_ wf::DateTime addingDate);

        // given a datetime with time part, find the corresponding container from MonthPanel
        _Check_return_ HRESULT GetContainerByDate(_In_ wf::DateTime datetime, _Outptr_result_maybenull_ CalendarViewDayItem** ppItem);

        _Check_return_ HRESULT SetYearDecadeDisplayDimensionsImpl(_In_ INT columns, _In_ INT rows);
        _Check_return_ HRESULT SetDisplayDateImpl(_In_ wf::DateTime date);

        void CoerceDate(_Inout_ wf::DateTime& date);

        _Check_return_ HRESULT SetKeyDownEventArgsFromCalendarItem(_In_ xaml_input::IKeyRoutedEventArgs* pArgs)
        {
            return ctl::AsWeak(pArgs, &m_wrKeyDownEventArgsFromCalendarItem);
        }

        static _Check_return_ HRESULT SetDayItemStyle(
            _In_ CalendarViewBaseItem* pItem,
            _In_opt_ xaml::IStyle* pStyle);

        bool IsMultipleEraCalendar() { return m_isMultipleEraCalendar; }

        static _Check_return_ HRESULT CreateCalendarLanguagesStatic(_In_ wrl_wrappers::HString&& language, _Outptr_ wfc::IIterable<HSTRING>** ppLanguages);

        static _Check_return_ HRESULT CanPanelShowFullScope(
            _In_ CalendarViewGeneratorHost* pHost,
            _Out_ bool* pCanPanelShowFullScope);

    private:

        _Check_return_ HRESULT ScrollToDate(
            _In_ CalendarViewGeneratorHost* pHost,
            _In_ wf::DateTime date);

        // With Animation. The target item should be realized
        // already, or we'll see unrealized area during the animation.
        _Check_return_ HRESULT ScrollToDateWithAnimation(
            _In_ CalendarViewGeneratorHost* pHost,
            _In_ wf::DateTime date);

        // Bring display date into view and
        // if panel can show a full scope, bring the first day of
        // the scope on the display date into view.
        _Check_return_ HRESULT BringDisplayDateIntoView(
            _In_ CalendarViewGeneratorHost* pHost);

        _Check_return_ HRESULT CreateCalendarAndMonthYearFormatter();

        _Check_return_ HRESULT CreateDateTimeFormatter(
            _In_ HSTRING format,
            _Outptr_ wg::DateTimeFormatting::IDateTimeFormatter** ppDateTimeFormatter);

        _Check_return_ HRESULT UpdateViewsSource();

        _Check_return_ HRESULT DetermineTheBiggestDayItemSize(_In_ wf::Size availableSize);

        _Check_return_ HRESULT DisconnectItemHosts();
        _Check_return_ HRESULT RegisterItemHosts();
        _Check_return_ HRESULT RefreshItemHosts();

        _Check_return_ HRESULT AttachVisibleIndicesUpdatedEvents();
        _Check_return_ HRESULT DetachVisibleIndicesUpdatedEvents();

        _Check_return_ HRESULT AttachScrollViewerFocusEngagedEvents();
        _Check_return_ HRESULT DetachScrollViewerFocusEngagedEvents();

        _Check_return_ HRESULT AttachButtonClickedEvents();
        _Check_return_ HRESULT DetachButtonClickedEvents();

        _Check_return_ HRESULT AttachScrollViewerKeyDownEvents();
        _Check_return_ HRESULT DetachScrollViewerKeyDownEvents();

        _Check_return_ HRESULT OnVisibleIndicesUpdated(_In_ CalendarViewGeneratorHost* pHost);

        _Check_return_ HRESULT OnScrollViewerFocusEngaged(
            _In_ xaml_controls::IFocusEngagedEventArgs* pArgs);

        _Check_return_ HRESULT OnHeaderButtonClicked();

        // When NavigationButton is pressed, the header is changed to the new date like Jan 2018. "Jan 2018" are expected for narrator after it's clicked.
        // SetNameStatic doesn't work since this is a post event, so RaiseNotificationEvent is used
        _Check_return_ HRESULT RaiseAutomationNotificationAfterNavigationButtonClicked();
        _Check_return_ HRESULT OnNavigationButtonClicked(_In_ bool forward);

        _Check_return_ HRESULT UpdateWeekDayNames();
        _Check_return_ HRESULT FormatWeekDayNames();
        _Check_return_ HRESULT UpdateNavigationButtonStates();
        _Check_return_ HRESULT UpdateHeaderText(_In_ bool withAnimation);

        _Check_return_ HRESULT UpdateItemsScopeState(
            _In_ CalendarViewGeneratorHost* pHost,
            _In_ bool ignoreWhenIsOutOfScopeDisabled,
            _In_ bool ignoreInDirectManipulation);

        _Check_return_ HRESULT OnIsOutOfScopePropertyChanged();
        _Check_return_ HRESULT OnIsTodayHighlightedPropertyChanged();
        _Check_return_ HRESULT OnDisplayModeChanged(
            _In_ xaml_controls::CalendarViewDisplayMode oldDisplayMode,
            _In_ xaml_controls::CalendarViewDisplayMode newDisplayMode);
        _Check_return_ HRESULT OnIsLabelVisibleChanged();

        // return the active generatorhost based on display mode
        _Check_return_ HRESULT GetActiveGeneratorHost(_Outptr_ CalendarViewGeneratorHost** ppHost);

        _Check_return_ HRESULT GetGeneratorHost(_In_ xaml_controls::CalendarViewDisplayMode mode, _Outptr_ CalendarViewGeneratorHost** ppHost);

        _Check_return_ HRESULT FormatYearName(_In_ wf::DateTime date, _Out_ HSTRING* pName);

        _Check_return_ HRESULT FormatMonthYearName(_In_ wf::DateTime date, _Out_ HSTRING* pName);

        _Check_return_ HRESULT OnAlignmentChanged(const PropertyChangedParams& args);

        // Given a key, returns the appropriate navigation action.
        _Check_return_ HRESULT TranslateKeyToKeyNavigationAction(
            _In_ wsy::VirtualKey key,
            _Out_ xaml_controls::KeyNavigationAction* pNavAction,
            _Out_ bool* pIsValidKey);

        _Check_return_ HRESULT FocusItemByDate(
            _In_ CalendarViewGeneratorHost* pHost,
            _In_ wf::DateTime date,
            _In_ xaml::FocusState focusState,
            _Out_ bool* pFocused);

        _Check_return_ HRESULT FocusItemByIndex(
            _In_ CalendarViewGeneratorHost* pHost,
            _In_ int index,
            _In_ xaml::FocusState focusState,
            _Out_ bool* pFocused);

        _Check_return_ HRESULT FocusItem(
            _In_ CalendarViewGeneratorHost* pHost,
            _In_ wf::DateTime date,
            _In_ int index,
            _In_ xaml::FocusState focusState,
            _Out_ bool* pFocused);

        // call internally, we don't need to coerce date or zero the time part
        _Check_return_ HRESULT SetDisplayDateInternal(_In_ wf::DateTime date);

        // Partially copy date from source to target based on displayMode.
        // for MonthMode, we copy Era, Year, Month and Day parts
        // for YearMode, we copy Era, Year and Month parts, keep the Day part.
        // for DecadeMode, we copy Era and Year parts, keep the Month and Day parts.
        //
        // Once the remaining part becomes invalid with the new copied parts,
        // we need to adjust the remaining part to the most reasonable value.
        // e.g. target: 3/31/2014, source 2/1/2013 and we want to copy month part,
        // the target will become 2/31/2013 and we'll adjust the day to 2/28/2013.
        _Check_return_ HRESULT CopyDate(
            _In_ xaml_controls::CalendarViewDisplayMode displayMode,
            _In_ wf::DateTime source,
            _Inout_ wf::DateTime& target);

        _Check_return_ HRESULT OnKeyboardNavigation(
            _In_ wsy::VirtualKey key,
            _In_ wsy::VirtualKey originalKey,
            _Out_ bool* handled);

        _Check_return_ HRESULT ForeachChildInPanel(_In_ CalendarPanel* pPanel, _In_ std::function<HRESULT(_In_ CalendarViewBaseItem*)>);

        _Check_return_ HRESULT ForeachHost(_In_ std::function<HRESULT(_In_ CalendarViewGeneratorHost* pHost)>);

        _Check_return_ HRESULT CreateCalendarLanguages();

        _Check_return_ HRESULT OnPrimaryPanelDesiredSizeChanged(_In_ CalendarViewGeneratorHost* pHost);

        _Check_return_ HRESULT UpdateLastDisplayedDate(_In_ xaml_controls::CalendarViewDisplayMode lastDisplayMode);

        _Check_return_ HRESULT UpdateWeekDayNameAPName(_In_reads_(count) wchar_t* str, _In_ size_t count, _In_ const wrl_wrappers::HString& name);

        _Check_return_ HRESULT InitializeIndexCorrectionTableIfNeeded();

        _Check_return_ HRESULT UpdateFlowDirectionForView();

#ifdef DBG
        void SetRoundedCalendarViewBaseItemChromeFallbackColor(KnownPropertyIndex propertyIndex, XUINT32 color);
        void SetRoundedCalendarViewBaseItemChromeFallbackThickness(KnownPropertyIndex propertyIndex, XTHICKNESS thickness);
        void SetRoundedCalendarViewBaseItemChromeFallbackCornerRadius(KnownPropertyIndex propertyIndex, XCORNERRADIUS cornerRadius);
        void SetRoundedCalendarViewBaseItemChromeFallbackProperties();
#endif // DBG

    private:
        TrackerPtr<wfc::IVector<wf::DateTime>> m_tpSelectedDates;

        TrackerPtr<xaml_controls::IButton> m_tpHeaderButton;
        TrackerPtr<xaml_controls::IButton> m_tpPreviousButton;
        TrackerPtr<xaml_controls::IButton> m_tpNextButton;

        TrackerPtr<xaml_controls::IGrid> m_tpViewsGrid;

        TrackerPtr<wg::ICalendar> m_tpCalendar;
        TrackerPtr<wg::DateTimeFormatting::IDateTimeFormatter> m_tpMonthYearFormatter;
        TrackerPtr<wg::DateTimeFormatting::IDateTimeFormatter> m_tpYearFormatter;

        TrackerPtr<CalendarViewGeneratorHost> m_tpMonthViewItemHost;
        TrackerPtr<CalendarViewGeneratorHost> m_tpYearViewItemHost;
        TrackerPtr<CalendarViewGeneratorHost> m_tpDecadeViewItemHost;

        TrackerPtr<xaml_controls::IScrollViewer> m_tpMonthViewScrollViewer;
        TrackerPtr<xaml_controls::IScrollViewer> m_tpYearViewScrollViewer;
        TrackerPtr<xaml_controls::IScrollViewer> m_tpDecadeViewScrollViewer;

        // we define last displayed date by following:
        // 1. if the last displayed item is visible, the date of last displayed item
        // 2. if last focused item is not visible, we use the first_visible_inscope_date
        // 3. when an item gets focused, we use the date of the focused item.
        //
        // the default last displayed date will be determined by following:
        // 1. display Date if it is requested, if it is not requested, then
        // 2. Today, if Today is not in given min/max range, then
        // 3. the closest date to Today (i.e. the coerced date of Today)
        wf::DateTime m_lastDisplayedDate;

        wf::DateTime m_today;
        // m_minDate and m_maxDate are effective min/max dates, which could be different
        // than the values return from get_MinDate/get_MaxDate.
        // because developer could set a minDate or maxDate that doesn't exist in
        // current calendarsystem. (e.g. UmAlQuraCalendar doesn't have 1/1/2099).
        wf::DateTime m_maxDate;
        wf::DateTime m_minDate;

        // the weekday of mindate.
        wg::DayOfWeek m_weekDayOfMinDate;

        TrackerPtr<xaml_primitives::ICalendarViewTemplateSettings> m_tpTemplateSettings;

        ctl::EventPtr<ButtonBaseClickEventCallback> m_epHeaderButtonClickHandler;
        ctl::EventPtr<ButtonBaseClickEventCallback> m_epPreviousButtonClickHandler;
        ctl::EventPtr<ButtonBaseClickEventCallback> m_epNextButtonClickHandler;

        ctl::EventPtr<UIElementKeyDownEventCallback> m_epMonthViewScrollViewerKeyDownEventHandler;
        ctl::EventPtr<UIElementKeyDownEventCallback> m_epYearViewScrollViewerKeyDownEventHandler;
        ctl::EventPtr<UIElementKeyDownEventCallback> m_epDecadeViewScrollViewerKeyDownEventHandler;

        static constexpr int s_minNumberOfWeeks = 2;
        static constexpr int s_maxNumberOfWeeks = 8;
        static constexpr int s_defaultNumberOfWeeks = 6;
        static constexpr int s_numberOfDaysInWeek = 7;

        int m_colsInYearDecadeView;     // default value is 4

        int m_rowsInYearDecadeView;     // default value is 4

        // if we decide to have a different startIndex in YearView or DecadeView, we should make a corresponding change at CalendarViewItemAutomationPeer::get_ColumnImpl
        // in MonthView, because we can set the DayOfWeek property, the first item is not always start from the first positon inside the Panel
        int m_monthViewStartIndex;

        // dayOfWeekNames stores abbreviated names of each day of the week. dayOfWeekNamesFull stores the full name to be read aloud by accessibility.
        std::vector<wrl_wrappers::HString> m_dayOfWeekNames;
        std::vector<wrl_wrappers::HString> m_dayOfWeekNamesFull;

        TrackerPtr<wfc::IIterable<HSTRING>> m_tpCalendarLanguages;

        ctl::EventPtr<DateTimeVectorChangedEventCallback> m_epSelectedDatesChangedHandler;


        // the keydown event args from CalendarItem.
        ctl::WeakRefPtr m_wrKeyDownEventArgsFromCalendarItem;

        // the focus state we need to set on the calendaritem after we change the display mode.
        xaml::FocusState m_focusStateAfterDisplayModeChanged;

        std::unique_ptr<DateComparer> m_dateComparer;

        //
        // Automation fields
        //

        // When Narrator gives focus to a day item, we expect it to read the month header
        // (assuming the focus used to be outside or on a day item at a different month).
        // During this focus transition, Narrator expects to be able to query the previous peer (if any).
        // So we need to keep track of the current and previous month header peers.
        TrackerPtr<CalendarViewHeaderAutomationPeer> m_currentHeaderPeer;
        TrackerPtr<CalendarViewHeaderAutomationPeer> m_previousHeaderPeer;

        // when mindate or maxdate changed, we set this flag
        bool m_dateSourceChanged : 1;

        // when calendar identifier changed, we set this flag
        bool m_calendarChanged : 1;

        bool m_itemHostsConnected : 1;

        bool m_areDirectManipulationStateChangeHandlersHooked : 1;

        // this flag indicts the change of SelectedDates comes from internal or external.
        bool m_isSelectedDatesChangingInternally : 1;

        // when true we need to move focus to a calendaritem after we change the display mode.
        bool m_focusItemAfterDisplayModeChanged : 1;

        bool m_isMultipleEraCalendar : 1;

        bool m_isSetDisplayDateRequested : 1;

        bool m_areYearDecadeViewDimensionsSet : 1;

        // After navigationbutton clicked, the head text doesn't change immediately. so we use this flag to tell if the update text is from navigation button
        bool m_isNavigationButtonClicked : 1;

#ifdef DBG
        bool m_roundedCalendarViewBaseItemChromeFallbackPropertiesSet{ false };
#endif // DBG
    };
}
