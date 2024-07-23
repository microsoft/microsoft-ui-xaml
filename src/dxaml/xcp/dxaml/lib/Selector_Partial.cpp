// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Selector.g.h"
#include "SelectorAutomationPeer.g.h"
#include "ScrollViewer.g.h"
#include "ItemCollection.g.h"
#include "SelectionChangedEventArgs.g.h"
#include "SelectorItem.g.h"
#include "SelectorItemAutomationPeer.g.h"
#include "ContentControl.g.h"
#include "ScrollContentPresenter.g.h"
#include "ModernCollectionBasePanel.g.h"
#include "StackPanel.g.h"
#include "ItemsPresenter.g.h"
#include "VirtualizingStackPanel.g.h"
#include "ItemIndexRange_Partial.h"
#include "PropertyPathParser.h"
#include "PropertyPath.h"
#include "Callback.h"

using namespace xaml_data;
using namespace DirectUI;
using namespace DirectUISynonyms;

// Uncomment to output Selector debugging information
//#define SLTR_DEBUG

Selector::Selector() :
    m_focusedIndex(-1),
    m_lastFocusedIndex(0),
    m_fUpdatingCurrentItemInCollectionView(false),
    m_fSynchronizeCurrentItem(false),
    m_bSelectionChangeCausedBySelectedValuePathPropertyChange(false),
    m_bCoercingSelectedeValueToNull(false),
    m_pInitializingData(NULL),
    m_pOldSelectedIndexToReport(NULL),
    m_isSelectionReentrancyLocked(false),
    m_skipScrollIntoView(false),
    m_inCollectionChange(false),
    m_skipFocusSuggestion(false)
{
}

Selector::~Selector()
{
    auto spSelectedItems = m_tpSelectedItemsImpl.GetSafeReference();
    if (spSelectedItems)
    {
        IGNOREHR(spSelectedItems->remove_VectorChanged(m_SelectedItemsVectorChangedToken));
    }

    if (m_focusEnagedToken.value)
    {
        VERIFYHR(this->remove_FocusEngaged(m_focusEnagedToken));
    }

    auto spMonitoredCV = m_tpMonitoredCV.GetSafeReference();
    if (spMonitoredCV)
    {
        VERIFYHR(DisconnectFromMonitoredCVCore(spMonitoredCV.Get()));
    }

    delete m_pInitializingData;
    m_pInitializingData = NULL;
}

// Prepares object's state
_Check_return_
    HRESULT
    Selector::Initialize()
{
    HRESULT hr = S_OK;

    ctl::ComPtr<wfc::VectorChangedEventHandler<IInspectable*>> spSelectedItemsVectorChangedHandler;
    ctl::ComPtr<ObservableTrackerCollection<IInspectable*>> spSelectedItems;
    ctl::ComPtr<TrackerCollection<xaml_data::ItemIndexRange*>> spSelectedRanges;
    ctl::ComPtr<wf::ITypedEventHandler<xaml::Controls::Control*, xaml::Controls::FocusEngagedEventArgs*>> spFocusEngagedHandler;
    IFC(SelectorGenerated::Initialize());

    // Provide our Selection instance a SelectionChangeApplier so we can forward selection change
    // notifications to our SelectorItems.
    IFC(m_selection.Initialize(static_cast<SelectionChangeApplier*>(this)));

    IFC(ctl::make < ObservableTrackerCollection < IInspectable* >> (&spSelectedItems));
    SetPtrValue(m_tpSelectedItemsImpl, spSelectedItems);

    spSelectedItemsVectorChangedHandler.Attach(
        new ClassMemberEventHandler <
        Selector,
        xaml_primitives::ISelector,
        wfc::VectorChangedEventHandler<IInspectable*>,
        wfc::IObservableVector<IInspectable*>,
        wfc::IVectorChangedEventArgs > (this, &Selector::OnSelectedItemsCollectionChanged, true /* subscribeToSelf */ ));

    IFC(m_tpSelectedItemsImpl->add_VectorChanged(spSelectedItemsVectorChangedHandler.Get(), &m_SelectedItemsVectorChangedToken));

    IFC(add_FocusEngaged(
        wrl::Callback<wf::ITypedEventHandler<xaml_controls::Control*, xaml::Controls::FocusEngagedEventArgs*>>(this, &Selector::OnFocusEngaged).Get(),
        &m_focusEnagedToken));

    IFC(ctl::make(&spSelectedRanges));
    SetPtrValue(m_tpSelectedRangesImpl, spSelectedRanges);

Cleanup:
    return hr;
}

// Invoked whenever application code or internal processes call
// ApplyTemplate.
IFACEMETHODIMP
    Selector::OnApplyTemplate()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDependencyObject> spElementScrollViewerAsDO;
    ctl::ComPtr<xaml_controls::IScrollViewer> spElementScrollViewer;

    IFC(SelectorGenerated::OnApplyTemplate());

    // Get the parts
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"ScrollViewer")).Get(), &spElementScrollViewerAsDO));
    spElementScrollViewer = spElementScrollViewerAsDO.AsOrNull<xaml_controls::IScrollViewer>();
    SetPtrValue(m_tpScrollViewer, spElementScrollViewer);
    if (m_tpScrollViewer)
    {
        m_tpScrollViewer.Cast<ScrollViewer>()->put_TemplatedParentHandlesScrolling(TRUE);
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Selector::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spOldValue;
    ctl::ComPtr<IInspectable> spNewValue;

    IFC(SelectorGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::Selector_SelectedIndex:
        IFC(CValueBoxer::UnboxObjectValue(args.m_pOldValue, /* pTargetType */ nullptr, &spOldValue));
        IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, /* pTargetType */ nullptr, &spNewValue));
        IFC(OnSelectedIndexChanged(spOldValue.Get(), spNewValue.Get()));
        break;
    case KnownPropertyIndex::Selector_SelectedItem:
        IFC(CValueBoxer::UnboxObjectValue(args.m_pOldValue, /* pTargetType */ nullptr, &spOldValue));
        IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, /* pTargetType */ nullptr, &spNewValue));
        IFC(OnSelectedItemChanged(spOldValue.Get(), spNewValue.Get()));
        break;
    case KnownPropertyIndex::Selector_IsSynchronizedWithCurrentItem:
        IFC(CValueBoxer::UnboxObjectValue(args.m_pOldValue, /* pTargetType */ nullptr, &spOldValue));
        IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, /* pTargetType */ nullptr, &spNewValue));
        IFC(OnIsSynchronizedWithCurrentItemChanged(spOldValue.Get(), spNewValue.Get()));
        break;
    case KnownPropertyIndex::Selector_SelectedValue:
        IFC(CValueBoxer::UnboxObjectValue(args.m_pOldValue, /* pTargetType */ nullptr, &spOldValue));
        IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, /* pTargetType */ nullptr, &spNewValue));
        IFC(OnSelectedValueChanged(spOldValue.Get(), spNewValue.Get()));
        break;
    case KnownPropertyIndex::Selector_SelectedValuePath:
        IFC(CValueBoxer::UnboxObjectValue(args.m_pOldValue, /* pTargetType */ nullptr, &spOldValue));
        IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, /* pTargetType */ nullptr, &spNewValue));
        IFC(OnSelectedValuePathChanged(spOldValue.Get(), spNewValue.Get()));
        break;
    case KnownPropertyIndex::ItemsControl_ItemsHost:
        // If Items host changes we need to make sure scrolling components are re-hooked.
        // For example if the ItemsPanelTemplate changes from horizontally aligned VSP to vertically aligned VSP,
        // we need to hook the new VSP as ScrollInfo.
        if (m_tpScrollViewer && m_tpScrollViewer.Cast<ScrollViewer>()->m_trElementScrollContentPresenter.Get())
        {
            IFC(GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
                ctl::ComPtr<Selector>(this),
                &Selector::OnItemsHostChanged)));
        }
        break;
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP Selector::OnKeyDown(
    _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isHandled = FALSE;

    // Ignore already handled events
    IFCPTR(pArgs);
    IFC(SelectorGenerated::OnKeyDown(pArgs));
    IFC(pArgs->get_Handled(&isHandled));
    if (isHandled)
    {
        goto Cleanup;
    }

    // If this is a zoom hotkey, forward it to our ScrollViewer.
    if (m_tpScrollViewer)
    {
        wsy::VirtualKey key = wsy::VirtualKey_None;
        wsy::VirtualKeyModifiers modifiers = wsy::VirtualKeyModifiers_None;
        ZoomDirection messageZoomDirection = ZoomDirection_None;

        IFC(GetKeyboardModifiers(&modifiers));
        IFC(pArgs->get_Key(&key));
        messageZoomDirection = ScrollViewer::GetKeyboardMessageZoomAction(modifiers, key);
        if (messageZoomDirection != ZoomDirection_None)
        {
            IFC(m_tpScrollViewer.Cast<ScrollViewer>()->ProcessPureInertiaInputMessage(messageZoomDirection, &isHandled));
        }
    }

    // Update when we've handled the event
    if (isHandled)
    {
        IFC(pArgs->put_Handled(isHandled));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Selector::OnSelectedValueChanged(
    _In_ IInspectable* pOldValue,
    _In_ IInspectable* pNewValue)
{
    HRESULT hr = S_OK;

    INT indexOfItemWithValue = -1;
    INT selectedIndex = -1;
    BOOLEAN undoChange = FALSE;
    BOOLEAN bHasItems = FALSE;

    if (!m_bCoercingSelectedeValueToNull)
    {
        ctl::ComPtr<IInspectable> spItemWithValue;

        // Avoid recursion.  If we are updating item in response to a change in the index then don't do any additional processing.  Also side-exit when parsing XAML because we defer processing the selection until after all the content has been loaded.
        if (!IsSelectionReentrancyAllowed() || IsInit())
        {
            goto Cleanup;
        }

        IFC(FindIndexOfItemWithValue(pNewValue, indexOfItemWithValue, &spItemWithValue));
        IFC(get_SelectedIndex(&selectedIndex));

        IFC(SelectJustThisItemInternal(selectedIndex, indexOfItemWithValue, spItemWithValue.Get(), FALSE /*animateIfBringIntoView*/, &undoChange));

        IFC(HasItems(bHasItems));

        // If setting SelectedValue does not result in item selection
        // then set coerce SelectedValue to null if there is an ItemsSource set
        if (pNewValue && indexOfItemWithValue == -1 && bHasItems)
        {
            IFC(CoerceSelectedValueToNull());
        }
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT Selector::CoerceSelectedValueToNull()
{
    HRESULT hr = S_OK;
    m_bCoercingSelectedeValueToNull = true;
    IFC(put_SelectedValue(NULL));

Cleanup:
    m_bCoercingSelectedeValueToNull = false;
    RRETURN(hr);
}

_Check_return_ HRESULT Selector::OnItemsHostChanged()
{
    HRESULT hr = S_OK;

    // If Items host changes we need to make sure scrolling components are re-hooked.
    // For example if the ItemsPanelTemplate changes from horizontally aligned VSP to vertically aligned VSP,
    // we need to hook the new VSP as ScrollInfo.
    if (m_tpScrollViewer && m_tpScrollViewer.Cast<ScrollViewer>()->m_trElementScrollContentPresenter.Get())
    {
        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->HookupScrollingComponents());
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT Selector::OnSelectedValuePathChanged(
    _In_ IInspectable* pOldValue,
    _In_ IInspectable* pNewValue)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<IInspectable> spSelectedValue;
    ctl::ComPtr<IInspectable> spNewSelectedItem;
    wrl_wrappers::HString strNewPath;
    PropertyPathParser* pPropertyPathParser = NULL;
    INT selectedIndex = -1;
    INT newSelectedIndex = -1;
    BOOLEAN undoChange = FALSE;

    IFC(ctl::do_get_value(*strNewPath.GetAddressOf(), pNewValue));
    m_tpSelectedValuePropertyPathListener.Clear();

    if (strNewPath.Get())
    {
        ctl::ComPtr<PropertyPathListener> spPropertyPathListener;

        pPropertyPathParser = new PropertyPathParser();
        IFC(pPropertyPathParser->SetSource(const_cast<WCHAR*>(strNewPath.GetRawBuffer(NULL)), FALSE));
        IFC(ctl::make<PropertyPathListener>(nullptr, pPropertyPathParser, false, false, &spPropertyPathListener));
        SetPtrValue(m_tpSelectedValuePropertyPathListener, spPropertyPathListener);
    }

    // If we are updating item in response to a change in the index then don't do any additional processing.  Also side-exit when parsing XAML because we defer processing the selection until after all the content has been loaded.
    if (IsInit())
    {
        goto Cleanup;
    }

    IFC(get_SelectedIndex(&selectedIndex));
    IFC(get_SelectedValue(&spSelectedValue));
    m_bSelectionChangeCausedBySelectedValuePathPropertyChange = true;
    IFC(FindIndexOfItemWithValue(spSelectedValue.Get(), newSelectedIndex, &spNewSelectedItem));

    IFC(SelectJustThisItemInternal(selectedIndex, newSelectedIndex, spNewSelectedItem.Get(), FALSE /*animateIfBringIntoView*/, &undoChange));

Cleanup:
    m_bSelectionChangeCausedBySelectedValuePathPropertyChange = false;
    delete pPropertyPathParser;
    return hr;
}

/// Returns the selected value of an item using a path.
_Check_return_ HRESULT Selector::GetSelectedValue(
    _In_ IInspectable* pItem,
    _Outptr_result_maybenull_ IInspectable** pSelectedValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pSelectedValue);
    *pSelectedValue = NULL;

    if (!m_tpSelectedValuePropertyPathListener
        || pItem == NULL)
    {
        ctl::ComPtr<IInspectable> spItem = pItem;
        IFC(spItem.MoveTo(pSelectedValue));
        goto Cleanup;
    }

    IFC(m_tpSelectedValuePropertyPathListener->SetSource(pItem));
    IFC(m_tpSelectedValuePropertyPathListener->GetValue(pSelectedValue));

Cleanup:
    RRETURN(hr);
}

/// Finds the index of the first item with a given property path value.
_Check_return_ HRESULT Selector::FindIndexOfItemWithValue(
    _In_ IInspectable* pValue,
    _Out_ INT &index,
    _Outptr_ IInspectable** pItemWithValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
    bool areValuesEqual = false;
    UINT nCount = 0;
    index = -1;

    IFCPTR(pItemWithValue);
    *pItemWithValue = NULL;

    IFC(get_Items(&spItems));
    IFC(spItems.Cast<ItemCollection>()->get_Size(&nCount));

    for (UINT cnt = 0; cnt < nCount; cnt++)
    {
        ctl::ComPtr<IInspectable> spItem;
        ctl::ComPtr<IInspectable> spItemValue;

        IFC(spItems.Cast<ItemCollection>()->GetAt(cnt, &spItem));
        IFC(GetSelectedValue(spItem.Get(), &spItemValue));
        IFC(PropertyValue::AreEqual(spItemValue.Get(), pValue, &areValuesEqual));
        if (areValuesEqual)
        {
            index = cnt;
            IFC(spItem.MoveTo(pItemWithValue));
            goto Cleanup;
        }
    }

Cleanup:
    RRETURN(hr);
}

// Reprocesses any SelectedItem or SelectedValue that was set prior to the ItemsSource being set
_Check_return_ HRESULT Selector::OnItemsSourceChanged(
    _In_ IInspectable* pNewValue)
{
    HRESULT hr = S_OK;
    UINT nSelectedCount = 0;
    SelectionChanger* pSelectionChanger = NULL;

    // Evaluate before OnItemsSourceChanged since the value can be modified during the call.
    bool hasPendingSelectedIndex = m_selectedIndexValueSetBeforeItemsAvailable >= 0;

    IFC(SelectorGenerated::OnItemsSourceChanged(pNewValue));

    IFC(BeginChange(&pSelectionChanger));

    // If we do not have a pending selected index, clear selection
    if (!hasPendingSelectedIndex)
    {
        IFC(m_selection.GetNumItemsSelected(nSelectedCount));
        for (UINT i = 0; i < nSelectedCount; ++i)
        {
            ctl::ComPtr<IInspectable> spItem;
            UINT itemIndex = 0;

            IFC(m_selection.GetAt(i, &spItem));
            IFC(m_selection.GetIndexAt(i, itemIndex));
            IFC(pSelectionChanger->Unselect(itemIndex, spItem.Get()));
        }
    }

    // If we're monitoring a CV and we're synchronizing the selected item
    // with the current item then select it here
    if (m_tpMonitoredCV && m_fSynchronizeCurrentItem)
    {
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
        ctl::ComPtr<IInspectable> spItem;
        UINT nCount = 0;
        INT iCurrentPosition = -1;

        IFC(m_tpMonitoredCV.Get()->get_CurrentItem(&spItem));
        IFC(m_tpMonitoredCV.Get()->get_CurrentPosition(&iCurrentPosition));

        IFC(get_Items(&spItems));
        IFC(spItems.Cast<ItemCollection>()->get_Size(&nCount));

        // we should add current to the selection only if it is within collection range
        if (iCurrentPosition > -1 && iCurrentPosition < (INT) nCount)
        {
            IFC(pSelectionChanger->Select(iCurrentPosition, spItem.Get(), FALSE /* canSelectMultiple */));
        }
#if DBG
        else
        {
            // if current index out of range then we shouldn't get any item from currency.
            ASSERT(!spItem);
        }
#endif
    }

    IFC(SelectAllSelectedSelectorItems(pSelectionChanger));
    IFC(EndChange(pSelectionChanger));
    pSelectionChanger = NULL;

    if (pNewValue && m_itemPendingSelection)
    {
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> items;
        UINT newIndex;
        BOOLEAN itemFound;

        IFC(get_Items(&items));

        // Check if we're trying to restore a value that's not in the collection.
        IFC(items.Cast<ItemCollection>()->IndexOf(m_itemPendingSelection.Get(), &newIndex, &itemFound));
        if (itemFound)
        {
            IFC(put_SelectedItem(m_itemPendingSelection.Get()));
        }

        m_itemPendingSelection.Clear();
    }

Cleanup:
    if (pSelectionChanger != NULL)
    {
        VERIFYRETURNHR(pSelectionChanger->Cancel());
    }
    return hr;
}

_Check_return_ HRESULT Selector::SelectAllSelectedSelectorItems(
    _In_ SelectionChanger* pSelectionChanger)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
    ctl::ComPtr<xaml::IDataTemplate> spItemTemplate;
    ItemCollection* pItemCollectionNoRef;
    BOOLEAN canSelectMultiple = FALSE;
    BOOLEAN isSelected = FALSE;
    IFC(get_CanSelectMultiple(&canSelectMultiple));

    IFC(get_Items(&spItems));
    pItemCollectionNoRef = spItems.Cast<ItemCollection>();

    // Items source is set from ItemSource, thus if the ItemSource is active
    // we skip this old behavior.
    if (pItemCollectionNoRef->ItemsSourceActive())
    {
        goto Cleanup;
    }

    IFC(get_ItemTemplate(&spItemTemplate));
    if (!spItemTemplate)
    {
        UINT nCount = 0;

        IFC(pItemCollectionNoRef->get_Size(&nCount));
        if (nCount > 0)
        {
            for (UINT i = 0; i < nCount; ++i)
            {
                ctl::ComPtr<IInspectable> spItem;
                ctl::ComPtr<ISelectorItem> spSelectorItem;

                IFC(spItems.Cast<ItemCollection>()->GetAt(i, &spItem));
                spSelectorItem = spItems.AsOrNull<ISelectorItem>();
                if (spSelectorItem)
                {
                    isSelected = FALSE;
                    IFC(spSelectorItem.Cast<SelectorItem>()->get_IsSelected(&isSelected));
                    if (isSelected)
                    {
                        IFC(pSelectionChanger->Select(i, spItem.Get(), canSelectMultiple));
                    }
                }
            }
        }
    }

Cleanup:
    return hr;
}

_Check_return_
    HRESULT
    Selector::OnSelectedIndexChanged(
    _In_ IInspectable* pOldValue,
    _In_ IInspectable* pNewValue)
{
    HRESULT hr = S_OK;

    INT oldValue = -1;
    INT newValue = -1;
    SelectionChanger* pSelectionChanger = NULL;

    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
    UINT nCount = 0;
    BOOLEAN undoChange = FALSE;
    BOOLEAN outOfRange = FALSE;

    // Avoid recursion.  If we are updating index in response to a change in the item then don't do any additional processing.
    if (!IsSelectionReentrancyAllowed() || IsInit())
    {
        goto Cleanup;
    }

    IFC(ctl::do_get_value(oldValue, pOldValue));
    IFC(ctl::do_get_value(newValue, pNewValue));

    IFC(get_Items(&spItems));
    IFC(spItems.Cast<ItemCollection>()->get_Size(&nCount));

    if (newValue >= -1 && newValue < (INT) nCount)
    {
        // See if the change can actually take place, if not we need
        // to undo it
        IFC(SelectJustThisItemInternal(oldValue, newValue, nullptr, FALSE /*animateIfBringIntoView*/, &undoChange));

        // We got a selected index that is valid, so throw away the
        // stored pending value that we wanted to apply.
        m_selectedIndexValueSetBeforeItemsAvailable = -1;
    }
    else
    {
        // The index is invalid so we need to undo the change
        undoChange = TRUE;
        outOfRange = TRUE;
    }

    // Undo the change if required
    if (undoChange)
    {
        // Don't modify SelectedItems
        HRESULT hr2 = S_OK;
        hr2 = BeginChange(&pSelectionChanger);
        if (SUCCEEDED(hr2))
        {
            hr2 = put_SelectedIndex(oldValue);
        }
        IFC(pSelectionChanger->Cancel());
        pSelectionChanger = NULL;
        IFC(hr2);

        if (outOfRange)
        {
            if (nCount == 0)
            {
                // No Items have been added yet, so store the index and
                // set the selection once we get the items.
                m_selectedIndexValueSetBeforeItemsAvailable = newValue;
            }
            else
            {
                // Only throw if the value was actually out of range and there are already existing items.
                // Un the case of the collection view cancelling the change
                // this is perfectly fine.
                IFC(E_INVALIDARG);
            }
        }
    }

Cleanup:
    if (pSelectionChanger != NULL)
    {
        VERIFYRETURNHR(pSelectionChanger->Cancel());
    }
    return hr;
}

_Check_return_
    HRESULT
    Selector::OnSelectedItemChanged(
    _In_ IInspectable* pOldValue,
    _In_ IInspectable* pNewValue)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
    UINT newIndex = 0;
    INT newSelectedIndex = -1;
    INT oldSelectedIndex = -1;
    BOOLEAN undoChange = FALSE;
    BOOLEAN bItemFound = FALSE;
    SelectionChanger* pSelectionChanger = NULL;

    // Avoid recursion.  If we are updating index in response to a change in the item then don't do any additional processing.
    if (!IsSelectionReentrancyAllowed() || IsInit())
    {
        goto Cleanup;
    }

    IFC(get_Items(&spItems));

    // Check if we're trying to set this to a value that's not in the collection
    IFC(spItems.Cast<ItemCollection>()->IndexOf(pNewValue, &newIndex, &bItemFound));
    if (bItemFound)
    {
        newSelectedIndex = newIndex;
    }

    // Custom values will not be found in the item collection prevent undoing the change in these cases.
    if (!bItemFound && pNewValue && !AreCustomValuesAllowed())
    {
        undoChange = TRUE;

        ctl::ComPtr<IInspectable> itemsSource;
        IFC(get_ItemsSource(&itemsSource));
        if (!itemsSource)
        {
            SetPtrValue(m_itemPendingSelection, pNewValue);
        }
    }
    else
    {
        IFC(get_SelectedIndex(&oldSelectedIndex));

        // If we couldn't change the selection that means that we need
        // to undo the change
        IFC(SelectJustThisItemInternal(oldSelectedIndex, newSelectedIndex, pNewValue, FALSE /*animateIfBringIntoView*/, &undoChange));
    }

    // If we need to undo the change do so here
    if (undoChange)
    {
        // Don't modify SelectedItems
        HRESULT hr2 = S_OK;
        hr2 = BeginChange(&pSelectionChanger);
        if (SUCCEEDED(hr2))
        {
            // Restore old value
            hr2 = put_SelectedItem(pOldValue);
        }

        IFC(pSelectionChanger->Cancel());
        pSelectionChanger = NULL;
        IFC(hr2);
    }

Cleanup:
    if (pSelectionChanger != NULL)
    {
        VERIFYRETURNHR(pSelectionChanger->Cancel());
    }
    return hr;
}

_Check_return_ HRESULT Selector::UpdateCVSynchronizationState()
{
    HRESULT hr = S_OK;
    const bool fOldSync = m_fSynchronizeCurrentItem;

    IFC(UpdateIsSynchronized());

    // If we didn't change state then we're done
    if (fOldSync == m_fSynchronizeCurrentItem)
    {
        // make sure that the SelectedItem is not synchronized
        if (!m_fSynchronizeCurrentItem)
        {
            IFC(put_SelectedItem(nullptr));
        }
        goto Cleanup;
    }

    // We're starting to synchronize
    if (!fOldSync && m_fSynchronizeCurrentItem)
    {
        ctl::ComPtr<IInspectable> spCurrentItem;

        ASSERT(m_tpMonitoredCV != nullptr); // In order for m_fSynchronizeCurrentItem to be true we must have a CV

        IFC(m_tpMonitoredCV.Get()->get_CurrentItem(&spCurrentItem));
        IFC(put_SelectedItem(spCurrentItem.Get()));
    }
    // We're stopping the synchronization
    else
    {
        IFC(put_SelectedItem(nullptr));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Selector::OnIsSynchronizedWithCurrentItemChanged(
    _In_ IInspectable* pOldValue,
    _In_ IInspectable* pNewValue)
{
    HRESULT hr = S_OK;
    BOOLEAN fValue = false;

    // Validate that the value is what we expect
    if (pNewValue != NULL)
    {
        // This must be an IPropertyValue and it must be a BOOLEAN
        IFC(ctl::do_get_value(fValue, pNewValue));
        if (fValue != false)
        {
            // We only accept FALSE or NULL as values for this property
            IFC(E_INVALIDARG);
        }
    }

    IFC(UpdateCVSynchronizationState());

Cleanup:

    RRETURN(hr);
}

// List of all items that are currently selected
_Check_return_
    HRESULT
    Selector::get_SelectedItemsInternal(
    _Outptr_ wfc::IVector<IInspectable*>** pValue)
{
    if (!m_tpDataSourceAsSelectionInfo)
    {
        IFC_RETURN(m_tpSelectedItemsImpl.CopyTo(pValue));
    }

    return S_OK;
}

// List of all ranges that are currently selected
_Check_return_
    HRESULT
    Selector::get_SelectedRangesInternal(
    _Outptr_ wfc::IVectorView<xaml_data::ItemIndexRange*>** pValue)
{
    if (m_tpDataSourceAsSelectionInfo)
    {
        IFC_RETURN(m_tpDataSourceAsSelectionInfo->GetSelectedRanges(pValue));
    }
    else
    {
        IFC_RETURN(m_tpSelectedRangesImpl.Cast<TrackerCollection<xaml_data::ItemIndexRange*>>()->GetView(pValue));
    }

    return S_OK;
}

// Whether or not this Selector allows multiple selection
_Check_return_
    HRESULT
    Selector::get_CanSelectMultiple(
    _Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    IFCPTR(pValue);
    *pValue = FALSE;

Cleanup:
    RRETURN(hr);
}

_Check_return_
    HRESULT
    Selector::OnSelectedItemsCollectionChanged(
    _In_ wfc::IObservableVector<IInspectable*>* pSender,
    _In_ wfc::IVectorChangedEventArgs* e)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spSelectedItem;
    ctl::ComPtr<IInspectable> spReplacedItem;
    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
    wfc::CollectionChange action =
        wfc::CollectionChange_Reset;
    UINT index = 0;
    BOOLEAN canSelectMultiple = FALSE;
    UINT itemIndex = 0;
    SelectionChanger* pSelectionChanger = NULL;

    if (!IsSelectionReentrancyAllowed() || IsInit() || m_tpDataSourceAsSelectionInfo)
    {
        goto Cleanup;
    }

    IFC(get_CanSelectMultiple(&canSelectMultiple));

    // TODO: Report Error Resx.Selector_CannotModifySelectedItems
    // Bug#95997
    IFCEXPECT(canSelectMultiple);

    IFC(e->get_CollectionChange(&action));

    IFC(BeginChange(&pSelectionChanger));

    switch (action)
    {
    case wfc::CollectionChange_Reset:
        {
            INT selectedIndex = -1;
            IFC(get_SelectedIndex(&selectedIndex));
            IFC(SelectJustThisItem(selectedIndex, -1, pSelectionChanger, NULL, NULL));
            break;
        }

    case wfc::CollectionChange_ItemInserted:
        {
            BOOLEAN found = FALSE;
            IFC(e->get_Index(&index));
            IFC(m_tpSelectedItemsImpl.Cast < ObservableTrackerCollection < IInspectable* >> ()->GetAt(index, &spSelectedItem));
            IFC(get_Items(&spItems));
            IFC(spItems.Cast<ItemCollection>()->IndexOf(spSelectedItem.Get(), &itemIndex, &found));

            // we have to start the selection process even if new item is not in Items
            // at the end "non-selectable" item will be removed from SelectedItems collection.
            if (found)
            {
                IFC(pSelectionChanger->Select(itemIndex, spSelectedItem.Get(), canSelectMultiple));
            }
            break;
        }

    case wfc::CollectionChange_ItemRemoved:
        {
            IFC(e->get_Index(&index));
            IFC(m_selection.GetAt(index, &spReplacedItem));
            IFC(m_selection.GetIndexAt(index, itemIndex));

            IFC(pSelectionChanger->Unselect(itemIndex, spReplacedItem.Get()));
            break;
        }

    case wfc::CollectionChange_ItemChanged:
        {
            BOOLEAN found = FALSE;
            IFC(e->get_Index(&index));
            IFC(m_selection.GetAt(index, &spReplacedItem));
            IFC(m_selection.GetIndexAt(index, itemIndex));

            IFC(pSelectionChanger->Unselect(itemIndex, spReplacedItem.Get()));

            IFC(m_tpSelectedItemsImpl.Cast < ObservableTrackerCollection < IInspectable* >> ()->GetAt(index, &spSelectedItem));
            IFC(get_Items(&spItems));
            IFC(spItems.Cast<ItemCollection>()->IndexOf(spSelectedItem.Get(), &itemIndex, &found));
            if (found)
            {
                IFC(pSelectionChanger->Select(itemIndex, spSelectedItem.Get(), canSelectMultiple));
            }
            break;
        }
    }

    IFC(EndChange(pSelectionChanger));
    pSelectionChanger = NULL;

Cleanup:
    if (pSelectionChanger != NULL)
    {
        VERIFYRETURNHR(pSelectionChanger->Cancel());
    }
    return hr;
}

_Check_return_
    HRESULT
    Selector::InvokeSelectionChanged(
    _In_opt_ wfc::IVector<IInspectable*>* pUnselectedItems,
    _In_opt_ wfc::IVector<IInspectable*>* pSelectedItems)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<SelectionChangedEventArgs> spArgs;
    BOOLEAN bAutomationListener = FALSE;

    // Create the args
    IFC(ctl::make<SelectionChangedEventArgs>(&spArgs));

    IFC(spArgs->put_RemovedItems(pUnselectedItems));
    IFC(spArgs->put_AddedItems(pSelectedItems));

    IFC(OnSelectionChanged(spArgs.Get()));

    IFC(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_SelectionPatternOnInvalidated, &bAutomationListener));
    if (!bAutomationListener)
    {
        IFC(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_SelectionItemPatternOnElementSelected, &bAutomationListener));
    }
    if (!bAutomationListener)
    {
        IFC(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_SelectionItemPatternOnElementAddedToSelection, &bAutomationListener));
    }
    if (!bAutomationListener)
    {
        IFC(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_SelectionItemPatternOnElementRemovedFromSelection, &bAutomationListener));
    }
    if (bAutomationListener)
    {
        ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeer;
        IFC(GetOrCreateAutomationPeer(&spAutomationPeer));
        if (spAutomationPeer)
        {
            ctl::ComPtr<xaml_automation_peers::ISelectorAutomationPeer> spSelectorAutomationPeer;

            IFC(spAutomationPeer.As(&spSelectorAutomationPeer));
            IFC(spSelectorAutomationPeer.Cast<SelectorAutomationPeer>()->RaiseSelectionEvents(spArgs.Get()));
        }
    }

Cleanup:
    return hr;
}

_Check_return_
    HRESULT
    Selector::OnSelectionChanged(
    _In_ xaml_controls::ISelectionChangedEventArgs* pSelectionChangedEventArgs)
{
    RRETURN(RaiseSelectionChanged(pSelectionChangedEventArgs));
}

_Check_return_
    HRESULT
    Selector::OnSelectionChanged(
    _In_ INT oldSelectedIndex,
    _In_ INT newSelectedIndex,
    _In_ IInspectable* pOldSelectedItem,
    _In_ IInspectable* pNewSelectedItem,
    _In_ BOOLEAN animateIfBringIntoView,
    _In_ xaml_input::FocusNavigationDirection focusNavigationDirection)
{
    HRESULT hr = S_OK;

    if (newSelectedIndex != -1)
    {
        // Only change the focus if there is a selected item.
        // Use InputActivationBehavior::NoActivate because just changing selected item by default shouldn't steal activation from another window/island.
        IFC(SetFocusedItem(newSelectedIndex, TRUE /*shouldScrollIntoView*/, animateIfBringIntoView, focusNavigationDirection, InputActivationBehavior::NoActivate));
    }

Cleanup:
    RRETURN(hr);
}

// Raise SelectionChanged event
_Check_return_
    HRESULT
    Selector::RaiseSelectionChanged(
    _In_ xaml_controls::ISelectionChangedEventArgs* pSelectionChangedEventArgs)
{
    HRESULT hr = S_OK;
    SelectionChangedEventSourceType* pEventSource = nullptr;

    IFC(GetSelectionChangedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), pSelectionChangedEventArgs));

Cleanup:
    RRETURN(hr);
}

// Handler for when the Items collection is changed
_Check_return_
    HRESULT
    Selector::NotifyOfSourceChanged(
    _In_ wfc::IObservableVector<IInspectable*>* pSender,
    _In_ wfc::IVectorChangedEventArgs* e) noexcept
{
    HRESULT hr = S_OK;
    wfc::CollectionChange action =
        wfc::CollectionChange_Reset;

    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
    ctl::ComPtr<IInspectable> spItem;
    ctl::ComPtr<ISelectorItem> spSelectorItem = NULL;
    SelectionChanger* pSelectionChanger = NULL;

    // If a collection change lead to a selection change, we should not bring into view the selected item.
    m_inCollectionChange = true;

    IFC(e->get_CollectionChange(&action));

    // in case of Remove we can show animation where container will be around and uncleared.
    // all pointer actions shouldn't go to its parent selector. Clear it here instead of in ClearContainerForItem.
    if (action == wfc::CollectionChange_ItemRemoved)
    {
        UINT itemIndex = 0;
        ctl::ComPtr<IDependencyObject> spContainer;

        IFC(e->get_Index(&itemIndex));

        IFC(ContainerFromIndex(itemIndex, &spContainer));
        IFC(spContainer.As<ISelectorItem>(&spSelectorItem));

        if (spContainer)
        {
            IFC(spSelectorItem.Cast<SelectorItem>()->SetParentSelector(nullptr));
        }
    }

    if (action != wfc::CollectionChange_Reset)
    {
        // to limit impact, for all but reset change notifications do not change invocation order
        IFC(SelectorGenerated::NotifyOfSourceChanged(pSender, e));
    }

    IFC(BeginChange(&pSelectionChanger));

    switch (action)
    {
    case wfc::CollectionChange_Reset:
        {
            UINT nSelectedCount = 0;
            UINT nNewItemCount = 0;
            bool arePropertyValuesEqual = false;
            ctl::ComPtr<xaml::IDependencyObject> spFocusedContainer;

            if (GetFocusedIndex() > -1)
            {
                // before we tell the panel about the reset, let's get the focused container
                IFC(ContainerFromIndex(GetFocusedIndex(), &spFocusedContainer));
                // set m_focusedIndex to -1 so that we don't get confused during prepare
                SetFocusedIndex(-1); // notice: in a reset, the index is not guaranteed anymore
            }

            IFC(m_selection.GetNumItemsSelected(nSelectedCount));
            IFC(get_Items(&spItems));
            if (spItems)
            {
                IFC(spItems.Cast<ItemCollection>()->get_Size(&nNewItemCount));
            }

            for (UINT i = 0; i < nSelectedCount; ++i)
            {
                ctl::ComPtr<IInspectable> spNewItem;

                UINT itemIndex = 0;
                IFC(m_selection.GetAt(i, &spItem));
                IFC(m_selection.GetIndexAt(i, itemIndex));
                arePropertyValuesEqual = false;
                if (spItems && itemIndex < nNewItemCount)
                {
                    IFC(spItems.Cast<ItemCollection>()->GetAt(itemIndex, &spNewItem));
                    IFC(PropertyValue::AreEqual(spItem.Get(), spNewItem.Get(), &arePropertyValuesEqual));
                }

                if (!arePropertyValuesEqual) // Only unselect if the values are not equal.
                {
                    IFC(pSelectionChanger->Unselect(itemIndex, spItem.Get()));
                }
            }

            IFC(SelectAllSelectedSelectorItems(pSelectionChanger));
            IFC(EndChange(pSelectionChanger));
            pSelectionChanger = NULL;

            // for reset, call into super-class after selection was updated.
            // modern panels need to have selection in sync with collection, so selection is updated above
            IFC(SelectorGenerated::NotifyOfSourceChanged(pSender, e));

            // focus is interesting. There are resets that are benign. They recreate everything and we can still maintain focus
            bool shouldResetFocus = true;
            if (spFocusedContainer)
            {
                // we used to have focus, maybe after this reset we can restore it somehow?
                INT newFocusedIndex = -1;
                IFC(IndexFromContainer(spFocusedContainer.Get(), &newFocusedIndex));
                if (newFocusedIndex > -1)
                {
                    // well, the old container still has focus, it just might have a different index
                    SetFocusedIndex(newFocusedIndex);
                    shouldResetFocus = false;
                }
            }
            if (shouldResetFocus)
            {
                IFC(SetFocusedItem(-1, FALSE /*shouldScrollIntoView*/));
            }
        }
        break;

    case wfc::CollectionChange_ItemInserted:
        {
            BOOLEAN isSelected = FALSE;
            UINT itemIndex = 0;
            INT focusedIndex = GetFocusedIndex();

            IFC(e->get_Index(&itemIndex));

            // An item was inserted before the focused item, we need
            // to update our focused item's index to reflect that.
            if (focusedIndex >= static_cast<int>(itemIndex))
            {
                ASSERT(focusedIndex != -1);

                focusedIndex++;
                SetFocusedIndex(focusedIndex);
                SetLastFocusedIndex(focusedIndex);
            }

            IFC(m_selection.AddSelectedIndex(itemIndex));

            // At the time an item is inserted, the only way for the item to already be
            // IsSelected == true is for the item to implement ISelectorItem itself, as
            // the item doesn't have a container yet.

            IFC(get_Items(&spItems));
            IFC(spItems.Cast<ItemCollection>()->GetAt(itemIndex, &spItem));
            spSelectorItem = spItem.AsOrNull<ISelectorItem>();

            if (spSelectorItem)
            {
                IFC(spSelectorItem->get_IsSelected(&isSelected));
                if (isSelected)
                {
                    BOOLEAN canSelectMultiple = FALSE;

                    IFC(get_CanSelectMultiple(&canSelectMultiple));
                    IFC(pSelectionChanger->Select(itemIndex, spItem.Get(), canSelectMultiple));
                }
            }

            if (isSelected)
            {
                IFC(EndChange(pSelectionChanger));
                pSelectionChanger = NULL;
            }
            else if (!IsSelectionReentrancyAllowed())
            {
                INT oldSelectedIndex = -1;
                UINT nSize = 0;

                IFC(spItems.Cast<ItemCollection>()->get_Size(&nSize));

                // If the item wasn't added at the end, then check if we need to update the
                // existing selection properties.
                if (itemIndex < (nSize - 1))
                {
                    IFC(get_SelectedIndex(&oldSelectedIndex));
                    if ((INT) itemIndex <= oldSelectedIndex && !IsInit())
                    {
                        ctl::ComPtr<IInspectable> spOldSelectedItem;

                        IFC(get_SelectedItem(&spOldSelectedItem));
                        IFC(UpdatePublicSelectionProperties(oldSelectedIndex, oldSelectedIndex + 1, spOldSelectedItem.Get(), spOldSelectedItem.Get()));
                    }
                }
                IFC(pSelectionChanger->Cancel());
                pSelectionChanger = NULL;
            }
        }
        break;

    case wfc::CollectionChange_ItemChanged:
        {
            BOOLEAN hasItem = FALSE;
            BOOLEAN isSelected = FALSE;
            UINT collectionIndex = 0;
            UINT itemIndex = 0;
            IFC(e->get_Index(&itemIndex));

            IFC(m_selection.Has(itemIndex, collectionIndex, hasItem));

            if (hasItem)
            {
                IFC(m_selection.GetAt(collectionIndex, &spItem));
                IFC(pSelectionChanger->Unselect(itemIndex, spItem.Get()));
            }

            IFC(get_Items(&spItems));
            IFC(spItems.Cast<ItemCollection>()->GetAt(itemIndex, &spItem));

            spSelectorItem = spItem.AsOrNull<ISelectorItem>();

            if (spSelectorItem)
            {
                IFC(spSelectorItem->get_IsSelected(&isSelected));
                if (isSelected)
                {
                    BOOLEAN canSelectMultiple = FALSE;

                    IFC(get_CanSelectMultiple(&canSelectMultiple));
                    IFC(pSelectionChanger->Select(itemIndex, spItem.Get(), canSelectMultiple));
                }
            }

            if (hasItem || isSelected)
            {
                IFC(EndChange(pSelectionChanger));
            }
            else
            {
                IFC(pSelectionChanger->Cancel());
            }
            pSelectionChanger = NULL;
        }
        break;
    }

Cleanup:
    if (pSelectionChanger != NULL)
    {
        VERIFYRETURNHR(pSelectionChanger->Cancel());
    }
    m_inCollectionChange = false;
    return hr;
}

// Handler for when the Items collection is changed
_Check_return_ HRESULT
    Selector::OnItemsChanged(
    _In_ wfc::IVectorChangedEventArgs* e)
{
    HRESULT hr = S_OK;
    wfc::CollectionChange action =
        wfc::CollectionChange_Reset;
    SelectionChanger* pSelectionChanger = NULL;

    UINT index = 0;

    // If a collection change lead to a selection change, we should not bring into view the selected item.
    m_inCollectionChange = true;

    IFC(e->get_CollectionChange(&action));

    switch (action)
    {
    case wfc::CollectionChange_ItemRemoved:
        {
            ctl::ComPtr<IInspectable> spItem;
            BOOLEAN hasItem = FALSE;
            UINT collectionIndex = 0; // Index in selected indexes.
            INT itemIndex = -1; // Index in Items.

            IFC(e->get_Index(&index));
            // need to cast unsigned to signed
            itemIndex = index;

            IFC(BeginChange(&pSelectionChanger));

            IFC(m_selection.Has(itemIndex, collectionIndex, hasItem));
            if (hasItem)
            {
                IFC(m_selection.GetAt(collectionIndex, &spItem));
                IFC(pSelectionChanger->Unselect(itemIndex, spItem.Get()));
            }

            IFC(m_selection.RemoveSelectedIndex(itemIndex));

            if (hasItem)
            {
                IFC(EndChange(pSelectionChanger));
                pSelectionChanger = NULL;
            }
            else if (pSelectionChanger != NULL && !IsSelectionReentrancyAllowed())
            {
                INT oldSelectedIndex = -1;
                IFC(get_SelectedIndex(&oldSelectedIndex));
                if (itemIndex <= oldSelectedIndex && !IsInit())
                {
                    ctl::ComPtr<IInspectable> spOldSelectedItem;

                    IFC(get_SelectedItem(&spOldSelectedItem));
                    IFC(UpdatePublicSelectionProperties(oldSelectedIndex, oldSelectedIndex - 1, spOldSelectedItem.Get(), spOldSelectedItem.Get()));
                }
                IFC(pSelectionChanger->Cancel());
                pSelectionChanger = NULL;
            }
        }
        break;
    case wfc::CollectionChange_ItemChanged:
    case wfc::CollectionChange_ItemInserted:
        // Check if the newly inserted item is actually a data virtualized
        // placeholder that hasn't been realized yet
        IFC(e->get_Index(&index));
        IFC(ShowPlaceholderIfVirtualized(index));
        break;

    case wfc::CollectionChange_Reset:
        {
            ctl::ComPtr<IInspectable> spItemsSource;
            ctl::ComPtr<xaml_data::ICollectionView> spCV;

            IFC(get_ItemsSource(&spItemsSource));
            spCV = spItemsSource.AsOrNull<xaml_data::ICollectionView>();
            if (m_tpMonitoredCV.Get() != spCV.Get())
            {
                IFC(DisconnectFromMonitoredCV());

                IFC(MonitorCV(spCV.Get()));

                IFC(UpdateIsSynchronized());
            }
        }
        break;
    }

    IFC(SelectorGenerated::OnItemsChanged(e));

    switch (action)
    {
        case wfc::CollectionChange_ItemRemoved:
        {
            UINT itemIndex;
            IFC(e->get_Index(&itemIndex));

            const int itemIndexInt = itemIndex;
            UINT itemsCount;
            INT focusedIndex = GetFocusedIndex();
            ctl::ComPtr<wfc::IObservableVector<IInspectable*>> items;

            IFC(get_Items(&items));
            IFC(items.Cast<ItemCollection>()->get_Size(&itemsCount));

            // Our focused item was deleted. We will give focus to the item with the same index
            // unless, of course, it's not possible because of the reduced items count.
            // We had to wait until SelectorGenerated::OnItemsChanged to give the items host
            // panel the chance to handle the collection change and update its container mapping.
            // Using InputActivationBehavior::NoActivate instead of the default InputActivationBehavior::RequestActivation
            // to avoid the window/island stealing focus.
            if (focusedIndex == itemIndexInt)
            {
                IFC(SetFocusedItem(
                        std::min(static_cast<int>(itemsCount) - 1, itemIndexInt),
                        false /*shouldScrollIntoView*/,
                        false /*animateIfBringIntoView*/,
                        xaml_input::FocusNavigationDirection::FocusNavigationDirection_None /*focusNavigationDirection*/,
                        InputActivationBehavior::NoActivate /*inputActivationBehavior*/));
            }
            // An item was removed before our focused item, we need to update
            // the focused item's index to reflect that.
            else if (focusedIndex > itemIndexInt)
            {
                ASSERT(focusedIndex > 0);
                focusedIndex--;

                SetFocusedIndex(focusedIndex);
                SetLastFocusedIndex(focusedIndex);
            }
            // If focus moved outside the selector, m_focusedIndex will be -1 and will not match the removed item index.
            // However, m_lastFocusedIndex could end up going outside the data range now - so make sure that we clamp it back.
            // This can happen if there is a context menu on the item, that deletes the item.
            else if (focusedIndex == -1 && itemsCount > 0 && GetLastFocusedIndex() >= static_cast<int>(itemsCount))
            {
                SetLastFocusedIndex(0);
            }
        }
        break;
    }

    if (m_selectedIndexValueSetBeforeItemsAvailable >= 0)
    {
        // There was a selected index set before Items was available. Now
        // that items is being reset, see if the index is in range and if so
        // set the selected index now.
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
        UINT nCount = 0;
        IFC(get_Items(&spItems));
        IFC(spItems.Cast<ItemCollection>()->get_Size(&nCount));
        if (m_selectedIndexValueSetBeforeItemsAvailable < static_cast<int>(nCount))
        {
            IFC(put_SelectedIndex(m_selectedIndexValueSetBeforeItemsAvailable));
        }
    }

Cleanup:
    if (pSelectionChanger != NULL)
    {
        VERIFYRETURNHR(pSelectionChanger->Cancel());
    }
    m_inCollectionChange = false;
    return hr;
}

IFACEMETHODIMP
    Selector::PrepareContainerForItemOverride(
    _In_ xaml::IDependencyObject* pElement,
    _In_ IInspectable* pItem)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<ISelectorItem> spSelectorItem;
    SelectorItem* pSelectorItemNoRef;
    INT newIndex = -1;
    BOOLEAN hasItem = FALSE;

    IFCPTR(pElement);

    IFC(SelectorGenerated::PrepareContainerForItemOverride(pElement, pItem));

    IFC(ctl::do_query_interface(spSelectorItem, pElement));
    pSelectorItemNoRef = spSelectorItem.Cast<SelectorItem>();

    // Clear the mouse-over state, as the item may still have this set.
    IFC(pSelectorItemNoRef->put_IsPointerOver(FALSE));

    // Store a weak reference to the parent Selector
    IFC(pSelectorItemNoRef->SetParentSelector(this));

    IFC(IndexFromContainer(pSelectorItemNoRef, &newIndex));

    if (newIndex != -1)
    {
        UINT collectionIndex = 0;

        // If the ISelectionInfo is implemented by the DataSource,
        // we let the DataSource handle selection
        if (m_tpDataSourceAsSelectionInfo)
        {
            // In this case, we only want to update the SelectorItem.IsSelected property
            // We do not want to update selection (or call any of the selection interfaces to select or deselect)
            // Changing IsSelected will call Selector::NotifyListItemSelected
            // In NotifyListItemSelected, we will not call selection interface functions when the Selection is prevented
            PreventSelectionReentrancy();

            IFC(m_tpDataSourceAsSelectionInfo->IsSelected(newIndex, &hasItem));
        }
        else
        {
            IFC(m_selection.Has(newIndex, collectionIndex, hasItem));
        }

        if (hasItem)
        {
            IFC(pSelectorItemNoRef->put_IsSelected(TRUE));
        }
        else
        {
            IFC(pSelectorItemNoRef->put_IsSelected(FALSE));
        }

        // we cannot check for focus on every container here, since that is very expensive
        if (newIndex == GetFocusedIndex())
        {
            ctl::ComPtr<xaml_controls::IPanel> spPanel;

            IFC(get_ItemsHost(&spPanel));
            IFC(SetFocusedItem(newIndex,
                /*shouldScrollIntoView*/
                !ctl::is<xaml_controls::IVirtualizingPanel>(spPanel.Get())
                && !ctl::is<IModernCollectionBasePanel>(spPanel)));
        }
    }

    // Check if the SelectorItem is a data virtualized placeholder
    // We do not call ShowPlaceholderIfVirtualized because it queries data we already have and updates the visual state which we do too
    pSelectorItemNoRef->m_isPlaceholder = (pItem == nullptr);

    IFC(pSelectorItemNoRef->UpdateVisualState(FALSE));

Cleanup:
    AllowSelectionReentrancy();
    RRETURN(hr);
}

IFACEMETHODIMP
    Selector::ClearContainerForItemOverride(
    _In_ xaml::IDependencyObject* pElement,
    _In_ IInspectable* pItem)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ISelectorItem> spSelectorItem;
    ctl::ComPtr<xaml::IDependencyObject> spStoredItem;

    IFC(SelectorGenerated::ClearContainerForItemOverride(pElement, pItem));

    IFC(ctl::do_query_interface(spSelectorItem, pElement));
    spStoredItem = ctl::query_interface_cast<xaml::IDependencyObject>(pItem);

    if (spStoredItem.Get() != pElement)
    {
        VirtualizationInformation *virtualizationInformation = spSelectorItem ? spSelectorItem.Cast<SelectorItem>()->GetVirtualizationInformation() : nullptr;
        bool isContainerFromTemplateRoot = virtualizationInformation != nullptr && virtualizationInformation->GetIsContainerFromTemplateRoot();

        // Do not clear content if we got the container from ItemTemplate
        if (!isContainerFromTemplateRoot)
        {
            IFC(spSelectorItem.Cast<SelectorItem>()->ClearValue(
                MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ContentControl_Content)));
        }
    }

    // Clear whether or not the SelectorItem is a data virtualized placeholder
    spSelectorItem.Cast<SelectorItem>()->m_isPlaceholder = FALSE;

Cleanup:
    RRETURN(hr);
}

// Called when a SelectorItem is clicked
// Allows subclasses (e.g. ListBox) to change behavior
_Check_return_
    HRESULT
    Selector::OnSelectorItemClicked(
    _In_ SelectorItem* pSelectorItem,
    _Out_ BOOLEAN* pFocused)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsSelected = TRUE;
    wsy::VirtualKeyModifiers nModifiersKey;

    IFC(pSelectorItem->Focus(xaml::FocusState_Pointer, pFocused));

    if (*pFocused)
    {
        IFC(CoreImports::Input_GetKeyboardModifiers(&nModifiersKey));
        if (wsy::VirtualKeyModifiers_Control ==
            (nModifiersKey & wsy::VirtualKeyModifiers_Control))
        {
            IFC(pSelectorItem->get_IsSelected(&bIsSelected));
            bIsSelected = !bIsSelected;
        }

        IFC(pSelectorItem->put_IsSelected(bIsSelected));
    }

    IFC(pSelectorItem->HasFocus(pFocused));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Selector::OnFocusEngaged(
    _In_ xaml::Controls::IControl* pSender,
    _In_ xaml::Controls::IFocusEngagedEventArgs* pArgs)
{
    BOOLEAN hasItems = false;
    IFC_RETURN(HasItems(hasItems));

    if (hasItems)
    {
        IFC_RETURN(SetFocusedItem(GetLastFocusedIndex(), true /*shouldScrollIntoView*/, true /*forceFocus*/, xaml::FocusState_Keyboard));
    }

    ctl::ComPtr<xaml_controls::IFocusEngagedEventArgs> spArgs(pArgs);

    IFC_RETURN(spArgs->put_Handled(true));

    return S_OK;
}

// Called to detect whether we can scroll to the View or not.
_Check_return_
    HRESULT
    Selector::CanScrollIntoView(_Out_ BOOLEAN& canScroll)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IPanel> spPanel;
    BOOLEAN isItemsHostInvalid = FALSE;
    bool isInLiveTree = false;

    canScroll = FALSE;

    IFC(get_ItemsHost(&spPanel));
    if (spPanel)
    {
        IFC(get_IsItemsHostInvalid(&isItemsHostInvalid));
        if (!isItemsHostInvalid)
        {
            isInLiveTree = IsInLiveTree();
        }
    }

    canScroll = !isItemsHostInvalid && isInLiveTree && !m_skipScrollIntoView && !m_inCollectionChange;

Cleanup:
    RRETURN(hr);
}

// Handles changing selection properties when a SelectorItem has IsSelected change
_Check_return_
    HRESULT
    Selector::NotifyListItemSelected(
    _In_ SelectorItem* pSelectorItem,
    _In_ BOOLEAN bIsSelected)
{
    HRESULT hr = S_OK;

    INT newIndex = 0;
    SelectionChanger* pSelectionChanger = nullptr;

    IFC(IndexFromContainer(pSelectorItem, &newIndex));

    if (m_tpDataSourceAsSelectionInfo)
    {
        IFC(InvokeDataSourceRangeSelection(!!bIsSelected /* select */, newIndex, 1));
    }
    else
    {
        UINT nCount = 0;
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
        ctl::ComPtr<IInspectable> spSelectedItem;

        if (!IsSelectionReentrancyAllowed())
        {
            goto Cleanup;
        }


        IFC(get_Items(&spItems));
        IFC(spItems.Cast<ItemCollection>()->get_Size(&nCount));

        if (newIndex >= 0 && newIndex < static_cast<INT>(nCount))
        {
            IFC(ItemFromContainer(pSelectorItem, &spSelectedItem));

            IFC(BeginChange(&pSelectionChanger));

            if (bIsSelected)
            {
                BOOLEAN canSelectMultiple = FALSE;
                IFC(get_CanSelectMultiple(&canSelectMultiple));
                IFC(pSelectionChanger->Select(newIndex, spSelectedItem.Get(), canSelectMultiple));
            }
            else
            {
                IFC(pSelectionChanger->Unselect(newIndex, spSelectedItem.Get()));
            }

            IFC(EndChange(pSelectionChanger));
            pSelectionChanger = nullptr;
        }
    }

Cleanup:
    if (pSelectionChanger != nullptr)
    {
        VERIFYRETURNHR(pSelectionChanger->Cancel());
    }

    // returning S_OK to maintain compat with blue - bug 927905
    // In blue we swallowed any exception thrown in the app's
    // SelectionChanged event handler
    return S_OK;
}

// Requests a SelectionChanger from our Selection instance.
// This is preferred over requesting the Selection directly.
_Check_return_
    HRESULT
    Selector::BeginChange(
    _Outptr_ SelectionChanger** pChanger)
{
    HRESULT hr = S_OK;

    IFCPTR(pChanger);

    IFC(m_selection.BeginChange(pChanger));

Cleanup:
    return hr;
}

// Applies the changes in the given SelectionChanger to this
// Selector, ensuring the state of public properties.
_Check_return_
    HRESULT
    Selector::EndChange(
    _In_ SelectionChanger* pChanger,
    _In_ BOOLEAN animateIfBringIntoView,
    _In_ xaml_input::FocusNavigationDirection focusNavigationDirection)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spCurrentSelectedItem;
    ctl::ComPtr<IInspectable> spNewSelectedItem;
    ctl::ComPtr<TrackerCollection<IInspectable*>> spUnselectedItems;
    ctl::ComPtr<TrackerCollection<IInspectable*>> spSelectedItems;
    BOOLEAN didChange = FALSE;
    INT currentSelectedIndex = -1;
    INT newSelectedIndex = -1;
    UINT nUnselectedCount = 0;
    UINT nSelectedCount = 0;
    BOOLEAN canSelectMultiple = FALSE;

    ASSERT(!IsSelectionReentrancyAllowed(), L"Reentrancy lock must be engaged when EndChange is called.");

    IFCPTR(pChanger);

    IFC(ctl::make < TrackerCollection < IInspectable* >> (&spUnselectedItems));
    IFC(ctl::make < TrackerCollection < IInspectable* >> (&spSelectedItems));

    IFC(get_CanSelectMultiple(&canSelectMultiple));

    // We need to prevent reentrancy until we finish calling UpdatePublicSelectionProperties.
    PreventSelectionReentrancy();

    IFC(pChanger->End(spUnselectedItems.Get(), spSelectedItems.Get(), canSelectMultiple));

    if (!m_pOldSelectedIndexToReport)
    {
        IFC(get_SelectedIndex(&currentSelectedIndex));
    }

    // Update SelectedItem, SelectedIndex
    IFC(m_selection.GetNumItemsSelected(nSelectedCount));
    if (nSelectedCount > 0)
    {
        UINT itemIndex = FALSE;
        IFC(m_selection.GetAt(0, &spNewSelectedItem));
        IFC(m_selection.GetIndexAt(0, itemIndex));
        newSelectedIndex = itemIndex;
    }

    IFC(get_SelectedItem(&spCurrentSelectedItem));
    IFC(UpdatePublicSelectionProperties(m_pOldSelectedIndexToReport ? *m_pOldSelectedIndexToReport : currentSelectedIndex, newSelectedIndex, spCurrentSelectedItem.Get(), spNewSelectedItem.Get(), animateIfBringIntoView, focusNavigationDirection));

    AllowSelectionReentrancy();

    IFC(spUnselectedItems->get_Size(&nUnselectedCount));
    IFC(spSelectedItems->get_Size(&nSelectedCount));

    didChange = (nUnselectedCount > 0 || nSelectedCount > 0);
    if (didChange)
    {
        IFC(UpdateSelectedItems());
        IFC(UpdateSelectedRanges());
        IFC(InvokeSelectionChanged(spUnselectedItems.Get(), spSelectedItems.Get()));
    }

    ClearOldIndexToReport();

Cleanup:
    AllowSelectionReentrancy();
    RRETURN(hr);
}

// Sets the SelectorItem for item to be selected
_Check_return_
    HRESULT
    Selector::SetItemIsSelected(
    _In_ UINT index,
    _In_ BOOLEAN isSelected)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ISelectorItem> spSelectorItem;

    ctl::ComPtr<IDependencyObject> spContainer;
    IFC(ContainerFromIndex(index, &spContainer));
    spSelectorItem = spContainer.AsOrNull<ISelectorItem>();

    if (spSelectorItem)
    {
        IFC(spSelectorItem->put_IsSelected(isSelected));
    }

Cleanup:
    RRETURN(hr);
}

// Handles selection of single item, and only that item
_Check_return_ HRESULT Selector::MakeSingleSelection(
    _In_ UINT index,
    _In_ BOOLEAN animateIfBringIntoView,
    _In_opt_ IInspectable* pSelectedItem,
    _In_ xaml_input::FocusNavigationDirection focusNavigationDirection)
{
    HRESULT hr = S_OK;

    int selectedIndex = -1;

    IFC(get_SelectedIndex(&selectedIndex));

    IFC(SelectJustThisItemInternal(selectedIndex, index, pSelectedItem, animateIfBringIntoView, nullptr, focusNavigationDirection));

Cleanup:
    return hr;
}

// Raises IsSelected property change Automation event on Item got selected.
_Check_return_
    HRESULT
    Selector::RaiseIsSelectedChangedAutomationEvent(xaml::IDependencyObject* pContainer, _In_ BOOLEAN isSelected)
{
    HRESULT hr = S_OK;
    BOOLEAN isUnsetValue = FALSE;
    BOOLEAN bAutomationListener = FALSE;

    IFC(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_PropertyChanged, &bAutomationListener));
    if (bAutomationListener)
    {
        ctl::ComPtr<IInspectable> spItem;

        IFCPTR(pContainer);
        IFC(ItemFromContainer(pContainer, &spItem));
        // We do not create item automation peer when item is null (for cases like placeholder containers)
        if (spItem)
        {
            IFC(DependencyPropertyFactory::IsUnsetValue(spItem.Get(), isUnsetValue));
            if (!isUnsetValue)
            {
                ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeer;

                IFC(GetOrCreateAutomationPeer(&spAutomationPeer));
                if (spAutomationPeer)
                {
                    ctl::ComPtr<xaml_automation_peers::IItemsControlAutomationPeer> spItemsControlAutomationPeer;

                    spItemsControlAutomationPeer = spAutomationPeer.AsOrNull<xaml_automation_peers::IItemsControlAutomationPeer>();
                    if (spItemsControlAutomationPeer)
                    {
                        ctl::ComPtr<xaml_automation_peers::IItemAutomationPeer> spItemAutomationPeer;

                        IFC(spItemsControlAutomationPeer.Cast<ItemsControlAutomationPeer>()->CreateItemAutomationPeer(spItem.Get(), &spItemAutomationPeer));
                        if (spItemAutomationPeer)
                        {
                            IFC(spItemAutomationPeer.Cast<ItemAutomationPeer>()->RaiseAutomationIsSelectedChanged(isSelected));
                        }
                    }
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Updates m_tpSelectedItemsImpl
_Check_return_
    HRESULT
    Selector::UpdateSelectedItems()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spCurrentItem;
    UINT nSelectedCount = 0;

    // prevent reentrancy or
    // other properties from being updated.
    PreventSelectionReentrancy();
    IFC(m_tpSelectedItemsImpl.Cast < ObservableTrackerCollection < IInspectable* >> ()->Clear());
    IFC(m_selection.GetNumItemsSelected(nSelectedCount));
    for (UINT i = 0; i < nSelectedCount; ++i)
    {
        IFC(m_selection.GetAt(i, &spCurrentItem));
        IFC(m_tpSelectedItemsImpl.Cast < ObservableTrackerCollection < IInspectable* >> ()->Append(spCurrentItem.Get()));
    }

Cleanup:
    AllowSelectionReentrancy();
    RRETURN(hr);
}

// Updates m_tpSelectedRangesImpl
_Check_return_
    HRESULT
    Selector::UpdateSelectedRanges()
{
    HRESULT hr = S_OK;

    std::vector<unsigned int> selectedIndexes;
    ctl::ComPtr<TrackerCollection<xaml_data::ItemIndexRange*>> spSelectedRanges;

    // prevent reentrancy or
    // other properties from being updated.
    PreventSelectionReentrancy();

    spSelectedRanges = m_tpSelectedRangesImpl.Cast<TrackerCollection<xaml_data::ItemIndexRange*>>();

    IFC(spSelectedRanges->Clear());

    selectedIndexes = std::move(m_selection.GetSelectedIndexes());

    // make sure indexes are sorted
    std::sort(selectedIndexes.begin(), selectedIndexes.end());

    IFC(ItemIndexRange::AppendItemIndexRangesFromSortedVectorToItemIndexRangeCollection(selectedIndexes, spSelectedRanges.Get()));

Cleanup:
    AllowSelectionReentrancy();
    return hr;
}

// Updates all properties that are publicly visible after a selection change
_Check_return_
    HRESULT
    Selector::UpdatePublicSelectionProperties(
    _In_ INT oldSelectedIndex,
    _In_ INT newSelectedIndex,
    _In_ IInspectable* pOldSelectedItem,
    _In_ IInspectable* pNewSelectedItem,
    _In_ BOOLEAN animateIfBringIntoView,
    _In_ xaml_input::FocusNavigationDirection focusNavigationDirection)
{
    ctl::ComPtr<IInspectable> spCurrentSelectedItem;
    ctl::ComPtr<IInspectable> spInternalSelectedValue;
    ctl::ComPtr<IInspectable> spSelectedValue;
    INT currentSelectedIndex = -1;
    bool areEqual = false;
    bool fDone = false;

    // if the selection interface (ISelectionInfo) is implemented, we let the developer handle selection with collection changes
    if (m_tpDataSourceAsSelectionInfo)
    {
        return S_OK;
    }

    ASSERT(!IsSelectionReentrancyAllowed(), L"Reentrancy lock must be active to call UpdatePublicSelectionProperties");

    // Custom Values have always an index of -1, avoid doing this check to update the selected value correctly.
    if (!AreCustomValuesAllowed())
    {
        if (oldSelectedIndex == newSelectedIndex)
        {
            IFC_RETURN(PropertyValue::AreEqual(pOldSelectedItem, pNewSelectedItem, &areEqual));
            if (areEqual)
            {
                // nothing changed.
                return S_OK;
            }
        }
    }

    IFC_RETURN(get_SelectedIndex(&currentSelectedIndex));
    if (newSelectedIndex != currentSelectedIndex)
    {
        IFC_RETURN(put_SelectedIndex(newSelectedIndex));
    }

    IFC_RETURN(get_SelectedItem(&spCurrentSelectedItem));
    IFC_RETURN(PropertyValue::AreEqual(spCurrentSelectedItem.Get(), pNewSelectedItem, &areEqual));
    if (!areEqual)
    {
        IFC_RETURN(put_SelectedItem(pNewSelectedItem));
    }

    IFC_RETURN(GetSelectedValue(pNewSelectedItem, &spInternalSelectedValue));
    // Don't clear the SelectedValue just because a change to the SelectedValuePath has cleared the selection.
    // This behavior ensures that these two properties can be changed together without the SelectedValue property
    // changing in between the two property sets.
    IFC_RETURN(get_SelectedValue(&spSelectedValue));
    bool areValuesEqual = false;
    IFC_RETURN(PropertyValue::AreEqual(spInternalSelectedValue.Get(), spSelectedValue.Get(), &areValuesEqual));
    if (!areValuesEqual && !m_bSelectionChangeCausedBySelectedValuePathPropertyChange)
    {
        IFC_RETURN(put_SelectedValue(spInternalSelectedValue.Get()));
    }

    // If the ItemCollection contains an ICollectionView sync the selected index
    IFC_RETURN(UpdateCurrentItemInCollectionView(pNewSelectedItem, &fDone));

    IFC_RETURN(get_SelectedIndex(&currentSelectedIndex));

    IFC_RETURN(get_SelectedItem(&spCurrentSelectedItem));
    IFC_RETURN(OnSelectionChanged(oldSelectedIndex, currentSelectedIndex, pOldSelectedItem, spCurrentSelectedItem.Get(), animateIfBringIntoView, focusNavigationDirection));

    return S_OK;
}

// Helper function to select one item in Selector
_Check_return_
    HRESULT
    Selector::SelectOneItemInternal(
    _In_ int index,
    _In_ IInspectable* item,
    _In_ BOOLEAN clearOldSelection)
{
    HRESULT hr = S_OK;

    SelectionChanger* pSelectionChanger = nullptr;

    if (m_tpDataSourceAsSelectionInfo)
    {
        if (clearOldSelection)
        {
            IFC(InvokeDataSourceClearSelection());
        }

        IFC(InvokeDataSourceRangeSelection(true /* select */, index, 1));
    }
    else
    {
        BOOLEAN canSelectMultiple = FALSE;

        IFC(get_CanSelectMultiple(&canSelectMultiple));

        IFC(BeginChange(&pSelectionChanger));

        if (clearOldSelection)
        {
            IFC(pSelectionChanger->UnselectAll());
        }

        IFC(pSelectionChanger->Select(index, item, canSelectMultiple));

        IFC(EndChange(pSelectionChanger));
        pSelectionChanger = nullptr;
    }

Cleanup:
    if (pSelectionChanger != nullptr)
    {
        VERIFYRETURNHR(pSelectionChanger->Cancel());
    }
    return hr;
}

// Helper function to select range of items in Selector
_Check_return_
    HRESULT
    Selector::SelectAllInternal()
{
    HRESULT hr = S_OK;

    unsigned int itemCount = 0;
    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
    SelectionChanger* pSelectionChanger = nullptr;

    IFC(get_Items(&spItems));
    IFC(spItems.Cast<ItemCollection>()->get_Size(&itemCount));

    if (m_tpDataSourceAsSelectionInfo)
    {
        IFC(InvokeDataSourceRangeSelection(true /* select */, 0, itemCount));
    }
    else
    {
        BOOLEAN canSelectMultiple = FALSE;

        IFC(get_CanSelectMultiple(&canSelectMultiple));

        if (!canSelectMultiple)
        {
            IFC(ErrorHelper::OriginateErrorUsingResourceID(E_NOT_SUPPORTED, ERROR_INVALID_MULTIPLE_SELECT));
        }
        else
        {
            IFC(BeginChange(&pSelectionChanger));

            for (unsigned int i = 0; i < itemCount; ++i)
            {
                ctl::ComPtr<IInspectable> spSelectedItem;

                IFC(spItems.Cast<ItemCollection>()->GetAt(i, &spSelectedItem));
                IFC(pSelectionChanger->Select(i, spSelectedItem.Get(), canSelectMultiple));
            }

            IFC(EndChange(pSelectionChanger));
            pSelectionChanger = nullptr;
        }
    }

Cleanup:
    if (pSelectionChanger != nullptr)
    {
        VERIFYRETURNHR(pSelectionChanger->Cancel());
    }
    return hr;
}

// Helper function to select range of items in Selector
_Check_return_
    HRESULT
    Selector::SelectRangeInternal(
    _In_ int firstIndex,
    _In_ unsigned int length,
    _In_ BOOLEAN clearOldSelection)
{
    HRESULT hr = S_OK;

    SelectionChanger* pSelectionChanger = nullptr;

    if (m_tpDataSourceAsSelectionInfo)
    {
        IFC(InvokeDataSourceRangeSelection(true /* select */, firstIndex, length));
    }
    else
    {
        BOOLEAN canSelectMultiple = FALSE;

        IFC(get_CanSelectMultiple(&canSelectMultiple));

        if (!canSelectMultiple)
        {
            IFC(ErrorHelper::OriginateErrorUsingResourceID(E_NOT_SUPPORTED, ERROR_INVALID_MULTIPLE_SELECT));
        }
        else
        {
            unsigned int itemCount = 0;
            ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;

            IFC(BeginChange(&pSelectionChanger));

            if (clearOldSelection)
            {
                IFC(pSelectionChanger->UnselectAll());
            }

            IFC(get_Items(&spItems));
            IFC(spItems.Cast<ItemCollection>()->get_Size(&itemCount));

            if (static_cast<unsigned int>(firstIndex) < itemCount)
            {
                unsigned int modifiedLength = std::min(length, itemCount - firstIndex);

                if (modifiedLength > 0)
                {
                    int lastIndex = firstIndex + modifiedLength - 1;

                    for (int i = firstIndex; i <= lastIndex; ++i)
                    {
                        ctl::ComPtr<IInspectable> spSelectedItem;

                        IFC(spItems.Cast<ItemCollection>()->GetAt(i, &spSelectedItem));
                        IFC(pSelectionChanger->Select(i, spSelectedItem.Get(), canSelectMultiple));
                    }
                }
            }

            IFC(EndChange(pSelectionChanger));
            pSelectionChanger = nullptr;
        }
    }

Cleanup:
    if (pSelectionChanger != nullptr)
    {
        VERIFYRETURNHR(pSelectionChanger->Cancel());
    }
    return hr;
}

// Helper function to deselect range of items in Selector
_Check_return_
    HRESULT
    Selector::DeselectRangeInternal(
    _In_ int firstIndex,
    _In_ unsigned int length)
{
    HRESULT hr = S_OK;

    SelectionChanger* pSelectionChanger = nullptr;

    if (m_tpDataSourceAsSelectionInfo)
    {
        IFC(InvokeDataSourceRangeSelection(false /* select */, firstIndex, length));
    }
    else
    {
        unsigned int itemCount = 0;
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;

        IFC(get_Items(&spItems));
        IFC(spItems.Cast<ItemCollection>()->get_Size(&itemCount));

        if (static_cast<unsigned int>(firstIndex) < itemCount)
        {
            unsigned int modifiedLength = std::min(length, itemCount - firstIndex);

            if (modifiedLength > 0)
            {
                int lastIndex = firstIndex + modifiedLength - 1;

                IFC(BeginChange(&pSelectionChanger));

                for (int i = firstIndex; i <= lastIndex; ++i)
                {
                    ctl::ComPtr<IInspectable> spSelectedItem;

                    IFC(spItems.Cast<ItemCollection>()->GetAt(i, &spSelectedItem));
                    IFC(pSelectionChanger->Unselect(i, spSelectedItem.Get()));
                }

                IFC(EndChange(pSelectionChanger));
                pSelectionChanger = nullptr;
            }
        }
    }

Cleanup:
    if (pSelectionChanger != nullptr)
    {
        VERIFYRETURNHR(pSelectionChanger->Cancel());
    }
    return hr;
}

// Helper function to select range of items in Selector
_Check_return_
    HRESULT
    Selector::ClearSelection()
{
    HRESULT hr = S_OK;

    SelectionChanger* pSelectionChanger = NULL;

    if (m_tpDataSourceAsSelectionInfo)
    {
        unsigned int itemCount = 0;
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;

        IFC(get_Items(&spItems));
        IFC(spItems.Cast<ItemCollection>()->get_Size(&itemCount));

        IFC(InvokeDataSourceRangeSelection(false /* select */, 0, itemCount));
    }
    else
    {
        IFC(BeginChange(&pSelectionChanger));

        IFC(pSelectionChanger->UnselectAll());

        IFC(EndChange(pSelectionChanger));

        pSelectionChanger = NULL;
    }

Cleanup:
    if (pSelectionChanger != NULL)
    {
        VERIFYRETURNHR(pSelectionChanger->Cancel());
    }
    return hr;
}

// Ask the CollectionView if it is appropriate to select a single item,
// then select just that item (in the context of the given SelectionChanger).
_Check_return_
    HRESULT
    Selector::SelectJustThisItem(
    _In_ const INT oldIndex,
    _In_ const INT newIndex,
    _In_ SelectionChanger* pChanger,
    _In_opt_ IInspectable* pSelectedItem,
    _Out_opt_ BOOLEAN* pShouldUndoChange)
{

    HRESULT hr = S_OK;
    bool fChangeDone = true;
    ctl::ComPtr<IInspectable> spSelectedItemCalculated = pSelectedItem;

    IFCPTR(pChanger);

    if (pShouldUndoChange)
    {
        *pShouldUndoChange = TRUE;
    }

    if (!spSelectedItemCalculated && newIndex >= 0)
    {
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;

        IFC(get_Items(&spItems));
        IFC(spItems.Cast<ItemCollection>()->GetAt(newIndex, &spSelectedItemCalculated));
    }

    // First tell the owner that it needs to update its collection view
    IFC(UpdateCurrentItemInCollectionView(spSelectedItemCalculated.Get(), &fChangeDone));
    if (!fChangeDone)
    {
        // The change was cancelled by the CV nothing else to do here
        goto Cleanup;
    }

    IFC(pChanger->UnselectAll());

    // Custom values have index -1
    if (newIndex >= 0 || (AreCustomValuesAllowed() && spSelectedItemCalculated))
    {
        BOOLEAN canSelectMultiple = FALSE;

        IFC(get_CanSelectMultiple(&canSelectMultiple));
        IFC(pChanger->Select(newIndex, spSelectedItemCalculated.Get(), canSelectMultiple));
    }

    IFC(ReportOldSelectedIndexAs(oldIndex));

    if (pShouldUndoChange)
    {
        *pShouldUndoChange = FALSE;
    }

Cleanup:
    return hr;
}

// Set the SelectorItem at index to be focused.
// If HasFocus() returns true, we propagate previous focused item's FocusState to the new focused item.
_Check_return_
    HRESULT
    Selector::SetFocusedItem(
    _In_ INT index,
    _In_ BOOLEAN shouldScrollIntoView,
    _In_ BOOLEAN animateIfBringIntoView,
    _In_ xaml_input::FocusNavigationDirection focusNavigationDirection,
    InputActivationBehavior inputActivationBehavior)
{
    HRESULT hr = S_OK;
    BOOLEAN hasFocus = FALSE;
    xaml::FocusState focusState = xaml::FocusState_Programmatic;

    IFC(HasFocus(&hasFocus));
    if (hasFocus)
    {
        ctl::ComPtr<DependencyObject> spFocused;
        ctl::ComPtr<IUIElement> spFocusedAsElement;

        IFC(GetFocusedElement(&spFocused));
        spFocusedAsElement = spFocused.AsOrNull<IUIElement>();
        if (spFocusedAsElement)
        {
            IFC(spFocusedAsElement->get_FocusState(&focusState));
            ASSERT(xaml::FocusState_Unfocused != focusState, L"FocusState_Unfocused unexpected since spFocusedAsElement is focused");
        }
    }

    IFC(SetFocusedItem(index, shouldScrollIntoView, FALSE /*forceFocus*/, focusState, animateIfBringIntoView, focusNavigationDirection, inputActivationBehavior));

Cleanup:
    RRETURN(hr);
}

// Set the SelectorItem at index to be focused using an explicit FocusState
// The focus is only set if forceFocus is TRUE or the Selector already has focus.
// ScrollIntoView is always called if shouldScrollIntoView is set (regardless of focus).
_Check_return_
    HRESULT
    Selector::SetFocusedItem(
    _In_ INT index,
    _In_ BOOLEAN shouldScrollIntoView,
    _In_ BOOLEAN forceFocus,
    _In_ xaml::FocusState focusState,
    _In_ BOOLEAN animateIfBringIntoView,
    _In_ xaml_input::FocusNavigationDirection focusNavigationDirection,
    InputActivationBehavior inputActivationBehavior)
{
#ifdef SLTR_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"SLTR[0x%p]: SetFocusedItem. index=%d, shouldScrollIntoView=%d, forceFocus=%d, focusState=%d, animateIfBringIntoView=%d, focusNavigationDirection=%d",
        this, index, shouldScrollIntoView, forceFocus, focusState, animateIfBringIntoView, focusNavigationDirection));
#endif

    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
    UINT nCount = 0;
    BOOLEAN bFocused = FALSE;
    BOOLEAN shouldFocus = FALSE;

    IFC(get_Items(&spItems));
    IFC(spItems.Cast<ItemCollection>()->get_Size(&nCount));
    if (index < 0 || static_cast<INT>(nCount) <= index)
    {
        index = -1;
    }

    if (index >= 0)
    {
        SetLastFocusedIndex(index);
    }

    if (!forceFocus)
    {
        IFC(HasFocus(&shouldFocus));
    }
    else
    {
        shouldFocus = TRUE;
    }

    if (shouldFocus)
    {
        SetFocusedIndex(index);
    }

    if (GetFocusedIndex() == -1)
    {
        if (shouldFocus)
        {
            // Since none of our child items have the focus, put the focus back on the main list box.
            //
            // This will happen e.g. when the focused item is being removed but is still in the visual tree at the time of this call.
            // Note that this call may fail e.g. if IsTabStop is FALSE, which is OK; it will just set focus
            // to the next focusable element (or clear focus if none is found).
            IFC(Focus(focusState, &bFocused)); // FUTURE: should handle inputActivationBehavior
        }
        goto Cleanup;
    }

    if (shouldScrollIntoView)
    {
        IFC(CanScrollIntoView(shouldScrollIntoView));
    }

    if (shouldScrollIntoView)
    {
        IFC(ScrollIntoView(
            index,
            FALSE /*isGroupItemIndex*/,
            FALSE /*isHeader*/,
            FALSE /*isFooter*/,
            FALSE /*isFromPublicAPI*/,
            TRUE  /*ensureContainerRealized*/,
            animateIfBringIntoView,
            xaml_controls::ScrollIntoViewAlignment::ScrollIntoViewAlignment_Default));
    }

    if (shouldFocus)
    {
        ctl::ComPtr<ISelectorItem> spSelectorItem;
        ctl::ComPtr<xaml::IDependencyObject> spContainer;

        IFC(ContainerFromIndex(index, &spContainer));
        spSelectorItem = spContainer.AsOrNull<ISelectorItem>();
        if (spSelectorItem)
        {
            IFC(spSelectorItem.Cast<SelectorItem>()->FocusSelfOrChild(focusState, animateIfBringIntoView, &bFocused, focusNavigationDirection, inputActivationBehavior));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Handler for when a SelectorItem received focus
_Check_return_ 
    HRESULT 
    Selector::ItemFocused(
    _In_ SelectorItem* pSelectorItem)
{
    INT focusedIndex{ -1 };

    // Set the focused index correctly
    IFC_RETURN(IndexFromContainer(pSelectorItem, &focusedIndex));
    SetFocusedIndex(focusedIndex);

    if (focusedIndex >= 0)
    {
        SetLastFocusedIndex(focusedIndex);
    }

#ifdef SLTR_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"SLTR[0x%p]: ItemFocused. index=%d", this, GetFocusedIndex()));
#endif

    return S_OK;
}

// Handler for when a SelectorItem lost focus
_Check_return_ 
    HRESULT 
    Selector::ItemUnfocused(
    _In_ SelectorItem* pSelectorItem)
{
    INT itemIndex{ -1 };

    // Set the focused index correctly
    IFC_RETURN(IndexFromContainer(pSelectorItem, &itemIndex));

    if (GetFocusedIndex() == itemIndex)
    {
        SetFocusedIndex(-1);
    }

#ifdef SLTR_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"SLTR[0x%p]: ItemUnfocused. index=%d", this, itemIndex));
#endif

    return S_OK;
}

// Selects the next item in the list.
_Check_return_
    HRESULT
    Selector::SelectNext(_Inout_ INT& index)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
    UINT nCount = 0;

    IFC(get_Items(&spItems));
    IFC(spItems.Cast<ItemCollection>()->get_Size(&nCount));

    if (nCount > 0)
    {
        INT internalSelectedIndex = index + 1;
        if (internalSelectedIndex <= static_cast<INT>(nCount) - 1)
        {
            IFC(SelectItemHelper(internalSelectedIndex, +1));
            if (internalSelectedIndex != -1)
            {
                index = internalSelectedIndex;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}


// Selects the previous item in the list.
_Check_return_
    HRESULT
    Selector::SelectPrev(_Inout_ INT& index)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
    UINT nCount = 0;

    IFC(get_Items(&spItems));
    IFC(spItems.Cast<ItemCollection>()->get_Size(&nCount));

    if (nCount > 0)
    {
        INT internalSelectedIndex = index - 1;
        if (internalSelectedIndex >= 0)
        {
            IFC(SelectItemHelper(internalSelectedIndex, -1));
            if (internalSelectedIndex != -1)
            {
                index = internalSelectedIndex;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Given a direction, searches through list for next available item to select.
_Check_return_
    HRESULT
    Selector::SelectItemHelper(
    _Inout_ INT& index,
    _In_ INT increment)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
    UINT nCount = 0;
    BOOLEAN isFocusable = FALSE;

    IFC(get_Items(&spItems));
    IFC(spItems.Cast<ItemCollection>()->get_Size(&nCount));

    for (; index > -1 && index < static_cast<INT>(nCount); index += increment)
    {
        ctl::ComPtr<IInspectable> spItem;

        IFC(spItems.Cast<ItemCollection>()->GetAt(index, &spItem));
        IFC(ItemsControl::IsFocusableHelper(spItem.Get(), isFocusable));

        if (isFocusable)
        {
            ctl::ComPtr<xaml::IDependencyObject> spContainer;

            IFC(ContainerFromIndex(index, &spContainer));
            IFC(ItemsControl::IsFocusableHelper(spContainer.Get(), isFocusable));

            if (isFocusable)
            {
                break;
            }
        }
    }

    if (!isFocusable)
    {
        // If no focusable item was found, set index to -1 so selection will not be updated.
        index = -1;
    }

Cleanup:
    RRETURN(hr);
}

// Get the ItemsHost's orientations. This is a stopgap while
// StackPanel doesn't implement IOrientedPanel. See IOrientedPanel
// for more information on what these two orientations mean.
// If the panel is neither a StackPanel nor a IOrientedPanel,
// both orientations are returned as Vertical.
_Check_return_
    HRESULT
    Selector::GetItemsHostOrientations(
    _Out_opt_ xaml_controls::Orientation* pPhysicalOrientation,
    _Out_opt_ xaml_controls::Orientation* pLogicalOrientation)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IPanel> spPanel;

    IFC(get_ItemsHost(&spPanel));
    IFC(GetPanelOrientations(spPanel.Get(), pPhysicalOrientation, pLogicalOrientation));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Selector::IsIndexSelected(_In_ int index, _Out_ bool* result)
{
    *result = false;

    if (index >= 0)
    {
        BOOLEAN isSelected;
        if (m_tpDataSourceAsSelectionInfo)
        {
            IFC_RETURN(m_tpDataSourceAsSelectionInfo->IsSelected(index, &isSelected));
        }
        else
        {
            UINT collectionIndexIgnored;
            IFC_RETURN(m_selection.Has(index, /* UINT& */ collectionIndexIgnored, /* BOOLEAN& */ isSelected));
        }

        *result = !!isSelected;
    }

    return S_OK;
}

// Get the given panel's orientations. This is a stopgap while
// StackPanel doesn't implement IOrientedPanel. See IOrientedPanel
// for more information on what these two orientations mean.
// If the panel is neither a StackPanel nor a IOrientedPanel,
// both orientations are returned as Vertical.
_Check_return_
    HRESULT
    Selector::GetPanelOrientations(
    _In_ IPanel* pPanel,
    _Out_opt_ xaml_controls::Orientation* pPhysicalOrientation,
    _Out_opt_ xaml_controls::Orientation* pLogicalOrientation)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IStackPanel> spStackPanel = NULL;
    xaml_controls::Orientation logicalOrientation =
        xaml_controls::Orientation_Vertical;
    xaml_controls::Orientation physicalOrientation =
        xaml_controls::Orientation_Vertical;

    spStackPanel = ctl::query_interface_cast<IStackPanel>(pPanel);
    if (spStackPanel)
    {
        IFC(spStackPanel.Cast<StackPanel>()->get_Orientation(&logicalOrientation));
        physicalOrientation = logicalOrientation;
    }
    else
    {
        ctl::ComPtr<IOrientedPanel> spOrientedPanel = NULL;
        spOrientedPanel = ctl::query_interface_cast<IOrientedPanel>(pPanel);
        if (spOrientedPanel)
        {
            IFC(spOrientedPanel->get_PhysicalOrientation(&physicalOrientation));
            IFC(spOrientedPanel->get_LogicalOrientation(&logicalOrientation));
        }
    }

    // TODO: Add support for arbitrary panels to provide their Orientation

Cleanup:
    if (pPhysicalOrientation)
    {
        *pPhysicalOrientation = physicalOrientation;
    }
    if (pLogicalOrientation)
    {
        *pLogicalOrientation = logicalOrientation;
    }
    RRETURN(hr);
}

// Indicate whether the specified item is currently visible.
_Check_return_
    HRESULT
    Selector::IsOnCurrentPage(
    _In_ INT index,
    _In_ BOOLEAN isGroupItemIndex,
    _In_ BOOLEAN allowPartialVisibility,
    _Out_opt_ wf::Rect* pScrollHostRect,
    _Out_opt_ wf::Rect* pContainerRect,
    _Out_ BOOLEAN& isOnCurrentPage)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDependencyObject> spDOContainer;
    ctl::ComPtr<xaml::IFrameworkElement> spContainer;
    wf::Rect emptyRect = {};

    if (pScrollHostRect)
    {
        *pScrollHostRect = emptyRect;
    }
    if (pContainerRect)
    {
        *pContainerRect = emptyRect;
    }

    if (isGroupItemIndex)
    {
        IFC(HeaderFromIndex(index, &spDOContainer));
    }
    else
    {
        IFC(ContainerFromIndex(index, &spDOContainer));
    }

    if (!spDOContainer)
    {
        // container is not realized
        isOnCurrentPage = FALSE;
        goto Cleanup;
    }

    IFC(spDOContainer.As(&spContainer));
    IFC(IsOnCurrentPage(spContainer.Cast<FrameworkElement>(), allowPartialVisibility, pScrollHostRect, pContainerRect, isOnCurrentPage));

Cleanup:
    RRETURN(hr);
}

// Indicate whether the specified item is currently visible.
_Check_return_
    HRESULT
    Selector::IsOnCurrentPage(
    _In_ FrameworkElement* pContainer,
    _In_ BOOLEAN allowPartialVisibility,
    _Out_opt_ wf::Rect* pScrollHostRect,
    _Out_opt_ wf::Rect* pContainerRect,
    _Out_ BOOLEAN& isOnCurrentPage)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IFrameworkElement> spScrollHost;
    ctl::ComPtr<xaml_media::IGeneralTransform> spTransform;
    wf::Rect emptyRect = {};
    wf::Rect scrollHostRect = {};
    wf::Rect containerRect = {};
    wf::Point containerCornerPoint = {};
    wf::Point containerSizePoint = {};
    wf::Point containerCornerTransformedPoint = {};
    wf::Point containerSizeTransformedPoint = {};
    DOUBLE actualHeight = 0;
    DOUBLE actualWidth = 0;
    xaml_controls::Orientation physicalOrientation = xaml_controls::Orientation_Vertical;
    isOnCurrentPage = TRUE;

    if (!m_tpScrollViewer)
    {
        goto Cleanup;
    }

    if (pScrollHostRect)
    {
        *pScrollHostRect = emptyRect;
    }
    if (pContainerRect)
    {
        *pContainerRect = emptyRect;
    }

    spScrollHost = m_tpScrollViewer.Cast<ScrollViewer>()->m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>();
    if (!spScrollHost)
    {
        spScrollHost = m_tpScrollViewer.Cast<ScrollViewer>();
    }

    // Get Rect for item host element
    if (!spScrollHost)
    {
        ASSERT(FALSE);
        goto Cleanup;
    }

    IFC(spScrollHost->get_ActualWidth(&actualWidth));
    IFC(spScrollHost->get_ActualHeight(&actualHeight));

    scrollHostRect.X = scrollHostRect.Y = 0;
    scrollHostRect.Width = static_cast<FLOAT>(actualWidth);
    scrollHostRect.Height = static_cast<FLOAT>(actualHeight);

    if (pScrollHostRect)
    {
        *pScrollHostRect = scrollHostRect;
    }

    if (!pContainer)
    {
        // container is not realized
        isOnCurrentPage = FALSE;
        goto Cleanup;
    }

    if (!pContainer->IsInLiveTree())
    {
        // container is not in live tree
        isOnCurrentPage = FALSE;
        goto Cleanup;
    }

    IFC(pContainer->TransformToVisual(spScrollHost.Cast<FrameworkElement>(), &spTransform));

    IFC(pContainer->get_ActualWidth(&actualWidth));
    IFC(pContainer->get_ActualHeight(&actualHeight));

    if (static_cast<CUIElement*>(pContainer->GetHandle())->GetIsMeasureDirty() ||
        actualWidth == 0 && actualHeight == 0)
    {
        // container has not been measured or was measured while outside the visible area, this can lead us to believe the container
        // is on current page when it is not. Set isOnCurrentPage to false to ensure we try to scroll if necessary.
        isOnCurrentPage = FALSE;
        goto Cleanup;
    }

    containerSizePoint.X = static_cast<FLOAT>(actualWidth);
    containerSizePoint.Y = static_cast<FLOAT>(actualHeight);

    // Get relative Rect for Container
    IFC(spTransform->TransformPoint(containerCornerPoint, &containerCornerTransformedPoint));
    IFC(spTransform->TransformPoint(containerSizePoint, &containerSizeTransformedPoint));

    containerRect.X = static_cast<FLOAT>(DoubleUtil::Min(containerCornerTransformedPoint.X, containerSizeTransformedPoint.X));
    containerRect.Y = static_cast<FLOAT>(DoubleUtil::Min(containerCornerTransformedPoint.Y, containerSizeTransformedPoint.Y));

    containerRect.Width = static_cast<FLOAT>(DoubleUtil::Max(DoubleUtil::Max(containerCornerTransformedPoint.X, containerSizeTransformedPoint.X) - containerRect.X, 0));
    containerRect.Height = static_cast<FLOAT>(DoubleUtil::Max(DoubleUtil::Max(containerCornerTransformedPoint.Y, containerSizeTransformedPoint.Y) - containerRect.Y, 0));

    IFC(GetItemsHostOrientations(&physicalOrientation, nullptr /*pLogicalOrientation*/));

    // Return result
    if (allowPartialVisibility)
    {
        // A partially visible item on the page is considered to be on it.
        if (physicalOrientation == xaml_controls::Orientation_Vertical)
        {
            isOnCurrentPage = !(containerRect.Y >= scrollHostRect.Y + scrollHostRect.Height) &&
                !(scrollHostRect.Y >= containerRect.Y + containerRect.Height);
        }
        else
        {
            isOnCurrentPage = !(containerRect.X >= scrollHostRect.X + scrollHostRect.Width) &&
                !(scrollHostRect.X >= containerRect.X + containerRect.Width);
        }
    }
    else
    {
        // Check for total visibility on the page.
        isOnCurrentPage = (physicalOrientation == xaml_controls::Orientation_Vertical ?
            (scrollHostRect.Y <= containerRect.Y) && (containerRect.Y + containerRect.Height <= scrollHostRect.Y + scrollHostRect.Height) :
        (scrollHostRect.X <= containerRect.X) && (containerRect.X + containerRect.Width <= scrollHostRect.X + scrollHostRect.Width));
    }

    if (pContainerRect)
    {
        *pContainerRect = containerRect;
    }

Cleanup:
    RRETURN(hr);
}

// Get the first visible item. If possible, return an item that's completely visible on the page.
// If no item is completely contained on the page, get the first item that is partially visible in the search
// direction.
_Check_return_
    HRESULT
    Selector::GetFirstItemOnCurrentPage(
    _Inout_ INT& startingIndex,
    _In_ BOOLEAN forward)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
    INT delta = (forward ? 1 : -1);
    INT probeIndex = startingIndex;
    UINT nCount = 0;
    BOOLEAN isOnCurrentPage = FALSE;

    IFC(get_Items(&spItems));
    IFC(spItems.Cast<ItemCollection>()->get_Size(&nCount));

    // Scan looking for the first visible element. Allow partial visibility here,
    // we want to know when we've hit the page.
    while ((0 <= probeIndex) && (probeIndex < static_cast<INT>(nCount)))
    {
        IFC(IsOnCurrentPage(
            probeIndex,
            FALSE, // isGroupItemIndex
            TRUE, // allowPartialVisibility
            NULL, // pScrollHostRect
            NULL, // pContainerRect
            isOnCurrentPage));
        if (isOnCurrentPage)
        {
            break;
        }

        startingIndex = probeIndex;
        probeIndex += delta;
    }

    if (isOnCurrentPage)
    {
        // This match on the current page may be a partial match. Advance and check if there's at least one full match after it.
        isOnCurrentPage = FALSE;
        startingIndex = probeIndex;
        probeIndex += delta;
        if ((0 <= probeIndex) && (probeIndex < static_cast<INT>(nCount)))
        {
            IFC(IsOnCurrentPage(
                probeIndex,
                FALSE, // isGroupItemIndex
                FALSE, // allowPartialVisibility
                NULL,  // pScrollHostRect
                NULL,  // pContainerRect
                isOnCurrentPage));
        }

        // Since there's at least one fully visible, keep searching for more in the search direction. Stop
        // as soon as an item is encountered that is not completely visible, and return the item before it.
        // If no item is completely visible, we won't enter this loop and will just return the first partially
        // visible item found.
        if (isOnCurrentPage)
        {
            startingIndex = probeIndex;
            probeIndex += delta;

            // Then scan looking for the last fully visible element.
            while ((0 <= probeIndex) && (probeIndex < static_cast<INT>(nCount)))
            {
                isOnCurrentPage = FALSE;

                IFC(IsOnCurrentPage(
                    probeIndex,
                    FALSE, // isGroupItemIndex
                    FALSE, // allowPartialVisibility
                    NULL,  // pScrollHostRect
                    NULL,  // pContainerRect
                    isOnCurrentPage));

                if (!isOnCurrentPage)
                {
                    break;
                }

                startingIndex = probeIndex;
                probeIndex += delta;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Move the focus forward/backward one page.
_Check_return_
    HRESULT
    Selector::NavigateByPage(
    _In_ BOOLEAN forward,
    _Out_ INT& newFocusedIndex)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
    ctl::ComPtr<IInspectable> spItem;

    newFocusedIndex = GetFocusedIndex();
    IFC(get_Items(&spItems));
    if (newFocusedIndex >= 0)
    {
        IFC(spItems.Cast<ItemCollection>()->GetAt(newFocusedIndex, &spItem));
    }

    // Get it visible to start with
    if (spItem)
    {
        BOOLEAN isOnCurrentPage = FALSE;
        IFC(IsOnCurrentPage(
            newFocusedIndex,
            FALSE, // isGroupItemIndex
            TRUE, // allowPartialVisibility - in this case, we will not scroll selected item into view if it's already partially visible, it does not have to completely visible.
            NULL, // pScrollHostRect
            NULL, // pContainerRect
            isOnCurrentPage));
        if (!isOnCurrentPage)
        {
            IFC(ScrollIntoView(
                newFocusedIndex,
                FALSE /*isGroupItemIndex*/,
                FALSE /*isHeader*/,
                FALSE /*isFooter*/,
                FALSE /*isFromPublicAPI*/,
                TRUE  /*ensureContainerRealized*/,
                FALSE /*animateIfBringIntoView*/,
                xaml_controls::ScrollIntoViewAlignment::ScrollIntoViewAlignment_Default));

            if (m_tpScrollViewer)
            {
                IFC(m_tpScrollViewer.Cast<ScrollViewer>()->UpdateLayout());
            }
        }
    }
    // Inlined implementation of NavigateByPageInternal
    if (!spItem)
    {
        // Select something
        IFC(GetFirstItemOnCurrentPage(newFocusedIndex, forward));
    }
    else
    {
        IFC(GetFirstItemOnCurrentPage(newFocusedIndex, forward));
        if (newFocusedIndex == GetFocusedIndex())
        {
            INT focusedIndex = newFocusedIndex;
            if (m_tpScrollViewer)
            {
                xaml_controls::Orientation physicalOrientation = xaml_controls::Orientation_Vertical;

                IFC(GetItemsHostOrientations(&physicalOrientation, NULL /*pLogicalOrientation*/));
                // Scroll a page in the relevant direction
                if (physicalOrientation == xaml_controls::Orientation_Vertical)
                {
                    if (forward)
                    {
                        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->PageDown());
                    }
                    else
                    {
                        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->PageUp());
                    }
                }
                else
                {
                    if (forward)
                    {
                        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->PageRight());
                    }
                    else
                    {
                        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->PageLeft());
                    }
                }
                IFC(m_tpScrollViewer.Cast<ScrollViewer>()->UpdateLayout());
            }

            // Select the "edge" element
            IFC(GetFirstItemOnCurrentPage(newFocusedIndex, forward));

            if (newFocusedIndex == focusedIndex)
            {
                UINT nCount = 0;
                IFC(spItems.Cast<ItemCollection>()->get_Size(&nCount));

                // Page navigation has to advance by at least 1 item. If items are > control's size,
                // even after a PageUp/Down this may not happen.
                if (forward &&
                    nCount > 0 &&
                    (newFocusedIndex < static_cast<INT>(nCount - 1)))
                {
                    newFocusedIndex++;
                }
                else if (!forward && newFocusedIndex > 0)
                {
                    newFocusedIndex--;
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
    HRESULT
    Selector::ScrollIntoViewInternal(
    _In_ IInspectable* item,
    _In_ BOOLEAN isHeader,
    _In_ BOOLEAN isFooter,
    _In_ BOOLEAN isFromPublicAPI,
    _In_ xaml_controls::ScrollIntoViewAlignment alignment,
    _In_ DOUBLE offset,
    _In_ BOOLEAN animateIfBringIntoView)
{
    HRESULT hr = S_OK;
    BOOLEAN found = FALSE;
    UINT index = 0;

    if (isHeader || isFooter)
    {
        // Index is not used when isHeader or isFooter is set to true.
        IFC(ScrollIntoView(
            1 /*index*/,
            FALSE /*isGroupItemIndex*/,
            isHeader,
            isFooter,
            isFromPublicAPI,
            FALSE /*ensureContainerRealized*/,
            animateIfBringIntoView,
            alignment,
            offset));
    }
    else
    {
        // First search in group items
        IFC(GetGroupItemIndex(item, &index, &found));
        if (found)
        {
            IFC(ScrollIntoView(index,
                TRUE  /*isGroupItemIndex*/,
                FALSE /*isHeader*/,
                FALSE /*isFooter*/,
                isFromPublicAPI,
                FALSE /*ensureContainerRealized*/,
                animateIfBringIntoView,
                alignment,
                offset));
        }
        else
        {
            ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;

            // if not found, search in the ItemCollection
            IFC(get_Items(&spItems));
            IFC(spItems.Cast<ItemCollection>()->IndexOf(item, &index, &found));
            if (found)
            {
                IFC(ScrollIntoView(index,
                    FALSE /*isGroupItemIndex*/,
                    FALSE /*isHeader*/,
                    FALSE /*isFooter*/,
                    isFromPublicAPI,
                    FALSE /*ensureContainerRealized*/,
                    animateIfBringIntoView,
                    alignment,
                    offset));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// if isGroupItemIndex is TRUE, the given index is groupItemIndex
// currentGroupIndex is only valid when isGroupItemIndex and animateIfBringIntoView are TRUE.
_Check_return_
    HRESULT
    Selector::ScrollIntoView(
    _In_ UINT index,
    _In_ BOOLEAN isGroupItemIndex,
    _In_ BOOLEAN isHeader,
    _In_ BOOLEAN isFooter,
    _In_ BOOLEAN isFromPublicAPI,
    _In_ BOOLEAN ensureContainerRealized,
    _In_ BOOLEAN animateIfBringIntoView,
    _In_ xaml_controls::ScrollIntoViewAlignment alignment,
    _In_ DOUBLE offset,
    _In_ UINT currentGroupIndex)
{
#ifdef SLTR_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"SLTR[0x%p]: ScrollIntoView. index=%d, isGroupItemIndex=%d, isHeader=%d, isFooter=%d, ensureContainerRealized=%d, animateIfBringIntoView=%d, alignment=%d, offset=%lf",
        this, index, isGroupItemIndex, isHeader, isFooter, ensureContainerRealized, animateIfBringIntoView, alignment, offset));
#endif

    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spContent;
    ctl::ComPtr<IItemsPresenter> spItemsPresenter;
    ctl::ComPtr<IPanel> spPanel;
    ctl::ComPtr<IModernCollectionBasePanel> spModernPanel;
    wf::Rect scrollHostRect = {};
    wf::Rect containerRect = {};
    BOOLEAN isOnCurrentPage = FALSE;
    BOOLEAN isThinkingInLogicalUnits = FALSE;
    xaml_controls::Orientation physicalOrientation = xaml_controls::Orientation_Vertical;
    BOOLEAN isVertical = FALSE;

    if (!m_tpScrollViewer)
    {
        goto Cleanup;
    }

    IFC(m_tpScrollViewer.Cast<ScrollViewer>()->get_Content(&spContent));

    spItemsPresenter = spContent.AsOrNull<IItemsPresenter>();

    if (!spItemsPresenter)
    {
        goto Cleanup;
    }

    // If we have a ModernCollectionPanel, we're going to delegate the work there
    IFC(get_ItemsHost(&spPanel));
    spModernPanel = spPanel.AsOrNull<IModernCollectionBasePanel>();
    if (spModernPanel && !isHeader && !isFooter)
    {
        // our panel can deal with group headers and items, not with the ListView.Header property
        if (isGroupItemIndex)
        {
            UINT neighboringItemIndex = 0;
            if (animateIfBringIntoView)
            {
                IFC(spModernPanel.Cast<ModernCollectionBasePanel>()->GetNeighboringItemIndex(currentGroupIndex, index /*newGroupIndex*/, &neighboringItemIndex));
            }

            IFC(spModernPanel.Cast<ModernCollectionBasePanel>()->ScrollGroupHeaderIntoView(
                index,
                alignment,
                offset,
                ensureContainerRealized /*forceSynchronous*/,
                animateIfBringIntoView /*animate*/,
                neighboringItemIndex));
        }
        else
        {
            IFC(spModernPanel.Cast<ModernCollectionBasePanel>()->ScrollItemIntoView(
                index,
                alignment,
                offset,
                ensureContainerRealized /*forceSynchronous*/,
                animateIfBringIntoView /*animate*/));
        }
        goto Cleanup;
    }

    bool reevaluateContainerRect;
    bool canReevaluateContainerRect = true;

    do
    {
        reevaluateContainerRect = false;

        // For ScrollIntoView, if the item is not completely on the current page, it's not considered visible. pass allowPartialVisibility = FALSE to
        // IsOnCurrentPage.
        if (isHeader)
        {
            ctl::ComPtr<ContentControl> spHeaderContainer;

            IFC(spItemsPresenter.Cast<ItemsPresenter>()->get_HeaderContainer(&spHeaderContainer));
            IFC(IsOnCurrentPage(spHeaderContainer.Get(), /*allowPartialVisibility*/FALSE, &scrollHostRect, &containerRect, isOnCurrentPage));
        }
        else if (isFooter)
        {
            ctl::ComPtr<ContentControl> spFooterContainer;

            // Force footer to load.
            IFC(spItemsPresenter.Cast<ItemsPresenter>()->LoadFooter(TRUE /* updateLayout */));

            IFC(spItemsPresenter.Cast<ItemsPresenter>()->get_FooterContainer(&spFooterContainer));
            IFC(IsOnCurrentPage(spFooterContainer.Get(), /*allowPartialVisibility*/FALSE, &scrollHostRect, &containerRect, isOnCurrentPage));
        }
        else
        {
            IFC(IsOnCurrentPage(index, isGroupItemIndex, /*allowPartialVisibility*/FALSE, &scrollHostRect, &containerRect, isOnCurrentPage));
        }

        if (isOnCurrentPage && xaml_controls::ScrollIntoViewAlignment_Default == alignment && offset == 0.0)
        {
            goto Cleanup;
        }

        IFC(GetItemsHostOrientations(&physicalOrientation, NULL /*pLogicalOrientation*/));
        isVertical = (physicalOrientation == xaml_controls::Orientation_Vertical);

        // special case. If first item is not on current page it might be scrolled by huge header (or header + leading padding).
        // in this case containerRect will have positive offset. But for CarouselPanel we will have item0 after itemN and item0 will have positive offset too.
        // We will ignore ScrollIntoView request for non-public calls when the offset are positive and our ItemsHost panel is not CarouselPanel.
        // we probably need the same special case for the trailing padding.
        if (!isFromPublicAPI && index == 0 &&
            !isOnCurrentPage &&
            xaml_controls::ScrollIntoViewAlignment_Default == alignment &&
            !ctl::is<ICarouselPanel>(spPanel))
        {
            if (isVertical)
            {
                if (containerRect.Y > 0)
                {
                    if (canReevaluateContainerRect && !spModernPanel && containerRect.X == VirtualizingPanel::ExtraContainerArrangeOffset)
                    {
                        // This situation may be encountered when processing the Home key while the ItemsPanel is a VirtualizingStackPanel. Some elements 
                        // are rendered off-screen temporarily. Forcing a layout and re-evaluating containerRect to avoid failure of the keystroke processing.
                        canReevaluateContainerRect = false;
                        reevaluateContainerRect = true;
                        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->UpdateLayout());
                    }
                    else
                    {
                        goto Cleanup;
                    }
                }
            }
            else
            {
                if (containerRect.X > 0)
                {
                    if (canReevaluateContainerRect && !spModernPanel && containerRect.Y == VirtualizingPanel::ExtraContainerArrangeOffset)
                    {
                        // This situation may be encountered when processing the Home key while the ItemsPanel is a VirtualizingStackPanel. Some elements 
                        // are rendered off-screen temporarily. Forcing a layout and re-evaluating containerRect to avoid failure of the keystroke processing.
                        canReevaluateContainerRect = false;
                        reevaluateContainerRect = true;
                        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->UpdateLayout());
                    }
                    else
                    {
                        goto Cleanup;
                    }
                }
            }
        }
    }
    while (reevaluateContainerRect);

    {
        ctl::ComPtr<VirtualizingStackPanelFactory> spFactory;

        IFC(ctl::make<VirtualizingStackPanelFactory>(&spFactory));
        IFC(spFactory->GetIsVirtualizing(this, &isThinkingInLogicalUnits));

        isThinkingInLogicalUnits &= static_cast<BOOLEAN>(ctl::is<IVirtualizingPanel>(spPanel));
    }

    // offset is not supported for legacy panels.
    ASSERT(offset == 0.0 || !isThinkingInLogicalUnits);

    // If it is Virtualizing, let it decide the ScrollIntoView
    // Note: we don't support footers for virtualizing panels.
    if (isThinkingInLogicalUnits && !isFooter)
    {
        DOUBLE pixelOffset = 0;

        if (isVertical)
        {
            // if container exists
            if (containerRect.Y != 0 || containerRect.Height != 0)
            {
                pixelOffset = containerRect.Y + containerRect.Height - scrollHostRect.Height;
            }
        }
        else
        {
            if (containerRect.X != 0 || containerRect.Width != 0)
            {
                pixelOffset = containerRect.X + containerRect.Width - scrollHostRect.Width;
            }
        }

        IFC(spItemsPresenter.Cast<ItemsPresenter>()->ScrollIntoView(index, isGroupItemIndex, isHeader, ensureContainerRealized /*forceSynchronous*/, pixelOffset, alignment));
    }
    else if (!isThinkingInLogicalUnits)
    {
        IFC(ScrollRectToViewport(containerRect, scrollHostRect, animateIfBringIntoView && spModernPanel != nullptr /*animateIfBringIntoView*/, alignment, offset, spItemsPresenter.Get()));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
    HRESULT
    Selector::ScrollRectToViewport(
    _In_ wf::Rect containerRect,
    _In_ wf::Rect scrollHostRect,
    _In_ BOOLEAN animateIfBringIntoView,
    _In_ xaml_controls::ScrollIntoViewAlignment alignment,
    _In_ DOUBLE offset,
    _In_ xaml_controls::IItemsPresenter* pItemsPresenter)
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation physicalOrientation = xaml_controls::Orientation_Vertical;

    IFC(GetItemsHostOrientations(&physicalOrientation, NULL /*pLogicalOrientation*/));

    ASSERT(pItemsPresenter);

    if (physicalOrientation == xaml_controls::Orientation_Vertical)
    {
        // Scroll into view vertically (first make the right bound visible, then the left)
        DOUBLE verticalOffset = 0.0;

        IFC(m_tpScrollViewer->get_VerticalOffset(&verticalOffset));

        switch (alignment)
        {
        case xaml_controls::ScrollIntoViewAlignment_Leading:
            {
                DOUBLE viewportWidth = 0.0;
                DOUBLE viewportHeight = 0.0;

                IFC(m_tpScrollViewer->get_ViewportWidth(&viewportWidth));
                IFC(m_tpScrollViewer->get_ViewportHeight(&viewportHeight));

                wf::Rect futureViewportRect = {
                    /* X = */ 0.0f,
                    /* Y = */ static_cast<FLOAT>(verticalOffset + containerRect.Y),
                    /* Width = */ static_cast<FLOAT>(viewportWidth),
                    /* Height = */ static_cast<FLOAT>(viewportHeight)
                };

                IFC(static_cast<ItemsPresenter*>(pItemsPresenter)->DelayLoadFooter(&futureViewportRect, TRUE /* updateLayout */));
                verticalOffset += containerRect.Y + offset;
                break;
            }
        case xaml_controls::ScrollIntoViewAlignment_Default:
        default:
            {
                DOUBLE verticalDelta = 0.0;
                DOUBLE scrollHostRectBottom = scrollHostRect.Y + scrollHostRect.Height;
                DOUBLE containerRectBottom = containerRect.Y + containerRect.Height;
                if (scrollHostRectBottom < containerRectBottom)
                {
                    verticalDelta = containerRectBottom - scrollHostRectBottom;
                    verticalOffset += verticalDelta;
                }
                if (containerRect.Y - verticalDelta < scrollHostRect.Y)
                {
                    verticalOffset -= scrollHostRect.Y - (containerRect.Y - verticalDelta);
                }
                verticalOffset += offset;
                break;
            }
        }

        if (animateIfBringIntoView)
        {
            BOOLEAN returnValueIgnored;
            wrl::ComPtr<IInspectable> spVerticalOffsetAsInspectable;
            wrl::ComPtr<wf::IReference<DOUBLE>> spVerticalOffset;

            IFC(PropertyValue::CreateFromDouble(verticalOffset, &spVerticalOffsetAsInspectable));
            IFC(spVerticalOffsetAsInspectable.As(&spVerticalOffset));

            IFC(m_tpScrollViewer.Cast<ScrollViewer>()->ChangeViewWithOptionalAnimation(
                nullptr,                // horizontalOffset
                spVerticalOffset.Get(), // verticalOffset
                nullptr,                // zoomFactor
                FALSE,                  // disableAnimation
                &returnValueIgnored));
        }
        else
        {
            IFC(m_tpScrollViewer.Cast<ScrollViewer>()->ScrollToVerticalOffsetInternal(verticalOffset));
        }
    }
    else
    {
        // Scroll into view horizontally (first make the bottom bound visible, then the top)
        DOUBLE horizontalOffset = 0.0;

        IFC(m_tpScrollViewer->get_HorizontalOffset(&horizontalOffset));

        switch (alignment)
        {
        case xaml_controls::ScrollIntoViewAlignment_Leading:
            {
                DOUBLE viewportWidth = 0.0;
                DOUBLE viewportHeight = 0.0;

                IFC(m_tpScrollViewer->get_ViewportWidth(&viewportWidth));
                IFC(m_tpScrollViewer->get_ViewportHeight(&viewportHeight));

                wf::Rect futureViewportRect = {
                    /* X = */ static_cast<FLOAT>(horizontalOffset + containerRect.X),
                    /* Y = */ 0.0f,
                    /* Width = */ static_cast<FLOAT>(viewportWidth),
                    /* Height = */ static_cast<FLOAT>(viewportHeight)
                };

                IFC(static_cast<ItemsPresenter*>(pItemsPresenter)->DelayLoadFooter(&futureViewportRect, TRUE /* updateLayout */));
                horizontalOffset += containerRect.X + offset;
                break;
            }
        case xaml_controls::ScrollIntoViewAlignment_Default:
        default:
            {
                DOUBLE horizontalDelta = 0.0;
                DOUBLE scrollHostRectRight = scrollHostRect.X + scrollHostRect.Width;
                DOUBLE containerRectRight = containerRect.X + containerRect.Width;
                if (scrollHostRectRight < containerRectRight)
                {
                    horizontalDelta = containerRectRight - scrollHostRectRight;
                    horizontalOffset += horizontalDelta;
                }
                if (containerRect.X - horizontalDelta < scrollHostRect.X)
                {
                    horizontalOffset -= scrollHostRect.X - (containerRect.X - horizontalDelta);
                }
                horizontalOffset += offset;
                break;
            }
        }

        if (animateIfBringIntoView)
        {
            BOOLEAN returnValueIgnored;
            wrl::ComPtr<IInspectable> spHorizontalOffsetAsInspectable;
            wrl::ComPtr<wf::IReference<DOUBLE>> spHorizontalOffset;

            IFC(PropertyValue::CreateFromDouble(horizontalOffset, &spHorizontalOffsetAsInspectable));
            IFC(spHorizontalOffsetAsInspectable.As(&spHorizontalOffset));

            IFC(m_tpScrollViewer.Cast<ScrollViewer>()->ChangeViewWithOptionalAnimation(
                spHorizontalOffset.Get(), // horizontalOffset
                nullptr,                  // verticalOffset
                nullptr,                  // zoomFactor
                FALSE,                    // disableAnimation
                &returnValueIgnored));
        }
        else
        {
            IFC(m_tpScrollViewer.Cast<ScrollViewer>()->ScrollToHorizontalOffsetInternal(horizontalOffset));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Call ElementScrollViewer.ScrollInDirection if possible.
_Check_return_
    HRESULT
    Selector::ElementScrollViewerScrollInDirection(
    _In_ wsy::VirtualKey key,
    _In_ BOOLEAN animate)
{
    HRESULT hr = S_OK;

    // When moving to C++20 this probably needs to change to the following:
    // if (nullptr != m_tpScrollViewer)
    // There is ambiguity as to which comparison operator should be used and that warning is treated as a break.
    if (NULL != m_tpScrollViewer)
    {
        if (animate)
        {
            // This is a move request within a header or footer. Only perform an animated move when the hosting panel is a modern panel. Moves from item to item
            // are only animated for modern panels. So for consistency, moves within headers are only animated for modern panels as well.

            ctl::ComPtr<IPanel> spPanel;
            ctl::ComPtr<IModernCollectionBasePanel> spModernPanel;

            IFC(get_ItemsHost(&spPanel));
            spModernPanel = spPanel.AsOrNull<IModernCollectionBasePanel>();

            animate = spModernPanel != nullptr;
        }

        if (animate)
        {
            IFC(m_tpScrollViewer.Cast<ScrollViewer>()->ScrollInDirection(key, true /*animate*/));
        }
        else
        {
            xaml_controls::Orientation physicalOrientation = xaml_controls::Orientation_Vertical;
            xaml::FlowDirection direction = xaml::FlowDirection_LeftToRight;
            BOOLEAN isVertical = FALSE;
            BOOLEAN invert = FALSE;

            IFC(GetItemsHostOrientations(&physicalOrientation, NULL /*pLogicalOrientation*/));
            isVertical = (physicalOrientation == xaml_controls::Orientation_Vertical);

            IFC(get_FlowDirection(&direction));
            invert = direction == xaml::FlowDirection_RightToLeft;

            switch (key)
            {
            case wsy::VirtualKey_PageUp:
                if (isVertical)
                {
                    IFC(m_tpScrollViewer.Cast<ScrollViewer>()->PageUp());
                }
                else
                {
                    if (invert)
                    {
                        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->PageRight());
                    }
                    else
                    {
                        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->PageLeft());
                    }
                }
                break;
            case wsy::VirtualKey_PageDown:
                if (isVertical)
                {
                    IFC(m_tpScrollViewer.Cast<ScrollViewer>()->PageDown());
                }
                else
                {
                    if (invert)
                    {
                        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->PageLeft());
                    }
                    else
                    {
                        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->PageRight());
                    }
                }
                break;
            case wsy::VirtualKey_Home:
                if (isVertical)
                {
                    IFC(m_tpScrollViewer.Cast<ScrollViewer>()->HandleVerticalScroll(xaml_primitives::ScrollEventType_First));
                }
                else
                {
                    IFC(m_tpScrollViewer.Cast<ScrollViewer>()->HandleHorizontalScroll(xaml_primitives::ScrollEventType_First));
                }
                break;
            case wsy::VirtualKey_End:
                if (isVertical)
                {
                    IFC(m_tpScrollViewer.Cast<ScrollViewer>()->HandleVerticalScroll(xaml_primitives::ScrollEventType_Last));
                }
                else
                {
                    IFC(m_tpScrollViewer.Cast<ScrollViewer>()->HandleHorizontalScroll(xaml_primitives::ScrollEventType_Last));
                }
                break;
            default:
                IFC(m_tpScrollViewer.Cast<ScrollViewer>()->ScrollInDirection(key, false /*animate*/));
                break;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
    HRESULT
    Selector::HandleNavigationKey(
    _In_ wsy::VirtualKey key,
    _In_ BOOLEAN scrollViewport,
    _Inout_ INT& newFocusedIndex)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
    BOOLEAN bInvertForRTL = FALSE;
    BOOLEAN isVertical = FALSE;
    xaml::FlowDirection direction = xaml::FlowDirection_LeftToRight;
    xaml_controls::Orientation physicalOrientation = xaml_controls::Orientation_Vertical;
    UINT nCount = 0;

    IFC(get_Items(&spItems));
    IFC(spItems.Cast<ItemCollection>()->get_Size(&nCount));

    IFC(get_FlowDirection(&direction));
    bInvertForRTL = (direction == xaml::FlowDirection_RightToLeft);
    IFC(GetItemsHostOrientations(&physicalOrientation, NULL /*pLogicalOrientation*/));
    isVertical = (physicalOrientation == xaml_controls::Orientation_Vertical);
    switch (key)
    {
    case wsy::VirtualKey_Left:
        if (isVertical && scrollViewport)
        {
            IFC(ElementScrollViewerScrollInDirection(wsy::VirtualKey_Left));
        }
        else
        {
            if (bInvertForRTL)
            {
                IFC(SelectNext(newFocusedIndex));
            }
            else
            {
                IFC(SelectPrev(newFocusedIndex));
            }

            if (GetFocusedIndex() == newFocusedIndex && scrollViewport)
            {
                IFC(ElementScrollViewerScrollInDirection(wsy::VirtualKey_Left));
            }
        }
        break;
    case wsy::VirtualKey_Up:
        if (!isVertical && scrollViewport)
        {
            IFC(ElementScrollViewerScrollInDirection(wsy::VirtualKey_Up));
        }
        else
        {
            IFC(SelectPrev(newFocusedIndex));
        }

        if (GetFocusedIndex() == newFocusedIndex && scrollViewport)
        {
            IFC(ElementScrollViewerScrollInDirection(wsy::VirtualKey_Up));
        }
        break;
    case wsy::VirtualKey_Right:
        if (isVertical && scrollViewport)
        {
            IFC(ElementScrollViewerScrollInDirection(wsy::VirtualKey_Right));
        }
        else
        {
            if (bInvertForRTL)
            {
                IFC(SelectPrev(newFocusedIndex));
            }
            else
            {
                IFC(SelectNext(newFocusedIndex));
            }

            if (GetFocusedIndex() == newFocusedIndex && scrollViewport)
            {
                IFC(ElementScrollViewerScrollInDirection(wsy::VirtualKey_Right));
            }
        }
        break;
    case wsy::VirtualKey_Down:
        if (!isVertical && scrollViewport)
        {
            IFC(ElementScrollViewerScrollInDirection(wsy::VirtualKey_Down));
        }
        else
        {
            IFC(SelectNext(newFocusedIndex));
        }

        if (GetFocusedIndex() == newFocusedIndex && scrollViewport)
        {
            IFC(ElementScrollViewerScrollInDirection(wsy::VirtualKey_Down));
        }
        break;
    case wsy::VirtualKey_Home:
        newFocusedIndex = 0;
        break;
    case wsy::VirtualKey_End:
        newFocusedIndex = nCount - 1;
        break;
    case wsy::VirtualKey_PageUp:
        IFC(NavigateByPage(/*forward*/FALSE, newFocusedIndex));
        break;
    case wsy::VirtualKey_GamepadLeftTrigger:
        if (isVertical)
        {
            IFC(NavigateByPage(/*forward*/FALSE, newFocusedIndex));
        }
        break;
    case wsy::VirtualKey_GamepadLeftShoulder:
        if (!isVertical)
        {
            IFC(NavigateByPage(/*forward*/FALSE, newFocusedIndex));
        }
        break;
    case wsy::VirtualKey_PageDown:
        IFC(NavigateByPage(/*forward*/TRUE, newFocusedIndex));
        break;
    case wsy::VirtualKey_GamepadRightTrigger:
        if (isVertical)
        {
            IFC(NavigateByPage(/*forward*/TRUE, newFocusedIndex));
        }
        break;
    case wsy::VirtualKey_GamepadRightShoulder:
        if (!isVertical)
        {
            IFC(NavigateByPage(/*forward*/TRUE, newFocusedIndex));
        }
        break;
    }
    newFocusedIndex = static_cast<INT>(MIN(newFocusedIndex, static_cast<INT>(nCount) - 1));
    newFocusedIndex = static_cast<INT>(MAX(newFocusedIndex, -1));

Cleanup:
    RRETURN(hr);
}

_Check_return_
    HRESULT
    Selector::AutomationPeerAddToSelection(
    _In_ UINT index,
    _In_ IInspectable* pItem)
{
    HRESULT hr = S_OK;
    SelectionChanger* pSelectionChanger = NULL;

    if (m_tpDataSourceAsSelectionInfo)
    {
        IFC(InvokeDataSourceRangeSelection(true /* select */, index, 1));
    }
    else
    {
        BOOLEAN canSelectMultiple = FALSE;
        IFC(get_CanSelectMultiple(&canSelectMultiple));

        // Use a SelectionChanger instance to prevent reentrancy or
        // other properties from being updated.
        IFC(BeginChange(&pSelectionChanger));

        IFC(pSelectionChanger->Select(index, pItem, canSelectMultiple));

        IFC(EndChange(pSelectionChanger));
        pSelectionChanger = NULL;
    }

Cleanup:
    if (pSelectionChanger)
    {
        VERIFYRETURNHR(pSelectionChanger->Cancel());
    }
    return hr;
}

_Check_return_
    HRESULT
    Selector::AutomationPeerRemoveFromSelection(
    _In_ UINT index,
    _In_ IInspectable* pItem)
{
    HRESULT hr = S_OK;
    SelectionChanger* pSelectionChanger = NULL;

    if (m_tpDataSourceAsSelectionInfo)
    {
        IFC(InvokeDataSourceRangeSelection(false /* select */, index, 1));
    }
    else
    {
        // Use a SelectionChanger instance to prevent reentrancy or
        // other properties from being updated.
        IFC(BeginChange(&pSelectionChanger));

        IFC(pSelectionChanger->Unselect(index, pItem));

        IFC(EndChange(pSelectionChanger));
        pSelectionChanger = NULL;
    }

Cleanup:
    if (pSelectionChanger)
    {
        VERIFYRETURNHR(pSelectionChanger->Cancel());
    }
    return hr;
}

_Check_return_
    HRESULT
    Selector::AutomationPeerIsSelected(
    _In_ IInspectable* item,
    _Out_ BOOLEAN* isSelected)
{
    *isSelected = FALSE;

    if (m_tpDataSourceAsSelectionInfo)
    {
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
        UINT index = 0;
        BOOLEAN found = FALSE;

        IFC_RETURN(get_Items(&spItems));

        IFC_RETURN(spItems.Cast<ItemCollection>()->IndexOf(item, &index, &found));
        if (found)
        {
            IFC_RETURN(m_tpDataSourceAsSelectionInfo->IsSelected(index, isSelected));
        }
    }
    else
    {
        wfc::IVector<IInspectable*>* selectedItems = NULL;
        UINT index = -1;

        auto releaseGuard = wil::scope_exit([&]()
        {
            ReleaseInterface(selectedItems);
        });

        IFC_RETURN(get_SelectedItemsInternal(&selectedItems));
        if (selectedItems)
        {
            IFC_RETURN(static_cast<ObservableTrackerCollection<IInspectable*>*>(selectedItems)->IndexOf(item, &index, isSelected));
        }
    }

    return S_OK;
}

// Gets first element that should take focus after Tab.
// Return NULL to default to FocusManager logic.
// Selector returns previous focused element.
_Check_return_ HRESULT Selector::GetFirstFocusableElementOverride(
    _Outptr_result_maybenull_ DependencyObject** ppFirstFocusable)
{
#ifdef SLTR_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"SLTR[0x%p]: GetFirstFocusableElementOverride."));
#endif

    *ppFirstFocusable = nullptr;

    if (!m_skipFocusSuggestion)
    {
        ctl::ComPtr<IDependencyObject> spFirstFocusableResult;

        // For ListView it is important to check if the focus is coming from outside due to issues in scenarios with header and group header.
        // Since we do not support those scenarios with the rest of the selectors, every time we are asked for first focusable element
        // we can give out the last focused element.
        IFC_RETURN(ContainerFromIndex(static_cast<INT>(GetLastFocusedIndex()), &spFirstFocusableResult));
        *ppFirstFocusable = static_cast<DependencyObject*>(spFirstFocusableResult.Detach());
    }

    return S_OK;
}

// Gets last element that should take focus after Shift+Tab.
_Check_return_ HRESULT Selector::GetLastFocusableElementOverride(
    _Outptr_result_maybenull_ DependencyObject** ppLastFocusable)
{
#ifdef SLTR_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"SLTR[0x%p]: GetLastFocusableElementOverride."));
#endif

    // Logic for GetFirst/LastFocusable element is invoked on Tab/Shift+Tab. The logic here is the same because
    // we want to go to the currently or previously focused item if nothing within the
    // items is currently focused, otherwise default to FocusManager logic.
    RRETURN(GetFirstFocusableElementOverride(ppLastFocusable));
}

// ISupportInitialize
_Check_return_ HRESULT
    Selector::BeginInitImpl()
{
    HRESULT hr = S_OK;

    m_pInitializingData = new InitializingData();

    IFC(get_SelectedValue(&m_pInitializingData->m_pInitialValue));
    IFC(get_SelectedItem(&m_pInitializingData->m_pInitialItem));
    IFC(get_SelectedIndex(&m_pInitializingData->m_nInitialIndex));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Selector::EndInitImpl(_In_opt_ DirectUI::XamlServiceProviderContext*)
{
    ctl::ComPtr<IInspectable> spFinalItem;
    ctl::ComPtr<IInspectable> spFinalValue;

    // You can not call EndInit before (or without first) calling BeginInit.
    IFCEXPECT_RETURN(m_pInitializingData);

    // Now that XAML parsing is over we will reset and play back any interesting property values.
    INT finalIndex = -1;
    UINT selectedCount = 0;

    IFC_RETURN(get_SelectedIndex(&finalIndex));
    IFC_RETURN(get_SelectedItem(&spFinalItem));
    IFC_RETURN(get_SelectedValue(&spFinalValue));

    IFC_RETURN(m_selection.GetNumItemsSelected(selectedCount));

    if (selectedCount == 0)
    {
        // No items had IsSelected set, so we can play back our values
        bool areEqual = false;
        IFC_RETURN(PropertyValue::AreEqual(m_pInitializingData->m_pInitialValue, spFinalValue.Get(), &areEqual));
        if (!areEqual)
        {
            IFC_RETURN(put_SelectedValue(m_pInitializingData->m_pInitialValue));

            delete m_pInitializingData;
            m_pInitializingData = nullptr;

            IFC_RETURN(put_SelectedValue(spFinalValue.Get()));
        }
        else if (m_pInitializingData->m_nInitialIndex != finalIndex)
        {
            IFC_RETURN(put_SelectedIndex(m_pInitializingData->m_nInitialIndex));

            delete m_pInitializingData;
            m_pInitializingData = nullptr;

            IFC_RETURN(put_SelectedIndex(finalIndex));
        }
        else if (m_pInitializingData->m_pInitialItem != spFinalItem.Get())
        {
            IFC_RETURN(put_SelectedItem(m_pInitializingData->m_pInitialItem));

            delete m_pInitializingData;
            m_pInitializingData = nullptr;

            IFC_RETURN(put_SelectedItem(spFinalItem.Get()));
        }
    }

    delete m_pInitializingData;
    m_pInitializingData = nullptr;

    return S_OK;
}

// SelectionChangeApplier implementation.

// Calls SetItemIsSelected(TRUE) on the given item.
_Check_return_
    HRESULT
    Selector::SelectIndex(
    _In_ UINT index)
{
    RRETURN(SetItemIsSelected(index, TRUE));
}

// Calls SetItemIsSelected(FALSE) on the given item.
_Check_return_
    HRESULT
    Selector::UnselectIndex(
    _In_ UINT index)
{
    RRETURN(SetItemIsSelected(index, FALSE));
}

_Check_return_ HRESULT SelectorFactory::GetIsSelectionActiveImpl(_In_ xaml::IDependencyObject* element, _Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;
    Selector* pSelector;

    IFCPTR(element);
    pSelector = static_cast<Selector*>(element);

    IFCEXPECT(pSelector);
    IFC(pSelector->get_IsSelectionActive(returnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Selector::HasItems(_Out_ BOOLEAN& bHasItems)
{
    ctl::ComPtr<IInspectable> spItemsSource;
    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;

    bHasItems = FALSE;
    IFC_RETURN(get_ItemsSource(&spItemsSource));
    IFC_RETURN(get_Items(&spItems));
    UINT itemCount = 0;
    IFC_RETURN(spItems.Cast<ItemCollection>()->get_Size(&itemCount));

    bHasItems = spItemsSource || itemCount > 0;

    return S_OK;
}

// Check whether the item at a given index is a data virtualized placeholder,
// and update its visuals if that's the case.
_Check_return_ HRESULT Selector::ShowPlaceholderIfVirtualized(
    _In_ UINT index)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IIterable<IInspectable*>> spItemsSource;

    // If the value of the new item is a placeholder, then we want to make sure
    // the SelectorItem will correctly represent it.
    IFC(get_ItemsSource(&spItemsSource));
    if (spItemsSource)
    {
        ctl::ComPtr<IDependencyObject> spContainer;

        // Check if the item being changed is a placeholder
        IFC(ContainerFromIndex(index, &spContainer));
        if (spContainer)
        {
            ctl::ComPtr<IInspectable> spValue;
            bool isPlaceholder = false;
            ctl::ComPtr<SelectorItem> spItem;

            IFC(ItemFromContainer(spContainer.Get(), &spValue));

            // NULL is now the placeholder
            isPlaceholder = spValue.Get() == nullptr;

            // Update the SelectorItem with whether or not it's a placeholder
            spItem = spContainer.AsOrNull<ISelectorItem>().Cast<SelectorItem>();
            if (spItem && spItem->m_isPlaceholder != isPlaceholder)
            {
                spItem->m_isPlaceholder = isPlaceholder;
                IFC(spItem->UpdateVisualState(TRUE));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Check whether the given SelectorItem is a data virtualized placeholder, and
// update its visuals if that's the case.
_Check_return_ HRESULT Selector::ShowPlaceholderIfVirtualized(
    _In_ SelectorItem* pItem)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IIterable<IInspectable*>> spItemsSource;

    IFCPTR(pItem);

    // If the value of the new item is a placeholder, then we want to make sure
    // the SelectorItem will correctly represent it.
    IFC(get_ItemsSource(&spItemsSource));
    if (spItemsSource)
    {
        ctl::ComPtr<IInspectable> spValue;
        bool isPlaceholder = false;

        // Check if the item being changed is a placeholder
        IFC(ItemFromContainer(pItem, &spValue));

        // note: do not go through get_Content here, that may not be set at all
        // because fast panning feature allows app code to decide whether it ever wants
        // to set the content

        // NULL is now the placeholder
        isPlaceholder = spValue.Get() == nullptr;

        // Update the SelectorItem with whether or not it's a placeholder
        if (pItem->m_isPlaceholder != isPlaceholder)
        {
            pItem->m_isPlaceholder = isPlaceholder;
            IFC(pItem->UpdateVisualState(TRUE));
        }
    }

Cleanup:
    RRETURN(hr);
}

// CV synchronization methods
_Check_return_ HRESULT Selector::DisconnectFromMonitoredCV()
{
    RRETURN(DisconnectFromMonitoredCVCore(m_tpMonitoredCV.Get()));
}

_Check_return_ HRESULT Selector::DisconnectFromMonitoredCVCore(_In_ IInspectable *pMonitoredCV)
{
    HRESULT hr = S_OK;

    if (m_epCVCurrentChanged)
    {
        if (pMonitoredCV)
        {
            IFC(m_epCVCurrentChanged.DetachEventHandler(pMonitoredCV));
        }
        m_tpMonitoredCV.Clear();
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Selector::MonitorCV(_In_opt_ xaml_data::ICollectionView *pCV)
{
    HRESULT hr = S_OK;

    ASSERT(!m_tpMonitoredCV);

    if (pCV == NULL)
    {
        goto Cleanup;
    }

    SetPtrValue(m_tpMonitoredCV, pCV);

    IFC(m_epCVCurrentChanged.AttachEventHandler(pCV,
        [this](IInspectable *pSender, IInspectable *pArgs)
    {
        return OnCurrentChanged();
    }));

Cleanup:

    RRETURN(hr);
}

_Check_return_ HRESULT Selector::OnCurrentChanged()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> pCurrentItem;
    ctl::ComPtr<IInspectable> pSelectedItem;
    bool areEqual = false;

    // Avoid re-entrancy
    if (m_fUpdatingCurrentItemInCollectionView || !m_fSynchronizeCurrentItem)
    {
        goto Cleanup;
    }

    IFC(m_tpMonitoredCV.Get()->get_CurrentItem(&pCurrentItem));
    IFC(get_SelectedItem(&pSelectedItem));

    IFC(PropertyValue::AreEqual(pCurrentItem.Get(), pSelectedItem.Get(), &areEqual));

    if (!areEqual)
    {
        IFC(put_SelectedItem(pCurrentItem.Get()));
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_ HRESULT Selector::UpdateCurrentItemInCollectionView(_In_opt_ IInspectable *pItem, _Out_ bool *pfDone)
{
    HRESULT hr = S_OK;
    BOOLEAN result = false;
    ctl::ComPtr<IInspectable> pCurrentItem;
    bool areEqual = false;

    *pfDone = TRUE;

    if (!m_fSynchronizeCurrentItem || !m_tpMonitoredCV)
    {
        goto Cleanup;
    }

    // Need to know if the new item is the same as it is already on the
    // collection view, if so we're done
    IFC(m_tpMonitoredCV.Get()->get_CurrentItem(&pCurrentItem));
    IFC(PropertyValue::AreEqual(pCurrentItem.Get(), pItem, &areEqual));
    if (areEqual)
    {
        goto Cleanup;
    }

    m_fUpdatingCurrentItemInCollectionView = true;

    IFC(m_tpMonitoredCV.Get()->MoveCurrentTo(pItem, &result));

    // Now that we have modified the collection view, ensure that it accepted the new value
    IFC(m_tpMonitoredCV.Get()->get_CurrentItem(&pCurrentItem));
    IFC(PropertyValue::AreEqual(pItem, pCurrentItem.Get(), &areEqual));
    *pfDone = areEqual;

Cleanup:

    m_fUpdatingCurrentItemInCollectionView = false;

    RRETURN(hr);
}

_Check_return_ HRESULT Selector::UpdateIsSynchronized()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wf::IReference<bool>> spIsSynchronizedReference;
    BOOLEAN fCanSelectMultiple = false;

    IFC(get_IsSynchronizedWithCurrentItem(&spIsSynchronizedReference));
    if (spIsSynchronizedReference)
    {
        // We only allow FALSE as the value of this property
        // so we can assume here that it is
        m_fSynchronizeCurrentItem = false;
    }
    else
    {
        IFC(get_CanSelectMultiple(&fCanSelectMultiple));
        m_fSynchronizeCurrentItem = m_tpMonitoredCV && !fCanSelectMultiple;
    }

Cleanup:
    RRETURN(hr);
}

// Report the old selected index as the given index.
_Check_return_
    HRESULT
    Selector::ReportOldSelectedIndexAs(
    _In_ INT index)
{
    HRESULT hr = S_OK;

    ClearOldIndexToReport();
    m_pOldSelectedIndexToReport = new INT(index);

    RRETURN(hr);//RRETURN_REMOVAL
}

// Stop reporting the old selected index.
void
    Selector::ClearOldIndexToReport()
{
    delete m_pOldSelectedIndexToReport;
    m_pOldSelectedIndexToReport = NULL;
}

BOOLEAN
    Selector::IsNavigationKey(
    _In_ wsy::VirtualKey key)
{
    return wsy::VirtualKey_Left == key ||
        wsy::VirtualKey_Up == key ||
        wsy::VirtualKey_Right == key ||
        wsy::VirtualKey_Down == key ||
        wsy::VirtualKey_Home == key ||
        wsy::VirtualKey_End == key ||
        wsy::VirtualKey_PageUp == key ||
        wsy::VirtualKey_PageDown == key;
}

_Check_return_
    HRESULT
    Selector::DataSourceGetIsSelected(
    _In_ SelectorItem* pSelectorItem,
    _Out_ BOOLEAN* pIsSelected,
    _Out_ bool* pIsValueSet)
{
    HRESULT hr = S_OK;

    *pIsValueSet = false;

    if (m_tpDataSourceAsSelectionInfo)
    {
        int itemIndex = -1;

        IFC(IndexFromContainer(pSelectorItem, &itemIndex));

        IFC(m_tpDataSourceAsSelectionInfo->IsSelected(itemIndex, pIsSelected));

        *pIsValueSet = true;
    }

Cleanup:
    return hr;
}

_Check_return_
    HRESULT
    Selector::SelectJustThisItemInternal(
    _In_ const INT oldIndex,
    _In_ const INT newIndex,
    _In_opt_ IInspectable* pSelectedItem,
    _In_ BOOLEAN animateIfBringIntoView,
    _Out_opt_ BOOLEAN* pShouldUndoChange,
    _In_ xaml_input::FocusNavigationDirection focusNavigationDirection)
{
    HRESULT hr = S_OK;

    SelectionChanger* pSelectionChanger = nullptr;

    if (pShouldUndoChange)
    {
        *pShouldUndoChange = FALSE;
    }

    if (m_tpDataSourceAsSelectionInfo)
    {
        IFC(InvokeDataSourceClearSelection());
        IFC(InvokeDataSourceRangeSelection(true /* select */, newIndex, 1));
    }
    else
    {
        IFC(BeginChange(&pSelectionChanger));

        IFC(SelectJustThisItem(oldIndex, newIndex, pSelectionChanger, pSelectedItem, pShouldUndoChange));

        IFC(EndChange(pSelectionChanger, animateIfBringIntoView, focusNavigationDirection));
        pSelectionChanger = nullptr;
    }

Cleanup:
    if (pSelectionChanger != nullptr)
    {
        VERIFYRETURNHR(pSelectionChanger->Cancel());
    }
    return hr;
}

_Check_return_
    HRESULT
    Selector::GetDataSourceAsSelectionInfo(
    _Outptr_ xaml_data::ISelectionInfo** ppDataSourceAsSelectionInfo)
{
    ARG_VALIDRETURNPOINTER(ppDataSourceAsSelectionInfo);
    *ppDataSourceAsSelectionInfo = nullptr;

    IFC_RETURN(m_tpDataSourceAsSelectionInfo.CopyTo(ppDataSourceAsSelectionInfo));

    return S_OK;
}

void
    Selector::SetDataSourceAsSelectionInfo(
    _In_ xaml_data::ISelectionInfo* const pDataSourceAsSelectionInfo)
{
    SetPtrValue(m_tpDataSourceAsSelectionInfo, pDataSourceAsSelectionInfo);
}

// Used to inform the data source of a range selection/deselection
// bool select indicates whether it's a select or deselect
_Check_return_
    HRESULT
    Selector::InvokeDataSourceRangeSelection(
    _In_ bool select,
    _In_ int firstIndex,
    _In_ unsigned int length)
{
    // Before the end of the function, we call UpdateVisibleAndCachedItemsSelectionAndVisualState
    // This function updates the IsSelected property of the SelectorItem
    // That triggers OnPropertyChanged2
    // OnPropertyChanged2 triggers OnIsSelectedChanged
    // OnIsSelectedChanged triggers NotifyListIsItemSelected
    // NotifyListIsItemSelected triggers InvokeDataSourceRangeSelection again
    if (!IsSelectionReentrancyAllowed())
    {
        return S_OK;
    }

    PreventSelectionReentrancy();

    auto allowSelectionReentrancy = wil::scope_exit([&]()
    {
        AllowSelectionReentrancy();
    });

    // when SelectedIndex < 0, it means we have no selection (cleared selection)
    // SelectJustThisItemInternal handles the call to invoke clear selection for ISelectionInfo interfaces
    if (firstIndex >= 0)
    {
        ctl::ComPtr<ItemIndexRange> spItemIndexRange;

        IFC_RETURN(ctl::make(&spItemIndexRange));
        IFC_RETURN(spItemIndexRange->put_FirstIndex(firstIndex));
        IFC_RETURN(spItemIndexRange->put_Length(length));

        if (select)
        {
            IFC_RETURN(m_tpDataSourceAsSelectionInfo->SelectRange(spItemIndexRange.Get()));
        }
        else
        {
            IFC_RETURN(m_tpDataSourceAsSelectionInfo->DeselectRange(spItemIndexRange.Get()));
        }
    }

    // call the Selector::InvokeSelectionChanged with AddedItems and RemovedItems being null
    // when the SelectionInterface is implemented, we let the developer handle SelectedItems and SelectedRanges
    // in here, we simply invoke a selection changed event
    IFC_RETURN(InvokeSelectionChanged(nullptr /* pUnselectedItems */, nullptr /* pSelectedItems */));

    // updates SelectedIndex
    IFC_RETURN(UpdatePublicSelectionPropertiesAfterDataSourceSelectionInfo());

    // updates the selection and visual state of visible and cached items
    IFC_RETURN(UpdateVisibleAndCachedItemsSelectionAndVisualState(true /* updateIsSelected */));

    return S_OK;
}

_Check_return_
    HRESULT
    Selector::InvokeDataSourceClearSelection()
{
    unsigned int selectedRangesCount = 0;
    ctl::ComPtr<xaml_data::IItemIndexRange> spItemIndexRange;
    ctl::ComPtr<wfc::IVectorView<xaml_data::ItemIndexRange*>> spSelectedRanges;

    IFC_RETURN(m_tpDataSourceAsSelectionInfo->GetSelectedRanges(&spSelectedRanges));

    IFC_RETURN(spSelectedRanges->get_Size(&selectedRangesCount));

    for (unsigned int i = 0; i < selectedRangesCount; ++i)
    {
        IFC_RETURN(spSelectedRanges->GetAt(i, &spItemIndexRange));

        IFC_RETURN(m_tpDataSourceAsSelectionInfo->DeselectRange(spItemIndexRange.Get()));
    }

    return S_OK;
}

// updates SelectedIndex, SelectedItem and SelectedValue after a selection using SelectionInfo occurs
_Check_return_
    HRESULT
    Selector::UpdatePublicSelectionPropertiesAfterDataSourceSelectionInfo()
{
    HRESULT hr = S_OK;

    bool selectedPropertiesUpdated = false;
    unsigned int selectedRangesCount = 0;
    ctl::ComPtr<wfc::IVectorView<xaml_data::ItemIndexRange*>> spSelectedRanges;

    IFC(m_tpDataSourceAsSelectionInfo->GetSelectedRanges(&spSelectedRanges));

    IFC(spSelectedRanges->get_Size(&selectedRangesCount));

    if (selectedRangesCount > 0)
    {
        unsigned int itemsCount = 0;
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;

        IFC(get_Items(&spItems));
        IFC(spItems.Cast<ItemCollection>()->get_Size(&itemsCount));

        if (itemsCount > 0)
        {
            int currentSelectedIndex = -1;
            int newSelectedIndex = -1;
            ctl::ComPtr<xaml_data::IItemIndexRange> spFirstRange;

            IFC(spSelectedRanges->GetAt(0, &spFirstRange));

            // update SelectedIndex
            IFC(get_SelectedIndex(&currentSelectedIndex));
            IFC(spFirstRange->get_FirstIndex(&newSelectedIndex));
            if (currentSelectedIndex != newSelectedIndex)
            {
                IFC(put_SelectedIndex(newSelectedIndex));
            }

            if (newSelectedIndex >= 0 && static_cast<unsigned int>(newSelectedIndex) < itemsCount)
            {
                bool fDone = false;
                bool areEqual = false;
                ctl::ComPtr<IInspectable> spCurrentSelectedItem;
                ctl::ComPtr<IInspectable> spNewSelectedItem;

                // update SelectedItem
                IFC(get_SelectedItem(&spCurrentSelectedItem));
                IFC(spItems.Cast<ItemCollection>()->GetAt(newSelectedIndex, &spNewSelectedItem));
                IFC(PropertyValue::AreEqual(spCurrentSelectedItem.Get(), spNewSelectedItem.Get(), &areEqual));
                if (!areEqual)
                {
                    ctl::ComPtr<IInspectable> spCurrentSelectedValue;
                    ctl::ComPtr<IInspectable> spNewSelectedValue;

                    IFC(put_SelectedItem(spNewSelectedItem.Get()));

                    // update SelectedValue
                    IFC(get_SelectedValue(&spCurrentSelectedValue));
                    IFC(GetSelectedValue(spNewSelectedItem.Get(), &spNewSelectedValue));
                    IFC(PropertyValue::AreEqual(spCurrentSelectedValue.Get(), spNewSelectedValue.Get(), &areEqual));
                    if (!areEqual)
                    {
                        IFC(put_SelectedValue(spNewSelectedValue.Get()));
                    }
                }

                // If the ItemCollection contains an ICollectionView sync the selected index
                IFC(UpdateCurrentItemInCollectionView(spNewSelectedItem.Get(), &fDone));

                IFC(get_SelectedIndex(&newSelectedIndex));

                IFC(get_SelectedItem(&spNewSelectedItem));
                IFC(OnSelectionChanged(currentSelectedIndex, newSelectedIndex, spCurrentSelectedItem.Get(), spNewSelectedItem.Get()));

                selectedPropertiesUpdated = true;
            }
        }
    }

    if (!selectedPropertiesUpdated)
    {
        IFC(put_SelectedIndex(-1));
        IFC(put_SelectedItem(nullptr));
        IFC(put_SelectedValue(nullptr));
    }

Cleanup:
    return hr;
}

// goes through the visible and cached items and updates their IsSelected property and their visual state
_Check_return_
    HRESULT
    Selector::UpdateVisibleAndCachedItemsSelectionAndVisualState(
    _In_ bool updateIsSelected)
{
    ctl::ComPtr<IPanel> spItemsPanelRoot;

    IFC_RETURN(get_ItemsPanelRoot(&spItemsPanelRoot));
    if (spItemsPanelRoot)
    {
        ctl::ComPtr<IModernCollectionBasePanel> spIModernCollectionBasePanel;

        spIModernCollectionBasePanel = spItemsPanelRoot.AsOrNull<IModernCollectionBasePanel>();
        if (spIModernCollectionBasePanel)
        {
            int firstCachedIndex = -1;
            int lastCachedIndex = -1;
            std::vector<unsigned int> pinnedElementsIndices;

            // get the data from the panel
            IFC_RETURN(spIModernCollectionBasePanel.Cast<ModernCollectionBasePanel>()->get_FirstCacheIndexBase(&firstCachedIndex));
            IFC_RETURN(spIModernCollectionBasePanel.Cast<ModernCollectionBasePanel>()->get_LastCacheIndexBase(&lastCachedIndex));
            IFC_RETURN(spIModernCollectionBasePanel.Cast<ModernCollectionBasePanel>()->GetPinnedElementsIndexVector(xaml_controls::ElementType_ItemContainer, &pinnedElementsIndices));

            const auto updateVisualState = [this, updateIsSelected](int itemIndex)
            {
                ctl::ComPtr<IDependencyObject> spContainer;

                IFC_RETURN(ContainerFromIndex(itemIndex, &spContainer));
                if (spContainer)
                {
                    ctl::ComPtr<ISelectorItem> spISelectorItem;

                    spISelectorItem = spContainer.AsOrNull<ISelectorItem>();
                    if (spISelectorItem)
                    {
                        SelectorItem* pSelectorItem = spISelectorItem.Cast<SelectorItem>();

                        // querying for the selection state from the ISelectionInfo interface
                        if (updateIsSelected && m_tpDataSourceAsSelectionInfo)
                        {
                            BOOLEAN isSelected = FALSE;

                            IFC_RETURN(m_tpDataSourceAsSelectionInfo->IsSelected(itemIndex, &isSelected));

                            // this will call ChangeVisualState internally so there is no point calling the below ChangeVisualState
                            IFC_RETURN(pSelectorItem->put_IsSelected(isSelected));
                        }
                        else
                        {
                            IFC_RETURN(pSelectorItem->ChangeVisualState(TRUE /* bUseTransitions */));
                        }
                    }
                }

                return S_OK;
            };

            for (int i = firstCachedIndex; i <= lastCachedIndex; ++i)
            {
                IFC_RETURN(updateVisualState(i));
            }

            for (int pinnedIndex : pinnedElementsIndices)
            {
                // Ignore containers for which we already updated the visual state.
                if (pinnedIndex < firstCachedIndex || pinnedIndex > lastCachedIndex)
                {
                    IFC_RETURN(updateVisualState(pinnedIndex));
                }
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT Selector::IsSelectionPatternApplicable(
    _Out_ bool* selectionPatternApplicable)
{
    *selectionPatternApplicable = true;
    return S_OK;
}

void Selector::SetAllowCustomValues(bool allow)
{
    m_customValuesAllowed = allow;
}
