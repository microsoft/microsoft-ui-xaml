// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

class ShapedGroup;

// Presents an ICollectionView's CollectionGroups in the group contract GroupedSourceAdapter reads.
// Each view group is wrapped in a ShapedGroup whose items ARE the view group's GroupItems, so a change
// inside a group reaches the adapter directly; the wrapper list follows CollectionGroups changes.
// The view stays the owner of its grouping and its group order.
//
// A group is identified by its key value (the Group object's "Key" property, the IGrouping<K,T>
// idiom, else the Group object), not by the group object, so expansion state survives a view that
// recreates its groups on every recompute (CollectionViewSource does).
class TableViewCollectionViewGroups final : public std::enable_shared_from_this<TableViewCollectionViewGroups>
{
public:
    // Null when the view is not grouped (CollectionGroups is null).
    static std::shared_ptr<TableViewCollectionViewGroups> TryCreate(winrt::Microsoft::UI::Xaml::Data::ICollectionView const& view);

    explicit TableViewCollectionViewGroups(winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> const& groups);
    ~TableViewCollectionViewGroups();

    // Reads the groups, then follows CollectionGroups changes.
    void Start();

    // The wrapped groups, in the view's group order. The vector object is stable.
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> const& Groups() const noexcept { return m_wrappers; }

    struct GroupInfo
    {
        winrt::hstring Identity;
        winrt::IInspectable Key{ nullptr };
        winrt::IInspectable Group{ nullptr };
    };
    std::vector<GroupInfo> CurrentGroups() const;

    // The identity a group with this key has.
    static winrt::hstring IdentityForKey(winrt::IInspectable const& key);
    // The identity of the group that holds `item`; empty when no group holds it.
    winrt::hstring GroupIdentityForItem(winrt::IInspectable const& item);
    // A view may move items between its groups without replacing a group; re-read membership.
    void InvalidateItemIdentities() noexcept { m_identityByItemValid = false; }

    // Raised after the wrapped groups changed (a group added, removed or replaced).
    void SetChangedHandler(std::function<void()> handler) { m_changed = std::move(handler); }

private:
    void Refresh();
    void OnGroupsChanged();

    winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> m_groups{ nullptr };
    winrt::event_token m_groupsChangedToken{};
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> m_wrappers{
        winrt::single_threaded_observable_vector<winrt::IInspectable>() };
    // Reused by identity, so a group's wrapper (and its expansion state) outlives a recompute.
    std::unordered_map<winrt::hstring, winrt::com_ptr<ShapedGroup>> m_wrapperByIdentity{};
    std::vector<winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable>> m_wrappedItems{};
    // Item address -> identity of the group holding it, built on demand.
    std::unordered_map<uintptr_t, winrt::hstring> m_identityByItem{};
    bool m_identityByItemValid{ false };
    std::function<void()> m_changed{};
    bool m_refreshing{ false };
    bool m_refreshPending{ false };
};
