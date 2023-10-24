// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <fwd/windows.globalization.h>

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{
    class TimePickerFlyoutPresenter :
        public TimePickerFlyoutPresenterGenerated
    {
    public:
        TimePickerFlyoutPresenter();

        // IFrameworkElementOverrides
        _Check_return_ HRESULT
        OnApplyTemplateImpl() override;

        // IUIElementOverrides
        _Check_return_ HRESULT
        OnCreateAutomationPeerImpl(
            _Outptr_ xaml::Automation::Peers::IAutomationPeer **returnValue) override;

        // ITimePickerFlyoutPresenter
        _Check_return_ HRESULT OnPropertyChanged(
            _In_ xaml::IDependencyPropertyChangedEventArgs* pArgs);

        static _Check_return_ HRESULT GetDefaultIsDefaultShadowEnabled(_Outptr_ IInspectable** ppIsDefaultShadowEnabledValue);

        _Check_return_ HRESULT PullPropertiesFromOwner(_In_ xaml_controls::ITimePickerFlyout* pOwner);

        _Check_return_ HRESULT SetAcceptDismissButtonsVisibility(_In_ bool isVisible);

        _Check_return_ HRESULT GetTime(_Out_ wf::TimeSpan* pTime);

        // String to be compared against ClockIdentifier property to determine the clock system
        // we are using.
        static const WCHAR* _strTwelveHourClock;
        static const INT32 _periodCoercionOffset;

    protected:
        _Check_return_ HRESULT OnKeyDownImpl(_In_ xaml_input::IKeyRoutedEventArgs* pEventArgs) override sealed;

    private:
        ~TimePickerFlyoutPresenter() {}

        _Check_return_ HRESULT InitializeImpl() override;

        // Formats to be used on datetimeformatters to get strings representing times
        static const WCHAR* _strHourFormat;
        static const WCHAR* _strMinuteFormat;
        static const WCHAR* _strPeriodFormat;

        // Corresponding number of timespan ticks per minute, hour and day. Note that a tick is a 100 nanosecond
        // interval.
        static const INT64 _timeSpanTicksPerMinute;
        static const INT64 _timeSpanTicksPerHour;
        static const INT64 _timeSpanTicksPerDay;

        static const WCHAR _firstPickerHostName[];
        static const WCHAR _secondPickerHostName[];
        static const WCHAR _thirdPickerHostName[];
        static const WCHAR _backgroundName[];
        static const WCHAR _contentPanelName[];
        static const WCHAR _titlePresenterName[];

        static const WCHAR _hourLoopingSelectorAutomationId[];
        static const WCHAR _minuteLoopingSelectorAutomationId[];
        static const WCHAR _periodLoopingSelectorAutomationId[];

        _Check_return_ HRESULT Initialize();

        // Clears everything, generates and sets the itemssources to selectors.
        _Check_return_ HRESULT RefreshSetup();

        _Check_return_ HRESULT SetTime(_In_ wf::TimeSpan time);

        // Reacts to changes in Time property. Time may have changed programmatically or end user may have changed the
        // selection of one of our selectors causing a change in Time.
        _Check_return_ HRESULT OnTimeChanged();

        // Reacts to change in MinuteIncrement property.
        _Check_return_ HRESULT OnMinuteIncrementChanged(
            _In_ INT32 oldValue,
            _In_ INT32 newValue);

        // Reacts to change in ClockIdentifier property.
        _Check_return_ HRESULT OnClockIdentifierChanged(
            _In_ HSTRING oldValue);

        // Checks whether the given time is in our acceptable range, coerces it or raises exception when necessary.
        _Check_return_ HRESULT CheckAndCoerceTime(
            _In_ wf::TimeSpan time,
            _Out_ wf::TimeSpan* pCoercedTime);

        // Creates a new wg::Calendar, taking into account the ClockIdentifier
        _Check_return_ HRESULT CreateNewCalendar(
            _In_ HSTRING strClockIdentifier,
            _Outptr_ wg::ICalendar** ppCalendar);

        // Creates a new DateTimeFormatter with the given format and clock identifier.
        _Check_return_ HRESULT CreateNewFormatterWithClock(
            _In_ HSTRING strFormat,
            _In_ HSTRING strClockIdentifier,
            _Outptr_ wg::DateTimeFormatting::IDateTimeFormatter** ppDateTimeFormatter);

        // Creates a new DateTimeFormatter with the given format and calendar identifier.
        _Check_return_ HRESULT CreateNewFormatterWithCalendar(
            _In_ HSTRING strFormat,
            _In_ HSTRING strCalendarIdentifier,
            _Outptr_ wg::DateTimeFormatting::IDateTimeFormatter** ppDateTimeFormatter);

        // Returns the cached DateTimeFormatter for the given Clock for generating the strings
        // representing the current Time for display on a FlyoutButton. If there isn't a cached
        // DateTimeFormatter instance, creates one and caches it to be returned for the following
        // calls with the same pair.
        _Check_return_ HRESULT GetTimeFormatter(
            _In_ HSTRING strClockIdentifier,
            _Outptr_ wg::DateTimeFormatting::IDateTimeFormatter ** ppDateFormatter);

        // Sets our sentinel date to the given calendar. This date is 21st of July 2011 midnight.
        // On this day there are no known daylight saving transitions.
        _Check_return_ HRESULT SetSentinelDate(
            _In_ wg::ICalendar* pCalendar);

        // Generate the collection that we will populate our hour picker with.
        _Check_return_ HRESULT GenerateHours();

        // Generate the collection that we will populate our minute picker with.
        _Check_return_ HRESULT GenerateMinutes();

        // Generate the collection that we will populate our period picker with.
        _Check_return_ HRESULT GeneratePeriods();

        // Clears the ItemsSource  properties of the selectors.
        _Check_return_ HRESULT ClearSelectors();

        // Reacts to change in selection of our selectors. Calculates the new date represented by the selected indices and updates the
        // Date property.
        _Check_return_ HRESULT OnSelectorSelectionChanged(
            _In_ IInspectable* pSender,
            _In_ xaml_controls::ISelectionChangedEventArgs* pArgs);

        // Gets the layout ordering of the selectors.
        _Check_return_ HRESULT GetOrder(
            _Out_ INT32* hourOrder,
            _Out_ INT32* minuteOrder,
            _Out_ INT32* periodOrder,
            _Out_ BOOLEAN* isRTL);

        // Updates the order of selectors in our layout. Also takes care of hiding/showing the selectors and related spacing depending our
        // public properties set by the user.
        _Check_return_ HRESULT UpdateOrderAndLayout();

        // Updates the selector selected indices to display our Time property.
        _Check_return_ HRESULT UpdateDisplay();

        // Updates the Time property according to the selected indices of the selectors.
        _Check_return_ HRESULT UpdateTime();

        // Translates ONLY the hour and minute fields of DateTime into TimeSpan
        _Check_return_ HRESULT GetTimeSpanFromDateTime(
            _In_ wf::DateTime dateTime,
            _Out_ wf::TimeSpan* pTimeSpan);

        // Translates a timespan to datetime. Note that, unrelated fields of datetime (year, day etc.)
        // are set to our sentinel values.s
        _Check_return_ HRESULT GetDateTimeFromTimeSpan(
            _In_ wf::TimeSpan timeSpan,
            _Out_ wf::DateTime* pDateTime);

        // Gets the minute increment and if it is 0, adjusts it to 60 so we will handle the 0
        // case correctly.
        _Check_return_ HRESULT GetAdjustedMinuteIncrement(
            _Out_ INT32* minuteIncrement);

        // The selection of the selectors in our template can be changed by two sources. First source is
        // the end user changing a field to select the desired time. Second source is us updating
        // the itemssources and selected indices. We only want to react to the first source as the
        // second one will cause an unintentional recurrence in our logic. So we use this locking mechanism to
        // anticipate selection changes caused by us and making sure we do not react to them. It is okay
        // that these locks are not atomic since they will be only accessed by a single thread so no race
        // condition can occur.
        void AllowReactionToSelectionChange()
        {
           _reactionToSelectionChangeAllowed = TRUE;
        }

        void PreventReactionToSelectionChange()
        {
           _reactionToSelectionChangeAllowed = FALSE;
        }

        BOOLEAN IsReactionToSelectionChangeAllowed()
        {
           return _reactionToSelectionChangeAllowed;
        }

        // Specifies if we are in 12 hour clock mode currently.
        BOOLEAN _is12HourClock;

        BOOLEAN _reactionToSelectionChangeAllowed;

        // Reference to a Button for invoking the TimePickerFlyout in the form factor APISet
        Private::TrackerPtr<xaml_controls::Primitives::IButtonBase> _tpFlyoutButton;

        Private::TrackerPtr<wfc::IVector<IInspectable*>> _tpHourSource;
        Private::TrackerPtr<wfc::IVector<IInspectable*>> _tpMinuteSource;
        Private::TrackerPtr<wfc::IVector<IInspectable*>> _tpPeriodSource;

        // Reference the picker selectors by order of appearance.
        // Used for arrow navigation, so stored as IControls for easy access
        // to the Focus() method.
        Private::TrackerPtr<xaml_controls::IControl> _tpFirstPickerAsControl;
        Private::TrackerPtr<xaml_controls::IControl> _tpSecondPickerAsControl;
        Private::TrackerPtr<xaml_controls::IControl> _tpThirdPickerAsControl;

        // References to the hosting borders.
        Private::TrackerPtr<xaml_controls::IBorder> _tpFirstPickerHost;
        Private::TrackerPtr<xaml_controls::IBorder> _tpSecondPickerHost;
        Private::TrackerPtr<xaml_controls::IBorder> _tpThirdPickerHost;

        // References to the columns of the Grid that will hold the day/month/year LoopingSelectors and the spacers.
        Private::TrackerPtr<xaml_controls::IColumnDefinition> _tpFirstPickerHostColumn;
        Private::TrackerPtr<xaml_controls::IColumnDefinition> _tpSecondPickerHostColumn;
        Private::TrackerPtr<xaml_controls::IColumnDefinition> _tpThirdPickerHostColumn;

        // References to elements which will act as the dividers between the LoopingSelectors.
        Private::TrackerPtr<IUIElement> _tpFirstPickerSpacing;
        Private::TrackerPtr<IUIElement> _tpSecondPickerSpacing;

        // Reference to the title presenter
         Private::TrackerPtr<xaml_controls::ITextBlock> _tpTitlePresenter;

        // Reference to the Header content presenter. We need this to collapse the visibility
        // when the Header and HeaderTemplate are null.
        Private::TrackerPtr<xaml::IUIElement> _tpHeaderPresenter;

        // Reference to background border
        Private::TrackerPtr<xaml_controls::IBorder> _tpBackgroundBorder;

        // Reference to our content panel. We will be setting the flowdirection property on our root to achieve
        // RTL where necessary.
        Private::TrackerPtr<IFrameworkElement> _tpContentPanel;

        // References to the elements for the accept and dismiss buttons.
        Private::TrackerPtr<IUIElement> _tpAcceptDismissHostGrid;
        Private::TrackerPtr<IUIElement> _tpAcceptButton;
        Private::TrackerPtr<IUIElement> _tpDismissButton;

        bool _acceptDismissButtonsVisible;

        // Reference to HourPicker Selector. We need this as we will change its item
        // source as our properties change.
        Private::TrackerPtr<xaml_primitives::ILoopingSelector> _tpHourPicker;

        // Reference to MinutePicler Selector. We need this as we will change its item
        // source as our properties change.
        Private::TrackerPtr<xaml_primitives::ILoopingSelector> _tpMinutePicker;

        // Reference to PeriodPicker Selector. We need this as we will change its item
        // source as our properties change.
        Private::TrackerPtr<xaml_primitives::ILoopingSelector> _tpPeriodPicker;

        // This calendar will be used over and over while we are generating the ItemsSources instead
        // of creating new calendars.
        Private::TrackerPtr<wg::ICalendar> _tpCalendar;

        // This DateTimeFormatter will be used over and over when updating the
        // FlyoutButton content property
        Private::TrackerPtr<wg::DateTimeFormatting::IDateTimeFormatter> _tpTimeFormatter;
        wrl_wrappers::HString _strTimeFormatterClockIdentifier;

        // Properties pulled from owner TimePickerFlyout
        wrl_wrappers::HString _clockIdentifier;
        wrl_wrappers::HString _title;
        INT32 _minuteIncrement;
        wf::TimeSpan _time;

        EventRegistrationToken _hourSelectionChangedToken;
        EventRegistrationToken _minuteSelectionChangedToken;
        EventRegistrationToken _periodSelectionChangedToken;
    };

    ActivatableClass(TimePickerFlyoutPresenter);

}
} } } XAML_ABI_NAMESPACE_END
