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

RowMetadataProvider::RowMetadataProvider(
    SourceKind sourceKind,
    winrt::ItemsSourceView const& flatRows,
    winrt::ItemsSourceView const& groupedRows,
    GroupedSourceAdapterPtr const& groupedAdapter,
    ItemKeySelector const& itemKeySelector) :
    m_sourceKind(sourceKind),
    m_flatRows(flatRows),
    m_groupedRows(groupedRows),
    m_groupedAdapter(groupedAdapter),
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
    }

    return TableViewRowInfo{ kind, groupLevel, isExpandable, isExpanded, childCount };
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
    }

    throw winrt::hresult_out_of_bounds();
}

bool RowMetadataProvider::SetGroupExpandedCore(winrt::IInspectable const& group, std::optional<bool> desired)
{
    if (!group || !m_groupedAdapter)
    {
        return false;
    }

    const bool isExpanded = desired.value_or(!m_groupedAdapter->IsGroupExpanded(group));
    m_groupedAdapter->SetGroupExpanded(group, isExpanded);
    return isExpanded;
}

void RowMetadataProvider::Expand(winrt::hstring const& key)
{
    SetGroupExpandedCore(ResolveGroupFromKey(key), true);
}

void RowMetadataProvider::Collapse(winrt::hstring const& key)
{
    SetGroupExpandedCore(ResolveGroupFromKey(key), false);
}

bool RowMetadataProvider::Toggle(winrt::hstring const& key)
{
    return SetGroupExpandedCore(ResolveGroupFromKey(key), std::nullopt);
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
    if (!m_groupedAdapter)
    {
        return;
    }

    m_groupedAdapter->ExpandAll();
}

void RowMetadataProvider::CollapseAllGroups()
{
    if (!m_groupedAdapter)
    {
        return;
    }

    m_groupedAdapter->CollapseAll();
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
    const auto identity = TabularShapingHelpers::GetGroupKeyIdentity(group);
    if (!identity.empty())
    {
        return identity;
    }

    return GetCanonicalGroupKey(TabularShapingHelpers::GetGroupKeyObject(group));
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
    if (!TabularShapingHelpers::TabularValueKey::TryGetStablePropertyKey(key, canonicalKey, true))
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
