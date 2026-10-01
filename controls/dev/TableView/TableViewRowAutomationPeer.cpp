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
    const auto rowIndex = tableView ? GetTrackedItemIndex(tableView) : -1;
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
    try
    {
        element = repeater.TryGetElement(rowIndex);
        if (!element)
        {
            element = repeater.GetOrCreateElement(rowIndex);
        }
    }
    catch (...)
    {
        ThrowElementNotAvailable();
    }

    if (auto const row = element.try_as<winrt::TableViewRow>())
    {
        if (!IsTrackedRow(row, tableView))
        {
            ThrowElementNotAvailable();
        }
    }
    else
    {
        ThrowElementNotAvailable();
    }

    if (auto const frameworkElement = element.try_as<winrt::FrameworkElement>())
    {
        frameworkElement.StartBringIntoView();
    }
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
        if (auto const candidate = tableImpl->UnwrapEditingDataItem(view.GetAt(m_lastKnownRowIndex));
            m_item.SameIdentityAs(candidate))
        {
            return m_lastKnownRowIndex;
        }
    }

    int32_t occurrence = 0;
    for (int32_t index = 0; index < count; ++index)
    {
        if (auto const candidate = tableImpl->UnwrapEditingDataItem(view.GetAt(index));
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
    if (!tableView || !item || targetIndex < 0)
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
        if (auto const candidate = tableImpl->UnwrapEditingDataItem(view.GetAt(index));
            candidate && TableView::SameInspectableIdentity(candidate, item))
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

    auto const rowItem = winrt::get_self<TableView>(tableView)->UnwrapEditingDataItem(row.DataContext());
    if (!rowItem)
    {
        return false;
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
        if (auto const item = winrt::get_self<TableView>(tableView)->UnwrapEditingDataItem(row.DataContext()))
        {
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

void TableViewRowAutomationPeer::TrackCurrentRowItem(winrt::TableViewRow const& row, winrt::TableView const& tableView)
{
    TrackRowItem(row, tableView);
}

void TableViewRowAutomationPeer::DropCellPeerCache()
{
    m_cellPeerCache.clear();
}

[[noreturn]] void TableViewRowAutomationPeer::ThrowElementNotAvailable()
{
    throw winrt::hresult_error(UIA_E_ELEMENTNOTAVAILABLE);
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
        return winrt::get_self<TableView>(tableView)->GetDataRowPositionInSetInternal(index);
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
        if (const auto count = winrt::get_self<TableView>(tableView)->GetDataRowSizeOfSetInternal(); count > 0)
        {
            return count;
        }
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
    peer.SetParent(*this);
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
    // The container is single-select, so "add" can only mean "make this the selection". XAML's own
    // single-select peers (ListViewItemAutomationPeer) behave the same way rather than failing.
    // Multiple must add to the selection instead of replacing it.
    Select();
}

void TableViewRowAutomationPeer::RemoveFromSelection()
{
    if (auto const tableView = GetOwningTableView())
    {
        if (const int32_t index = GetRowIndex(); index >= 0)
        {
            // Deselect only clears when THIS row is the selection, so a stale UIA call
            // cannot wipe out a selection the user has since moved elsewhere.
            winrt::get_self<TableView>(tableView)->Deselect(index);
        }
    }
}

void TableViewRowAutomationPeer::Select()
{
    if (auto const tableView = GetOwningTableView())
    {
        if (const int32_t index = GetRowIndex(); index >= 0)
        {
            // toggle=false: UIA Select() means "make this the selection", never "clear it".
            winrt::get_self<TableView>(tableView)->SelectRowIndexFromInteraction(index, false);
        }
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
