// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//
//      Represents DatePicker control. DatePicker is a XAML UI control that allows
//      the selection of dates.  DatePicker supports formatting of dates for the various
//      international calendars supported by Windows.  The purpose of the control is
//      to provide a standardized, touch-based XAML control for selecting localized dates.

#include "precomp.h"
#include "DatePicker.g.h"
#include "ComboBox.g.h"
#include "Selector.g.h"
#include "ItemsControl.g.h"
#include "DatePickerValueChangedEventArgs.g.h"
#include "DatePickerSelectedValueChangedEventArgs.g.h"
#include "Button.g.h"
#include "Border.g.h"
#include "TextBlock.g.h"
#include "UIElement.g.h"
#include "DatePickerAutomationPeer.g.h"
#include "Window.g.h"
#include <xstrutil.h>
#include "AutomationProperties.h"
#include "localizedResource.h"
#include <PhoneImports.h>
#include <windows.globalization.datetimeformatting.h>

#define DATEPICKER_RTL_CHARACTER_CODE 8207
#define DATEPICKER_MIN_MAX_YEAR_DEAFULT_OFFSET 100
#define DATEPICKER_SENTINELTIME_HOUR 12
#define DATEPICKER_SENTINELTIME_MINUTE 0
#define DATEPICKER_SENTINELTIME_SECOND 0
#define DATEPICKER_WRAP_AROUND_MONTHS_FIRST_INDEX 1

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace std::placeholders;

#undef max
#undef min

DatePicker::DatePicker()
    : m_numberOfYears(0),
      m_reactionToSelectionChangeAllowed(TRUE),
      m_isInitializing(TRUE),
      m_hasValidYearRange(FALSE)
{
    m_startDate.UniversalTime = GetNullDateSentinelValue();
    m_endDate.UniversalTime = GetNullDateSentinelValue();
    m_defaultDate.UniversalTime = GetNullDateSentinelValue();
    m_todaysDate.UniversalTime = GetNullDateSentinelValue();
}

DatePicker::~DatePicker()
{
    // This will ensure the pending async operation
    // completes, closed the open dialog, and doesn't
    // try to execute a callback to a DatePicker that
    // no longer exists.
    if (m_tpAsyncSelectionInfo)
    {
        VERIFYHR(m_tpAsyncSelectionInfo->Cancel());
    }

    if (m_windowActivatedHandler && DXamlCore::GetCurrent() != nullptr)
    {
        Window* window = nullptr;
        VERIFYHR(DXamlCore::GetCurrent()->GetAssociatedWindowNoRef(this, &window));
        if (window)
        {
            VERIFYHR(m_windowActivatedHandler.DetachEventHandler(ctl::iinspectable_cast(window)));
        }
    }
}

// Initialize the DatePicker
_Check_return_ HRESULT DatePicker::PrepareState()
{
    IFC_RETURN(DatePickerGenerated::PrepareState());

    // We should update our state during initialization because we still want our dps to function properly
    // until we get applied a template, to do this we need our state information.
    IFC_RETURN(UpdateState());
    return S_OK;
}

// Called when the IsEnabled property changes.
_Check_return_ HRESULT DatePicker::OnIsEnabledChanged(_In_ IsEnabledChangedEventArgs* pArgs)
{
    IFC_RETURN(UpdateVisualState());
    return S_OK;
}

// Change to the correct visual state for the DatePicker.
_Check_return_ HRESULT DatePicker::ChangeVisualState(_In_ bool useTransitions)
{
    BOOLEAN isEnabled = FALSE;
    BOOLEAN ignored = FALSE;

    IFC_RETURN(get_IsEnabled(&isEnabled));

    if (!isEnabled)
    {
        IFC_RETURN(GoToState(useTransitions, L"Disabled", &ignored));
    }
    else
    {
        IFC_RETURN(GoToState(useTransitions, L"Normal", &ignored));
    }

#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
    // HeaderStates VisualStateGroup.
    xaml_controls::ControlHeaderPlacement headerPlacement = xaml_controls::ControlHeaderPlacement_Top;
    IFC_RETURN(get_HeaderPlacement(&headerPlacement));

    switch (headerPlacement)
    {
        case DirectUI::ControlHeaderPlacement::Top:
            IFC_RETURN(GoToState(useTransitions, L"TopHeader", &ignored));
            break;

        case DirectUI::ControlHeaderPlacement::Left:
            IFC_RETURN(GoToState(useTransitions, L"LeftHeader", &ignored));
            break;
    }
#endif

    ctl::ComPtr<wf::IReference<wf::DateTime>> selectedDate;
    IFC_RETURN(get_SelectedDate(&selectedDate));

    if (selectedDate)
    {
        IFC_RETURN(GoToState(useTransitions, L"HasDate", &ignored));
    }
    else
    {
        IFC_RETURN(GoToState(useTransitions, L"HasNoDate", &ignored));
    }

    return S_OK;
}

_Check_return_ HRESULT DatePicker::OnKeyDown(_In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    IFCPTR_RETURN(pArgs);
    wsy::VirtualKeyModifiers nModifierKeys;
    wsy::VirtualKey key = wsy::VirtualKey_None;

    IFC_RETURN(pArgs->get_Key(&key));
    IFC_RETURN(CoreImports::Input_GetKeyboardModifiers(&nModifierKeys));

    // Alt+Up or Alt+Down opens the DatePickerFlyout
    // but only if a FlyoutButton is present (we don't want to be able to trigger the flyout with the keyboard but not the mouse)
    if ((key == wsy::VirtualKey_Up || key == wsy::VirtualKey_Down)
        && (0 != (nModifierKeys & wsy::VirtualKeyModifiers_Menu))
        && m_tpFlyoutButton)
    {
        IFC_RETURN(pArgs->put_Handled(TRUE));
        IFC_RETURN(ShowPickerFlyout());
    }

    return S_OK;
}

// Get DatePicker template parts and create the sources if they are not already there
IFACEMETHODIMP
DatePicker::OnApplyTemplate()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ISelector> spDayPicker;
    ctl::ComPtr<ISelector> spMonthPicker;
    ctl::ComPtr<ISelector> spYearPicker;
    ctl::ComPtr<IBorder> spFirstPickerHost;
    ctl::ComPtr<IBorder> spSecondPickerHost;
    ctl::ComPtr<IBorder> spThirdPickerHost;
    ctl::ComPtr<IFrameworkElement> spLayoutRoot;
    ctl::ComPtr<IUIElement> spSpacingHolderOne;
    ctl::ComPtr<IUIElement> spSpacingHolderTwo;
    ctl::ComPtr<IButtonBase> spFlyoutButton;
    ctl::ComPtr<ITextBlock> spYearTextBlock;
    ctl::ComPtr<ITextBlock> spMonthTextBlock;
    ctl::ComPtr<ITextBlock> spDayTextBlock;
    ctl::ComPtr<IColumnDefinition> spDayColumn;
    ctl::ComPtr<IColumnDefinition> spMonthColumn;
    ctl::ComPtr<IColumnDefinition> spYearColumn;
    ctl::ComPtr<IColumnDefinition> spFirstSpacerColumn;
    ctl::ComPtr<IColumnDefinition> spSecondSpacerColumn;
    ctl::ComPtr<IGrid> spFlyoutButtonContentGrid;
    ctl::ComPtr<TrackerCollection<IInspectable*>> spCollection;

    wrl_wrappers::HString strAutomationName;
    wrl_wrappers::HString strParentAutomationName;
    wrl_wrappers::HString strComboAutomationName;

    //Clean up existing parts
    if (m_tpDayPicker.Get())
    {
        IFC(m_epDaySelectionChangedHandler.DetachEventHandler(m_tpDayPicker.Get()));
    }

    if (m_tpMonthPicker.Get())
    {
        IFC(m_epMonthSelectionChangedHandler.DetachEventHandler(m_tpMonthPicker.Get()));
    }

    if (m_tpYearPicker.Get())
    {
        IFC(m_epYearSelectionChangedHandler.DetachEventHandler(m_tpYearPicker.Get()));
    }

    if (m_tpFlyoutButton.Get())
    {
        IFC(m_epFlyoutButtonClickHandler.DetachEventHandler(m_tpFlyoutButton.Get()));
    }

    m_tpDayPicker.Clear();
    m_tpMonthPicker.Clear();
    m_tpYearPicker.Clear();

    m_tpFirstPickerHost.Clear();
    m_tpSecondPickerHost.Clear();
    m_tpThirdPickerHost.Clear();

    m_tpDayColumn.Clear();
    m_tpMonthColumn.Clear();
    m_tpYearColumn.Clear();
    m_tpFirstSpacerColumn.Clear();
    m_tpSecondSpacerColumn.Clear();

    m_tpFirstPickerSpacing.Clear();
    m_tpSecondPickerSpacing.Clear();
    m_tpLayoutRoot.Clear();
    m_tpHeaderPresenter.Clear();

    m_tpFlyoutButton.Clear();
    m_tpFlyoutButtonContentGrid.Clear();

    m_tpYearTextBlock.Clear();
    m_tpMonthTextBlock.Clear();
    m_tpDayTextBlock.Clear();

    IFC(DatePickerGenerated::OnApplyTemplate());

    // Get selectors for day/month/year pickers
    IFC(GetTemplatePart<ISelector>(STR_LEN_PAIR(L"DayPicker"), spDayPicker.ReleaseAndGetAddressOf()));
    IFC(GetTemplatePart<ISelector>(STR_LEN_PAIR(L"MonthPicker"), spMonthPicker.ReleaseAndGetAddressOf()));
    IFC(GetTemplatePart<ISelector>(STR_LEN_PAIR(L"YearPicker"), spYearPicker.ReleaseAndGetAddressOf()));

    // Get the borders which will be hosting the selectors
    IFC(GetTemplatePart<IBorder>(STR_LEN_PAIR(L"FirstPickerHost"), spFirstPickerHost.ReleaseAndGetAddressOf()));
    IFC(GetTemplatePart<IBorder>(STR_LEN_PAIR(L"SecondPickerHost"), spSecondPickerHost.ReleaseAndGetAddressOf()));
    IFC(GetTemplatePart<IBorder>(STR_LEN_PAIR(L"ThirdPickerHost"), spThirdPickerHost.ReleaseAndGetAddressOf()));

    IFC(GetTemplatePart<IColumnDefinition>(STR_LEN_PAIR(L"DayColumn"), spDayColumn.ReleaseAndGetAddressOf()));
    IFC(GetTemplatePart<IColumnDefinition>(STR_LEN_PAIR(L"MonthColumn"), spMonthColumn.ReleaseAndGetAddressOf()));
    IFC(GetTemplatePart<IColumnDefinition>(STR_LEN_PAIR(L"YearColumn"), spYearColumn.ReleaseAndGetAddressOf()));
    IFC(GetTemplatePart<IColumnDefinition>(STR_LEN_PAIR(L"FirstSpacerColumn"), spFirstSpacerColumn.ReleaseAndGetAddressOf()));
    IFC(GetTemplatePart<IColumnDefinition>(STR_LEN_PAIR(L"SecondSpacerColumn"), spSecondSpacerColumn.ReleaseAndGetAddressOf()));

    IFC(GetTemplatePart<ITextBlock>(STR_LEN_PAIR(L"YearTextBlock"), spYearTextBlock.ReleaseAndGetAddressOf()));
    IFC(GetTemplatePart<ITextBlock>(STR_LEN_PAIR(L"MonthTextBlock"), spMonthTextBlock.ReleaseAndGetAddressOf()));
    IFC(GetTemplatePart<ITextBlock>(STR_LEN_PAIR(L"DayTextBlock"), spDayTextBlock.ReleaseAndGetAddressOf()));

    IFC(GetTemplatePart<IGrid>(STR_LEN_PAIR(L"FlyoutButtonContentGrid"), spFlyoutButtonContentGrid.ReleaseAndGetAddressOf()));


    // Get the spacing holders which will be acting as margins between the hosts
    IFC(GetTemplatePart<IUIElement>(STR_LEN_PAIR(L"FirstPickerSpacing"), spSpacingHolderOne.ReleaseAndGetAddressOf()));
    IFC(GetTemplatePart<IUIElement>(STR_LEN_PAIR(L"SecondPickerSpacing"), spSpacingHolderTwo.ReleaseAndGetAddressOf()));
    IFC(GetTemplatePart<IFrameworkElement>(STR_LEN_PAIR(L"LayoutRoot"), spLayoutRoot.ReleaseAndGetAddressOf()));

    IFC(GetTemplatePart<IButtonBase>(STR_LEN_PAIR(L"FlyoutButton"), spFlyoutButton.ReleaseAndGetAddressOf()));

    SetPtrValue(m_tpDayPicker, spDayPicker.Get());
    SetPtrValue(m_tpMonthPicker, spMonthPicker.Get());
    SetPtrValue(m_tpYearPicker, spYearPicker.Get());

    SetPtrValue(m_tpFirstPickerSpacing, spSpacingHolderOne.Get());
    SetPtrValue(m_tpSecondPickerSpacing, spSpacingHolderTwo.Get());

    SetPtrValue(m_tpFirstPickerHost, spFirstPickerHost.Get());
    SetPtrValue(m_tpSecondPickerHost, spSecondPickerHost.Get());
    SetPtrValue(m_tpThirdPickerHost, spThirdPickerHost.Get());

    SetPtrValue(m_tpDayColumn, spDayColumn.Get());
    SetPtrValue(m_tpMonthColumn, spMonthColumn.Get());
    SetPtrValue(m_tpYearColumn, spYearColumn.Get());
    SetPtrValue(m_tpFirstSpacerColumn, spFirstSpacerColumn.Get());
    SetPtrValue(m_tpSecondSpacerColumn, spSecondSpacerColumn.Get());

    SetPtrValue(m_tpLayoutRoot, spLayoutRoot.Get());

    SetPtrValue(m_tpYearTextBlock, spYearTextBlock.Get());
    SetPtrValue(m_tpMonthTextBlock, spMonthTextBlock.Get());
    SetPtrValue(m_tpDayTextBlock, spDayTextBlock.Get());

    SetPtrValue(m_tpFlyoutButton, spFlyoutButton.Get());
    SetPtrValue(m_tpFlyoutButtonContentGrid, spFlyoutButtonContentGrid.Get());

    IFC(UpdateHeaderPresenterVisibility());

    IFC(DirectUI::AutomationProperties::GetNameStatic(this, strParentAutomationName.ReleaseAndGetAddressOf()));
    if (strParentAutomationName.Length() == 0)
    {
        ctl::ComPtr<IInspectable> spHeaderAsInspectable;
        IFC(get_Header(&spHeaderAsInspectable));
        if (spHeaderAsInspectable)
        {
            IFC(FrameworkElement::GetStringFromObject(spHeaderAsInspectable.Get(), strParentAutomationName.ReleaseAndGetAddressOf()));
        }
    }
    // Hook up the selection changed events for selectors, we will be reacting to these events.
    if (m_tpDayPicker.Get())
    {
        IFC(m_epDaySelectionChangedHandler.AttachEventHandler(m_tpDayPicker.Get(),
            std::bind(&DatePicker::OnSelectorSelectionChanged, this, _1, _2)));

        IFC(DirectUI::AutomationProperties::GetNameStatic(m_tpDayPicker.Cast<ComboBox>(), strAutomationName.ReleaseAndGetAddressOf()));
        if(strAutomationName.Get() == NULL)
        {
            IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_DATEPICKER_DAY, strAutomationName.ReleaseAndGetAddressOf()));
            IFC(strAutomationName.Concat(strParentAutomationName, strComboAutomationName));
            IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpDayPicker.Cast<ComboBox>(), strComboAutomationName.Get()));
        }
    }
    if (m_tpMonthPicker.Get())
    {
        IFC(m_epMonthSelectionChangedHandler.AttachEventHandler(m_tpMonthPicker.Get(),
            std::bind(&DatePicker::OnSelectorSelectionChanged, this, _1, _2)));

        IFC(DirectUI::AutomationProperties::GetNameStatic(m_tpMonthPicker.Cast<ComboBox>(), strAutomationName.ReleaseAndGetAddressOf()));
        if(strAutomationName.Get() == NULL)
        {
            IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_DATEPICKER_MONTH, strAutomationName.ReleaseAndGetAddressOf()));
            IFC(strAutomationName.Concat(strParentAutomationName, strComboAutomationName));
            IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpMonthPicker.Cast<ComboBox>(), strComboAutomationName));
        }
    }
    if (m_tpYearPicker.Get())
    {
        IFC(m_epYearSelectionChangedHandler.AttachEventHandler(m_tpYearPicker.Get(),
            std::bind(&DatePicker::OnSelectorSelectionChanged, this, _1, _2)));

        IFC(DirectUI::AutomationProperties::GetNameStatic(m_tpYearPicker.Cast<ComboBox>(), strAutomationName.ReleaseAndGetAddressOf()));
        if(strAutomationName.Get() == NULL)
        {
            IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_DATEPICKER_YEAR, strAutomationName.ReleaseAndGetAddressOf()));
            IFC(strAutomationName.Concat(strParentAutomationName, strComboAutomationName));
            IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpYearPicker.Cast<ComboBox>(), strComboAutomationName));
        }
    }

    if (m_tpFlyoutButton.Get())
    {
        IFC(m_epFlyoutButtonClickHandler.AttachEventHandler(m_tpFlyoutButton.Get(),
            std::bind(&DatePicker::OnFlyoutButtonClick, this, _1, _2)));

        IFC(RefreshFlyoutButtonAutomationName());
    }

    Window* window = nullptr;
    VERIFYHR(DXamlCore::GetCurrent()->GetAssociatedWindowNoRef(this, &window));
    if (window)
    {
        ctl::WeakRefPtr weakInstance;
        IFC(ctl::AsWeak(this, &weakInstance));

        IFC(m_windowActivatedHandler.AttachEventHandler(
            window,
            [weakInstance] (IInspectable*, xaml::IWindowActivatedEventArgs* pArgs) mutable
            {
                HRESULT hr = S_OK;
                ctl::ComPtr<DatePicker> instance  = weakInstance.AsOrNull<DatePicker>();
                if (instance != nullptr)
                {
                    xaml::WindowActivationState state =
                        xaml::WindowActivationState_CodeActivated;
                    IFC(pArgs->get_WindowActivationState(&state));

                    if (state == xaml::WindowActivationState_CodeActivated
                        || state == xaml::WindowActivationState_PointerActivated)
                    {
                        IFC(instance->RefreshSetup());
                    }
                }

            Cleanup:
                RRETURN(hr);
            }));
    }

    // Create the collections that we will use as itemssources for the selectors.
    if (!(m_tpYearSource.Get() && m_tpMonthSource.Get() && m_tpDaySource.Get()))
    {
        IFC(ctl::make(&spCollection));
        SetPtrValue(m_tpYearSource, spCollection.Get());

        IFC(ctl::make(&spCollection));
        SetPtrValue(m_tpMonthSource, spCollection.Get());

        IFC(ctl::make(&spCollection));
        SetPtrValue(m_tpDaySource, spCollection.Get());
    }

    IFC(RefreshSetup());

    IFC(UpdateVisualState(FALSE));

Cleanup:
    m_isInitializing = FALSE;
    RRETURN(hr);
}

// Clears the ItemsSource and SelectedItem properties of the selectors.
_Check_return_
HRESULT
DatePicker::ClearSelectors(
    _In_ BOOLEAN clearDay,
    _In_ BOOLEAN clearMonth,
    _In_ BOOLEAN clearYear)
{
    HRESULT hr = S_OK;

    //Clear Selector SelectedItems and ItemSources
    const CDependencyProperty* pItemsSourceProperty = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ItemsControl_ItemsSource);

    if (m_tpDayPicker.Get() && clearDay)
    {
        IFC(m_tpDayPicker.Cast<Selector>()->ClearValue(pItemsSourceProperty));
    }

    if (m_tpMonthPicker.Get() && clearMonth)
    {
        IFC(m_tpMonthPicker.Cast<Selector>()->ClearValue(pItemsSourceProperty));
    }

    if (m_tpYearPicker.Get() && clearYear)
    {
        IFC(m_tpYearPicker.Cast<Selector>()->ClearValue(pItemsSourceProperty));
    }

Cleanup:
    RRETURN(hr);
}

// Get indices of related fields of current Date for generated itemsources.
_Check_return_
HRESULT
DatePicker::GetIndices(
    _Inout_ INT32& yearIndex,
    _Inout_ INT32& monthIndex,
    _Inout_ INT32& dayIndex)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wg::ICalendar> spCurrentCalendar;
    wf::DateTime currentDate = {};
    wrl_wrappers::HString strCalendarIdentifier;
    INT32 currentIndex = 0;
    INT32 firstIndex = 0;
    INT32 monthsInThisYear = 0;

    IFC(GetSelectedDate(&currentDate));
    IFC(get_CalendarIdentifier(strCalendarIdentifier.GetAddressOf()));

    // We will need the second calendar for calculating the year difference
    IFC(CreateNewCalendar(strCalendarIdentifier.Get(), &spCurrentCalendar));
#ifndef _PREFAST_ // PREfast bug DevDiv:554051
    ASSERT(spCurrentCalendar);
#endif

    IFC(spCurrentCalendar->SetDateTime(ClampDate(currentDate, m_startDate, m_endDate)));
    IFC(m_tpCalendar->SetDateTime(m_startDate));
    IFC(GetYearDifference(m_tpCalendar.Get(), spCurrentCalendar.Get(), yearIndex));

    IFC(spCurrentCalendar->get_FirstMonthInThisYear(&firstIndex));
    IFC(spCurrentCalendar->get_Month(&currentIndex));
    IFC(spCurrentCalendar->get_NumberOfMonthsInThisYear(&monthsInThisYear));
    if (currentIndex - firstIndex >= 0)
    {
        monthIndex = currentIndex - firstIndex;
    }
    else
    {
        // A special case is in some ThaiCalendar years first month
        // of the year is April, last month is March and month flow is wrap-around
        // style; April, March .... November, December, January, February, March. So the first index
        // will be 4 and last index will be 3. We are handling the case to convert this wraparound behavior
        // into selected index.
        monthIndex = currentIndex - firstIndex + monthsInThisYear;
    }
    IFC(spCurrentCalendar->get_FirstDayInThisMonth(&firstIndex));
    IFC(spCurrentCalendar->get_Day(&currentIndex));
    dayIndex = currentIndex - firstIndex;

Cleanup:
    RRETURN(hr);
}

// Clears everything and refreshes the helper objects. After that, generates and
// sets the itemssources to selectors.
_Check_return_
HRESULT
DatePicker::RefreshSetup()
{
    HRESULT hr = S_OK;

    // Since we will be clearing itemssources / selecteditems and putting new ones, selection changes will be fired from the
    // selectors. However, we do not want to process them as if the end user has purposefully changed the selection on a selector.
    PreventReactionToSelectionChange();

    // This will recalculate the startyear/endyear etc and will tell us if we have a valid range to generate sources.
    IFC(UpdateState());

    if (m_hasValidYearRange)
    {
        // When we are refreshing all our setup, year selector should
        // also be refreshed.
        IFC(RefreshSources(TRUE /*Refresh day */, TRUE /* Refresh month*/, TRUE /* Refresh year */));

        wf::DateTime currentDate = {};
        IFC(get_Date(&currentDate));

        if (currentDate.UniversalTime != GetNullDateSentinelValue())
        {
            INT32 yearIndex = 0;
            INT32 monthIndex = 0;
            INT32 dayIndex = 0;
            wf::DateTime date = {};

            // If we refreshed our set up due to a property change, this may have caused us to coerce and change the current displayed date. For example
            // min/max year changes may have force us to coerce the current datetime to the nearest value inside the valid range.
            // So, we should update our DateTime property. If there is a change, we will end up firing the event as desired, if there isn't a change
            // we will just no_op.
            IFC(GetIndices(yearIndex, monthIndex, dayIndex));
            IFC(GetDateFromIndices(yearIndex, monthIndex, dayIndex, &date));
            // We are checking to see if new value is different from the current one. This is because even if they are same,
            // calling put_Date will break any Binding on Date (if there is any) that this DatePicker is target of.
            if (currentDate.UniversalTime != date.UniversalTime)
            {
                IFC(put_Date(date));
            }
        }
    }

Cleanup:
    AllowReactionToSelectionChange();
    RRETURN(hr);
}

 // Regenerate the itemssource for the day/month/yearpickers and select the appropriate indices that represent the current DateTime.
 // Depending on which field changes we might not need to refresh some of the sources.
_Check_return_
HRESULT
DatePicker::RefreshSources(
    _In_ BOOLEAN refreshDay,
    _In_ BOOLEAN refreshMonth,
    _In_ BOOLEAN refreshYear)
{
    HRESULT hr = S_OK;
    INT32 yearIndex = 0;
    INT32 monthIndex = 0;
    INT32 dayIndex = 0;

    PreventReactionToSelectionChange();

    IFC(GetIndices(yearIndex, monthIndex, dayIndex));

    IFC(ClearSelectors(refreshDay, refreshMonth, refreshYear));

    if (m_tpYearPicker.Get())
    {
        if (refreshYear)
        {
            IFC(GenerateYears());
            IFC(m_tpYearPicker.Cast<Selector>()->put_ItemsSource(ctl::iinspectable_cast(m_tpYearSource.Get())));
        }
        IFC(m_tpYearPicker->put_SelectedIndex(yearIndex));
    }

    if (m_tpMonthPicker.Get())
    {
        if (refreshMonth)
        {
            IFC(GenerateMonths(yearIndex));
            IFC(m_tpMonthPicker.Cast<Selector>()->put_ItemsSource(ctl::iinspectable_cast(m_tpMonthSource.Get())));
        }
        IFC(m_tpMonthPicker->put_SelectedIndex(monthIndex));
    }

    if (m_tpDayPicker.Get())
    {
        if(refreshDay)
        {
            IFC(GenerateDays(yearIndex, monthIndex));
            IFC(m_tpDayPicker.Cast<Selector>()->put_ItemsSource(ctl::iinspectable_cast(m_tpDaySource.Get())));
        }
        IFC(m_tpDayPicker->put_SelectedIndex(dayIndex));
    }

    if (m_tpFlyoutButton.Get())
    {
        IFC(UpdateFlyoutButtonContent());
    }

Cleanup:
    AllowReactionToSelectionChange();
    RRETURN(hr);
}

// Generate the collection that we will populate our year picker with.
_Check_return_
HRESULT
DatePicker::GenerateYears()
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString strYearFormat;
    wrl_wrappers::HString strYear;
    wrl_wrappers::HString strCalendarIdentifier;
    ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatter;
    ctl::ComPtr<IInspectable> spInspectable;
    wf::DateTime dateTime;

    IFC(get_CalendarIdentifier(strCalendarIdentifier.GetAddressOf()));
    IFC(get_YearFormat(strYearFormat.GetAddressOf()));
    IFC(GetYearFormatter(strYearFormat.Get(), strCalendarIdentifier.Get(), &spFormatter));

    IFC(m_tpYearSource->Clear());

    for (INT32 yearOffset = 0; yearOffset < m_numberOfYears; yearOffset++)
    {
        IFC(m_tpCalendar->SetDateTime(m_startDate));
        IFC(m_tpCalendar->AddYears(yearOffset));

        IFC(m_tpCalendar->GetDateTime(&dateTime));
        IFC(spFormatter->Format(dateTime, strYear.ReleaseAndGetAddressOf()));

        IFC(DirectUI::PropertyValue::CreateFromString(strYear.Get(), &spInspectable));
        IFC(m_tpYearSource->Append(spInspectable.Get()));
    }

Cleanup:
    RRETURN(hr);
}

// Generate the collection that we will populate our month picker with.
_Check_return_
HRESULT
DatePicker::GenerateMonths(
    _In_ INT32 yearOffset)
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString strMonthFormat;
    wrl_wrappers::HString strMonth;
    wrl_wrappers::HString strCalendarIdentifier;
    ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatter;
    ctl::ComPtr<IInspectable> spInspectable;
    wf::DateTime dateTime;
    INT32 monthOffset = 0;
    INT32 numberOfMonths = 0;
    INT32 firstMonthInThisYear = 0;

    IFC(get_CalendarIdentifier(strCalendarIdentifier.GetAddressOf()));
    IFC(get_MonthFormat(strMonthFormat.GetAddressOf()));
    IFC(GetMonthFormatter(strMonthFormat.Get(), strCalendarIdentifier.Get(), &spFormatter));

    IFC(m_tpCalendar->SetDateTime(m_startDate));
    IFC(m_tpCalendar->AddYears(yearOffset));
    IFC(m_tpCalendar->get_NumberOfMonthsInThisYear(&numberOfMonths));
    IFC(m_tpCalendar->get_FirstMonthInThisYear(&firstMonthInThisYear));

    IFC(m_tpMonthSource->Clear());

    for (monthOffset = 0; monthOffset < numberOfMonths; monthOffset++)
    {
        IFC(m_tpCalendar->put_Month(firstMonthInThisYear));
        IFC(m_tpCalendar->AddMonths(monthOffset));
        IFC(m_tpCalendar->GetDateTime(&dateTime));
        IFC(spFormatter->Format(dateTime, strMonth.ReleaseAndGetAddressOf()));

        IFC(DirectUI::PropertyValue::CreateFromString(strMonth.Get(), &spInspectable));
        IFC(m_tpMonthSource->Append(spInspectable.Get()));
    }

Cleanup:
    RRETURN(hr);
}

// Generate the collection that we will populate our day picker with.
_Check_return_
HRESULT
DatePicker::GenerateDays(
    _In_ INT32 yearOffset,
    _In_ INT32 monthOffset)
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString strDayFormat;
    wrl_wrappers::HString strCalendarIdentifier;
    wrl_wrappers::HString strDay;
    ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatter;
    ctl::ComPtr<IInspectable> spInspectable;
    wf::DateTime dateTime;
    INT32 dayOffset = 0;
    INT32 numberOfDays = 0;
    INT32 firstDayInThisMonth = 0;
    INT32 firstMonthInThisYear = 0;

    IFC(get_CalendarIdentifier(strCalendarIdentifier.GetAddressOf()));
    IFC(get_DayFormat(strDayFormat.GetAddressOf()));
    IFC(GetDayFormatter(strDayFormat.Get(), strCalendarIdentifier.Get(), &spFormatter));

    IFC(m_tpCalendar->SetDateTime(m_startDate));
    IFC(m_tpCalendar->AddYears(yearOffset));
    IFC(m_tpCalendar->get_FirstMonthInThisYear(&firstMonthInThisYear));
    IFC(m_tpCalendar->put_Month(firstMonthInThisYear));
    IFC(m_tpCalendar->AddMonths(monthOffset));
    IFC(m_tpCalendar->get_NumberOfDaysInThisMonth(&numberOfDays));
    IFC(m_tpCalendar->get_FirstDayInThisMonth(&firstDayInThisMonth));

    IFC(m_tpDaySource->Clear());

    for (dayOffset = 0; dayOffset < numberOfDays; dayOffset++)
    {
        IFC(m_tpCalendar->put_Day(firstDayInThisMonth + dayOffset));
        IFC(m_tpCalendar->GetDateTime(&dateTime));
        IFC(spFormatter->Format(dateTime, strDay.ReleaseAndGetAddressOf()));

        IFC(DirectUI::PropertyValue::CreateFromString(strDay.Get(), &spInspectable));
        IFC(m_tpDaySource->Append(spInspectable.Get()));
    }

Cleanup:
   RRETURN(hr);
}

// Reacts to change in selection of our selectors. Calculates the new date represented by the selected indices and updates the
// Date property.
_Check_return_
HRESULT
DatePicker::OnSelectorSelectionChanged(
    _In_ IInspectable* pSender,
    _In_ xaml_controls::ISelectionChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    if (IsReactionToSelectionChangeAllowed())
    {
        INT32 yearIndex = 0;
        INT32 monthIndex = 0;
        INT32 dayIndex = 0;
        wf::DateTime date = {};
        wf::DateTime currentDate = {};

        if (m_tpYearPicker)
        {
            IFC(m_tpYearPicker.AsOrNull<ISelector>()->get_SelectedIndex(&yearIndex));
        }

        if (m_tpMonthPicker)
        {
            IFC(m_tpMonthPicker.AsOrNull<ISelector>()->get_SelectedIndex(&monthIndex));
        }

        if (m_tpDayPicker)
        {
            IFC(m_tpDayPicker.AsOrNull<ISelector>()->get_SelectedIndex(&dayIndex));
        }

        IFC(GetDateFromIndices(yearIndex, monthIndex, dayIndex, &date));
        IFC(get_Date(&currentDate));
        // We are checking to see if new value is different from the current one. This is because even if they are same,
        // calling put_Date will break any Binding on Date (if there is any) that this DatePicker is target of.
        if (currentDate.UniversalTime != date.UniversalTime)
        {
            IFC(put_Date(date));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Reacts to the FlyoutButton being pressed. Invokes a form-factor specific flyout if present.
_Check_return_
HRESULT
DatePicker::OnFlyoutButtonClick(
   _In_ IInspectable* pSender,
   _In_ xaml::IRoutedEventArgs* pArgs)
{
    return ShowPickerFlyout();
}

_Check_return_ HRESULT DatePicker::ShowPickerFlyout()
{
    HRESULT hr = S_OK;

    if (!m_tpAsyncSelectionInfo)
    {
        ctl::ComPtr<IInspectable> spAsyncAsInspectable;
        ctl::ComPtr<wf::IAsyncOperation<wf::IReference<wf::DateTime>*>> spAsyncOperation;
        ctl::WeakRefPtr wpThis;

        IFC(ctl::AsWeak(this, &wpThis));

        auto callbackPtr = Microsoft::WRL::Callback<wf::IAsyncOperationCompletedHandler<wf::IReference<wf::DateTime>*>>(
            [wpThis] (wf::IAsyncOperation<wf::IReference<wf::DateTime>*>* getOperation, wf::AsyncStatus status) mutable
        {
            HRESULT hr = S_OK;
            ctl::ComPtr<DatePicker> spThis;
            spThis = wpThis.AsOrNull<xaml_controls::IDatePicker>().Cast<DatePicker>();

            if (spThis)
            {
                IFC(spThis->OnGetDatePickerSelectionAsyncCompleted(getOperation, status));
            }

       Cleanup:
            RRETURN(hr);
        });

        auto xamlControlsGetDatePickerSelectionPtr = reinterpret_cast<decltype(&XamlControlsGetDatePickerSelection)>(::GetProcAddress(GetPhoneModule(), "XamlControlsGetDatePickerSelection"));
        IFC(xamlControlsGetDatePickerSelectionPtr(ctl::as_iinspectable(this), ctl::as_iinspectable(m_tpFlyoutButton.Get()), spAsyncAsInspectable.GetAddressOf()));

        spAsyncOperation = spAsyncAsInspectable.AsOrNull<wf::IAsyncOperation<wf::IReference<wf::DateTime>*>>();
        IFCEXPECT(spAsyncOperation.Get());

        IFC(spAsyncOperation->put_Completed(callbackPtr.Get()));
        IFC(SetPtrValueWithQI(m_tpAsyncSelectionInfo, spAsyncOperation.Get()));
    }

Cleanup:
    RRETURN(hr);
}

// Callback passed to the GetDatePickerSelectionAsync method. Called when a form-factor specific
// flyout returns with a new DateTime value to update the DatePicker's DateTime.
_Check_return_ HRESULT DatePicker::OnGetDatePickerSelectionAsyncCompleted(
    wf::IAsyncOperation<wf::IReference<wf::DateTime>*>* getOperation,
    wf::AsyncStatus status)
{
    IFC_RETURN(CheckThread());

    m_tpAsyncSelectionInfo.Clear();

    if (status == wf::AsyncStatus::Completed)
    {
        ctl::ComPtr<wf::IReference<wf::DateTime>> selectedDate;
        IFC_RETURN(getOperation->GetResults(&selectedDate));

        // A null IReference object is returned when the user cancels out of the
        // DatePicker.
        if (selectedDate)
        {
            // We set SelectedDate instead of Date in order to ensure that the value
            // propagates to both SelectedDate and Date.
            // See the comment at the top of OnPropertyChanged2 for details.
            IFC_RETURN(put_SelectedDate(selectedDate.Get()));
        }
    }

    return S_OK;
}

// Interprets the selected indices of the selectors and creates and returns a DateTime corresponding to the date represented by these
// indices.
_Check_return_
HRESULT
DatePicker::GetDateFromIndices(
    _In_ INT32 yearIndex,
    _In_ INT32 monthIndex,
    _In_ INT32 dayIndex,
    _Out_ wf::DateTime* date) noexcept
{
    HRESULT hr = S_OK;
    wf::DateTime current = {};
    INT32 safeIndex = 0;
    INT32 firstIndex = 0;
    INT32 totalNumber = 0;
    INT32 period = 0;
    INT32 hour = 0;
    INT32 minute = 0;
    INT32 second = 0;
    INT32 nanosecond = 0;
    INT32 newYear = 0;
    INT32 newMonth = 0;
    INT32 previousYear = 0;
    INT32 previousMonth = 0;
    INT32 previousDay = 0;
    INT32 lastIndex = 0;

    IFC(get_Date(&current));
    current = ClampDate(current, m_startDate, m_endDate);
    IFC(m_tpCalendar->SetDateTime(current));
    // We want to preserve the time information. So we keep them around in order to prevent them overwritten by our sentinel time.
    IFC(m_tpCalendar->get_Period(&period));
    IFC(m_tpCalendar->get_Hour(&hour));
    IFC(m_tpCalendar->get_Minute(&minute));
    IFC(m_tpCalendar->get_Second(&second));
    IFC(m_tpCalendar->get_Nanosecond(&nanosecond));
    IFC(m_tpCalendar->get_Year(&previousYear));
    IFC(m_tpCalendar->get_Month(&previousMonth));
    IFC(m_tpCalendar->get_Day(&previousDay));

    IFC(m_tpCalendar->SetDateTime(m_startDate));
    IFC(m_tpCalendar->put_Period(period));
    IFC(m_tpCalendar->put_Hour(hour));
    IFC(m_tpCalendar->put_Minute(minute));
    IFC(m_tpCalendar->put_Second(second));
    IFC(m_tpCalendar->put_Nanosecond(nanosecond));

    IFC(m_tpCalendar->AddYears(yearIndex));
    IFC(m_tpCalendar->get_Year(&newYear));

    IFC(m_tpCalendar->get_FirstMonthInThisYear(&firstIndex));
    IFC(m_tpCalendar->get_NumberOfMonthsInThisYear(&totalNumber));
    IFC(m_tpCalendar->get_LastMonthInThisYear(&lastIndex));

    if (firstIndex > lastIndex)
    {
        if (monthIndex + firstIndex > totalNumber)
        {
            safeIndex = monthIndex + firstIndex - totalNumber;
        }
        else
        {
            safeIndex = monthIndex + firstIndex;
        }

        if (previousYear != newYear)
        {
            // Year has changed in some transitions in Thai Calendar, this will change the first month, and last month indices of the year.
            safeIndex = std::max(std::min(previousMonth, totalNumber), DATEPICKER_WRAP_AROUND_MONTHS_FIRST_INDEX);
        }
    }
    else
    {
        if (previousYear == newYear)
        {
            safeIndex = std::max(std::min(monthIndex + firstIndex, firstIndex + totalNumber - 1), firstIndex);
        }
        else
        {
            // Year has changed in some transitions in Thai Calendar, this will change the first month, and last month indices of the year.
            safeIndex = std::max(std::min(previousMonth, firstIndex + totalNumber - 1), firstIndex);
        }
    }

    IFC(m_tpCalendar->put_Month(safeIndex));
    IFC(m_tpCalendar->get_Month(&newMonth));

    IFC(m_tpCalendar->get_FirstDayInThisMonth(&firstIndex));
    IFC(m_tpCalendar->get_NumberOfDaysInThisMonth(&totalNumber));
    // We also need to coerce the day index into the safe range because a change in month or year may have changed the number of days
    // rendering our previous index invalid.
    safeIndex = std::max(std::min(dayIndex + firstIndex, firstIndex + totalNumber - 1), firstIndex);
    if(previousYear != newYear || previousMonth != newMonth)
    {
        safeIndex = std::max(std::min(previousDay, firstIndex + totalNumber - 1), firstIndex);
    }
    IFC(m_tpCalendar->put_Day(safeIndex));

    IFC(m_tpCalendar->GetDateTime(date));

Cleanup:
    RRETURN(hr);
}

// Gives the default values for our properties.
_Check_return_ HRESULT
DatePicker::GetDefaultValue2(
    _In_ const CDependencyProperty* pDP,
    _Out_ CValue* pValue)
{
    ctl::ComPtr<wg::ICalendar> spCalendar;

    IFCPTR_RETURN(pDP);
    IFCPTR_RETURN(pValue);

    switch (pDP->GetIndex())
    {
    case KnownPropertyIndex::DatePicker_MinYear:
        {
            wf::DateTime minYearDate = {};
            wf::DateTime minCalendarDate = {};
            wf::DateTime maxCalendarDate = {};

            if (m_tpGregorianCalendar.Get() == nullptr)
            {
                IFC_RETURN(CreateNewCalendar(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"GregorianCalendar")).Get(), &spCalendar));
                SetPtrValue(m_tpGregorianCalendar, spCalendar.Get());
            }

#ifndef _PREFAST_ // PREfast bug DevDiv:554051
            ASSERT(m_tpGregorianCalendar);
            ASSERT(m_tpCalendar);
#endif
            IFC_RETURN(m_tpCalendar->SetToMin());
            IFC_RETURN(m_tpCalendar->GetDateTime(&minCalendarDate));
            IFC_RETURN(m_tpCalendar->SetToMax());
            IFC_RETURN(m_tpCalendar->GetDateTime(&maxCalendarDate));
            //Default value is today's date minus 100 Gregorian years.
            IFC_RETURN(m_tpGregorianCalendar->SetToNow());
            IFC_RETURN(m_tpGregorianCalendar->AddYears(-DATEPICKER_MIN_MAX_YEAR_DEAFULT_OFFSET));
            IFC_RETURN(m_tpGregorianCalendar->GetDateTime(&minYearDate));

            pValue->SetDateTime(ClampDate(minYearDate, minCalendarDate, maxCalendarDate));
            break;
        }
    case KnownPropertyIndex::DatePicker_MaxYear:
        {
            wf::DateTime maxYearDate = {};
            wf::DateTime minCalendarDate = {};
            wf::DateTime maxCalendarDate = {};

            if (m_tpGregorianCalendar.Get() == nullptr)
            {
                IFC_RETURN(CreateNewCalendar(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"GregorianCalendar")).Get(), &spCalendar));
                SetPtrValue(m_tpGregorianCalendar, spCalendar.Get());
            }
#ifndef _PREFAST_ // PREfast bug DevDiv:554051
            ASSERT(m_tpGregorianCalendar);
            ASSERT(m_tpCalendar);
#endif
            IFC_RETURN(m_tpCalendar->SetToMin());
            IFC_RETURN(m_tpCalendar->GetDateTime(&minCalendarDate));
            IFC_RETURN(m_tpCalendar->SetToMax());
            IFC_RETURN(m_tpCalendar->GetDateTime(&maxCalendarDate));
            //Default value is today's date plus 100 Gregorian years.
            IFC_RETURN(m_tpGregorianCalendar->SetToNow());
            IFC_RETURN(m_tpGregorianCalendar->AddYears(DATEPICKER_MIN_MAX_YEAR_DEAFULT_OFFSET));
            IFC_RETURN(m_tpGregorianCalendar->GetDateTime(&maxYearDate));

            pValue->SetDateTime(ClampDate(maxYearDate, minCalendarDate, maxCalendarDate));
            break;
        }
    case KnownPropertyIndex::DatePicker_Date:
        {
            pValue->SetDateTime(m_defaultDate);
            break;
        }
    default:
        IFC_RETURN(DatePickerGenerated::GetDefaultValue2(pDP, pValue));
        break;
    }

    return S_OK;
}

// Reacts to the changes in string typed properties. Reverts the property value to the last valid value,
// if property change causes an exception.
_Check_return_
HRESULT
DatePicker::OnStringTypePropertyChanged(
    _In_ KnownPropertyIndex nPropertyIndex,
    _In_ IInspectable* pOldValue,
    _In_ IInspectable* pNewValue)
{
    HRESULT hr = S_OK;

    IFC(RefreshSetup());

Cleanup:
    if (FAILED(hr))
    {
        wrl_wrappers::HString strOldValue;

        VERIFYHR(ctl::do_get_value(*strOldValue.GetAddressOf(), pOldValue));
        switch (nPropertyIndex)
        {
        case KnownPropertyIndex::DatePicker_CalendarIdentifier:
            VERIFYHR(put_CalendarIdentifier(strOldValue));
            break;
        case KnownPropertyIndex::DatePicker_DayFormat:
            VERIFYHR(put_DayFormat(strOldValue));
            break;
        case KnownPropertyIndex::DatePicker_MonthFormat:
            VERIFYHR(put_MonthFormat(strOldValue));
            break;
        case KnownPropertyIndex::DatePicker_YearFormat:
            VERIFYHR(put_YearFormat(strOldValue));
            break;
        }
    }
    RRETURN(hr);
}

// Reacts to changes in Date property. Day may have changed programmatically or end user may have changed the
// selection of one of our selectors causing a change in Date.
_Check_return_
HRESULT
DatePicker::OnDateChanged(
    _In_ wf::DateTime oldValue,
    _In_ wf::DateTime newValue)
{
    if (m_hasValidYearRange)
    {
        wf::DateTime clampedNewDate = {};
        wf::DateTime clampedOldDate = {};

        // The DateTime value set may be out of our acceptable range.
        clampedNewDate =
            newValue.UniversalTime != GetNullDateSentinelValue() ?
                ClampDate(newValue, m_startDate, m_endDate) :
                newValue;
        clampedOldDate =
            oldValue.UniversalTime != GetNullDateSentinelValue() ?
                ClampDate(oldValue, m_startDate, m_endDate) :
                oldValue;

        // We are checking to see if new value is different from the current one. This is because even if they are same,
        // calling put_Date will break any Binding on Date (if there is any) that this DatePicker is target of.
        if (clampedNewDate.UniversalTime != newValue.UniversalTime)
        {
            // We need to coerce the date into the acceptable range. This will trigger another OnDateChanged which
            // will take care of executing the logic needed.
            IFC_RETURN(put_Date(clampedNewDate));
            return S_OK;
        }

        BOOLEAN refreshMonth = FALSE;
        BOOLEAN refreshDay = FALSE;

        // Some calendars don't start at day0 (1/1/1).
        // e.g. the first day of Japanese calendar is day681907 (1/1/1868).
        // However, we use day0 as a flag for "unset", so by default clampedOldDate is day0.
        // This will cause exceptions in SetDateTime below since it's not a valid date in Japanese calendar scope.
        // If either clampedOldDate or clampedNewDate is day0, which means we are either setting a new date to an uninitialize DatePicker or clearing the existing selected date,
        // skip the checks in "else" block and just refresh the calendar.
        if (clampedNewDate.UniversalTime == clampedOldDate.UniversalTime
            || clampedOldDate.UniversalTime == GetNullDateSentinelValue()
            || clampedNewDate.UniversalTime == GetNullDateSentinelValue())
        {
            // It looks like we clamped an invalid date into an acceptable one, we need to refresh the sources.
            refreshMonth = TRUE;
            refreshDay = TRUE;
        }
        else
        {
            INT32 newYear = 0;
            INT32 oldYear = 0;
            INT32 newMonth = 0;
            INT32 oldMonth = 0;

            IFC_RETURN(m_tpCalendar->SetDateTime(clampedOldDate));
            IFC_RETURN(m_tpCalendar->get_Year(&oldYear));
            IFC_RETURN(m_tpCalendar->get_Month(&oldMonth));

            IFC_RETURN(m_tpCalendar->SetDateTime(clampedNewDate));
            IFC_RETURN(m_tpCalendar->get_Year(&newYear));
            IFC_RETURN(m_tpCalendar->get_Month(&newMonth));

            if (oldYear != newYear)
            {
                refreshMonth = TRUE;
                refreshDay = TRUE;
            }
            else if (oldMonth != newMonth)
            {
                refreshDay = TRUE;
            }
        }

        // Change in year will invalidate month and days.
        // Change in month will invalidate days.
        // No need to refresh any itemsources only if the day changes.
        if (refreshDay || refreshMonth)
        {
            IFC_RETURN(RefreshSources(refreshDay, refreshMonth, FALSE));
        }
        else
        {
            // Just set the indices to correct values. If the date has been changed by the end user
            // using the comboboxes put_SelectedIndex will no-op since they are already at the correct
            // index.
            INT32 yearIndex = 0;
            INT32 monthIndex = 0;
            INT32 dayIndex = 0;

            IFC_RETURN(GetIndices(yearIndex, monthIndex, dayIndex));
            if (m_tpYearPicker)
            {
                IFC_RETURN(m_tpYearPicker->put_SelectedIndex(yearIndex));
            }
            if (m_tpMonthPicker)
            {
                IFC_RETURN(m_tpMonthPicker->put_SelectedIndex(monthIndex));
            }
            if (m_tpDayPicker)
            {
                IFC_RETURN(m_tpDayPicker->put_SelectedIndex(dayIndex));
            }
        }

        if (m_tpFlyoutButton.Get())
        {
            IFC_RETURN(UpdateFlyoutButtonContent());
        }
    }

    ctl::ComPtr<DatePickerValueChangedEventArgs> valueChangedEventArgs;
    DateChangedEventSourceType* dateChangedEventSource = nullptr;

    // Create and populate the value changed event args
    IFC_RETURN(ctl::make<DatePickerValueChangedEventArgs>(&valueChangedEventArgs));
    IFC_RETURN(valueChangedEventArgs->put_OldDate(oldValue));
    IFC_RETURN(valueChangedEventArgs->put_NewDate(newValue));

    // Raise event
    IFC_RETURN(DatePickerGenerated::GetDateChangedEventSourceNoRef(&dateChangedEventSource));
    IFC_RETURN(dateChangedEventSource->Raise(ctl::as_iinspectable(this), valueChangedEventArgs.Get()));

    ctl::ComPtr<DatePickerSelectedValueChangedEventArgs> selectedValueChangedEventArgs;
    SelectedDateChangedEventSourceType* selectedDateChangedEventSource = nullptr;

    // Create and populate the selected value changed event args
    IFC_RETURN(ctl::make<DatePickerSelectedValueChangedEventArgs>(&selectedValueChangedEventArgs));

    if (oldValue.UniversalTime != GetNullDateSentinelValue())
    {
        ctl::ComPtr<IInspectable> boxedDateAsI;
        ctl::ComPtr<wf::IReference<wf::DateTime>> boxedDate;
        IFC_RETURN(PropertyValue::CreateFromDateTime(oldValue, &boxedDateAsI));
        IFC_RETURN(boxedDateAsI.As(&boxedDate));

        IFC_RETURN(selectedValueChangedEventArgs->put_OldDate(boxedDate.Get()));
    }
    else
    {
        IFC_RETURN(selectedValueChangedEventArgs->put_OldDate(nullptr));
    }

    if (newValue.UniversalTime != GetNullDateSentinelValue())
    {
        ctl::ComPtr<IInspectable> boxedDateAsI;
        ctl::ComPtr<wf::IReference<wf::DateTime>> boxedDate;
        IFC_RETURN(PropertyValue::CreateFromDateTime(newValue, &boxedDateAsI));
        IFC_RETURN(boxedDateAsI.As(&boxedDate));

        IFC_RETURN(selectedValueChangedEventArgs->put_NewDate(boxedDate.Get()));
    }
    else
    {
        IFC_RETURN(selectedValueChangedEventArgs->put_NewDate(nullptr));
    }

    // Raise event
    IFC_RETURN(DatePickerGenerated::GetSelectedDateChangedEventSourceNoRef(&selectedDateChangedEventSource));
    IFC_RETURN(selectedDateChangedEventSource->Raise(this, selectedValueChangedEventArgs.Get()));

    return S_OK;
}

// Updates the visibility of the Header ContentPresenter
_Check_return_
HRESULT
DatePicker::UpdateHeaderPresenterVisibility()
{
    ctl::ComPtr<xaml::IDataTemplate> spHeaderTemplate;
    ctl::ComPtr<IInspectable> spHeader;

    IFC_RETURN(get_HeaderTemplate(&spHeaderTemplate));
    IFC_RETURN(get_Header(&spHeader));

    IFC_RETURN(ConditionallyGetTemplatePartAndUpdateVisibility(
        XSTRING_PTR_EPHEMERAL(L"HeaderContentPresenter"),
        (spHeader || spHeaderTemplate),
        m_tpHeaderPresenter));

    return S_OK;
}


// Handle the custom property changed event and call the
// OnPropertyChanged2 methods.
_Check_return_
HRESULT
DatePicker::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(DatePickerGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::DatePicker_Date:
        if (!m_isPropagatingDate)
        {
            wf::DateTime date;
            IFC_RETURN(get_Date(&date));

            // When SelectedDate is set to null, we set Date to a sentinel value that also represents null,
            // and vice-versa.
            if (date.UniversalTime != GetNullDateSentinelValue())
            {
                ctl::ComPtr<IInspectable> boxedDateAsI;
                ctl::ComPtr<wf::IReference<wf::DateTime>> boxedDate;
                IFC_RETURN(PropertyValue::CreateFromDateTime(date, &boxedDateAsI));
                IFC_RETURN(boxedDateAsI.As(&boxedDate));

                {
                    m_isPropagatingDate = true;
                    auto scopeGuard = wil::scope_exit([this]() { m_isPropagatingDate = false; });
                    IFC_RETURN(put_SelectedDate(boxedDate.Get()));
                }
            }
            else
            {
                m_isPropagatingDate = true;
                auto scopeGuard = wil::scope_exit([this]() { m_isPropagatingDate = false; });
                IFC_RETURN(put_SelectedDate(nullptr));
            }
        }
        break;

    case KnownPropertyIndex::DatePicker_SelectedDate:
        if (!m_isPropagatingDate)
        {
            ctl::ComPtr<wf::IReference<wf::DateTime>> selectedDate;
            IFC_RETURN(get_SelectedDate(&selectedDate));

            if (selectedDate)
            {
                wf::DateTime date;
                IFC_RETURN(selectedDate->get_Value(&date));

                {
                    m_isPropagatingDate = true;
                    auto scopeGuard = wil::scope_exit([this]() { m_isPropagatingDate = false; });
                    IFC_RETURN(put_Date(date));
                }
            }
            else
            {
                m_isPropagatingDate = true;
                auto scopeGuard = wil::scope_exit([this]() { m_isPropagatingDate = false; });
                IFC_RETURN(put_Date(GetNullDateSentinel()));
            }
        }
        break;
    }

    // Except for the interaction between Date and SelectedDate, every other property change can wait
    // until we're done initializing in OnApplyTemplate, since they all only affect visual state.
    if (!m_isInitializing)
    {
        switch(args.m_pDP->GetIndex())
        {
        case KnownPropertyIndex::DatePicker_Date:
            {
            wf::DateTime oldValue, newValue;
            IFC_RETURN(CValueBoxer::UnboxValue(args.m_pOldValue, &oldValue));
            IFC_RETURN(CValueBoxer::UnboxValue(args.m_pNewValue, &newValue));

            IFC_RETURN(OnDateChanged(oldValue, newValue));
            }
            break;

        case KnownPropertyIndex::DatePicker_SelectedDate:
            if (m_tpFlyoutButton.Get())
            {
                IFC_RETURN(UpdateFlyoutButtonContent());
            }

            IFC_RETURN(UpdateVisualState());
            break;

        case KnownPropertyIndex::DatePicker_CalendarIdentifier:
        case KnownPropertyIndex::DatePicker_DayFormat:
        case KnownPropertyIndex::DatePicker_MonthFormat:
        case KnownPropertyIndex::DatePicker_YearFormat:
        case KnownPropertyIndex::DatePicker_MinYear:
        case KnownPropertyIndex::DatePicker_MaxYear:
            {
            ctl::ComPtr<IInspectable> spOldValue, spNewValue;
            IFC_RETURN(CValueBoxer::UnboxObjectValue(args.m_pOldValue, args.m_pDP->GetPropertyType(), &spOldValue));
            IFC_RETURN(CValueBoxer::UnboxObjectValue(args.m_pNewValue, args.m_pDP->GetPropertyType(), &spNewValue));
            IFC_RETURN(OnStringTypePropertyChanged(args.m_pDP->GetIndex(), spOldValue.Get(), spNewValue.Get()));
            }
            break;

        case KnownPropertyIndex::DatePicker_DayVisible:
        case KnownPropertyIndex::DatePicker_MonthVisible:
        case KnownPropertyIndex::DatePicker_YearVisible:
            IFC_RETURN(UpdateOrderAndLayout());
            break;

        case KnownPropertyIndex::DatePicker_Header:
        case KnownPropertyIndex::DatePicker_HeaderTemplate:
            IFC_RETURN(UpdateHeaderPresenterVisibility());
            break;

#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
        case KnownPropertyIndex::DatePicker_HeaderPlacement:
            IFC_RETURN(UpdateVisualState());
            break;
#endif
        }
    }

    return S_OK;
}

// Returns the cached DateTimeFormatter for the given Calendar - Format pair for generating the strings
// representing the years in our date range. If there isn't a cached DateTimeFormatter instance,
// creates one and caches it to be returned for the following calls with the same pair.
_Check_return_
HRESULT
DatePicker::GetYearFormatter(
    _In_ HSTRING strFormat,
    _In_ HSTRING strCalendarIdentifier,
    _Outptr_ wg::DateTimeFormatting::IDateTimeFormatter** ppDateTimeFormatter)
{
    HRESULT hr = S_OK;

    // We can only use the cached formatter if there is a cached formatter, cached formatter's format is the same as the new one's
    // and cached formatter's calendar identifier is the same as the new one's.
    if (!(m_tpYearFormatter.Get()
        && strFormat == m_strYearFormat
        && strCalendarIdentifier == m_strYearCalendarIdentifier))
    {
        // We either do not have a cached formatter or it is stale. We need a create a new one and cache it along
        // with its identifying info.
        ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatter;

        m_tpYearFormatter.Clear();
        IFC(CreateNewFormatter(strFormat, strCalendarIdentifier, &spFormatter));
        SetPtrValue(m_tpYearFormatter, spFormatter);
        IFC(::WindowsDuplicateString(strFormat, m_strYearFormat.ReleaseAndGetAddressOf()));
        IFC(::WindowsDuplicateString(strCalendarIdentifier, m_strYearCalendarIdentifier.ReleaseAndGetAddressOf()));
    }

    IFC(m_tpYearFormatter.CopyTo(ppDateTimeFormatter));

Cleanup:
    RRETURN(hr);
}

// Returns the cached DateTimeFormatter for the given Calendar - Format pair for generating the strings
// representing the months in our date range. If there isn't a cached DateTimeFormatter instance,
// creates one and caches it to be returned for the following calls with the same pair.
_Check_return_
HRESULT
DatePicker::GetMonthFormatter(
    _In_ HSTRING strFormat,
    _In_ HSTRING strCalendarIdentifier,
    _Outptr_ wg::DateTimeFormatting::IDateTimeFormatter** ppDateTimeFormatter)
{
    HRESULT hr = S_OK;

    // We can only use the cached formatter if there is a cached formatter, cached formatter's format is the same as the new one's
    // and cached formatter's calendar identifier is the same as the new one's.
    if (!(m_tpMonthFormatter.Get()
        && strFormat == m_strMonthFormat
        && strCalendarIdentifier == m_strMonthCalendarIdentifier))
    {
        // We either do not have a cached formatter or it is stale. We need a create a new one and cache it along
        // with its identifying info.
        ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatter;

        m_tpMonthFormatter.Clear();
        IFC(CreateNewFormatter(strFormat, strCalendarIdentifier, &spFormatter));
        SetPtrValue(m_tpMonthFormatter, spFormatter.Get());
        IFC(::WindowsDuplicateString(strFormat, m_strYearFormat.ReleaseAndGetAddressOf()));
        IFC(::WindowsDuplicateString(strCalendarIdentifier, m_strMonthCalendarIdentifier.ReleaseAndGetAddressOf()));
    }

    IFC(m_tpMonthFormatter.CopyTo(ppDateTimeFormatter));

Cleanup:
    RRETURN(hr);
}

// Returns the cached DateTimeFormatter for the given Calendar - Format pair for generating the strings
// representing the days in our date range. If there isn't a cached DateTimeFormatter instance,
// creates one and caches it to be returned for the following calls with the same pair.
_Check_return_
HRESULT
DatePicker::GetDayFormatter(
    _In_ HSTRING strFormat,
    _In_ HSTRING strCalendarIdentifier,
    _Outptr_ wg::DateTimeFormatting::IDateTimeFormatter** ppDateTimeFormatter)
{
    HRESULT hr = S_OK;

    // We can only use the cached formatter if there is a cached formatter, cached formatter's format is the same as the new one's
    // and cached formatter's calendar identifier is the same as the new one's.
    if (!(m_tpDayFormatter.Get()
        && strFormat == m_strDayFormat
        && strCalendarIdentifier == m_strDayCalendarIdentifier))
    {
        // We either do not have a cached formatter or it is stale. We need a create a new one and cache it along
        // with its identifying info.
        ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatter;

        m_tpDayFormatter.Clear();
        IFC(CreateNewFormatter(strFormat, strCalendarIdentifier, &spFormatter));
        SetPtrValue(m_tpDayFormatter, spFormatter.Get());
        IFC(::WindowsDuplicateString(strFormat, m_strYearFormat.ReleaseAndGetAddressOf()));
        IFC(::WindowsDuplicateString(strCalendarIdentifier, m_strMonthCalendarIdentifier.ReleaseAndGetAddressOf()));
    }

    IFC(m_tpDayFormatter.CopyTo(ppDateTimeFormatter));

Cleanup:
    RRETURN(hr);
}

// Creates a new DateTimeFormatter with the given parameters.
_Check_return_
HRESULT
DatePicker::CreateNewFormatter(
    _In_ HSTRING strFormat,
    _In_ HSTRING strCalendarIdentifier,
    _Outptr_ wg::DateTimeFormatting::IDateTimeFormatter** ppDateTimeFormatter)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatterFactory> spFormatterFactory;
    ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatter;
    ctl::ComPtr<wfc::__FIVectorView_1_HSTRING_t> spLanguages;
    wrl_wrappers::HString strGeographicRegion;
    wrl_wrappers::HString strClock;

    *ppDateTimeFormatter = nullptr;

    IFC(ctl::GetActivationFactory(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"Windows.Globalization.DateTimeFormatting.DateTimeFormatter")).Get(), &spFormatterFactory));
    IFCPTR(spFormatterFactory.Get());

    IFC(spFormatterFactory->CreateDateTimeFormatter(strFormat, spFormatter.ReleaseAndGetAddressOf()));

    IFC(spFormatter->get_GeographicRegion(strGeographicRegion.GetAddressOf()));
    IFC(spFormatter->get_Languages(spLanguages.ReleaseAndGetAddressOf()));
    IFC(spFormatter->get_Clock(strClock.GetAddressOf()));

    IFC(spFormatterFactory->CreateDateTimeFormatterContext(
            strFormat,/* Format string */
            spLanguages.AsOrNull<wfc::__FIIterable_1_HSTRING_t>().Get(), /* Languages*/
            strGeographicRegion.Get(), /* Geographic region */
            strCalendarIdentifier, /* Calendar */
            strClock.Get(), /* Clock */
            spFormatter.ReleaseAndGetAddressOf()));

    *ppDateTimeFormatter = spFormatter.Detach();

Cleanup:
    RRETURN(hr);
}

// Creates a new wg::Calendar, taking into account the Calendar Identifier
// represented by our public "Calendar" property.
_Check_return_
HRESULT
DatePicker::CreateNewCalendar(
    _In_ HSTRING strCalendarIdentifier,
    _Outptr_ wg::ICalendar** ppCalendar)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wg::ICalendarFactory> spCalendarFactory;
    ctl::ComPtr<wg::ICalendar> spCalendar;
    ctl::ComPtr<wfc::__FIVectorView_1_HSTRING_t> spLanguages;
    wrl_wrappers::HString strClock;

    *ppCalendar = nullptr;

    IFC(wf::ActivateInstance(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"Windows.Globalization.Calendar")).Get(), spCalendar.ReleaseAndGetAddressOf()));
    IFC(spCalendar->get_Languages(spLanguages.ReleaseAndGetAddressOf()));
    IFC(spCalendar->GetClock(strClock.GetAddressOf()));

    //Create the calendar
    IFC(ctl::GetActivationFactory(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"Windows.Globalization.Calendar")).Get(), &spCalendarFactory));
    IFC(spCalendarFactory->CreateCalendar(
            spLanguages.AsOrNull<wfc::__FIIterable_1_HSTRING_t>().Get(), /* Languages*/
            strCalendarIdentifier, /* Calendar */
            strClock.Get(), /* Clock */
            spCalendar.ReleaseAndGetAddressOf()));

    *ppCalendar = spCalendar.Detach();

Cleanup:
    RRETURN(hr);
}

// Returns the cached DateTimeFormatter for the given Calendar for generating the strings
// representing the current Date for display on a FlyoutButton. If there isn't a cached
// DateTimeFormatter instance, creates one and caches it to be returned for the following
// calls with the same pair.
_Check_return_
HRESULT
DatePicker::GetDateFormatter(
    _In_ HSTRING strCalendarIdentifier,
   _Outptr_ wg::DateTimeFormatting::IDateTimeFormatter** ppDateFormatter)
{
    HRESULT hr = S_OK;

    // We can only use the cached formatter if there is a cached formatter, cached formatter's format is the same as the new one's
    // and cached formatter's calendar identifier is the same as the new one's.
    if (!(m_tpDateFormatter.Get()
        && strCalendarIdentifier == m_strDateCalendarIdentifier))
    {
        // We either do not have a cached formatter or it is stale. We need a create a new one and cache it along
        // with its identifying info.
        ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatter;

        m_tpDateFormatter.Clear();
        IFC(CreateNewFormatter(
            wrl_wrappers::HStringReference(STR_LEN_PAIR(L"shortdate")).Get(),
            strCalendarIdentifier,
            &spFormatter));
        SetPtrValue(m_tpDateFormatter, spFormatter.Get());
        IFC(::WindowsDuplicateString(strCalendarIdentifier, m_strMonthCalendarIdentifier.ReleaseAndGetAddressOf()));
    }

    IFC(m_tpDateFormatter.CopyTo(ppDateFormatter));

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
DatePicker::UpdateFlyoutButtonContent()
{
    wrl_wrappers::HString strFormattedDate;
    ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spDateFormatter;
    wrl_wrappers::HString strCalendarIdentifier;
    wf::DateTime date = {};
    ctl::ComPtr<IInspectable> spInspectable;
    ctl::ComPtr<wf::IReference<HSTRING>> spReference;
    ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spYearFormatter;
    wrl_wrappers::HString strYearFormat;
    wrl_wrappers::HString strYear;
    ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spMonthFormatter;
    wrl_wrappers::HString strMonthFormat;
    wrl_wrappers::HString strMonth;
    ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spDayFormatter;
    wrl_wrappers::HString strDayFormat;
    wrl_wrappers::HString strDay;

    // Get the calendar identifier string from the DP and use it to retrieve the cached
    // DateFormatter.
    IFC_RETURN(get_CalendarIdentifier(strCalendarIdentifier.GetAddressOf()));
    IFC_RETURN(GetDateFormatter(strCalendarIdentifier.Get(), spDateFormatter.GetAddressOf()));

    // Get the date to display.
    ctl::ComPtr<wf::IReference<wf::DateTime>> selectedDate;
    IFC_RETURN(get_SelectedDate(&selectedDate));

    if (selectedDate)
    {
        IFC_RETURN(selectedDate->get_Value(&date));
    }

    date = ClampDate(date, m_startDate, m_endDate);

    // For Blue apps (or a DatePicker template based on what was shipped in Blue), we only have the FlyoutButton
    // Set the Content of the FlyoutButton to the formatted date.
    if (m_tpFlyoutButton.Get() && !m_tpYearTextBlock && !m_tpMonthTextBlock && !m_tpDayTextBlock)
    {
        // Format the Date into a string and set it as the content of the FlyoutButton
        IFC_RETURN(spDateFormatter->Format(date, strFormattedDate.GetAddressOf()));
        IFC_RETURN(PropertyValue::CreateFromString(strFormattedDate.Get(), &spInspectable));
        IFC_RETURN(spInspectable.As(&spReference));

        IFC_RETURN(m_tpFlyoutButton.Cast<Button>()->put_Content(spReference.Get()));
    }
    // For the Threshold template we set the Day, Month and Year strings on separate TextBlocks:
    if (m_tpYearTextBlock.Get())
    {
        if (selectedDate)
        {
            IFC_RETURN(get_YearFormat(strYearFormat.GetAddressOf()));
            IFC_RETURN(GetYearFormatter(strYearFormat.Get(), strCalendarIdentifier.Get(), &spYearFormatter));
            IFC_RETURN(spYearFormatter->Format(date, strYear.ReleaseAndGetAddressOf()));

            IFC_RETURN(m_tpYearTextBlock->put_Text(strYear));
        }
        else
        {
            wrl_wrappers::HString placeholderText;
            IFC_RETURN(DXamlCore::GetCurrent()->GetLocalizedResourceString(
                TEXT_DATEPICKER_YEAR_PLACEHOLDER,
                placeholderText.ReleaseAndGetAddressOf()));

            IFC_RETURN(m_tpYearTextBlock->put_Text(placeholderText));
        }
    }
    if (m_tpMonthTextBlock.Get())
    {
        if (selectedDate)
        {
            IFC_RETURN(get_MonthFormat(strMonthFormat.GetAddressOf()));
            IFC_RETURN(GetMonthFormatter(strMonthFormat.Get(), strCalendarIdentifier.Get(), &spMonthFormatter));
            IFC_RETURN(spMonthFormatter->Format(date, strMonth.ReleaseAndGetAddressOf()));

            IFC_RETURN(m_tpMonthTextBlock->put_Text(strMonth));
        }
        else
        {
            wrl_wrappers::HString placeholderText;
            IFC_RETURN(DXamlCore::GetCurrent()->GetLocalizedResourceString(
                TEXT_DATEPICKER_MONTH_PLACEHOLDER,
                placeholderText.ReleaseAndGetAddressOf()));

            IFC_RETURN(m_tpMonthTextBlock->put_Text(placeholderText));
        }
    }
    if (m_tpDayTextBlock.Get())
    {
        if (selectedDate)
        {
            IFC_RETURN(get_DayFormat(strDayFormat.GetAddressOf()));
            IFC_RETURN(GetDayFormatter(strDayFormat.Get(), strCalendarIdentifier.Get(), &spDayFormatter));
            IFC_RETURN(spDayFormatter->Format(date, strDay.ReleaseAndGetAddressOf()));

            IFC_RETURN(m_tpDayTextBlock->put_Text(strDay));
        }
        else
        {
            wrl_wrappers::HString placeholderText;
            IFC_RETURN(DXamlCore::GetCurrent()->GetLocalizedResourceString(
                TEXT_DATEPICKER_DAY_PLACEHOLDER,
                placeholderText.ReleaseAndGetAddressOf()));

            IFC_RETURN(m_tpDayTextBlock->put_Text(placeholderText));
        }
    }
    IFC_RETURN(RefreshFlyoutButtonAutomationName());

    return S_OK;
}

// Given two calendars, finds the difference of years between them. Note that we are counting on the two
// calendars will have the same system.
_Check_return_
HRESULT
DatePicker::GetYearDifference(
    _In_ wg::ICalendar* pStartCalendar,
    _In_ wg::ICalendar* pEndCalendar,
    _Inout_ INT32& difference)
{
    HRESULT hr = S_OK;
    INT32 startEra = 0;
    INT32 endEra = 0;
    INT32 startYear = 0;
    INT32 endYear = 0;
    wrl_wrappers::HString strStartCalendarSystem;
    wrl_wrappers::HString strEndCalendarSystem;

    IFC(pStartCalendar->GetCalendarSystem(strStartCalendarSystem.GetAddressOf()));
    IFC(pEndCalendar->GetCalendarSystem(strEndCalendarSystem.GetAddressOf()));
    ASSERT(strStartCalendarSystem == strEndCalendarSystem, L"Calendar systems do not match.");

    difference = 0;

    // Get the eras and years of the calendars.
    IFC(pStartCalendar->get_Era(&startEra));
    IFC(pEndCalendar->get_Era(&endEra));

    IFC(pStartCalendar->get_Year(&startYear));
    IFC(pEndCalendar->get_Year(&endYear));

    while (startEra != endEra || startYear != endYear)
    {
        // Add years to start calendar until their eras and years both match.
        IFC(pStartCalendar->AddYears(1));
        difference++;
        IFC(pStartCalendar->get_Era(&startEra));
        IFC(pStartCalendar->get_Year(&startYear));
    }

Cleanup:
    RRETURN(hr);
}


// Clamps the given date within the range defined by the min and max dates. Note that it is caller's responsibility
// to feed appropriate min/max values that defines a valid date range.
wf::DateTime
DatePicker::ClampDate(
    _In_ wf::DateTime date,
    _In_ wf::DateTime minDate,
    _In_ wf::DateTime maxDate)
{
    if (date.UniversalTime < minDate.UniversalTime)
    {
        return minDate;
    }
    else if (date.UniversalTime > maxDate.UniversalTime)
    {
        return maxDate;
    }
    return date;
}

// The order of date fields vary depending on geographic region, calendar type etc. This function determines the M/D/Y order using
// globalization APIs. It also determines whether the fields should be laid RTL.
_Check_return_
HRESULT
DatePicker::GetOrder(
    _Out_ INT32* yearOrder,
    _Out_ INT32* monthOrder,
    _Out_ INT32* dayOrder,
    _Out_ BOOLEAN* isRTL)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatter;
    ctl::ComPtr<__FIVectorView_1_HSTRING> spPatterns;
    wrl_wrappers::HString strDate;
    wrl_wrappers::HString strCalendarIdentifier;

    IFC(get_CalendarIdentifier(strCalendarIdentifier.GetAddressOf()));
    IFC(CreateNewFormatter(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"day month.full year")).Get(), strCalendarIdentifier.Get(), &spFormatter));
    IFC(spFormatter->get_Patterns(&spPatterns));
    IFC(spPatterns->GetAt(0, strDate.GetAddressOf()));

    if (strDate.Get())
    {
        LPCWSTR szDate;
        UINT32 length = 0;
        LPCWSTR dayOccurence = nullptr;
        LPCWSTR monthOccurence = nullptr;
        LPCWSTR yearOccurence = nullptr;

        szDate = strDate.GetRawBuffer(&length);

        //The calendar is right-to-left if the first character of the pattern string is the rtl character
        *isRTL = szDate[0] == DATEPICKER_RTL_CHARACTER_CODE;

        // We do string search to determine the order of the fields.
        dayOccurence = xstrstr(szDate, L"{day");
        monthOccurence = xstrstr(szDate, L"{month");
        yearOccurence = xstrstr(szDate, L"{year");

        if (dayOccurence < monthOccurence)
        {
            if (dayOccurence < yearOccurence)
            {
                *dayOrder = 0;
                if (monthOccurence < yearOccurence)
                {
                    *monthOrder = 1;
                    *yearOrder = 2;
                }
                else
                {
                    *monthOrder = 2;
                    *yearOrder = 1;
                }
            }
            else
            {
                *dayOrder = 1;
                *monthOrder = 2;
                *yearOrder = 0;
            }
        }
        else
        {
            if (dayOccurence < yearOccurence)
            {
                *dayOrder = 1;
                *monthOrder = 0;
                *yearOrder = 2;
            }
            else
            {
                *dayOrder = 2;
                if (monthOccurence < yearOccurence)
                {
                    *monthOrder = 0;
                    *yearOrder = 1;
                }
                else
                {

                    *monthOrder = 1;
                    *yearOrder = 0;
                }
            }
        }

    }

Cleanup:
    RRETURN(hr);
}

// Updates the order of selectors in our layout. Also takes care of hiding/showing the comboboxes and related spacing depending our
// public properties set by the user.
_Check_return_
HRESULT
DatePicker::UpdateOrderAndLayout() noexcept
{
    HRESULT hr = S_OK;
    INT32 yearOrder = 0;
    INT32 monthOrder = 0;
    INT32 dayOrder = 0;
    BOOLEAN isRTL = FALSE;
    BOOLEAN dayVisible = FALSE;
    BOOLEAN monthVisible = FALSE;
    BOOLEAN yearVisible = FALSE;
    BOOLEAN firstHostPopulated = FALSE;
    BOOLEAN secondHostPopulated = FALSE;
    BOOLEAN thirdHostPopulated = FALSE;
    ctl::ComPtr<IInspectable> spInspectable;
    ctl::ComPtr<wfc::IVector<xaml_controls::ColumnDefinition*>> spColumns;
    BOOLEAN columnIsFound = FALSE;
    UINT32 columnIndex = 0;
    ctl::ComPtr<xaml_controls::IColumnDefinition> firstTextBlockColumn = nullptr;
    ctl::ComPtr<xaml_controls::IColumnDefinition> secondTextBlockColumn = nullptr;
    ctl::ComPtr<xaml_controls::IColumnDefinition> thirdTextBlockColumn = nullptr;

    IFC(GetOrder(&yearOrder, &monthOrder, &dayOrder, &isRTL));

    // Some of the Calendars are RTL (Hebrew, Um Al Qura) we need to change the flow direction of DatePicker to accommodate these
    // calendars.
    if (m_tpLayoutRoot.Get())
    {
        IFC(m_tpLayoutRoot.Cast<Border>()->put_FlowDirection(isRTL ?
            xaml::FlowDirection_RightToLeft : xaml::FlowDirection_LeftToRight));
    }

    // Clear the children of hosts first, so we never risk putting one picker in two hosts and failing.
    if (m_tpFirstPickerHost.Get())
    {
        IFC(m_tpFirstPickerHost->put_Child(nullptr));
    }
    if (m_tpSecondPickerHost.Get())
    {
        IFC(m_tpSecondPickerHost->put_Child(nullptr));
    }
    if (m_tpThirdPickerHost.Get())
    {
        IFC(m_tpThirdPickerHost->put_Child(nullptr));
    }

    // Clear the columns of the grid first. We will re-add the columns that we need further down.
    if (m_tpFlyoutButtonContentGrid)
    {
        IFC(m_tpFlyoutButtonContentGrid->get_ColumnDefinitions(&spColumns));
        spColumns->Clear();
    }

    IFC(get_DayVisible(&dayVisible));
    IFC(get_MonthVisible(&monthVisible));
    IFC(get_YearVisible(&yearVisible));


    // For Blue apps:
    // Assign the selectors to the hosts, if the selector is not shown, we will not put the selector inside the related hosts. Note that we
    // could have just collapsed selector or its host to accomplish hiding, however, we decided not to put the hidden fields to already
    // crowded visual tree.
    // For Threshold apps:
    // We want to add the YearColumn, MonthColumn and DayColumn into the grid.Columns collection in the correct order.
    switch (yearOrder)
    {
        case 0:
            if (m_tpFirstPickerHost.Get() && m_tpYearPicker.Get() && yearVisible)
            {
                IFC(m_tpFirstPickerHost->put_Child(m_tpYearPicker.Cast<Selector>()));
                firstHostPopulated = TRUE;
            }
            else if (m_tpYearTextBlock && yearVisible)
            {
                firstHostPopulated = TRUE;
                firstTextBlockColumn = m_tpYearColumn.Get();
            }
            break;
        case 1:
            if (m_tpSecondPickerHost.Get() && m_tpYearPicker.Get() && yearVisible)
            {
                IFC(m_tpSecondPickerHost->put_Child(m_tpYearPicker.Cast<Selector>()));
                secondHostPopulated = TRUE;
            }
            else if (m_tpYearTextBlock && yearVisible)
            {
                secondHostPopulated = TRUE;
                secondTextBlockColumn = m_tpYearColumn.Get();
            }
            break;
        case 2:
            if (m_tpThirdPickerHost.Get() && m_tpYearPicker.Get() && yearVisible)
            {
                IFC(m_tpThirdPickerHost->put_Child(m_tpYearPicker.Cast<Selector>()));
                thirdHostPopulated = TRUE;
            }
            else if (m_tpYearTextBlock && yearVisible)
            {
                thirdHostPopulated = TRUE;
                thirdTextBlockColumn = m_tpYearColumn.Get();
            }
            break;
    }

    switch (monthOrder)
    {
        case 0:
            if (m_tpFirstPickerHost.Get() && m_tpMonthPicker.Get() && monthVisible)
            {
                IFC(m_tpFirstPickerHost->put_Child(m_tpMonthPicker.Cast<Selector>()));
                firstHostPopulated = TRUE;
            }
            else if (m_tpMonthTextBlock && monthVisible)
            {
                firstHostPopulated = TRUE;
                firstTextBlockColumn = m_tpMonthColumn.Get();
            }
            break;
        case 1:
            if (m_tpSecondPickerHost.Get() && m_tpMonthPicker.Get() && monthVisible)
            {
                IFC(m_tpSecondPickerHost->put_Child(m_tpMonthPicker.Cast<Selector>()));
                secondHostPopulated = TRUE;
            }
            else if (m_tpMonthTextBlock && monthVisible)
            {
                secondHostPopulated = TRUE;
                secondTextBlockColumn = m_tpMonthColumn.Get();
            }
            break;
        case 2:
            if (m_tpThirdPickerHost.Get() && m_tpMonthPicker.Get() && monthVisible)
            {
                IFC(m_tpThirdPickerHost->put_Child(m_tpMonthPicker.Cast<Selector>()));
                thirdHostPopulated = TRUE;
            }
            else if (m_tpMonthTextBlock && monthVisible)
            {
                thirdHostPopulated = TRUE;
                thirdTextBlockColumn = m_tpMonthColumn.Get();
            }
            break;
    }

    switch (dayOrder)
    {
        case 0:
            if (m_tpFirstPickerHost.Get() && m_tpDayPicker.Get() && dayVisible)
            {
                IFC(m_tpFirstPickerHost->put_Child(m_tpDayPicker.Cast<Selector>()));
                firstHostPopulated = TRUE;
            }
            else if (m_tpDayTextBlock && dayVisible)
            {
                firstHostPopulated = TRUE;
                firstTextBlockColumn = m_tpDayColumn.Get();
            }
            break;
        case 1:
            if (m_tpSecondPickerHost.Get() && m_tpDayPicker.Get() && dayVisible)
            {
                IFC(m_tpSecondPickerHost->put_Child(m_tpDayPicker.Cast<Selector>()));
                secondHostPopulated = TRUE;
            }
            else if (m_tpDayTextBlock && dayVisible)
            {
                secondHostPopulated = TRUE;
                secondTextBlockColumn = m_tpDayColumn.Get();
            }
            break;
        case 2:
            if (m_tpThirdPickerHost.Get() && m_tpDayPicker.Get() && dayVisible)
            {
                IFC(m_tpThirdPickerHost->put_Child(m_tpDayPicker.Cast<Selector>()));
                thirdHostPopulated = TRUE;
            }
            else if (m_tpDayTextBlock && dayVisible)
            {
                thirdHostPopulated = TRUE;
                thirdTextBlockColumn = m_tpDayColumn.Get();
            }
            break;
    }


    // Add the columns to the grid in the correct order (as computed in the switch statement above).
    if (spColumns)
    {
        if (firstTextBlockColumn)
        {
            spColumns->Append(firstTextBlockColumn.Get());
        }

        if (m_tpFirstSpacerColumn)
        {
            spColumns->Append(m_tpFirstSpacerColumn.Get());
        }

        if (secondTextBlockColumn)
        {
            spColumns->Append(secondTextBlockColumn.Get());
        }

        if (m_tpSecondSpacerColumn)
        {
            spColumns->Append(m_tpSecondSpacerColumn.Get());
        }

        if (thirdTextBlockColumn)
        {
            spColumns->Append(thirdTextBlockColumn.Get());
        }
    }

    // Set the Grid.Column property on the Day/Month/Year TextBlocks to the index of the matching ColumnDefinition
    // (e.g. YearTextBlock Grid.Column = columns.IndexOf(YearColumn)
    if (m_tpYearTextBlock && m_tpYearColumn && yearVisible && spColumns)
    {
        IFC(spColumns->IndexOf(m_tpYearColumn.Get(), &columnIndex, &columnIsFound));
        ASSERT(columnIsFound);
        IFC(PropertyValue::CreateFromInt32(columnIndex, &spInspectable));
        IFC(m_tpYearTextBlock.Cast<TextBlock>()->SetValueByKnownIndex(KnownPropertyIndex::Grid_Column, spInspectable.Get()));
    }
    if (m_tpMonthTextBlock && m_tpMonthColumn && monthVisible && spColumns)
    {
        IFC(spColumns->IndexOf(m_tpMonthColumn.Get(), &columnIndex, &columnIsFound));
        ASSERT(columnIsFound);
        IFC(PropertyValue::CreateFromInt32(columnIndex, &spInspectable));
        IFC(m_tpMonthTextBlock.Cast<TextBlock>()->SetValueByKnownIndex(KnownPropertyIndex::Grid_Column, spInspectable.Get()));
    }
    if (m_tpDayTextBlock && m_tpDayColumn && dayVisible && spColumns)
    {
        IFC(spColumns->IndexOf(m_tpDayColumn.Get(), &columnIndex, &columnIsFound));
        ASSERT(columnIsFound);
        IFC(PropertyValue::CreateFromInt32(columnIndex, &spInspectable));
        IFC(m_tpDayTextBlock.Cast<TextBlock>()->SetValueByKnownIndex(KnownPropertyIndex::Grid_Column, spInspectable.Get()));
    }

    // Collapse the Day/Month/Year TextBlocks if DayVisible/MonthVisible/YearVisible are false.
    if (m_tpDayTextBlock)
    {
        IFC(m_tpDayTextBlock.Cast<TextBlock>()->put_Visibility(dayVisible ? xaml::Visibility_Visible : xaml::Visibility_Collapsed));
    }
    if (m_tpMonthTextBlock)
    {
        IFC(m_tpMonthTextBlock.Cast<TextBlock>()->put_Visibility(monthVisible ? xaml::Visibility_Visible : xaml::Visibility_Collapsed));
    }
    if (m_tpYearTextBlock)
    {
        IFC(m_tpYearTextBlock.Cast<TextBlock>()->put_Visibility(yearVisible ? xaml::Visibility_Visible : xaml::Visibility_Collapsed));
    }

    // Determine if we will show the spacings and assign visibilities to spacing holders. We will determine if the spacings
    // are shown by looking at which borders are populated.
    // Also move the spacers to the correct column.
    if (m_tpFirstPickerSpacing.Get())
    {
        IFC(m_tpFirstPickerSpacing.Cast<Border>()->put_Visibility(
            firstHostPopulated && (secondHostPopulated || thirdHostPopulated) ?
            xaml::Visibility_Visible : xaml::Visibility_Collapsed));
        if (m_tpFirstSpacerColumn && spColumns)
        {
            IFC(spColumns->IndexOf(m_tpFirstSpacerColumn.Get(), &columnIndex, &columnIsFound));
            IFC(PropertyValue::CreateFromInt32(columnIndex, &spInspectable));
            IFC(m_tpFirstPickerSpacing.Cast<UIElement>()->SetValueByKnownIndex(KnownPropertyIndex::Grid_Column, spInspectable.Get()));
        }
    }
    if (m_tpSecondPickerSpacing.Get())
    {
        IFC(m_tpSecondPickerSpacing.Cast<Border>()->put_Visibility(
            secondHostPopulated && thirdHostPopulated ?
            xaml::Visibility_Visible : xaml::Visibility_Collapsed));
        if (m_tpSecondSpacerColumn && spColumns)
        {
            IFC(spColumns->IndexOf(m_tpSecondSpacerColumn.Get(), &columnIndex, &columnIsFound));
            IFC(PropertyValue::CreateFromInt32(columnIndex, &spInspectable));
            IFC(m_tpSecondPickerSpacing.Cast<UIElement>()->SetValueByKnownIndex(KnownPropertyIndex::Grid_Column, spInspectable.Get()));
        }
    }

Cleanup:
    RRETURN(hr);
}


// We execute our logic depending on some state information such as start date, end date, number of years etc. These state
// variables need to be updated whenever a public property change occurs which affects them.
_Check_return_
HRESULT
DatePicker::UpdateState() noexcept
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wg::ICalendar> spCalendar;
    wrl_wrappers::HString strCalendarIdentifier;
    INT32 month = 0;
    INT32 day = 0;
    wf::DateTime minYearDate = {};
    wf::DateTime maxYearDate = {};
    wf::DateTime maxCalendarDate = {};
    wf::DateTime minCalendarDate = {};

    // Create a calendar with the the current CalendarIdentifier
    m_tpCalendar.Clear();
    IFC(get_CalendarIdentifier(strCalendarIdentifier.GetAddressOf()));
    IFC(CreateNewCalendar(strCalendarIdentifier.Get(), &spCalendar));
#ifndef _PREFAST_ // PREfast bug DevDiv:554051
    ASSERT(spCalendar);
#endif
    SetPtrValue(m_tpCalendar, spCalendar.Get());

    // Get the minYear and maxYear dates
    IFC(get_MinYear(&minYearDate));
    IFC(get_MaxYear(&maxYearDate));

    // We do not have a valid range if our MinYear is later than our MaxYear
    m_hasValidYearRange = minYearDate.UniversalTime <= maxYearDate.UniversalTime;

    if (m_hasValidYearRange)
    {
        // Find the earliest and latest dates available for this calendar.
        IFC(m_tpCalendar->SetToMin());
        IFC(m_tpCalendar->GetDateTime(&minCalendarDate));

        //Find the latest date available for this calendar.
        IFC(m_tpCalendar->SetToMax());
        IFC(m_tpCalendar->GetDateTime(&maxCalendarDate));

        minYearDate = ClampDate(minYearDate, minCalendarDate, maxCalendarDate);
        maxYearDate = ClampDate(maxYearDate, minCalendarDate, maxCalendarDate);

        // Since we only care about the year field of minYearDate and maxYearDate we will change other fields into first day and last day
        // of the year respectively.
        IFC(m_tpCalendar->SetDateTime(minYearDate));
        IFC(m_tpCalendar->get_FirstMonthInThisYear(&month));
        IFC(m_tpCalendar->put_Month(month));
        IFC(m_tpCalendar->get_FirstDayInThisMonth(&day));
        IFC(m_tpCalendar->put_Day(day));
        IFC(m_tpCalendar->GetDateTime(&minYearDate));

        IFC(m_tpCalendar->SetDateTime(maxYearDate));
        IFC(m_tpCalendar->get_LastMonthInThisYear(&month));
        IFC(m_tpCalendar->put_Month(month));
        IFC(m_tpCalendar->get_LastDayInThisMonth(&day));
        IFC(m_tpCalendar->put_Day(day));
        IFC(m_tpCalendar->GetDateTime(&maxYearDate));

        IFC(m_tpCalendar->SetDateTime(minYearDate));
        //Set our sentinel time to the start date as we will be using it while generating item sources, we do not need to do this for end date
        IFC(m_tpCalendar->put_Hour(DATEPICKER_SENTINELTIME_HOUR));
        IFC(m_tpCalendar->put_Minute(DATEPICKER_SENTINELTIME_MINUTE));
        IFC(m_tpCalendar->put_Second(DATEPICKER_SENTINELTIME_SECOND));
        IFC(m_tpCalendar->GetDateTime(&m_startDate));
        m_endDate = maxYearDate;

        // Find the number of years in our range
        IFC(m_tpCalendar->SetDateTime(m_startDate));
        IFC(CreateNewCalendar(strCalendarIdentifier.Get(), &spCalendar));
#ifndef _PREFAST_ // PREfast bug DevDiv:554051
        ASSERT(spCalendar);
#endif
        IFC(spCalendar->SetDateTime(m_endDate));

        IFC(GetYearDifference(m_tpCalendar.Get(), spCalendar.Get(), m_numberOfYears));
        m_numberOfYears++; //since we should include both start and end years
    }
    else
    {
        // We do not want to display anything if we do not have a valid year range
        IFC(ClearSelectors(TRUE /*Clear day*/, TRUE /*Clear month*/, TRUE/*Clear year*/));
    }

    IFC(UpdateOrderAndLayout());

Cleanup:
    RRETURN(hr);
}

// Create DatePickerAutomationPeer to represent the DatePicker.
IFACEMETHODIMP DatePicker::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IDatePickerAutomationPeer> spDatePickerAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IDatePickerAutomationPeerFactory> spDatePickerAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;

    IFCPTR(ppAutomationPeer);
    *ppAutomationPeer = NULL;

    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::DatePickerAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spDatePickerAPFactory));

    IFC(spDatePickerAPFactory.Cast<DatePickerAutomationPeerFactory>()->CreateInstanceWithOwner(this,
        NULL,
        &spInner,
        &spDatePickerAutomationPeer));
    IFC(spDatePickerAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DatePicker::GetSelectedDateAsString(_Out_ HSTRING* strPlainText)
{
    ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatter;
    wrl_wrappers::HString strData;
    wrl_wrappers::HString strCalendarIdentifier;
    wf::DateTime date = {};

    IFC_RETURN(GetSelectedDate(&date));

    if (date.UniversalTime != GetNullDateSentinelValue())
    {
        IFC_RETURN(get_CalendarIdentifier(strCalendarIdentifier.GetAddressOf()));
        IFC_RETURN(CreateNewFormatter(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"day month.full year")).Get(), strCalendarIdentifier.Get(), &spFormatter));
        IFC_RETURN(spFormatter->Format(date, strData.ReleaseAndGetAddressOf()));
        IFC_RETURN(strData.CopyTo(strPlainText));
    }

    return S_OK;
}


_Check_return_ HRESULT DatePicker::RefreshFlyoutButtonAutomationName()
{
    if (m_tpFlyoutButton.Get())
    {
        wrl_wrappers::HString strParentAutomationName;
        IFC_RETURN(DirectUI::AutomationProperties::GetNameStatic(this, strParentAutomationName.ReleaseAndGetAddressOf()));
        if (strParentAutomationName.Length() == 0)
        {
            ctl::ComPtr<IInspectable> spHeaderAsInspectable;
            IFC_RETURN(get_Header(&spHeaderAsInspectable));
            if (spHeaderAsInspectable)
            {
                IFC_RETURN(FrameworkElement::GetStringFromObject(spHeaderAsInspectable.Get(), strParentAutomationName.ReleaseAndGetAddressOf()));
            }
        }
        LPCWSTR pszParent = strParentAutomationName.GetRawBuffer(NULL);


        wrl_wrappers::HString strSelectedValue;
        IFC_RETURN(GetSelectedDateAsString(strSelectedValue.GetAddressOf()));
        LPCWSTR pszSelectedValue = strSelectedValue.GetRawBuffer(NULL);

        wrl_wrappers::HString strMsgFormat;
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_NAME_DATEPICKER, strMsgFormat.GetAddressOf()));
        LPCWSTR pszMsgFormat = strMsgFormat.GetRawBuffer(NULL);

        WCHAR szBuffer[MAX_PATH];
        int cchBuffer = 0;

        ctl::ComPtr<wf::IReference<wf::DateTime>> selectedDate;
        IFC_RETURN(get_SelectedDate(&selectedDate));

        if (selectedDate)
        {
            cchBuffer = FormatMsg(szBuffer, pszMsgFormat, pszParent, pszSelectedValue);
        }
        else
        {
            cchBuffer = FormatMsg(szBuffer, pszMsgFormat, pszParent, L"");
        }

        // no charater wrote, szBuffer is blank don't update NameProperty
        if (cchBuffer > 0)
        {
            IFC_RETURN(DirectUI::AutomationProperties::SetNameStatic(m_tpFlyoutButton.Cast<Button>(), wrl_wrappers::HStringReference(szBuffer).Get()));
        }
    }
    return S_OK;
}

/* static */ wf::DateTime DatePicker::GetNullDateSentinel()
{
    wf::DateTime nullDate = {};
    nullDate.UniversalTime = GetNullDateSentinelValue();
    return nullDate;
}

/* static */ long DatePicker::GetNullDateSentinelValue()
{
    return 0;
}

_Check_return_ HRESULT DatePicker::GetTodaysDate(_Out_ wf::DateTime* todaysDate)
{
    if (m_todaysDate.UniversalTime == GetNullDateSentinelValue())
    {
        ctl::ComPtr<wg::ICalendar> calendar;
        wrl_wrappers::HString calendarIdentifier;

        IFC_RETURN(get_CalendarIdentifier(calendarIdentifier.GetAddressOf()));
        IFC_RETURN(CreateNewCalendar(calendarIdentifier.Get(), &calendar));
#ifndef _PREFAST_ // PREfast bug DevDiv:554051
        ASSERT(calendar);
#endif
        IFC_RETURN(calendar->SetToNow());
        IFC_RETURN(calendar->GetDateTime(&m_todaysDate));
        m_todaysDate = ClampDate(m_todaysDate, m_startDate, m_endDate);
    }

    *todaysDate = m_todaysDate;
    return S_OK;
}

_Check_return_ HRESULT DatePicker::GetSelectedDate(_Out_ wf::DateTime* date)
{
    ctl::ComPtr<wf::IReference<wf::DateTime>> selectedDate;
    IFC_RETURN(get_SelectedDate(&selectedDate));

    if (selectedDate)
    {
        IFC_RETURN(selectedDate->get_Value(date));
    }

    return S_OK;
}