// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewTestHooks.h"

#include <atomic>

namespace
{
    using RowCellRebuiltEvent = winrt::event<winrt::Windows::Foundation::EventHandler<winrt::IInspectable>>;

    // Leaked, as RepeaterTestHooks is: a handler left registered is never released during DLL unload.
    std::atomic<RowCellRebuiltEvent*> s_rowCellRebuilt{ nullptr };

    RowCellRebuiltEvent& EnsureRowCellRebuilt()
    {
        auto* existing = s_rowCellRebuilt.load(std::memory_order_acquire);
        if (!existing)
        {
            auto* const created = new RowCellRebuiltEvent();
            if (s_rowCellRebuilt.compare_exchange_strong(existing, created, std::memory_order_acq_rel))
            {
                existing = created;
            }
            else
            {
                delete created;
            }
        }
        return *existing;
    }

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

winrt::event_token TableViewTestHooks::RowCellRebuilt(winrt::Windows::Foundation::EventHandler<winrt::IInspectable> const& handler)
{
    return EnsureRowCellRebuilt().add(handler);
}

void TableViewTestHooks::RowCellRebuilt(winrt::event_token const& token)
{
    if (auto* const rowCellRebuilt = s_rowCellRebuilt.load(std::memory_order_acquire))
    {
        rowCellRebuilt->remove(token);
    }
}

void TableViewTestHooks::NoteRowCellRebuild(winrt::IInspectable const& row)
{
    auto* const rowCellRebuilt = s_rowCellRebuilt.load(std::memory_order_acquire);
    if (!rowCellRebuilt || !*rowCellRebuilt)
    {
        return;
    }
    // Runs inside layout: a failing test handler must not take the row rebuild down with it.
    try
    {
        (*rowCellRebuilt)(row, nullptr);
    }
    catch (...)
    {
    }
}
