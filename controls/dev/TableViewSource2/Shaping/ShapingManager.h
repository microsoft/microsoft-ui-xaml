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
// That collection is ShapedCollectionView, a native read-only IBindableVector View() returns;
// TableViewSource2::GetItemsSourceView() wraps it once in a winrt::ItemsSourceView and caches it.
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
    // open: SourceTable::ScheduleRefresh() only (a re-read is owed at the outermost
    // EndHoldResets; the table is not marked diverged, so incremental changes keep showing).
    void Refresh();

    // ---- DeferRefresh (decided) -------------------------------------------------------------
    // WPF model: defers reshapes and re-reads, not incremental source changes.
    // Begin: throws while an apply runs (#19). The outermost Begin (depth 0 -> 1) calls
    // SourceTable::BeginHoldResets: an incremental source change still reaches OnSourceChanged and
    // is shaped surgically with the applied spec and published at once; a source Reset is held.
    // The live-shaping drain is skipped while open (changes stay in m_liveDirty, as under a hold).
    // End: asserts depth > 0. Inner ends only decrement. The outermost end calls
    // SourceTable::EndHoldResets; if it reports a pending change (a held Reset, a Refresh() while
    // open, or a parked hierarchy), it runs SourceTable::Refresh() (one re-read that also builds a
    // pending hierarchy), whose Reset goes to Reapply() with m_desired: one reshape, one Reset.
    // Otherwise ApplyDesiredSpec() (nothing when the spec did not change). Then PostLiveDrain if
    // changes are pending. An exception from app code propagates (applied spec stays old,
    // m_desired is kept, as for a verb; a re-read that fails validation leaves the table diverged
    // for the next notification or Refresh; any other failure leaves the re-read owed, so the next
    // commit retries it); the depth is 0 either way.
    void BeginDeferRefresh();
    void EndDeferRefresh();
    bool IsRefreshDeferred() const noexcept;

    // The getter never throws. The setter forwards to SourceTable::SetLiveShaping; called while an
    // apply runs it throws InvalidOperationException (#19), like a verb.
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
    // and ++m_ownerGeneration,
    // then PostLiveDrain if changes are pending. A non-null sink then calls
    // SourceTable::BindOwnerThread(), so a source built off the UI thread is bound to the owner's
    //     thread once (see SourceTable, Threading). The same sink again is a no-op. A non-null sink
        // throws while an apply runs (#19), before anything changes, so TableViewSource2 can call it
        // before updating its own state. nullptr (a release) never throws, even mid-commit: the owner
        // must be able to let go from inside its own callbacks. Raw pointer: TableViewSource2 sets
        // nullptr before the owner is destroyed.
        void SetOwner(IShapingOwner* sink);

        // Runs at the end of every commit that completes (after m_isApplying is cleared, so verbs and
        // claims may run). TableViewSource2 uses it to run owner-released waiters deferred by a
        // release during a commit. Its exception propagates after any owner-callback fault. A commit
        // unwound by an engine exception does not run it. Cleared by Clear(). Captures only the
        // native TableViewSource2, never an app object.
        void SetCommitEndedHandler(std::function<void()> handler) noexcept;

        // True while a commit is applying (between BeginCommit and EndCommit).
        bool IsApplying() const noexcept;

    // Counted live-shaping hold (O4): while m_liveHoldCount > 0, DrainLiveChanges defers (changes
    // stay in m_liveDirty). The owner takes one per reason (cell edit, pointer press), so nested
    // reasons compose. Begin returns the owner generation the hold belongs to (m_ownerGeneration,
    // bumped by SetOwner and Clear, which drop every hold); End with that value releases it, and
    // End with an older generation is ignored, so an owner that ends a hold after a handoff cannot
    // release the next owner's (drift #48). Dropping to 0 calls PostLiveDrain if changes are
    // pending: never drains inline, since the owner is usually inside an input handler. Verbs and
    // Refresh are not held: a reshape folds m_liveDirty into its rebuild (drift #41).
    uint64_t BeginLiveShapingHold() noexcept;
    void EndLiveShapingHold(uint64_t generation);

    // Called once by TableViewSource2 after construction: returns a weak reference to it. The
    // posted drain resolves it and holds the strong reference until DrainLiveChanges returns, so
    // app code run by the drain (or a final release on another thread) cannot destroy this object
    // mid-drain (drift #42). Not set: the drain is not posted.
    void SetWeakOwnerResolver(std::function<winrt::weak_ref<winrt::IInspectable>()> resolver) noexcept;

    // ---- Output -----------------------------------------------------------------------------

    // The visible list (D1): the ShapedCollectionView, as its IBindableVector, created on first
    // call with &m_rows as its back-reference (D2) and cached. Not an ItemsSourceView (that type's
    // implementation lives in the Controls DLL and cannot be derived from here):
    // TableViewSource2::GetItemsSourceView() wraps this once with winrt::ItemsSourceView{ View() }
    // and caches the wrapper, as v1 TableViewSource::GetItemsSourceView() is consumed. Stays the
    // same object for this coordinator's lifetime: reshapes raise CollectionChanged on it, never
    // replace it. Returns nullptr after Clear().
    winrt::Microsoft::UI::Xaml::Interop::IBindableVector View();

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
    // Ignored when no verb is applied, or IsLiveShaping is off (a ParentBy key watch raised it).
    void OnItemChanged(SourceRecord* record, winrt::hstring const& propertyName);

    // Posts DrainLiveChanges to the current DispatcherQueue unless one is already posted
    // (m_liveDrainPosted), nothing is pending, or !CanDrain() (EndCommit, EndLiveShapingHold and
    // SetOwner post again). The callback holds a weak reference to the owning TableViewSource2
    // and m_liveDrainToken weakly (drift #42): it returns if either expired, and otherwise keeps
    // the owner alive until the drain returns; it swallows the drain's exception (drift #40).
    // Never throws. No queue on this
    // thread (headless): nothing is posted, and the changes wait for the next post or a reshape.
    void PostLiveDrain();

    // The posted callback. One commit:
    //   up to c_liveBulkThreshold entries
    //                            -> m_engine.ApplyItemChanged per entry (surgical), in arrival
    //                               order; an entry that throws leaves the tree as it was, the
    //                               rest still apply, and the first error propagates after the
    //                               commit (drift #40);
    //   more than c_liveBulkThreshold
    //                            -> m_engine.ApplyItemsChanged (re-read dirty records only, then
    //                               one structural rebuild and a stable Reset); if it throws
    //                               (nothing written), the per-entry path runs instead, so only
    //                               the entries that throw are dropped (drift #40).
    // Moves m_liveDirty into a local batch and clears m_liveDrainPosted before
    // any shaping, so a re-entrant OnItemChanged (app code run by the drain) lands in the fresh
    // map and posts the next drain. Runs only when CanDrain(): skipped while m_liveHoldCount > 0
    // (#18 O4: an in-progress cell edit or pointer press, v1 m_liveShapingHold), a DeferRefresh is
    // open, or an apply runs; EndLiveShapingHold, SetOwner and EndCommit post it again.
    void DrainLiveChanges();

    // Applies Diff(applied, desired); then Retain()s the delegates and paths both specs use. A
    // sort-only Diff (sort or group direction changed, nothing else) with no live changes folded
    // in adds ShapingChangeFlags::Order to the Reset it raises; any other reshape adds Membership
    // (#18 O5). Folding (drift #41): pending m_liveDirty entries move to a local before the apply;
    // on success they are consumed and only entries queued during the apply stay (and are
    // posted); on a throw they are merged back.
    void ApplyDesiredSpec();

    // Rebuilds the shaped tree from SourceTable::Records() with the desired spec. Never reads
    // the source. Used for a SourceTable Reset (`arrivals` = SourceChange::Added); goes through
    // ShapingEngine::ReapplyForSource, so a delegate throw still drops the departed records.
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
    // Every engine entry point and expansion verb runs between these, through one RAII scope:
    //   { CommitScope scope{ *this, retired }; m_engine.Apply(...); }  // ~CommitScope: EndCommit
    //   AfterCommit();                                                 // only on normal exit
    // BeginCommit: asserts CanRunVerb(), sets m_isApplying, SourceTable::BeginFreeze,
    // m_commit = {}.
    //
    // EndCommit runs from the scope's destructor (also on unwind), so it never throws: a throw
    // from a destructor terminates the process. It only closes the scope:
    //   - m_commit.moveNet != 0 -> Membership (rows moved and the count changed: not a pure
    //     reorder).
    //   - m_commit.listenerFault set -> NotifyReset, so the view and the repeater agree again.
    //   - m_owner->OnShapingApplied({m_commit.changes, retired}) if changes != None, retired is
    //     non-empty or chromeDirty is set (O2, O3, O5, P9; drift #21). Skipped on unwind from an
    //     engine throw (nothing was written).
    //     A throw from it is caught into m_commit.listenerFault (if still empty).
    //   - ShapingEngine::ReleaseRetired, SourceTable::EndFreeze, m_isApplying cleared.
    //   - EndFreeze reported a pending change: on unwind, ResyncSource(post: true) (AfterCommit
    //     will not run, and nothing runs while the exception unwinds into the app); otherwise
    //     m_commit.resyncDue is set.
    // On unwind a recorded listenerFault is dropped: the engine's exception propagates.
    // retired: the SourceChange's Removed records when called from OnSourceChanged (alive until
    // that handler returns); empty for verbs, expansion and live shaping. No owner: the owner
    // calls are skipped; no view yet: the tree has no listener and nothing is raised.
    // The RAII scope that calls BeginCommit / EndCommit (defined in ShapingManager.cpp).
    class CommitScope;

    void BeginCommit();
    void EndCommit(std::span<SourceRecord const* const> retired, bool unwinding) noexcept;

    // Called by the verb or handler that opened the scope, after the scope object is gone, when no
    // exception escaped it. Takes m_commit (leaving it empty), then:
    //   - resyncDue: ResyncSource(post: listenerFault set), so an app exception is never
    //     followed by a synchronous re-read before it reaches the app.
    //   - Rethrows listenerFault (the first handler exception).
    void AfterCommit();

    // ---- Gates (decided) --------------------------------------------------------------------
    // One predicate per kind of work, so each rule lives in one place (the spec's coordinator
    // state table lists the full matrix, including SourceTable's freeze, hold and divergence).
    //   CanRunVerb: a verb, Refresh, DeferRefresh begin, SetOwner or an expansion verb may start;
    //     otherwise it throws (#19).
    //   CanReshape: a verb may apply now; otherwise (DeferRefresh open) it edits m_desired only.
    //   CanDrain: the live drain may run; otherwise it stays pending and is posted again by
    //     whatever closed the gate.
    bool CanRunVerb() const noexcept;     // !m_isApplying
    // Preamble of a hierarchy verb: throws illegal_method_call while an apply runs; false when the
    // source is detached (nothing to do).
    bool BeginVerb(wchar_t const* verb) const;
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
    // winrt::get_self<ShapedCollectionView>). Tracked, because TableView holds it too. Null until
    // the first View(); the tree listener is installed by the constructor, so commits before then
    // still reach the owner. std::optional because ~tracker_ref
    // dereferences the owner; Clear() detaches and resets it while the owner is alive.
    std::optional<tracker_ref<winrt::Microsoft::UI::Xaml::Interop::IBindableVector>> m_view;

    // The implementation behind m_view, for the noexcept listener. Set with m_view, nulled by Clear().
    ShapedCollectionView* m_viewImpl{ nullptr };

    // Cached result of RowMetadata() (R4). Plain shared_ptr: it holds no app object, and TableView
    // keeps its own shared_ptr. Null until the first RowMetadata(); Clear() detaches and resets it.
    std::shared_ptr<ShapedRowMetadata> m_rowMetadata;

    // Notification sink set by SetOwner (#18 O1), or nullptr. Not owned. Mirrors
    // TableViewSource2::m_owner; arbitration is there (B2).
    IShapingOwner* m_owner{ nullptr };

    // Live-shaping holds taken by the owner (#18 O4). Reset to 0 when SetOwner changes the sink,
    // and by Clear(), both of which also bump m_ownerGeneration so a hold taken before cannot be
    // ended against the new count (drift #48).
    uint32_t m_liveHoldCount{ 0 };
    uint64_t m_ownerGeneration{ 0 };

    // A source notification with more than this many items (a range event, or an insert adopting
    // several orphans under ParentBy) goes through Reapply() and one stable Reset instead of
    // surgical Link / Unlink. Descendants carried by an item do not count. 1 matches v1 ShapedItemsSource.
    static constexpr uint32_t c_bulkThreshold = 1;

    // Live-shaping drain (DrainLiveChanges): more than this many dirty records in one turn go
    // through ApplyItemsChanged (one rebuild, one stable Reset); this many or fewer are applied
    // surgically, one ApplyItemChanged each, in the same commit. Higher than c_bulkThreshold: the
    // dirty set is what an editing app touched in one turn (usually a few records), and a handful
    // of surgical remove / insert pairs is cheaper than a rebuild plus a scroll re-anchor.
    static constexpr uint32_t c_liveBulkThreshold = 16;

    // Re-entrancy guard for verbs (CanRunVerb): a verb or Refresh while an apply runs throws.
    // Source notifications are guarded separately by SourceTable's freeze.
    bool m_isApplying{ false };

    // SetCommitEndedHandler.
    std::function<void()> m_commitEnded;

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
    // Never throws: Refresh() runs app code and validation, so a throw is caught, the table stays
    // as it was (SourceTable stays diverged), and one retry is posted
    // (SourceTable::PostReread, guarded by its m_rereadPosted); the verb that opened the scope
    // has already succeeded. The posted retry
    // does nothing if SourceTable::IsRereadDue() is false by then (a notification already
    // re-read). A posted retry that throws is swallowed. After a validation failure the table is
    // diverged with no re-read owed: the next source notification or Refresh retries. After any
    // other failure the re-read stays owed: the next commit's EndFreeze reports it again.
    void ResyncSource(bool post) noexcept;

    // Live-shaping changes pending for the next DrainLiveChanges (E7). Keys are raw record
    // pointers: OnSourceChanged erases removed records and their descendants before the engine
    // applies the change, a SourceTable Reset or any Reapply empties the map (it re-reads every
    // record), and a verb that reshapes folds the map into its rebuild (drift #41), so no drain
    // runs over keys a rebuild replaced. Clear() empties it, so a drain posted before teardown is
    // a no-op. Sequence is the arrival order: a batch is applied in it (drift #44).
    struct LiveDirty
    {
        winrt::hstring PropertyName;
        uint64_t Sequence{ 0 };
    };
    std::unordered_map<SourceRecord*, LiveDirty> m_liveDirty;
    uint64_t m_liveSequence{ 0 };
    bool m_liveDrainPosted{ false };

    // The drain's lifetime guards (drift #42). m_liveDrainToken points at this object, created by
    // the constructor and reset by Clear(), so a drain that runs after teardown finds it expired
    // and does nothing. m_weakOwnerResolver (SetWeakOwnerResolver) gives the weak reference to
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
        // when changes is None (P9; drift #21).
        bool chromeDirty{ false };
    };
    CommitState m_commit;
};
