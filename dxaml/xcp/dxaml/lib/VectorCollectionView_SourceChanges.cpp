// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      CVS shaping engine: applying a source Add, Remove, Replace or Reset to a flat or grouped shaped view
//      (WPF: ListCollectionView.ProcessCollectionChanged). See VectorCollectionView_Shaping.cpp.

#include "precomp.h"
#include "VectorCollectionView_ShapingInternal.h"

using namespace DirectUI;

// A source change (WPF: ListCollectionView.ProcessCollectionChanged).
_Check_return_ HRESULT VectorCollectionView::ProcessCollectionChanged(_In_ wfc::IVectorChangedEventArgs* pArgs)
{
    wfc::CollectionChange change = wfc::CollectionChange_Reset;
    UINT index = 0;
    bool isApplied = false;

    ++m_sourceListChangeCount;

    if (!m_tpInternalList)
    {
        m_areSourceIndicesValid = false;
        return S_OK;
    }

    // A change made while app code runs (a Filter, a getter) or while the view raises its changes (a handler)
    // is applied once that unwinds, by a recompute.
    if (m_shapeBusyDepth > 0)
    {
        m_areSourceIndicesValid = false;
        m_isRecomputePending = true;
        return S_OK;
    }

    if (m_deferRefreshLevel > 0)
    {
        m_areSourceIndicesValid = false;
        m_isRefreshPending = true;
        return S_OK;
    }

    IFC_RETURN(pArgs->get_CollectionChange(&change));

    // Some vectors (C++/CX Platform::Collections) fail Index on a Reset.
    if (change != wfc::CollectionChange_Reset)
    {
        IFC_RETURN(pArgs->get_Index(&index));
    }

    // App code that fails while the change is planned leaves the output as it was (nothing is raised before the
    // plan is complete); a recompute then applies the change, or, if it fails too, the next change does.
    RaiseFailureScope raiseFailure;
    HRESULT hr = S_OK;
    if (m_areSourceIndicesValid && change != wfc::CollectionChange_Reset)
    {
        hr = ProcessCollectionChangedWithAdjustedIndex(change, index, &isApplied);
        if (FAILED(hr))
        {
            ContainedFailureReport::Capture(hr);
            m_areSourceIndicesValid = false;
            isApplied = false;
        }
    }

    // A failed recompute marks the view itself; one that only raised a failing handler leaves it consistent.
    const HRESULT hrUpdate = isApplied ? UpdateLiveShapingItems(change, index) : RefreshOrDefer();
    if (FAILED(hrUpdate))
    {
        m_areSourceIndicesValid = m_areSourceIndicesValid && !isApplied;
        KeepFirstFailure(&hr, hrUpdate);
    }
    else
    {
        KeepFirstFailure(&hr, RunPendingRecompute());
    }
    KeepFirstFailure(&hr, raiseFailure.Result());
    return hr;
}

// Applies one Add, Remove or Replace from the source to a flat shaped view as the matching vector
// change, at the position a full recompute would give the item. Leaves *pIsApplied false when a
// full recompute is needed instead.
_Check_return_ HRESULT VectorCollectionView::ProcessCollectionChangedWithAdjustedIndex(wfc::CollectionChange change, UINT index, _Out_ bool* pIsApplied)
{
    auto pOutput = static_cast<InternalList*>(m_tpInternalList.Get());
    const UINT sourceChangeCount = m_sourceListChangeCount;
    const UINT notFound = static_cast<UINT>(-1);
    UINT sourceSize = 0;
    UINT oldPosition = notFound;
    UINT newPosition = notFound;
    ctl::ComPtr<IInspectable> spItem;
    HRESULT hr = S_OK;

    *pIsApplied = false;

    IFC_RETURN(m_tpSourceList->get_Size(&sourceSize));
    if (change != wfc::CollectionChange_ItemInserted && change != wfc::CollectionChange_ItemRemoved && change != wfc::CollectionChange_ItemChanged)
    {
        return S_OK;
    }

    // The source has already changed; a size or index that does not match ours means a missed change.
    const UINT expectedSize = m_sourceListSize
        + ((change == wfc::CollectionChange_ItemInserted) ? 1 : 0)
        - ((change == wfc::CollectionChange_ItemRemoved) ? 1 : 0);
    if (!m_spSortState || sourceSize != expectedSize || index >= ((sourceSize > m_sourceListSize) ? sourceSize : m_sourceListSize))
    {
        return S_OK;
    }
    m_sourceListSize = sourceSize;

    if (m_isGrouping)
    {
        IFC_RETURN(ProcessGroupedCollectionChanged(change, index, pIsApplied));
        return S_OK;
    }

    if (m_isHierarchical)
    {
        IFC_RETURN(ProcessHierarchyCollectionChanged(change, index, pIsApplied));
        return S_OK;
    }

    auto spSort = m_spSortState;
    auto& slots = spSort->flatSlots;
    if (change != wfc::CollectionChange_ItemInserted)
    {
        auto found = std::find(m_sourceIndices.begin(), m_sourceIndices.end(), index);
        if (found != m_sourceIndices.end())
        {
            oldPosition = static_cast<UINT>(found - m_sourceIndices.begin());
        }
    }

    if (change == wfc::CollectionChange_ItemInserted)
    {
        for (auto& sourceIndex : m_sourceIndices)
        {
            if (sourceIndex >= index)
            {
                ++sourceIndex;
            }
        }
    }

    // App code (Filter, property getters, CustomSort) runs only here.
    SortKeys keys;
    if (change != wfc::CollectionChange_ItemRemoved)
    {
        IFC_RETURN(m_tpSourceList->GetAt(index, spItem.ReleaseAndGetAddressOf()));

        {
            BusyScope busy(m_shapeBusyDepth);
            hr = [&]() -> HRESULT
            {
                bool isAccepted = true;

                ShapingFilterEvents filterEvents;
                IFC_RETURN(GetFilterEvents(&filterEvents));
                IFC_RETURN(PassesFilter(spItem.Get(), filterEvents, &isAccepted));
                if (!isAccepted)
                {
                    return S_OK;
                }

                // The output without the replaced item; items whose live sort keys changed are not compared.
                IFC_RETURN(spSort->comparer.EvaluateKeys(spItem.Get(), &keys));
                IFC_RETURN(FindSortedRange(
                    static_cast<UINT>(m_sourceIndices.size()),
                    oldPosition,
                    [&](UINT position, int* pResult)
                    {
                        return CompareWithSlot(*spSort, spItem.Get(), keys, index,
                            [pOutput](UINT outputPosition, IInspectable** ppOther) { return pOutput->GetAt(outputPosition, ppOther); },
                            position, slots[position], m_sourceIndices[position], pResult);
                    },
                    [this](UINT position) { return m_sourceIndices[position]; },
                    [this, change, index](UINT sourceIndex) { return IsSortDirtyAfterSourceChange(change, index, sourceIndex); },
                    &newPosition));
                return S_OK;
            }();
        }
        IFC_RETURN(hr);

        if (sourceChangeCount != m_sourceListChangeCount)
        {
            return S_OK;
        }
    }

    // Bookkeeping first, then the notifications, each matching the output at the time it is raised.
    {
        BusyScope busy(m_shapeBusyDepth);
        hr = [&]() -> HRESULT
        {
            if (oldPosition != notFound && oldPosition == newPosition)
            {
                spSort->keys.Set(slots[oldPosition], keys);
                IFC_RETURN(pOutput->InternalSetAt(oldPosition, spItem.Get()));
                return S_OK;
            }

            if (oldPosition != notFound)
            {
                m_sourceIndices.erase(m_sourceIndices.begin() + oldPosition);
                spSort->keys.Free(slots[oldPosition]);
                slots.erase(slots.begin() + oldPosition);
            }

            if (change == wfc::CollectionChange_ItemRemoved)
            {
                for (auto& sourceIndex : m_sourceIndices)
                {
                    if (sourceIndex > index)
                    {
                        --sourceIndex;
                    }
                }
            }

            if (oldPosition != notFound)
            {
                IFC_RETURN(pOutput->InternalRemoveAt(oldPosition));
            }

            if (newPosition != notFound)
            {
                m_sourceIndices.insert(m_sourceIndices.begin() + newPosition, index);
                slots.insert(slots.begin() + newPosition, spSort->keys.Allocate(keys));
                IFC_RETURN(pOutput->InternalInsertAt(newPosition, spItem.Get()));
            }

            return S_OK;
        }();
    }
    IFC_RETURN(hr);

    // A handler changed the source while the change was raised: recompute fully.
    *pIsApplied = (sourceChangeCount == m_sourceListChangeCount);
    return S_OK;
}

// A source change on a grouped view (WPF: ListCollectionView.AddItemToGroups / RemoveItemFromGroups): the item
// joins or leaves its bottom-level group at its sorted position, a group it starts is inserted among its siblings,
// and a group it empties is removed. Each vector of the group tree raises its own change.
_Check_return_ HRESULT VectorCollectionView::ProcessGroupedCollectionChanged(wfc::CollectionChange change, UINT index, _Out_ bool* pIsApplied)
{
    auto pRoot = static_cast<CollectionViewGroupRoot*>(static_cast<CollectionViewGroupInternal*>(m_spGroupRoot.Get()));
    auto spSort = m_spSortState;
    const UINT sourceChangeCount = m_sourceListChangeCount;
    CollectionViewGroupInternal* pOldGroup = nullptr;
    UINT oldPosition = 0;
    GroupedInsertion insertion;
    ctl::ComPtr<IInspectable> spItem;
    SortKeys keys;
    HRESULT hr = S_OK;
    auto isSkipped = [this, change, index](UINT sourceIndex) { return IsSortDirtyAfterSourceChange(change, index, sourceIndex); };

    *pIsApplied = false;
    if (!pRoot || pRoot->HasMultiGroupItems())
    {
        return S_OK;
    }

    const bool hasOld = (change != wfc::CollectionChange_ItemInserted) && pRoot->FindLeaf(index, &pOldGroup, &oldPosition);
    if (change == wfc::CollectionChange_ItemInserted)
    {
        pRoot->ShiftSourceIndices(index, 1);
    }

    // App code (Filter, property getters, converters, comparers, NamesMatch) runs only here, before anything is
    // raised; the insertion is planned for the tree as the removal leaves it.
    bool isInPlace = false;
    bool isInMultipleGroups = false;
    if (change != wfc::CollectionChange_ItemRemoved)
    {
        IFC_RETURN(m_tpSourceList->GetAt(index, spItem.ReleaseAndGetAddressOf()));

        {
            BusyScope busy(m_shapeBusyDepth);
            hr = [&]() -> HRESULT
            {
                bool isAccepted = true;
                std::vector<ctl::ComPtr<IInspectable>> names;
                ShapingFilterEvents filterEvents;
                IFC_RETURN(GetFilterEvents(&filterEvents));
                IFC_RETURN(PassesFilter(spItem.Get(), filterEvents, &isAccepted));
                if (!isAccepted)
                {
                    return S_OK;
                }

                IFC_RETURN(spSort->comparer.EvaluateKeys(spItem.Get(), &keys));
                bool hasCollectionName = false;
                IFC_RETURN(pRoot->GetGroupNames(spItem.Get(), &names, &hasCollectionName));
                if (hasCollectionName)
                {
                    isInMultipleGroups = true;
                    return S_OK;
                }
                if (hasOld)
                {
                    bool isInOldGroup = false;
                    IFC_RETURN(pRoot->IsGroupPath(pOldGroup, names, &isInOldGroup));
                    if (isInOldGroup)
                    {
                        insertion.isAccepted = true;
                        insertion.pGroup = pOldGroup;
                        IFC_RETURN(FindLeafPosition(pOldGroup, *spSort, spItem.Get(), keys, index, oldPosition, isSkipped, &insertion.position));
                        isInPlace = (insertion.position == oldPosition);
                        return S_OK;
                    }
                }

                IFC_RETURN(PlanGroupedInsertion(pRoot, *spSort, spItem.Get(), keys, names, index, hasOld ? MakeGroupedExclusion(pOldGroup, oldPosition) : GroupedExclusion(), isSkipped, &insertion));
                return S_OK;
            }();
        }
        IFC_RETURN(hr);

        if (sourceChangeCount != m_sourceListChangeCount || isInMultipleGroups)
        {
            return S_OK;
        }
    }

    {
        BusyScope busy(m_shapeBusyDepth);
        hr = [&]() -> HRESULT
        {
            if (hasOld && isInPlace)
            {
                IFC_RETURN(ApplyGroupedReplace(*spSort, pOldGroup, oldPosition, spItem.Get(), keys));
                return S_OK;
            }

            if (hasOld)
            {
                IFC_RETURN(ApplyGroupedRemoval(pRoot, *spSort, pOldGroup, oldPosition));
            }

            if (change == wfc::CollectionChange_ItemRemoved)
            {
                pRoot->ShiftSourceIndices(index, -1);
            }

            if (insertion.isAccepted)
            {
                IFC_RETURN(ApplyGroupedInsertion(pRoot, *spSort, insertion, spItem.Get(), keys, index));
            }

            return S_OK;
        }();
    }
    IFC_RETURN(hr);

    // A handler changed the source while the change was raised: recompute fully. So does a change that left groups
    // ordered by their items (ItemCount) out of order.
    *pIsApplied = (sourceChangeCount == m_sourceListChangeCount);
    IFC_RETURN(CheckGroupOrderAfterChange(pIsApplied));
    return S_OK;
}

_Check_return_ HRESULT VectorCollectionView::CheckGroupOrderAfterChange(_Inout_ bool* pIsApplied)
{
    auto pRoot = static_cast<CollectionViewGroupRoot*>(static_cast<CollectionViewGroupInternal*>(m_spGroupRoot.Get()));
    if (*pIsApplied && m_isGrouping && pRoot && pRoot->HasVariableGroupOrder())
    {
        bool isValid = true;
        IFC_RETURN(pRoot->IsGroupOrderValid(pRoot, 0, &isValid));
        *pIsApplied = isValid;
    }
    return S_OK;
}
