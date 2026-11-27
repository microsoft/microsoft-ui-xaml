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
//   - grouped: a header row (GroupedEntry) plus the data rows of each of the view's CollectionGroups.
// Group expansion state is this control's, never the view's: one view can feed several controls.
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

    // The view's items.
    uint32_t SourceRowCount() const;

    // Raised after the rows were rebuilt: a different ItemsSourceView or row-metadata provider, or a
    // group set rebuilt in place.
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

private:
    enum class Kind { None, Flat, Grouped };

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

    // The view's CollectionGroups; null when the view is not grouped.
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> m_viewGroups{ nullptr };
    winrt::event_token m_viewGroupsChangedToken{};
    // Replaced while app code runs (a group VectorChanged reread): loops that call app code iterate a copy.
    std::vector<ViewGroup> m_groups;
    bool m_attachingViewGroups{ false };
    bool m_attachViewGroupsPending{ false };
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

};
