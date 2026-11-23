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

    // Source order. Keeps keyed rows alive so no KeyByItem address can be recycled.
    std::vector<winrt::IInspectable> Rows;
    // Object (identity) keys, held for the structure's lifetime. Node keys persist as expansion
    // intent and selection anchors until the next structure prunes them; holding the objects
    // means no new key can take a still-referenced address in the meantime.
    std::vector<winrt::IInspectable> KeyObjects;
    // ABI pointer -> nodeKey, for every row.
    std::unordered_map<void*, std::wstring> KeyByItem;
    // nodeKey -> position in Rows. Keys view KeyByItem's values (node-based, so they stay put).
    std::unordered_map<std::wstring_view, size_t> IndexByKey;
    // Per row: key (into KeyByItem), parent position or Root, children in source order.
    std::vector<std::wstring const*> NodeKeys;
    std::vector<size_t> ParentIndex;
    std::vector<std::vector<size_t>> ChildIndices;
    std::vector<size_t> RootIndices;

    // Position-free maps an incremental update (TableViewRowFlattener::TryApplyTreeChange) keeps current, with
    // Rows and KeyByItem. NodeKeys, ParentIndex, ChildIndices, RootIndices and IndexByKey describe the build only.
    // nodeKey -> row; keys view KeyByItem's values.
    std::unordered_map<std::wstring_view, winrt::IInspectable> ItemByKey;
    // ABI pointer -> the parent key the row declared (empty for none), resolved or not.
    std::unordered_map<void*, std::wstring> ParentKeyByItem;
    // Parent keys rows declared that no row has (orphans, shown as roots) -> how many rows declared each.
    std::unordered_map<std::wstring, uint32_t> OrphanParentKeys;

    // The parent key a row resolves to: its declared parent's key if a row has it, else empty (a root).
    std::wstring_view ResolvedParentKey(winrt::IInspectable const& item) const;
};

// Parent-key index: one filtered, sorted reading of a ParentStructure. Immutable once built.
struct ParentKeyIndex
{
    // Items are in sorted order within every sibling list.
    std::vector<winrt::IInspectable> Roots;
    // parent nodeKey -> children. Keys view strings owned by Structure.
    std::unordered_map<std::wstring_view, std::vector<winrt::IInspectable>> Children;
    std::unordered_set<std::wstring> ContextKeys; // kept only because a descendant matched the filter
    // Nodes the filter keeps: the matches and the ancestors that give them context.
    size_t KeptNodeCount{};
    // Owns the keys all lookups resolve against; its key set is complete and unfiltered.
    std::shared_ptr<const ParentStructure> Structure;

    const std::vector<winrt::IInspectable>* TryGetChildren(std::wstring_view nodeKey) const;
    std::wstring const* TryGetKey(winrt::IInspectable const& item) const;
};

using ParentKeyFilter = std::function<bool(winrt::IInspectable const&)>;
// Sorts one sibling list in place, stably. Called only for lists of two or more rows.
using SiblingSorter = std::function<void(std::vector<winrt::IInspectable>&)>;

// A row's node key and parent node key as the selectors returned them, so a later build can skip them.
struct CachedNodeKeys
{
    std::wstring Key;
    std::wstring ParentKey;
};
// By the row's ABI pointer. The caller drops the entry of a row whose keys may have changed. Rows with object
// keys are never cached.
using NodeKeyCache = std::unordered_map<void*, CachedNodeKeys>;

// Runs key/parent selectors once per row, or reads a row's keys from `cache`. False with `error` on
// duplicate/null key, self-parent or cycle. On success `cache` (when given) holds exactly the rows' keys.
bool BuildParentStructure(
    std::vector<winrt::IInspectable> const& rows,
    KeySelector const& keySelector,
    KeySelector const& parentKeySelector,
    ParentStructure& out,
    winrt::hstring& error,
    NodeKeyCache* cache = nullptr);

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

// A row's node key and parent node key through the selectors (a selector that throws reads as no key).
void ReadNodeKeys(
    KeySelector const& keySelector,
    KeySelector const& parentKeySelector,
    winrt::IInspectable const& item,
    std::wstring& nodeKey,
    std::wstring& parentKey);

// True when MakeNodeKey keyed by object address (address may be recycled, so unprovable).
bool IsObjectNodeKey(std::wstring_view nodeKey) noexcept;

}
