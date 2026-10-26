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
// Validated tree over the UNFILTERED source, in source order. Independent of filter/sort, so a
// reshape can reuse it once no key or parent key moved. Immutable once built.
struct ParentStructure
{
    static constexpr size_t Root = SIZE_MAX;

    struct NodeIdentity
    {
        ObjectAddress Address{ ObjectAddress::Null };
        winrt::hstring Identity;
    };

    // Source order. Keeps addressed rows alive so their identities cannot be recycled.
    std::vector<winrt::IInspectable> Rows;
    // Object (identity) keys, held for the structure's lifetime so no new key can take an address
    // still referenced by NodeKeys/IndexByKey.
    std::vector<winrt::IInspectable> KeyObjects;
    // Index-aligned with Rows; independent of app keys used to resolve parent links.
    std::vector<NodeIdentity> RowIdentities;
    std::unordered_map<ObjectAddress, size_t> IndexByAddress;
    // App-key snapshots for parent resolution and structure-reuse checks, not row identity.
    std::vector<std::wstring> NodeKeys;
    // nodeKey -> position in Rows. Keys view NodeKeys, which is sized once before population.
    std::unordered_map<std::wstring_view, size_t> IndexByKey;
    // Per row: parent position or Root, children in source order.
    std::vector<size_t> ParentIndex;
    std::vector<std::vector<size_t>> ChildIndices;
    std::vector<size_t> RootIndices;
};

// Parent-key index: one filtered, sorted reading of a ParentStructure. Immutable once built.
struct ParentKeyIndex
{
    // Items are in sorted order within every sibling list.
    std::vector<winrt::IInspectable> Roots;
    // Parent object address -> children.
    std::unordered_map<ObjectAddress, std::vector<winrt::IInspectable>> Children;
    std::unordered_set<winrt::hstring> ContextIdentities; // kept only because a descendant matched the filter
    // Owns the complete, unfiltered node identities and app-key snapshots.
    std::shared_ptr<const ParentStructure> Structure;

    const std::vector<winrt::IInspectable>* TryGetChildren(ObjectAddress address) const;
    ParentStructure::NodeIdentity const* TryGetNodeForItem(winrt::IInspectable const& item) const;
    ParentStructure::NodeIdentity const* TryGetNodeForIdentity(winrt::hstring const& identity) const;
};

using ParentKeyFilter = std::function<bool(winrt::IInspectable const&)>;
// Sorts one sibling list in place, stably. Called only for lists of two or more rows.
using SiblingSorter = std::function<void(std::vector<winrt::IInspectable>&)>;

// Runs key/parent selectors once per row. False with `error` on duplicate/null key, self-parent
// or cycle.
bool BuildParentStructure(
    std::vector<winrt::IInspectable> const& rows,
    KeySelector const& keySelector,
    KeySelector const& parentKeySelector,
    ParentStructure& out,
    winrt::hstring& error);

// True when re-running the selectors over `structure.Rows` yields the same keys and parents.
// Object keys always fail: a freed key's address can be reused, so they cannot be proven unchanged.
bool ParentStructureStillMatches(
    ParentStructure const& structure,
    KeySelector const& keySelector,
    KeySelector const& parentKeySelector);

// Runs no selector. Empty `filter`/`sort` = none. Filter keeps matches plus ancestors; sort orders
// each sibling list (roots only if `sortRoots`), ties in source order.
void BuildParentKeyIndex(
    std::shared_ptr<const ParentStructure> const& structure,
    ParentKeyFilter const& filter,
    SiblingSorter const& sort,
    bool sortRoots,
    ParentKeyIndex& out);

// Exposed for the adapter: "node:" + lookup key, or empty when `key` means "no key".
std::wstring MakeNodeKey(winrt::IInspectable const& key);

// True when MakeNodeKey keyed by object address (address may be recycled, so unprovable).
bool IsObjectNodeKey(std::wstring_view nodeKey) noexcept;

}
