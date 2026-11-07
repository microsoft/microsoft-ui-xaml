// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewColumn.h"
#include "TableViewColumnReorderingEventArgs.h"
#include "TableViewColumnReorderedEventArgs.h"
#include "TableViewAutomationHelpers.h"
#include "ResourceAccessor.h"
#include "Utils.h"

#include <algorithm>
#include <cmath>
#include <vector>

// User column reorder: header drag and Ctrl+Shift+Left/Right (TableView_Keyboard.cpp) both apply through TryReorderColumn.

static constexpr std::wstring_view s_ColumnReorderOverlayPartName{ L"PART_ColumnReorderOverlay"sv };
static constexpr std::wstring_view s_ColumnReorderIndicatorPartName{ L"PART_ColumnReorderIndicator"sv };

namespace
{
    constexpr double c_autoScrollZone{ 24.0 };
    constexpr double c_autoScrollStep{ 16.0 };
    constexpr double c_touchDragThreshold{ 16.0 };
    constexpr double c_ghostOpacity{ 0.7 };
    constexpr double c_sourceHeaderOpacity{ 0.4 };

    double DragThreshold(bool vertical, const winrt::UIElement& element, bool touch)
    {
        double scale = 1.0;
        if (auto const root = element.XamlRoot(); root && root.RasterizationScale() > 0.0)
        {
            scale = root.RasterizationScale();
        }
        const auto dpi = static_cast<UINT>(std::lround(USER_DEFAULT_SCREEN_DPI * scale));
        const double logical = (std::max)(::GetSystemMetricsForDpi(vertical ? SM_CYDRAG : SM_CXDRAG, dpi), 1) / scale;
        return touch ? (std::max)(logical, c_touchDragThreshold) : logical;
    }

    bool IsOwnedBy(const winrt::TableViewColumn& column, const winrt::TableView& owner)
    {
        return column && winrt::get_self<TableViewColumn>(column)->GetOwningTableView() == owner;
    }

    // Same rule as ComputeLeadingFrozenWidth: unowned entries are skipped, the first owned
    // non-Leading column ends the prefix.
    std::vector<winrt::TableViewColumn> FrozenPrefix(const std::vector<winrt::TableViewColumn>& order, const winrt::TableView& owner)
    {
        std::vector<winrt::TableViewColumn> prefix;
        for (auto const& column : order)
        {
            if (!IsOwnedBy(column, owner))
            {
                continue;
            }
            if (column.FrozenEdge() != winrt::TableViewFrozenEdge::Leading)
            {
                break;
            }
            prefix.push_back(column);
        }
        return prefix;
    }
}

bool TableView::IsColumnUserReorderableInternal(const winrt::TableViewColumn& column)
{
    return CanUserReorderColumns() && column && column.CanReorder() && IsOwnedBy(column, *this) && IsVisibleColumn(column);
}

bool TableView::CanUserMoveColumnInternal(const winrt::TableViewColumn& column)
{
    if (!IsColumnUserReorderableInternal(column))
    {
        return false;
    }
    std::vector<winrt::TableViewColumn> others;
    const int32_t own = ColumnReorderSlots(column, others);
    return own >= 0 &&
        ((own > 0 && IsColumnReorderSlotAllowed(column, others, own - 1)) ||
         (own < static_cast<int32_t>(others.size()) && IsColumnReorderSlotAllowed(column, others, own + 1)));
}

bool TableView::IsColumnMovingInternal(const winrt::TableViewColumn& column) const
{
    return column && m_movingColumn.get() == column;
}

void TableView::OnCanUserReorderColumnsPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& /*args*/)
{
    CancelColumnReorderDrag();
    HookColumnReorderInput();
    RefreshHeaderTabStopsInternal();
}

void TableView::OnColumnCanReorderChanged()
{
    CancelColumnReorderDrag();
    RefreshHeaderTabStopsInternal();
}

// A move may not change which columns are in the frozen prefix, so a column stays on its side of
// the boundary and a non-prefix Leading column cannot be pulled into the prefix. Trailing columns
// are not frozen by layout, so reorder treats them as scrolling columns.
bool TableView::IsColumnMoveAllowed(uint32_t from, uint32_t to)
{
    auto const columns = Columns();
    if (!columns || from >= columns.Size() || to >= columns.Size())
    {
        return false;
    }
    if (from == to)
    {
        return true;
    }

    std::vector<winrt::TableViewColumn> order(columns.Size(), nullptr);
    columns.GetMany(0, order);
    auto const before = FrozenPrefix(order, *this);
    auto const moved = order[from];
    order.erase(order.begin() + from);
    order.insert(order.begin() + to, moved);
    auto const after = FrozenPrefix(order, *this);

    return before.size() == after.size() &&
        std::all_of(before.begin(), before.end(), [&after](auto const& column)
            { return std::find(after.begin(), after.end(), column) != after.end(); });
}

// The visible owned columns other than column, and the slot column holds among them.
int32_t TableView::ColumnReorderSlots(const winrt::TableViewColumn& column, std::vector<winrt::TableViewColumn>& others)
{
    others.clear();
    int32_t own = -1;
    if (auto const columns = Columns())
    {
        for (auto const& other : columns)
        {
            if (other == column)
            {
                own = static_cast<int32_t>(others.size());
            }
            else if (IsOwnedBy(other, *this) && IsVisibleColumn(other))
            {
                others.push_back(other);
            }
        }
    }
    return own;
}

// Slot k: directly before others[k], or after the last one. The result is the column's index in
// Columns after the move.
int32_t TableView::ColumnReorderTargetForSlot(
    const winrt::TableViewColumn& column, const std::vector<winrt::TableViewColumn>& others, int32_t slot)
{
    auto const columns = Columns();
    uint32_t from = 0;
    if (!columns || !columns.IndexOf(column, from) || slot < 0 || slot > static_cast<int32_t>(others.size()))
    {
        return -1;
    }
    if (others.empty())
    {
        return static_cast<int32_t>(from);
    }

    auto const indexAfterRemoval = [&columns, from](const winrt::TableViewColumn& other) -> int32_t
    {
        uint32_t index = 0;
        if (!columns.IndexOf(other, index))
        {
            return -1;
        }
        return static_cast<int32_t>(index > from ? index - 1 : index);
    };

    if (slot < static_cast<int32_t>(others.size()))
    {
        return indexAfterRemoval(others[static_cast<size_t>(slot)]);
    }
    const int32_t last = indexAfterRemoval(others.back());
    return last < 0 ? -1 : last + 1;
}

bool TableView::IsColumnReorderSlotAllowed(
    const winrt::TableViewColumn& column, const std::vector<winrt::TableViewColumn>& others, int32_t slot)
{
    uint32_t from = 0;
    const int32_t to = ColumnReorderTargetForSlot(column, others, slot);
    return to >= 0 && Columns().IndexOf(column, from) && IsColumnMoveAllowed(from, static_cast<uint32_t>(to));
}

static winrt::TableViewColumn VisibleColumnAt(const winrt::IVector<winrt::TableViewColumn>& columns, int32_t visibleIndex)
{
    for (auto const& column : columns)
    {
        if (IsVisibleColumn(column) && visibleIndex-- == 0)
        {
            return column;
        }
    }
    return nullptr;
}

static int32_t VisibleColumnIndex(const winrt::IVector<winrt::TableViewColumn>& columns, const winrt::TableViewColumn& target)
{
    int32_t visibleIndex = 0;
    for (auto const& column : columns)
    {
        if (column == target)
        {
            return visibleIndex;
        }
        visibleIndex += IsVisibleColumn(column) ? 1 : 0;
    }
    return -1;
}

bool TableView::TryReorderColumn(const winrt::TableViewColumn& column, const std::function<int32_t()>& targetIndex)
{
    // Committing runs app code, so the target is computed only once the edit has closed.
    if (!IsColumnUserReorderableInternal(column) || (IsEditing() && !CommitEdit()) || !IsColumnUserReorderableInternal(column))
    {
        return false;
    }

    auto const columns = Columns();
    uint32_t from = 0;
    const int32_t newIndex = columns.IndexOf(column, from) ? targetIndex() : -1;
    if (newIndex < 0 || static_cast<uint32_t>(newIndex) >= columns.Size() || static_cast<uint32_t>(newIndex) == from ||
        !IsColumnMoveAllowed(from, static_cast<uint32_t>(newIndex)))
    {
        return false;
    }

    const uint32_t stamp = m_columnsChangeStamp;
    if (m_columnReorderingEventSource)
    {
        auto const args = winrt::make_self<TableViewColumnReorderingEventArgs>(column, static_cast<int32_t>(from), newIndex);
        try { m_columnReorderingEventSource(*this, *args); }
        catch (...) { TableViewTip::RethrowAppFault(TableViewTip::AppFaultKind::Callback); }
        if (args->Cancel() || stamp != m_columnsChangeStamp || Columns() != columns || IsEditing() ||
            !IsColumnUserReorderableInternal(column) || !IsColumnMoveAllowed(from, static_cast<uint32_t>(newIndex)))
        {
            return false;
        }
    }

    // The body's column cursor and a focused body cell follow their column, not its visible index.
    auto const cursorColumn = m_columnCursorEstablished ? VisibleColumnAt(columns, m_currentCellColumn) : nullptr;
    int32_t focusedRow = -1;
    int32_t focusedCell = -1;
    auto const focusedCellColumn = TryGetFocusedCell(focusedRow, focusedCell, false /* requireExactCell */) ? VisibleColumnAt(columns, focusedCell) : nullptr;

    {
        m_movingColumn = winrt::make_weak(column);
        auto const movingGuard = wil::scope_exit([this]() noexcept { m_movingColumn = nullptr; });
        // Cannot fail between the two: both indexes are checked above, and the vector's event does not
        // propagate a throwing VectorChanged handler.
        columns.RemoveAt(from);
        columns.InsertAt(static_cast<uint32_t>(newIndex), column);
    }

    // Now, not on the queued tick: focus stays on the moved header, and a repeated key reads the new order.
    m_rebuildHeadersQueued = false;
    ++m_rebuildHeadersQueueGeneration;
    RebuildHeaders();
    InvalidateMeasure();
    if (cursorColumn)
    {
        SetColumnCursorInternal(VisibleColumnIndex(columns, cursorColumn));
    }
    // Rows rebuild their cells on the next tick, which drops the focused cell; this runs after them.
    if (auto const queue = focusedCellColumn ? DispatcherQueue() : nullptr)
    {
        queue.TryEnqueue([weakThis = get_weak(), weakColumn = winrt::make_weak(focusedCellColumn), focusedRow]()
            {
                auto const strongThis = weakThis.get();
                auto const cellColumn = weakColumn.get();
                const int32_t index = strongThis && cellColumn ? VisibleColumnIndex(strongThis->Columns(), cellColumn) : -1;
                if (index >= 0)
                {
                    strongThis->FocusCell(focusedRow, index);
                }
            });
    }

    AnnounceColumnMove(column);

    if (m_columnReorderedEventSource)
    {
        auto const args = winrt::make_self<TableViewColumnReorderedEventArgs>(column, static_cast<int32_t>(from), newIndex);
        try { m_columnReorderedEventSource(*this, *args); }
        catch (...) { TableViewTip::RethrowAppFault(TableViewTip::AppFaultKind::Callback); }
    }
    return true;
}

void TableView::AnnounceColumnMove(const winrt::TableViewColumn& column)
{
    if (!winrt::AutomationPeer::ListenerExists(winrt::AutomationEvents::Notification))
    {
        return;
    }

    try
    {
        auto const peer = winrt::FrameworkElementAutomationPeer::FromElement(*this);
        auto const format = ResourceAccessor::GetLocalizedStringResource(SR_TableViewColumnMoved);
        std::vector<winrt::TableViewColumn> others;
        const int32_t own = ColumnReorderSlots(column, others);
        if (!peer || format.empty() || own < 0)
        {
            return;
        }

        // The header item's name, so an icon-only header is announced by its AutomationProperties.Name.
        auto const headerPeer = GetRealizedColumnHeaderPeer(*this, column);
        winrt::hstring name = headerPeer ? headerPeer.GetName() : winrt::hstring{};
        if (name.empty())
        {
            name = TableViewDetails::GetColumnHeaderText(column);
        }

        auto const index = TableViewDetails::FormatIntegerForCurrentCulture(own + 1);
        auto const count = TableViewDetails::FormatIntegerForCurrentCulture(static_cast<int32_t>(others.size()) + 1);
        peer.RaiseNotificationEvent(
            winrt::AutomationNotificationKind::ActionCompleted,
            winrt::AutomationNotificationProcessing::MostRecent,
            StringUtil::FormatString(format, name.c_str(), index.c_str(), count.c_str()),
            L"TableViewColumnMoved");
    }
    catch (...)
    {
        // Best-effort announcement; the move itself already applied.
    }
}

// ----- Pointer -----

static void RestoreLocalValue(const winrt::DependencyObject& target, const winrt::DependencyProperty& property, const winrt::IInspectable& saved)
{
    if (saved == winrt::DependencyProperty::UnsetValue())
    {
        target.ClearValue(property);
    }
    else
    {
        target.SetValue(property, saved);
    }
}

void TableView::HookColumnReorderInput()
{
    auto const host = m_headerHost.get();
    const bool hook = host && CanUserReorderColumns();
    auto const hooked = m_columnReorderHookedHost.get();
    if (hook && hooked == host)
    {
        return;
    }

    if (hooked)
    {
        for (auto const& [routedEvent, handler] : m_columnReorderHandlers)
        {
            hooked.RemoveHandler(routedEvent, handler);
        }
        if (auto const saved = std::exchange(m_columnReorderSavedManipulationMode, std::nullopt))
        {
            RestoreLocalValue(hooked, winrt::UIElement::ManipulationModeProperty(), *saved);
        }
    }
    m_columnReorderHandlers.clear();
    m_columnReorderHookedHost = nullptr;
    if (!hook)
    {
        return;
    }

    auto const weakThis = get_weak();
    auto const pointerHandler = [weakThis](void (TableView::*method)(const winrt::PointerRoutedEventArgs&))
    {
        return winrt::box_value(winrt::PointerEventHandler([weakThis, method](auto const&, winrt::PointerRoutedEventArgs const& args)
            {
                if (auto const strongThis = weakThis.get())
                {
                    (strongThis.get()->*method)(args);
                }
            }));
    };
    auto const add = [this, &host](const winrt::RoutedEvent& routedEvent, const winrt::IInspectable& handler, bool handledEventsToo)
    {
        host.AddHandler(routedEvent, handler, handledEventsToo);
        m_columnReorderHandlers.emplace_back(routedEvent, handler);
    };

    // Press without handled events: a press the resize gripper or interactive header content took is theirs.
    add(winrt::UIElement::PointerPressedEvent(), pointerHandler(&TableView::OnHeaderHostPointerPressedForReorder), false);
    add(winrt::UIElement::PointerMovedEvent(), pointerHandler(&TableView::OnHeaderHostPointerMovedForReorder), true);
    add(winrt::UIElement::PointerReleasedEvent(), pointerHandler(&TableView::OnHeaderHostPointerReleasedForReorder), true);
    add(winrt::UIElement::PointerCaptureLostEvent(), pointerHandler(&TableView::OnHeaderHostPointerEndedForReorder), true);
    add(winrt::UIElement::PointerCanceledEvent(), pointerHandler(&TableView::OnHeaderHostPointerEndedForReorder), true);
    // A press-and-hold opens the context menu instead.
    add(winrt::UIElement::ContextRequestedEvent(),
        winrt::box_value(winrt::TypedEventHandler<winrt::UIElement, winrt::ContextRequestedEventArgs>([weakThis](auto const&, auto const&)
            {
                if (auto const strongThis = weakThis.get(); strongThis && strongThis->m_columnReorder && !strongThis->m_columnReorder->dragging)
                {
                    strongThis->m_columnReorder.reset();
                }
            })),
        true);
    m_columnReorderHookedHost = winrt::make_weak(host.as<winrt::UIElement>());
    RefreshColumnReorderManipulationMode();
}

// While a header can move, a horizontal touch drag on the row is not taken as a pan by an ancestor
// ScrollViewer; vertical pans still scroll it.
void TableView::RefreshColumnReorderManipulationMode()
{
    auto const hooked = m_columnReorderHookedHost.get();
    if (!hooked)
    {
        return;
    }
    bool movable = false;
    for (auto const& column : Columns())
    {
        if (CanUserMoveColumnInternal(column))
        {
            movable = true;
            break;
        }
    }
    if (movable == m_columnReorderSavedManipulationMode.has_value())
    {
        return;
    }
    if (movable)
    {
        m_columnReorderSavedManipulationMode = hooked.ReadLocalValue(winrt::UIElement::ManipulationModeProperty());
        hooked.ManipulationMode(winrt::ManipulationModes::System | winrt::ManipulationModes::TranslateX);
        return;
    }
    RestoreLocalValue(hooked, winrt::UIElement::ManipulationModeProperty(), *std::exchange(m_columnReorderSavedManipulationMode, std::nullopt));
}

void TableView::OnHeaderHostPointerPressedForReorder(const winrt::PointerRoutedEventArgs& args)
{
    m_suppressHeaderTap = false;
    auto const row = m_headerRow.get();
    winrt::FrameworkElement cell{ nullptr };
    auto const column = HeaderColumnFromSource(args.OriginalSource(), cell);
    if ((m_columnReorder && m_columnReorder->dragging) || !row || !cell || !CanUserMoveColumnInternal(column))
    {
        return;
    }

    auto const point = args.GetCurrentPoint(row);
    auto const device = args.Pointer().PointerDeviceType();
    if ((device == winrt::Microsoft::UI::Input::PointerDeviceType::Mouse && !point.Properties().IsLeftButtonPressed()) ||
        (device == winrt::Microsoft::UI::Input::PointerDeviceType::Pen && point.Properties().IsBarrelButtonPressed()))
    {
        return;
    }

    ColumnReorderDragState state;
    state.column = winrt::make_weak(column);
    state.header = winrt::make_weak(cell.as<winrt::UIElement>());
    state.pointerId = args.Pointer().PointerId();
    state.touch = device == winrt::Microsoft::UI::Input::PointerDeviceType::Touch;
    state.pressPoint = point.Position();
    state.lastPoint = state.pressPoint;
    state.grabOffset = args.GetCurrentPoint(cell).Position().X;
    m_columnReorder = std::move(state);
}

void TableView::OnHeaderHostPointerMovedForReorder(const winrt::PointerRoutedEventArgs& args)
{
    if (!m_columnReorder || args.Pointer().PointerId() != m_columnReorder->pointerId)
    {
        return;
    }

    auto const row = m_headerRow.get();
    auto const point = row ? args.GetCurrentPoint(row) : nullptr;
    if (!point || !point.IsInContact())
    {
        CancelColumnReorderDrag();
        return;
    }
    m_columnReorder->lastPoint = point.Position();

    if (!m_columnReorder->dragging)
    {
        auto const header = m_columnReorder->header.get();
        const bool touch = m_columnReorder->touch;
        if (header &&
            std::abs(point.Position().X - m_columnReorder->pressPoint.X) <= DragThreshold(false, header, touch) &&
            std::abs(point.Position().Y - m_columnReorder->pressPoint.Y) <= DragThreshold(true, header, touch))
        {
            return;
        }
        if (!header)
        {
            m_columnReorder.reset();
            return;
        }
        // Set first: capturing raises CaptureLost on a previous holder, which must see a drag in progress.
        m_columnReorder->dragging = true;
        if (!header.CapturePointer(args.Pointer()) || !m_columnReorder)
        {
            CancelColumnReorderDrag();
            return;
        }
        if (!BeginColumnReorderDrag())
        {
            CancelColumnReorderDrag();
            return;
        }
    }

    UpdateColumnReorderDrag();
    args.Handled(true);
}

void TableView::OnHeaderHostPointerReleasedForReorder(const winrt::PointerRoutedEventArgs& args)
{
    if (!m_columnReorder || args.Pointer().PointerId() != m_columnReorder->pointerId)
    {
        return;
    }
    if (m_columnReorder->dragging)
    {
        args.Handled(true);
    }
    EndColumnReorderDrag(true /* commit */);
}

void TableView::OnHeaderHostPointerEndedForReorder(const winrt::PointerRoutedEventArgs& args)
{
    if (!m_columnReorder || args.Pointer().PointerId() != m_columnReorder->pointerId)
    {
        return;
    }

    // Only our own capture ending cancels; another element's capture lost bubbles here too.
    auto const header = m_columnReorder->header.get();
    if (m_columnReorder->dragging && header && args.OriginalSource() != header)
    {
        return;
    }
    CancelColumnReorderDrag();
}

void TableView::PrepareColumnReorderSlots(ColumnReorderDragState& state)
{
    auto const column = state.column.get();
    state.ownSlot = ColumnReorderSlots(column, state.others);
    state.minSlot = state.ownSlot;
    state.maxSlot = state.ownSlot;
    if (state.ownSlot < 0)
    {
        return;
    }
    while (state.minSlot > 0 && IsColumnReorderSlotAllowed(column, state.others, state.minSlot - 1))
    {
        --state.minSlot;
    }
    while (state.maxSlot < static_cast<int32_t>(state.others.size()) && IsColumnReorderSlotAllowed(column, state.others, state.maxSlot + 1))
    {
        ++state.maxSlot;
    }
    RefreshColumnReorderBounds(state);
}

// Content coordinates; a frozen header's bounds add the body offset when read.
void TableView::RefreshColumnReorderBounds(ColumnReorderDragState& state)
{
    state.bounds.clear();
    auto const host = m_headerHost.get();
    if (!host)
    {
        return;
    }

    auto const column = state.column.get();
    double x = 0.0;
    bool inPrefix = true;
    for (auto const& child : host.Children())
    {
        auto const cell = child.try_as<winrt::FrameworkElement>();
        auto const cellColumn = cell ? cell.Tag().try_as<winrt::TableViewColumn>() : nullptr;
        if (!cellColumn)
        {
            continue;
        }
        const bool visible = IsVisibleColumn(cellColumn);
        const double width = visible ? (std::max)(0.0, cellColumn.ActualWidth()) : 0.0;
        inPrefix = inPrefix && cellColumn.FrozenEdge() == winrt::TableViewFrozenEdge::Leading;
        if (cellColumn == column)
        {
            state.frozen = inPrefix;
        }
        else if (state.bounds.size() < state.others.size() && state.others[state.bounds.size()] == cellColumn)
        {
            state.bounds.push_back({ x, x + width, inPrefix });
        }
        x += width;
    }
}

bool TableView::BeginColumnReorderDrag()
{
    auto& state = *m_columnReorder;
    auto const column = state.column.get();
    auto const header = state.header.get().try_as<winrt::FrameworkElement>();
    if (!header || !CanUserMoveColumnInternal(column))
    {
        return false;
    }

    PrepareColumnReorderSlots(state);
    if (state.bounds.size() != state.others.size())
    {
        return false;
    }
    m_suppressHeaderTap = true;

    auto const weakThis = get_weak();
    if (auto const host = m_headerHost.get())
    {
        m_columnReorderHostSizeChangedRevoker = host.SizeChanged(winrt::auto_revoke, [weakThis](auto const&, auto const&)
            {
                if (auto const strongThis = weakThis.get(); strongThis && strongThis->m_columnReorder)
                {
                    strongThis->RefreshColumnReorderBounds(*strongThis->m_columnReorder);
                }
            });
    }

    // Esc while the pointer is down: the header need not have focus.
    if (auto const content = XamlRoot() ? XamlRoot().Content() : nullptr)
    {
        m_columnReorderKeyHandler = winrt::box_value(winrt::KeyEventHandler([weakThis](auto const&, winrt::KeyRoutedEventArgs const& args)
            {
                if (auto const strongThis = weakThis.get(); strongThis && args.Key() == winrt::VirtualKey::Escape && strongThis->m_columnReorder)
                {
                    strongThis->CancelColumnReorderDrag();
                    args.Handled(true);
                }
            }));
        content.AddHandler(winrt::UIElement::PreviewKeyDownEvent(), m_columnReorderKeyHandler, true);
        m_columnReorderKeyTarget = winrt::make_weak(content);
    }

    if (!m_columnReorderOverlay.get())
    {
        m_columnReorderOverlay.set(GetTemplateChild(winrt::hstring{ s_ColumnReorderOverlayPartName }).try_as<winrt::Canvas>());
        m_columnReorderIndicator.set(GetTemplateChild(winrt::hstring{ s_ColumnReorderIndicatorPartName }).try_as<winrt::Rectangle>());
    }
    auto const overlay = m_columnReorderOverlay.get();
    if (!overlay)
    {
        return true;
    }

    // High Contrast shows only the line and the dimmed header: a pressed header is highlight-filled, and a snapshot of it would hide the line.
    if (IsHighContrast())
    {
        state.headerOpacity = header.ReadLocalValue(winrt::UIElement::OpacityProperty());
        header.Opacity(c_sourceHeaderOpacity);
        return true;
    }

    winrt::Image image;
    image.Stretch(winrt::Stretch::Fill);
    winrt::Border ghost;
    ghost.Width(header.ActualWidth());
    ghost.Height(header.ActualHeight());
    ghost.Child(image);
    ghost.IsHitTestVisible(false);
    // The snapshot is already laid out in the header's flow direction.
    ghost.FlowDirection(winrt::FlowDirection::LeftToRight);
    ghost.Opacity(c_ghostOpacity);
    winrt::AutomationProperties::SetAccessibilityView(ghost, winrt::AccessibilityView::Raw);
    winrt::RenderTargetBitmap bitmap;
    image.Source(bitmap);
    // Below the indicator.
    overlay.Children().InsertAt(0, ghost);
    m_columnReorderGhost.set(ghost);

    // Dimmed only once captured, so the snapshot shows the header at full strength.
    auto const weakHeader = winrt::make_weak(header);
    bitmap.RenderAsync(header).Completed(
        [weakThis, weakHeader](auto const&, auto const&)
        {
            auto const strongThis = weakThis.get();
            auto const liveHeader = weakHeader.get();
            if (strongThis && liveHeader && strongThis->m_columnReorder && strongThis->m_columnReorder->dragging &&
                strongThis->m_columnReorder->header.get() == liveHeader.as<winrt::UIElement>())
            {
                strongThis->m_columnReorder->headerOpacity = liveHeader.ReadLocalValue(winrt::UIElement::OpacityProperty());
                liveHeader.Opacity(c_sourceHeaderOpacity);
            }
        });
    return true;
}

int32_t TableView::ColumnReorderSlotAt(const ColumnReorderDragState& state, double contentX, double offset)
{
    int32_t nearest = 0;
    for (size_t i = 0; i < state.bounds.size(); ++i)
    {
        auto const& bounds = state.bounds[i];
        const double shift = bounds.frozen ? offset : 0.0;
        if (contentX > (bounds.left + bounds.right) / 2.0 + shift)
        {
            nearest = static_cast<int32_t>(i) + 1;
        }
    }
    return std::clamp(nearest, state.minSlot, state.maxSlot);
}

// Logical coordinates: XAML mirrors RTL above the header host, so no flow-direction case. The
// overlay shares the header viewport's origin.
bool TableView::ColumnReorderViewport(const ColumnReorderDragState& state, double& pointerX, double& width, double& offset)
{
    auto const row = m_headerRow.get();
    auto const scroller = m_headerScroller.get();
    if (!row || !scroller)
    {
        return false;
    }
    auto const bodyScroller = m_bodyScroller.get();
    offset = bodyScroller ? bodyScroller.HorizontalOffset() : 0.0;
    pointerX = state.lastPoint.X - scroller.TransformToVisual(row).TransformPoint({ 0.0f, 0.0f }).X;
    width = scroller.ActualWidth();
    return true;
}

void TableView::UpdateColumnReorderDrag()
{
    if (!m_columnReorder || !m_columnReorder->dragging)
    {
        return;
    }
    auto& state = *m_columnReorder;
    auto const row = m_headerRow.get();
    double pointerX = 0.0, viewportWidth = 0.0, offset = 0.0;
    if (!row || !ColumnReorderViewport(state, pointerX, viewportWidth, offset))
    {
        CancelColumnReorderDrag();
        return;
    }
    const int32_t slot = ColumnReorderSlotAt(state, pointerX + offset, offset);

    if (auto const indicator = m_columnReorderIndicator.get())
    {
        if (slot == state.ownSlot || state.bounds.empty())
        {
            indicator.Visibility(winrt::Visibility::Collapsed);
        }
        else
        {
            auto const& bounds = slot < static_cast<int32_t>(state.bounds.size()) ? state.bounds[static_cast<size_t>(slot)] : state.bounds.back();
            const double edge = (slot < static_cast<int32_t>(state.bounds.size()) ? bounds.left : bounds.right) + (bounds.frozen ? 0.0 : -offset);
            const double width = std::isnan(indicator.Width()) ? 0.0 : indicator.Width();
            winrt::Canvas::SetLeft(indicator, std::clamp(edge - width / 2.0, 0.0, (std::max)(0.0, viewportWidth - width)));
            indicator.Height(row.ActualHeight());
            indicator.Visibility(winrt::Visibility::Visible);
        }
    }
    if (auto const ghost = m_columnReorderGhost.get())
    {
        const double width = ghost.Width();
        winrt::Canvas::SetLeft(ghost, std::clamp(pointerX - state.grabOffset, 0.0, (std::max)(0.0, viewportWidth - width)));
    }

    // A frozen column moves only within the frozen band, which does not scroll.
    int32_t step = 0;
    if (auto const bodyScroller = m_bodyScroller.get(); !state.frozen && bodyScroller)
    {
        if (pointerX < ComputeLeadingFrozenWidth() + c_autoScrollZone && offset > 0.0)
        {
            step = -1;
        }
        else if (pointerX > viewportWidth - c_autoScrollZone && offset < bodyScroller.ScrollableWidth())
        {
            step = 1;
        }
    }
    m_columnReorderScrollStep = step;
    if (step != 0 && !m_columnReorderScrollTimer)
    {
        if (auto const queue = DispatcherQueue())
        {
            m_columnReorderScrollTimer = queue.CreateTimer();
            m_columnReorderScrollTimer.Interval(std::chrono::milliseconds(50));
            m_columnReorderScrollTimer.Tick([weakThis = get_weak()](winrt::DispatcherQueueTimer const& sender, auto const&)
                {
                    if (auto const strongThis = weakThis.get())
                    {
                        strongThis->OnColumnReorderScrollTick();
                    }
                    else
                    {
                        sender.Stop();
                    }
                });
            m_columnReorderScrollTimer.Start();
        }
    }
    else if (step == 0)
    {
        StopColumnReorderScroll();
    }
}

void TableView::StopColumnReorderScroll()
{
    m_columnReorderScrollStep = 0;
    if (m_columnReorderScrollTimer)
    {
        m_columnReorderScrollTimer.Stop();
        m_columnReorderScrollTimer = nullptr;
    }
}

void TableView::OnColumnReorderScrollTick()
{
    auto const scroller = m_bodyScroller.get();
    if (!m_columnReorder || !m_columnReorder->dragging || m_columnReorderScrollStep == 0 || !scroller)
    {
        StopColumnReorderScroll();
        return;
    }

    const double target = std::clamp(scroller.HorizontalOffset() + m_columnReorderScrollStep * c_autoScrollStep, 0.0, scroller.ScrollableWidth());
    scroller.ChangeView(winrt::IReference<double>{ target }, nullptr, nullptr, true /* disableAnimation */);
    UpdateColumnReorderDrag();
}

bool TableView::ConsumeColumnReorderTap()
{
    return std::exchange(m_suppressHeaderTap, false);
}

void TableView::CancelColumnReorderDrag()
{
    EndColumnReorderDrag(false /* commit */);
}

void TableView::EndColumnReorderDrag(bool commit)
{
    if (!m_columnReorder)
    {
        return;
    }

    // Cleared first: releasing the capture and applying the move both re-enter the pointer handlers.
    auto state = std::move(*m_columnReorder);
    m_columnReorder.reset();
    StopColumnReorderScroll();
    m_columnReorderHostSizeChangedRevoker.revoke();
    if (auto const target = m_columnReorderKeyTarget.get())
    {
        target.RemoveHandler(winrt::UIElement::PreviewKeyDownEvent(), m_columnReorderKeyHandler);
    }
    m_columnReorderKeyTarget = nullptr;
    m_columnReorderKeyHandler = nullptr;

    if (auto const ghost = m_columnReorderGhost.get())
    {
        uint32_t index = 0;
        if (auto const overlay = m_columnReorderOverlay.get(); overlay && overlay.Children().IndexOf(ghost, index))
        {
            overlay.Children().RemoveAt(index);
        }
        m_columnReorderGhost.set(nullptr);
    }
    if (auto const indicator = m_columnReorderIndicator.get())
    {
        indicator.Visibility(winrt::Visibility::Collapsed);
    }

    auto const header = state.header.get();
    if (header && state.dragging)
    {
        if (state.headerOpacity)
        {
            RestoreLocalValue(header, winrt::UIElement::OpacityProperty(), state.headerOpacity);
        }
        header.ReleasePointerCaptures();
    }

    auto const column = state.column.get();
    if (!commit || !state.dragging || !column)
    {
        return;
    }
    TryReorderColumn(column, [this, &state]()
        {
            PrepareColumnReorderSlots(state);
            double pointerX = 0.0, width = 0.0, offset = 0.0;
            if (state.ownSlot < 0 || state.bounds.size() != state.others.size() || !ColumnReorderViewport(state, pointerX, width, offset))
            {
                return -1;
            }
            const int32_t slot = ColumnReorderSlotAt(state, pointerX + offset, offset);
            return slot == state.ownSlot ? -1 : ColumnReorderTargetForSlot(state.column.get(), state.others, slot);
        });
}
