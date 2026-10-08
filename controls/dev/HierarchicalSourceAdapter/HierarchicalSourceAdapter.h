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

// Single-responsibility hierarchical adapter -- the sibling of GroupedSourceAdapter on the other
// flattening axis.
//
// The adapter has ONE job: project a validated, immutable ParentKeyIndex (built by layer 2 from the
// app's flat source) into the currently VISIBLE rows, held in an ordinary IObservableVector that the
// control wraps in an ItemsSourceView and hands to the ItemsRepeater. "Visible" means not hidden
// behind a collapsed ancestor -- the walk never descends into a collapsed node.
//
// It is a pure projector. It observes nothing (layer 2 is the only listener on the source), calls no
// app code (the index already holds every key and every child list), and cannot fail part-way (the
// index was validated before it was handed over), so a walk or a splice either publishes or does
// nothing.
//
// Every row this adapter produces is a RAW APP ITEM. There is no header entry, no wrapper, no
// per-row COM allocation. "Parent-ness" is row METADATA -- depth, has-children, is-expanded,
// sibling position -- carried in a parallel POD vector, not in the row value.
//
// Lifetime, threading and re-entrancy are copied from GroupedSourceAdapter deliberately:
// UI-thread-affine, m_rebuildInFlight / m_pendingRebuild coalescing, teardown on the owning thread
// because every strong owner is a ReferenceTracker. Always owned by a shared_ptr: every public entry
// point that publishes pins itself with shared_from_this, because a handler of its own notification
// may drop the last external owner (and ClearIndex detaches it; see there).
//
// See docs/design-notes/TabularControls/Hierarchical-Data-Rows-Parent-Key-Design.md.
class HierarchicalSourceAdapter : public std::enable_shared_from_this<HierarchicalSourceAdapter>
{
public:
    // UI-thread-affine: construct on a UI thread with a DispatcherQueue. The queue is captured only
    // to assert that affinity in chk. Constructing off a UI thread throws RPC_E_WRONG_THREAD.
    //
    // The constructor also flips the expansion baseline to COLLAPSED. RowExpansionModel defaults to
    // EXPANDED, which is right for grouping and wrong here: every node nobody has touched would
    // resolve expanded, and the very first Rebuild would realize the whole tree.
    HierarchicalSourceAdapter();

    // The flat row axis the repeater consumes: a single ItemsSourceView wrapping the materialized
    // observable vector. Created ONCE over m_entries (whose identity is stable for the adapter's
    // lifetime -- Rebuild ReplaceAll's its contents and a single-node toggle splices a contiguous
    // run in place), so an identity a consumer captures stays valid across rebuilds.
    winrt::ItemsSourceView Entries() const { return m_entriesView; }

    // Publishes a new projection over `index`, then prunes expansion intent to the keys that still
    // exist in the UNFILTERED source (so intent for a row hidden by a filter survives the filter).
    //
    // `rootSegments` partitions index->Roots into consecutive runs of the given lengths, one per
    // group bucket under a grouped hierarchy. A root's sibling position is computed within its
    // segment. Empty means one segment spanning every root (ungrouped).
    void SetIndex(std::shared_ptr<const ShapingHelpers::ParentKeyIndex> index, std::vector<size_t> rootSegments);

    // Drops the index WITHOUT publishing (see the .cpp). Intent is kept; see ResetIntentQuietly.
    // Also DETACHES the adapter: the projection it feeds is being retracted, so a publication that
    // is still unwinding when this runs (the owner re-entered from one of its notifications) stops
    // at the next safe point and raises nothing further. Only SetIndex re-attaches it.
    void ClearIndex();

    // Clears all expansion intent and the filter overlay WITHOUT rebuilding. Used when the parent
    // relation is re-declared or retracted: a different relation is a different tree, so intent
    // recorded against the old one must not leak into the new one. The caller follows with
    // SetIndex / ClearIndex, which publishes.
    void ResetIntentQuietly();

    // Drops the collapse overrides recorded against filter context rows. Called when the filter
    // changes; the following SetIndex publishes.
    void ResetFilterOverlay();

    // "The published projection changed" -- raised ONCE per coherent publish, after the entries,
    // the descriptors and the index map all agree again. Deliberately not Entries().CollectionChanged,
    // which fires from inside the vector mutation (torn state) and once per row of a splice.
    void ProjectionChanged(std::function<void()> fn);

    // Per-node expand / collapse, addressed by NODE key ("node:" + the app key's lookup form). The
    // app key survives re-sort, re-filter, regroup, reparenting and re-creation of the item object,
    // so intent keyed by it does too.
    //
    // IsNodeExpanded answers the state the row is PRESENTED in when the node is visible, and
    // otherwise the overlay for a context key and the recorded intent for any other key.
    // SetNodeExpanded on a context row moves only the filter overlay; it never writes intent.
    bool IsNodeExpanded(winrt::hstring const& nodeKey) const;
    void SetNodeExpanded(winrt::hstring const& nodeKey, bool isExpanded);

    // Bulk verbs. These move the model's BASELINE rather than looping over live nodes, so a node
    // that does not exist yet still inherits the intent -- and so they never splice. One Reset.
    void ExpandAll();
    void CollapseAll();

    // Expands `nodeKey` and every descendant that has children, then publishes once. Context rows
    // in the subtree are expanded through the overlay (no intent), all others through intent.
    void ExpandSubtree(winrt::hstring const& nodeKey);

    // Index -> descriptor, for the row-metadata provider. A POD side-vector, index-aligned 1:1 with
    // the visible rows, costing a few words per VISIBLE row with zero COM allocations.
    struct NodeRow
    {
        winrt::IInspectable Item{ nullptr };
        winrt::hstring NodeKey;
        // Node key of the parent row; empty for roots.
        winrt::hstring ParentKey;

        // 0-BASED. Roots are 0, because indent is Depth * RowIndentSize and a root must render
        // flush. TableViewRowInfo::Level is 1-based, so RowMetadataProvider emits Depth + 1.
        int32_t Depth{ 0 };

        // Always known: the number of children the index kept for this node.
        int32_t ChildCount{ 0 };

        // 1-based position within the node's shaped sibling set, and that set's size. For roots
        // under grouping the sibling set is the root's group bucket. Back PositionInSet/SizeOfSet.
        int32_t SiblingIndex{ 0 };
        int32_t SiblingCount{ 0 };

        bool HasChildren{ false };
        bool IsExpanded{ false };

        // Kept only because a descendant matched the active filter.
        bool IsContext{ false };
    };

    NodeRow const* TryGetNodeRow(int32_t index) const;
    bool TryGetIndexForNodeKey(winrt::hstring const& nodeKey, int32_t& index) const;

    // Descriptor for a visible row addressed by its ITEM rather than its index. Needed when the
    // hierarchy is composed under grouping: the presented row axis is then the grouped adapter's,
    // whose indices interleave header rows and so do not line up with this adapter's. Sound because
    // an item occupies at most one row (every item has exactly one parent). Returns null for an item
    // that is not currently visible.
    NodeRow const* TryGetNodeRowForItem(winrt::IInspectable const& item) const;

    // Does this expansion key address a NODE in this adapter's key space? Positive prefix test, so
    // a stale or foreign key cannot become node intent.
    static bool IsNodeKey(winrt::hstring const& key);

private:
    void Rebuild();
    bool OnUiThread() const;
    void AssertRebuildOnUiThread() const;
    void OnExpansionChanged(ShapingHelpers::RowExpansionModel::Change const& change);

    // Brings ONE node's presented state in line with its effective state: a ranged splice when
    // possible, the authoritative Rebuild otherwise. Coalesces with an in-flight rebuild.
    void ApplySingleToggle(winrt::hstring const& nodeKey, bool isExpanded);

    // Presented expansion state: a context row follows the filter overlay alone (expanded unless
    // the user collapsed it under the current filter); every other row follows persistent intent.
    bool EffectiveExpanded(winrt::hstring const& nodeKey, bool hasChildren, bool isContext) const;

    // EffectiveExpanded for a key addressed without its descriptor; reads has-children and
    // context-ness from the index.
    bool EffectiveExpandedForKey(winrt::hstring const& nodeKey) const;

    // Appends the visible rows for siblings[begin, end) -- each row followed by its visible
    // subtree -- with an explicit stack rather than recursion, so depth is unbounded.
    void EmitRange(
        std::vector<winrt::IInspectable> const& siblings,
        size_t begin,
        size_t end,
        winrt::hstring const& parentKey,
        int32_t depth,
        std::vector<winrt::IInspectable>& built,
        std::vector<NodeRow>& descriptors) const;

    // Incremental expand/collapse of a SINGLE node: splices just that node's visible subtree into
    // or out of the projection instead of dropping and re-realizing every container via a full
    // Rebuild + Reset.
    //
    // The run is contiguous by construction: a node's visible descendants are exactly the rows after
    // it whose Depth is greater than its own. Collapse removes that run; expand emits the subtree
    // from the index (honouring nested intent) and inserts it.
    //
    // Returns false -- and the caller falls back to Rebuild -- when the key does not resolve, the
    // descriptor/entry counts disagree, or the run exceeds c_maxSpliceRows.
    bool TryApplyExpansionSplice(winrt::hstring const& nodeKey, bool expand);

    // The three structures below are only correct AS A SET. Every mutation goes through these two
    // helpers rather than touching the vectors directly.
    void InsertRows(int32_t index, std::vector<winrt::IInspectable> const& items, std::vector<NodeRow> const& descriptors);
    void RemoveRows(int32_t index, int32_t count);

    void RaiseProjectionChanged();

    // Rebuilds m_indexByItem from m_descriptors if a mutation has invalidated it. const because
    // the lookup it serves is const; the cache members are mutable for the same reason.
    void EnsureItemIndex() const;

    // Same, for m_indexByNodeKey (invalidated row by row during a splice).
    void EnsureNodeKeyIndex() const;

    // Mutates the expansion model without its Changed notification driving a splice or rebuild.
    // Returns whether the model reported any change, so the caller can publish once.
    template <typename Fn>
    bool MutateExpansionQuietly(Fn&& fn);

    // The validated index being projected, and how its roots are partitioned into group buckets.
    std::shared_ptr<const ShapingHelpers::ParentKeyIndex> m_index;
    // The structure expansion intent was last pruned against (see SetIndex).
    std::weak_ptr<const ShapingHelpers::ParentStructure> m_prunedForStructure;
    std::vector<size_t> m_rootSegments;

    // Context rows the user collapsed while the current filter is active. Cleared when the filter
    // changes; never written into persistent intent.
    std::unordered_set<winrt::hstring> m_filterOverlayCollapsed;

    // See ProjectionChanged. Held by value; the owner clears it by passing nullptr.
    std::function<void()> m_projectionChanged{ nullptr };

    // THE flat projection: the visible rows, as raw app items.
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> m_entries{
        winrt::single_threaded_observable_vector<winrt::IInspectable>() };

    // One ItemsSourceView over m_entries, created once (m_entries identity is stable for life).
    winrt::ItemsSourceView m_entriesView{ nullptr };

    // Row metadata, index-aligned 1:1 with m_entries.
    std::vector<NodeRow> m_descriptors;

    // Node key -> index, so the metadata provider resolves an identity by hash. Rebuild publishes it
    // eagerly; a splice invalidates it row by row (see InsertRows) and the next lookup rebuilds it
    // in one O(visible rows) pass from m_descriptors, so it can never disagree with them.
    mutable std::unordered_map<winrt::hstring, int32_t> m_indexByNodeKey;
    mutable bool m_indexByNodeKeyValid{ true };

    // Item ABI pointer -> descriptor index, for the grouped hierarchical path where a row's index
    // addresses the grouped axis and the item is the only handle both axes share. A LAZY cache:
    // every mutation invalidates it and the next lookup rebuilds it in one O(V) pass.
    mutable std::unordered_map<void*, int32_t> m_indexByItem;
    mutable bool m_indexByItemValid{ false };

    // Expand/collapse intent, keyed by node key. Baseline is flipped to collapsed in the constructor.
    ShapingHelpers::RowExpansionModel m_expansion;

    // Set while MutateExpansionQuietly runs; the model's Changed handler records instead of acting.
    bool m_suppressExpansionChanged{ false };
    bool m_expansionChangedWhileSuppressed{ false };

    // Affinity assertion only (chk); the adapter never marshals.
    winrt::weak_ref<winrt::Microsoft::UI::Dispatching::DispatcherQueue> m_uiQueue{ nullptr };

    // Re-entrancy guard: a request that arrives while a rebuild or splice is publishing is
    // remembered and run once after unwind. Plain bool: the adapter is UI-thread-affine.
    bool m_rebuildInFlight{ false };
    bool m_pendingRebuild{ false };

    // Set by ClearIndex. Every notifying loop checks it after each vector mutation, because the
    // owner may retract the projection from inside that notification; see ClearIndex.
    bool m_detached{ false };
};

using HierarchicalSourceAdapterPtr = std::shared_ptr<HierarchicalSourceAdapter>;
