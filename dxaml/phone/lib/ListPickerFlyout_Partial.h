// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "FlyoutAsyncOperationManager.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{
    extern __declspec(selectany) const WCHAR ListPickerFlyoutShowAtAsyncOperationName[] = L"Windows.Foundation.IAsyncOperation`1<Windows.Foundation.Collections.IVectorView`1<Object>> Microsoft.UI.Xaml.Controls.ListPickerFlyout.ShowAtAsync";

    class ListPickerGetSelectionAsyncOperation;

    class ListPickerFlyout :
        public ListPickerFlyoutGenerated
    {

    public:
        ListPickerFlyout();

        _Check_return_ HRESULT OnPropertyChanged(
            _In_ xaml::IDependencyPropertyChangedEventArgs* pArgs);

        _Check_return_ HRESULT SetItemsOwner(
             _In_ xaml_controls::IItemsControl* pItemsOwner);

        _Check_return_ static HRESULT GetDefaultSelectionMode(
            _Outptr_ IInspectable** ppValue);

    public:

        // IPickerFlyoutBaseOverrides Impl
        _Check_return_ HRESULT OnConfirmedImpl() override;
        _Check_return_ HRESULT ShouldShowConfirmationButtonsImpl(_Out_ BOOLEAN* returnValue) override;

        // IFlyoutBaseOverrides Impl
        _Check_return_ HRESULT CreatePresenterImpl(_Outptr_ xaml_controls::IControl** returnValue) override;

        _Check_return_ HRESULT ShowAtAsyncImpl(
            _In_ xaml::IFrameworkElement* pTarget,
            _Outptr_ wf::IAsyncOperation<wfc::IVectorView<IInspectable*>*>** returnValue);

    private:
        ~ListPickerFlyout() { }

        _Check_return_ HRESULT InitializeImpl() override;

        // ListPickerFlyout Impl

        _Check_return_ HRESULT OnItemsHostContainerContentChanging(
            _In_ xaml_controls::IListViewBase* pSender,
            _In_ xaml_controls::IContainerContentChangingEventArgs* pArgs);

         _Check_return_ HRESULT OnItemsHostSelectionChanged(
            _In_ IInspectable* pSender,
            _In_ xaml_controls::ISelectionChangedEventArgs* pArgs);

        _Check_return_ HRESULT OnItemsHostItemClick(
            _In_ IInspectable* pSender,
            _In_ xaml_controls::IItemClickEventArgs* pArgs);

        _Check_return_ HRESULT OnOpening(_In_ IInspectable* pSender, _In_ IInspectable* pArgs);
        _Check_return_ HRESULT OnClosed(_In_ IInspectable* pSender, _In_ IInspectable* pArgs);

        _Check_return_ HRESULT UpdateHighlightedItem(_In_opt_ xaml_controls::IListViewItem* pNewContainer);

        static _Check_return_ HRESULT CopyCollectionTo(
            _In_ wfc::IIterable<IInspectable*>* original,
            _In_ wfc::IVector<IInspectable*>* copy);

        static _Check_return_ HRESULT DiffCollections(
            _In_ wfc::IIterable<IInspectable*>* v1,
            _In_ wfc::IIterable<IInspectable*>* v2,
            _Outptr_ wfc::IVector<IInspectable*>** in1Not2,
            _Outptr_ wfc::IVector<IInspectable*>** in2Not1);

        Private::TrackerPtr<xaml_controls::IListPickerFlyoutPresenter> _tpFlyoutPresenter;
        Private::TrackerPtr<xaml_controls::IListViewBase> _tpItemsHost;
        Private::TrackerPtr<xaml_controls::IItemsControl> _tpItemsHostAsIC;
        Private::TrackerPtr<xaml_primitives::ISelector> _tpItemsHostAsSelector;
        Private::TrackerPtr<wfc::IVector<IInspectable*>> _tpOriginalSelectedItems;
        Private::TrackerPtr<xaml_controls::IControl> _tpHighlightedContainer;
        Private::TrackerPtr<xaml_controls::IItemsControl> _tpItemsOwner;

        BOOLEAN _isSelectionAccepted;
        BOOLEAN _ignorePropertyChanges;
        FlyoutAsyncOperationManager<wfc::IVectorView<IInspectable*>*, ListPickerFlyout, ListPickerFlyoutShowAtAsyncOperationName> _asyncOperationManager;
    };

    ActivatableClassWithFactory(ListPickerFlyout, ListPickerFlyoutFactory);

}}}} XAML_ABI_NAMESPACE_END;
