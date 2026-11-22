// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewColumn.h"
#include "TableViewColumnReorderedEventArgs.h"
#include "TableViewAutomationHelpers.h"
#include "ResourceAccessor.h"
#include "Utils.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <vector>

static constexpr std::wstring_view s_ColumnReorderIndicatorPartName{ L"PART_ColumnReorderIndicator"sv };

namespace
{
    std::vector<winrt::TableViewColumn> FrozenPrefix(const std::vector<winrt::TableViewColumn>& order, const winrt::TableView& owner)
    {
        std::vector<winrt::TableViewColumn> prefix;
        TableViewDetails::ForEachLeadingFrozenColumn(owner, order, [&prefix](auto const& column) { prefix.push_back(column); });
        return prefix;
    }
}

void TableView::OnCanUserReorderColumnsPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& /*args*/)
{
    CancelColumnReorderDrag();
    m_suppressHeaderTap = false;
    HookColumnReorderInput();
    RefreshHeaderTabStopsInternal();
}

bool TableView::CanUserMoveColumnInternal(const winrt::TableViewColumn& column)
{
    auto const slots = GetColumnReorderSlots(column, false /* fullRange */);
    return slots.own >= 0 && slots.min < slots.max;
}

// A move may not change which columns are in the frozen prefix.
bool TableView::IsColumnMoveAllowed(uint32_t from, uint32_t to)
{
    auto const columns = Columns();
    if (!columns || from >= columns.Size() || to >= columns.Size())
    {
        return false;
    }

    std::vector<winrt::TableViewColumn> order(columns.Size(), nullptr);
    columns.GetMany(0, order);
    auto const before = FrozenPrefix(order, *this);
    auto const moved = order[from];
    order.erase(order.begin() + from);
    order.insert(order.begin() + to, moved);
    auto const after = FrozenPrefix(order, *this);
    return before.size() == after.size() &&
        std::all_of(before.begin(), before.end(), [&after](auto const& column) { return std::find(after.begin(), after.end(), column) != after.end(); });
}

// Slot k is directly before others[k], or after the last one; the allowed slots are contiguous.
TableView::ColumnReorderSlots TableView::GetColumnReorderSlots(const winrt::TableViewColumn& column, bool fullRange)
{
    ColumnReorderSlots slots;
    auto const columns = Columns();
    if (!CanUserReorderColumns() || !columns || !TableViewDetails::IsColumnOwnedBy(*this, column) || !IsVisibleColumn(column))
    {
        return slots;
    }
    for (auto const& other : columns)
    {
        if (other == column)
        {
            slots.own = static_cast<int32_t>(slots.others.size());
        }
        else if (TableViewDetails::IsColumnOwnedBy(*this, other) && IsVisibleColumn(other))
        {
            slots.others.push_back(other);
        }
    }

    slots.min = slots.max = slots.own;
    auto const allowed = [&](int32_t slot)
    {
        uint32_t from = 0;
        const int32_t to = ColumnReorderTargetIndex(column, slots, slot);
        return to >= 0 && columns.IndexOf(column, from) && IsColumnMoveAllowed(from, static_cast<uint32_t>(to));
    };
    while (slots.min > 0 && allowed(slots.min - 1))
    {
        --slots.min;
        if (!fullRange)
        {
            break;
        }
    }
    while (slots.max < static_cast<int32_t>(slots.others.size()) && allowed(slots.max + 1))
    {
        ++slots.max;
        if (!fullRange)
        {
            break;
        }
    }
    return slots;
}

// The column's index in Columns once it is moved to the slot.
int32_t TableView::ColumnReorderTargetIndex(const winrt::TableViewColumn& column, const ColumnReorderSlots& slots, int32_t slot)
{
    auto const columns = Columns();
    uint32_t from = 0;
    uint32_t index = 0;
    if (!columns || !columns.IndexOf(column, from) || slot < 0 || slot > static_cast<int32_t>(slots.others.size()) || slots.others.empty())
    {
        return -1;
    }
    const bool last = slot == static_cast<int32_t>(slots.others.size());
    if (!columns.IndexOf(slots.others[static_cast<size_t>(last ? slot - 1 : slot)], index))
    {
        return -1;
    }
    return static_cast<int32_t>(index > from ? index - 1 : index) + (last ? 1 : 0);
}

bool TableView::TryReorderColumn(const winrt::TableViewColumn& column, int32_t slot)
{
    // A move never commits an open edit implicitly.
    auto const slots = IsEditing() ? ColumnReorderSlots{} : GetColumnReorderSlots(column, true /* fullRange */);
    auto const columns = Columns();
    uint32_t from = 0;
    const int32_t to = slots.own < 0 || slot == slots.own || slot < slots.min || slot > slots.max ? -1 : ColumnReorderTargetIndex(column, slots, slot);
    if (to < 0 || !columns.IndexOf(column, from))
    {
        return false;
    }

    // The column cursor and body focus are visible indexes; carry them to their column's new index.
    auto const visibleAt = [&columns](int32_t index)
    {
        for (auto const& candidate : columns)
        {
            if (IsVisibleColumn(candidate) && index-- == 0)
            {
                return candidate;
            }
        }
        return winrt::TableViewColumn{ nullptr };
    };
    int32_t focusRow = -1;
    int32_t focusIndex = -1;
    bool focusCell = false;
    auto const focusColumn = TryResolveLiveBodyFocusPosition(focusRow, focusIndex, focusCell) && focusCell ? visibleAt(focusIndex) : nullptr;
    auto const root = focusColumn ? XamlRoot() : nullptr;
    // Cells are not Controls, so read UIElement.FocusState; pointer focus is replayed as Programmatic.
    auto const focusedElement = root ? winrt::FocusManager::GetFocusedElement(root).try_as<winrt::UIElement>() : nullptr;
    auto const focusState = focusedElement && focusedElement.FocusState() != winrt::FocusState::Keyboard ? winrt::FocusState::Programmatic : winrt::FocusState::Keyboard;
    auto const cursorColumn = m_columnCursorEstablished ? visibleAt(m_currentCellColumn) : nullptr;
    auto const lastFocusColumn = m_lastBodyFocusValid ? visibleAt(m_lastBodyFocusColumn) : nullptr;

    uint32_t newIndex = 0;
    {
        m_movingColumn = winrt::make_weak(column);
        auto const movingGuard = wil::scope_exit([this]() noexcept { m_movingColumn = nullptr; });
        uint32_t present = 0;
        // An app vector can throw after applying the change; put the column back only if it is missing.
        auto const bail = [&](bool restore)
        {
            TableViewTelemetry::ReportError(m_telemetrySession, 0, TableViewTip::Operation::ColumnReorder, TableViewTip::Stage::HeaderRefresh, true);
            if (restore && Columns() == columns && !columns.IndexOf(column, present))
            {
                try { columns.InsertAt((std::min)(from, columns.Size()), column); } catch (...) {}
            }
            return false;
        };
        try
        {
            columns.RemoveAt(from);
        }
        catch (...)
        {
            return bail(true);
        }
        // VectorChanged handlers run between the two calls and can change Columns.
        if (Columns() != columns || columns.IndexOf(column, present))
        {
            return bail(false);
        }
        try
        {
            columns.InsertAt((std::min)(static_cast<uint32_t>(to), columns.Size()), column);
        }
        catch (...)
        {
            return bail(true);
        }
        if (!columns.IndexOf(column, newIndex))
        {
            return bail(true);
        }
    }

    // Now, not on the queued tick: focus stays on the moved header, and a repeated key reads the new order.
    m_rebuildHeadersQueued = false;
    ++m_rebuildHeadersQueueGeneration;
    try
    {
        RebuildHeaders();
        InvalidateMeasure();
    }
    catch (...)
    {
        TableViewTelemetry::ReportError(m_telemetrySession, 0, TableViewTip::Operation::HeaderRefresh, TableViewTip::Stage::HeaderRefresh, true);
        QueueRebuildHeaders();
    }

    // A focused body cell is the current cell, so the cursor follows its column.
    if (focusColumn || cursorColumn)
    {
        SetColumnCursorInternal(VisibleColumnIndexOf(columns, focusColumn ? focusColumn : cursorColumn));
    }
    if (lastFocusColumn)
    {
        m_lastBodyFocusColumn = (std::max)(0, VisibleColumnIndexOf(columns, lastFocusColumn));
    }
    // Removing the focused cell's column drops its focus; restore it, in its original state, once the row has laid out the cell.
    if (focusColumn == column)
    {
        BeginPendingBodyFocus(focusRow, VisibleColumnIndexOf(columns, focusColumn), true /* cellLevel */, std::nullopt, nullptr);
        m_pendingReshapeFocusState = focusState;
    }
    else if (focusColumn)
    {
        // The row re-lays out its cells, which can drop another cell's focus; restore it only if focus was lost.
        auto token = std::make_shared<winrt::event_token>();
        *token = LayoutUpdated([weakThis = get_weak(), token, focusRow, focusColumn, focusState](auto const&, auto const&)
            {
                auto const strongThis = weakThis.get();
                if (!strongThis)
                {
                    return;
                }
                strongThis->LayoutUpdated(*token);
                auto const root = strongThis->XamlRoot();
                if (root && !winrt::FocusManager::GetFocusedElement(root))
                {
                    strongThis->BeginPendingBodyFocus(focusRow, VisibleColumnIndexOf(strongThis->Columns(), focusColumn), true /* cellLevel */, std::nullopt, nullptr);
                    strongThis->m_pendingReshapeFocusState = focusState;
                }
            });
    }

    RaiseTableViewNotification(*this, winrt::AutomationNotificationKind::ActionCompleted, [&]()
        {
            // The header item's name, so an icon-only header is announced by its AutomationProperties.Name.
            auto const headerPeer = GetRealizedColumnHeaderPeer(*this, column);
            winrt::hstring name = headerPeer ? headerPeer.GetName() : winrt::hstring{};
            if (name.empty())
            {
                name = TableViewDetails::GetColumnHeaderText(column);
            }
            auto const format = ResourceAccessor::GetLocalizedStringResource(SR_TableViewColumnMoved);
            auto const position = TableViewDetails::FormatIntegerForCurrentCulture(VisibleColumnIndexOf(columns, column) + 1);
            auto const count = TableViewDetails::FormatIntegerForCurrentCulture(CountVisibleColumns(columns));
            return format.empty() ? winrt::hstring{} : StringUtil::FormatString(format, name.c_str(), position.c_str(), count.c_str());
        },
        L"TableViewColumnMoved");

    if (m_columnReorderedEventSource)
    {
        auto const args = winrt::make_self<TableViewColumnReorderedEventArgs>(column, static_cast<int32_t>(from), static_cast<int32_t>(newIndex));
        try { m_columnReorderedEventSource(*this, *args); }
        catch (...) { TableViewTip::RethrowAppFault(TableViewTip::AppFaultKind::Callback); }
    }
    return true;
}

// Pointer: driven by the header host's manipulation; System keeps vertical pans.

void TableView::HookColumnReorderInput()
{
    auto const host = m_headerHost.get();
    const bool hook = host && CanUserReorderColumns();
    auto const hooked = m_columnReorderHookedHost.get();
    if (hook && hooked == host)
    {
        return;
    }

    m_suppressHeaderTap = false;
    if (hooked)
    {
        for (auto const& [routedEvent, handler] : m_columnReorderHandlers)
        {
            hooked.RemoveHandler(routedEvent, handler);
        }
        if (m_columnReorderSavedManipulationMode == winrt::DependencyProperty::UnsetValue())
        {
            hooked.ClearValue(winrt::UIElement::ManipulationModeProperty());
        }
        else
        {
            hooked.SetValue(winrt::UIElement::ManipulationModeProperty(), m_columnReorderSavedManipulationMode);
        }
    }
    m_columnReorderHandlers.clear();
    m_columnReorderHookedHost = nullptr;
    if (!hook)
    {
        return;
    }

    auto const weakThis = get_weak();
    auto const add = [this, &host](const winrt::RoutedEvent& routedEvent, const winrt::IInspectable& handler, bool handledEventsToo)
    {
        host.AddHandler(routedEvent, handler, handledEventsToo);
        m_columnReorderHandlers.emplace_back(routedEvent, handler);
    };
    auto const ended = [weakThis](bool captureLost)
    {
        return winrt::box_value(winrt::PointerEventHandler([weakThis, captureLost](auto const&, winrt::PointerRoutedEventArgs const& args)
            {
                // A header cell losing its own capture bubbles here too; only the host's ends the drag.
                auto const strongThis = weakThis.get();
                if (!strongThis || !strongThis->m_columnReorder)
                {
                    return;
                }
                auto const& pointer = strongThis->m_columnReorder->pointer;
                if (captureLost ? args.OriginalSource() == strongThis->m_columnReorderHookedHost.get() :
                    pointer && args.Pointer().PointerId() == pointer.PointerId())
                {
                    strongThis->CancelColumnReorderDrag();
                }
            }));
    };

    add(winrt::UIElement::PointerPressedEvent(), winrt::box_value(winrt::PointerEventHandler([weakThis](auto const&, auto const& args)
        {
            if (auto const strongThis = weakThis.get())
            {
                strongThis->OnHeaderHostPointerPressedForReorder(args);
            }
        })), true);
    add(winrt::UIElement::PointerCaptureLostEvent(), ended(true), true);
    add(winrt::UIElement::PointerCanceledEvent(), ended(false), true);
    add(winrt::UIElement::ManipulationStartedEvent(), winrt::box_value(winrt::ManipulationStartedEventHandler([weakThis](auto const&, auto const& args)
        {
            if (auto const strongThis = weakThis.get(); strongThis && strongThis->BeginColumnReorderDrag())
            {
                args.Handled(true);
            }
        })), false);
    add(winrt::UIElement::ManipulationDeltaEvent(), winrt::box_value(winrt::ManipulationDeltaEventHandler([weakThis](auto const&, auto const& args)
        {
            if (auto const strongThis = weakThis.get(); strongThis && strongThis->UpdateColumnReorderDrag(args.Cumulative().Translation.X))
            {
                args.Handled(true);
            }
        })), true);
    add(winrt::UIElement::ManipulationCompletedEvent(), winrt::box_value(winrt::ManipulationCompletedEventHandler([weakThis](auto const&, auto const& args)
        {
            if (auto const strongThis = weakThis.get(); strongThis && strongThis->UpdateColumnReorderDrag(args.Cumulative().Translation.X))
            {
                args.Handled(true);
                strongThis->EndColumnReorderDrag(true /* commit */);
            }
        })), true);

    m_columnReorderSavedManipulationMode = host.ReadLocalValue(winrt::UIElement::ManipulationModeProperty());
    host.ManipulationMode(winrt::ManipulationModes::System | winrt::ManipulationModes::TranslateX);
    m_columnReorderHookedHost = winrt::make_weak(host.as<winrt::UIElement>());
}

void TableView::OnHeaderHostPointerPressedForReorder(const winrt::PointerRoutedEventArgs& args)
{
    if (m_columnReorder && m_columnReorder->dragging)
    {
        return;
    }
    m_columnReorder.reset();
    m_suppressHeaderTap = false;

    // A press the gripper or interactive header content handled is theirs.
    auto const row = m_headerRow.get();
    winrt::FrameworkElement cell{ nullptr };
    auto const column = !args.Handled() && row ? HeaderColumnFromSource(args.OriginalSource(), cell) : nullptr;
    auto const point = row ? args.GetCurrentPoint(row) : nullptr;
    if (!column || !point || !CanUserMoveColumnInternal(column) ||
        (args.Pointer().PointerDeviceType() == winrt::Microsoft::UI::Input::PointerDeviceType::Mouse && !point.Properties().IsLeftButtonPressed()))
    {
        return;
    }

    ColumnReorderDragState state;
    state.column = winrt::make_weak(column);
    state.pointer = args.Pointer();
    state.pressX = point.Position().X;
    state.pointerX = state.pressX;
    m_columnReorder = std::move(state);
}

bool TableView::BeginColumnReorderDrag()
{
    if (!m_columnReorder || m_columnReorder->dragging)
    {
        return false;
    }
    auto& state = *m_columnReorder;
    auto const column = state.column.get();
    auto const row = m_headerRow.get();
    auto const host = m_headerHost.get();
    if (column && !IsEditing())
    {
        state.slots = GetColumnReorderSlots(column, true /* fullRange */);
    }
    // Until the header offset catches up with the body, the band is placed by compositor Translation that TransformToVisual does not see.
    auto const headerScroller = m_headerScroller.get();
    auto const bodyScroller = m_bodyScroller.get();
    const double bodyOffset = bodyScroller ? bodyScroller.HorizontalOffset() : 0.0;
    const bool bandMoving = headerScroller && bodyScroller && headerScroller != bodyScroller &&
        (std::abs(headerScroller.HorizontalOffset() - bodyOffset) >= 0.5 || std::abs(m_headerRequestedOffset - bodyOffset) >= 0.5);
    if (!row || !host || state.slots.min >= state.slots.max || m_bodyManipulating || bandMoving)
    {
        m_columnReorder.reset();
        return false;
    }

    // Header-row coordinates; any header scroll during the drag cancels it.
    for (auto const& child : host.Children())
    {
        auto const cell = child.try_as<winrt::FrameworkElement>();
        auto const cellColumn = cell ? cell.Tag().try_as<winrt::TableViewColumn>() : nullptr;
        if (cellColumn && state.edges.size() < state.slots.others.size() && state.slots.others[state.edges.size()] == cellColumn)
        {
            const double left = cell.TransformToVisual(row).TransformPoint({ 0.0f, 0.0f }).X;
            state.edges.emplace_back(left, left + cell.ActualWidth());
        }
    }
    if (state.edges.size() != state.slots.others.size())
    {
        m_columnReorder.reset();
        return false;
    }
    // A scrolling column's slots start at the frozen band's trailing edge.
    if (state.slots.min > 0)
    {
        state.bandEdge = state.edges[static_cast<size_t>(state.slots.min - 1)].second;
    }
    state.headerOffset = headerScroller ? headerScroller.HorizontalOffset() : 0.0;

    m_suppressHeaderTap = true;
    ++m_columnReorderGeneration;
    // Set before capturing: a header cell losing its capture re-enters the pointer handlers.
    state.dragging = true;
    if (state.pointer.PointerDeviceType() == winrt::Microsoft::UI::Input::PointerDeviceType::Mouse)
    {
        host.CapturePointer(state.pointer);
    }
    if (!m_columnReorder)
    {
        return false;
    }
    m_columnReorderEscape.Attach(XamlRoot(), [weakThis = get_weak()]()
        {
            auto const strongThis = weakThis.get();
            if (!strongThis || !strongThis->m_columnReorder)
            {
                return false;
            }
            strongThis->CancelColumnReorderDrag();
            return true;
        });
    if (!m_columnReorderIndicator.get())
    {
        m_columnReorderIndicator.set(GetTemplateChild(winrt::hstring{ s_ColumnReorderIndicatorPartName }).try_as<winrt::Rectangle>());
    }
    return true;
}

int32_t TableView::ColumnReorderSlotAt(const ColumnReorderDragState& state)
{
    // A column hidden under the frozen band is skipped; a partly hidden one splits at its visible middle.
    const double pointerX = (std::max)(state.pointerX, state.bandEdge);
    int32_t slot = state.slots.min;
    for (int32_t k = state.slots.min; k < state.slots.max; ++k)
    {
        auto const& [left, right] = state.edges[static_cast<size_t>(k)];
        slot += right <= state.bandEdge || pointerX > ((std::max)(left, state.bandEdge) + right) / 2.0 ? 1 : 0;
    }
    return slot;
}

bool TableView::UpdateColumnReorderDrag(double translationX)
{
    if (!m_columnReorder || !m_columnReorder->dragging)
    {
        return false;
    }
    auto& state = *m_columnReorder;
    // The manipulation reports physical translation.
    state.pointerX = state.pressX + (FlowDirection() == winrt::FlowDirection::RightToLeft ? -translationX : translationX);

    auto const indicator = m_columnReorderIndicator.get();
    auto const row = m_headerRow.get();
    if (!indicator || !row)
    {
        return true;
    }
    const int32_t slot = ColumnReorderSlotAt(state);
    if (slot == state.slots.own || state.stale)
    {
        indicator.Visibility(winrt::Visibility::Collapsed);
        return true;
    }

    // The edge between the neighbours; a scrolling header under the frozen band yields to the band's edge.
    auto const& edges = state.edges;
    const size_t index = static_cast<size_t>(slot);
    double x = index < edges.size() ? edges[index].first : edges.back().second;
    if (index > 0 && index < edges.size())
    {
        x = (std::max)(x, edges[index - 1].second);
    }
    x = (std::max)(x, state.bandEdge);
    const double width = std::isnan(indicator.Width()) ? 0.0 : indicator.Width();
    indicator.Margin(winrt::Thickness{ std::clamp(x - width / 2.0, 0.0, (std::max)(0.0, row.ActualWidth() - width)), 0.0, 0.0, 0.0 });
    indicator.Visibility(winrt::Visibility::Visible);
    return true;
}

bool TableView::ConsumeColumnReorderTap()
{
    return std::exchange(m_suppressHeaderTap, false);
}

void TableView::CancelColumnReorderDrag()
{
    EndColumnReorderDrag(false /* commit */);
}

// Scroll and manipulation events can arrive inside layout, where releasing capture fail-fasts, so a drag ends on the next tick.
void TableView::QueueCancelColumnReorderDrag()
{
    if (!m_columnReorder || !m_columnReorder->dragging)
    {
        CancelColumnReorderDrag();
        return;
    }
    if (std::exchange(m_columnReorder->stale, true))
    {
        return;
    }
    if (auto const dispatcher = DispatcherQueue())
    {
        dispatcher.TryEnqueue([weakThis = get_weak(), generation = m_columnReorderGeneration]()
            {
                if (auto const strongThis = weakThis.get(); strongThis && strongThis->m_columnReorderGeneration == generation)
                {
                    strongThis->CancelColumnReorderDrag();
                }
            });
    }
}

void TableView::EndColumnReorderDrag(bool commit)
{
    if (!m_columnReorder)
    {
        return;
    }

    // Cleared first: releasing the capture and applying the move both re-enter the handlers.
    auto const state = std::move(*m_columnReorder);
    m_columnReorder.reset();
    m_columnReorderEscape.Detach();
    if (auto const indicator = m_columnReorderIndicator.get())
    {
        indicator.Visibility(winrt::Visibility::Collapsed);
    }
    if (!state.dragging)
    {
        return;
    }
    if (auto const host = m_columnReorderHookedHost.get(); host && host.XamlRoot())
    {
        host.ReleasePointerCapture(state.pointer);
    }
    if (auto const column = state.column.get(); commit && column && !state.stale)
    {
        TryReorderColumn(column, ColumnReorderSlotAt(state));
    }
}
