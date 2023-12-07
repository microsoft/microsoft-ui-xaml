// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "FlyoutAsyncOperationManager.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{
    extern __declspec(selectany) const WCHAR TimePickerFlyoutShowAtAsyncOperationName[] = L"Windows.Foundation.IAsyncOperation`1<Windows.Foundation.IReference`1<Windows.Foundation.TimeSpan>> Microsoft.UI.Xaml.Controls.TimePickerFlyout.ShowAtAsync";

    class TimePickerGetSelectionAsyncOperation;

    class TimePickerFlyout :
        public TimePickerFlyoutGenerated
    {

    public:
        TimePickerFlyout();

        static _Check_return_ HRESULT GetDefaultTime(_Outptr_ IInspectable** currentTime);
        static _Check_return_ HRESULT GetDefaultClockIdentifier(_Outptr_ IInspectable** clockIdentifier);
        static _Check_return_ HRESULT GetDefaultMinuteIncrement(_Outptr_ IInspectable** clockIdentifier);

    private:
        ~TimePickerFlyout();

        _Check_return_ HRESULT InitializeImpl() override;

        _Check_return_ HRESULT OnOpening(_In_ IInspectable* pSender, _In_ IInspectable* pArgs);

        _Check_return_ HRESULT OnOpened(_In_ IInspectable* pSender, _In_ IInspectable* pArgs);

        _Check_return_ HRESULT OnClosed(_In_ IInspectable* pSender, _In_ IInspectable* pArgs);

        _Check_return_ HRESULT OnAcceptClick(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnDismissClick(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnKeyDown(_In_ IInspectable* pSender, _In_ xaml_input::IKeyRoutedEventArgs* pEventArgs);

    public:
        _Check_return_ HRESULT ShowAtAsyncImpl(
            _In_ xaml::IFrameworkElement* pTarget,
            _Outptr_ wf::IAsyncOperation<wf::IReference<wf::TimeSpan>*>** ppOperation);

        _Check_return_ HRESULT OnConfirmedImpl() override;

        _Check_return_ HRESULT ShouldShowConfirmationButtonsImpl(
            _Out_ BOOLEAN* returnValue) override;

        _Check_return_ HRESULT CreatePresenterImpl(
            _Outptr_ xaml_controls::IControl** returnValue) override;

    private:
        static const WCHAR* s_strHourFormat;
        static const INT64 s_timeSpanTicksPerMinute;
        static const INT64 s_timeSpanTicksPerHour;
        static const INT64 s_timeSpanTicksPerDay;

        Private::TrackerPtr<ITimePickerFlyoutPresenter> _tpPresenter;
        Private::TrackerPtr<IFrameworkElement> _tpTarget;
        Private::TrackerPtr<xaml_primitives::IButtonBase> _tpAcceptButton;
        Private::TrackerPtr<xaml_primitives::IButtonBase> _tpDismissButton;

        xaml_controls::FlyoutAsyncOperationManager<wf::IReference<wf::TimeSpan>*, TimePickerFlyout, TimePickerFlyoutShowAtAsyncOperationName> _asyncOperationManager;
        EventRegistrationToken _acceptClickToken;
        EventRegistrationToken _dismissClickToken;
        EventRegistrationToken _keyDownToken;
    };

    ActivatableClassWithFactory(TimePickerFlyout, TimePickerFlyoutFactory);

} } } } XAML_ABI_NAMESPACE_END
