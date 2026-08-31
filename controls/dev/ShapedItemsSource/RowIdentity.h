// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>

#include "ShapingDescriptions.h"

// The row-identity index of the shaping stack: how a row's stable string identity is produced,
// validated, and mapped to its position in the projection.
//
// Identity is what lets an incremental change be applied surgically instead of by rebuilding:
// given an identity, the projection index is a hash lookup rather than an O(n) ABI scan over the
// live vector. Keeping the set and the index together here is what keeps the two in step -- they
// are only correct as a pair, and every mutation path has to shift both the same way.
namespace RowIdentity
{
    // A caller-supplied projection from an item to its stable string identity. Distinct from
    // TabularKeySelector, which returns an arbitrary key object.
    using TabularIdentitySelector = std::function<winrt::hstring(winrt::IInspectable const& item)>;


    // The row-identity selector, derived from each item's COM identity (its canonical IUnknown
    // pointer). That is unique among live objects and stable for as long as the object lives, which
    // is exactly the window a projection needs. It is NOT stable across a re-created item -- an app
    // that rebuilds its item objects gets fresh identities and therefore a full rebuild rather than
    // a surgical update. There is no app-supplied alternative: this is the only source of row
    // identity.
    TabularShapingHelpers::TabularKeySelector MakeObjectIdentitySelector();

    bool TryGetRequiredRowIdentity(
        winrt::IInspectable const& item,
        TabularShapingHelpers::TabularKeySelector const& keySelector,
        winrt::hstring& identity,
        wchar_t const*& reason);

    // Confirm every row has a usable identity and that no two rows share one. Since identity is the
    // item's object address, the only way that can fail is one object occupying more than one row.
    bool ValidateRowIdentities(
        std::vector<winrt::IInspectable> const& rows,
        TabularShapingHelpers::TabularKeySelector const& keySelector,
        wchar_t const*& reason);

    void ClearFlatRowIdentityTracking(
        std::unordered_set<winrt::hstring>& identities,
        std::unordered_map<winrt::hstring, uint32_t>& identityToIndex);

    void RebuildFlatRowIdentityTracking(
        std::vector<winrt::IInspectable> const& rows,
        bool identityRequired,
        TabularShapingHelpers::TabularKeySelector const& keySelector,
        std::unordered_set<winrt::hstring>& identities,
        std::unordered_map<winrt::hstring, uint32_t>& identityToIndex);
    bool TryGetTrackedFlatRowIndex(
        winrt::hstring const& identity,
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> const& rows,
        TabularShapingHelpers::TabularKeySelector const& keySelector,
        std::unordered_map<winrt::hstring, uint32_t> const& identityToIndex,
        uint32_t& index);
    // Cost contract: both shifts are O(n) in the number of tracked identities, because a hash map
    // keyed by identity has no way to reach "every entry at or after this position" without
    // visiting all of them. The work per entry is an integer compare and a conditional increment,
    // so the constant is small, but a stream of m mid-list changes is O(n*m).
    //
    // Two things keep that acceptable at the sizes this control targets. Callers skip the sweep
    // entirely for tail append and tail remove, which is the shape of bulk load, so filling a
    // projection is O(n) overall rather than O(n^2). And the indices must be exact rather than
    // approximate: callers compare the tracked index against the index the source reported and
    // fall back to a full reprojection on any mismatch, so the map cannot be allowed to drift and
    // repaired lazily.
    //
    // The mid-list insert stream -- an append into a sorted projection -- is the case this does not
    // cover, and it is inherent to storing absolute indices. Removing the map does not fix it: the
    // lookup then becomes a linear scan that extracts an identity per row, which is more expensive
    // per element than the shift it replaces. The structural fix is an order-statistics container
    // that maintains positions in O(log n), which is what a live-shaping projection would need.
    void ShiftTrackedFlatRowIndicesForInsert(
        std::unordered_map<winrt::hstring, uint32_t>& identityToIndex,
        uint32_t insertedIndex);
    void ShiftTrackedFlatRowIndicesForRemove(
        std::unordered_map<winrt::hstring, uint32_t>& identityToIndex,
        uint32_t removedIndex);

    bool TryGetGroupIdentity(
        winrt::IInspectable const& key,
        TabularIdentitySelector const& groupIdentitySelector,
        winrt::hstring& identity,
        wchar_t const*& reason);
    bool GroupKeysEqual(winrt::IInspectable const& a, winrt::IInspectable const& b);
    winrt::hstring StringifyKey(winrt::IInspectable const& key);
}
