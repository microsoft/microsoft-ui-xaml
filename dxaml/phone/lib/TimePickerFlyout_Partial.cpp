// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <RuntimeProfiler.h>

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{
const WCHAR* TimePickerFlyout::s_strHourFormat = L"{hour.integer(1)}";
const INT64 TimePickerFlyout::s_timeSpanTicksPerMinute = 600000000;
const INT64 TimePickerFlyout::s_timeSpanTicksPerHour = 36000000000;
const INT64 TimePickerFlyout::s_timeSpanTicksPerDay = 864000000000;

TimePickerFlyout::TimePickerFlyout() :
    _asyncOperationManager(FlyoutAsyncOperationManager<wf::IReference<wf::TimeSpan>*, TimePickerFlyout, TimePickerFlyoutShowAtAsyncOperationName>(Private::ReferenceTrackerHelper<TimePickerFlyout>(this)))
{
    __RP_Marker_ClassByName("TimePickerFlyout");
}

TimePickerFlyout::~TimePickerFlyout()
{
    ASYNCTRACE(L"TimePickerFlyout deconstructor called.");
}

_Check_return_ HRESULT
TimePickerFlyout::InitializeImpl()
{
    HRESULT hr = S_OK;
    EventRegistrationToken openingToken = { };
    EventRegistrationToken openedToken = {};
    EventRegistrationToken closedToken = {};
    wrl::ComPtr<xaml_primitives::IPickerFlyoutBaseFactory> spInnerFactory;
    wrl::ComPtr<xaml_primitives::IPickerFlyoutBase> spInnerInstance;
    wrl::ComPtr<IInspectable> spInnerInspectable;
    wrl::ComPtr<xaml_primitives::IFlyoutBase> spFlyoutBase;

    IFC(TimePickerFlyoutGenerated::InitializeImpl());

   IFC(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_Primitives_PickerFlyoutBase).Get(),
            &spInnerFactory));
   IFC(spInnerFactory->CreateInstance(
            static_cast<ITimePickerFlyout*>(this),
            &spInnerInspectable,
            &spInnerInstance));

   IFC(SetComposableBasePointers(
            spInnerInspectable.Get(),
            spInnerFactory.Get()));

    IFC(QueryInterface(
        __uuidof(xaml_primitives::IFlyoutBase),
        &spFlyoutBase));

    IFC(spFlyoutBase->add_Opening(
        wrl::Callback<wf::IEventHandler<IInspectable*>>
        (this, &TimePickerFlyout::OnOpening).Get(),
        &openingToken));

    IFC(spFlyoutBase->add_Opened(
        wrl::Callback<wf::IEventHandler<IInspectable*>>
        (this, &TimePickerFlyout::OnOpened).Get(),
        &openedToken));

    IFC(spFlyoutBase->add_Closed(
        wrl::Callback<wf::IEventHandler<IInspectable*>>
        (this, &TimePickerFlyout::OnClosed).Get(),
        &closedToken));

    IFC(_asyncOperationManager.Initialize(
        spFlyoutBase.Get(),
        // Cancellation value provider function
        [] () -> wf::IReference<wf::TimeSpan>*
        {
            return nullptr;
        }));

    {
        wrl::ComPtr<xaml_primitives::IFlyoutBasePrivate> spFlyoutBasePrivate;
        IFC(GetComposableBase().As(&spFlyoutBasePrivate));
        IFC(spFlyoutBasePrivate->put_UsePickerFlyoutTheme(TRUE));

        IFC(spFlyoutBase->put_Placement(xaml_primitives::FlyoutPlacementMode_Right));
    }

Cleanup:
    RRETURN(hr);
}

// -----
// IPickerFlyoutBaseOverrides Impl
// -----

_Check_return_ HRESULT
TimePickerFlyout::ShouldShowConfirmationButtonsImpl(_Out_ BOOLEAN* result)
{
    *result = FALSE;

    RRETURN(S_OK);
}

_Check_return_ HRESULT
TimePickerFlyout::OnConfirmedImpl()
{
    HRESULT hr = S_OK;
    wf::TimeSpan oldTimeSpan = {};
    wf::TimeSpan newTimeSpan = {};
    wrl::ComPtr<xaml_controls::TimePickedEventArgs> spArgs;
    wrl::ComPtr<IInspectable> spBoxedTimeSpan;
    wrl::ComPtr<wf::IReference<wf::TimeSpan>> spBoxedTsAsReference;

    IFC(get_Time(&oldTimeSpan));
    IFC(static_cast<TimePickerFlyoutPresenter*>(_tpPresenter.Get())->GetTime(&newTimeSpan));

    IFC(put_Time(newTimeSpan));

    IFC(Private::ValueBoxer::CreateTimeSpan(newTimeSpan, &spBoxedTimeSpan));
    IFC(spBoxedTimeSpan.As(&spBoxedTsAsReference));
    IFC(_asyncOperationManager.Complete(spBoxedTsAsReference.Get()));

    IFC(wrl::MakeAndInitialize<xaml_controls::TimePickedEventArgs>(&spArgs));
    IFC(spArgs->put_OldTime(oldTimeSpan));
    IFC(spArgs->put_NewTime(newTimeSpan));
    IFC(m_TimePickedEventSource.InvokeAll(this, spArgs.Get()));

    IFC(TimePickerFlyoutGenerated::OnConfirmedImpl());

Cleanup:
    RRETURN(hr);
}

// -----
// IFlyoutBaseOverrides Impl
// -----

_Check_return_ HRESULT
TimePickerFlyout::CreatePresenterImpl(_Outptr_ xaml_controls::IControl** returnValue)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<ITimePickerFlyoutPresenter> spFlyoutPresenter;

    IFC(wrl::MakeAndInitialize<TimePickerFlyoutPresenter>(&spFlyoutPresenter));
    IFC(SetPtrValue(_tpPresenter, spFlyoutPresenter.Get()));
    IFC(spFlyoutPresenter.CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

// -----
// TimePickerFlyout Impl
// -----

_Check_return_ HRESULT
TimePickerFlyout::ShowAtAsyncImpl(
    _In_ xaml::IFrameworkElement* pTarget,
    _Outptr_ wf::IAsyncOperation<wf::IReference<wf::TimeSpan>*>** ppAction)
{
    HRESULT hr = S_OK;

    IFC(SetPtrValue(_tpTarget, pTarget));
    IFC(_asyncOperationManager.Start(pTarget, ppAction));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
TimePickerFlyout::OnOpening(
    _In_ IInspectable* /* pSender */,
    _In_ IInspectable* /* pArgs */)
{
    wrl::ComPtr<xaml_input::IInputManagerStatics> inputManagerStatics;
    xaml_input::LastInputDeviceType lastInputDeviceType;

    ASSERT(_tpPresenter, "Expected non-null presenter");

    IFC_RETURN(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Input_InputManager).Get(),
        &inputManagerStatics));

    IFC_RETURN(inputManagerStatics->GetLastInputDeviceType(&lastInputDeviceType));

    IFC_RETURN(static_cast<TimePickerFlyoutPresenter*>(_tpPresenter.Get())->PullPropertiesFromOwner(this));
    IFC_RETURN(static_cast<TimePickerFlyoutPresenter*>(_tpPresenter.Get())->SetAcceptDismissButtonsVisibility(lastInputDeviceType != xaml_input::LastInputDeviceType_GamepadOrRemote));

    if (_tpTarget)
    {
        wrl::ComPtr<xaml::IFrameworkElement> spPresenterAsFE;
        wrl::ComPtr<xaml::IFrameworkElement> spTargetAsFE;
        DOUBLE actualWidth = 0;

        IFC_RETURN(_tpPresenter.As(&spPresenterAsFE));
        IFC_RETURN(_tpTarget.As(&spTargetAsFE));

        //The width of the flyout should equal that of the target element.
        IFC_RETURN(spTargetAsFE->get_ActualWidth(&actualWidth));
        IFC_RETURN(spPresenterAsFE->put_Width(actualWidth));
        //Also set MinWidth as FlyoutBase can change Width.
        IFC_RETURN(spPresenterAsFE->put_MinWidth(actualWidth));
    }

    return S_OK;
}

_Check_return_ HRESULT
TimePickerFlyout::OnOpened(
    _In_ IInspectable* /* pSender */,
    _In_ IInspectable* /* pArgs */)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml::IUIElement> spFlyoutPresenterAsUIE;
    wrl::ComPtr<xaml_controls::IControl> spFlyoutPresenterAsControl;
    wrl::ComPtr<xaml_controls::IControlProtected> spFlyoutPresenterAsControlProtected;
    wrl::ComPtr<xaml::IDependencyObject> spDismissButtonAsDO;
    wrl::ComPtr<xaml::IUIElement> spDismissButtonAsUIE;
    wrl::ComPtr<xaml_primitives::IButtonBase> spDismissButtonAsButtonBase;
    wrl::ComPtr<xaml::IDependencyObject> spAcceptButtonAsDO;
    wrl::ComPtr<xaml::IUIElement> spAcceptButtonAsUIE;
    wrl::ComPtr<xaml_primitives::IButtonBase> spAcceptButtonAsButtonBase;
    wrl::ComPtr<xaml_automation::IAutomationPropertiesStatics> spAutomationPropertiesStatics;
    wrl::ComPtr<IDependencyObject> spButtonAsDO;
    wrl_wrappers::HString strAutomationName;

    IFC(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Automation_AutomationProperties).Get(), &spAutomationPropertiesStatics));

    IFC(_tpPresenter.As(&spFlyoutPresenterAsUIE));
    IFC(_tpPresenter.As(&spFlyoutPresenterAsControl));
    IFC(_tpPresenter.As(&spFlyoutPresenterAsControlProtected));

    if (_tpTarget)
    {
        wf::Point point = { 0, 0 };
        wrl::ComPtr<xaml_primitives::IFlyoutBasePrivate> spFlyoutBase;

        IFC(DateTimePickerFlyoutHelper::CalculatePlacementPosition(_tpTarget.Get(), spFlyoutPresenterAsControl.Get(), &point));
        IFC(GetComposableBase().As(&spFlyoutBase));
        IFC(spFlyoutBase->PlaceFlyoutForDateTimePicker(point));
    }

    //Hook up OnAcceptClick and OnDismissClick event handlers:
    IFC(spFlyoutPresenterAsControlProtected->GetTemplateChild(
        wrl_wrappers::HStringReference(L"DismissButton").Get(),
        &spDismissButtonAsDO));
    if (spDismissButtonAsDO)
    {
        IFC(spDismissButtonAsDO.As(&spDismissButtonAsUIE));
        IGNOREHR(spDismissButtonAsDO.As(&spDismissButtonAsButtonBase));
        IFC(SetPtrValue(_tpDismissButton, spDismissButtonAsButtonBase.Get()));
    }

    IFC(spFlyoutPresenterAsControlProtected->GetTemplateChild(
        wrl_wrappers::HStringReference(L"AcceptButton").Get(),
        &spAcceptButtonAsDO));
    if (spAcceptButtonAsDO)
    {
        IFC(spAcceptButtonAsDO.As(&spAcceptButtonAsUIE));
        IGNOREHR(spAcceptButtonAsDO.As(&spAcceptButtonAsButtonBase));
        IFC(SetPtrValue(_tpAcceptButton, spAcceptButtonAsButtonBase.Get()));
    }

    if (_tpAcceptButton)
    {
        ASSERT(spAcceptButtonAsUIE);
        IFC(_tpAcceptButton->add_Click(
            wrl::Callback<xaml::IRoutedEventHandler>(this, &TimePickerFlyout::OnAcceptClick).Get(),
            &_acceptClickToken));

        IFC(Private::FindStringResource(UIA_DIALOG_ACCEPT, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(_tpAcceptButton.As(&spButtonAsDO));
        IFC(spAutomationPropertiesStatics->SetName(spButtonAsDO.Get(), strAutomationName.Get()));
    }

    if (_tpDismissButton)
    {
        ASSERT(spDismissButtonAsUIE);
        IFC(_tpDismissButton->add_Click(
            wrl::Callback<xaml::IRoutedEventHandler>(this, &TimePickerFlyout::OnDismissClick).Get(),
            &_dismissClickToken));

        IFC(Private::FindStringResource(UIA_DIALOG_DISMISS, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(_tpDismissButton.As(&spButtonAsDO));
        IFC(spAutomationPropertiesStatics->SetName(spButtonAsDO.Get(), strAutomationName.Get()));
    }

    ASSERT(spFlyoutPresenterAsUIE);
    IFC(spFlyoutPresenterAsUIE->add_KeyDown(
        wrl::Callback<xaml_input::IKeyEventHandler>(this, &TimePickerFlyout::OnKeyDown).Get(),
        &_keyDownToken));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT TimePickerFlyout::OnClosed(
    _In_ IInspectable* pSender,
    _In_ IInspectable* pArgs)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml::IUIElement> spFlyoutPresenterAsUIE;
    UNREFERENCED_PARAMETER(pSender);
    UNREFERENCED_PARAMETER(pArgs);

    IFC(_tpPresenter.As(&spFlyoutPresenterAsUIE));

    if (_tpAcceptButton)
    {
        IFC(_tpAcceptButton->remove_Click(_acceptClickToken));
        _tpAcceptButton.Clear();
    }

    if (_tpDismissButton)
    {
        IFC(_tpDismissButton->remove_Click(_dismissClickToken));
        _tpDismissButton.Clear();
    }

    if (spFlyoutPresenterAsUIE)
    {
        IFC(spFlyoutPresenterAsUIE->remove_KeyDown(_keyDownToken));
    }


Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT TimePickerFlyout::OnAcceptClick(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pArgs)
{
    UNREFERENCED_PARAMETER(pSender);
    UNREFERENCED_PARAMETER(pArgs);

    RRETURN(OnConfirmed());
}

_Check_return_ HRESULT TimePickerFlyout::OnDismissClick(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    UNREFERENCED_PARAMETER(pSender);
    UNREFERENCED_PARAMETER(pArgs);
    wrl::ComPtr<xaml_primitives::IFlyoutBase> spFlyoutBase;

    IFC(GetComposableBase().As(&spFlyoutBase));

    IFC(spFlyoutBase->Hide());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT TimePickerFlyout::OnKeyDown(_In_ IInspectable* pSender, _In_ xaml_input::IKeyRoutedEventArgs* pEventArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN bHandled = FALSE;
    BOOLEAN bShouldConfirm = FALSE;
    wsy::VirtualKey key = wsy::VirtualKey_None;
    wsy::VirtualKeyModifiers nModifierKeys;

    UNREFERENCED_PARAMETER(pSender);
    IFCPTR(pEventArgs);

    IFC(pEventArgs->get_Handled(&bHandled));
    if (bHandled)
    {
        goto Cleanup;
    }

    IFC(pEventArgs->get_Key(&key));

    if ((key == wsy::VirtualKey_Up || key == wsy::VirtualKey_Down))
    {
        IFC(PlatformHelpers::GetKeyboardModifiers(&nModifierKeys));

        if (nModifierKeys & wsy::VirtualKeyModifiers_Menu)
        {
            bShouldConfirm = TRUE;
        }
    }
    else if (key == wsy::VirtualKey_Space || key == wsy::VirtualKey_Enter)
    {
        bShouldConfirm = TRUE;
    }

    if (bShouldConfirm)
    {
        IFC(pEventArgs->put_Handled(TRUE));
        IFC(OnConfirmed());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT TimePickerFlyout::GetDefaultTime(_Outptr_ IInspectable** ppCurrentTimeProp)
{
    wrl::ComPtr<wg::ICalendarFactory> spCalendarFactory;
    wrl::ComPtr<wg::ICalendar> spCalendar;
    wrl::ComPtr<wfc::IVectorView<HSTRING>> spLanguages;
    wrl::ComPtr<wfc::IIterable<HSTRING>> spLanguagesAsIterable;

    wrl_wrappers::HString strClock;
    wf::TimeSpan retTimeSpan = {};
    INT32 hour = 0;
    INT32 minute = 0;

    *ppCurrentTimeProp = nullptr;

    IFC_RETURN(wf::ActivateInstance(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Globalization_Calendar).Get(),
        &spCalendar));
    IFC_RETURN(spCalendar->get_Languages(&spLanguages));
    IFC_RETURN(spLanguages.As(&spLanguagesAsIterable));

    IFC_RETURN(spCalendar->GetClock(strClock.GetAddressOf()));

    IFC_RETURN(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Globalization_Calendar).Get(),
        &spCalendarFactory));

    IFC_RETURN(spCalendarFactory->CreateCalendar(
        spLanguagesAsIterable.Get(),
        wrl_wrappers::HStringReference(L"GregorianCalendar").Get(),
        strClock,
        spCalendar.ReleaseAndGetAddressOf()));

    IFC_RETURN(spCalendar->SetToNow());
    IFC_RETURN(spCalendar->get_Minute(&minute));
    IFC_RETURN(spCalendar->get_Hour(&hour));

    const bool isTwelveHourClock = strClock == wrl_wrappers::HStringReference(TimePickerFlyoutPresenter::_strTwelveHourClock);
    if (isTwelveHourClock)
    {
        INT32 period = 0;
        INT32 firstPeriodInThisDay = 0;

        IFC_RETURN(spCalendar->get_Period(&period));
        IFC_RETURN(spCalendar->get_FirstPeriodInThisDay(&firstPeriodInThisDay));

        if (period == firstPeriodInThisDay)
        {
            if (hour == TimePickerFlyoutPresenter::_periodCoercionOffset)
            {
                hour = 0;
            }
        }
        else
        {
            if (hour != TimePickerFlyoutPresenter::_periodCoercionOffset)
            {
                hour += TimePickerFlyoutPresenter::_periodCoercionOffset;
            }
        }
    }

    retTimeSpan.Duration += minute * s_timeSpanTicksPerMinute;
    retTimeSpan.Duration += hour * s_timeSpanTicksPerHour;

    IFC_RETURN(Private::ValueBoxer::CreateTimeSpan(retTimeSpan, ppCurrentTimeProp));

    return S_OK;
}

_Check_return_ HRESULT
TimePickerFlyout::GetDefaultClockIdentifier(_Outptr_ IInspectable** ppClockIdentifier)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatter;
    wrl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatterFactory> spFormatterFactory;

    wrl_wrappers::HString strClockIdentifier;

    *ppClockIdentifier = NULL;

    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Globalization_DateTimeFormatting_DateTimeFormatter).Get(),
        &spFormatterFactory));

    IFC(spFormatterFactory->CreateDateTimeFormatter(
        wrl_wrappers::HStringReference(s_strHourFormat).Get(), &spFormatter));

    IFC(spFormatter->get_Clock(strClockIdentifier.GetAddressOf()));
    IFC(Private::ValueBoxer::CreateString(strClockIdentifier.Get(), ppClockIdentifier));

Cleanup:
    RRETURN(hr);
}


_Check_return_ HRESULT
TimePickerFlyout::GetDefaultMinuteIncrement(_Outptr_ IInspectable** ppDefaultMinuteIncrementValue)
{
    HRESULT hr = S_OK;

    *ppDefaultMinuteIncrementValue = NULL;

    IFC(Private::ValueBoxer::CreateInt32(1, ppDefaultMinuteIncrementValue));
Cleanup:
    RRETURN(hr);
}

} } } } XAML_ABI_NAMESPACE_END
