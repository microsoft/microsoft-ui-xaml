// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewRow.h"
#include "TableViewRowAutomationPeer.h"
#include "TableViewCellAutomationPeer.h"
#include "TableViewAutomationHelpers.h"
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
            auto const repeater = tableImpl->GetRowsRepeaterInternal();
            if (!repeater)
            {
                return false;
            }

            auto const metadata = tableImpl->RowMetadataInternal();
            return metadata && metadata->TryGetGroupMembership(repeater.ItemsSourceView(), rowIndex, positionInSet, sizeOfSet);
        }
        catch (...)
        {
            return false;
        }
    }

    bool TryGetGroupedRowSetMetadata(TableView* tableImpl, int32_t rowIndex, int32_t& positionInSet, int32_t& sizeOfSet)
    {
        if (!tableImpl || rowIndex < 0 || !tableImpl->IsGroupedRowSource())
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
        if (auto const tableView = winrt::get_self<TableViewRow>(owner)->GetOwningTableView())
        {
            TrackRowItem(owner, tableView);
        }
    }
    GetRowIndex();
}

hstring TableViewRowAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::TableViewRow>();
}

winrt::AutomationControlType TableViewRowAutomationPeer::GetAutomationControlTypeCore()
{
    // Data rows are DataItems.
    return winrt::AutomationControlType::DataItem;
}

winrt::IInspectable TableViewRowAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    // SelectionItem is advertised only while the owner can actually select. A recycled row has no
    // owner, so it correctly advertises nothing.
    if (patternInterface == winrt::PatternInterface::SelectionItem)
    {
        if (auto const tableView = GetOwningTableView())
        {
            if (winrt::get_self<TableView>(tableView)->CanSelectRows())
            {
                return *this;
            }
        }
    }

    if (patternInterface == winrt::PatternInterface::VirtualizedItem && IsVirtualized())
    {
        return *this;
    }

    return __super::GetPatternCore(patternInterface);
}

winrt::TableViewRow TableViewRowAutomationPeer::GetRow() const
{
    return Owner().try_as<winrt::TableViewRow>();
}

void TableViewRowAutomationPeer::ShowContextMenuCore()
{
    // A recycled row raising the request would open a menu for whatever item it holds next.
    if (!GetOwningTableView())
    {
        ThrowElementNotAvailable();
    }

    if (auto const row = GetRow(); row && winrt::get_self<TableViewRow>(row)->ShowContextMenuFromPeer())
    {
        return;
    }

    __super::ShowContextMenuCore();
}

winrt::TableView TableViewRowAutomationPeer::GetOwningTableView()
{
    if (auto const row = Owner().try_as<winrt::TableViewRow>())
    {
        auto const tableView = winrt::get_self<TableViewRow>(row)->GetOwningTableView();
        if (tableView && IsTrackedRow(row, tableView))
        {
            m_lastOwningTable = winrt::make_weak(tableView);
            return tableView;
        }
    }

    return nullptr;
}

int32_t TableViewRowAutomationPeer::GetRowIndex()
{
    auto const row = Owner().try_as<winrt::TableViewRow>();
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
    if (GetRowIndex() >= 0)
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
        [this, &tableView](winrt::TableViewRow const& row) { return IsTrackedRow(row, tableView); });
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

winrt::IInspectable TableViewRowAutomationPeer::GetTrackedItem() const
{
    return m_item.Resolve();
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
    if (m_lastKnownRowIndex >= 0 && m_lastKnownRowIndex < count)
    {
        if (auto const candidate = tableImpl->UnwrapDataItem(view.GetAt(m_lastKnownRowIndex));
            m_item.SameIdentityAs(candidate))
        {
            return m_lastKnownRowIndex;
        }
    }

    int32_t occurrence = 0;
    for (int32_t index = 0; index < count; ++index)
    {
        if (auto const candidate = tableImpl->UnwrapDataItem(view.GetAt(index));
            m_item.SameIdentityAs(candidate))
        {
            if (m_trackedItemOccurrence < 0 || occurrence == m_trackedItemOccurrence)
            {
                return index;
            }
            ++occurrence;
        }
    }

    return -1;
}

int32_t TableViewRowAutomationPeer::GetItemOccurrenceAtIndex(
    winrt::TableView const& tableView,
    winrt::IInspectable const& item,
    int32_t targetIndex)
{
    if (!tableView || targetIndex < 0)
    {
        return -1;
    }

    auto const tableImpl = winrt::get_self<TableView>(tableView);
    auto const repeater = tableImpl->GetRowsRepeaterInternal();
    auto const view = repeater ? repeater.ItemsSourceView() : nullptr;
    if (!view || targetIndex >= view.Count())
    {
        return -1;
    }

    int32_t occurrence = 0;
    for (int32_t index = 0; index <= targetIndex; ++index)
    {
        if (auto const candidate = tableImpl->UnwrapDataItem(view.GetAt(index));
            TableView::SameInspectableIdentity(candidate, item))
        {
            if (index == targetIndex)
            {
                return occurrence;
            }
            ++occurrence;
        }
    }

    return -1;
}

bool TableViewRowAutomationPeer::IsTrackedRow(winrt::TableViewRow const& row, winrt::TableView const& tableView)
{
    if (!row || !tableView)
    {
        return false;
    }

    auto const rowItem = winrt::get_self<TableView>(tableView)->UnwrapDataItem(row.DataContext());
    if (!rowItem)
    {
        auto const repeater = winrt::get_self<TableView>(tableView)->GetRowsRepeaterInternal();
        const auto index = repeater ? repeater.GetElementIndex(row) : -1;
        auto const view = repeater ? repeater.ItemsSourceView() : nullptr;
        if (!view || index < 0 || index >= view.Count() ||
            winrt::get_self<TableView>(tableView)->UnwrapDataItem(view.GetAt(index)))
        {
            return false;
        }
    }

    if (!m_item.IsTracking())
    {
        TrackRowItem(row, tableView);
        return true;
    }

    if (!m_item.SameIdentityAs(rowItem))
    {
        return false;
    }

    if (auto const repeater = winrt::get_self<TableView>(tableView)->GetRowsRepeaterInternal())
    {
        if (const auto rowIndex = repeater.GetElementIndex(row); rowIndex >= 0)
        {
            const auto occurrence = GetItemOccurrenceAtIndex(tableView, rowItem, rowIndex);
            if (m_trackedItemOccurrence >= 0 && occurrence != m_trackedItemOccurrence)
            {
                return false;
            }

            TrackRowItem(row, tableView);
        }
    }

    return true;
}

void TableViewRowAutomationPeer::TrackRowItem(winrt::TableViewRow const& row, winrt::TableView const& tableView)
{
    if (row && tableView)
    {
        auto const item = winrt::get_self<TableView>(tableView)->UnwrapDataItem(row.DataContext());
        m_item.Track(item);
        m_trackedItemOccurrence = -1;
        if (auto const repeater = winrt::get_self<TableView>(tableView)->GetRowsRepeaterInternal())
        {
            if (const auto rowIndex = repeater.GetElementIndex(row); rowIndex >= 0)
            {
                m_lastKnownRowIndex = rowIndex;
                m_trackedItemOccurrence = GetItemOccurrenceAtIndex(tableView, item, rowIndex);
            }
        }
    }
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

    auto const item = winrt::get_self<TableView>(tableView)->UnwrapDataItem(row.DataContext());
    if (!item)
    {
        return IsTrackedRow(row, tableView);
    }
    return m_item.SameIdentityAs(item);
}

void TableViewRowAutomationPeer::TrackCurrentRowItem(winrt::TableViewRow const& row, winrt::TableView const& tableView)
{
    TrackRowItem(row, tableView);
}

void TableViewRowAutomationPeer::DropCellPeerCache()
{
    m_cellPeerCache.clear();
}

hstring TableViewRowAutomationPeer::GetNameCore()
{
    auto const row = Owner().try_as<winrt::TableViewRow>();
    if (!row)
    {
        return {};
    }
    if (!GetOwningTableView())
    {
        ThrowElementNotAvailable();
    }

    // Preserve explicit app metadata, not a base peer's Content/DataContext stringification.
    if (auto const name = winrt::AutomationProperties::GetName(Owner()); !name.empty())
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
    // An app-set AutomationProperties value wins, as in every dxaml peer that computes this.
    if (const auto provided = __super::GetPositionInSetCore(); provided > 0)
    {
        return provided;
    }

    const auto index = GetRowIndex();
    if (index < 0)
    {
        // 0 is UIA's "not specified"; -1 would reach the client verbatim.
        return 0;
    }

    if (auto const tableView = GetOwningTableView())
    {
        auto const tableImpl = winrt::get_self<TableView>(tableView);

        int32_t groupPosition = 0;
        int32_t groupSize = 0;
        if (TryGetGroupedRowSetMetadata(tableImpl, index, groupPosition, groupSize))
        {
            return groupPosition;
        }

        return tableImpl->GetDataRowPositionInSetInternal(index);
    }

    return 0;
}

int32_t TableViewRowAutomationPeer::GetSizeOfSetCore()
{
    if (const auto provided = __super::GetSizeOfSetCore(); provided > 0)
    {
        return provided;
    }

    if (auto const tableView = GetOwningTableView())
    {
        auto const tableImpl = winrt::get_self<TableView>(tableView);

        int32_t groupPosition = 0;
        int32_t groupSize = 0;
        if (TryGetGroupedRowSetMetadata(tableImpl, GetRowIndex(), groupPosition, groupSize))
        {
            return groupSize;
        }

        if (const auto count = tableImpl->GetDataRowSizeOfSetInternal(); count > 0)
        {
            return count;
        }
    }

    return 0;
}

int32_t TableViewRowAutomationPeer::GetLevelCore()
{
    if (const auto provided = __super::GetLevelCore(); provided > 0)
    {
        return provided;
    }

    if (auto const tableView = GetOwningTableView())
    {
        return winrt::get_self<TableView>(tableView)->GetGroupedRowAutomationLevelInternal(GetRowIndex());
    }
    return 0;
}

winrt::AutomationPeer TableViewRowAutomationPeer::GetOrCreateCellPeer(
    winrt::FrameworkElement const& cell)
{
    if (!cell)
    {
        return nullptr;
    }

    for (auto const& entry : m_cellPeerCache)
    {
        if (entry.peer && entry.cell.get() == cell)
        {
            return entry.peer.get();
        }
    }

    auto const row = Owner().try_as<winrt::TableViewRow>();
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
    winrt::AutomationPeer parent = EventsSource();
    if (!parent)
    {
        parent = *this;
    }
    peer.SetParent(parent);
    m_cellPeerCache.emplace_back(this, cell, peer);
    return peer;
}

// ----- ISelectionItemProvider -----

bool TableViewRowAutomationPeer::IsSelected()
{
    if (auto const row = Owner().try_as<winrt::TableViewRow>())
    {
        if (!GetOwningTableView())
        {
            return false;
        }
        return row.IsSelected();
    }

    return false;
}

winrt::IRawElementProviderSimple TableViewRowAutomationPeer::SelectionContainer()
{
    if (auto const tableView = GetOwningTableView())
    {
        if (auto const peer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(tableView))
        {
            return ProviderFromPeer(peer);
        }
    }

    return nullptr;
}

void TableViewRowAutomationPeer::AddToSelection()
{
    ApplySelectionItemAction(SelectionItemAction::Add);
}

void TableViewRowAutomationPeer::RemoveFromSelection()
{
    ApplySelectionItemAction(SelectionItemAction::Remove);
}

void TableViewRowAutomationPeer::Select()
{
    ApplySelectionItemAction(SelectionItemAction::Select);
}

void TableViewRowAutomationPeer::ApplySelectionItemAction(SelectionItemAction action)
{
    // Enabled first: an app-derived IsEnabledCore can change the rows the index is resolved against.
    ThrowIfSelectionItemNotEnabled(IsEnabled());
    auto const tableView = GetOwningTableView();
    const int32_t rowIndex = GetRowIndex();
    ApplySelectionItemActionAt(action, tableView, rowIndex);
}

void TableViewRowAutomationPeer::ThrowIfSelectionItemNotEnabled(bool isEnabled)
{
    if (!isEnabled)
    {
        throw winrt::hresult_error(UIA_E_ELEMENTNOTENABLED);
    }
}

void TableViewRowAutomationPeer::ApplySelectionItemActionAt(
    SelectionItemAction action,
    winrt::TableView const& tableView,
    int32_t rowIndex)
{
    if (!tableView)
    {
        return;
    }

    auto const impl = winrt::get_self<TableView>(tableView);
    // Per the UIA SelectionItem spec, single-select fails rather than replacing another row's selection.
    // Ask the model: SelectedIndex and the row's IsSelected lag it while selection observers run.
    if (action == SelectionItemAction::Add)
    {
        EnsureCanAddRowToSelection(*impl, rowIndex);
    }
    if (rowIndex < 0)
    {
        return;
    }

    if (action == SelectionItemAction::Remove)
    {
        // Deselect only clears when THIS row is the selection, so a stale UIA call
        // cannot wipe out a selection the user has since moved elsewhere.
        impl->Deselect(rowIndex);
    }
    else
    {
        // toggle=false: UIA Select() means "make this the selection", never "clear it".
        impl->SelectRowIndexFromInteraction(rowIndex, false);
    }
}

// See header comment for rationale.
winrt::IVector<winrt::AutomationPeer> TableViewRowAutomationPeer::GetChildrenCore()
{
    auto children = winrt::single_threaded_vector<winrt::AutomationPeer>();

    auto const row = Owner().try_as<winrt::TableViewRow>();
    if (!row)
    {
        return children;
    }
    if (!GetOwningTableView())
    {
        ThrowElementNotAvailable();
    }

    auto const rowImpl = winrt::get_self<TableViewRow>(row);
    if (!rowImpl)
    {
        return children;
    }

    auto cellsHost = rowImpl->GetCellsHostPanelInternal();
    if (!cellsHost)
    {
        // Mid-realize fallback: expose row chrome rather than an empty vector.
        return __super::GetChildrenCore();
    }

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
