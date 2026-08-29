// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <RuntimeProfiler.h>

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{

ListPickerFlyout::ListPickerFlyout() :
    _isSelectionAccepted(FALSE),
    _ignorePropertyChanges(FALSE),
    _asyncOperationManager(FlyoutAsyncOperationManager<wfc::IVectorView<IInspectable*>*, ListPickerFlyout, ListPickerFlyoutShowAtAsyncOperationName>(Private::ReferenceTrackerHelper<ListPickerFlyout>(this)))
{
    __RP_Marker_ClassByName("ListPickerFlyout");
}

_Check_return_ HRESULT
ListPickerFlyout::InitializeImpl()
{
    HRESULT hr = S_OK;
    EventRegistrationToken closedToken = { };
    EventRegistrationToken openingToken = { };
    EventRegistrationToken itemsHostSelectionChangedToken = { };
    EventRegistrationToken itemsHostItemClickToken = { };
    wrl::ComPtr<xaml_primitives::IPickerFlyoutBaseFactory> spInnerFactory;
    wrl::ComPtr<xaml_primitives::IPickerFlyoutBase> spDelegatingInner;
    wrl::ComPtr<IInspectable> spNonDelegatingInnerInspectable;
    wrl::ComPtr<xaml::IDependencyObject> spThisAsDO;
    wrl::ComPtr<xaml_primitives::IFlyoutBase> spFlyoutBase;
    wrl::ComPtr<wfci_::Vector<IInspectable*>> spOriginalSelectedItems;
    wrl::ComPtr<xaml_controls::IListPickerFlyoutPresenter> spFlyoutPresenter;
    ListPickerFlyoutPresenter* pPresenterNoRef = nullptr;
    wrl::ComPtr<xaml_controls::IListViewBase> spItemsHost;
    wrl::ComPtr<wfc::IVector<IInspectable*>> spSelectedItems;
    wrl::ComPtr<xaml_primitives::IPickerFlyoutBaseStatics> spPickerFlyoutBaseStatics;
    wrl_wrappers::HString defaultTitle;

    IFC(ListPickerFlyoutGenerated::InitializeImpl());

    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_Primitives_PickerFlyoutBase).Get(),
        &spInnerFactory));

    IFC(spInnerFactory->CreateInstance(
        static_cast<IListPickerFlyout*>(this),
        &spNonDelegatingInnerInspectable,
        &spDelegatingInner));

    IFC(SetComposableBasePointers(
        spNonDelegatingInnerInspectable.Get(),
        spInnerFactory.Get()));

     IFC(QueryInterface(
        __uuidof(xaml_primitives::IFlyoutBase),
        &spFlyoutBase));

    IFC(spFlyoutBase->add_Closed(
        wrl::Callback<wf::IEventHandler<IInspectable*>>(this, &ListPickerFlyout::OnClosed).Get(),
        &closedToken));

    IFC(spFlyoutBase->add_Opening(
        wrl::Callback<wf::IEventHandler<IInspectable*>>(this, &ListPickerFlyout::OnOpening).Get(),
        &openingToken));

    // Create a vector to store the pre-opening selection state
    IFC(wfci_::Vector<IInspectable*>::Make(&spOriginalSelectedItems));
    IFC(SetPtrValue(_tpOriginalSelectedItems, static_cast<wfc::IVector<IInspectable*>*>(spOriginalSelectedItems.Get())));

    // Initialize a presenter and cache the items host ListViewBase. Rather than duplicating
    // all of the ListPickerFlyout properties on ListPickerFlyoutPresenter so that the
    // items host in the presenter can bind to them with a TemplateBinding, we're going to
    // directly forward all property changes to the items host.
    IFC(wrl::MakeAndInitialize<ListPickerFlyoutPresenter>(&spFlyoutPresenter));
    IFC(SetPtrValue(_tpFlyoutPresenter, static_cast<xaml_controls::IListPickerFlyoutPresenter*>(spFlyoutPresenter.Get())));
    pPresenterNoRef = static_cast<ListPickerFlyoutPresenter*>(spFlyoutPresenter.Get());

    IFC(pPresenterNoRef->get_ItemsHost(&spItemsHost));
    if (spItemsHost)
    {
        wrl::ComPtr<xaml_controls::IItemsControl> spItemsHostAsIC;
        wrl::ComPtr<xaml_primitives::ISelector> spItemsHostAsSelector;

        IFC(spItemsHost.As(&spItemsHostAsIC));
        IFC(spItemsHost.As(&spItemsHostAsSelector));

        IFC(SetPtrValue(_tpItemsHost, spItemsHost.Get()));
        IFC(SetPtrValue(_tpItemsHostAsIC, spItemsHostAsIC.Get()));
        IFC(SetPtrValue(_tpItemsHostAsSelector, spItemsHostAsSelector.Get()));

        // Share the SelectedItems vector with the ItemsHost
        IFC(spNonDelegatingInnerInspectable.As(&spThisAsDO));
        IFC(_tpItemsHost->get_SelectedItems(&spSelectedItems));
        IFC(spThisAsDO->SetValue(ListPickerFlyoutFactory::s_SelectedItemsProperty.Get(), spSelectedItems.Get()));

        // Register for ItemsHost SelectionChanged
        IFC(_tpItemsHostAsSelector->add_SelectionChanged(
            wrl::Callback<xaml_controls::ISelectionChangedEventHandler>(this, &ListPickerFlyout::OnItemsHostSelectionChanged).Get(),
            &itemsHostSelectionChangedToken));

        IFC(_tpItemsHost->put_IsItemClickEnabled(TRUE));
        IFC(_tpItemsHost->add_ItemClick(
            wrl::Callback<xaml_controls::IItemClickEventHandler>(this, &ListPickerFlyout::OnItemsHostItemClick).Get(),
            &itemsHostItemClickToken));
    }

    // Set the default Title
    IFC(Private::FindStringResource(
        TEXT_LISTPICKERFLYOUT_TITLE,
        defaultTitle.GetAddressOf()));
    IFC(spInnerFactory.As(&spPickerFlyoutBaseStatics));
    IFC(spPickerFlyoutBaseStatics->SetTitle(spThisAsDO.Get(), defaultTitle.Get()));

    // Set the default SelectedIndex
    IFC(put_SelectedIndex(-1));

    IFC(_asyncOperationManager.Initialize(
        spFlyoutBase.Get(),
        // Cancellation value provider function
        [] () -> wfc::IVectorView<IInspectable*>*
        {
            return nullptr;
        }));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ListPickerFlyout::GetDefaultSelectionMode(
    _Outptr_ IInspectable** ppValue)
{
    RRETURN(Private::ValueBoxer::CreateReference<ListPickerFlyoutSelectionMode>(ListPickerFlyoutSelectionMode_Single, ppValue));
}

_Check_return_ HRESULT
ListPickerFlyout::OnPropertyChanged(_In_ xaml::IDependencyPropertyChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<IInspectable> spNewValue;
    wrl::ComPtr<xaml::IDependencyProperty> spPropertyInfo;

    if (!_tpItemsHost)
    {
        // Nothing to do as we don't have an items host to propagate the changes to.
        goto Cleanup;
    }

    ASSERT(_tpItemsHostAsIC, "ItemsHost is not an ItemsControl. This should not be possible.");
    ASSERT(_tpItemsHostAsSelector, "ItemsHost is not a Selector. This should not be possible.");

    if (_ignorePropertyChanges)
    {
        // The property change was triggered by an update to the items host, so no need
        // to propagate the change back to the items host again.
        goto Cleanup;
    }

    IFC(pArgs->get_Property(&spPropertyInfo));
    IFC(pArgs->get_NewValue(&spNewValue));

    if (spPropertyInfo.Get() == ListPickerFlyoutFactory::s_ItemsSourceProperty)
    {
        IFC(_tpItemsHostAsIC->put_ItemsSource(spNewValue.Get()));
    }
    else if (spPropertyInfo.Get() == ListPickerFlyoutFactory::s_SelectionModeProperty)
    {
        wrl::ComPtr<wf::IReference<xaml_controls::ListPickerFlyoutSelectionMode>> spNewValueAsIRef;
        xaml_controls::ListPickerFlyoutSelectionMode selectionMode = xaml_controls::ListPickerFlyoutSelectionMode_Single;

        IFC(spNewValue.As(&spNewValueAsIRef));
        IFC(spNewValueAsIRef->get_Value(&selectionMode));

        if (selectionMode == xaml_controls::ListPickerFlyoutSelectionMode_Multiple)
        {
            IFC(_tpItemsHost->put_IsItemClickEnabled(FALSE));
            IFC(_tpItemsHost->put_SelectionMode(xaml_controls::ListViewSelectionMode_Multiple));
        }
        else
        {
            IFC(_tpItemsHost->put_IsItemClickEnabled(TRUE));
            IFC(_tpItemsHost->put_SelectionMode(xaml_controls::ListViewSelectionMode_None));
        }
    }
    else if (spPropertyInfo.Get() == ListPickerFlyoutFactory::s_DisplayMemberPathProperty)
    {
        wrl::ComPtr<wf::IPropertyValue> spNewValueAsPV;
        wrl_wrappers::HString displayMemberPath;

        IFC(spNewValue.As(&spNewValueAsPV));
        IFC(spNewValueAsPV->GetString(displayMemberPath.GetAddressOf()));
        IFC(_tpItemsHostAsIC->put_DisplayMemberPath(displayMemberPath.Get()));
    }
    else if (spPropertyInfo.Get() == ListPickerFlyoutFactory::s_ItemTemplateProperty)
    {
        wrl::ComPtr<xaml::IDataTemplate> spItemTemplate;

        IFC(spNewValue.As(&spItemTemplate));
        IFC(_tpItemsHostAsIC->put_ItemTemplate(spItemTemplate.Get()));
    }
    else if (spPropertyInfo.Get() == ListPickerFlyoutFactory::s_SelectedValuePathProperty)
    {
        wrl::ComPtr<wf::IPropertyValue> spNewValueAsPV;
        wrl_wrappers::HString selectedValuePath;

        IFC(spNewValue.As(&spNewValueAsPV));
        IFC(spNewValueAsPV->GetString(selectedValuePath.GetAddressOf()));
        IFC(_tpItemsHostAsSelector->put_SelectedValuePath(selectedValuePath.Get()));
    }
    else if (spPropertyInfo.Get() == ListPickerFlyoutFactory::s_SelectedValueProperty)
    {
        IFC(_tpItemsHostAsSelector->put_SelectedValue(spNewValue.Get()));

        // The items host will null out the selection if the specified value does not
        // match any items in the ItemsSource. This will trigger a SelectionChanged
        // event on the items host just like any other selection change would, so there's
        // no need to do additional work here to stay in sync.
    }
    else if (spPropertyInfo.Get() == ListPickerFlyoutFactory::s_SelectedIndexProperty)
    {
        wrl::ComPtr<wf::IPropertyValue> spNewValueAsPV;
        INT32 newSelectedIndex = -1;

        IFC(spNewValue.As(&spNewValueAsPV));
        IFC(spNewValueAsPV->GetInt32(&newSelectedIndex));
        IFC(_tpItemsHostAsSelector->put_SelectedIndex(newSelectedIndex));
    }
    else if (spPropertyInfo.Get() == ListPickerFlyoutFactory::s_SelectedItemProperty)
    {
        IFC(_tpItemsHostAsSelector->put_SelectedItem(spNewValue.Get()));

        // The items host may revert the change if the specified item is not in the ItemsSource.
        // Ensure that we stay in sync.
        {
            wrl::ComPtr<IInspectable> spSelectedItem;

            IFC(_tpItemsHostAsSelector->get_SelectedItem(&spSelectedItem));
            _ignorePropertyChanges = TRUE;
            IFC(put_SelectedItem(spSelectedItem.Get()));
            _ignorePropertyChanges = FALSE;
        }
    }

Cleanup:
    RRETURN(hr);
}


// -----
// IPickerFlyoutBaseOverrides Impl
// -----

_Check_return_ HRESULT
ListPickerFlyout::OnConfirmedImpl()
{
    HRESULT hr = S_OK;
    wrl::ComPtr<wfc::IVector<IInspectable*>> spSelectedItems;
    wrl::ComPtr<wfc::IIterable<IInspectable*>> spSelectedItemsAsIterable;
    wrl::ComPtr<wfc::IIterable<IInspectable*>> spOriginalSelectedItemsAsIterable;
    wrl::ComPtr<wfc::IVectorView<IInspectable*>> spSelectedItemsAsVectorView;
    wrl::ComPtr<xaml_controls::ItemsPickedEventArgs> spSelectionCompletedArgs;
    wrl::ComPtr<wfc::IVector<IInspectable*>> spAddedItems;
    wrl::ComPtr<wfc::IVector<IInspectable*>> spRemovedItems;

    _isSelectionAccepted = TRUE;

    IFC(get_SelectedItems(&spSelectedItems));
    IFC(_tpOriginalSelectedItems.As(&spOriginalSelectedItemsAsIterable));

    IFC(spSelectedItems->GetView(&spSelectedItemsAsVectorView));
    IFC(_asyncOperationManager.Complete(spSelectedItemsAsVectorView.Get()));

    IFC(wrl::MakeAndInitialize<xaml_controls::ItemsPickedEventArgs>(&spSelectionCompletedArgs));
    IFC(spSelectedItems.As(&spSelectedItemsAsIterable));
    IFC(DiffCollections(spOriginalSelectedItemsAsIterable.Get(), spSelectedItemsAsIterable.Get(), &spRemovedItems, &spAddedItems));
    IFC(spSelectionCompletedArgs->put_AddedItems(spAddedItems.Get()));
    IFC(spSelectionCompletedArgs->put_RemovedItems(spRemovedItems.Get()));
    IFC(m_ItemsPickedEventSource.InvokeAll(this, spSelectionCompletedArgs.Get()));

    IFC(ListPickerFlyoutGenerated::OnConfirmedImpl());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ListPickerFlyout::ShouldShowConfirmationButtonsImpl(_Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;
    xaml_controls::ListPickerFlyoutSelectionMode selectionMode = xaml_controls::ListPickerFlyoutSelectionMode_Single;

    IFC(get_SelectionMode(&selectionMode));
    *returnValue = selectionMode == xaml_controls::ListPickerFlyoutSelectionMode_Multiple;

Cleanup:
    RRETURN(hr);
}

// -----
// IFlyoutBaseOverrides Impl
// -----

_Check_return_ HRESULT
ListPickerFlyout::CreatePresenterImpl(_Outptr_ xaml_controls::IControl** returnValue)
{
    HRESULT hr = S_OK;

    ASSERT(_tpFlyoutPresenter, "ListPickerFlyout presenter should never be null");
    IFC(_tpFlyoutPresenter.CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}


// ListPickerFlyout Impl


_Check_return_ HRESULT
ListPickerFlyout::ShowAtAsyncImpl(
    _In_ xaml::IFrameworkElement* pTarget,
    _Outptr_ wf::IAsyncOperation<wfc::IVectorView<IInspectable*>*>** returnValue)
{
    HRESULT hr = S_OK;

    IFC(_asyncOperationManager.Start(pTarget, returnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ListPickerFlyout::SetItemsOwner(
    _In_ xaml_controls::IItemsControl* pItemsOwner)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml_controls::IItemsControl> spComboBoxAsIC(pItemsOwner);
    EventRegistrationToken itemsHostContainerContentChangingToken = { };

    ASSERT(_tpItemsHost, "Expected items host to be created before SetDisabledIndices is called.");
    ASSERT(!_tpItemsOwner, "Expected that ListPickerFlyout original item source is set only once.");

    IFC(SetPtrValue(_tpItemsOwner, spComboBoxAsIC.Get()));

    IFC(_tpItemsHost->add_ContainerContentChanging(
        wrl::Callback<wf::ITypedEventHandler<xaml_controls::ListViewBase*, xaml_controls::ContainerContentChangingEventArgs*>>(
            this, &ListPickerFlyout::OnItemsHostContainerContentChanging).Get(),
        &itemsHostContainerContentChangingToken));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ListPickerFlyout::OnItemsHostContainerContentChanging(
    _In_ xaml_controls::IListViewBase* /*pSender*/,
    _In_ xaml_controls::IContainerContentChangingEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    INT32 index = -1;
    INT32 selectedIndex = -1;
    ListPickerFlyoutSelectionMode selectionMode = ListPickerFlyoutSelectionMode_Single;
    wrl::ComPtr<xaml_primitives::ISelectorItem> spContainer;
    BOOLEAN isEnabled = FALSE;
    wrl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
    wrl::ComPtr<wfc::IVectorView<IInspectable*>> spItemsAsVectorView;
    wrl::ComPtr<IInspectable> spSourceItem;
    wrl::ComPtr<xaml_controls::IComboBoxItem> spSourceItemAsCBI;

    IFC(pArgs->get_ItemContainer(&spContainer));
    IFC(pArgs->get_ItemIndex(&index));
    ASSERT(index >= 0, "Index of an existing item cannot be negative.");

    IFC(_tpItemsOwner->get_Items(&spItems));
    IFC(spItems.As(&spItemsAsVectorView));

    IFC(spItemsAsVectorView->GetAt(index, &spSourceItem));

    if (SUCCEEDED(spSourceItem.As(&spSourceItemAsCBI)))
    {
        wrl::ComPtr<xaml_controls::IControl> spSourceItemAsControl;
        wrl::ComPtr<xaml_primitives::ISelectorItem> spTargetItem;
        wrl::ComPtr<xaml::IDependencyObject> spTargetItemAsDO;
        wrl::ComPtr<xaml::IDependencyObject> spSourceItemAsDO;

        IFC(spSourceItemAsCBI.As(&spSourceItemAsControl));
        IFC(spSourceItemAsControl->get_IsEnabled(&isEnabled));

        IFC(pArgs->get_ItemContainer(&spTargetItem));

        if (!isEnabled)
        {
            wrl::ComPtr<xaml_controls::IControl> spTargetItemAsControl;

            IFC(spTargetItem.As(&spTargetItemAsControl));
            IFC(spTargetItemAsControl->put_IsEnabled(FALSE));
        }

        IFC(spSourceItemAsCBI.As(&spSourceItemAsDO));
        IFC(spTargetItem.As(&spTargetItemAsDO));

        IFC(Private::AutomationHelper::CopyAutomationProperties(
            spSourceItemAsDO.Get(),
            spTargetItemAsDO.Get()));
    }

    IFC(get_SelectedIndex(&selectedIndex));
    IFC(get_SelectionMode(&selectionMode));
    if (selectionMode == ListPickerFlyoutSelectionMode_Single && selectedIndex == index)
    {
        wrl::ComPtr<xaml_controls::IListViewItem> spContainerAsLvi;
        IFC(spContainer.As(&spContainerAsLvi));
        IFC(UpdateHighlightedItem(spContainerAsLvi.Get()));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ListPickerFlyout::OnItemsHostSelectionChanged(
    _In_ IInspectable* /*pSender*/,
    _In_ xaml_controls::ISelectionChangedEventArgs* /*pArgs*/)
{
    HRESULT hr = S_OK;
    INT32 selectedIndex = -1;
    ListPickerFlyoutSelectionMode selectionMode = ListPickerFlyoutSelectionMode_Single;
    wrl::ComPtr<IInspectable> spSelectedItem;
    wrl::ComPtr<IInspectable> spSelectedValue;

    IFC(_tpItemsHostAsSelector->get_SelectedIndex(&selectedIndex));
    IFC(_tpItemsHostAsSelector->get_SelectedItem(&spSelectedItem));
    IFC(_tpItemsHostAsSelector->get_SelectedValue(&spSelectedValue));

    _ignorePropertyChanges = TRUE;
    IFC(put_SelectedIndex(selectedIndex));
    IFC(put_SelectedItem(spSelectedItem.Get()));
    IFC(put_SelectedValue(spSelectedValue.Get()));
    _ignorePropertyChanges = FALSE;

    IFC(get_SelectionMode(&selectionMode));
    if (selectionMode == ListPickerFlyoutSelectionMode_Single)
    {
        if (selectedIndex == -1)
        {
            IFC(UpdateHighlightedItem(nullptr));
        }
        else
        {
            wrl::ComPtr<xaml_controls::IItemContainerMapping> spItemContainerMapping;
            wrl::ComPtr<xaml::IDependencyObject> spContainerAsDO;
            wrl::ComPtr<xaml_controls::IListViewItem> spContainer;

            IFC(_tpItemsHost.As(&spItemContainerMapping));
            IFC(spItemContainerMapping->ContainerFromItem(spSelectedItem.Get(), &spContainerAsDO));
            if (spContainerAsDO)
            {
                IFC(spContainerAsDO.As(&spContainer))
                IFC(UpdateHighlightedItem(spContainer.Get()));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ListPickerFlyout::UpdateHighlightedItem(_In_opt_ xaml_controls::IListViewItem* pNewContainer)
{
    HRESULT hr = S_OK;
    boolean bUnusedReturnVal;
    wrl::ComPtr<xaml::IVisualStateManagerStatics> spVsmStatics;
    wrl::ComPtr<xaml_controls::IListViewItem> spNewContainer(pNewContainer);
    wrl::ComPtr<xaml_controls::IControl> spNewContainerAsControl;

    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_VisualStateManager).Get(),
        &spVsmStatics));

    if (_tpHighlightedContainer)
    {
        IFC(spVsmStatics->GoToState(_tpHighlightedContainer.Get(),
            wrl_wrappers::HStringReference(L"NoHighlight").Get(),
            true,
            &bUnusedReturnVal));
        _tpHighlightedContainer.Clear();
    }

    if (spNewContainer)
    {
        IFC(spNewContainer.As(&spNewContainerAsControl));
        IFC(spVsmStatics->GoToState(spNewContainerAsControl.Get(),
            wrl_wrappers::HStringReference(L"Highlighted").Get(),
            true,
            &bUnusedReturnVal));

        IFC(SetPtrValue(_tpHighlightedContainer, spNewContainerAsControl.Get()));
    }


Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ListPickerFlyout::OnItemsHostItemClick(
    _In_ IInspectable* /*pSender*/,
    _In_ xaml_controls::IItemClickEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<IInspectable> spItemClicked;
    wrl::ComPtr<xaml_controls::IItemContainerMapping> spItemContainerMapping;
    wrl::ComPtr<xaml::IDependencyObject> spContainer;
    INT32 selectedIndex = -1;

#ifdef DBG
    xaml_controls::ListPickerFlyoutSelectionMode selectionMode = xaml_controls::ListPickerFlyoutSelectionMode_Single;
    IFC(get_SelectionMode(&selectionMode));
    ASSERT(selectionMode == xaml_controls::ListPickerFlyoutSelectionMode_Single, "ItemsHost ItemClick event should be disabled in multiselect mode.");
#endif

    IFC(pArgs->get_ClickedItem(&spItemClicked));

    // We use this method of getting the selected index instead of just calling
    // put_SelectedItem or getting the index with IndexOf because in in the case
    // where there are values typed objects with duplicate values in the
    // items collection, IndexOf or put_SelectedItem will use the first item that
    // matches instead of the item that was actually clicked. The IItemContainerMapping
    // methods for ListView have a special case that allows the search for a matching
    // item to begin with the clicked item, thereby yielding the correct result.
    IFC(_tpItemsHost.As(&spItemContainerMapping));
    IFC(spItemContainerMapping->ContainerFromItem(spItemClicked.Get(), &spContainer));
    IFC(spItemContainerMapping->IndexFromContainer(spContainer.Get(), &selectedIndex));
    IFC(put_SelectedIndex(selectedIndex));

    IFC(OnConfirmed());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ListPickerFlyout::OnOpening(
    _In_ IInspectable* /* pSender */,
    _In_ IInspectable* /* pArgs */)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml::IDependencyObject> spThisAsDO;
    wrl::ComPtr<xaml_primitives::IPickerFlyoutBaseStatics> spPickerFlyoutBaseStatics;
    wrl::ComPtr<wfc::IVector<IInspectable*>> spSelectedItems;
    wrl::ComPtr<wfc::IIterable<IInspectable*>> spSelectedItemsAsIterable;
    ListPickerFlyoutPresenter* pPresenterNoRef = static_cast<ListPickerFlyoutPresenter*>(_tpFlyoutPresenter.Get());
    wrl_wrappers::HString title;

#ifdef DBG
    UINT32 size = 0;
    ASSERT(_tpOriginalSelectedItems, "Vector to store pre-open selection state is null.");
    IFC(_tpOriginalSelectedItems->get_Size(&size));
    ASSERT(size == 0, "Record of pre-open selection state should have been cleared.");
#endif

    // Store the selection state at the time of opening so that we can do a comparison when
    // new selection is accepted, or restore the old values when the selection is canceled.
    IFC(get_SelectedItems(&spSelectedItems));
    IFC(spSelectedItems.As(&spSelectedItemsAsIterable));
    IFC(ListPickerFlyout::CopyCollectionTo(spSelectedItemsAsIterable.Get(), _tpOriginalSelectedItems.Get()));

    // Only Title needs to be propagated to the presenter, since other values have been kept
    // in sync with the items host as they have changed.
    IFC(QueryInterface(__uuidof(xaml::IDependencyObject), &spThisAsDO));
    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_Primitives_PickerFlyoutBase).Get(),
        &spPickerFlyoutBaseStatics));
    IFC(spPickerFlyoutBaseStatics->GetTitle(spThisAsDO.Get(), title.GetAddressOf()));

    IFC(pPresenterNoRef->SetTitle(title.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ListPickerFlyout::OnClosed(
    _In_ IInspectable* /* pSender */,
    _In_ IInspectable* /* pArgs */)
{
    HRESULT hr = S_OK;

    if (!_isSelectionAccepted)
    {
        // If the selection was canceled, we need to revert the selection state back to
        // what it was before the flyout was opened.
        xaml_controls::ListPickerFlyoutSelectionMode selectionMode = xaml_controls::ListPickerFlyoutSelectionMode_Single;
        IFC(get_SelectionMode(&selectionMode));
        if (selectionMode == xaml_controls::ListPickerFlyoutSelectionMode_Multiple)
        {
            wrl::ComPtr<wfc::IVector<IInspectable*>> spSelectedItems;
            wrl::ComPtr<wfc::IIterable<IInspectable*>> spOriginalSelectedItemsAsIterable;

            IFC(get_SelectedItems(&spSelectedItems));
            IFC(_tpOriginalSelectedItems.As(&spOriginalSelectedItemsAsIterable));
            IFC(spSelectedItems->Clear());
            IFC(ListPickerFlyout::CopyCollectionTo(spOriginalSelectedItemsAsIterable.Get(), spSelectedItems.Get()));
        }
        // If we're in single selection mode the selection is accepted as soon as it
        // changes, so if it wasn't accepted we know there were no changes and
        // there's nothing to revert.
    }

    IFC(_tpOriginalSelectedItems->Clear());
    _isSelectionAccepted = FALSE;

Cleanup:
    RRETURN(hr);
}

// Simple helper to copy the elements of one collection into another collection
_Check_return_ HRESULT
ListPickerFlyout::CopyCollectionTo(
    _In_ wfc::IIterable<IInspectable*>* original,
    _In_ wfc::IVector<IInspectable*>* copy)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<wfc::IIterator<IInspectable*>> iter;
    BOOLEAN iterHasCurrent = FALSE;
    wrl::ComPtr<IInspectable> spCurrent;

    IFC(original->First(&iter));
    IFC(iter->get_HasCurrent(&iterHasCurrent));
    while (iterHasCurrent)
    {
        IFC(iter->get_Current(spCurrent.ReleaseAndGetAddressOf()));
        IFC(copy->Append(spCurrent.Get()));
        IFC(iter->MoveNext(&iterHasCurrent));
    }

Cleanup:
    RRETURN(hr);
}

// Provides a simple O(n * m) diff of 2 collections of elements. While the algorithm is slow
// in the worst case of two large, completely different collections, in our scenario,
// where the two collections represent the before/after selection states, this is very
// unlikely to happen. Typically the two collections will be quite similar, or small, or
// both, so further optimization is unnecessary.
_Check_return_ HRESULT
ListPickerFlyout::DiffCollections(
    _In_ wfc::IIterable<IInspectable*>* i1,
    _In_ wfc::IIterable<IInspectable*>* i2,
    _Outptr_ wfc::IVector<IInspectable*>** in1Not2,
    _Outptr_ wfc::IVector<IInspectable*>** in2Not1)
{
    HRESULT hr = S_OK;
    UINT32 count = 0;
    BOOLEAN found = FALSE;
    wrl::ComPtr<wfc::IIterator<IInspectable*>> iter1;
    BOOLEAN iter1HasCurrent = FALSE;
    wrl::ComPtr<IInspectable> spCurrent1;
    wrl::ComPtr<wfc::IIterator<IInspectable*>> iter2;
    BOOLEAN iter2HasCurrent = FALSE;
    wrl::ComPtr<IInspectable> spCurrent2;
    wrl::ComPtr<wfc::IVector<IInspectable*>> spIn1Not2;
    wrl::ComPtr<wfc::IVector<IInspectable*>> spIn2Not1;
    wrl::ComPtr<wfci_::Vector<IInspectable*>> spCollection;

    *in1Not2 = nullptr;
    *in2Not1 = nullptr;

    IFC(wfci_::Vector<IInspectable*>::Make(spCollection.ReleaseAndGetAddressOf()));
    IFC(spCollection.As(&spIn1Not2));
    IFC(wfci_::Vector<IInspectable*>::Make(spCollection.ReleaseAndGetAddressOf()));
    IFC(spCollection.As(&spIn2Not1));

    // We start out assuming that none of the elements in the first collection are
    // not in the second, and that all of the elements in the second collection are
    // not in the first.
    IFC(i2->First(&iter2));
    IFC(iter2->get_HasCurrent(&iter2HasCurrent));
    while (iter2HasCurrent)
    {
        IFC(iter2->get_Current(spCurrent2.ReleaseAndGetAddressOf()));
        IFC(spIn2Not1->Append(spCurrent2.Get()));
        IFC(iter2->MoveNext(&iter2HasCurrent));
    }

    // Then we go through the collections and correct, adding the elements that are
    // in the first collection and not in the second, and removing the elements that are
    // in the second and the first.
    IFC(i1->First(&iter1));
    IFC(iter1->get_HasCurrent(&iter1HasCurrent));

    while (iter1HasCurrent)
    {
        found = FALSE;
        IFC(iter1->get_Current(spCurrent1.ReleaseAndGetAddressOf()));
        IFC(spIn2Not1->get_Size(&count));

        for (UINT32 i = 0; i < count; i++)
        {
            IFC(spIn2Not1->GetAt(i, spCurrent2.ReleaseAndGetAddressOf()));
            if (spCurrent1.Get() == spCurrent2.Get())
            {
                found = TRUE;
                IFC(spIn2Not1->RemoveAt(i));
                break;
            }
        }

        if (!found)
        {
            IFC(spIn1Not2->Append(spCurrent1.Get()));
        }

        IFC(iter1->MoveNext(&iter1HasCurrent));
    }

    IFC(spIn1Not2.CopyTo(in1Not2));
    IFC(spIn2Not1.CopyTo(in2Not1));

Cleanup:
    RRETURN(hr);
}

}}}} XAML_ABI_NAMESPACE_END