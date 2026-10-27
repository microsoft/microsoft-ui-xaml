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

enum class HierarchyKind : uint8_t
{
    None,              // Flat: every record is top-level.
    ParentKey,         // ParentBy(key, parentKey) over the root collection.
    ChildCollections,  // ChildrenBy(childrenSelector), eagerly materialized.
};

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
//   - Flat (HierarchyKind::None): every record is top-level.
//   - ParentKey (ParentBy): every item is in the root collection, so every record is in Records().
//     The tree is an index over them (m_keyIndex, m_nodeKeys). Key rules follow v1
//     (Hierarchical-Data-Rows-Parent-Key-Design.md section 3): value keys compare by value, object
//     keys by reference; a null/empty parent key or an orphan makes a top-level record; a null/empty
//     key, a duplicate key, a self-parent or a cycle (found by counting records reachable from the
//     top level) throws hresult_invalid_argument before any mutation.
//   - ChildCollections (ChildrenBy): Records() are the top level. Every other record is owned by
//     its parent's ChildFeed (CollectionAccessor2 + event token + ordered records), one per
//     non-null child collection, enumerated eagerly and recursively. A child added to any
//     collection brings its whole subtree. Child collections are source: their changes are tracked
//     whether or not live shaping is on.
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
//     The source has already changed, so the table is out of sync: m_outOfSync is set and every
//     later notification is handled as a Reset (re-enumerate + full validation) until one succeeds,
//     which raises a single Reset and clears the flag. Positional deltas are never applied on
//     top of a diverged table.
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
//     one Reset. A change raised re-entrantly while frozen (decided, #19: inside a SourceChanged
//     dispatch or a Freeze scope) is not applied: the table is left as it was, m_pendingReset is
//     set, and the source is re-read with one Reset when the outermost dispatch or freeze ends.
//     OPEN: under ParentBy one source change can produce several tree changes (an insert adopts
//     existing orphans, each a Move from top level to the new item). Deliver them as an ordered
//     batch in one dispatch, or collapse to Reset above a threshold.
//   - ItemChanged: forwards (SourceRecord*, propertyName) when a subscribed record raises
//     PropertyChanged. A null record means "unknown items changed" (see Threading); an empty
//     propertyName means all properties changed.
//
// Threading (same contract as v1 ShapedItemsSource):
//   - Create() captures the owning thread's DispatcherQueue and thread id. All table state, and
//     both handlers, belong to that thread.
//   - Source and child collection notifications must arrive on the owning thread (the contract
//     every XAML collection consumer imposes); they are not marshalled.
//   - Item PropertyChanged raised on another thread is posted to m_ownerQueue with only weak
//     pointers and the property name; the weak pointers are locked there. If the post fails, or
//     there is no queue and the thread differs, the change is dropped and m_itemChangeDropped is
//     set. The next ItemChanged delivered on the owning thread is preceded by
//     ItemChanged(nullptr, L"") so the coordinator re-evaluates every record. The flag is cleared
//     by Refresh() and SetLiveShaping(false).
//
// Lifetime: always owned by std::shared_ptr and created only through Create(). Every event lambda
// registered on the source, a child collection or an item captures weak_from_this() (plus the
// owning record's weak_ptr for child-collection and item events) and drops the notification if
// either has expired, so a late or in-flight event can never touch a destroyed table. The weak
// pointers are locked only on the owning UI thread: a lock taken on a worker thread could become
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
//     feeds (and their records) first, then every root record, resets m_sourceAccessor, the
//     selectors and the key index, and nulls m_trackerOwner. The table is then detached: empty, owner-free, and safe to outlive
//     TableViewSource2 (consumer shared_ptrs, a posted off-thread lambda that has the table locked).
//     Every public entry point and event lambda returns early when detached. ~SourceTable() asserts
//     it was detached.
//   - tracker_ref holds no reference count itself; the owner's tracker handle does. When the GC
//     collects a cycle (item <-> VM <-> TableViewSource2), items and the collection may already be
//     collected when Clear() runs, so teardown never calls tracker_ref::get(): handlers are nulled
//     first (Clear() raises nothing), lookups are cleared wholesale, and every revoke goes through
//     safe_get().
class SourceTable : public std::enable_shared_from_this<SourceTable>
{
    struct PrivateToken { explicit PrivateToken() = default; };

public:
    using SourceChangedHandler = std::function<void(SourceChange const& change)>;
    using ItemChangedHandler = std::function<void(SourceRecord* record, winrt::hstring const& propertyName)>;

    // Constructs the table, captures the owning thread's DispatcherQueue and thread id, resolves
    // CollectionAccessor2(owner, source), subscribes to source change notifications, and performs
    // the initial Refresh(). Must be called on the owning (UI) thread.
    static std::shared_ptr<SourceTable> Create(
        ITrackerHandleManager const* owner,
        winrt::Windows::Foundation::IInspectable const& source);

    // Public only so std::make_shared can reach it; PrivateToken restricts callers to Create().
    SourceTable(PrivateToken, ITrackerHandleManager const* owner);

    // Asserts the table was detached by Clear(); never touches m_trackerOwner.
    ~SourceTable();

    SourceTable(SourceTable const&) = delete;
    SourceTable& operator=(SourceTable const&) = delete;

    // ---- Source lifecycle & records ---------------------------------------------------------

    // Re-enumerates the bound source (and, under ChildrenBy, every child collection; under ParentBy,
    // every key) and reconciles against existing records (initial load, Reset, or explicit
    // Refresh() for non-observable sources). No-op when detached.
    void Refresh();

    // Teardown (see Detach contract); must run on the UI thread while m_trackerOwner is alive.
    // Also unsubscribes every child collection and clears the key index. Idempotent.
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
    // Children and hold raw SourceRecord pointers across app code. A notification only sets
    // m_pendingReset (the source has diverged). Nests. ShapingManager freezes around every engine
    // call and Publish. EndFreeze returns true when the outermost freeze ends with m_pendingReset
    // set; the caller then calls Refresh() (re-read, one Reset), now or posted. Item
    // PropertyChanged is not affected (it never mutates the table).
    void BeginFreeze() noexcept;
    [[nodiscard]] bool EndFreeze() noexcept;
    bool IsFrozen() const noexcept;

    // ---- Source tree (hierarchy) ------------------------------------------------------------

    // Top-level records of the source tree, in source order. Flat: the same records as Records().
    // Shaping walks these and SourceRecord::Children.
    std::vector<SourceRecord*> const& TopLevel() const noexcept;

    HierarchyKind GetHierarchyKind() const noexcept;

    // ParentBy. Evaluates both selectors once per record and validates the whole tree (key rules in
    // the class comment); a throw leaves the previous tree in place. Then replaces the previous
    // hierarchy, links every record, and raises one Reset. Must be called on the UI thread.
    void SetParentKeyHierarchy(
        winrt::TableViewKeySelector2 const& keySelector,
        winrt::TableViewKeySelector2 const& parentKeySelector);

    // ChildrenBy. Enumerates every non-null child collection recursively and validates the whole
    // tree before creating any child record; a throw leaves the previous tree in place. Subscribes
    // to every observable child collection and raises one Reset. Must be called on the UI thread.
    void SetChildCollectionHierarchy(winrt::TableViewChildrenSelector2 const& childrenSelector);

    // Back to flat: ResetTreeToFlat(), resets the selectors, then one Reset. No-op when flat.
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

    // Subscribes to exactly one notification interface, in INotifyCollectionChanged >
    // IObservableVector > IBindableObservableVector precedence, and stores m_sourceEventToken.
    void SubscribeToSourceChanges();

    // Re-derives the subscribed interface with the same precedence, through the accessor's
    // safe_get (AsXxx(true)), so a collected source is skipped. Swallows revoke failures.
    void UnsubscribeFromSourceChanges() noexcept;
    void OnSourceCollectionChanged(winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args);
    void OnSourceVectorChanged(winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args);
    void OnSourceBindableVectorChanged();

    // Rebuild against a fresh materialization: reuse by raw pointer at the same position, then by
    // raw pointer or address among unmatched records; create the rest; remove what is left over.
    // Callers validate with ValidateItems() first.
    void Reconcile(std::vector<winrt::Windows::Foundation::IInspectable> const& items);

    // Throws hresult_invalid_argument on a null item or an object that appears more than once.
    // No table state is touched.
    static void ValidateItems(std::vector<winrt::Windows::Foundation::IInspectable> const& items);

    // Throws hresult_invalid_argument if any of `items` (an incoming item plus, under ChildrenBy, its
    // whole materialized subtree) is null, appears twice in `items`, or is already in the table
    // outside `replacing` and its subtree (pass nullptr for an insert). Uses m_rawLookup then
    // m_addressLookup; no table state is touched.
    void ValidateIncoming(
        std::vector<winrt::Windows::Foundation::IInspectable> const& items,
        SourceRecord const* replacing) const;

    // Internal source edits applied from root-collection notifications. InsertAt and ReplaceAt call
    // ValidateIncoming() before mutating; on failure they set m_outOfSync and rethrow. Each also
    // updates the tree (ParentKey: key index, parent link, orphan adoption; ChildCollections: build
    // or destroy the record's subtree) before raising.
    void InsertAt(uint32_t index, winrt::Windows::Foundation::IInspectable const& item);
    void RemoveAt(uint32_t index);
    void ReplaceAt(uint32_t index, winrt::Windows::Foundation::IInspectable const& item);
    void Move(uint32_t fromIndex, uint32_t toIndex);

    SourceRecordPtr CreateRecord(winrt::Windows::Foundation::IInspectable const& item);

    // Raise SourceChanged (or queue it while a dispatch is in progress), then destroy the records
    // that left. Records in `removed` must already be out of their owner, the tree and the lookups.
    void RaiseSourceChanged(
        SourceChangeKind kind,
        SourceRecord* oldParent,
        SourceRecord* newParent,
        uint32_t oldIndex,
        uint32_t newIndex,
        std::vector<SourceRecord*> const& added,
        std::vector<SourceRecordPtr>& removed);

    // After the outermost dispatch returns (and no freeze is held): if a notification arrived
    // while frozen, Refresh() (one Reset).
    void DrainPendingChanges();

    void AddToLookups(SourceRecord* record);

    // Incremental paths only (reads Item.get()); Clear() clears the maps wholesale instead.
    void RemoveFromLookups(SourceRecord* record) noexcept;

    // The PropertyChanged lambda captures weak_from_this(), the record's weak_ptr, m_ownerQueue,
    // m_ownerThreadId and m_itemChangeDropped. Off the owning thread it posts to m_ownerQueue (or
    // sets the dropped flag); on it, it locks the table and calls OnRecordPropertyChanged, which
    // locks the record.
    void SubscribeRecordPropertyChanged(SourceRecordPtr const& record);
    void UnsubscribeRecordPropertyChanged(SourceRecord* record) noexcept;
    void OnRecordPropertyChanged(SourceRecordWeakPtr const& weakRecord, winrt::hstring const& propertyName);

    // ---- Source tree: shared ----------------------------------------------------------------

    // Children list a record with this parent lives in: parent->Children, or m_topLevel for
    // nullptr. Used only by tree maintenance (insert, remove, reparent, sibling index for
    // SourceChange); readers walk Parent / Children directly and stop at nullptr.
    std::vector<SourceRecord*>& SiblingsOf(SourceRecord* parent) noexcept;

    // Unlinks `record` from its current parent and links it under `newParent` at `index`, then
    // recomputes Depth for the moved subtree.
    void Reparent(SourceRecord* record, SourceRecord* newParent, uint32_t index);
    void UpdateDepths(SourceRecord* record) noexcept;

    // Makes every root record top-level with no children and Depth 0, destroys the child feeds and
    // clears the key index. Raises nothing; callers raise the Reset.
    void ResetTreeToFlat() noexcept;

    // Called from OnRecordPropertyChanged under a hierarchy: re-evaluates the record's key and parent
    // key (reparent) or its children property (swap the feed), and raises the resulting
    // SourceChange before ItemChanged is forwarded. Returns true if the tree changed.
    bool ReevaluateHierarchy(SourceRecord* record);

    // ---- Source tree: ParentKey -------------------------------------------------------------

    // Lookup form of a selector result (v1 MakeNodeKey): value types by value, objects by
    // reference. Empty for null.
    static winrt::hstring MakeNodeKey(winrt::Windows::Foundation::IInspectable const& key);

    // Evaluates and validates keys into scratch state (throws before mutating), then links the tree.
    void BuildParentKeyTree();

    // Counts records reachable from the top level; any unreached record is on or below a cycle.
    void ValidateNoCycles() const;

    // ---- Source tree: ChildCollections ------------------------------------------------------

    // One tracked child collection.
    struct ChildFeed
    {
        // The record whose children this feed holds. Weak; see Lifetime.
        SourceRecordWeakPtr Owner;

        // Optional so Clear() can destroy its tracker_refs while m_trackerOwner is alive.
        std::optional<CollectionAccessor2> Accessor;
        winrt::event_token EventToken{};

        // Sole strong references to this collection's records, in Owner->Children order.
        std::vector<SourceRecordPtr> Records;

        // Set when a change to this collection was rejected (null/duplicate). The next notification
        // for this feed rebuilds its subtree as a Reset of that subtree.
        bool OutOfSync{ false };
    };

    // Evaluates childrenSelector for `record` and, recursively, its descendants, enumerating each
    // non-null child collection into `items` (pre-order) without creating anything, so the whole
    // subtree can be validated first.
    void CollectSubtreeItems(
        winrt::Windows::Foundation::IInspectable const& item,
        std::vector<winrt::Windows::Foundation::IInspectable>& items) const;

    // Creates `record`'s feed (if its collection is non-null), its child records and, recursively,
    // their subtrees, and links them. Callers validate the whole subtree first.
    void BuildChildFeed(SourceRecordPtr const& record);

    // Unsubscribes (safe_get), resets the accessor, removes the feed's records and their subtrees
    // from the lookups and destroys them. Teardown-safe.
    void DestroyChildFeed(SourceRecord const* owner) noexcept;

    void SubscribeToChildChanges(ChildFeed& feed);
    void UnsubscribeFromChildChanges(ChildFeed& feed) noexcept;

    // A rejected change sets the feed's OutOfSync.
    void OnChildCollectionChanged(SourceRecordWeakPtr const& owner, winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args);
    void OnChildVectorChanged(SourceRecordWeakPtr const& owner, winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args);
    void OnChildBindableVectorChanged(SourceRecordWeakPtr const& owner);

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

    HierarchyKind m_hierarchyKind{ HierarchyKind::None };

    // App delegates, tracked (see Reference tracking; v1 held them strongly). Optional so
    // ClearHierarchy() and Clear() can destroy them while m_trackerOwner is alive. Invoked only on
    // live UI-thread paths, never during teardown.
    std::optional<tracker_ref<winrt::TableViewKeySelector2>> m_keySelector;
    std::optional<tracker_ref<winrt::TableViewKeySelector2>> m_parentKeySelector;
    std::optional<tracker_ref<winrt::TableViewChildrenSelector2>> m_childrenSelector;

    // ParentKey: node key -> record, and each record's cached keys. Object keys are held while the
    // tree is current so their addresses cannot be reused (v1 section 3.1). Kept off SourceRecord
    // so flat tables pay nothing.
    struct NodeKeys
    {
        winrt::hstring Key;
        winrt::hstring ParentKey;
        // Object keys only (value keys need only Key); tracked like Item.
        std::optional<tracker_ref<winrt::Windows::Foundation::IInspectable>> KeyObject;
    };
    std::unordered_map<winrt::hstring, SourceRecord*> m_keyIndex;
    std::unordered_map<SourceRecord const*, NodeKeys> m_nodeKeys;

    // ChildCollections: one feed per record with a non-null child collection.
    std::unordered_map<SourceRecord const*, std::unique_ptr<ChildFeed>> m_childFeeds;

    std::unordered_map<void*, SourceRecord*> m_rawLookup;
    std::unordered_map<ObjectAddress, SourceRecord*> m_addressLookup;

    bool m_identityIndexEnabled{ false };
    std::unordered_map<winrt::hstring, SourceRecord*> m_identityIndex;

    bool m_liveShapingEnabled{ false };

    // Owning thread. Null queue off a dispatcher thread: item changes then run inline on the owner
    // thread and are flagged as dropped on others.
    winrt::Microsoft::UI::Dispatching::DispatcherQueue m_ownerQueue{ nullptr };
    DWORD m_ownerThreadId{ 0 };

    // Set off-thread when an item PropertyChanged could not be marshalled to the owner. Shared so
    // the subscription lambda can set it without locking the table off-thread.
    std::shared_ptr<std::atomic<bool>> m_itemChangeDropped{ std::make_shared<std::atomic<bool>>(false) };

    SourceChangedHandler m_sourceChanged;
    ItemChangedHandler m_itemChanged;

    // Re-entrancy (decided, #19). The table is frozen while m_isDispatching or m_freezeDepth > 0:
    // a source notification is not applied and only sets m_pendingReset, which a Refresh() (one
    // Reset) clears. Nothing is removed while frozen, so no removed records are held.
    bool m_isDispatching{ false };
    uint32_t m_freezeDepth{ 0 };
    bool m_pendingReset{ false };

    // Set when an incremental notification was rejected (null/duplicate). The source has diverged
    // from m_records, so every later notification is handled as a Reset until validation succeeds.
    bool m_outOfSync{ false };
};
