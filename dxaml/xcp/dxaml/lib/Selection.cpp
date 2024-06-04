// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      A transaction-based selection tracker.

#include "precomp.h"
#include "Selection.h"

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace xaml_data;

Selection::Selection():
    m_selectionChanger()
{
}

Selection::~Selection()
{
}

_Check_return_ HRESULT Selection::Initialize(_In_ SelectionChangeApplier* pChangeApplier)
{
    HRESULT hr = S_OK;

    IFC(ctl::make<InternalSelectedItemsStorage>(&m_spSelectedItems));
    IFC(m_selectionChanger.Initialize(this, pChangeApplier));

Cleanup:
    RRETURN(hr);
}

// Returns a copy of the list of selected indexes.
std::vector<UINT> Selection::GetSelectedIndexes() const
{
    ASSERT(m_spSelectedItems != nullptr);
    return std::move(m_spSelectedItems->GetSelectedIndexes());
}

// Obtains the index of the selected item at the given position in the selection.
_Check_return_ HRESULT Selection::GetIndexAt(
    _In_ UINT position,
    _Out_ UINT& itemIndex)
{
    HRESULT hr = S_OK;

    IFCEXPECT(m_spSelectedItems);

    IFC(m_spSelectedItems->GetIndexAt(position, itemIndex));

Cleanup:
    RRETURN(hr);
}

// Obtains the the selected item at the given position in the selection.
_Check_return_ HRESULT Selection::GetAt(
    _In_ UINT position,
    _Outptr_ IInspectable** ppSelectedItem)
{
    HRESULT hr = S_OK;

    IFCPTR(ppSelectedItem);

    IFC(m_spSelectedItems->GetAt(position, ppSelectedItem));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Selection::AddSelectedIndex(
    _In_ UINT itemIndex)
{
    HRESULT hr = S_OK;

    IFC(m_spSelectedItems->Inserted(itemIndex));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Selection::RemoveSelectedIndex(
    _In_ UINT itemIndex)
{
    HRESULT hr = S_OK;

    IFC(m_spSelectedItems->Removed(itemIndex));
    IFC(m_selectionChanger.AccountForRemovedItem(itemIndex));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Selection::Has(
    _In_ UINT itemIndex,
    _Out_ UINT& position,
    _Out_ BOOLEAN& hasItem)
{
    HRESULT hr = S_OK;

    IFC(m_spSelectedItems->Has(itemIndex, position, hasItem));

Cleanup:
    RRETURN(hr);
}

// Whether or not we're in the middle of a selection change.
BOOLEAN Selection::IsChangeActive()
{
    return m_selectionChanger.IsActive();
}

// Obtains the number of selected items.
_Check_return_ HRESULT Selection::GetNumItemsSelected(
    _Out_ UINT& size)
{
    HRESULT hr = S_OK;
    UINT returnSize = 0;

    size = 0;

    IFCEXPECT(m_spSelectedItems);

    IFC(m_spSelectedItems->get_Size(&returnSize));
    size = returnSize;

Cleanup:
    RRETURN(hr);
}

// Start selection change.
_Check_return_ HRESULT Selection::BeginChange(
    _Outptr_ SelectionChanger** changer)
{
    HRESULT hr = S_OK;

    IFCPTR(changer);

    m_selectionChanger.BeginInternal();

    *changer = static_cast<SelectionChanger*>(&m_selectionChanger);

Cleanup:
    RRETURN(hr);
}

#pragma region SelectionChangerImpl implementation

Selection::SelectionChangerImpl::SelectionChangerImpl() :
    m_pOwnerNoRef(NULL),
    m_pChangeApplier(NULL),
    m_bIsActive(FALSE),
    m_unselectAllRequested(FALSE)
{
    m_indexDeletedDuringThisChange.index = 0;
    m_indexDeletedDuringThisChange.empty = true;
}

Selection::SelectionChangerImpl::~SelectionChangerImpl()
{
    IGNOREHR(Cleanup());
}

// Prepares object's state
_Check_return_
HRESULT
Selection::SelectionChangerImpl::Initialize(
    _In_ Selection* pOwner,
    _In_ SelectionChangeApplier* pChangeApplier)
{
    HRESULT hr = S_OK;

    IFCPTR(pOwner);
    IFCPTR(pChangeApplier);

    m_pOwnerNoRef = pOwner;
    m_pChangeApplier = pChangeApplier;
    IFC(ctl::make<InternalSelectedItemsStorage>(&m_spItemsToSelect));
    IFC(ctl::make<InternalSelectedItemsStorage>(&m_spItemsToUnselect));
    IFC(Cleanup());

Cleanup:
    RRETURN(hr);
}

// Start selection change
void
Selection::SelectionChangerImpl::BeginInternal()
{
    ASSERT(!m_bIsActive, L"SelectionChanger already active");
    m_bIsActive = TRUE;
    m_unselectAllRequested = FALSE;
}

// End selection change
_Check_return_
HRESULT
Selection::SelectionChangerImpl::End(
    _In_ wfc::IVector<IInspectable*>* pUnselectedItems,
    _In_ wfc::IVector<IInspectable*>* pSelectedItems,
    _In_ BOOLEAN canSelectMultiple)
{
    HRESULT hr = S_OK;

    ASSERT(m_bIsActive, L"Selection change must be active before calling SelectionChanger.End()");

    // Make sure that we're not selecting too many items
    // Skip this if we're optimizing for a UnselectAll-only transaction.
    if (!m_unselectAllRequested)
    {
        IFC(ApplyCanSelectMultiple(canSelectMultiple));
    }

    // Dispatch any pending UnselectAll requests.
    IFC(UnrollUnselectAllRequest());

    // Create lists of what was actually selected and unselected
    IFC(CreateDeltaSelectionChange(pUnselectedItems, pSelectedItems));

Cleanup:
    VERIFYRETURNHR(Cleanup());
    RRETURN(hr);
}

_Check_return_
HRESULT
Selection::SelectionChangerImpl::Select(
    _In_ int itemIndex,
    _In_ IInspectable* pItem,
    _In_ BOOLEAN canSelectMultiple)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spSelectedItem;
    BOOLEAN bHasSelected = FALSE;
    BOOLEAN bHasUnselected = FALSE;
    UINT position = 0;

    // If we were optimizing for the UnselectAll-only case, turn off the optimization here.
    IFC(UnrollUnselectAllRequest());

    IFC(m_spItemsToUnselect->Has(itemIndex, position, bHasUnselected));
    if (bHasUnselected)
    {
        ctl::ComPtr<IInspectable> spUnselectedItem;
        bool areEqual = false;

        IFC(m_spItemsToUnselect->GetAt(position, &spUnselectedItem));
        IFC(PropertyValue::AreEqual(pItem, spUnselectedItem.Get(), &areEqual));
        if (areEqual)
        {
            // We were going to unselect it, so don't
            IFC(m_spItemsToUnselect->RemoveAt(position));
            goto Cleanup;
        }
    }

    IFC(m_pOwnerNoRef->Has(itemIndex, position, bHasSelected));
    if(bHasSelected)
    {
        bool areEqual = false;
        IFC(m_pOwnerNoRef->GetAt(position, &spSelectedItem));
        IFC(PropertyValue::AreEqual(pItem, spSelectedItem.Get(), &areEqual));
        if (areEqual)
        {
            // It's already selected
            goto Cleanup;
        }
    }

    IFC(m_spItemsToSelect->Has(itemIndex, position, bHasSelected));
    if(bHasSelected)
    {
        bool areEqual = false;
        IFC(m_pOwnerNoRef->GetAt(position, &spSelectedItem));
        IFC(PropertyValue::AreEqual(pItem, spSelectedItem.Get(), &areEqual));
        if (areEqual)
        {
            // We're already going to select it
            goto Cleanup;
        }
    }

    // Too many items would be selected for the mode, deselect some
    if (!canSelectMultiple)
    {
        UINT nSelectedCount = 0;
        IFC(m_spItemsToSelect->get_Size(&nSelectedCount));

        for (UINT i = 0; i < nSelectedCount; ++i)
        {
            UINT itemIndexToUnselect = 0;
            IFC(m_spItemsToSelect->GetAt(i, &spSelectedItem));
            IFC(m_spItemsToSelect->GetIndexAt(i, itemIndexToUnselect));
            IFC(m_spItemsToUnselect->Add(itemIndexToUnselect, spSelectedItem.Get()));
        }

        IFC(m_spItemsToSelect->Clear());
    }

    IFC(m_spItemsToSelect->Add(itemIndex, pItem));

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
Selection::SelectionChangerImpl::Unselect(
    _In_ UINT itemIndex,
    _In_ IInspectable* pItem)
{
    HRESULT hr = S_OK;
    BOOLEAN bHasSelected = FALSE;
    BOOLEAN bHasUnselected = FALSE;
    UINT position = 0;

    if (m_unselectAllRequested)
    {
        // We are going to clear the entire selection anyway.
        goto Cleanup;
    }

    IFC(m_spItemsToSelect->Has(itemIndex, position, bHasSelected));
    if (bHasSelected)
    {
        ctl::ComPtr<IInspectable> spSelectedItem;
        bool areEqual = false;

        IFC(m_spItemsToSelect->GetAt(position, &spSelectedItem));
        IFC(PropertyValue::AreEqual(pItem, spSelectedItem.Get(), &areEqual));
        if (areEqual)
        {
            // If we were going to select it, don't
            IFC(m_spItemsToSelect->RemoveAt(position));
            goto Cleanup;
        }
    }

    IFC(m_pOwnerNoRef->Has(itemIndex, position, bHasSelected));
    if(!bHasSelected)
    {
        // If it's not selected, nothing to do
        goto Cleanup;
    }

    IFC(m_spItemsToUnselect->Has(itemIndex, position, bHasUnselected));
    if (bHasUnselected)
    {
        ctl::ComPtr<IInspectable> spUnselectedItem;
        bool areEqual = false;

        IFC(m_spItemsToUnselect->GetAt(position, &spUnselectedItem));
        IFC(PropertyValue::AreEqual(pItem, spUnselectedItem.Get(), &areEqual));
        if (areEqual)
        {
            // We're already going to unselect it
            goto Cleanup;
        }
    }

    IFC(m_spItemsToUnselect->Add(itemIndex, pItem));

Cleanup:
    RRETURN(hr);
}

// Unselect all selected items.
_Check_return_
HRESULT
Selection::SelectionChangerImpl::UnselectAll()
{
    HRESULT hr = S_OK;

    m_unselectAllRequested = TRUE;

    RRETURN(hr);
}

// If m_unselectAllRequested is true, clear it and
// remove all items by adding them to m_spItemsToUnselect.
// This is part of a perf optimization to rapidly handle
// the UnselectAll case.
_Check_return_
HRESULT
Selection::SelectionChangerImpl::UnrollUnselectAllRequest()
{
    HRESULT hr = S_OK;

    if (m_unselectAllRequested)
    {
        UINT nSelectedCount = 0;

        // Very important we clear this here, because
        // Unselect short-circuits if we don't.
        m_unselectAllRequested = FALSE;

        IFC(m_pOwnerNoRef->GetNumItemsSelected(nSelectedCount));
        for (UINT i = 0; i < nSelectedCount; ++i)
        {
            ctl::ComPtr<IInspectable> spSelectedItem;

            UINT itemIndex = 0;
            IFC(m_pOwnerNoRef->GetAt(i, &spSelectedItem));
            IFC(m_pOwnerNoRef->GetIndexAt(i, itemIndex));
            IFC(Unselect(itemIndex, spSelectedItem.Get()));
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
Selection::SelectionChangerImpl::Cancel()
{
    HRESULT hr = S_OK;

    ASSERT(m_bIsActive, L"SelectionChanger not active");

    IFC(Cleanup());

Cleanup:
    RRETURN(hr);
}

// We need to know if an item was removed, so we can correctly differentiate
// removed items from items which simply ended up with the removed item's index.
_Check_return_
HRESULT
Selection::SelectionChangerImpl::AccountForRemovedItem(_In_ UINT removedItemIndex)
{
    m_indexDeletedDuringThisChange.index = removedItemIndex;
    m_indexDeletedDuringThisChange.empty = false;

    RRETURN(S_OK);
}


BOOLEAN
Selection::SelectionChangerImpl::IsActive()
{
    return m_bIsActive;
}

_Check_return_
 HRESULT
 Selection::SelectionChangerImpl::Cleanup()
{
    HRESULT hr = S_OK;

    m_bIsActive = FALSE;
    m_unselectAllRequested = FALSE;
    if (m_spItemsToSelect)
    {
        IFC(m_spItemsToSelect->Clear());
    }
    if (m_spItemsToUnselect)
    {
        IFC(m_spItemsToUnselect->Clear());
    }

    m_indexDeletedDuringThisChange.empty = true;

Cleanup:
    RRETURN(hr);
}

// Ensures that the selection change is valid for the current mode
_Check_return_
HRESULT
Selection::SelectionChangerImpl::ApplyCanSelectMultiple(
    _In_ BOOLEAN canSelectMultiple)
{
    HRESULT hr = S_OK;
    UINT nToSelectCount = 0;

    if (canSelectMultiple)
    {
        goto Cleanup;
    }

    IFC(m_spItemsToSelect->get_Size(&nToSelectCount));
    ASSERT(nToSelectCount <= 1, L"Too many items selected for a single select selector");

    if (nToSelectCount == 1)
    {
        UINT nSelectedCount = 0;
        IFC(m_pOwnerNoRef->m_spSelectedItems->get_Size(&nSelectedCount));
        if (nSelectedCount > 0)
        {

            IFC(m_spItemsToUnselect->Clear());
            for (UINT nIndex = 0; nIndex < nSelectedCount; ++nIndex)
            {
                ctl::ComPtr<IInspectable> spItem;
                UINT itemIndex = 0;

                IFC(m_pOwnerNoRef->GetAt(nIndex, &spItem));
                IFC(m_pOwnerNoRef->GetIndexAt(nIndex, itemIndex));

                IFC(m_spItemsToUnselect->Add(itemIndex, spItem.Get()));
            }
        }
    }
    else
    {
        UINT nSelectedCount = 0;
        IFC(m_spItemsToSelect->Clear());
        IFC(m_pOwnerNoRef->m_spSelectedItems->get_Size(&nSelectedCount));
        if (nSelectedCount > 1)
        {
            IFC(m_spItemsToUnselect->Clear());
            for (UINT nIndex = 1; nIndex < nSelectedCount; ++nIndex)
            {
                ctl::ComPtr<IInspectable> spItem;
                UINT itemIndex = 0;

                IFC(m_pOwnerNoRef->GetAt(nIndex, &spItem));
                IFC(m_pOwnerNoRef->GetIndexAt(nIndex, itemIndex));

                IFC(m_spItemsToUnselect->Add(itemIndex, spItem.Get()));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
Selection::SelectionChangerImpl::CreateDeltaSelectionChange(
    _In_ wfc::IVector<IInspectable*>* pUnselectedItems,
    _In_ wfc::IVector<IInspectable*>* pSelectedItems)
{
    UINT nUnselectedCount = 0;
    UINT nSelectedCount = 0;

    IFC_RETURN(m_spItemsToUnselect->get_Size(&nUnselectedCount));
    IFC_RETURN(m_spItemsToSelect->get_Size(&nSelectedCount));

    // Make sure that selection change is consistent
    // Create list of items that are actually to be selected/deselected
    if (nUnselectedCount == 0 && nSelectedCount == 0)
    {
        return S_OK;
    }

    // We currently don't support removing items and selecting items in one operation.
    // The way our code is architected we shouldn't ever try this. This is here in case
    // there's a side-path we don't know about.
    ASSERT(nSelectedCount == 0 || m_indexDeletedDuringThisChange.empty);

    for (UINT nIndex = 0; nIndex < nUnselectedCount; ++nIndex)
    {
        ctl::ComPtr<IInspectable> spItem;
        UINT itemIndex = 0;
        UINT position = 0;
        BOOLEAN hasSelected = FALSE;

        IFC_RETURN(m_spItemsToUnselect->GetAt(nIndex, &spItem));
        IFC_RETURN(m_spItemsToUnselect->GetIndexAt(nIndex, itemIndex));
        // Note that itemIndex may well be an old index if items were added or removed.
        // This is fine but we need to be careful because the removed item's index
        // may now belong an item that used to be just after the removed item.
        // This will confuse the change applier, which isn't capable of understanding
        // removed items.
        // If the item was removed, we don't do any extra processing as the item
        // was already removed from m_spSelectedItems by RemoveSelectedIndex.
        if (m_indexDeletedDuringThisChange.empty || m_indexDeletedDuringThisChange.index != itemIndex)
        {
            // itemIdx can be -1 for Custom values, these values don't inherit from ISelectorItem so don't try to unselect them.
            if ((int)itemIndex >= 0)
            {
                IFC_RETURN(m_pChangeApplier->UnselectIndex(itemIndex));
            }

            IFC_RETURN(m_pOwnerNoRef->Has(itemIndex, position, hasSelected));
            IFCEXPECT_RETURN(hasSelected);

            IFC_RETURN(m_pOwnerNoRef->m_spSelectedItems->RemoveAt(position));
        }

        IFC_RETURN(pUnselectedItems->Append(spItem.Get()));
    }

    // Select all selected items
    for (UINT nIndex = 0; nIndex < nSelectedCount; ++nIndex)
    {
        ctl::ComPtr<IInspectable> spItem;
        UINT itemIndex = 0;

        IFC_RETURN(m_spItemsToSelect->GetAt(nIndex, &spItem));
        IFC_RETURN(m_spItemsToSelect->GetIndexAt(nIndex, itemIndex));

        // itemIdx can be -1 for Custom values, these values don't inherit from ISelectorItem so don't try to select them.
        if ((int)itemIndex >= 0)
        {
            IFC_RETURN(m_pChangeApplier->SelectIndex(itemIndex));
        }

#if DBG
        UINT position = 0;
        BOOLEAN hasSelected = FALSE;
        ASSERT(SUCCEEDED(m_pOwnerNoRef->Has(itemIndex, position, hasSelected)) && !hasSelected);
#endif

        IFC_RETURN(m_pOwnerNoRef->m_spSelectedItems->Add(itemIndex, spItem.Get()));
        IFC_RETURN(pSelectedItems->Append(spItem.Get()));
    }
    return S_OK;
}

#pragma endregion


#pragma region InternalSelectedItemsStorage implementation

_Check_return_
HRESULT
Selection::InternalSelectedItemsStorage::Add(
    _In_ UINT itemIndex,
    _In_ IInspectable* pItem)
{
    HRESULT hr = S_OK;
    m_indexlist.push_back(itemIndex);
    IFC(TrackerCollection::Append(pItem));

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
Selection::InternalSelectedItemsStorage::Has(
    _In_ UINT itemIndex,
    _Out_ UINT& position,
    _Out_ BOOLEAN& hasItem)
{
    hasItem = FALSE;
    position = 0;

    std::vector<UINT>::iterator it;
    UINT nPosition = 0;

    for (nPosition = 0, it = m_indexlist.begin(); it != m_indexlist.end(); ++nPosition, ++it)
    {
        if (*it == itemIndex)
        {
            position = nPosition;
            hasItem = TRUE;
            break;
        }
    }

    RRETURN(S_OK);
}

_Check_return_
HRESULT
Selection::InternalSelectedItemsStorage::GetIndexAt(
    _In_ UINT position,
    _Out_ UINT& itemIndex)
{
    itemIndex = m_indexlist.at(position);
    RRETURN(S_OK);
}

_Check_return_
HRESULT
Selection::InternalSelectedItemsStorage::Inserted(
    _In_ UINT itemIndex)
{
    std::vector<UINT>::iterator it;

    // An item was inserted. Update the selected index
    // list so our indexes keep matching up.
    for (it = m_indexlist.begin(); it != m_indexlist.end(); ++it)
    {
        if (itemIndex <= *it)
        {
            ++(*it);
        }
    }
    RRETURN(S_OK);
}

_Check_return_
HRESULT
Selection::InternalSelectedItemsStorage::Removed(
    _In_ UINT itemIndex)
{
    HRESULT hr = S_OK;

    std::vector<UINT>::iterator it;
    UINT indexInIndexList = 0;

    // An item was removed. Update the selected index
    // list so our indexes keep matching up.
    // We also need to remove the item which has been removed.
    for (it = m_indexlist.begin(); it != m_indexlist.end();)
    {
        if (itemIndex == *it)
        {
            it = m_indexlist.erase(it);
            IFC(TrackerCollection::RemoveAt(indexInIndexList));
        }
        else
        {
            if (itemIndex < *it)
            {
                --(*it);
            }
            ++it;
            ++indexInIndexList;
        }
    }

Cleanup:
    RRETURN(hr);
}

// Returns a copy of the list of selected indexes.
std::vector<UINT>
Selection::InternalSelectedItemsStorage::GetSelectedIndexes() const
{
    // Clone the selected index list.
    return m_indexlist;

}

IFACEMETHODIMP
Selection::InternalSelectedItemsStorage::RemoveAt(
    _In_ UINT position)
{
    HRESULT hr = S_OK;

    IFCEXPECTRC(position < m_indexlist.size(), E_BOUNDS);
    m_indexlist.erase(m_indexlist.begin() + position);
    IFC(TrackerCollection::RemoveAt(position));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
Selection::InternalSelectedItemsStorage::Clear()
{
    HRESULT hr = S_OK;
    m_indexlist.clear();
    IFC(TrackerCollection::Clear());

Cleanup:
    RRETURN(hr);
}

#pragma endregion
