// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "pch.h"
#include "common.h"

#include "HierarchyContract.h"

// Per-node structure cache for a children-selector hierarchy -- the ChildrenWith counterpart of
// the ParentBy path's cached parent structure.
//
// Without it, every pass over the tree pays the full cost of discovering the tree again: per node
// an app call into the children selector, an enumeration of the result across the ABI, a QI plus a
// string format to mint the node's identity, and an unsubscribe/resubscribe on its children
// collection. Under an active filter that is paid TWICE per refresh, once by the retention pre-walk
// and once by the adapter's emit walk, each with its own set of subscriptions.
//
// The cache holds, per item object:
//   - the item's identity string, minted once;
//   - the last children collection the selector returned, with its enumerated (UNSHAPED) contents;
//   - ONE long-lived subscription to that collection;
//   - the last SHAPED (filtered + sorted) copy of those contents, keyed by the list's version and
//     the owner's shape generation, so an unchanged sibling list is not re-shaped by every walk.
//
// Validity. A node whose collection is OBSERVED and has raised no change is trusted as-is: the
// selector is not called again and the cached contents are reused. Collection change notifications
// are therefore the only invalidator; an app that silently swaps the collection a node's selector
// returns, or mutates a collection that cannot notify in place of one that can, is not seen until
// something else invalidates the node. A node whose collection is NOT observable (or whose selector
// returned null) keeps the pre-cache behaviour: the selector is re-called and the result
// re-enumerated once per pass, so the retention walk and the emit walk still share that one call.
//
// Lifetime. Nodes and their subscriptions survive rebuilds, splices and collapses -- a collapsed
// subtree is exactly what a later re-expand wants to reuse. A node is released only when it can no
// longer be reached: when a re-enumeration drops a child or the root set loses a member, the
// outermost pass ends with a mark-and-sweep from the roots through every cached child list, which
// keeps an item reachable from another parent and is immune to cycles. Reset releases everything
// (re-declaration, retraction, source replacement).
//
// Item tracking (live shaping). When enabled, every cached item's INotifyPropertyChanged is
// observed and each node records the cached lists it is a member of. A property change marks the
// node so the next revalidation re-asks the selector (a replaced Children property is a structure
// change), and hands the item to the owner, which decides whether it can move its parent's
// shaped list and invalidates that list through InvalidateShapeOfParents. Disabled, no item is
// subscribed and no membership is recorded.
//
// UI-thread-affine, like both of its owners. Not thread-safe.
class HierarchyStructureCache : public std::enable_shared_from_this<HierarchyStructureCache>
{
public:
    HierarchyStructureCache() = default;
    ~HierarchyStructureCache();

    HierarchyStructureCache(HierarchyStructureCache const&) = delete;
    HierarchyStructureCache& operator=(HierarchyStructureCache const&) = delete;

    using Children = std::shared_ptr<const std::vector<winrt::IInspectable>>;

    // Raised after a cached children collection reports a change. The node is already marked stale
    // by then, so a rebuild triggered from the handler re-enumerates it. Held by value and copied
    // before invoking, so the handler may replace it.
    void ChangedHandler(std::function<void()> fn) { m_changed = std::move(fn); }

    // Raised, while item tracking is on, after a tracked item raised PropertyChanged. Same copy
    // semantics as ChangedHandler.
    using ItemChangedFn = std::function<void(winrt::IInspectable const& item, winrt::hstring const& propertyName)>;
    void ItemChangedHandler(ItemChangedFn fn) { m_itemChanged = std::move(fn); }

    // Brackets one logical walk. Nested Begin/End pairs share the outermost pass, so an owner can
    // span the retention walk and the emit walk with one pass and the adapter's own Begin/End
    // simply joins it. The outermost EndPass releases whatever a removal made unreachable.
    void BeginPass();
    void EndPass();

    // RAII over BeginPass/EndPass. Holds the cache strongly so a Reset or owner swap inside the
    // pass cannot leave it dangling.
    class Pass
    {
    public:
        explicit Pass(std::shared_ptr<HierarchyStructureCache> cache) : m_cache(std::move(cache)) { m_cache->BeginPass(); }
        ~Pass() { m_cache->EndPass(); }
        Pass(Pass const&) = delete;
        Pass& operator=(Pass const&) = delete;
    private:
        std::shared_ptr<HierarchyStructureCache> m_cache;
    };

    // The complete, UNSHAPED root set -- the anchor reachability is measured from. Until the first
    // call nothing is ever released short of Reset. A root that drops out schedules a sweep.
    void Roots(std::vector<winrt::IInspectable> const& roots);

    // The node's identity string. Mints and caches it on first access.
    winrt::hstring Identity(winrt::IInspectable const& item);

    // The node's children, UNSHAPED, as the selector last reported them. Callers that shape must
    // copy first. A selector that throws yields an empty set, matching what the walks did before.
    // Never null.
    Children ChildrenOf(winrt::IInspectable const& item, ShapingHelpers::ChildrenFn const& selector);

    // The node's children after `shape` (null: unshaped). Reuses the previous result while neither
    // the list nor the shape generation has moved; a shape that throws caches nothing. Never null.
    Children ShapedChildrenOf(
        winrt::IInspectable const& item,
        ShapingHelpers::ChildrenFn const& selector,
        ShapingHelpers::ShapeSiblingsFn const& shape);

    // Every cached shaped list is out of date: the sort or filter that produced them changed.
    void InvalidateShapedChildren() noexcept { ++m_shapeGeneration; }

    // `item` changed in a way that can move it within, or into or out of, the shaped lists it is a
    // member of. Requires item tracking (membership is not recorded otherwise). Returns whether any
    // cached list was invalidated.
    bool InvalidateShapeOfParents(winrt::IInspectable const& item);

    // Turns item PropertyChanged tracking on or off for every cached node and for nodes created
    // later. Off revokes every item subscription and forgets list membership.
    void TrackItemChanges(bool enabled);
    bool IsTrackingItemChanges() const noexcept { return m_trackItems; }

    // Re-asks the selector for every realized node whose item raised PropertyChanged since the last
    // call, and marks the node stale when it now returns a different collection. Returns whether
    // any node became stale, i.e. whether the visible structure may have changed.
    bool RevalidateItemChanges(ShapingHelpers::ChildrenFn const& selector);

    bool IsRoot(winrt::IInspectable const& item) const
    {
        return item && m_rootKeys.find(winrt::get_abi(item)) != m_rootKeys.end();
    }

    // Drops every node and revokes every subscription. Called when the hierarchy is re-declared or
    // retracted: a new selector may map the same item to a different collection.
    void Reset();

    size_t NodeCount() const { return m_nodes.size(); }

    // Canonical object identity: the IUnknown pointer, as "0x%zx".
    static winrt::hstring MakeObjectIdentity(winrt::IInspectable const& item);

private:
    struct Node
    {
        winrt::IInspectable Item{ nullptr };
        winrt::hstring Identity;
        bool HasIdentity{ false };

        winrt::IInspectable Collection{ nullptr };
        Children Items;
        winrt::event_token CollectionToken{};
        winrt::event_token VectorToken{};
        winrt::event_token BindableToken{};
        winrt::event_token PropertyToken{};

        // True once Collection has a live subscription.
        bool Observed{ false };
        // True when the selector returned null: there is no collection that could change
        // silently, so, like an observed one, it is trusted until the item itself changes.
        bool NoCollection{ false };
        // Set by a change notification; cleared by the next enumeration.
        bool Stale{ true };
        // Set by the item's PropertyChanged: the selector must be asked again before Items is
        // trusted, because the property it reads may have been replaced.
        bool SelectorDirty{ false };
        // Generation in which an UNOBSERVED node's children were last validated against the
        // selector. Observed nodes do not need it.
        uint64_t ValidatedGeneration{ 0 };

        // Bumped whenever Items is replaced, or a member changed in a way that may re-shape it.
        uint64_t ListVersion{ 0 };
        Children Shaped;
        uint64_t ShapedListVersion{ 0 };
        uint64_t ShapedGeneration{ 0 };

        // Keys of the cached lists this item is a member of. Maintained only while tracking.
        std::vector<void*> Parents;
    };

    Node& Touch(winrt::IInspectable const& item);
    void ReplaceItems(void* key, Node& node, Children items);
    void LinkParent(void* parentKey, winrt::IInspectable const& child);
    void UnlinkParent(void* parentKey, void* childKey);
    void Subscribe(void* key, Node& node);
    void SubscribeItem(void* key, Node& node);
    static void Revoke(Node& node) noexcept;
    static void RevokeItem(Node& node) noexcept;
    void OnCollectionChanged(void* key);
    void OnItemPropertyChanged(void* key, winrt::hstring const& propertyName);
    void Sweep();

    std::unordered_map<void*, Node> m_nodes;
    std::function<void()> m_changed;
    ItemChangedFn m_itemChanged;

    // Strong references keep the keys from being recycled by other objects while they are held.
    std::vector<winrt::IInspectable> m_roots;
    std::unordered_set<void*> m_rootKeys;
    bool m_hasRoots{ false };

    uint64_t m_generation{ 0 };
    uint64_t m_shapeGeneration{ 1 };
    int32_t m_passDepth{ 0 };
    bool m_reachabilityDirty{ false };
    bool m_trackItems{ false };
    bool m_hasSelectorDirty{ false };

    static const Children s_empty;
};

using HierarchyStructureCachePtr = std::shared_ptr<HierarchyStructureCache>;
