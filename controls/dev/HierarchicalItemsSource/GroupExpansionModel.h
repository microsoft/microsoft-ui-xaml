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

// Layer 3 of the Tabular shaping stack: expand/collapse INTENT, separated from the structure the
// intent applies to.
//
// A group's expansion state looks like it belongs on the group, but it does not. Groups are
// derived: the shaping layer above re-mints them on every reshape, so state stored on a group
// dies with each sort, filter or regroup — which is exactly why "collapse a group, then re-sort"
// used to lose the collapse. Expansion is a property of the USER's intent about a KEY, and it
// outlives every group object that key ever had.
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
namespace HierarchicalItemsSource
{
    class GroupExpansionModel
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
        void RetainOnly(std::unordered_set<winrt::hstring> const& liveKeys);

        void Clear();

    private:
        void RaiseChanged(Change change) const;

        bool m_defaultExpanded{ true };
        // Exceptions only. A key present here resolves to the OPPOSITE of m_defaultState, which
        // is why moving the default has to clear the set rather than reinterpret it.
        std::unordered_set<winrt::hstring> m_nonDefault;
        ChangedHandler m_changed{ nullptr };
    };
}
