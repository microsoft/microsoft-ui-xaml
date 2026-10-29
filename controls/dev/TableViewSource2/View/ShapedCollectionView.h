// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>

#include "ItemsSourceView.h"

class ShapingManager;

// The visible list TableView binds to (declarations only). See
// docs/design-notes/TabularControls/Shaping-Architecture-Redesign.md, "WinRT visible list".
//
// A virtual, read-only collection over ShapedTree's walk (V1, P1, P10): it holds no copy. Derives from
// ItemsSourceView inside the controls DLL, as InspectingDataSource does (D1), and
// ShapingManager::View() returns it as is; ItemsRepeater uses an ItemsSourceView it is given
// without wrapping it. One object per ShapingManager, created on the first View() and kept for the
// coordinator's lifetime: reshapes raise CollectionChanged on it, never replace it.
//
// Row value (V2): a data row is the app item (SourceRecord::Item); a group header is its
// GroupHeaderRow token, created on first read and cached on GroupNode::HeaderRow.
// Row key (V3): a record's Identity() ("0x" + address); a group's GroupNode::Identity ("g|" ...).
//
// Back-reference (D2): a non-owning ShapingManager*, nulled by Detach() from ShapingManager::Clear().
// TableView may hold the view after its TableViewSource2 is torn down; once detached, Count is 0,
// GetAt throws E_BOUNDS and every key or item lookup returns -1. Every lookup goes through the
// manager, which owns the engine (and its ShapedTree) and the SourceTable.
//
// Events (P8): ShapedTree raises each change as it makes it; ShapingManager, its listener, calls
// NotifyRemoved or NotifyAdded right away, so Count and GetAt already match every event. A rebuild
// or expand-all calls NotifyReset once. Nothing is recorded or replayed.
//
// Event item lists (D3): in parity with InspectingDataSource::OnVectorChanged, Remove and Add args
// carry a bindable Vector of count nullptrs. Only the size is meaningful; the repeater reads only
// NewItems().Size() / OldItems().Size(), and consumers read rows through GetAt.
//
// UI-thread affine.
class ShapedCollectionView :
    public winrt::implements<ShapedCollectionView, ItemsSourceView>
{
public:
    ForwardRefToBaseReferenceTracker(ItemsSourceView)

    explicit ShapedCollectionView(ShapingManager& owner);

#pragma region ItemsSourceViewOverrides

    // ShapingManager::RowCount(); 0 once detached.
    int32_t GetSizeCore() override;

    // ShapedTree::GetAt(index) (tree walk with cursor, P1/P3) mapped to its row value: the app item, or the group's GroupHeaderRow (created
    // and cached on GroupNode::HeaderRow on first read). Throws E_BOUNDS out of range or detached.
    winrt::IInspectable GetAtCore(int index) override;

    // Always true: the repeater pools elements by key, so a stable Reset reuses them.
    bool HasKeyIndexMappingCore() override;

    // The row's key: SourceRecord::Identity() or GroupNode::Identity. Throws E_BOUNDS out of range
    // or detached. Called on every realize and recycle; no allocation after the first call per row.
    winrt::hstring KeyFromIndexCore(int index) override;

    // ShapingManager::IndexOfKey (V3 dispatch on the "g|" marker; record keys are parsed back to
    // an address, D4). -1 when unknown, hidden or detached.
    int IndexFromKeyCore(winrt::hstring const& key) override;

    // ShapingManager::IndexOfItem: the item's record through SourceTable::TryFindByItem, then its
    // flat index, O(depth); a GroupHeaderRow resolves through its Identity. -1 when unknown, hidden
    // or detached. Not a linear scan.
    int IndexOfCore(winrt::IInspectable const& value) override;

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

    // ---- Lifetime ---------------------------------------------------------------------------

    // Nulls m_owner. Called first by ShapingManager::Clear(), before anything the view reads is
    // torn down. Raises no event: the consumer is going away or rebinding. Idempotent.
    void Detach() noexcept;

private:
    // Non-owning (D2). Null after Detach().
    ShapingManager* m_owner{ nullptr };
};
