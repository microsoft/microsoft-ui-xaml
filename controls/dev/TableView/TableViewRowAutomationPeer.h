// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TableViewRow.h"
#include "TableViewRowAutomationPeer.g.h"
#include "TableViewAutomationHelpers.h"
#include "TableViewItemPeerCache.h"

#include <functional>
#include <vector>

class TableViewRowAutomationPeer :
    public ReferenceTracker<TableViewRowAutomationPeer, winrt::implementation::TableViewRowAutomationPeerT, winrt::IVirtualizedItemProvider>
{
public:
    struct ItemPeerTag {};

    // A row's own peer, which the row's OnCreateAutomationPeer may replace with a derived one.
    TableViewRowAutomationPeer(winrt::TableViewRow const& owner);
    // The peer of the item a row shows; see CreateItemPeerInternal.
    TableViewRowAutomationPeer(winrt::TableViewRow const& owner, ItemPeerTag);

    // IAutomationPeerOverrides
    hstring GetClassNameCore();
    winrt::AutomationControlType GetAutomationControlTypeCore();
    hstring GetLocalizedControlTypeCore();
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);

    // IAutomationPeerOverrides. Routes through TableView so focus and stale checks apply to a row
    // style or template ContextFlyout too; falls back to the framework when TableView shows nothing.
    void ShowContextMenuCore();

    // A TableViewRow is a Control with no content of its own, so the base peer computes no name
    // and AT announces a bare "data item". Compose the visible cell texts instead.
    hstring GetNameCore();

    // Rows are virtualized: UIA only ever sees the realized window, so the control has to supply
    // "row i of n" on the same flat basis as IGridProvider. A tree row reports its position
    // within its sibling set instead.
    int32_t GetPositionInSetCore();
    int32_t GetSizeOfSetCore();

    // Expose direct cell wrappers only to avoid deep, costly UIA subtree walks.
    winrt::IVector<winrt::AutomationPeer> GetChildrenCore();

    // ISelectionItemProvider — the row reports state the owning TableView holds; it never keeps
    // its own copy, because rows are recycled.
    bool IsSelected();
    winrt::IRawElementProviderSimple SelectionContainer();
    void AddToSelection();
    void RemoveFromSelection();
    void Select();

    // IVirtualizedItemProvider — available only after the realized row has been recycled out.
    void Realize();
    // Creates the row at rowIndex and brings it into view (UIA Realize only, never a read). Throws
    // ElementNotAvailable when it cannot, or when the row is not the expected one.
    static void RealizeRowAt(
        winrt::TableView const& tableView,
        int32_t rowIndex,
        std::function<bool(winrt::TableViewRow const&)> const& isExpectedRow);

    // Single source of cell-peer identity for GetChildrenCore and Grid.GetItem; UIA compares
    // providers by identity.
    winrt::AutomationPeer GetOrCreateCellPeer(winrt::FrameworkElement const& cell);
    bool CanReuseForRowItem(winrt::TableViewRow const& row, winrt::TableView const& tableView);

    // ListView's model: UIA is handed the item's peer through the row's own peer's EventsSource.
    // The item peer is always this base type. It keeps SelectionItem, ExpandCollapse and
    // VirtualizedItem, and forwards everything else to the own peer of the row now showing its
    // item, which may be a derived type. With no row it answers as a virtualized item (name from
    // the data, offscreen) while the item is in the source, and throws ElementNotAvailable after.
    winrt::IInspectable NavigateCore(winrt::Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection const& direction);
    winrt::Rect GetBoundingRectangleCore();
    bool IsOffscreenCore();
    bool IsEnabledCore();
    bool HasKeyboardFocusCore();
    bool IsKeyboardFocusableCore();
    winrt::Point GetClickablePointCore();
    winrt::hstring GetAutomationIdCore();
    void SetFocusCore();
    winrt::hstring GetHelpTextCore();
    winrt::hstring GetItemStatusCore();
    winrt::hstring GetItemTypeCore();
    winrt::hstring GetFullDescriptionCore();
    winrt::Windows::Foundation::Collections::IIterable<winrt::AutomationPeer> GetDescribedByCore();
    winrt::hstring GetAccessKeyCore();
    winrt::hstring GetAcceleratorKeyCore();
    winrt::AutomationPeer GetLabeledByCore();
    winrt::Microsoft::UI::Xaml::Automation::Peers::AutomationLiveSetting GetLiveSettingCore();
    winrt::Microsoft::UI::Xaml::Automation::Peers::AutomationHeadingLevel GetHeadingLevelCore();
    int32_t GetCultureCore();
    bool IsRequiredForFormCore();
    bool IsControlElementCore();
    bool IsContentElementCore();

    static TableViewRowAutomationPeer* FromPeerInternal(winrt::AutomationPeer const& peer);
    // The live item peer cached for the item at `index`, if any.
    static winrt::AutomationPeer FindItemPeerInternal(
        TableViewItemPeerCache& cache, winrt::TableView const& tableView, winrt::IInspectable const& item, int32_t index, bool byIndex);
    // An item peer for `item` in `row`; `byIndex` follows the row's index while the source keeps `sourceVersion`.
    static winrt::AutomationPeer CreateItemPeerInternal(
        winrt::TableViewRow const& row, winrt::IInspectable const& item, bool byIndex, uint64_t sourceVersion);
    TableViewItemPeerBinding& ItemPeerBindingInternal() noexcept { return m_binding; }
    // Whether this peer may show `item` in `row`: it tracks that item and no other row shows it through this peer.
    bool CanBindToItemInternal(winrt::IInspectable const& item, winrt::TableViewRow const& row, bool byIndex);
    void OnBoundToRowInternal(winrt::TableViewRow const& row, int32_t rowIndex);
    // A row's own peer records the item its row last showed, so a re-prepared row knows whether its
    // cell peers still describe the item.
    void TrackItemInternal(winrt::IInspectable const& item);
    bool TracksItemInternal(winrt::IInspectable const& item) const { return m_item.SameIdentityAs(item); }
    void DropCellPeerCacheForRowInternal(winrt::TableViewRow const& row);
    // The source index of an item peer's item, realized or not; -1 once it is gone.
    int32_t TrackedItemIndexInternal();

    // IExpandCollapseProvider — hierarchical rows only. A leaf reports LeafNode rather than the
    // pattern being withdrawn, matching TableViewGroupHeaderAutomationPeer.
    winrt::ExpandCollapseState ExpandCollapseState();
    void Expand();
    void Collapse();

    // IAutomationPeerOverrides3. Level is the row's 1-based tree depth.
    int32_t GetLevelCore();

    // Raised by the owning row on expansion change so clients don't read stale state.
    void RaiseExpandCollapseAutomationEvent(winrt::ExpandCollapseState oldState, winrt::ExpandCollapseState newState);

    // Raised by the owning row on every hierarchy restamp. Clients cache Level/PositionInSet/
    // SizeOfSet, so a reparent or sibling insert must notify; compares against the last values
    // this peer reported and raises only for the ones that moved.
    void RaiseHierarchyPositionChanged();

private:

    // Last values handed to a client through the Get*Core overrides; -1 until first read.
    int32_t m_lastReportedLevel{ -1 };
    int32_t m_lastReportedPositionInSet{ -1 };
    int32_t m_lastReportedSizeOfSet{ -1 };

    // The owning TableView, or null once the row has been recycled out of the tree.
    winrt::TableView GetOwningTableView();
    // This row's index in the owner's ItemsSource index space, or -1 when unrealized.
    int32_t GetRowIndex();
    bool IsVirtualized();
    void RealizeCore();
    int32_t GetTrackedItemIndex(winrt::TableView const& tableView);
    bool ShowsTrackedItem(winrt::TableViewRow const& row, winrt::TableView const& tableView) const;

    bool IsLastOwningTableEnabled() const;
    int32_t GetLastOwningTableCulture() const;

    // How a peer answers: a row's own peer from its own element; an item peer through the own peer
    // of the row showing its item, as a virtualized item, or not at all. Self stays a route so every
    // getter is one switch over the same resolve.
    enum class Route { Self, Forward, Virtualized, Gone };
    // Resolves the route, re-binding to a realized row that shows the item if needed. Never throws.
    Route GetRoute(winrt::TableViewRow* row = nullptr);
    // GetRoute, throwing ElementNotAvailable for Gone; `forwardTo` is set for Forward.
    Route RequireRoute(winrt::AutomationPeer& forwardTo, winrt::TableViewRow* row = nullptr);
    // The row showing this item; throws ElementNotAvailable when there is none.
    winrt::TableViewRow RequireRealizedRow();
    winrt::TableViewRow TryGetBoundRow() const;
    winrt::TableViewRow TryRebindToRealizedRow();
    // For a row's own peer, the item peer it forwards to while that peer is bound to this row.
    winrt::TableViewRowAutomationPeer GetBoundItemPeer() const;

    enum class HierarchyValue { Level, PositionInSet, SizeOfSet };
    int32_t ComputeHierarchyValue(HierarchyValue value, Route route, winrt::AutomationPeer const& forwardTo);
    // Resolves once, records the value as reported to a client, and returns it.
    int32_t ReportHierarchyValue(HierarchyValue value, int32_t& lastReported);

    // What a SelectionItem call acts on, resolved once.
    struct SelectionTarget
    {
        Route route{ Route::Gone };
        winrt::TableView tableView{ nullptr };
        int32_t index{ -1 };
        winrt::TableViewRow row{ nullptr };
        winrt::AutomationPeer forwardTo{ nullptr };
    };
    // Throws ElementNotAvailable when stale.
    SelectionTarget ResolveSelectionTarget();
    void SelectionTargetFor(SelectionTarget& target);
    void ThrowIfNotEnabled(SelectionTarget const& target);

    // Joins the visible cells' display text in visual order.
    static std::wstring ComposeCellTexts(
        TableViewRow* rowImpl,
        winrt::Panel const& cellsHost,
        bool allowPeerCreation);

    // Dead cell entries stay cached until the next prune during child enumeration or peer lookup.
    struct CellPeerCacheEntry
    {
        CellPeerCacheEntry(
            ITrackerHandleManager const* owner,
            winrt::FrameworkElement const& cellElement,
            winrt::AutomationPeer const& cellPeer)
            : cell(winrt::make_weak(cellElement))
            , peer(owner, cellPeer)
        {
        }

        winrt::weak_ref<winrt::FrameworkElement> cell{ nullptr };
        tracker_ref<winrt::AutomationPeer> peer;
    };

    std::vector<CellPeerCacheEntry> m_cellPeerCache;
    // The row whose cells m_cellPeerCache holds.
    winrt::weak_ref<winrt::TableViewRow> m_cellsRow{ nullptr };
    TableViewItemPeerBinding m_binding;
    winrt::weak_ref<winrt::TableView> m_lastOwningTable{ nullptr };
    TableViewTrackedItemIdentity m_item;
    // Where the item was last seen; a miss re-finds the occurrence nearest to it. For a by-index
    // peer this is its index, valid while the source keeps m_sourceVersion.
    int32_t m_lastKnownRowIndex{ -1 };
    bool m_byIndex{ false };
    uint64_t m_sourceVersion{ 0 };
    // An item peer keeps the row that created it, its Owner, alive: the framework holds the owner
    // unreferenced, and this peer outlives that row whenever the item moves to another one. The
    // row -> own peer -> EventsSource -> this cycle is broken by EventsSource(nullptr) when the row is cleared.
    tracker_ref<winrt::TableViewRow> m_ownerRow{ this };
    winrt::TableViewRow GetRow() const;
    bool IsHierarchicalRow() const;
    // Passed to the owner unresolved so it stays idempotent.
    void SetExpansion(bool expand);
    // Sibling-set position/size. False when not a realized tree row (UIA gets 0, "unknown").
    bool TryGetSiblingPosition(int32_t& positionInSet, int32_t& sizeOfSet);
};
