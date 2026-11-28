// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"

#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "TableViewRowInfo.h"
#include "GroupedEntry.h"
#include "TableViewRowFlattener.h"

namespace winrt::Microsoft::UI::Xaml::Controls::Tabular::Primitives::implementation
{

class RowMetadataProvider : public ITableViewRowMetadataProvider
{
public:
    using ItemKeySelector = TableViewRowItemKeySelector;
    using FlattenerPtr = std::shared_ptr<TableViewRowFlattener>;

    ~RowMetadataProvider();

    // Identity contract:
    // - Flat rows preserve ItemsSourceView/IKeyIndexMapping keys when the source supplies them.
    // - Otherwise m_itemKeySelector supplies item keys. Rows without either identity source
    //   report no stable identity; item string/display representations are never identity.
    // - Group expansion keys are "group:" + the group's stable identity.
    static TableViewRowMetadataProvider CreateForFlatRows(
        winrt::ItemsSourceView const& rows,
        ItemKeySelector const& itemKeySelector = {});
    static TableViewRowMetadataProvider CreateForGroupedRows(
        FlattenerPtr const& flattener,
        ItemKeySelector const& itemKeySelector = {});

    TableViewRowInfo GetRowInfo(int32_t index) override;
    winrt::hstring GetIdentity(int32_t index) override;
    bool TryGetIndexForIdentity(winrt::hstring const& identity, int32_t& index) override;
    bool Expand(winrt::hstring const& key) override;
    bool Collapse(winrt::hstring const& key) override;
    bool Toggle(winrt::hstring const& key) override;
    bool WouldChangeExpansion(winrt::hstring const& key, std::optional<bool> desired) noexcept override;

    // Bulk group commands. No-ops when the source is not grouped.
    void ExpandAllGroups() override;
    void CollapseAllGroups() override;

    std::vector<TableViewGroupExpansionInfo> GetGroupExpansionInfos() override;
    winrt::hstring GetGroupExpansionIdentity(winrt::hstring const& groupIdentity) override;
    bool TryGetGroupExpansionInfo(winrt::hstring const& identity, TableViewGroupExpansionInfo& info) override;

    bool TryGetGroupMembership(
        winrt::IInspectable const& rows,
        int32_t index,
        int32_t& positionInGroup,
        int32_t& groupSize) override;

    bool TryGetGroupHeaderSet(
        winrt::IInspectable const& rows,
        int32_t index,
        int32_t& position,
        int32_t& size) override;

    enum class SourceKind
    {
        Flat,
        Grouped,
    };

    RowMetadataProvider(
        SourceKind sourceKind,
        winrt::ItemsSourceView const& rows,
        FlattenerPtr const& flattener,
        ItemKeySelector const& itemKeySelector);

private:
    bool HasGroups() const noexcept
    {
        return m_flattener && m_sourceKind == SourceKind::Grouped;
    }

    // Single implementation behind the group expand/collapse/toggle entry points. `desired` empty means
    // toggle. Returns true when the expansion state changed.
    bool SetGroupExpandedCore(winrt::hstring const& groupIdentity, std::optional<bool> desired);

    winrt::IInspectable GetRow(int32_t index) const;
    winrt::com_ptr<GroupedEntry> TryGetGroupHeaderEntry(int32_t index) const;
    int32_t DataRowGroupLevel(int32_t index);

    winrt::hstring GetItemKey(winrt::IInspectable const& item) const;
    // The identity of the live group a "group:" key names; empty when none.
    winrt::hstring ResolveGroupFromKey(winrt::hstring const& key) const;

    static bool IsGroupExpansionKey(winrt::hstring const& key);
    static winrt::hstring AppendPrefix(std::wstring_view prefix, winrt::hstring const& key);
    static bool SameObject(winrt::IInspectable const& a, winrt::IInspectable const& b);

    SourceKind m_sourceKind{ SourceKind::Flat };
    winrt::ItemsSourceView m_rows{ nullptr };
    // Strong: a handler of an expansion change may drop the flattener's other owner mid-call.
    FlattenerPtr m_flattener{};
    ItemKeySelector m_itemKeySelector{};

    // Lazily built identity -> row index, rebuilt wholesale: a grouped expand/collapse splices a range and a
    // re-sort permutes every row, so a map that is patched would be wrong.
    void InvalidateIdentityIndex();
    void EnsureIdentityIndex();
    std::unordered_map<winrt::hstring, int32_t> m_identityToIndex;
    bool m_identityIndexValid{ false };

    // Sorted header-band indexes; invalidated on CollectionChanged and keyed to row count. Built by
    // a plain scan: mid-expand a header's count can lead its inserted rows, so striding skips groups.
    void InvalidateGroupHeaderIndices();
    void EnsureGroupHeaderIndices(int32_t rowCount);
    std::vector<int32_t> m_groupHeaderIndices;
    int32_t m_groupHeaderIndicesRowCount{ -1 };

    // Raw token, revoked in the destructor: every strong owner (TableView) is a ReferenceTracker whose
    // final_release runs on the UI thread. The weak alive-flag turns a notification racing teardown into
    // a no-op.
    winrt::event_token m_rowsChangedToken{};
    std::shared_ptr<bool> m_alive{ std::make_shared<bool>(true) };
};

}
