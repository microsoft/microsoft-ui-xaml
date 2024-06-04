// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ListViewBase.g.h"
#include "XamlBehaviorMode.h"
#include "ListViewBaseItem.g.h"
#include "ItemCollection.g.h"
#include "ItemIndexRange.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Gets a collection containing the currently selected items in the ListView.
_Check_return_ HRESULT ListViewBase::get_SelectedItemsImpl(
    _Outptr_ wfc::IVector<IInspectable*>** pValue)
{
    HRESULT hr = S_OK;

    ARG_VALIDRETURNPOINTER(pValue);
    *pValue = NULL;

    // Use the Selector::get_SelectedItemsInternal implementation
    IFC(get_SelectedItemsInternal(pValue));

    // TODO: We'll probably need to optimize this behavior for users that call
    // SelectAll on a large, virtualized collection.  The current plan is that
    // we'll provide an implementation of IVector that forwards along to an
    // ItemCollection, but copies over all the indices as soon as the original
    // collection grows or replaces an item.

Cleanup:
    return hr;
}

// Gets a collection containing the currently selected ranges in the ListView.
_Check_return_ HRESULT ListViewBase::get_SelectedRangesImpl(
    _Outptr_ wfc::IVectorView<xaml_data::ItemIndexRange*>** ppValue)
{
    ARG_VALIDRETURNPOINTER(ppValue);
    *ppValue = NULL;

    // Use the Selector::get_SelectedRangesInternal implementation
    IFC_RETURN(get_SelectedRangesInternal(ppValue));

    return S_OK;
}

// Handles changes to the SelectionMode property by updating or clearing
// selection.
_Check_return_ HRESULT ListViewBase::OnSelectionModeChanged(
    _In_ xaml_controls::ListViewSelectionMode oldMode,
    _In_ xaml_controls::ListViewSelectionMode newMode)
{
    HRESULT hr = S_OK;
    INT selectedIndex = -1;

    m_isInOnSelectionModeChanged = TRUE;

    switch (newMode)
    {
        case xaml_controls::ListViewSelectionMode_None:
        {
            // Clear any selection when we're switching to None
            IFC(get_SelectedIndex(&selectedIndex));
            if (selectedIndex >= 0)
            {
                IFC(ClearSelection());
            }
            break;
        }
        case xaml_controls::ListViewSelectionMode_Single:
        {
            // Restrict selection to a single item when we're changing to Single
            IFC(get_SelectedIndex(&selectedIndex));
            if (selectedIndex >= 0)
            {
                IFC(MakeSingleSelection(selectedIndex, FALSE /*animateIfBringIntoView*/, NULL));
            }
            break;
        }
    }

    // Update the CollectionView synchronization state now that the
    // selection mode has changed
    IFC(UpdateCVSynchronizationState());

Cleanup:
    m_isInOnSelectionModeChanged = FALSE;
    return hr;
}

// Gets a value indicating whether the ListView can select multiple items at
// once.
_Check_return_ HRESULT ListViewBase::get_CanSelectMultiple(
    _Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    xaml_controls::ListViewSelectionMode mode = xaml_controls::ListViewSelectionMode_Single;

    IFCPTR(pValue);

    // We can select multiple items as long as we're not in either Single or
    // None selection modes.
    IFC(get_SelectionMode(&mode));
    *pValue =
        mode != xaml_controls::ListViewSelectionMode_Single &&
        mode != xaml_controls::ListViewSelectionMode_None;

    // TODO: We should look to see if there are any customer scenarios for this
    // method and possibly add another version that would factor in a primary
    // input device (i.e., we can't always multi-select with touch input if
    // we've disabled Swipe and have an ItemClick event.

Cleanup:
    RRETURN(hr);
}

// Called after the Selector has changed the selected item.  We determine
// whether or not we want to call the base implementation which will focus the
// newSelectedIndex for us.
_Check_return_ HRESULT ListViewBase::OnSelectionChanged(
    _In_ INT oldSelectedIndex,
    _In_ INT newSelectedIndex,
    _In_ IInspectable* pOldSelectedItem,
    _In_ IInspectable* pNewSelectedItem,
    _In_ BOOLEAN animateIfBringIntoView,
    _In_ xaml_input::FocusNavigationDirection focusNavigationDirection)
{
    HRESULT hr = S_OK;

    // Don't focus any new item during a reorder.
    if (!m_isInItemReorderAfterDrop)
    {
        xaml_controls::ListViewSelectionMode mode = xaml_controls::ListViewSelectionMode_Single;

        // Prevent selection from changing when SelectionMode is None
        IFC(get_SelectionMode(&mode));
        if (mode != xaml_controls::ListViewSelectionMode_None)
        {
            bool movedExistingSelection = (newSelectedIndex != -1) && (oldSelectedIndex == -1);
            bool selectionChanged = oldSelectedIndex != newSelectedIndex;

            // Only call base to focus the item if we actually moved
            // the selection or if there was a change when SelectionMode is Single
            if (movedExistingSelection ||
                (selectionChanged && (mode == xaml_controls::ListViewSelectionMode_Single)))
            {
                // Focus the item.
                IFC(ListViewBaseGenerated::OnSelectionChanged(
                    oldSelectedIndex,
                    newSelectedIndex,
                    pOldSelectedItem,
                    pNewSelectedItem,
                    animateIfBringIntoView,
                    focusNavigationDirection));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Handles changes to the Items collection.  We should adjust m_pAnchorIndex if
// any item before that index was added or removed. Additionally, we should
// cancel a drag/drop if the item we're dragging is removed, and we should cancel
// any swipe in progress.
_Check_return_ HRESULT ListViewBase::OnItemsChanged(
    _In_ wfc::IVectorChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    wfc::CollectionChange action = wfc::CollectionChange_Reset;
    UINT index = 0;
    BOOLEAN shouldConsiderDragItemIndexForCancelDrag = FALSE;
    BOOLEAN allowCancelDrag = !m_isInItemReorderAfterDrop;  // Default to always cancel drag, unless we're currently in the middle
                                                            // of reordering items after a drop.

    IFCPTR(pArgs);
    IFC(ListViewBaseGenerated::OnItemsChanged(pArgs));

    IFC(pArgs->get_CollectionChange(&action));
    switch (action)
    {
        case wfc::CollectionChange_Reset:
        {
            // Clear the anchor index if the entire collection was cleared
            ReleaseAnchorIndex();
            break;
        }
        case wfc::CollectionChange_ItemInserted:
        {
            // If an item was inserted before the anchored item, move the anchor
            // index forward an element
            if (m_pAnchorIndex != NULL)
            {
                IFC(pArgs->get_Index(&index));

                if (index <= *m_pAnchorIndex)
                {
                    (*m_pAnchorIndex)++;
                }
            }

            // Insertion is not a problem for an ongoing drag - we'll never
            // lose our currently dragged item.
            allowCancelDrag = FALSE;
            break;
        }
        case wfc::CollectionChange_ItemRemoved:
        {
            IFC(pArgs->get_Index(&index));

            if (m_pAnchorIndex != NULL)
            {
                if (index == *m_pAnchorIndex)
                {
                    // If the anchored item was removed, clear the index
                    ReleaseAnchorIndex();
                }
                else if (index < *m_pAnchorIndex)
                {
                    // If the removed item was before the anchored item, move the
                    // anchor index back an element
                    (*m_pAnchorIndex)--;
                }
            }

            shouldConsiderDragItemIndexForCancelDrag = TRUE;
            break;
        }
        case wfc::CollectionChange_ItemChanged:
        {
            IFC(pArgs->get_Index(&index));

            if (m_pAnchorIndex != NULL)
            {
                // If the anchored item was swapped out, clear the anchor index
                if (index == *m_pAnchorIndex)
                {
                    ReleaseAnchorIndex();
                }
            }

            shouldConsiderDragItemIndexForCancelDrag = TRUE;
            break;
        }
    }

    if (allowCancelDrag)
    {
        // Possibly continue the drag if the removed/changed index is not the primary dragged item.
        if (shouldConsiderDragItemIndexForCancelDrag && m_tpPrimaryDraggedContainer)
        {
            INT draggedItemIndex = 0;
            IFC(IndexFromContainer(m_tpPrimaryDraggedContainer.Cast<ListViewBaseItem>(), &draggedItemIndex));

            // Cancel the drag if the dragged item was removed/changed, or if the dragged item is no longer
            // recognized by the ICG
            allowCancelDrag = (draggedItemIndex == -1) || (draggedItemIndex == index);
        }

        if (allowCancelDrag)
        {
            IFC(CancelDrag());
            ASSERT(!IsInDragDrop());
        }
    }

    // Recalculate the reorder hints, etc if there is (still) a drag drop in progress.
    if (IsInDragDrop() && !m_isInItemReorderAfterDrop)
    {
        IFC(ResetAllItemsForLiveReorder());
        IFC(ProcessDragOverAt(m_lastDragOverPoint, NULL /*pHandled*/));
    }

Cleanup:
    // Note: We're just returning an HRESULT as an HRESULT here.  It would be
    // best to change the return type on the original method.
    RRETURN(hr);
}

// Handles selection of single item, and only that item
_Check_return_ HRESULT ListViewBase::MakeSingleSelection(
    _In_ UINT index,
    _In_ BOOLEAN animateIfBringIntoView,
    _In_opt_ IInspectable* pSelectedItem,
    _In_ xaml_input::FocusNavigationDirection focusNavigationDirection)
{
    HRESULT hr = S_OK;

    xaml_controls::ListViewSelectionMode mode = xaml_controls::ListViewSelectionMode_Single;

    IFC(get_SelectionMode(&mode));
    if (mode == xaml_controls::ListViewSelectionMode_Extended ||
        mode == xaml_controls::ListViewSelectionMode_Multiple)
    {
        IFC(SetAnchorIndex(index));
    }

    IFC(Selector::MakeSingleSelection(index, animateIfBringIntoView, pSelectedItem, focusNavigationDirection));

Cleanup:
    return hr;
}

// Handles selection of single item, and only that item
_Check_return_ HRESULT ListViewBase::MakeSingleSelection(
    _In_ ListViewBaseItem* pItem,
    _In_ xaml_input::FocusNavigationDirection focusNavigationDirection)
{
    HRESULT hr = S_OK;
    INT index = -1;
    ctl::ComPtr<IInspectable> spSelectedItem;

    IFCPTR(pItem);

    IFC(IndexFromContainer(pItem, &index));
    // in the ItemClick event (which precedes this code) we have found multiple partners
    // to actually clear their collection. After talking to them, the feature team is convinced
    // that that code should be able to be written and thus we have decided to ignore the selection in this case.

    // we can only select an item if we can find it in the collection
    if (index >= 0)
    {
        IFC(ItemFromContainer(pItem, &spSelectedItem));
        IFC(MakeSingleSelection(index, FALSE /*animateIfBringIntoView*/, spSelectedItem.Get(), focusNavigationDirection));
    }
Cleanup:
    RRETURN(hr);
}

// Toggle IsSelected of item
_Check_return_ HRESULT ListViewBase::MakeToggleSelection(
    _In_ ListViewBaseItem* pItem)
{
    HRESULT hr = S_OK;
    xaml_controls::ListViewSelectionMode mode = xaml_controls::ListViewSelectionMode_Single;
    BOOLEAN isSelected = FALSE;

    IFCPTR(pItem);

    IFC(get_SelectionMode(&mode));
    if (mode == xaml_controls::ListViewSelectionMode_Extended ||
        mode == xaml_controls::ListViewSelectionMode_Multiple)
    {
        INT index = -1;
        IFC(IndexFromContainer(pItem, &index));


        // in the ItemClick event (which precedes this code) we have found multiple partners
        // to actually clear their collection. After talking to them, the feature team is convinced
        // that that code should be able to be written and thus we have decided to ignore the selection in this case.

        // we can only select an item if we can find it in the collection
        if (index >= 0)
        {
            IFC(SetAnchorIndex(index));
        }
    }

    IFC(pItem->get_IsSelected(&isSelected));
    IFC(pItem->put_IsSelected(!isSelected));

Cleanup:
    RRETURN(hr);
}

// Select a range of items beginning with current anchor
_Check_return_ HRESULT ListViewBase::MakeRangeSelection(
    _In_ UINT index,
    _In_ BOOLEAN clearOldSelection)
{
    unsigned int firstIndex = 0;
    unsigned int lastIndex = index;
    unsigned int length = 0;
    xaml_controls::ListViewSelectionMode mode = xaml_controls::ListViewSelectionMode_Single;

    IFC_RETURN(get_SelectionMode(&mode));

    ASSERT(mode != xaml_controls::ListViewSelectionMode_Single &&
           mode != xaml_controls::ListViewSelectionMode_None,
           L"Invalid selection mode in range selection");

    // if the anchor index exists then use its value
    // otherwise, we check if a SelectedIndex is set
    // SelectedIndex can be either set using the property itself directly
    // or by using SelectRange... in both cases the value is set programmatically
    // and not by user interaction
    if (m_pAnchorIndex)
    {
        firstIndex = *m_pAnchorIndex;
    }
    else
    {
        int selectedIndex = -1;

        IFC_RETURN(get_SelectedIndex(&selectedIndex));

        if (selectedIndex == -1)
        {
            firstIndex = 0;
        }
        else
        {
            firstIndex = selectedIndex;
            IFC_RETURN(SetAnchorIndex(firstIndex));
        }
    }

    if (firstIndex > index)
    {
        lastIndex = firstIndex;
        firstIndex = index;
    }

    // compute the length of the range for selection/deselection
    length = lastIndex - firstIndex + 1;

    // In threshold, Shift+Click with Multiple selection mode invokes a selection that depends on the anchor item
    if (mode == xaml_controls::ListViewSelectionMode_Multiple)
    {
        // MakeRangeSelection will only be called in the case where the Selection mode is not None or Single
        // and when there is at least one item, hence it is safe to set the anchor index to 0 in our fallback case
        bool isAnchorSelected;
        IFC_RETURN(IsIndexSelected(m_pAnchorIndex ? *m_pAnchorIndex : 0, &isAnchorSelected));

        if (isAnchorSelected)
        {
            // call the Selector::SelectRangeInternal
            IFC_RETURN(SelectRangeInternal(firstIndex /*firstIndex*/, length /*length*/, clearOldSelection));
        }
        else
        {
            // call the Selector::DeselectRangeInternal
            IFC_RETURN(DeselectRangeInternal(firstIndex /*firstIndex*/, length /*length*/));
        }
    }
    else
    {
        // call the Selector::SelectRangeInternal
        IFC_RETURN(SelectRangeInternal(firstIndex /*firstIndex*/, length /*length*/, clearOldSelection));
    }

    return S_OK;
}

// Select a range of items beginning with current anchor
_Check_return_ HRESULT ListViewBase::MakeRangeSelection(
    _In_ ListViewBaseItem* pItem,
    _In_ BOOLEAN clearOldSelection)
{
    HRESULT hr = S_OK;
    INT index = -1;

    IFCPTR(pItem);

    IFC(IndexFromContainer(pItem, &index));

    IFCEXPECT(index >= 0);
    IFC(MakeRangeSelection(index, clearOldSelection));

Cleanup:
    RRETURN(hr);
}

// Selects all the items.
_Check_return_ HRESULT ListViewBase::SelectAllImpl()
{
    // call the Selector::SelectAllInternal
    return SelectAllInternal();
}

// Selects a range of items in the ListView.
_Check_return_ HRESULT ListViewBase::SelectRangeImpl(
    _In_ xaml_data::IItemIndexRange* pItemIndexRange)
{
    int rangeFirstIndex = 0;
    unsigned int rangeLength = 0;

    IFC_RETURN(pItemIndexRange->get_FirstIndex(&rangeFirstIndex));
    IFC_RETURN(pItemIndexRange->get_Length(&rangeLength));

    // call the Selector::SelectRangeInternal
    IFC_RETURN(SelectRangeInternal(rangeFirstIndex /* startIndex */, rangeLength /* length */, FALSE /* clearOldSelection */));

    return S_OK;
}

// Deselects a range of items in the ListView.
_Check_return_ HRESULT ListViewBase::DeselectRangeImpl(
    _In_ xaml_data::IItemIndexRange* pItemIndexRange)
{
    int rangeFirstIndex = 0;
    unsigned int rangeLength = 0;

    IFC_RETURN(pItemIndexRange->get_FirstIndex(&rangeFirstIndex));
    IFC_RETURN(pItemIndexRange->get_Length(&rangeLength));

    // call the Selector::DeselectRangeInternal
    IFC_RETURN(DeselectRangeInternal(rangeFirstIndex /* firstIndex */, rangeLength /* length */));

    return S_OK;
}

// Deselect all the items.
_Check_return_ HRESULT ListViewBase::DeselectAllImpl()
{
    // call the Selector::ClearSelection
    return ClearSelection();
}

_Check_return_ HRESULT ListViewBase::InitializeDataSourceSelectionInfo()
{
    ctl::ComPtr<IInspectable> spItemsSourceAsIInspectable;

    IFC_RETURN(get_ItemsSource(&spItemsSourceAsIInspectable));
    if (spItemsSourceAsIInspectable)
    {
        auto spItemsSourceAsSI = spItemsSourceAsIInspectable.AsOrNull<xaml_data::ISelectionInfo>();
        if (spItemsSourceAsSI)
        {
            // call the Selector::SetDataSourceAsSelectionInfo
            SetDataSourceAsSelectionInfo(spItemsSourceAsSI.Get());
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ListViewBase::IsSelectionPatternApplicable(
    _Out_ bool* selectionPatternApplicable)
{
    auto selectionMode = xaml_controls::ListViewSelectionMode::ListViewSelectionMode_None;
    IFC_RETURN(get_SelectionMode(&selectionMode));
    *selectionPatternApplicable = (selectionMode != xaml_controls::ListViewSelectionMode::ListViewSelectionMode_None);

    return S_OK;
}
