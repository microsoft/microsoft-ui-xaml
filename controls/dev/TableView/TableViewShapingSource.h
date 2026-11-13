// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include <functional>
#include <memory>
#include <optional>

#include <winrt/Microsoft.UI.Dispatching.h>
#include "TableViewRowInfo.h"
#include "TableViewTipTypes.h"
#include "ShapingDescriptions.h"
#include "RowIdentity.h"

class ShapedItemsSource;

// TableView's internal facade over the shaping engine. Not a projected type: TableView wraps every
// ItemsSource (a plain collection or an ICollectionView) in one of these so the control has exactly
// one row pipeline. Apps shape data with CollectionViewSource instead.
//
// It owns no projection. Filtering, sorting, grouping, source subscriptions, incremental change
// application, and identity tracking all live in ShapedItemsSource below it; this class turns the
// shape the engine produced into an ItemsSourceView plus a row-metadata provider, and tells the
// owning TableView when the projection it cached is no longer the one being shown.
//
// Reference-tracked (like ItemsRepeater's ItemsSourceView) so final_release marshals the teardown
// cascade (~TableViewShapingSource -> m_engine drop -> ~ShapedItemsSource -> revoke) to the owning
// UI thread, and tracker_ref members let the GC walk this native object's strong refs into managed
// rows. That is why ShapedItemsSource needs no thread guard of its own.
class TableViewShapingSource :
    public ReferenceTracker<TableViewShapingSource, reference_tracker_implements_t<winrt::IInspectable>::type>
{
public:
    ~TableViewShapingSource();
    explicit TableViewShapingSource(winrt::IInspectable const& items);

    static winrt::com_ptr<TableViewShapingSource> Create(winrt::IInspectable const& items);

    winrt::IInspectable Source() const;

    void GroupBy(ShapingHelpers::KeySelector const& key, RowIdentity::IdentitySelector const& groupIdentitySelector);
    // Builds a tree from each item's key and its parent's key (TableView.ItemKeyPath / ParentKeyPath).
    void ParentBy(ShapingHelpers::KeySelector const& keySelector, ShapingHelpers::KeySelector const& parentKeySelector);
    void ClearParentBy();
    // Makes sortAxisToken the only item sort level in one reshape (None clears every level).
    void SortExclusive(winrt::hstring const& sortAxisToken, ShapingHelpers::KeySelector const& key, winrt::hstring const& sortMemberPath, winrt::SortDirection direction);
    // Clears every item sort level.
    void ClearSort();
    void ClearSort(winrt::hstring const& sortAxisToken);
    void Refresh();
    // A refresh deferral for the owning control (UI thread). Each BeginDeferRefresh must be paired
    // with one EndDeferRefresh; the outermost End reshapes once if anything changed, and may throw
    // what that reshape throws (or E_ILLEGAL_METHOD_CALL when called from inside an app delegate,
    // without ending the deferral).
    void BeginDeferRefresh();
    void EndDeferRefresh();
    // Ends one deferral on a later (low-priority) dispatcher turn, posting again while the end is
    // refused as re-entrant, up to a retry cap. With no queue to post to, ends it inline.
    void EndDeferRefreshLater();
    // The group identity key maps to under the current GroupBy, computed as bucketing computes it.
    // nullopt without GroupBy, or when the key has no identity.
    std::optional<winrt::hstring> TryGetGroupIdentityForKey(winrt::IInspectable const& key) const;
    // The groups currently projected, in display order; empty when not grouped. Group is the object
    // the grouped adapter tracks (GroupedSourceAdapter::ResolveLiveGroupByIdentity returns the same
    // object for Identity).
    struct GroupInfo
    {
        winrt::hstring Identity;
        winrt::IInspectable Key{ nullptr };
        winrt::IInspectable Group{ nullptr };
    };
    std::vector<GroupInfo> CurrentGroups() const;
    // Throws E_ILLEGAL_METHOD_CALL when ending the outermost deferral now would reshape from
    // inside an app delegate.
    void ThrowIfEndDeferRefreshReentrant() const;
    // Whether any item sort level is declared.
    bool HasItemSort() const;
    // The active sort axes in precedence order, so the owner can detect an axis it does not own
    // and, when that axis names a property, attribute it to a column.
    struct ActiveSortAxisInfo
    {
        winrt::hstring AxisToken;
        // Empty when the axis was declared with a key selector no property path expresses.
        winrt::hstring SortMemberPath;
        winrt::SortDirection Direction{ winrt::SortDirection::None };
    };
    std::vector<ActiveSortAxisInfo> ActiveSortAxisInfos() const;

    // The axis token a path-declared sort owns.
    static winrt::hstring SortAxisTokenForPath(winrt::hstring const& sortMemberPath);
    // A key selector that reads path through a binding; throws E_INVALIDARG for an empty path.
    static ShapingHelpers::KeySelector MakePathKeySelector(winrt::hstring const& path);

    bool IsLiveShaping() const;

    // UI-thread affine after construction/binding: shaping verbs and projection mutation must
    // run on the owning UI thread. Only source change notifications are marshaled back here.
    winrt::ItemsSourceView GetItemsSourceView();
    TableViewRowMetadataProvider GetRowMetadata();
    // True when the source is ACTUALLY projected as grouped rows (the effective shape, not the
    // declared intent): until a GroupBy produced a usable projection, the rows are raw items.
    bool IsGrouped() const;
    // What is declared on this source, not the projection in effect, read without calling app code.
    bool HasFilter() const;
    bool HasGroupBy() const;
    bool HasParentBy() const;
    // Rows before the filter (every node for a tree), and data rows after it, without group
    // headers and whatever is expanded. Neither reads app code.
    uint32_t SourceRowCount() const;
    uint32_t DataRowCount() const;

    // Set while this source is shown by its TableView. Held weakly - the TableView holds this
    // source. Typed as IInspectable so this type does not name the control.
    // Returns false, and leaves the existing owner bound, when a different live TableView owns it.
    bool SetOwningTableView(winrt::IInspectable const& owner);
    // Detaches `owner`; a no-op when another TableView owns this source.
    void ReleaseOwningTableView(winrt::IInspectable const& owner);
    // True when a TableView other than `candidate` owns this source.
    bool IsOwnedByOther(winrt::IInspectable const& candidate) const
    {
        auto const owner = m_owningTableView.get();
        return owner && owner != candidate;
    }
    // Raised after a rebuild changed the projected shape (grouped <-> flat), which swaps the
    // ItemsSourceView and row-metadata provider the owner cached.
    void SetProjectionChangedHandler(std::function<void()> handler) { m_projectionChanged = std::move(handler); }
    // Raised when a shaping verb rewrote the projection. `reorderOnly` is true when membership is
    // unchanged and only the order moved.
    void SetShapingChangedHandler(std::function<void(bool)> handler) { m_shapingChanged = std::move(handler); }
    using ShapingTipCompleted = std::function<void(TableViewTip::Operation, uint64_t, bool failed,
        std::optional<TableViewTip::IgnoreReason>, std::optional<HRESULT>, TableViewTip::AppFaultKind,
        TableViewTip::ContainedFaults)>;
    void SetShapingTipHandlers(
        winrt::IInspectable const& owner,
        std::function<uint64_t(TableViewTip::Operation, bool)> begin,
        ShapingTipCompleted complete);
    // Lets the owner refresh a custom comparer's ranks as items change (see
    // ShapedItemsSource::SetLiveItemChangedHook).
    void SetLiveItemChangedHook(std::function<bool(winrt::IInspectable const&, bool)> hook);
    // Lets the owner drop comparer ranks for rows that left the source (see
    // ShapedItemsSource::SetRowsReleasedHook).
    void SetRowsReleasedHook(std::function<void(winrt::IInspectable const&, std::vector<winrt::IInspectable> const&)> hook);
    // Lets the owner learn that live shaping reacted to an item change (see
    // ShapedItemsSource::SetLiveItemReshapeHandler).
    void SetLiveItemReshapeHandler(std::function<void()> handler);
    // False when sorting would fail because the rows include a null item or one object twice.
    bool CanApplySort() const;
    // Set to true when this source is destroyed; lets helpers that hold a raw pointer detect it.
    std::shared_ptr<bool const> DestroyedFlag() const noexcept { return m_obsolete; }

private:
    struct ShapingTip;
    struct ShapingTipHandlers
    {
        std::function<uint64_t(TableViewTip::Operation, bool)> begin;
        ShapingTipCompleted complete;
    };
    void ReplaceSortsCore(winrt::hstring const& sortAxisToken, ShapingHelpers::KeySelector const& key, winrt::hstring const& sortMemberPath, winrt::SortDirection direction);
    void ThrowIfWrongThread() const;
    // Re-derives everything this class caches from the shape the engine just produced.
    void OnProjectionRebuilt();
    TableViewRowItemKeySelector MakeIdentitySelector() const;
    void NotifyOwnerProjectionChanged();
    // Tells the owner a shaping verb rewrote the projection, so it can raise the UIA
    // structure-changed event that a programmatic reshape has no other trigger for.
    void NotifyOwnerShapingChanged(bool reorderOnly);

    std::shared_ptr<bool> m_obsolete{ std::make_shared<bool>(false) };
    std::shared_ptr<ShapedItemsSource> m_engine{};
    // Weak: the owning TableView references this source, so a strong back-pointer would be a cycle.
    winrt::weak_ref<winrt::IInspectable> m_owningTableView{ nullptr };
    // Installed by the owner and cleared when it detaches, so a source that has been swapped out
    // cannot drive its former owner.
    std::function<void()> m_projectionChanged{};
    std::function<void(bool)> m_shapingChanged{};
    std::shared_ptr<ShapingTipHandlers> m_shapingTipHandlers;
    ShapingTip* m_shapingTip{};
    uint64_t m_ownerGeneration{};
    uint32_t m_notificationDepth{};
    // tracker_ref so the GC can walk this strong reference into the (possibly managed) projected
    // rows and collect any cycle; also swaps to null safely during finalization.
    tracker_ref<winrt::ItemsSourceView> m_itemsSourceView{ this };
    TableViewRowMetadataProvider m_rowMetadata{};
    // Used to post the end of a deferral when this source was created on a thread without a queue.
    winrt::Microsoft::UI::Dispatching::DispatcherQueue m_deferralQueue{ nullptr };
};
