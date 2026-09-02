// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewCellsPanel.h"
#include "TableViewAutomationHelpers.h"
#include "TableViewColumnHeaderAutomationPeer.h"
#include "TableViewToolTipHelpers.h"
#include "TableViewColumnHeaderAutomationPeer.properties.cpp"
#include "ResourceAccessor.h"
#include "Utils.h"

#include <limits>

namespace
{
    // Two 32-bit halves of the column's stable IUnknown, which is the cheapest per-column
    // identity available here. Widen to 64-bit before shifting so this stays correct on 32-bit,
    // where uintptr_t is 32-bit and `>> 32` would be an out-of-range shift; the high part is
    // simply 0 there.
    std::array<int32_t, 2> RuntimeIdPartsForColumn(winrt::TableViewColumn const& column)
    {
        if (!column)
        {
            return { 0, 0 };
        }

        const uint64_t identity = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(winrt::get_unknown(column)));
        return
        {
            static_cast<int32_t>(identity & 0xffffffffull),
            static_cast<int32_t>((identity >> 32) & 0xffffffffull)
        };
    }
}

TableViewColumnHeaderAutomationPeer::TableViewColumnHeaderAutomationPeer(
    winrt::TableView const& owner,
    winrt::TableViewColumn const& column)
    : ReferenceTracker(owner)
    , m_column(winrt::make_weak(column))
    , m_columnRuntimeIdParts(RuntimeIdPartsForColumn(column))
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

winrt::com_array<int32_t> TableViewColumnHeaderAutomationPeer::GetRuntimeIdCore()
{
    // Header peers are all owned by the TableView, so the owner-derived RuntimeId the base
    // would supply is identical for every column - a UIA protocol violation that makes the
    // headers indistinguishable to assistive technology. Build a self-contained id instead:
    // the UiaAppendRuntimeId prefix keeps it well-formed as a framework-appended runtime id,
    // the control-family tag namespaces it, and the column identity parts make it unique and
    // stable for the lifetime of the column.
    return winrt::com_array<int32_t>({
        3, // UiaAppendRuntimeId
        static_cast<int32_t>(0x54564348), // 'TVCH' control-family tag
        m_columnRuntimeIdParts[0],
        m_columnRuntimeIdParts[1]
    });
}

hstring TableViewColumnHeaderAutomationPeer::GetAutomationIdCore()
{
    // An author-supplied id on the realized header always wins.
    if (auto const headerElement = GetHeaderElement())
    {
        if (auto const automationId = winrt::AutomationProperties::GetAutomationId(headerElement);
            !automationId.empty())
        {
            return automationId;
        }
    }

    // Otherwise fall back to the same column identity backing the RuntimeId, so headers stay
    // addressable in UI automation before their templates realize.
    std::wstring automationId{ L"TableViewColumnHeader_" };
    automationId.append(std::to_wstring(m_columnRuntimeIdParts[0]));
    automationId.push_back(L'_');
    automationId.append(std::to_wstring(m_columnRuntimeIdParts[1]));
    return hstring{ automationId };
}

hstring TableViewColumnHeaderAutomationPeer::GetHelpTextCore()
{
    auto const column = m_column.get();
    if (!column)
    {
        return __super::GetHelpTextCore();
    }

    // Only a column the control will actually sort reports a sort state; on any other column the
    // absence of a sort state is the honest answer.
    winrt::hstring sortText{};
    if (IsSortableColumn())
    {
        switch (column.SortDirection())
        {
        case winrt::SortDirection::Ascending:
            sortText = ResourceAccessor::GetLocalizedStringResource(SR_TableViewSortAscendingHelpText);
            break;
        case winrt::SortDirection::Descending:
            sortText = ResourceAccessor::GetLocalizedStringResource(SR_TableViewSortDescendingHelpText);
            break;
        case winrt::SortDirection::None:
        default:
            sortText = ResourceAccessor::GetLocalizedStringResource(SR_TableViewSortNoneHelpText);
            break;
        }
    }

    // Read from the realized header: this peer is virtual, so the element's AutomationProperties
    // would never reach a client. String content only, matching the cell rule.
    winrt::hstring toolTipText{};
    if (auto const headerElement = GetHeaderElement())
    {
        if (auto const text = TableViewDetails::TryGetOwnedToolTipText(headerElement))
        {
            toolTipText = *text;
        }
    }

    // Dropped when it merely repeats the header's own name, so it is not announced twice. Only a
    // control-owned tooltip reaches here, so text the app set is never affected.
    if (!toolTipText.empty() && toolTipText == GetNameCore())
    {
        toolTipText = {};
    }

    if (toolTipText.empty())
    {
        return sortText.empty() ? __super::GetHelpTextCore() : sortText;
    }

    if (sortText.empty())
    {
        return toolTipText;
    }

    // Both carry information a sighted user gets at a glance, so neither is dropped.
    auto const format = ResourceAccessor::GetLocalizedStringResource(SR_TableViewColumnHeaderHelpTextFormat);
    if (format.empty())
    {
        return toolTipText;
    }

    return StringUtil::FormatString(format, toolTipText.c_str(), sortText.c_str());
}

winrt::IInspectable TableViewColumnHeaderAutomationPeer::GetPatternCore(winrt::PatternInterface patternInterface)
{
    if (patternInterface == winrt::PatternInterface::Invoke && IsSortableColumn())
    {
        return *this;
    }

    return __super::GetPatternCore(patternInterface);
}

void TableViewColumnHeaderAutomationPeer::Invoke()
{
    if (auto const column = m_column.get())
    {
        if (auto const owner = Owner().try_as<winrt::TableView>())
        {
            winrt::get_self<TableView>(owner)->ToggleSortDirection(column);
        }
    }
}

bool TableViewColumnHeaderAutomationPeer::IsSortableColumn()
{
    auto const column = m_column.get();
    if (!column || !column.CanSort())
    {
        return false;
    }

    auto const owner = Owner().try_as<winrt::TableView>();
    return owner && owner.CanUserSortColumns();
}

int32_t TableViewColumnHeaderAutomationPeer::GetPositionInSetCore()
{
    // Complements the distinct RuntimeId and GetNameCore: expose the 1-based visible column
    // position so AT (Narrator) can announce "column i of n" as the user moves across headers.
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
