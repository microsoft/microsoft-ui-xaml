// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include <functional>
#include <vector>

#include "TableViewRowInfo.h"

// The contract between TableView and the row source it is bound to: the queries and commands the
// control actually issues, and nothing of a source's own fluent shaping surface.
//
// A classic COM interface rather than a WinRT one, so it stays out of the public API and can carry
// native types (std::function hooks, the row-metadata provider). Any source object assigned to
// TableView.ItemsSource that implements it is bound directly; TableView finds it by QueryInterface,
// so several source implementations can be swapped under one TableView without the control naming
// any of them. All callers are in this DLL and on the owning UI thread.
struct __declspec(uuid("04ee5ded-4ad2-4678-8a98-675665560e71")) __declspec(novtable) ITableViewSource : ::IUnknown
{
    // One active sort axis, in precedence order. SortMemberPath is empty when the axis was declared
    // with a key selector no property path expresses.
    struct ActiveSortAxisInfo
    {
        winrt::hstring AxisToken;
        winrt::hstring SortMemberPath;
        winrt::SortDirection Direction{ winrt::SortDirection::None };
    };

    // --- Ownership: one TableView binds a source at a time ---
    // Returns false, and leaves the existing owner bound, when a different live TableView owns it.
    virtual bool SetOwningTableView(winrt::IInspectable const& owner) = 0;
    // Detaches `owner`; a no-op when another TableView owns this source.
    virtual void ReleaseOwningTableView(winrt::IInspectable const& owner) = 0;
    // True when a TableView other than `candidate` owns this source.
    virtual bool IsOwnedByOther(winrt::IInspectable const& candidate) const = 0;
    // A refused TableView waits here and is offered the source when its owner releases it.
    virtual void AddOwnerReleasedHandler(winrt::IInspectable const& waiter, std::function<void()> handler) = 0;
    virtual void RemoveOwnerReleasedHandler(winrt::IInspectable const& waiter) = 0;

    // --- Projection ---
    virtual winrt::ItemsSourceView GetItemsSourceView() = 0;
    virtual TableViewRowMetadataProvider GetRowMetadata() = 0;
    // True when the rows are actually projected as grouped rows (the effective shape, not intent).
    virtual bool IsGrouped() const = 0;
    // True when `item` (by object identity) is in the shaped, filtered rows, regardless of group or
    // node expansion.
    virtual bool ContainsShapedItem(winrt::IInspectable const& item) const = 0;

    // --- Owner notifications and hooks; installed on bind ---
    // Raised after a rebuild swapped the ItemsSourceView and row-metadata provider.
    virtual void SetProjectionChangedHandler(std::function<void()> handler) = 0;
    // Raised when a shaping verb rewrote the projection; true when only the order moved.
    virtual void SetShapingChangedHandler(std::function<void(bool)> handler) = 0;
    // The owner holds live restores while its cell editor is open and resumes them when it closes.
    virtual void SetLiveShapingHold(std::function<bool()> isHeld) = 0;
    virtual void ResumeHeldLiveShaping() = 0;
    // Lets the owner refresh a custom comparer's ranks as items change.
    virtual void SetLiveItemChangedHook(std::function<bool(winrt::IInspectable const&, bool)> hook) = 0;
    // Lets the owner drop comparer ranks for rows that left the source.
    virtual void SetRowsReleasedHook(std::function<void(winrt::IInspectable const&, std::vector<winrt::IInspectable> const&)> hook) = 0;
    // Lets the owner learn that live shaping reacted to an item change.
    virtual void SetLiveItemReshapeHandler(std::function<void()> handler) = 0;
    // Optional. Lets the owner restamp realized rows whose metadata a source patched in place,
    // without a collection change.
    virtual void SetRowsMetadataChangedHook(std::function<void()> /*hook*/) {}
    // Optional. Applies a change the source deferred to the next dispatcher turn, so an owner verb
    // that addresses rows by index or item acts on the current projection.
    virtual void FlushPendingChanges() {}

    // --- Column-header sort ---
    // False when sorting would fail because the rows include a null item or one object twice.
    virtual bool CanApplySort() const = 0;
    virtual std::vector<ActiveSortAxisInfo> ActiveSortAxisInfos() const = 0;
    // Replaces the axis named previousSortAxisToken (if any) with sortAxisToken.
    virtual void ReplaceSortAxis(winrt::hstring const& previousSortAxisToken, winrt::hstring const& sortAxisToken,
        winrt::TableViewKeySelector const& key, winrt::hstring const& sortMemberPath, winrt::SortDirection direction) = 0;
    virtual void ClearAllSortAxes() = 0;
    virtual void ClearSortAxis(winrt::hstring const& sortAxisToken) = 0;
    // Makes sortAxisToken the only sort axis.
    virtual void ClearSortAxesExcept(winrt::hstring const& sortAxisToken) = 0;
};

// The source interface on `source`, or null when it is null or implements none.
inline winrt::com_ptr<::ITableViewSource> TryGetTableViewSource(winrt::IInspectable const& source)
{
    return source ? source.try_as<::ITableViewSource>() : nullptr;
}

// The source interface on `source`, which must implement it.
inline winrt::com_ptr<::ITableViewSource> GetTableViewSource(winrt::IInspectable const& source)
{
    return source.as<::ITableViewSource>();
}
