// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents a PageStackEntryTrackerCollection.
//      Wraps TrackerCollection<PageStackEntry> and provides VectorChanging
//      and VectorChanged events.

#include "precomp.h"
#include "PageStackEntryTrackerCollection.h"
#include "NavigationHistory.h"
#include "PageStackEntry.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT
PageStackEntryTrackerCollection::Init(
    _In_ NavigationHistory* pNavigationHistory,
    _In_ BOOLEAN isBackStack)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES

    IFC(SetNavigationHistory(pNavigationHistory));
    m_isBackStack = isBackStack;

Cleanup:
    RRETURN(S_OK);
}

IFACEMETHODIMP PageStackEntryTrackerCollection::SetAt(_In_ UINT index, _In_opt_ T_abi item)
{
    HRESULT hr = S_OK;

    UINT nCount = 0;
    IFC(get_Size(&nCount));
    IFCEXPECTRC(index < nCount, E_BOUNDS);

    IFC(OnVectorChanging(wfc::CollectionChange_ItemChanged, index, item));
    IFC(TrackerCollection<xaml::Navigation::PageStackEntry*>::SetAt(index, item));
    IFC(OnVectorChanged(wfc::CollectionChange_ItemInserted, index));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP PageStackEntryTrackerCollection::InsertAt(_In_ UINT index, _In_ T_abi item)
{
    HRESULT hr = S_OK;

    UINT nCount = 0;
    IFC(get_Size(&nCount));
    IFCEXPECTRC(index <= nCount, E_BOUNDS);

    IFC(OnVectorChanging(wfc::CollectionChange_ItemInserted, index, item));
    IFC(TrackerCollection<xaml::Navigation::PageStackEntry*>::InsertAt(index, item));
    IFC(OnVectorChanged(wfc::CollectionChange_ItemInserted, index));

 Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP PageStackEntryTrackerCollection::RemoveAt(_In_ UINT index)
{
    HRESULT hr = S_OK;

    UINT nCount = 0;
    IFC(get_Size(&nCount));
    IFCEXPECTRC(index < nCount, E_BOUNDS);

    IFC(OnVectorChanging(wfc::CollectionChange_ItemRemoved, index, NULL));
    IFC(TrackerCollection<xaml::Navigation::PageStackEntry*>::RemoveAt(index));
    IFC(OnVectorChanged(wfc::CollectionChange_ItemRemoved, index));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP PageStackEntryTrackerCollection::Append(_In_opt_ T_abi item)
{
    HRESULT hr = S_OK;
    UINT nCount = 0;

    IFC(get_Size(&nCount));
    IFC(OnVectorChanging(wfc::CollectionChange_ItemInserted, nCount, item));
    IFC(TrackerCollection<xaml::Navigation::PageStackEntry*>::Append(item));
    IFC(OnVectorChanged(wfc::CollectionChange_ItemInserted, nCount));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP PageStackEntryTrackerCollection::RemoveAtEnd()
{
    HRESULT hr = S_OK;
    UINT nCount = 0;

    IFC(get_Size(&nCount));

    IFCEXPECTRC(nCount > 0, E_BOUNDS);

    IFC(RemoveAt(nCount - 1));
Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP PageStackEntryTrackerCollection::Clear()
{
    HRESULT hr = S_OK;
    IFC(OnVectorChanging(wfc::CollectionChange_Reset, 0, NULL));
    IFC(TrackerCollection<xaml::Navigation::PageStackEntry*>::Clear());
    IFC(OnVectorChanged(wfc::CollectionChange_Reset, 0));

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method: AppendInternal
//
//  Synopsis:
//     Append without any change notifiers, for internal use.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
PageStackEntryTrackerCollection::AppendInternal(
    _In_opt_ T_abi item)
{
    HRESULT hr = S_OK;

    IFC(TrackerCollection<xaml::Navigation::PageStackEntry*>::Append(item));

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method: RemoveAtEndInternal
//
//  Synopsis:
//     RemoveAtEnd without any change notifiers, for internal use.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
PageStackEntryTrackerCollection::RemoveAtEndInternal()
{
    HRESULT hr = S_OK;

    IFC(TrackerCollection<xaml::Navigation::PageStackEntry*>::RemoveAtEnd());

Cleanup:
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Method: ClearInternal
//
//  Synopsis:
//     Clear without any change notifiers, for internal use.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
PageStackEntryTrackerCollection::ClearInternal()
{
    HRESULT hr = S_OK;

    IFC(TrackerCollection<xaml::Navigation::PageStackEntry*>::Clear());

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method: GetAtEnd
//
//  Synopsis:
//     Get the last entry in the Collection.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
PageStackEntryTrackerCollection::GetAtEnd(_Outptr_ T_abi *item)
{
    HRESULT hr = S_OK;
    UINT nCount = 0;

    IFC(get_Size(&nCount));

    IFC(TrackerCollection<xaml::Navigation::PageStackEntry*>::GetAt(nCount - 1, item));

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method: OnVectorChanging
//
//  Synopsis:
//     Notify the owner of this collection (NavigationHistory) that
//     the collection is changing.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
PageStackEntryTrackerCollection::OnVectorChanging(
    _In_ wfc::CollectionChange action,
    _In_ UINT index,
    _In_opt_ T_abi item)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<NavigationHistory> spNavigationHistory;
    ctl::ComPtr<xaml::Navigation::IPageStackEntry> spItem;

    spItem = ctl::query_interface_cast<xaml::Navigation::IPageStackEntry>(item);
    PageStackEntry* pEntry = spItem.Cast<PageStackEntry>();

    IFC(GetNavigationHistory(&spNavigationHistory));
    if(spNavigationHistory)
    {
        IFC(spNavigationHistory->OnPageStackChanging(m_isBackStack, action, index, pEntry));
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method: OnVectorChanged
//
//  Synopsis:
//     Notify the owner of this collection (NavigationHistory) that
//     the collection changed.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
PageStackEntryTrackerCollection::OnVectorChanged(
    _In_ wfc::CollectionChange action,
    _In_ UINT index)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<NavigationHistory> spNavigationHistory;

    IFC(GetNavigationHistory(&spNavigationHistory));
    if(spNavigationHistory)
    {
        IFC(spNavigationHistory->OnPageStackChanged(m_isBackStack, action, index));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PageStackEntryTrackerCollection::SetNavigationHistory(_In_ NavigationHistory* pNavigationHistory)
{
    HRESULT hr = S_OK;
    IFC(ctl::AsWeak(pNavigationHistory, &m_wrNavigationHistory));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PageStackEntryTrackerCollection::GetNavigationHistory(_Outptr_ NavigationHistory** ppNavigationHistory)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spNavigationHistory;

    *ppNavigationHistory = NULL;

    IFC(m_wrNavigationHistory.As(&spNavigationHistory));
    *ppNavigationHistory = static_cast<NavigationHistory*>(spNavigationHistory.Detach());

Cleanup:
    RRETURN(hr);
}
