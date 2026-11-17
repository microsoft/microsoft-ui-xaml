// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "TableViewRowInfo.h"
#include "ShapingDescriptions.h"

class GroupedSourceAdapter;
class HierarchicalSourceAdapter;
class ShapedGroup;
class TableViewCollectionViewGroups;

// Turns the ICollectionView TableView displays into the rows its ItemsRepeater realizes, the way WPF's
// ItemContainerGenerator turns an ItemsControl's view into containers. It never sorts, filters or
// groups: the view does (TableView wraps a plain collection in an internal CollectionViewSource). It
// only chooses how the view's items are presented:
//   - flat: the view itself, so the view's fine-grained changes reach the repeater unchanged;
//   - grouped: a header row plus data rows per view group (CollectionGroups, GroupedSourceAdapter);
//   - tree: ItemKeyPath / ParentKeyPath nest the view's items (HierarchicalSourceAdapter). Siblings keep
//     the view's order, and under a grouped view each root is placed in the group that holds it.
//
// UI-thread affine. Owned by one TableView.
class TableViewRowProjection : public std::enable_shared_from_this<TableViewRowProjection>
{
public:
    explicit TableViewRowProjection(winrt::Microsoft::UI::Xaml::Data::ICollectionView const& view);
    ~TableViewRowProjection();

    // Subscribes to the view and builds the first projection.
    void Start();

    winrt::Microsoft::UI::Xaml::Data::ICollectionView const& View() const noexcept { return m_view; }

    // Nests the rows: an item whose parent key equals another item's key is its child. Throws E_INVALIDARG
    // (keeping the current rows) on a duplicate or null key, a self-parent or a cycle. Re-declaring resets
    // the rows' expansion state.
    void SetHierarchy(ShapingHelpers::KeySelector const& keySelector, ShapingHelpers::KeySelector const& parentKeySelector);
    void ClearHierarchy();
    bool HasHierarchy() const noexcept { return static_cast<bool>(m_keySelector); }

    // True when the rows include group headers.
    bool IsGrouped() const noexcept { return m_groups != nullptr; }

    winrt::ItemsSourceView ItemsSourceView() const noexcept { return m_itemsSourceView; }
    TableViewRowMetadataProvider RowMetadata() const noexcept { return m_rowMetadata; }

    // The identity a group with this key has, as group expansion is keyed. nullopt when not grouped.
    std::optional<winrt::hstring> TryGetGroupIdentityForKey(winrt::IInspectable const& key) const;
    struct GroupInfo
    {
        winrt::hstring Identity;
        winrt::IInspectable Key{ nullptr };
        // The group object the grouped adapter tracks.
        winrt::IInspectable Group{ nullptr };
    };
    std::vector<GroupInfo> CurrentGroups() const;

    // The view's items, and the data rows shown before expansion (every node for a tree).
    uint32_t SourceRowCount() const;
    uint32_t DataRowCount() const;

    // Raised after the projection was rebuilt: a different ItemsSourceView or row-metadata provider, or a
    // tree or group set rebuilt in place.
    void SetProjectionChangedHandler(std::function<void()> handler) { m_projectionChanged = std::move(handler); }

    // A row's identity: the item's COM identity, unique among live objects.
    static winrt::hstring ObjectIdentity(winrt::IInspectable const& item);

private:
    enum class Kind { None, Flat, Grouped, Tree, GroupedTree };

    // Rebuilds for the current declaration; coalesces requests raised while it runs.
    void Update();
    void UpdateCore();
    void UpdateTree(Kind kind);
    std::vector<winrt::IInspectable> ReadViewItems() const;
    void ResliceTreeGroups();
    void ReleaseTree();
    void OnViewChanged();
    void OnGroupsChanged();
    void RaiseProjectionChanged() const;

    winrt::Microsoft::UI::Xaml::Data::ICollectionView m_view{ nullptr };
    winrt::event_token m_viewChangedToken{};
    std::shared_ptr<TableViewCollectionViewGroups> m_groups{};

    ShapingHelpers::KeySelector m_keySelector{ nullptr };
    ShapingHelpers::KeySelector m_parentKeySelector{ nullptr };
    // Bumped by every SetHierarchy / ClearHierarchy, so a rebuild reading items (app code) can tell the
    // relation it is building was replaced.
    uint64_t m_declarationGeneration{ 0 };
    bool m_resetTreeIntent{ false };

    Kind m_kind{ Kind::None };
    winrt::ItemsSourceView m_flatView{ nullptr };
    winrt::ItemsSourceView m_itemsSourceView{ nullptr };
    TableViewRowMetadataProvider m_rowMetadata{};
    std::shared_ptr<GroupedSourceAdapter> m_groupedAdapter{};
    std::shared_ptr<HierarchicalSourceAdapter> m_hierarchicalAdapter{};

    // Tree under a grouped view: one group per bucket of roots, whose items are the roots and their
    // visible descendants (re-sliced from the tree on every expansion change).
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> m_treeGroupSource{ nullptr };
    std::vector<winrt::com_ptr<ShapedGroup>> m_treeGroups{};
    std::unordered_map<winrt::hstring, winrt::com_ptr<ShapedGroup>> m_treeGroupByIdentity{};
    std::unordered_map<void*, size_t> m_rootBucket{};
    uint64_t m_treeGeneration{ 0 };
    bool m_reslicing{ false };

    bool m_updating{ false };
    bool m_updatePending{ false };
    std::function<void()> m_projectionChanged{};
};
