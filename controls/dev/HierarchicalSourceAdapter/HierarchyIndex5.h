// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "pch.h"
#include "common.h"

namespace ShapingHelpers {}
namespace cw5
{
namespace ShapingHelpers { using namespace ::ShapingHelpers; }


// The eagerly built, fully SHAPED structure of a children-selector hierarchy declared without a
// HasChildrenSelector (WithChildren(children) is eager; WithChildren(children, hasChildren) stays
// lazy and never has one).
//
// Built in full by the owner on a refresh that changes how sibling sets are sorted, or when it
// cannot be updated (see below), in one O(N) walk over the structure cache that also derives filter retention and shapes every
// sibling set. The adapter then emits from it without asking the selector, the cache or the shape
// again, and without the walk's per-row cycle and duplicate checks: the build has already proved
// there are none.
//
// One node per OBJECT: an item reachable under two parents (a DAG) is one node referenced twice;
// its path keys still differ per parent, because those are minted by the emit walk.
//
// Validity is against the structure cache it was built from: any change the cache sees (a
// collection change, a re-enumeration that moved a list, a shape invalidation, a live item change,
// a reset) moves the cache's version, after which the adapter ignores the index and walks the slow
// way until the owner brings it current again. The owner does that incrementally
// from the cache's change log (HierarchyStructureCache::TakeChangeLog): it re-reads and re-shapes
// only the nodes whose lists moved, indexes any subtree it has not seen, forgets released nodes, and
// re-stamps CacheVersion -- mutating this object in place, which is why it is only ever done while
// no reader holds it beyond the owner's and the adapter's references (readers pin it for the
// duration of a walk). Under a filter it also keeps retention current the same way: it re-reads
// the filter verdict of only the nodes that may have changed (every node when the filter itself
// changed), re-derives which nodes are kept in one flat pass over the unshaped edges below when a
// verdict or an edge moved, and re-shapes only the sibling sets whose kept membership moved.
// Anything the log cannot describe or too many dead slots means a full rebuild. A node is
// additionally emitted the slow way when it is not Trusted (see below), which keeps every
// diagnostic the walk raises exactly where it was. An index is never current over a structure with
// a cycle or a chain deeper than the walk supports.
struct HierarchyIndex
{
    static constexpr uint32_t npos = ~uint32_t{ 0 };

    enum class State : uint8_t
    {
        Out,
        Match,
        // Kept by the filter only for a matching descendant.
        Context,
    };

    struct Node
    {
        winrt::IInspectable Item{ nullptr };
        winrt::hstring Identity;
        // The node's SHAPED (filtered + sorted) children, as indices into Nodes, nulls skipped.
        std::vector<uint32_t> Children;
        // The node's UNSHAPED children as the cache last reported them, as indices into Nodes,
        // sorted and without duplicates; and every node whose unshaped children include this one
        // (more than one in a DAG). Kept for every node, shown or not, so retention and the
        // cycle and depth checks never have to walk the cache.
        std::vector<uint32_t> Raw;
        std::vector<uint32_t> Parents;
        // Levels from this node to its deepest descendant, itself included. Exact after a build;
        // never below the truth after an update (a removed edge does not lower it).
        int32_t Height{ 1 };
        // Size of the shaped sibling set, nulls included -- what the walk reports as ChildCount.
        int32_t ChildCount{ 0 };
        // Out, Match or Context under a filter (Out also when unreachable); Match for every live
        // node without one.
        State Retention{ State::Match };
        // The filter's last verdict for the item. Meaningful only while Filtered.
        bool Passes{ true };
        // Retention == Context, as shown: only for a node that was shaped.
        bool IsContext{ false };
        // Children may be emitted from this node. False when its collection cannot notify (it must
        // be re-asked once per pass, as the walk does), when its shaped set holds the same object
        // twice (the walk rejects that, once visible), or when it was never shaped (filtered out).
        bool Trusted{ false };
        // Released by the cache: no longer in ByItem, linked to nothing, never shown.
        bool Dead{ false };
    };

    std::vector<Node> Nodes;
    // Item ABI pointer -> index into Nodes. Every key is pinned by its node's Item.
    std::unordered_map<void*, uint32_t> ByItem;
    // The root set (unfiltered) the index was last made current against.
    std::vector<uint32_t> Roots;
    // Slots no longer in ByItem: nodes the cache released since the build. Still pinned (their
    // Item keeps the key from being recycled) but unreachable; a full rebuild compacts them.
    uint32_t DeadSlots{ 0 };
    // Built with filter retention: Retention, IsContext and the shaped sets depend on the kept set.
    bool Filtered{ false };
    // The owner's filter generation the verdicts were read under.
    uint64_t FilterGeneration{ 0 };

    // The structure cache this was built from, and that cache's version at the end of the build.
    void const* Cache{ nullptr };
    uint64_t CacheVersion{ 0 };

    Node const* Find(winrt::IInspectable const& item) const
    {
        if (!item)
        {
            return nullptr;
        }
        const auto found = ByItem.find(winrt::get_abi(item));
        return found == ByItem.end() ? nullptr : &Nodes[found->second];
    }
};

using HierarchyIndexPtr = std::shared_ptr<const HierarchyIndex>;

} // namespace cw5
