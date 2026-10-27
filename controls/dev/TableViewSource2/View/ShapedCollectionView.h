// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>

#include "ItemsSourceView.h"

class ShapingManager;

// The visible list TableView binds to (declarations only). See
// docs/design-notes/TabularControls/Shaping-Architecture-Redesign.md, "WinRT visible list".
//
// A virtual, read-only collection over the flattener's m_rows (V1): it holds no copy. Derives from
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
// manager, which owns the flattener, the engine, the SourceTable and the mid-publish checks (V4).
//
// Publishing (V4): ShapingManager::Publish applies one splice to m_rows, then calls NotifyRemoved /
// NotifyAdded for it, so Count and GetAt match every event; a bulk delta calls NotifyReset once.
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

    // m_rows[index] mapped to its row value: the app item, or the group's GroupHeaderRow (created
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

    // ---- Publishing (called only by ShapingManager::Publish) -------------------------------

    // A ranged Remove of count rows at index. m_rows must already reflect it.
    void NotifyRemoved(uint32_t index, uint32_t count);

    // A ranged Add of count rows at index. m_rows must already reflect it.
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

// The item lists of ShapedCollectionView's Remove and Add args (decided, D3). Holds a count only:
// Size() is the count, GetAt returns nullptr, mutators throw. The repeater reads only
// NewItems().Size() / OldItems().Size(), so a 10k-row splice costs one tiny object, not a 10k-entry
// vector (InspectingDataSource appends one nullptr per item).
class CountOnlyItemList :
    public winrt::implements<CountOnlyItemList, winrt::IBindableVector, winrt::IBindableIterable>
{
public:
    explicit CountOnlyItemList(uint32_t count) noexcept : m_count(count) {}

    // IBindableVector (read side).
    uint32_t Size() const noexcept { return m_count; }
    winrt::IInspectable GetAt(uint32_t index) const;     // nullptr; E_BOUNDS when index >= Size()
    winrt::IBindableVectorView GetView() const;
    bool IndexOf(winrt::IInspectable const& value, uint32_t& index) const noexcept;  // false

    // IBindableVector (mutators): throw E_ILLEGAL_METHOD_CALL.
    void SetAt(uint32_t index, winrt::IInspectable const& value);
    void InsertAt(uint32_t index, winrt::IInspectable const& value);
    void RemoveAt(uint32_t index);
    void Append(winrt::IInspectable const& value);
    void RemoveAtEnd();
    void Clear();

    // IBindableIterable: Size() nullptrs.
    winrt::IBindableIterator First() const;

private:
    uint32_t m_count{ 0 };
};
