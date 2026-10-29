// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Microsoft.UI.Xaml.Interop.h>
#include <winrt/Windows.Foundation.h>

#include "CollectionAccessor2.h"
#include "SourceRecord.h"
#include "SourceHierarchy.h"

// The record table for one input source and its source tree, plus their lookups.
//
//   Records()      root-collection records, one per source position, in source order (owning)
//   TopLevel()     top-level records of the source tree (Parent == nullptr)
//   raw lookup     get_abi(item) -> record   (always on; pipeline map-back, no QI)
//   address lookup Address       -> record   (always on; duplicate check + COM identity lookup)
//   identity index Identity      -> record   (shaped only; selection, focus, expansion lookups)
//
// The lookups, identity index and live shaping cover every record in the tree, including
// ChildrenBy records that are not in Records().
//
// Source tree (hierarchy):
//   - The tree must exist before any other verb runs (filter keeps ancestors of matches, sort orders
//     each sibling set, GroupBy buckets the top-level records), so it is part of the source, not of
//     shaping. Shaping reads only TopLevel() and SourceRecord::Parent / Children / Depth, and never
//     cares which verb built the tree.
//   - Flat (m_hierarchy null): every record is top-level.
//   - Each hierarchy verb is a SourceHierarchy strategy (Source/SourceHierarchy.h, decided: split
//     out of SourceTable): ParentKeyHierarchy (ParentBy: an index over Records()) and
//     ChildCollectionsHierarchy (ChildrenBy: Records() are the top level, every other record is
//     owned by its parent's child feed). Key rules, feeds and validation are documented there.
//     SourceTable keeps the shared tree (m_topLevel and the link operations) and implements
//     ISourceHierarchyHost privately for the strategies.
//   - Changing the hierarchy verb rewrites Parent / Children / Depth on every record (O(n)) and
//     raises one Reset.
//   - A change to an existing item's key, parent key or children property is seen only through
//     live shaping (see ReevaluateHierarchy()).
//
// Owns the CollectionAccessor2 for the bound source, subscribes to its change notifications, and
// keeps m_records, the tree and the lookup maps synchronized automatically. Duplicate objects and
// null items are rejected upfront across the whole tree in all modes (shaped or not), so every key
// maps to at most one record and identity is the item itself, never a path. This is stricter than
// v1, which allowed both while unshaped. The identity index is built only while shaping is active
// so unshaped tables avoid formatting identity strings.
//
// Rejection contract:
//   - Validation runs before any mutation: a rejected change leaves m_records, the lookups and
//     subscriptions exactly as they were, and raises no SourceChanged.
//   - Failure throws hresult_invalid_argument with v1's messages ("The same item object appears in
//     the source more than once..." / "A row could not be given a stable identity: null item").
//   - Create() / Refresh(): the whole materialization is validated first by ValidateItems() (one
//     pass with scratch raw-pointer and address sets), so the throw reaches the caller of
//     From(items) or Refresh().
//   - Incremental notifications (InsertAt / ReplaceAt): the throw propagates out of the
//     CollectionChanged / VectorChanged handler, i.e. into the app's Add() / Insert() / setter.
//     The source has already changed, so the table has diverged: m_diverged is set and every
//     later notification is handled as a re-read (re-enumerate + full validation) until one
//     succeeds, which raises a single Reset and clears the flag. Positional deltas are never
//     applied on top of a diverged table.
//
// Also owns per-record INotifyPropertyChanged subscriptions when live shaping is enabled
// (SetLiveShaping(true)), irrespective of whether any shaping verbs are currently active. Adding or
// clearing verbs never walks records to subscribe or unsubscribe; new records subscribe on creation
// while live shaping is on, and ~SourceRecord() revokes on removal.
//
// SourceTable is the only subscriber to the source and its items. Downstream stages never subscribe
// directly; they consume:
//   - SourceChanged: reports each collection change in terms of records:
//       1. m_records (or the child feed), the tree links and the lookups are updated.
//       2. SourceChanged is raised synchronously on the UI thread (single handler: the coordinator,
//          which fans out to engine, projection and owner in a fixed order).
//       3. Removed records are destroyed (revoking any live PropertyChangedToken).
//     Root-collection and child-collection notifications go through the same path, re-entrancy
//     gate and queue. Multi-item notifications are split into single-item changes or reported as
//     one Reset. A change raised re-entrantly while frozen (decided, #19: a SourceChanged dispatch
//     holds a freeze, as does the coordinator's commit scope) is not applied: the table is left as
//     it was, m_diverged and m_rereadDue are set, and the source is re-read with one Reset when
//     the outermost freeze ends. While resets are held (an open DeferRefresh), incremental changes
//     are applied as usual but a Reset is not: it is skipped and the table stops applying changes
//     until the re-read. One rule (see Divergence below); "skip" = not applied, sets m_diverged and
//     m_rereadDue:
//
//       table state          | incremental change | Reset (or multi-item)
//       ---------------------|--------------------|----------------------
//       frozen               | skip               | skip
//       held, not diverged   | apply              | skip
//       held, diverged       | skip               | skip
//       free, not diverged   | apply              | re-read
//       free, diverged       | re-read            | re-read
//     Under ParentBy one source change can produce several tree changes (an insert adopts
//     existing orphans, each a Move from top level to the new item). Decided: adopting one orphan
//     is surgical (the insert plus one Move); adopting more goes to Reapply() through
//     ShapingManager::c_bulkThreshold.
//   - ItemChanged: forwards (SourceRecord*, propertyName) when a subscribed record raises
//     PropertyChanged. The record is never null; an empty propertyName means all properties
//     changed.
//
// Threading (decided: bind once, from the no-queue state only):
//   - Create() captures the current thread's DispatcherQueue and thread id into m_binding when the
//     thread has a queue (bound). Off a dispatcher thread (a view model built in Task.Run, a
//     headless test) it leaves m_binding unbound; Create() may run there.
//   - BindOwnerThread() (from ShapingManager::SetOwner with a non-null owner, always on the UI
//     thread) binds an unbound table to the owner's thread once. A table already bound keeps its
//     binding; an owner on a different thread only asserts in debug. Nothing can have been posted
//     while unbound (there was no queue), so binding never strands a post; only the one write
//     needs safe publication (the thread id is stored last, with release order).
//   - All table state and both handlers belong to the bound thread, or before binding to the
//     thread the app uses it on.
//   - Source and child collection notifications must arrive on that thread (the contract every
//     XAML collection consumer imposes); they are not marshalled.
//   - Item PropertyChanged: bound and on the bound thread, or unbound -> delivered inline. Bound
//     and on another thread -> posted to the bound queue with only weak pointers and the property
//     name; the weak pointers are locked there. A failed TryEnqueue means the queue is shutting
//     down: the change is ignored (no dropped-change flag, no recovery).
//
// Lifetime: always owned by std::shared_ptr and created only through Create(). Every event lambda
// registered on the source, a child collection or an item captures weak_from_this() (plus the
// owning record's weak_ptr for child-collection and item events) and drops the notification if
// either has expired, so a late or in-flight event can never touch a destroyed table. The weak
// pointers are locked only on the bound thread (or, while unbound, the raising thread, which is
// the only thread using the table): a lock taken on a worker thread could become
// the last strong reference and run ~SourceTable() (and its UI-affine revokes) there.
// Subscriptions start in Create(), after construction, because weak_from_this() is empty inside
// the constructor.
//
// Reference tracking: m_trackerOwner points to the owning TableViewSource2 (an
// ITrackerHandleManager). Every app-supplied object or delegate held here goes through tracker_ref
// bound to m_trackerOwner: CollectionAccessor2 (root and child feeds), each SourceRecord::Item, the
// hierarchy selectors and ParentBy object keys. A strong reference to a .NET object (an app lambda
// capturing the page or VM) would root a cycle through TableViewSource2 that the GC cannot see.
// Source-change subscriptions store a raw event_token rather than winrt::event_revoker (which holds
// an untracked strong COM pointer). Each record costs one NewTrackerHandle + SetTrackerValue on
// creation and one DeleteTrackerHandle on removal (same as Vector<IInspectable>).
//
// Detach contract:
//   - ~tracker_ref dereferences its owner even when empty, so no tracker_ref may outlive
//     m_trackerOwner. ~TableViewSource2() calls Clear() on the UI thread, which destroys the child
//     feeds (and their records) first by releasing m_hierarchy (which also drops the selectors and
//     the key index), then every root record, resets m_sourceAccessor, and nulls m_trackerOwner. The table is then detached: empty, owner-free, and safe to outlive
//     TableViewSource2 (consumer shared_ptrs, a posted off-thread lambda that has the table locked).
//     Every public entry point and event lambda returns early when detached. ~SourceTable() asserts
//     it was detached.
//   - tracker_ref holds no reference count itself; the owner's tracker handle does. When the GC
//     collects a cycle (item <-> VM <-> TableViewSource2), items and the collection may already be
//     collected when Clear() runs, so teardown never calls tracker_ref::get(): handlers are nulled
//     first (Clear() raises nothing), lookups are cleared wholesale, and every revoke goes through
//     safe_get().
class SourceTable : public std::enable_shared_from_this<SourceTable>, private ISourceHierarchyHost
{
    struct PrivateToken { explicit PrivateToken() = default; };

public:
    using SourceChangedHandler = std::function<void(SourceChange const& change)>;
    using ItemChangedHandler = std::function<void(SourceRecord* record, winrt::hstring const& propertyName)>;

    // Constructs the table, captures the current thread's DispatcherQueue and thread id when it has
    // a queue (else leaves the table unbound; see Threading), resolves CollectionAccessor2(owner,
    // source), subscribes to source change notifications, and performs the initial Refresh(). May
    // run off the UI thread.
    static std::shared_ptr<SourceTable> Create(
        ITrackerHandleManager const* owner,
        winrt::Windows::Foundation::IInspectable const& source);

    // Public only so std::make_shared can reach it; PrivateToken restricts callers to Create().
    SourceTable(PrivateToken, ITrackerHandleManager const* owner);

    // Asserts the table was detached by Clear(); never touches m_trackerOwner.
    ~SourceTable();

    // Binds an unbound table to the current thread's DispatcherQueue and thread id (see
    // Threading). Called on the UI thread by ShapingManager::SetOwner for a non-null owner. No-op
    // when already bound to this thread; when bound to another thread, keeps that binding and
    // asserts in debug. No-op when detached.
    void BindOwnerThread() noexcept;

    SourceTable(SourceTable const&) = delete;
    SourceTable& operator=(SourceTable const&) = delete;

    // ---- Source lifecycle & records ---------------------------------------------------------

    // Re-enumerates the bound source (and, under ChildrenBy, every child collection; under ParentBy,
    // every key) and reconciles against existing records (initial load, Reset, out-of-sync
    // recovery, the end of a freeze or reset hold, or explicit Refresh() for non-observable
    // sources). No-op when detached. Decided (reuse, per-feed match), see SourceHierarchy:
    //   1. Read: enumerate, ValidateItems(), strategy->Prepare(host, items) (the parked strategy if
    //      any). A throw leaves the table untouched.
    //   2. Match: MatchRecords() for the root, strategy->MatchForRefresh() for each child feed.
    //      Unmatched records leave the lookups before anything is created.
    //   3. Link: CommitRecords(), ResetTreeToFlat(), strategy->Commit(). A parked strategy replaces
    //      the current one between 2 and 3 (TakeChildRecords(), Release()).
    //   4. One Reset: Added = records created, Removed = records that left.
    // A reused record keeps its pointer, Identity(), PropertyChanged subscription and expansion
    // flag. On success clears m_diverged and m_rereadDue after step 3, before the Reset is raised:
    // the table matches the source then, and a change made by a Reset handler is skipped (frozen)
    // and owed again rather than lost. On a validation failure (null or
    // duplicate item) sets m_diverged and clears m_rereadDue: the source is in the state a rejected
    // change leaves, so the next notification recovers it, not every commit. On any other throw
    // (app enumerator, selector) both flags are left as they were, so an owed re-read is retried
    // at the next report point.
    // Anti-loop (decided: one synchronous re-read per turn): Refresh never re-reads again
    // synchronously. When its own Reset dispatch ends with a re-read owed (an app that changes the
    // source in a Reset handler, directly or through shaping callbacks), it calls PostReread()
    // instead of reporting it, so every caller (DrainPendingChanges, ShapingManager::ResyncSource,
    // a verb's Refresh, EndDeferRefresh) runs at most one re-read in its turn.
    void Refresh();

    // Teardown (see Detach contract); must run on the UI thread while m_trackerOwner is alive.
    // Releases m_hierarchy and drops m_pendingHierarchy first (child feeds unsubscribed, keys and
    // selectors dropped). Idempotent.
    void Clear();

    bool IsDetached() const noexcept;

    // Root collection only (Records()); see TopLevel().
    uint32_t Size() const noexcept;
    SourceRecord* At(uint32_t index) const;
    std::vector<SourceRecordPtr> const& Records() const noexcept;

    void SetSourceChangedHandler(SourceChangedHandler handler);
    void SetItemChangedHandler(ItemChangedHandler handler);

    // ---- Freeze (decided, #19) --------------------------------------------------------------
    // While frozen (depth > 0), root- and child-collection notifications are not applied: records,
    // links and lookups stay exactly as they were, so the coordinator can iterate TopLevel() and
    // Children and hold raw SourceRecord pointers across app code. A notification is skipped: it
    // only sets m_diverged and m_rereadDue. Nests. Two holders: the SourceChanged dispatch
    // (RaiseSourceChanged freezes around the handler call, so there is no separate dispatching
    // flag) and ShapingManager's commit scope (every engine call and its events). EndFreeze returns
    // true when the outermost freeze ends with m_rereadDue set and no reset hold open (an open hold
    // leaves it for EndHoldResets); the caller then calls Refresh() (re-read, one Reset), now or
    // posted. Reporting does not clear m_rereadDue: only Refresh() does, so a re-read that throws
    // for a reason other than validation is reported again at the next outermost EndFreeze. A
    // table diverged only by a rejected change does not report true: re-reading it would fail
    // validation again on every commit; the next notification re-reads instead. Item
    // PropertyChanged is not affected (it never mutates the table).
    void BeginFreeze() noexcept;
    [[nodiscard]] bool EndFreeze() noexcept;
    bool IsFrozen() const noexcept;

    // ---- Reset hold (decided, DeferRefresh) -------------------------------------------------
    // WPF model: an open DeferRefresh defers re-reads, not incremental changes. While held
    // (depth > 0) and not frozen, an incremental root- or child-collection change is applied and
    // raised as usual (the coordinator shapes it with the applied spec). A Reset (including a
    // multi-item change reported as one) is skipped, and from then on later notifications are
    // skipped as under a freeze until the re-read. Hierarchy verbs are parked (m_pendingHierarchy);
    // the current tree keeps receiving changes. Nests. EndHoldResets returns true when the
    // outermost hold ends with m_rereadDue set or a hierarchy parked; the caller then calls
    // Refresh(), which re-reads once and commits the parked hierarchy. Like EndFreeze, it does not
    // clear m_rereadDue.
    void BeginHoldResets() noexcept;
    [[nodiscard]] bool EndHoldResets() noexcept;

    // Asks for one re-read at the outermost EndHoldResets without marking the table diverged, so
    // incremental changes keep being applied until then. ShapingManager::Refresh() while a
    // DeferRefresh is open. Sets m_rereadDue; asserts a hold is open. Raises nothing.
    void ScheduleRefresh() noexcept;

    // ---- Divergence (decided) ---------------------------------------------------------------
    // m_diverged: the table no longer matches the source (see the table in the header comment).
    // Set by a skipped notification, a rejected incremental change or a Refresh() that fails
    // validation, and RequestReset (the ISourceHierarchyHost member a strategy calls for a
    // rejected child change). m_rereadDue: a re-read is owed (see the members). Both are cleared
    // only by a successful Refresh(). IsRereadDue is read by the posted resync, which does nothing
    // once the re-read has happened (a notification already re-read) or is no longer owed.
    bool IsRereadDue() const noexcept;

    // ---- Source tree (hierarchy) ------------------------------------------------------------

    // Top-level records of the source tree, in source order. Flat: the same records as Records().
    // Shaping walks these and SourceRecord::Children.
    std::vector<SourceRecord*> const& TopLevel() const noexcept;

    // True when a ParentBy or ChildrenBy strategy is committed (m_hierarchy set). A strategy parked
    // in m_pendingHierarchy (DeferRefresh) does not count until Refresh() commits it. Backs
    // ShapedRowQueries::IsHierarchical().
    bool HasHierarchy() const noexcept;

    // ParentBy and ChildrenBy construct the strategy, then Prepare() it (every app call and check;
    // a throw leaves the previous tree and strategy in place), TakeChildRecords() and Release() the
    // previous strategy, ResetTreeToFlat(), Commit() the new one and raise one Reset (Removed: the
    // previous child records). Must be called on the UI thread.
    // ParentBy: evaluates both selectors once per record (key rules in ParentKeyHierarchy).
    void SetParentKeyHierarchy(
        winrt::TableViewKeySelector const& keySelector,
        winrt::TableViewKeySelector const& parentKeySelector);

    // ChildrenBy: enumerates every non-null child collection recursively before creating any child
    // record, and subscribes to every observable child collection at Commit().
    void SetChildCollectionHierarchy(winrt::TableViewChildrenSelector2 const& childrenSelector);

    // Back to flat: TakeChildRecords() and Release() the strategy, ResetTreeToFlat(), then one
    // Reset. No-op when flat.
    // All three, while resets are held (an open DeferRefresh; ShapingManager rejects them during an
    // apply): park the unprepared strategy (nullptr for ClearHierarchy) in m_pendingHierarchy,
    // building nothing; the current tree stays and keeps receiving changes. The next Refresh()
    // prepares and commits it in place of the current one.
    void ClearHierarchy();

    // ---- Live shaping (per-record INotifyPropertyChanged) -----------------------------------

    // Subscribes every record in the tree (root and child collections) to INotifyPropertyChanged
    // when enabled, irrespective of active shaping verbs. WatchState::NotObservable stays cached
    // across toggles so non-observable items are probed only once.
    void SetLiveShaping(bool enabled);
    bool IsLiveShaping() const noexcept;

    // ---- Lookups ----------------------------------------------------------------------------

    // Exact raw-pointer hit, no QI. Always available.
    SourceRecord* TryFindByRawPointer(winrt::Windows::Foundation::IInspectable const& item) const noexcept;

    // Canonical IUnknown address hit. Always available; also backs duplicate-object detection.
    SourceRecord* TryFindByAddress(ObjectAddress address) const noexcept;

    // Raw-pointer lookup, falling back to one QI + address lookup when an item is reached through a
    // different interface pointer. Always O(1).
    SourceRecord* TryFindByItem(winrt::Windows::Foundation::IInspectable const& item) const;

    // Built in O(n) from existing records with no QI when the first shaping verb turns on; dropped
    // when the last verb turns off so unshaped tables keep Identity() lazy.
    void EnableIdentityIndex();
    void DisableIdentityIndex() noexcept;
    bool HasIdentityIndex() const noexcept;

    SourceRecord* TryFindByIdentity(winrt::hstring const& identity) const;

private:
    // Called by Create() once the object is owned by a shared_ptr: binds m_sourceAccessor,
    // subscribes to source changes with weak_from_this(), and performs the initial Refresh().
    void AttachSource(winrt::Windows::Foundation::IInspectable const& source);

    // Subscribes to the accessor's one notification interface (switch on GetObserveKind(); the
    // accessor owns the INotifyCollectionChanged > IObservableVector > IBindableObservableVector
    // precedence) and stores m_sourceEventToken.
    void SubscribeToSourceChanges();

    // Revokes through the same GetObserveKind() switch, using the accessor's safe_get (AsXxx(true)),
    // so a collected source is skipped. Swallows revoke failures.
    void UnsubscribeFromSourceChanges() noexcept;
    void OnSourceCollectionChanged(winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args);
    void OnSourceVectorChanged(winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args);
    void OnSourceBindableVectorChanged();

    // Refresh() step 2 for the root collection: match a fresh, validated materialization against
    // m_records, by raw pointer at the same position, then by raw pointer or address among the
    // unmatched. Returns, per position of items, the kept record or nullptr. Moves every unmatched
    // record out of m_records and the lookups into removed. Creates nothing, calls no app code.
    std::vector<SourceRecord*> MatchRecords(
        std::vector<winrt::Windows::Foundation::IInspectable> const& items,
        std::vector<SourceRecordPtr>& removed);

    // Refresh() step 3: rebuilds m_records in items order from the kept records, creating a record
    // for each nullptr in matches (appended to added). Calls no strategy hook: the strategy links
    // the tree in Commit().
    void CommitRecords(
        std::vector<winrt::Windows::Foundation::IInspectable> const& items,
        std::vector<SourceRecord*> const& matches,
        std::vector<SourceRecord*>& added);

    // Throws hresult_invalid_argument on a null item or an object that appears more than once.
    // No table state is touched.
    static void ValidateItems(std::vector<winrt::Windows::Foundation::IInspectable> const& items);

    // ValidateIncoming (host member, below): throws hresult_invalid_argument if any of `items` (an
    // incoming item plus what m_hierarchy->CollectIncoming adds) is null, appears twice in `items`,
    // or is already in the table outside `replacing` and its subtree (nullptr for an insert). Uses
    // m_rawLookup then m_addressLookup; no table state is touched.

    // Internal source edits applied from root-collection notifications. InsertAt and ReplaceAt call
    // ValidateIncoming() (over the item plus m_hierarchy->CollectIncoming) before mutating; on
    // failure they set m_diverged and rethrow. Each then calls the strategy's OnInserted /
    // OnRemoving / OnMoved (a replace is OnRemoving then OnInserted) before raising.
    void InsertAt(uint32_t index, winrt::Windows::Foundation::IInspectable const& item);
    void RemoveAt(uint32_t index);
    void ReplaceAt(uint32_t index, winrt::Windows::Foundation::IInspectable const& item);
    void Move(uint32_t fromIndex, uint32_t toIndex);

    // CreateRecord and RaiseSourceChanged are host members (below). RaiseSourceChanged raises
    // SourceChanged inside a freeze (BeginFreeze / EndFreeze around the handler call; nothing is
    // queued: a re-entrant change is skipped), then destroys the records that left;
    // records in `removed` must already be out of their owner, the tree and the lookups. It fills
    // m_removedView from `removed` for SourceChange::Removed (see SourceRecord.h).

    // At the end of a notification handler, root or child: if the dispatch's EndFreeze returned
    // true (re-read due, outermost, no hold), Refresh() (one Reset). This is the turn's one
    // synchronous re-read; a change made during that Reset is posted by Refresh (PostReread).
    void DrainPendingChanges();

    // Posts one re-read to the current thread's DispatcherQueue (as ShapingManager::PostLiveDrain
    // does; it only runs on the UI thread, so it does not depend on m_binding) unless one is
    // already posted (m_rereadPosted). The
    // callback holds weak_from_this() and returns if the table is gone or detached; it clears
    // m_rereadPosted, then calls Refresh() only if IsRereadDue() and the table is neither frozen
    // nor held (otherwise the owed re-read is left for that freeze's or hold's report point). A
    // throw from it is swallowed: the flags follow Refresh()'s failure rules. No queue (headless
    // or test): nothing is posted and the re-read stays owed until the next report point or
    // Refresh().
    void PostReread() noexcept;

    void AddToLookups(SourceRecord* record);

    // Incremental paths only (reads Item.get()); Clear() clears the maps wholesale instead.
    void RemoveFromLookups(SourceRecord* record) noexcept;

    // The PropertyChanged lambda captures weak_from_this(), the record's weak_ptr and m_binding.
    // Bound and off the bound thread, it posts to the bound queue (a failed post is ignored);
    // otherwise it locks the table and calls OnRecordPropertyChanged, which locks the record.
    void SubscribeRecordPropertyChanged(SourceRecordPtr const& record);
    void UnsubscribeRecordPropertyChanged(SourceRecord* record) noexcept;
    void OnRecordPropertyChanged(SourceRecordWeakPtr const& weakRecord, winrt::hstring const& propertyName);

    // ---- Source tree: shared ----------------------------------------------------------------

    // SiblingsOf and Reparent are host members (below). SiblingsOf: the children list a record
    // with this parent lives in (parent->Children, or m_topLevel for nullptr); used only by tree
    // maintenance, readers walk Parent / Children directly. Reparent: unlinks the record and links
    // it under newParent at index, then UpdateDepths for the moved subtree.
    void UpdateDepths(SourceRecord* record) noexcept;

    // Makes every root record top-level with no children and Depth 0. Raises nothing; callers
    // release the old strategy first (it destroys the child feeds) and raise the Reset.
    void ResetTreeToFlat() noexcept;

    // Called from OnRecordPropertyChanged when m_hierarchy is set: m_hierarchy->Reevaluate, before
    // ItemChanged is forwarded. Returns true if the tree changed.
    bool ReevaluateHierarchy(SourceRecord* record, winrt::hstring const& propertyName);

    // ---- ISourceHierarchyHost (private; see Source/SourceHierarchy.h) ------------------------

    ITrackerHandleManager const* TrackerOwner() const noexcept override;
    std::weak_ptr<ISourceHierarchyHost> WeakHost() noexcept override;
    SourceHierarchy* CurrentHierarchy() noexcept override;
    std::vector<SourceRecordPtr> const& RootRecords() const noexcept override;
    SourceRecordPtr CreateRecord(winrt::Windows::Foundation::IInspectable const& item) override;
    void ReleaseRecord(SourceRecord* record) noexcept override;
    void ValidateIncoming(
        std::vector<winrt::Windows::Foundation::IInspectable> const& items,
        SourceRecord const* replacing) const override;
    std::vector<SourceRecord*>& SiblingsOf(SourceRecord* parent) noexcept override;
    void Reparent(SourceRecord* record, SourceRecord* newParent, uint32_t index) override;
    bool AdmitChange(bool isReset) override;
    void RaiseSourceChanged(
        SourceChangeKind kind,
        SourceRecord* oldParent,
        SourceRecord* newParent,
        uint32_t oldIndex,
        uint32_t newIndex,
        std::vector<SourceRecord*> const& added,
        std::vector<SourceRecordPtr>& removed) override;
    void RequestReset() noexcept override;

    // Null once detached (Clear()).
    ITrackerHandleManager const* m_trackerOwner{ nullptr };

    // Optional so Clear() can destroy its tracker_refs while the owner is alive.
    std::optional<CollectionAccessor2> m_sourceAccessor;
    // Stored as a token (like InspectingDataSource::m_eventToken) rather than winrt::event_revoker,
    // which holds an untracked strong COM reference to the source and would defeat tracker_ref.
    winrt::event_token m_sourceEventToken{};

    std::vector<SourceRecordPtr> m_records;

    // Backs TopLevel() (non-owning).
    std::vector<SourceRecord*> m_topLevel;

    // Scratch for SourceChange::Removed: RaiseSourceChanged clears it, copies the raw pointers of
    // its owning `removed` vector in, raises, then clears it again. One buffer is enough: the
    // freeze around the handler skips a re-entrant change, so raises never nest. Keeps its
    // capacity across changes.
    std::vector<SourceRecord const*> m_removedView;

    // The current hierarchy strategy; null = flat. Its selectors and keys are tracked, so Clear()
    // releases it first, while m_trackerOwner is alive. Kept off SourceRecord so flat tables pay
    // nothing.
    std::unique_ptr<SourceHierarchy> m_hierarchy;

    // A hierarchy verb received while resets are held (DeferRefresh): the unprepared strategy, or
    // nullptr for ClearHierarchy; nullopt when none is pending. The next Refresh() prepares and
    // commits it instead of re-preparing m_hierarchy: a throw discards it and leaves the current
    // tree and strategy in place. A later verb while still held replaces it. Clear() drops it.
    std::optional<std::unique_ptr<SourceHierarchy>> m_pendingHierarchy;

    std::unordered_map<void*, SourceRecord*> m_rawLookup;
    std::unordered_map<ObjectAddress, SourceRecord*> m_addressLookup;

    bool m_identityIndexEnabled{ false };
    std::unordered_map<winrt::hstring, SourceRecord*> m_identityIndex;

    bool m_liveShapingEnabled{ false };

    // Thread binding (see Threading). Written once: at Create() on a dispatcher thread, or by the
    // first BindOwnerThread() while unbound; never changed after. Shared so the item lambdas read
    // it without locking the table off-thread. threadId == 0 means unbound; it is stored after
    // queue with release order and loaded with acquire, so a reader that sees it also sees queue.
    struct ThreadBinding
    {
        winrt::Microsoft::UI::Dispatching::DispatcherQueue queue{ nullptr };
        std::atomic<DWORD> threadId{ 0 };
    };
    std::shared_ptr<ThreadBinding> m_binding{ std::make_shared<ThreadBinding>() };

    SourceChangedHandler m_sourceChanged;
    ItemChangedHandler m_itemChanged;

    // Re-entrancy (decided, #19). Frozen while > 0: the SourceChanged dispatch and the
    // coordinator's commit scope each hold one. A notification while frozen is not applied and
    // only sets m_diverged and m_rereadDue. Nothing is removed while frozen, so no removed records
    // are held.
    uint32_t m_freezeDepth{ 0 };

    // Open reset holds (DeferRefresh). While > 0, a Reset is skipped instead of re-read.
    uint32_t m_resetHoldDepth{ 0 };

    // The table does not match the source (see Divergence). While set, every notification, root
    // or child, is handled as a whole-tree re-read, skipped while frozen or held; positional
    // deltas are never applied. Cleared only by a successful Refresh(). One flag for what were
    // three causes: a change while frozen, a Reset while held, a rejected change.
    bool m_diverged{ false };

    // A re-read is owed: reported at each report point (outermost EndFreeze with no hold, or
    // outermost EndHoldResets) until a Refresh() clears it. Set by a skipped notification and by
    // ScheduleRefresh; not by a rejected change, so a table diverged only by rejection is re-read
    // by the next notification rather than by every commit. Cleared by a successful Refresh() and
    // by one that fails validation (which turns it into a rejection); kept when Refresh() throws
    // for any other reason, so the next report point retries.
    bool m_rereadDue{ false };

    // A PostReread() callback is queued and has not run yet.
    bool m_rereadPosted{ false };
};
