// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewRow.h"
#include "TableViewRowAutomationPeer.h"
#include "TableViewCellAutomationPeer.h"
#include "TableViewRowAutomationPeer.properties.cpp"

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

    // ExpandCollapse is conditional here, unlike on the group header: that container is only ever
    // a group, whereas a row is a tree node only under a hierarchical source. Advertising the
    // pattern on a flat or grouped table would report every row as a LeafNode, telling a screen
    // reader the grid is a tree that happens to be fully collapsed.
    //
    // Keyed off Level rather than IsExpandable so a LEAF of a tree keeps the pattern and reports
    // LeafNode, which is what lets a client tell "nothing to expand" apart from "not a tree".
    if (patternInterface == winrt::PatternInterface::ExpandCollapse && IsHierarchicalRow())
    {
        return *this;
    }

    return __super::GetPatternCore(patternInterface);
}

winrt::TableViewRow TableViewRowAutomationPeer::GetRow() const
{
    return Owner().try_as<winrt::TableViewRow>();
}

bool TableViewRowAutomationPeer::IsHierarchicalRow() const
{
    // Level is 0 for flat and grouped sources and 1-based for tree rows, which is exactly the
    // "is this a tree node" question -- see the Level DP's comment in TableView.idl.
    auto const row = GetRow();
    return row && row.Level() > 0;
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

// ----- IExpandCollapseProvider -----

winrt::ExpandCollapseState TableViewRowAutomationPeer::ExpandCollapseState()
{
    auto const row = GetRow();
    if (!row || !row.IsExpandable())
    {
        // Covers both a genuine leaf and a recycled row. A client that asked for the pattern on a
        // non-tree row cannot get here, because GetPatternCore withheld it.
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
    auto const row = GetRow();
    if (!row || !row.IsExpandable())
    {
        return;
    }

    // Direction is passed through rather than resolved into a toggle here, for the reason spelled
    // out in TableViewGroupHeaderAutomationPeer::SetExpansion: the mutation is applied on a later
    // turn, so a guard reading IsExpanded() (the last state pushed to this container) cannot make
    // the request directional, and ExpandCollapsePattern requires Expand/Collapse to be idempotent.
    //
    // SetGroupExpansion is the shared entry point for both axes -- it resolves the container's
    // identity while its index is still current and defers only the reshape -- which is the same
    // path the chevron gesture takes in TableViewRow::OnExpanderGutterPointerPressed.
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

// ----- IAutomationPeerOverrides3 -----

int32_t TableViewRowAutomationPeer::GetLevelCore()
{
    // The Level DP is already 1-based precisely so it can be handed to UIA unmodified. -1 is UIA's
    // "not applicable", which is what a flat or grouped row must report -- 0 would claim a real
    // level and make an ordinary table sound like a tree. TreeViewItemAutomationPeer uses the same
    // -1 default.
    auto const row = GetRow();
    if (!row)
    {
        return -1;
    }

    const int32_t level = row.Level();
    return level > 0 ? level : -1;
}

int32_t TableViewRowAutomationPeer::GetPositionInSetCore()
{
    int32_t position = 0;
    int32_t sizeOfSet = 0;
    return TryGetSiblingPosition(position, sizeOfSet) ? position : 0;
}

int32_t TableViewRowAutomationPeer::GetSizeOfSetCore()
{
    int32_t position = 0;
    int32_t sizeOfSet = 0;
    return TryGetSiblingPosition(position, sizeOfSet) ? sizeOfSet : 0;
}

// Counts this row's position within its SIBLING set by walking the flat row axis outward from this
// row, which is where the sibling set lives: a collapsed sibling is still a row (only its
// DESCENDANTS leave the axis), so every sibling is present and the walk sees all of them.
//
// Reporting flat-axis coordinates instead would have a screen reader announce "4,217 of 900,000"
// for the second child of a node. The walk costs O(siblings + their visible descendants), which is
// the price of not storing a parallel sibling index; it runs only when a client asks.
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

    TableViewRowInfo selfInfo{};
    if (!tableViewImpl->TryGetTableViewSourceRowInfo(index, selfInfo) ||
        selfInfo.Kind != TableViewRowKind::Data)
    {
        return false;
    }

    const int32_t level = selfInfo.Level;
    if (level <= 0)
    {
        return false;
    }

    // Walks until the sibling set is bounded: a shallower data row is the parent, and a group
    // header is a bucket boundary, because a grouped tree buckets only the roots.
    auto countSiblings = [tableViewImpl, level](int32_t from, int32_t step, int32_t limit)
    {
        int32_t count = 0;
        for (int32_t i = from; i != limit; i += step)
        {
            TableViewRowInfo info{};
            if (!tableViewImpl->TryGetTableViewSourceRowInfo(i, info))
            {
                break;
            }

            if (info.Kind != TableViewRowKind::Data || info.Level < level)
            {
                break;
            }

            if (info.Level == level)
            {
                ++count;
            }
            // info.Level > level is a descendant of a sibling: skipped, not counted.
        }
        return count;
    };

    const int32_t rowCount = tableViewImpl->GetRowCountInternal();
    const int32_t before = countSiblings(index - 1, -1, -1);
    const int32_t after = countSiblings(index + 1, 1, rowCount);

    positionInSet = before + 1;
    sizeOfSet = before + 1 + after;
    return true;
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
    for (uint32_t i = 0; i < count; ++i)
    {
        if (auto const cellElement = cellChildren.GetAt(i).try_as<winrt::UIElement>())
        {
            auto const column = rowImpl->GetCellOwningColumn(cellElement);
            if (!column || column.Visibility() != winrt::Visibility::Visible)
            {
                continue;
            }

            if (auto const cellFE = cellElement.try_as<winrt::FrameworkElement>())
            {
                // Dedicated cell peers provide names, coordinates, and header references.
                winrt::AutomationPeer const cellPeer =
                    winrt::make<TableViewCellAutomationPeer>(cellFE, row, column, visibleColumnIndex);
                children.Append(cellPeer);
            }
            ++visibleColumnIndex;
        }
    }

    return children;
}
