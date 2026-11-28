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
    std::atomic<RowCellRebuiltEvent*> s_rowAutomationPropertyChanged{ nullptr };
    std::atomic<bool> s_isAutomationListenerForced{ false };

    RowCellRebuiltEvent& EnsureEvent(std::atomic<RowCellRebuiltEvent*>& slot)
    {
        auto* existing = slot.load(std::memory_order_acquire);
        if (!existing)
        {
            auto* const created = new RowCellRebuiltEvent();
            if (slot.compare_exchange_strong(existing, created, std::memory_order_acq_rel))
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

void TableViewTestHooks::SetAutomationListenerForced(bool isForced)
{
    s_isAutomationListenerForced.store(isForced, std::memory_order_release);
}

winrt::event_token TableViewTestHooks::RowAutomationPropertyChanged(winrt::Windows::Foundation::EventHandler<winrt::IInspectable> const& handler)
{
    return EnsureEvent(s_rowAutomationPropertyChanged).add(handler);
}

void TableViewTestHooks::RowAutomationPropertyChanged(winrt::event_token const& token)
{
    if (auto* const changed = s_rowAutomationPropertyChanged.load(std::memory_order_acquire))
    {
        changed->remove(token);
    }
}

bool TableViewTestHooks::AutomationListenerExists(winrt::AutomationEvents eventId)
{
    return s_isAutomationListenerForced.load(std::memory_order_acquire) || winrt::AutomationPeer::ListenerExists(eventId);
}

void TableViewTestHooks::NoteRowAutomationPropertyChanged(winrt::IInspectable const& row, wchar_t const* property)
{
    auto* const changed = s_rowAutomationPropertyChanged.load(std::memory_order_acquire);
    if (!changed || !*changed)
    {
        return;
    }
    try
    {
        (*changed)(row, winrt::box_value(winrt::hstring{ property }));
    }
    catch (...)
    {
    }
}
