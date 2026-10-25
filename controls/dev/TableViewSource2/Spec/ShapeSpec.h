// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include <winrt/Windows.Foundation.h>

// Shape spec for TableViewSource2 (declarations only).
// See docs/design-notes/TabularControls/Shaping-Architecture-Redesign.md, "5. Shape spec".
//
// The shape spec is the set of verbs in force, stored as plain data: WHAT is applied. App
// delegates (HOW) live in ShapeDelegates and are referred to here by DelegateId. So the spec:
//   - holds no COM references, needs no tracker_ref and has no teardown rules;
//   - is copyable and comparable with ==;
//   - can be read by column headers and saved settings (paths stay strings).
//
// Not in the spec: hierarchy (ParentBy / ChildrenBy selectors live in SourceTable; a hierarchy
// change arrives as a SourceTable Reset) and IsLiveShaping (a SourceTable setting).
//
// Logical order: filter -> group -> sort rows within each group. The engine may pre-sort for
// efficiency as long as the result matches this order.
//
// Open (decided with the engine): who owns the spec, whether verbs edit a pending copy in place or
// produce new copies, and how DeferRefresh batches are applied.

// Refers to a delegate registered in ShapeDelegates. A plain value: holding or copying one does not
// keep the delegate alive; the registry decides that at commit. The same delegate object always
// maps to the same id, which is how repeated Sort calls are matched.
enum class DelegateId : uint32_t
{
    None = 0,
};

// One declaration of a delegate. ShapeDelegates::Register returns a new Version on every call, even
// for a delegate it already knows. A delegate can read app state that has moved (for example, a
// filter comparing against a field), so re-declaring the same delegate must still count as a
// change (as v1 does); the Version makes that visible to ==, keeping Diff a pure comparison of two
// specs. Matching (SortSpec::Matches) uses Id only.
struct DelegateRef
{
    DelegateId Id{ DelegateId::None };
    uint32_t Version{ 0 };

    explicit operator bool() const noexcept { return Id != DelegateId::None; }
    bool operator==(DelegateRef const&) const = default;
};

// One filter: a TableViewPredicate2.
struct FilterSpec
{
    DelegateRef Predicate;

    bool operator==(FilterSpec const&) const = default;
};

// One sort level. Levels are ordered by priority: the first decides, later ones break ties.
struct SortSpec
{
    enum class Kind : uint8_t
    {
        Path,       // Sort(sortMemberPath, direction)
        Key,        // Sort(keySelector, direction)
        Comparer,   // Sort(comparer)
    };

    Kind SortKind{ Kind::Key };

    // Path only. Matched by string, so re-sorting the same path replaces this level. Re-declaring
    // a path with the same direction yields an equal SortSpec (no work): the item's own property
    // is covered by live shaping.
    winrt::hstring Path;

    // Key: a TableViewKeySelector2. Comparer: a TableViewComparer2. Matched by delegate Id.
    DelegateRef Delegate;

    // Path and Key: Ascending or Descending (None removes the level, so it is never stored).
    // Comparer: always None; the comparer defines the full order of its level.
    winrt::SortDirection2 Direction{ winrt::SortDirection2::None };

    // True when this level and `other` refer to the same path or the same delegate Id, regardless
    // of direction and Version. A Sort call matching an existing level replaces it in place.
    bool Matches(SortSpec const& other) const noexcept;

    bool operator==(SortSpec const&) const = default;
};

struct GroupSpec
{
    // A TableViewKeySelector2.
    DelegateRef Key;

    // Orders the groups by key. None keeps the order in which groups first appear in the filtered
    // source. Row sort (ShapeSpec::Sorts) never moves groups.
    winrt::SortDirection2 Direction{ winrt::SortDirection2::None };

    bool operator==(GroupSpec const&) const = default;
};

struct ShapeSpec
{
    // Combined with AND, evaluated in order, stopping at the first rejection; a throwing predicate
    // rejects the item. The public Filter verb replaces the existing filter, so today this holds at
    // most one entry; the list leaves room for per-column filters.
    std::vector<FilterSpec> Filters;

    // nullopt = not grouped.
    std::optional<GroupSpec> Group;

    // [0] = highest priority. Empty = source order.
    std::vector<SortSpec> Sorts;

    // Equal specs need no work: every re-declaration bumps a Version.
    bool operator==(ShapeSpec const&) const = default;
};

// What the engine has to redo to go from one spec to the next. Flags, so independent changes
// (for example group order and row sort) combine. Hierarchy changes are not here; they arrive from
// SourceTable as a Reset. Per-row key cache invalidation is added if the engine caches keys.
enum class ShapeDelta : uint8_t
{
    None = 0x0,
    Sort = 0x1,         // re-sort rows within each group
    GroupOrder = 0x2,   // reorder groups only
    GroupKey = 0x4,     // re-bucket into groups, then re-sort
    Filter = 0x8,       // re-filter from SourceTable (no source re-read), then regroup and re-sort
};
DEFINE_ENUM_FLAG_OPERATORS(ShapeDelta);

// Pure comparison of two specs (including Versions). Filter changes are never classified as
// narrowing or widening: predicates are opaque, so any change re-tests every record.
ShapeDelta Diff(ShapeSpec const& before, ShapeSpec const& after) noexcept;
