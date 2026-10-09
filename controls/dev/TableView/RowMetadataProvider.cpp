// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "RowMetadataProvider.h"
#include "TableViewRowInfo.h"
#include "GroupedSourceAdapter.h"
#include "GroupedEntry.h"
#include "GroupContract.h"
#include "ShapingHelpers.h"

#include <string>
#include <string_view>

namespace winrt::Microsoft::UI::Xaml::Controls::Tabular::Primitives::implementation
{
namespace
{
    constexpr std::wstring_view c_groupExpansionPrefix{ L"group:" };

    bool StartsWith(winrt::hstring const& value, std::wstring_view prefix)
    {
        const std::wstring_view view{ value.c_str(), value.size() };
        return view.size() >= prefix.size() && view.substr(0, prefix.size()) == prefix;
    }
}

RowMetadataProvider::~RowMetadataProvider()
{
    // Order matters: kill the subscription's effect first, so it is already inert no matter what
    // happens below (the handler checks this flag under a weak lock).
    m_alive.reset();

    // Teardown runs on the owning UI thread: every strong owner of this provider is a
    // ReferenceTracker (TableViewSource, TableView) whose final_release marshals destruction to the
    // captured DispatcherQueue, so the shared_ptr that drops this object drops it on that thread.
    // Revoking the XAML subscription below is therefore safe without a thread guard.
    try
    {
        if (m_groupedRows && m_groupedRowsChangedToken)
        {
            m_groupedRows.CollectionChanged(m_groupedRowsChangedToken);
        }

        if (m_flatRows && m_flatRowsChangedToken)
        {
            m_flatRows.CollectionChanged(m_flatRowsChangedToken);
        }

        if (m_hierarchicalRows && m_hierarchicalRowsChangedToken)
        {
            m_hierarchicalRows.CollectionChanged(m_hierarchicalRowsChangedToken);
        }
    }
    catch (...)
    {
    }
}

TableViewRowMetadataProvider RowMetadataProvider::CreateForFlatRows(
    winrt::ItemsSourceView const& rows,
    ItemKeySelector const& itemKeySelector)
{
    return std::make_shared<RowMetadataProvider>(
        SourceKind::Flat,
        rows,
        nullptr,
        nullptr,
        itemKeySelector);
}

TableViewRowMetadataProvider RowMetadataProvider::CreateForGroupedRows(
    winrt::ItemsSourceView const& rows,
    GroupedSourceAdapterPtr const& adapter,
    ItemKeySelector const& itemKeySelector)
{
    return std::make_shared<RowMetadataProvider>(
        SourceKind::Grouped,
        nullptr,
        rows,
        adapter,
        itemKeySelector);
}

TableViewRowMetadataProvider RowMetadataProvider::CreateForGroupedRows(
    GroupedSourceAdapterPtr const& adapter,
    ItemKeySelector const& itemKeySelector)
{
    return CreateForGroupedRows(adapter ? adapter->Entries() : nullptr, adapter, itemKeySelector);
}

TableViewRowMetadataProvider RowMetadataProvider::CreateForHierarchicalRows(
    HierarchicalSourceAdapterPtr const& adapter,
    ItemKeySelector const& itemKeySelector)
{
    return std::make_shared<RowMetadataProvider>(
        SourceKind::Hierarchical,
        nullptr,
        nullptr,
        nullptr,
        itemKeySelector,
        adapter ? adapter->Entries() : nullptr,
        adapter);
}

TableViewRowMetadataProvider RowMetadataProvider::CreateForGroupedHierarchicalRows(
    GroupedSourceAdapterPtr const& groupedAdapter,
    HierarchicalSourceAdapterPtr const& hierarchicalAdapter,
    ItemKeySelector const& itemKeySelector)
{
    // Only the grouped rows are subscribed: node toggles reach us as grouped-adapter changes,
    // so subscribing to the hierarchy too would invalidate the identity index twice.
    return std::make_shared<RowMetadataProvider>(
        SourceKind::GroupedHierarchical,
        nullptr,
        groupedAdapter ? groupedAdapter->Entries() : nullptr,
        groupedAdapter,
        itemKeySelector,
        nullptr,
        hierarchicalAdapter);
}

RowMetadataProvider::RowMetadataProvider(
    SourceKind sourceKind,
    winrt::ItemsSourceView const& flatRows,
    winrt::ItemsSourceView const& groupedRows,
    GroupedSourceAdapterPtr const& groupedAdapter,
    ItemKeySelector const& itemKeySelector,
    winrt::ItemsSourceView const& hierarchicalRows,
    HierarchicalSourceAdapterPtr const& hierarchicalAdapter) :
    m_sourceKind(sourceKind),
    m_flatRows(flatRows),
    m_groupedRows(groupedRows),
    m_groupedAdapter(groupedAdapter),
    m_hierarchicalRows(hierarchicalRows),
    m_hierarchicalAdapter(hierarchicalAdapter),
    m_itemKeySelector(itemKeySelector)
{
    // Subscribe to whichever source this provider indexes so the reverse map cannot outlive the
    // rows it describes. Without this a stale identity would resolve to a moved or deleted row.
    //
    // The handler captures a weak alive-flag so a notification that races teardown becomes a no-op
    // once the destructor resets the flag -- GC / re-entrancy safety, independent of threading.
    std::weak_ptr<bool> weakAlive = m_alive;
    auto onChanged = [this, weakAlive](auto&&, auto&&)
    {
        if (weakAlive.lock())
        {
            InvalidateIdentityIndex();
        }
    };

    if (m_groupedRows)
    {
        m_groupedRowsChangedToken = m_groupedRows.CollectionChanged(onChanged);
    }
    else if (m_hierarchicalRows)
    {
        m_hierarchicalRowsChangedToken = m_hierarchicalRows.CollectionChanged(onChanged);
    }
    else if (m_flatRows)
    {
        m_flatRowsChangedToken = m_flatRows.CollectionChanged(onChanged);
    }
}

void RowMetadataProvider::InvalidateIdentityIndex()
{
    m_identityIndexValid = false;
    m_identityToIndex.clear();
}

void RowMetadataProvider::EnsureIdentityIndex()
{
    if (m_identityIndexValid)
    {
        return;
    }

    m_identityToIndex.clear();
    m_identityIndexValid = true;

    int32_t rowCount = 0;
    switch (m_sourceKind)
    {
    case SourceKind::Flat:
        rowCount = m_flatRows ? m_flatRows.Count() : 0;
        break;
    case SourceKind::Grouped:
        rowCount = m_groupedRows ? m_groupedRows.Count() : 0;
        break;
    case SourceKind::Hierarchical:
        rowCount = m_hierarchicalRows ? m_hierarchicalRows.Count() : 0;
        break;
    case SourceKind::GroupedHierarchical:
        rowCount = m_groupedRows ? m_groupedRows.Count() : 0;
        break;
    }

    if (rowCount <= 0)
    {
        return;
    }

    m_identityToIndex.reserve(static_cast<size_t>(rowCount));
    for (int32_t index = 0; index < rowCount; ++index)
    {
        winrt::hstring identity;
        try
        {
            identity = GetIdentity(index);
        }
        catch (...)
        {
            continue;
        }

        if (!identity.empty())
        {
            // First writer wins, matching the scans this replaces. Identities are validated
            // unique upstream (an ambiguous one throws rather than projecting), so the tie
            // cannot legitimately occur; resolving it consistently just keeps the replacement
            // behaviour-identical if it ever does.
            m_identityToIndex.emplace(identity, index);
        }
    }
}

bool RowMetadataProvider::TryGetIndexForIdentity(winrt::hstring const& identity, int32_t& index)
{
    index = -1;
    if (identity.empty())
    {
        return false;
    }

    EnsureIdentityIndex();
    auto const found = m_identityToIndex.find(identity);
    if (found == m_identityToIndex.end())
    {
        return false;
    }

    index = found->second;
    return true;
}

TableViewRowInfo RowMetadataProvider::GetRowInfo(int32_t index)
{
    auto kind = TableViewRowKind::Data;
    int32_t groupLevel = 0;
    bool isExpandable = false;
    bool isExpanded = false;
    int32_t childCount = 0;
    // Set only for hierarchical data rows.
    HierarchicalSourceAdapter::NodeRow const* node = nullptr;

    switch (m_sourceKind)
    {
    case SourceKind::Flat:
        (void)GetFlatItem(index);
        break;

    case SourceKind::Grouped:
    {
        const auto entry = TryGetGroupHeaderEntry(index);
        if (entry)
        {
            kind = TableViewRowKind::GroupHeader;
            childCount = entry->GroupItemCount();
            // An empty group has nothing to expand into, so it presents as a leaf. Callers that
            // previously derived this from GroupItemCount() themselves now read it from here.
            isExpandable = childCount > 0;
            isExpanded = entry->IsExpanded();
        }
        else
        {
            groupLevel = 1;
        }
        break;
    }

    case SourceKind::Hierarchical:
    {
        if (m_hierarchicalAdapter)
        {
            node = m_hierarchicalAdapter->TryGetNodeRow(index);
        }
        break;
    }

    case SourceKind::GroupedHierarchical:
    {
        const auto entry = TryGetGroupHeaderEntry(index);
        if (entry)
        {
            // Count roots, not rows, so the header count does not change as nodes expand. Falls back to
            // the row count if the projection did not publish one.
            kind = TableViewRowKind::GroupHeader;
            childCount = entry->GroupItemCount();
            if (auto const counted = entry->Group().try_as<ShapingHelpers::IGroupChildCount>())
            {
                const int32_t rootCount = counted->GroupChildCount();
                if (rootCount >= 0)
                {
                    childCount = rootCount;
                }
            }
            // Expandability follows rows, so an empty group stays a leaf.
            isExpandable = entry->GroupItemCount() > 0;
            isExpanded = entry->IsExpanded();
            break;
        }

        // Index addresses the grouped axis (with headers); the item is the only handle shared with
        // the hierarchy adapter.
        groupLevel = 1;
        if (m_hierarchicalAdapter)
        {
            node = m_hierarchicalAdapter->TryGetNodeRowForItem(GetGroupedRow(index));
        }
        break;
    }
    }

    TableViewRowInfo info{ kind, groupLevel, isExpandable, isExpanded, childCount };
    if (node)
    {
        // Depth is 0-based; Level (and UIA AutomationProperties.Level) is 1-based.
        info.Level = node->Depth + 1;
        info.IsExpandable = node->HasChildren;
        info.IsExpanded = node->IsExpanded;
        info.ChildCount = node->ChildCount;
        info.ParentIdentity = node->ParentKey;
        info.PositionInSet = static_cast<uint32_t>(node->SiblingIndex);
        info.SizeOfSet = static_cast<uint32_t>(node->SiblingCount);
    }
    return info;
}

winrt::hstring RowMetadataProvider::GetIdentity(int32_t index)
{
    switch (m_sourceKind)
    {
    case SourceKind::Flat:
        if (!m_flatRows)
        {
            throw winrt::hresult_out_of_bounds();
        }
        if (m_flatRows.HasKeyIndexMapping())
        {
            return m_flatRows.KeyFromIndex(index);
        }
        return GetItemKey(GetFlatItem(index));

    case SourceKind::Grouped:
    {
        const auto entry = TryGetGroupHeaderEntry(index);
        if (entry)
        {
            return GetGroupExpansionKey(entry->Group());
        }
        return GetItemKey(GetGroupedRow(index));
    }

    case SourceKind::Hierarchical:
    {
        // Node key, matching what expand/collapse takes; survives item re-creation.
        if (m_hierarchicalAdapter)
        {
            if (auto const* const node = m_hierarchicalAdapter->TryGetNodeRow(index))
            {
                return node->NodeKey;
            }
        }
        throw winrt::hresult_out_of_bounds();
    }

    case SourceKind::GroupedHierarchical:
    {
        const auto entry = TryGetGroupHeaderEntry(index);
        if (entry)
        {
            return GetGroupExpansionKey(entry->Group());
        }

        // Node key so identity matches Toggle's addressing. Falls back to the item key mid-reshape.
        auto const item = GetGroupedRow(index);
        if (m_hierarchicalAdapter)
        {
            if (auto const* const node = m_hierarchicalAdapter->TryGetNodeRowForItem(item))
            {
                return node->NodeKey;
            }
        }
        return GetItemKey(item);
    }
    }

    throw winrt::hresult_out_of_bounds();
}

bool RowMetadataProvider::SetGroupExpandedCore(winrt::IInspectable const& group, std::optional<bool> desired)
{
    if (!group || !m_groupedAdapter)
    {
        return false;
    }

    // Strong ref: a handler of the resulting change may replace this provider mid-call.
    auto const adapter = m_groupedAdapter;
    const bool before = adapter->IsGroupExpanded(group);
    adapter->SetGroupExpanded(group, desired.value_or(!before));
    return adapter->IsGroupExpanded(group) != before;
}

void RowMetadataProvider::Expand(winrt::hstring const& key)
{
    if (IsNodeExpansionKey(key))
    {
        SetNodeExpandedCore(key, true);
        return;
    }
    SetGroupExpandedCore(ResolveGroupFromKey(key), true);
}

void RowMetadataProvider::Collapse(winrt::hstring const& key)
{
    if (IsNodeExpansionKey(key))
    {
        SetNodeExpandedCore(key, false);
        return;
    }
    SetGroupExpandedCore(ResolveGroupFromKey(key), false);
}

bool RowMetadataProvider::Toggle(winrt::hstring const& key)
{
    if (IsNodeExpansionKey(key))
    {
        return SetNodeExpandedCore(key, std::nullopt);
    }
    return SetGroupExpandedCore(ResolveGroupFromKey(key), std::nullopt);
}

// Under GroupedHierarchical both "group:" and "node:" keys arrive here; route on the key
// (disjoint prefixes), not SourceKind.
bool RowMetadataProvider::IsNodeExpansionKey(winrt::hstring const& key) const
{
    if (!m_hierarchicalAdapter || key.empty())
    {
        return false;
    }

    // Positive prefix test, so a stale or foreign identity cannot become node intent.
    return HierarchicalSourceAdapter::IsNodeKey(key);
}

bool RowMetadataProvider::SetNodeExpandedCore(winrt::hstring const& nodeKey, std::optional<bool> desired)
{
    if (!m_hierarchicalAdapter || nodeKey.empty())
    {
        return false;
    }

    auto const adapter = m_hierarchicalAdapter;
    const bool before = adapter->IsNodeExpanded(nodeKey);
    adapter->SetNodeExpanded(nodeKey, desired.value_or(!before));

    // Reports any change (collapse included), not the resulting state.
    return adapter->IsNodeExpanded(nodeKey) != before;
}

winrt::IInspectable RowMetadataProvider::GetGroupedRow(int32_t index) const
{
    if (!m_groupedRows || index < 0 || index >= m_groupedRows.Count())
    {
        throw winrt::hresult_out_of_bounds();
    }

    return m_groupedRows.GetAt(index);
}

winrt::com_ptr<GroupedEntry> RowMetadataProvider::TryGetGroupHeaderEntry(int32_t index) const
{
    // A grouped row is either a header, for which the projection mints a GroupedEntry, or an app
    // item exposed as-is. So this is a genuine test, not an assertion: null means "data row".
    // The IGroupedEntryTag probe is what makes it safe against an arbitrary app object, since a
    // bare try_as<GroupedEntry> succeeds on every WinRT object and then dereferences garbage.
    return TryGetGroupedEntry(GetGroupedRow(index));
}

void RowMetadataProvider::ExpandAllGroups()
{
    // Groups only, even when composed: driving the hierarchy too would expand every tree node.
    // See ExpandAllRows.
    if (auto const adapter = m_groupedAdapter)
    {
        adapter->ExpandAll();
    }
}

void RowMetadataProvider::CollapseAllGroups()
{
    if (auto const adapter = m_groupedAdapter)
    {
        adapter->CollapseAll();
    }
}

void RowMetadataProvider::ExpandAllRows()
{
    // Moves the hierarchy baseline; no app code runs. Does not open group headers when composed.
    if (auto const adapter = m_hierarchicalAdapter)
    {
        adapter->ExpandAll();
    }
}

void RowMetadataProvider::ExpandSubtree(winrt::hstring const& key)
{
    if (IsNodeExpansionKey(key))
    {
        auto const adapter = m_hierarchicalAdapter;
        adapter->ExpandSubtree(key);
    }
}

void RowMetadataProvider::CollapseAllRows()
{
    if (auto const adapter = m_hierarchicalAdapter)
    {
        adapter->CollapseAll();
    }
}

winrt::IInspectable RowMetadataProvider::GetFlatItem(int32_t index) const
{
    if (!m_flatRows || index < 0 || index >= m_flatRows.Count())
    {
        throw winrt::hresult_out_of_bounds();
    }

    return m_flatRows.GetAt(index);
}

winrt::hstring RowMetadataProvider::GetItemKey(winrt::IInspectable const& item) const
{
    if (!item)
    {
        return L"";
    }

    if (m_itemKeySelector)
    {
        auto key = m_itemKeySelector(item);
        if (!key.empty())
        {
            return key;
        }
    }

    return L"";
}

winrt::hstring RowMetadataProvider::GetGroupKey(winrt::IInspectable const& group) const
{
    // Read through the declared ICollectionViewGroup contract, using the same derivation the
    // grouped adapter uses. The two MUST agree: an expansion key produced here is handed back to
    // the adapter's identity lookup, and a mismatch would silently degrade every expand/collapse
    // to the O(rows) scan below.
    const auto identity = ShapingHelpers::GetGroupKeyIdentity(group);
    if (!identity.empty())
    {
        return identity;
    }

    return GetCanonicalGroupKey(ShapingHelpers::GetGroupKeyObject(group));
}

winrt::hstring RowMetadataProvider::GetGroupExpansionKey(winrt::IInspectable const& group) const
{
    const auto groupKey = GetGroupKey(group);
    return groupKey.empty() ? winrt::hstring{} : AppendPrefix(c_groupExpansionPrefix, groupKey);
}

winrt::IInspectable RowMetadataProvider::ResolveGroupFromKey(winrt::hstring const& key) const
{
    if (!IsGroupExpansionKey(key) || !m_groupedRows)
    {
        return nullptr;
    }

    const std::wstring_view keyView{ key.c_str(), key.size() };
    return ResolveGroupFromGroupKey(winrt::hstring{ keyView.substr(c_groupExpansionPrefix.size()) });
}

winrt::IInspectable RowMetadataProvider::ResolveGroupFromGroupKey(winrt::hstring const& groupKey) const
{
    if (groupKey.empty() || !m_groupedRows)
    {
        return nullptr;
    }

    if (m_groupedAdapter)
    {
        if (auto group = m_groupedAdapter->ResolveLiveGroupByIdentity(groupKey))
        {
            return group;
        }
    }

    winrt::IInspectable resolved{ nullptr };
    // Only header rows are GroupedEntry; data rows are the app items themselves and fail the probe.
    for (int32_t i = 0; i < m_groupedRows.Count(); ++i)
    {
        if (auto entry = TryGetGroupedEntry(m_groupedRows.GetAt(i)))
        {
            auto group = entry->Group();
            if (group && GetGroupKey(group) == groupKey)
            {
                if (!resolved)
                {
                    resolved = group;
                }
                else if (!SameObject(resolved, group))
                {
                    return nullptr;
                }
            }
        }
    }

    return resolved;
}

bool RowMetadataProvider::IsGroupExpansionKey(winrt::hstring const& key)
{
    return StartsWith(key, c_groupExpansionPrefix);
}

winrt::hstring RowMetadataProvider::AppendPrefix(std::wstring_view prefix, winrt::hstring const& key)
{
    std::wstring value{ prefix };
    value.append(key.c_str(), key.size());
    return winrt::hstring{ value };
}

winrt::hstring RowMetadataProvider::GetCanonicalGroupKey(winrt::IInspectable const& key)
{
    // Delegate to the shaping layer's canonicalizer rather than formatting here. A group key
    // string minted on this side is compared against one minted by the shaping engine, so the
    // two must be produced by the same code: a second implementation drifts silently. It did --
    // this used to format floats in decimal while the engine formats their exact bit pattern,
    // so no float-keyed group could ever match by string and every lookup fell through to the
    // slower object comparison.
    //
    // rejectEmptyString keeps the existing contract that an empty key string means "no usable
    // canonical key", which the caller reads as "fall back to comparing the key objects".
    winrt::hstring canonicalKey;
    if (!ShapingHelpers::ValueKey::TryGetStablePropertyKey(key, canonicalKey, true))
    {
        return L"";
    }

    return canonicalKey;
}

bool RowMetadataProvider::SameObject(winrt::IInspectable const& a, winrt::IInspectable const& b)
{
    auto aUnknown = a.try_as<::IUnknown>();
    auto bUnknown = b.try_as<::IUnknown>();
    return aUnknown && bUnknown && aUnknown.get() == bUnknown.get();
}

}
