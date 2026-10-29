// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <exception>
#include <memory>
#include <optional>
#include <span>
#include <unordered_map>

#include "pch.h"
#include "common.h"

#include "Shaping/IShapingOwner.h"
#include "Shaping/IShapedExpansion.h"
#include "Source/SourceTable.h"
#include "Engine/ShapedTree.h"
#include "Engine/ShapingEngine.h"
#include "Spec/ShapeSpec.h"
#include "Spec/ShapeDelegates.h"
#include "Spec/PathEvaluator.h"
#include "View/ShapedCollectionView.h"
#include "View/ShapedRowMetadata.h"
#include "View/ShapedRowQueries.h"

// The shaping coordinator (declarations only). See
// docs/design-notes/TabularControls/Shaping-Architecture-Redesign.md, "6. Coordinator".
//
// Owns every shaping part behind TableViewSource2 and wires them together. TableViewSource2 stays
// a thin WinRT shell: it validates verb arguments and forwards here.
//
//   Owns:  SourceTable (records and source tree), the desired ShapeSpec, ShapeDelegates,
//          PathEvaluator, the group expansion model, the engine (applied spec, ShapedTree), the
//          read-only row queries (ShapedRowQueries), the visible list (ShapedCollectionView,
//          handed to TableView) and the row metadata provider (ShapedRowMetadata).
//   Runs:  verb -> Diff(applied, desired) -> engine apply, inside one commit scope. The tree
//          raises each row event as the engine makes the change (P8); this object is its
//          IShapedRowsListener and forwards each to the view. Local changes are surgical Link /
//          Unlink; reshapes go through Reapply() and raise a stable Reset ("Change pipeline").
//   Hears: SourceTable::SourceChanged and ItemChanged; it is their only subscriber.
//   Tells: the owner (IShapingOwner, #18) before a Reset and after each unit of work. Which
//          owner is TableViewSource2's decision (B2); it passes the result through SetOwner.
//
// Unlike v1 ShapedItemsSource it is plain C++, not the WinRT collection the repeater binds to.
// That collection is ShapedCollectionView, an ItemsSourceView subclass View() returns as is.
//
// Adapters see only what they use (decided): ShapedCollectionView holds the ShapedRowQueries
// (reads); ShapedRowMetadata holds the ShapedRowQueries and this object as IShapedExpansion (the
// four expansion actions, which need the commit scope). Neither can reach the verbs, Refresh or
// Clear.
//
// Reference tracking: holds no app object itself. SourceTable, ShapeDelegates and m_view bind their
// tracker_refs to m_trackerOwner (the owning TableViewSource2). Clear() must run while that owner
// is alive; ~TableViewSource2() calls it.
//
// Threading: UI-thread affine, like SourceTable.

class ShapingManager final : private IShapedRowsListener, private IShapedExpansion
{
public:
    // Creates the SourceTable over `items` (SourceTable::Create, initial load) and installs this
    // object as its SourceChanged and ItemChanged handler. Must be called on the UI thread.
    ShapingManager(ITrackerHandleManager const* trackerOwner, winrt::IInspectable const& items);
    ~ShapingManager();

    ShapingManager(ShapingManager const&) = delete;
    ShapingManager& operator=(ShapingManager const&) = delete;

    // ---- Spec verbs: edit m_desired, then apply synchronously ------------------------------
    // Arguments are already validated by TableViewSource2. App-code exceptions propagate (see
    // "Misbehaving records"). Called while an apply runs: throws InvalidOperationException.
    // While a DeferRefresh is open (m_deferDepth > 0) they edit m_desired only; the outermost
    // EndDeferRefresh applies it.

    void Filter(winrt::TableViewPredicate const& predicate);
    void GroupBy(winrt::TableViewKeySelector const& key, winrt::SortDirection groupDirection);
    void Sort(winrt::hstring const& sortMemberPath, winrt::SortDirection direction);
    void Sort(winrt::TableViewKeySelector const& key, winrt::SortDirection direction);
    void Sort(winrt::TableViewComparer2 const& comparer);
    void ClearFilter();
    void ClearGroupBy();
    void ClearSort();

    // ---- Hierarchy verbs: forward to SourceTable, which raises one Reset -------------------
    // While a DeferRefresh is open SourceTable holds resets, so it stores the selectors as pending
    // and builds them at the re-read in EndDeferRefresh.

    void ParentBy(winrt::TableViewKeySelector const& key, winrt::TableViewKeySelector const& parentKey);
    void ChildrenBy(winrt::TableViewChildrenSelector2 const& childrenSelector);
    void ClearHierarchy();

    // ---- Source -----------------------------------------------------------------------------

    // The only path that re-enumerates the source on request. Calls SourceTable::Refresh(), whose
    // Reset comes back through OnSourceChanged and runs Reapply() once. While a DeferRefresh is
    // open: sets m_refreshRequested only.
    void Refresh();

    // ---- DeferRefresh (decided) -------------------------------------------------------------
    // WPF model: defers reshapes and re-reads, not incremental source changes.
    // Begin: throws while an apply runs (#19). The outermost Begin (depth 0 -> 1) calls
    // SourceTable::BeginHoldResets: an incremental source change still reaches OnSourceChanged and
    // is shaped surgically with the applied spec and published at once; a source Reset is held.
    // The live-shaping drain is skipped while open (changes stay in m_liveDirty, as under a hold).
    // End: asserts depth > 0. Inner ends only decrement. The outermost end calls
    // SourceTable::EndHoldResets; if it reports a pending change, or m_refreshRequested is set, it
    // runs SourceTable::Refresh() (one re-read that also builds a pending hierarchy), whose Reset
    // goes to Reapply() with m_desired: one reshape, one Reset. Otherwise ApplyDesiredSpec()
    // (nothing when the spec did not change). Then PostLiveDrain if changes are pending. An
    // exception from app code propagates (applied spec stays old, m_desired is kept, as for a
    // verb); m_refreshRequested is cleared and the depth is 0 either way.
    void BeginDeferRefresh();
    void EndDeferRefresh();
    bool IsRefreshDeferred() const noexcept;

    bool IsLiveShaping() const;
    void IsLiveShaping(bool value);

    // ---- Owner sink (#18) -------------------------------------------------------------------
    // Arbitration (single owner, refusal, waiters, handoff) lives in TableViewSource2 (decided,
    // B2). This object only needs to know whom to notify and whose holds to count.

    // Sets the notification target: OnResetting / OnShapingApplied go to sink, or nowhere when it
    // is nullptr. Called only by TableViewSource2, after it has accepted a claim (sink = the new
    // owner) or a release (sink = nullptr). When sink differs from the current one, every
    // live-shaping hold is dropped (holds belong to the owner that took them): m_liveHoldCount = 0,
    // then PostLiveDrain if changes are pending. The same sink again is a no-op. Throws while an
    // apply runs (#19), before anything changes, so TableViewSource2 can call it before updating
    // its own state. Raw pointer: TableViewSource2 sets nullptr before the owner is destroyed.
    void SetOwner(IShapingOwner* sink);

    // Counted live-shaping hold (O4): while m_liveHoldCount > 0, DrainLiveChanges defers (changes
    // stay in m_liveDirty). The owner takes one per reason (cell edit, pointer press), so nested
    // reasons compose. End at count 0 asserts. Dropping to 0 calls PostLiveDrain if changes are
    // pending: never drains inline, since the owner is usually inside an input handler. Verbs and
    // Refresh are not held: they flush m_liveDirty synchronously as before.
    void BeginLiveShapingHold() noexcept;
    void EndLiveShapingHold();

    // ---- Output -----------------------------------------------------------------------------

    // The ItemsSourceView TableView binds to (as v1 TableViewSource::GetItemsSourceView()): the
    // ShapedCollectionView itself (D1), created on first call with &m_rows as its back-reference
    // (D2) and cached. Stays the same object for this coordinator's lifetime: reshapes raise
    // CollectionChanged on it, never replace it. Returns nullptr after Clear().
    winrt::ItemsSourceView View();

    // ---- Owner queries (decided: narrow queries, no Source() accessor) ----------------------

    // True when item (by object identity) is a record in the shaped tree, shown or not: it is in
    // the source and ShapingEngine::InShapedTree holds (passes the filter, or is an ancestor of a
    // match). A record under a collapsed group or parent counts. False when the item is not in the
    // source, is excluded, is a group header value, is nullptr, or after Clear().
    // SourceTable::TryFindByItem, then ShapingEngine::InShapedTree. Reads the tree as it is, so
    // inside a Reset or OnShapingApplied it already answers for the new shape (P8). Replaces v1
    // ContainsShapedItem; renamed because "contains" did not say which of "in the source",
    // "in the shaped tree" or "visible" it answers. Caller: TableView::TerminateEditForReset
    // (false = the edited item left). Visible position is ShapedRowQueries::IndexOfItem.
    // Forwards to m_rows.IsItemInShapedTree.
    bool IsItemInShapedTree(winrt::IInspectable const& item) const;

    // The row metadata provider TableView reads (as v1 TableViewSource::GetRowMetadata()):
    // the ShapedRowMetadata adapter (R1), created on first call with &m_rows and this object as
    // IShapedExpansion as its back-references (R4) and cached. Same object for this coordinator's
    // lifetime. nullptr after Clear().
    std::shared_ptr<ITableViewRowMetadataProvider> RowMetadata();

    // ---- Lifetime ---------------------------------------------------------------------------

    // Teardown: m_owner = nullptr and m_liveHoldCount = 0 (no callbacks during teardown;
    // TableViewSource2 has already dropped its waiters), then ShapedCollectionView::Detach() (it
    // holds a raw ShapedRowQueries*), then ShapedRowMetadata::Detach() (it holds that and a raw
    // IShapedExpansion*), then ShapingEngine::Clear() (ShapedTree::Clear, which drops the
    // listener, the cursor and every group; then tracked keys), then SourceTable::Clear() (detach,
    // handlers nulled, records destroyed), ShapeDelegates::Clear(), PathEvaluator::Clear(),
    // m_view.reset(), m_rowMetadata.reset(). m_rows stays (a member); every query on it then
    // reports "not found". Must run on the UI thread while m_trackerOwner is alive. Idempotent.
    void Clear();

private:
    // SourceTable callbacks.
    void OnSourceChanged(SourceChange const& change);
    // Live shaping (decided, E7: coalesce per dispatcher turn). Never shapes inside the app's
    // setter: adds (record, propertyName) to m_liveDirty (a second, different name for the same
    // record becomes the empty name, "re-read everything") and posts one DrainLiveChanges.
    // record == nullptr (a dropped off-thread change) sets m_liveAllDirty instead. Ignored when no
    // verb is active.
    void OnItemChanged(SourceRecord* record, winrt::hstring const& propertyName);

    // Posts DrainLiveChanges to the current DispatcherQueue unless one is already posted
    // (m_liveDrainPosted). The callback holds a weak reference to the owning TableViewSource2 and
    // returns if it is gone.
    void PostLiveDrain();

    // The posted callback, and the synchronous flush before a verb. One commit:
    //   m_liveAllDirty           -> Reapply();
    //   one entry                -> m_engine.ApplyItemChanged (surgical);
    //   more than c_bulkThreshold -> m_engine.ApplyItemsChanged (re-read dirty records only, then
    //                               one structural rebuild and a stable Reset).
    // Moves m_liveDirty into a local batch and clears m_liveAllDirty and m_liveDrainPosted before
    // any shaping, so a re-entrant OnItemChanged (app code run by the drain) lands in the fresh
    // map and posts the next drain. Skipped while m_liveHoldCount > 0 (#18 O4:
    // an in-progress cell edit or pointer press, v1 m_liveShapingHold) or a DeferRefresh is open;
    // EndLiveShapingHold and the outermost EndDeferRefresh post it again.
    void DrainLiveChanges();

    // Applies Diff(applied, desired); then Retain()s the delegates and paths both specs use. A
    // sort-only Diff (sort or group direction changed, nothing else) adds ShapingChangeFlags::Order
    // to the Reset it raises; any other reshape adds Membership (#18 O5).
    void ApplyDesiredSpec();

    // Rebuilds the shaped tree from SourceTable::Records() with the desired spec. Never reads
    // the source. Used for a SourceTable Reset, a Diff that falls back to a full rebuild, and
    // dropped-ItemChanged recovery.
    void Reapply();

    // ---- IShapedRowsListener (decided, P8) --------------------------------------------------
    // The single path from the tree to consumers, whatever made the change (an engine commit, an
    // expansion verb, or an expand-all verb). Each call runs while the engine or tree is mid-write,
    // so all are noexcept: a throw from a handler is caught into m_listenerFault, every later event
    //     of the commit is suppressed, EndCommit raises one Reset, and AfterCommit rethrows. Item lists hold
        // count nullptrs, as InspectingDataSource's do (D3); Replace is never raised (O6).

    // ShapedCollectionView::NotifyRemoved / NotifyAdded. Accumulates m_changes: isMove sets Order
    // and adds +-count to m_moveNet; any other call sets Membership.
    void OnRowsRemoved(uint32_t index, uint32_t count, bool isMove) noexcept override;
    void OnRowsInserted(uint32_t index, uint32_t count, bool isMove) noexcept override;

    // m_owner->OnResetting() (anchors, O2 / P5: the tree still holds the old rows).
    void OnRowsResetting() noexcept override;

    // ShapedCollectionView::NotifyReset; sets ShapingChangeFlags::Reset.
    void OnRowsReset() noexcept override;

    // ---- IShapedExpansion (driven by ShapedRowMetadata only) -------------------------------
    // Contracts on IShapedExpansion. Each runs inside a commit scope (#19: throws while an apply
    // runs), whose events go out as they happen (P8).

    bool SetExpanded(ShapedNode* node, std::optional<bool> desired) override;
    void SetAllExpanded(ExpansionAxis axis, bool expanded) override;
    void ExpandSubtree(ShapedNode* node) override;
    std::optional<uint32_t> Reveal(ShapedNode* node) override;

    // ---- Commit scope (#19, P8) -------------------------------------------------------------
    // Every engine entry point and expansion verb runs between these, through one RAII scope:
    //   { CommitScope scope{ *this, retired }; m_engine.Apply(...); }  // ~CommitScope: EndCommit
    //   AfterCommit();                                                 // only on normal exit
    // BeginCommit: m_isApplying, SourceTable::BeginFreeze, m_changes / m_moveNet / m_listenerFault
    // / m_resyncDue cleared.
    //
    // EndCommit runs from the scope's destructor (also on unwind), so it never throws: a throw
    // from a destructor terminates the process. It only closes the scope:
    //   - m_moveNet != 0 -> Membership (rows moved and the count changed: not a pure reorder).
    //   - m_listenerFault set -> NotifyReset, so the view and the repeater agree again.
    //   - m_owner->OnShapingApplied({m_changes, retired}) if m_changes != None or retired is
    //     non-empty (O2, O3, O5, P9). Skipped on unwind from an engine throw (nothing was written).
    //     A throw from it is caught into m_listenerFault (if still empty).
    //   - ShapingEngine::ReleaseRetired, SourceTable::EndFreeze, m_isApplying cleared.
    //   - EndFreeze reported a pending change: on unwind, ResyncSource(post: true) (AfterCommit
    //     will not run, and nothing runs while the exception unwinds into the app); otherwise
    //     m_resyncDue is set.
    // On unwind a recorded m_listenerFault is dropped: the engine's exception propagates.
    // retired: the SourceChange's Removed records when called from OnSourceChanged (alive until
    // that handler returns); empty for verbs, expansion and live shaping. No owner: the owner
    // calls are skipped; no view yet: the tree has no listener and nothing is raised.
    void BeginCommit();
    void EndCommit(std::span<SourceRecord const* const> retired, bool unwinding) noexcept;

    // Called by the verb or handler that opened the scope, after the scope object is gone, when no
    // exception escaped it. Takes and clears m_resyncDue and m_listenerFault, then:
    //   - m_resyncDue: ResyncSource(post: m_listenerFault set), so an app exception is never
    //     followed by a synchronous re-read before it reaches the app.
    //   - Rethrows m_listenerFault (the first handler exception).
    void AfterCommit();

    ITrackerHandleManager const* m_trackerOwner{ nullptr };

    // Shared only so event handlers (item PropertyChanged, child feeds via WeakHost()) can capture
    // it weakly; nothing outside this object holds it. Clear() detaches it.
    std::shared_ptr<SourceTable> m_sourceTable;

    // What the verbs have declared. The applied copy lives in the engine (Phase 4).
    ShapeSpec m_desired;
    ShapeDelegates m_delegates;
    PathEvaluator m_paths;

    // Cached result of View(): the ShapedCollectionView (the listener reaches it through
    // winrt::get_self<ShapedCollectionView>). Tracked, because TableView holds it too. Null until
    // the first View(), which also calls m_engine.Tree().SetListener(this): until then nothing is
    // raised. std::optional because ~tracker_ref
    // dereferences the owner; Clear() detaches and resets it while the owner is alive.
    std::optional<tracker_ref<winrt::ItemsSourceView>> m_view;

    // Cached result of RowMetadata() (R4). Plain shared_ptr: it holds no app object, and TableView
    // keeps its own shared_ptr. Null until the first RowMetadata(); Clear() detaches and resets it.
    std::shared_ptr<ShapedRowMetadata> m_rowMetadata;

    // Notification sink set by SetOwner (#18 O1), or nullptr. Not owned. Mirrors
    // TableViewSource2::m_owner; arbitration is there (B2).
    IShapingOwner* m_owner{ nullptr };

    // Live-shaping holds taken by the owner (#18 O4). Reset to 0 when SetOwner changes the sink,
    // and by Clear().
    uint32_t m_liveHoldCount{ 0 };

    // A source notification with more than this many items (a range event, or an insert adopting
    // several orphans under ParentBy) goes through Reapply() and one stable Reset instead of
    // surgical Link / Unlink. Descendants carried by an item do not count. 1 matches v1 ShapedItemsSource.
    static constexpr uint32_t c_bulkThreshold = 1;

    // Re-entrancy guard: a verb or Refresh while an apply runs throws.
    bool m_isApplying{ false };

    // Open DeferRefresh handles (WPF nesting). While > 0, verbs edit m_desired only, Refresh sets
    // m_refreshRequested, SourceTable holds resets and the live drain is skipped. Clear() resets both,
    // so a handle closed after teardown does nothing.
    uint32_t m_deferDepth{ 0 };
    bool m_refreshRequested{ false };

    // Source change during an apply (decided, #19: freeze, then one deferred Reset). Every engine
    // entry point and expansion verb runs inside one commit scope (BeginCommit / EndCommit). A
    // source change raised by app code inside it (a predicate, comparer or key selector, or a
    // binding, converter or element factory in an event handler) is not applied to the table, so
    // records and links never change under the engine or the tree. When EndFreeze reports a pending
    // change, ResyncSource runs SourceTable::Refresh(), whose Reset goes to Reapply().
    // It runs synchronously once, from AfterCommit (never from the scope's destructor); it is
    // posted instead when the scope ended by an exception, when a handler faulted, or when the
    // resync's own apply was disturbed again (one resync per turn, so an app that mutates on every
    // callback cannot loop).
    // Never throws: Refresh() runs app code and validation, so a throw is caught, the table stays
    // as it was (SourceTable keeps m_pendingReset / m_outOfSync), and one retry is posted
    // (m_resyncPosted); the verb that opened the scope has already succeeded. A posted retry that
    // throws is swallowed; the next source notification, Refresh or verb retries.
    void ResyncSource(bool post) noexcept;
    bool m_resyncPosted{ false };

    // Live-shaping changes pending for the next DrainLiveChanges (E7). Keys are raw record
    // pointers: OnSourceChanged erases removed records and their descendants before the engine
    // applies the change, a SourceTable Reset or any Reapply empties the map (it re-reads every
    // record), and a verb drains it synchronously before applying, so a verb never runs over stale
    // keys. Clear() empties it, so a drain posted before teardown is a no-op.
    std::unordered_map<SourceRecord*, winrt::hstring> m_liveDirty;
    bool m_liveAllDirty{ false };     // ItemChanged(nullptr): the drain does Reapply()
    bool m_liveDrainPosted{ false };

    // Source tree + applied spec -> shaped tree (ShapedTree, P10). Every entry point is bracketed
    // here by the commit scope. Built in the constructor's member-init list after m_sourceTable is
    // created (it binds to *m_sourceTable), with m_trackerOwner, m_delegates and m_paths. Group
    // expansion lives in its ShapedTree (reached through Tree()).
    ShapingEngine m_engine;

    // Read-only queries over m_engine and *m_sourceTable, handed to the adapters (decided). Declared
    // after m_engine and bound in the member-init list. The adapters hold a raw pointer to it,
    // nulled by their Detach() in Clear().
    ShapedRowQueries m_rows;

    // The current commit (P8): what its events did, the net row count of its move events, and the
    // first exception a handler threw. Cleared by BeginCommit, read by EndCommit; m_listenerFault is
    // rethrown by AfterCommit.
    ShapingChangeFlags m_changes{ ShapingChangeFlags::None };
    int64_t m_moveNet{ 0 };
    std::exception_ptr m_listenerFault;

    // EndFreeze reported a source change during a commit that ended normally; AfterCommit resyncs.
    bool m_resyncDue{ false };

};
