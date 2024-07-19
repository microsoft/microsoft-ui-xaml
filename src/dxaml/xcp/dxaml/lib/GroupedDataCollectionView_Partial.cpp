// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "GroupedDataCollectionView.g.h"
#include "CollectionViewGroup.g.h"
#include "VectorChangedEventArgs.g.h"
#include "PropertyPathParser.h"
#include "PropertyPath.h"

using namespace DirectUI;
using namespace xaml;
using namespace xaml_data;

GroupedDataCollectionView::GroupedDataCollectionView(): m_nNumberOfItemsInAllGroups(cNumberOfItemsNotCached)
{ }

GroupedDataCollectionView::~GroupedDataCollectionView()
{
    if (m_epVectorChangedHandler)
    {
        auto spSource = m_tpObservable.GetSafeReference();
        if (spSource)
        {
            VERIFYHR(m_epVectorChangedHandler.DetachEventHandler(spSource.Get()));
        }
    }
}

IFACEMETHODIMP GroupedDataCollectionView::GetAt(_In_opt_ UINT index, _Out_  IInspectable **item)
{
    HRESULT hr = S_OK;
    UINT nGroups = 0;
    UINT nGroupSize = 0;
    UINT nGroupBaseIndex = 0;
    ICollectionViewGroup *pGroup = NULL;
    ctl::ComPtr<wfc::IObservableVector<IInspectable *>> spGroups;

    ARG_VALIDRETURNPOINTER(item);

    IFC(get_CollectionGroups(&spGroups));

    IFC(spGroups.Cast<ReadOnlyObservableTrackerCollection<IInspectable *>>()->get_Size(&nGroups));

    for (UINT i = 0; i < nGroups; i++)
    {
        IFC(GetGroupAt(i, &pGroup));
        IFC(GetGroupSize(pGroup, &nGroupSize));

        // The index is in this group, retrieve the
        // the item from it and return it
        if (index < (nGroupBaseIndex + nGroupSize))
        {
            IFC(GetGroupItem(pGroup, index - nGroupBaseIndex, item));
            goto Cleanup;
        }

        // The index is not in this group, update the base index and
        // move on to the next group
        nGroupBaseIndex += nGroupSize;

        ReleaseInterface(pGroup);
    }

Cleanup:

    ReleaseInterface(pGroup);

    RRETURN(hr);
}

IFACEMETHODIMP GroupedDataCollectionView::get_Size(_Out_ UINT *size)
{
    HRESULT hr = S_OK;

    ARG_VALIDRETURNPOINTER(size);

    if (m_nNumberOfItemsInAllGroups == cNumberOfItemsNotCached)
    {
        IFC(CalculateCount(&m_nNumberOfItemsInAllGroups));
    }

    *size = m_nNumberOfItemsInAllGroups;

Cleanup:

    RRETURN(hr);
}

IFACEMETHODIMP GroupedDataCollectionView::GetView(_Outptr_result_maybenull_ wfc::IVectorView<IInspectable *>** view)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVectorView<IInspectable *>> spResult;

    IFC(CheckThread());
    ARG_VALIDRETURNPOINTER(view);

    IFC(ctl::ComObject<TrackerView<IInspectable *>>::CreateInstance(spResult.ReleaseAndGetAddressOf()));
    spResult.Cast<TrackerView<IInspectable *>>()->SetCollection(this);

    *view = spResult.Detach();

Cleanup:

    RRETURN(hr);
}

IFACEMETHODIMP GroupedDataCollectionView::IndexOf(_In_opt_ IInspectable * value, _Out_ UINT *index, _Out_ BOOLEAN *found)
{
    HRESULT hr = S_OK;
    UINT nGroups = 0;
    UINT nCurrentIndex = 0;
    UINT nGroupSize = 0;
    ICollectionViewGroup *pGroup = NULL;
    wfc::IVector<IInspectable *> *pItems = NULL;
    ctl::ComPtr<wfc::IObservableVector<IInspectable *>> spGroups;

    ARG_VALIDRETURNPOINTER(index);
    ARG_VALIDRETURNPOINTER(found);

    IFC(get_CollectionGroups(&spGroups));
    IFC(spGroups.Cast<ReadOnlyObservableTrackerCollection<IInspectable *>>()->get_Size(&nGroups));

    for (UINT i = 0; i < nGroups; i++)
    {
        IFC(GetGroupAt(i, &pGroup));
        IFC(GetGroupItems(pGroup, &pItems));

        IFC(pItems->get_Size(&nGroupSize));
        IFC(pItems->IndexOf(value, index, found));

        // If we found the value in the current group we need to
        // offset the index by the current cumulative index
        if (*found)
        {
            *index += nCurrentIndex;
            goto Cleanup;
        }

        // Advance to the next group
        nCurrentIndex += nGroupSize;

        ReleaseInterface(pGroup);
        ReleaseInterface(pItems);
    }

    *index = 0;
    *found = false;

Cleanup:

    ReleaseInterface(pGroup);
    ReleaseInterface(pItems);

    RRETURN(hr);
}

// IIterable<IInspectable *>
IFACEMETHODIMP GroupedDataCollectionView::First(_Outptr_ wfc::IIterator<IInspectable *> **value)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IIterator<IInspectable *>> spResult;

    IFC(CheckThread());
    ARG_VALIDRETURNPOINTER(value);

    IFC(ctl::ComObject<TrackerIterator<IInspectable *>>::CreateInstance(spResult.ReleaseAndGetAddressOf()));
    spResult.Cast<TrackerIterator<IInspectable *>>()->SetCollection(this);

    *value = spResult.Detach();

Cleanup:

    RRETURN(hr);
}

// And finally return the groups
_Check_return_
HRESULT
GroupedDataCollectionView::get_CollectionGroupsImpl(_Out_ wfc::IObservableVector<IInspectable *> **value)
{
    RRETURN(m_tpCollectionGroups.CopyTo(value));
}

// IPropertyPathListenerHost
_Check_return_
HRESULT
GroupedDataCollectionView::GetTraceString(_Outptr_result_z_ const WCHAR **pszTraceString)
{
    // TODO: Return a good trace string for the CollectionView
    *pszTraceString = L"";
    return S_OK;
}

_Check_return_
HRESULT
GroupedDataCollectionView::SourceChanged()
{
    // This should never happen
    ASSERT(FALSE);
    return E_UNEXPECTED;
}

_Check_return_
HRESULT
GroupedDataCollectionView::OnGroupItemsChanged(
    _In_ ICollectionViewGroup *pGroup,
    _In_ wfc::IVectorChangedEventArgs *pArgs)
{
    HRESULT hr = S_OK;
    wfc::CollectionChange change;
    UINT nGroupIndex = 0;
    UINT nGroupBaseIndex = 0;
    UINT nGroupItemsIndex = 0;
    BOOLEAN fGroupFound = false;
    ctl::ComPtr<wfc::IObservableVector<IInspectable *>> spGroups;

    IFC(get_CollectionGroups(&spGroups));
    IFC(spGroups.Cast<ReadOnlyObservableTrackerCollection<IInspectable *>>()->IndexOf(pGroup, &nGroupIndex, &fGroupFound));
    IFCEXPECT(fGroupFound);

    IFC(GetBaseIndexOfGroup(nGroupIndex, &nGroupBaseIndex));

    IFC(pArgs->get_CollectionChange(&change));

    switch (change)
    {
    case wfc::CollectionChange_ItemInserted:
        IFC(pArgs->get_Index(&nGroupItemsIndex));
        IFC(NotifyOfItemChange(nGroupBaseIndex + nGroupItemsIndex, change));
        break;

    case wfc::CollectionChange_ItemRemoved:
        IFC(pArgs->get_Index(&nGroupItemsIndex));
        IFC(NotifyOfItemChange(nGroupBaseIndex + nGroupItemsIndex, change));
        break;

    case wfc::CollectionChange_ItemChanged:
        IFC(pArgs->get_Index(&nGroupItemsIndex));
        IFC(NotifyOfItemChange(nGroupBaseIndex + nGroupItemsIndex, change));
        break;

    case wfc::CollectionChange_Reset:
        IFC(NotifyOfItemChange(nGroupIndex, change));
        break;
    }

Cleanup:

    RRETURN(hr);
}


_Check_return_
HRESULT GroupedDataCollectionView::SetSource(
    _In_ wfc::IVector<IInspectable *> *pSource,
    _In_ IPropertyPath *pItemsPath)
{
    HRESULT hr = S_OK;
    ReadOnlyObservableTrackerCollection<IInspectable *> *pGroups = NULL;

    SetPtrValue(m_tpSource, pSource);

    SetPtrValueWithQIOrNull(m_tpObservable, pSource);

    IFC(CreatePropertyPathListener(pItemsPath));

    IFC(ctl::ComObject<ReadOnlyObservableTrackerCollection<IInspectable *>>::CreateInstance(&pGroups));

    SetPtrValue(m_tpCollectionGroups, pGroups);
    IFC(RaisePropertyChanged(STR_LEN_PAIR(L"CollectionGroups")));

    IFC(CalculateGroups());

    if (m_tpObservable)
    {
        IFC(m_epVectorChangedHandler.AttachEventHandler(m_tpObservable.Get(),
            [this](wfc::IObservableVector<IInspectable *> *pSender, wfc::IVectorChangedEventArgs *pArgs)
            {
                HRESULT hr = S_OK;

                auto pegThis = ctl::try_make_autopeg(this);
                if (pegThis)
                {
                    IFC(OnGroupsChange(pArgs));
                }

            Cleanup:
                RRETURN(hr);
            }));
    }

Cleanup:

    ctl::release_interface(pGroups);

    RRETURN(hr);
}

_Check_return_
HRESULT
GroupedDataCollectionView::CreatePropertyPathListener(_In_ IPropertyPath *pItemsPath)
{
    wrl_wrappers::HString strPath;
    LPCWSTR szPath = NULL;
    ctl::ComPtr<PropertyPathListener> spListener;
    PropertyPathParser parser{};

    // If there's no items path then we short cut, this will also mean
    // that the group object is also the source of the items, i.e. it is iterable
    if (pItemsPath == nullptr)
    {
        return S_OK;
    }

    IFC_RETURN(pItemsPath->get_Path(strPath.GetAddressOf()));
    szPath = strPath.GetRawBuffer(NULL /* length, optional */);

    // TODO: Can we percolate the fact that we're called from the parser so we
    // can resolve fully qualified DPs as well?
    IFC_RETURN(parser.SetSource(const_cast<WCHAR*>(szPath), FALSE /* fIsCalledFromParser */));

    IFC_RETURN(ctl::make<PropertyPathListener>(this, &parser, false /* fListenToChanges */, false /* fUseWeakReferenceForSource */, &spListener));
    SetPtrValue(m_tpListener, spListener);

    return S_OK;
}

_Check_return_
HRESULT
GroupedDataCollectionView::CalculateGroups()
{
    HRESULT hr = S_OK;
    UINT nSize = 0;
    IInspectable *pGroup = NULL;
    ICollectionViewGroup *pCVG = NULL;
    ctl::ComPtr<wfc::IObservableVector<IInspectable *>> spGroups;
    ReadOnlyObservableTrackerCollection<IInspectable *> *pGroups = NULL;

    IFC(get_CollectionGroups(&spGroups));
    spGroups.CastTo(&pGroups);

    ClearGroupsOwner();
    IFC(pGroups->InternalClear());

    IFC(m_tpSource->get_Size(&nSize));

    for (UINT i = 0; i < nSize; i++)
    {
        IFC(m_tpSource->GetAt(i, &pGroup));
        IFC(CalculateCollectionViewGroup(pGroup, &pCVG));

        IFC(pGroups->InternalAppend(pCVG));

        ReleaseInterface(pGroup);
        ReleaseInterface(pCVG);
    }

Cleanup:

    ReleaseInterface(pGroup);
    ReleaseInterface(pCVG);

    RRETURN(hr);
}

_Check_return_
HRESULT
GroupedDataCollectionView::CalculateCollectionViewGroup(
    _In_ IInspectable *pGroup,
    _Outptr_ ICollectionViewGroup **ppCVG)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<CollectionViewGroup> spCVG;
    ctl::ComPtr<IInspectable> spItems;

    IFC(ctl::make(&spCVG));

    // If we have a listener then use it to get to the items object
    // otherwise the object itself is the source of the items
    if (m_tpListener)
    {
        IFC(m_tpListener->SetSource(pGroup));
        if (m_tpListener->FullPathExists())
        {
            IFC(m_tpListener->GetValue(&spItems));
        }
    }
    else
    {
        spItems = pGroup;
    }

    // Initialize the group
    IFC(spCVG->SetSource(pGroup, spItems.Get(), this));

    // And return the newly created group to the caller
    *ppCVG = spCVG.Detach();

Cleanup:

    if (m_tpListener)
    {
        // Always clean up the listener so we don't keep a hidden reference to the
        // group object
        IGNOREHR(m_tpListener->SetSource(NULL));
    }

    RRETURN(hr);
}

_Check_return_
HRESULT
GroupedDataCollectionView::CalculateCount(_Out_ UINT *count)
{
    HRESULT hr = S_OK;
    UINT nGroups = 0;
    UINT nGroupSize = 0;
    ICollectionViewGroup *pGroup = NULL;
    ctl::ComPtr<wfc::IObservableVector<IInspectable *>> spGroups;

    // Long path sum the count of each group
    *count = 0;

    IFC(get_CollectionGroups(&spGroups));
    IFC(spGroups.Cast<ReadOnlyObservableTrackerCollection<IInspectable *>>()->get_Size(&nGroups));

    for (UINT i = 0; i < nGroups; i++)
    {
        IFC(GetGroupAt(i, &pGroup));

        IFC(GetGroupSize(pGroup, &nGroupSize));

        *count += nGroupSize;

        ReleaseInterface(pGroup);
    }

Cleanup:

    ReleaseInterface(pGroup);

    RRETURN(hr);
}


_Check_return_
HRESULT
GroupedDataCollectionView::CreateInstance(
    _In_ wfc::IVector<IInspectable *> *pSource,
    _In_ IPropertyPath *pItemsPath,
    _Outptr_ ICollectionView **instance)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<GroupedDataCollectionView> spCV;

    IFC(ctl::make(&spCV));
    IFC(spCV->SetSource(pSource, pItemsPath));

    *instance = spCV.Detach();

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
GroupedDataCollectionView::OnGroupsChange(_In_ wfc::IVectorChangedEventArgs *pArgs)
{
    HRESULT hr = S_OK;
    wfc::CollectionChange change;
    UINT changeIndex = 0;
    IInspectable *pGroup = NULL;
    ICollectionViewGroup *pCVG = NULL;
    ICollectionViewGroup *pOldCVG = NULL;
    ctl::ComPtr<wfc::IObservableVector<IInspectable *>> spGroups;
    ReadOnlyObservableTrackerCollection<IInspectable *> *pGroups = NULL;

    IFC(get_CollectionGroups(&spGroups));
    spGroups.CastTo(&pGroups);

    IFC(pArgs->get_CollectionChange(&change));

    // TODO: Update the items collection as needed
    // This should also take care of updating the base class

    switch (change)
    {
    case wfc::CollectionChange_ItemInserted:
        // A new group has appeared, we calculate a new ICVG and add it
        // to the collection
        IFC(pArgs->get_Index(&changeIndex));
        IFC(m_tpSource->GetAt(changeIndex, &pGroup));
        IFC(CalculateCollectionViewGroup(pGroup, &pCVG));
        IFC(pGroups->InternalInsertAt(changeIndex, pCVG));
        IFC(NotifyOfGroupChange(changeIndex, change, pCVG));
        break;

    case wfc::CollectionChange_ItemRemoved:
        // A group has been deleted, remove it from the group collection
        IFC(pArgs->get_Index(&changeIndex));
        IFC(GetGroupAt(changeIndex, &pCVG));
        IFC(NotifyOfGroupChange(changeIndex, change, pCVG));
        IFC(pGroups->InternalRemoveAt(changeIndex));
        static_cast<CollectionViewGroup*>(pCVG)->ClearOwner();
        break;

    case wfc::CollectionChange_ItemChanged:
        // A group has been changed, this will be notified as a
        // removal of the group and an insertion of the group
        IFC(pArgs->get_Index(&changeIndex));
        IFC(GetGroupAt(changeIndex, &pOldCVG));
        IFC(NotifyOfGroupChange(changeIndex, wfc::CollectionChange_ItemRemoved, pOldCVG));
        static_cast<CollectionViewGroup*>(pOldCVG)->ClearOwner();

        IFC(m_tpSource->GetAt(changeIndex, &pGroup));
        IFC(CalculateCollectionViewGroup(pGroup, &pCVG));
        IFC(pGroups->InternalSetAt(changeIndex, pCVG));
        IFC(NotifyOfGroupChange(changeIndex, wfc::CollectionChange_ItemInserted, pCVG));
        break;

    case wfc::CollectionChange_Reset:
        // The group collections has totally changed, recalculate the entire set of groups
        IFC(CalculateGroups());
        IFC(NotifyOfGroupChange(0, change));
        break;
    }

Cleanup:

    ReleaseInterface(pGroup);
    ReleaseInterface(pCVG);
    ReleaseInterface(pOldCVG);

    RRETURN(hr);
}

_Check_return_
HRESULT
GroupedDataCollectionView::NotifyOfItemChange(
    _In_ UINT nIndex,
    _In_ wfc::CollectionChange change)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<VectorChangedEventArgs> spArgs;
    DXamlCore* pCore = DXamlCore::GetCurrent();

#ifdef GDCV_DBG
    WCHAR szTrace[256];
    IFCEXPECT(swprintf_s(szTrace, 256, L"GDCV::NotifyOfItemChange  CollectionChange %d, index %d", change, nIndex) >= 0);
    Trace(szTrace);
#endif

    switch (change)
    {
    case wfc::CollectionChange_ItemInserted:
        m_nNumberOfItemsInAllGroups++;
        break;
    case wfc::CollectionChange_ItemRemoved:
        m_nNumberOfItemsInAllGroups--;
        break;
    case wfc::CollectionChange_Reset:
        m_nNumberOfItemsInAllGroups = cNumberOfItemsNotCached;    // need to do a slow calc next time we request it
        break;
    }

    IFC(pCore->GetVectorChangedEventArgsFromPool(&spArgs));

    IFC(spArgs->put_CollectionChange(change));
    IFC(spArgs->put_Index(nIndex));

    IFC(ProcessCollectionChange(spArgs.Get()));

    IFC(pCore->ReleaseVectorChangedEventArgsToPool(spArgs.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
GroupedDataCollectionView::NotifyOfGroupChange(
    _In_ UINT nIndex,
    _In_ wfc::CollectionChange change,
    _In_ ICollectionViewGroup *pCVG)
{
    HRESULT hr = S_OK;
    UINT nGroupSize = 0;
    UINT nGroupBaseIndex = 0;
    wfc::CollectionChange changeToReport = wfc::CollectionChange_Reset;

#ifdef GDCV_DBG
    WCHAR szTrace[256];
    IFCEXPECT(swprintf_s(szTrace, 256, L"GDCV::NotifyOfGroupChange  CollectionChange %d, index %d", change, nIndex) >= 0);
    Trace(szTrace);
#endif

    switch (change)
    {
    case wfc::CollectionChange_ItemInserted:
        IFCEXPECT(pCVG);
        IFC(GetGroupSize(pCVG, &nGroupSize));
        IFC(GetBaseIndexOfGroup(nIndex, &nGroupBaseIndex));
        changeToReport = change;
        break;

    case wfc::CollectionChange_ItemRemoved:
        IFCEXPECT(pCVG);
        IFC(GetGroupSize(pCVG, &nGroupSize));
        IFC(GetBaseIndexOfGroup(nIndex, &nGroupBaseIndex));
        changeToReport = change;
        break;

    case wfc::CollectionChange_ItemChanged:
        // This should never be reported
        ASSERT(FALSE);
        goto Cleanup;

    case wfc::CollectionChange_Reset:
        IFC(NotifyOfItemChange(0, change));
        goto Cleanup;
    }

    // Simulate the change for each individual item in the flattened list
    for (int i = nGroupSize - 1; i >= 0; i--)
    {
        IFC(NotifyOfItemChange(nGroupBaseIndex + i, changeToReport));
    }

Cleanup:

    RRETURN(hr);
}


_Check_return_
HRESULT
GroupedDataCollectionView::GetBaseIndexOfGroup(_In_ UINT nGroupIndex, _Out_ UINT *pnGroupBaseIndex)
{
    HRESULT hr = S_OK;
    UINT nGroups = 0;
    UINT nCurrentIndex = 0;
    UINT nGroupSize = 0;
    IInspectable *pGroupInsp = NULL;
    ICollectionViewGroup *pGroup = NULL;
    wfc::IObservableVector<IInspectable *> *pObservableItems = NULL;
    wfc::IVector<IInspectable *> *pItems = NULL;
    ctl::ComPtr<wfc::IObservableVector<IInspectable *>> spGroups;
    ReadOnlyObservableTrackerCollection<IInspectable *> *pGroups = NULL;

    IFC(get_CollectionGroups(&spGroups));
    spGroups.CastTo(&pGroups);

    IFC(pGroups->get_Size(&nGroups));
    IFCEXPECT(nGroupIndex < nGroups);

    for (UINT i = 0; i < nGroupIndex; i++)
    {
        IFC(pGroups->GetAt(i, &pGroupInsp));
        IFC(ctl::do_query_interface(pGroup, pGroupInsp));

        IFC(pGroup->get_GroupItems(&pObservableItems));
        IFC(ctl::do_query_interface(pItems, pObservableItems));

        IFC(pItems->get_Size(&nGroupSize));

        // Advance to the next group
        nCurrentIndex += nGroupSize;

        ReleaseInterface(pGroupInsp);
        ReleaseInterface(pGroup);
        ReleaseInterface(pObservableItems);
        ReleaseInterface(pItems);
    }

    *pnGroupBaseIndex = nCurrentIndex;

Cleanup:

    ReleaseInterface(pGroupInsp);
    ReleaseInterface(pGroup);
    ReleaseInterface(pObservableItems);
    ReleaseInterface(pItems);

    RRETURN(hr);
}

_Check_return_
HRESULT
GroupedDataCollectionView::GetGroupItems(
    _In_ ICollectionViewGroup *pGroup,
    _Outptr_ wfc::IVector<IInspectable *> **ppItems)
{
    HRESULT hr = S_OK;
    wfc::IObservableVector<IInspectable *> *pObservableItems = NULL;

    IFC(pGroup->get_GroupItems(&pObservableItems));
    IFC(ctl::do_query_interface(*ppItems, pObservableItems));

Cleanup:

    ReleaseInterface(pObservableItems);

    RRETURN(hr);
}

_Check_return_
HRESULT
GroupedDataCollectionView::GetGroupItem(
    _In_ ICollectionViewGroup *pGroup,
    _In_ UINT nIndex,
    _Outptr_ IInspectable **ppItem)
{
    HRESULT hr = S_OK;
    wfc::IVector<IInspectable *> *pItems = NULL;

    IFC(GetGroupItems(pGroup, &pItems));

    IFC(pItems->GetAt(nIndex, ppItem));

Cleanup:

    ReleaseInterface(pItems);

    RRETURN(hr);
}

_Check_return_
HRESULT
GroupedDataCollectionView::GetGroupAt(
    UINT nIndex,
    _Outptr_ ICollectionViewGroup **ppGroup)
{
    HRESULT hr = S_OK;
    IInspectable *pGroupInsp = NULL;
    ctl::ComPtr<wfc::IObservableVector<IInspectable *>> spGroups;

    IFC(get_CollectionGroups(&spGroups));
    IFC(spGroups.Cast<ReadOnlyObservableTrackerCollection<IInspectable *>>()->GetAt(nIndex, &pGroupInsp));
    IFC(ctl::do_query_interface(*ppGroup, pGroupInsp));

Cleanup:

    ReleaseInterface(pGroupInsp);

    RRETURN(hr);
}

_Check_return_
HRESULT
GroupedDataCollectionView::GetGroupSize(
    _In_ ICollectionViewGroup *pGroup,
    _Out_ UINT *pnSize)
{
    HRESULT hr = S_OK;
    wfc::IVector<IInspectable *> *pItems = NULL;

    IFC(GetGroupItems(pGroup, &pItems));
    IFC(pItems->get_Size(pnSize));

Cleanup:

    ReleaseInterface(pItems);

    RRETURN(hr);
}

void GroupedDataCollectionView::ClearGroupsOwner()
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    UINT nGroups = 0;
    ICollectionViewGroup *pCVG = NULL;
    CollectionViewGroup *pGroup = NULL;
    ctl::ComPtr<wfc::IObservableVector<IInspectable *>> spGroups;

    IFC(get_CollectionGroups(&spGroups));

    IFC(spGroups.Cast<ReadOnlyObservableTrackerCollection<IInspectable *>>()->get_Size(&nGroups));

    for (UINT i = 0; i < nGroups; i++)
    {
        IFC(GetGroupAt(i, &pCVG));
        pGroup = static_cast<CollectionViewGroup *>(pCVG);

        pGroup->ClearOwner();

        ReleaseInterface(pCVG);
    }

Cleanup:

    ReleaseInterface(pCVG);

    return;
}



