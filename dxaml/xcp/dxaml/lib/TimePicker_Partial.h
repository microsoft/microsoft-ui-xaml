// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents TimePicker control. TimePicker is a XAML UI control that allows
//      the selection of times.

#pragma once

#include "TimePicker.g.h"
#include <fwd/windows.globalization.h>

namespace DirectUI
{
    PARTIAL_CLASS(TimePicker)
    {
        public:
            TimePicker();
            ~TimePicker() override;

        protected:

            // Get TimePicker template parts and create the sources if they are not already there
            IFACEMETHOD(OnApplyTemplate)() override;

            // Override OnCreateAutomationPeer()
            IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);

            // Gives the default values for our properties.
            _Check_return_ HRESULT GetDefaultValue2(
                _In_ const CDependencyProperty* pDP,
                _Out_ CValue* pValue) override;

            // Handle the custom property changed event and call the
            // OnPropertyChanged methods.
            _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

            _Check_return_ HRESULT PrepareState() override;

            _Check_return_ HRESULT ChangeVisualState(_In_ bool useTransitions = TRUE) override;

            // IsEnabled property changed handler.
            _Check_return_ HRESULT OnIsEnabledChanged(_In_ DirectUI::IsEnabledChangedEventArgs* pArgs) override;
            
            _Check_return_ HRESULT OnLoaded(_In_ IInspectable* sender, _In_ xaml::IRoutedEventArgs* args);

            IFACEMETHOD(OnKeyDown)(_In_ xaml_input::IKeyRoutedEventArgs* pArgs) override;

        private:

            // String to be compared against ClockIdentifier property to determine the clock system
            // we are using.
            static const WCHAR* s_strTwelveHourClock;

            // Formats to be used on datetimeformatters to get strings representing times
            static const WCHAR* s_strHourFormat;
            static const WCHAR* s_strMinuteFormat;
            static const WCHAR* s_strPeriodFormat;

            // Corresponding number of timespan ticks per minute, hour and day. Note that a tick is a 100 nanosecond
            // interval.
            static const INT64 s_timeSpanTicksPerMinute;
            static const INT64 s_timeSpanTicksPerHour;
            static const INT64 s_timeSpanTicksPerDay;

            // Clears everything, generates and sets the itemssources to selectors.
            _Check_return_ HRESULT RefreshSetup();

            // Reacts to changes in Time property. Time may have changed programmatically or end user may have changed the
            // selection of one of our selectors causing a change in Time.
            _Check_return_ HRESULT OnTimeChanged(
                _In_ IInspectable* pOldValue,
                _In_ IInspectable* pNewValue);

            // Reacts to change in MinuteIncrement property.
            _Check_return_ HRESULT OnMinuteIncrementChanged(
                _In_ IInspectable* pOldValue,
                _In_ IInspectable* pNewValue);

            // Reacts to change in ClockIdentifier property.
            _Check_return_ HRESULT OnClockIdentifierChanged(
                _In_ IInspectable* pOldValue,
                _In_ IInspectable* pNewValue);

            // Reacts to the FlyoutButton being pressed. Invokes a form-factor specific flyout if present.
             _Check_return_ HRESULT OnFlyoutButtonClick(
                _In_ IInspectable* pSender,
                _In_ xaml::IRoutedEventArgs* pArgs);

            // Callback passed to the GetTimePickerSelectionAsync method. Called when a form-factor specific
            // flyout returns with a new DateTime value to update the TimePicker's DateTime.
            _Check_return_ HRESULT OnGetTimePickerSelectionAsyncCompleted(
                wf::IAsyncOperation<wf::IReference<wf::TimeSpan>*>* getOperation,
                wf::AsyncStatus status);

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

             // Updates the Content of the FlyoutButton to be the current time.
             _Check_return_ HRESULT UpdateFlyoutButtonContent();

            // Updates the visibility of the Header ContentPresenter
            _Check_return_ HRESULT UpdateHeaderPresenterVisibility();

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

            // Triggers the TimePickerFlyout
            _Check_return_ HRESULT ShowPickerFlyout();

            // The selection of the selectors in our template can be changed by two sources. First source is
            // the end user changing a field to select the desired time. Second source is us updating
            // the itemssources and selected indices. We only want to react to the first source as the
            // second one will cause an unintentional recurrence in our logic. So we use this locking mechanism to
            // anticipate selection changes caused by us and making sure we do not react to them. It is okay
            // that these locks are not atomic since they will be only accessed by a single thread so no race
            // condition can occur.
            void AllowReactionToSelectionChange()
            {
               m_reactionToSelectionChangeAllowed = TRUE;
            }

            void PreventReactionToSelectionChange()
            {
               m_reactionToSelectionChangeAllowed = FALSE;
            }

            BOOLEAN IsReactionToSelectionChangeAllowed()
            {
               return m_reactionToSelectionChangeAllowed;
            }

            // Get a string representation of the selected date
            _Check_return_ HRESULT GetSelectedTimeAsString(_Out_ HSTRING* strPlainText);

            //Update button accessible name when selected time changes
            _Check_return_ HRESULT RefreshFlyoutButtonAutomationName();

            static wf::TimeSpan GetNullTimeSentinel();
            static long GetNullTimeSentinelValue();

            _Check_return_ HRESULT GetCurrentTime(_Out_ wf::TimeSpan* currentTime);
            _Check_return_ HRESULT GetSelectedTime(_Out_ wf::TimeSpan* time);

            // Specifies if we are in 12 hour clock mode currently.
            BOOLEAN m_is12HourClock;

            BOOLEAN m_reactionToSelectionChangeAllowed;

            // Reference to a Button for invoking the TimePickerFlyout in the form factor APISet
            TrackerPtr<xaml_primitives::IButtonBase> m_tpFlyoutButton;

            // References to the TextBlocks that are used to display the Hour/Minute/Period.
            TrackerPtr<xaml_controls::ITextBlock> m_tpHourTextBlock;
            TrackerPtr<xaml_controls::ITextBlock> m_tpMinuteTextBlock;
            TrackerPtr<xaml_controls::ITextBlock> m_tpPeriodTextBlock;

            TrackerPtr<TrackerCollection<IInspectable*>> m_tpHourSource;
            TrackerPtr<TrackerCollection<IInspectable*>> m_tpMinuteSource;
            TrackerPtr<TrackerCollection<IInspectable*>> m_tpPeriodSource;

            // References to the hosting borders.
            TrackerPtr<xaml_controls::IBorder> m_tpFirstPickerHost;
            TrackerPtr<xaml_controls::IBorder> m_tpSecondPickerHost;
            TrackerPtr<xaml_controls::IBorder> m_tpThirdPickerHost;

            // References to the columns that will hold the hour/minute/period textblocks.
            TrackerPtr<xaml_controls::IColumnDefinition> m_tpFirstTextBlockColumn;
            TrackerPtr<xaml_controls::IColumnDefinition> m_tpSecondTextBlockColumn;
            TrackerPtr<xaml_controls::IColumnDefinition> m_tpThirdTextBlockColumn;

            // References to the column dividers between the hour/minute/period textblocks.
            TrackerPtr<IUIElement> m_tpFirstColumnDivider;
            TrackerPtr<IUIElement> m_tpSecondColumnDivider;

            // Reference to the Header content presenter. We need this to collapse the visibility
            // when the Header and HeaderTemplate are null.
            TrackerPtr<xaml::IUIElement> m_tpHeaderPresenter;

            // Reference to our lay out root. We will be setting the flowdirection property on our root to achieve
            // RTL where necessary.
            TrackerPtr<IFrameworkElement> m_tpLayoutRoot;

            // Reference to HourPicker Selector. We need this as we will change its item
            // source as our properties change.
            TrackerPtr<xaml_primitives::ISelector> m_tpHourPicker;

            // Reference to MinutePicler Selector. We need this as we will change its item
            // source as our properties change.
            TrackerPtr<xaml_primitives::ISelector> m_tpMinutePicker;

            // Reference to PeriodPicker Selector. We need this as we will change its item
            // source as our properties change.
            TrackerPtr<xaml_primitives::ISelector> m_tpPeriodPicker;

            // This calendar will be used over and over while we are generating the ItemsSources instead
            // of creating new calendars.
            TrackerPtr<wg::ICalendar> m_tpCalendar;

            ctl::EventPtr<SelectionChangedEventCallback> m_epHourSelectionChangedHandler;
            ctl::EventPtr<SelectionChangedEventCallback> m_epMinuteSelectionChangedHandler;
            ctl::EventPtr<SelectionChangedEventCallback> m_epPeriodSelectionChangedHandler;

            ctl::EventPtr<ButtonBaseClickEventCallback> m_epFlyoutButtonClickHandler;

            // Events and references used to respond to Window.Activated
            ctl::EventPtr<WindowActivatedEventCallback> m_windowActivatedHandler;
            ctl::EventPtr<FrameworkElementLoadedEventCallback> m_loadedEventHandler;

            // The default Time value if no Time is set.
            wf::TimeSpan m_defaultTime;

            // The current time.
            wf::TimeSpan m_currentTime;

            // Keeps track of the pending async operation and allows
            // for cancellation.
            TrackerPtr<IAsyncInfo> m_tpAsyncSelectionInfo;

            bool m_isPropagatingTime{ false };
    };
}
