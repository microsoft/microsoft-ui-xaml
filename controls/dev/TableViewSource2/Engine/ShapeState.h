// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include <winrt/Windows.Foundation.h>

#include "tracker_ref.h"

struct SourceRecord;

// Engine state stored inline on every SourceRecord (declarations only).
// See docs/design-notes/TabularControls/Shaping-Architecture-Redesign.md, "Engine state storage".
//
// A leaf header: only the standard library, WinRT and tracker_ref. SourceRecord.h includes it, so
// Source/ depends on this one header and not on the engine.
//
// Inline, not a lazily allocated unique_ptr: the large parts are vectors, which allocate nothing
// until used, so inline costs only the fixed fields per record and avoids a heap allocation per
// record when shaping starts, a pointer hop and null check on every access, and a rule for when to
// free. Revisit if the fixed size grows.
//
// Written only by the engine (and the flattener for VisibleDescendantCount). The source tree links
// (SourceRecord::Parent/Children) are never written here. UI-thread affine.
enum class ShapeFlags : uint8_t
{
    None = 0x0,

    // Passes every filter predicate (true when no filter is active).
    PassesFilter = 0x1,

    // Some source descendant passes the filter, so this record is shown as an ancestor of a match
    // even if it fails the filter itself.
    HasPassingDescendant = 0x2,

    // ShapedChildren is current. When clear, the shaped children are SourceRecord::Children as-is
    // (no filter and no sort active, or the record is not expanded and has no matches yet).
    ChildrenShaped = 0x4,

    // SortKeys is current for the applied spec. Cleared on ItemChanged and on a ShapeDelta::Sort.
    SortKeysValid = 0x8,
};
DEFINE_ENUM_FLAG_OPERATORS(ShapeFlags);

struct ShapeState
{
    ShapeFlags Flags{ ShapeFlags::None };

    // Visible rows below this record in the visible list (honouring expansion). Lets the flattener
    // find a row's visible position in O(depth) and splice only the affected range. Matches
    // GroupNode::VisibleDescendantCount for groups. Maintained by the flattener.
    uint32_t VisibleDescendantCount{ 0 };

    // One cached key per SortSpec of the applied spec, in level order. Path and Key levels hold the
    // value read once per record (PathEvaluator / ShapeDelegates); Comparer levels hold nothing (the
    // comparer is called with the items). Optional so the engine can destroy keys while the owner
    // is alive; tracked like SourceRecord::Item because a key can reference the app's objects.
    // Valid only while SortKeysValid is set.
    std::vector<std::optional<tracker_ref<winrt::Windows::Foundation::IInspectable>>> SortKeys;

    // Source children after filter and sort, in shaped order. Non-owning. Valid only while
    // ChildrenShaped is set; built only for parents that are expanded or have matches.
    std::vector<SourceRecord*> ShapedChildren;

    // Group membership is not stored here: the engine maps top-level records to their GroupNode
    // (m_groupOf, see the shaped tree section), and the group key lives on GroupNode::Key.
};
