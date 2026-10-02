// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include <functional>
#include <vector>

#include <winrt/Windows.Foundation.h>

// How an INTRINSIC hierarchy is READ, expressed once.
//
// Layer 3 owns this for the same reason layer 3 owns GroupContract.h: layer 3 is the layer that
// CONSUMES the contract. Layer 2 satisfies it without including the adapter header, which keeps
// the two siblings rather than a stack.
//
// The contrast with grouping is the whole reason this file is a set of callbacks rather than a set
// of object probes. A group is DERIVED: layer 2 bucketizes a flat list and hands layer 3 finished
// ShapedGroup objects, so the contract is "how do I read this group object". A hierarchy is
// INTRINSIC: the app's parent object already owns its children, and nobody can know the shape of
// the tree without asking the app, one node at a time, as the walk reaches it. So the contract is
// "how do I ask", and the answers are functions.
namespace ShapingHelpers
{
    // An item's children. Returns a collection, or null/empty meaning "leaf".
    //
    // Called ONLY for nodes the walk has decided to descend into (see the adapter's Rebuild), which
    // is what makes a collapsed subtree free. It runs app code, so it may throw; the adapter treats
    // a throw as "no children" rather than failing the whole rebuild, matching how RebuildGrouped
    // guards m_groupSelector.
    using ChildrenFn = std::function<winrt::IInspectable(winrt::IInspectable const& item)>;

    // "Is this item expandable", answered WITHOUT enumerating children.
    //
    // Optional, and the only reason a lazy tree is possible: without it the adapter has to call
    // ChildrenFn on every visible row just to decide whether to draw a chevron, which realizes one
    // level below the visible set. Supplying it is what makes a collapsed 1M-node tree cost
    // O(roots) rather than O(roots + their children).
    using HasChildrenFn = std::function<bool(winrt::IInspectable const& item)>;

    // Shape ONE sibling set -- the children of a single node -- as the walk reaches it.
    //
    // Per-level rather than once up front, because filtering and sorting a hierarchy means
    // filtering and sorting each sibling set among its own peers; a comparison between a parent and
    // its own child is meaningless and must not be expressible. Same principle as the per-bucket
    // ApplySort(bucket.Items, GroupOrder, -1) in RebuildGrouped, and the same thing WPF does by
    // giving each level its own CollectionView.
    //
    // RE-ENTRANCY CONTRACT, and it is load-bearing: this runs INSIDE the adapter's Rebuild, unlike
    // anything in the grouped path (where layer 2 finishes all shaping before layer 3 is handed
    // anything). The implementation must not call back into any adapter mutator -- not Source(),
    // not SetNodeExpanded, not ExpandAll/CollapseAll. It may read and shape the vector it is given
    // and nothing else. The adapter's m_rebuildInFlight guard coalesces a re-entrant Rebuild into a
    // single follow-up pass rather than nesting, so a violation degrades to a redundant rebuild
    // rather than corruption -- but it is still a caller bug, and chk asserts it.
    using ShapeSiblingsFn = std::function<void(std::vector<winrt::IInspectable>& siblings)>;
}
