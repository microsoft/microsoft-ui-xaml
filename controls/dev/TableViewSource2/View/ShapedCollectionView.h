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
// C++/WinRT object implementing IBindableVector (+ IBindableVectorView, IBindableIterable),
// INotifyCollectionChanged and IKeyIndexMapping (D1). It does not derive from ItemsSourceView:
// TableViewSource2 compiles into the Tabular DLL, while ItemsSourceView's implementation type
// (Repeater/ItemsSourceView.h) lives in the Controls DLL and its runtime class exposes no
// overridable interface, so its *Core virtuals cannot be overridden across the DLL boundary.
// Instead TableViewSource2::GetItemsSourceView() wraps this object once with the public
// winrt::ItemsSourceView{ collection } constructor (as v1 GroupedSourceAdapter wraps its entries)
// and caches the wrapper. The wrapper's InspectingDataSource takes the IBindableVector path (this
// object is not an IVector<IInspectable>), observes INotifyCollectionChanged (it prefers that over
// VectorChanged) and picks up IKeyIndexMapping.
//
// One object per ShapingManager, created on the first View() and kept for the coordinator's
// lifetime: reshapes raise CollectionChanged on it, never replace it.
//
// Row value (V2): a data row is the app item (SourceRecord::Item); a group header is its
// GroupedEntry (shared with v1, so TableView's TryGetGroupedEntry recognizes it), created on first
// read and cached on GroupNode::HeaderRow (Milestone 4).
// Row key (V3): ShapedRowQueries::KeyAt, the one identity helper (ShapedRowMetadata uses it too):
// a record's Identity() ("0x" + address); a group's GroupNode::Identity ("g|" ...).
//
// Back-reference (D2): a non-owning ShapedRowQueries* (ShapingManager::m_rows), nulled by Detach()
// from ShapingManager::Clear(). TableView may hold the view after its TableViewSource2 is torn
// down; once detached, Size is 0, GetAt throws E_BOUNDS and every key or item lookup returns -1 /
// false. Read-only by construction: every mutator throws E_ILLEGAL_METHOD_CALL, and the view sees
// the row queries only, never the coordinator's verbs.
//
// Events (P8): ShapedTree raises each change as it makes it; ShapingManager, its listener, calls
// NotifyRemoved or NotifyAdded right away, so Size and GetAt already match every event. A rebuild
// or expand-all calls NotifyReset once. Nothing is recorded or replayed.
//
// Event item lists (D3): in parity with InspectingDataSource::OnVectorChanged, Remove and Add args
// carry an IBindableVector of count nullptrs (the unused side, and both on Reset, an empty one,
// never null). Only the size is meaningful; the repeater reads only
// NewItems().Size() / OldItems().Size(), and consumers read rows through GetAt.
//
// Holds no app object: CollectionChanged handlers are kept by winrt::event (the ItemsSourceView
// wrapper subscribes with a raw-this delegate and unsubscribes in its destructor).
//
// UI-thread affine.
class ShapedCollectionView :
    public winrt::implements<
        ShapedCollectionView,
        winrt::Microsoft::UI::Xaml::Interop::IBindableVector,
        winrt::Microsoft::UI::Xaml::Interop::IBindableVectorView,
        winrt::Microsoft::UI::Xaml::Interop::IBindableIterable,
        winrt::Microsoft::UI::Xaml::Interop::INotifyCollectionChanged,
        winrt::Microsoft::UI::Xaml::Controls::IKeyIndexMapping>
{
public:
    explicit ShapedCollectionView(ShapedRowQueries const& rows) noexcept;

#pragma region IBindableVector / IBindableVectorView

    // ShapedRowQueries::RowCount(); 0 once detached.
    uint32_t Size();

    // ShapedTree::GetAt(index) (tree walk with cursor, P1/P3) mapped to its row value: the app
    // item, or the group's GroupedEntry (Milestone 4: throws E_NOTIMPL on a group until then).
    // Throws E_BOUNDS out of range or detached.
    winrt::Windows::Foundation::IInspectable GetAt(uint32_t index);

    // ShapedRowQueries::IndexOfItem: the item's record through SourceTable::TryFindByItem, then its
    // flat index, O(depth); a GroupedEntry resolves through its Identity (Milestone 4). False when
    // unknown, hidden or detached. Not a linear scan.
    bool IndexOf(winrt::Windows::Foundation::IInspectable const& value, uint32_t& index);

    // This object, as IBindableVectorView (read-only already, same tree).
    winrt::Microsoft::UI::Xaml::Interop::IBindableVectorView GetView();

    // Read-only: each throws E_ILLEGAL_METHOD_CALL.
    void SetAt(uint32_t index, winrt::Windows::Foundation::IInspectable const& value);
    void InsertAt(uint32_t index, winrt::Windows::Foundation::IInspectable const& value);
    void RemoveAt(uint32_t index);
    void Append(winrt::Windows::Foundation::IInspectable const& value);
    void RemoveAtEnd();
    void Clear();

#pragma endregion

#pragma region IBindableIterable

    // An index iterator over this object (Size / GetAt per step).
    winrt::Microsoft::UI::Xaml::Interop::IBindableIterator First();

#pragma endregion

#pragma region INotifyCollectionChanged

    winrt::event_token CollectionChanged(
        winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventHandler const& handler);
    void CollectionChanged(winrt::event_token const& token);

#pragma endregion

#pragma region IKeyIndexMapping

    // ShapedRowQueries::KeyAt (V3). Throws E_BOUNDS out of range or detached. Called on every
    // realize and recycle.
    winrt::hstring KeyFromIndex(int32_t index);

    // ShapedRowQueries::IndexOfKey (V3 dispatch on the "g|" marker; record keys are parsed back to
    // an address, D4). -1 when unknown, hidden or detached.
    int32_t IndexFromKey(winrt::hstring const& key);

#pragma endregion

    // ---- Events (called only by ShapingManager's IShapedRowsListener) ----------------------

    // A ranged Remove of count rows at index; OldItems holds count nullptrs (D3). The tree already
    // reflects it (P8).
    void NotifyRemoved(uint32_t index, uint32_t count);

    // A ranged Add of count rows at index; NewItems holds count nullptrs (D3). The tree already
    // reflects it (P8).
    void NotifyAdded(uint32_t index, uint32_t count);

    // One stable Reset: the repeater re-reads Size and reuses elements by key.
    void NotifyReset();

    // ---- Lifetime ---------------------------------------------------------------------------

    // Nulls m_rows. Called first by ShapingManager::Clear(), before anything the view reads is
    // torn down. Raises no event: the consumer is going away or rebinding. Idempotent.
    void Detach() noexcept;

private:
    void Raise(winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args);

    // Non-owning (D2). Null after Detach().
    ShapedRowQueries const* m_rows{ nullptr };

    winrt::event<winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventHandler> m_collectionChanged;
};
