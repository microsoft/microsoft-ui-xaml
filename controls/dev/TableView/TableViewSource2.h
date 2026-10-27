// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include <functional>
#include <memory>

#include "TableViewSource2.g.h"
#include "ITableViewSource.h"
#include "TableViewRowInfo.h"

class ShapedItemsSource2;

// A TableViewSource variant whose rows are identified by an app-supplied key rather than by
// object address. Every shape (flat, grouped, hierarchical by parent key, and grouped over a
// hierarchy) is one pre-order row array in ShapedItemsSource2; this class translates the public
// verbs into that engine and publishes its rows plus a row-metadata provider to the owning
// TableView through ::ITableViewSource.
//
// Reference-tracked for the same reasons as TableViewSource: its teardown runs on the owning UI
// thread, and the GC can walk its references into app objects.
class TableViewSource2 :
    public ReferenceTracker<TableViewSource2, winrt::implementation::TableViewSource2T, winrt::composable, winrt::composing, ::ITableViewSource>
{
public:
    ~TableViewSource2();
    TableViewSource2(winrt::IInspectable const& items, winrt::TableViewIdentitySelector const& keySelector);

    static winrt::TableViewSource2 From(winrt::IInspectable const& items);
    static winrt::TableViewSource2 From(winrt::IInspectable const& items, winrt::TableViewIdentitySelector const& keySelector);

    winrt::ITableViewShapingSource Filter(winrt::TableViewPredicate const& predicate);
    winrt::ITableViewShapingSource GroupBy(winrt::TableViewKeySelector const& key);
    winrt::ITableViewShapingSource GroupBy(winrt::TableViewKeySelector const& key, winrt::TableViewIdentitySelector const& groupIdentitySelector);
    winrt::ITableViewShapingSource ParentBy(winrt::TableViewIdentitySelector const& parentSelector);
    winrt::ITableViewShapingSource Sort(winrt::TableViewKeySelector const& key, winrt::SortDirection direction);
    winrt::ITableViewShapingSource Sort(winrt::hstring const& sortMemberPath, winrt::SortDirection direction);
    winrt::ITableViewShapingSource ClearFilter();
    winrt::ITableViewShapingSource ClearGroupBy();
    winrt::ITableViewShapingSource ClearParentBy();
    winrt::ITableViewShapingSource ClearSort();
    // Optional ITableViewShapingSource verbs this variant does not support. Rows already carry a
    // key, so a hierarchy is declared by parent key alone (ParentByIdentity).
    winrt::ITableViewShapingSource ParentBy(winrt::TableViewKeySelector const&, winrt::TableViewKeySelector const&) { throw winrt::hresult_not_implemented(); }
    winrt::ITableViewShapingSource ChildrenWith(winrt::TableViewChildrenSelector const&) { throw winrt::hresult_not_implemented(); }
    winrt::ITableViewShapingSource ChildrenWith(winrt::TableViewChildrenSelector const&, winrt::TableViewHasChildrenPredicate const&) { throw winrt::hresult_not_implemented(); }
    winrt::ITableViewShapingSource ClearChildrenWith() { throw winrt::hresult_not_implemented(); }

    bool IsLiveShaping() const;
    void IsLiveShaping(bool value);

    // Internal sort commands for the owning control (see TableViewSource).
    void SortReplacing(winrt::hstring const& previousSortAxisToken, winrt::hstring const& sortAxisToken, winrt::TableViewKeySelector const& key, winrt::hstring const& sortMemberPath, winrt::SortDirection direction);
    void ClearSort(winrt::hstring const& sortAxisToken);
    void ClearSortsExcept(winrt::hstring const& sortAxisToken);

    // ::ITableViewSource
    void ReplaceSortAxis(winrt::hstring const& previousSortAxisToken, winrt::hstring const& sortAxisToken,
        winrt::TableViewKeySelector const& key, winrt::hstring const& sortMemberPath, winrt::SortDirection direction) override
    {
        SortReplacing(previousSortAxisToken, sortAxisToken, key, sortMemberPath, direction);
    }
    void ClearAllSortAxes() override { ClearSort(); }
    void ClearSortAxis(winrt::hstring const& sortAxisToken) override { ClearSort(sortAxisToken); }
    void ClearSortAxesExcept(winrt::hstring const& sortAxisToken) override { ClearSortsExcept(sortAxisToken); }
    std::vector<ActiveSortAxisInfo> ActiveSortAxisInfos() const override;

    winrt::ItemsSourceView GetItemsSourceView() override;
    TableViewRowMetadataProvider GetRowMetadata() override;
    // True when GroupBy is declared: every v2 projection of a declared group is grouped.
    bool IsGrouped() const override;
    bool ContainsShapedItem(winrt::IInspectable const& item) const override;
    // Every row has an identity and the engine sorts its own rows, so sorting always applies.
    bool CanApplySort() const override { return true; }

    bool HasFilter() const;
    bool HasGroupBy() const;
    bool HasParentBy() const;
    uint32_t SourceRowCount() const;
    uint32_t DataRowCount() const;

    bool SetOwningTableView(winrt::IInspectable const& owner) override;
    void ReleaseOwningTableView(winrt::IInspectable const& owner) override;
    bool IsOwnedByOther(winrt::IInspectable const& candidate) const override
    {
        auto const owner = m_owningTableView.get();
        return owner && owner != candidate;
    }
    void AddOwnerReleasedHandler(winrt::IInspectable const& waiter, std::function<void()> handler) override;
    void RemoveOwnerReleasedHandler(winrt::IInspectable const& waiter) override;
    void SetProjectionChangedHandler(std::function<void()> handler) override { m_projectionChanged = std::move(handler); }
    void SetShapingChangedHandler(std::function<void(bool)> handler) override { m_shapingChanged = std::move(handler); }
    void SetLiveShapingHold(std::function<bool()> isHeld) override;
    void ResumeHeldLiveShaping() override;
    void SetLiveItemChangedHook(std::function<bool(winrt::IInspectable const&, bool)> hook) override;
    void SetRowsReleasedHook(std::function<void(winrt::IInspectable const&, std::vector<winrt::IInspectable> const&)> hook) override;
    void SetLiveItemReshapeHandler(std::function<void()> handler) override;

private:
    void NotifyOwnerProjectionChanged();
    void NotifyOwnerShapingChanged(bool reorderOnly);

    std::shared_ptr<ShapedItemsSource2> m_engine{};
    winrt::weak_ref<winrt::IInspectable> m_owningTableView{ nullptr };
    std::function<void()> m_projectionChanged{};
    std::function<void(bool)> m_shapingChanged{};
    uint32_t m_notificationDepth{};
    struct OwnerReleasedWaiter
    {
        winrt::weak_ref<winrt::IInspectable> Waiter;
        std::function<void()> Handler;
    };
    std::vector<OwnerReleasedWaiter> m_ownerReleasedWaiters{};
    // One view and one provider for the engine's life: v2 never swaps its presented vector.
    tracker_ref<winrt::ItemsSourceView> m_itemsSourceView{ this };
    TableViewRowMetadataProvider m_rowMetadata{};
};
