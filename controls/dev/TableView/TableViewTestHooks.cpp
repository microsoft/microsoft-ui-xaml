// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewTestHooks.h"

namespace
{
    winrt::Windows::Foundation::EventHandler<winrt::IInspectable> s_rowCellRebuildHandler{ nullptr };

    TableView* TableViewFrom(winrt::IInspectable const& tableView)
    {
        auto const table = tableView.try_as<winrt::TableView>();
        return table ? winrt::get_self<TableView>(table) : nullptr;
    }
}

bool TableViewTestHooks::BeginColumnReorderDrag(winrt::IInspectable const& tableView, winrt::IInspectable const& column)
{
    auto const table = TableViewFrom(tableView);
    return table && table->BeginColumnReorderDragForTesting(column.try_as<winrt::TableViewColumn>());
}

bool TableViewTestHooks::UpdateColumnReorderDrag(winrt::IInspectable const& tableView, double translationX)
{
    auto const table = TableViewFrom(tableView);
    return table && table->UpdateColumnReorderDragForTesting(translationX);
}

bool TableViewTestHooks::CompleteColumnReorderDrag(winrt::IInspectable const& tableView, double translationX)
{
    auto const table = TableViewFrom(tableView);
    return table && table->CompleteColumnReorderDragForTesting(translationX);
}

bool TableViewTestHooks::IsColumnReorderDragging(winrt::IInspectable const& tableView)
{
    auto const table = TableViewFrom(tableView);
    return table && table->IsColumnReorderDraggingForTesting();
}

void TableViewTestHooks::SetRowCellRebuildHandler(winrt::Windows::Foundation::EventHandler<winrt::IInspectable> const& handler)
{
    s_rowCellRebuildHandler = handler;
}

void TableViewTestHooks::NoteRowCellRebuild(winrt::IInspectable const& row)
{
    if (auto const handler = s_rowCellRebuildHandler)
    {
        handler(row, nullptr);
    }
}
