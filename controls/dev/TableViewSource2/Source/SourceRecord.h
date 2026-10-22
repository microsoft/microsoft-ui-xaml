// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <atomic>
#include <cstddef>
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

// Shared source tracking for TableViewSource2
// (docs/design-notes/TabularControls/TableView-Row-Identity-Reuse-Spec.md).
//
// One SourceRecord per source position holds the values that depend only on the object: the item,
// its canonical address, its lazily formatted identity, and its live-shaping subscription. Every
// consumer that asks "which row is this?" reads these instead of re-deriving them.
//
// UI-thread affine. SourceTable holds the sole strong shared_ptr<SourceRecord> per entry so async
// PropertyChanged callbacks can safely capture weak_ptr<SourceRecord>, while downstream stages hold
// non-owning SourceRecord* pointers and drop them synchronously when notified before destruction.

// Canonical IUnknown pointer of an item. Physical identity: stable for the object's lifetime.
using ObjectAddress = uintptr_t;

// Whether the item has been probed for INotifyPropertyChanged, and the result. Cached per record
// because a heterogeneous source can mix observable and non-observable items.
enum class WatchState : uint8_t
{
    Unchecked,
    NotObservable,
    Subscribed,
};

struct SourceRecord
{
    SourceRecord(
        ITrackerHandleManager const* owner,
        winrt::Windows::Foundation::IInspectable const& item);

    // Revokes PropertyChangedToken through Item.safe_get(): during a GC-driven teardown the item may
    // already be collected, in which case the revoke is skipped. Never uses Item.get().
    ~SourceRecord();

    SourceRecord(SourceRecord const&) = delete;
    SourceRecord& operator=(SourceRecord const&) = delete;

    // Reference-tracked through the owning TableViewSource2 (like Vector<IInspectable>) so the .NET
    // GC can collect cycles when an item references its owner. Kept alive until ~SourceRecord() so
    // COM addresses are not reused while the record is in the table. get_abi(Item.get()) is the
    // raw-pointer lookup key. Item.get() is valid only on live paths (incremental changes, Refresh,
    // PropertyChanged handling); teardown paths use Item.safe_get().
    tracker_ref<winrt::Windows::Foundation::IInspectable> Item;

    // One QI at creation.
    ObjectAddress Address{ 0 };

    // Live-shaping subscription; empty token when not subscribed. Revoked in ~SourceRecord().
    winrt::event_token PropertyChangedToken{};
    WatchState Watch{ WatchState::Unchecked };

    // Logical identity, formatted on first use and then cached. The only way to read it.
    winrt::hstring const& Identity() const;

private:
    mutable winrt::hstring m_identity;
};

using SourceRecordPtr = std::shared_ptr<SourceRecord>;
using SourceRecordWeakPtr = std::weak_ptr<SourceRecord>;

// Change kind reuses the platform's collection-change action (Add, Remove, Replace, Move, Reset).
using SourceChangeKind = winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedAction;

// One normalized, record-level source change. Raised after m_records and the lookups are updated
// and before the removed records are destroyed.
struct SourceChange
{
    SourceChangeKind Kind{ SourceChangeKind::Reset };

    // Source position before the change. Remove, Replace, Move.
    uint32_t OldIndex{ 0 };

    // Source position after the change. Add, Replace, Move.
    uint32_t NewIndex{ 0 };

    // Records now in the table. Add and Replace carry one; Reset carries every new record.
    std::vector<SourceRecord*> const& Added;

    // Records that left the table. Alive (Item, Identity()) until the handler returns.
    // Remove and Replace carry one; Reset carries every record that left.
    std::vector<SourceRecord const*> const& Removed;
};

// The record table for one input source, plus its lookups.
//
//   Records()      one record per source position, in source order
//   raw lookup     get_abi(item) -> record   (always on; pipeline map-back, no QI)
//   address lookup Address       -> record   (always on; duplicate check + COM identity lookup)
//   identity index Identity      -> record   (shaped only; selection, focus, expansion lookups)
//
// Owns the CollectionAccessor2 for the bound source, subscribes to its change notifications, and
// keeps m_records and the lookup maps synchronized automatically. Duplicate objects and null items
// are rejected upfront in all modes (shaped or not), so every key maps to at most one record. This
// is stricter than v1, which allowed both while unshaped. The identity index is
// built only while shaping is active so unshaped tables avoid formatting identity strings.
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
//       1. m_records and the lookups are updated.
//       2. SourceChanged is raised synchronously on the UI thread (single handler: the coordinator,
//          which fans out to engine, projection and owner in a fixed order).
//       3. Removed records are destroyed (revoking any live PropertyChangedToken).
//     Multi-item notifications are split into single-item changes or reported as one Reset. A change
//     raised re-entrantly from inside the handler is queued and delivered after the handler returns;
//     several queued changes may be coalesced into one Reset.
//   - ItemChanged: forwards (SourceRecord*, propertyName) when a subscribed record raises
//     PropertyChanged. A null record means "unknown items changed" (see Threading); an empty
//     propertyName means all properties changed.
//
// Threading (same contract as v1 ShapedItemsSource):
//   - Create() captures the owning thread's DispatcherQueue and thread id. All table state, and
//     both handlers, belong to that thread.
//   - Source collection notifications must arrive on the owning thread (the contract every XAML
//     collection consumer imposes); they are not marshalled.
//   - Item PropertyChanged raised on another thread is posted to m_ownerQueue with only weak
//     pointers and the property name; the weak pointers are locked there. If the post fails, or
//     there is no queue and the thread differs, the change is dropped and m_itemChangeDropped is
//     set. The next ItemChanged delivered on the owning thread is preceded by
//     ItemChanged(nullptr, L"") so the coordinator re-evaluates every record. The flag is cleared
//     by Refresh() and SetLiveShaping(false).
//
// Lifetime: always owned by std::shared_ptr and created only through Create(). Every event lambda
// registered on the source or on an item captures weak_from_this() (plus the record's weak_ptr for
// item events) and drops the notification if either has expired, so a late or in-flight event can
// never touch a destroyed table. The weak pointers are locked only on the owning UI thread: a lock
// taken on a worker thread could become the last strong reference and run ~SourceTable() (and its
// UI-affine revokes) there. Subscriptions start in Create(), after construction,
// because weak_from_this() is empty inside the constructor.
//
// Reference tracking: m_trackerOwner points to the owning TableViewSource2 (an ITrackerHandleManager).
// Both CollectionAccessor2 and each SourceRecord::Item hold their references through tracker_ref
// bound to m_trackerOwner, and source-change subscriptions store a raw event_token rather than
// winrt::event_revoker (which holds an untracked strong COM pointer). Each record costs one
// NewTrackerHandle + SetTrackerValue on creation and one DeleteTrackerHandle on removal (same as
// Vector<IInspectable>).
//
// Detach contract:
//   - ~tracker_ref dereferences its owner even when empty, so no tracker_ref may outlive
//     m_trackerOwner. ~TableViewSource2() calls Clear() on the UI thread, which destroys every
//     record, resets m_sourceAccessor, and nulls m_trackerOwner. The table is then detached: empty,
//     owner-free, and safe to outlive TableViewSource2 (consumer shared_ptrs, a posted off-thread
//     lambda that has the table locked). Every public entry point and event lambda returns early
//     when detached. ~SourceTable() asserts it was detached.
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

    // Re-enumerates the bound source and reconciles against existing records (initial load, Reset,
    // or explicit Refresh() for non-observable sources). No-op when detached.
    void Refresh();

    // Teardown; must run on the UI thread while m_trackerOwner is alive (~TableViewSource2()).
    // Nulls both handlers (raises nothing), unsubscribes from the source via safe_get(), clears the
    // lookups wholesale, destroys every record, resets m_sourceAccessor, and nulls m_trackerOwner.
    // Idempotent. Afterwards the table is detached and every entry point is a no-op.
    void Clear();

    bool IsDetached() const noexcept;

    uint32_t Size() const noexcept;
    SourceRecord* At(uint32_t index) const;
    std::vector<SourceRecordPtr> const& Records() const noexcept;

    void SetSourceChangedHandler(SourceChangedHandler handler);
    void SetItemChangedHandler(ItemChangedHandler handler);

    // ---- Live shaping (per-record INotifyPropertyChanged) -----------------------------------

    // Subscribes every record to INotifyPropertyChanged when enabled, irrespective of active
    // shaping verbs. WatchState::NotObservable stays cached across toggles so non-observable items
    // are probed only once.
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

    // Throws hresult_invalid_argument if `item` is null or already present in the table at any
    // position other than `replacingIndex` (pass UINT32_MAX for an insert). Uses m_rawLookup then
    // m_addressLookup; no table state is touched.
    void ValidateIncoming(winrt::Windows::Foundation::IInspectable const& item, uint32_t replacingIndex) const;

    // Internal source edits applied from change notifications. InsertAt and ReplaceAt call
    // ValidateIncoming() before mutating; on failure they set m_outOfSync and rethrow.
    void InsertAt(uint32_t index, winrt::Windows::Foundation::IInspectable const& item);
    void RemoveAt(uint32_t index);
    void ReplaceAt(uint32_t index, winrt::Windows::Foundation::IInspectable const& item);
    void Move(uint32_t fromIndex, uint32_t toIndex);

    SourceRecordPtr CreateRecord(winrt::Windows::Foundation::IInspectable const& item);

    // Raise SourceChanged (or queue it while a dispatch is in progress), then destroy the records
    // that left. Records in `removed` must already be out of m_records and the lookups.
    void RaiseSourceChanged(
        SourceChangeKind kind,
        uint32_t oldIndex,
        uint32_t newIndex,
        std::vector<SourceRecord*> const& added,
        std::vector<SourceRecordPtr>& removed);

    // Deliver queued re-entrant changes after the outermost dispatch returns.
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

    // Null once detached (Clear()).
    ITrackerHandleManager const* m_trackerOwner{ nullptr };

    // Optional so Clear() can destroy its tracker_refs while the owner is alive.
    std::optional<CollectionAccessor2> m_sourceAccessor;
    // Stored as a token (like InspectingDataSource::m_eventToken) rather than winrt::event_revoker,
    // which holds an untracked strong COM reference to the source and would defeat tracker_ref.
    winrt::event_token m_sourceEventToken{};

    std::vector<SourceRecordPtr> m_records;

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

    // Re-entrancy guard. While true, source notifications are applied to the table but their
    // SourceChanged is deferred; m_pendingReset collapses deferred changes into one Reset.
    bool m_isDispatching{ false };
    bool m_pendingReset{ false };
    std::vector<SourceRecordPtr> m_pendingRemoved;

    // Set when an incremental notification was rejected (null/duplicate). The source has diverged
    // from m_records, so every later notification is handled as a Reset until validation succeeds.
    bool m_outOfSync{ false };
};
