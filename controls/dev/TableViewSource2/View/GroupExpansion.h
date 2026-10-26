// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

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
// a v2 type: no change handler (the flattener toggles state itself and builds the delta), and a
// single-key Erase for the incremental prune.
//
// Owned by ShapingManager; read and written only by the Flattener. Holds no app object (keys are
// identity strings, not group key objects). UI-thread affine.
//
// Keyed by the hstring GroupNode::Identity itself, not a hash of it: one representation of a
// group's identity, no collisions, and copies share the reference-counted buffer. Lookups are per
// group, never per row.
class GroupExpansion
{
public:
    // Baseline for a group with no entry. Groups start expanded, as in v1.
    bool IsExpanded(winrt::hstring const& identity) const;

    // Records an exception, or erases it when expanded equals the baseline.
    void SetExpanded(winrt::hstring const& identity, bool expanded);

    // ExpandAllGroups / CollapseAllGroups: moves the baseline and drops every exception.
    void SetAll(bool expanded);

    // Prune: a group dropped by an incremental change. O(1).
    void Erase(winrt::hstring const& identity);

    // Prune: full regroup. Keeps only entries whose identity is in liveIdentities (the new groups,
    // built while the old tree still holds its keys).
    void RetainOnly(std::unordered_set<winrt::hstring> const& liveIdentities);

    // GroupBy with a new selector: old identities mean other keys, so every exception is dropped.
    // The baseline is kept: an ExpandAllGroups / CollapseAllGroups is a user preference that survives a regroup
    // (v1 resets to expanded instead).
    void Clear();

private:
    bool m_baselineExpanded{ true };

    // Identities whose state is the opposite of m_baselineExpanded.
    std::unordered_set<winrt::hstring> m_exceptions;
};
