// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <functional>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "pch.h"
#include "common.h"

#include <winrt/Microsoft.UI.Dispatching.h>

#include "ParentKeyIndex.h"
#include "RowExpansionModel.h"

// Projects a validated, immutable ParentKeyIndex into the currently visible rows (raw app items; no
// wrappers). Pure projector: observes nothing, calls no app code, cannot fail part-way. Per-row
// metadata lives in a parallel POD vector. UI-thread-affine; publishing entry points pin themselves
// with shared_from_this because a notification handler may drop the last owner.
class HierarchicalSourceAdapter : public std::enable_shared_from_this<HierarchicalSourceAdapter>
{
public:
    // Must be constructed on a UI thread (throws RPC_E_WRONG_THREAD otherwise). Expansion baseline
    // is COLLAPSED so the first Rebuild does not realize the whole tree.
    HierarchicalSourceAdapter();

    // Created once over m_entries, whose identity is stable for the adapter's lifetime.
    winrt::ItemsSourceView Entries() const { return m_entriesView; }

    // Publishes a projection over `index`, then prunes intent to keys in the UNFILTERED source.
    // `rootSegments` splits index->Roots into per-group-bucket runs; empty means one segment.
    void SetIndex(std::shared_ptr<const ShapingHelpers::ParentKeyIndex> index, std::vector<size_t> rootSegments);

    // Drops the index without publishing; keeps intent. Detaches the adapter so an unwinding
    // publication stops at the next safe point. Only SetIndex re-attaches.
    void ClearIndex();

    // Clears intent and filter overlay without rebuilding (new relation = new tree). Caller then
    // calls SetIndex / ClearIndex.
    void ResetIntentQuietly();

    // Drops collapse overrides on filter context rows. Called when the filter changes.
    void ResetFilterOverlay();

    // Raised once per coherent publish. Not Entries().CollectionChanged, which fires mid-mutation
    // and once per spliced row.
    void ProjectionChanged(std::function<void()> fn);

    // Addressed by node key ("node:" + app key lookup form). SetNodeExpanded on a context row
    // moves only the filter overlay, never intent.
    bool IsNodeExpanded(winrt::hstring const& nodeKey) const;
    void SetNodeExpanded(winrt::hstring const& nodeKey, bool isExpanded);

    // Move the model's baseline, so not-yet-existing nodes inherit it. One Reset, no splice.
    void ExpandAll();
    void CollapseAll();

    // Expands `nodeKey` and all descendants (context rows via overlay), then publishes once.
    void ExpandSubtree(winrt::hstring const& nodeKey);

    // Row metadata, index-aligned 1:1 with the visible rows.
    struct NodeRow
    {
        winrt::IInspectable Item{ nullptr };
        winrt::hstring NodeKey;
        // Node key of the parent row; empty for roots.
        winrt::hstring ParentKey;

        // 0-based (roots render flush); TableViewRowInfo::Level is Depth + 1.
        int32_t Depth{ 0 };

        int32_t ChildCount{ 0 };

        // 1-based position in the shaped sibling set and its size. Under grouping, a root's
        // sibling set is its group bucket.
        int32_t SiblingIndex{ 0 };
        int32_t SiblingCount{ 0 };

        bool HasChildren{ false };
        bool IsExpanded{ false };

        // Kept only because a descendant matched the active filter.
        bool IsContext{ false };
    };

    NodeRow const* TryGetNodeRow(int32_t index) const;
    bool TryGetIndexForNodeKey(winrt::hstring const& nodeKey, int32_t& index) const;

    // Lookup by item, for grouped hierarchies where presented indices include header rows. Sound
    // because an item occupies at most one row. Null if not visible.
    NodeRow const* TryGetNodeRowForItem(winrt::IInspectable const& item) const;

    // Positive prefix test, so a stale or foreign key cannot become node intent.
    static bool IsNodeKey(winrt::hstring const& key);

private:
    void Rebuild();
    bool OnUiThread() const;
    void AssertRebuildOnUiThread() const;
    void OnExpansionChanged(ShapingHelpers::RowExpansionModel::Change const& change);

    // Splice when possible, Rebuild otherwise. Coalesces with an in-flight rebuild.
    void ApplySingleToggle(winrt::hstring const& nodeKey, bool isExpanded);

    // Context rows follow the filter overlay; all others follow persistent intent.
    bool EffectiveExpanded(winrt::hstring const& nodeKey, bool hasChildren, bool isContext) const;

    bool EffectiveExpandedForKey(winrt::hstring const& nodeKey) const;

    // Explicit stack, not recursion, so depth is unbounded.
    void EmitRange(
        std::vector<winrt::IInspectable> const& siblings,
        size_t begin,
        size_t end,
        winrt::hstring const& parentKey,
        int32_t depth,
        std::vector<winrt::IInspectable>& built,
        std::vector<NodeRow>& descriptors) const;

    // Splices one node's visible subtree (the contiguous run of deeper rows after it) in or out.
    // Returns false, so the caller rebuilds, if the key does not resolve, descriptor/entry counts
    // disagree, or the run exceeds c_maxSpliceRows.
    bool TryApplyExpansionSplice(winrt::hstring const& nodeKey, bool expand);

    // Entries, descriptors and index maps are only correct as a set; mutate only through these.
    void InsertRows(int32_t index, std::vector<winrt::IInspectable> const& items, std::vector<NodeRow> const& descriptors);
    void RemoveRows(int32_t index, int32_t count);

    void RaiseProjectionChanged();

    void EnsureItemIndex() const;
    void EnsureNodeKeyIndex() const;

    // Mutates expansion without Changed driving a splice/rebuild; returns whether anything changed.
    template <typename Fn>
    bool MutateExpansionQuietly(Fn&& fn);

    std::shared_ptr<const ShapingHelpers::ParentKeyIndex> m_index;
    // Structure intent was last pruned against (see SetIndex).
    std::weak_ptr<const ShapingHelpers::ParentStructure> m_prunedForStructure;
    std::vector<size_t> m_rootSegments;

    // Context rows collapsed under the current filter; cleared on filter change, never intent.
    std::unordered_set<winrt::hstring> m_filterOverlayCollapsed;

    std::function<void()> m_projectionChanged{ nullptr };

    // Visible rows, as raw app items.
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> m_entries{
        winrt::single_threaded_observable_vector<winrt::IInspectable>() };

    winrt::ItemsSourceView m_entriesView{ nullptr };

    // Index-aligned 1:1 with m_entries.
    std::vector<NodeRow> m_descriptors;

    // Lazy caches over m_descriptors: invalidated by mutations, rebuilt in one O(V) pass on lookup.
    mutable std::unordered_map<winrt::hstring, int32_t> m_indexByNodeKey;
    mutable bool m_indexByNodeKeyValid{ true };

    // Item ABI pointer -> index; the item is the only handle shared with the grouped axis.
    mutable std::unordered_map<void*, int32_t> m_indexByItem;
    mutable bool m_indexByItemValid{ false };

    ShapingHelpers::RowExpansionModel m_expansion;

    bool m_suppressExpansionChanged{ false };
    bool m_expansionChangedWhileSuppressed{ false };

    // Affinity assertion only (chk).
    winrt::weak_ref<winrt::Microsoft::UI::Dispatching::DispatcherQueue> m_uiQueue{ nullptr };

    // Re-entrancy guard: requests during a publish are coalesced and run once after unwind.
    bool m_rebuildInFlight{ false };
    bool m_pendingRebuild{ false };

    // Set by ClearIndex; notifying loops check it after each mutation (owner may retract mid-notify).
    bool m_detached{ false };
};

using HierarchicalSourceAdapterPtr = std::shared_ptr<HierarchicalSourceAdapter>;
