// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "ShapingDescriptions.h"

namespace ShapingHelpers
{
// The validated tree over the UNFILTERED source, in source order: every row's key and the
// position of its parent. It depends only on the source and the key/parent selectors, never on
// filter or sort, so a reshape reuses it instead of re-running the app's selectors over every row.
// Immutable once built.
struct ParentStructure
{
    static constexpr size_t Root = SIZE_MAX;

    // Source order. Holds every keyed row alive, so no KeyByItem entry can outlive its object and
    // alias a new one at the same address.
    std::vector<winrt::IInspectable> Rows;
    // ABI pointer -> nodeKey, for every row.
    std::unordered_map<void*, std::wstring> KeyByItem;
    // Per row (by position in Rows): its key (points into KeyByItem), parent position or Root, and
    // child positions in source order.
    std::vector<std::wstring const*> NodeKeys;
    std::vector<size_t> ParentIndex;
    std::vector<std::vector<size_t>> ChildIndices;
    std::vector<size_t> RootIndices;
};

// Parent-key index: one filtered and sorted reading of a ParentStructure. Pure and immutable once
// built.
struct ParentKeyIndex
{
    // Items are in sorted order within every sibling list.
    std::vector<winrt::IInspectable> Roots;
    // parent nodeKey -> children. Keys view strings owned by Structure.
    std::unordered_map<std::wstring_view, std::vector<winrt::IInspectable>> Children;
    std::unordered_set<std::wstring> ContextKeys; // kept only because a descendant matched the filter
    // Owns the keys every lookup above resolves against. Its key table also covers rows the filter
    // hid, which is harmless: lookups only ever ask about indexed rows. Its keys are the complete
    // unfiltered key set (expansion pruning).
    std::shared_ptr<const ParentStructure> Structure;

    const std::vector<winrt::IInspectable>* TryGetChildren(std::wstring_view nodeKey) const;
    std::wstring const* TryGetKey(winrt::IInspectable const& item) const;
};

using ParentKeyFilter = std::function<bool(winrt::IInspectable const&)>;
// Sorts one sibling list in place, stably. Called only for lists of two or more rows.
using SiblingSorter = std::function<void(std::vector<winrt::IInspectable>&)>;

// `rows` is the source, in source order. Runs the key and parent selectors once per row. Returns
// false and fills `error` on a duplicate / null key, self-parent or cycle.
bool BuildParentStructure(
    std::vector<winrt::IInspectable> const& rows,
    KeySelector const& keySelector,
    KeySelector const& parentKeySelector,
    ParentStructure& out,
    winrt::hstring& error);

// Runs no key or parent selector. `filter` may be empty (no filter); when set, only matches and
// their ancestors are indexed. `sort` may be empty (source order); otherwise each sibling list is
// sorted among its own peers -- the roots too, unless `sortRoots` is false (the grouped path orders
// those itself). Ties keep source order.
void BuildParentKeyIndex(
    std::shared_ptr<const ParentStructure> const& structure,
    ParentKeyFilter const& filter,
    SiblingSorter const& sort,
    bool sortRoots,
    ParentKeyIndex& out);

// Exposed for the adapter: "node:" + lookup key, or empty when `key` means "no key".
std::wstring MakeNodeKey(winrt::IInspectable const& key);

}
