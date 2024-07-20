// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TimePicker.g.h"
#include "TimePickerValueChangedEventArgs.g.h"
#include "TimePickerSelectedValueChangedEventArgs.g.h"
#include "ComboBox.g.h"
#include "Button.g.h"
#include "Selector.g.h"
#include "ItemsControl.g.h"
#include "Border.g.h"
#include "TimePickerAutomationPeer.g.h"
#include "Window.g.h"
#include <xstrutil.h>
#include "AutomationProperties.h"
#include "localizedResource.h"
#include <PhoneImports.h>
#include <windows.globalization.datetimeformatting.h>

// This is July 15th, 2011 as our sentinel date. There are no known
//  daylight savings transitions that happened on that date.
#define TIMEPICKER_SENTINELDATE_YEAR 2011
#define TIMEPICKER_SENTINELDATE_MONTH 7
#define TIMEPICKER_SENTINELDATE_DAY 15
#define TIMEPICKER_SENTINELDATE_TIMEFIELDS 0
#define TIMEPICKER_SENTINELDATE_HOUR12 12
#define TIMEPICKER_SENTINELDATE_HOUR24 0
#define TIMEPICKER_COERCION_INDEX 0
#define TIMEPICKER_COERCION_OFFSET 12
#define TIMEPICKER_AM_INDEX 0
#define TIMEPICKER_PM_INDEX 1
#define TIMEPICKER_RTL_CHARACTER_CODE 8207
#define TIMEPICKER_MINUTEINCREMENT_MIN 0
#define TIMEPICKER_MINUTEINCREMENT_MAX 59
// When the minute increment is set to 0, we want to only have 00 at the minute picker. This
// can be easily obtained by treating 0 as 60 with our existing logic. So during our logic, if we see
// that minute increment is zero we will use 60 in our calculations.
#define TIMEPICKER_MINUTEINCREMENT_ZERO_REPLACEMENT 60

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace std::placeholders;

const WCHAR* TimePicker::s_strTwelveHourClock = L"12HourClock";
const WCHAR* TimePicker::s_strHourFormat = L"{hour.integer(1)}";
const WCHAR* TimePicker::s_strMinuteFormat = L"{minute.integer(2)}";
const WCHAR* TimePicker::s_strPeriodFormat = L"{period.abbreviated(2)}";
const INT64 TimePicker::s_timeSpanTicksPerMinute = 600000000;
const INT64 TimePicker::s_timeSpanTicksPerHour = 36000000000;
const INT64 TimePicker::s_timeSpanTicksPerDay = 864000000000;

TimePicker::TimePicker()
    :m_is12HourClock(FALSE),
     m_reactionToSelectionChangeAllowed(FALSE)
{
    m_defaultTime.Duration = GetNullTimeSentinelValue();
    m_currentTime.Duration = GetNullTimeSentinelValue();
}

TimePicker::~TimePicker()
{
    // This will ensure the pending async operation
    // completes and closes the open dialog
    if (m_tpAsyncSelectionInfo)
    {
        IGNOREHR(m_tpAsyncSelectionInfo->Cancel());
    }

    if (m_loadedEventHandler)
    {
        IGNOREHR(m_loadedEventHandler.DetachEventHandler(ctl::iinspectable_cast(this)));
    }

    if (m_windowActivatedHandler && DXamlCore::GetCurrent())
    {
        Window* window = nullptr;
        IGNOREHR(DXamlCore::GetCurrent()->GetAssociatedWindowNoRef(this, &window));
        if (window)
        {
            IGNOREHR(m_windowActivatedHandler.DetachEventHandler(ctl::iinspectable_cast(window)));
        }
    }
}

_Check_return_ HRESULT TimePicker::PrepareState()
{
    ctl::ComPtr<TrackerCollection<IInspectable*>> spCollection;

    IFC_RETURN(TimePickerGenerated::PrepareState());

    //Initialize the collections we will be using as itemssources.
    IFC_RETURN(ctl::make(&spCollection));
    SetPtrValue(m_tpHourSource, spCollection.Get());

    IFC_RETURN(ctl::make(&spCollection));
    SetPtrValue(m_tpMinuteSource, spCollection.Get());

    IFC_RETURN(ctl::make(&spCollection));
    SetPtrValue(m_tpPeriodSource, spCollection.Get());

    IFC_RETURN(RefreshSetup());

    IFC_RETURN(m_loadedEventHandler.AttachEventHandler(this, std::bind(&TimePicker::OnLoaded, this, _1, _2)));

    return S_OK;
}

_Check_return_ HRESULT TimePicker::OnLoaded(_In_ IInspectable* /*sender*/, _In_ xaml::IRoutedEventArgs* /*args*/)
{
    Window* window = nullptr;
    IFC_RETURN(DXamlCore::GetCurrent()->GetAssociatedWindowNoRef(this, &window));
    if (window)
    {
        ctl::WeakRefPtr weakInstance;
        IFC_RETURN(ctl::AsWeak(this, &weakInstance));

        IFC_RETURN(m_windowActivatedHandler.AttachEventHandler(
            window,
            [weakInstance](IInspectable*, xaml::IWindowActivatedEventArgs*) mutable
            {
                ctl::ComPtr<TimePicker> instance = weakInstance.AsOrNull<TimePicker>();
                if (instance != nullptr)
                {
                    return instance->RefreshSetup();
                }
                return S_OK;
            }));
    }

    return S_OK;
}

// Called when the IsEnabled property changes.
_Check_return_ HRESULT TimePicker::OnIsEnabledChanged(_In_ IsEnabledChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

// Change to the correct visual state for the TimePicker.
_Check_return_ HRESULT TimePicker::ChangeVisualState(_In_ bool useTransitions)
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

    ctl::ComPtr<wf::IReference<wf::TimeSpan>> selectedTime;
    IFC_RETURN(get_SelectedTime(&selectedTime));

    if (selectedTime)
    {
        IFC_RETURN(GoToState(useTransitions, L"HasTime", &ignored));
    }
    else
    {
        IFC_RETURN(GoToState(useTransitions, L"HasNoTime", &ignored));
    }

    return S_OK;
}

_Check_return_ HRESULT TimePicker::OnKeyDown(_In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    IFCPTR_RETURN(pArgs);
    wsy::VirtualKeyModifiers nModifierKeys;
    wsy::VirtualKey key = wsy::VirtualKey_None;

    IFC_RETURN(pArgs->get_Key(&key));
    IFC_RETURN(CoreImports::Input_GetKeyboardModifiers(&nModifierKeys));

    // Alt+Up or Alt+Down opens the TimePickerFlyout
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

// Gives the default values for our properties.
_Check_return_ HRESULT TimePicker::GetDefaultValue2(
    _In_ const CDependencyProperty* pDP,
    _Outptr_ CValue* pValue)
{
    IFCPTR_RETURN(pDP);
    IFCPTR_RETURN(pValue);

    switch (pDP->GetIndex())
    {
    case KnownPropertyIndex::TimePicker_ClockIdentifier:
        {
            // Our default clock identifier is user's system clock. We use datetimeformatter to get the system clock.
            wrl_wrappers::HString strClockIdentifier;
            xstring_ptr xstrClockIdentifier;
            ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatter;
            ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatterFactory> spFormatterFactory;

            IFC_RETURN(ctl::GetActivationFactory(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"Windows.Globalization.DateTimeFormatting.DateTimeFormatter")).Get(), &spFormatterFactory));
            IFCPTR_RETURN(spFormatterFactory.Get());

            IFC_RETURN(spFormatterFactory->CreateDateTimeFormatter(wrl_wrappers::HStringReference(s_strHourFormat).Get(), &spFormatter));
            IFC_RETURN(spFormatter->get_Clock(strClockIdentifier.GetAddressOf()));

            IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(strClockIdentifier.Get(), &xstrClockIdentifier));
            pValue->SetString(std::move(xstrClockIdentifier));
            break;
        }
    case KnownPropertyIndex::TimePicker_Time:
        {
            IFC_RETURN(CValueBoxer::BoxValue(pValue, m_defaultTime));
            break;
        }
    default:
        IFC_RETURN(TimePickerGenerated::GetDefaultValue2(pDP, pValue));
        break;
    }

    return S_OK;
}

// Reacts to change in MinuteIncrement property.
_Check_return_
HRESULT
TimePicker::OnMinuteIncrementChanged(
    _In_ IInspectable* pOldValue,
    _In_ IInspectable* pNewValue)
{
    HRESULT hr = S_OK;
    INT32 newValue = 0;

    IFC(ctl::do_get_value(newValue, pNewValue));
    if (newValue < TIMEPICKER_MINUTEINCREMENT_MIN || newValue > TIMEPICKER_MINUTEINCREMENT_MAX)
    {
        INT32 oldValue = 0;

        IFC(ctl::do_get_value(oldValue, pNewValue));
        IFC(put_MinuteIncrement(oldValue));
        IFC(E_INVALIDARG);
    }
    IFC(RefreshSetup());

Cleanup:
    RRETURN(hr);
}

// Reacts to change in ClockIdentifier property.
_Check_return_
HRESULT
TimePicker::OnClockIdentifierChanged(
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
        VERIFYHR(put_ClockIdentifier(strOldValue.Get()));
    }
    RRETURN(hr);
}

// Handle the custom property changed event and call the
// OnPropertyChanged2 methods.
_Check_return_
HRESULT
TimePicker::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(TimePickerGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::TimePicker_Time:
        {
            ctl::ComPtr<IInspectable> spOldValue, spNewValue;
            IFC_RETURN(CValueBoxer::UnboxObjectValue(args.m_pOldValue, args.m_pDP->GetPropertyType(), &spOldValue));
            IFC_RETURN(CValueBoxer::UnboxObjectValue(args.m_pNewValue, args.m_pDP->GetPropertyType(), &spNewValue));

            if (!m_isPropagatingTime)
            {
                wf::TimeSpan newValue = {};
                IFC_RETURN(ctl::do_get_value(newValue, spNewValue.Get()));

                // When SelectedTime is set to null, we set Time to a sentinel value that also represents null,
                // and vice-versa.
                if (newValue.Duration != GetNullTimeSentinelValue())
                {
                    ctl::ComPtr<IInspectable> boxedTimeAsI;
                    ctl::ComPtr<wf::IReference<wf::TimeSpan>> boxedTime;
                    IFC_RETURN(PropertyValue::CreateFromTimeSpan(newValue, &boxedTimeAsI));
                    IFC_RETURN(boxedTimeAsI.As(&boxedTime));

                    {
                        m_isPropagatingTime = true;
                        auto scopeGuard = wil::scope_exit([this]() { m_isPropagatingTime = false; });
                        IFC_RETURN(put_SelectedTime(boxedTime.Get()));
                    }
                }
                else
                {
                    m_isPropagatingTime = true;
                    auto scopeGuard = wil::scope_exit([this]() { m_isPropagatingTime = false; });
                    IFC_RETURN(put_SelectedTime(nullptr));
                }
            }

            IFC_RETURN(OnTimeChanged(spOldValue.Get(), spNewValue.Get()));
        }
        break;

    case KnownPropertyIndex::TimePicker_SelectedTime:
        {
            if (!m_isPropagatingTime)
            {
                ctl::ComPtr<wf::IReference<wf::TimeSpan>> selectedTime;
                IFC_RETURN(get_SelectedTime(&selectedTime));

                if (selectedTime)
                {
                    wf::TimeSpan time;
                    IFC_RETURN(selectedTime->get_Value(&time));

                    {
                        m_isPropagatingTime = true;
                        auto scopeGuard = wil::scope_exit([this]() { m_isPropagatingTime = false; });
                        IFC_RETURN(put_Time(time));
                    }
                }
                else
                {
                    m_isPropagatingTime = true;
                    auto scopeGuard = wil::scope_exit([this]() { m_isPropagatingTime = false; });
                    IFC_RETURN(put_Time(GetNullTimeSentinel()));
                }
            }

            if (m_tpFlyoutButton.Get())
            {
                IFC_RETURN(UpdateFlyoutButtonContent());
            }

            IFC_RETURN(UpdateVisualState());
            break;
        }

    case KnownPropertyIndex::TimePicker_MinuteIncrement:
        {
            ctl::ComPtr<IInspectable> spOldValue, spNewValue;
            IFC_RETURN(CValueBoxer::UnboxObjectValue(args.m_pOldValue, args.m_pDP->GetPropertyType(), &spOldValue));
            IFC_RETURN(CValueBoxer::UnboxObjectValue(args.m_pNewValue, args.m_pDP->GetPropertyType(), &spNewValue));
            IFC_RETURN(OnMinuteIncrementChanged(spOldValue.Get(), spNewValue.Get()));
        }
        break;

    case KnownPropertyIndex::TimePicker_ClockIdentifier:
        {
            ctl::ComPtr<IInspectable> spOldValue, spNewValue;
            IFC_RETURN(CValueBoxer::UnboxObjectValue(args.m_pOldValue, args.m_pDP->GetPropertyType(), &spOldValue));
            IFC_RETURN(CValueBoxer::UnboxObjectValue(args.m_pNewValue, args.m_pDP->GetPropertyType(), &spNewValue));
            IFC_RETURN(OnClockIdentifierChanged(spOldValue.Get(), spNewValue.Get()));
        }
        break;

    case KnownPropertyIndex::TimePicker_Header:
    case KnownPropertyIndex::TimePicker_HeaderTemplate:
        IFC_RETURN(UpdateHeaderPresenterVisibility());
        break;

#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
    case KnownPropertyIndex::TimePicker_HeaderPlacement:
        IFC_RETURN(UpdateVisualState());
        break;
#endif
    }

    return S_OK;
}

// Reacts to changes in Time property. Time may have changed programmatically or end user may have changed the
// selection of one of our selectors causing a change in Time.
_Check_return_
HRESULT
TimePicker::OnTimeChanged(
    _In_ IInspectable* pOldValue,
    _In_ IInspectable* pNewValue)
{
    wf::TimeSpan oldValue = {};
    wf::TimeSpan newValue = {};
    wf::TimeSpan coercedTime = {};
    ctl::ComPtr<wg::ICalendar> spCalendar;
    wrl_wrappers::HString strClockIdentifier;

    IFC_RETURN(ctl::do_get_value(oldValue, pOldValue));
    IFC_RETURN(ctl::do_get_value(newValue, pNewValue));

    // It is possible for the value of ClockIdentifier to change without us getting a property changed notification.
    // This can happen if the property is unset (i.e. default value) and so the effective value matches the system settings.
    // If the system settings changes, the effective value of ClockIdentifier can change.
    IFC_RETURN(get_ClockIdentifier(strClockIdentifier.GetAddressOf()));
    BOOLEAN newIs12HourClock = strClockIdentifier == wrl_wrappers::HStringReference(s_strTwelveHourClock);
    if (newIs12HourClock != m_is12HourClock)
    {
        IFC_RETURN(RefreshSetup());
    }

    // The Time might have changed due to a change in the time zone.
    // To account for this we need to re-create m_tpCalendar (which internally uses the time zone as it was at the time of its creation).
    IFC_RETURN(CreateNewCalendar(strClockIdentifier.Get(), &spCalendar));
    m_tpCalendar.Clear();
    SetPtrValue(m_tpCalendar, spCalendar.Get());

    IFC_RETURN(CheckAndCoerceTime(newValue, &coercedTime));

    // We are checking to see if new value is different from the current one. This is because even if they are same,
    // calling put_Time will break any Binding on Time (if there is any) that this TimePicker is target of.
    if (newValue.Duration != coercedTime.Duration)
    {
        // We are coercing the time. The new property change will execute the necessary logic so
        // we will just go to cleanup after this call.
        IFC_RETURN(put_Time(coercedTime));
        return S_OK;
    }

    IFC_RETURN(UpdateDisplay());

    ctl::ComPtr<TimePickerValueChangedEventArgs> valueChangedEventArgs;
    TimeChangedEventSourceType* timeChangedEventSource = nullptr;

    // Create and populate the value changed event args
    IFC_RETURN(ctl::make<TimePickerValueChangedEventArgs>(&valueChangedEventArgs));
    IFC_RETURN(valueChangedEventArgs->put_OldTime(oldValue));
    IFC_RETURN(valueChangedEventArgs->put_NewTime(newValue));

    // Raise event
    IFC_RETURN(TimePickerGenerated::GetTimeChangedEventSourceNoRef(&timeChangedEventSource));
    IFC_RETURN(timeChangedEventSource->Raise(ctl::as_iinspectable(this), valueChangedEventArgs.Get()));

    ctl::ComPtr<TimePickerSelectedValueChangedEventArgs> selectedValueChangedEventArgs;
    SelectedTimeChangedEventSourceType* selectedTimeChangedEventSource = nullptr;

    // Create and populate the selected value changed event args
    IFC_RETURN(ctl::make<TimePickerSelectedValueChangedEventArgs>(&selectedValueChangedEventArgs));

    if (oldValue.Duration != GetNullTimeSentinelValue())
    {
        ctl::ComPtr<IInspectable> boxedTimeAsI;
        ctl::ComPtr<wf::IReference<wf::TimeSpan>> boxedTime;
        IFC_RETURN(PropertyValue::CreateFromTimeSpan(oldValue, &boxedTimeAsI));
        IFC_RETURN(boxedTimeAsI.As(&boxedTime));

        IFC_RETURN(selectedValueChangedEventArgs->put_OldTime(boxedTime.Get()));
    }
    else
    {
        IFC_RETURN(selectedValueChangedEventArgs->put_OldTime(nullptr));
    }

    if (newValue.Duration != GetNullTimeSentinelValue())
    {
        ctl::ComPtr<IInspectable> boxedTimeAsI;
        ctl::ComPtr<wf::IReference<wf::TimeSpan>> boxedTime;
        IFC_RETURN(PropertyValue::CreateFromTimeSpan(newValue, &boxedTimeAsI));
        IFC_RETURN(boxedTimeAsI.As(&boxedTime));

        IFC_RETURN(selectedValueChangedEventArgs->put_NewTime(boxedTime.Get()));
    }
    else
    {
        IFC_RETURN(selectedValueChangedEventArgs->put_NewTime(nullptr));
    }

    // Raise event
    IFC_RETURN(TimePickerGenerated::GetSelectedTimeChangedEventSourceNoRef(&selectedTimeChangedEventSource));
    IFC_RETURN(selectedTimeChangedEventSource->Raise(this, selectedValueChangedEventArgs.Get()));

    return S_OK;
}

// Reacts to the FlyoutButton being pressed. Invokes a form-factor specific flyout if present.
_Check_return_
HRESULT
TimePicker::OnFlyoutButtonClick(
    _In_ IInspectable* pSender,
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    return ShowPickerFlyout();
}

_Check_return_ HRESULT TimePicker::ShowPickerFlyout()
{
    HRESULT hr = S_OK;

    if (!m_tpAsyncSelectionInfo)
    {
        ctl::ComPtr<IInspectable> spAsyncAsInspectable;
        ctl::ComPtr<wf::IAsyncOperation<wf::IReference<wf::TimeSpan>*>> spAsyncOperation;
        ctl::WeakRefPtr wpThis;

        IFC(ctl::AsWeak(this, &wpThis));

        auto callbackPtr = Microsoft::WRL::Callback<wf::IAsyncOperationCompletedHandler<wf::IReference<wf::TimeSpan>*>>(
            [wpThis] (wf::IAsyncOperation<wf::IReference<wf::TimeSpan>*>* getOperation, wf::AsyncStatus status) mutable
        {
            HRESULT hr = S_OK;
            ctl::ComPtr<TimePicker> spThis;
            spThis = wpThis.AsOrNull<xaml_controls::ITimePicker>().Cast<TimePicker>();

            if (spThis)
            {
                IFC(spThis->OnGetTimePickerSelectionAsyncCompleted(getOperation, status));
            }

        Cleanup:
            RRETURN(hr);
        });

        auto xamlControlsGetTimePickerSelectionPtr = reinterpret_cast<decltype(&XamlControlsGetTimePickerSelection)>(::GetProcAddress(GetPhoneModule(), "XamlControlsGetTimePickerSelection"));
        IFC(xamlControlsGetTimePickerSelectionPtr(ctl::as_iinspectable(this), ctl::as_iinspectable(m_tpFlyoutButton.Get()), spAsyncAsInspectable.GetAddressOf()));

        spAsyncOperation = spAsyncAsInspectable.AsOrNull<wf::IAsyncOperation<wf::IReference<wf::TimeSpan>*>>();
        IFCEXPECT(spAsyncOperation.Get());
        IFC(spAsyncOperation->put_Completed(callbackPtr.Get()));
        IFC(SetPtrValueWithQI(m_tpAsyncSelectionInfo, spAsyncOperation.Get()));
    }

Cleanup:
    RRETURN(hr);
}

// Callback passed to the GetTimePickerSelectionAsync method. Called when a form-factor specific
// flyout returns with a new TimeSpan value to update the TimePicker's DateTime.
_Check_return_ HRESULT TimePicker::OnGetTimePickerSelectionAsyncCompleted(
    wf::IAsyncOperation<wf::IReference<wf::TimeSpan>*>* getOperation,
    wf::AsyncStatus status)
{
    IFC_RETURN(CheckThread());

    m_tpAsyncSelectionInfo.Clear();

    if (status == wf::AsyncStatus::Completed)
    {
        ctl::ComPtr<wf::IReference<wf::TimeSpan>> selectedTime;
        IFC_RETURN(getOperation->GetResults(&selectedTime));

        // A null IReference object is returned when the user cancels out of the
        // TimePicker.
        if (selectedTime)
        {
            // We set SelectedTime instead of Time in order to ensure that the value
            // propagates to both SelectedTime and Time.
            // See the comment at the top of OnPropertyChanged2 for details.
            IFC_RETURN(put_SelectedTime(selectedTime.Get()));
        }
    }

    return S_OK;
}

// Checks whether the given time is in our acceptable range, coerces it or raises exception when necessary.
_Check_return_
HRESULT
TimePicker::CheckAndCoerceTime(
    _In_ wf::TimeSpan time,
    _Out_ wf::TimeSpan* pCoercedTime)
{
    wf::TimeSpan coercedTime = {};
    wf::DateTime dateTime = {};
    INT32 minute = 0;
    INT32 minuteIncrement = 0;

    // Check the value of time, we do not accept negative timespan values
    // except for the null-time sentinel value.
    if (time.Duration == GetNullTimeSentinelValue())
    {
        *pCoercedTime = time;
        return S_OK;
    }
    else if (time.Duration < 0)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    // If the time's duration is greater than 24 hours, we coerce it down to 24 hour range
    // by taking mod of it.
    coercedTime.Duration = time.Duration % s_timeSpanTicksPerDay;

    // Finally we coerce the minutes to a factor of MinuteIncrement
    IFC_RETURN(GetAdjustedMinuteIncrement(&minuteIncrement));
    IFC_RETURN(GetDateTimeFromTimeSpan(coercedTime, &dateTime));
    IFC_RETURN(m_tpCalendar->SetDateTime(dateTime));
    IFC_RETURN(m_tpCalendar->get_Minute(&minute));
    IFC_RETURN(m_tpCalendar->put_Minute(minute - (minute % minuteIncrement)));
    IFC_RETURN(m_tpCalendar->GetDateTime(&dateTime));
    IFC_RETURN(GetTimeSpanFromDateTime(dateTime, pCoercedTime));

    return S_OK;
}

// Get TimePicker template parts and create the sources if they are not already there
IFACEMETHODIMP
TimePicker::OnApplyTemplate()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ISelector> spHourPicker;
    ctl::ComPtr<ISelector> spMinutePicker;
    ctl::ComPtr<ISelector> spPeriodPicker;
    ctl::ComPtr<IBorder> spFirstPickerHost;
    ctl::ComPtr<IBorder> spSecondPickerHost;
    ctl::ComPtr<IBorder> spThirdPickerHost;
    ctl::ComPtr<IFrameworkElement> spLayoutRoot;
    ctl::ComPtr<IButtonBase> spFlyoutButton;
    ctl::ComPtr<ITextBlock> spHourTextBlock;
    ctl::ComPtr<ITextBlock> spMinuteTextBlock;
    ctl::ComPtr<ITextBlock> spPeriodTextBlock;
    ctl::ComPtr<IColumnDefinition> spFirstTextBlockColumn;
    ctl::ComPtr<IColumnDefinition> spSecondTextBlockColumn;
    ctl::ComPtr<IColumnDefinition> spThirdTextBlockColumn;
    ctl::ComPtr<IUIElement> spFirstColumnDivider;
    ctl::ComPtr<IUIElement> spSecondColumnDivider;
    wrl_wrappers::HString strAutomationName;
    wrl_wrappers::HString strParentAutomationName;
    wrl_wrappers::HString strComboAutomationName;
    // Clean up existing template parts
    if (m_tpHourPicker.Get())
    {
        IFC(m_epHourSelectionChangedHandler.DetachEventHandler(m_tpHourPicker.Get()));
    }

    if (m_tpMinutePicker.Get())
    {
        IFC(m_epMinuteSelectionChangedHandler.DetachEventHandler(m_tpMinutePicker.Get()));
    }

    if (m_tpPeriodPicker.Get())
    {
        IFC(m_epPeriodSelectionChangedHandler.DetachEventHandler(m_tpPeriodPicker.Get()));
    }

    if (m_tpFlyoutButton.Get())
    {
        IFC(m_epFlyoutButtonClickHandler.DetachEventHandler(m_tpFlyoutButton.Get()));
    }

    m_tpHourPicker.Clear();
    m_tpMinutePicker.Clear();
    m_tpPeriodPicker.Clear();

    m_tpFirstPickerHost.Clear();
    m_tpSecondPickerHost.Clear();
    m_tpThirdPickerHost.Clear();
    m_tpFirstTextBlockColumn.Clear();
    m_tpSecondTextBlockColumn.Clear();
    m_tpThirdTextBlockColumn.Clear();
    m_tpFirstColumnDivider.Clear();
    m_tpSecondColumnDivider.Clear();
    m_tpHeaderPresenter.Clear();
    m_tpLayoutRoot.Clear();
    m_tpFlyoutButton.Clear();

    m_tpHourTextBlock.Clear();
    m_tpMinuteTextBlock.Clear();
    m_tpPeriodTextBlock.Clear();

    IFC(TimePickerGenerated::OnApplyTemplate());

    // Get selectors for hour/minute/period pickers
    IFC(GetTemplatePart<ISelector>(STR_LEN_PAIR(L"HourPicker"), spHourPicker.ReleaseAndGetAddressOf()));
    IFC(GetTemplatePart<ISelector>(STR_LEN_PAIR(L"MinutePicker"), spMinutePicker.ReleaseAndGetAddressOf()));
    IFC(GetTemplatePart<ISelector>(STR_LEN_PAIR(L"PeriodPicker"), spPeriodPicker.ReleaseAndGetAddressOf()));

    // Get the hosting borders
    IFC(GetTemplatePart<IBorder>(STR_LEN_PAIR(L"FirstPickerHost"), spFirstPickerHost.ReleaseAndGetAddressOf()));
    IFC(GetTemplatePart<IBorder>(STR_LEN_PAIR(L"SecondPickerHost"), spSecondPickerHost.ReleaseAndGetAddressOf()));
    IFC(GetTemplatePart<IBorder>(STR_LEN_PAIR(L"ThirdPickerHost"), spThirdPickerHost.ReleaseAndGetAddressOf()));

    // Get the TextBlocks that are used to display the Hour/Minute/Period
    IFC(GetTemplatePart<ITextBlock>(STR_LEN_PAIR(L"HourTextBlock"), spHourTextBlock.ReleaseAndGetAddressOf()));
    IFC(GetTemplatePart<ITextBlock>(STR_LEN_PAIR(L"MinuteTextBlock"), spMinuteTextBlock.ReleaseAndGetAddressOf()));
    IFC(GetTemplatePart<ITextBlock>(STR_LEN_PAIR(L"PeriodTextBlock"), spPeriodTextBlock.ReleaseAndGetAddressOf()));

    // Get the ColumnDefinitions that are used to lay out the Hour/Minute/Period TextBlocks.
    IFC(GetTemplatePart<IColumnDefinition>(STR_LEN_PAIR(L"FirstTextBlockColumn"), spFirstTextBlockColumn.ReleaseAndGetAddressOf()));
    IFC(GetTemplatePart<IColumnDefinition>(STR_LEN_PAIR(L"SecondTextBlockColumn"), spSecondTextBlockColumn.ReleaseAndGetAddressOf()));
    IFC(GetTemplatePart<IColumnDefinition>(STR_LEN_PAIR(L"ThirdTextBlockColumn"), spThirdTextBlockColumn.ReleaseAndGetAddressOf()));

    // Get the the column dividers between the hour/minute/period textblocks.
    IFC(GetTemplatePart<IUIElement>(STR_LEN_PAIR(L"FirstColumnDivider"), spFirstColumnDivider.ReleaseAndGetAddressOf()));
    IFC(GetTemplatePart<IUIElement>(STR_LEN_PAIR(L"SecondColumnDivider"), spSecondColumnDivider.ReleaseAndGetAddressOf()));

    //Get the LayoutRoot
    IFC(GetTemplatePart<IFrameworkElement>(STR_LEN_PAIR(L"LayoutRoot"), spLayoutRoot.ReleaseAndGetAddressOf()));

    IFC(GetTemplatePart<IButtonBase>(STR_LEN_PAIR(L"FlyoutButton"), spFlyoutButton.ReleaseAndGetAddressOf()));

    SetPtrValue(m_tpHourPicker, spHourPicker.Get());
    SetPtrValue(m_tpMinutePicker, spMinutePicker.Get());
    SetPtrValue(m_tpPeriodPicker, spPeriodPicker.Get());

    SetPtrValue(m_tpFirstPickerHost, spFirstPickerHost.Get());
    SetPtrValue(m_tpSecondPickerHost, spSecondPickerHost.Get());
    SetPtrValue(m_tpThirdPickerHost, spThirdPickerHost.Get());

    SetPtrValue(m_tpFirstTextBlockColumn, spFirstTextBlockColumn.Get());
    SetPtrValue(m_tpSecondTextBlockColumn, spSecondTextBlockColumn.Get());
    SetPtrValue(m_tpThirdTextBlockColumn, spThirdTextBlockColumn.Get());

    SetPtrValue(m_tpFirstColumnDivider, spFirstColumnDivider.Get());
    SetPtrValue(m_tpSecondColumnDivider, spSecondColumnDivider.Get());

    SetPtrValue(m_tpHourTextBlock, spHourTextBlock.Get());
    SetPtrValue(m_tpMinuteTextBlock, spMinuteTextBlock.Get());
    SetPtrValue(m_tpPeriodTextBlock, spPeriodTextBlock.Get());

    SetPtrValue(m_tpLayoutRoot, spLayoutRoot.Get());

    SetPtrValue(m_tpFlyoutButton, spFlyoutButton.Get());

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
    if (m_tpHourPicker.Get())
    {
        IFC(m_epHourSelectionChangedHandler.AttachEventHandler(m_tpHourPicker.Get(),
            std::bind(&TimePicker::OnSelectorSelectionChanged, this, _1, _2)));

        IFC(DirectUI::AutomationProperties::GetNameStatic(m_tpHourPicker.Cast<ComboBox>(), strAutomationName.ReleaseAndGetAddressOf()));
        if(strAutomationName.Get() == NULL)
        {
            IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_TIMEPICKER_HOUR, strAutomationName.ReleaseAndGetAddressOf()));
            IFC(strAutomationName.Concat(strParentAutomationName, strComboAutomationName));
            IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpHourPicker.Cast<ComboBox>(), strComboAutomationName));
        }
    }
    if (m_tpMinutePicker.Get())
    {
        IFC(m_epMinuteSelectionChangedHandler.AttachEventHandler(m_tpMinutePicker.Get(),
            std::bind(&TimePicker::OnSelectorSelectionChanged, this, _1, _2)));

        IFC(DirectUI::AutomationProperties::GetNameStatic(m_tpMinutePicker.Cast<ComboBox>(), strAutomationName.ReleaseAndGetAddressOf()));
        if(strAutomationName.Get() == NULL)
        {
            IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_TIMEPICKER_MINUTE, strAutomationName.ReleaseAndGetAddressOf()));
            IFC(strAutomationName.Concat(strParentAutomationName, strComboAutomationName));
            IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpMinutePicker.Cast<ComboBox>(), strComboAutomationName));
        }
    }
    if (m_tpPeriodPicker.Get())
    {
        IFC(m_epPeriodSelectionChangedHandler.AttachEventHandler(m_tpPeriodPicker.Get(),
            std::bind(&TimePicker::OnSelectorSelectionChanged, this, _1, _2)));

        IFC(DirectUI::AutomationProperties::GetNameStatic(m_tpPeriodPicker.Cast<ComboBox>(), strAutomationName.ReleaseAndGetAddressOf()));
        if(strAutomationName.Get() == NULL)
        {
            IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_TIMEPICKER_PERIOD, strAutomationName.ReleaseAndGetAddressOf()));
            IFC(strAutomationName.Concat(strParentAutomationName, strComboAutomationName));
            IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpPeriodPicker.Cast<ComboBox>(), strComboAutomationName));
        }
    }

    IFC(RefreshSetup());

    if (m_tpFlyoutButton.Get())
    {
        IFC(m_epFlyoutButtonClickHandler.AttachEventHandler(m_tpFlyoutButton.Get(),
            std::bind(&TimePicker::OnFlyoutButtonClick, this, _1, _2)));

        IFC(RefreshFlyoutButtonAutomationName());

        IFC(UpdateFlyoutButtonContent());
    }

    IFC(UpdateVisualState(FALSE));

Cleanup:
    RRETURN(hr);
}

// Updates the visibility of the Header ContentPresenter
_Check_return_
HRESULT
TimePicker::UpdateHeaderPresenterVisibility()
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

// Reacts to change in selection of our selectors. Calculates the new date represented by the selected indices and updates the
// Date property.
_Check_return_
HRESULT
TimePicker::OnSelectorSelectionChanged(
    _In_ IInspectable* pSender,
    _In_ xaml_controls::ISelectionChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    if (IsReactionToSelectionChangeAllowed())
    {
        IFC(UpdateTime());
    }

Cleanup:
    RRETURN(hr);
}

// Updates the Time property according to the selected indices of the selectors.
_Check_return_
HRESULT
TimePicker::UpdateTime()
{
    HRESULT hr = S_OK;
    INT32 hourIndex = 0;
    INT32 minuteIndex = 0;
    INT32 minuteIncrement = 0;
    INT32 periodIndex = 0;
    wf::DateTime dateTime = {};
    wf::TimeSpan timeSpan = {};
    wf::TimeSpan currentTime = {};

    IFC(get_Time(&currentTime));

    // When the selectors are template bound the time is coerces through the
    // setting of their valid indices.
    if (m_tpHourPicker || m_tpMinutePicker || m_tpPeriodPicker)
    {
        if (m_tpHourPicker)
        {
            IFC(m_tpHourPicker.AsOrNull<ISelector>()->get_SelectedIndex(&hourIndex));
        }

        if (m_tpMinutePicker)
        {
            IFC(m_tpMinutePicker.AsOrNull<ISelector>()->get_SelectedIndex(&minuteIndex));
        }

        if (m_tpPeriodPicker)
        {
            IFC(m_tpPeriodPicker.AsOrNull<ISelector>()->get_SelectedIndex(&periodIndex));
        }

        IFC(SetSentinelDate(m_tpCalendar.Get()));

        if (m_is12HourClock)
        {
            INT32 firstPeriodInThisDay = 0;

            IFC(m_tpCalendar->get_FirstPeriodInThisDay(&firstPeriodInThisDay));
            IFC(m_tpCalendar->put_Period(periodIndex + firstPeriodInThisDay));
            // 12 hour clock time flow is 12, 1, 2, 3 ... 11 for both am and pm times. So if the index is 0 we need
            // to put hour 12 into hour calendar.
            if (hourIndex == TIMEPICKER_COERCION_INDEX)
            {
                IFC(m_tpCalendar->put_Hour(TIMEPICKER_COERCION_OFFSET));
            }
            else
            {
                IFC(m_tpCalendar->put_Hour(hourIndex));
            }
        }
        else
        {
            IFC(m_tpCalendar->put_Hour(hourIndex));
        }

        IFC(GetAdjustedMinuteIncrement(&minuteIncrement));
        IFC(m_tpCalendar->put_Minute(minuteIncrement * minuteIndex));

        IFC(m_tpCalendar->GetDateTime(&dateTime));
        IFC(GetTimeSpanFromDateTime(dateTime, &timeSpan));
    }
    else
    {
        // When no selectors are template bound (phone template) we
        // still want to coerce the time to sit on a valid MinuteIncrement.
        IFC(CheckAndCoerceTime(currentTime, &timeSpan));
    }

    // We are checking to see if new value is different from the current one. This is because even if they are same,
    // calling put_Time will break any Binding on Time (if there is any) that this TimePicker is target of.
    if (currentTime.Duration != timeSpan.Duration)
    {
        IFC(put_Time(timeSpan));
    }

Cleanup:
    RRETURN(hr);
}

//Updates the Content of the FlyoutButton to be the current time.
_Check_return_
HRESULT
TimePicker::UpdateFlyoutButtonContent()
{
    wrl_wrappers::HString strFormattedDate;
    ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spDateFormatter;
    wrl_wrappers::HString strClockIdentifier;
    wf::TimeSpan timeSpan = {};
    wf::DateTime date = {};
    ctl::ComPtr<IInspectable> spInspectable;
    ctl::ComPtr<wf::IReference<HSTRING>> spReference;

    // Get the calendar and clock identifier strings from the DP and use it to retrieve the cached
    // DateFormatter.
    IFC_RETURN(get_ClockIdentifier(strClockIdentifier.GetAddressOf()));

    // Get the DateTime that will be used to construct the string(s) to display.
    ctl::ComPtr<wf::IReference<wf::TimeSpan>> selectedTime;
    IFC_RETURN(get_SelectedTime(&selectedTime));

    if (selectedTime)
    {
        IFC_RETURN(selectedTime->get_Value(&timeSpan));
    }

    IFC_RETURN(GetDateTimeFromTimeSpan(timeSpan, &date));

    // For Blue apps (or a TimePicker template based on what was shipped in Blue), we only have the FlyoutButton.
    // Set the Content of the FlyoutButton to the formatted time.
    if (m_tpFlyoutButton.Get() && !m_tpHourTextBlock && !m_tpMinuteTextBlock && !m_tpPeriodTextBlock)
    {
        IFC_RETURN(GetTimeFormatter(strClockIdentifier.Get(), spDateFormatter.GetAddressOf()));
        IFC_RETURN(spDateFormatter->Format(date, strFormattedDate.GetAddressOf()));
        IFC_RETURN(PropertyValue::CreateFromString(strFormattedDate, &spInspectable));
        IFC_RETURN(spInspectable.As(&spReference));

        IFC_RETURN(m_tpFlyoutButton.Cast<Button>()->put_Content(spReference.Get()));
    }
    // For the Threshold template we set the Hour, Minute and Period strings on separate TextBlocks:
    if (m_tpHourTextBlock.Get())
    {
        if (selectedTime)
        {
            IFC_RETURN(CreateNewFormatterWithClock(wrl_wrappers::HStringReference(s_strHourFormat).Get(),
                strClockIdentifier.Get(), &spDateFormatter));
            IFC_RETURN(spDateFormatter->Format(date, strFormattedDate.GetAddressOf()));

            IFC_RETURN(m_tpHourTextBlock->put_Text(strFormattedDate));
        }
        else
        {
            wrl_wrappers::HString placeholderText;
            IFC_RETURN(DXamlCore::GetCurrent()->GetLocalizedResourceString(
                TEXT_TIMEPICKER_HOUR_PLACEHOLDER,
                placeholderText.ReleaseAndGetAddressOf()));

            IFC_RETURN(m_tpHourTextBlock->put_Text(placeholderText));
        }
    }
    if (m_tpMinuteTextBlock.Get())
    {
        if (selectedTime)
        {
            IFC_RETURN(CreateNewFormatterWithClock(wrl_wrappers::HStringReference(s_strMinuteFormat).Get(),
                strClockIdentifier.Get(), &spDateFormatter));
            IFC_RETURN(spDateFormatter->Format(date, strFormattedDate.GetAddressOf()));

            IFC_RETURN(m_tpMinuteTextBlock->put_Text(strFormattedDate));
        }
        else
        {
            wrl_wrappers::HString placeholderText;
            IFC_RETURN(DXamlCore::GetCurrent()->GetLocalizedResourceString(
                TEXT_TIMEPICKER_MINUTE_PLACEHOLDER,
                placeholderText.ReleaseAndGetAddressOf()));

            IFC_RETURN(m_tpMinuteTextBlock->put_Text(placeholderText));
        }
    }
    if (m_tpPeriodTextBlock.Get())
    {
        if (selectedTime)
        {
            IFC_RETURN(CreateNewFormatterWithClock(wrl_wrappers::HStringReference(s_strPeriodFormat).Get(),
                strClockIdentifier.Get(), &spDateFormatter));
            IFC_RETURN(spDateFormatter->Format(date, strFormattedDate.GetAddressOf()));

            IFC_RETURN(m_tpPeriodTextBlock->put_Text(strFormattedDate));
        }
        else
        {
            wrl_wrappers::HString placeholderText;
            IFC_RETURN(DXamlCore::GetCurrent()->GetLocalizedResourceString(
                TEXT_TIMEPICKER_PERIOD_PLACEHOLDER,
                placeholderText.ReleaseAndGetAddressOf()));

            IFC_RETURN(m_tpPeriodTextBlock->put_Text(placeholderText));
        }
    }
    IFC_RETURN(RefreshFlyoutButtonAutomationName());

    return S_OK;
}

// Creates a new wg::Calendar, taking into account the ClockIdentifier
_Check_return_
HRESULT
TimePicker::CreateNewCalendar(
    _In_ HSTRING strClockIdentifier,
    _Outptr_ wg::ICalendar** ppCalendar)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wg::ICalendarFactory> spCalendarFactory;
    ctl::ComPtr<wg::ICalendar> spCalendar;
    ctl::ComPtr<wfc::__FIVectorView_1_HSTRING_t> spLanguages;

    *ppCalendar = nullptr;

    IFC(wf::ActivateInstance(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"Windows.Globalization.Calendar")).Get(), spCalendar.ReleaseAndGetAddressOf()));
    IFC(spCalendar->get_Languages(&spLanguages));

    // Create the calendar
    IFC(ctl::GetActivationFactory(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"Windows.Globalization.Calendar")).Get(), &spCalendarFactory));
    IFC(spCalendarFactory->CreateCalendar(
            spLanguages.AsOrNull<wfc::__FIIterable_1_HSTRING_t>().Get(), /* Languages*/
            wrl_wrappers::HStringReference(STR_LEN_PAIR(L"GregorianCalendar")).Get(), /* Calendar */
            strClockIdentifier, /* Clock */
            &spCalendar));

    *ppCalendar = spCalendar.Detach();

Cleanup:
    RRETURN(hr);
}

// Creates a new DateTimeFormatter with the given format and clock identifier.
_Check_return_
HRESULT
TimePicker::CreateNewFormatterWithClock(
    _In_ HSTRING strFormat,
    _In_ HSTRING strClockIdentifier,
    _Outptr_ wg::DateTimeFormatting::IDateTimeFormatter** ppDateTimeFormatter)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatterFactory> spFormatterFactory;
    ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatter;
    ctl::ComPtr<wfc::__FIVectorView_1_HSTRING_t> spLanguages;
    wrl_wrappers::HString strGeographicRegion;
    wrl_wrappers::HString strCalendarSystem;

    *ppDateTimeFormatter = nullptr;

    IFC(ctl::GetActivationFactory(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"Windows.Globalization.DateTimeFormatting.DateTimeFormatter")).Get(), &spFormatterFactory));
    IFCPTR(spFormatterFactory.Get());

    IFC(spFormatterFactory->CreateDateTimeFormatter(strFormat, &spFormatter));

    IFC(spFormatter->get_GeographicRegion(strGeographicRegion.GetAddressOf()));
    IFC(spFormatter->get_Languages(&spLanguages));
    IFC(spFormatter->get_Calendar(strCalendarSystem.GetAddressOf()));

    IFC(spFormatterFactory->CreateDateTimeFormatterContext(
            strFormat,/* Format string */
            spLanguages.AsOrNull<wfc::__FIIterable_1_HSTRING_t>().Get(), /* Languages*/
            strGeographicRegion.Get(), /* Geographic region */
            strCalendarSystem.Get(), /* Calendar */
            strClockIdentifier, /* Clock */
            &spFormatter));

    *ppDateTimeFormatter = spFormatter.Detach();

Cleanup:
    RRETURN(hr);
}

// Creates a new DateTimeFormatter with the given format and calendar identifier.
_Check_return_
HRESULT
TimePicker::CreateNewFormatterWithCalendar(
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

    IFC(spFormatterFactory->CreateDateTimeFormatter(strFormat, &spFormatter));

    IFC(spFormatter->get_GeographicRegion(strGeographicRegion.GetAddressOf()));
    IFC(spFormatter->get_Languages(&spLanguages));
    IFC(spFormatter->get_Clock(strClock.GetAddressOf()));

    IFC(spFormatterFactory->CreateDateTimeFormatterContext(
            strFormat,/* Format string */
            spLanguages.AsOrNull<wfc::__FIIterable_1_HSTRING_t>().Get(), /* Languages*/
            strGeographicRegion.Get(), /* Geographic region */
            strCalendarIdentifier, /* Calendar */
            strClock.Get(), /* Clock */
            &spFormatter));

    *ppDateTimeFormatter = spFormatter.Detach();

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT TimePicker::GetTimeFormatter(
    _In_ HSTRING strClockIdentifier,
    _Outptr_ wg::DateTimeFormatting::IDateTimeFormatter ** ppDateFormatter)
{
    return CreateNewFormatterWithClock(
                wrl_wrappers::HStringReference(STR_LEN_PAIR(L"shorttime")).Get(),
                strClockIdentifier,
                ppDateFormatter);
}

// Sets our sentinel date to the given calendar. This date is 21st of July 2011 midnight.
// On this day there are no known daylight saving transitions.
_Check_return_
HRESULT
TimePicker::SetSentinelDate(
    _In_ wg::ICalendar* pCalendar)
{
    HRESULT hr = S_OK;

    IFC(pCalendar->put_Year(TIMEPICKER_SENTINELDATE_YEAR));
    IFC(pCalendar->put_Month(TIMEPICKER_SENTINELDATE_MONTH));
    IFC(pCalendar->put_Day(TIMEPICKER_SENTINELDATE_DAY));

    if (m_is12HourClock)
    {
        INT32 firstPeriodInThisDay = 0;

        IFC(pCalendar->get_FirstPeriodInThisDay(&firstPeriodInThisDay));
        IFC(pCalendar->put_Period(firstPeriodInThisDay));
        IFC(pCalendar->put_Hour(TIMEPICKER_SENTINELDATE_HOUR12));
    }
    else
    {
        IFC(pCalendar->put_Hour(TIMEPICKER_SENTINELDATE_HOUR24));
    }
    IFC(pCalendar->put_Minute(TIMEPICKER_SENTINELDATE_TIMEFIELDS));
    IFC(pCalendar->put_Second(TIMEPICKER_SENTINELDATE_TIMEFIELDS));
    IFC(pCalendar->put_Nanosecond(TIMEPICKER_SENTINELDATE_TIMEFIELDS));

Cleanup:
    RRETURN(hr);
}

// Generate the collection that we will populate our hour picker with.
_Check_return_
HRESULT
TimePicker::GenerateHours()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatter;
    wrl_wrappers::HString strClockIdentifier;
    wrl_wrappers::HString strHour;
    wf::DateTime dateTime = {};
    ctl::ComPtr<IInspectable> spInspectable;
    INT32 firstHourInThisPeriod = 0;
    INT32 numberOfHours = 0;

    IFC(get_ClockIdentifier(strClockIdentifier.GetAddressOf()));
    IFC(CreateNewFormatterWithClock(wrl_wrappers::HStringReference(s_strHourFormat).Get(),
                            strClockIdentifier.Get(), &spFormatter));

    IFC(SetSentinelDate(m_tpCalendar.Get()));
    IFC(m_tpCalendar->get_NumberOfHoursInThisPeriod(&numberOfHours));
    IFC(m_tpCalendar->get_FirstHourInThisPeriod(&firstHourInThisPeriod));
    IFC(m_tpCalendar->put_Hour(firstHourInThisPeriod));

    IFC(m_tpHourSource->Clear());

    for (INT32 hourOffset = 0; hourOffset < numberOfHours; hourOffset++)
    {
        IFC(m_tpCalendar->GetDateTime(&dateTime));
        IFC(spFormatter->Format(dateTime, strHour.ReleaseAndGetAddressOf()));

        IFC(DirectUI::PropertyValue::CreateFromString(strHour.Get(), &spInspectable));
        IFC(m_tpHourSource->Append(spInspectable.Get()));

        IFC(m_tpCalendar->AddHours(1));
    }

Cleanup:
    RRETURN(hr);
}

// Generate the collection that we will populate our minute picker with.
_Check_return_
HRESULT
TimePicker::GenerateMinutes()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatter;
    wrl_wrappers::HString strClockIdentifier;
    wrl_wrappers::HString strMinute;
    wf::DateTime dateTime = {};
    ctl::ComPtr<IInspectable> spInspectable;
    INT32 minuteIncrement = 0;
    INT32 lastMinute = 0;
    INT32 firstMinuteInThisHour = 0;

    IFC(get_ClockIdentifier(strClockIdentifier.GetAddressOf()));
    IFC(CreateNewFormatterWithClock(wrl_wrappers::HStringReference(s_strMinuteFormat).Get(),
                            strClockIdentifier.Get(), &spFormatter));
    IFC(SetSentinelDate(m_tpCalendar.Get()));
    IFC(GetAdjustedMinuteIncrement(&minuteIncrement));
    IFC(m_tpCalendar->get_LastMinuteInThisHour(&lastMinute));
    IFC(m_tpCalendar->get_FirstMinuteInThisHour(&firstMinuteInThisHour));

    IFC(m_tpMinuteSource->Clear())

    for (INT32 i = firstMinuteInThisHour; i <= lastMinute / minuteIncrement; i++)
    {
        IFC(m_tpCalendar->put_Minute(i * minuteIncrement));

        IFC(m_tpCalendar->GetDateTime(&dateTime));
        IFC(spFormatter->Format(dateTime, strMinute.ReleaseAndGetAddressOf()));

        IFC(DirectUI::PropertyValue::CreateFromString(strMinute.Get(), &spInspectable));
        IFC(m_tpMinuteSource->Append(spInspectable.Get()));
    }

Cleanup:
    RRETURN(hr);
}

// Generate the collection that we will populate our period picker with.
_Check_return_
HRESULT
TimePicker::GeneratePeriods()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatter;
    wrl_wrappers::HString strClockIdentifier;
    wrl_wrappers::HString strPeriod;
    ctl::ComPtr<IInspectable> spInspectable;
    INT32 firstPeriodInThisDay = 0;
    wf::DateTime dateTime = {};
    BOOLEAN twelveHourNotSupported = FALSE;

    IFC(get_ClockIdentifier(strClockIdentifier.GetAddressOf()));
    IFC(CreateNewFormatterWithClock(wrl_wrappers::HStringReference(s_strPeriodFormat).Get(),
                            strClockIdentifier.Get(), &spFormatter));
    IFC(SetSentinelDate(m_tpCalendar.Get()));

    IFC(m_tpPeriodSource->Clear());

    IFC(m_tpCalendar->get_FirstPeriodInThisDay(&firstPeriodInThisDay));
    IFC(m_tpCalendar->put_Period(firstPeriodInThisDay));
    IFC(m_tpCalendar->GetDateTime(&dateTime));
    IFC(spFormatter->Format(dateTime, strPeriod.ReleaseAndGetAddressOf()));

    if (strPeriod.Length() == 0)
    {
        // In some locales AM/PM symbols are not defined for periods. For those cases, Globalization will give us ""(empty string)
        // for AM and "." for PM. Empty string causes ContentPresenter to clear DataContext, this causes problems for us becasuse if
        // someone sets a DataContext to an ancestor of TimePicker, when the ContentPresenter on the ComboBox's nameplate gets its
        // Content set to empty string, it will display an unrelated string using the DataContext.
        twelveHourNotSupported = TRUE;
        IFC(strPeriod.Concat(wrl_wrappers::HStringReference(STR_LEN_PAIR(L".")), strPeriod));
    }

    IFC(DirectUI::PropertyValue::CreateFromString(strPeriod.Get(), &spInspectable));
    IFC(m_tpPeriodSource->Append(spInspectable.Get()));

    IFC(m_tpCalendar->AddPeriods(1));
    IFC(m_tpCalendar->GetDateTime(&dateTime));
    IFC(spFormatter->Format(dateTime, strPeriod.ReleaseAndGetAddressOf()));

    if (twelveHourNotSupported)
    {
        IFC(strPeriod.Concat(wrl_wrappers::HStringReference(STR_LEN_PAIR(L".")), strPeriod));
    }

    IFC(DirectUI::PropertyValue::CreateFromString(strPeriod.Get(), &spInspectable));
    IFC(m_tpPeriodSource->Append(spInspectable.Get()));

Cleanup:
    RRETURN(hr);
}

// Clears the ItemsSource  properties of the selectors.
HRESULT
TimePicker::ClearSelectors()
{
    HRESULT hr = S_OK;

    //Clear Selector ItemSources
    const CDependencyProperty* pItemsSourceProperty = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ItemsControl_ItemsSource);

    if (m_tpHourPicker.Get())
    {
        IFC(m_tpHourPicker.Cast<Selector>()->ClearValue(pItemsSourceProperty));
    }

    if (m_tpMinutePicker.Get())
    {
        IFC(m_tpMinutePicker.Cast<Selector>()->ClearValue(pItemsSourceProperty));
    }

    if (m_tpPeriodPicker.Get())
    {
        IFC(m_tpPeriodPicker.Cast<Selector>()->ClearValue(pItemsSourceProperty));
    }

Cleanup:
    RRETURN(hr);
}

// Gets the layout ordering of the selectors.
_Check_return_
HRESULT
TimePicker::GetOrder(
    _Out_ INT32* hourOrder,
    _Out_ INT32* minuteOrder,
    _Out_ INT32* periodOrder,
    _Out_ BOOLEAN* isRTL)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatterWithClock;
    ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatterWithCalendar;
    ctl::ComPtr<__FIVectorView_1_HSTRING> spPatterns;
    wrl_wrappers::HString strPattern;
    wrl_wrappers::HString strClockIdentifier;
    UINT32 length = 0;
    LPCWSTR szPattern;

    IFC(CreateNewFormatterWithCalendar(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"month.full")).Get(),
        wrl_wrappers::HStringReference(STR_LEN_PAIR(L"GregorianCalendar")).Get(), &spFormatterWithCalendar));
    IFC(spFormatterWithCalendar->get_Patterns(&spPatterns));
    IFC(spPatterns->GetAt(0, strPattern.ReleaseAndGetAddressOf()));

    szPattern = strPattern.GetRawBuffer(&length);

    *isRTL = szPattern[0] == TIMEPICKER_RTL_CHARACTER_CODE;

    IFC(get_ClockIdentifier(strClockIdentifier.GetAddressOf()));
    IFC(CreateNewFormatterWithClock(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"hour minute")).Get(), strClockIdentifier.Get(), &spFormatterWithClock));
    IFC(spFormatterWithClock->get_Patterns(&spPatterns));
    IFC(spPatterns->GetAt(0, strPattern.ReleaseAndGetAddressOf()));

    if (strPattern.Get())
    {
        LPCWSTR hourOccurence = nullptr;
        LPCWSTR minuteOccurence = nullptr;
        LPCWSTR periodOccurence = nullptr;

        szPattern = strPattern.GetRawBuffer(&length);

        // We do string search to determine the order of the fields.
        hourOccurence = xstrstr(szPattern, L"{hour");
        minuteOccurence = xstrstr(szPattern, L"{minute");
        periodOccurence = xstrstr(szPattern, L"{period");

        if (hourOccurence < minuteOccurence)
        {
            if (hourOccurence < periodOccurence)
            {
                *hourOrder = 0;
                if (minuteOccurence < periodOccurence)
                {
                    *minuteOrder = 1;
                    *periodOrder = 2;
                }
                else
                {
                    *minuteOrder = 2;
                    *periodOrder = 1;
                }
            }
            else
            {
                *hourOrder = 1;
                *minuteOrder = 2;
                *periodOrder = 0;
            }
        }
        else
        {
            if (hourOccurence < periodOccurence)
            {
                *hourOrder = 1;
                *minuteOrder = 0;
                *periodOrder = 2;
            }
            else
            {
                *hourOrder = 2;
                if (minuteOccurence < periodOccurence)
                {
                    *minuteOrder = 0;
                    *periodOrder = 1;
                }
                else
                {

                    *minuteOrder = 1;
                    *periodOrder = 0;
                }
            }
        }

        // Thi is a trick we are mimicking from from our js counterpart. In rtl languages if we just naively lay out our pickers right-to-left
        // it will not be correct. Say our LTR layout is HH MM PP, when we just lay it out RTL it will be PP MM HH, however we actually want
        // our pickers to be laid out as PP HH MM as hour and minute fields are english numerals and they should be laid out LTR. So we are
        // preempting our lay out mechanism and swapping hour and minute pickers, thus the final lay out will be correct.
        if (*isRTL)
        {
            std::swap(*hourOrder, *minuteOrder);
        }
    }

Cleanup:
    RRETURN(hr);
}

// Updates the order of selectors in our layout. Also takes care of hiding/showing the selectors and related spacing depending our
// public properties set by the user.
_Check_return_
HRESULT
TimePicker::UpdateOrderAndLayout()
{
    HRESULT hr = S_OK;
    INT32 hourOrder = 0;
    INT32 minuteOrder = 0;
    INT32 periodOrder = 0;
    BOOLEAN firstHostPopulated = FALSE;
    BOOLEAN secondHostPopulated = FALSE;
    BOOLEAN thirdHostPopulated = FALSE;
    BOOLEAN isRTL = FALSE;
    xaml::GridLength starGridLength = {};
    xaml::GridLength zeroGridLength = {};
    ctl::ComPtr<IUIElement> spHourElement;
    ctl::ComPtr<IUIElement> spMinuteElement;
    ctl::ComPtr<IUIElement> spPeriodElement;

    zeroGridLength.GridUnitType = xaml::GridUnitType_Pixel;
    zeroGridLength.Value = 0.0;
    starGridLength.GridUnitType = xaml::GridUnitType_Star;
    starGridLength.Value = 1.0;

    IFC(GetOrder(&hourOrder, &minuteOrder, &periodOrder, &isRTL));

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

    spHourElement = m_tpHourPicker.Get() ? m_tpHourPicker.AsOrNull<IUIElement>().Get() : m_tpHourTextBlock.AsOrNull<IUIElement>().Get();
    spMinuteElement = m_tpMinutePicker.Get() ? m_tpMinutePicker.AsOrNull<IUIElement>().Get() : m_tpMinuteTextBlock.AsOrNull<IUIElement>().Get();
    spPeriodElement = m_tpPeriodPicker.Get() ? m_tpPeriodPicker.AsOrNull<IUIElement>().Get() : m_tpPeriodTextBlock.AsOrNull<IUIElement>().Get();

    // Assign the selectors to the hosts.
    switch (hourOrder)
    {
        case 0:
            if (m_tpFirstPickerHost.Get() && spHourElement.Get())
            {
                IFC(m_tpFirstPickerHost->put_Child(spHourElement.Get()));
                firstHostPopulated = TRUE;
            }
            break;
        case 1:
            if (m_tpSecondPickerHost.Get() && spHourElement.Get())
            {
                IFC(m_tpSecondPickerHost->put_Child(spHourElement.Get()));
                secondHostPopulated = TRUE;
            }
            break;
        case 2:
            if (m_tpThirdPickerHost.Get() && spHourElement.Get())
            {
                IFC(m_tpThirdPickerHost->put_Child(spHourElement.Get()));
                thirdHostPopulated = TRUE;
            }
            break;
    }

    switch (minuteOrder)
    {
        case 0:
            if (m_tpFirstPickerHost.Get() && spMinuteElement.Get())
            {
                IFC(m_tpFirstPickerHost->put_Child(spMinuteElement.Get()));
                firstHostPopulated = TRUE;
            }
            break;
        case 1:
            if (m_tpSecondPickerHost.Get() && spMinuteElement.Get())
            {
                IFC(m_tpSecondPickerHost->put_Child(spMinuteElement.Get()));
                secondHostPopulated = TRUE;
            }
            break;
        case 2:
            if (m_tpThirdPickerHost.Get() && spMinuteElement.Get())
            {
                IFC(m_tpThirdPickerHost->put_Child(spMinuteElement.Get()));
                thirdHostPopulated = TRUE;
            }
            break;
    }

    switch (periodOrder)
    {
        case 0:
            if (m_tpFirstPickerHost.Get() && spPeriodElement.Get() && m_is12HourClock)
            {
                IFC(m_tpFirstPickerHost->put_Child(spPeriodElement.Get()));
                firstHostPopulated = TRUE;
            }
            break;
        case 1:
            if (m_tpSecondPickerHost.Get() && spPeriodElement.Get() && m_is12HourClock)
            {
                IFC(m_tpSecondPickerHost->put_Child(spPeriodElement.Get()));
                secondHostPopulated = TRUE;
            }
            break;
        case 2:
            if (m_tpThirdPickerHost.Get() && spPeriodElement.Get() && m_is12HourClock)
            {
                IFC(m_tpThirdPickerHost->put_Child(spPeriodElement.Get()));
                thirdHostPopulated = TRUE;
            }
            break;
    }

    //Show the columns that are in use. Hide the columns that are not in use.
    if (m_tpFirstTextBlockColumn.Get())
    {
        IFC(m_tpFirstTextBlockColumn->put_Width(firstHostPopulated ? starGridLength : zeroGridLength));
    }
    if (m_tpSecondTextBlockColumn.Get())
    {
        IFC(m_tpSecondTextBlockColumn->put_Width(secondHostPopulated ? starGridLength : zeroGridLength));
    }
    if (m_tpThirdTextBlockColumn.Get())
    {
        IFC(m_tpThirdTextBlockColumn->put_Width(thirdHostPopulated ? starGridLength : zeroGridLength));
    }

    if (m_tpFirstPickerHost.Get())
    {
        IFC(m_tpFirstPickerHost.Cast<Border>()->put_Visibility(firstHostPopulated ? xaml::Visibility_Visible
            : xaml::Visibility_Collapsed));
    }
    if (m_tpSecondPickerHost.Get())
    {
        IFC(m_tpSecondPickerHost.Cast<Border>()->put_Visibility(secondHostPopulated ? xaml::Visibility_Visible
            : xaml::Visibility_Collapsed));
    }
    if (m_tpThirdPickerHost.Get())
    {
        IFC(m_tpThirdPickerHost.Cast<Border>()->put_Visibility(thirdHostPopulated ? xaml::Visibility_Visible
            : xaml::Visibility_Collapsed));
    }

    // Determine if we will show the dividers and assign visibilities to them. We will determine if the dividers
    // are shown by looking at which borders are populated.
    if (m_tpFirstColumnDivider.Get())
    {
        IFC(m_tpFirstColumnDivider.Cast<Border>()->put_Visibility(
            firstHostPopulated && (secondHostPopulated || thirdHostPopulated) ?
            xaml::Visibility_Visible : xaml::Visibility_Collapsed));
    }
    if (m_tpSecondColumnDivider.Get())
    {
        IFC(m_tpSecondColumnDivider.Cast<Border>()->put_Visibility(
            secondHostPopulated && thirdHostPopulated ?
            xaml::Visibility_Visible : xaml::Visibility_Collapsed));
    }

Cleanup:
    RRETURN(hr);
}

// Updates the selector selected indices to display our Time property.
_Check_return_
HRESULT
TimePicker::UpdateDisplay()
{
    HRESULT hr = S_OK;
    wf::DateTime dateTime = {};
    wf::TimeSpan timeSpan = {};
    INT32 hour = 0;
    INT32 minuteIncrement = 0;
    INT32 minute;
    INT32 period = 0;
    INT32 firstPeriodInThisDay = 0;
    INT32 firstMinuteInThisHour = 0;
    INT32 firstHourInThisPeriod = 0;

    PreventReactionToSelectionChange();

    IFC(GetSelectedTime(&timeSpan));
    IFC(GetDateTimeFromTimeSpan(timeSpan, &dateTime));
    IFC(m_tpCalendar->SetDateTime(dateTime));

    // Calculate the period index and set it
    if (m_is12HourClock)
    {
        IFC(m_tpCalendar->get_Period(&period));
        IFC(m_tpCalendar->get_FirstPeriodInThisDay(&firstPeriodInThisDay));
        if (m_tpPeriodPicker.Get())
        {
            IFC(m_tpPeriodPicker->put_SelectedIndex(period - firstPeriodInThisDay));
        }
    }

    // Calculate the hour index and set it
    IFC(m_tpCalendar->get_Hour(&hour));
    IFC(m_tpCalendar->get_FirstHourInThisPeriod(&firstHourInThisPeriod));
    if (m_is12HourClock)
    {
        // For 12 hour clock 12 am and 12 pm are always the first element (index 0) in hour picker.
        // Other hours translate directly to indices. So it is sufficient to make a mod operation while translating
        // hour to index.
        if (m_tpHourPicker.Get())
        {
            IFC(m_tpHourPicker->put_SelectedIndex(hour % TIMEPICKER_COERCION_OFFSET));
        }
    }
    else
    {
        // For 24 hour clock, Hour translates exactly to the hour picker's selected index.
        if (m_tpHourPicker.Get())
        {
            IFC(m_tpHourPicker->put_SelectedIndex(hour));
        }
    }

    // Calculate the minute index and set it
    IFC(GetAdjustedMinuteIncrement(&minuteIncrement));
    IFC(m_tpCalendar->get_Minute(&minute));
    IFC(m_tpCalendar->get_FirstMinuteInThisHour(&firstMinuteInThisHour));
    if (m_tpMinutePicker)
    {
        IFC(m_tpMinutePicker->put_SelectedIndex((minute / minuteIncrement) - firstMinuteInThisHour));
    }

    if (m_tpFlyoutButton)
    {
        IFC(UpdateFlyoutButtonContent());
    }

Cleanup:
    AllowReactionToSelectionChange();
    RRETURN(hr);
}

// Clears everything, generates and sets the itemssources to selectors.
_Check_return_
HRESULT
TimePicker::RefreshSetup()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wg::ICalendar> spCalendar;
    wrl_wrappers::HString strClockIdentifier;

    PreventReactionToSelectionChange();

    IFC(get_ClockIdentifier(strClockIdentifier.GetAddressOf()));

    m_is12HourClock = strClockIdentifier == wrl_wrappers::HStringReference(s_strTwelveHourClock);

    IFC(CreateNewCalendar(strClockIdentifier.Get(), &spCalendar));
    IFC(SetSentinelDate(spCalendar.Get()));

    // Clock identifier change may have rendered m_tpCalendar stale.
    IFC(get_ClockIdentifier(strClockIdentifier.GetAddressOf()));
    IFC(CreateNewCalendar(strClockIdentifier.Get(), &spCalendar));
    m_tpCalendar.Clear();
    SetPtrValue(m_tpCalendar, spCalendar.Get());

    IFC(ClearSelectors());
    IFC(UpdateOrderAndLayout());

    if (m_tpHourPicker.Get())
    {
        IFC(GenerateHours());
        IFC(m_tpHourPicker.Cast<Selector>()->put_ItemsSource(ctl::iinspectable_cast(m_tpHourSource.Get())));
    }

    if (m_tpMinutePicker.Get())
    {
        IFC(GenerateMinutes());
        IFC(m_tpMinutePicker.Cast<Selector>()->put_ItemsSource(ctl::iinspectable_cast(m_tpMinuteSource.Get())));
    }

    if (m_tpPeriodPicker.Get())
    {
        IFC(GeneratePeriods());
        IFC(m_tpPeriodPicker.Cast<Selector>()->put_ItemsSource(ctl::iinspectable_cast(m_tpPeriodSource.Get())))
    }

    IFC(UpdateDisplay());
    IFC(UpdateTime());

Cleanup:
    AllowReactionToSelectionChange();
    RRETURN(hr);
}

// Translates ONLY the hour and minute fields of DateTime into TimeSpan.
_Check_return_
HRESULT
TimePicker::GetTimeSpanFromDateTime(
    _In_ wf::DateTime dateTime,
    _Out_ wf::TimeSpan* pTimeSpan)
{
    HRESULT hr = S_OK;
    wf::TimeSpan timeSpan = {};
    INT32 hour = 0;
    INT32 minute = 0;

    ASSERT(m_tpCalendar.Get() != nullptr);
    IFC(m_tpCalendar->SetDateTime(dateTime));

    IFC(m_tpCalendar->get_Minute(&minute));
    timeSpan.Duration += minute * s_timeSpanTicksPerMinute;

    IFC(m_tpCalendar->get_Hour(&hour));
    if (m_is12HourClock)
    {
        INT32 period = 0;
        INT32 firstPeriodInThisDay = 0;

        IFC(m_tpCalendar->get_Period(&period));
        IFC(m_tpCalendar->get_FirstPeriodInThisDay(&firstPeriodInThisDay));

        if (period == firstPeriodInThisDay)
        {
            if (hour == TIMEPICKER_COERCION_OFFSET)
            {
                hour = 0;
            }
        }
        else
        {
            if (hour != TIMEPICKER_COERCION_OFFSET)
            {
                hour += TIMEPICKER_COERCION_OFFSET;
            }
        }
    }
    timeSpan.Duration += hour * s_timeSpanTicksPerHour;

    *pTimeSpan = timeSpan;

Cleanup:
    RRETURN(hr);
}

// Translates a timespan to datetime. Note that, unrelated fields of datetime (year, day etc.)
// are set to our sentinel values.
_Check_return_
HRESULT
TimePicker::GetDateTimeFromTimeSpan(
    _In_ wf::TimeSpan timeSpan,
    _Out_ wf::DateTime* pDateTime)
{
    HRESULT hr = S_OK;
    wf::DateTime dateTime = {};

    ASSERT(m_tpCalendar.Get() != nullptr);
    IFC(SetSentinelDate(m_tpCalendar.Get()));
    IFC(m_tpCalendar->GetDateTime(&dateTime));

    dateTime.UniversalTime += timeSpan.Duration;

    *pDateTime = dateTime;

Cleanup:
    RRETURN(hr);
}

// Gets the minute increment and if it is 0, adjusts it to 60 so we will handle the 0
// case correctly.
_Check_return_
HRESULT
TimePicker::GetAdjustedMinuteIncrement(
    _Out_ INT32* minuteIncrement)

{
    HRESULT hr = S_OK;

    IFC(get_MinuteIncrement(minuteIncrement));
    if (*minuteIncrement == 0)
    {
        *minuteIncrement = TIMEPICKER_MINUTEINCREMENT_ZERO_REPLACEMENT;
    }

Cleanup:
    RRETURN(hr);
}

// Create TimePickerAutomationPeer to represent the TimePicker.
IFACEMETHODIMP TimePicker::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::ITimePickerAutomationPeer> spTimePickerAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::ITimePickerAutomationPeerFactory> spTimePickerAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;

    IFCPTR(ppAutomationPeer);
    *ppAutomationPeer = NULL;

    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::TimePickerAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spTimePickerAPFactory));

    IFC(spTimePickerAPFactory.Cast<TimePickerAutomationPeerFactory>()->CreateInstanceWithOwner(this,
        NULL,
        &spInner,
        &spTimePickerAutomationPeer));
    IFC(spTimePickerAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT TimePicker::GetSelectedTimeAsString(_Out_ HSTRING* strPlainText)
{
    wrl_wrappers::HString strData;
    wrl_wrappers::HString strClockIdentifier;
    ctl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spTimeFormatter;
    wf::DateTime date = {};
    wf::TimeSpan currentTime = {};

    IFC_RETURN(GetSelectedTime(&currentTime));
    IFC_RETURN(GetDateTimeFromTimeSpan(currentTime, &date));

    if (currentTime.Duration != 0)
    {
        IFC_RETURN(get_ClockIdentifier(strClockIdentifier.GetAddressOf()));
        IFC_RETURN(GetTimeFormatter(strClockIdentifier.Get(), spTimeFormatter.GetAddressOf()));
        IFC_RETURN(spTimeFormatter->Format(date, strData.GetAddressOf()));
        IFC_RETURN(strData.CopyTo(strPlainText));
    }
    return S_OK;
}

_Check_return_ HRESULT TimePicker::RefreshFlyoutButtonAutomationName()
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
        IFC_RETURN(GetSelectedTimeAsString(strSelectedValue.GetAddressOf()));
        LPCWSTR pszSelectedValue = strSelectedValue.GetRawBuffer(NULL);

        wrl_wrappers::HString strMsgFormat;
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_NAME_TIMEPICKER, strMsgFormat.GetAddressOf()));
        LPCWSTR pszMsgFormat = strMsgFormat.GetRawBuffer(NULL);

        WCHAR szBuffer[MAX_PATH] = {};
        int cchBuffer = 0;
        
        ctl::ComPtr<wf::IReference<wf::TimeSpan>> selectedTime;
        IFC_RETURN(get_SelectedTime(&selectedTime));

        if (selectedTime)
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

/* static */ wf::TimeSpan TimePicker::GetNullTimeSentinel()
{
    wf::TimeSpan nullTime = {};
    nullTime.Duration = GetNullTimeSentinelValue();
    return nullTime;
}

/* static */ long TimePicker::GetNullTimeSentinelValue()
{
    return -1;
}

_Check_return_ HRESULT TimePicker::GetCurrentTime(_Out_ wf::TimeSpan* currentTime)
{
    if (m_currentTime.Duration == GetNullTimeSentinelValue())
    {
        wf::DateTime dateTime = {};
        ctl::ComPtr<wg::ICalendar> calendar;

        IFC_RETURN(CreateNewCalendar(wrl_wrappers::HStringReference(s_strTwelveHourClock).Get(), &calendar));
        IFC_RETURN(calendar->SetToNow());
        IFC_RETURN(calendar->GetDateTime(&dateTime));
        IFC_RETURN(GetTimeSpanFromDateTime(dateTime, &m_currentTime));
    }

    *currentTime = m_currentTime;
    return S_OK;
}

_Check_return_ HRESULT TimePicker::GetSelectedTime(_Out_ wf::TimeSpan* time)
{
    ctl::ComPtr<wf::IReference<wf::TimeSpan>> selectedTime;
    IFC_RETURN(get_SelectedTime(&selectedTime));

    if (selectedTime)
    {
        IFC_RETURN(selectedTime->get_Value(time));
    }

    return S_OK;
}