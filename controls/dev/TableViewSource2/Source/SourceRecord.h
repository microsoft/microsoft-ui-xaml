// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include <winrt/Microsoft.UI.Xaml.Interop.h>
#include <winrt/Windows.Foundation.h>

#include "tracker_ref.h"

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

    // ---- Source tree (written only by SourceTable) ------------------------------------------
    //
    // The tree the source describes, built by ParentBy / ChildrenBy; flat sources leave every record
    // top-level with no children. Never written by Filter, Sort or GroupBy: group membership, group
    // level and expansion live in the shaping layer; filtered/sorted children, filter flags, cached
    // keys and visible counts live in the engine's ShapeState slot (below, not declared yet).
    // Non-owning; ownership stays with the item's collection (m_records or a ChildFeed).

    // Source parent; nullptr for a top-level record.
    SourceRecord* Parent{ nullptr };

    // Source children in source order (ParentBy: root-collection order among siblings;
    // ChildrenBy: the child collection's order).
    std::vector<SourceRecord*> Children;

    // Top level = 0. Updated for the whole subtree on reparent.
    uint16_t Depth{ 0 };

    bool HasChildren() const noexcept { return !Children.empty(); }

    // ---- Engine state (decided, not declared yet) -------------------------------------------
    //
    // One engine-owned ShapeState slot per record (type defined in Engine/, opaque here): filter
    // pass and has-passing-descendant flags, cached sort/group keys, filtered and sorted children,
    // VisibleDescendantCount. Tree links come from Parent/Children above. Written only by the
    // engine. OPEN: inline storage or a lazily allocated unique_ptr.

    // Logical identity, formatted on first use and then cached. The only way to read it.
    winrt::hstring const& Identity() const;

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
    // one record). Reset: every record now in the table.
    std::vector<SourceRecord*> const& Added;

    // Remove and Replace: the departed subtree in pre-order, alive (Item, Identity()) until the
    // handler returns. Reset: every record that left.
    std::vector<SourceRecord const*> const& Removed;
};
