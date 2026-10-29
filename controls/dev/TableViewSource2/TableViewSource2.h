// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "pch.h"
#include "common.h"

#include "TableViewSource2.g.h"
#include "Shaping/ShapingManager.h"

// TiP observation of the public shaping verbs (internal, not in IDL; drift #63). The owner maps
// these to its TiP operations; the source layer never names TableViewTip.
enum class ShapingVerbKind : uint8_t
{
    Filter,     // Filter, ClearFilter
    Group,      // GroupBy, ClearGroupBy
    Sort,       // Sort (all three overloads), ClearSort
};

struct ShapingVerbOutcome
{
    bool Completed{};       // The verb returned normally.
    bool Committed{};       // It changed the applied spec (not deferred, not a re-declaration).
    bool OwnerChanged{};    // The owner that registered the observer let go while it ran.
    bool InvalidArgument{}; // It threw E_INVALIDARG (app data, as v1 AppFaultKind::Data).
    std::optional<HRESULT> Error;
};

struct ShapingVerbObserver
{
    // Returns the owner's operation generation (0: not measured). `meaningful` is false for a
    // Clear* verb with nothing to clear.
    std::function<uint64_t(ShapingVerbKind, bool meaningful)> Begin;
    std::function<void(ShapingVerbKind, uint64_t generation, ShapingVerbOutcome const&)> End;
};

// Scaffold for the shaping redesign. Exposes the same fluent verb surface as TableViewSource;
// From() constructs a working instance. Shaping state lives in ShapingManager (the coordinator);
// this class validates arguments and forwards.
//
// Reference-tracked for the same reasons as TableViewSource: tracker_ref members let the GC walk
// references to app-supplied objects. final_release (ReferenceTracker) posts the delete to the
// constructing thread's DispatcherQueue when it has one. From() called off a dispatcher thread
// (Task.Run) captures no queue, so the delete, and the Clear() in ~TableViewSource2(), run on
// the releasing thread. OPEN: for a table bound later by SetOwner that thread may not be the
// owner's; marshal teardown to SourceTable's bound queue, or require From() on the UI thread.
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

    // Internal (not in IDL): true while a commit applies. DeferRefreshHandle::Close throws then,
    // before it marks itself closed, so the handle can still be closed after the apply.
    bool IsApplying() const noexcept;

    // Forwards to ShapingManager, which forwards to SourceTable (owner of the flag and the
    // per-item subscriptions). The setter throws while an apply runs (#19).
    bool IsLiveShaping() const;
    void IsLiveShaping(bool value);

    // Internal (not in IDL): the applied spec, read-only, for column sort indicators (replaces v1
    // ActiveSortAxisInfos). Forwards to ShapingManager::AppliedSpec(). nullptr after teardown.
    ShapeSpec const* AppliedSpec() const noexcept;

    // Internal (not in IDL): the ItemsSourceView TableView binds its rows repeater to, as v1
    // TableViewSource::GetItemsSourceView(). Wraps ShapingManager::View() in a winrt::ItemsSourceView
    // created once and cached in m_itemsSourceView (rebuilt if the view collection is ever
    // recreated). nullptr after teardown.
    winrt::ItemsSourceView GetItemsSourceView();

    // Internal (not in IDL): the row metadata provider for TableView, as v1
    // TableViewSource::GetRowMetadata(). Forwards to ShapingManager::RowMetadata(). nullptr after
    // teardown.
    TableViewRowMetadataProvider GetRowMetadata();

    // Internal (not in IDL): replaces v1 ContainsShapedItem. Forwards to
    // ShapingManager::IsItemInShapedTree (in the source and in the shaped tree, shown or not).
    // TableView reaches shaping state only through narrow queries like this, never SourceTable.
    // False after teardown.
    bool IsItemInShapedTree(winrt::IInspectable const& item) const;

    // ---- Owner (internal, not in IDL; #18). Arbitration lives here (decided, B2) ------------
    // Which TableView owns this source is a question about the WinRT object, not about shaping, so
    // the single-owner rule and the handoff queue are kept here, and ShapingManager only receives
    // the outcome through ShapingManager::SetOwner(sink). Together these replace v1
    // SetOwningTableView / ReleaseOwningTableView / Add / RemoveOwnerReleasedHandler and the
    // separate handler setters (O1). No-ops after teardown; TrySetOwner then returns false and
    // AddOwnerReleasedWaiter drops the waiter.

    // Claims ownership (O7, handoff: v1 model). Single owner: returns false, without throwing,
    // when a different owner is registered; a refused owner is normal and transient (a navigation
    // loads the new page before it unloads the old one), so the caller renders empty and waits
    // with AddOwnerReleasedWaiter. The same owner again is an idempotent true. A successful claim
    // calls m_shapingManager->SetOwner(owner) first (it throws while an apply runs, #19, before
    // anything here changes), then sets m_owner. It does not remove waiters: the caller must call
    // RemoveOwnerReleasedWaiter (TableView: StopAwaitingSource2) before TrySetOwner. Raw pointer:
    // the owner calls ReleaseOwner before it is destroyed.
    bool TrySetOwner(IShapingOwner* owner);

    // Releases ownership. A no-op unless owner is m_owner, so a refused table cannot detach the
    // real one. Calls m_shapingManager->SetOwner(nullptr) (drops the owner's live-shaping holds;
    // never throws, even from inside a commit), then clears m_owner. Last, runs the waiters (see
    // RunOwnerReleasedWaiters); a release from inside a commit defers them to the commit's end,
    // where their claims can succeed.
    void ReleaseOwner(IShapingOwner* owner);

    // A refused owner registers to hear when the owner lets go. One entry per waiter (re-adding
    // replaces it); the waiter is held weakly. Remove also prunes collected waiters. The handler
    // runs on the UI thread from ReleaseOwner and should re-check that it still wants the source
    // (v1: IsLoaded, then reclaim).
    void AddOwnerReleasedWaiter(winrt::IInspectable const& waiter, std::function<void()> handler);
    void RemoveOwnerReleasedWaiter(winrt::IInspectable const& waiter);

    // Counted live-shaping hold (O4). Forward to ShapingManager, which owns the drain; replaces v1
    // SetLiveShapingHold and ResumeHeldLiveShaping. Begin returns the owner generation to pass to
    // End; holds are dropped when the owner changes, and an End from an earlier owner is ignored
    // (drift #48). Begin returns 0 and End is a no-op after teardown.
    uint64_t BeginLiveShapingHold() noexcept;
    void EndLiveShapingHold(uint64_t generation);

    // TiP (internal, drift #63; replaces v1 SetShapingTipHandlers). Brackets every public
    // Filter / GroupBy / Sort / Clear* call that passes argument validation. Ignored unless owner
    // is the registered owner; ReleaseOwner drops it (detach). Pass nullptr to clear.
    void SetShapingVerbObserver(IShapingOwner* owner, std::shared_ptr<ShapingVerbObserver> observer);

private:
    // Runs apply between the observer's Begin and End (a no-op wrapper without an observer).
    template <class Apply>
    void ObserveVerb(ShapingVerbKind kind, bool meaningful, Apply&& apply);

    // Runs the waiters FIFO while no owner is registered: each is popped before it runs (it
    // re-enters TrySetOwner, and one that does not claim is not retried), and collected waiters
    // are skipped. A waiter may claim synchronously; the remaining ones stay queued. Called by
    // ReleaseOwner outside a commit, and by ShapingManager's commit-ended handler.
    void RunOwnerReleasedWaiters();

    // The coordinator: owns SourceTable (which holds the app's source collection), the desired
    // spec, the delegate registry, and later the engine (with its ShapedTree) and View. Its
    // tracker_refs are bound to this, so ~TableViewSource2() calls m_shapingManager->Clear() while
    // this object is still alive. Before that, the destructor nulls m_owner and clears
    // m_ownerReleasedWaiters without running them (no callbacks during teardown).
    std::unique_ptr<ShapingManager> m_shapingManager;

    // Cache for GetItemsSourceView(). A tracker_ref bound to this, like the other framework/app
    // objects this source holds: the ItemsSourceView holds our view collection, which holds app
    // items, so the GC must be able to walk it. std::optional because ~tracker_ref dereferences
    // the owner. m_itemsSourceViewCollection is the collection it wraps (identity only, never
    // dereferenced; the wrapper keeps it alive), so a recreated view invalidates the cache.
    // Reset in ~TableViewSource2 without get().
    std::optional<tracker_ref<winrt::ItemsSourceView>> m_itemsSourceView;
    void* m_itemsSourceViewCollection{ nullptr };

    // The registered owner (#18 O1, O7), or nullptr. Not owned; the owner releases it before it
    // dies. ShapingManager holds the same pointer as its notification sink.
    IShapingOwner* m_owner{ nullptr };

    // The owner's TiP observer (SetShapingVerbObserver), or null. Copied by ObserveVerb, so a
    // release during the verb still pairs Begin with End.
    std::shared_ptr<ShapingVerbObserver> m_verbObserver;

    // Tables refused by TrySetOwner, waiting for ReleaseOwner (handoff, v1 OwnerReleasedWaiter).
    // FIFO. Waiter is weak so a waiting table that goes away is skipped and pruned. Not
    // tracker_refs: a weak_ref and a native std::function hold no strong app reference for the GC
    // to walk.
    struct OwnerReleasedWaiter
    {
        winrt::weak_ref<winrt::IInspectable> Waiter;
        std::function<void()> Handler;
    };
    std::vector<OwnerReleasedWaiter> m_ownerReleasedWaiters;
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
