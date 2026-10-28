// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include <winrt/Microsoft.UI.Xaml.Interop.h>
#include <winrt/Windows.Foundation.h>

#include "tracker_ref.h"
#include "Engine/ShapeState.h"

// Per-item source record and record-level change for TableViewSource2
// (docs/design-notes/TabularControls/TableView-Row-Identity-Reuse-Spec.md).
//
// One SourceRecord per source position holds the values that depend only on the object: the item,
// its canonical address, its lazily formatted identity, and its live-shaping subscription. Every
// consumer that asks "which row is this?" reads these instead of re-deriving them.
//
// UI-thread affine. SourceTable (SourceTable.h) holds the sole strong shared_ptr<SourceRecord> per
// entry so async PropertyChanged callbacks can safely capture weak_ptr<SourceRecord>, while
// downstream stages hold non-owning SourceRecord* pointers and drop them synchronously when notified
// before destruction.

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

// Derives from ShapedNode (Engine/ShapeState.h, Kind = Record), so a record is a node of the shaped
// tree as it stands: the shaped tree's per-node counts and position live on that base. The source
// tree (Parent / Children below) is not the shaped tree: the base has no parent link, so a walk
// over Parent stays on records.
//
// Field order packs the small fields (Depth, Watch) together, which absorbs the padding ShapeState
// adds by keeping its one-byte Flags apart from the base.
struct SourceRecord : ShapedNode
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

    // ---- Source tree (written only by SourceTable) ------------------------------------------
    //
    // The tree the source describes, built by ParentBy / ChildrenBy; flat sources leave every record
    // top-level with no children. Never written by Filter, Sort or GroupBy: group membership, group
    // level and expansion live in the shaping layer; filtered/sorted children, filter flags, cached
    // keys and visible counts live in Shape (below).
    // Non-owning; ownership stays with the item's collection (m_records or a ChildCollectionsHierarchy child feed).

    // Source parent; nullptr for a top-level record.
    SourceRecord* Parent{ nullptr };

    // Source children in source order (ParentBy: root-collection order among siblings;
    // ChildrenBy: the child collection's order).
    std::vector<SourceRecord*> Children;

    // Top level = 0. Updated for the whole subtree on reparent.
    uint16_t Depth{ 0 };

    bool HasChildren() const noexcept { return !Children.empty(); }

    // ---- Live-shaping subscription -----------------------------------------------------------
    //
    // Watch follows Depth so the two share one 8-byte slot.
    WatchState Watch{ WatchState::Unchecked };

    // Empty token when not subscribed. Revoked in ~SourceRecord().
    winrt::event_token PropertyChangedToken{};

    // ---- Engine state (written only by the engine and its ShapedTree) ------------------------
    //
    // The per-node counts and position (VisibleDescendantCount, IndexInParent, ChildRows) are on
    // the ShapedNode base. The record-only rest is stored inline (Engine/ShapeState.h): filter
    // flags, cached sort keys, filtered and sorted children, the group link. Tree links stay in
    // Parent/Children above. Cached keys are tracker_refs, so ~SourceRecord destroys them with
    // Item, under the same teardown rules.
    ShapeState Shape;

    // Logical identity, formatted on first use and then cached. The only way to read it.
    // Format (fixed, D4): "0x" + Address in hex. Also the record's row key in ShapedCollectionView.
    winrt::hstring const& Identity() const;

    // Inverse of Identity(): the Address a record identity encodes, or nullopt when identity is not
    // in that format (a "g|" group key, or garbage). Lets IndexFromKey resolve a record through
    // SourceTable::TryFindByAddress, which is always on, instead of the identity index (D4).
    static std::optional<ObjectAddress> TryParseIdentity(winrt::hstring const& identity) noexcept;

private:
    mutable winrt::hstring m_identity;
};

using SourceRecordPtr = std::shared_ptr<SourceRecord>;
using SourceRecordWeakPtr = std::weak_ptr<SourceRecord>;

// Change kind reuses the platform's collection-change action (Add, Remove, Replace, Move, Reset).
using SourceChangeKind = winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedAction;

// One normalized, record-level change to the source. Raised after the records, the tree links
// and the lookups are updated and before the removed records are destroyed. Positions are within
// the parent's children (TopLevel() when the parent is null).
struct SourceChange
{
    SourceChangeKind Kind{ SourceChangeKind::Reset };

    // Parent before the change; nullptr = top level. Remove, Replace, Move.
    SourceRecord* OldParent{ nullptr };

    // Parent after the change; nullptr = top level. Add, Replace, Move. A Move with
    // OldParent != NewParent is a reparent (ParentBy key change).
    SourceRecord* NewParent{ nullptr };

    // Position among OldParent's children before the change. Remove, Replace, Move.
    uint32_t OldIndex{ 0 };

    // Position among NewParent's children after the change. Add, Replace, Move.
    uint32_t NewIndex{ 0 };

    // Add and Replace: the new subtree in pre-order (Added[0] is its root; a flat source carries
    // one record). Reset: every record created by it (a record reused by Refresh() is not here).
    std::vector<SourceRecord*> const& Added;

    // Remove and Replace: the departed subtree in pre-order, alive (Item, Identity()) until the
    // handler returns. Reset: every record that left (root and child; reused ones did not).
    std::vector<SourceRecord const*> const& Removed;
};
