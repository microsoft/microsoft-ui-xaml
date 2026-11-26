// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <winrt/Microsoft.UI.Dispatching.h>

#include "TableViewRowInfo.h"
#include "ShapingDescriptions.h"
#include "ParentKeyIndex.h"
#include "RowExpansionModel.h"

// Turns the ICollectionView TableView displays into the flat row list its single ItemsRepeater
// realizes, the role WPF's ItemContainerGenerator plays for an ItemsControl. It never sorts, filters or
// groups: the view does (TableView wraps a plain collection in an internal CollectionViewSource). It
// only lays the view out as rows:
//   - flat: the view itself, so the view's fine-grained changes reach the repeater unchanged;
//   - grouped: a header row (GroupedEntry) plus the data rows of each of the view's CollectionGroups;
//   - tree: ItemKeyPath / ParentKeyPath nest the view's items (ParentKeyIndex). Siblings keep the view's
//     order, and under a grouped view each root is placed in the group that holds it.
// Group and node expansion state is this control's, never the view's: one view can feed several controls.
//
// UI-thread affine. Owned by one TableView.
class TableViewRowFlattener : public std::enable_shared_from_this<TableViewRowFlattener>
{
public:
    explicit TableViewRowFlattener(winrt::Microsoft::UI::Xaml::Data::ICollectionView const& view);
    ~TableViewRowFlattener();

    // Subscribes to the view and builds the first rows.
    void Start();

    winrt::Microsoft::UI::Xaml::Data::ICollectionView const& View() const noexcept { return m_view; }

    // Nests the rows: an item whose parent key equals another item's key is its child. Throws E_INVALIDARG
    // (keeping the current rows) on a duplicate or null key, a self-parent or a cycle. Re-declaring resets
    // the rows' expansion state.
    void SetHierarchy(ShapingHelpers::KeySelector const& keySelector, ShapingHelpers::KeySelector const& parentKeySelector);
    void ClearHierarchy();
    bool HasHierarchy() const noexcept { return static_cast<bool>(m_keySelector); }

    // True when the rows include group headers.
    bool IsGrouped() const noexcept { return m_viewGroups != nullptr; }

    winrt::ItemsSourceView ItemsSourceView() const noexcept { return m_itemsSourceView; }
    // Null for flat rows: a row is its item. A new provider whenever the kind of rows changes.
    TableViewRowMetadataProvider RowMetadata();

    // The identity a group with this key has, as group expansion is keyed. nullopt when not grouped.
    std::optional<winrt::hstring> TryGetGroupIdentityForKey(winrt::IInspectable const& key) const;
    struct GroupInfo
    {
        winrt::hstring Identity;
        winrt::IInspectable Key{ nullptr };
    };
    std::vector<GroupInfo> CurrentGroups() const;

    // The view's items, and the data rows shown before expansion (every node for a tree).
    uint32_t SourceRowCount() const;
    uint32_t DataRowCount() const;

    // Raised after the rows were rebuilt: a different ItemsSourceView or row-metadata provider, or a
    // tree or group set rebuilt in place.
    void SetRowsChangedHandler(std::function<void()> handler) { m_rowsChanged = std::move(handler); }

    // A row's identity: the item's COM identity, unique among live objects.
    static winrt::hstring ObjectIdentity(winrt::IInspectable const& item);

    // --- Group headers (grouped rows) ---

    winrt::ItemsSourceView GroupedRows() const noexcept { return m_groupRowsView; }
    // Keyed by group identity (GroupedEntry::Identity).
    bool IsGroupExpanded(winrt::hstring const& groupIdentity) const;
    void SetGroupExpanded(winrt::hstring const& groupIdentity, bool isExpanded);
    // Move the baseline, so groups that do not exist yet arrive in that state too.
    void ExpandAllGroups();
    void CollapseAllGroups();
    // The groups the grouped rows were last built from, in display order.
    std::vector<GroupInfo> const& LiveGroups() const noexcept { return m_liveGroups; }
    GroupInfo const* TryGetLiveGroup(winrt::hstring const& groupIdentity) const;

    // --- Tree nodes (tree rows) ---

    winrt::ItemsSourceView TreeRows() const noexcept { return m_treeRowsView; }

    struct NodeRow
    {
        winrt::IInspectable Item{ nullptr };
        winrt::hstring NodeKey;
        // Node key of the parent row; empty for roots.
        winrt::hstring ParentKey;
        // 0-based (roots render flush); TableViewRowInfo::Level is Depth + 1.
        int32_t Depth{ 0 };
        int32_t ChildCount{ 0 };
        // 1-based position in the sibling set and its size. Under grouping, a root's sibling set is its group.
        int32_t SiblingIndex{ 0 };
        int32_t SiblingCount{ 0 };
        bool HasChildren{ false };
        bool IsExpanded{ false };
        // Kept only because a descendant matched the active filter.
        bool IsContext{ false };
    };

    // Index into TreeRows.
    NodeRow const* TryGetNodeRow(int32_t index) const;
    bool TryGetIndexForNodeKey(winrt::hstring const& nodeKey, int32_t& index) const;
    // For grouped trees, whose row indices include headers. Sound because an item occupies at most one row.
    NodeRow const* TryGetNodeRowForItem(winrt::IInspectable const& item) const;

    // Addressed by node key ("node:" + app key lookup form). On a context row, moves only the filter
    // overlay, never intent.
    bool IsNodeExpanded(winrt::hstring const& nodeKey) const;
    void SetNodeExpanded(winrt::hstring const& nodeKey, bool isExpanded);
    void ExpandAllNodes();
    void CollapseAllNodes();
    void ExpandSubtree(winrt::hstring const& nodeKey);

    // Positive prefix test, so a stale or foreign key cannot become node intent.
    static bool IsNodeKey(winrt::hstring const& key);

private:
    enum class Kind { None, Flat, Grouped, Tree, GroupedTree };

    // Rebuilds for the current declaration; coalesces requests raised while it runs.
    void Update();
    // Under a grouped view: rebuilds on a later dispatcher turn, once the view has finished moving items.
    void ScheduleUpdate();
    // A tree rebuilds at most once per dispatcher turn; false when no dispatcher can run the next turn.
    bool QueueTreeTurnEnd();
    void OnTreeTurnEnd();
    void UpdateCore();
    void UpdateTree(Kind kind);
    std::vector<winrt::IInspectable> ReadViewItems() const;
    void ReleaseTree();
    void OnViewChanged(winrt::Windows::Foundation::Collections::CollectionChange change, uint32_t index);
    void RaiseRowsChanged() const;
    void AssertOnUiThread() const;
    void SetKind(Kind kind, winrt::ItemsSourceView const& rows);

    // --- The view's groups ---

    struct ViewGroup
    {
        winrt::hstring Identity;
        winrt::IInspectable Key{ nullptr };
        // The group's GroupItems; null for a group that is only enumerable, read once into Snapshot.
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> Items{ nullptr };
        std::vector<winrt::IInspectable> Snapshot;
        // The group object when it is itself an ICollectionViewGroup: a multi-level view's group, whose
        // GroupItems are its subgroups above the bottom level.
        winrt::Microsoft::UI::Xaml::Data::ICollectionViewGroup Inner{ nullptr };
    };
    // Follows the view's CollectionGroups object; true when it changed (the view started or stopped grouping).
    bool AttachViewGroups();
    void OnViewGroupsChanged(winrt::Windows::Foundation::Collections::CollectionChange change);
    void ReadViewGroupsNow() noexcept;
    void ReadViewGroups();
    void OnViewGroupSetChanged();
    std::unordered_map<uintptr_t, winrt::hstring> GroupIdentityByItem() const;
    static winrt::hstring IdentityForKey(winrt::IInspectable const& key);

    // --- Grouped rows ---

    enum class GroupSource { None, ViewGroups, TreeGroups };
    struct GroupSlice
    {
        winrt::hstring Identity;
        winrt::IInspectable Key{ nullptr };
        std::vector<winrt::IInspectable> Items;
        // Roots under a tree; -1 means "count the rows".
        int32_t ChildCount{ -1 };
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> Observed{ nullptr };
        // Depth in a multi-level view; a group above the bottom level shows its subgroups, not its items.
        int32_t Level{ 0 };
        bool IsBottomLevel{ true };
    };
    static void AppendSubgroupSlices(
        winrt::Microsoft::UI::Xaml::Data::ICollectionViewGroup const& group,
        GroupSlice&& slice,
        std::vector<GroupSlice>& slices);
    void AttachGroupRows(GroupSource source);
    void DetachGroupRows();
    std::vector<GroupSlice> ReadGroupSlices() const;
    std::vector<winrt::IInspectable> ReadGroupItems(winrt::hstring const& groupIdentity) const;
    void RebuildGroupRows();
    void OnGroupExpansionChanged(ShapingHelpers::RowExpansionModel::Change const& change);
    // Splices one group's rows in or out; false when the rows are not shaped as expected (Rebuild then).
    bool TryApplyGroupExpansionSplice(winrt::hstring const& intentKey, bool expand);
    void UnsubscribeFromGroupItems();
    static winrt::hstring GroupIntentKey(winrt::hstring const& groupIdentity);

    // --- Tree rows ---

    void SetTreeIndex(std::shared_ptr<ShapingHelpers::ParentKeyIndex> index, std::vector<size_t> rootSegments);
    void ClearTreeIndex();
    void ResetNodeIntentQuietly();
    void RebuildTreeRows();
    void OnNodeExpansionChanged(ShapingHelpers::RowExpansionModel::Change const& change);
    void ApplySingleNodeToggle(winrt::hstring const& nodeKey, bool isExpanded);
    bool EffectiveExpanded(winrt::hstring const& nodeKey, bool hasChildren, bool isContext) const;
    bool EffectiveExpandedForKey(winrt::hstring const& nodeKey) const;
    // Explicit stack, not recursion, so depth is unbounded.
    void EmitNodeRange(
        std::vector<winrt::IInspectable> const& siblings,
        size_t begin,
        size_t end,
        winrt::hstring const& parentKey,
        int32_t depth,
        std::vector<winrt::IInspectable>& built,
        std::vector<NodeRow>& nodeRows) const;
    // Splices one node's visible subtree in or out; false (Rebuild then) when the key does not resolve,
    // the rows and node rows disagree, or the run is long.
    bool TryApplyNodeExpansionSplice(winrt::hstring const& nodeKey, bool expand);
    // Publishes rebuilt rows as the few removals and insertions that turn the current rows into them; false
    // when that would be many (a Reset then).
    bool TryPublishTreeRowsAsSplice(std::vector<winrt::IInspectable> const& built, std::vector<NodeRow>& nodeRows);
    // Applies one view Insert, Remove or Replace to the tree index and splices the visible rows, without reading
    // the other rows; false (a rebuild then) for what it does not localize: a row that adopts orphans, a row with
    // children leaving, object keys, a duplicate, a grouped tree.
    bool TryApplyTreeChange(winrt::Windows::Foundation::Collections::CollectionChange change, uint32_t index);
    // keys: the new row's node key and declared parent key when already read (a Replace reads them first).
    bool TryInsertTreeNode(uint32_t viewIndex, std::pair<std::wstring, std::wstring>* keys = nullptr);
    // A Replace's new row can take the old one's place: read and checked before the old row is removed. isSameRow: the
    // same object with the same keys, refreshed in place by TryRefreshTreeRow.
    bool TryReadReplacementKeys(uint32_t viewIndex, std::pair<std::wstring, std::wstring>& keys, bool& isSameRow);
    bool TryRefreshTreeRow(uint32_t viewIndex);
    bool TryRemoveTreeNode(uint32_t viewIndex);
    int32_t FindNodeRowIndex(winrt::IInspectable const& item, int32_t from = 0) const;
    // Renumbers the visible rows of one sibling list (roots when parentRow is -1) after it changed.
    void RenumberSiblingRows(int32_t parentRow, int32_t depth, int32_t siblingCount);
    // Tree rows, node rows and index maps are only correct as a set; mutate only through these. False when a handler
    // of a row change replaced or released the tree (see m_treeGeneration): the caller stops, and the rebuild that
    // the new tree asked for (m_treeRowsPending) or the release publishes the rows.
    bool InsertTreeRows(int32_t index, std::vector<winrt::IInspectable> const& items, std::vector<NodeRow> const& nodeRows);
    bool RemoveTreeRows(int32_t index, int32_t count);
    bool IsSameTree(uint64_t generation, winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> const& rows) const noexcept;
    void OnTreeRowsPublished();
    void EnsureItemIndex() const;
    void EnsureNodeKeyIndex() const;
    template <typename Fn>
    bool MutateNodeExpansionQuietly(Fn&& fn);

    // --- Grouped tree ---

    void ResliceTreeGroups();

    winrt::Microsoft::UI::Xaml::Data::ICollectionView m_view{ nullptr };
    winrt::event_token m_viewChangedToken{};
    winrt::Microsoft::UI::Xaml::Data::INotifyPropertyChanged m_viewNotify{ nullptr };
    winrt::event_token m_viewPropertyChangedToken{};
    // The view implements ICollectionViewShaping: it raises a regroup as one Reset of complete groups.
    bool m_isShapingView{ false };
    // Affinity assertion only (chk).
    winrt::weak_ref<winrt::Microsoft::UI::Dispatching::DispatcherQueue> m_uiQueue{ nullptr };

    ShapingHelpers::KeySelector m_keySelector{ nullptr };
    ShapingHelpers::KeySelector m_parentKeySelector{ nullptr };
    // Bumped by every SetHierarchy / ClearHierarchy, so a rebuild reading items (app code) can tell the
    // relation it is building was replaced.
    uint64_t m_declarationGeneration{ 0 };
    bool m_resetTreeIntent{ false };

    Kind m_kind{ Kind::None };
    winrt::ItemsSourceView m_flatView{ nullptr };
    winrt::ItemsSourceView m_itemsSourceView{ nullptr };
    // Weak: the provider holds this flattener. TableView keeps the current one alive.
    std::weak_ptr<ITableViewRowMetadataProvider> m_rowMetadata{};
    bool m_rowMetadataStale{ true };

    bool m_updating{ false };
    bool m_updatePending{ false };
    bool m_updateQueued{ false };
    // Tree mode: rebuilt this turn / a view change waits for the next turn / the turn end is queued.
    bool m_treeUpdatedThisTurn{ false };
    bool m_treeUpdatePending{ false };
    bool m_treeTurnQueued{ false };
    uint64_t m_updateCount{ 0 };
    std::function<void()> m_rowsChanged{};

    // The view's CollectionGroups; null when the view is not grouped.
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> m_viewGroups{ nullptr };
    winrt::event_token m_viewGroupsChangedToken{};
    std::vector<ViewGroup> m_groups;
    bool m_readingGroups{ false };
    bool m_readGroupsPending{ false };
    bool m_readGroupsQueued{ false };

    GroupSource m_groupSource{ GroupSource::None };
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> m_groupRows{
        winrt::single_threaded_observable_vector<winrt::IInspectable>() };
    winrt::ItemsSourceView m_groupRowsView{ nullptr };
    // Keyed by group identity, so it survives a view that recreates its groups.
    ShapingHelpers::RowExpansionModel m_groupExpansion;
    std::vector<GroupInfo> m_liveGroups;
    // The grouped rows hold subgroup headers, so one group's rows are not a single span.
    bool m_hasSubgroupRows{ false };
    struct GroupItemsSubscription
    {
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> Items{ nullptr };
        winrt::event_token Token{};
    };
    std::vector<GroupItemsSubscription> m_groupItemsSubscriptions;
    // A change raised synchronously inside ReplaceAll is remembered and run once after unwind.
    bool m_groupRowsInFlight{ false };
    bool m_groupRowsPending{ false };
    bool m_groupRowsProfiled{ false };

    // Owned here: a plain tree's index and its structure are updated in place by TryApplyTreeChange.
    std::shared_ptr<ShapingHelpers::ParentKeyIndex> m_treeIndex;
    std::shared_ptr<ShapingHelpers::ParentStructure> m_treeStructure;
    // The tree index describes the view's current items (a plain tree, built or kept current incrementally).
    bool m_treeIndexIsCurrent{ false };
    bool m_treeChangeInFlight{ false };
    // Keys of rows removed in place this turn; their expansion intent is dropped at the turn's end unless they returned.
    std::vector<winrt::hstring> m_removedTreeKeys;
    // Each row's keys as last read, so a rebuild after an Insert, Remove or Replace reads only the rows that
    // change brought in. Emptied by a Reset, a new declaration and a released tree.
    ShapingHelpers::NodeKeyCache m_treeKeyCache;
    // Structure intent was last pruned against (see SetTreeIndex).
    std::weak_ptr<const ShapingHelpers::ParentStructure> m_prunedForStructure;
    std::vector<size_t> m_rootSegments;
    // Context rows collapsed under the current filter; never intent.
    std::unordered_set<winrt::hstring> m_filterOverlayCollapsed;
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> m_treeRows{
        winrt::single_threaded_observable_vector<winrt::IInspectable>() };
    winrt::ItemsSourceView m_treeRowsView{ nullptr };
    // Index-aligned 1:1 with m_treeRows.
    std::vector<NodeRow> m_nodeRows;
    // Lazy caches over m_nodeRows: invalidated by mutations, rebuilt in one pass on lookup.
    mutable std::unordered_map<winrt::hstring, int32_t> m_indexByNodeKey;
    mutable bool m_indexByNodeKeyValid{ true };
    mutable std::unordered_map<void*, int32_t> m_indexByItem;
    mutable bool m_indexByItemValid{ false };
    // Collapsed baseline, so the first build does not realize the whole tree.
    ShapingHelpers::RowExpansionModel m_nodeExpansion;
    bool m_suppressNodeExpansionChanged{ false };
    bool m_nodeExpansionChangedWhileSuppressed{ false };
    bool m_treeRowsInFlight{ false };
    // A tree rebuild for a view change may publish a splice instead of a Reset (see UpdateTree).
    bool m_isRebuildingTreeForView{ false };
    bool m_treeRowsPending{ false };
    // Set by ClearTreeIndex; notifying loops check it after each mutation (a handler may retract the tree).
    bool m_treeDetached{ false };
    // Bumped by SetTreeIndex and ClearTreeIndex (so by ReleaseTree too). A handler of a row change can release the
    // tree and declare a new one before it returns, which clears m_treeDetached again and empties m_nodeRows: the
    // notifying loops compare this and the m_treeRows they started on, not only m_treeDetached.
    uint64_t m_treeGeneration{ 0 };

    // Tree under a grouped view: one group per bucket of roots, whose rows are the roots and their visible
    // descendants (re-sliced from the tree rows on every expansion change).
    struct TreeGroup
    {
        winrt::hstring Identity;
        winrt::IInspectable Key{ nullptr };
        std::vector<winrt::IInspectable> Items;
        int32_t RootCount{ 0 };
    };
    std::vector<TreeGroup> m_treeGroups;
    std::unordered_map<void*, size_t> m_rootBucket;
    bool m_sliceTreeIntoGroups{ false };
};
