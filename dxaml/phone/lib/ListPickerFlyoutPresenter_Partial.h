// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{
    class ListPickerFlyoutPresenter :
        public ListPickerFlyoutPresenterGenerated
    {

    public:
        ListPickerFlyoutPresenter() {}

        // IUIElementOverrides
        _Check_return_ HRESULT
        OnCreateAutomationPeerImpl(
            _Outptr_ xaml::Automation::Peers::IAutomationPeer **returnValue) override;

        _Check_return_ HRESULT SetTitle(_In_ HSTRING title);

    protected:
        _Check_return_ HRESULT OnApplyTemplateImpl() override;

    private:
        ~ListPickerFlyoutPresenter() {}

        _Check_return_ HRESULT InitializeImpl() override;

        Private::TrackerPtr<xaml_controls::IPanel> _tpItemsHostPanel;
        Private::TrackerPtr<xaml_controls::ITextBlock> _tpTitlePresenter;
        wrl_wrappers::HString _title;
    };

    ActivatableClassWithFactory(ListPickerFlyoutPresenter, ListPickerFlyoutPresenterFactory);

}}}} XAML_ABI_NAMESPACE_END;
