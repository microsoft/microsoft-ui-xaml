// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>

#include <winrt/Microsoft.UI.Xaml.Interop.h>

class ShapedRowQueries;

// The visible list TableView binds to. See
// docs/design-notes/TabularControls/Shaping-Architecture-Redesign.md, "WinRT visible list".
//
// A virtual, read-only collection over ShapedTree's walk (V1, P1, P10): it holds no copy. A native
// C++/WinRT object implementing ItemsSourceView's default interface (IItemsSourceView) and
// INotifyCollectionChanged (decided, M16; replaces D1). It does not derive from ItemsSourceView:
// TableViewSource2 compiles into the Tabular DLL, while ItemsSourceView's implementation type
// (Repeater/ItemsSourceView.h) lives in the Controls DLL, and the runtime class exposes no
// overridable interface. Since every interface the ItemsSourceView projected class needs is
// implemented here, ShapingManager::View() returns this object as winrt::ItemsSourceView (a
// QueryInterface), with no InspectingDataSource wrapper in between. No code may get_self it as
// the ItemsSourceView implementation type.
//
// One object per ShapingManager, created on the first View() and kept for the coordinator's
// lifetime: reshapes raise CollectionChanged on it, never replace it.
//
// Row value (V2): a data row is the app item (SourceRecord::Item); a group header is its
// GroupedEntry (shared with v1, so TableView's TryGetGroupedEntry recognizes it), created on first
// read and cached on GroupNode::HeaderRow. The entry holds no key: its Group() resolves the key
// on each call through a weak reference to this view (GroupKeyOf), so no native strong reference
// to an app object outlives the tracker_ref on GroupNode::Key.
// Row key (V3): a record's Identity() ("0x" + address); a group's GroupNode::Identity ("g|" ...).
//
// Back-reference (D2): a non-owning ShapedRowQueries* (ShapingManager::m_rows), nulled by Detach()
// from ShapingManager::Clear(). TableView may hold the view after its TableViewSource2 is torn
// down; once detached, Count is 0, GetAt and KeyFromIndex throw E_BOUNDS and every key or item
// lookup returns -1. Read-only by construction: the view sees the row queries only, never the
// coordinator's verbs.
//
// Events (P8): ShapedTree raises each change as it makes it; ShapingManager, its listener, calls
// NotifyRemoved or NotifyAdded right away, so Count and GetAt already match every event. A rebuild
// or expand-all calls NotifyReset once. Nothing is recorded or replayed.
//
// Event item lists (D3): in parity with InspectingDataSource::OnVectorChanged, Remove and Add args
// carry a bindable vector of count nullptrs (the unused side, and both on Reset, an empty one,
// never null). Only the size is meaningful; the repeater reads only NewItems().Size() /
// OldItems().Size(), and consumers read rows through GetAt.
//
// Holds no app object: CollectionChanged handlers are kept by winrt::event and dropped by Detach().
//
// UI-thread affine.
class ShapedCollectionView :
    public winrt::implements<
        ShapedCollectionView,
        winrt::Microsoft::UI::Xaml::Controls::IItemsSourceView,
        winrt::Microsoft::UI::Xaml::Interop::INotifyCollectionChanged>
{
public:
    explicit ShapedCollectionView(ShapedRowQueries const& rows) noexcept;

#pragma region IItemsSourceView

    // ShapedRowQueries::RowCount(); 0 once detached.
    int32_t Count();

    // ShapedTree::GetAt(index) (tree walk with cursor, P1/P3) mapped to its row value: the app
    // item, or the group's GroupedEntry (created on first read with Key, Identity and count /
    // expanded snapshots, then cached on GroupNode::HeaderRow). Throws E_BOUNDS out of range or
    // detached.
    winrt::Windows::Foundation::IInspectable GetAt(int32_t index);

    // Always true: the repeater pools elements by key, so a stable Reset reuses them.
    bool HasKeyIndexMapping();

    // The row's key: ShapedRowQueries::KeyAt (SourceRecord::Identity() or GroupNode::Identity).
    // Throws E_BOUNDS out of range or detached. Called on every realize and recycle; no allocation
    // after the first call per row.
    winrt::hstring KeyFromIndex(int32_t index);

    // ShapedRowQueries::IndexOfKey (V3 dispatch on the "g|" marker; record keys are parsed back to
    // an address, D4). -1 when unknown, hidden or detached.
    int32_t IndexFromKey(winrt::hstring const& key);

    // ShapedRowQueries::IndexOfItem: the item's record through SourceTable::TryFindByItem, then its
    // flat index, O(depth); a GroupedEntry (TryGetGroupedEntry) resolves through its Identity. -1
    // when unknown, hidden or detached. Not a linear scan.
    int32_t IndexOf(winrt::Windows::Foundation::IInspectable const& value);

#pragma endregion

#pragma region INotifyCollectionChanged

    winrt::event_token CollectionChanged(
        winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventHandler const& handler);
    void CollectionChanged(winrt::event_token const& token) noexcept;

#pragma endregion

    // ---- Events (called only by ShapingManager's IShapedRowsListener) ----------------------

    // A ranged Remove of count rows at index; OldItems holds count nullptrs (D3). The tree already
    // reflects it (P8).
    void NotifyRemoved(uint32_t index, uint32_t count);

    // A ranged Add of count rows at index; NewItems holds count nullptrs (D3). The tree already
    // reflects it (P8).
    void NotifyAdded(uint32_t index, uint32_t count);

    // One stable Reset: the repeater re-reads Count and reuses elements by key.
    void NotifyReset();

    // ---- Group header key ------------------------------------------------------------------

    // The GroupBy key of the live group with this identity (GroupNode::Key, a tracker_ref), read
    // fresh on each call. Backs GroupedEntry::Group() for v2 headers. Null when the group is gone,
    // it has no key, or the view is detached.
    winrt::Windows::Foundation::IInspectable GroupKeyOf(winrt::hstring const& identity) const;

    // ---- Lifetime ---------------------------------------------------------------------------

    // Nulls m_rows and drops every CollectionChanged handler. Called first by
    // ShapingManager::Clear(), before anything the view reads is torn down. Raises no event: the
    // consumer is going away or rebinding. Idempotent.
    void Detach() noexcept;

private:
    void Raise(winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args);

    // Non-owning (D2). Null after Detach().
    ShapedRowQueries const* m_rows{ nullptr };

    winrt::event<winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventHandler> m_collectionChanged;
};
