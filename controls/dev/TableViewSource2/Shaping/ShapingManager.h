// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

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
//          PathEvaluator, the engine (applied spec, ShapedTree, which owns the group expansion
//          model), the read-only row queries (ShapedRowQueries), the visible list
//          (ShapedCollectionView, handed to TableView) and the row metadata provider
//          (ShapedRowMetadata).
//   Runs:  verb -> Diff(applied, desired) -> engine apply, inside one commit scope. The tree
//          raises each row event as the engine makes the change (P8); this object is its
//          IShapedRowsListener and forwards each to the view. Local changes are surgical Link /
//          Unlink; reshapes go through Reapply() and raise a stable Reset ("Change pipeline").
//   Hears: SourceTable::SourceChanged and ItemChanged; it is their only subscriber.
//   Tells: the owner (IShapingOwner, #18) before a Reset and after each unit of work. Which
//          owner is TableViewSource2's decision (B2); it passes the result through SetOwner.
//
// Unlike v1 ShapedItemsSource it is plain C++, not the WinRT collection the repeater binds to.
// That collection is ShapedCollectionView, which implements ItemsSourceView's default interface
// natively (decided, M16) and View() returns as is.
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
// Threading: follows SourceTable's binding (see SourceTable, Threading). Constructed on any thread
// (From() may run in Task.Run); after SetOwner binds the table, used only on the owner's thread.

class ShapingManager final : private IShapedRowsListener, private IShapedExpansion
{
public:
    // Creates the SourceTable over `items` (SourceTable::Create, initial load) and installs this
    // object as its SourceChanged and ItemChanged handler. May run off the UI thread, like
    // SourceTable::Create; the table stays unbound until SetOwner binds it.
    ShapingManager(ITrackerHandleManager const* trackerOwner, winrt::IInspectable const& items);
    ~ShapingManager();

    ShapingManager(ShapingManager const&) = delete;
    ShapingManager& operator=(ShapingManager const&) = delete;

    // ---- Spec verbs: edit m_desired, then apply synchronously ------------------------------
    // Arguments are already validated by TableViewSource2. App-code exceptions propagate (see
    // "Misbehaving records"). Called while an apply runs: throws InvalidOperationException
    // (BeginVerb). After Clear(): no-op. While a DeferRefresh is open (m_deferDepth > 0) they edit
    // m_desired only; the outermost EndDeferRefresh applies it.

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
    // open: SourceTable::ScheduleRefresh() only (a re-read is owed at the outermost
    // EndHoldResets; the table is not marked diverged, so incremental changes keep showing).
    void Refresh();

    // ---- DeferRefresh (decided) -------------------------------------------------------------
    // WPF model: defers reshapes and re-reads, not incremental source changes.
    // Begin and End: throw InvalidOperationException while an apply runs (#19; decided, M12),
    // before anything changes, so the caller's handle stays open and a later Close ends the
    // scope. TableViewSource2's handle marks itself closed before calling End, so an app exception
    // from the apply still leaves it closed; DeferRefresh() after Clear() throws; Close after the
    // source is gone is a no-op (those rules live in TableViewSource2). The outermost Begin (depth 0 -> 1) calls
    // SourceTable::BeginHoldResets: an incremental source change still reaches OnSourceChanged and
    // is shaped surgically with the applied spec and published at once; a source Reset is held.
    // The live-shaping drain is skipped while open (changes stay in m_liveDirty, as under a hold).
    // End: asserts depth > 0. Inner ends only decrement. The outermost end calls
    // SourceTable::EndHoldResets; if it reports a pending change (a held Reset, a Refresh() while
    // open, or a parked hierarchy), it runs SourceTable::Refresh() (one re-read that also builds a
    // pending hierarchy), whose Reset goes to Reapply() with m_desired: one reshape, one Reset.
    // Otherwise ApplyDesiredSpec() (nothing when the spec did not change and the engine is not
    // stale). Then PostLiveDrain if changes are pending. Decided (M11: all-or-nothing): an
    // exception from app code propagates; applied spec stays old and m_desired is kept, as for a
    // verb, so the next verb or Refresh applies the full Diff. A failed re-read leaves the table
    // diverged with nothing owed and a parked hierarchy dropped (SourceTable::Refresh, M18): the
    // next notification or Refresh re-reads. The depth is 0 either way.
    void BeginDeferRefresh();
    void EndDeferRefresh();
    bool IsRefreshDeferred() const noexcept;

    // The getter never throws. The setter forwards to SourceTable::SetLiveShaping; called while an
    // apply runs it throws InvalidOperationException (#19), like a verb. Decided (M10): it only
    // gates item changes from then on (OnItemChanged ignores them while off); changes already
    // queued are kept and the posted drain applies them. It never reshapes or re-reads, also
    // while a DeferRefresh is open (queued changes wait for the close, which posts the drain).
    bool IsLiveShaping() const;
    void IsLiveShaping(bool value);

    // The spec the shown tree reflects (m_engine.Applied()), read-only. Column sort indicators read
    // it, so an arrow never claims an order the rows do not have (spec, "Readers"; replaces v1
    // ActiveSortAxisInfos). Lags
    // m_desired while a DeferRefresh is open, and stays old when an apply throws. The object is
    // updated in place by each apply; copy what must survive the next one.
    ShapeSpec const& AppliedSpec() const noexcept;

    // ---- Owner sink (#18) -------------------------------------------------------------------
    // Arbitration (single owner, refusal, waiters, handoff) lives in TableViewSource2 (decided,
    // B2). This object only needs to know whom to notify and whose holds to count.

    // Sets the notification target: OnResetting / OnShapingApplied go to sink, or nowhere when it
    // is nullptr. Called only by TableViewSource2, after it has accepted a claim (sink = the new
    // owner) or a release (sink = nullptr). When sink differs from the current one, every
    // live-shaping hold is dropped (holds belong to the owner that took them): m_liveHoldCount = 0
    // and ++m_ownerGeneration, then PostLiveDrain if changes are pending. A non-null sink then calls
    // SourceTable::BindOwnerThread(), so a source built off the UI thread is bound to the owner's
    // thread once (see SourceTable, Threading). The same sink again is a no-op. Decided (M1): a
    // non-null sink throws while an apply runs (#19), before anything changes, so TableViewSource2
    // can call it before updating its own state; nullptr (a release) never throws, even
    // mid-commit: the owner must be able to let go from inside its own callbacks. Raw pointer:
    // TableViewSource2 sets nullptr before the owner is destroyed.
    void SetOwner(IShapingOwner* sink);

    // Runs at the end of every commit that completes (from AfterCommit, after m_isApplying is
    // cleared, so verbs and claims may run). TableViewSource2 uses it to run owner-released
    // waiters deferred by a release during a commit (M1). Its exception propagates after any
    // owner-callback fault. A commit unwound by an engine exception does not run it. Cleared by
    // Clear(). Captures only the native TableViewSource2, never an app object.
    void SetCommitEndedHandler(std::function<void()> handler) noexcept;

    // True while a commit is applying (between BeginCommit and EndCommit). TableViewSource2 reads
    // it to defer waiters and to reject a DeferRefresh Close mid-apply (M1, M12).
    bool IsApplying() const noexcept;

    // Counted live-shaping hold (O4): while m_liveHoldCount > 0, DrainLiveChanges defers (changes
    // stay in m_liveDirty). The owner takes one per reason (cell edit, pointer press), so nested
    // reasons compose. Decided (M8): Begin returns the owner generation the hold belongs to
    // (m_ownerGeneration, bumped by SetOwner and Clear, which drop every hold); End with that value
    // releases it, and End with an older generation, or at count 0, is ignored, so an owner that
    // ends a hold after a handoff cannot release the next owner's. Dropping to 0 calls
    // PostLiveDrain if changes are pending: never drains inline, since the owner is usually inside
    // an input handler. Verbs and Refresh are not held: a reshape folds m_liveDirty into its
    // rebuild (M4).
    uint64_t BeginLiveShapingHold() noexcept;
    void EndLiveShapingHold(uint64_t generation) noexcept;

    // Called once by TableViewSource2 after construction: returns a weak reference to it. The
    // posted drain resolves it and holds the strong reference until DrainLiveChanges returns, so
    // app code run by the drain (or a final release on another thread) cannot destroy this object
    // mid-drain (decided, M5). Not set: the drain is not posted.
    void SetWeakOwnerResolver(std::function<winrt::weak_ref<winrt::IInspectable>()> resolver) noexcept;

    // ---- Output -----------------------------------------------------------------------------

    // The ItemsSourceView TableView binds to (as v1 TableViewSource::GetItemsSourceView()): the
    // ShapedCollectionView itself, which implements ItemsSourceView's default interface (decided,
    // M16), created on first call with &m_rows as its back-reference (D2) and cached. Stays the
    // same object for this coordinator's lifetime: reshapes raise CollectionChanged on it, never
    // replace it. Returns nullptr after Clear().
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
    // the ShapedRowMetadata adapter (R1), created on first call with &m_rows, this object as
    // IShapedExpansion and the view (View() is materialized first, so TryGetGroupMembership can
    // compare by identity) as its back-references (R4) and cached. Same object for this
    // coordinator's lifetime. nullptr after Clear().
    std::shared_ptr<ITableViewRowMetadataProvider> RowMetadata();

    // ---- Lifetime ---------------------------------------------------------------------------

    // Teardown: m_owner = nullptr, m_liveHoldCount = 0 and ++m_ownerGeneration (no callbacks
    // during teardown; TableViewSource2 has already dropped its waiters), m_commitEnded and
    // m_weakOwnerResolver cleared, m_liveDrainToken reset and m_liveDirty emptied (a posted drain
    // then does nothing), m_deferDepth = 0, then ShapedCollectionView::Detach() (it
    // holds a raw ShapedRowQueries*), then ShapedRowMetadata::Detach() (it holds that and a raw
    // IShapedExpansion*), then ShapingEngine::Clear() (ShapedTree::Clear, which drops the
    // listener, the cursor and every group; then tracked keys), then SourceTable::Clear() (detach,
    // handlers nulled, records destroyed), ShapeDelegates::Clear(), PathEvaluator::Clear(),
    // m_view.reset() and m_viewImpl = nullptr, m_rowMetadata.reset(). m_rows stays (a member); every query on it then
    // reports "not found". Must run on the UI thread while m_trackerOwner is alive. Idempotent.
    void Clear();

private:
    // SourceTable callbacks.
    // A Reset (including SourceTable::Refresh's) goes to Reapply(change.Added); a single-item
    // change to ShapingEngine::ApplySourceChange, which rebuilds on its own while stale (M15).
    // Removed records are tombstoned in m_liveDirty first (M6). A surgical change sets
    // m_commit.chromeDirty (M2).
    void OnSourceChanged(SourceChange const& change);
    // Live shaping (decided, E7: coalesce per dispatcher turn). Never shapes inside the app's
    // setter: appends (record, propertyName) to m_liveDirty in arrival order (a second, different
    // name for the same record turns its entry's name into the empty name, "re-read everything",
    // in place; M6) and posts one DrainLiveChanges. Ignored when no verb is applied, or
    // IsLiveShaping is off (M10; a ParentBy key watch raised it).
    void OnItemChanged(SourceRecord* record, winrt::hstring const& propertyName);

    // Posts DrainLiveChanges to the current DispatcherQueue unless one is already posted
    // (m_liveDrainPosted), nothing is pending, or !CanDrain() (EndCommit, EndLiveShapingHold,
    // SetOwner and the outermost EndDeferRefresh post again; M9). The callback holds a weak
    // reference to the owning TableViewSource2 (m_weakOwnerResolver) and m_liveDrainToken weakly
    // (M5): it returns if either expired, and otherwise keeps the owner alive until the drain
    // returns. The drain's exception is not swallowed: the callback reports it through
    // ReportPostedError (Application.UnhandledException), as WPF's posted live-shaping work
    // surfaces it (M3). No queue on
    // this thread (headless or test): nothing is posted, and the changes wait for the next post
    // or a reshape.
    void PostLiveDrain() noexcept;

    // The posted callback. One commit over the batch, in arrival order, tombstones skipped:
    //   up to c_liveBulkThreshold entries
    //                            -> m_engine.ApplyItemChanged per entry (surgical); decided
    //                               (M3): the first entry that throws stops the batch, the
    //                               entries before it stay committed, the rest are dropped, the
    //                               engine is stale, and the error is rethrown after the commit
    //                               closes normally (AfterCommit);
    //   more than c_liveBulkThreshold
    //                            -> m_engine.ApplyItemsChanged (re-read dirty records only, then
    //                               one structural rebuild and a stable Reset); a throw leaves
    //                               nothing written and the engine stale, and propagates.
    // No requeue: a dropped change shows at the next rebuild (the stale engine re-reads all).
    // Moves m_liveDirty into a local batch and clears m_liveDrainPosted before
    // any shaping, so a re-entrant OnItemChanged (app code run by the drain) lands in the fresh
    // list and posts the next drain. Runs only when CanDrain(): skipped while m_liveHoldCount > 0
    // (#18 O4: an in-progress cell edit or pointer press, v1 m_liveShapingHold), a DeferRefresh is
    // open, or an apply runs (M7: a re-entrant source removal is held by the commit's freeze).
    void DrainLiveChanges();

    // m_liveDirty upkeep (M6). Queue appends an entry or updates the record's entry in place (a
    // different name becomes the empty name); it never posts. Clear empties the list and index.
    void QueueLiveChange(SourceRecord* record, winrt::hstring const& propertyName);
    void ClearLiveChanges() noexcept;

    // Applies Diff(applied, desired), or ShapeDelta::All while the engine is stale (M15); nothing
    // when the Diff is empty and the engine is not stale. clearOverlay (D6, M15) is true only when
    // the Diff names ShapeDelta::Filter (a filter verb ran, now or inside the closing DeferRefresh);
    // the folded live delta never sets it. Decided (M4): a reshape folds m_liveDirty into its
    // rebuild: the batch moves to a local and Filter | GroupKey | Sort join the delta; success
    // consumes it (entries queued during the apply stay and EndCommit posts them), a throw
    // merges it back in front of them. A sort-only Diff with nothing folded adds
    // ShapingChangeFlags::Order to the Reset it raises; any other reshape adds Membership (#18
    // O5). Then Retain()s the delegates and paths both specs use.
    void ApplyDesiredSpec();

    // Rebuilds the shaped tree from SourceTable::Records() for a SourceTable Reset: forwards to
    // ShapingEngine::ReapplyForSource(spec, arrivals) (M13 b), which never clears OverlayCollapsed
    // and, on a throw, drops departed records and hides `arrivals` (change.Added) before
    // rethrowing. Never reads the source. spec (M13 a): a copy of m_engine.Applied() while a
    // DeferRefresh is open (a host Reset, such as a ParentBy adoption, must not apply verbs
    // deferred in the scope), m_desired otherwise. Empties m_liveDirty (every record is re-read).
    void Reapply(std::span<SourceRecord* const> arrivals);

    // ---- IShapedRowsListener (decided, P8) --------------------------------------------------
    // The single path from the tree to consumers, whatever made the change (an engine commit, an
    // expansion verb, or an expand-all verb). Each call runs while the engine or tree is mid-write,
    // so all are noexcept: a throw from a handler is caught into m_commit.listenerFault, every later
    // event of the commit is suppressed, EndCommit raises one Reset, and AfterCommit rethrows. Item
    // lists hold count nullptrs, as InspectingDataSource's do (D3); Replace is never raised (O6).

    // ShapedCollectionView::NotifyRemoved / NotifyAdded. Accumulates m_commit.changes: isMove sets
    // Order and adds +-count to m_commit.moveNet; any other call sets Membership.
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
    // Every engine entry point and expansion verb runs between these, through one RAII scope
    // (decided, M17; defined in the .cpp):
    //   { CommitScope scope{ *this, retired }; m_engine.Apply(...); }  // ~CommitScope: EndCommit
    //   AfterCommit();                                                 // only on normal exit
    // CommitScope's constructor calls BeginCommit; its destructor calls EndCommit with
    // unwinding = std::uncaught_exceptions() above the count at construction.
    class CommitScope;

    // BeginCommit: asserts CanRunVerb(), sets m_isApplying, SourceTable::BeginFreeze,
    // m_commit = {}.
    //
    // EndCommit runs from the scope's destructor (also on unwind), so it never throws: a throw
    // from a destructor terminates the process. It only closes the scope:
    //   - m_commit.moveNet != 0 -> Membership (rows moved and the count changed: not a pure
    //     reorder).
    //   - m_commit.listenerFault set -> NotifyReset, so the view and the repeater agree again.
    //   - m_owner->OnShapingApplied({m_commit.changes, retired}) if changes != None, retired is
    //     non-empty or m_commit.chromeDirty is set (O2, O3, O5, P9; M2). Also on unwind (M17):
    //     a surgical live batch that stopped at a throw has written rows the owner must hear
    //     about; an engine throw that wrote nothing leaves changes None and retired empty.
    //     A throw from it is caught into m_commit.listenerFault (if still empty).
    //   - ShapingEngine::ReleaseRetired, SourceTable::EndFreeze, m_isApplying cleared.
    //   - EndFreeze reported a pending change: on unwind, ResyncSource(post: true) (AfterCommit
    //     will not run, and nothing runs while the exception unwinds into the app); otherwise
    //     m_commit.resyncDue is set.
    //   - PostLiveDrain if changes are pending (M9: entries queued during the apply).
    // On unwind a recorded listenerFault is dropped: the engine's exception propagates.
    // retired: the SourceChange's Removed records when called from OnSourceChanged (alive until
    // that handler returns); empty for verbs, expansion and live shaping. No owner: the owner
    // calls are skipped; no view yet: the tree has no listener and nothing is raised.
    void BeginCommit();
    void EndCommit(std::span<SourceRecord const* const> retired, bool unwinding) noexcept;

    // Called by the verb or handler that opened the scope, after the scope object is gone, when no
    // exception escaped it. `faulted`: the caller caught an engine exception inside the scope
    // (a source change or live drain whose rows were partly written) and rethrows it after this
    // returns; it swallows this call's own exceptions. Takes m_commit (leaving it empty), then:
    //   - resyncDue: ResyncSource(post: listenerFault set or faulted), so an app exception is
    //     never followed by a synchronous re-read before it reaches the app.
    //   - Runs m_commitEnded (M1), also when the resync threw.
    //   - Rethrows, first found: listenerFault (the first handler exception), the synchronous
    //     resync's exception (M14), m_commitEnded's exception.
    void AfterCommit(bool faulted = false);

    // The verb entry gate (decided, M17): throws InvalidOperationException
    // ("TableViewSource2.<verb> cannot be called while the source is being shaped.") when
    // !CanRunVerb(); returns false after Clear() (the verb is then a no-op).
    bool BeginVerb(wchar_t const* verb) const;

    // ---- Gates (decided) --------------------------------------------------------------------
    // One predicate per kind of work, so each rule lives in one place (the spec's coordinator
    // state table lists the full matrix, including SourceTable's freeze, hold and divergence).
    //   CanRunVerb: a verb, Refresh, DeferRefresh begin or end, a non-null SetOwner or an
    //     expansion verb may start; otherwise it throws (#19).
    //   CanReshape: a verb may apply now; otherwise (DeferRefresh open) it edits m_desired only.
    //   CanDrain: the live drain may run; otherwise it stays pending and is posted again by
    //     whatever closed the gate.
    bool CanRunVerb() const noexcept;     // !m_isApplying
    bool CanReshape() const noexcept;     // m_deferDepth == 0
    bool CanDrain() const noexcept;       // !m_isApplying && m_deferDepth == 0 && m_liveHoldCount == 0

    ITrackerHandleManager const* m_trackerOwner{ nullptr };

    // Shared only so event handlers (item PropertyChanged, child feeds via WeakHost()) can capture
    // it weakly; nothing outside this object holds it. Clear() detaches it.
    std::shared_ptr<SourceTable> m_sourceTable;

    // What the verbs have declared. The applied copy lives in the engine (Phase 4).
    ShapeSpec m_desired;
    ShapeDelegates m_delegates;
    PathEvaluator m_paths;

    // Cached result of View(): the ShapedCollectionView (the listener reaches it through
    // m_viewImpl). Tracked, because TableView holds it too. Null until the first View(); until then
    // the listener has no view to notify. std::optional because ~tracker_ref dereferences the
    // owner; Clear() detaches and resets it while the owner is alive.
    std::optional<tracker_ref<winrt::ItemsSourceView>> m_view;
    // Non-owning pointer to m_view's implementation; null exactly when m_view is.
    ShapedCollectionView* m_viewImpl{ nullptr };

    // Cached result of RowMetadata() (R4). Plain shared_ptr: it holds no app object, and TableView
    // keeps its own shared_ptr. Null until the first RowMetadata(); Clear() detaches and resets it.
    std::shared_ptr<ShapedRowMetadata> m_rowMetadata;

    // Notification sink set by SetOwner (#18 O1), or nullptr. Not owned. Mirrors
    // TableViewSource2::m_owner; arbitration is there (B2).
    IShapingOwner* m_owner{ nullptr };

    // SetCommitEndedHandler (M1).
    std::function<void()> m_commitEnded;

    // Live-shaping holds taken by the owner (#18 O4). Reset to 0 when SetOwner changes the sink,
    // and by Clear(), both of which also bump m_ownerGeneration so a hold taken before cannot be
    // ended against the new count (M8).
    uint32_t m_liveHoldCount{ 0 };
    uint64_t m_ownerGeneration{ 0 };

    // Live-shaping drain (DrainLiveChanges): more than this many dirty records in one turn go
    // through ApplyItemsChanged (one rebuild, one stable Reset); this many or fewer are applied
    // surgically, one ApplyItemChanged each, in the same commit. Higher than the source-side bulk
    // threshold (ParentKeyHierarchy::c_bulkThreshold, 1): the dirty set is what an editing app touched in one turn (usually a few records), and a handful
    // of surgical remove / insert pairs is cheaper than a rebuild plus a scroll re-anchor.
    static constexpr uint32_t c_liveBulkThreshold = 16;

    // Re-entrancy guard for verbs (CanRunVerb): a verb or Refresh while an apply runs throws.
    // Source notifications are guarded separately by SourceTable's freeze.
    bool m_isApplying{ false };

    // Open DeferRefresh handles (WPF nesting). While > 0, verbs edit m_desired only, Refresh calls
    // SourceTable::ScheduleRefresh, SourceTable holds resets and the live drain is skipped. No
    // "refresh requested" flag here: SourceTable's m_rereadDue carries it. Clear() resets it, so
    // a handle closed after teardown does nothing.
    uint32_t m_deferDepth{ 0 };

    // Source change during an apply (decided, #19: freeze, then one deferred Reset). Every engine
    // entry point and expansion verb runs inside one commit scope (BeginCommit / EndCommit). A
    // source change raised by app code inside it (a predicate, comparer or key selector, or a
    // binding, converter or element factory in an event handler) is not applied to the table, so
    // records and links never change under the engine or the tree. When EndFreeze reports a pending
    // change, ResyncSource runs SourceTable::Refresh(), whose Reset goes to Reapply().
    // It runs synchronously once, from AfterCommit (never from the scope's destructor); it is
    // posted instead when the scope ended by an exception or when a handler faulted. A change made
    // during the resync's own Reset is not re-read here: SourceTable::Refresh posts it
    // (PostReread; one synchronous re-read per turn, so an app that mutates on every callback
    // cannot loop).
    // Decided (M14): post = false runs SourceTable::Refresh() synchronously; its exception
    // propagates out of AfterCommit after the commit closed (the verb that opened the scope has
    // already applied). A failed re-read leaves the table diverged with nothing owed (M18): the
    // next source notification or Refresh re-reads. post = true calls SourceTable::PostReread()
    // (it guards against a second post); the posted re-read's exception is reported through
    // ReportPostedError (Application.UnhandledException).
    void ResyncSource(bool post);

    // Live-shaping changes pending for the next DrainLiveChanges (E7; decided, M6). An arrival-
    // ordered list plus an index by record: a second change to a listed record updates its entry
    // in place (a different name becomes the empty name), so the drain applies entries in the
    // order they first arrived without sorting. Raw record pointers: OnSourceChanged tombstones
    // the entries of removed records and their descendants (Record = nullptr, index entry erased)
    // before the engine applies the change, so the drain skips them; a SourceTable Reset or any
    // Reapply empties both (it re-reads every record); a reshape folds them into its rebuild (M4).
    // Clear() empties both, so a drain posted before teardown is a no-op.
    std::vector<LiveChange> m_liveDirty;
    std::unordered_map<SourceRecord*, uint32_t> m_liveDirtyIndex;
    bool m_liveDrainPosted{ false };

    // The drain's lifetime guards (M5). m_liveDrainToken points at this object, created by the
    // constructor and reset by Clear(), so a drain that runs after teardown finds it expired and
    // does nothing. m_weakOwnerResolver (SetWeakOwnerResolver) gives the weak reference to
    // TableViewSource2 the posted callback resolves and holds for the length of the drain.
    std::shared_ptr<ShapingManager*> m_liveDrainToken;
    std::function<winrt::weak_ref<winrt::IInspectable>()> m_weakOwnerResolver;

    // Source tree + applied spec -> shaped tree (ShapedTree, P10). Every entry point is bracketed
    // here by the commit scope. Built in the constructor's member-init list after m_sourceTable is
    // created (it binds to *m_sourceTable), with m_trackerOwner, m_delegates and m_paths. Group
    // expansion lives in its ShapedTree (reached through Tree()).
    ShapingEngine m_engine;

    // Read-only queries over m_engine and *m_sourceTable, handed to the adapters (decided). Declared
    // after m_engine and bound in the member-init list. The adapters hold a raw pointer to it,
    // nulled by their Detach() in Clear().
    ShapedRowQueries m_rows;

    // The current commit (P8). Reset by BeginCommit, filled by the listener and EndCommit, taken
    // by AfterCommit. Grouped so one assignment clears it and no field is forgotten.
    struct CommitState
    {
        // What the commit's events did.
        ShapingChangeFlags changes{ ShapingChangeFlags::None };
        // Net row count of its move events (+ inserted, - removed).
        int64_t moveNet{ 0 };
        // The first exception a handler threw; rethrown by AfterCommit.
        std::exception_ptr listenerFault;
        // EndFreeze reported a source change and the scope ended normally; AfterCommit resyncs.
        bool resyncDue{ false };
        // A surgical source change was applied. It may change a staying row's chrome without a row
        // event (a first child linked under a collapsed parent), so EndCommit calls the owner even
        // when changes is None (P9; decided, M2).
        bool chromeDirty{ false };
    };
    CommitState m_commit;
};
