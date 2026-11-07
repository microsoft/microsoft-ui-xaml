// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewRow.h"
#include "TableViewRowAutomationPeer.h"
#include "TableViewAutomationPeer.h"
#include "TableViewCellAutomationPeer.h"
#include "TableViewAutomationHelpers.h"
#include "TableViewSource.h"
#include "TableViewRowAutomationPeer.properties.cpp"

#include <UIAutomationCore.h>
#include <UIAutomationCoreApi.h>
#include <algorithm>
#include <string>

namespace
{
    // Grouped sources report PositionInSet/SizeOfSet relative to the containing group, excluding
    // header bands. The provider's cached band index avoids a quadratic per-row walk; the walk
    // below is the fallback.
    bool TryGetGroupedRowSetMetadataFromProvider(TableView* tableImpl, int32_t rowIndex, int32_t& positionInSet, int32_t& sizeOfSet)
    {
        try
        {
            // Grouping exists only on an app-assigned TableViewSource (the ItemsSource itself).
            auto const source = tableImpl->ItemsSource().try_as<winrt::TableViewSource>();
            auto const repeater = tableImpl->GetRowsRepeaterInternal();
            if (!source || !repeater)
            {
                return false;
            }

            auto const metadata = winrt::get_self<::TableViewSource>(source)->GetRowMetadata();
            return metadata && metadata->TryGetGroupMembership(repeater.ItemsSourceView(), rowIndex, positionInSet, sizeOfSet);
        }
        catch (...)
        {
            return false;
        }
    }

    bool TryGetGroupedRowSetMetadata(TableView* tableImpl, int32_t rowIndex, int32_t& positionInSet, int32_t& sizeOfSet)
    {
        if (!tableImpl || rowIndex < 0 || !tableImpl->IsTableViewSourceGrouped())
        {
            return false;
        }

        const auto rowCount = tableImpl->GetRowCountInternal();
        if (rowIndex >= rowCount || tableImpl->IsGroupHeaderRow(rowIndex))
        {
            return false;
        }

        if (TryGetGroupedRowSetMetadataFromProvider(tableImpl, rowIndex, positionInSet, sizeOfSet))
        {
            return true;
        }

        int32_t first = rowIndex;
        while (first > 0 && !tableImpl->IsGroupHeaderRow(first - 1))
        {
            --first;
        }

        int32_t last = rowIndex;
        while (last + 1 < rowCount && !tableImpl->IsGroupHeaderRow(last + 1))
        {
            ++last;
        }

        positionInSet = rowIndex - first + 1;
        sizeOfSet = last - first + 1;
        return true;
    }
}

TableViewRowAutomationPeer::TableViewRowAutomationPeer(winrt::TableViewRow const& owner)
    : ReferenceTracker(owner)
{
    if (owner)
    {
        winrt::get_self<TableViewRow>(owner)->SyncAutomationPeerToItemInternal(this);
    }
}

TableViewRowAutomationPeer::TableViewRowAutomationPeer(winrt::TableViewRow const& owner, ItemPeerTag)
    : ReferenceTracker(owner)
{
}

TableViewRowAutomationPeer* TableViewRowAutomationPeer::FromPeerInternal(winrt::AutomationPeer const& peer)
{
    auto const rowPeer = peer ? peer.try_as<winrt::TableViewRowAutomationPeer>() : nullptr;
    return rowPeer ? winrt::get_self<TableViewRowAutomationPeer>(rowPeer) : nullptr;
}

winrt::AutomationPeer TableViewRowAutomationPeer::FindItemPeerInternal(
    TableViewItemPeerCache& cache, winrt::TableView const& tableView, winrt::IInspectable const& item, int32_t index, bool byIndex)
{
    auto const key = TableViewRow::RowItemPeerKeyInternal(item, index, byIndex, winrt::get_self<TableView>(tableView)->RowsSourceVersionInternal());
    auto const peer = cache.GetOrCreate(key, []() { return winrt::AutomationPeer{ nullptr }; });
    auto* const impl = FromPeerInternal(peer);
    return impl && impl->m_binding.IsItemPeer() && impl->GetTrackedItemIndex(tableView) == index ? peer : nullptr;
}

winrt::AutomationPeer TableViewRowAutomationPeer::CreateItemPeerInternal(
    winrt::TableViewRow const& row, winrt::IInspectable const& item, bool byIndex, uint64_t sourceVersion)
{
    auto const peer = winrt::make_self<TableViewRowAutomationPeer>(row, ItemPeerTag{});
    peer->m_ownerRow.set(row);
    peer->m_item.Track(item);
    peer->m_byIndex = byIndex;
    peer->m_sourceVersion = sourceVersion;
    return peer.as<winrt::AutomationPeer>();
}

// An item peer answers from the own peer of the row showing its item, or with a virtualized
// item's value; a row's own peer answers from its own element.
#define TABLEVIEW_ROW_PEER_ROUTE(forwarded, virtualizedValue, selfValue) \
    winrt::AutomationPeer forwardTo{ nullptr }; \
    switch (RequireRoute(forwardTo)) \
    { \
    case Route::Forward: return forwarded; \
    case Route::Virtualized: return virtualizedValue; \
    default: return selfValue; \
    }
#define TABLEVIEW_ROW_PEER_FORWARD(name, virtualizedValue) \
    TABLEVIEW_ROW_PEER_ROUTE(forwardTo.name(), virtualizedValue, __super::name##Core())

hstring TableViewRowAutomationPeer::GetClassNameCore() { TABLEVIEW_ROW_PEER_ROUTE(forwardTo.GetClassName(), winrt::hstring_name_of<winrt::TableViewRow>(), winrt::hstring_name_of<winrt::TableViewRow>()) }
winrt::AutomationControlType TableViewRowAutomationPeer::GetAutomationControlTypeCore() { TABLEVIEW_ROW_PEER_ROUTE(forwardTo.GetAutomationControlType(), winrt::AutomationControlType::DataItem, winrt::AutomationControlType::DataItem) }
hstring TableViewRowAutomationPeer::GetLocalizedControlTypeCore() { TABLEVIEW_ROW_PEER_FORWARD(GetLocalizedControlType, __super::GetLocalizedControlTypeCore()) }
winrt::Rect TableViewRowAutomationPeer::GetBoundingRectangleCore() { TABLEVIEW_ROW_PEER_FORWARD(GetBoundingRectangle, winrt::Rect{}) }
bool TableViewRowAutomationPeer::IsOffscreenCore() { TABLEVIEW_ROW_PEER_FORWARD(IsOffscreen, true) }
bool TableViewRowAutomationPeer::IsEnabledCore() { TABLEVIEW_ROW_PEER_FORWARD(IsEnabled, IsLastOwningTableEnabled()) }
bool TableViewRowAutomationPeer::HasKeyboardFocusCore() { TABLEVIEW_ROW_PEER_FORWARD(HasKeyboardFocus, false) }
bool TableViewRowAutomationPeer::IsKeyboardFocusableCore() { TABLEVIEW_ROW_PEER_FORWARD(IsKeyboardFocusable, false) }
winrt::Point TableViewRowAutomationPeer::GetClickablePointCore() { TABLEVIEW_ROW_PEER_FORWARD(GetClickablePoint, winrt::Point{}) }
winrt::hstring TableViewRowAutomationPeer::GetAutomationIdCore() { TABLEVIEW_ROW_PEER_FORWARD(GetAutomationId, winrt::hstring{}) }
void TableViewRowAutomationPeer::SetFocusCore() { TABLEVIEW_ROW_PEER_FORWARD(SetFocus, (void)0) }
winrt::hstring TableViewRowAutomationPeer::GetHelpTextCore() { TABLEVIEW_ROW_PEER_FORWARD(GetHelpText, winrt::hstring{}) }
winrt::hstring TableViewRowAutomationPeer::GetItemStatusCore() { TABLEVIEW_ROW_PEER_FORWARD(GetItemStatus, winrt::hstring{}) }
winrt::hstring TableViewRowAutomationPeer::GetItemTypeCore() { TABLEVIEW_ROW_PEER_FORWARD(GetItemType, winrt::hstring{}) }
winrt::hstring TableViewRowAutomationPeer::GetFullDescriptionCore() { TABLEVIEW_ROW_PEER_FORWARD(GetFullDescription, winrt::hstring{}) }
winrt::hstring TableViewRowAutomationPeer::GetAccessKeyCore() { TABLEVIEW_ROW_PEER_FORWARD(GetAccessKey, winrt::hstring{}) }
winrt::hstring TableViewRowAutomationPeer::GetAcceleratorKeyCore() { TABLEVIEW_ROW_PEER_FORWARD(GetAcceleratorKey, winrt::hstring{}) }
winrt::AutomationPeer TableViewRowAutomationPeer::GetLabeledByCore() { TABLEVIEW_ROW_PEER_FORWARD(GetLabeledBy, nullptr) }
winrt::Microsoft::UI::Xaml::Automation::Peers::AutomationLiveSetting TableViewRowAutomationPeer::GetLiveSettingCore() { TABLEVIEW_ROW_PEER_FORWARD(GetLiveSetting, winrt::Microsoft::UI::Xaml::Automation::Peers::AutomationLiveSetting::Off) }
winrt::Microsoft::UI::Xaml::Automation::Peers::AutomationHeadingLevel TableViewRowAutomationPeer::GetHeadingLevelCore() { TABLEVIEW_ROW_PEER_FORWARD(GetHeadingLevel, winrt::Microsoft::UI::Xaml::Automation::Peers::AutomationHeadingLevel::None) }
int32_t TableViewRowAutomationPeer::GetCultureCore() { TABLEVIEW_ROW_PEER_FORWARD(GetCulture, GetLastOwningTableCulture()) }
bool TableViewRowAutomationPeer::IsRequiredForFormCore() { TABLEVIEW_ROW_PEER_FORWARD(IsRequiredForForm, false) }
bool TableViewRowAutomationPeer::IsControlElementCore() { TABLEVIEW_ROW_PEER_FORWARD(IsControlElement, true) }
bool TableViewRowAutomationPeer::IsContentElementCore() { TABLEVIEW_ROW_PEER_FORWARD(IsContentElement, true) }

#undef TABLEVIEW_ROW_PEER_FORWARD
#undef TABLEVIEW_ROW_PEER_ROUTE

winrt::IInspectable TableViewRowAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    winrt::AutomationPeer forwardTo{ nullptr };
    winrt::TableViewRow row{ nullptr };
    const auto route = RequireRoute(forwardTo, &row);

    // SelectionItem is advertised only while the owning table can actually select: a row's own peer
    // in the recycle pool advertises nothing, and a stale item peer has already thrown above.
    if (patternInterface == winrt::PatternInterface::SelectionItem)
    {
        SelectionTarget target{ route };
        target.row = row;
        SelectionTargetFor(target);
        if (target.tableView && winrt::get_self<TableView>(target.tableView)->CanSelectRows())
        {
            return *this;
        }
        return nullptr;
    }

    if (patternInterface == winrt::PatternInterface::VirtualizedItem)
    {
        if (route == Route::Virtualized)
        {
            return *this;
        }
        return nullptr;
    }
    if (route == Route::Virtualized)
    {
        return nullptr;
    }

    // Tree rows only (keyed off Level, not IsExpandable): a tree leaf reports LeafNode, while flat/
    // grouped rows withhold the pattern so the grid isn't announced as a collapsed tree.
    if (patternInterface == winrt::PatternInterface::ExpandCollapse)
    {
        if (IsHierarchicalRow())
        {
            return *this;
        }
        return nullptr;
    }

    // The patterns that act on the item are answered above, by this peer, which re-checks its item on
    // every call. TableViewRow's own peer offers no others; one a derived row peer adds is that row's,
    // as ListView's item peers hand out their container peer's.
    if (route == Route::Forward)
    {
        return forwardTo.GetPattern(patternInterface);
    }
    return __super::GetPatternCore(patternInterface);
}

winrt::TableViewRow TableViewRowAutomationPeer::GetRow() const
{
    return Owner().try_as<winrt::TableViewRow>();
}

winrt::TableViewRow TableViewRowAutomationPeer::TryGetBoundRow() const
{
    if (!m_binding.IsItemPeer())
    {
        return GetRow();
    }

    auto const container = m_binding.Resolve(
        [](winrt::UIElement const& element)
        {
            auto const row = element.try_as<winrt::TableViewRow>();
            return row ? winrt::get_self<TableViewRow>(row)->RowItemIdentityInternal() : winrt::hstring{};
        },
        [this](winrt::AutomationPeer const& source)
        {
            return FromPeerInternal(source) == this;
        });
    // A row the owner's current repeater does not hold (its template was reapplied) shows nothing.
    auto const row = container ? container.as<winrt::TableViewRow>() : nullptr;
    auto const tableView = row ? winrt::get_self<TableViewRow>(row)->GetOwningTableView() : nullptr;
    auto const repeater = tableView ? winrt::get_self<TableView>(tableView)->GetRowsRepeaterInternal() : nullptr;
    return repeater && repeater.GetElementIndex(row) >= 0 && ShowsTrackedItem(row, tableView) ? row : nullptr;
}

winrt::TableViewRow TableViewRowAutomationPeer::TryRebindToRealizedRow()
{
    // The item may have been realized into a row whose own peer does not exist yet, so nothing
    // re-synced it; creating that peer binds this one if it is the item's cached peer.
    auto const tableView = m_lastOwningTable.get();
    if (!m_binding.IsItemPeer() || !tableView)
    {
        return nullptr;
    }

    auto const repeater = winrt::get_self<TableView>(tableView)->GetRowsRepeaterInternal();
    const int32_t index = repeater ? GetTrackedItemIndex(tableView) : -1;
    auto const row = index >= 0 ? repeater.TryGetElement(index).try_as<winrt::TableViewRow>() : nullptr;
    if (!row || !ShowsTrackedItem(row, tableView))
    {
        return nullptr;
    }

    if (winrt::FrameworkElementAutomationPeer::FromElement(row))
    {
        winrt::get_self<TableViewRow>(row)->SyncAutomationPeerToItemInternal();
    }
    else
    {
        winrt::FrameworkElementAutomationPeer::CreatePeerForElement(row);
    }
    return TryGetBoundRow();
}

TableViewRowAutomationPeer::Route TableViewRowAutomationPeer::GetRoute(winrt::TableViewRow* row)
{
    if (!m_binding.IsItemPeer())
    {
        if (row)
        {
            *row = GetRow();
        }
        return Route::Self;
    }

    auto bound = TryGetBoundRow();
    if (!bound)
    {
        bound = TryRebindToRealizedRow();
    }
    if (row)
    {
        *row = bound;
    }
    if (bound)
    {
        return Route::Forward;
    }
    return IsVirtualized() ? Route::Virtualized : Route::Gone;
}

TableViewRowAutomationPeer::Route TableViewRowAutomationPeer::RequireRoute(winrt::AutomationPeer& forwardTo, winrt::TableViewRow* row)
{
    winrt::TableViewRow bound{ nullptr };
    const auto route = GetRoute(&bound);
    if (route == Route::Gone)
    {
        ThrowElementNotAvailable();
    }
    if (route == Route::Forward)
    {
        forwardTo = winrt::FrameworkElementAutomationPeer::FromElement(bound);
    }
    if (row)
    {
        *row = bound;
    }
    return route;
}

winrt::TableViewRow TableViewRowAutomationPeer::RequireRealizedRow()
{
    winrt::TableViewRow row{ nullptr };
    if (GetRoute(&row) == Route::Gone || !row)
    {
        ThrowElementNotAvailable();
    }
    return row;
}

winrt::TableViewRowAutomationPeer TableViewRowAutomationPeer::GetBoundItemPeer() const
{
    if (m_binding.IsItemPeer())
    {
        return nullptr;
    }
    auto const itemPeer = EventsSource().try_as<winrt::TableViewRowAutomationPeer>();
    return itemPeer && winrt::get_self<TableViewRowAutomationPeer>(itemPeer)->TryGetBoundRow() == GetRow() ? itemPeer : nullptr;
}

bool TableViewRowAutomationPeer::CanBindToItemInternal(winrt::IInspectable const& item, winrt::TableViewRow const& row, bool byIndex)
{
    // A by-index peer's cache key already names its index and source version.
    if (byIndex != m_byIndex || (!byIndex && m_item.IsTracking() && !m_item.SameIdentityAs(item)))
    {
        return false;
    }
    // Another row showing a duplicate of the item keeps this peer; this row gets its own.
    auto const bound = m_binding.IsItemPeer() ? TryGetBoundRow() : nullptr;
    return !bound || bound == row;
}

void TableViewRowAutomationPeer::OnBoundToRowInternal(winrt::TableViewRow const& row, int32_t rowIndex)
{
    if (auto const tableView = winrt::get_self<TableViewRow>(row)->GetOwningTableView())
    {
        m_lastOwningTable = winrt::make_weak(tableView);
    }
    if (rowIndex >= 0)
    {
        m_lastKnownRowIndex = rowIndex;
    }
    if (m_cellsRow.get() != row)
    {
        m_cellPeerCache.clear();
        m_cellsRow = winrt::make_weak(row);
    }
}

void TableViewRowAutomationPeer::TrackItemInternal(winrt::IInspectable const& item)
{
    if (!m_item.SameIdentityAs(item))
    {
        m_item.Track(item);
    }
}

void TableViewRowAutomationPeer::DropCellPeerCacheForRowInternal(winrt::TableViewRow const& row)
{
    if (m_cellsRow.get() == row)
    {
        m_cellPeerCache.clear();
    }
}

int32_t TableViewRowAutomationPeer::TrackedItemIndexInternal()
{
    auto const tableView = m_lastOwningTable.get();
    return m_binding.IsItemPeer() && tableView ? GetTrackedItemIndex(tableView) : -1;
}

bool TableViewRowAutomationPeer::IsLastOwningTableEnabled() const
{
    auto const tableView = m_lastOwningTable.get();
    return !tableView || tableView.IsEnabled();
}

int32_t TableViewRowAutomationPeer::GetLastOwningTableCulture() const
{
    auto const tableView = m_lastOwningTable.get();
    auto const tablePeer = tableView ? winrt::FrameworkElementAutomationPeer::CreatePeerForElement(tableView) : nullptr;
    return tablePeer ? tablePeer.GetCulture() : 0;
}

winrt::IInspectable TableViewRowAutomationPeer::NavigateCore(winrt::Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection const& direction)
{
    using Direction = winrt::Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection;
    if (m_binding.IsItemPeer())
    {
        winrt::AutomationPeer forwardTo{ nullptr };
        if (RequireRoute(forwardTo) == Route::Virtualized)
        {
            if (direction == Direction::Parent)
            {
                return TableViewAutomationPeer::RowsParentPeerInternal(m_lastOwningTable.get());
            }
            return nullptr;
        }
        // The framework's tree holds the row's own peer, not this one; children are this peer's cells.
        if (direction != Direction::FirstChild && direction != Direction::LastChild)
        {
            return forwardTo.Navigate(direction);
        }
    }
    return __super::NavigateCore(direction);
}

winrt::Windows::Foundation::Collections::IIterable<winrt::AutomationPeer> TableViewRowAutomationPeer::GetDescribedByCore()
{
    winrt::AutomationPeer forwardTo{ nullptr };
    winrt::TableViewRow row{ nullptr };
    switch (RequireRoute(forwardTo, &row))
    {
    case Route::Forward:
    {
        // AutomationPeer does not project GetDescribedBy, so resolve the row's own list.
        auto const describedBy = winrt::single_threaded_vector<winrt::AutomationPeer>();
        for (auto const& element : winrt::AutomationProperties::GetDescribedBy(row))
        {
            if (auto const uiElement = element.try_as<winrt::UIElement>())
            {
                if (auto const peer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(uiElement))
                {
                    describedBy.Append(peer);
                }
            }
        }
        return describedBy;
    }
    case Route::Virtualized:
        return nullptr;
    default:
        return __super::GetDescribedByCore();
    }
}

void TableViewRowAutomationPeer::ShowContextMenuCore()
{
    winrt::AutomationPeer forwardTo{ nullptr };
    winrt::TableViewRow row{ nullptr };
    if (RequireRoute(forwardTo, &row) == Route::Virtualized)
    {
        ThrowElementNotAvailable();
    }
    if (forwardTo)
    {
        forwardTo.ShowContextMenu();
        return;
    }

    // A recycled row raising the request would open a menu for whatever item it holds next.
    if (!GetOwningTableView())
    {
        ThrowElementNotAvailable();
    }

    if (winrt::get_self<TableViewRow>(row)->ShowContextMenuFromPeer())
    {
        return;
    }
    __super::ShowContextMenuCore();
}

bool TableViewRowAutomationPeer::IsHierarchicalRow() const
{
    // Level: 0 for flat/grouped, 1-based for tree rows.
    auto const row = TryGetBoundRow();
    return row && row.Level() > 0;
}

winrt::TableView TableViewRowAutomationPeer::GetOwningTableView()
{
    if (auto const row = TryGetBoundRow())
    {
        auto const tableView = winrt::get_self<TableViewRow>(row)->GetOwningTableView();
        if (tableView && winrt::get_self<TableView>(tableView)->UnwrapEditingDataItem(row.DataContext()))
        {
            m_lastOwningTable = winrt::make_weak(tableView);
            return tableView;
        }
    }

    return nullptr;
}

int32_t TableViewRowAutomationPeer::GetRowIndex()
{
    auto const row = TryGetBoundRow();
    auto const tableView = GetOwningTableView();
    if (!row || !tableView)
    {
        return -1;
    }

    if (auto const repeater = winrt::get_self<TableView>(tableView)->GetRowsRepeaterInternal())
    {
        if (const auto rowIndex = repeater.GetElementIndex(row); rowIndex >= 0)
        {
            m_lastOwningTable = winrt::make_weak(tableView);
            m_lastKnownRowIndex = rowIndex;
            return rowIndex;
        }
    }

    return -1;
}

bool TableViewRowAutomationPeer::IsVirtualized()
{
    // Only an item peer outlives its row; a row's own peer always describes that row.
    if (!m_binding.IsItemPeer() || GetRowIndex() >= 0)
    {
        return false;
    }

    auto const tableView = m_lastOwningTable.get();
    if (!tableView || m_lastKnownRowIndex < 0)
    {
        return false;
    }

    auto const tableImpl = winrt::get_self<TableView>(tableView);
    const auto rowIndex = GetTrackedItemIndex(tableView);
    if (rowIndex < 0 || rowIndex >= tableImpl->GetRowCountInternal())
    {
        return false;
    }

    auto const repeater = tableImpl->GetRowsRepeaterInternal();
    const bool isVirtualized = repeater && !repeater.TryGetElement(rowIndex);
    if (isVirtualized)
    {
        m_lastKnownRowIndex = rowIndex;
    }
    return isVirtualized;
}
void TableViewRowAutomationPeer::Realize()
{
    if (!IsVirtualized())
    {
        winrt::AutomationPeer forwardTo{ nullptr };
        RequireRoute(forwardTo);
        return;
    }

    // IVirtualizedItemProvider::Realize is synchronous: on return the client immediately re-queries
    // this provider and expects the element realized, with VirtualizedItem no longer supported.
    // Deferring to the dispatcher hands the client a still-virtualized provider.
    RealizeCore();
}

void TableViewRowAutomationPeer::RealizeCore()
{
    auto const tableView = m_lastOwningTable.get();
    RealizeRowAt(tableView, tableView ? GetTrackedItemIndex(tableView) : -1,
        [this, &tableView](winrt::TableViewRow const& row) { return ShowsTrackedItem(row, tableView); });
}

void TableViewRowAutomationPeer::RealizeRowAt(
    winrt::TableView const& tableView,
    int32_t rowIndex,
    std::function<bool(winrt::TableViewRow const&)> const& isExpectedRow)
{
    if (!tableView || rowIndex < 0)
    {
        ThrowElementNotAvailable();
    }

    auto const tableImpl = winrt::get_self<TableView>(tableView);
    if (rowIndex >= tableImpl->GetRowCountInternal())
    {
        ThrowElementNotAvailable();
    }

    auto const repeater = tableImpl->GetRowsRepeaterInternal();
    if (!repeater)
    {
        ThrowElementNotAvailable();
    }

    winrt::UIElement element{ nullptr };
    bool madeAnchor = false;
    try
    {
        element = repeater.TryGetElement(rowIndex);
        if (!element)
        {
            element = repeater.GetOrCreateElement(rowIndex);
            madeAnchor = true;
        }
    }
    catch (...)
    {
        ThrowElementNotAvailable();
    }

    auto const row = element.try_as<winrt::TableViewRow>();
    bool isExpected = row && isExpectedRow(row);
    if (isExpected)
    {
        // Arranged first, as ItemsRepeater requires; layout runs app code that can change the source or recycle the row.
        try
        {
            row.UpdateLayout();
        }
        catch (winrt::hresult_error const&)
        {
            isExpected = false;
        }
    }
    if (!isExpected || repeater.TryGetElement(rowIndex) != row || !isExpectedRow(row))
    {
        if (madeAnchor)
        {
            tableImpl->ReleaseRowsRepeaterAnchor(element);
        }
        ThrowElementNotAvailable();
    }
    row.StartBringIntoView();
}

int32_t TableViewRowAutomationPeer::GetTrackedItemIndex(winrt::TableView const& tableView)
{
    if (!tableView || !m_item.IsTracking())
    {
        return -1;
    }

    auto const tableImpl = winrt::get_self<TableView>(tableView);
    auto const repeater = tableImpl->GetRowsRepeaterInternal();
    auto const view = repeater ? repeater.ItemsSourceView() : nullptr;
    if (!view)
    {
        return -1;
    }

    const int32_t count = view.Count();
    if (m_byIndex)
    {
        // Once the source changes, the index may name another value: the peer is gone, never retargeted.
        return tableImpl->RowsSourceVersionInternal() == m_sourceVersion && m_lastKnownRowIndex < count ? m_lastKnownRowIndex : -1;
    }

    if (m_lastKnownRowIndex >= 0 && m_lastKnownRowIndex < count)
    {
        if (auto const candidate = tableImpl->UnwrapEditingDataItem(view.GetAt(m_lastKnownRowIndex));
            m_item.SameIdentityAs(candidate))
        {
            return m_lastKnownRowIndex;
        }
    }

    // A miss (the source changed under the item) picks the occurrence nearest to where it was.
    int32_t nearest = -1;
    for (int32_t index = 0; index < count; ++index)
    {
        if (auto const candidate = tableImpl->UnwrapEditingDataItem(view.GetAt(index));
            m_item.SameIdentityAs(candidate))
        {
            if (nearest < 0 || std::abs(index - m_lastKnownRowIndex) < std::abs(nearest - m_lastKnownRowIndex))
            {
                nearest = index;
            }
            if (index >= m_lastKnownRowIndex)
            {
                break;
            }
        }
    }

    if (nearest >= 0)
    {
        m_lastKnownRowIndex = nearest;
    }
    return nearest;
}

bool TableViewRowAutomationPeer::ShowsTrackedItem(winrt::TableViewRow const& row, winrt::TableView const& tableView) const
{
    if (!row || !tableView)
    {
        return false;
    }
    auto const tableImpl = winrt::get_self<TableView>(tableView);
    if (m_byIndex)
    {
        auto const repeater = tableImpl->GetRowsRepeaterInternal();
        return repeater && tableImpl->RowsSourceVersionInternal() == m_sourceVersion &&
            m_lastKnownRowIndex >= 0 && repeater.GetElementIndex(row) == m_lastKnownRowIndex;
    }
    return m_item.SameIdentityAs(tableImpl->UnwrapEditingDataItem(row.DataContext()));
}

bool TableViewRowAutomationPeer::CanReuseForRowItem(
    winrt::TableViewRow const& row,
    winrt::TableView const& tableView)
{
    if (!row || !tableView)
    {
        return false;
    }

    if (!m_item.IsTracking())
    {
        return true;
    }

    auto const item = winrt::get_self<TableView>(tableView)->UnwrapEditingDataItem(row.DataContext());
    return m_item.SameIdentityAs(item);
}

hstring TableViewRowAutomationPeer::GetNameCore()
{
    winrt::AutomationPeer forwardTo{ nullptr };
    switch (RequireRoute(forwardTo))
    {
    case Route::Forward:
        return forwardTo.GetName();
    case Route::Virtualized:
    {
        // Named from the data, as an unrealized selected row is. The route just found the item at m_lastKnownRowIndex.
        auto const tableImpl = winrt::get_self<TableView>(m_lastOwningTable.get());
        auto const view = tableImpl->GetRowsRepeaterInternal().ItemsSourceView();
        return TableViewAutomationPeer::ComposeItemNameInternal(
            *tableImpl, tableImpl->UnwrapEditingDataItem(view.GetAt(m_lastKnownRowIndex)));
    }
    default:
        break;
    }

    auto const row = GetRow();
    if (!GetOwningTableView())
    {
        ThrowElementNotAvailable();
    }

    // Preserve explicit app metadata, not a base peer's Content/DataContext stringification.
    if (auto const name = winrt::AutomationProperties::GetName(row); !name.empty())
    {
        return name;
    }
    if (auto const label = GetLabeledBy())
    {
        if (auto const name = label.GetName(); !name.empty())
        {
            return name;
        }
    }

    auto const rowImpl = winrt::get_self<TableViewRow>(row);
    auto const cellsHost = rowImpl ? rowImpl->GetCellsHostPanelInternal() : nullptr;
    if (!cellsHost)
    {
        return {};
    }

    // First collect cheap visible-cell text without creating peers; this runs on every UIA name query.
    std::wstring composed = ComposeCellTexts(rowImpl, cellsHost, false /* allowPeerCreation */);
    if (composed.empty())
    {
        // Only nameless rows fall back to bounded peer creation for visible template content.
        composed = ComposeCellTexts(rowImpl, cellsHost, true /* allowPeerCreation */);
    }

    if (composed.empty())
    {
        // Template content with no name of its own either. The data item is the last thing left
        // that can distinguish this row from its neighbours.
        return ItemToName(row.DataContext());
    }

    return winrt::hstring{ composed };
}
std::wstring TableViewRowAutomationPeer::ComposeCellTexts(
    TableViewRow* rowImpl,
    winrt::Panel const& cellsHost,
    bool allowPeerCreation)
{
    std::wstring composed;
    auto const separator = LocalizedOrFallbackForTableViewAutomation(SR_TableViewCellTextSeparator, L", ");
    auto const cellChildren = cellsHost.Children();
    const auto count = cellChildren.Size();
    for (uint32_t i = 0; i < count; ++i)
    {
        auto const cellElement = cellChildren.GetAt(i).try_as<winrt::UIElement>();
        if (!cellElement || !IsVisibleColumn(rowImpl->GetCellOwningColumn(cellElement)))
        {
            continue;
        }

        auto const text = GetCellDisplayText(cellElement.try_as<winrt::FrameworkElement>(), allowPeerCreation);
        if (text.empty())
        {
            continue;
        }

        if (!composed.empty())
        {
            composed += separator.c_str();
        }
        composed += text;
    }

    return composed;
}

int32_t TableViewRowAutomationPeer::GetPositionInSetCore()
{
    return ReportHierarchyValue(HierarchyValue::PositionInSet, m_lastReportedPositionInSet);
}

int32_t TableViewRowAutomationPeer::GetSizeOfSetCore()
{
    return ReportHierarchyValue(HierarchyValue::SizeOfSet, m_lastReportedSizeOfSet);
}

int32_t TableViewRowAutomationPeer::ReportHierarchyValue(HierarchyValue value, int32_t& lastReported)
{
    winrt::AutomationPeer forwardTo{ nullptr };
    const auto route = RequireRoute(forwardTo);
    lastReported = ComputeHierarchyValue(value, route, forwardTo);
    return lastReported;
}

int32_t TableViewRowAutomationPeer::ComputeHierarchyValue(HierarchyValue value, Route route, winrt::AutomationPeer const& forwardTo)
{
    switch (route)
    {
    case Route::Self:
        break;
    case Route::Forward:
        return value == HierarchyValue::Level ? forwardTo.GetLevel()
            : value == HierarchyValue::PositionInSet ? forwardTo.GetPositionInSet()
            : forwardTo.GetSizeOfSet();
    default:
        return 0;
    }

    // An app-set AutomationProperties value wins, as in every dxaml peer that computes this.
    const auto provided = value == HierarchyValue::Level ? __super::GetLevelCore()
        : value == HierarchyValue::PositionInSet ? __super::GetPositionInSetCore()
        : __super::GetSizeOfSetCore();
    if (provided > 0)
    {
        return provided;
    }

    if (value == HierarchyValue::Level)
    {
        // Level is already 1-based; 0 (not a tree row) is UIA's "not specified".
        auto const row = TryGetBoundRow();
        return row ? row.Level() : 0;
    }

    // A tree row reports its place among its siblings, not its offset into the flat row axis.
    int32_t position = 0;
    int32_t sizeOfSet = 0;
    if (TryGetSiblingPosition(position, sizeOfSet))
    {
        return value == HierarchyValue::PositionInSet ? position : sizeOfSet;
    }

    // 0 is UIA's "not specified"; -1 would reach the client verbatim.
    const auto index = GetRowIndex();
    auto const tableView = GetOwningTableView();
    if (!tableView || (index < 0 && value == HierarchyValue::PositionInSet))
    {
        return 0;
    }

    auto const tableImpl = winrt::get_self<TableView>(tableView);
    int32_t groupPosition = 0;
    int32_t groupSize = 0;
    if (TryGetGroupedRowSetMetadata(tableImpl, index, groupPosition, groupSize))
    {
        return value == HierarchyValue::PositionInSet ? groupPosition : groupSize;
    }
    if (value == HierarchyValue::PositionInSet)
    {
        return tableImpl->GetDataRowPositionInSetInternal(index);
    }
    return std::max(0, tableImpl->GetDataRowSizeOfSetInternal());
}
winrt::AutomationPeer TableViewRowAutomationPeer::GetOrCreateCellPeer(
    winrt::FrameworkElement const& cell)
{
    if (!cell)
    {
        return nullptr;
    }

    // One cache per item: a row's own peer hands out its item peer's cell peers.
    if (auto const itemPeer = GetBoundItemPeer())
    {
        return winrt::get_self<TableViewRowAutomationPeer>(itemPeer)->GetOrCreateCellPeer(cell);
    }

    auto const row = TryGetBoundRow();
    if (m_cellsRow.get() != row)
    {
        m_cellPeerCache.clear();
        m_cellsRow = winrt::make_weak(row);
    }

    for (auto const& entry : m_cellPeerCache)
    {
        if (entry.peer && entry.cell.get() == cell)
        {
            return entry.peer.get();
        }
    }

    if (!row || !GetOwningTableView())
    {
        return nullptr;
    }

    // Pruned here as well as on the GetChildrenCore rebuild: a client that only addresses cells
    // through IGridProvider::GetItem never walks children, so this is its only prune point.
    m_cellPeerCache.erase(
        std::remove_if(
            m_cellPeerCache.begin(),
            m_cellPeerCache.end(),
            [](CellPeerCacheEntry const& entry) { return !entry.peer || !entry.cell.get(); }),
        m_cellPeerCache.end());

    auto const peer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(cell);
    if (!peer)
    {
        return nullptr;
    }
    // GetChildren normally establishes this relationship, but Grid.GetItem can be
    // the first and only acquisition route. Connect that same peer before publishing it.
    peer.SetParent(*this);
    m_cellPeerCache.emplace_back(this, cell, peer);
    return peer;
}

// ----- ISelectionItemProvider -----

void TableViewRowAutomationPeer::SelectionTargetFor(SelectionTarget& target)
{
    if (target.route == Route::Virtualized)
    {
        auto const tableView = m_lastOwningTable.get();
        target.index = GetTrackedItemIndex(tableView);
        target.tableView = target.index >= 0 ? tableView : nullptr;
    }
    else if (target.route != Route::Gone && target.row)
    {
        target.tableView = winrt::get_self<TableViewRow>(target.row)->GetOwningTableView();
        auto const repeater = target.tableView ? winrt::get_self<TableView>(target.tableView)->GetRowsRepeaterInternal() : nullptr;
        target.index = repeater ? repeater.GetElementIndex(target.row) : -1;
    }
}

TableViewRowAutomationPeer::SelectionTarget TableViewRowAutomationPeer::ResolveSelectionTarget()
{
    // A stale peer must fail rather than act on whatever item its old row shows now.
    SelectionTarget target;
    target.route = RequireRoute(target.forwardTo, &target.row);
    SelectionTargetFor(target);
    return target;
}

void TableViewRowAutomationPeer::ThrowIfNotEnabled(SelectionTarget const& target)
{
    const bool enabled = target.route == Route::Forward ? target.forwardTo.IsEnabled()
        : target.route == Route::Virtualized ? IsLastOwningTableEnabled()
        : __super::IsEnabledCore();
    if (!enabled)
    {
        throw winrt::hresult_error(UIA_E_ELEMENTNOTENABLED);
    }
}

bool TableViewRowAutomationPeer::IsSelected()
{
    auto const target = ResolveSelectionTarget();
    if (!target.tableView)
    {
        return false;
    }
    if (target.route == Route::Virtualized)
    {
        return winrt::get_self<TableView>(target.tableView)->IsSelected(target.index);
    }
    return target.row.IsSelected();
}

winrt::IRawElementProviderSimple TableViewRowAutomationPeer::SelectionContainer()
{
    auto const target = ResolveSelectionTarget();
    if (target.tableView)
    {
        if (auto const peer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(target.tableView))
        {
            return ProviderFromPeer(peer);
        }
    }

    return nullptr;
}

void TableViewRowAutomationPeer::AddToSelection()
{
    auto const target = ResolveSelectionTarget();
    ThrowIfNotEnabled(target);
    if (!target.tableView)
    {
        return;
    }

    // Per the UIA SelectionItem spec, single-select fails rather than replacing another row's selection.
    // Ask the model: SelectedIndex and the row's IsSelected lag it while selection observers run.
    auto const impl = winrt::get_self<TableView>(target.tableView);
    if (!impl->CanSelectMultipleRows() && impl->SelectedIndexInternal() >= 0 && !impl->IsSelected(target.index))
    {
        throw winrt::hresult_error(UIA_E_INVALIDOPERATION);
    }
    if (target.index >= 0)
    {
        impl->SelectRowIndexFromInteraction(target.index, false);
    }
}

void TableViewRowAutomationPeer::RemoveFromSelection()
{
    auto const target = ResolveSelectionTarget();
    ThrowIfNotEnabled(target);
    if (target.tableView && target.index >= 0)
    {
        // Deselect only clears when THIS row is the selection, so a stale UIA call
        // cannot wipe out a selection the user has since moved elsewhere.
        winrt::get_self<TableView>(target.tableView)->Deselect(target.index);
    }
}

void TableViewRowAutomationPeer::Select()
{
    auto const target = ResolveSelectionTarget();
    ThrowIfNotEnabled(target);
    if (target.tableView && target.index >= 0)
    {
        // toggle=false: UIA Select() means "make this the selection", never "clear it".
        winrt::get_self<TableView>(target.tableView)->SelectRowIndexFromInteraction(target.index, false);
    }
}
// ----- IExpandCollapseProvider -----

winrt::ExpandCollapseState TableViewRowAutomationPeer::ExpandCollapseState()
{
    auto const row = RequireRealizedRow();
    if (!row.IsExpandable())
    {
        // Leaf or recycled row; non-tree rows never expose the pattern.
        return winrt::ExpandCollapseState::LeafNode;
    }

    return row.IsExpanded()
        ? winrt::ExpandCollapseState::Expanded
        : winrt::ExpandCollapseState::Collapsed;
}

void TableViewRowAutomationPeer::Expand()
{
    SetExpansion(true);
}

void TableViewRowAutomationPeer::Collapse()
{
    SetExpansion(false);
}

void TableViewRowAutomationPeer::SetExpansion(bool expand)
{
    auto const row = RequireRealizedRow();
    if (!row.IsExpandable())
    {
        return;
    }

    // Pass direction, not a toggle: the mutation is deferred, so IsExpanded() can't make it
    // idempotent as ExpandCollapsePattern requires. Same entry point as the chevron gesture.
    if (auto const tableView = GetOwningTableView())
    {
        winrt::get_self<TableView>(tableView)->SetGroupExpansion(row, expand);
    }
}

void TableViewRowAutomationPeer::RaiseExpandCollapseAutomationEvent(
    winrt::ExpandCollapseState oldState,
    winrt::ExpandCollapseState newState)
{
    if (oldState == newState)
    {
        return;
    }

    if (!winrt::AutomationPeer::ListenerExists(winrt::AutomationEvents::PropertyChanged))
    {
        return;
    }

    try
    {
        RaisePropertyChangedEvent(
            winrt::ExpandCollapsePatternIdentifiers::ExpandCollapseStateProperty(),
            box_value(oldState),
            box_value(newState));
    }
    catch (...)
    {
        // best-effort: the tree may be tearing down.
    }
}

void TableViewRowAutomationPeer::RaiseHierarchyPositionChanged()
{
    // A row that left the tree (ClearParentBy, or a recycled row bound to a flat item) still owes
    // a client that read its tree Level the change back.
    if (!IsHierarchicalRow() && m_lastReportedLevel <= 0)
    {
        return;
    }

    try
    {
        winrt::TableViewRow row{ nullptr };
        const auto route = GetRoute(&row);
        auto const forwardTo = route == Route::Forward ? winrt::FrameworkElementAutomationPeer::FromElement(row) : nullptr;

        // Compare against what this peer last reported to a client (recorded by the Get*Core
        // overrides). -1 means never reported, so no client can hold a stale value.
        auto raiseIfChanged = [&](winrt::AutomationProperty const& property, int32_t& last, HierarchyValue value)
        {
            if (last < 0)
            {
                return;
            }
            if (const int32_t current = ComputeHierarchyValue(value, route, forwardTo); last != current)
            {
                RaisePropertyChangedEvent(property, box_value(last), box_value(current));
                last = current;
            }
        };

        raiseIfChanged(winrt::AutomationElementIdentifiers::LevelProperty(), m_lastReportedLevel, HierarchyValue::Level);
        raiseIfChanged(winrt::AutomationElementIdentifiers::PositionInSetProperty(), m_lastReportedPositionInSet, HierarchyValue::PositionInSet);
        raiseIfChanged(winrt::AutomationElementIdentifiers::SizeOfSetProperty(), m_lastReportedSizeOfSet, HierarchyValue::SizeOfSet);
    }
    catch (...)
    {
        // best-effort: the tree may be tearing down.
    }
}

// ----- IAutomationPeerOverrides3 -----

int32_t TableViewRowAutomationPeer::GetLevelCore()
{
    return ReportHierarchyValue(HierarchyValue::Level, m_lastReportedLevel);
}
// Position within the sibling set (not the flat axis), O(1) from the hierarchy descriptor.
bool TableViewRowAutomationPeer::TryGetSiblingPosition(int32_t& positionInSet, int32_t& sizeOfSet)
{
    positionInSet = 0;
    sizeOfSet = 0;

    auto const tableView = GetOwningTableView();
    if (!tableView || !IsHierarchicalRow())
    {
        return false;
    }

    const int32_t index = GetRowIndex();
    if (index < 0)
    {
        return false;
    }

    auto const tableViewImpl = winrt::get_self<TableView>(tableView);

    TableViewRowInfo info{};
    if (!tableViewImpl->TryGetTableViewSourceRowInfo(index, info) ||
        info.Kind != TableViewRowKind::Data ||
        info.SizeOfSet == 0)
    {
        return false;
    }

    positionInSet = static_cast<int32_t>(info.PositionInSet);
    sizeOfSet = static_cast<int32_t>(info.SizeOfSet);
    return true;
}


// See header comment for rationale.
winrt::IVector<winrt::AutomationPeer> TableViewRowAutomationPeer::GetChildrenCore()
{
    // No children is nullptr throughout. The cells are parented to the item peer UIA sees, so they
    // follow it across rows.
    if (GetBoundItemPeer())
    {
        return nullptr;
    }

    winrt::AutomationPeer forwardTo{ nullptr };
    winrt::TableViewRow row{ nullptr };
    if (RequireRoute(forwardTo, &row) == Route::Virtualized)
    {
        return nullptr;
    }
    if (!GetOwningTableView())
    {
        ThrowElementNotAvailable();
    }

    auto const rowImpl = winrt::get_self<TableViewRow>(row);
    auto cellsHost = rowImpl->GetCellsHostPanelInternal();
    if (!cellsHost)
    {
        // Mid-realize fallback: a row's own peer exposes its chrome; an item peer must not walk its creating row's.
        return forwardTo ? nullptr : __super::GetChildrenCore();
    }

    auto children = winrt::single_threaded_vector<winrt::AutomationPeer>();
    const auto cellChildren = cellsHost.Children();
    const auto count = cellChildren.Size();

    // Rebuilt wholesale: peers for cells dropped by a rebuild or recycle are released, while a
    // surviving cell keeps the same peer and therefore the same provider identity.
    std::vector<CellPeerCacheEntry> liveCache;
    liveCache.reserve(count);

    for (uint32_t i = 0; i < count; ++i)
    {
        if (auto const cellElement = cellChildren.GetAt(i).try_as<winrt::UIElement>())
        {
            auto const column = rowImpl->GetCellOwningColumn(cellElement);
            if (!IsVisibleColumn(column))
            {
                continue;
            }

            if (auto const cellFE = cellElement.try_as<winrt::FrameworkElement>())
            {
                // Dedicated cell peers provide names, coordinates, and header references.
                if (auto const cellPeer = GetOrCreateCellPeer(cellFE))
                {
                    liveCache.emplace_back(this, cellFE, cellPeer);
                    children.Append(cellPeer);
                }
            }
        }
    }

    m_cellPeerCache = std::move(liveCache);

    return children;
}
