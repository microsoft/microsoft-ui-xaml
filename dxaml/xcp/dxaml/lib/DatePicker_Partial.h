// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//
//      Represents DatePicker control. DatePicker is a XAML UI control that allows
//      the selection of dates.  DatePicker supports formatting of dates for the various
//      international calendars supported by Windows.  The purpose of the control is
//      to provide a standardized, touch-based XAML control for selecting localized dates.

#include <fwd/windows.globalization.h>

#pragma once

#include "DatePicker.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(DatePicker)
    {
        public:
            DatePicker();
            ~DatePicker() override;

        protected:
            // Get DatePicker template parts and create the sources if they are not already there
            IFACEMETHOD(OnApplyTemplate)() override;

            // Override OnCreateAutomationPeer()
            IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue) override;

            // Handle the custom property changed event and call the
            // OnPropertyChanged methods.
            _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

            // Gives the default values for our properties.
            _Check_return_ HRESULT GetDefaultValue2(
                _In_ const CDependencyProperty* pDP,
                _Out_ CValue* pValue) override;

            _Check_return_ HRESULT PrepareState() override;

            _Check_return_ HRESULT ChangeVisualState(_In_ bool useTransitions = TRUE) override;

            // IsEnabled property changed handler.
            _Check_return_ HRESULT OnIsEnabledChanged(_In_ DirectUI::IsEnabledChangedEventArgs* pArgs) override;

            IFACEMETHOD(OnKeyDown)(_In_ xaml_input::IKeyRoutedEventArgs* pArgs) override;

        private:
            // Creates a new DateTimeFormatter with the given parameters.
            _Check_return_ HRESULT CreateNewFormatter(
                _In_ HSTRING strPattern,
                _In_ HSTRING strCalendarIdentifier,
                _Out_ wg::DateTimeFormatting::IDateTimeFormatter** ppDateTimeFormatter);

            // Returns the cached DateTimeFormatter for the given Calendar - Format pair for generating the strings
            // representing the years in our date range. If there isn't a cached DateTimeFormatter instance,
            // creates one and caches it to be returned for the following calls with the same pair.
            _Check_return_ HRESULT GetYearFormatter(
                _In_ HSTRING strFormat,
                _In_ HSTRING strCalendarIdentifier,
                _Out_ wg::DateTimeFormatting::IDateTimeFormatter** ppDateTimeFormatter);

            // Returns the cached DateTimeFormatter for the given Calendar - Format pair for generating the strings
            // representing the months in our date range. If there isn't a cached DateTimeFormatter instance,
            // creates one and caches it to be returned for the following calls with the same pair.
            _Check_return_ HRESULT GetMonthFormatter(
                _In_ HSTRING strFormat,
                _In_ HSTRING strCalendarIdentifier,
                _Out_ wg::DateTimeFormatting::IDateTimeFormatter** ppDateTimeFormatter);

            // Returns the cached DateTimeFormatter for the given Calendar - Format pair for generating the strings
            // representing the days in our date range. If there isn't a cached DateTimeFormatter instance,
            // creates one and caches it to be returned for the following calls with the same pair.
            _Check_return_ HRESULT GetDayFormatter(
                _In_ HSTRING strFormat,
                _In_ HSTRING strCalendarIdentifier,
                _Out_ wg::DateTimeFormatting::IDateTimeFormatter** ppDateTimeFormatter);

            // Returns the cached DateTimeFormatter for the given Calendar for generating the strings
            // representing the current Date for display on a FlyoutButton. If there isn't a cached
            // DateTimeFormatter instance, creates one and caches it to be returned for the following
            // calls with the same pair.
            _Check_return_ HRESULT GetDateFormatter(
                _In_ HSTRING strCalendarIdentifier,
               _Outptr_ wg::DateTimeFormatting::IDateTimeFormatter** ppDateFormatter);


            // Clears everything and refreshes the helper objects. After that, generates and
            // sets the itemssources to selectors.
            _Check_return_ HRESULT RefreshSetup();

            // We execute our logic depending on some state information such as start date, end date, number of years etc. These state
            // variables need to be updated whenever a public property change occurs which affects them.
            _Check_return_ HRESULT UpdateState() noexcept;

            // Creates a new wg::Calendar, taking into account the Calendar Identifier
            // represented by our public "Calendar" property.
            _Check_return_ HRESULT CreateNewCalendar(
                _In_ HSTRING strCalendarIdentifier,
                _Outptr_ wg::ICalendar** ppCalendar);

            // Updates the Content of the FlyoutButton to be the current date.
            _Check_return_ HRESULT UpdateFlyoutButtonContent();

            // Updates the visibility of the Header ContentPresenter
            _Check_return_ HRESULT UpdateHeaderPresenterVisibility();

            // Clamps the given date within the range defined by the min and max dates. Note that it is caller's responsibility
            // to feed appropriate min/max values that defines a valid date range.
            wf::DateTime ClampDate(
                _In_ wf::DateTime date,
                _In_ wf::DateTime minDate,
                _In_ wf::DateTime maxDate);

             // Reacts to change in selection of our selectors. Calculates the new date represented by the selected indices and updates the
             // Date property.
            _Check_return_ HRESULT OnSelectorSelectionChanged(
                _In_ IInspectable* pSender,
                _In_ xaml_controls::ISelectionChangedEventArgs* pArgs);

            // Reacts to the changes in string typed properties. Reverts the property value to the last valid value,
            // if property change causes an exception.
            _Check_return_ HRESULT OnStringTypePropertyChanged(
                _In_ KnownPropertyIndex nPropertyIndex,
                _In_ IInspectable* pOldValue,
                _In_ IInspectable* pNewValue);

            // Reacts to changes in Date property. Day may have changed programmatically or end user may have changed the
            // selection of one of our selectors causing a change in Date.
            _Check_return_ HRESULT OnDateChanged(
                _In_ wf::DateTime oldValue,
                _In_ wf::DateTime newValue);

            // Reacts to the FlyoutButton being pressed. Invokes a form-factor specific flyout if present.
            _Check_return_ HRESULT OnFlyoutButtonClick(
                _In_ IInspectable* pSender,
                _In_ xaml::IRoutedEventArgs* pArgs);

            // Callback passed to the GetDatePickerSelectionAsync method. Called when a form-factor specific
            // flyout returns with a new DateTime value to update the DatePicker's DateTime.
            _Check_return_ HRESULT OnGetDatePickerSelectionAsyncCompleted(
                wf::IAsyncOperation<wf::IReference<wf::DateTime>*>* getOperation,
                wf::AsyncStatus status);

            // Given two calendars, finds the difference of years between them. Note that we are counting on the two
            // calendars will have the same system.
            _Check_return_ HRESULT GetYearDifference(
                _In_ wg::ICalendar* pStartCalendar,
                _In_ wg::ICalendar* pEndCalendar,
                _Inout_ INT32& difference);

            // Clears the ItemsSource and SelectedItem properties of the selectors.
            _Check_return_ HRESULT ClearSelectors(
                _In_ BOOLEAN clearDay,
                _In_ BOOLEAN clearMonth,
                _In_ BOOLEAN clearYear);

            // Generate the collection that we will populate our year picker with.
            _Check_return_ HRESULT GenerateYears();

            // Generate the collection that we will populate our month picker with.
            _Check_return_ HRESULT GenerateMonths(
                _In_ INT32 yearOffset);

            // Generate the collection that we will populate our day picker with.
            _Check_return_ HRESULT GenerateDays(
                _In_ INT32 yearOffset,
                _In_ INT32 monthOffset);

            // Regenerate the itemssource for the day/month/yearpickers and select the appropriate indices that represent the current DateTime.
            // Depending on which field changes we might not need to refresh some of the sources.
            _Check_return_ HRESULT RefreshSources(
                _In_ BOOLEAN refreshDay,
                _In_ BOOLEAN refreshMonth,
                _In_ BOOLEAN refreshYear);

            // Get indices of related fields of current Date for generated itemsources.
            _Check_return_ HRESULT GetIndices(
                _Inout_ INT32& yearIndex,
                _Inout_ INT32& monthIndex,
                _Inout_ INT32& dayIndex);

            _Check_return_ HRESULT OnCalendarChanged(
                _In_ IInspectable* pOldValue,
                _In_ IInspectable* pNewValue);

            // Interprets the selected indices of the selectors and creates and returns a DateTime corresponding to the date represented by these
            // indices.
            _Check_return_ HRESULT GetDateFromIndices(
                _In_ INT32 yearIndex,
                _In_ INT32 monthIndex,
                _In_ INT32 dayIndex,
                _Out_ wf::DateTime* date) noexcept;

            // The order of date fields vary depending on geographic region, calendar type etc. This function determines the M/D/Y order using
            // globalization APIs. It also determines whether the fields should be laid RTL.
            _Check_return_ HRESULT GetOrder(
                _Out_ INT32* yearOrder,
                _Out_ INT32* monthOrder,
                _Out_ INT32* dayOrder,
                _Out_ BOOLEAN* isRTL);

            // Updates the order of selectors in our layout. Also takes care of hiding/showing the comboboxes and related spacing depending our
            // public properties set by the user.
            _Check_return_ HRESULT UpdateOrderAndLayout() noexcept;

            // Triggers the DatePickerFlyout
            _Check_return_ HRESULT ShowPickerFlyout();

            // The selection of the selectors in our template can be changed by two sources. First source is
            // the end user changing a field to select the desired date. Second source is us updating
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
            _Check_return_ HRESULT GetSelectedDateAsString(_Out_ HSTRING* strPlainText);

            //Update button accessible name when selected time changes
            _Check_return_ HRESULT RefreshFlyoutButtonAutomationName();

            static wf::DateTime GetNullDateSentinel();
            static long GetNullDateSentinelValue();

            _Check_return_ HRESULT GetTodaysDate(_Out_ wf::DateTime* todaysDate);
            _Check_return_ HRESULT GetSelectedDate(_Out_ wf::DateTime* date);

            // Reference to a Button for invoking the DatePickerFlyout in the form factor APISet
            TrackerPtr<xaml_primitives::IButtonBase> m_tpFlyoutButton;

            // References to the TextBlocks that are used to display the Day/Month and Year.
            TrackerPtr<xaml_controls::ITextBlock> m_tpYearTextBlock;
            TrackerPtr<xaml_controls::ITextBlock> m_tpMonthTextBlock;
            TrackerPtr<xaml_controls::ITextBlock> m_tpDayTextBlock;

            TrackerPtr<TrackerCollection<IInspectable*>> m_tpDaySource;
            TrackerPtr<TrackerCollection<IInspectable*>> m_tpMonthSource;
            TrackerPtr<TrackerCollection<IInspectable*>> m_tpYearSource;

            // Reference to daypicker Selector. We need this as we will change its item
            // source as our properties change.
            TrackerPtr<xaml_primitives::ISelector> m_tpDayPicker;

            // Reference to monthpicker Selector. We need this as we will change its item
            // source as our properties change.
            TrackerPtr<xaml_primitives::ISelector> m_tpMonthPicker;

            // Reference to yearpicker Selector. We need this as we will change its item
            // source as our properties change.
            TrackerPtr<xaml_primitives::ISelector> m_tpYearPicker;

            // Reference to the Header content presenter. We need this to collapse the visibility
            // when the Header and HeaderTemplate are null.
            TrackerPtr<xaml::IUIElement> m_tpHeaderPresenter;

            // References to the hosting borders.
            TrackerPtr<xaml_controls::IBorder> m_tpFirstPickerHost;
            TrackerPtr<xaml_controls::IBorder> m_tpSecondPickerHost;
            TrackerPtr<xaml_controls::IBorder> m_tpThirdPickerHost;

            // References to the columns that will hold the day/month/year textblocks and the spacers.
            TrackerPtr<xaml_controls::IColumnDefinition> m_tpDayColumn;
            TrackerPtr<xaml_controls::IColumnDefinition> m_tpMonthColumn;
            TrackerPtr<xaml_controls::IColumnDefinition> m_tpYearColumn;
            TrackerPtr<xaml_controls::IColumnDefinition> m_tpFirstSpacerColumn;
            TrackerPtr<xaml_controls::IColumnDefinition> m_tpSecondSpacerColumn;

            // Reference to the grid that holds the day/month/year textblocks.
            TrackerPtr<xaml_controls::IGrid> m_tpFlyoutButtonContentGrid;

            // References to spacing borders which will act as our margins between the hosts.
            TrackerPtr<IUIElement> m_tpFirstPickerSpacing;
            TrackerPtr<IUIElement> m_tpSecondPickerSpacing;

            // Reference to our lay out root. We will be setting the flowdirection property on our root to achieve
            // RTL where necessary.
            TrackerPtr<IFrameworkElement> m_tpLayoutRoot;

            // This calendar will be used over and over while we are generating the ItemsSources instead
            // of creating new calendars.
            TrackerPtr<wg::ICalendar> m_tpCalendar;

            // This DateTimeFormatter will be used over and over when updating the FlyoutButton Content property
            TrackerPtr<wg::DateTimeFormatting::IDateTimeFormatter> m_tpDateFormatter;
            wrl_wrappers::HString m_strDateCalendarIdentifier;

            // We use Gregorian Calendar while calculating the default values of Min and Max years. Instead of creating
            // a new calendar every time these values are reached, we create one and reuse it.
            TrackerPtr<wg::ICalendar> m_tpGregorianCalendar;

            // Cached DateTimeFormatter for year and Calendar - Format pair it is related to.
            TrackerPtr<wg::DateTimeFormatting::IDateTimeFormatter> m_tpYearFormatter;
            wrl_wrappers::HString m_strYearFormat;
            wrl_wrappers::HString m_strYearCalendarIdentifier;

            // Cached DateTimeFormatter for year and Calendar - Format pair it is related to.
            TrackerPtr<wg::DateTimeFormatting::IDateTimeFormatter> m_tpMonthFormatter;
            wrl_wrappers::HString m_strMonthFormat;
            wrl_wrappers::HString m_strMonthCalendarIdentifier;

            // Cached DateTimeFormatter for year and Calendar - Format pair it is related to.
            TrackerPtr<wg::DateTimeFormatting::IDateTimeFormatter> m_tpDayFormatter;
            wrl_wrappers::HString m_strDayFormat;
            wrl_wrappers::HString m_strDayCalendarIdentifier;

            // Represent the first date choosable by datepicker. Note that the year of this date can be
            // different from the MinYear as the MinYear value can be unrepresentable depending on the
            // type of the calendar.
            wf::DateTime m_startDate;

            // The year of this date is the latest year that can be selectable by the date picker. Note that
            // month and date values do not necessarily represent the end date of  our date picker since we
            // do not need that information readily. Also note that, this year may be different from the MaxYear
            // since MaxYear may be unrepresentable depending on the calendar.
            wf::DateTime m_endDate;

            ctl::EventPtr<SelectionChangedEventCallback> m_epDaySelectionChangedHandler;
            ctl::EventPtr<SelectionChangedEventCallback> m_epMonthSelectionChangedHandler;
            ctl::EventPtr<SelectionChangedEventCallback> m_epYearSelectionChangedHandler;

            ctl::EventPtr<ButtonBaseClickEventCallback> m_epFlyoutButtonClickHandler;

            ctl::EventPtr<WindowActivatedEventCallback> m_windowActivatedHandler;

            // See the comment of AllowReactionToSelectionChange method for use of this variable.
            BOOLEAN m_reactionToSelectionChangeAllowed;

            BOOLEAN m_isInitializing;

            // Specifies if we have a valid year range to generate dates. We do not have a valid range if our minimum year is
            // greater than our maximum year.
            BOOLEAN m_hasValidYearRange;

            INT32 m_numberOfYears;

            // Default date to be used if no Date is set by the user.
            wf::DateTime m_defaultDate;

            // Today's date.
            wf::DateTime m_todaysDate;

            // Keeps track of the pending async operation and allows
            // for cancellation.
            TrackerPtr<IAsyncInfo> m_tpAsyncSelectionInfo;

            bool m_isPropagatingDate{ false };
    };
}
