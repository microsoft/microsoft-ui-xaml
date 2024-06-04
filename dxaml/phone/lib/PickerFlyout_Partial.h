// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "FlyoutAsyncOperationManager.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{
    extern __declspec(selectany) const WCHAR PickerFlyoutShowAtAsyncOperationName[] = L"Windows.Foundation.IAsyncOperation`1<Boolean> Microsoft.UI.Xaml.Controls.PickerFlyout.ShowAtAsync";

    class PickerFlyoutShowAsyncOperation;

    class PickerFlyout :
        public PickerFlyoutGenerated
    {

    public:

        PickerFlyout();

    private:
        ~PickerFlyout();

        // PickerFlyout Impl
        _Check_return_ HRESULT InitializeImpl() override;

    public:
        _Check_return_ HRESULT ShowAtAsyncImpl(
            _In_ xaml::IFrameworkElement* pTarget,
            _Outptr_ wf::IAsyncOperation<bool>** returnValue);

        _Check_return_ HRESULT OnOpening(_In_ IInspectable* pSender, _In_ IInspectable* pArgs);

        _Check_return_ HRESULT CompleteAsyncOperation(_In_ BOOLEAN isSelectionAccepted);

        // IPickerFlyoutBaseOverrides Impl
        _Check_return_ HRESULT OnConfirmedImpl() override;
        _Check_return_ HRESULT ShouldShowConfirmationButtonsImpl(_Out_ BOOLEAN* returnValue) override;

        // IFlyoutBaseOverrides Impl
        _Check_return_ HRESULT CreatePresenterImpl(_Outptr_ xaml_controls::IControl** returnValue) override;

    private:
        Private::TrackerPtr<IPickerFlyoutPresenter> _tpFlyoutPresenter;
        FlyoutAsyncOperationManager<bool, PickerFlyout, PickerFlyoutShowAtAsyncOperationName> _asyncOperationManager;
    };

    ActivatableClassWithFactory(PickerFlyout, PickerFlyoutFactory);

}}}} XAML_ABI_NAMESPACE_END
