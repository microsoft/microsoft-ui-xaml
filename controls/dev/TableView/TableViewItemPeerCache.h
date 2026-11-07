// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <algorithm>
#include <unordered_map>

// ListView-style item peers for recycled TableView containers. A container's own peer is an internal
// forwarder whose EventsSource is the peer of the item it currently shows; item peers are cached
// weakly by identity, so an item keeps one UIA element (one RuntimeId) across containers.
class TableViewItemPeerCache
{
public:
    template <typename Create>
    winrt::AutomationPeer GetOrCreate(winrt::hstring const& identity, Create&& create)
    {
        if (auto const it = m_peers.find(identity); it != m_peers.end())
        {
            if (auto peer = it->second.get())
            {
                return peer;
            }
        }

        auto peer = create();
        if (peer)
        {
            if (m_peers.size() >= m_pruneAt)
            {
                std::erase_if(m_peers, [](auto const& entry) { return !entry.second.get(); });
                m_pruneAt = std::max<size_t>(32, m_peers.size() * 2);
            }
            m_peers.insert_or_assign(identity, winrt::make_weak(peer));
        }
        return peer;
    }

    // Replaces a live entry that no longer stands for `identity`, as when a new item reuses the
    // address of a dead one whose peer a client still holds.
    void Assign(winrt::hstring const& identity, winrt::AutomationPeer const& peer)
    {
        m_peers.insert_or_assign(identity, winrt::make_weak(peer));
    }

private:
    std::unordered_map<winrt::hstring, winrt::weak_ref<winrt::AutomationPeer>> m_peers;
    size_t m_pruneAt{ 32 };
};

// Held by an item peer: the identity it represents and the container last bound to show it.
class TableViewItemPeerBinding
{
public:
    bool IsItemPeer() const noexcept { return m_isItemPeer; }
    winrt::hstring const& Identity() const noexcept { return m_identity; }
    // The container last bound to show this identity, whether or not it still does; see Resolve.
    winrt::UIElement Container() const { return m_container.get(); }

    bool IsBoundTo(winrt::hstring const& identity, winrt::UIElement const& container) const
    {
        return m_isItemPeer && m_identity == identity && m_container.get() == container;
    }

    void Bind(winrt::hstring const& identity, winrt::UIElement const& container)
    {
        m_isItemPeer = true;
        m_identity = identity;
        m_container = winrt::make_weak(container);
    }

    // The container if it still shows this identity and still forwards UIA to this peer; else null.
    template <typename IdentityOf, typename IsSelf>
    winrt::UIElement Resolve(IdentityOf&& identityOf, IsSelf&& isSelf) const
    {
        auto const container = m_container.get();
        if (!container || identityOf(container) != m_identity)
        {
            return nullptr;
        }
        auto const containerPeer = winrt::FrameworkElementAutomationPeer::FromElement(container);
        return containerPeer && isSelf(containerPeer.EventsSource()) ? container : nullptr;
    }

private:
    winrt::hstring m_identity;
    winrt::weak_ref<winrt::UIElement> m_container{ nullptr };
    bool m_isItemPeer{ false };
};

namespace TableViewItemPeers
{
    // The peer UIA sees for a container: its own peer's EventsSource (the item peer) when set, else that peer.
    inline winrt::AutomationPeer ItemPeerOfContainer(winrt::UIElement const& container, bool create)
    {
        if (!container)
        {
            return nullptr;
        }
        auto const peer = create
            ? winrt::FrameworkElementAutomationPeer::CreatePeerForElement(container)
            : winrt::FrameworkElementAutomationPeer::FromElement(container);
        if (auto const source = peer ? peer.EventsSource() : nullptr)
        {
            return source;
        }
        return peer;
    }

    // The container an item peer is bound to while that container still forwards UIA to it; else null.
    inline winrt::UIElement ContainerOf(TableViewItemPeerBinding const& binding, winrt::AutomationPeer const& itemPeer)
    {
        auto const container = binding.Container();
        return container && ItemPeerOfContainer(container, false) == itemPeer ? container : nullptr;
    }

    // The item peer `containerPeer` should forward to for `identity`: `current` if it is already bound
    // to this container, else the cached (or newly created) peer, rebound to this container.
    template <typename BindingOf, typename Create>
    winrt::AutomationPeer Resolve(
        TableViewItemPeerCache* cache,
        winrt::AutomationPeer const& current,
        winrt::hstring const& identity,
        winrt::UIElement const& container,
        BindingOf&& bindingOf,
        Create&& create)
    {
        if (auto* const binding = current ? bindingOf(current) : nullptr; binding && binding->IsBoundTo(identity, container))
        {
            return current;
        }
        auto const peer = cache ? cache->GetOrCreate(identity, create) : create();
        auto* const binding = peer ? bindingOf(peer) : nullptr;
        if (!binding)
        {
            return nullptr;
        }
        binding->Bind(identity, container);
        return peer;
    }
}
