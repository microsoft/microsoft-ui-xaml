// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "ShapingDescriptions.h"

namespace ShapingHelpers
{
// Parent-key index over a flat, already-sorted row list. Pure and immutable once built.
struct ParentKeyIndex
{
    // Items are in sorted order within every sibling list.
    std::vector<winrt::IInspectable> Roots;
    std::unordered_map<std::wstring, std::vector<winrt::IInspectable>> Children; // parent nodeKey -> children
    std::unordered_map<void*, std::wstring> KeyByItem;                            // ABI pointer -> nodeKey
    std::unordered_set<std::wstring> UnfilteredKeys; // every key in the source (expansion pruning)
    std::unordered_set<std::wstring> ContextKeys; // kept only because a descendant matched the filter

    const std::vector<winrt::IInspectable>* TryGetChildren(std::wstring const& nodeKey) const;
    std::wstring const* TryGetKey(winrt::IInspectable const& item) const;
};

using ParentKeyFilter = std::function<bool(winrt::IInspectable const&)>;

// Returns false and fills `error` on duplicate / null key, self-parent or cycle.
// `filter` may be empty (no filter). When set, only matches + their ancestors are indexed.
bool BuildParentKeyIndex(
    std::vector<winrt::IInspectable> const& sortedRows,
    KeySelector const& keySelector,
    KeySelector const& parentKeySelector,
    ParentKeyFilter const& filter,
    ParentKeyIndex& out,
    winrt::hstring& error);

// Exposed for the adapter: "node:" + lookup key, or empty when `key` means "no key".
std::wstring MakeNodeKey(winrt::IInspectable const& key);

// User-facing form of a node key for error text: the raw value for a value key, or a placeholder
// for an object key, whose lookup form is only a pointer.
winrt::hstring DescribeNodeKey(std::wstring_view nodeKey);

}
