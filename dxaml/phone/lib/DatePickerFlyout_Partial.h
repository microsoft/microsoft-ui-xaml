// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "FlyoutAsyncOperationManager.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{
    extern __declspec(selectany) const WCHAR DatePickerFlyoutShowAtAsyncOperationName[] = L"Windows.Foundation.IAsyncOperation`1<Windows.Foundation.IReference`1<Windows.Foundation.DateTime>> Microsoft.UI.Xaml.Controls.DatePickerFlyout.ShowAtAsync";

    class DatePickerFlyout :
        public DatePickerFlyoutGenerated
    {

    public:
        DatePickerFlyout();

        _Check_return_ HRESULT OnPropertyChanged(
            _In_ xaml::IDependencyPropertyChangedEventArgs* pArgs);

        static _Check_return_ HRESULT GetDefaultCalendarIdentifier(_Outptr_ IInspectable** ppDefaultCalendarIdentifierValue);
        static _Check_return_ HRESULT GetDefaultDate(_Outptr_ IInspectable** ppDefaultDateValue);
        static _Check_return_ HRESULT GetDefaultDayVisible(_Outptr_ IInspectable** ppDayVisibleValue);
        static _Check_return_ HRESULT GetDefaultMonthVisible(_Outptr_ IInspectable** ppMonthVisibleValue);
        static _Check_return_ HRESULT GetDefaultYearVisible(_Outptr_ IInspectable** ppYearVisibleValue);
        static _Check_return_ HRESULT GetDefaultMinYear(_Outptr_ IInspectable** ppDefaultMinYearValue);
        static _Check_return_ HRESULT GetDefaultMaxYear(_Outptr_ IInspectable** ppDefaultMaxYearValue);
        static _Check_return_ HRESULT GetDefaultDayFormat(_Outptr_ IInspectable** ppDefaultDayFormat);
        static _Check_return_ HRESULT GetDefaultMonthFormat(_Outptr_ IInspectable** ppDefaultMonthFormat);
        static _Check_return_ HRESULT GetDefaultYearFormat(_Outptr_ IInspectable** ppDefaultYearFormat);


    private:
        ~DatePickerFlyout();

        _Check_return_ HRESULT InitializeImpl() override;

        static _Check_return_ HRESULT EnsureCalendar();

        _Check_return_ HRESULT OnOpening(_In_ IInspectable* pSender, _In_ IInspectable* pArgs);

        _Check_return_ HRESULT OnOpened(_In_ IInspectable* pSender, _In_ IInspectable* pArgs);

        _Check_return_ HRESULT OnClosed(_In_ IInspectable* pSender, _In_ IInspectable* pArgs);

        _Check_return_ HRESULT OnAcceptClick(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnDismissClick(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnKeyDown(_In_ IInspectable* pSender, _In_ xaml_input::IKeyRoutedEventArgs* pEventArgs);

    public:
        _Check_return_ HRESULT ShowAtAsyncImpl(
            _In_ xaml::IFrameworkElement* pTarget,
            _Outptr_ wf::IAsyncOperation<wf::IReference<wf::DateTime>*>** ppAction);

    private:
        _Check_return_ HRESULT OnConfirmedImpl() override;

        _Check_return_ HRESULT ShouldShowConfirmationButtonsImpl(
            _Out_ BOOLEAN* returnValue) override;

        _Check_return_ HRESULT CreatePresenterImpl(
            _Outptr_ xaml_controls::IControl** returnValue) override;

        // The number of years the default Max and Min year field will
        // be set from the current year.
        static const INT32 _deltaYears = 50;
        static wrl::ComPtr<wg::ICalendar> s_spCalendar;

        Private::TrackerPtr<IDatePickerFlyoutPresenter> _tpPresenter;
        Private::TrackerPtr<IFrameworkElement> _tpTarget;
        FlyoutAsyncOperationManager<wf::IReference<wf::DateTime>*, DatePickerFlyout, DatePickerFlyoutShowAtAsyncOperationName> _asyncOperationManager;

        Private::TrackerPtr<xaml_primitives::IButtonBase> _tpAcceptButton;
        Private::TrackerPtr<xaml_primitives::IButtonBase> _tpDismissButton;
        EventRegistrationToken _acceptClickToken;
        EventRegistrationToken _dismissClickToken;
        EventRegistrationToken _keyDownToken;
    };

    ActivatableClassWithFactory(DatePickerFlyout, DatePickerFlyoutFactory);

} } } } XAML_ABI_NAMESPACE_END
