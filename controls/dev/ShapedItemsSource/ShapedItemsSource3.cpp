// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ShapedItemsSource3.h"
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

ShapedItemsSource3::ShapedItemsSource3(winrt::IInspectable const& items, IdentitySelector const& keySelector) :
    m_source(items),
    m_keySelector(keySelector),
    m_presented(winrt::single_threaded_observable_vector<winrt::IInspectable>()),
    m_ownerQueue(winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread()),
    m_ownerThreadId(::GetCurrentThreadId())
{
    // Tree nodes start collapsed; group headers start expanded (the model's default).
    m_nodeExpansion.SetAllExpanded(false);
}

ShapedItemsSource3::~ShapedItemsSource3()
{
    Unsubscribe();
    ClearLiveSubscriptions();
}

void ShapedItemsSource3::Start()
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

void ShapedItemsSource3::SetFilter(ShapingHelpers::Predicate const& predicate)
{
    m_pipeline.SetFilter(predicate);
    m_filterOverlayCollapsed.clear();
    Republish(UpdateReason::Verb);
}

void ShapedItemsSource3::ClearFilter()
{
    m_pipeline.ClearFilter();
    m_filterOverlayCollapsed.clear();
    Republish(UpdateReason::Verb);
}

void ShapedItemsSource3::SetSort(
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

void ShapedItemsSource3::ClearSorts()
{
    m_pipeline.ClearSorts();
    Republish(UpdateReason::Verb);
}

void ShapedItemsSource3::ClearSort(winrt::hstring const& axisToken)
{
    m_pipeline.ClearSort(axisToken);
    Republish(UpdateReason::Verb);
}

void ShapedItemsSource3::ClearSortsExcept(winrt::hstring const& axisToken)
{
    m_pipeline.ClearSortsExcept(axisToken);
    Republish(UpdateReason::Verb);
}

void ShapedItemsSource3::SetGroup(ShapingHelpers::KeySelector const& key, IdentitySelector const& groupIdentity)
{
    m_groupKey = key;
    m_groupIdentity = groupIdentity;
    m_pipeline.MarkGroupVerb(key);
    Republish(UpdateReason::Verb);
}

void ShapedItemsSource3::ClearGroup()
{
    m_groupKey = nullptr;
    m_groupIdentity = nullptr;
    m_pipeline.ClearGroupVerb();
    Republish(UpdateReason::Verb);
}

void ShapedItemsSource3::SetParent(IdentitySelector const& parentSelector)
{
    m_parentSelector = parentSelector;
    Republish(UpdateReason::Verb);
}

void ShapedItemsSource3::ClearParent()
{
    m_parentSelector = nullptr;
    Republish(UpdateReason::Verb);
}

// --- Projection queries ------------------------------------------------------------------------

ShapedItemsSource3::ProjectionKind ShapedItemsSource3::Kind() const noexcept
{
    if (HasGroup())
    {
        return HasParent() ? ProjectionKind::GroupedHierarchical : ProjectionKind::Grouped;
    }
    return HasParent() ? ProjectionKind::Hierarchical : ProjectionKind::Flat;
}

bool ShapedItemsSource3::ContainsItem(winrt::IInspectable const& item) const
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

int32_t ShapedItemsSource3::PositionOfPresented(winrt::IInspectable const& row) const
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

int32_t ShapedItemsSource3::PresentedIndexOf(int32_t pos) const
{
    // m_visible is increasing: a scan in pre-order.
    auto const it = std::lower_bound(m_visible.begin(), m_visible.end(), pos);
    if (it == m_visible.end() || *it != pos)
    {
        return -1;
    }
    return static_cast<int32_t>(it - m_visible.begin());
}

winrt::hstring const& ShapedItemsSource3::KeyAt(int32_t pos) const
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

int32_t ShapedItemsSource3::PositionOfKey(winrt::hstring const& key) const
{
    auto const it = m_keyToSrc.find(key);
    if (it == m_keyToSrc.end() || static_cast<size_t>(it->second) >= m_srcToPos.size())
    {
        return -1;
    }
    return m_srcToPos[it->second];
}

int32_t ShapedItemsSource3::PositionOfGroup(winrt::hstring const& groupKey) const
{
    auto const it = m_groupToBucket.find(groupKey);
    return it == m_groupToBucket.end() ? -1 : m_groups[it->second].HeaderPos;
}

bool ShapedItemsSource3::IsContextRow(int32_t pos) const
{
    if (!m_pipeline.HasFilter() || pos < 0 || static_cast<size_t>(pos) >= m_rows.size())
    {
        return false;
    }
    auto const src = m_rows[pos].Src;
    return src >= 0 && static_cast<size_t>(src) < m_src.size() && !m_src[src].SelfMatch;
}

// --- Expansion ---------------------------------------------------------------------------------

bool ShapedItemsSource3::SetNodeExpanded(winrt::hstring const& key, bool expanded)
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
    if (!TrySetExpandedInPlace(pos))
    {
        Republish(UpdateReason::Expansion);
    }
    return true;
}

bool ShapedItemsSource3::SetGroupExpanded(winrt::hstring const& groupKey, bool expanded)
{
    auto const pos = PositionOfGroup(groupKey);
    if (pos < 0 || m_rows[pos].Expanded == expanded)
    {
        return false;
    }
    m_groupExpansion.SetExpanded(groupKey, expanded);
    if (!TrySetExpandedInPlace(pos))
    {
        Republish(UpdateReason::Expansion);
    }
    return true;
}

void ShapedItemsSource3::SetAllGroupsExpanded(bool expanded)
{
    m_groupExpansion.SetAllExpanded(expanded);
    Republish(UpdateReason::Expansion);
}

void ShapedItemsSource3::SetAllNodesExpanded(bool expanded)
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

void ShapedItemsSource3::ExpandSubtree(winrt::hstring const& key)
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

void ShapedItemsSource3::Subscribe()
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

void ShapedItemsSource3::Unsubscribe()
{
    ShapingHelpers::SafeRevoke(m_collectionChangedRevoker);
    ShapingHelpers::SafeRevoke(m_vectorChangedRevoker);
    ShapingHelpers::SafeRevoke(m_bindableVectorChangedRevoker);
}

void ShapedItemsSource3::OnSourceChanged(winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args)
{
    using Action = winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedAction;
    auto result = EditResult::Reload;
    if (!m_cacheInvalid)
    {
        BeginEdit(UpdateReason::Source);
        try
        {
            switch (args.Action())
            {
            case Action::Add:
            {
                auto const items = args.NewItems();
                auto const index = args.NewStartingIndex();
                if (items && index >= 0)
                {
                    result = ApplyInsert(static_cast<uint32_t>(index), items.Size());
                }
                break;
            }
            case Action::Remove:
            {
                auto const items = args.OldItems();
                auto const index = args.OldStartingIndex();
                if (items && index >= 0)
                {
                    result = ApplyRemove(static_cast<uint32_t>(index), items.Size());
                }
                break;
            }
            case Action::Replace:
            {
                auto const items = args.NewItems();
                auto const oldItems = args.OldItems();
                auto const index = args.NewStartingIndex();
                if (items && oldItems && items.Size() == oldItems.Size() && index >= 0)
                {
                    result = ApplyReplace(static_cast<uint32_t>(index), items.Size());
                }
                break;
            }
            default:
                break;
            }
        }
        catch (...)
        {
            AbortEdit();
            throw;
        }
    }
    FinishSourceEdit(result);
}

void ShapedItemsSource3::OnSourceVectorChanged(winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args)
{
    using Change = winrt::Windows::Foundation::Collections::CollectionChange;
    auto result = EditResult::Reload;
    if (!m_cacheInvalid)
    {
        BeginEdit(UpdateReason::Source);
        try
        {
            switch (args.CollectionChange())
            {
            case Change::ItemInserted:
                result = ApplyInsert(args.Index(), 1);
                break;
            case Change::ItemRemoved:
                result = ApplyRemove(args.Index(), 1);
                break;
            case Change::ItemChanged:
                result = ApplyReplace(args.Index(), 1);
                break;
            default:
                break;
            }
        }
        catch (...)
        {
            AbortEdit();
            throw;
        }
    }
    FinishSourceEdit(result);
}

void ShapedItemsSource3::FinishSourceEdit(EditResult result)
{
    if (result == EditResult::Done)
    {
        EndEdit();
        return;
    }

    // Whatever was applied in place reaches the presented rows first; the rebuild diffs from there.
    AbortEdit();
    if (result == EditResult::Reload)
    {
        ReloadAll();
    }
    Republish(UpdateReason::Source);
}

ShapedItemsSource3::EditResult ShapedItemsSource3::SourceEditResult()
{
    if (m_structureValid)
    {
        return EditResult::Done;
    }
    // The rows no longer describe the source: a failed rebuild must not keep them.
    ++m_srcGeneration;
    return EditResult::Rebuild;
}

void ShapedItemsSource3::OnSourceReset()
{
    ReloadAll();
    Republish(UpdateReason::Source);
}

winrt::hstring ShapedItemsSource3::EvaluateKey(winrt::IInspectable const& item) const
{
    if (!item)
    {
        ThrowCallerContract(L"TableViewSource3: items cannot contain null.");
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
        ThrowCallerContract(L"TableViewSource3: the key selector returned an empty key.");
    }
    return key;
}

void ShapedItemsSource3::Evaluate(SrcRow& row)
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

void ShapedItemsSource3::EvaluateAll()
{
    m_structureValid = false;
    m_axes = m_pipeline.ActiveSortAxes(-1, -1);
    UpdateAxisSplits();
    for (auto& row : m_src)
    {
        Evaluate(row);
    }
    RebuildParentRefs();
}

void ShapedItemsSource3::UpdateAxisSplits()
{
    m_allAxes.resize(m_axes.size());
    std::iota(m_allAxes.begin(), m_allAxes.end(), size_t{ 0 });
    m_preAxes.clear();
    m_postAxes.clear();
    auto const groupOrder = m_pipeline.GroupOrder();
    for (size_t k = 0; k < m_axes.size(); ++k)
    {
        (m_axes[k].Order < groupOrder ? m_preAxes : m_postAxes).push_back(k);
    }
}

void ShapedItemsSource3::RebuildParentRefs()
{
    m_parentRefs.clear();
    for (auto const& row : m_src)
    {
        AddParentRef(row.ParentKey);
    }
}

void ShapedItemsSource3::AddParentRef(winrt::hstring const& parentKey)
{
    if (!parentKey.empty())
    {
        ++m_parentRefs[parentKey];
    }
}

void ShapedItemsSource3::DropParentRef(winrt::hstring const& parentKey)
{
    if (parentKey.empty())
    {
        return;
    }
    if (auto const it = m_parentRefs.find(parentKey); it != m_parentRefs.end() && --it->second <= 0)
    {
        m_parentRefs.erase(it);
    }
}

void ShapedItemsSource3::InsertSrcRow(int32_t s, SrcRow&& row)
{
    for (auto& [key, index] : m_keyToSrc)
    {
        index += index >= s ? 1 : 0;
    }
    for (auto& [identity, index] : m_itemToSrc)
    {
        index += index >= s ? 1 : 0;
    }
    for (auto& r : m_rows)
    {
        r.Src += r.Src >= s ? 1 : 0;
    }
    m_keyToSrc.emplace(row.Key, s);
    m_itemToSrc.emplace(IdentityOf(row.Item), s);
    m_srcToPos.insert(m_srcToPos.begin() + s, -1);
    AddParentRef(row.ParentKey);
    m_src.insert(m_src.begin() + s, std::move(row));
}

void ShapedItemsSource3::EraseSrcRow(int32_t s)
{
    auto row = std::move(m_src[s]);
    m_src.erase(m_src.begin() + s);
    m_keyToSrc.erase(row.Key);
    m_itemToSrc.erase(IdentityOf(row.Item));
    for (auto& [key, index] : m_keyToSrc)
    {
        index -= index > s ? 1 : 0;
    }
    for (auto& [identity, index] : m_itemToSrc)
    {
        index -= index > s ? 1 : 0;
    }
    for (auto& r : m_rows)
    {
        r.Src -= r.Src > s ? 1 : 0;
    }
    m_srcToPos.erase(m_srcToPos.begin() + s);
    DropParentRef(row.ParentKey);

    // Expansion intent dies with its row, as a rebuild's RetainOnly would drop it.
    m_nodeExpansion.Forget(row.Key);
    m_filterOverlayCollapsed.erase(row.Key);
    ReleaseRow(row.Item);
}

void ShapedItemsSource3::CommitRow(int32_t s, SrcRow&& fresh)
{
    auto& old = m_src[s];
    if (old.Key != fresh.Key)
    {
        m_keyToSrc.erase(old.Key);
        m_keyToSrc.insert_or_assign(fresh.Key, s);
    }
    if (IdentityOf(old.Item) != IdentityOf(fresh.Item))
    {
        m_itemToSrc.erase(IdentityOf(old.Item));
        m_itemToSrc.insert_or_assign(IdentityOf(fresh.Item), s);
    }
    if (old.ParentKey != fresh.ParentKey)
    {
        DropParentRef(old.ParentKey);
        AddParentRef(fresh.ParentKey);
    }
    old = std::move(fresh);
}

void ShapedItemsSource3::RebuildLookups()
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

void ShapedItemsSource3::ReloadAll()
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
                ThrowCallerContract(L"TableViewSource3: duplicate row key '" + row.Key + L"'.");
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

ShapedItemsSource3::EditResult ShapedItemsSource3::ApplyInsert(uint32_t index, uint32_t count)
{
    if (index > m_src.size() || !m_accessor.IsIndexable())
    {
        return EditResult::Reload;
    }

    std::vector<SrcRow> rows(count);
    std::unordered_set<winrt::hstring> keys;
    for (uint32_t i = 0; i < count; ++i)
    {
        if (!m_accessor.TryGetAt(index + i, rows[i].Item))
        {
            return EditResult::Reload;
        }
    }

    try
    {
        for (auto& row : rows)
        {
            row.Key = EvaluateKey(row.Item);
            if (m_keyToSrc.contains(row.Key) || !keys.insert(row.Key).second)
            {
                ThrowCallerContract(L"TableViewSource3: duplicate row key '" + row.Key + L"'.");
            }
        }
    }
    catch (...)
    {
        m_cacheInvalid = true;
        throw;
    }

    // One row at a time, so a row inserted after its parent in the same batch still lands in place.
    for (uint32_t i = 0; i < count; ++i)
    {
        auto& row = rows[i];
        if (m_liveItemChangedHook)
        {
            m_liveItemChangedHook(row.Item, true);
        }
        Evaluate(row);
        if (m_liveShapingEnabled)
        {
            m_liveShaping->Subscribe(row.Item);
        }

        auto const s = static_cast<int32_t>(index + i);
        InsertSrcRow(s, std::move(row));
        if (m_structureValid && TryReshape(s, ReshapeKind::Added, nullptr) != ReshapeResult::Applied)
        {
            m_structureValid = false;
        }
    }
    return SourceEditResult();
}

ShapedItemsSource3::EditResult ShapedItemsSource3::ApplyRemove(uint32_t index, uint32_t count)
{
    if (static_cast<size_t>(index) + count > m_src.size())
    {
        return EditResult::Reload;
    }

    auto const s = static_cast<int32_t>(index);
    for (uint32_t i = 0; i < count; ++i)
    {
        if (m_structureValid && TryReshape(s, ReshapeKind::Removed, nullptr) != ReshapeResult::Applied)
        {
            m_structureValid = false;
        }
        EraseSrcRow(s);
    }
    return SourceEditResult();
}

ShapedItemsSource3::EditResult ShapedItemsSource3::ApplyReplace(uint32_t index, uint32_t count)
{
    if (static_cast<size_t>(index) + count > m_src.size() || !m_accessor.IsIndexable())
    {
        return EditResult::Reload;
    }

    std::vector<SrcRow> rows(count);
    for (uint32_t i = 0; i < count; ++i)
    {
        if (!m_accessor.TryGetAt(index + i, rows[i].Item))
        {
            return EditResult::Reload;
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
                ThrowCallerContract(L"TableViewSource3: duplicate row key '" + row.Key + L"'.");
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
        auto const s = static_cast<int32_t>(index + i);
        auto& row = rows[i];
        auto const& old = m_src[s];
        bool const objectChanged = !SameObject(old.Item, row.Item);
        if (objectChanged)
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

        if (!m_structureValid || row.Key != old.Key)
        {
            // A key change moves the row's identity (and its children's parent): not local.
            CommitRow(s, std::move(row));
            m_structureValid = false;
            continue;
        }

        if (!RowChangedShape(old, row))
        {
            // Same place in the shape: swap the object where it is presented.
            CommitRow(s, std::move(row));
            auto const pos = m_srcToPos[s];
            if (objectChanged && pos >= 0)
            {
                if (auto const at = PresentedIndexOf(pos); at >= 0)
                {
                    RecordSet(static_cast<size_t>(at), m_src[s].Item);
                }
                auto const parentPos = m_rows[pos].ParentPos;
                if (parentPos >= 0 && m_rows[parentPos].Src < 0)
                {
                    SyncBucket(m_rows[parentPos].Group);
                }
            }
            continue;
        }

        auto const result = TryReshape(s, ReshapeKind::Changed, &row);
        if (result == ReshapeResult::NotLocal)
        {
            CommitRow(s, std::move(row));
        }
        if (result != ReshapeResult::Applied)
        {
            m_structureValid = false;
        }
    }
    return SourceEditResult();
}

void ShapedItemsSource3::ReleaseRow(winrt::IInspectable const& item)
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

void ShapedItemsSource3::Republish(UpdateReason reason)
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
        ClearLivePending();
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

void ShapedItemsSource3::ClearProjection()
{
    m_structureValid = false;
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

int ShapedItemsSource3::CompareRows(int32_t a, int32_t b, std::vector<size_t> const& axes) const
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

void ShapedItemsSource3::BuildStructure()
{
    m_structureValid = false;
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
                ThrowCallerContract(L"TableViewSource3: row '" + m_src[i].Key + L"' is its own parent.");
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
                ThrowCallerContract(L"TableViewSource3: parent keys form a cycle at row '" + m_src[current].Key + L"'.");
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
                ThrowCallerContract(L"TableViewSource3: " + winrt::hstring{ row.GroupError } + L" for row '" + row.Key + L"'.");
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
                ThrowCallerContract(L"TableViewSource3: duplicate group identity '" + row.GroupKey + L"'.");
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
    m_structureValid = true;
}

void ShapedItemsSource3::UpdateGroupObjects(std::vector<winrt::IInspectable> const& bucketValues, std::vector<std::vector<int32_t>> const& bucketUnits)
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

bool ShapedItemsSource3::ExpansionStateOf(int32_t pos) const
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

void ShapedItemsSource3::ComputeExpansion()
{
    for (int32_t pos = 0; pos < static_cast<int32_t>(m_rows.size()); ++pos)
    {
        m_rows[pos].Expanded = ExpansionStateOf(pos);
    }
    UpdateGroupEntries();
}

void ShapedItemsSource3::UpdateGroupEntries()
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

void ShapedItemsSource3::ComputeVisible()
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

winrt::IInspectable ShapedItemsSource3::PresentedObjectAt(int32_t pos) const
{
    auto const& row = m_rows[pos];
    return row.Src >= 0 ? m_src[row.Src].Item : m_groups[row.Group].Entry;
}

winrt::hstring ShapedItemsSource3::PresentedIdAt(int32_t pos) const
{
    auto const& row = m_rows[pos];
    // A header's id cannot collide with a key that does not start with U+0001.
    return row.Src >= 0 ? m_src[row.Src].Key : winrt::hstring{ L"\x01" } + m_groups[row.Group].GroupKey;
}

ShapedItemsSource3::RowSignature ShapedItemsSource3::SignatureAt(int32_t pos) const
{
    auto const& row = m_rows[pos];
    return { row.Depth, row.ChildCount, row.PosInSet, row.SizeOfSet, row.Expanded };
}

void ShapedItemsSource3::ApplyPresented(UpdateReason reason)
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

    // Edits are applied in place only when the retained rows keep their relative order, and only
    // when the presented rows are what m_presentedIds says they are.
    bool ordered = !std::exchange(m_presentedOutOfSync, false);
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
    auto edits = (oldEnd - prefix - retained) + (newEnd - prefix - retained);

    // The old position of each new row, or SIZE_MAX for an inserted one.
    std::vector<size_t> oldIndexOf;
    // A retained row is replaced when its object changed (a replaced item, a re-minted header)
    // or, outside a pure expansion the consumer restamps itself, when its metadata moved.
    bool const restamp = reason != UpdateReason::Expansion;
    if (ordered && edits <= c_maxIncrementalEdits)
    {
        oldIndexOf.assign(newCount, SIZE_MAX);
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

        // Restamps count as edits: a change that shifts every row's metadata (a group header
        // added above a whole tree) is cheaper as one reset than as a replace per row.
        if (restamp)
        {
            for (size_t k = 0; k < newCount && edits <= c_maxIncrementalEdits; ++k)
            {
                auto const old = oldIndexOf[k];
                if (old != SIZE_MAX && !(m_presentedSignatures[old] == newSignatures[k]))
                {
                    ++edits;
                }
            }
        }
    }

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

void ShapedItemsSource3::RaiseShapingChanged(bool reorderOnly)
{
    if (m_shapingChanged)
    {
        m_shapingChanged(reorderOnly);
    }
}

// --- Incremental structure ---------------------------------------------------------------------
//
// Each primitive keeps m_rows, m_srcToPos, m_groups, m_visible and the presented mirrors
// (m_presentedIds, m_presentedSignatures) consistent, and records the presented edits that bring
// m_presented along. Positions shift by integer passes; no app code runs except key comparisons.

int ShapedItemsSource3::CompareKeys(SrcRow const& a, SrcRow const& b, std::vector<size_t> const& axes) const
{
    for (auto const k : axes)
    {
        auto const& keyA = k < a.SortKeys.size() ? a.SortKeys[k] : nullptr;
        auto const& keyB = k < b.SortKeys.size() ? b.SortKeys[k] : nullptr;
        auto const c = ShapingHelpers::ValueComparer::Compare(keyA, keyB);
        if (c != 0)
        {
            return m_axes[k].Direction == winrt::SortDirection::Descending ? -c : c;
        }
    }
    return 0;
}

ShapedItemsSource3::RunKind ShapedItemsSource3::RunKindOf(int32_t parentPos) const
{
    if (parentPos < 0)
    {
        return RunKind::TopLevel;
    }
    return m_rows[parentPos].Src < 0 ? RunKind::Units : RunKind::Children;
}

bool ShapedItemsSource3::RunLess(SrcRow const& a, int32_t aSrc, int32_t bSrc, RunKind kind) const
{
    // The orders BuildStructure's stable sorts produce: a group's units by (post, pre, source),
    // every other run by (all axes, source).
    auto const& b = m_src[bSrc];
    auto c = CompareKeys(a, b, kind == RunKind::Units ? m_postAxes : m_allAxes);
    if (c == 0 && kind == RunKind::Units)
    {
        c = CompareKeys(a, b, m_preAxes);
    }
    return c != 0 ? c < 0 : aSrc < bSrc;
}

int32_t ShapedItemsSource3::FindSlot(int32_t parentPos, int32_t s) const
{
    auto const& row = m_src[s];
    auto const kind = RunKindOf(parentPos);
    auto const start = RunStart(parentPos);
    auto const end = RunEnd(parentPos);
    if (!m_parentSelector)
    {
        // Without a hierarchy every sibling is one row, so the run is a sorted range.
        auto lo = start;
        auto hi = end;
        while (lo < hi)
        {
            auto const mid = lo + (hi - lo) / 2;
            if (RunLess(row, s, m_rows[mid].Src, kind))
            {
                hi = mid;
            }
            else
            {
                lo = mid + 1;
            }
        }
        return lo;
    }
    for (auto c = start; c < end; c = m_rows[c].End)
    {
        if (RunLess(row, s, m_rows[c].Src, kind))
        {
            return c;
        }
    }
    return end;
}

void ShapedItemsSource3::RebuildEntryToPos()
{
    m_entryToPos.clear();
    for (auto const& group : m_groups)
    {
        if (group.Entry)
        {
            m_entryToPos.emplace(winrt::get_abi(group.Entry), group.HeaderPos);
        }
    }
}

std::vector<ShapedItemsSource3::Row> ShapedItemsSource3::Detach(int32_t pos)
{
    auto const end = m_rows[pos].End;
    auto const k = end - pos;

    auto const first = std::lower_bound(m_visible.begin(), m_visible.end(), pos);
    auto const last = std::lower_bound(first, m_visible.end(), end);
    auto const at = static_cast<size_t>(first - m_visible.begin());
    EraseVisible(at, static_cast<size_t>(last - first));
    for (auto it = m_visible.begin() + at; it != m_visible.end(); ++it)
    {
        *it -= k;
    }

    std::vector<Row> held(m_rows.begin() + pos, m_rows.begin() + end);
    for (size_t j = 0; j < held.size(); ++j)
    {
        auto& row = held[j];
        row.End -= pos;
        row.ParentPos = j == 0 ? -1 : row.ParentPos - pos;
        if (row.Src >= 0)
        {
            m_srcToPos[row.Src] = -1;
            --m_includedDataRows;
        }
    }
    m_rows.erase(m_rows.begin() + pos, m_rows.begin() + end);

    for (int32_t i = 0; i < static_cast<int32_t>(m_rows.size()); ++i)
    {
        auto& row = m_rows[i];
        if (row.End > pos)
        {
            row.End -= k;
        }
        if (row.ParentPos > pos)
        {
            row.ParentPos -= k;
        }
        if (i >= pos && row.Src >= 0)
        {
            m_srcToPos[row.Src] = i;
        }
    }
    for (auto& group : m_groups)
    {
        if (group.HeaderPos > pos)
        {
            group.HeaderPos -= k;
        }
    }
    RebuildEntryToPos();
    return held;
}

void ShapedItemsSource3::Attach(std::vector<Row>&& held, int32_t parentPos, int32_t q)
{
    auto const k = static_cast<int32_t>(held.size());
    auto const depth = parentPos < 0 ? 0 : m_rows[parentPos].Depth + 1;
    auto const group = parentPos < 0 ? -1 : m_rows[parentPos].Group;
    auto const depthDelta = depth - held[0].Depth;

    // Rows whose subtree spans q grow; so do the parent and its ancestors when q is their end.
    for (int32_t i = 0; i < q; ++i)
    {
        if (m_rows[i].End > q)
        {
            m_rows[i].End += k;
        }
    }
    for (auto p = parentPos; p >= 0; p = m_rows[p].ParentPos)
    {
        if (m_rows[p].End == q)
        {
            m_rows[p].End += k;
        }
    }
    for (auto i = static_cast<size_t>(q); i < m_rows.size(); ++i)
    {
        auto& row = m_rows[i];
        row.End += k;
        if (row.ParentPos >= q)
        {
            row.ParentPos += k;
        }
    }

    for (size_t j = 0; j < held.size(); ++j)
    {
        auto& row = held[j];
        row.Depth += depthDelta;
        row.Group = group;
        row.End += q;
        row.ParentPos = j == 0 ? parentPos : row.ParentPos + q;
        m_includedDataRows += row.Src >= 0 ? 1 : 0;
    }
    m_rows.insert(m_rows.begin() + q, std::make_move_iterator(held.begin()), std::make_move_iterator(held.end()));

    for (auto i = static_cast<size_t>(q); i < m_rows.size(); ++i)
    {
        if (m_rows[i].Src >= 0)
        {
            m_srcToPos[m_rows[i].Src] = static_cast<int32_t>(i);
        }
    }
    for (auto& g : m_groups)
    {
        if (g.HeaderPos >= q)
        {
            g.HeaderPos += k;
        }
    }
    RebuildEntryToPos();

    auto const vq = std::lower_bound(m_visible.begin(), m_visible.end(), q);
    for (auto it = vq; it != m_visible.end(); ++it)
    {
        *it += k;
    }
    auto const at = static_cast<size_t>(vq - m_visible.begin());

    // The parent's count and expansion first: a first child can open a parent.
    RenumberRun(parentPos);
    if (IsShown(q))
    {
        std::vector<int32_t> shown;
        VisibleScan(q, q + k, shown);
        InsertVisible(at, shown);
    }
}

void ShapedItemsSource3::RenumberRun(int32_t parentPos)
{
    auto const start = RunStart(parentPos);
    auto const end = RunEnd(parentPos);
    uint32_t count = 0;
    for (auto c = start; c < end; c = m_rows[c].End)
    {
        ++count;
    }
    uint32_t index = 0;
    for (auto c = start; c < end; c = m_rows[c].End)
    {
        m_rows[c].PosInSet = ++index;
        m_rows[c].SizeOfSet = count;
    }
    if (parentPos >= 0)
    {
        m_rows[parentPos].ChildCount = static_cast<int32_t>(count);
        m_rows[parentPos].Expanded = ExpansionStateOf(parentPos);
    }
}

void ShapedItemsSource3::SyncBucket(int32_t bucket)
{
    auto& group = m_groups[bucket];
    auto const header = group.HeaderPos;

    std::vector<winrt::IInspectable> items;
    items.reserve(static_cast<size_t>(m_rows[header].ChildCount));
    for (auto c = header + 1; c < m_rows[header].End; c = m_rows[c].End)
    {
        items.push_back(m_src[m_rows[c].Src].Item);
    }
    group.ItemCount = m_rows[header].ChildCount;

    auto& cached = m_groupCache[group.GroupKey];
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
    cached.Group->GroupChildCount(m_parentSelector ? static_cast<int32_t>(items.size()) : -1);

    auto const expanded = m_rows[header].Expanded;
    if (!cached.Entry || cached.EntryCount != group.ItemCount || cached.EntryExpanded != expanded)
    {
        cached.Entry = winrt::make<GroupedEntry>(cached.Group.as<winrt::IInspectable>(), group.ItemCount, expanded);
        cached.EntryCount = group.ItemCount;
        cached.EntryExpanded = expanded;
        group.Entry = cached.Entry;
        RebuildEntryToPos();
        if (auto const at = PresentedIndexOf(header); at >= 0)
        {
            // The new object carries the new metadata, so this is also the header's restamp.
            m_presentedSignatures[at] = SignatureAt(header);
            RecordSet(static_cast<size_t>(at), group.Entry);
        }
    }
}

bool ShapedItemsSource3::UnitLess(int32_t a, int32_t b) const
{
    auto const c = CompareKeys(m_src[a], m_src[b], m_preAxes);
    return c != 0 ? c < 0 : a < b;
}

int32_t ShapedItemsSource3::MinUnitOf(int32_t bucket) const
{
    auto const header = m_groups[bucket].HeaderPos;
    int32_t best = -1;
    for (auto c = header + 1; c < m_rows[header].End; c = m_rows[c].End)
    {
        auto const s = m_rows[c].Src;
        if (best < 0 || UnitLess(s, best))
        {
            best = s;
        }
    }
    return best;
}

bool ShapedItemsSource3::GroupOrderHolds(int32_t bucket) const
{
    // Groups are ordered by their first unit in (pre-GroupBy sorts, source) order.
    if (bucket < 0 || m_groups.size() < 2)
    {
        return true;
    }
    auto const mine = MinUnitOf(bucket);
    if (bucket > 0 && !UnitLess(MinUnitOf(bucket - 1), mine))
    {
        return false;
    }
    return static_cast<size_t>(bucket) + 1 >= m_groups.size() || UnitLess(mine, MinUnitOf(bucket + 1));
}

void ShapedItemsSource3::VisibleScan(int32_t from, int32_t to, std::vector<int32_t>& out) const
{
    int32_t hideDeeperThan = INT32_MAX;
    for (auto pos = from; pos < to; ++pos)
    {
        auto const& row = m_rows[pos];
        if (row.Depth > hideDeeperThan)
        {
            continue;
        }
        hideDeeperThan = INT32_MAX;
        out.push_back(pos);
        if (!row.Expanded)
        {
            hideDeeperThan = row.Depth;
        }
    }
}

bool ShapedItemsSource3::IsShown(int32_t pos) const
{
    for (auto p = m_rows[pos].ParentPos; p >= 0; p = m_rows[p].ParentPos)
    {
        if (!m_rows[p].Expanded)
        {
            return false;
        }
    }
    return true;
}

int32_t ShapedItemsSource3::ParentIdOf(int32_t parentPos) const
{
    if (parentPos < 0)
    {
        return -1;
    }
    auto const& row = m_rows[parentPos];
    return row.Src >= 0 ? row.Src : -(row.Group + 2);
}

int32_t ShapedItemsSource3::PosOfParentId(int32_t id) const
{
    if (id == -1)
    {
        return -1;
    }
    return id >= 0 ? m_srcToPos[id] : m_groups[-(id + 2)].HeaderPos;
}

void ShapedItemsSource3::RestampRow(int32_t pos, size_t index)
{
    auto const signature = SignatureAt(pos);
    if (m_presentedSignatures[index] == signature)
    {
        return;
    }
    m_presentedSignatures[index] = signature;
    // A pure expansion leaves the restamp to the consumer, as ApplyPresented does.
    if (m_editReason != UpdateReason::Expansion)
    {
        RecordSet(index, PresentedObjectAt(pos));
    }
}

void ShapedItemsSource3::RestampRun(int32_t parentId)
{
    auto const parentPos = PosOfParentId(parentId);
    if (parentId != -1 && parentPos < 0)
    {
        return;
    }
    auto it = m_visible.begin();
    auto const visit = [&](int32_t pos)
    {
        it = std::lower_bound(it, m_visible.end(), pos);
        if (it != m_visible.end() && *it == pos)
        {
            RestampRow(pos, static_cast<size_t>(it - m_visible.begin()));
        }
    };
    if (parentPos >= 0)
    {
        visit(parentPos);
    }
    for (auto c = RunStart(parentPos); c < RunEnd(parentPos); c = m_rows[c].End)
    {
        visit(c);
    }
}

void ShapedItemsSource3::InsertVisible(size_t at, std::vector<int32_t> const& shown)
{
    if (shown.empty())
    {
        return;
    }
    std::vector<winrt::IInspectable> objects;
    std::vector<winrt::hstring> ids;
    std::vector<RowSignature> signatures;
    objects.reserve(shown.size());
    ids.reserve(shown.size());
    signatures.reserve(shown.size());
    for (auto const pos : shown)
    {
        objects.push_back(PresentedObjectAt(pos));
        ids.push_back(PresentedIdAt(pos));
        signatures.push_back(SignatureAt(pos));
    }
    m_visible.insert(m_visible.begin() + at, shown.begin(), shown.end());
    m_presentedIds.insert(m_presentedIds.begin() + at, std::make_move_iterator(ids.begin()), std::make_move_iterator(ids.end()));
    m_presentedSignatures.insert(m_presentedSignatures.begin() + at, signatures.begin(), signatures.end());
    RecordInsert(at, std::move(objects));
}

void ShapedItemsSource3::EraseVisible(size_t at, size_t count)
{
    if (count == 0)
    {
        return;
    }
    m_visible.erase(m_visible.begin() + at, m_visible.begin() + at + count);
    m_presentedIds.erase(m_presentedIds.begin() + at, m_presentedIds.begin() + at + count);
    m_presentedSignatures.erase(m_presentedSignatures.begin() + at, m_presentedSignatures.begin() + at + count);
    RecordRemove(at, count);
}

ShapedItemsSource3::ReshapeResult ShapedItemsSource3::TryReshape(int32_t s, ReshapeKind kind, SrcRow* fresh)
{
    if (!m_structureValid || m_presentedOutOfSync)
    {
        return ReshapeResult::NotLocal;
    }

    auto const& cached = m_src[s];
    auto const& next = kind == ReshapeKind::Changed ? *fresh : cached;
    if (kind == ReshapeKind::Changed && next.Key != cached.Key)
    {
        return ReshapeResult::NotLocal;
    }
    // An added row would adopt existing rows; a removed one would orphan its children.
    if (kind != ReshapeKind::Changed && m_parentSelector && m_parentRefs.contains(cached.Key))
    {
        return ReshapeResult::NotLocal;
    }

    auto const hasFilter = m_pipeline.HasFilter();
    auto const oldPos = kind == ReshapeKind::Added ? -1 : m_srcToPos[s];
    bool const oldIncluded = oldPos >= 0;
    auto const oldParentPos = oldIncluded ? m_rows[oldPos].ParentPos : -1;
    bool const hasChildren = oldIncluded && m_rows[oldPos].ChildCount > 0;

    bool newIncluded = false;
    int32_t newParentSrc = -1;
    int32_t bucket = -1;
    if (kind != ReshapeKind::Removed)
    {
        if (!hasFilter)
        {
            newIncluded = true;
        }
        else if (hasChildren)
        {
            // Turning a parent into a context row (or back) changes how its expansion resolves.
            if (next.SelfMatch != cached.SelfMatch)
            {
                return ReshapeResult::NotLocal;
            }
            newIncluded = true;
        }
        else
        {
            newIncluded = next.SelfMatch;
        }

        if (m_parentSelector && !next.ParentKey.empty())
        {
            if (next.ParentKey == next.Key)
            {
                return ReshapeResult::NotLocal;
            }
            if (auto const it = m_keyToSrc.find(next.ParentKey); it != m_keyToSrc.end())
            {
                newParentSrc = it->second;
                // A cycle is the caller's error, which the rebuild reports.
                size_t steps = 0;
                for (auto a = newParentSrc; a >= 0; ++steps)
                {
                    if (a == s || steps > m_src.size())
                    {
                        return ReshapeResult::NotLocal;
                    }
                    auto const& parentKey = m_src[a].ParentKey;
                    auto const up = parentKey.empty() ? m_keyToSrc.end() : m_keyToSrc.find(parentKey);
                    a = up == m_keyToSrc.end() ? -1 : up->second;
                }
            }
        }

        if (newIncluded)
        {
            if (newParentSrc >= 0)
            {
                // An excluded parent would become a context row.
                if (m_srcToPos[newParentSrc] < 0)
                {
                    return ReshapeResult::NotLocal;
                }
            }
            else if (m_groupKey)
            {
                if (next.GroupError)
                {
                    return ReshapeResult::NotLocal;
                }
                auto const it = m_groupToBucket.find(next.GroupKey);
                if (it == m_groupToBucket.end())
                {
                    return ReshapeResult::NotLocal;
                }
                bucket = it->second;
                if (!m_groupIdentity)
                {
                    auto const groupIt = m_groupCache.find(next.GroupKey);
                    if (groupIt == m_groupCache.end() || !RowIdentity::GroupKeysEqual(groupIt->second.Group->Group(), next.GroupValue))
                    {
                        return ReshapeResult::NotLocal;
                    }
                }
            }
        }
    }

    auto const oldParentId = oldIncluded ? ParentIdOf(oldParentPos) : INT32_MIN;
    auto const newParentId = !newIncluded ? INT32_MIN : newParentSrc >= 0 ? newParentSrc : bucket >= 0 ? -(bucket + 2) : -1;
    if (oldIncluded && oldParentId != newParentId && oldParentPos >= 0 && m_rows[oldParentPos].ChildCount == 1)
    {
        auto const parentSrc = m_rows[oldParentPos].Src;
        // An emptied group, or a context row left with no match, disappears.
        if (parentSrc < 0 || (hasFilter && !m_src[parentSrc].SelfMatch))
        {
            return ReshapeResult::NotLocal;
        }
    }

    bool committed = false;
    try
    {
        std::vector<Row> held;
        int32_t oldBucket = -1;
        if (oldIncluded)
        {
            if (oldParentPos >= 0 && m_rows[oldParentPos].Src < 0)
            {
                oldBucket = m_rows[oldParentPos].Group;
            }
            held = Detach(oldPos);
            RenumberRun(oldParentPos);
        }

        if (kind == ReshapeKind::Changed)
        {
            CommitRow(s, std::move(*fresh));
        }
        committed = true;

        if (newIncluded)
        {
            if (held.empty())
            {
                Row row;
                row.Src = s;
                row.End = 1;
                held.push_back(row);
            }
            auto const parentPos = PosOfParentId(newParentId);
            auto const slot = FindSlot(parentPos, s);
            Attach(std::move(held), parentPos, slot);
        }

        if (oldBucket >= 0)
        {
            SyncBucket(oldBucket);
        }
        if (bucket >= 0 && bucket != oldBucket)
        {
            SyncBucket(bucket);
        }
        if (oldIncluded)
        {
            RestampRun(oldParentId);
        }
        if (newIncluded && newParentId != oldParentId)
        {
            RestampRun(newParentId);
        }

        if (m_groupKey && (!GroupOrderHolds(oldBucket) || !GroupOrderHolds(bucket)))
        {
            return ReshapeResult::AppliedNeedsRebuild;
        }
        return ReshapeResult::Applied;
    }
    catch (...)
    {
        // A comparison threw part way: the rows are not trustworthy, but the source row must be.
        if (kind == ReshapeKind::Changed && !committed)
        {
            CommitRow(s, std::move(*fresh));
        }
        m_structureValid = false;
        return ReshapeResult::AppliedNeedsRebuild;
    }
}

bool ShapedItemsSource3::TrySetExpandedInPlace(int32_t pos)
{
    if (!m_structureValid || m_presentedOutOfSync)
    {
        return false;
    }

    BeginEdit(UpdateReason::Expansion);
    try
    {
        auto& row = m_rows[pos];
        auto const expanded = ExpansionStateOf(pos);
        if (row.Expanded != expanded)
        {
            row.Expanded = expanded;
            if (auto const at = PresentedIndexOf(pos); at >= 0)
            {
                auto const first = static_cast<size_t>(at) + 1;
                if (!expanded)
                {
                    auto const last = std::lower_bound(m_visible.begin() + first, m_visible.end(), row.End);
                    EraseVisible(first, static_cast<size_t>(last - m_visible.begin()) - first);
                }
                else
                {
                    std::vector<int32_t> shown;
                    VisibleScan(pos + 1, row.End, shown);
                    InsertVisible(first, shown);
                }
                RestampRow(pos, static_cast<size_t>(at));
            }
        }
        if (row.Src < 0)
        {
            SyncBucket(row.Group);
        }
    }
    catch (...)
    {
        AbortEdit();
        throw;
    }
    EndEdit();
    return true;
}

void ShapedItemsSource3::BeginEdit(UpdateReason reason)
{
    m_editReason = reason;
    m_editOps.clear();
    m_editCount = 0;
    m_editReset = false;
}

bool ShapedItemsSource3::CountEdits(size_t count)
{
    m_editCount += count;
    if (!m_editReset && m_editCount > c_maxIncrementalEdits)
    {
        // Past this, one reset is cheaper for the consumer than the edits.
        m_editReset = true;
        m_editOps.clear();
    }
    return !m_editReset;
}

void ShapedItemsSource3::RecordRemove(size_t at, size_t count)
{
    if (CountEdits(count))
    {
        m_editOps.push_back({ PresentedOp::Kind::Remove, static_cast<uint32_t>(at), static_cast<uint32_t>(count), {} });
    }
}

void ShapedItemsSource3::RecordInsert(size_t at, std::vector<winrt::IInspectable>&& objects)
{
    auto const count = objects.size();
    if (CountEdits(count))
    {
        m_editOps.push_back({ PresentedOp::Kind::Insert, static_cast<uint32_t>(at), static_cast<uint32_t>(count), std::move(objects) });
    }
}

void ShapedItemsSource3::RecordSet(size_t at, winrt::IInspectable const& object)
{
    if (CountEdits(1))
    {
        m_editOps.push_back({ PresentedOp::Kind::Set, static_cast<uint32_t>(at), 1, { object } });
    }
}

void ShapedItemsSource3::EndEdit()
{
    auto const ops = std::exchange(m_editOps, {});
    auto const reset = std::exchange(m_editReset, false);
    m_editCount = 0;
    try
    {
        if (reset)
        {
            std::vector<winrt::IInspectable> objects;
            objects.reserve(m_visible.size());
            for (auto const pos : m_visible)
            {
                objects.push_back(PresentedObjectAt(pos));
            }
            m_presented.ReplaceAll(objects);
            return;
        }
        for (auto const& op : ops)
        {
            switch (op.Op)
            {
            case PresentedOp::Kind::Remove:
                for (uint32_t j = 0; j < op.Count; ++j)
                {
                    m_presented.RemoveAt(op.Index);
                }
                break;
            case PresentedOp::Kind::Insert:
                for (uint32_t j = 0; j < op.Count; ++j)
                {
                    m_presented.InsertAt(op.Index + j, op.Objects[j]);
                }
                break;
            case PresentedOp::Kind::Set:
                m_presented.SetAt(op.Index, op.Objects[0]);
                break;
            }
        }
    }
    catch (...)
    {
        m_presentedOutOfSync = true;
        throw;
    }
}

void ShapedItemsSource3::AbortEdit()
{
    if (m_editReset)
    {
        // The mirrors moved past what m_presented shows; the next publish resets it.
        m_editOps.clear();
        m_editReset = false;
        m_editCount = 0;
        m_presentedOutOfSync = true;
        return;
    }
    EndEdit();
}

// --- Live shaping ------------------------------------------------------------------------------

void ShapedItemsSource3::SetLiveShaping(bool enabled)
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

void ShapedItemsSource3::RefreshLiveSubscriptions()
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

void ShapedItemsSource3::ClearLiveSubscriptions()
{
    m_liveShaping->UnsubscribeAll();
    m_liveShapingDirty = false;
    m_liveShapingHeld = false;
    m_liveShapingChangeDropped->store(false);
    ClearLivePending();
}

bool ShapedItemsSource3::RowChangedShape(SrcRow const& cached, SrcRow const& fresh) const
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

void ShapedItemsSource3::OnLiveItemChanged(winrt::IInspectable const& item)
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
        if (!m_liveFull)
        {
            // The restore evaluates it; the cache must keep matching the rows until then.
            QueueLiveItem(item);
        }
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
        QueueLiveItem(item);
    }
}

void ShapedItemsSource3::QueueLiveItem(winrt::IInspectable const& item)
{
    if (m_livePending.size() >= c_maxIncrementalEdits)
    {
        // Past this, re-reading every row is cheaper than relocating each.
        MarkLiveDirtyForItem();
        return;
    }
    if (!m_liveShapingDirty && m_liveItemReshape)
    {
        m_liveItemReshape();
    }
    m_livePending.push_back(item);
    m_liveShapingDirty = true;
    EnsureLiveRestore();
}

void ShapedItemsSource3::ClearLivePending()
{
    m_livePending.clear();
    m_liveFull = false;
}

void ShapedItemsSource3::MarkLiveDirty()
{
    m_liveFull = true;
    m_livePending.clear();
    m_liveShapingDirty = true;
    EnsureLiveRestore();
}

void ShapedItemsSource3::MarkLiveDirtyForItem()
{
    if (!m_liveShapingDirty && m_liveItemReshape)
    {
        m_liveItemReshape();
    }
    MarkLiveDirty();
}

void ShapedItemsSource3::EnsureLiveRestore()
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

void ShapedItemsSource3::PostLiveRestore()
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

void ShapedItemsSource3::RestoreLive()
{
    if (!m_liveShapingDirty)
    {
        return;
    }

    if (!m_liveShapingEnabled)
    {
        m_liveShapingDirty = false;
        ClearLivePending();
        return;
    }

    if (m_liveShapingHold && m_liveShapingHold())
    {
        // Moving rows would end the edit; stay dirty until the owner resumes.
        m_liveShapingHeld = true;
        return;
    }

    if (!m_liveFull && TryRestoreLiveIncrementally())
    {
        return;
    }
    Republish(UpdateReason::Live);
}

bool ShapedItemsSource3::TryRestoreLiveIncrementally()
{
    if (m_cacheInvalid || !m_structureValid || m_presentedOutOfSync || m_liveShapingChangeDropped->load())
    {
        return false;
    }

    auto const pending = std::exchange(m_livePending, {});
    BeginEdit(UpdateReason::Live);
    bool applied = true;
    std::unordered_set<void const*> seen;
    for (auto const& item : pending)
    {
        if (!seen.insert(IdentityOf(item)).second)
        {
            continue;
        }
        auto const it = m_itemToSrc.find(IdentityOf(item));
        if (it == m_itemToSrc.end())
        {
            continue;
        }
        auto const s = it->second;

        SrcRow fresh;
        fresh.Item = m_src[s].Item;
        try
        {
            fresh.Key = EvaluateKey(item);
        }
        catch (...)
        {
            applied = false;
            break;
        }
        Evaluate(fresh);
        if (!RowChangedShape(m_src[s], fresh))
        {
            continue;
        }
        if (TryReshape(s, ReshapeKind::Changed, &fresh) != ReshapeResult::Applied)
        {
            applied = false;
            break;
        }
    }

    if (!applied)
    {
        // The full restore re-reads every row, including those applied here.
        AbortEdit();
        return false;
    }

    EndEdit();
    m_liveShapingDirty = false;
    m_liveShapingChangeDropped->store(false);
    ClearLivePending();
    RaiseShapingChanged(false /* reorderOnly */);
    return true;
}

void ShapedItemsSource3::ResumeHeldLiveShaping()
{
    if (!std::exchange(m_liveShapingHeld, false))
    {
        return;
    }
    EnsureLiveRestore();
}
