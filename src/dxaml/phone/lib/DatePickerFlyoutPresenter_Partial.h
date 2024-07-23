// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <NamespaceAliases.h>
#include <windows.globalization.datetimeformatting.h>

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{
    class DatePickerFlyoutPresenter :
        public DatePickerFlyoutPresenterGenerated
    {
    public:
        DatePickerFlyoutPresenter();

        // IFrameworkElementOverrides
        _Check_return_ HRESULT
        OnApplyTemplateImpl() override;

        // IUIElementOverrides
        _Check_return_ HRESULT
        OnCreateAutomationPeerImpl(
            _Outptr_ xaml::Automation::Peers::IAutomationPeer **returnValue) override;

        // IDatePickerFlyoutPresenter
        _Check_return_ HRESULT OnPropertyChanged(
            _In_ xaml::IDependencyPropertyChangedEventArgs* pArgs);

        static _Check_return_ HRESULT GetDefaultIsDefaultShadowEnabled(_Outptr_ IInspectable** ppIsDefaultShadowEnabledValue);

        _Check_return_ HRESULT PullPropertiesFromOwner(_In_ xaml_controls::IDatePickerFlyout* pOwner);

        _Check_return_ HRESULT SetAcceptDismissButtonsVisibility(_In_ bool isVisible);

        _Check_return_ HRESULT GetDate(_Out_ wf::DateTime* pDate);

    protected:
        _Check_return_ HRESULT OnKeyDownImpl(_In_ xaml_input::IKeyRoutedEventArgs* pEventArgs) override sealed;

    private:
        ~DatePickerFlyoutPresenter() {}

        _Check_return_ HRESULT InitializeImpl() override;

        // Creates a new DateTimeFormatter with the given parameters.
        _Check_return_ HRESULT CreateNewFormatter(
            _In_ HSTRING strPattern,
            _In_ HSTRING strCalendarIdentifier,
            _Outptr_ wg::DateTimeFormatting::IDateTimeFormatter** ppDateTimeFormatter);

        // Returns the cached DateTimeFormatter for the given Calendar - Format pair for generating the strings
        // representing the years in our date range. If there isn't a cached DateTimeFormatter instance,
        // creates one and caches it to be returned for the following calls with the same pair.
        _Check_return_ HRESULT GetYearFormatter(
            _In_ HSTRING strCalendarIdentifier,
            _Outptr_ wg::DateTimeFormatting::IDateTimeFormatter** ppPrimaryDateTimeFormatter);

        // Returns the cached DateTimeFormatter for the given Calendar - Format pair for generating the strings
        // representing the months in our date range. If there isn't a cached DateTimeFormatter instance,
        // creates one and caches it to be returned for the following calls with the same pair.
        _Check_return_ HRESULT GetMonthFormatter(
            _In_ HSTRING strCalendarIdentifier,
            _Outptr_ wg::DateTimeFormatting::IDateTimeFormatter** ppPrimaryDateTimeFormatter);

        // Returns the cached DateTimeFormatter for the given Calendar - Format pair for generating the strings
        // representing the days in our date range. If there isn't a cached DateTimeFormatter instance,
        // creates one and caches it to be returned for the following calls with the same pair. Also returns a
        // formatter that will be used for
        _Check_return_ HRESULT GetDayFormatter(
            _In_ HSTRING strCalendarIdentifier,
            _Outptr_ wg::DateTimeFormatting::IDateTimeFormatter** ppPrimaryDateTimeFormatter);

        // Clears everything and refreshes the helper objects. After that, generates and
        // sets the itemssources to selectors.
         _Check_return_ HRESULT RefreshSetup();

         // We execute our logic depending on some state information such as start date, end date, number of years etc. These state
         // variables need to be updated whenever a public property change occurs which affects them.
         _Check_return_ HRESULT UpdateState();

         // Creates a new wg::Calendar, taking into account the Calendar Identifier
         // represented by our public "Calendar" property.
         _Check_return_ HRESULT CreateNewCalendar(
            _In_ HSTRING strCalendarIdentifier,
            _Inout_ wrl::ComPtr<wg::ICalendar>* pspCalendar);

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
         _Check_return_ HRESULT OnCalendarIdentifierPropertyChanged(
            _In_ HSTRING oldValue);

         _Check_return_ HRESULT SetDate(_In_ wf::DateTime newDate);

         // Reacts to changes in Date property. Day may have changed programmatically or end user may have changed the
         // selection of one of our selectors causing a change in Date.
         _Check_return_ HRESULT OnDateChanged(
            _In_ wf::DateTime oldValue,
            _In_ wf::DateTime newValue);

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
         _Check_return_ HRESULT RefreshSourcesAndSetSelectedIndices(
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
            _Out_ wf::DateTime* date);

         // The order of date fields vary depending on geographic region, calendar type etc. This function determines the M/D/Y order using
         // globalization APIs. It also determines whether the fields should be laid RTL.
         _Check_return_ HRESULT GetOrder(
            _Out_ INT32* yearOrder,
            _Out_ INT32* monthOrder,
            _Out_ INT32* dayOrder,
            _Out_ BOOLEAN* isRTL);

         // Updates the order of selectors in our layout. Also takes care of hiding/showing the comboboxes and related spacing depending our
         // public properties set by the user.
         _Check_return_ HRESULT UpdateOrderAndLayout();

         // The selection of the selectors in our template can be changed by two sources. First source is
         // the end user changing a field to select the desired date. Second source is us updating
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

         Private::TrackerPtr<wfc::IVector<IInspectable*>> _tpDaySource;
         Private::TrackerPtr<wfc::IVector<IInspectable*>> _tpMonthSource;
         Private::TrackerPtr<wfc::IVector<IInspectable*>> _tpYearSource;

         // Reference to daypicker Selector. We need this as we will change its item
         // source as our properties change.
         Private::TrackerPtr<xaml_primitives::ILoopingSelector> _tpDayPicker;

         // Reference to monthpicker Selector. We need this as we will change its item
         // source as our properties change.
         Private::TrackerPtr<xaml_primitives::ILoopingSelector> _tpMonthPicker;

         // Reference to yearpicker Selector. We need this as we will change its item
         // source as our properties change.
         Private::TrackerPtr<xaml_primitives::ILoopingSelector> _tpYearPicker;

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

         // Reference to the Grid that will hold the LoopingSelectors and the spacers.
         Private::TrackerPtr<xaml_controls::IGrid> _tpPickerHostGrid;

         // References to the columns of the Grid that will hold the day/month/year LoopingSelectors and the spacers.
         Private::TrackerPtr<xaml_controls::IColumnDefinition> _tpDayColumn;
         Private::TrackerPtr<xaml_controls::IColumnDefinition> _tpMonthColumn;
         Private::TrackerPtr<xaml_controls::IColumnDefinition> _tpYearColumn;
         Private::TrackerPtr<xaml_controls::IColumnDefinition> _tpFirstSpacerColumn;
         Private::TrackerPtr<xaml_controls::IColumnDefinition> _tpSecondSpacerColumn;

         // References to elements which will act as the dividers between the LoopingSelectors.
         Private::TrackerPtr<IUIElement> _tpFirstPickerSpacing;
         Private::TrackerPtr<IUIElement> _tpSecondPickerSpacing;

         // Reference to the background border
         Private::TrackerPtr<xaml_controls::IBorder> _tpBackgroundBorder;

         // Reference to the title presenter
         Private::TrackerPtr<xaml_controls::ITextBlock> _tpTitlePresenter;

         // Reference to our content panel. We will be setting the flowdirection property on our root to achieve
         // RTL where necessary.
         Private::TrackerPtr<IFrameworkElement> _tpContentPanel;

         // References to the elements for the accept and dismiss buttons.
         Private::TrackerPtr<IUIElement> _tpAcceptDismissHostGrid;
         Private::TrackerPtr<IUIElement> _tpAcceptButton;
         Private::TrackerPtr<IUIElement> _tpDismissButton;

         bool _acceptDismissButtonsVisible;

         // This calendar will be used over and over while we are generating the ItemsSources instead
         // of creating new calendars.
         Private::TrackerPtr<wg::ICalendar> _tpCalendar;
         Private::TrackerPtr<wg::ICalendar> _tpBaselineCalendar;

         // We use Gregorian Calendar while calculating the default values of Min and Max years. Instead of creating
         // a new calendar every time these values are reached, we create one and reuse it.
         Private::TrackerPtr<wg::ICalendar> _tpGregorianCalendar;

         // Cached DateTimeFormatter for year and Calendar - Format pair it is related to.
         Private::TrackerPtr<wg::DateTimeFormatting::IDateTimeFormatter> _tpPrimaryYearFormatter;
         wrl_wrappers::HString _strYearCalendarIdentifier;

         // Cached DateTimeFormatter for year and Calendar - Format pair it is related to.
         Private::TrackerPtr<wg::DateTimeFormatting::IDateTimeFormatter> _tpPrimaryMonthFormatter;
         wrl_wrappers::HString _strMonthCalendarIdentifier;

         // Cached DateTimeFormatter for year and Calendar - Format pair it is related to.
         Private::TrackerPtr<wg::DateTimeFormatting::IDateTimeFormatter> _tpPrimaryDayFormatter;
         wrl_wrappers::HString _strDayCalendarIdentifier;

         // Represent the first date choosable by datepicker. Note that the year of this date can be
         // different from the MinYear as the MinYear value can be unrepresentable depending on the
         // type of the calendar.
         wf::DateTime _startDate{};

         // The year of this date is the latest year that can be selectable by the date picker. Note that
         // month and date values do not necessarily represent the end date of  our date picker since we
         // do not need that information readily. Also note that, this year may be different from the MaxYear
         // since MaxYear may be unrepresentable depending on the calendar.
         wf::DateTime _endDate{};

         EventRegistrationToken _daySelectionChangedToken{};
         EventRegistrationToken _monthSelectionChangedToken{};
         EventRegistrationToken _yearSelectionChangedToken{};

         // See the comment of AllowReactionToSelectionChange method for use of this variable.
         BOOLEAN _reactionToSelectionChangeAllowed;

         BOOLEAN _isInitializing;

         // Specifies if we have a valid year range to generate dates. We do not have a valid range if our minimum year is
         // greater than our maximum year.
         BOOLEAN _hasValidYearRange;

         INT32 _numberOfYears;

         // Properties pulled from owner DatePickerFlyout
         wrl_wrappers::HString _calendarIdentifier;
         wrl_wrappers::HString _title;
         BOOLEAN _dayVisible;
         BOOLEAN _monthVisible;
         BOOLEAN _yearVisible;
         wf::DateTime _minYear;
         wf::DateTime _maxYear;
         wf::DateTime _date; // Will be modified to reflect the user's selection
         wrl_wrappers::HString _dayFormat;
         wrl_wrappers::HString _monthFormat;
         wrl_wrappers::HString _yearFormat;

         static const WCHAR _firstPickerHostName[];
         static const WCHAR _secondPickerHostName[];
         static const WCHAR _thirdPickerHostName[];
         static const WCHAR _backgroundName[];
         static const WCHAR _contentPanelName[];
         static const WCHAR _titlePresenterName[];

         static const WCHAR _dayLoopingSelectorAutomationId[];
         static const WCHAR _monthLoopingSelectorAutomationId[];
         static const WCHAR _yearLoopingSelectorAutomationId[];
    };

    ActivatableClass(DatePickerFlyoutPresenter);

}
} } } XAML_ABI_NAMESPACE_END
