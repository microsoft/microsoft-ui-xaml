// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      CVS shaping engine: currency of a shaped view (WPF: ListCollectionView.RefreshOverride,
//      AdjustCurrencyForMove / ForReplace, MoveCurrencyOffDeletedElement). See VectorCollectionView_Shaping.cpp.
//
//      An unshaped view keeps CollectionView's currency contract unchanged. A shaped view differs only for the
//      changes it makes itself:
//      - A recompute (a rule change, Refresh, a source Reset, grouping on or off) raises a non-cancelable
//        CurrentChanging first, places the currency before the Reset is raised (before the first item and after the
//        last stay there, else the current item is followed, else the first item), and raises CurrentChanged once.
//      - A live restore moves an item with ItemRemoved + ItemInserted (WinRT has no Move): when the current item only
//        moved, currency moves with it and only CurrentPosition changes, as WPF's Move. An item that left the view
//        moves currency to the item now at its place (or the last), as a removal does.
//      - A source Replace that keeps the current item's place moves currency to the new item (WPF), not to the first
//        item (CollectionView looks the replaced item up).

#include "precomp.h"
#include "VectorCollectionView_ShapingInternal.h"

using namespace DirectUI;

void VectorCollectionView::EnterCurrencyScope(_Inout_ CurrencyScope* pScope)
{
    pScope->pPrevious = m_pCurrencyScope;
    pScope->ppSlot = &m_pCurrencyScope;
    m_pCurrencyScope = pScope;
}

// Handlers of the currency changes an End raises (and a recompute they cause) are outside the scope.
void VectorCollectionView::LeaveCurrencyScope(_Inout_ CurrencyScope* pScope)
{
    if (pScope->ppSlot)
    {
        *pScope->ppSlot = pScope->pPrevious;
        pScope->ppSlot = nullptr;
    }
}

_Check_return_ HRESULT VectorCollectionView::BeginRefreshCurrency(_Inout_ CurrencyScope* pScope, bool isChangingRaisedFirst)
{
    IFC_RETURN(get_CurrentItemImpl(pScope->spItem.ReleaseAndGetAddressOf()));
    IFC_RETURN(get_CurrentPositionImpl(&pScope->position));
    IFC_RETURN(get_IsCurrentBeforeFirstImpl(&pScope->wasBeforeFirst));
    IFC_RETURN(get_IsCurrentAfterLastImpl(&pScope->wasAfterLast));
    EnterCurrencyScope(pScope);

    // WPF raises it before recomputing, so a handler sees the old currency ("save dirty information"). A recompute
    // that may raise nothing (an unchanged output) raises it with its Reset instead.
    if (isChangingRaisedFirst)
    {
        pScope->isChangingRaised = true;
        BusyScope busy(m_shapeBusyDepth);
        IFC_RETURN(RaiseCurrentChanging());
    }
    return S_OK;
}

// WPF: ListCollectionView.RefreshOverride. Silent: the Reset that follows announces it, CurrentChanged after it.
_Check_return_ HRESULT VectorCollectionView::PlaceCurrencyAfterRefresh(_Inout_ CurrencyScope* pScope)
{
    ctl::ComPtr<IInspectable> spItem;
    ctl::ComPtr<IInspectable> spNewItem;
    BOOLEAN isBeforeFirst = FALSE;
    BOOLEAN isAfterLast = FALSE;
    UINT size = 0;
    INT position = -1;

    // A second Reset in the same recompute follows the first's placement, not what a handler of it did.
    if (pScope->isPlaced)
    {
        spItem = pScope->spPlacedItem;
        isBeforeFirst = pScope->placedPosition < 0;
        isAfterLast = pScope->isPlacedAfterLast;
    }
    else
    {
        IFC_RETURN(get_CurrentItemImpl(spItem.ReleaseAndGetAddressOf()));
        IFC_RETURN(get_IsCurrentBeforeFirstImpl(&isBeforeFirst));
        IFC_RETURN(get_IsCurrentAfterLastImpl(&isAfterLast));
    }

    IFC_RETURN(get_Size(&size));
    if (isBeforeFirst || size == 0)
    {
        position = -1;
    }
    else if (isAfterLast)
    {
        position = static_cast<INT>(size);
    }
    else
    {
        UINT index = 0;
        BOOLEAN isFound = FALSE;
        IFC_RETURN(IndexOf(spItem.Get(), &index, &isFound));
        position = isFound ? static_cast<INT>(index) : 0;
        IFC_RETURN(GetAt(static_cast<UINT>(position), spNewItem.ReleaseAndGetAddressOf()));
    }

    SetCurrentNoRaise(spNewItem.Get(), position, size);
    pScope->isPlaced = true;
    pScope->placedPosition = position;
    pScope->isPlacedAfterLast = (position >= static_cast<INT>(size));
    pScope->spPlacedItem = spNewItem;
    return S_OK;
}

_Check_return_ HRESULT VectorCollectionView::EndRefreshCurrency(_Inout_ CurrencyScope* pScope)
{
    ctl::ComPtr<IInspectable> spNowItem;
    BOOLEAN isBeforeFirst = FALSE;
    BOOLEAN isAfterLast = FALSE;
    INT nowPosition = -1;

    LeaveCurrencyScope(pScope);

    // Nothing was announced (an unchanged output): nothing to place.
    if (!pScope->isChangingRaised)
    {
        return S_OK;
    }

    // No Reset reached the view.
    if (!pScope->isPlaced)
    {
        IFC_RETURN(PlaceCurrencyAfterRefresh(pScope));
    }

    if (pScope->pPrevious && pScope->pPrevious->isLive)
    {
        pScope->pPrevious->isResolved = true;
        pScope->pPrevious->isDetached = false;
    }

    IFC_RETURN(get_CurrentItemImpl(spNowItem.ReleaseAndGetAddressOf()));
    IFC_RETURN(get_CurrentPositionImpl(&nowPosition));
    IFC_RETURN(get_IsCurrentBeforeFirstImpl(&isBeforeFirst));
    if (isBeforeFirst && pScope->placedPosition >= 0)
    {
        // A handler of the Reset moved it before the first item: a synchronized Selector clears a selection a Reset
        // moved, which moves the currency off the item. Placed again, so the Selector selects the followed item again.
        // A handler that moved it to an item (MoveCurrentTo) is respected, as in WPF.
        IFC_RETURN(RaiseCurrentChanging());
        IFC_RETURN(SetCurrentIndexAt(pScope->placedPosition, nullptr));
        return S_OK;
    }

    // In CollectionView's order: the properties, then CurrentChanged.
    IFC_RETURN(get_IsCurrentAfterLastImpl(&isAfterLast));
    if (nowPosition != pScope->position)
    {
        IFC_RETURN(RaisePropertyChanged(STR_LEN_PAIR(L"CurrentPosition")));
    }
    if (spNowItem.Get() != pScope->spItem.Get())
    {
        IFC_RETURN(RaisePropertyChanged(STR_LEN_PAIR(L"CurrentItem")));
    }
    if (isAfterLast != pScope->wasAfterLast)
    {
        IFC_RETURN(RaisePropertyChanged(STR_LEN_PAIR(L"IsCurrentAfterLast")));
    }
    if (isBeforeFirst != pScope->wasBeforeFirst)
    {
        IFC_RETURN(RaisePropertyChanged(STR_LEN_PAIR(L"IsCurrentBeforeFirst")));
    }
    IFC_RETURN(RaiseCurrentChanged());
    return S_OK;
}

_Check_return_ HRESULT VectorCollectionView::BeginLiveCurrency(_Inout_ CurrencyScope* pScope)
{
    pScope->isLive = true;
    IFC_RETURN(get_CurrentItemImpl(pScope->spItem.ReleaseAndGetAddressOf()));
    IFC_RETURN(get_CurrentPositionImpl(&pScope->position));
    IFC_RETURN(get_IsCurrentBeforeFirstImpl(&pScope->wasBeforeFirst));
    IFC_RETURN(get_IsCurrentAfterLastImpl(&pScope->wasAfterLast));
    EnterCurrencyScope(pScope);
    return S_OK;
}

// After a live restore the currency is on the item it was on, wherever the item moved. A handler that moved it
// meanwhile (a synchronized Selector clears the selection of a removed item) is overruled, as before. An item that
// left the view moves currency to the item now at its place, or the last (WPF: MoveCurrencyOffDeletedElement).
_Check_return_ HRESULT VectorCollectionView::EndLiveCurrency(_Inout_ CurrencyScope* pScope)
{
    ctl::ComPtr<IInspectable> spNowItem;
    INT nowPosition = -1;
    UINT size = 0;
    UINT index = 0;
    BOOLEAN isFound = FALSE;

    LeaveCurrencyScope(pScope);
    if (pScope->isResolved || pScope->wasBeforeFirst || pScope->wasAfterLast || !pScope->spItem)
    {
        return S_OK;
    }

    IFC_RETURN(get_Size(&size));
    IFC_RETURN(IndexOf(pScope->spItem.Get(), &index, &isFound));
    IFC_RETURN(get_CurrentItemImpl(spNowItem.ReleaseAndGetAddressOf()));
    IFC_RETURN(get_CurrentPositionImpl(&nowPosition));
    pScope->isDetached = false;

    if (isFound)
    {
        if (spNowItem.Get() != pScope->spItem.Get())
        {
            IFC_RETURN(RaiseCurrentChanging());
            IFC_RETURN(SetCurrentIndexAt(static_cast<INT>(index), nullptr));
        }
        else if (nowPosition != static_cast<INT>(index))
        {
            SetCurrentNoRaise(pScope->spItem.Get(), static_cast<INT>(index), size);
            IFC_RETURN(RaisePropertyChanged(STR_LEN_PAIR(L"CurrentPosition")));
        }
        return S_OK;
    }

    INT target = (pScope->detachedPosition >= 0) ? pScope->detachedPosition : pScope->position;
    target = std::min(target, static_cast<INT>(size) - 1);
    IFC_RETURN(RaiseCurrentChanging());
    IFC_RETURN(SetCurrentIndexAt(target, nullptr));
    return S_OK;
}

_Check_return_ HRESULT VectorCollectionView::AdjustCurrencyForChange(_In_ wfc::IVectorChangedEventArgs* pArgs, bool isRaised, _Out_ bool* pIsHandled)
{
    wfc::CollectionChange change = wfc::CollectionChange_Reset;
    UINT index = 0;
    auto pScope = m_pCurrencyScope;

    *pIsHandled = false;
    if (!IsShaped())
    {
        return S_OK;
    }

    IFC_RETURN(pArgs->get_CollectionChange(&change));
    if (change == wfc::CollectionChange_Reset)
    {
        if (!pScope || pScope->isLive)
        {
            // CollectionView follows the current item (a live restore that re-sorts with a Reset).
            if (pScope)
            {
                pScope->isDetached = false;
            }
            return S_OK;
        }

        if (!isRaised)
        {
            if (!pScope->isChangingRaised)
            {
                pScope->isChangingRaised = true;
                IFC_RETURN(RaiseCurrentChanging());
            }
            IFC_RETURN(PlaceCurrencyAfterRefresh(pScope));
        }
        *pIsHandled = true;
        return S_OK;
    }

    IFC_RETURN(pArgs->get_Index(&index));
    const INT changeIndex = static_cast<INT>(index);

    if (pScope && pScope->isLive)
    {
        ctl::ComPtr<IInspectable> spNowItem;
        INT nowPosition = -1;
        IFC_RETURN(get_CurrentItemImpl(spNowItem.ReleaseAndGetAddressOf()));
        IFC_RETURN(get_CurrentPositionImpl(&nowPosition));

        if (isRaised && pScope->isMoveRaisePending)
        {
            pScope->isMoveRaisePending = false;
            IFC_RETURN(RaisePropertyChanged(STR_LEN_PAIR(L"CurrentPosition")));
            *pIsHandled = true;
            return S_OK;
        }

        if (pScope->isDetached)
        {
            // Moved meanwhile by a handler: CollectionView follows it again.
            if (spNowItem.Get() != pScope->spItem.Get())
            {
                pScope->isDetached = false;
                return S_OK;
            }

            if (!isRaised)
            {
                if (change == wfc::CollectionChange_ItemInserted)
                {
                    ctl::ComPtr<IInspectable> spInserted;
                    IFC_RETURN(GetAt(index, spInserted.ReleaseAndGetAddressOf()));
                    if (spInserted.Get() == pScope->spItem.Get())
                    {
                        // The current item only moved (WPF: AdjustCurrencyForMove): currency moves with it, and
                        // only CurrentPosition changes.
                        UINT size = 0;
                        IFC_RETURN(get_Size(&size));
                        SetCurrentNoRaise(spInserted.Get(), changeIndex, size);
                        pScope->isDetached = false;
                        pScope->isMoveRaisePending = (changeIndex != nowPosition);
                    }
                    else if (changeIndex <= pScope->detachedPosition)
                    {
                        ++pScope->detachedPosition;
                    }
                }
                else if (change == wfc::CollectionChange_ItemRemoved && changeIndex < pScope->detachedPosition)
                {
                    --pScope->detachedPosition;
                }
            }
            *pIsHandled = true;
            return S_OK;
        }

        // The current item leaves; it comes back if it only moved.
        if (change == wfc::CollectionChange_ItemRemoved && changeIndex == nowPosition && spNowItem.Get() == pScope->spItem.Get())
        {
            pScope->isDetached = true;
            pScope->detachedPosition = nowPosition;
            *pIsHandled = true;
            return S_OK;
        }
    }

    if (change == wfc::CollectionChange_ItemChanged)
    {
        INT nowPosition = -1;
        IFC_RETURN(get_CurrentPositionImpl(&nowPosition));
        if (isRaised)
        {
            // WPF: AdjustCurrencyForReplace, then MoveCurrencyOffDeletedElement: the replacement becomes current.
            IFC_RETURN(RaiseCurrentChanging());
            IFC_RETURN(SetCurrentIndexAt(changeIndex, nullptr));
            *pIsHandled = true;
        }
        else if (changeIndex == nowPosition)
        {
            *pIsHandled = true;
        }
    }
    return S_OK;
}
