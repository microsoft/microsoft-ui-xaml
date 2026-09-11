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

#include <algorithm>
#include <string>

TableViewRowAutomationPeer::TableViewRowAutomationPeer(winrt::TableViewRow const& owner)
    : ReferenceTracker(owner)
{
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

    return __super::GetPatternCore(patternInterface);
}

winrt::TableView TableViewRowAutomationPeer::GetOwningTableView()
{
    if (auto const row = Owner().try_as<winrt::TableViewRow>())
    {
        return winrt::get_self<TableViewRow>(row)->GetOwningTableView();
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
        return repeater.GetElementIndex(row);
    }

    return -1;
}

hstring TableViewRowAutomationPeer::GetNameCore()
{
    // An explicit AutomationProperties.Name on the row always wins.
    if (auto const name = __super::GetNameCore(); !name.empty())
    {
        return name;
    }

    auto const row = Owner().try_as<winrt::TableViewRow>();
    if (!row)
    {
        return {};
    }

    auto const rowImpl = winrt::get_self<TableViewRow>(row);
    auto const cellsHost = rowImpl ? rowImpl->GetCellsHostPanelInternal() : nullptr;
    if (!cellsHost)
    {
        return {};
    }

    // Visible cells only, in visual order, so the announcement matches what a sighted user reads
    // across the row. Empty cells are skipped rather than producing runs of separators.
    std::wstring composed;
    auto const cellChildren = cellsHost.Children();
    const auto count = cellChildren.Size();
    for (uint32_t i = 0; i < count; ++i)
    {
        auto const cellElement = cellChildren.GetAt(i).try_as<winrt::UIElement>();
        if (!cellElement || !IsVisibleColumn(rowImpl->GetCellOwningColumn(cellElement)))
        {
            continue;
        }

        auto const text = GetCellDisplayText(cellElement.try_as<winrt::FrameworkElement>(), false /* allowPeerCreation */);
        if (text.empty())
        {
            continue;
        }

        if (!composed.empty())
        {
            composed += L", ";
        }
        composed += text;
    }

    return winrt::hstring{ composed };
}

int32_t TableViewRowAutomationPeer::GetPositionInSetCore()
{
    // An app-set AutomationProperties.PositionInSet wins, matching how every dxaml peer that
    // computes this property defers to the container value first.
    if (const auto provided = __super::GetPositionInSetCore(); provided > 0)
    {
        return provided;
    }

    const auto index = GetRowIndex();
    if (index < 0)
    {
        // 0, not -1: 0 is UIA's "not specified". An unrealized row honestly reports unknown.
        return 0;
    }

    // Grouped: report the position WITHIN the owning group and exclude the header bands, which is
    // what ItemsControlAutomationPeer does (indexInsideGroup) and what NavigationView and TreeView
    // do in this repo. A global index counted over a projection that interleaves headers and rows
    // would announce a number the user cannot relate to anything on screen.
    if (int32_t positionInGroup = 0, sizeOfGroup = 0; TryGetGroupPosition(index, positionInGroup, sizeOfGroup))
    {
        return positionInGroup;
    }

    return index + 1;
}

int32_t TableViewRowAutomationPeer::GetSizeOfSetCore()
{
    if (const auto provided = __super::GetSizeOfSetCore(); provided > 0)
    {
        return provided;
    }

    if (const auto index = GetRowIndex(); index >= 0)
    {
        if (int32_t positionInGroup = 0, sizeOfGroup = 0; TryGetGroupPosition(index, positionInGroup, sizeOfGroup))
        {
            return sizeOfGroup;
        }
    }

    // Ungrouped: the full projection, the same basis TableViewAutomationPeer::RowCount and GetItem
    // use, so "i of n" stays consistent with grid addressing.
    if (auto const tableView = GetOwningTableView())
    {
        if (const auto count = winrt::get_self<TableView>(tableView)->GetRowCountInternal(); count > 0)
        {
            return count;
        }
    }

    return 0;
}

// Resolves a data row's 1-based position within its group and the group's item count. Returns false
// when the source is not grouped, so callers fall back to the flat basis.
bool TableViewRowAutomationPeer::TryGetGroupPosition(int32_t rowIndex, int32_t& positionInGroup, int32_t& sizeOfGroup)
{
    auto const tableView = GetOwningTableView();
    if (!tableView)
    {
        return false;
    }

    auto const tableViewImpl = winrt::get_self<TableView>(tableView);
    if (!tableViewImpl->IsTableViewSourceGrouped())
    {
        return false;
    }

    // Walk back to the owning header. Bounded by the group's size, not the row count - the same
    // shape as NavigationViewItemAutomationPeer, which walks its repeater resetting at each header.
    for (int32_t i = rowIndex - 1; i >= 0; --i)
    {
        TableViewRowInfo info{};
        if (!tableViewImpl->TryGetTableViewSourceRowInfo(i, info))
        {
            return false;
        }

        if (info.Kind == TableViewRowKind::GroupHeader)
        {
            positionInGroup = rowIndex - i;
            // ChildCount is the group's item count and the single source of truth for group size.
            sizeOfGroup = info.ChildCount;
            return sizeOfGroup > 0;
        }
    }

    return false;
}

winrt::AutomationPeer TableViewRowAutomationPeer::GetOrCreateCellPeer(
    winrt::FrameworkElement const& cell,
    winrt::TableViewColumn const& column,
    int32_t visibleColumnIndex)
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
    if (!row)
    {
        return nullptr;
    }

    // Pruned on the miss path, not only when GetChildrenCore rebuilds: a client that only ever
    // addresses cells through IGridProvider::GetItem never walks children, so without this the
    // vector would keep every entry whose cell has since been released. Mirrors the identical
    // prune in TableViewAutomationPeer::GetOrCreateColumnHeaderPeer.
    m_cellPeerCache.erase(
        std::remove_if(
            m_cellPeerCache.begin(),
            m_cellPeerCache.end(),
            [](CellPeerCacheEntry const& entry) { return !entry.peer || !entry.cell.get(); }),
        m_cellPeerCache.end());

    // Cached on miss so Grid.GetItem - which can be the first, or only, path a client takes to a
    // cell - still yields a provider whose identity survives the next query.
    winrt::AutomationPeer const peer = winrt::make<TableViewCellAutomationPeer>(cell, row, column, visibleColumnIndex);
    m_cellPeerCache.emplace_back(this, cell, peer);
    return peer;
}

// ----- ISelectionItemProvider -----

bool TableViewRowAutomationPeer::IsSelected()
{
    if (auto const row = Owner().try_as<winrt::TableViewRow>())
    {
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
    int32_t visibleColumnIndex = 0;

    // Rebuilt wholesale so peers for cells dropped by a rebuild or a recycle are released here,
    // while a cell that survives keeps the very same peer - and therefore the same UIA provider
    // identity - across enumerations.
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
                if (auto const cellPeer = GetOrCreateCellPeer(cellFE, column, visibleColumnIndex))
                {
                    liveCache.emplace_back(this, cellFE, cellPeer);
                    children.Append(cellPeer);
                }
            }
            ++visibleColumnIndex;
        }
    }

    m_cellPeerCache = std::move(liveCache);

    return children;
}
