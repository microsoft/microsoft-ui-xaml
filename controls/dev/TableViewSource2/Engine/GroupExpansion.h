// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <winrt/Windows.Foundation.h>

// Group expansion model (declarations only). See
// docs/design-notes/TabularControls/Shaping-Architecture-Redesign.md, "Expansion".
//
// GroupNodes are rebuilt on every GroupKey change, so their expansion is kept here, keyed by
// GroupNode::Identity. Records do not use this: they keep ShapeFlags::Expanded on their ShapeState.
//
// Stores only exceptions to a baseline, so ExpandAllGroups / CollapseAllGroups are O(1) and a group that does
// not exist yet resolves to the baseline. Same scheme as v1 ShapingHelpers::RowExpansionModel, but
// a v2 type: no change handler (ShapedTree toggles state itself and raises the events), and a
// single-key Erase for the incremental prune.
//
// Owned, read and written only by ShapedTree (m_groupExpansion). Holds no app object (keys are
// identity strings, not group key objects). UI-thread affine.
//
// Pruned so entries exist only for live groups (decided, F4): Erase on a surgical group drop
// (ShapedTree::Unlink), RetainOnly at every rebuild (ShapedTree::EndReset), Clear on a new GroupBy
// selector (EndReset) and at teardown (ShapedTree::Clear). Identity embeds a
// reference key's IUnknown address, so a stale entry could be inherited by a new key at a freed
// address.
//
// Keyed by the hstring GroupNode::Identity itself, not a hash of it: one representation of a
// group's identity, no collisions, and copies share the reference-counted buffer. Lookups are per
// group, never per row.
struct GroupNode;

class GroupExpansion
{
public:
    // Baseline for a group with no entry. Groups start expanded, as in v1.
    bool IsExpanded(winrt::hstring const& identity) const;

    // Records an exception, or erases it when expanded equals the baseline.
    void SetExpanded(winrt::hstring const& identity, bool expanded);

    // ExpandAllGroups / CollapseAllGroups: moves the baseline and drops every exception.
    void SetAll(bool expanded);

    // Prune: a group dropped by a surgical change (ShapedTree::Unlink without isMove, once per
    // group in the removed subtree). O(1).
    void Erase(winrt::hstring const& identity);

    // Prune: at every engine rebuild's ShapedTree::EndReset (filter, sort, regroup, Reapply, E7
    // bulk). Keeps only the exceptions whose identity is a live group (the engine's
    // m_groupsByIdentity, built while the old tree still held its keys). O(exceptions).
    // Spelled out rather than GroupMap (Engine/ShapedTree.h), which includes this header.
    void RetainOnly(std::unordered_map<winrt::hstring, std::unique_ptr<GroupNode>> const& liveGroups);

    // GroupBy with a new selector (old identities mean other keys) and teardown: every exception
    // is dropped. The baseline is kept: an ExpandAllGroups / CollapseAllGroups is a user preference that survives a regroup
    // (v1 resets to expanded instead).
    void Clear();

private:
    bool m_baselineExpanded{ true };

    // Identities whose state is the opposite of m_baselineExpanded.
    std::unordered_set<winrt::hstring> m_exceptions;
};
