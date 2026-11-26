// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Experimental WPF-style shaping (sort, filter, group): the engine of the VectorCollectionView a
//      CollectionViewSource creates over a vector (WPF: ListCollectionView). See
//      docs\design-notes\CollectionViewSourceShaping-POC.md.

#include "precomp.h"
#include "VectorCollectionView_ShapingInternal.h"
#include <XamlTraceLogging.h>

using namespace DirectUI;

// A local diagnostic event, not telemetry: no Microsoft telemetry keyword or privacy tag, so it is not uploaded and
// needs no data review; a trace that enables the provider (WPR, xperf) records it. Payload: HRESULTs only.
void ShapingEngine::TraceContainedFailure(HRESULT hr, HRESULT capturedHr, bool hasErrorInfo)
{
    TraceLoggingWrite(
        g_hTraceProvider,
        "CollectionViewContainedFailure",
        TraceLoggingDescription("A CollectionView contained a failure of app code while it applied a change."),
        TraceLoggingLevel(WINEVENT_LEVEL_WARNING),
        TraceLoggingHResult(hr, "HResult"),
        TraceLoggingHResult(capturedHr, "CapturedHResult"),
        TraceLoggingBoolean(hasErrorInfo, "HasErrorInfo"));
}

HRESULT VectorCollectionView::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(xaml_data::ICollectionViewShaping)) && m_isShapingEnabled)
    {
        *ppObject = static_cast<xaml_data::ICollectionViewShaping*>(this);
        AddRefOuter();
        return S_OK;
    }

    if (InlineIsEqualGUID(iid, __uuidof(xaml_data::ICollectionViewLiveShaping)) && m_isShapingEnabled)
    {
        *ppObject = static_cast<xaml_data::ICollectionViewLiveShaping*>(this);
        AddRefOuter();
        return S_OK;
    }

    return VectorCollectionViewGenerated::QueryInterfaceImpl(iid, ppObject);
}

_Check_return_ HRESULT VectorCollectionView::EnableShaping(_In_ CollectionViewSource* pHost)
{
    m_isShapingEnabled = true;
    SetPtrValue(m_tpSourceList, m_tpSource.Get());
    IFC_RETURN(ctl::AsWeak(pHost, &m_hostRef));
    SetFilterEvents(pHost->GetFilterEventsIfHandled());
    return S_OK;
}

void VectorCollectionView::SetFilterEvents(_In_opt_ IFilterEventSource* pEvents)
{
    // The reference tracker walk reads it; the previous source is released after the lock.
    ctl::ComPtr<IFilterEventSource> spPrevious(pEvents);
    {
        AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());
        m_spFilterEvents.Swap(spPrevious);
    }
}

_Check_return_ HRESULT VectorCollectionView::ApplyShapingRules(
    _In_ wfc::IVector<xaml_data::SortDescription*>* pSortDescriptions,
    _In_ wfc::IVector<xaml_data::GroupDescription*>* pGroupDescriptions,
    _In_opt_ xaml_data::ICollectionViewFilter* pFilter,
    _In_opt_ xaml_data::ICollectionViewComparer* pCustomSort,
    _In_opt_ HSTRING culture)
{
    SetPtrValue(m_tpSortDescriptions, pSortDescriptions);
    SetPtrValue(m_tpGroupDescriptions, pGroupDescriptions);
    SetPtrValue(m_tpFilter, pFilter);
    SetPtrValue(m_tpCustomSort, pCustomSort);
    IFC_RETURN(m_culture.Set(culture));
    IFC_RETURN(OnShapingRulesChanged());
    return S_OK;
}

void VectorCollectionView::SetSourceCollection(_In_opt_ IInspectable* pSource)
{
    SetPtrValue(m_tpSourceCollection, pSource);
}

_Check_return_ HRESULT VectorCollectionView::ValidateCulture(_In_opt_ HSTRING value)
{
    UINT32 length = 0;
    LPCWSTR szValue = WindowsGetStringRawBuffer(value, &length);
    IFCEXPECTRC_RETURN(length == 0 || (length < LOCALE_NAME_MAX_LENGTH && IsValidLocaleName(szValue)), E_INVALIDARG);
    return S_OK;
}

_Check_return_ HRESULT VectorCollectionView::OnGroupDescriptionChanged(_In_ GroupDescription* pDescription)
{
    UINT index = 0;
    BOOLEAN isFound = FALSE;

    if (!m_tpGroupDescriptions || !m_isGrouping)
    {
        return S_OK;
    }

    IFC_RETURN(m_tpGroupDescriptions->IndexOf(ctl::interface_cast<xaml_data::IGroupDescription>(pDescription), &index, &isFound));
    if (isFound)
    {
        IFC_RETURN(OnShapingRulesChanged());
    }

    return S_OK;
}

// The descriptions are copied into collections of the view's own; the host keeps the ones it shared. The
// copy changes no rule, so it recomputes nothing.
_Check_return_ HRESULT VectorCollectionView::DetachFromHost()
{
    ctl::ComPtr<wfc::IVector<xaml_data::SortDescription*>> spSortDescriptions;
    ctl::ComPtr<wfc::IVector<xaml_data::GroupDescription*>> spGroupDescriptions;
    const bool wasRefreshPending = m_isRefreshPending;
    HRESULT hr = S_OK;

    m_hostRef = ctl::WeakRefPtr();
    SetFilterEvents(nullptr);
    if (!m_tpSortDescriptions || !m_tpGroupDescriptions)
    {
        return S_OK;
    }

    IFC_RETURN(CreateShapingDescriptions(spSortDescriptions.ReleaseAndGetAddressOf(), spGroupDescriptions.ReleaseAndGetAddressOf()));

    ++m_deferRefreshLevel;
    hr = [&]() -> HRESULT
    {
        UINT count = 0;
        IFC_RETURN(m_tpSortDescriptions->get_Size(&count));
        for (UINT i = 0; i < count; ++i)
        {
            ctl::ComPtr<xaml_data::ISortDescription> spDescription;
            IFC_RETURN(m_tpSortDescriptions->GetAt(i, spDescription.ReleaseAndGetAddressOf()));
            IFC_RETURN(spSortDescriptions->Append(spDescription.Get()));
        }

        IFC_RETURN(m_tpGroupDescriptions->get_Size(&count));
        for (UINT i = 0; i < count; ++i)
        {
            ctl::ComPtr<xaml_data::IGroupDescription> spDescription;
            IFC_RETURN(m_tpGroupDescriptions->GetAt(i, spDescription.ReleaseAndGetAddressOf()));
            IFC_RETURN(spGroupDescriptions->Append(spDescription.Get()));
        }
        return S_OK;
    }();
    --m_deferRefreshLevel;
    m_isRefreshPending = wasRefreshPending;
    IFC_RETURN(hr);

    SetPtrValue(m_tpSortDescriptions, spSortDescriptions.Get());
    SetPtrValue(m_tpGroupDescriptions, spGroupDescriptions.Get());
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::get_SortDescriptions(_Outptr_ wfc::IObservableVector<xaml_data::SortDescription*>** ppValue)
{
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC_RETURN(CheckThread());
    IFCEXPECT_RETURN(m_tpSortDescriptions);
    IFC_RETURN(m_tpSortDescriptions.CopyTo(ppValue));
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::get_GroupDescriptions(_Outptr_ wfc::IObservableVector<xaml_data::GroupDescription*>** ppValue)
{
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC_RETURN(CheckThread());
    IFCEXPECT_RETURN(m_tpGroupDescriptions);
    IFC_RETURN(m_tpGroupDescriptions.CopyTo(ppValue));
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::get_Filter(_Outptr_result_maybenull_ xaml_data::ICollectionViewFilter** ppValue)
{
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC_RETURN(CheckThread());
    IFC_RETURN(m_tpFilter.CopyTo(ppValue));
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::put_Filter(_In_opt_ xaml_data::ICollectionViewFilter* pValue)
{
    IFC_RETURN(CheckThread());
    SetPtrValue(m_tpFilter, pValue);
    IFC_RETURN(OnShapingRulesChanged());
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::get_CustomSort(_Outptr_result_maybenull_ xaml_data::ICollectionViewComparer** ppValue)
{
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC_RETURN(CheckThread());
    IFC_RETURN(m_tpCustomSort.CopyTo(ppValue));
    return S_OK;
}

// As in WPF, a CustomSort replaces the SortDescriptions, and adding a SortDescription drops the CustomSort.
IFACEMETHODIMP VectorCollectionView::put_CustomSort(_In_opt_ xaml_data::ICollectionViewComparer* pValue)
{
    UINT sortCount = 0;

    IFC_RETURN(CheckThread());
    SetPtrValue(m_tpCustomSort, pValue);

    if (pValue && m_tpSortDescriptions)
    {
        IFC_RETURN(m_tpSortDescriptions->get_Size(&sortCount));
    }

    if (sortCount > 0)
    {
        m_isApplyingCustomSort = true;
        HRESULT hr = m_tpSortDescriptions->Clear();
        m_isApplyingCustomSort = false;
        IFC_RETURN(hr);
    }

    IFC_RETURN(OnShapingRulesChanged());
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::get_SourceCollection(_Outptr_result_maybenull_ IInspectable** ppValue)
{
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC_RETURN(CheckThread());
    IFC_RETURN(m_tpSourceCollection.CopyTo(ppValue));
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::get_Culture(_Out_ HSTRING* pValue)
{
    ARG_VALIDRETURNPOINTER(pValue);
    IFC_RETURN(CheckThread());
    IFC_RETURN(m_culture.CopyTo(pValue));
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::put_Culture(_In_opt_ HSTRING value)
{
    IFC_RETURN(CheckThread());
    IFC_RETURN(ValidateCulture(value));
    IFC_RETURN(m_culture.Set(value));
    IFC_RETURN(OnShapingRulesChanged());
    return S_OK;
}

// WPF: ListCollectionView.CanSort / CanFilter / CanGroup, all true.
IFACEMETHODIMP VectorCollectionView::get_CanSort(_Out_ BOOLEAN* pValue)
{
    ARG_VALIDRETURNPOINTER(pValue);
    *pValue = TRUE;
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::get_CanFilter(_Out_ BOOLEAN* pValue)
{
    ARG_VALIDRETURNPOINTER(pValue);
    *pValue = TRUE;
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::get_CanGroup(_Out_ BOOLEAN* pValue)
{
    ARG_VALIDRETURNPOINTER(pValue);
    *pValue = TRUE;
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::Refresh()
{
    IFC_RETURN(CheckThread());
    ResetIncrementalLoadState();
    IFC_RETURN(RefreshOrDefer());
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::DeferRefresh(_Outptr_ wf::IClosable** ppReturnValue)
{
    Microsoft::WRL::ComPtr<CollectionViewRefreshDeferral<VectorCollectionView>> spDeferral;

    ARG_VALIDRETURNPOINTER(ppReturnValue);
    *ppReturnValue = nullptr;
    IFC_RETURN(CheckThread());

    IFC_RETURN(wrl::MakeAndInitialize<CollectionViewRefreshDeferral<VectorCollectionView>>(&spDeferral, this));
    ++m_deferRefreshLevel;
    *ppReturnValue = spDeferral.Detach();
    return S_OK;
}

_Check_return_ HRESULT VectorCollectionView::EndDeferRefresh()
{
    ASSERT(m_deferRefreshLevel > 0);
    if (m_deferRefreshLevel == 0 || --m_deferRefreshLevel > 0)
    {
        return S_OK;
    }

    if (!m_isRefreshPending)
    {
        // Live changes made while deferred are restored now, incrementally.
        IFC_RETURN(RestoreLiveShaping());
        return S_OK;
    }

    m_isRefreshPending = false;
    IFC_RETURN(OnShapingRulesChanged());
    return S_OK;
}

_Check_return_ HRESULT VectorCollectionView::OnSortDescriptionsChanged()
{
    if (m_isApplyingCustomSort)
    {
        return S_OK;
    }

    if (m_tpCustomSort)
    {
        UINT sortCount = 0;
        IFC_RETURN(m_tpSortDescriptions->get_Size(&sortCount));
        if (sortCount > 0)
        {
            m_tpCustomSort.Clear();
        }
    }

    IFC_RETURN(OnShapingRulesChanged());
    return S_OK;
}

bool VectorCollectionView::PresentsSourceOrder()
{
    bool hasRules = true;
    UINT outputSize = 0;
    UINT sourceSize = 0;
    // The output follows the source only while the view listens to it with valid source indices.
    return m_tpInternalList && m_tpSourceList && m_tpSourceListObservable && m_epSourceListChangedHandler &&
        m_areSourceIndicesValid && !m_isGrouping &&
        m_deferRefreshLevel == 0 && m_shapeBusyDepth == 0 && !m_isRefreshPending && !m_isRecomputePending &&
        SUCCEEDED(HasShapingRules(&hasRules)) && !hasRules &&
        SUCCEEDED(m_tpInternalList->get_Size(&outputSize)) && SUCCEEDED(m_tpSourceList->get_Size(&sourceSize)) &&
        outputSize == sourceSize;
}

_Check_return_ HRESULT VectorCollectionView::HasShapingRules(_Out_ bool* pHasRules)
{
    UINT sortCount = 0;
    UINT groupCount = 0;

    if (m_tpSortDescriptions)
    {
        IFC_RETURN(m_tpSortDescriptions->get_Size(&sortCount));
    }

    if (m_tpGroupDescriptions)
    {
        IFC_RETURN(m_tpGroupDescriptions->get_Size(&groupCount));
    }

    *pHasRules = sortCount > 0 || groupCount > 0 || m_tpFilter || m_tpCustomSort || m_spFilterEvents;
    return S_OK;
}

// A rule changed (WPF: RefreshOrDefer after a SortDescriptions, GroupDescriptions or Filter change).
_Check_return_ HRESULT VectorCollectionView::OnShapingRulesChanged()
{
    if (!m_isShapingEnabled)
    {
        return S_OK;
    }

    // Only a rule change starts incremental loading over, not a source change RunPendingRecompute replays.
    ResetIncrementalLoadState();
    IFC_RETURN(ApplyShapingRules());
    return S_OK;
}

_Check_return_ HRESULT VectorCollectionView::ApplyShapingRules()
{
    UINT groupCount = 0;
    bool hasRules = false;

    if (m_shapeBusyDepth > 0)
    {
        m_isRecomputePending = true;
        return S_OK;
    }

    if (m_deferRefreshLevel > 0)
    {
        m_isRefreshPending = true;
        return S_OK;
    }

    if (m_tpGroupDescriptions)
    {
        IFC_RETURN(m_tpGroupDescriptions->get_Size(&groupCount));
    }
    const bool isGrouping = (groupCount > 0);

    // Reshaped in place, so a consumer holding the view keeps a live view.
    if (m_tpInternalList && isGrouping == m_isGrouping)
    {
        IFC_RETURN(RefreshOrDefer());
        return S_OK;
    }

    // An unshaped view stays untouched until a rule is set.
    IFC_RETURN(HasShapingRules(&hasRules));
    if (!m_tpInternalList && !hasRules)
    {
        return S_OK;
    }

    IFC_RETURN(StartShaping(isGrouping));
    IFC_RETURN(RunPendingRecompute());
    return S_OK;
}

_Check_return_ HRESULT VectorCollectionView::RunPendingRecompute()
{
    // A handler that changes the view on every change of it would never finish (in WPF it overflows the stack).
    constexpr UINT c_maxPendingRecomputes = 8;

    if (!m_isRecomputePending || m_shapeBusyDepth > 0 || m_isRunningPendingRecompute)
    {
        return S_OK;
    }

    m_isRunningPendingRecompute = true;
    auto done = wil::scope_exit([this]() noexcept { m_isRunningPendingRecompute = false; });
    for (UINT pass = 0; m_isRecomputePending; ++pass)
    {
        m_isRecomputePending = false;
        if (pass == c_maxPendingRecomputes)
        {
            // Recomputed on the next source change or Refresh.
            static constexpr WCHAR c_message[] = L"A handler of the view's changes changed its rules or source on each recompute, so the view stopped recomputing.";
            m_areSourceIndicesValid = false;
            IFC_RETURN(ErrorHelper::OriginateError(E_ILLEGAL_METHOD_CALL, ARRAYSIZE(c_message) - 1, c_message));
        }
        IFC_RETURN(ApplyShapingRules());
    }
    return S_OK;
}

// Computes the output into the view's flat or group list and presents it, announced as one Reset; currency
// follows the item. As in WPF, starting or stopping grouping keeps the view and its CollectionGroups object:
// CollectionGroups turns non-null or null, PropertyChanged("CollectionGroups") is raised, then the Reset.
_Check_return_ HRESULT VectorCollectionView::StartShaping(bool isGrouping)
{
    ctl::ComPtr<wfc::IVector<IInspectable*>> spPreviousOutput = m_tpInternalList.Get();
    const bool wasGrouping = m_isGrouping;
    HRESULT hr = S_OK;

    if (isGrouping && !m_tpGroupList)
    {
        ctl::ComPtr<InternalList> spGroupList;
        ctl::ComPtr<xaml_data::ICollectionView> spGrouped;
        IFC_RETURN(ctl::ComObject<InternalList>::CreateInstance(spGroupList.ReleaseAndGetAddressOf()));
        IFC_RETURN(GroupedDataCollectionView::CreateInstance(spGroupList.Get(), nullptr /* pItemsPath */, spGrouped.ReleaseAndGetAddressOf()));
        static_cast<GroupedDataCollectionView*>(spGrouped.Get())->SetRaisesSingleGroupsReset();
        SetPtrValue(m_tpGroupList, static_cast<wfc::IVector<IInspectable*>*>(spGroupList.Get()));
        SetPtrValue(m_tpGroupedPresentation, spGrouped.Get());
    }
    else if (!isGrouping && !m_tpFlatList)
    {
        ctl::ComPtr<InternalList> spFlatList;
        IFC_RETURN(ctl::ComObject<InternalList>::CreateInstance(spFlatList.ReleaseAndGetAddressOf()));
        SetPtrValue(m_tpFlatList, static_cast<wfc::IVector<IInspectable*>*>(spFlatList.Get()));
    }

    m_isGrouping = isGrouping;
    SetPtrValue(m_tpInternalList, isGrouping ? m_tpGroupList.Get() : m_tpFlatList.Get());

    // Followed before the first compute, so a change the app makes while it runs is not missed.
    if (!m_epSourceListChangedHandler)
    {
        auto spObservable = m_tpSourceList.AsOrNull<wfc::IObservableVector<IInspectable*>>();
        if (spObservable)
        {
            SetPtrValue(m_tpSourceListObservable, spObservable.Get());
            IFC_RETURN(m_epSourceListChangedHandler.AttachEventHandler(m_tpSourceListObservable.Get(),
                [this](wfc::IObservableVector<IInspectable*>* pSender, wfc::IVectorChangedEventArgs* pArgs)
                {
                    auto pegThis = ctl::try_make_autopeg(this);
                    if (pegThis)
                    {
                        // The source's other listeners must still hear the change, so app code that failed in the
                        // view is reported rather than returned to the source; the view stays consistent.
                        ContainedFailureReport report;
                        const HRESULT hr = ProcessCollectionChanged(pArgs);
                        if (IsUncontainedFailure(hr))
                        {
                            return hr;
                        }
                        if (FAILED(hr))
                        {
                            report.Report(hr);
                        }
                    }
                    return S_OK;
                }));
        }
    }

    m_isRecomputePending = false;
    RaiseFailureScope raiseFailure;
    CurrencyScope currency;
    IFC_RETURN(BeginRefreshCurrency(&currency, true /* isChangingRaisedFirst */));
    BusyScope busy(m_shapeBusyDepth);
    hr = RefreshOverride();
    if (FAILED(hr))
    {
        m_isGrouping = wasGrouping;
        SetPtrValue(m_tpInternalList, spPreviousOutput.Get());
        m_areSourceIndicesValid = false;
        IFC_RETURN(hr);
    }

    // The output is read-only, so index-based writes through the view fail instead of reordering the source.
    hr = [&]() -> HRESULT
    {
        if (isGrouping)
        {
            ctl::ComPtr<wfc::IVector<IInspectable*>> spGroupedItems;
            IFC_RETURN(m_tpGroupedPresentation.As(&spGroupedItems));
            IFC_RETURN(RetargetSource(spGroupedItems.Get(), !wasGrouping));
        }
        else
        {
            IFC_RETURN(RetargetSource(m_tpFlatList.Get(), wasGrouping));
        }

        // The output no longer presented lets go of its items.
        if (spPreviousOutput && spPreviousOutput.Get() != m_tpInternalList.Get())
        {
            IFC_RETURN(static_cast<InternalList*>(spPreviousOutput.Get())->ReplaceAll({}));
        }
        return S_OK;
    }();
    IFC_RETURN(hr);
    IFC_RETURN(EndRefreshCurrency(&currency));
    IFC_RETURN(raiseFailure.Result());
    return S_OK;
}

_Check_return_ HRESULT VectorCollectionView::get_CollectionGroupsImpl(_Out_ wfc::IObservableVector<IInspectable *> **value)
{
    if (m_isGrouping && m_tpGroupedPresentation)
    {
        IFC_RETURN(m_tpGroupedPresentation->get_CollectionGroups(value));
        return S_OK;
    }

    IFC_RETURN(CollectionView::get_CollectionGroupsImpl(value));
    return S_OK;
}

// Full recompute (WPF: ListCollectionView.RefreshOverride): filter, then stable multi-key sort, then group by
// first appearance. isSkippedWhenUnchanged: a flat output equal to the current one raises nothing.
_Check_return_ HRESULT VectorCollectionView::RefreshOverride(bool isSkippedWhenUnchanged)
{
    struct Entry
    {
        ctl::ComPtr<IInspectable> item;
        std::vector<ctl::ComPtr<IInspectable>> groupNames;
        bool hasCollectionName = false;
        UINT sourceIndex = 0;
        UINT slot = 0;
    };

    auto spSort = std::make_shared<ShapingSortState>();
    ctl::ComPtr<CollectionViewGroupRoot> spRoot;
    ShapingFilterEvents filterEvents;
    std::vector<Entry> entries;
    std::vector<ctl::ComPtr<IInspectable>> outputItems;
    std::vector<UINT> outputSourceIndices;
    const UINT sourceChangeCount = m_sourceListChangeCount;
    UINT size = 0;

    IFC_RETURN(spSort->comparer.Initialize(m_tpSortDescriptions.Get(), m_tpCustomSort.Get(), m_culture.Get(), spSort->references));
    spSort->keys.Reset(spSort->comparer.FieldCount());
    IFC_RETURN(m_tpSourceList->get_Size(&size));

    if (m_isGrouping)
    {
        IFC_RETURN(ctl::ComObject<CollectionViewGroupRoot>::CreateInstance(spRoot.ReleaseAndGetAddressOf()));
        IFC_RETURN(spRoot->InitializeRoot(m_tpGroupDescriptions.Get(), this, m_culture.Get(), size));
    }

    IFC_RETURN(GetFilterEvents(&filterEvents));
    entries.reserve(size);

    for (UINT i = 0; i < size; ++i)
    {
        Entry entry;
        SortKeys keys;
        bool isAccepted = true;
        entry.sourceIndex = i;
        IFC_RETURN(m_tpSourceList->GetAt(i, entry.item.ReleaseAndGetAddressOf()));

        IFC_RETURN(PassesFilter(entry.item.Get(), filterEvents, &isAccepted));
        if (!isAccepted)
        {
            continue;
        }

        IFC_RETURN(spSort->comparer.EvaluateKeys(entry.item.Get(), &keys));
        entry.slot = spSort->keys.Allocate(std::move(keys));

        if (spRoot)
        {
            IFC_RETURN(spRoot->GetGroupNames(entry.item.Get(), &entry.groupNames, &entry.hasCollectionName));
        }

        entries.push_back(std::move(entry));
    }

    if (!spSort->comparer.IsEmpty())
    {
        // A CustomSort that fails, or that the sort finds inconsistent, ends the sort and the recompute; the previous
        // output stays. The SortDescriptions compare cached keys with a total order (CompareSortKeys, then the
        // unique source index), so they keep std::stable_sort; app code can make the order inconsistent only through
        // a CustomSort, which takes the bounds-safe sort.
        const auto& sort = *spSort;
        if (sort.comparer.HasCustomSort())
        {
            IFC_RETURN(SafeStableSort(entries, [&sort](const Entry& left, const Entry& right, bool* pIsBefore) -> HRESULT
            {
                int result = 0;
                IFC_RETURN(sort.comparer.Compare(left.item.Get(), nullptr, left.sourceIndex, right.item.Get(), nullptr, right.sourceIndex, &result));
                *pIsBefore = result < 0;
                return S_OK;
            }));
        }
        else
        {
            std::stable_sort(entries.begin(), entries.end(), [&sort](const Entry& left, const Entry& right)
            {
                int result = 0;
                VERIFYHR(sort.comparer.Compare(left.item.Get(), sort.keys.At(left.slot), left.sourceIndex, right.item.Get(), sort.keys.At(right.slot), right.sourceIndex, &result));
                return result < 0;
            });
        }
    }

    if (!spRoot)
    {
        outputItems.reserve(entries.size());
        outputSourceIndices.reserve(entries.size());
        spSort->flatSlots.reserve(entries.size());
        for (auto& entry : entries)
        {
            outputItems.push_back(std::move(entry.item));
            outputSourceIndices.push_back(entry.sourceIndex);
            spSort->flatSlots.push_back(entry.slot);
        }
    }
    else
    {
        for (const auto& entry : entries)
        {
            IFC_RETURN(spRoot->AddLoading(entry.item.Get(), entry.groupNames, entry.sourceIndex, entry.slot, entry.hasCollectionName));
        }
        IFC_RETURN(spRoot->FinishLoading());

        outputItems.reserve(spRoot->Subgroups().size());
        for (const auto& spGroup : spRoot->Subgroups())
        {
            outputItems.push_back(ctl::as_iinspectable(spGroup.Get()));
        }
        spRoot->PresentSubgroups(static_cast<InternalList*>(m_tpInternalList.Get()));
    }

    IFC_RETURN(RebuildLiveShapingItems());

    bool isUnchanged = isSkippedWhenUnchanged && !spRoot && !m_spGroupRoot && m_areSourceIndicesValid && outputSourceIndices == m_sourceIndices;
    for (UINT i = 0; isUnchanged && i < outputItems.size(); ++i)
    {
        ctl::ComPtr<IInspectable> spPresented;
        IFC_RETURN(m_tpInternalList->GetAt(i, spPresented.ReleaseAndGetAddressOf()));
        isUnchanged = spPresented.Get() == outputItems[i].Get();
    }

    // Recorded before ReplaceAll raises, so a handler that changes the source sees consistent state. The tracker walk
    // reads the sort state and the group root under the lock; the previous ones are released outside it.
    m_sourceIndices = std::move(outputSourceIndices);
    ctl::ComPtr<xaml_data::ICollectionViewGroup> spPreviousRoot = spRoot ? static_cast<xaml_data::ICollectionViewGroup*>(spRoot.Get()) : nullptr;
    {
        AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());
        std::swap(m_spSortState, spSort);
        std::swap(m_spGroupRoot, spPreviousRoot);
    }
    m_sourceListSize = size;
    m_areSourceIndicesValid = (sourceChangeCount == m_sourceListChangeCount);
    if (!isUnchanged)
    {
        IFC_RETURN(static_cast<InternalList*>(m_tpInternalList.Get())->ReplaceAll(outputItems));
    }
    return S_OK;
}

// The CollectionViewSource whose Filter event handlers filter the view, if any. As in WPF, its event replaces
// the Filter delegate while it has handlers.
_Check_return_ HRESULT VectorCollectionView::GetFilterEvents(_Out_ ShapingFilterEvents* pEvents)
{
    pEvents->spEvents = m_spFilterEvents;
    pEvents->spSender.Reset();
    if (pEvents->spEvents)
    {
        ctl::ComPtr<xaml_data::ICollectionViewSource> spHost;
        IFC_RETURN(m_hostRef.As(&spHost));
        pEvents->spSender = ctl::as_iinspectable(spHost.Get());
    }
    return S_OK;
}

// Whether the view shows the item (WPF: CollectionView.PassesFilter).
_Check_return_ HRESULT VectorCollectionView::PassesFilter(_In_opt_ IInspectable* pItem, const ShapingFilterEvents& events, _Out_ bool* pIsAccepted)
{
    *pIsAccepted = true;
    if (events.spEvents)
    {
        IFC_RETURN(CollectionViewSource::RaiseFilterEvent(events.spEvents.Get(), events.spSender.Get(), pItem, pIsAccepted));
    }
    else if (m_tpFilter)
    {
        boolean accepted = true;
        IFC_RETURN(m_tpFilter->Invoke(pItem, &accepted));
        *pIsAccepted = !!accepted;
    }
    return S_OK;
}

_Check_return_ HRESULT VectorCollectionView::RefreshOrDefer(bool isSkippedWhenUnchanged)
{
    HRESULT hr = S_OK;

    if (!m_tpInternalList)
    {
        return S_OK;
    }

    // Asked for while app code runs or the view raises its changes: done once that unwinds.
    if (m_shapeBusyDepth > 0)
    {
        m_isRecomputePending = true;
        return S_OK;
    }

    if (m_deferRefreshLevel > 0)
    {
        m_isRefreshPending = true;
        return S_OK;
    }

    // A recompute that app code fails keeps the previous output, which no longer follows the source or the rules:
    // the next change recomputes.
    RaiseFailureScope raiseFailure;
    m_isRecomputePending = false;

    // WPF (ListCollectionView.RefreshOverride): currency stays before the first or after the last item, else
    // follows the item, else goes to the first (VectorCollectionView_Currency.cpp).
    CurrencyScope currency;
    IFC_RETURN(BeginRefreshCurrency(&currency, !isSkippedWhenUnchanged));
    {
        BusyScope busy(m_shapeBusyDepth);
        hr = RefreshOverride(isSkippedWhenUnchanged);
    }
    if (FAILED(hr))
    {
        m_areSourceIndicesValid = false;
        IFC_RETURN(hr);
    }

    IFC_RETURN(EndRefreshCurrency(&currency));

    IFC_RETURN(RunPendingRecompute());
    IFC_RETURN(raiseFailure.Result());
    return S_OK;
}

void VectorCollectionView::DetachSourceList()
{
    if (m_epSourceListChangedHandler)
    {
        auto spSource = m_tpSourceListObservable.GetSafeReference();
        if (spSource)
        {
            VERIFYHR(m_epSourceListChangedHandler.DetachEventHandler(spSource.Get()));
        }
        else
        {
            m_epSourceListChangedHandler = ctl::EventPtr<VectorChangedEventCallback>();
        }
    }

    m_tpSourceListObservable.Clear();
    m_areSourceIndicesValid = false;
    ClearLiveShapingItems();
}

_Check_return_ HRESULT DirectUI::ShapingNamesMatch(xaml_data::CollectionViewStringComparison comparison, _In_opt_ IInspectable* pGroupName, _In_opt_ IInspectable* pItemName, _Out_ bool* pMatch)
{
    IFC_RETURN(GroupNamesMatch(comparison, pGroupName, pItemName, pMatch));
    return S_OK;
}

_Check_return_ HRESULT DirectUI::ShapingGroupNameFromProperty(
    _In_opt_ HSTRING propertyName,
    _In_opt_ xaml_data::IValueConverter* pConverter,
    _In_opt_ IInspectable* pItem,
    INT level,
    _In_opt_ HSTRING culture,
    _Outptr_result_maybenull_ IInspectable** ppName)
{
    ShapingKeySelector selector;
    ctl::ComPtr<IInspectable> spName;

    *ppName = nullptr;
    IFC_RETURN(CreateKeySelector(propertyName, &selector));
    IFC_RETURN(EvaluateKey(selector, pItem, spName.ReleaseAndGetAddressOf()));
    IFC_RETURN(ConvertGroupName(pConverter, level, culture, spName));
    *ppName = spName.Detach();
    return S_OK;
}
