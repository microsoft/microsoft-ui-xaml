// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewTestHooks.h"

namespace
{
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