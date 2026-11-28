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
#include "RowExpansionModel.h"

// Turns the ICollectionView TableView displays into the flat row list its single ItemsRepeater
// realizes, the role WPF's ItemContainerGenerator plays for an ItemsControl. It never sorts, filters or
// groups: the view does (TableView wraps a plain collection in an internal CollectionViewSource). It
// only lays the view out as rows:
//   - flat: the view itself, so the view's fine-grained changes reach the repeater unchanged;
//   - grouped: a header row (GroupedEntry) plus the data rows of each of the view's CollectionGroups;
//   - tree: the view's nodes (ICollectionViewHierarchy.CollectionNodes), each expanded node followed by its children.
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

    // True when the rows include group headers.
    bool IsGrouped() const noexcept { return m_viewGroups != nullptr; }
    // True when the view's groups have subgroups (more than one GroupDescription), collapsed or not.
    bool HasSubgroups() const noexcept { return IsGrouped() && m_hasSubgroupRows; }

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

    // The view's items, and the data rows shown before expansion (every kept node of a tree).
    uint32_t SourceRowCount() const;
    uint32_t DataRowCount() const;

    // True when the rows are the view's tree.
    bool HasHierarchy() const noexcept { return m_kind == Kind::Tree; }

    // Raised after the rows were rebuilt: a different ItemsSourceView or row-metadata provider, or a
    // group set rebuilt in place.
    void SetRowsChangedHandler(std::function<void()> handler) { m_rowsChanged = std::move(handler); }
    // Raised when tree rows' metadata (sibling position, child count) changed while the rows themselves did not.
    void SetRowMetadataChangedHandler(std::function<void()> handler) { m_rowMetadataChanged = std::move(handler); }

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
        // 1-based position in the sibling set and its size.
        int32_t SiblingIndex{ 0 };
        int32_t SiblingCount{ 0 };
        bool HasChildren{ false };
        bool IsExpanded{ false };
        // Kept only because a descendant passes the view's filter.
        bool IsContext{ false };
        winrt::Microsoft::UI::Xaml::Data::ICollectionViewNode Node{ nullptr };
        // The node's PropertyChanged (HasChildren), followed while the row exists.
        winrt::event_token NodeToken{};
        // The node's Children, read and followed only while the row is expanded.
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> Children{ nullptr };
        winrt::event_token ChildrenToken{};
    };

    // Index into TreeRows.
    NodeRow const* TryGetNodeRow(int32_t index) const;
    bool TryGetIndexForNodeKey(winrt::hstring const& nodeKey, int32_t& index) const;

    // Addressed by node key ("node:" + the node's Key). On a context row, moves only the filter overlay, never
    // intent.
    bool IsNodeExpanded(winrt::hstring const& nodeKey) const;
    void SetNodeExpanded(winrt::hstring const& nodeKey, bool isExpanded);
    void ExpandAllNodes();
    void CollapseAllNodes();
    void ExpandSubtree(winrt::hstring const& nodeKey);
    // TableView.ExpandRow / CollapseRow: the item's node; expanding also expands its ancestors. False when the item
    // has no node.
    bool SetItemExpanded(winrt::IInspectable const& item, bool isExpanded);

    // Positive prefix test, so a stale or foreign key cannot become node intent.
    static bool IsNodeKey(winrt::hstring const& key);

private:
    enum class Kind { None, Flat, Grouped, Tree };

    // Rebuilds for the view's current grouping; coalesces requests raised while it runs.
    void Update();
    void UpdateCore();
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
    // Follows the view's CollectionGroups object and updates the rows when it changed (the view started or stopped
    // grouping). Deferred (m_attachViewGroupsPending) while the groups are being attached, read or published or the
    // rows updated: those walks run app code that can raise PropertyChanged("CollectionGroups").
    void AttachViewGroups();
    // True when the CollectionGroups object changed.
    bool AttachViewGroupsNow();
    bool IsBusyWithGroups() const noexcept;
    bool CanAttachViewGroupsInUpdate() const noexcept;
    // Runs a deferred AttachViewGroups (and its Update) once nothing that defers it is on the stack.
    void RunPendingAttachViewGroups();
    // A view that returns a new CollectionGroups on every read and raises PropertyChanged("CollectionGroups") during
    // each read would re-attach forever. Each loop re-attaches at most this many times, then drops the request and keeps
    // the rows of the groups attached last.
    static constexpr int c_maxReattachesPerLoop = 8;
    // False (and the pending re-attach dropped and logged) once 'reattaches' reached c_maxReattachesPerLoop.
    bool WithinReattachCap(int reattaches) noexcept;
    void OnViewGroupsChanged(winrt::Windows::Foundation::Collections::CollectionChange change);
    void ReadViewGroupsNow() noexcept;
    void ReadViewGroups();
    static winrt::hstring IdentityForKey(winrt::IInspectable const& key);

    // --- Grouped rows ---

    struct GroupSlice
    {
        winrt::hstring Identity;
        winrt::IInspectable Key{ nullptr };
        std::vector<winrt::IInspectable> Items;
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> Observed{ nullptr };
        // Depth in a multi-level view; a group above the bottom level shows its subgroups, not its items.
        int32_t Level{ 0 };
        bool IsBottomLevel{ true };
    };
    static void AppendSubgroupSlices(
        winrt::Microsoft::UI::Xaml::Data::ICollectionViewGroup const& group,
        GroupSlice&& slice,
        std::vector<GroupSlice>& slices);
    void AttachGroupRows();
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

    // Follows the view's CollectionNodes object; true when it changed.
    bool AttachViewNodesNow();
    void DetachTreeRows();
    void RebuildTreeRows();
    void ApplyTreeRows(std::vector<winrt::IInspectable>&& items, std::vector<NodeRow>&& rows);
    // Appends the rows of the nodes in a list and, under each expanded node, its children; explicit stack.
    void EmitNodeRows(
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> const& list,
        uint32_t begin,
        uint32_t end,
        winrt::hstring const& parentKey,
        int32_t depth,
        std::vector<winrt::IInspectable>& items,
        std::vector<NodeRow>& rows);
    NodeRow MakeNodeRow(winrt::Microsoft::UI::Xaml::Data::ICollectionViewNode const& node, winrt::hstring const& parentKey, int32_t depth, int32_t siblingIndex, int32_t siblingCount);
    static winrt::hstring NodeKeyFor(winrt::Microsoft::UI::Xaml::Data::ICollectionViewNode const& node, winrt::IInspectable const& item = nullptr);
    bool EffectiveExpanded(winrt::hstring const& nodeKey, bool hasChildren, bool isContext) const;
    void SubscribeToNode(NodeRow& row);
    void SubscribeToChildren(NodeRow& row);
    // Both of a row's subscriptions.
    void UnsubscribeFromChildren(NodeRow& row);
    void OnNodeHasChildrenChanged(winrt::Microsoft::UI::Xaml::Data::ICollectionViewNode const& node);
    void OnNodeListChanged(
        winrt::hstring const& parentKey,
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> const& list,
        winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args);
    bool TryApplyNodeListChange(
        winrt::hstring const& parentKey,
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> const& list,
        winrt::Windows::Foundation::Collections::CollectionChange change,
        uint32_t index);
    void OnViewItemsChanged(winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args);
    void OnViewItemChanged(uint32_t index);
    // The rows no longer follow the view: rebuilt at the end of the view's change, or on the next turn.
    void QueueTreeRowsRebuild();
    bool TryGetIndexForNode(winrt::Microsoft::UI::Xaml::Data::ICollectionViewNode const& node, int32_t& index) const;
    void OnNodeExpansionChanged(ShapingHelpers::RowExpansionModel::Change const& change);
    bool TryApplyNodeExpansionSplice(winrt::hstring const& nodeKey);
    // The row after a row's visible subtree.
    int32_t SubtreeEnd(int32_t row) const;
    // Renumbers the rows of one sibling list (roots when parentRow is -1) and the parent's child count.
    void RenumberSiblingRows(int32_t parentRow);
    void InsertTreeRows(int32_t index, std::vector<winrt::IInspectable> const& items, std::vector<NodeRow>&& rows);
    void RemoveTreeRows(int32_t index, int32_t count);
    void EnsureNodeKeyIndex() const;
    void RaiseRowMetadataChanged() const;
    void QueueForgetRemovedKeys();

    winrt::Microsoft::UI::Xaml::Data::ICollectionView m_view{ nullptr };
    winrt::Microsoft::UI::Xaml::Data::INotifyPropertyChanged m_viewNotify{ nullptr };
    winrt::event_token m_viewPropertyChangedToken{};
    // The view implements ICollectionViewShaping: it raises a regroup as one Reset of complete groups.
    bool m_isShapingView{ false };
    // Affinity assertion only (chk).
    winrt::weak_ref<winrt::Microsoft::UI::Dispatching::DispatcherQueue> m_uiQueue{ nullptr };

    Kind m_kind{ Kind::None };
    winrt::ItemsSourceView m_flatView{ nullptr };
    winrt::ItemsSourceView m_itemsSourceView{ nullptr };
    // Weak: the provider holds this flattener. TableView keeps the current one alive.
    std::weak_ptr<ITableViewRowMetadataProvider> m_rowMetadata{};
    bool m_rowMetadataStale{ true };

    bool m_updating{ false };
    bool m_updatePending{ false };
    std::function<void()> m_rowsChanged{};
    std::function<void()> m_rowMetadataChanged{};

    // The view's CollectionGroups; null when the view is not grouped.
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> m_viewGroups{ nullptr };
    winrt::event_token m_viewGroupsChangedToken{};
    // Replaced while app code runs (a group VectorChanged reread): loops that call app code iterate a copy.
    std::vector<ViewGroup> m_groups;
    bool m_attachingViewGroups{ false };
    bool m_attachViewGroupsPending{ false };
    // A CollectionNodes change, deferred the same way (and while tree rows are spliced).
    bool m_attachViewNodesPending{ false };
    bool m_runningPendingAttach{ false };
    bool m_readingGroups{ false };
    bool m_readGroupsPending{ false };
    bool m_readGroupsQueued{ false };
    // Bumped whenever m_viewGroups is replaced or cleared, so a queued read of the old groups is dropped.
    uint32_t m_viewGroupsGeneration{ 0 };

    bool m_groupRowsAttached{ false };
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

    winrt::Microsoft::UI::Xaml::Data::ICollectionViewHierarchy m_hierarchy{ nullptr };
    // The view's CollectionNodes; null while the view is flat.
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> m_viewNodes{ nullptr };
    winrt::event_token m_viewNodesChangedToken{};
    winrt::event_token m_viewItemsChangedToken{};
    bool m_treeRowsAttached{ false };
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> m_treeRows{
        winrt::single_threaded_observable_vector<winrt::IInspectable>() };
    winrt::ItemsSourceView m_treeRowsView{ nullptr };
    // Index-aligned 1:1 with m_treeRows.
    std::vector<NodeRow> m_nodeRows;
    mutable std::unordered_map<winrt::hstring, int32_t> m_indexByNodeKey;
    // By node object (ICollectionViewNode ABI pointer); rebuilt with m_indexByNodeKey.
    mutable std::unordered_map<void*, int32_t> m_indexByNode;
    mutable bool m_indexByNodeKeyValid{ false };
    // Collapsed baseline, so the first build realizes only the roots.
    ShapingHelpers::RowExpansionModel m_nodeExpansion;
    // Context rows (shown for a filter match below them) start expanded; collapsing one is recorded here, not as
    // intent.
    std::unordered_set<winrt::hstring> m_filterOverlayCollapsed;
    // Keys of rows removed this turn: their intent is dropped at the turn's end unless they returned (a live move is a
    // remove and an insert).
    std::vector<winrt::hstring> m_removedNodeKeys;
    bool m_forgetQueued{ false };
    // A change raised while the tree rows are being changed is remembered and applied as one rebuild after.
    bool m_treeRowsInFlight{ false };
    bool m_treeRowsPending{ false };
    bool m_treeRowsStale{ false };
    bool m_treeRebuildQueued{ false };
    // A handler that changes the tree on every rebuild: after this many passes the next one waits a turn.
    static constexpr int c_maxTreeRebuildPasses = 8;
};
