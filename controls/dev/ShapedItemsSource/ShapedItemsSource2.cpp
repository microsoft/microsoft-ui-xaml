// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ShapedItemsSource2.h"
#include "ShapedGroup.h"
#include "GroupedEntry.h"
#include "ShapingRevoke.h"

#include <algorithm>
#include <numeric>

namespace
{
    // Beyond this many inserts and removes, one Reset is cheaper for the consumer than the edits.
    constexpr size_t c_maxIncrementalEdits = 256;

    [[noreturn]] void ThrowCallerContract(winrt::hstring const& message)
    {
        throw ShapingHelpers::CallerError(winrt::hresult_invalid_argument(message), ShapingHelpers::c_callerDataFault);
    }

    void const* IdentityOf(winrt::IInspectable const& item)
    {
        return item ? winrt::get_abi(item.as<winrt::Windows::Foundation::IUnknown>()) : nullptr;
    }

    bool SameObject(winrt::IInspectable const& a, winrt::IInspectable const& b)
    {
        if (winrt::get_abi(a) == winrt::get_abi(b))
        {
            return true;
        }
        return a && b && IdentityOf(a) == IdentityOf(b);
    }

    winrt::Windows::Foundation::Collections::IVectorChangedEventArgs TryAsVectorChangedArgs(
        winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args)
    {
        return args;
    }

    winrt::Windows::Foundation::Collections::IVectorChangedEventArgs TryAsVectorChangedArgs(
        winrt::IInspectable const& args)
    {
        return args.try_as<winrt::Windows::Foundation::Collections::IVectorChangedEventArgs>();
    }
}

ShapedItemsSource2::ShapedItemsSource2(winrt::IInspectable const& items, IdentitySelector const& keySelector) :
    m_source(items),
    m_keySelector(keySelector),
    m_presented(winrt::single_threaded_observable_vector<winrt::IInspectable>()),
    m_ownerQueue(winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread()),
    m_ownerThreadId(::GetCurrentThreadId())
{
    // Tree nodes start collapsed; group headers start expanded (the model's default).
    m_nodeExpansion.SetAllExpanded(false);
}

ShapedItemsSource2::~ShapedItemsSource2()
{
    Unsubscribe();
    ClearLiveSubscriptions();
}

void ShapedItemsSource2::Start()
{
    m_liveShaping->SetChangeHandler(
        [weakThis = weak_from_this(), queue = m_ownerQueue, ownerThreadId = m_ownerThreadId, dropped = m_liveShapingChangeDropped](winrt::IInspectable const& item, winrt::hstring const&)
        {
            if (queue && !queue.HasThreadAccess())
            {
                // Off-thread: all touched state belongs to the owning thread, so marshal it.
                const bool posted = queue.TryEnqueue([weakThis, item]()
                    {
                        if (auto const strongThis = weakThis.lock())
                        {
                            strongThis->OnLiveItemChanged(item);
                        }
                    });
                if (!posted)
                {
                    dropped->store(true);
                }
                return;
            }

            if (!queue && ::GetCurrentThreadId() != ownerThreadId)
            {
                // No queue to marshal to; the next live change on the owner thread forces a full restore.
                dropped->store(true);
                return;
            }

            if (auto const strongThis = weakThis.lock())
            {
                strongThis->OnLiveItemChanged(item);
            }
        });

    m_accessor = ShapingHelpers::CollectionAccessor{ m_source, ShapingHelpers::CollectionOwner::Caller };
    ReloadAll();
    Subscribe();
    Republish(UpdateReason::Verb);
}

// --- Verbs -------------------------------------------------------------------------------------

void ShapedItemsSource2::SetFilter(ShapingHelpers::Predicate const& predicate)
{
    m_pipeline.SetFilter(predicate);
    m_filterOverlayCollapsed.clear();
    Republish(UpdateReason::Verb);
}

void ShapedItemsSource2::ClearFilter()
{
    m_pipeline.ClearFilter();
    m_filterOverlayCollapsed.clear();
    Republish(UpdateReason::Verb);
}

void ShapedItemsSource2::SetSort(
    winrt::hstring const& previousAxisToken,
    winrt::hstring const& axisToken,
    ShapingHelpers::KeySelector const& key,
    winrt::Windows::Foundation::IUnknown const& keyIdentity,
    winrt::hstring const& sortMemberPath,
    winrt::SortDirection direction)
{
    m_pipeline.SetSort(previousAxisToken, axisToken, key, keyIdentity, sortMemberPath, direction);
    Republish(UpdateReason::Verb);
}

void ShapedItemsSource2::ClearSorts()
{
    m_pipeline.ClearSorts();
    Republish(UpdateReason::Verb);
}

void ShapedItemsSource2::ClearSort(winrt::hstring const& axisToken)
{
    m_pipeline.ClearSort(axisToken);
    Republish(UpdateReason::Verb);
}

void ShapedItemsSource2::ClearSortsExcept(winrt::hstring const& axisToken)
{
    m_pipeline.ClearSortsExcept(axisToken);
    Republish(UpdateReason::Verb);
}

void ShapedItemsSource2::SetGroup(ShapingHelpers::KeySelector const& key, IdentitySelector const& groupIdentity)
{
    m_groupKey = key;
    m_groupIdentity = groupIdentity;
    m_pipeline.MarkGroupVerb(key);
    Republish(UpdateReason::Verb);
}

void ShapedItemsSource2::ClearGroup()
{
    m_groupKey = nullptr;
    m_groupIdentity = nullptr;
    m_pipeline.ClearGroupVerb();
    Republish(UpdateReason::Verb);
}

void ShapedItemsSource2::SetParent(IdentitySelector const& parentSelector)
{
    m_parentSelector = parentSelector;
    Republish(UpdateReason::Verb);
}

void ShapedItemsSource2::ClearParent()
{
    m_parentSelector = nullptr;
    Republish(UpdateReason::Verb);
}

// --- Projection queries ------------------------------------------------------------------------

ShapedItemsSource2::ProjectionKind ShapedItemsSource2::Kind() const noexcept
{
    if (HasGroup())
    {
        return HasParent() ? ProjectionKind::GroupedHierarchical : ProjectionKind::Grouped;
    }
    return HasParent() ? ProjectionKind::Hierarchical : ProjectionKind::Flat;
}

bool ShapedItemsSource2::ContainsItem(winrt::IInspectable const& item) const
{
    if (!item)
    {
        return false;
    }
    auto const it = m_itemToSrc.find(IdentityOf(item));
    return it != m_itemToSrc.end() &&
        static_cast<size_t>(it->second) < m_srcToPos.size() &&
        m_srcToPos[it->second] >= 0;
}

int32_t ShapedItemsSource2::PositionOfPresented(winrt::IInspectable const& row) const
{
    if (!row)
    {
        return -1;
    }
    if (auto const entry = m_entryToPos.find(winrt::get_abi(row)); entry != m_entryToPos.end())
    {
        return entry->second;
    }
    auto const it = m_itemToSrc.find(IdentityOf(row));
    if (it == m_itemToSrc.end() || static_cast<size_t>(it->second) >= m_srcToPos.size())
    {
        return -1;
    }
    return m_srcToPos[it->second];
}

int32_t ShapedItemsSource2::PresentedIndexOf(int32_t pos) const
{
    // m_visible is increasing: a scan in pre-order.
    auto const it = std::lower_bound(m_visible.begin(), m_visible.end(), pos);
    if (it == m_visible.end() || *it != pos)
    {
        return -1;
    }
    return static_cast<int32_t>(it - m_visible.begin());
}

winrt::hstring const& ShapedItemsSource2::KeyAt(int32_t pos) const
{
    static winrt::hstring const s_empty;
    if (pos < 0 || static_cast<size_t>(pos) >= m_rows.size())
    {
        return s_empty;
    }
    auto const src = m_rows[pos].Src;
    if (src < 0 || static_cast<size_t>(src) >= m_src.size())
    {
        return s_empty;
    }
    return m_src[src].Key;
}

int32_t ShapedItemsSource2::PositionOfKey(winrt::hstring const& key) const
{
    auto const it = m_keyToSrc.find(key);
    if (it == m_keyToSrc.end() || static_cast<size_t>(it->second) >= m_srcToPos.size())
    {
        return -1;
    }
    return m_srcToPos[it->second];
}

int32_t ShapedItemsSource2::PositionOfGroup(winrt::hstring const& groupKey) const
{
    auto const it = m_groupToBucket.find(groupKey);
    return it == m_groupToBucket.end() ? -1 : m_groups[it->second].HeaderPos;
}

bool ShapedItemsSource2::IsContextRow(int32_t pos) const
{
    if (!m_pipeline.HasFilter() || pos < 0 || static_cast<size_t>(pos) >= m_rows.size())
    {
        return false;
    }
    auto const src = m_rows[pos].Src;
    return src >= 0 && static_cast<size_t>(src) < m_src.size() && !m_src[src].SelfMatch;
}

// --- Expansion ---------------------------------------------------------------------------------

bool ShapedItemsSource2::SetNodeExpanded(winrt::hstring const& key, bool expanded)
{
    auto const pos = PositionOfKey(key);
    if (pos < 0 || PresentedIndexOf(pos) < 0 || m_rows[pos].ChildCount == 0 || m_rows[pos].Expanded == expanded)
    {
        return false;
    }

    // A context row is open because of the filter, not the user; collapsing it is filter-scoped.
    if (IsContextRow(pos))
    {
        if (expanded)
        {
            m_filterOverlayCollapsed.erase(key);
        }
        else
        {
            m_filterOverlayCollapsed.insert(key);
        }
    }
    else
    {
        m_nodeExpansion.SetExpanded(key, expanded);
    }
    Republish(UpdateReason::Expansion);
    return true;
}

bool ShapedItemsSource2::SetGroupExpanded(winrt::hstring const& groupKey, bool expanded)
{
    auto const pos = PositionOfGroup(groupKey);
    if (pos < 0 || m_rows[pos].Expanded == expanded)
    {
        return false;
    }
    m_groupExpansion.SetExpanded(groupKey, expanded);
    Republish(UpdateReason::Expansion);
    return true;
}

void ShapedItemsSource2::SetAllGroupsExpanded(bool expanded)
{
    m_groupExpansion.SetAllExpanded(expanded);
    Republish(UpdateReason::Expansion);
}

void ShapedItemsSource2::SetAllNodesExpanded(bool expanded)
{
    m_nodeExpansion.SetAllExpanded(expanded);
    m_filterOverlayCollapsed.clear();
    if (!expanded)
    {
        for (int32_t pos = 0; pos < static_cast<int32_t>(m_rows.size()); ++pos)
        {
            if (IsContextRow(pos))
            {
                m_filterOverlayCollapsed.insert(KeyAt(pos));
            }
        }
    }
    Republish(UpdateReason::Expansion);
}

void ShapedItemsSource2::ExpandSubtree(winrt::hstring const& key)
{
    auto const pos = PositionOfKey(key);
    if (pos < 0)
    {
        return;
    }
    for (int32_t i = pos; i < m_rows[pos].End; ++i)
    {
        if (m_rows[i].Src < 0 || m_rows[i].ChildCount == 0)
        {
            continue;
        }
        auto const& rowKey = KeyAt(i);
        if (IsContextRow(i))
        {
            m_filterOverlayCollapsed.erase(rowKey);
        }
        else
        {
            m_nodeExpansion.SetExpanded(rowKey, true);
        }
    }
    Republish(UpdateReason::Expansion);
}

// --- Source ------------------------------------------------------------------------------------

void ShapedItemsSource2::Subscribe()
{
    Unsubscribe();
    if (!m_source)
    {
        return;
    }

    auto weakThis = weak_from_this();
    auto applyVectorChange = [weakThis](auto&&, auto&& args)
    {
        if (auto strongThis = weakThis.lock())
        {
            if (auto vectorArgs = TryAsVectorChangedArgs(args))
            {
                strongThis->OnSourceVectorChanged(vectorArgs);
            }
            else
            {
                strongThis->OnSourceReset();
            }
        }
    };

    if (auto const& collection = m_accessor.AsNotifyCollectionChanged())
    {
        m_collectionChangedRevoker = collection.CollectionChanged(winrt::auto_revoke,
            [weakThis](auto&&, winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args)
            {
                if (auto strongThis = weakThis.lock())
                {
                    strongThis->OnSourceChanged(args);
                }
            });
    }
    else if (auto const& vector = m_accessor.AsObservableVector())
    {
        m_vectorChangedRevoker = vector.VectorChanged(winrt::auto_revoke, applyVectorChange);
    }
    else if (auto const& bindableVector = m_accessor.AsBindableObservableVector())
    {
        m_bindableVectorChangedRevoker = bindableVector.VectorChanged(winrt::auto_revoke, applyVectorChange);
    }
}

void ShapedItemsSource2::Unsubscribe()
{
    ShapingHelpers::SafeRevoke(m_collectionChangedRevoker);
    ShapingHelpers::SafeRevoke(m_vectorChangedRevoker);
    ShapingHelpers::SafeRevoke(m_bindableVectorChangedRevoker);
}

void ShapedItemsSource2::OnSourceChanged(winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args)
{
    using Action = winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedAction;
    bool applied = false;
    if (!m_cacheInvalid)
    {
        switch (args.Action())
        {
        case Action::Add:
        {
            auto const items = args.NewItems();
            auto const index = args.NewStartingIndex();
            applied = items && index >= 0 && TryInsertRows(static_cast<uint32_t>(index), items.Size());
            break;
        }
        case Action::Remove:
        {
            auto const items = args.OldItems();
            auto const index = args.OldStartingIndex();
            applied = items && index >= 0 && TryRemoveRows(static_cast<uint32_t>(index), items.Size());
            break;
        }
        case Action::Replace:
        {
            auto const items = args.NewItems();
            auto const oldItems = args.OldItems();
            auto const index = args.NewStartingIndex();
            applied = items && oldItems && items.Size() == oldItems.Size() && index >= 0 &&
                TryReplaceRows(static_cast<uint32_t>(index), items.Size());
            break;
        }
        default:
            break;
        }
    }

    if (!applied)
    {
        ReloadAll();
    }
    Republish(UpdateReason::Source);
}

void ShapedItemsSource2::OnSourceVectorChanged(winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args)
{
    using Change = winrt::Windows::Foundation::Collections::CollectionChange;
    bool applied = false;
    if (!m_cacheInvalid)
    {
        switch (args.CollectionChange())
        {
        case Change::ItemInserted:
            applied = TryInsertRows(args.Index(), 1);
            break;
        case Change::ItemRemoved:
            applied = TryRemoveRows(args.Index(), 1);
            break;
        case Change::ItemChanged:
            applied = TryReplaceRows(args.Index(), 1);
            break;
        default:
            break;
        }
    }

    if (!applied)
    {
        ReloadAll();
    }
    Republish(UpdateReason::Source);
}

void ShapedItemsSource2::OnSourceReset()
{
    ReloadAll();
    Republish(UpdateReason::Source);
}

winrt::hstring ShapedItemsSource2::EvaluateKey(winrt::IInspectable const& item) const
{
    if (!item)
    {
        ThrowCallerContract(L"TableViewSource2: items cannot contain null.");
    }

    winrt::hstring key;
    try
    {
        key = m_keySelector(item);
    }
    catch (...)
    {
        ShapingHelpers::RethrowAsCallerError(ShapingHelpers::c_callerDataFault);
    }

    if (key.empty())
    {
        ThrowCallerContract(L"TableViewSource2: the key selector returned an empty key.");
    }
    return key;
}

void ShapedItemsSource2::Evaluate(SrcRow& row)
{
    auto& faults = m_pipeline.SwallowedFaultsTally();

    // A throwing predicate excludes the row; the pipeline counts it.
    row.SelfMatch = m_pipeline.PassesFilter(row.Item);

    row.SortKeys.clear();
    row.SortKeys.reserve(m_axes.size());
    for (auto const& axis : m_axes)
    {
        winrt::IInspectable key{ nullptr };
        try
        {
            key = axis.Key(row.Item);
        }
        catch (...)
        {
            // A throwing key sorts as null.
            faults.Note();
        }
        row.SortKeys.push_back(std::move(key));
    }

    row.GroupValue = nullptr;
    row.GroupKey = {};
    row.GroupError = nullptr;
    if (m_groupKey)
    {
        try
        {
            row.GroupValue = m_groupKey(row.Item);
        }
        catch (...)
        {
            faults.Note();
        }

        winrt::hstring groupKey;
        wchar_t const* reason{ nullptr };
        if (RowIdentity::TryGetGroupIdentity(row.GroupValue, m_groupIdentity, groupKey, reason))
        {
            row.GroupKey = std::move(groupKey);
        }
        else
        {
            row.GroupError = reason ? reason : L"unusable group key";
        }
    }

    row.ParentKey = {};
    if (m_parentSelector)
    {
        try
        {
            row.ParentKey = m_parentSelector(row.Item);
        }
        catch (...)
        {
            // A throwing parent selector makes the row a root.
            faults.Note();
        }
    }
}

void ShapedItemsSource2::EvaluateAll()
{
    m_axes = m_pipeline.ActiveSortAxes(-1, -1);
    for (auto& row : m_src)
    {
        Evaluate(row);
    }
}

void ShapedItemsSource2::RebuildLookups()
{
    m_keyToSrc.clear();
    m_itemToSrc.clear();
    m_keyToSrc.reserve(m_src.size());
    m_itemToSrc.reserve(m_src.size());
    for (int32_t i = 0; i < static_cast<int32_t>(m_src.size()); ++i)
    {
        m_keyToSrc.emplace(m_src[i].Key, i);
        m_itemToSrc.emplace(IdentityOf(m_src[i].Item), i);
    }
    ++m_srcGeneration;
}

void ShapedItemsSource2::ReloadAll()
{
    std::vector<winrt::IInspectable> items;
    try
    {
        items = m_accessor.Enumerate(true /* throwIfUnsupported */);
    }
    catch (...)
    {
        m_cacheInvalid = true;
        throw;
    }

    std::vector<SrcRow> rows;
    rows.reserve(items.size());
    std::unordered_set<winrt::hstring> keys;
    keys.reserve(items.size());
    try
    {
        for (auto const& item : items)
        {
            SrcRow row;
            row.Item = item;
            row.Key = EvaluateKey(item);
            if (!keys.insert(row.Key).second)
            {
                ThrowCallerContract(L"TableViewSource2: duplicate row key '" + row.Key + L"'.");
            }
            rows.push_back(std::move(row));
        }
    }
    catch (...)
    {
        m_cacheInvalid = true;
        throw;
    }

    if (m_rowsReleasedHook)
    {
        m_rowsReleasedHook(nullptr, items);
    }

    m_src = std::move(rows);
    RebuildLookups();
    EvaluateAll();
    m_cacheInvalid = false;
    RefreshLiveSubscriptions();
}

bool ShapedItemsSource2::TryInsertRows(uint32_t index, uint32_t count)
{
    if (index > m_src.size() || !m_accessor.IsIndexable())
    {
        return false;
    }

    std::vector<SrcRow> rows(count);
    std::unordered_set<winrt::hstring> keys;
    for (uint32_t i = 0; i < count; ++i)
    {
        if (!m_accessor.TryGetAt(index + i, rows[i].Item))
        {
            return false;
        }
    }

    try
    {
        for (auto& row : rows)
        {
            row.Key = EvaluateKey(row.Item);
            if (m_keyToSrc.contains(row.Key) || !keys.insert(row.Key).second)
            {
                ThrowCallerContract(L"TableViewSource2: duplicate row key '" + row.Key + L"'.");
            }
        }
    }
    catch (...)
    {
        m_cacheInvalid = true;
        throw;
    }

    for (auto& row : rows)
    {
        if (m_liveItemChangedHook)
        {
            m_liveItemChangedHook(row.Item, true);
        }
        Evaluate(row);
        if (m_liveShapingEnabled)
        {
            m_liveShaping->Subscribe(row.Item);
        }
    }

    m_src.insert(m_src.begin() + index, std::make_move_iterator(rows.begin()), std::make_move_iterator(rows.end()));
    RebuildLookups();
    return true;
}

bool ShapedItemsSource2::TryRemoveRows(uint32_t index, uint32_t count)
{
    if (static_cast<size_t>(index) + count > m_src.size())
    {
        return false;
    }

    for (uint32_t i = 0; i < count; ++i)
    {
        ReleaseRow(m_src[index + i].Item);
    }
    m_src.erase(m_src.begin() + index, m_src.begin() + index + count);
    RebuildLookups();
    return true;
}

bool ShapedItemsSource2::TryReplaceRows(uint32_t index, uint32_t count)
{
    if (static_cast<size_t>(index) + count > m_src.size() || !m_accessor.IsIndexable())
    {
        return false;
    }

    std::vector<SrcRow> rows(count);
    for (uint32_t i = 0; i < count; ++i)
    {
        if (!m_accessor.TryGetAt(index + i, rows[i].Item))
        {
            return false;
        }
    }

    try
    {
        std::unordered_set<winrt::hstring> keys;
        for (uint32_t i = 0; i < count; ++i)
        {
            auto& row = rows[i];
            row.Key = EvaluateKey(row.Item);
            auto const existing = m_keyToSrc.find(row.Key);
            // A replacement may keep a replaced row's key, but not take another row's.
            bool const takenByOther = existing != m_keyToSrc.end() &&
                (existing->second < static_cast<int32_t>(index) || existing->second >= static_cast<int32_t>(index + count));
            if (takenByOther || !keys.insert(row.Key).second)
            {
                ThrowCallerContract(L"TableViewSource2: duplicate row key '" + row.Key + L"'.");
            }
        }
    }
    catch (...)
    {
        m_cacheInvalid = true;
        throw;
    }

    for (uint32_t i = 0; i < count; ++i)
    {
        auto& row = rows[i];
        auto& old = m_src[index + i];
        if (!SameObject(old.Item, row.Item))
        {
            ReleaseRow(old.Item);
            if (m_liveItemChangedHook)
            {
                m_liveItemChangedHook(row.Item, true);
            }
            if (m_liveShapingEnabled)
            {
                m_liveShaping->Subscribe(row.Item);
            }
        }
        Evaluate(row);
        old = std::move(row);
    }
    RebuildLookups();
    return true;
}

void ShapedItemsSource2::ReleaseRow(winrt::IInspectable const& item)
{
    if (m_rowsReleasedHook)
    {
        m_rowsReleasedHook(item, {});
    }
    if (m_liveShapingEnabled)
    {
        m_liveShaping->Unsubscribe(item);
    }
}

// --- Structure ---------------------------------------------------------------------------------

void ShapedItemsSource2::Republish(UpdateReason reason)
{
    try
    {
        if (reason != UpdateReason::Expansion)
        {
            if (m_cacheInvalid || reason == UpdateReason::Live)
            {
                // A live restore re-reads every row: coalesced changes were not snapshotted.
                ReloadAll();
            }
            else if (reason == UpdateReason::Verb)
            {
                EvaluateAll();
            }
            BuildStructure();
        }
        ComputeExpansion();
        ComputeVisible();
        ApplyPresented(reason);
    }
    catch (...)
    {
        // Rows built from a source that has since moved would point at the wrong items.
        if (m_rowsGeneration != m_srcGeneration)
        {
            ClearProjection();
        }
        throw;
    }

    if (reason == UpdateReason::Verb || reason == UpdateReason::Live)
    {
        // Every row was just re-read, so nothing live is pending.
        m_liveShapingDirty = false;
        m_liveShapingChangeDropped->store(false);
    }

    if (reason == UpdateReason::Verb && m_projectionRebuilt)
    {
        m_projectionRebuilt();
    }
    if (reason == UpdateReason::Verb || reason == UpdateReason::Live)
    {
        RaiseShapingChanged(false /* reorderOnly */);
    }
}

void ShapedItemsSource2::ClearProjection()
{
    m_rows.clear();
    m_groups.clear();
    m_groupToBucket.clear();
    m_entryToPos.clear();
    m_srcToPos.assign(m_src.size(), -1);
    m_includedDataRows = 0;
    m_visible.clear();
    m_rowsGeneration = m_srcGeneration;
    try
    {
        ApplyPresented(UpdateReason::Source);
    }
    catch (...)
    {
    }
}

int ShapedItemsSource2::CompareRows(int32_t a, int32_t b, std::vector<size_t> const& axes) const
{
    auto const& keysA = m_src[a].SortKeys;
    auto const& keysB = m_src[b].SortKeys;
    for (auto const k : axes)
    {
        auto const& keyA = k < keysA.size() ? keysA[k] : nullptr;
        auto const& keyB = k < keysB.size() ? keysB[k] : nullptr;
        auto const c = ShapingHelpers::ValueComparer::Compare(keyA, keyB);
        if (c != 0)
        {
            return m_axes[k].Direction == winrt::SortDirection::Descending ? -c : c;
        }
    }
    return 0;
}

void ShapedItemsSource2::BuildStructure()
{
    auto const n = static_cast<int32_t>(m_src.size());

    // 1. Parents. A missing parent makes a root; a self-parent or a cycle is the caller's error.
    std::vector<int32_t> parent(n, -1);
    if (m_parentSelector)
    {
        for (int32_t i = 0; i < n; ++i)
        {
            auto const& parentKey = m_src[i].ParentKey;
            if (parentKey.empty())
            {
                continue;
            }
            auto const it = m_keyToSrc.find(parentKey);
            if (it == m_keyToSrc.end())
            {
                continue;
            }
            if (it->second == i)
            {
                ThrowCallerContract(L"TableViewSource2: row '" + m_src[i].Key + L"' is its own parent.");
            }
            parent[i] = it->second;
        }

        // 0 = unvisited, 1 = on the current walk, 2 = known to reach a root.
        std::vector<uint8_t> state(n, 0);
        std::vector<int32_t> path;
        for (int32_t i = 0; i < n; ++i)
        {
            path.clear();
            int32_t current = i;
            while (current >= 0 && state[current] == 0)
            {
                state[current] = 1;
                path.push_back(current);
                current = parent[current];
            }
            if (current >= 0 && state[current] == 1)
            {
                ThrowCallerContract(L"TableViewSource2: parent keys form a cycle at row '" + m_src[current].Key + L"'.");
            }
            for (auto const p : path)
            {
                state[p] = 2;
            }
        }
    }

    // 2. Children lists, in source order.
    std::vector<int32_t> childStart(static_cast<size_t>(n) + 1, 0);
    std::vector<int32_t> roots;
    for (int32_t i = 0; i < n; ++i)
    {
        if (parent[i] >= 0)
        {
            ++childStart[parent[i] + 1];
        }
        else
        {
            roots.push_back(i);
        }
    }
    std::partial_sum(childStart.begin(), childStart.end(), childStart.begin());
    std::vector<int32_t> children(childStart[n]);
    {
        std::vector<int32_t> fill(childStart.begin(), childStart.end() - 1);
        for (int32_t i = 0; i < n; ++i)
        {
            if (parent[i] >= 0)
            {
                children[fill[parent[i]]++] = i;
            }
        }
    }

    // 3. Inclusion: a row the filter keeps, or an ancestor of one.
    std::vector<uint8_t> included(n, 1);
    if (m_pipeline.HasFilter())
    {
        for (int32_t i = 0; i < n; ++i)
        {
            included[i] = m_src[i].SelfMatch ? 1 : 0;
        }
        if (m_parentSelector)
        {
            // Children come after their parent in a pre-order walk, so a reverse walk sees them first.
            std::vector<int32_t> order;
            order.reserve(n);
            std::vector<int32_t> stack(roots.rbegin(), roots.rend());
            while (!stack.empty())
            {
                auto const node = stack.back();
                stack.pop_back();
                order.push_back(node);
                for (auto c = childStart[node]; c < childStart[node + 1]; ++c)
                {
                    stack.push_back(children[c]);
                }
            }
            for (auto it = order.rbegin(); it != order.rend(); ++it)
            {
                if (included[*it] && parent[*it] >= 0)
                {
                    included[parent[*it]] = 1;
                }
            }
        }
    }

    std::vector<size_t> allAxes(m_axes.size());
    std::iota(allAxes.begin(), allAxes.end(), size_t{ 0 });
    auto const sortBy = [this](std::vector<int32_t>& run, std::vector<size_t> const& axes)
    {
        if (!axes.empty() && run.size() > 1)
        {
            std::stable_sort(run.begin(), run.end(), [this, &axes](int32_t a, int32_t b) { return CompareRows(a, b, axes) < 0; });
        }
    };

    // 4. Units: the rows grouping and the top-level sort apply to (roots under ParentBy).
    std::vector<int32_t> units;
    for (auto const r : roots)
    {
        if (included[r])
        {
            units.push_back(r);
        }
    }

    std::vector<std::vector<int32_t>> buckets;
    std::vector<winrt::IInspectable> bucketValues;
    std::vector<GroupInfo> groups;
    std::unordered_map<winrt::hstring, int32_t> groupToBucket;
    if (m_groupKey)
    {
        // Sorts declared before GroupBy order the groups (by first appearance); later ones order
        // the rows within each group.
        std::vector<size_t> preAxes;
        std::vector<size_t> postAxes;
        auto const groupOrder = m_pipeline.GroupOrder();
        for (size_t k = 0; k < m_axes.size(); ++k)
        {
            (m_axes[k].Order < groupOrder ? preAxes : postAxes).push_back(k);
        }

        sortBy(units, preAxes);
        for (auto const u : units)
        {
            auto const& row = m_src[u];
            if (row.GroupError)
            {
                ThrowCallerContract(L"TableViewSource2: " + winrt::hstring{ row.GroupError } + L" for row '" + row.Key + L"'.");
            }
            auto const [it, inserted] = groupToBucket.try_emplace(row.GroupKey, static_cast<int32_t>(buckets.size()));
            if (inserted)
            {
                buckets.emplace_back();
                bucketValues.push_back(row.GroupValue);
                GroupInfo info;
                info.GroupKey = row.GroupKey;
                groups.push_back(std::move(info));
            }
            else if (!m_groupIdentity && !RowIdentity::GroupKeysEqual(bucketValues[it->second], row.GroupValue))
            {
                ThrowCallerContract(L"TableViewSource2: duplicate group identity '" + row.GroupKey + L"'.");
            }
            buckets[it->second].push_back(u);
        }
        for (auto& bucket : buckets)
        {
            // Stable, so rows tied on the later sorts keep the earlier sorts' order.
            sortBy(bucket, postAxes);
        }
    }
    else
    {
        sortBy(units, allAxes);
    }

    // 5. The pre-order array. Sibling runs below the top level sort on every axis.
    std::vector<Row> rows;
    rows.reserve(static_cast<size_t>(n) + buckets.size());
    std::vector<int32_t> srcToPos(n, -1);
    int32_t currentGroup = -1;
    struct Frame
    {
        int32_t Node;
        int32_t Depth;
        int32_t ParentPos;
        uint32_t PosInSet;
        uint32_t SizeOfSet;
    };
    std::vector<Frame> stack;
    std::vector<int32_t> kids;
    auto const emitTree = [&](int32_t root, int32_t depth, int32_t parentPos, uint32_t posInSet, uint32_t sizeOfSet)
    {
        stack.push_back({ root, depth, parentPos, posInSet, sizeOfSet });
        while (!stack.empty())
        {
            auto const frame = stack.back();
            stack.pop_back();

            auto const pos = static_cast<int32_t>(rows.size());
            srcToPos[frame.Node] = pos;

            kids.clear();
            for (auto c = childStart[frame.Node]; c < childStart[frame.Node + 1]; ++c)
            {
                if (included[children[c]])
                {
                    kids.push_back(children[c]);
                }
            }
            sortBy(kids, allAxes);

            Row row;
            row.Src = frame.Node;
            row.Group = currentGroup;
            row.Depth = frame.Depth;
            row.ParentPos = frame.ParentPos;
            row.PosInSet = frame.PosInSet;
            row.SizeOfSet = frame.SizeOfSet;
            row.ChildCount = static_cast<int32_t>(kids.size());
            rows.push_back(row);

            auto const size = static_cast<uint32_t>(kids.size());
            for (auto k = size; k-- > 0;)
            {
                stack.push_back({ kids[k], frame.Depth + 1, pos, k + 1, size });
            }
        }
    };

    if (m_groupKey)
    {
        for (int32_t b = 0; b < static_cast<int32_t>(buckets.size()); ++b)
        {
            currentGroup = b;
            auto const headerPos = static_cast<int32_t>(rows.size());
            Row header;
            header.Group = b;
            header.ChildCount = static_cast<int32_t>(buckets[b].size());
            rows.push_back(header);
            groups[b].HeaderPos = headerPos;
            groups[b].ItemCount = header.ChildCount;

            auto const size = static_cast<uint32_t>(buckets[b].size());
            for (uint32_t k = 0; k < size; ++k)
            {
                emitTree(buckets[b][k], 1, headerPos, k + 1, size);
            }
        }
    }
    else
    {
        auto const size = static_cast<uint32_t>(units.size());
        for (uint32_t k = 0; k < size; ++k)
        {
            emitTree(units[k], 0, -1, k + 1, size);
        }
    }

    // 6. Subtree ends: a row's subtree closes at the next row no deeper than it.
    std::vector<int32_t> open;
    for (int32_t j = 0; j < static_cast<int32_t>(rows.size()); ++j)
    {
        while (!open.empty() && rows[open.back()].Depth >= rows[j].Depth)
        {
            rows[open.back()].End = j;
            open.pop_back();
        }
        open.push_back(j);
    }
    for (auto const p : open)
    {
        rows[p].End = static_cast<int32_t>(rows.size());
    }

    // Commit.
    uint32_t dataRows = 0;
    for (auto const& row : rows)
    {
        dataRows += row.Src >= 0 ? 1 : 0;
    }
    m_rows = std::move(rows);
    m_srcToPos = std::move(srcToPos);
    m_groups = std::move(groups);
    m_groupToBucket = std::move(groupToBucket);
    m_includedDataRows = dataRows;
    m_rowsGeneration = m_srcGeneration;

    UpdateGroupObjects(bucketValues, buckets);

    // Expansion intent outlives filtering and collapse, but not the rows it was about.
    std::unordered_set<winrt::hstring> liveKeys;
    liveKeys.reserve(m_src.size());
    for (auto const& row : m_src)
    {
        liveKeys.insert(row.Key);
    }
    m_nodeExpansion.RetainOnly(liveKeys);
    std::erase_if(m_filterOverlayCollapsed, [&liveKeys](winrt::hstring const& key) { return !liveKeys.contains(key); });

    std::unordered_set<winrt::hstring> liveGroups;
    for (auto const& group : m_groups)
    {
        liveGroups.insert(group.GroupKey);
    }
    m_groupExpansion.RetainOnly(liveGroups);
}

void ShapedItemsSource2::UpdateGroupObjects(std::vector<winrt::IInspectable> const& bucketValues, std::vector<std::vector<int32_t>> const& bucketUnits)
{
    std::unordered_map<winrt::hstring, CachedGroup> cache;
    cache.reserve(m_groups.size());
    for (size_t b = 0; b < m_groups.size(); ++b)
    {
        auto const& groupKey = m_groups[b].GroupKey;
        CachedGroup cached;
        if (auto const it = m_groupCache.find(groupKey); it != m_groupCache.end())
        {
            cached = std::move(it->second);
            cached.Group->Key(bucketValues[b]);
        }
        else
        {
            cached.Group = winrt::make_self<ShapedGroup>(bucketValues[b], groupKey);
        }

        std::vector<winrt::IInspectable> items;
        items.reserve(bucketUnits[b].size());
        for (auto const u : bucketUnits[b])
        {
            items.push_back(m_src[u].Item);
        }

        auto const current = cached.Group->GroupItems();
        bool same = current && current.Size() == items.size();
        for (uint32_t i = 0; same && i < items.size(); ++i)
        {
            same = winrt::get_abi(current.GetAt(i)) == winrt::get_abi(items[i]);
        }
        if (!same)
        {
            cached.Group->SetItems(items);
        }
        // Over a hierarchy the header counts its roots.
        cached.Group->GroupChildCount(m_parentSelector ? static_cast<int32_t>(items.size()) : -1);

        cache.emplace(groupKey, std::move(cached));
    }
    m_groupCache = std::move(cache);
}

bool ShapedItemsSource2::ExpansionStateOf(int32_t pos) const
{
    auto const& row = m_rows[pos];
    if (row.ChildCount == 0)
    {
        return false;
    }
    if (row.Src < 0)
    {
        return m_groupExpansion.IsExpanded(m_groups[row.Group].GroupKey);
    }
    auto const& key = m_src[row.Src].Key;
    if (IsContextRow(pos))
    {
        return !m_filterOverlayCollapsed.contains(key);
    }
    return m_nodeExpansion.IsExpanded(key);
}

void ShapedItemsSource2::ComputeExpansion()
{
    for (int32_t pos = 0; pos < static_cast<int32_t>(m_rows.size()); ++pos)
    {
        m_rows[pos].Expanded = ExpansionStateOf(pos);
    }
    UpdateGroupEntries();
}

void ShapedItemsSource2::UpdateGroupEntries()
{
    m_entryToPos.clear();
    for (auto& group : m_groups)
    {
        auto& cached = m_groupCache[group.GroupKey];
        auto const expanded = m_rows[group.HeaderPos].Expanded;
        // A header's object carries its count and expansion, so it is re-minted when either moves.
        if (!cached.Entry || cached.EntryCount != group.ItemCount || cached.EntryExpanded != expanded)
        {
            cached.Entry = winrt::make<GroupedEntry>(cached.Group.as<winrt::IInspectable>(), group.ItemCount, expanded);
            cached.EntryCount = group.ItemCount;
            cached.EntryExpanded = expanded;
        }
        group.Entry = cached.Entry;
        m_entryToPos.emplace(winrt::get_abi(group.Entry), group.HeaderPos);
    }
}

void ShapedItemsSource2::ComputeVisible()
{
    m_visible.clear();
    m_visible.reserve(m_rows.size());
    int32_t hideDeeperThan = INT32_MAX;
    for (int32_t pos = 0; pos < static_cast<int32_t>(m_rows.size()); ++pos)
    {
        auto const& row = m_rows[pos];
        if (row.Depth > hideDeeperThan)
        {
            continue;
        }
        hideDeeperThan = INT32_MAX;
        m_visible.push_back(pos);
        if (!row.Expanded)
        {
            hideDeeperThan = row.Depth;
        }
    }
}

winrt::IInspectable ShapedItemsSource2::PresentedObjectAt(int32_t pos) const
{
    auto const& row = m_rows[pos];
    return row.Src >= 0 ? m_src[row.Src].Item : m_groups[row.Group].Entry;
}

winrt::hstring ShapedItemsSource2::PresentedIdAt(int32_t pos) const
{
    auto const& row = m_rows[pos];
    // A header's id cannot collide with a key that does not start with U+0001.
    return row.Src >= 0 ? m_src[row.Src].Key : winrt::hstring{ L"\x01" } + m_groups[row.Group].GroupKey;
}

ShapedItemsSource2::RowSignature ShapedItemsSource2::SignatureAt(int32_t pos) const
{
    auto const& row = m_rows[pos];
    return { row.Depth, row.ChildCount, row.PosInSet, row.SizeOfSet, row.Expanded };
}

void ShapedItemsSource2::ApplyPresented(UpdateReason reason)
{
    auto const newCount = m_visible.size();
    std::vector<winrt::hstring> newIds;
    std::vector<RowSignature> newSignatures;
    newIds.reserve(newCount);
    newSignatures.reserve(newCount);
    for (auto const pos : m_visible)
    {
        newIds.push_back(PresentedIdAt(pos));
        newSignatures.push_back(SignatureAt(pos));
    }

    auto const oldCount = m_presentedIds.size();
    size_t prefix = 0;
    while (prefix < oldCount && prefix < newCount && m_presentedIds[prefix] == newIds[prefix])
    {
        ++prefix;
    }
    size_t oldEnd = oldCount;
    size_t newEnd = newCount;
    while (oldEnd > prefix && newEnd > prefix && m_presentedIds[oldEnd - 1] == newIds[newEnd - 1])
    {
        --oldEnd;
        --newEnd;
    }

    std::unordered_map<winrt::hstring, size_t> oldMiddle;
    oldMiddle.reserve(oldEnd - prefix);
    for (size_t k = prefix; k < oldEnd; ++k)
    {
        oldMiddle.emplace(m_presentedIds[k], k);
    }
    std::unordered_map<winrt::hstring, size_t> newMiddle;
    newMiddle.reserve(newEnd - prefix);
    for (size_t k = prefix; k < newEnd; ++k)
    {
        newMiddle.emplace(newIds[k], k);
    }

    // Edits are applied in place only when the retained rows keep their relative order.
    bool ordered = true;
    size_t retained = 0;
    size_t lastNew = 0;
    for (size_t k = prefix; k < oldEnd && ordered; ++k)
    {
        auto const it = newMiddle.find(m_presentedIds[k]);
        if (it == newMiddle.end())
        {
            continue;
        }
        ordered = retained == 0 || it->second > lastNew;
        lastNew = it->second;
        ++retained;
    }
    auto const edits = (oldEnd - prefix - retained) + (newEnd - prefix - retained);

    if (!ordered || edits > c_maxIncrementalEdits)
    {
        std::vector<winrt::IInspectable> objects;
        objects.reserve(newCount);
        for (auto const pos : m_visible)
        {
            objects.push_back(PresentedObjectAt(pos));
        }
        m_presented.ReplaceAll(objects);
    }
    else
    {
        // The old position of each new row, or SIZE_MAX for an inserted one.
        std::vector<size_t> oldIndexOf(newCount, SIZE_MAX);
        for (size_t k = 0; k < prefix; ++k)
        {
            oldIndexOf[k] = k;
        }
        for (size_t k = newEnd; k < newCount; ++k)
        {
            oldIndexOf[k] = k - newEnd + oldEnd;
        }
        for (size_t k = prefix; k < newEnd; ++k)
        {
            if (auto const it = oldMiddle.find(newIds[k]); it != oldMiddle.end())
            {
                oldIndexOf[k] = it->second;
            }
        }

        for (size_t k = oldEnd; k-- > prefix;)
        {
            if (!newMiddle.contains(m_presentedIds[k]))
            {
                m_presented.RemoveAt(static_cast<uint32_t>(k));
            }
        }
        for (size_t k = prefix; k < newEnd; ++k)
        {
            if (oldIndexOf[k] == SIZE_MAX)
            {
                m_presented.InsertAt(static_cast<uint32_t>(k), PresentedObjectAt(m_visible[k]));
            }
        }

        // A retained row is replaced when its object changed (a replaced item, a re-minted header)
        // or, outside a pure expansion the consumer restamps itself, when its metadata moved.
        bool const restamp = reason != UpdateReason::Expansion;
        for (size_t k = 0; k < newCount; ++k)
        {
            auto const old = oldIndexOf[k];
            if (old == SIZE_MAX)
            {
                continue;
            }
            auto const object = PresentedObjectAt(m_visible[k]);
            if (!SameObject(m_presented.GetAt(static_cast<uint32_t>(k)), object) ||
                (restamp && !(m_presentedSignatures[old] == newSignatures[k])))
            {
                m_presented.SetAt(static_cast<uint32_t>(k), object);
            }
        }
    }

    m_presentedIds = std::move(newIds);
    m_presentedSignatures = std::move(newSignatures);
}

void ShapedItemsSource2::RaiseShapingChanged(bool reorderOnly)
{
    if (m_shapingChanged)
    {
        m_shapingChanged(reorderOnly);
    }
}

// --- Live shaping ------------------------------------------------------------------------------

void ShapedItemsSource2::SetLiveShaping(bool enabled)
{
    if (m_liveShapingEnabled == enabled)
    {
        return;
    }

    m_liveShapingEnabled = enabled;
    if (enabled)
    {
        // In-place edits made while untracked are invisible to snapshots, so catch up once.
        RefreshLiveSubscriptions();
        if (m_liveItemChangedHook)
        {
            m_liveItemChangedHook(nullptr, true);
        }
        MarkLiveDirty();
    }
    else
    {
        ClearLiveSubscriptions();
    }
}

void ShapedItemsSource2::RefreshLiveSubscriptions()
{
    if (!m_liveShapingEnabled)
    {
        return;
    }

    std::unordered_set<void const*> live;
    live.reserve(m_src.size());
    for (auto const& row : m_src)
    {
        m_liveShaping->Subscribe(row.Item);
        live.insert(IdentityOf(row.Item));
    }
    m_liveShaping->RetainOnly(live);
}

void ShapedItemsSource2::ClearLiveSubscriptions()
{
    m_liveShaping->UnsubscribeAll();
    m_liveShapingDirty = false;
    m_liveShapingHeld = false;
    m_liveShapingChangeDropped->store(false);
}

bool ShapedItemsSource2::RowChangedShape(SrcRow const& cached, SrcRow const& fresh) const
{
    if (cached.Key != fresh.Key ||
        cached.SelfMatch != fresh.SelfMatch ||
        cached.ParentKey != fresh.ParentKey ||
        cached.GroupKey != fresh.GroupKey ||
        (cached.GroupError == nullptr) != (fresh.GroupError == nullptr) ||
        cached.SortKeys.size() != fresh.SortKeys.size())
    {
        return true;
    }
    for (size_t k = 0; k < cached.SortKeys.size(); ++k)
    {
        if (ShapingHelpers::ValueComparer::Compare(cached.SortKeys[k], fresh.SortKeys[k]) != 0)
        {
            return true;
        }
    }
    return false;
}

void ShapedItemsSource2::OnLiveItemChanged(winrt::IInspectable const& item)
{
    if (!m_liveShapingEnabled || !item)
    {
        return;
    }

    if (m_liveShapingChangeDropped->exchange(false))
    {
        MarkLiveDirtyForItem();
    }

    // Before the coalescing early-out: the owner must see every changed item.
    if (m_liveItemChangedHook && m_liveItemChangedHook(item, m_liveShapingDirty))
    {
        MarkLiveDirtyForItem();
        return;
    }

    if (m_liveShapingDirty)
    {
        EnsureLiveRestore();
        return;
    }

    auto const it = m_itemToSrc.find(IdentityOf(item));
    if (it == m_itemToSrc.end())
    {
        return;
    }

    // Compare, don't store: the cache must keep matching the projection until the restore runs.
    SrcRow fresh;
    fresh.Item = item;
    try
    {
        fresh.Key = EvaluateKey(item);
    }
    catch (...)
    {
        // The restore reports nothing either; it re-reads and fails the same way.
        MarkLiveDirtyForItem();
        return;
    }
    Evaluate(fresh);
    if (RowChangedShape(m_src[it->second], fresh))
    {
        MarkLiveDirtyForItem();
    }
}

void ShapedItemsSource2::MarkLiveDirty()
{
    m_liveShapingDirty = true;
    EnsureLiveRestore();
}

void ShapedItemsSource2::MarkLiveDirtyForItem()
{
    if (!m_liveShapingDirty && m_liveItemReshape)
    {
        m_liveItemReshape();
    }
    MarkLiveDirty();
}

void ShapedItemsSource2::EnsureLiveRestore()
{
    if (!m_liveShapingDirty || m_liveShapingRestorePosted)
    {
        return;
    }

    if (m_liveShapingHeld)
    {
        // If the hold lifted without a resume (the owner went away mid-edit), re-post.
        if (m_liveShapingHold && m_liveShapingHold())
        {
            return;
        }
        m_liveShapingHeld = false;
    }

    PostLiveRestore();
}

void ShapedItemsSource2::PostLiveRestore()
{
    if (m_liveShapingRestorePosted)
    {
        return;
    }

    if (m_ownerQueue)
    {
        auto weakThis = weak_from_this();
        // Posted: coalesces a turn's writes into one rebuild, outside the app's PropertyChanged.
        m_liveShapingRestorePosted = m_ownerQueue.TryEnqueue([weakThis]()
            {
                if (auto const strongThis = weakThis.lock())
                {
                    strongThis->m_liveShapingRestorePosted = false;
                    try
                    {
                        strongThis->RestoreLive();
                    }
                    catch (...)
                    {
                        // No app call to receive this; the source stays dirty and the next change retries.
                    }
                }
            });
        return;
    }

    // No dispatcher on the owner thread (test host): restore inline, contained like the posted path.
    try
    {
        RestoreLive();
    }
    catch (...)
    {
    }
}

void ShapedItemsSource2::RestoreLive()
{
    if (!m_liveShapingDirty)
    {
        return;
    }

    if (!m_liveShapingEnabled)
    {
        m_liveShapingDirty = false;
        return;
    }

    if (m_liveShapingHold && m_liveShapingHold())
    {
        // Moving rows would end the edit; stay dirty until the owner resumes.
        m_liveShapingHeld = true;
        return;
    }

    Republish(UpdateReason::Live);
}

void ShapedItemsSource2::ResumeHeldLiveShaping()
{
    if (!std::exchange(m_liveShapingHeld, false))
    {
        return;
    }
    EnsureLiveRestore();
}
