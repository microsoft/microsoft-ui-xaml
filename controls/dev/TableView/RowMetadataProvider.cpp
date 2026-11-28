// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "RowMetadataProvider.h"
#include "TableViewRowInfo.h"
#include "GroupedEntry.h"

#include <algorithm>
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
    // First, so the subscription is inert no matter what happens below.
    m_alive.reset();

    try
    {
        if (m_rows && m_rowsChangedToken)
        {
            m_rows.CollectionChanged(m_rowsChangedToken);
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
    return std::make_shared<RowMetadataProvider>(SourceKind::Flat, rows, nullptr, itemKeySelector);
}

TableViewRowMetadataProvider RowMetadataProvider::CreateForGroupedRows(
    FlattenerPtr const& flattener,
    ItemKeySelector const& itemKeySelector)
{
    return std::make_shared<RowMetadataProvider>(
        SourceKind::Grouped, flattener ? flattener->GroupedRows() : nullptr, flattener, itemKeySelector);
}

RowMetadataProvider::RowMetadataProvider(
    SourceKind sourceKind,
    winrt::ItemsSourceView const& rows,
    FlattenerPtr const& flattener,
    ItemKeySelector const& itemKeySelector) :
    m_sourceKind(sourceKind),
    m_rows(rows),
    m_flattener(flattener),
    m_itemKeySelector(itemKeySelector)
{
    // So the reverse map cannot outlive the rows it describes: a stale identity would resolve to a moved
    // or deleted row.
    std::weak_ptr<bool> weakAlive = m_alive;
    if (m_rows)
    {
        m_rowsChangedToken = m_rows.CollectionChanged([this, weakAlive](auto&&, auto&&)
        {
            if (weakAlive.lock())
            {
                InvalidateIdentityIndex();
                InvalidateGroupHeaderIndices();
            }
        });
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

    const int32_t rowCount = m_rows ? m_rows.Count() : 0;
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
            // First writer wins; identities are unique, so this only keeps a tie deterministic.
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

void RowMetadataProvider::InvalidateGroupHeaderIndices()
{
    m_groupHeaderIndicesRowCount = -1;
    m_groupHeaderIndices.clear();
}

void RowMetadataProvider::EnsureGroupHeaderIndices(int32_t rowCount)
{
    if (m_groupHeaderIndicesRowCount == rowCount)
    {
        return;
    }

    m_groupHeaderIndices.clear();
    for (int32_t i = 0; i < rowCount; ++i)
    {
        if (TryGetGroupedEntry(m_rows.GetAt(i)))
        {
            m_groupHeaderIndices.push_back(i);
        }
    }
    m_groupHeaderIndicesRowCount = rowCount;
}

bool RowMetadataProvider::TryGetGroupMembership(
    winrt::IInspectable const& rows,
    int32_t index,
    int32_t& positionInGroup,
    int32_t& groupSize)
{
    positionInGroup = 0;
    groupSize = 0;

    // Only answer for our own rows, so the index can't resolve against stale ones.
    if (m_sourceKind != SourceKind::Grouped || !m_rows || !rows || !SameObject(rows, m_rows))
    {
        return false;
    }

    const auto rowCount = m_rows.Count();
    if (index < 0 || index >= rowCount)
    {
        return false;
    }

    EnsureGroupHeaderIndices(rowCount);

    // First band after the row; the one before it (if any) opens the row's group.
    const auto next = std::upper_bound(m_groupHeaderIndices.begin(), m_groupHeaderIndices.end(), index);
    const int32_t previousHeader = next == m_groupHeaderIndices.begin() ? -1 : *(next - 1);
    if (previousHeader == index)
    {
        // A header band is not a member of any group's data set.
        return false;
    }
    const int32_t nextHeader = next == m_groupHeaderIndices.end() ? rowCount : *next;

    positionInGroup = index - previousHeader;
    groupSize = nextHeader - previousHeader - 1;
    return true;
}

bool RowMetadataProvider::TryGetGroupHeaderSet(
    winrt::IInspectable const& rows,
    int32_t index,
    int32_t& position,
    int32_t& size)
{
    position = 0;
    size = 0;
    if (m_sourceKind != SourceKind::Grouped || !m_rows || !rows || !SameObject(rows, m_rows))
    {
        return false;
    }

    const auto rowCount = m_rows.Count();
    if (index < 0 || index >= rowCount)
    {
        return false;
    }

    EnsureGroupHeaderIndices(rowCount);
    const auto begin = m_groupHeaderIndices.begin();
    const auto end = m_groupHeaderIndices.end();
    const auto self = std::lower_bound(begin, end, index);
    if (self == end || *self != index)
    {
        return false;
    }
    const auto selfEntry = TryGetGroupHeaderEntry(index);
    if (!selfEntry)
    {
        return false;
    }

    // A lower Level is the parent (before) or the parent's next sibling (after): the set ends there.
    const int32_t level = selfEntry->Level();
    auto const sameLevel = [&](int32_t headerIndex, bool& stop)
    {
        const auto entry = TryGetGroupHeaderEntry(headerIndex);
        const int32_t other = entry ? entry->Level() : level;
        stop = other < level;
        return other == level;
    };

    int32_t before = 0;
    for (auto it = self; it != begin;)
    {
        bool stop = false;
        const bool sibling = sameLevel(*--it, stop);
        if (stop)
        {
            break;
        }
        before += sibling ? 1 : 0;
    }
    int32_t after = 0;
    for (auto it = self + 1; it != end; ++it)
    {
        bool stop = false;
        const bool sibling = sameLevel(*it, stop);
        if (stop)
        {
            break;
        }
        after += sibling ? 1 : 0;
    }

    position = before + 1;
    size = position + after;
    return true;
}

TableViewRowInfo RowMetadataProvider::GetRowInfo(int32_t index)
{
    auto kind = TableViewRowKind::Data;
    int32_t groupLevel = 0;
    bool isExpandable = false;
    bool isExpanded = false;
    int32_t childCount = 0;

    switch (m_sourceKind)
    {
    case SourceKind::Flat:
        (void)GetRow(index);
        break;

    case SourceKind::Grouped:
        if (const auto entry = TryGetGroupHeaderEntry(index))
        {
            kind = TableViewRowKind::GroupHeader;
            groupLevel = entry->Level();
            childCount = entry->GroupItemCount();
            // An empty group has nothing to expand into, so it presents as a leaf.
            isExpandable = childCount > 0;
            isExpanded = entry->IsExpanded();
            break;
        }

        groupLevel = DataRowGroupLevel(index);
        break;
    }

    return TableViewRowInfo{ kind, groupLevel, isExpandable, isExpanded, childCount };
}

winrt::hstring RowMetadataProvider::GetIdentity(int32_t index)
{
    switch (m_sourceKind)
    {
    case SourceKind::Flat:
        if (!m_rows)
        {
            throw winrt::hresult_out_of_bounds();
        }
        if (m_rows.HasKeyIndexMapping())
        {
            return m_rows.KeyFromIndex(index);
        }
        return GetItemKey(GetRow(index));

    case SourceKind::Grouped:
        if (const auto entry = TryGetGroupHeaderEntry(index))
        {
            return GetGroupExpansionIdentity(entry->Identity());
        }
        return GetItemKey(GetRow(index));
    }

    throw winrt::hresult_out_of_bounds();
}

bool RowMetadataProvider::SetGroupExpandedCore(winrt::hstring const& groupIdentity, std::optional<bool> desired)
{
    if (groupIdentity.empty() || !HasGroups())
    {
        return false;
    }

    auto const flattener = m_flattener;
    const bool before = flattener->IsGroupExpanded(groupIdentity);
    flattener->SetGroupExpanded(groupIdentity, desired.value_or(!before));
    return flattener->IsGroupExpanded(groupIdentity) != before;
}

bool RowMetadataProvider::Expand(winrt::hstring const& key)
{
    return SetGroupExpandedCore(ResolveGroupFromKey(key), true);
}

bool RowMetadataProvider::Collapse(winrt::hstring const& key)
{
    return SetGroupExpandedCore(ResolveGroupFromKey(key), false);
}

bool RowMetadataProvider::Toggle(winrt::hstring const& key)
{
    return SetGroupExpandedCore(ResolveGroupFromKey(key), std::nullopt);
}

// The guard of SetGroupExpandedCore: a group that resolves, in another state than the one requested.
bool RowMetadataProvider::WouldChangeExpansion(winrt::hstring const& key, std::optional<bool> desired) noexcept
{
    try
    {
        auto const groupIdentity = ResolveGroupFromKey(key);
        return !groupIdentity.empty() && HasGroups() &&
            (!desired || m_flattener->IsGroupExpanded(groupIdentity) != *desired);
    }
    catch (...)
    {
        return true;
    }
}

winrt::IInspectable RowMetadataProvider::GetRow(int32_t index) const
{
    if (!m_rows || index < 0 || index >= m_rows.Count())
    {
        throw winrt::hresult_out_of_bounds();
    }

    return m_rows.GetAt(index);
}

// One deeper than the header the row is under: 1, or more under a multi-level view's subgroup headers.
int32_t RowMetadataProvider::DataRowGroupLevel(int32_t index)
{
    if (!m_rows)
    {
        return 1;
    }

    const auto rowCount = m_rows.Count();
    EnsureGroupHeaderIndices(rowCount);
    const auto next = std::upper_bound(m_groupHeaderIndices.begin(), m_groupHeaderIndices.end(), index);
    if (next == m_groupHeaderIndices.begin())
    {
        return 1;
    }

    const auto header = TryGetGroupHeaderEntry(*(next - 1));
    return header ? header->Level() + 1 : 1;
}

winrt::com_ptr<GroupedEntry> RowMetadataProvider::TryGetGroupHeaderEntry(int32_t index) const
{
    // Null means "data row": a grouped row is either a GroupedEntry header or the app item itself.
    return TryGetGroupedEntry(GetRow(index));
}

void RowMetadataProvider::ExpandAllGroups()
{
    if (HasGroups())
    {
        auto const flattener = m_flattener;
        flattener->ExpandAllGroups();
    }
}

void RowMetadataProvider::CollapseAllGroups()
{
    if (HasGroups())
    {
        auto const flattener = m_flattener;
        flattener->CollapseAllGroups();
    }
}

std::vector<TableViewGroupExpansionInfo> RowMetadataProvider::GetGroupExpansionInfos()
{
    std::vector<TableViewGroupExpansionInfo> infos;
    if (!HasGroups())
    {
        return infos;
    }

    for (auto const& group : m_flattener->LiveGroups())
    {
        infos.push_back({ GetGroupExpansionIdentity(group.Identity), group.Key, m_flattener->IsGroupExpanded(group.Identity) });
    }
    return infos;
}

winrt::hstring RowMetadataProvider::GetGroupExpansionIdentity(winrt::hstring const& groupIdentity)
{
    return groupIdentity.empty() ? winrt::hstring{} : AppendPrefix(c_groupExpansionPrefix, groupIdentity);
}

bool RowMetadataProvider::TryGetGroupExpansionInfo(winrt::hstring const& identity, TableViewGroupExpansionInfo& info)
{
    if (!HasGroups())
    {
        return false;
    }

    auto const* const group = m_flattener->TryGetLiveGroup(ResolveGroupFromKey(identity));
    if (!group)
    {
        return false;
    }

    info.Identity = identity;
    info.Key = group->Key;
    info.IsExpanded = m_flattener->IsGroupExpanded(group->Identity);
    return true;
}

winrt::hstring RowMetadataProvider::GetItemKey(winrt::IInspectable const& item) const
{
    if (!item || !m_itemKeySelector)
    {
        return L"";
    }

    return m_itemKeySelector(item);
}

winrt::hstring RowMetadataProvider::ResolveGroupFromKey(winrt::hstring const& key) const
{
    if (!IsGroupExpansionKey(key) || !HasGroups())
    {
        return {};
    }

    const std::wstring_view keyView{ key.c_str(), key.size() };
    winrt::hstring const groupIdentity{ keyView.substr(c_groupExpansionPrefix.size()) };
    return m_flattener->TryGetLiveGroup(groupIdentity) ? groupIdentity : winrt::hstring{};
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

bool RowMetadataProvider::SameObject(winrt::IInspectable const& a, winrt::IInspectable const& b)
{
    auto aUnknown = a.try_as<::IUnknown>();
    auto bUnknown = b.try_as<::IUnknown>();
    return aUnknown && bUnknown && aUnknown.get() == bUnknown.get();
}

}
