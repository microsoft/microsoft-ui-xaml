// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "HierarchicalSourceAdapter.h"
#include "RuntimeProfiler.h"
#include "ShapingHelpers.h"
#include "ShapingRevoke.h"

#include <string_view>
#include <utility>

#include <winrt/Microsoft.UI.Xaml.Interop.h>
#include <winrt/Microsoft.UI.Dispatching.h>

namespace
{
    // The root sibling set's parent path. Doubles as the prefix sentinel meaning "the walk
    // enumerated the roots in full", so root-level intent is prunable on exactly the same rule as
    // every deeper level rather than being a special case.
    constexpr std::wstring_view c_nodeKeyPrefix{ L"node:" };
}

HierarchicalSourceAdapter::HierarchicalSourceAdapter()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_HierarchicalSourceAdapter);

    auto queue = winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread();
    if (!queue)
    {
        throw winrt::hresult_error(RPC_E_WRONG_THREAD, L"HierarchicalSourceAdapter must be constructed on a UI thread.");
    }
    m_uiQueue = winrt::make_weak(queue);

    // One view over the stable entries vector, handed to every consumer.
    m_entriesView = winrt::ItemsSourceView{ m_entries };

    // THE load-bearing line of this constructor. RowExpansionModel defaults to EXPANDED and stores
    // intent only where it differs from that default, which is right for grouping (a grid of
    // collapsed headers would hide all data on first paint) and fatal here: IsExpanded() would
    // answer true for every path nobody has touched, so Emit would descend into everything and the
    // visible-only walk would degenerate into a full O(tree) realization on the very first
    // Rebuild. Set before the changed handler is attached, so establishing the baseline does not
    // raise a Change into a half-constructed adapter.
    m_expansion.SetDefaultExpanded(false);

    m_expansion.SetChangedHandler(
        [this](ShapingHelpers::RowExpansionModel::Change const& change)
        {
            OnExpansionChanged(change);
        });
}

HierarchicalSourceAdapter::~HierarchicalSourceAdapter()
{
    DetachFromSource();
}

void HierarchicalSourceAdapter::Source(winrt::IInspectable const& roots)
{
    AssertNotInShapeSiblings();

    if (m_source == roots)
    {
        return;
    }

    DetachFromSource();
    m_source = roots;
    AttachToSource();
    Rebuild();
}

void HierarchicalSourceAdapter::DetachSourceQuietly()
{
    DetachFromSource();
    m_source = nullptr;
}

void HierarchicalSourceAdapter::ChildrenSelector(ShapingHelpers::ChildrenFn fn)
{
    AssertNotInShapeSiblings();

    m_childrenSelector = std::move(fn);
    Rebuild();
}

void HierarchicalSourceAdapter::HasChildrenSelector(ShapingHelpers::HasChildrenFn fn)
{
    AssertNotInShapeSiblings();

    m_hasChildrenSelector = std::move(fn);
    Rebuild();
}

void HierarchicalSourceAdapter::ShapeSiblings(ShapingHelpers::ShapeSiblingsFn fn)
{
    // Reassigning from inside the callback would destroy the std::function currently executing.
    AssertNotInShapeSiblings();

    m_shapeSiblings = std::move(fn);
    Rebuild();
}

void HierarchicalSourceAdapter::ReleaseCallbacks()
{
    // Same reason the setters assert: clearing m_shapeSiblings from inside itself would destroy
    // the std::function currently executing.
    AssertNotInShapeSiblings();

    m_childrenSelector = nullptr;
    m_hasChildrenSelector = nullptr;
    m_shapeSiblings = nullptr;

    // Deliberately no Rebuild: the caller has already detached the source and dropped the projection
    // callback, and the three setters' own rebuilds would publish three Resets over rows the
    // non-hierarchical projection is about to replace wholesale.
}

void HierarchicalSourceAdapter::ProjectionChanged(std::function<void()> fn){
    m_projectionChanged = std::move(fn);
}

void HierarchicalSourceAdapter::RaiseProjectionChanged(){
    // Copied before invoking: the handler is free to re-declare the hierarchy (which reassigns
    // m_projectionChanged) and must not destroy the std::function it is running inside.
    if (auto const handler = m_projectionChanged)
    {
        handler();
    }
}

// --- Path identity ------------------------------------------------------------------------------
winrt::hstring HierarchicalSourceAdapter::ObjectIdentity(winrt::IInspectable const& item)
{
    // COM identity: the IUnknown obtained by QI is the canonical per-object pointer, so two
    // references to the same object always stringify identically and two distinct objects never
    // collide -- including two boxed copies of the same value, which are separate objects and
    // therefore separate rows.
    //
    // Deliberately NOT ShapingHelpers::ValueKey::ToObjectLookupKey, which prefers a VALUE key when
    // the item has one. Two value-equal siblings (two boxed "Documents" strings under one parent)
    // would then mint the same path key, collapsing two real rows onto one index-map slot. Value
    // identity is right for a group KEY, which names a bucket; it is wrong for a node, which names
    // a row.
    //
    // This duplicates the primitive in RowIdentity::MakeObjectIdentitySelector rather than calling
    // it, because RowIdentity belongs to layer 2 and this adapter is layer 3; the two are siblings,
    // not a stack (see HierarchyContract.h). If a third consumer ever needs it, lift it into
    // ShapingHelpers (layer 1) rather than crossing the layers here.
    uintptr_t address = 0;
    if (item)
    {
        auto const unknown = item.as<winrt::Windows::Foundation::IUnknown>();
        address = reinterpret_cast<uintptr_t>(winrt::get_abi(unknown));
    }

    wchar_t buffer[2 + (sizeof(uintptr_t) * 2) + 1]{};
    swprintf_s(buffer, L"0x%zx", static_cast<size_t>(address));
    return winrt::hstring{ buffer };
}

winrt::hstring HierarchicalSourceAdapter::MakePathKey(winrt::hstring const& parentPath, winrt::hstring const& itemIdentity)
{
    return parentPath + L"/" + itemIdentity;
}

bool HierarchicalSourceAdapter::IsNodePathKey(winrt::hstring const& key)
{
    const std::wstring_view view{ key.c_str(), key.size() };

    // "node:" alone is the ROOT SIBLING SET's parent path, never a row, so a real node key is
    // strictly longer and carries at least one separator.
    return view.size() > c_nodeKeyPrefix.size() + 1 &&
        view.compare(0, c_nodeKeyPrefix.size(), c_nodeKeyPrefix) == 0 &&
        view[c_nodeKeyPrefix.size()] == L'/';
}

winrt::hstring HierarchicalSourceAdapter::ParentPathOf(winrt::hstring const& pathKey)
{
    const std::wstring_view view{ pathKey.c_str(), pathKey.size() };
    const auto lastSeparator = view.find_last_of(L'/');
    if (lastSeparator == std::wstring_view::npos)
    {
        // Not a path this adapter minted. Returning the key unchanged cannot match any enumerated
        // prefix, so the prune leaves it alone -- the non-destructive answer for a key we do not
        // recognise.
        return pathKey;
    }

    return winrt::hstring{ view.substr(0, lastSeparator) };
}

// --- The one responsibility: materialize the visible rows ----------------------------------------

void HierarchicalSourceAdapter::Rebuild()
{
    // Projection mutations are UI-thread-affine. Source notifications are required to arrive on the
    // owning UI thread and reach here synchronously; off-thread delivery is app misuse.
    AssertRebuildOnUiThread();

    if (m_rebuildInFlight)
    {
        // A change notification raised synchronously inside ReplaceAll, or a ShapeSiblings callback
        // that violated its contract, re-entered Rebuild. Don't drop it (that would leave m_entries
        // stale) and don't nest it -- remember it and run one coalesced follow-up after unwind.
        m_pendingRebuild = true;
        return;
    }

    bool runPending = false;
    do
    {
        m_rebuildInFlight = true;
        m_pendingRebuild = false;
        auto resetGuard = wil::scope_exit([this, &runPending]() noexcept
        {
            m_rebuildInFlight = false;
            runPending = m_pendingRebuild;
            m_pendingRebuild = false;
        });

        // Node subscriptions are accumulated into a LOCAL and swapped in only on success. The old
        // ones stay live until then, so a walk that throws -- a cycle, an over-deep chain, a
        // duplicate path key -- leaves the previously published projection and its subscriptions
        // exactly as they were, rather than half-torn-down. Unsubscribing first and subscribing as
        // we walk would strand the old handlers and orphan the new ones on the way out.
        std::vector<winrt::IInspectable> built;
        std::vector<NodeRow> descriptors;
        std::unordered_set<winrt::hstring> liveKeys;
        std::unordered_set<winrt::hstring> enumeratedPrefixes;
        std::vector<NodeSubscription> subscriptions;

        auto walkGuard = wil::scope_exit([&subscriptions]() noexcept
        {
            // Runs on the throwing path only -- released below once the walk has succeeded.
            RevokeAll(subscriptions);
        });

        // The roots are always walked in full, so root-level intent is always prunable.
        enumeratedPrefixes.insert(winrt::hstring{ c_nodeKeyPrefix });

        const auto source = Source();
        if (source)
        {
            auto roots = ShapingHelpers::EnumerateInspectableItems(source);

            // The root sibling set is shaped by layer 2 before it reaches us (it is handed in as
            // Source), so it is NOT re-shaped here. Deeper levels are shaped as the walk reaches
            // them, because their membership is not knowable until then.
            std::unordered_set<winrt::hstring> ancestorIds;
            for (auto const& root : roots)
            {
                if (!root)
                {
                    continue;
                }

                Emit(root, 0, winrt::hstring{ c_nodeKeyPrefix }, ancestorIds, built, descriptors, liveKeys, enumeratedPrefixes, subscriptions);
            }
        }

        // Past this point the walk cannot throw, so the new subscriptions are keepers and the old
        // ones are finally dead.
        walkGuard.release();
        UnsubscribeFromAllNodes();
        m_nodeSubscriptions = std::move(subscriptions);

        // The index map is built into a LOCAL. Both the reserve and the inserts allocate, and
        // filling m_indexByPathKey in place means a failure part-way leaves a published map that
        // describes only some of the published descriptors -- worse than either the old or the new
        // state, because RowMetadataProvider would resolve some identities and silently miss others.
        std::unordered_map<winrt::hstring, int32_t> indexByPathKey;
        indexByPathKey.reserve(descriptors.size());
        for (size_t i = 0; i < descriptors.size(); ++i)
        {
            indexByPathKey.emplace(descriptors[i].PathKey, static_cast<int32_t>(i));
        }

        // Publish the descriptor side-table and the index map BEFORE the Reset, so a consumer that
        // reacts synchronously to the collection change sees metadata that already agrees with the
        // rows it is being told about. Both hand-offs are moves and cannot throw, so the pair goes
        // live together or not at all.
        auto previousDescriptors = std::exchange(m_descriptors, std::move(descriptors));
        auto previousIndex = std::exchange(m_indexByPathKey, std::move(indexByPathKey));
        m_indexByItemValid = false;

        // ReplaceAll has two failure modes that want OPPOSITE answers, and the exception cannot tell
        // them apart. It assigns the container first and raises the single Reset afterwards, so:
        //   - a throw from the assignment leaves the OLD rows published, and the new metadata has to
        //     be rolled back or it describes rows nobody can see;
        //   - a throw out of a consumer's Reset handler happens once the new rows are already live,
        //     and rolling back there would be the actual corruption -- old metadata over new rows.
        // So the guard decides on the published row count instead. The count is conclusive: an
        // assignment that fits in the existing capacity cannot allocate and so cannot throw, meaning
        // a failed assignment always implies a growth that did not happen, and the old count must
        // therefore differ from the one we were trying to publish.
        const auto publishedCount = static_cast<uint32_t>(built.size());
        auto publishGuard = wil::scope_exit([this, publishedCount, &previousDescriptors, &previousIndex]() noexcept
        {
            if (m_entries.Size() == publishedCount)
            {
                // The rows went in; only a handler objected. The new pair is the coherent one.
                return;
            }

            m_descriptors = std::move(previousDescriptors);
            m_indexByPathKey = std::move(previousIndex);
            m_indexByItemValid = false;
        });

        // One Reset for the whole projection.
        m_entries.ReplaceAll(built);
        publishGuard.release();

        // Pruned only once the new projection is actually live. Pruning first would drop intent for
        // keys the rollback path then brings back, so a node that stayed visible would quietly
        // resolve to the baseline expansion rather than to what the user chose. Nothing observes
        // intent during the Reset, so deferring it costs no coherence.
        PruneExpansionIntent(liveKeys, enumeratedPrefixes);

        // resetGuard runs here, publishing into runPending whether a re-entrant request arrived.
    } while (runPending);

    // One coherent edge for the whole rebuild, after the last iteration has published.
    RaiseProjectionChanged();
}

void HierarchicalSourceAdapter::Emit(
    winrt::IInspectable const& item,
    int32_t depth,
    winrt::hstring const& parentPath,
    std::unordered_set<winrt::hstring>& ancestorIds,
    std::vector<winrt::IInspectable>& built,
    std::vector<NodeRow>& descriptors,
    std::unordered_set<winrt::hstring>& liveKeys,
    std::unordered_set<winrt::hstring>& enumeratedPrefixes,
    std::vector<NodeSubscription>& subscriptions)
{
    if (depth >= c_maxHierarchyDepth)
    {
        // Emit recurses, so an adversarially deep (but acyclic) chain would overflow the stack
        // before any allocator complained. Throw rather than truncate: a silently truncated tree is
        // a data bug the app never learns about.
        throw winrt::hresult_invalid_argument(
            L"The hierarchy is deeper than the supported maximum (256 levels). Check the "
            L"WithChildren(...) children selector for an unintentionally deep or self-extending "
            L"structure.");
    }

    // One identity per node, shared by the path key and the cycle check below. Both need it and it
    // costs a QI plus a string; MakePathKey takes it rather than recomputing it from the item.
    const auto selfId = ObjectIdentity(item);
    const auto path = MakePathKey(parentPath, selfId);

    // Cycle detection is path-local by construction: the same object under two different parents is
    // not a cycle and stays legal, while an item that is its own ancestor is rejected.
    if (ancestorIds.find(selfId) != ancestorIds.end())
    {
        throw winrt::hresult_invalid_argument(
            L"The hierarchy contains a cycle: an item appears as its own ancestor. Check the "
            L"WithChildren(...) children selector -- a node must not reach itself through its own "
            L"descendants.");
    }

    // Whether the node is expandable. With a HasChildrenSelector this costs one app call and no
    // enumeration, which is the entire point of the overload: it is what keeps a collapsed subtree
    // genuinely unrealized. Without one, the only way to answer is to enumerate, which realizes one
    // level below every visible row.
    bool hasChildren = false;
    int32_t childCount = -1;

    // The raw collection object AND its enumerated contents. Both are retained because they are
    // needed for different things and must refer to the SAME call: the contents drive the walk, and
    // the collection object is what a subscription has to observe. Calling the selector a second
    // time to get one of them could hand back a different object -- an app that materializes
    // children on demand commonly returns a fresh collection per call -- leaving the adapter
    // subscribed to a collection nobody is showing.
    winrt::IInspectable childrenCollection{ nullptr };
    std::vector<winrt::IInspectable> children;
    bool childrenRealized = false;

    const auto realizeChildren = [&]()
    {
        if (childrenRealized || !m_childrenSelector)
        {
            return;
        }
        childrenRealized = true;
        try
        {
            childrenCollection = m_childrenSelector(item);
            children = ShapingHelpers::EnumerateInspectableItems(childrenCollection);
        }
        catch (...)
        {
            // App code. A throwing selector means "no children we can see", not "fail the whole
            // projection" -- the same guard RebuildGrouped puts around m_groupSelector.
            childrenCollection = nullptr;
            children.clear();
        }

        // Subscribed as soon as the collection exists, NOT only once the node is expanded. Without a
        // HasChildrenSelector a COLLAPSED node still realizes its children here, purely to answer
        // expandability. Leaving that collection unobserved means an app removing the last child
        // while the parent stays collapsed never reaches the adapter, and the row keeps a chevron
        // that opens onto nothing. The collection was realized either way, so this costs a
        // subscription, not a materialization.
        SubscribeToNode(path, childrenCollection, subscriptions);
    };

    if (m_hasChildrenSelector)
    {
        try
        {
            hasChildren = m_hasChildrenSelector(item);
        }
        catch (...)
        {
            hasChildren = false;
        }
        // ChildCount stays -1: expandable, but nobody has enumerated, so the count is genuinely
        // unknown. That is the lazy contract, not a missing value.
    }
    else if (m_childrenSelector)
    {
        realizeChildren();
        childCount = static_cast<int32_t>(children.size());
        hasChildren = childCount > 0;
    }

    const bool isExpanded = hasChildren && m_expansion.IsExpanded(path);

    NodeRow row;
    row.Item = item;
    row.PathKey = path;
    row.Depth = depth;
    row.ChildCount = childCount;
    row.HasChildren = hasChildren;
    row.IsExpanded = isExpanded;

    built.push_back(item);
    descriptors.push_back(std::move(row));

    // A path key must be unique, and a collision is never benign: m_indexByPathKey would map one
    // key to one of two rows, so metadata lookups and expansion toggles would silently address the
    // wrong row. It means the same object appears twice in one sibling set, which is the same
    // duplicate-identity bug ShapedItemsSource already rejects outright -- so reject it here too,
    // at the level that can name the offending position.
    if (!liveKeys.insert(path).second)
    {
        throw winrt::hresult_invalid_argument(
            L"The hierarchy contains the same item twice under one parent. Row identity is per "
            L"object, so duplicate siblings cannot be told apart. Check the WithChildren(...) "
            L"children selector -- each item must appear at most once in a given child set.");
    }

    if (!isExpanded)
    {
        return;
    }

    // Expanded: now the children must be realized whether or not HasChildrenSelector spared us
    // earlier, and shaped as their own sibling set.
    realizeChildren();
    InvokeShapeSiblings(children);

    // The count is only knowable once the set is realized AND shaped -- a filter may have removed
    // some -- so it is written back here rather than at push time.
    descriptors.back().ChildCount = static_cast<int32_t>(children.size());

    if (children.empty())
    {
        // Expandability was promised (by a HasChildrenSelector, or by a child set that has since
        // emptied) but nothing survives to show: the selector returned none, or ShapeSiblings
        // filtered them all away. Publishing HasChildren here would draw a chevron on a row with
        // nothing under it, and IsExpanded would mark it open onto an empty run. It is a leaf for
        // this publish. The recorded intent is deliberately left alone, so the node re-expands by
        // itself if children come back.
        descriptors.back().HasChildren = false;
        descriptors.back().IsExpanded = false;
        return;
    }

    // This child set was seen in full, so intent stored beneath it is safe to prune.
    enumeratedPrefixes.insert(path);

    ancestorIds.insert(selfId);
    for (auto const& child : children)
    {
        if (!child)
        {
            continue;
        }

        Emit(child, depth + 1, path, ancestorIds, built, descriptors, liveKeys, enumeratedPrefixes, subscriptions);
    }
    ancestorIds.erase(selfId);
}

void HierarchicalSourceAdapter::PruneExpansionIntent(
    std::unordered_set<winrt::hstring> const& liveKeys,
    std::unordered_set<winrt::hstring> const& enumeratedPrefixes)
{
    m_expansion.RetainOnlyUnder(liveKeys, enumeratedPrefixes, &HierarchicalSourceAdapter::ParentPathOf);
}

// --- Expansion -----------------------------------------------------------------------------------

bool HierarchicalSourceAdapter::IsNodeExpanded(winrt::hstring const& pathKey) const
{
    return m_expansion.IsExpanded(pathKey);
}

void HierarchicalSourceAdapter::SetNodeExpanded(winrt::hstring const& pathKey, bool isExpanded)
{
    AssertNotInShapeSiblings();

    if (pathKey.empty())
    {
        return;
    }

    if (m_expansion.IsExpanded(pathKey) == isExpanded)
    {
        // Intent already agrees. Normally that means the projection does too -- but a previous
        // materialization can have failed after the intent was committed (a cycle or an over-deep
        // chain throws out of the walk), which would otherwise trap the node: every retry would
        // early-return here and the user could never expand it again. If the node is materialized
        // and disagrees with the intent, reconcile instead of returning.
        int32_t index = -1;
        if (TryGetIndexForPathKey(pathKey, index))
        {
            if (auto const* row = TryGetNodeRow(index); row && row->HasChildren && row->IsExpanded != isExpanded)
            {
                Rebuild();
            }
        }
        return;
    }

    // Through the model, not around it, so a programmatic ExpandAll and a per-node toggle share a
    // path. The model raises Changed -> OnExpansionChanged -> splice or Rebuild.
    m_expansion.SetExpanded(pathKey, isExpanded);
}

void HierarchicalSourceAdapter::ExpandAll()
{
    AssertNotInShapeSiblings();

    // Moves the BASELINE, so nodes that do not exist yet also arrive expanded. On a large or lazy
    // tree this realizes everything the children selector can reach, bounded only by the cycle and
    // depth guards -- the inherent cost of "expand everything" over a materialized axis.
    m_expansion.SetAllExpanded(true);
}

void HierarchicalSourceAdapter::CollapseAll()
{
    AssertNotInShapeSiblings();

    m_expansion.SetAllExpanded(false);
}

void HierarchicalSourceAdapter::OnExpansionChanged(ShapingHelpers::RowExpansionModel::Change const& change)
{
    AssertRebuildOnUiThread();

    if (m_rebuildInFlight)
    {
        // A change raised synchronously while a Rebuild/splice is unwinding. m_expansion already
        // reflects the new intent, so remember it and let the outer operation run one coalesced
        // Rebuild afterward rather than nesting.
        m_pendingRebuild = true;
        return;
    }

    // Only a single-node toggle can be a ranged splice. A baseline move (ExpandAll/CollapseAll) or
    // a multi-key batch shifts the whole visible-row set, for which a full Rebuild ending in one
    // Reset is both correct and cheapest.
    if (change.AffectsAllKeys || change.Keys.size() != 1)
    {
        Rebuild();
        return;
    }

    bool runPending = false;
    bool applied = false;
    {
        m_rebuildInFlight = true;
        m_pendingRebuild = false;
        auto resetGuard = wil::scope_exit([this, &runPending]() noexcept
        {
            m_rebuildInFlight = false;
            runPending = m_pendingRebuild;
            m_pendingRebuild = false;
        });

        applied = TryApplyExpansionSplice(change.Keys.front(), change.IsExpanded);
    }

    // A re-entrant request, or a splice that couldn't resolve the node, resolves to a single
    // authoritative Rebuild after the guard unwinds.
    if (runPending || !applied)
    {
        Rebuild();
    }
}

bool HierarchicalSourceAdapter::TryApplyExpansionSplice(winrt::hstring const& pathKey, bool expand)
{
    // The two structures must already agree, or a ranged splice computed from one and applied to
    // both would silently corrupt the pairing. Rebuild is authoritative.
    if (m_descriptors.size() != static_cast<size_t>(m_entries.Size()))
    {
        return false;
    }

    int32_t index = -1;
    if (!TryGetIndexForPathKey(pathKey, index) || index < 0 || static_cast<size_t>(index) >= m_descriptors.size())
    {
        // The node isn't materialized in the projection (it is behind a collapsed ancestor, or the
        // projection is mid-reshape); let Rebuild reconcile from source.
        return false;
    }

    auto& node = m_descriptors[static_cast<size_t>(index)];
    if (node.IsExpanded == expand)
    {
        return true;  // Already coherent for this node.
    }

    if (!node.HasChildren)
    {
        // Intent was recorded for a leaf. Nothing to splice, and the projection is already right.
        return true;
    }

    const int32_t nodeDepth = node.Depth;

    if (!expand)
    {
        // Collapse: a node's visible descendants are exactly the contiguous run of rows after it
        // whose Depth exceeds its own, up to the first row that does not.
        int32_t runEnd = index + 1;
        while (static_cast<size_t>(runEnd) < m_descriptors.size() && m_descriptors[static_cast<size_t>(runEnd)].Depth > nodeDepth)
        {
            ++runEnd;
        }

        // Revoke the subscriptions belonging to the node and everything under it BEFORE the rows
        // go. Rebuild drops subscriptions wholesale, but a splice does not run Rebuild -- leaving
        // them would keep the adapter listening to collections it no longer shows, holding each one
        // alive through ItemsForRevocation and triggering a pointless full Rebuild on any change
        // inside a subtree the user just collapsed.
        UnsubscribeUnder(pathKey);

        // Descriptor first: RemoveRows raises the vector notification, and the node's own row must
        // already read "collapsed" by the time anyone reacts to it.
        m_descriptors[static_cast<size_t>(index)].IsExpanded = false;
        RemoveRows(index + 1, runEnd - (index + 1));
        RaiseProjectionChanged();
        return true;
    }

    // Expand: emit this node's subtree into temporaries, honouring nested intent, then insert the
    // run. Nothing the walk produces touches published state until the walk has completed, so a
    // throw out of Emit (cycle, over-deep chain, duplicate sibling) leaves the projection exactly
    // as it was. The splice then declines rather than propagating, and the caller's authoritative
    // Rebuild re-raises the same diagnostic from a single place -- one recovery policy instead of
    // two that must agree.
    if (!m_childrenSelector)
    {
        return false;
    }

    // One call, retaining both the collection object and its contents, for the same reason Emit
    // does: the subscription must observe the very collection that was enumerated.
    winrt::IInspectable childrenCollection{ nullptr };
    std::vector<winrt::IInspectable> children;
    try
    {
        childrenCollection = m_childrenSelector(node.Item);
        children = ShapingHelpers::EnumerateInspectableItems(childrenCollection);
    }
    catch (...)
    {
        return false;
    }

    InvokeShapeSiblings(children);

    if (children.empty())
    {
        // The node was marked expandable but has nothing left to show -- the last child was removed
        // from a collection that raised no notification, or a HasChildrenSelector promised children
        // the selector no longer returns. Splicing anyway would publish an expanded, empty, still
        // expandable row that only an unrelated rebuild could clear. Decline instead: the caller's
        // authoritative Rebuild re-walks the node, sees the empty set, republishes it as a leaf and
        // restamps the realized rows.
        return false;
    }

    std::vector<winrt::IInspectable> built;
    std::vector<NodeRow> descriptors;
    std::unordered_set<winrt::hstring> liveKeys;
    std::unordered_set<winrt::hstring> enumeratedPrefixes;
    std::vector<NodeSubscription> subscriptions;

    // The ancestor set for a splice starts from this node's own path rather than empty, so cycle
    // detection stays as strong as it is during a full walk.
    std::unordered_set<winrt::hstring> ancestorIds;
    {
        const std::wstring_view view{ pathKey.c_str(), pathKey.size() };
        auto cursor = view;
        while (true)
        {
            const auto lastSeparator = cursor.find_last_of(L'/');
            if (lastSeparator == std::wstring_view::npos)
            {
                break;
            }
            ancestorIds.insert(winrt::hstring{ cursor.substr(lastSeparator + 1) });
            cursor = cursor.substr(0, lastSeparator);
        }
    }

    try
    {
        for (auto const& child : children)
        {
            if (!child)
            {
                continue;
            }

            Emit(child, nodeDepth + 1, pathKey, ancestorIds, built, descriptors, liveKeys, enumeratedPrefixes, subscriptions);
        }
    }
    catch (...)
    {
        RevokeAll(subscriptions);
        return false;
    }

    // Emit's own uniqueness check only covers the run being spliced, because its live-key set is
    // local to this walk. A new key colliding with one ALREADY in the projection is the same
    // corruption from outside that window, so check against the published map too and hand the
    // decision to Rebuild, which sees the whole tree at once.
    for (auto const& descriptor : descriptors)
    {
        if (m_indexByPathKey.find(descriptor.PathKey) != m_indexByPathKey.end())
        {
            RevokeAll(subscriptions);
            return false;
        }
    }

    // The node's own descriptor is updated BEFORE the insertion, for the same reason the collapse
    // path does it: InsertRows raises the vector notification, and a consumer reacting to it must
    // see the parent already reading "expanded" with its resolved child count.
    {
        auto& refreshed = m_descriptors[static_cast<size_t>(index)];
        refreshed.IsExpanded = true;
        refreshed.ChildCount = static_cast<int32_t>(children.size());
    }

    InsertRows(index + 1, built, descriptors);

    // The run is in. Adopt the subscriptions the walk accumulated, including this node's own.
    SubscribeToNode(pathKey, childrenCollection, m_nodeSubscriptions);
    m_nodeSubscriptions.insert(
        m_nodeSubscriptions.end(),
        std::make_move_iterator(subscriptions.begin()),
        std::make_move_iterator(subscriptions.end()));
    RaiseProjectionChanged();
    return true;
}

// --- The triple invariant ------------------------------------------------------------------------

void HierarchicalSourceAdapter::InsertRows(
    int32_t index,
    std::vector<winrt::IInspectable> const& items,
    std::vector<NodeRow> const& descriptors)
{
    MUX_ASSERT(items.size() == descriptors.size());
    if (items.empty())
    {
        return;
    }

    const int32_t count = static_cast<int32_t>(items.size());

    // Metadata FIRST, rows last. m_entries.InsertAt raises CollectionChanged synchronously, so any
    // consumer that reacts to it must find the descriptor side-table and the index map already
    // agreeing with the rows it is being told about. The reverse order publishes a torn triple.
    m_descriptors.insert(m_descriptors.begin() + index, descriptors.begin(), descriptors.end());
    m_indexByItemValid = false;

    // Shift every tracked index at or after the insertion point. O(visible rows) in integer
    // compares -- the cost RowIdentity documents for the same reason: a hash map keyed by identity
    // cannot reach "every entry at or after this position" without visiting all of them. A splice
    // at the tail shifts nothing, which is the common bulk-load shape.
    for (auto& entry : m_indexByPathKey)
    {
        if (entry.second >= index)
        {
            entry.second += count;
        }
    }

    for (int32_t i = 0; i < count; ++i)
    {
        m_indexByPathKey[descriptors[static_cast<size_t>(i)].PathKey] = index + i;
    }

    uint32_t insertAt = static_cast<uint32_t>(index);
    for (auto const& item : items)
    {
        m_entries.InsertAt(insertAt++, item);
    }
}

void HierarchicalSourceAdapter::RemoveRows(int32_t index, int32_t count)
{
    if (count <= 0)
    {
        return;
    }

    // Metadata first, for the same reason InsertRows does it.
    for (int32_t i = 0; i < count; ++i)
    {
        m_indexByPathKey.erase(m_descriptors[static_cast<size_t>(index + i)].PathKey);
    }

    m_descriptors.erase(m_descriptors.begin() + index, m_descriptors.begin() + index + count);
    m_indexByItemValid = false;

    for (auto& entry : m_indexByPathKey)
    {
        if (entry.second >= index)
        {
            entry.second -= count;
        }
    }

    for (int32_t i = 0; i < count; ++i)
    {
        m_entries.RemoveAt(static_cast<uint32_t>(index));
    }
}

// --- Lookups --------------------------------------------------------------------------------------

HierarchicalSourceAdapter::NodeRow const* HierarchicalSourceAdapter::TryGetNodeRow(int32_t index) const
{
    if (index < 0 || static_cast<size_t>(index) >= m_descriptors.size())
    {
        return nullptr;
    }

    return &m_descriptors[static_cast<size_t>(index)];
}

HierarchicalSourceAdapter::NodeRow const* HierarchicalSourceAdapter::TryGetNodeRowForItem(winrt::IInspectable const& item) const
{
    if (!item)
    {
        return nullptr;
    }

    // O(1) against a cache rebuilt lazily after any mutation, rather than the O(visible rows) scan
    // this used to be. The scan was not expensive by itself, but the grouped hierarchical path
    // performs it once per row, and an identity-index rebuild performs it once per row per row.
    EnsureItemIndex();

    const auto found = m_indexByItem.find(winrt::get_abi(item));
    if (found == m_indexByItem.end() ||
        found->second < 0 ||
        static_cast<size_t>(found->second) >= m_descriptors.size())
    {
        return nullptr;
    }

    return &m_descriptors[static_cast<size_t>(found->second)];
}

void HierarchicalSourceAdapter::EnsureItemIndex() const
{
    if (m_indexByItemValid)
    {
        return;
    }

    m_indexByItem.clear();
    m_indexByItem.reserve(m_descriptors.size());

    // Front to back with first-match-wins, which is what the linear scan answered: an object that
    // appears at two places in the tree resolves to its topmost row, as it always has.
    for (size_t i = 0; i < m_descriptors.size(); ++i)
    {
        if (auto const& descriptorItem = m_descriptors[i].Item)
        {
            m_indexByItem.emplace(winrt::get_abi(descriptorItem), static_cast<int32_t>(i));
        }
    }

    m_indexByItemValid = true;
}

bool HierarchicalSourceAdapter::TryGetIndexForPathKey(winrt::hstring const& pathKey, int32_t& index) const{
    index = -1;
    if (pathKey.empty())
    {
        return false;
    }

    const auto found = m_indexByPathKey.find(pathKey);
    if (found == m_indexByPathKey.end())
    {
        return false;
    }

    index = found->second;
    return true;
}

// --- Threading ------------------------------------------------------------------------------------

bool HierarchicalSourceAdapter::OnUiThread() const
{
    auto const queue = m_uiQueue.get();
    return queue && queue.HasThreadAccess();
}

void HierarchicalSourceAdapter::AssertRebuildOnUiThread() const
{
    // A source bound off the UI thread captures no queue, so affinity can't be proven either way
    // and the assert stands down rather than firing on something it cannot judge.
    MUX_ASSERT(!m_uiQueue.get() || OnUiThread());
}

// --- Source / node subscription ---------------------------------------------------------------------

void HierarchicalSourceAdapter::AttachToSource()
{
    auto src = Source();
    if (!src)
    {
        return;
    }

    m_attachedSourceForRevocation = src;

    // Notifications are required on the owning UI thread and applied synchronously as a full
    // Rebuild + Reset. INotifyCollectionChanged first -- typed CLR ObservableCollection<T> won't
    // surface as IObservableVector.
    if (auto incc = src.try_as<winrt::Microsoft::UI::Xaml::Interop::INotifyCollectionChanged>())
    {
        m_outerCollectionChangedToken = incc.CollectionChanged(
            [weakThis = weak_from_this()](auto&&, auto&&)
            {
                if (auto strongThis = weakThis.lock()) { strongThis->Rebuild(); }
            });
    }
    else if (auto obs = src.try_as<winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable>>())
    {
        m_outerVectorChangedToken = obs.VectorChanged(
            [weakThis = weak_from_this()](auto&&, auto&&)
            {
                if (auto strongThis = weakThis.lock()) { strongThis->Rebuild(); }
            });
    }
    else if (auto bobs = src.try_as<winrt::Microsoft::UI::Xaml::Interop::IBindableObservableVector>())
    {
        m_outerBindableVectorChangedToken = bobs.VectorChanged(
            [weakThis = weak_from_this()](auto&&, auto&&)
            {
                if (auto strongThis = weakThis.lock()) { strongThis->Rebuild(); }
            });
    }
}

void HierarchicalSourceAdapter::DetachFromSource()
{
    auto attached = m_attachedSourceForRevocation;
    if (attached)
    {
        if (m_outerCollectionChangedToken.value != 0)
        {
            if (auto incc = attached.try_as<winrt::Microsoft::UI::Xaml::Interop::INotifyCollectionChanged>())
            {
                ShapingHelpers::SafeRevokeWith([&] { incc.CollectionChanged(m_outerCollectionChangedToken); });
            }
        }
        if (m_outerVectorChangedToken.value != 0)
        {
            if (auto obs = attached.try_as<winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable>>())
            {
                ShapingHelpers::SafeRevokeWith([&] { obs.VectorChanged(m_outerVectorChangedToken); });
            }
        }
        if (m_outerBindableVectorChangedToken.value != 0)
        {
            if (auto bobs = attached.try_as<winrt::Microsoft::UI::Xaml::Interop::IBindableObservableVector>())
            {
                ShapingHelpers::SafeRevokeWith([&] { bobs.VectorChanged(m_outerBindableVectorChangedToken); });
            }
        }
    }
    m_outerCollectionChangedToken = {};
    m_outerVectorChangedToken = {};
    m_outerBindableVectorChangedToken = {};
    m_attachedSourceForRevocation = nullptr;

    UnsubscribeFromAllNodes();
}

void HierarchicalSourceAdapter::SubscribeToNode(winrt::hstring const& pathKey, winrt::IInspectable const& childrenCollection, std::vector<NodeSubscription>& into)
{
    if (!childrenCollection)
    {
        return;
    }

    NodeSubscription sub;
    sub.PathKey = pathKey;
    sub.ItemsForRevocation = childrenCollection;

    // Any change inside a node's children is a full Rebuild -- no fast path, so no weak_ref to the
    // node is needed inside the callback (it captures only weak_from_this, never a strong back-ref).
    if (auto incc = childrenCollection.try_as<winrt::Microsoft::UI::Xaml::Interop::INotifyCollectionChanged>())
    {
        sub.CollectionToken = incc.CollectionChanged(
            [weakThis = weak_from_this()](auto&&, auto&&)
            {
                if (auto strongThis = weakThis.lock()) { strongThis->Rebuild(); }
            });
    }
    else if (auto obs = childrenCollection.try_as<winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable>>())
    {
        sub.Token = obs.VectorChanged(
            [weakThis = weak_from_this()](auto&&, auto&&)
            {
                if (auto strongThis = weakThis.lock()) { strongThis->Rebuild(); }
            });
    }
    else if (auto bobs = childrenCollection.try_as<winrt::Microsoft::UI::Xaml::Interop::IBindableObservableVector>())
    {
        sub.BindableToken = bobs.VectorChanged(
            [weakThis = weak_from_this()](auto&&, auto&&)
            {
                if (auto strongThis = weakThis.lock()) { strongThis->Rebuild(); }
            });
    }

    if (sub.CollectionToken.value != 0 || sub.Token.value != 0 || sub.BindableToken.value != 0)
    {
        into.push_back(std::move(sub));
    }
}

void HierarchicalSourceAdapter::RevokeAll(std::vector<NodeSubscription>& subscriptions)
{
    for (auto& sub : subscriptions)
    {
        RevokeSubscription(sub);
    }

    subscriptions.clear();
}

void HierarchicalSourceAdapter::InvokeShapeSiblings(std::vector<winrt::IInspectable>& siblings)
{
    if (!m_shapeSiblings)
    {
        return;
    }

    // Set even if the callback throws: the flag must describe the stack, not the outcome.
    m_inShapeSiblings = true;
    auto guard = wil::scope_exit([this]() noexcept { m_inShapeSiblings = false; });

    m_shapeSiblings(siblings);
}

void HierarchicalSourceAdapter::AssertNotInShapeSiblings() const
{
    // ShapeSiblings runs mid-walk, over structures that are half-built by definition. A mutator
    // called from inside it cannot be honoured in place, so m_rebuildInFlight absorbs it into a
    // redundant rebuild rather than corrupting anything -- correct, but silently wasteful and a
    // sign of a real bug upstream. Assert in chk so it gets found at authoring time.
    MUX_ASSERT_MSG(!m_inShapeSiblings, "ShapeSiblings must not call back into the adapter.");
}

void HierarchicalSourceAdapter::RevokeSubscription(NodeSubscription& sub)
{
    // Teardown is UI-thread-guaranteed by the owning ReferenceTrackers (TableViewSource / TableView),
    // so revoking here never crosses threads. SafeRevokeWith + the weak_from_this callbacks remain
    // for re-entrancy / GC-safety.
    auto strong = sub.ItemsForRevocation;
    if (!strong)
    {
        return;
    }

    if (sub.CollectionToken.value != 0)
    {
        if (auto incc = strong.try_as<winrt::Microsoft::UI::Xaml::Interop::INotifyCollectionChanged>())
        {
            ShapingHelpers::SafeRevokeWith([&] { incc.CollectionChanged(sub.CollectionToken); });
        }
    }
    if (sub.Token.value != 0)
    {
        if (auto obs = strong.try_as<winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable>>())
        {
            ShapingHelpers::SafeRevokeWith([&] { obs.VectorChanged(sub.Token); });
        }
    }
    if (sub.BindableToken.value != 0)
    {
        if (auto bobs = strong.try_as<winrt::Microsoft::UI::Xaml::Interop::IBindableObservableVector>())
        {
            ShapingHelpers::SafeRevokeWith([&] { bobs.VectorChanged(sub.BindableToken); });
        }
    }
}

void HierarchicalSourceAdapter::UnsubscribeUnder(winrt::hstring const& pathKey)
{
    // The node itself and everything beneath it. A descendant's path is the node's path plus a
    // separator and more, so the prefix test needs that separator: without it, "node:/0x1a" would
    // also match the unrelated sibling "node:/0x1ab".
    const std::wstring_view prefix{ pathKey.c_str(), pathKey.size() };

    for (auto it = m_nodeSubscriptions.begin(); it != m_nodeSubscriptions.end();)
    {
        const std::wstring_view candidate{ it->PathKey.c_str(), it->PathKey.size() };
        const bool isSelf = candidate == prefix;
        const bool isDescendant =
            candidate.size() > prefix.size() &&
            candidate.substr(0, prefix.size()) == prefix &&
            candidate[prefix.size()] == L'/';

        if (isSelf || isDescendant)
        {
            RevokeSubscription(*it);
            it = m_nodeSubscriptions.erase(it);
        }
        else
        {
            it = std::next(it);
        }
    }
}

void HierarchicalSourceAdapter::UnsubscribeFromAllNodes()
{
    RevokeAll(m_nodeSubscriptions);
}
