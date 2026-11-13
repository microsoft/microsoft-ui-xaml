// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>

// Presents an ICollectionView's CollectionGroups through the GroupBy of the TableViewSource the
// control projects it with: an item's group key is the Group of the ICollectionViewGroup holding
// it (or that Group's "Key" value), and group order follows the view (each group's items are
// contiguous in the flat view, so the default SortGroups(None) keeps the view's order). The view
// stays the owner of its grouping. A group's identity derives from its key value, not the group
// object, so expansion state survives a view that recreates its groups on every recompute.
class TableViewCollectionViewGroups final : public std::enable_shared_from_this<TableViewCollectionViewGroups>
{
public:
    // Null when the view is not grouped (CollectionGroups is null).
    static std::shared_ptr<TableViewCollectionViewGroups> TryCreate(winrt::Microsoft::UI::Xaml::Data::ICollectionView const& view);

    explicit TableViewCollectionViewGroups(
        winrt::Microsoft::UI::Xaml::Data::ICollectionView const& view,
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> const& groups);
    ~TableViewCollectionViewGroups();

    // Declares the grouping on `source` (throws what GroupBy throws) and then follows CollectionGroups changes.
    void Attach(winrt::TableViewSource const& source);

private:
    struct Entry
    {
        winrt::Microsoft::UI::Xaml::Data::ICollectionViewGroup Group{ nullptr };
        uint32_t Index{};
        winrt::IInspectable Key{ nullptr };
    };

    winrt::IInspectable KeyFor(winrt::IInspectable const& item);
    winrt::hstring IdentityFor(winrt::IInspectable const& key) const;
    winrt::IInspectable TryFind(uintptr_t address) const;
    void Rebuild();
    void OnGroupsChanged(winrt::Windows::Foundation::Collections::CollectionChange change);
    bool TryRefreshSource();

    winrt::Microsoft::UI::Xaml::Data::ICollectionView m_view{ nullptr };
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> m_groups{ nullptr };
    winrt::event_token m_groupsChangedToken{};
    winrt::weak_ref<winrt::TableViewSource> m_source{ nullptr };
    // Keyed by canonical IUnknown address. An entry is trusted only after re-reading its slot,
    // so a stale map is detected without depending on event handler order.
    std::unordered_map<uintptr_t, Entry> m_entries{};
    // View items that no group held at the last rebuild; they share one fallback group.
    std::unordered_set<uintptr_t> m_ungrouped{};
    winrt::IInspectable m_ungroupedKey{ winrt::box_value(winrt::hstring{}) };
    bool m_dirty{ true };
    bool m_refreshQueued{ false };
};
