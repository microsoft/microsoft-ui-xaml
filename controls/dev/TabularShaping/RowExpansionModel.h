// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <winrt/Windows.Foundation.h>

// Layer 1 of the Tabular shaping stack: expand/collapse INTENT, separated from the structure the
// intent applies to. Shared by both flattening axes — grouping keys it by a group's stable
// identity, hierarchy keys it by a node's stable identity — which is why it lives in layer 1
// rather than beside either adapter.
//
// A group's (or node's) expansion state looks like it belongs on the group, but it does not.
// Groups are derived: the shaping layer above re-mints them on every reshape, so state stored on
// a group dies with each sort, filter or regroup — which is exactly why "collapse a group, then
// re-sort" used to lose the collapse. Expansion is a property of the USER's intent about a KEY,
// and it outlives every group object that key ever had.
//
// So this type stores intent keyed by a caller-supplied string, holds no reference to any group,
// and knows nothing about rows, runs or a projection. What it owns is the one non-obvious rule:
// intent is stored only where it DIFFERS from the default. That is what makes "expand all" O(1)
// rather than O(groups), keeps the store from growing without bound across many toggles, and
// makes `SetAllExpanded` behave correctly for groups that do not exist yet -- a group that
// arrives later inherits the default rather than an intent nobody expressed about it.
//
// Deliberately free of WinRT collection types, XAML and any tabular vocabulary: a TreeView, a
// grouped ItemsRepeater or an app-authored hierarchy can use it directly, and it is testable
// with no dispatcher and no host.
namespace ShapingHelpers
{
    class RowExpansionModel
    {
    public:
        // Keys whose resolved state changed, and what they changed TO. Empty `Keys` with
        // `AffectsAllKeys` true means the default moved, so every key with no explicit intent
        // changed at once -- the case a consumer must answer with a full rebuild rather than a
        // per-key splice.
        struct Change
        {
            std::vector<winrt::hstring> Keys;
            bool AffectsAllKeys{ false };
            bool IsExpanded{ true };
        };

        using ChangedHandler = std::function<void(Change const&)>;

        // Raised after intent changes, never during. A handler may re-enter and read state.
        void SetChangedHandler(ChangedHandler handler) { m_changed = std::move(handler); }

        // What a key with no explicit intent resolves to. Setting it CLEARS every explicit
        // intent: a caller changing the default is declaring a new baseline, and keeping the
        // old exceptions would resolve keys against a baseline nobody asked for.
        bool DefaultExpanded() const noexcept { return m_defaultExpanded; }
        void SetDefaultExpanded(bool expanded);

        bool IsExpanded(winrt::hstring const& key) const;
        void SetExpanded(winrt::hstring const& key, bool isExpanded);
        void Toggle(winrt::hstring const& key) { SetExpanded(key, !IsExpanded(key)); }

        // Moves the baseline and drops every exception, so keys that do not exist yet also
        // resolve to `isExpanded`. This is "expand all" as an intent, not as a loop over the
        // groups that happen to be live.
        void SetAllExpanded(bool isExpanded);

        // Drops intent for keys that no longer exist. Without this, an intent for a group that
        // vanished (a filter removed its last row, the source was reassigned) lingers forever and
        // the store grows unbounded across changing datasets. Silent: pruning a dead key changes
        // no live key's resolved state.
        //
        // Safe for a consumer whose live-key set is COMPLETE -- grouping's is, because every
        // group emits a header whether it is collapsed or not. A consumer that only enumerates
        // part of its structure must use RetainOnlyUnder instead.
        void RetainOnly(std::unordered_set<winrt::hstring> const& liveKeys);

        // The same prune, scoped to the part of the structure the caller actually LOOKED AT.
        //
        // A lazy consumer -- a tree that does not descend into collapsed nodes -- cannot produce
        // a complete live-key set. Its collapsed subtrees are not absent because they vanished;
        // they are absent because nobody walked them. Handing that partial set to RetainOnly
        // silently deletes the user's intent for everything hidden behind a collapse, which shows
        // up as "collapse a grandchild, collapse its parent, re-expand the parent, and the
        // grandchild is expanded again".
        //
        // So the caller also reports which keys it enumerated IN FULL, as a set of parent path
        // prefixes. A key is dropped only when some ANCESTOR-or-self of it has an enumerated
        // parent prefix and is itself not live -- i.e. only when the caller genuinely looked where
        // that branch should have been and did not find it. Keys with no enumerated ancestor are
        // left alone and get their chance to be pruned the next time that part of the structure is
        // walked.
        //
        // The walk has to go all the way up, not just to the immediate parent: if A was enumerated
        // and its child B vanished, nothing ever enumerates A/B again, so intent stored for A/B/C
        // would otherwise survive forever.
        //
        // `parentPrefixOf` extracts a key's immediate parent prefix; it is supplied by the caller
        // because the key format is the caller's, not this type's -- this class deliberately knows
        // nothing about paths, groups, rows or any other structure. It is expected to reach a fixed
        // point (or an empty string) at a root; the walk is bounded either way.
        void RetainOnlyUnder(
            std::unordered_set<winrt::hstring> const& liveKeys,
            std::unordered_set<winrt::hstring> const& enumeratedPrefixes,
            std::function<winrt::hstring(winrt::hstring const&)> const& parentPrefixOf);

        void Clear();

    private:
        // Upper bound on the ancestor walk in RetainOnlyUnder. Generous relative to any real
        // structure's depth; it exists so a malformed key format that never reduces cannot spin.
        static constexpr int32_t c_maxPrefixWalk = 1024;

        // True when an ancestor-or-self of `key` was looked for and not found, which proves the
        // whole branch below that ancestor is gone.
        static bool IsProvablyDead(
            winrt::hstring const& key,
            std::unordered_set<winrt::hstring> const& liveKeys,
            std::unordered_set<winrt::hstring> const& enumeratedPrefixes,
            std::function<winrt::hstring(winrt::hstring const&)> const& parentPrefixOf);

        void RaiseChanged(Change change) const;

        bool m_defaultExpanded{ true };
        // Exceptions only. A key present here resolves to the OPPOSITE of m_defaultState, which
        // is why moving the default has to clear the set rather than reinterpret it.
        std::unordered_set<winrt::hstring> m_nonDefault;
        ChangedHandler m_changed{ nullptr };
    };
}
