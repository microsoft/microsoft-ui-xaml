// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"

#include <optional>
#include <string_view>

#include "TableViewRowInfo.h"
#include "GroupedEntry.h"
#include "GroupedSourceAdapter.h"
#include "HierarchicalSourceAdapter.h"

namespace winrt::Microsoft::UI::Xaml::Controls::Tabular::Primitives::implementation
{

class RowMetadataProvider : public ITableViewRowMetadataProvider
{
public:
    using ItemKeySelector = TableViewRowItemKeySelector;

    ~RowMetadataProvider();

    // Identity contract:
    // - Flat rows preserve ItemsSourceView/IKeyIndexMapping keys when the source supplies them.
    // - Otherwise m_itemKeySelector supplies item keys. Rows without either identity source
    //   report no stable identity; item string/display representations are never identity.
    // - Group expansion keys are "group:" + canonical stable group identity.
    // - Duplicate canonical group identities fail resolution rather than choosing an arbitrary group.
    static TableViewRowMetadataProvider CreateForFlatRows(
        winrt::ItemsSourceView const& rows,
        ItemKeySelector const& itemKeySelector = {});
    static TableViewRowMetadataProvider CreateForGroupedRows(
        winrt::ItemsSourceView const& rows,
        GroupedSourceAdapterPtr const& adapter = nullptr,
        ItemKeySelector const& itemKeySelector = {});
    static TableViewRowMetadataProvider CreateForGroupedRows(
        GroupedSourceAdapterPtr const& adapter,
        ItemKeySelector const& itemKeySelector = {});

    // Hierarchical rows are all DATA rows -- there is no header type to discriminate -- so the
    // adapter's per-row descriptor is the only source of level/expandability, and the expansion
    // key is the node's path key rather than a "group:" key.
    static TableViewRowMetadataProvider CreateForHierarchicalRows(
        HierarchicalSourceAdapterPtr const& adapter,
        ItemKeySelector const& itemKeySelector = {});

    // Both axes: GroupBy applied to the roots of a hierarchy. The presented row axis is the
    // GROUPED adapter's, so header rows exist and their indices interleave with data rows -- which
    // means a data row's index here does NOT address the hierarchy adapter. The row's item does,
    // via TryGetNodeRowForItem, and that is how level/expandability are recovered.
    static TableViewRowMetadataProvider CreateForGroupedHierarchicalRows(
        GroupedSourceAdapterPtr const& groupedAdapter,
        HierarchicalSourceAdapterPtr const& hierarchicalAdapter,
        ItemKeySelector const& itemKeySelector = {});

    TableViewRowInfo GetRowInfo(int32_t index) override;
    winrt::hstring GetIdentity(int32_t index) override;
    bool TryGetIndexForIdentity(winrt::hstring const& identity, int32_t& index) override;
    void Expand(winrt::hstring const& key) override;
    void Collapse(winrt::hstring const& key) override;
    bool Toggle(winrt::hstring const& key) override;

    // Bulk group commands. No-ops when the source is not grouped.
    void ExpandAllGroups() override;
    void CollapseAllGroups() override;

    enum class SourceKind
    {
        Flat,
        Grouped,
        Hierarchical,
        // Grouped rows whose DATA rows are also tree nodes. Row kinds and identities come from the
        // grouped axis; level, expandability and node expansion come from the hierarchy adapter.
        GroupedHierarchical,
    };

    RowMetadataProvider(
        SourceKind sourceKind,
        winrt::ItemsSourceView const& flatRows,
        winrt::ItemsSourceView const& groupedRows,
        GroupedSourceAdapterPtr const& groupedAdapter,
        ItemKeySelector const& itemKeySelector,
        winrt::ItemsSourceView const& hierarchicalRows = nullptr,
        HierarchicalSourceAdapterPtr const& hierarchicalAdapter = nullptr);

private:
    // Single implementation behind all six expand/collapse/toggle entry points. They differ only
    // in how the caller names the group (row key vs. the app's GroupBy key), so resolution stays
    // in the wrappers and the state change lives here exactly once. `desired` empty means toggle.
    // Returns the resulting expansion state; false when there is no group or no adapter.
    bool SetGroupExpandedCore(winrt::IInspectable const& group, std::optional<bool> desired);

    // The hierarchy equivalent, keyed by node path rather than by a resolved group object. No
    // resolution step: the path key IS the adapter's addressing scheme.
    bool SetNodeExpandedCore(winrt::hstring const& pathKey, std::optional<bool> desired);

    // True when `key` addresses a tree node rather than a group. Required because the composed
    // projection routes both key spaces through the same Expand/Collapse/Toggle surface.
    bool IsNodeExpansionKey(winrt::hstring const& key) const;

    winrt::IInspectable GetGroupedRow(int32_t index) const;
    winrt::com_ptr<GroupedEntry> TryGetGroupHeaderEntry(int32_t index) const;
    winrt::IInspectable GetFlatItem(int32_t index) const;

    winrt::hstring GetItemKey(winrt::IInspectable const& item) const;
    winrt::hstring GetGroupKey(winrt::IInspectable const& group) const;
    winrt::hstring GetGroupExpansionKey(winrt::IInspectable const& group) const;
    winrt::IInspectable ResolveGroupFromKey(winrt::hstring const& key) const;
    winrt::IInspectable ResolveGroupFromGroupKey(winrt::hstring const& groupKey) const;

    static bool IsGroupExpansionKey(winrt::hstring const& key);
    static winrt::hstring AppendPrefix(std::wstring_view prefix, winrt::hstring const& key);
    static winrt::hstring GetCanonicalGroupKey(winrt::IInspectable const& key);
    static bool SameObject(winrt::IInspectable const& a, winrt::IInspectable const& b);

    SourceKind m_sourceKind{ SourceKind::Flat };
    winrt::ItemsSourceView m_flatRows{ nullptr };
    winrt::ItemsSourceView m_groupedRows{ nullptr };
    GroupedSourceAdapterPtr m_groupedAdapter{};
    winrt::ItemsSourceView m_hierarchicalRows{ nullptr };
    HierarchicalSourceAdapterPtr m_hierarchicalAdapter{};
    ItemKeySelector m_itemKeySelector{};

    // Lazily built identity -> row index over the rows this provider wraps. Rebuilt wholesale
    // rather than maintained incrementally: the sources it indexes signal change but not enough
    // of it to patch a map (a grouped expand/collapse splices a range, a re-sort permutes every
    // row), and a map that is wrong is worse than one that is rebuilt.
    void InvalidateIdentityIndex();
    void EnsureIdentityIndex();
    std::unordered_map<winrt::hstring, int32_t> m_identityToIndex;
    bool m_identityIndexValid{ false };

    // Subscriptions to XAML's ItemsSourceView are held as raw tokens, not auto-revokers.
    //
    // Teardown is guaranteed on the owning UI thread: every strong owner of this provider is a
    // ReferenceTracker (TableViewSource, TableView) whose final_release marshals destruction to the
    // captured DispatcherQueue. So the destructor revokes the subscription directly, with no thread
    // guard. (Before TableViewSource became a ReferenceTracker a GC could destroy this on the
    // finalizer thread, which is why a guard used to be needed.)
    //
    // The weak alive-flag is still held: the destructor resets it first, so a notification that
    // races teardown becomes a no-op under the weak lock -- GC / re-entrancy safety, not threading.
    winrt::event_token m_groupedRowsChangedToken{};
    winrt::event_token m_flatRowsChangedToken{};
    winrt::event_token m_hierarchicalRowsChangedToken{};
    std::shared_ptr<bool> m_alive{ std::make_shared<bool>(true) };
};

}
