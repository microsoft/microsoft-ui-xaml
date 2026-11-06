// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <memory>
#include <span>
#include <unordered_map>

#include <winrt/Windows.Foundation.h>

#include "ShapeSpec.h"

struct SortMemberPathResolver;   // TableView/SortMemberPathResolver.h, shared with v1

// Evaluates the property paths of SortSpec levels of kind Path (declarations only).
// See docs/design-notes/TabularControls/Shaping-Architecture-Redesign.md, "5. Shape spec".
//
// A path level carries a string, not a delegate, so it has no entry in ShapeDelegates. Its key is
// read with v1's SortMemberPathResolver (a one-time {Binding} on a throwaway ContentControl), so a
// path that displays in a cell also sorts, and v1 and v2 never disagree about what a path means.
//
// Caching: one resolver per path, created on first use. Building the binding is the expensive
// part; reusing it makes each later Value call a DataContext write and a property read.
// Retain() drops resolvers for paths no spec in use refers to, mirroring ShapeDelegates::Retain.
//
// Holds no app objects between calls (the resolver clears the item after reading the key), so it
// needs no tracker_ref.
//
// Threading: UI-thread affine (resolvers drive the binding engine). A path sort therefore reads
// its keys on the UI thread.
//
// Re-entrancy: Resolve runs the binding, so app code (a property getter, ICustomPropertyProvider)
// runs while a resolver is on the stack and can call back in. While a Value call is evaluating:
// - a nested Value gives nullptr (treated as a misbehaving record). Each resolver owns one probe,
//   so a nested read on the same path would overwrite the outer DataContext, then clear it, and
//   the outer read would return the wrong key. A nested read on another path would be safe but
//   is refused too, to keep the rule simple.
// - Retain does nothing; resolvers it would drop are dropped by the next Retain.
// - Clear is deferred to the end of the outermost Value, so the resolver on the stack is never
//   destroyed under itself.
// The guard covers this object's own state only. The owner must keep the PathEvaluator alive
// for the duration of a Value call.
class PathEvaluator
{
public:
    PathEvaluator() noexcept;
    ~PathEvaluator();

    PathEvaluator(PathEvaluator const&) = delete;
    PathEvaluator& operator=(PathEvaluator const&) = delete;

    // Reads `path` from `item`. A path that cannot be evaluated gives nullptr (spec, "Misbehaving
    // records"), as does a re-entrant call. Allocation failures (a resolver for a new path)
    // propagate; the engine reads keys in its compute phase, so nothing has been written.
    winrt::IInspectable Value(winrt::hstring const& path, winrt::IInspectable const& item);

    // Keeps only the resolvers for paths referenced by `specsInUse`; drops the rest. Allocates
    // nothing, so it cannot fail after an apply has committed. Does nothing while a Value call is
    // evaluating.
    void Retain(std::span<ShapeSpec const* const> specsInUse);

    // Teardown: destroys every resolver. Must run on the UI thread. While a Value call is
    // evaluating, the destruction is deferred to the end of the outermost Value.
    void Clear() noexcept;

private:
    std::unordered_map<winrt::hstring, std::unique_ptr<SortMemberPathResolver>> m_resolvers;

    // True while a resolver is evaluating, so app code may be running on this stack.
    bool m_isResolving{ false };

    // Clear was called while m_isResolving; runs when the outermost Value returns.
    bool m_clearPending{ false };
};
