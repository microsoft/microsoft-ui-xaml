// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

#include "pch.h"
#include "common.h"

#include "Shaping/IShapingOwner.h"
#include "Source/SourceTable.h"
#include "Engine/ShapingEngine.h"
#include "Spec/ShapeSpec.h"
#include "Spec/ShapeDelegates.h"
#include "Spec/PathEvaluator.h"
#include "View/Flattener.h"
#include "View/GroupExpansion.h"
#include "View/ShapedCollectionView.h"
#include "View/ShapedRowMetadata.h"

// The shaping coordinator (declarations only). See
// docs/design-notes/TabularControls/Shaping-Architecture-Redesign.md, "6. Coordinator".
//
// Owns every shaping part behind TableViewSource2 and wires them together. TableViewSource2 stays
// a thin WinRT shell: it validates verb arguments and forwards here.
//
//   Owns:  SourceTable (records and source tree), the desired ShapeSpec, ShapeDelegates,
//          PathEvaluator, the group expansion model, the flattener, the engine (applied spec,
//          shaped tree), the visible list (ShapedCollectionView, handed to TableView) and the row
//          metadata provider (ShapedRowMetadata).
//   Runs:  verb -> Diff(applied, desired) -> engine apply -> flat delta -> publish to the
//          visible list. Local changes go engine -> IShapedEditSink (m_flattener) as ShapedEdits;
//          reshapes go through Reapply() and publish a stable Reset ("Change pipeline").
//   Hears: SourceTable::SourceChanged and ItemChanged; it is their only subscriber.
//   Tells: the owner (IShapingOwner, #18) before a Reset and after each published unit of work.
//
// Unlike v1 ShapedItemsSource it is plain C++, not the WinRT collection the repeater binds to.
// That collection is ShapedCollectionView, an ItemsSourceView subclass View() returns as is; it
// answers every read through the visible-list helpers below.
//
// Reference tracking: holds no app object itself. SourceTable, ShapeDelegates and m_view bind their
// tracker_refs to m_trackerOwner (the owning TableViewSource2). Clear() must run while that owner
// is alive; ~TableViewSource2() calls it.
//
// Threading: UI-thread affine, like SourceTable.

// Which kind of node an expand-all / collapse-all acts on (R2). Opening every group and expanding
// every tree node are different user actions (ITableViewRowMetadataProvider has one verb per axis).
enum class ExpansionAxis : uint8_t
{
    Groups,   // group headers: GroupExpansion baseline
    Rows,     // records: ShapeFlags::Expanded / OverlayCollapsed
};

class ShapingManager
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
    // While a DeferRefresh is open the table is frozen, so SourceTable stores the selectors as
    // pending and builds them at the re-read in EndDeferRefresh.

    void ParentBy(winrt::TableViewKeySelector const& key, winrt::TableViewKeySelector const& parentKey);
    void ChildrenBy(winrt::TableViewChildrenSelector2 const& childrenSelector);
    void ClearHierarchy();

    // ---- Source -----------------------------------------------------------------------------

    // The only path that re-enumerates the source on request. Calls SourceTable::Refresh(), whose
    // Reset comes back through OnSourceChanged and runs Reapply() once. While a DeferRefresh is
    // open: sets m_refreshRequested only.
    void Refresh();

    // ---- DeferRefresh (decided) -------------------------------------------------------------
    // Begin: throws while an apply runs (#19). The outermost Begin (depth 0 -> 1) calls
    // SourceTable::BeginFreeze, so source changes are held and only set its pending reset (#19);
    // the live-shaping drain is skipped while open (changes stay in m_liveDirty, as under a hold).
    // End: asserts depth > 0. Inner ends only decrement. The outermost end calls
    // SourceTable::EndFreeze; if it reports a pending change, or m_refreshRequested is set, it
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

    // ---- Owner (#18) ------------------------------------------------------------------------

    // Claims ownership (O1, O7, handoff decided: v1 model). Single owner: returns false, without
    // throwing, when a different owner is registered; a refused owner is normal and transient (a
    // navigation loads the new page before it unloads the old one), so the caller renders empty
    // and waits with AddOwnerReleasedWaiter. The same owner again is an idempotent true. A
    // successful claim also removes that owner's waiter. Throws while an apply runs (#19). Raw
    // pointer: the owner calls ReleaseOwner before it is destroyed.
    bool TrySetOwner(IShapingOwner* owner);

    // Releases ownership. A no-op unless owner is the registered owner, so a refused table cannot
    // detach the real one. Releases every live-shaping hold the owner took (m_liveHoldCount = 0,
    // PostLiveDrain if changes are pending). Last, runs the waiters FIFO while no owner is
    // registered: each is popped before it runs (it re-enters TrySetOwner, and one that does not
    // claim is not retried), and collected waiters are skipped. A waiter may claim synchronously;
    // the remaining ones stay queued. Throws while an apply runs (#19).
    void ReleaseOwner(IShapingOwner* owner);

    // A refused owner registers to hear when the owner lets go. One entry per waiter (re-adding
    // replaces it); the waiter is held weakly. Remove also prunes collected waiters. The handler
    // runs on the UI thread from ReleaseOwner and should re-check that it still wants the source
    // (v1: IsLoaded, then reclaim).
    void AddOwnerReleasedWaiter(winrt::IInspectable const& waiter, std::function<void()> handler);
    void RemoveOwnerReleasedWaiter(winrt::IInspectable const& waiter);

    // Counted live-shaping hold (O4): while m_liveHoldCount > 0, DrainLiveChanges defers (changes
    // stay in m_liveDirty). The owner takes one per reason (cell edit, pointer press), so nested
    // reasons compose. End at count 0 asserts. Dropping to 0 calls PostLiveDrain if changes are
    // pending: never drains inline, since the owner is usually inside an input handler. Verbs and
    // Refresh are not held: they flush m_liveDirty synchronously as before.
    void BeginLiveShapingHold() noexcept;
    void EndLiveShapingHold();

    // ---- Output -----------------------------------------------------------------------------

    // The ItemsSourceView TableView binds to (as v1 TableViewSource::GetItemsSourceView()): the
    // ShapedCollectionView itself (D1), created on first call with *this as its back-reference
    // (D2) and cached. Stays the same object for this coordinator's lifetime: reshapes raise
    // CollectionChanged on it, never replace it. Returns nullptr after Clear().
    winrt::ItemsSourceView View();

    // ---- Visible list (read by ShapedCollectionView only) ---------------------------------
    // Every read the view makes goes through these, so the flattener, the engine, the SourceTable
    // and the mid-publish rule (V4) stay behind one object.

    // Flattener::Size(): the published state.
    uint32_t RowCount() const noexcept;

    // Flattener::GetAt(index); the view maps it to the row value and key. Throws E_BOUNDS.
    ShapedRef RowAt(uint32_t index) const;

    // Row key -> flat index (V3). A "g|" key: m_engine.GroupsByIdentity(). Otherwise a record key:
    // SourceRecord::TryParseIdentity, then SourceTable::TryFindByAddress (always on, so it works
    // without the identity index, D4). Then Flattener::IndexOf. Mid-publish (V4), IndexOf answers
    // for the final state, so the result is checked against m_rows and, if they disagree, m_rows
    // is searched. nullopt when unknown or hidden.
    std::optional<uint32_t> IndexOfKey(winrt::hstring const& key) const;

    // Row value -> flat index: a GroupHeaderRow by its Identity, else SourceTable::TryFindByItem.
    // Same IndexOf and mid-publish rule as IndexOfKey. nullopt when unknown or hidden.
    std::optional<uint32_t> IndexOfItem(winrt::IInspectable const& value) const;

    // ---- Owner queries (decided: narrow queries, no Source() accessor) ----------------------

    // True when item (by object identity) is a record in the shaped tree: it passes the filter,
    // whether or not it is shown (a collapsed group or parent still counts). SourceTable::TryFindByItem,
    // then ShapingEngine::InShapedTree. Header rows never match. As v1 ContainsShapedItem (the
    // owner checks that the edited item survived a reshape). False for nullptr or after Clear().
    bool ContainsItem(winrt::IInspectable const& item) const;

    // The row metadata provider TableView reads (as v1 TableViewSource::GetRowMetadata()):
    // the ShapedRowMetadata adapter (R1), created on first call with *this as its back-reference
    // (R4) and cached. Same object for this coordinator's lifetime. nullptr after Clear().
    std::shared_ptr<ITableViewRowMetadataProvider> RowMetadata();

    // ---- Row metadata (read and driven by ShapedRowMetadata only) --------------------------

    // TableViewRowInfo for RowAt(index), built on demand from the ShapedRef (spec field table):
    // Kind, Level (ShapedDepth), IsExpandable, IsExpanded (Flattener::IsExpanded), ChildCount,
    // ParentIdentity, PositionInSet (O(siblings)), SizeOfSet. Never stored. Mid-publish (V4) the
    // row comes from m_rows but per-node fields from the final tree; the RowStateEdits that follow
    // the splices bring the owner up to date. Throws E_BOUNDS.
    TableViewRowInfo RowInfo(uint32_t index) const;

    // Row key -> node, the first half of IndexOfKey (V3/D4 dispatch). Shown or not. nullopt when
    // unknown.
    std::optional<ShapedRef> TryResolveKey(winrt::hstring const& key) const;

    // Expand (true), collapse (false) or toggle (nullopt) one node: Flattener::Expand / Collapse,
    // then Publish, inside the #19 apply scope (throws while an apply runs). Returns whether the
    // state changed; false when node is not expandable. Typed, not keyed (decided, R2): keys stay
    // a TableView-interface concern. Per-item expand is internal only (decided, #20): a public
    // per-item API is a TableView API item (Phase 6) and will resolve to a ShapedRef and call this.
    bool SetExpanded(ShapedRef node, std::optional<bool> desired);

    // Flattener::ExpandAll* / CollapseAll* for the axis, then Publish (a Reset). Same scope rule.
    void SetAllExpanded(ExpansionAxis axis, bool expanded);

    // Flattener::ExpandSubtree, then Publish. No-op for a group or when !IsHierarchical(). Same
    // scope rule.
    void ExpandSubtree(ShapedRef node);

    // Reveal (decided, #20; internal, public API deferred to Phase 6): nullopt, with nothing
    // changed, when ShapingEngine::InShapedTree(node) is false (filtered out). Otherwise
    // Flattener::Reveal, then Publish (one splice, or nothing when already shown), and returns
    // Flattener::IndexOf(node). Same scope rule. Callers: a future ScrollIntoView(item), selection
    // by item, jumping to a filter match.
    std::optional<uint32_t> Reveal(ShapedRef node);

    // The source tree has a hierarchy (SourceTable's hierarchy kind is not None).
    bool IsHierarchical() const noexcept;

    // R3, v1 meaning: index is a data row; walk ShapedParent up to its bottom-level group (O(depth));
    // position = index - IndexOf(group) (1-based), size = Flattener::ShownDescendantCount(group).
    // False when ungrouped, a header, or out of range. Mid-publish: IndexOf is checked against
    // m_rows as in IndexOfKey.
    bool TryGetGroupMembership(uint32_t index, uint32_t& positionInGroup, uint32_t& groupSize) const;

    // ---- Lifetime ---------------------------------------------------------------------------

    // Teardown: m_owner = nullptr and m_ownerReleasedWaiters cleared without running them (no
    // callbacks during teardown), then
    // ShapedCollectionView::Detach() (it holds a raw ShapingManager*), then
    // ShapedRowMetadata::Detach() (same reason), then Flattener::Clear() (it holds raw record pointers), then ShapingEngine::Clear()
    // (drops groups and tracked keys), then SourceTable::Clear()
    // (detach, handlers nulled, records destroyed), ShapeDelegates::Clear(),
    // PathEvaluator::Clear(), m_view.reset(), m_rowMetadata.reset(). Must run on the UI thread while m_trackerOwner is alive. Idempotent.
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

    // The posted callback, and the synchronous flush before a verb. One commit, one Publish:
    //   m_liveAllDirty           -> Reapply();
    //   one entry                -> m_engine.ApplyItemChanged (surgical);
    //   more than c_bulkThreshold -> m_engine.ApplyItemsChanged (re-read dirty records only, then
    //                               one structural rebuild and a stable Reset).
    // Empties m_liveDirty and clears both flags first. Skipped while m_liveHoldCount > 0 (#18 O4:
    // an in-progress cell edit or pointer press, v1 m_liveShapingHold) or a DeferRefresh is open;
    // EndLiveShapingHold and the outermost EndDeferRefresh post it again.
    void DrainLiveChanges();

    // Applies Diff(applied, desired); then Retain()s the delegates and paths both specs use. A
    // sort-only Diff (sort or group direction changed, nothing else) adds ShapingChangeFlags::Order
    // to the Reset it publishes; any other reshape adds Membership (#18 O5).
    void ApplyDesiredSpec();

    // Rebuilds the shaped tree from SourceTable::Records() with the desired spec. Never reads
    // the source. Used for a SourceTable Reset, a Diff that falls back to a full rebuild, and
    // dropped-ItemChanged recovery.
    void Reapply();

    // The single path from a FlatDelta to consumers, whatever produced it (an engine commit,
    // Expand / Collapse, or an expand-all verb). Decided (V4, #18):
    //   - Surgical: for each splice in order, Flattener::ApplySplice, then
    //     ShapedCollectionView::NotifyRemoved (if RemovedCount) and NotifyAdded (if Inserted is
    //     non-empty). Item lists are CountOnlyItemList placeholders (D3); Replace is never raised (O6).
    //   - Bulk: m_owner->OnResetting() (old rows still readable, O2), Flattener::ApplyReset, then
    //     ShapedCollectionView::NotifyReset.
    //   - Then m_owner->OnShapingApplied({delta.Changes, delta.States, retired}) if the delta is
    //     non-empty or retired is non-empty (O2, O3, O5), then ShapingEngine::ReleaseRetired.
    // retired: the SourceChange's Removed records when called from OnSourceChanged (alive until
    // that handler returns); empty for verbs, expansion and live shaping.
    // Runs inside the #19 apply scope: a handler that calls a verb throws. An empty delta with no
    // retired records raises nothing. No owner: the owner calls are skipped.
    void Publish(FlatDelta const& delta, std::span<SourceRecord const* const> retired = {});

    ITrackerHandleManager const* m_trackerOwner{ nullptr };

    // Shared because consumers may hold it past this object's lifetime; Clear() detaches it.
    std::shared_ptr<SourceTable> m_sourceTable;

    // What the verbs have declared. The applied copy lives in the engine (Phase 4).
    ShapeSpec m_desired;
    ShapeDelegates m_delegates;
    PathEvaluator m_paths;

    // Cached result of View(): the ShapedCollectionView (Publish reaches it through
    // winrt::get_self<ShapedCollectionView>). Tracked, because TableView holds it too. Null until
    // the first View(): Publish then raises nothing. std::optional because ~tracker_ref
    // dereferences the owner; Clear() detaches and resets it while the owner is alive.
    std::optional<tracker_ref<winrt::ItemsSourceView>> m_view;

    // Cached result of RowMetadata() (R4). Plain shared_ptr: it holds no app object, and TableView
    // keeps its own shared_ptr. Null until the first RowMetadata(); Clear() detaches and resets it.
    std::shared_ptr<ShapedRowMetadata> m_rowMetadata;

    // The registered owner (#18 O1, O7), or nullptr. Not owned; the owner clears it before it dies.
    IShapingOwner* m_owner{ nullptr };

    // Tables refused by TrySetOwner, waiting for ReleaseOwner (handoff, v1 OwnerReleasedWaiter).
    // FIFO. Waiter is weak so a waiting table that goes away is skipped and pruned.
    struct OwnerReleasedWaiter
    {
        winrt::weak_ref<winrt::IInspectable> Waiter;
        std::function<void()> Handler;
    };
    std::vector<OwnerReleasedWaiter> m_ownerReleasedWaiters;

    // Live-shaping holds taken by the owner (#18 O4). Reset to 0 by ReleaseOwner and Clear().
    uint32_t m_liveHoldCount{ 0 };

    // A source notification with more than this many items (a range event, or an insert adopting
    // several orphans under ParentBy) goes through Reapply() and one stable Reset instead of
    // ShapedEdits. Descendants carried by an item do not count. 1 matches v1 ShapedItemsSource.
    static constexpr uint32_t c_bulkThreshold = 1;

    // Re-entrancy guard: a verb or Refresh while an apply runs throws.
    bool m_isApplying{ false };

    // Open DeferRefresh handles (WPF nesting). While > 0, verbs edit m_desired only, Refresh sets
    // m_refreshRequested, the table is frozen and the live drain is skipped. Clear() resets both,
    // so a handle closed after teardown does nothing.
    uint32_t m_deferDepth{ 0 };
    bool m_refreshRequested{ false };

    // Source change during an apply (decided, #19: freeze, then one deferred Reset). Every engine
    // entry point and its Publish run inside one scope: m_isApplying, SourceTable::BeginFreeze,
    // Flattener::BeginCommit, the engine call, EndCommit -> Publish, EndFreeze. A source change
    // raised by app code inside that scope (a predicate, comparer or key selector, or a binding,
    // converter or element factory during Publish) is not applied to the table, so records and
    // links never change under the engine or the flattener. When EndFreeze reports a pending
    // change, ResyncSource runs SourceTable::Refresh(), whose Reset goes to Reapply().
    // It runs synchronously once; it is posted instead when the scope ended by an exception, or
    // when the resync's own apply was disturbed again (one resync per turn, so an app that mutates
    // on every callback cannot loop).
    void ResyncSource(bool post);
    bool m_resyncPosted{ false };

    // Live-shaping changes pending for the next DrainLiveChanges (E7). Keys are raw record
    // pointers: OnSourceChanged erases removed records and their descendants before the engine
    // applies the change, a SourceTable Reset or any Reapply empties the map (it re-reads every
    // record), and a verb drains it synchronously before applying, so a verb never runs over stale
    // keys. Clear() empties it, so a drain posted before teardown is a no-op.
    std::unordered_map<SourceRecord*, winrt::hstring> m_liveDirty;
    bool m_liveAllDirty{ false };     // ItemChanged(nullptr): the drain does Reapply()
    bool m_liveDrainPosted{ false };

    // Group expansion, keyed by GroupNode::Identity. Read and written by m_flattener. Pruned
    // (decided, F4): after every engine commit whose delta is IsReset, this calls
    // RetainOnly(m_engine.GroupsByIdentity()); the flattener Erases on a surgical group drop;
    // cleared when GroupBy gets a new selector. Declared before m_flattener, which references it.
    GroupExpansion m_groupExpansion;

    // Shaped tree -> visible list; the engine's IShapedEditSink.
    Flattener m_flattener{ m_groupExpansion };

    // Source tree + applied spec -> shaped tree. Declared after m_flattener, its IShapedEditSink.
    // Every entry point is bracketed here: m_flattener.BeginCommit(), the engine call,
    // EndCommit() -> Publish, with a scope that calls AbortCommit() if the call throws.
    // Built in the constructor's member-init list after m_sourceTable is created (it binds to
    // *m_sourceTable), with m_trackerOwner, m_delegates, m_paths and m_flattener.
    ShapingEngine m_engine;

};
