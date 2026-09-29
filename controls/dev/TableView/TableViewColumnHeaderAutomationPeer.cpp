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
#include <algorithm>
#include <vector>

namespace
{
    struct ColumnAutomationIdEntry
    {
        winrt::weak_ref<winrt::TableViewColumn> column{ nullptr };
        int32_t id{ 0 };
    };

    struct TableAutomationIdScope
    {
        winrt::weak_ref<winrt::TableView> table{ nullptr };
        std::vector<ColumnAutomationIdEntry> columns;
        int32_t nextId{ 1 };
    };

    std::vector<TableAutomationIdScope>& TableAutomationIdScopes()
    {
        static std::vector<TableAutomationIdScope> scopes;
        return scopes;
    }

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

    void DropExpiredColumnEntries(TableAutomationIdScope& scope)
    {
        scope.columns.erase(
            std::remove_if(scope.columns.begin(), scope.columns.end(), [](auto const& entry)
            {
                return !entry.column.get();
            }),
            scope.columns.end());
    }

    TableAutomationIdScope& AutomationIdScopeForTable(winrt::TableView const& table)
    {
        auto& scopes = TableAutomationIdScopes();
        scopes.erase(
            std::remove_if(scopes.begin(), scopes.end(), [](auto const& scope)
            {
                return !scope.table.get();
            }),
            scopes.end());

        for (auto& scope : scopes)
        {
            if (scope.table.get() == table)
            {
                DropExpiredColumnEntries(scope);
                return scope;
            }
        }

        scopes.push_back({ winrt::make_weak(table), {}, 1 });
        return scopes.back();
    }

    int32_t RegisteredAutomationIdForColumn(TableAutomationIdScope& scope, winrt::TableViewColumn const& column)
    {
        for (auto const& entry : scope.columns)
        {
            if (entry.column.get() == column)
            {
                return entry.id;
            }
        }

        const auto id = scope.nextId++;
        scope.columns.push_back({ winrt::make_weak(column), id });
        return id;
    }

    int32_t StableAutomationIdPartForColumn(winrt::TableView const& table, winrt::TableViewColumn const& column)
    {
        if (!table || !column)
        {
            return 0;
        }

        auto& scope = AutomationIdScopeForTable(table);
        if (auto const columns = table.Columns())
        {
            for (auto const& currentColumn : columns)
            {
                if (currentColumn)
                {
                    RegisteredAutomationIdForColumn(scope, currentColumn);
                }
            }
        }

        return RegisteredAutomationIdForColumn(scope, column);
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
    , m_columnAutomationIdPart(StableAutomationIdPartForColumn(table, column))
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

    if (m_columnAutomationIdPart > 0)
    {
        std::wstring automationId{ L"TableViewColumnHeader_" };
        automationId.append(std::to_wstring(m_columnAutomationIdPart));
        return hstring{ automationId };
    }

    return {};
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
