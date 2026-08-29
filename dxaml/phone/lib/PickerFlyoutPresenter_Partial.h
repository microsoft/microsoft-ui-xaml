// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{

    class PickerFlyoutPresenter :
        public PickerFlyoutPresenterGenerated
    {

    public:
        PickerFlyoutPresenter() {}

        _Check_return_ HRESULT SetTitle(_In_ HSTRING title);

        // IUIElementOverrides
        _Check_return_ HRESULT
        OnCreateAutomationPeerImpl(
            _Outptr_ xaml::Automation::Peers::IAutomationPeer **returnValue) override;

    protected:
        _Check_return_ HRESULT OnApplyTemplateImpl() override;

    private:
        ~PickerFlyoutPresenter() {}

        _Check_return_ HRESULT InitializeImpl();

        Private::TrackerPtr<xaml_controls::ITextBlock> _tpTitlePresenter;
        wrl_wrappers::HString _title;
    };

    ActivatableClass(PickerFlyoutPresenter);
}}}} XAML_ABI_NAMESPACE_END
