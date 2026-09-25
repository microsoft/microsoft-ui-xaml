// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "pch.h"
#include "common.h"

#include <winrt/Microsoft.UI.Dispatching.h>

#include "HierarchyContract.h"
#include "RowExpansionModel.h"

// Single-responsibility hierarchical adapter -- the sibling of GroupedSourceAdapter on the other
// flattening axis.
//
// The adapter has ONE job: walk the app's intrinsic tree and MATERIALIZE the currently VISIBLE
// rows into one flat list, held in an ordinary IObservableVector that the control wraps in an
// ItemsSourceView and hands to the ItemsRepeater. "Visible" means not hidden behind a collapsed
// ancestor -- the walk never descends into a collapsed node, which is what makes a materialized
// row axis viable for a tree at all.
//
// The defining difference from grouping, and the source of nearly every design decision below:
// every row this adapter produces is a RAW APP ITEM. There is no header entry, no wrapper, no
// per-row COM allocation. A parent is not chrome above its children; it is a real data row that
// happens to own more rows. "Parent-ness" is therefore row METADATA -- depth, has-children,
// is-expanded -- carried in a parallel POD vector, not in the row value.
//
// Lifetime, threading and re-entrancy are copied from GroupedSourceAdapter deliberately, not
// coincidentally: UI-thread-affine, weak callbacks, m_rebuildInFlight / m_pendingRebuild
// coalescing, teardown on the owning thread because every strong owner is a ReferenceTracker.
// Two adapters that drift on those contracts would be two bugs, not one.
//
// See docs/design-notes/TabularControls/Hierarchical-Data-Rows-Design.md.
class HierarchicalSourceAdapter : public std::enable_shared_from_this<HierarchicalSourceAdapter>
{
public:
    // UI-thread-affine: construct on a UI thread with a DispatcherQueue. Source notifications are
    // required to arrive on this thread and are applied synchronously; the queue is captured only
    // to assert that affinity in chk. Constructing off a UI thread throws RPC_E_WRONG_THREAD.
    //
    // The constructor also flips the expansion baseline to COLLAPSED, and that single line is what
    // makes the visible-only walk real. RowExpansionModel defaults to EXPANDED and stores intent
    // only where it DIFFERS from the default -- correct for grouping, where a grid of collapsed
    // headers would hide all data on first paint. Taken as-is here it is fatal: IsExpanded() would
    // answer true for every path nobody has touched, so the walk would descend into everything and
    // degenerate to a full O(tree) realization on the very first Rebuild.
    HierarchicalSourceAdapter();
    ~HierarchicalSourceAdapter();

    // The flat row axis the repeater consumes: a single ItemsSourceView wrapping the materialized
    // observable vector. Created ONCE over m_entries (whose object identity is stable for the
    // adapter's lifetime -- Rebuild ReplaceAll's its contents and a single-node toggle splices a
    // contiguous run in place, but the vector instance never changes), so an identity a consumer
    // captures stays valid across rebuilds. Same contract, and the same container-preservation
    // cost, as the grouped adapter: the wrapped object is a plain IObservableVector, so the
    // repeater reaches it through InspectingDataSource, which reports no key mapping. Containers
    // below a splice point are therefore re-prepared even though the splice itself is surgical.
    winrt::ItemsSourceView Entries() const { return m_entriesView; }

    // The shaped ROOT sibling set, in order. Roots only -- every deeper level is reached through
    // ChildrenSelector as the walk descends, because a level's shape is not knowable until the
    // walk gets there.
    winrt::IInspectable Source() const { return m_source; }
    void Source(winrt::IInspectable const& roots);
    void DetachSourceQuietly();

    // The hierarchy contract (HierarchyContract.h). ChildrenSelector is required for the adapter
    // to produce anything but a flat list of roots; the other two are optional.
    void ChildrenSelector(ShapingHelpers::ChildrenFn fn);
    void HasChildrenSelector(ShapingHelpers::HasChildrenFn fn);
    void ShapeSiblings(ShapingHelpers::ShapeSiblingsFn fn);

    // Per-node expand / collapse intent, addressed by PATH key.
    //
    // A path key rather than the node's own object identity, for two reasons that are not the same
    // reason. The weaker one: the same object can sit under two parents, and a flat object key
    // could not tell the two rows apart. The load-bearing one: intent has to survive a re-sort or
    // re-filter at EVERY level, which is exactly the regression the identity-string key fixed for
    // groups, lifted to N levels. Shaping re-mints structure; intent must outlive it.
    bool IsNodeExpanded(winrt::hstring const& pathKey) const;
    void SetNodeExpanded(winrt::hstring const& pathKey, bool isExpanded);

    // Bulk verbs. These move the model's BASELINE rather than looping over live nodes, so a node
    // that does not exist yet still inherits the intent -- and so they never splice. One Reset,
    // matching the grouped adapter.
    //
    // ExpandAll is the single most expensive call on this type: it makes every node the children
    // selector can reach visible, bounded only by the cycle and depth guards below. On a lazy tree
    // it forces realization of the whole thing. That is inherent to "expand everything" over a
    // materialized axis and is the same exposure ExpandAllGroups already has.
    void ExpandAll();
    void CollapseAll();

    // Guards on the walk, because ChildrenSelector is app code over an arbitrary object graph and
    // neither Emit's recursion nor the graph's finiteness is something this type may assume.
    // Both throw E_INVALIDARG with a caller-facing diagnostic rather than silently truncating: a
    // truncated tree is a data bug the app never learns about.
    static constexpr int32_t c_maxHierarchyDepth = 256;

    // Index -> descriptor, for the row-metadata provider.
    //
    // Depth cannot ride on the row value -- that would reintroduce the per-row wrapper this stack
    // just got rid of -- and cannot be recomputed from the item, since the same object may appear
    // at two depths. A POD side-vector is the only option that keeps both invariants, and it costs
    // a few words per VISIBLE row with zero COM allocations.
    struct NodeRow
    {
        winrt::IInspectable Item{ nullptr };
        winrt::hstring PathKey;

        // 0-BASED. Roots are 0, because indent is Depth * RowIndentSize and a root must render
        // flush. Note TableViewRowInfo::Level is 1-based (grouped data rows already report 1, and
        // UIA's AutomationProperties.Level is 1-based by definition), so RowMetadataProvider emits
        // Depth + 1. The conversion happens there, once.
        int32_t Depth{ 0 };

        // Resolved children count; -1 means unrealized/unknown, which is the lazy case: a
        // HasChildrenSelector said "expandable" without anyone enumerating children, so the row
        // presents a chevron and the count is not yet knowable.
        int32_t ChildCount{ -1 };

        bool HasChildren{ false };
        bool IsExpanded{ false };
    };

    NodeRow const* TryGetNodeRow(int32_t index) const;
    bool TryGetIndexForPathKey(winrt::hstring const& pathKey, int32_t& index) const;

    // Descriptor for a visible row addressed by its ITEM rather than its index. Needed when the
    // hierarchy is composed under grouping: the presented row axis is then the grouped adapter's,
    // whose indices interleave header rows and so do not line up with this adapter's. The item is
    // the only thing both axes agree on.
    //
    // Sound because a given object occupies at most one visible row: layer 2 rejects a source in
    // which one object appears twice, and the walk rejects duplicate siblings, so item -> row is a
    // function. Returns null for an item that is not currently visible (collapsed away, filtered
    // out, or simply not part of this tree).
    NodeRow const* TryGetNodeRowForItem(winrt::IInspectable const& item) const;

private:
    // One expanded node's subscription to its own children collection. Declared here rather than
    // beside m_nodeSubscriptions because the walk takes a vector of these by reference -- see Emit.
    struct NodeSubscription
    {
        winrt::hstring PathKey;
        winrt::IInspectable ItemsForRevocation{ nullptr };
        winrt::event_token CollectionToken{};
        winrt::event_token Token{};
        winrt::event_token BindableToken{};
    };

    void Rebuild();
    bool OnUiThread() const;
    void AssertRebuildOnUiThread() const;
    void OnExpansionChanged(ShapingHelpers::RowExpansionModel::Change const& change);

    // One walk step: append `item`'s row, then recurse into its children if and only if it is
    // expanded. Carries `ancestorIds` -- the object identities on the current root-to-node path --
    // purely to detect a cycle. That check is PATH-local by construction: the same object under
    // two different parents is not a cycle and stays legal, while an item that is its own ancestor
    // is rejected.
    //
    // `enumeratedPrefixes` accumulates the path keys whose child sets this walk saw IN FULL. It
    // exists only to make pruning safe -- see PruneExpansionIntent.
    //
    // Every output, INCLUDING the node subscriptions, is a caller-owned local. Emit can throw
    // partway through (cycle, over-deep chain, a duplicate path key), and nothing it has produced
    // may have touched published state by then: the caller discards the locals, revokes the
    // subscriptions it accumulated, and the previously published projection is still intact and
    // self-consistent. Subscribing straight into m_nodeSubscriptions would leave live handlers for
    // rows that were never published.
    void Emit(
        winrt::IInspectable const& item,
        int32_t depth,
        winrt::hstring const& parentPath,
        std::unordered_set<winrt::hstring>& ancestorIds,
        std::vector<winrt::IInspectable>& built,
        std::vector<NodeRow>& descriptors,
        std::unordered_set<winrt::hstring>& liveKeys,
        std::unordered_set<winrt::hstring>& enumeratedPrefixes,
        std::vector<NodeSubscription>& subscriptions);

    // Drop intent for nodes that no longer exist -- but NOT by retaining only the visible keys.
    //
    // That is the obvious implementation and it is a bug. The walk deliberately does not descend
    // into collapsed nodes, so a collapsed node's descendants produce no live keys at all; pruning
    // against the visible set would delete the user's intent for every subtree hidden behind a
    // collapse. Observable symptom: collapse a grandchild, collapse its parent, re-expand the
    // parent -- and the grandchild comes back EXPANDED, because its intent was pruned while it was
    // invisible.
    //
    // Grouping has no such hazard, which is exactly why its rule cannot be copied: every group
    // emits a header whether collapsed or not, so its live-key set is genuinely complete.
    //
    // So: prune a key only when a STRICT ANCESTOR of it was walked and did not produce it. Keys
    // under a prefix this pass never enumerated are left alone; they are pruned the next time that
    // parent is expanded and walked. Bounded, and non-destructive.
    void PruneExpansionIntent(
        std::unordered_set<winrt::hstring> const& liveKeys,
        std::unordered_set<winrt::hstring> const& enumeratedPrefixes);

    // Incremental expand/collapse of a SINGLE node: splices just that node's visible subtree
    // into/out of the projection instead of dropping and re-realizing every container via a full
    // Rebuild + Reset. The direct analogue of TryApplyExpansionSplice.
    //
    // The run is contiguous by construction: a node's visible descendants are exactly the rows
    // after it whose Depth is greater than its own, up to the first row that is not. Collapse
    // walks forward to find that run and removes it; expand emits the subtree into a temp vector
    // (honouring nested intent, and carrying the same cycle/depth guards) and inserts it.
    //
    // Returns false -- and the caller falls back to the authoritative Rebuild -- when the path key
    // does not resolve, the descriptor/entry counts disagree, or a rebuild is already in flight.
    // Bulk ExpandAll/CollapseAll and baseline moves never take this path.
    bool TryApplyExpansionSplice(winrt::hstring const& pathKey, bool expand);

    void AttachToSource();
    void DetachFromSource();

    // Subscriptions are keyed by the owning node's path so a collapse can revoke exactly the run it
    // removes. Rebuild drops them wholesale; a splice cannot, and an orphaned subscription would
    // both pin its collection alive and force a pointless Rebuild on any change inside a subtree
    // the user has collapsed.
    //
    // `into` is the caller's accumulator, never m_nodeSubscriptions directly, so a walk that throws
    // leaves no live handler behind. See Emit.
    void SubscribeToNode(winrt::hstring const& pathKey, winrt::IInspectable const& childrenCollection, std::vector<NodeSubscription>& into);
    void UnsubscribeUnder(winrt::hstring const& pathKey);
    void UnsubscribeFromAllNodes();
    static void RevokeAll(std::vector<NodeSubscription>& subscriptions);

    // ShapeSiblings is app/layer-2 code that runs INSIDE the walk -- a hazard the grouped adapter
    // does not have, because a group's members arrive already shaped. The contract is that it must
    // not call back into the adapter; this wrapper makes the contract checkable instead of merely
    // documented, by publishing "a callback is on the stack" for the mutators to assert on.
    void InvokeShapeSiblings(std::vector<winrt::IInspectable>& siblings);
    void AssertNotInShapeSiblings() const;

    // The three structures below are only correct AS A SET, the same way RowIdentity's identity
    // set and index map are. Every mutation goes through these two helpers rather than touching
    // the vectors directly, because "mutate all three in the same statement block" is a rule that
    // three call sites will eventually break.
    void InsertRows(int32_t index, std::vector<winrt::IInspectable> const& items, std::vector<NodeRow> const& descriptors);
    void RemoveRows(int32_t index, int32_t count);

    // pathKey = "node:" + "/" + id(root) + "/" + ... + "/" + id(node), where id(x) is the canonical
    // IUnknown pointer -- the same primitive flat and grouped rows already use. A root's parent path
    // is the bare prefix "node:", which doubles as the sentinel for "the root sibling set" in
    // enumeratedPrefixes.
    static winrt::hstring ObjectIdentity(winrt::IInspectable const& item);
    static winrt::hstring MakePathKey(winrt::hstring const& parentPath, winrt::IInspectable const& item);
    static winrt::hstring ParentPathOf(winrt::hstring const& pathKey);

    winrt::IInspectable m_source{ nullptr };

    ShapingHelpers::ChildrenFn m_childrenSelector{ nullptr };
    ShapingHelpers::HasChildrenFn m_hasChildrenSelector{ nullptr };
    ShapingHelpers::ShapeSiblingsFn m_shapeSiblings{ nullptr };

    // THE flat projection: the visible rows, as raw app items. Materialized in full on every
    // Rebuild via a single ReplaceAll; a single-node toggle splices one contiguous run in place.
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> m_entries{
        winrt::single_threaded_observable_vector<winrt::IInspectable>() };

    // One ItemsSourceView over m_entries, created once (m_entries identity is stable for life).
    winrt::ItemsSourceView m_entriesView{ nullptr };

    // Row metadata, index-aligned 1:1 with m_entries.
    std::vector<NodeRow> m_descriptors;

    // Path key -> index, so the metadata provider resolves an identity by hash rather than by an
    // O(n) ABI scan over the live vector.
    //
    // Cost contract, stated because it is not free: the LOOKUP is O(1), but keeping the map exact
    // is not. A splice at index i shifts every index after the run, which is an O(visible rows)
    // sweep -- the identical cost, for the identical reason, that RowIdentity documents for
    // ShiftTrackedFlatRowIndicesForInsert/ForRemove ("a hash map keyed by identity has no way to
    // reach every entry at or after this position without visiting all of them"). Two mitigations
    // carry over verbatim: a splice at the TAIL skips the sweep entirely, and the indices must be
    // exact rather than approximate, so any detected desync falls back to Rebuild instead of being
    // repaired lazily.
    std::unordered_map<winrt::hstring, int32_t> m_indexByPathKey;

    // Expand/collapse intent, keyed by path so it survives reshapes at every level. Baseline is
    // flipped to collapsed in the constructor; see the note there.
    ShapingHelpers::RowExpansionModel m_expansion;

    // Affinity assertion only (chk); the adapter never marshals. Teardown runs on the owning UI
    // thread because every strong owner (TableViewSource / TableView) is a ReferenceTracker whose
    // final_release marshals destruction there, so no revoke thread guard is needed.
    winrt::weak_ref<winrt::Microsoft::UI::Dispatching::DispatcherQueue> m_uiQueue{ nullptr };

    winrt::IInspectable m_attachedSourceForRevocation{ nullptr };
    winrt::event_token m_outerCollectionChangedToken{};
    winrt::event_token m_outerVectorChangedToken{};
    winrt::event_token m_outerBindableVectorChangedToken{};

    // One subscription per EXPANDED, VISIBLE node's children collection, dropped wholesale at the
    // top of Rebuild. Subscription count is therefore bounded by visible rows, not by tree size --
    // a recursive per-node subscription over the whole tree would both leak and scale with the
    // tree rather than the viewport.
    std::vector<NodeSubscription> m_nodeSubscriptions;

    static void RevokeSubscription(NodeSubscription& sub);

    // Simple re-entrancy guard: a change notification raised synchronously inside ReplaceAll, or a
    // ShapeSiblings implementation that violates its contract, must not start a nested Rebuild.
    // The re-entrant request is remembered and run once after unwind. Plain bool, not atomic: the
    // adapter is UI-thread-affine and every mutation asserts that.
    bool m_rebuildInFlight{ false };
    bool m_pendingRebuild{ false };

    // True while an app-supplied ShapeSiblings callback is on the stack. Exists so the mutators can
    // assert the callback's no-reentrancy contract rather than trusting it: a violation is a bug in
    // app or layer-2 code that is otherwise invisible until it manifests as a redundant rebuild.
    bool m_inShapeSiblings{ false };
};

using HierarchicalSourceAdapterPtr = std::shared_ptr<HierarchicalSourceAdapter>;
