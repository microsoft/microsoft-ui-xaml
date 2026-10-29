// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "TableViewSource2.g.h"
#include "Shaping/ShapingManager.h"

// Scaffold for the shaping redesign. Exposes the same fluent verb surface as TableViewSource;
// From() constructs a working instance, and every shaping verb throws hresult_not_implemented
// until the new engine is wired in. Shaping state lives in ShapingManager (the coordinator);
// this class validates arguments and forwards.
//
// Reference-tracked for the same reasons as TableViewSource: final_release runs on the owning
// UI thread, and tracker_ref members let the GC walk references to app-supplied objects.
class TableViewSource2 :
    public ReferenceTracker<TableViewSource2, winrt::implementation::TableViewSource2T, winrt::composable, winrt::composing>
{
public:
    // Not activatable in IDL: instances are created only through From(items).
    explicit TableViewSource2(winrt::IInspectable const& items);
    ~TableViewSource2();

    static winrt::TableViewSource2 From(winrt::IInspectable const& items);

    winrt::TableViewSource2 Filter(winrt::TableViewPredicate const& predicate);
    winrt::TableViewSource2 GroupBy(winrt::TableViewKeySelector const& key, winrt::SortDirection groupDirection);
    winrt::TableViewSource2 Sort(winrt::hstring const& sortMemberPath, winrt::SortDirection direction);
    winrt::TableViewSource2 Sort(winrt::TableViewKeySelector const& key, winrt::SortDirection direction);
    winrt::TableViewSource2 Sort(winrt::TableViewComparer2 const& comparer);
    winrt::TableViewSource2 ParentBy(winrt::TableViewKeySelector const& key, winrt::TableViewKeySelector const& parentKey);
    winrt::TableViewSource2 ChildrenBy(winrt::TableViewChildrenSelector2 const& childrenSelector);
    winrt::TableViewSource2 ClearFilter();
    winrt::TableViewSource2 ClearGroupBy();
    winrt::TableViewSource2 ClearSort();
    winrt::TableViewSource2 ClearHierarchy();

    void Refresh();

    // Returns a new DeferRefreshHandle after ShapingManager::BeginDeferRefresh (decided, DeferRefresh
    // session). Throws while an apply runs (#19), or after teardown.
    winrt::Windows::Foundation::IClosable DeferRefresh();

    // Internal (not in IDL): called once by DeferRefreshHandle::Close. Forwards to
    // ShapingManager::EndDeferRefresh; no-op after teardown.
    void EndDeferRefresh();

    // Forwards to ShapingManager, which forwards to SourceTable (owner of the flag and the
    // per-item subscriptions).
    bool IsLiveShaping() const;
    void IsLiveShaping(bool value);

    // Internal (not in IDL): the row metadata provider for TableView, as v1
    // TableViewSource::GetRowMetadata(). Forwards to ShapingManager::RowMetadata(). nullptr after
    // teardown.
    TableViewRowMetadataProvider GetRowMetadata();

    // Internal (not in IDL): as v1 ContainsShapedItem. Forwards to ShapingManager::ContainsItem.
    // TableView reaches shaping state only through narrow queries like this, never SourceTable.
    // False after teardown.
    bool ContainsItem(winrt::IInspectable const& item) const;

    // Internal (not in IDL), for TableView (#18). Forward to ShapingManager:
    //   TrySetOwner: single owner (O7); false, not a throw, if another owner is registered.
    //   ReleaseOwner: releases it and its live-shaping holds, then runs the waiters (handoff).
    //   Add / RemoveOwnerReleasedWaiter: a refused table waits for the release (v1 model).
    //   Together these replace v1 SetOwningTableView / ReleaseOwningTableView /
    //   Add / RemoveOwnerReleasedHandler and the separate handler setters (O1).
    //   Begin / EndLiveShapingHold: counted hold (O4); replaces v1 SetLiveShapingHold and
    //     ResumeHeldLiveShaping.
    // No-ops after teardown; TrySetOwner then returns false and Add drops the waiter.
    bool TrySetOwner(IShapingOwner* owner);
    void ReleaseOwner(IShapingOwner* owner);
    void AddOwnerReleasedWaiter(winrt::IInspectable const& waiter, std::function<void()> handler);
    void RemoveOwnerReleasedWaiter(winrt::IInspectable const& waiter);
    void BeginLiveShapingHold() noexcept;
    void EndLiveShapingHold();

private:
    [[noreturn]] static void ThrowNotImplemented(wchar_t const* verb);

    // The coordinator: owns SourceTable (which holds the app's source collection), the desired
    // spec, the delegate registry, and later the engine (with its ShapedTree) and View. Its
    // tracker_refs are bound to this, so ~TableViewSource2() calls m_shapingManager->Clear() while
    // this object is still alive.
    std::unique_ptr<ShapingManager> m_shapingManager;
};

// The handle DeferRefresh() returns (decided, DeferRefresh session: IClosable, WPF nesting). Close
// calls ShapingManager::EndDeferRefresh once; a second Close is a no-op. Holds a weak reference, so
// an open handle does not keep the source alive; Close after the source is gone, or after
// teardown, is a no-op. Not reference-tracked: it holds no app object. UI-thread affine.
struct DeferRefreshHandle : winrt::implements<DeferRefreshHandle, winrt::Windows::Foundation::IClosable>
{
    explicit DeferRefreshHandle(winrt::weak_ref<winrt::TableViewSource2> source) noexcept;

    // Exceptions from app code during the outermost reshape propagate from here.
    void Close();

private:
    winrt::weak_ref<winrt::TableViewSource2> m_source;
    bool m_closed{ false };
};
