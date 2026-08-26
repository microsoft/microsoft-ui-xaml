// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewCellsPanel.h"
#include "TableViewAutomationHelpers.h"
#include "TableViewColumnHeaderAutomationPeer.h"
#include "TableViewColumnHeaderAutomationPeer.properties.cpp"

#include <limits>

TableViewColumnHeaderAutomationPeer::TableViewColumnHeaderAutomationPeer(
    winrt::TableView const& owner,
    winrt::TableViewColumn const& column)
    : ReferenceTracker(owner)
    , m_column(winrt::make_weak(column))
{
}

hstring TableViewColumnHeaderAutomationPeer::GetClassNameCore()
{
    return L"TableViewColumnHeader";
}

hstring TableViewColumnHeaderAutomationPeer::GetNameCore()
{
    // Prefer string headers so screen readers announce a distinct column name.
    if (auto const headerString = TryGetColumnHeaderString(m_column.get()))
    {
        return *headerString;
    }

    // For template headers, use the realized header cell and avoid the TableView owner's name.
    if (auto const headerElement = GetHeaderElement())
    {
        if (auto const peer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(headerElement))
        {
            const auto name = peer.GetName();
            if (!name.empty())
            {
                return name;
            }
        }
    }

    return {};
}

winrt::Windows::Foundation::Collections::IVector<winrt::AutomationPeer> TableViewColumnHeaderAutomationPeer::GetChildrenCore()
{
    // Column headers are leaf HeaderItems; do not expose the TableView subtree.
    return winrt::single_threaded_vector<winrt::AutomationPeer>();
}

winrt::AutomationControlType TableViewColumnHeaderAutomationPeer::GetAutomationControlTypeCore()
{
    // HeaderItem is the UIA control type for table column headers.
    return winrt::AutomationControlType::HeaderItem;
}

int32_t TableViewColumnHeaderAutomationPeer::GetPositionInSetCore()
{
    // 1-based column position so AT can announce "column i of n".
    const auto index = GetColumnIndex();
    return index >= 0 ? index + 1 : -1;
}

int32_t TableViewColumnHeaderAutomationPeer::GetSizeOfSetCore()
{
    // Total visible column count, so PositionInSet reads as "i of n".
    if (auto const owner = Owner().try_as<winrt::TableView>())
    {
        if (auto const columns = owner.Columns())
        {
            int32_t count = 0;
            for (auto const& col : columns)
            {
                if (IsVisibleColumn(col)) { ++count; }
            }
            if (count > 0) { return count; }
        }
    }
    return -1;
}

winrt::Windows::Foundation::Rect TableViewColumnHeaderAutomationPeer::GetBoundingRectangleCore()
{
    // Use this header's realized cell bounds; unrealized headers have no on-screen rect.
    if (auto const headerElement = GetHeaderElement())
    {
        if (auto const peer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(headerElement))
        {
            return peer.GetBoundingRectangle();
        }
    }
    return {};
}

winrt::Windows::Foundation::Point TableViewColumnHeaderAutomationPeer::GetClickablePointCore()
{
    if (auto const headerElement = GetHeaderElement())
    {
        if (auto const peer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(headerElement))
        {
            return peer.GetClickablePoint();
        }
    }
    // Unrealized headers have no clickable point (NaN per UIA convention).
    return { std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN() };
}

int32_t TableViewColumnHeaderAutomationPeer::GetColumnIndex()
{
    // Logical visible column index, matching GetSizeOfSetCore's visible basis
    // and the rendered header order, so PositionInSet ("i") and SizeOfSet ("n") stay
    // consistent even when Columns contains null holes or collapsed columns.
    // Columns are matched by identity; a column instance is a single logical position
    // (single-owner model), so the same instance appearing twice in Columns is unsupported.
    if (auto const owner = Owner().try_as<winrt::TableView>())
    {
        if (auto const col = m_column.get())
        {
            if (!IsVisibleColumn(col))
            {
                return -1;
            }

            if (auto const columns = owner.Columns())
            {
                int32_t logicalIndex = 0;
                for (auto const& c : columns)
                {
                    if (c == col) { return logicalIndex; }
                    if (IsVisibleColumn(c)) { ++logicalIndex; }
                }
            }
        }
    }
    return -1;
}

winrt::FrameworkElement TableViewColumnHeaderAutomationPeer::GetHeaderElement()
{
    // Match by Tag so null Columns entries do not skew logical indexes.
    auto const owner = Owner().try_as<winrt::TableView>();
    auto const col = m_column.get();
    if (!owner || !col)
    {
        return nullptr;
    }

    auto const host = winrt::get_self<TableView>(owner)->GetHeaderHostInternal();
    if (!host)
    {
        return nullptr;
    }

    return TableViewCellsPanel::CellForColumn(host, col);
}
