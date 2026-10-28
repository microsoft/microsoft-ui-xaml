// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include <functional>
#include <memory>

#include "TableViewSource6.g.h"
#include "ITableViewSource.h"
#include "TableViewRowInfo.h"

class ShapedItemsSource6;

// A TableViewSource variant whose rows are identified by object identity and kept in the model's
// own hierarchy (ChildrenWith). The presented rows are never flattened: ShapedItemsSource6 keeps
// one wrapper per item with its visible-subtree count, and an index is resolved by walking down
// the tree. ParentBy and an app-supplied row key are not supported.
class TableViewSource6 :
    public ReferenceTracker<TableViewSource6, winrt::implementation::TableViewSource6T, winrt::composable, winrt::composing, ::ITableViewSource>
{
public:
    ~TableViewSource6();
    explicit TableViewSource6(winrt::IInspectable const& items);

    static winrt::TableViewSource6 From(winrt::IInspectable const& items);
    static winrt::TableViewSource6 From(winrt::IInspectable const& items, winrt::TableViewIdentitySelector const& keySelector);

    winrt::ITableViewShapingSource Filter(winrt::TableViewPredicate const& predicate);
    winrt::ITableViewShapingSource GroupBy(winrt::TableViewKeySelector const& key);
    winrt::ITableViewShapingSource GroupBy(winrt::TableViewKeySelector const& key, winrt::TableViewIdentitySelector const& groupIdentitySelector);
    winrt::ITableViewShapingSource ParentBy(winrt::TableViewIdentitySelector const&) { throw winrt::hresult_not_implemented(); }
    winrt::ITableViewShapingSource ParentBy(winrt::TableViewKeySelector const&, winrt::TableViewKeySelector const&) { throw winrt::hresult_not_implemented(); }
    winrt::ITableViewShapingSource Sort(winrt::TableViewKeySelector const& key, winrt::SortDirection direction);
    winrt::ITableViewShapingSource Sort(winrt::hstring const& sortMemberPath, winrt::SortDirection direction);
    winrt::ITableViewShapingSource ClearFilter();
    winrt::ITableViewShapingSource ClearGroupBy();
    winrt::ITableViewShapingSource ClearParentBy() { throw winrt::hresult_not_implemented(); }
    winrt::ITableViewShapingSource ClearSort();
    winrt::ITableViewShapingSource ChildrenWith(winrt::TableViewChildrenSelector const& childrenSelector);
    winrt::ITableViewShapingSource ChildrenWith(winrt::TableViewChildrenSelector const& childrenSelector, winrt::TableViewHasChildrenPredicate const& hasChildrenSelector);
    winrt::ITableViewShapingSource ClearChildrenWith();

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
    bool IsGrouped() const override;
    bool ContainsShapedItem(winrt::IInspectable const& item) const override;
    bool CanApplySort() const override { return true; }

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
    void SetRowsMetadataChangedHook(std::function<void()> hook) override;

private:
    void NotifyOwnerProjectionChanged();
    void NotifyOwnerShapingChanged(bool reorderOnly);

    std::shared_ptr<ShapedItemsSource6> m_engine{};
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
    tracker_ref<winrt::ItemsSourceView> m_itemsSourceView{ this };
    TableViewRowMetadataProvider m_rowMetadata{};
};
