// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"

#include <optional>
#include <string_view>

#include "TableViewRowInfo.h"
#include "GroupedEntry.h"
#include "FlatGroupedSourceAdapter.h"

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
        FlatGroupedSourceAdapterPtr const& adapter = nullptr,
        ItemKeySelector const& itemKeySelector = {});
    static TableViewRowMetadataProvider CreateForGroupedRows(
        FlatGroupedSourceAdapterPtr const& adapter,
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
    };

    RowMetadataProvider(
        SourceKind sourceKind,
        winrt::ItemsSourceView const& flatRows,
        winrt::ItemsSourceView const& groupedRows,
        FlatGroupedSourceAdapterPtr const& groupedAdapter,
        ItemKeySelector const& itemKeySelector);

private:
    // Single implementation behind all six expand/collapse/toggle entry points. They differ only
    // in how the caller names the group (row key vs. the app's GroupBy key), so resolution stays
    // in the wrappers and the state change lives here exactly once. `desired` empty means toggle.
    // Returns the resulting expansion state; false when there is no group or no adapter.
    bool SetGroupExpandedCore(winrt::IInspectable const& group, std::optional<bool> desired);

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
    FlatGroupedSourceAdapterPtr m_groupedAdapter{};
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
    std::shared_ptr<bool> m_alive{ std::make_shared<bool>(true) };
};

}
