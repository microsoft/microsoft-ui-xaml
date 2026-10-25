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
class PathEvaluator
{
public:
    PathEvaluator() noexcept;
    ~PathEvaluator();

    PathEvaluator(PathEvaluator const&) = delete;
    PathEvaluator& operator=(PathEvaluator const&) = delete;

    // Reads `path` from `item`. Error handling follows the engine's error rule (open, see the spec).
    winrt::IInspectable Value(winrt::hstring const& path, winrt::IInspectable const& item);

    // Keeps only the resolvers for paths referenced by `specsInUse`; drops the rest.
    void Retain(std::span<ShapeSpec const* const> specsInUse);

    // Teardown: destroys every resolver. Must run on the UI thread.
    void Clear() noexcept;

private:
    std::unordered_map<winrt::hstring, std::unique_ptr<SortMemberPathResolver>> m_resolvers;
};
