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
#include <UIAutomationCore.h>
#include <UIAutomationCoreApi.h>

namespace
{
    winrt::FrameworkElement ResolveHeader(winrt::TableView const& table, winrt::TableViewColumn const& column)
    {
        if (table && column)
        {
            if (auto const host = winrt::get_self<TableView>(table)->GetHeaderHostInternal())
            {
                if (auto const header = TableViewCellsPanel::CellForColumn(host, column))
                {
                    return header;
                }
            }
        }
        throw winrt::hresult_invalid_argument(L"The column must have a realized header.");
    }

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
    : TableViewColumnHeaderAutomationPeer(ResolveHeader(owner, column), owner, column)
{
}

TableViewColumnHeaderAutomationPeer::TableViewColumnHeaderAutomationPeer(
    winrt::FrameworkElement const& header,
    winrt::TableView const& table,
    winrt::TableViewColumn const& column)
    : ReferenceTracker(header)
    , m_column(winrt::make_weak(column))
    , m_table(winrt::make_weak(table))
    , m_columnRuntimeIdParts(RuntimeIdPartsForColumn(column))
{
}

hstring TableViewColumnHeaderAutomationPeer::GetClassNameCore()
{
    return L"TableViewColumnHeader";
}

hstring TableViewColumnHeaderAutomationPeer::GetNameCore()
{
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

    if (auto const headerString = TryGetColumnHeaderString(m_column.get()))
    {
        return *headerString;
    }

    // Read template content, never re-enter this header's own peer.
    if (auto const header = Owner().try_as<winrt::Panel>())
    {
        uint32_t remaining = 64;
        for (auto const& child : header.Children())
        {
            if (auto const name = GetCellContentName(child.try_as<winrt::FrameworkElement>(), true, 8, remaining);
                !name.empty())
            {
                return name;
            }
        }
    }

    return {};
}

winrt::AutomationControlType TableViewColumnHeaderAutomationPeer::GetAutomationControlTypeCore()
{
    // HeaderItem is the UIA control type for table column headers.
    return winrt::AutomationControlType::HeaderItem;
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

    // Keep a column-derived fallback AutomationId; RuntimeId belongs to the visual.
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
            sortText = TryGetLocalizedString(SR_TableViewSortAscendingHelpText);
            break;
        case winrt::SortDirection::Descending:
            sortText = TryGetLocalizedString(SR_TableViewSortDescendingHelpText);
            break;
        case winrt::SortDirection::None:
        default:
            sortText = TryGetLocalizedString(SR_TableViewSortNoneHelpText);
            break;
        }
    }

    // String only; rich content is mouse-only, as with cells.
    winrt::hstring toolTipText{};
    if (auto const text = TableViewDetails::TryGetString(column.HeaderToolTip()))
    {
        toolTipText = *text;
    }

    // Dropped when it repeats the header name, to avoid a double announcement.
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

    // A sighted user gets both at a glance, so neither is dropped.
    auto const format = TryGetLocalizedString(SR_TableViewColumnHeaderHelpTextFormat);
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

bool TableViewColumnHeaderAutomationPeer::IsEnabledCore()
{
    // Grid is not a Control, so its base peer does not report inherited disabled state.
    auto const table = m_table.get();
    if (!table || !table.IsEnabled())
    {
        return false;
    }

    // A template control (for example the header ScrollViewer) can be disabled
    // independently of the table. Its coerced state applies to this header too.
    auto ancestor = winrt::VisualTreeHelper::GetParent(Owner());
    while (ancestor && ancestor != table)
    {
        if (auto const control = ancestor.try_as<winrt::Control>(); control && !control.IsEnabled())
        {
            return false;
        }
        ancestor = winrt::VisualTreeHelper::GetParent(ancestor);
    }
    return true;
}

void TableViewColumnHeaderAutomationPeer::Invoke()
{
    if (auto const header = GetHeaderElement(); !header || !header.IsLoaded())
    {
        throw winrt::hresult_error(UIA_E_ELEMENTNOTAVAILABLE);
    }
    if (!IsEnabled())
    {
        throw winrt::hresult_error(UIA_E_ELEMENTNOTENABLED);
    }
    if (!IsSortableColumn())
    {
        throw winrt::hresult_illegal_method_call();
    }
    if (auto const column = m_column.get())
    {
        if (auto const owner = m_table.get())
        {
            winrt::get_self<TableView>(owner)->ToggleSortDirection(column);
        }
    }
}

bool TableViewColumnHeaderAutomationPeer::IsSortableColumn()
{
    auto const column = m_column.get();
    if (!column || !column.CanSort() || !IsVisibleColumn(column) || !GetHeaderElement())
    {
        return false;
    }

    auto const owner = m_table.get();
    return owner && owner.IsLoaded() && owner.CanUserSortColumns();
}

int32_t TableViewColumnHeaderAutomationPeer::GetPositionInSetCore()
{
    // The header's attached override wins over the visible-column position.
    if (auto const header = GetHeaderElement())
    {
        if (const auto provided = winrt::AutomationProperties::GetPositionInSet(header); provided > 0)
        {
            return provided;
        }
    }

    // 1-based visible column position, so AT can announce "column i of n".
    const auto index = GetColumnIndex();

    // 0 is UIA's "not specified"; valid values are 1-based, so -1 reached the client as a nonsense
    // position.
    return index >= 0 ? index + 1 : 0;
}

int32_t TableViewColumnHeaderAutomationPeer::GetSizeOfSetCore()
{
    if (auto const header = GetHeaderElement())
    {
        if (const auto provided = winrt::AutomationProperties::GetSizeOfSet(header); provided > 0)
        {
            return provided;
        }
    }

    // Total visible column count, so PositionInSet reads as "i of n".
    if (auto const owner = m_table.get())
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

    return 0;
}

int32_t TableViewColumnHeaderAutomationPeer::GetColumnIndex()
{
    // Logical visible column index, matching GetSizeOfSetCore's visible basis
    // and the rendered header order, so PositionInSet ("i") and SizeOfSet ("n") stay
    // consistent even when Columns contains null holes or collapsed columns.
    // Columns are matched by identity; a column instance is a single logical position
    // (single-owner model), so the same instance appearing twice in Columns is unsupported.
    if (auto const owner = m_table.get())
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
    auto const owner = m_table.get();
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

    auto const header = TableViewCellsPanel::CellForColumn(host, col);
    return header == Owner() ? header : nullptr;
}
