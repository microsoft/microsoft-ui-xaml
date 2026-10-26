// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "HierarchyStructureCache.h"
#include "ShapingHelpers.h"
#include "ShapingRevoke.h"

#include <winrt/Microsoft.UI.Xaml.Data.h>
#include <winrt/Microsoft.UI.Xaml.Interop.h>

#include <algorithm>
#include <utility>

namespace
{
    bool SameMembers(std::vector<winrt::IInspectable> const& a, std::vector<winrt::IInspectable> const& b) noexcept
    {
        if (a.size() != b.size())
        {
            return false;
        }
        for (size_t i = 0; i < a.size(); ++i)
        {
            if (winrt::get_abi(a[i]) != winrt::get_abi(b[i]))
            {
                return false;
            }
        }
        return true;
    }
}

const HierarchyStructureCache::Children HierarchyStructureCache::s_empty =
    std::make_shared<const std::vector<winrt::IInspectable>>();

HierarchyStructureCache::~HierarchyStructureCache()
{
    for (auto& entry : m_nodes)
    {
        Revoke(entry.second);
        RevokeItem(entry.second);
    }
}

winrt::hstring HierarchyStructureCache::MakeObjectIdentity(winrt::IInspectable const& item)
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
    // it, because RowIdentity belongs to layer 2 and the hierarchy adapter is layer 3; the two are
    // siblings, not a stack (see HierarchyContract.h).
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

void HierarchyStructureCache::BeginPass()
{
    if (m_passDepth++ == 0)
    {
        ++m_generation;
    }
}

void HierarchyStructureCache::EndPass()
{
    MUX_ASSERT(m_passDepth > 0);
    if (m_passDepth <= 0)
    {
        return;
    }

    if (--m_passDepth == 0 && m_reachabilityDirty)
    {
        m_reachabilityDirty = false;
        Sweep();
    }
}

void HierarchyStructureCache::Roots(std::vector<winrt::IInspectable> const& roots)
{
    std::unordered_set<void*> keys;
    keys.reserve(roots.size());
    for (auto const& root : roots)
    {
        if (root)
        {
            keys.insert(winrt::get_abi(root));
        }
    }

    if (m_hasRoots && !m_reachabilityDirty)
    {
        for (auto const key : m_rootKeys)
        {
            if (keys.find(key) == keys.end())
            {
                m_reachabilityDirty = true;
                break;
            }
        }
    }

    m_roots = roots;
    m_rootKeys = std::move(keys);
    m_hasRoots = true;

    if (m_trackItems)
    {
        for (auto const& root : m_roots)
        {
            if (root)
            {
                Touch(root);
            }
        }
    }

    if (m_passDepth == 0 && m_reachabilityDirty)
    {
        m_reachabilityDirty = false;
        Sweep();
    }
}

HierarchyStructureCache::Node& HierarchyStructureCache::Touch(winrt::IInspectable const& item)
{
    // Keyed by the ABI pointer the caller holds. The node keeps a strong reference to the item, so
    // the pointer cannot be recycled for a different object while the node exists.
    void* const key = winrt::get_abi(item);
    auto [it, inserted] = m_nodes.try_emplace(key);
    auto& node = it->second;
    if (inserted)
    {
        node.Item = item;
        if (m_trackItems)
        {
            SubscribeItem(key, node);
        }
    }
    return node;
}

winrt::hstring HierarchyStructureCache::Identity(winrt::IInspectable const& item)
{
    if (!item)
    {
        return MakeObjectIdentity(item);
    }

    auto& node = Touch(item);
    if (!node.HasIdentity)
    {
        node.Identity = MakeObjectIdentity(item);
        node.HasIdentity = true;
    }
    return node.Identity;
}

HierarchyStructureCache::Children HierarchyStructureCache::ChildrenOf(
    winrt::IInspectable const& item,
    ShapingHelpers::ChildrenFn const& selector)
{
    if (!item || !selector)
    {
        return s_empty;
    }

    void* const key = winrt::get_abi(item);
    auto& node = Touch(item);

    if (node.Items && !node.Stale && !node.SelectorDirty)
    {
        // An observed collection that has raised no change is trusted without asking the selector:
        // its change notifications are the invalidator. So is a null result (a leaf): only the
        // item can change it, which live shaping reports through SelectorDirty. An unobserved
        // collection is asked once per pass, so the retention walk and the emit walk share that
        // call; outside a pass there is no generation to share, so it is always asked.
        if (node.Observed || node.NoCollection || (m_passDepth > 0 && node.ValidatedGeneration == m_generation))
        {
            return node.Items;
        }
    }

    winrt::IInspectable collection{ nullptr };
    bool selectorFailed = false;
    try
    {
        collection = selector(item);
    }
    catch (...)
    {
        // App code. A throwing selector means "no children we can see", not "fail the whole
        // projection".
        selectorFailed = true;
    }

    node.ValidatedGeneration = m_generation;
    node.SelectorDirty = false;
    // A throwing selector is not trusted: it is asked again next time.
    node.NoCollection = !selectorFailed && !collection;

    if (selectorFailed || !collection)
    {
        Revoke(node);
        node.Collection = nullptr;
        ReplaceItems(key, node, s_empty);
        node.Stale = false;
        return node.Items;
    }

    const bool sameCollection = node.Collection && winrt::get_abi(node.Collection) == winrt::get_abi(collection);
    if (sameCollection && node.Observed && !node.Stale && node.Items)
    {
        // The selector handed back the very collection we enumerated last time, and that collection
        // has raised no change since. Its contents are what we cached.
        return node.Items;
    }

    std::vector<winrt::IInspectable> items;
    try
    {
        items = ShapingHelpers::EnumerateInspectableItems(collection);
    }
    catch (...)
    {
        items.clear();
    }

    if (!sameCollection)
    {
        Revoke(node);
        node.Collection = collection;
        Subscribe(key, node);
    }

    ReplaceItems(key, node, std::make_shared<const std::vector<winrt::IInspectable>>(std::move(items)));
    node.Stale = false;
    return node.Items;
}

HierarchyStructureCache::Children HierarchyStructureCache::ShapedChildrenOf(
    winrt::IInspectable const& item,
    ShapingHelpers::ChildrenFn const& selector,
    ShapingHelpers::ShapeSiblingsFn const& shape)
{
    auto raw = ChildrenOf(item, selector);
    if (!shape || !item || !selector)
    {
        return raw;
    }

    void* const key = winrt::get_abi(item);
    auto found = m_nodes.find(key);
    if (found == m_nodes.end())
    {
        return raw;
    }

    {
        auto const& node = found->second;
        if (node.Shaped && node.Items == raw &&
            node.ShapedListVersion == node.ListVersion &&
            node.ShapedGeneration == m_shapeGeneration)
        {
            return node.Shaped;
        }
    }

    const auto listVersion = found->second.ListVersion;
    const auto generation = m_shapeGeneration;

    // Shaped as a copy: the raw set is shared with every other reader.
    std::vector<winrt::IInspectable> shaped{ *raw };
    shape(shaped);
    auto result = std::make_shared<const std::vector<winrt::IInspectable>>(std::move(shaped));

    // Found again: shape is app code, and anything it set off may have released the node. Cached
    // only when nothing it was computed from has moved in the meantime.
    found = m_nodes.find(key);
    if (found != m_nodes.end() &&
        found->second.Items == raw &&
        found->second.ListVersion == listVersion &&
        m_shapeGeneration == generation)
    {
        auto& node = found->second;
        node.Shaped = result;
        node.ShapedListVersion = listVersion;
        node.ShapedGeneration = generation;
    }
    return result;
}

void HierarchyStructureCache::ReplaceItems(void* key, Node& node, Children items)
{
    MUX_ASSERT(items);
    auto const previous = node.Items;
    if (previous && (previous == items || SameMembers(*previous, *items)))
    {
        // Same members in the same order: the list did not change, so neither does anything shaped
        // from it. This is what lets an unobservable collection, re-enumerated every pass, keep its
        // shaped list.
        return;
    }

    if (previous && !previous->empty())
    {
        std::unordered_set<void*> next;
        next.reserve(items->size());
        for (auto const& child : *items)
        {
            if (child)
            {
                next.insert(winrt::get_abi(child));
            }
        }

        for (auto const& child : *previous)
        {
            void* const childKey = winrt::get_abi(child);
            if (child && next.find(childKey) == next.end())
            {
                // A member left this list. It may still be reachable through another parent, so
                // whether it is released is decided by the sweep at the end of the pass.
                m_reachabilityDirty = true;
                if (m_trackItems)
                {
                    UnlinkParent(key, childKey);
                }
            }
        }
    }

    node.Items = std::move(items);
    ++node.ListVersion;
    node.Shaped = nullptr;

    if (m_trackItems)
    {
        // Captured first: LinkParent inserts into the map. References survive a rehash, but the
        // list is held strongly so nothing below depends on that.
        auto const members = node.Items;
        for (auto const& child : *members)
        {
            LinkParent(key, child);
        }
    }
}

void HierarchyStructureCache::LinkParent(void* parentKey, winrt::IInspectable const& child)
{
    if (!child)
    {
        return;
    }

    auto& parents = Touch(child).Parents;
    if (std::find(parents.begin(), parents.end(), parentKey) == parents.end())
    {
        parents.push_back(parentKey);
    }
}

void HierarchyStructureCache::UnlinkParent(void* parentKey, void* childKey)
{
    auto it = m_nodes.find(childKey);
    if (it == m_nodes.end())
    {
        return;
    }

    auto& parents = it->second.Parents;
    parents.erase(std::remove(parents.begin(), parents.end(), parentKey), parents.end());
}

bool HierarchyStructureCache::InvalidateShapeOfParents(winrt::IInspectable const& item)
{
    if (!item)
    {
        return false;
    }

    auto it = m_nodes.find(winrt::get_abi(item));
    if (it == m_nodes.end())
    {
        return false;
    }

    bool invalidated = false;
    for (auto const parentKey : it->second.Parents)
    {
        if (auto parent = m_nodes.find(parentKey); parent != m_nodes.end())
        {
            ++parent->second.ListVersion;
            parent->second.Shaped = nullptr;
            invalidated = true;
        }
    }
    return invalidated;
}

void HierarchyStructureCache::TrackItemChanges(bool enabled)
{
    if (m_trackItems == enabled)
    {
        return;
    }
    m_trackItems = enabled;

    if (!enabled)
    {
        for (auto& entry : m_nodes)
        {
            RevokeItem(entry.second);
            entry.second.Parents.clear();
            entry.second.SelectorDirty = false;
        }
        m_hasSelectorDirty = false;
        return;
    }

    // Existing nodes first, while iterating is safe; linking below inserts nodes for list members
    // that were never visited, and Touch subscribes those as it creates them.
    std::vector<std::pair<void*, Children>> lists;
    for (auto& entry : m_nodes)
    {
        SubscribeItem(entry.first, entry.second);
        if (entry.second.Items)
        {
            lists.emplace_back(entry.first, entry.second.Items);
        }
    }

    for (auto const& root : m_roots)
    {
        if (root)
        {
            Touch(root);
        }
    }

    for (auto const& [parentKey, members] : lists)
    {
        for (auto const& child : *members)
        {
            LinkParent(parentKey, child);
        }
    }
}

bool HierarchyStructureCache::RevalidateItemChanges(ShapingHelpers::ChildrenFn const& selector)
{
    if (!m_hasSelectorDirty)
    {
        return false;
    }
    m_hasSelectorDirty = false;

    std::vector<void*> dirty;
    for (auto& entry : m_nodes)
    {
        if (std::exchange(entry.second.SelectorDirty, false) && entry.second.Items)
        {
            dirty.push_back(entry.first);
        }
    }

    bool changed = false;
    for (auto const key : dirty)
    {
        auto it = m_nodes.find(key);
        if (it == m_nodes.end())
        {
            continue;
        }

        auto const item = it->second.Item;
        winrt::IInspectable collection{ nullptr };
        bool selectorFailed = !selector;
        if (selector)
        {
            try
            {
                collection = selector(item);
            }
            catch (...)
            {
                selectorFailed = true;
            }
        }

        // Found again: the selector is app code.
        it = m_nodes.find(key);
        if (it == m_nodes.end())
        {
            continue;
        }

        auto& node = it->second;
        if (selectorFailed || winrt::get_abi(collection) != winrt::get_abi(node.Collection))
        {
            node.Stale = true;
            changed = true;
        }
    }
    return changed;
}

void HierarchyStructureCache::Subscribe(void* key, Node& node)
{
    node.Observed = false;
    if (!node.Collection)
    {
        return;
    }

    // Weak back-reference: the subscription must not keep the cache alive, and a notification
    // that arrives after teardown is a no-op. The node is found again by key rather than captured,
    // because the map may rehash or evict it in the meantime.
    auto handler = [weakThis = weak_from_this(), key](auto&&, auto&&)
    {
        if (auto strongThis = weakThis.lock())
        {
            strongThis->OnCollectionChanged(key);
        }
    };

    try
    {
        if (auto incc = node.Collection.try_as<winrt::Microsoft::UI::Xaml::Interop::INotifyCollectionChanged>())
        {
            node.CollectionToken = incc.CollectionChanged(handler);
        }
        else if (auto obs = node.Collection.try_as<winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable>>())
        {
            node.VectorToken = obs.VectorChanged(handler);
        }
        else if (auto bobs = node.Collection.try_as<winrt::Microsoft::UI::Xaml::Interop::IBindableObservableVector>())
        {
            node.BindableToken = bobs.VectorChanged(handler);
        }
    }
    catch (...)
    {
        // An unobservable collection is still usable; it is simply re-enumerated every pass.
    }

    node.Observed = node.CollectionToken.value != 0 || node.VectorToken.value != 0 || node.BindableToken.value != 0;
}

void HierarchyStructureCache::SubscribeItem(void* key, Node& node)
{
    if (node.PropertyToken.value != 0 || !node.Item)
    {
        return;
    }

    try
    {
        if (auto inpc = node.Item.try_as<winrt::Microsoft::UI::Xaml::Data::INotifyPropertyChanged>())
        {
            node.PropertyToken = inpc.PropertyChanged(
                [weakThis = weak_from_this(), key](auto&&, winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs const& args)
                {
                    if (auto strongThis = weakThis.lock())
                    {
                        winrt::hstring propertyName;
                        try
                        {
                            propertyName = args.PropertyName();
                        }
                        catch (...)
                        {
                            // Treated as "everything changed", the same as an empty name.
                        }
                        strongThis->OnItemPropertyChanged(key, propertyName);
                    }
                });
        }
    }
    catch (...)
    {
        // An item that cannot be observed is simply not live-shaped.
    }
}

void HierarchyStructureCache::Revoke(Node& node) noexcept
{
    auto const collection = node.Collection;
    if (collection)
    {
        if (node.CollectionToken.value != 0)
        {
            ShapingHelpers::SafeRevokeWith([&]
            {
                collection.as<winrt::Microsoft::UI::Xaml::Interop::INotifyCollectionChanged>().CollectionChanged(node.CollectionToken);
            });
        }
        if (node.VectorToken.value != 0)
        {
            ShapingHelpers::SafeRevokeWith([&]
            {
                collection.as<winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable>>().VectorChanged(node.VectorToken);
            });
        }
        if (node.BindableToken.value != 0)
        {
            ShapingHelpers::SafeRevokeWith([&]
            {
                collection.as<winrt::Microsoft::UI::Xaml::Interop::IBindableObservableVector>().VectorChanged(node.BindableToken);
            });
        }
    }

    node.CollectionToken = {};
    node.VectorToken = {};
    node.BindableToken = {};
    node.Observed = false;
}

void HierarchyStructureCache::RevokeItem(Node& node) noexcept
{
    if (node.PropertyToken.value != 0 && node.Item)
    {
        auto const item = node.Item;
        auto const token = node.PropertyToken;
        ShapingHelpers::SafeRevokeWith([&]
        {
            item.as<winrt::Microsoft::UI::Xaml::Data::INotifyPropertyChanged>().PropertyChanged(token);
        });
    }
    node.PropertyToken = {};
}

void HierarchyStructureCache::OnCollectionChanged(void* key)
{
    auto it = m_nodes.find(key);
    if (it == m_nodes.end())
    {
        return;
    }
    it->second.Stale = true;

    // Copied: the handler may re-declare the hierarchy and replace m_changed.
    if (auto const handler = m_changed)
    {
        handler();
    }
}

void HierarchyStructureCache::OnItemPropertyChanged(void* key, winrt::hstring const& propertyName)
{
    auto it = m_nodes.find(key);
    if (it == m_nodes.end() || !m_trackItems)
    {
        return;
    }

    auto& node = it->second;
    if (node.Items)
    {
        node.SelectorDirty = true;
        m_hasSelectorDirty = true;
    }

    auto const item = node.Item;
    if (auto const handler = m_itemChanged)
    {
        handler(item, propertyName);
    }
}

void HierarchyStructureCache::Sweep()
{
    if (!m_hasRoots)
    {
        // Nothing to measure reachability from yet, so nothing can be proven unreachable.
        return;
    }

    // Mark: everything reachable from the roots through cached lists. A stale node's list is the
    // one it had before its change -- a superset of what it may now hold -- so the sweep can only
    // over-retain until that node is re-enumerated, never release a node still in use. The reached
    // set doubles as the cycle guard.
    std::unordered_set<void*> reached;
    reached.reserve(m_nodes.size());
    std::vector<void*> pending;
    for (auto const key : m_rootKeys)
    {
        if (reached.insert(key).second)
        {
            pending.push_back(key);
        }
    }

    while (!pending.empty())
    {
        void* const key = pending.back();
        pending.pop_back();

        auto it = m_nodes.find(key);
        if (it == m_nodes.end() || !it->second.Items)
        {
            continue;
        }

        for (auto const& child : *it->second.Items)
        {
            if (child)
            {
                void* const childKey = winrt::get_abi(child);
                if (reached.insert(childKey).second)
                {
                    pending.push_back(childKey);
                }
            }
        }
    }

    // Sweep: moved out before revoking, so a revoke that calls back into app code cannot observe a
    // node that is half released.
    std::vector<std::pair<void*, Node>> released;
    for (auto it = m_nodes.begin(); it != m_nodes.end();)
    {
        if (reached.find(it->first) != reached.end())
        {
            ++it;
            continue;
        }

        released.emplace_back(it->first, std::move(it->second));
        it = m_nodes.erase(it);
    }

    if (m_trackItems)
    {
        for (auto const& [key, node] : released)
        {
            if (node.Items)
            {
                for (auto const& child : *node.Items)
                {
                    if (child)
                    {
                        UnlinkParent(key, winrt::get_abi(child));
                    }
                }
            }
        }
    }

    for (auto& entry : released)
    {
        Revoke(entry.second);
        RevokeItem(entry.second);
    }
}

void HierarchyStructureCache::Reset()
{
    // Moved out first: revoking can call back into app code, which must not observe a half-cleared
    // map.
    auto nodes = std::move(m_nodes);
    m_nodes.clear();
    m_roots.clear();
    m_rootKeys.clear();
    m_hasRoots = false;
    m_reachabilityDirty = false;
    m_hasSelectorDirty = false;
    ++m_shapeGeneration;
    for (auto& entry : nodes)
    {
        Revoke(entry.second);
        RevokeItem(entry.second);
    }
}