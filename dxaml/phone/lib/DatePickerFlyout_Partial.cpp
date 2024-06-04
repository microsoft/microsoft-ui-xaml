// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <RuntimeProfiler.h>

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{

wrl::ComPtr<wg::ICalendar> DatePickerFlyout::s_spCalendar;

DatePickerFlyout::DatePickerFlyout() :
    _asyncOperationManager(FlyoutAsyncOperationManager<wf::IReference<wf::DateTime>*, DatePickerFlyout, DatePickerFlyoutShowAtAsyncOperationName>(Private::ReferenceTrackerHelper<DatePickerFlyout>(this)))
{
    __RP_Marker_ClassByName("DatePickerFlyout");
}

DatePickerFlyout::~DatePickerFlyout()
{
    ASYNCTRACE(L"DatePickerFlyout deconstructor called.");
}

_Check_return_ HRESULT
DatePickerFlyout::InitializeImpl()
{
    HRESULT hr = S_OK;
    EventRegistrationToken openingToken = {};
    EventRegistrationToken openedToken = {};
    EventRegistrationToken closedToken = {};
    wrl::ComPtr<xaml_primitives::IPickerFlyoutBaseFactory> spInnerFactory;
    wrl::ComPtr<xaml_primitives::IPickerFlyoutBase> spInnerInstance;
    wrl::ComPtr<IInspectable> spInnerInspectable;
    wrl::ComPtr<xaml_primitives::IFlyoutBase> spFlyoutBase;

    IFC(DatePickerFlyoutGenerated::InitializeImpl());

    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_Primitives_PickerFlyoutBase).Get(),
        &spInnerFactory));
    IFC(spInnerFactory->CreateInstance(
        static_cast<IDatePickerFlyout*>(this),
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
        (this, &DatePickerFlyout::OnOpening).Get(),
        &openingToken));

    IFC(spFlyoutBase->add_Opened(
        wrl::Callback<wf::IEventHandler<IInspectable*>>
        (this, &DatePickerFlyout::OnOpened).Get(),
        &openedToken));

    IFC(spFlyoutBase->add_Closed(
        wrl::Callback<wf::IEventHandler<IInspectable*>>
        (this, &DatePickerFlyout::OnClosed).Get(),
        &closedToken));


    IFC(_asyncOperationManager.Initialize(
        spFlyoutBase.Get(),
        // Cancellation value provider function
        [] () -> wf::IReference<wf::DateTime>*
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
DatePickerFlyout::ShouldShowConfirmationButtonsImpl(_Out_ BOOLEAN* result)
{
    *result = FALSE;

    RRETURN(S_OK);
}

_Check_return_ HRESULT
DatePickerFlyout::OnConfirmedImpl()
{
    HRESULT hr = S_OK;
    wf::DateTime oldDateTime = {};
    wf::DateTime newDateTime = {};
    wrl::ComPtr<xaml_controls::DatePickedEventArgs> spArgs;
    wrl::ComPtr<IInspectable> spBoxedDateTime;
    wrl::ComPtr<wf::IReference<wf::DateTime>> spBoxedDtAsReference;

    IFC(get_Date(&oldDateTime));
    IFC(static_cast<DatePickerFlyoutPresenter*>(_tpPresenter.Get())->GetDate(&newDateTime));
    IFC(put_Date(newDateTime));

    IFC(Private::ValueBoxer::CreateDateTime(newDateTime, &spBoxedDateTime));
    IFC(spBoxedDateTime.As(&spBoxedDtAsReference));
    IFC(_asyncOperationManager.Complete(spBoxedDtAsReference.Get()));

    IFC(wrl::MakeAndInitialize<xaml_controls::DatePickedEventArgs>(&spArgs));
    IFC(spArgs->put_OldDate(oldDateTime));
    IFC(spArgs->put_NewDate(newDateTime));
    IFC(m_DatePickedEventSource.InvokeAll(this, spArgs.Get()));

    IFC(DatePickerFlyoutGenerated::OnConfirmedImpl());

Cleanup:
    RRETURN(hr);
}

// -----
// IFlyoutBaseOverrides Impl
// -----

_Check_return_ HRESULT
DatePickerFlyout::CreatePresenterImpl(_Outptr_ xaml_controls::IControl** returnValue)
{
    HRESULT hr = S_OK;

    ASSERT(!_tpPresenter, "Expected CreatePresenterImpl to be called only once.");

    wrl::ComPtr<IDatePickerFlyoutPresenter> spFlyoutPresenter;
    IFC(wrl::MakeAndInitialize<DatePickerFlyoutPresenter>(&spFlyoutPresenter));
    IFC(SetPtrValue(_tpPresenter, spFlyoutPresenter.Get()));
    IFC(spFlyoutPresenter.CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DatePickerFlyout::OnPropertyChanged(
    _In_ xaml::IDependencyPropertyChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    UNREFERENCED_PARAMETER(pArgs);
    RRETURN(hr);
}

_Check_return_ HRESULT
DatePickerFlyout::ShowAtAsyncImpl(
    _In_ xaml::IFrameworkElement* pTarget,
    _Outptr_ wf::IAsyncOperation<wf::IReference<wf::DateTime>*>** ppOperation)
{
    HRESULT hr = S_OK;

    IFC(SetPtrValue(_tpTarget, pTarget));
    IFC(_asyncOperationManager.Start(pTarget, ppOperation));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
DatePickerFlyout::OnOpening(
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

    IFC_RETURN(static_cast<DatePickerFlyoutPresenter*>(_tpPresenter.Get())->PullPropertiesFromOwner(this));
    IFC_RETURN(static_cast<DatePickerFlyoutPresenter*>(_tpPresenter.Get())->SetAcceptDismissButtonsVisibility(lastInputDeviceType != xaml_input::LastInputDeviceType_GamepadOrRemote));

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
DatePickerFlyout::OnOpened(
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
            wrl::Callback<xaml::IRoutedEventHandler>(this, &DatePickerFlyout::OnAcceptClick).Get(),
            &_acceptClickToken));

        IFC(Private::FindStringResource(UIA_DIALOG_ACCEPT, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(_tpAcceptButton.As(&spButtonAsDO));
        IFC(spAutomationPropertiesStatics->SetName(spButtonAsDO.Get(), strAutomationName.Get()));
    }

    if (_tpDismissButton)
    {
        ASSERT(spDismissButtonAsUIE);
        IFC(_tpDismissButton->add_Click(
            wrl::Callback<xaml::IRoutedEventHandler>(this, &DatePickerFlyout::OnDismissClick).Get(),
            &_dismissClickToken));

        IFC(Private::FindStringResource(UIA_DIALOG_DISMISS, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(_tpDismissButton.As(&spButtonAsDO));
        IFC(spAutomationPropertiesStatics->SetName(spButtonAsDO.Get(), strAutomationName.Get()));
    }

    ASSERT(spFlyoutPresenterAsUIE);

    IFC(spFlyoutPresenterAsUIE->add_KeyDown(
        wrl::Callback<xaml_input::IKeyEventHandler>(this, &DatePickerFlyout::OnKeyDown).Get(),
        &_keyDownToken));

Cleanup:
    RRETURN(hr);
}


_Check_return_ HRESULT DatePickerFlyout::OnClosed(
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

_Check_return_ HRESULT DatePickerFlyout::OnAcceptClick(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pArgs)
{
    UNREFERENCED_PARAMETER(pSender);
    UNREFERENCED_PARAMETER(pArgs);

    RRETURN(OnConfirmed());
}

_Check_return_ HRESULT DatePickerFlyout::OnDismissClick(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pArgs)
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

_Check_return_ HRESULT DatePickerFlyout::OnKeyDown(_In_ IInspectable* pSender, _In_ xaml_input::IKeyRoutedEventArgs* pEventArgs)
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

_Check_return_ HRESULT DatePickerFlyout::GetDefaultCalendarIdentifier(_Outptr_ IInspectable** ppDefaultCalendarIdentifierValue)
{
    HRESULT hr = S_OK;

    IFC(Private::ValueBoxer::CreateString(wrl_wrappers::HStringReference(L"GregorianCalendar").Get(), ppDefaultCalendarIdentifierValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DatePickerFlyout::GetDefaultDayVisible(_Outptr_ IInspectable** ppDayVisibleValue)
{
    HRESULT hr = S_OK;

    IFC(Private::ValueBoxer::CreateBoolean(true, ppDayVisibleValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DatePickerFlyout::GetDefaultMonthVisible(_Outptr_ IInspectable** ppMonthVisibleValue)
{
    HRESULT hr = S_OK;

    IFC(Private::ValueBoxer::CreateBoolean(true, ppMonthVisibleValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DatePickerFlyout::GetDefaultYearVisible(_Outptr_ IInspectable** ppYearVisibleValue)
{
    HRESULT hr = S_OK;

    IFC(Private::ValueBoxer::CreateBoolean(true, ppYearVisibleValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DatePickerFlyout::GetDefaultDate(_Outptr_ IInspectable** ppDefaultDateValue)
{
    HRESULT hr = S_OK;
    wf::DateTime currentDate = {};

    IFC(EnsureCalendar());
    IFC(s_spCalendar->SetToNow());
    IFC(s_spCalendar->GetDateTime(&currentDate));
    IFC(Private::ValueBoxer::CreateDateTime(currentDate, ppDefaultDateValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DatePickerFlyout::GetDefaultMinYear(_Outptr_ IInspectable** ppDefaultMinYearValue)
{
    HRESULT hr = S_OK;
    wf::DateTime minDate = {};

    IFC(EnsureCalendar());
    IFC(s_spCalendar->SetToNow());
    IFC(s_spCalendar->AddYears(-_deltaYears));
    IFC(s_spCalendar->GetDateTime(&minDate));
    IFC(Private::ValueBoxer::CreateDateTime(minDate, ppDefaultMinYearValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DatePickerFlyout::GetDefaultMaxYear(_Outptr_ IInspectable** ppDefaultMaxYearValue)
{
    HRESULT hr = S_OK;
    wf::DateTime maxDate = {};

    IFC(EnsureCalendar());
    IFC(s_spCalendar->SetToNow());
    IFC(s_spCalendar->AddYears(_deltaYears));
    IFC(s_spCalendar->GetDateTime(&maxDate));
    IFC(Private::ValueBoxer::CreateDateTime(maxDate, ppDefaultMaxYearValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DatePickerFlyout::GetDefaultDayFormat(_Outptr_ IInspectable** ppDefaultDayFormatValue)
{
    return Private::ValueBoxer::CreateString(wrl_wrappers::HStringReference(L"day").Get(), ppDefaultDayFormatValue);
}

_Check_return_ HRESULT DatePickerFlyout::GetDefaultMonthFormat(_Outptr_ IInspectable** ppDefaultMonthFormatValue)
{
    return Private::ValueBoxer::CreateString(wrl_wrappers::HStringReference(L"{month.full}").Get(), ppDefaultMonthFormatValue);
}

_Check_return_ HRESULT DatePickerFlyout::GetDefaultYearFormat(_Outptr_ IInspectable** ppDefaultYearFormatValue)
{
    return Private::ValueBoxer::CreateString(wrl_wrappers::HStringReference(L"year.full").Get(), ppDefaultYearFormatValue);
}

_Check_return_ HRESULT DatePickerFlyout::EnsureCalendar()
{
    HRESULT hr = S_OK;

    if (!s_spCalendar)
    {
        wrl::ComPtr<wg::ICalendarFactory> spCalendarFactory;
        wrl::ComPtr<wg::ICalendar> spCalendar;
        wrl::ComPtr<wfc::IVectorView<HSTRING>> spLanguages;
        wrl::ComPtr<wfc::IIterable<HSTRING>> spLanguagesAsIterable;
        wrl_wrappers::HString strClock;

        IFC(wf::ActivateInstance(
            wrl_wrappers::HStringReference(RuntimeClass_Windows_Globalization_Calendar).Get(),
            &spCalendar));
        IFC(spCalendar->get_Languages(&spLanguages));
        IFC(spLanguages.As(&spLanguagesAsIterable));

        IFC(spCalendar->GetClock(strClock.GetAddressOf()));

        IFC(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Windows_Globalization_Calendar).Get(),
            &spCalendarFactory));

        IFC(spCalendarFactory->CreateCalendar(
            spLanguagesAsIterable.Get(),
            wrl_wrappers::HStringReference(L"GregorianCalendar").Get(),
            strClock,
            spCalendar.ReleaseAndGetAddressOf()));

        IFC(spCalendar.CopyTo(&s_spCalendar));
    }

Cleanup:
    RRETURN(hr);
}

} } } } XAML_ABI_NAMESPACE_END
