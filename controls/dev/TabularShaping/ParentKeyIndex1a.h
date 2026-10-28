// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ShapingDescriptions.h"
namespace ShapingHelpers {}
namespace pb1a
{
namespace ShapingHelpers { using namespace ::ShapingHelpers; }

namespace ShapingHelpers
{
// A node is its item's canonical IUnknown pointer. It is the row identity, the expansion-intent
// key and the key of every map below. An address is only meaningful while something holds the
// item; ParentNode::Item does for as long as the node is indexed, so no live node's address can be
// recycled.
using NodeId = void*;

// Null for a null item.
NodeId GetNodeId(winrt::IInspectable const& item);

// "node:0x<address>", the public row identity of a node. Parsing proves nothing about liveness:
// resolve the result against a structure before acting on it.
winrt::hstring FormatNodeIdentity(NodeId node);
NodeId TryParseNodeIdentity(winrt::hstring const& identity) noexcept;

struct ParentNode
{
    winrt::IInspectable Item{ nullptr };
    // Selector results, held so an object key's address cannot be recycled while it is indexed.
    winrt::IInspectable KeyObject{ nullptr };
    winrt::IInspectable ParentKeyObject{ nullptr };
    // Lookup forms (MakeNodeKey); used only to resolve parents. Key is never empty; ParentKey is
    // empty for a declared root.
    std::wstring Key;
    std::wstring ParentKey;
    // Resolved parent; null for a root (declared, or an orphan whose parent key is unknown).
    NodeId Parent{ nullptr };
    // Position in Rows: the tiebreak that keeps each sibling list in source order.
    size_t Rank{ 0 };
};

// One item's selector results. Computed before a structure is touched, because selectors are app
// code that may re-enter.
struct ResolvedNode
{
    winrt::IInspectable Item{ nullptr };
    winrt::IInspectable KeyObject{ nullptr };
    winrt::IInspectable ParentKeyObject{ nullptr };
    std::wstring Key;
    std::wstring ParentKey;
};

// Validated tree over the UNFILTERED source. Built once per relation, then patched in place by its
// owner on the UI thread -- never while a refresh is reading it -- so a source change costs
// selector calls for the changed items only. A patch that would make it invalid (duplicate or
// empty key, self-parent, cycle, rows that do not line up) returns false and leaves it unusable;
// the owner then discards it and rebuilds, which reports the error.
struct ParentStructure
{
    // Source order. Holds every item, so no NodeId below can be recycled.
    std::vector<winrt::IInspectable> Rows;
    std::unordered_map<NodeId, ParentNode> Nodes;
    std::unordered_map<std::wstring, NodeId> NodeByKey;
    // Declared parent key -> the nodes waiting for it (roots meanwhile), so an arriving parent
    // adopts them without a scan.
    std::unordered_map<std::wstring, std::vector<NodeId>> Orphans;
    // Unshaped sibling lists, in Rank order. Roots include orphans.
    std::unordered_map<NodeId, std::vector<NodeId>> Children;
    std::vector<NodeId> Roots;

    // Unique per build, so a shape cache can tell a rebuilt structure from the one it describes.
    uint64_t Epoch{ 0 };

    // Change log since the shape cache last consumed it (see BuildParentKeyIndex). A null entry
    // in DirtyLists names the roots.
    std::unordered_set<NodeId> DirtyLists;
    // Nodes whose filter verdict must be re-read.
    std::unordered_set<NodeId> DirtyNodes;
    // Removed items, held until consumed so their addresses cannot be reused meanwhile.
    std::vector<winrt::IInspectable> RemovedItems;
    // An edge moved, so filter context (ancestors of matches) must be recomputed.
    bool EdgesChanged{ false };

    ParentNode const* TryGetNode(NodeId node) const;
    std::vector<NodeId> const* TryGetChildren(NodeId parent) const;
};

// Parent-key index: one filtered, sorted reading of a ParentStructure, published to the adapter.
// Immutable once built; sibling lists are shared with the shape cache, not copied.
struct ParentKeyIndex
{
    using SiblingList = std::vector<winrt::IInspectable>;
    using ListMap = std::unordered_map<NodeId, std::shared_ptr<const SiblingList>>;

    // Items are in sorted order within every sibling list.
    std::vector<winrt::IInspectable> Roots;
    // Parent node -> its shaped children; absent when none survive the filter.
    std::shared_ptr<const ListMap> Children;
    // Nodes kept only because a descendant matched the filter.
    std::shared_ptr<const std::unordered_set<NodeId>> ContextNodes;
    // The unfiltered tree. UI-thread only; patched in place between publications, so it may run
    // ahead of this index until the next one.
    std::shared_ptr<const ParentStructure> Structure;

    SiblingList const* TryGetChildren(NodeId node) const;
    bool IsContext(NodeId node) const;
    // True when `node` is in the unfiltered tree.
    bool ContainsNode(NodeId node) const;
};

// The shaped (filtered and sorted) sibling lists of one structure, reused across reshapes. A list
// is rebuilt only when its membership changed, a member was marked dirty, or the sort/filter
// generation moved.
struct ParentShapeCache
{
    using ListMap = ParentKeyIndex::ListMap;

    // Epoch of the structure described; 0 = nothing cached.
    uint64_t Epoch{ 0 };
    uint64_t FilterGeneration{ 0 };
    uint64_t SortGeneration{ 0 };
    bool HasFilter{ false };
    bool HasSort{ false };
    bool RootsSorted{ false };

    // Per-node filter verdict and resulting state (0 = out, 1 = match, 2 = context). Empty with no
    // filter, where every node is a match.
    std::unordered_map<NodeId, bool> Passes;
    std::unordered_map<NodeId, uint8_t> State;
    std::shared_ptr<const std::unordered_set<NodeId>> ContextNodes;

    // Shared with the published index; copied on write.
    std::shared_ptr<ListMap> Lists;
    std::vector<winrt::IInspectable> Roots;

    void Clear() noexcept;
};

using ParentKeyFilter = std::function<bool(winrt::IInspectable const&)>;
// Sorts one sibling list in place, stably. Called only for lists of two or more rows.
using SiblingSorter = std::function<void(std::vector<winrt::IInspectable>&)>;

// Runs both selectors for `item`; a throwing selector counts as null.
ResolvedNode ResolveParentNode(
    winrt::IInspectable const& item,
    KeySelector const& keySelector,
    KeySelector const& parentKeySelector);

// Runs key/parent selectors once per row. False with `error` on duplicate/null key, self-parent
// or cycle.
bool BuildParentStructure(
    std::vector<winrt::IInspectable> const& rows,
    KeySelector const& keySelector,
    KeySelector const& parentKeySelector,
    ParentStructure& out,
    winrt::hstring& error);

// In-place patches, mirroring a source change. Run no selector. False = the structure is unusable.
// Inserts `nodes` at Rows[index]; links them and lets each adopt the orphans waiting for its key.
bool InsertParentNodes(ParentStructure& structure, size_t index, std::vector<ResolvedNode> const& nodes);
// Removes `count` rows at Rows[index]; their children become orphan roots awaiting their key.
// `expected`, when non-empty, must match the rows removed.
bool RemoveParentNodes(ParentStructure& structure, size_t index, size_t count, std::vector<winrt::IInspectable> const& expected);
// Moves `count` rows from Rows[oldIndex] to Rows[newIndex] (index after removal).
bool MoveParentNodes(ParentStructure& structure, size_t oldIndex, size_t newIndex, size_t count);
// Re-reads an existing node's key and parent key, re-linking it when either moved. Also marks it
// dirty (its sort/filter inputs may have changed). An item that is not a node is ignored.
bool UpdateParentNode(ParentStructure& structure, ResolvedNode const& node);

// Builds the index from `cache`, rebuilding only what the structure's change log and the
// generations invalidate, then consumes the log. Empty `filter`/`sort` = none. Filter keeps
// matches plus ancestors; sort orders each sibling list (roots only if `sortRoots`), ties in source
// order. A throw (from `sort`) leaves the cache empty, so the next build starts over.
void BuildParentKeyIndex(
    std::shared_ptr<ParentStructure> const& structure,
    ParentShapeCache& cache,
    ParentKeyFilter const& filter,
    uint64_t filterGeneration,
    SiblingSorter const& sort,
    uint64_t sortGeneration,
    bool sortRoots,
    ParentKeyIndex& out);

// "node:" + lookup key, or empty when `key` means "no key". Resolves parent keys only.
std::wstring MakeNodeKey(winrt::IInspectable const& key);

}

} // namespace pb1a
