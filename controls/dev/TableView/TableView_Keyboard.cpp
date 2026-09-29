// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewRow.h"
#include "ResizeGripper.h"
#include "TableViewGroupHeader.h"
#include "GridCoordinateHelper.h"
#include "ResourceAccessor.h"
#include "Utils.h"

#include <algorithm>
#include <cmath>
#include <limits>


namespace
{
    bool IsKeyDown(winrt::VirtualKey key)
    {
        return (winrt::InputKeyboardSource::GetKeyStateForCurrentThread(key) &
            winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down;
    }

    // Row cells carry the same column Tag as header cells, so finding a tagged ancestor is not
    // enough: the walk must actually reach the header host, or a focused cell (an open editor,
    // most visibly) would resolve to a column and let arrow keys resize it.
    // Returns the focused header cell in headerCell, so a caller that needs the cell does not have
    // to scan the header band again to find what this walk already passed through.
    winrt::TableViewColumn ResolveFocusedHeaderColumn(
        const winrt::IInspectable& source,
        const winrt::Panel& headerHost,
        winrt::FrameworkElement& headerCell)
    {
        headerCell = nullptr;
        if (!headerHost)
        {
            return nullptr;
        }

        winrt::TableViewColumn candidate{ nullptr };
        auto current = source.try_as<winrt::DependencyObject>();
        while (current)
        {
            if (current == headerHost)
            {
                return candidate;
            }

            if (!candidate)
            {
                if (auto const element = current.try_as<winrt::FrameworkElement>())
                {
                    candidate = element.Tag().try_as<winrt::TableViewColumn>();
                    if (candidate)
                    {
                        headerCell = element;
                    }
                }
            }
            current = winrt::VisualTreeHelper::GetParent(current);
        }

        headerCell = nullptr;
        return nullptr;
    }

    void AnnounceColumnWidthOn(const winrt::IInspectable& announcer, const winrt::TableViewColumn& column)
    {
        auto const element = announcer.try_as<winrt::UIElement>();
        if (!element || !column)
        {
            return;
        }

        if (!winrt::AutomationPeer::ListenerExists(winrt::AutomationEvents::Notification))
        {
            return;
        }

        auto peer = winrt::FrameworkElementAutomationPeer::FromElement(element);
        if (!peer)
        {
            return;
        }

        winrt::hstring headerName = peer.GetName();
        if (headerName.empty())
        {
            if (auto const stringable = column.Header().try_as<winrt::IStringable>())
            {
                headerName = stringable.ToString();
            }
        }

        auto const width = winrt::to_hstring(static_cast<int32_t>(std::lround(column.ActualWidth())));

        try
        {
            auto const format = ResourceAccessor::GetLocalizedStringResource(SR_TableViewColumnWidthChanged);
            if (format.empty())
            {
                return;
            }

            peer.RaiseNotificationEvent(
                winrt::AutomationNotificationKind::Other,
                winrt::AutomationNotificationProcessing::MostRecent,
                StringUtil::FormatString(format, headerName.c_str(), width.c_str()),
                L"TableViewColumnWidthChangedActivityId");
        }
        catch (...)
        {
            // Best-effort UIA announcement; missing PRI/UIA must not cancel the resize.
        }
    }
}

// Both input paths end in DragCompleted, so the announcement lives there rather than in the key
// handler: a pointer resize was otherwise completely silent to assistive technology.
void TableView::AnnounceColumnWidth(const winrt::IInspectable& announcer, const winrt::TableViewColumn& column)
{
    AnnounceColumnWidthOn(announcer, column);
}

bool TableView::TryHandleHeaderColumnResizeKey(const winrt::KeyRoutedEventArgs& args)
{
    if (args.Handled())
    {
        return false;
    }

    // Escape aborts a pointer drag in flight; the host reverts to the width it captured at
    // DragStarted. Checked before the arrow keys because it is valid regardless of focus.
    if (args.Key() == winrt::Windows::System::VirtualKey::Escape)
    {
        if (auto const drag = m_activeColumnResizeDrag)
        {
            CancelColumnResizeDrag();
            args.Handled(true);
            return true;
        }
        return false;
    }

    const auto key = args.Key();
    if (key != winrt::Windows::System::VirtualKey::Left &&
        key != winrt::Windows::System::VirtualKey::Right)
    {
        return false;
    }

    if (IsKeyDown(winrt::VirtualKey::Menu))
    {
        return false;
    }

    if (!CanUserResizeColumns())
    {
        return false;
    }

    winrt::FrameworkElement headerCell{ nullptr };
    auto const column = ResolveFocusedHeaderColumn(args.OriginalSource(), m_headerHost.get(), headerCell);
    if (!column || !column.CanResize())
    {
        return false;
    }

    auto const gripper = FindResizeGripperInCell(headerCell);
    if (!gripper)
    {
        return false;
    }

    // The gripper owns direction, the RTL mirror, the step size and the Shift multiplier: one
    // implementation for both key paths.
    if (!gripper.TryKeyboardStep(key))
    {
        return false;
    }

    // The gripper carries no UIA value, so the resize is otherwise silent. The announcement is
    // raised from DragCompleted, which both input paths reach.

    args.Handled(true);
    return true;
}

winrt::TableViewColumn TableView::ResolveHeaderSortKeyTarget(const winrt::KeyRoutedEventArgs& args)
{
    if (IsKeyDown(winrt::VirtualKey::Menu) ||
        IsKeyDown(winrt::VirtualKey::Control) ||
        IsKeyDown(winrt::VirtualKey::Shift))
    {
        return nullptr;
    }

    if (!CanUserSortColumns())
    {
        return nullptr;
    }

    winrt::FrameworkElement headerCell{ nullptr };
    auto const column = ResolveFocusedHeaderColumn(args.OriginalSource(), m_headerHost.get(), headerCell);
    if (!column || !column.CanSort())
    {
        return nullptr;
    }

    // The walk above matches any descendant of a header cell, so require the key to have been
    // raised on the header chrome itself: a header template can host a TextBox, and
    // AcceptsReturn=false leaves Enter unhandled, which would otherwise toggle the sort.
    if (args.OriginalSource().try_as<winrt::DependencyObject>() != headerCell)
    {
        return nullptr;
    }

    return column;
}

// Enter sorts the column whose header has focus. Left/Right are already taken by resize, so
// activation lands on the standard activation keys.
//
// Space is deliberately NOT activated here: XAML activation semantics (ButtonInteraction) are that
// Enter fires on key down while Space arms on key down and fires on key up, which is what lets a
// user press Space, change their mind and move focus away without activating.
bool TableView::TryHandleHeaderSortKey(const winrt::KeyRoutedEventArgs& args)
{
    if (args.Handled())
    {
        return false;
    }

    const auto key = args.Key();
    const bool isEnter = key == winrt::Windows::System::VirtualKey::Enter;
    const bool isSpace = key == winrt::Windows::System::VirtualKey::Space;
    if (!isEnter && !isSpace)
    {
        return false;
    }

    auto const column = ResolveHeaderSortKeyTarget(args);
    if (!column)
    {
        return false;
    }

    if (args.KeyStatus().WasKeyDown)
    {
        if (isSpace && m_headerSortSpaceArmedColumn.get() == column)
        {
            args.Handled(true);
            return true;
        }
        return false;
    }

    if (isSpace)
    {
        m_headerSortSpaceArmedColumn = winrt::make_weak(column);
        args.Handled(true);
        return true;
    }

    if (!ToggleSortDirection(column))
    {
        return false;
    }

    args.Handled(true);
    return true;
}

bool TableView::TryHandleHeaderSortKeyUp(const winrt::KeyRoutedEventArgs& args)
{
    auto const armed = m_headerSortSpaceArmedColumn.get();
    if (!armed)
    {
        return false;
    }

    if (args.Key() != winrt::Windows::System::VirtualKey::Space)
    {
        return false;
    }

    if (args.Handled())
    {
        return false;
    }

    m_headerSortSpaceArmedColumn = nullptr;

    if (ResolveHeaderSortKeyTarget(args) != armed)
    {
        return false;
    }

    if (!ToggleSortDirection(armed))
    {
        return false;
    }

    args.Handled(true);
    return true;
}

void TableView::OnKeyUpForHeaderSort(
    const winrt::IInspectable& /*sender*/,
    const winrt::KeyRoutedEventArgs& args)
{
    TryHandleHeaderSortKeyUp(args);
}

void TableView::OnPreviewKeyDownForNavigation(
    const winrt::IInspectable& /*sender*/,
    const winrt::KeyRoutedEventArgs& args)
{
    // Snapshot before XAML's built-in focus navigation so bubbling handlers move from the pre-key cell.
    switch (args.Key())
    {
    case winrt::Windows::System::VirtualKey::Up:
    case winrt::Windows::System::VirtualKey::Down:
    case winrt::Windows::System::VirtualKey::Home:
    case winrt::Windows::System::VirtualKey::End:
    case winrt::Windows::System::VirtualKey::PageUp:
    case winrt::Windows::System::VirtualKey::PageDown:
        m_navAnchorRow = GetFocusedRowIndex();
        break;
    default:
        break;
    }

    switch (args.Key())
    {
    case winrt::Windows::System::VirtualKey::Up:
    case winrt::Windows::System::VirtualKey::Down:
    case winrt::Windows::System::VirtualKey::Left:
    case winrt::Windows::System::VirtualKey::Right:
    case winrt::Windows::System::VirtualKey::Home:
    case winrt::Windows::System::VirtualKey::End:
    case winrt::Windows::System::VirtualKey::PageUp:
    case winrt::Windows::System::VirtualKey::PageDown:
        m_navAnchorCellRow = -1;
        m_navAnchorCellColumn = -1;
        TryGetFocusedCell(m_navAnchorCellRow, m_navAnchorCellColumn, true /* requireExactCell */);
        break;
    default:
        break;
    }
}

void TableView::OnKeyDownForNavigation(
    const winrt::IInspectable& /*sender*/,
    const winrt::KeyRoutedEventArgs& args)
{
    // An open editor owns its keys. Row navigation would scroll the edited row out of the
    // realization window, recycling it mid-edit, and a single-line TextBox does not mark
    // PageUp/PageDown handled - so without this guard the DEFAULT editor is enough to trigger it.
    // WPF's DataGrid suppresses navigation the same way while a cell is being edited.
    if (IsEditing())
    {
        if (auto const editingElement = CurrentEditingElement())
        {
            if (auto const root = XamlRoot())
            {
                auto focused = winrt::FocusManager::GetFocusedElement(root).try_as<winrt::DependencyObject>();
                while (focused)
                {
                    if (focused == editingElement)
                    {
                        return;
                    }
                    focused = winrt::VisualTreeHelper::GetParent(focused);
                }
            }
        }
    }

    // Column resize from a focused header: after the editing guard, so an open editor keeps its
    // arrow keys, and before row navigation, since the header band is not part of it.
    if (TryHandleHeaderColumnResizeKey(args))
    {
        return;
    }

    if (TryHandleHeaderSortKey(args))
    {
        return;
    }

    // Registered with handledEventsToo so navigation can still run after the ancestor
    // PART_BodyScroller marks nav keys Handled for scrolling. But handledEventsToo also
    // surfaces keys a focused *descendant* consumed (e.g. an editor/ComboBox inside a
    // TableViewTemplateColumn cell). Distinguish the two: only act on an already-handled
    // key when focus is on one of OUR TableViewRow containers (the scroller-handled case).
    // Requiring the focused row to belong to m_rowsRepeater also prevents a nested
    // TableView's inner-row key from double-navigating this outer table.
    if (args.Handled())
    {
        bool focusOnOurRow = false;
        if (auto const root = XamlRoot())
        {
            // A group header is as much "one of our containers" as a data row: both are elements of
            // m_rowsRepeater and both are valid arrow-navigation anchors. Recognizing only rows here
            // ate keys pressed while a header had focus (the scroller marks nav keys Handled before
            // this bubbling handler runs, so the guard returned early and no navigation happened).
            //
            // A focused CELL counts too, and is now the normal case. It is deliberately matched
            // exactly rather than by walking up from any descendant: a TextBox or ComboBox inside a
            // template column marks Home/End/arrows Handled for itself, and letting those through
            // here would take the keys away from the control the user is actually in.
            if (auto const focused = winrt::FocusManager::GetFocusedElement(root).try_as<winrt::UIElement>())
            {
                if (focused.try_as<winrt::TableViewRow>() || focused.try_as<winrt::TableViewGroupHeader>())
                {
                    if (auto const repeater = m_rowsRepeater.get())
                    {
                        focusOnOurRow = repeater.GetElementIndex(focused) >= 0;
                    }
                }
                else
                {
                    int32_t focusedRow = -1, focusedColumn = -1;
                    focusOnOurRow = TryGetFocusedCell(focusedRow, focusedColumn, true /* requireExactCell */);
                }
            }
        }
        if (!focusOnOurRow)
        {
            return;
        }
    }

    const auto key = args.Key();

    if (TryHandleCellNavigationKey(args))
    {
        return;
    }

    const int32_t rowCount = GetItemsSourceCount();
    if (rowCount <= 0)
    {
        return;
    }

    if (key == winrt::Windows::System::VirtualKey::Space && CanSelectRows())
    {
        if (auto const root = XamlRoot())
        {
            if (auto const repeater = m_rowsRepeater.get())
            {
                int32_t focusedIndex = -1;
                if (auto const focusedRow = winrt::FocusManager::GetFocusedElement(root).try_as<winrt::TableViewRow>())
                {
                    focusedIndex = repeater.GetElementIndex(focusedRow);
                }
                else
                {
                    int32_t focusedColumn = -1;
                    if (!TryGetFocusedCell(focusedIndex, focusedColumn, true /* requireExactCell */))
                    {
                        focusedIndex = -1;
                    }
                }

                if (focusedIndex >= 0)
                {
                    SelectRowIndexFromInteraction(focusedIndex);
                    args.Handled(true);
                }
            }
        }

        return;
    }

    // Anchor on the row focus was on BEFORE this key (captured in PreviewKeyDown). The framework's
    // built-in navigation may have already advanced focus one row and marked the key Handled;
    // navigating from the post-move row would skip a row.
    int32_t currentRow = m_navAnchorRow;

    // Ctrl+Arrow moves the focus cursor WITHOUT selecting, matching ListViewBase. Without it a
    // keyboard-only user cannot review other rows and come back, and every row they pass through
    // raises SelectionChanged plus UIA selection events - a selection storm for a screen reader.
    const bool isControlDown = IsKeyDown(winrt::VirtualKey::Control);

    // Focus was not on one of our rows (the user clicked a header, tabbed away and back, or closed
    // a dialog). With selection on, resume relative-navigation from the SELECTED row rather than
    // restarting at row 0 - otherwise Down after clicking away yanks the selection to the top of
    // the table. Absolute keys (Home/End/Page*) keep their own entry points below.
    if (currentRow < 0 &&
        (key == winrt::Windows::System::VirtualKey::Up ||
            key == winrt::Windows::System::VirtualKey::Down))
    {
        if (const int32_t selectedRow = SelectedIndexInternal();
            selectedRow >= 0 && selectedRow < rowCount)
        {
            currentRow = selectedRow;
        }
    }
    if (currentRow < 0)
    {
        // First navigation key enters at the WPF DataGrid/ListView-equivalent row.
        int32_t initialRow = -1;
        switch (key)
        {
        case winrt::Windows::System::VirtualKey::Up:
        case winrt::Windows::System::VirtualKey::Down:
        case winrt::Windows::System::VirtualKey::Home:
        case winrt::Windows::System::VirtualKey::PageUp:
            initialRow = 0;
            break;
        case winrt::Windows::System::VirtualKey::End:
            initialRow = rowCount - 1;
            break;
        case winrt::Windows::System::VirtualKey::PageDown:
            initialRow = std::clamp(GetEstimatedRowsPerPage() - 1, 0, rowCount - 1);
            break;
        default:
            break;
        }
        if (initialRow >= 0 && FocusRow(initialRow))
        {
            // Single selection follows the keyboard cursor, matching ListView and WPF's DataGrid.
            if (!isControlDown)
            {
                SelectRowIndexFromKeyboardFocus(initialRow);
            }
            args.Handled(true);
        }
        return;
    }

    int32_t newRow = currentRow;
    bool consumeKey = true;
    // Absolute/page keys are consumed at boundaries; Up/Down may escape the table.
    bool absoluteRowNav = false;

    switch (key)
    {
    case winrt::Windows::System::VirtualKey::Up:
    {
        // Use the 1-column logical grid to reuse clamping behavior.
        GridCoordinateHelper helper{ rowCount, 1 };
        int32_t nextR = -1, nextC = -1;
        if (helper.TryGetNextFocusableCell(currentRow, 0,
                winrt::FocusNavigationDirection::Up, false, nextR, nextC))
        {
            newRow = nextR;
        }
        break;
    }
    case winrt::Windows::System::VirtualKey::Down:
    {
        GridCoordinateHelper helper{ rowCount, 1 };
        int32_t nextR = -1, nextC = -1;
        if (helper.TryGetNextFocusableCell(currentRow, 0,
                winrt::FocusNavigationDirection::Down, false, nextR, nextC))
        {
            newRow = nextR;
        }
        break;
    }
    case winrt::Windows::System::VirtualKey::Home:
        newRow = 0;
        absoluteRowNav = true;
        break;
    case winrt::Windows::System::VirtualKey::End:
        newRow = rowCount - 1;
        absoluteRowNav = true;
        break;
    case winrt::Windows::System::VirtualKey::PageUp:
    {
        const int32_t step = GetEstimatedRowsPerPage();
        newRow = std::max(0, currentRow - step);
        absoluteRowNav = true;
        break;
    }
    case winrt::Windows::System::VirtualKey::PageDown:
    {
        const int32_t step = GetEstimatedRowsPerPage();
        newRow = std::min(rowCount - 1, currentRow + step);
        absoluteRowNav = true;
        break;
    }
    default:
        consumeKey = false;
        break;
    }

    if (consumeKey)
    {
        const bool hasCellAnchor =
            m_navAnchorRow >= 0 &&
            m_navAnchorCellRow == m_navAnchorRow &&
            m_navAnchorCellColumn >= 0;
        const int32_t anchorColumn = hasCellAnchor ? m_navAnchorCellColumn : m_currentCellColumn;
        if (newRow != currentRow)
        {
            if (FocusCell(newRow, anchorColumn))
            {
                if (!isControlDown)
                {
                    SelectRowIndexFromKeyboardFocus(newRow);
                }
                args.Handled(true);
            }
        }
        else if (absoluteRowNav)
        {
            args.Handled(true);
        }
    }
}

bool TableView::FocusRow(int32_t index)
{
    return FocusCell(index, m_currentCellColumn);
}

// ----- Cell-level keyboard focus -----

winrt::TableViewRow TableView::GetRealizedRowAt(int32_t rowIndex) const
{
    if (rowIndex < 0)
    {
        return nullptr;
    }
    if (auto const repeater = m_rowsRepeater.get())
    {
        return repeater.TryGetElement(rowIndex).try_as<winrt::TableViewRow>();
    }
    return nullptr;
}

winrt::UIElement TableView::ResolveFocusEntryCell(
    winrt::TableViewRow const& row, winrt::DependencyObject const& oldFocusedElement)
{
    if (!row)
    {
        return nullptr;
    }

    auto targetRow = row;

    if (m_currentCellRow >= 0 && !IsWithinThisTableView(oldFocusedElement))
    {
        if (auto const remembered = GetRealizedRowAt(m_currentCellRow))
        {
            targetRow = remembered;
        }
    }

    auto const rowImpl = winrt::get_self<TableViewRow>(targetRow);
    const int32_t cellCount = rowImpl->GetVisibleCellCountInternal();
    if (cellCount <= 0)
    {
        return nullptr;
    }

    return rowImpl->GetVisibleCellInternal(std::clamp(m_currentCellColumn, 0, cellCount - 1));
}

bool TableView::IsWithinThisTableView(winrt::DependencyObject const& element)
{
    winrt::TableView const self = *this;
    auto const selfObject = self.try_as<winrt::DependencyObject>();
    auto current = element;
    while (current)
    {
        if (current == selfObject)
        {
            return true;
        }
        current = winrt::VisualTreeHelper::GetParent(current);
    }
    return false;
}

void TableView::OnRowCellFocusChanged(winrt::TableViewRow const& row)
{
    if (!row)
    {
        return;
    }

    auto const repeater = m_rowsRepeater.get();
    if (!repeater)
    {
        return;
    }

    auto const rowImpl = winrt::get_self<TableViewRow>(row);

    winrt::UIElement focusedCell{ nullptr };
    if (auto const root = XamlRoot())
    {
        if (auto const focused = winrt::FocusManager::GetFocusedElement(root).try_as<winrt::DependencyObject>())
        {
            focusedCell = rowImpl->FindOwnCellInternal(focused, false /* requireExact */);
        }
    }

    if (!focusedCell)
    {
        return;
    }

    const int32_t columnIndex = rowImpl->GetVisibleCellIndexInternal(focusedCell);
    if (columnIndex >= 0)
    {
        m_currentCellColumn = columnIndex;
    }

    const int32_t rowIndex = repeater.GetElementIndex(row);
    if (rowIndex >= 0)
    {
        m_currentCellRow = rowIndex;
    }

    if (!IsEditing())
    {
        if (auto const column = rowImpl->GetCellOwningColumn(focusedCell))
        {
            winrt::IInspectable item{ nullptr };
            if (rowIndex >= 0 && TryGetItemAtRowIndex(rowIndex, item) && item)
            {
                SetCurrentCell(item, column);
            }
        }
    }
}

bool TableView::TryGetFocusedCell(int32_t& rowIndex, int32_t& columnIndex, bool requireExactCell) const
{
    rowIndex = -1;
    columnIndex = -1;

    auto const repeater = m_rowsRepeater.get();
    auto const root = XamlRoot();
    if (!repeater || !root)
    {
        return false;
    }

    auto const focused = winrt::FocusManager::GetFocusedElement(root).try_as<winrt::DependencyObject>();
    if (!focused)
    {
        return false;
    }

    winrt::DependencyObject node = focused;
    while (node)
    {
        if (auto const row = node.try_as<winrt::TableViewRow>())
        {
            const int32_t index = repeater.GetElementIndex(row);
            if (index >= 0)
            {
                auto const cell = winrt::get_self<TableViewRow>(row)->FindOwnCellInternal(focused, requireExactCell);
                if (!cell)
                {
                    return false;
                }
                const int32_t column = winrt::get_self<TableViewRow>(row)->GetVisibleCellIndexInternal(cell);
                if (column < 0)
                {
                    return false;
                }
                rowIndex = index;
                columnIndex = column;
                return true;
            }
        }
        node = winrt::VisualTreeHelper::GetParent(node);
    }

    return false;
}

bool TableView::FocusCell(int32_t rowIndex, int32_t visibleColumnIndex)
{
    auto repeater = m_rowsRepeater.get();
    if (!repeater)
    {
        return false;
    }

    const auto rowCount = GetItemsSourceCount();
    if (rowIndex < 0 || rowIndex >= rowCount)
    {
        return false;
    }

    if (visibleColumnIndex >= 0)
    {
        m_currentCellColumn = visibleColumnIndex;
    }
    const int32_t targetColumn = m_currentCellColumn;

    if (m_pendingFocusLayoutToken.value)
    {
        LayoutUpdated(m_pendingFocusLayoutToken);
        m_pendingFocusLayoutToken = {};
    }

    auto element = repeater.GetOrCreateElement(rowIndex);
    if (!element)
    {
        return false;
    }

    auto frameworkElement = element.try_as<winrt::FrameworkElement>();
    if (frameworkElement)
    {
        frameworkElement.StartBringIntoView();
    }

    if (element.try_as<winrt::Control>())
    {
        if (frameworkElement &&
            (!frameworkElement.IsLoaded() ||
                frameworkElement.ActualHeight() <= 0.0 ||
                !winrt::VisualTreeHelper::GetParent(frameworkElement)))
        {
            auto weakThis = get_weak();
            m_pendingFocusLayoutToken = LayoutUpdated(
                [weakThis, rowIndex, targetColumn](winrt::IInspectable const&, winrt::IInspectable const&)
                {
                    if (auto strongThis = weakThis.get())
                    {
                        if (strongThis->m_pendingFocusLayoutToken.value)
                        {
                            strongThis->LayoutUpdated(strongThis->m_pendingFocusLayoutToken);
                            strongThis->m_pendingFocusLayoutToken = {};
                        }

                        if (rowIndex < 0 || rowIndex >= strongThis->GetItemsSourceCount())
                        {
                            return;
                        }

                        if (auto repeater = strongThis->m_rowsRepeater.get())
                        {
                            if (auto element = repeater.GetOrCreateElement(rowIndex))
                            {
                                strongThis->FocusRealizedRowCell(element, targetColumn);
                            }
                        }
                    }
                });
            return true;
        }

        return FocusRealizedRowCell(element, targetColumn);
    }
    return false;
}

// Focuses the cell at `visibleColumnIndex` inside an already-realized container. Group headers are
// elements of the same repeater and have no cells, so they keep taking container focus.
bool TableView::FocusRealizedRowCell(winrt::UIElement const& element, int32_t visibleColumnIndex)
{
    if (auto const row = element.try_as<winrt::TableViewRow>())
    {
        auto const rowImpl = winrt::get_self<TableViewRow>(row);
        const int32_t cellCount = rowImpl->GetVisibleCellCountInternal();
        if (cellCount > 0)
        {
            const int32_t column = std::clamp(visibleColumnIndex, 0, cellCount - 1);
            if (rowImpl->FocusVisibleCellInternal(column, winrt::FocusState::Keyboard))
            {
                return true;
            }
        }
    }

    if (auto const control = element.try_as<winrt::Control>())
    {
        return control.Focus(winrt::FocusState::Keyboard);
    }
    return false;
}

// Left / Right / Home / End (and their Ctrl forms) move the cursor within the grid of cells.
//
// Modelled on WPF's DataGrid and on ListView/GridView keyboarding: Left/Right step one cell and
// stop at the row's first/last visible column without wrapping, Home/End jump to the ends of the
// CURRENT row, and Ctrl+Home/Ctrl+End jump to the first cell of the first row / the last cell of
// the last row.
//
// Moving within a row never touches selection: selection stays row-level, so there is nothing to
// change and nothing to announce. That is also why this path never calls
// SelectRowIndexFromKeyboardFocus and so never has to reach for the double-speak suppression flag.
bool TableView::TryHandleCellNavigationKey(const winrt::KeyRoutedEventArgs& args)
{
    if (IsKeyDown(winrt::VirtualKey::Menu))
    {
        return false;
    }

    if (!TryMoveCellCursorFromAnchor(
            args.Key(), IsKeyDown(winrt::VirtualKey::Control),
            m_navAnchorCellRow, m_navAnchorCellColumn))
    {
        return false;
    }

    args.Handled(true);
    return true;
}

bool TableView::TryMoveCellCursor(winrt::Windows::System::VirtualKey key, bool isControlDown)
{
    int32_t anchorRow = -1;
    int32_t anchorColumn = -1;
    TryGetFocusedCell(anchorRow, anchorColumn, true /* requireExactCell */);
    return TryMoveCellCursorFromAnchor(key, isControlDown, anchorRow, anchorColumn);
}

bool TableView::TryMoveCellCursorFromAnchor(
    winrt::Windows::System::VirtualKey key, bool isControlDown,
    int32_t anchorRow, int32_t anchorColumn)
{
    const bool isLeft = key == winrt::Windows::System::VirtualKey::Left;
    const bool isRight = key == winrt::Windows::System::VirtualKey::Right;
    const bool isHome = key == winrt::Windows::System::VirtualKey::Home;
    const bool isEnd = key == winrt::Windows::System::VirtualKey::End;
    if (!isLeft && !isRight && !isHome && !isEnd)
    {
        return false;
    }

    if (isControlDown && (isLeft || isRight))
    {
        return false;
    }

    // Live focus is needed to make the re-assert below idempotent.
    int32_t liveRow = -1;
    int32_t liveColumn = -1;
    const bool onLiveCell = TryGetFocusedCell(liveRow, liveColumn, true /* requireExactCell */);

    int32_t currentRow = anchorRow;
    int32_t currentColumn = anchorColumn;
    // Use the pre-key anchor; live focus may already include XAML's built-in directional move.
    const bool onCell = currentRow >= 0 && currentColumn >= 0;

    if (onCell && onLiveCell && liveRow != currentRow)
    {
        currentRow = liveRow;
        currentColumn = liveColumn;
    }

    const int32_t rowCount = GetItemsSourceCount();
    if (rowCount <= 0)
    {
        return false;
    }

    if (!onCell)
    {
        // Ctrl+Home/End may enter the grid; relative keys need an existing cell cursor.
        if (!isControlDown || (!isHome && !isEnd))
        {
            return false;
        }

        const int32_t targetRow = isHome ? 0 : rowCount - 1;
        return FocusCell(targetRow, isHome ? 0 : c_lastColumnSentinel);
    }

    auto const row = GetRealizedRowAt(currentRow);
    if (!row)
    {
        return false;
    }

    const int32_t cellCount = winrt::get_self<TableViewRow>(row)->GetVisibleCellCountInternal();
    if (cellCount <= 0)
    {
        return false;
    }

    if (isControlDown && (isHome || isEnd))
    {
        const int32_t targetRow = isHome ? 0 : rowCount - 1;
        // Ctrl moves the cursor without dragging selection/UIA events through every row.
        FocusCell(targetRow, isHome ? 0 : c_lastColumnSentinel);
        return true;
    }

    int32_t targetColumn = currentColumn;
    if (isHome)
    {
        targetColumn = 0;
    }
    else if (isEnd)
    {
        targetColumn = cellCount - 1;
    }
    else
    {
        // RTL mirrors the arrows: Right means "towards the row end" in reading order.
        const bool isRtl = FlowDirection() == winrt::FlowDirection::RightToLeft;
        const bool forward = isRight != isRtl;
        targetColumn = std::clamp(currentColumn + (forward ? 1 : -1), 0, cellCount - 1);
    }

    // Re-assert idempotently: XAML's built-in Left/Right navigation may already have moved focus, so a naive second move skipped columns and boundary cells.
    // Selection stays row-scoped, avoiding the double-speak suppression path entirely.
    if (!onLiveCell || liveRow != currentRow || liveColumn != targetColumn)
    {
        winrt::get_self<TableViewRow>(row)->FocusVisibleCellInternal(
            targetColumn, winrt::FocusState::Keyboard);
    }
    m_currentCellColumn = targetColumn;

    return true;
}

int32_t TableView::GetFocusedRowIndex() const
{
    if (auto repeater = m_rowsRepeater.get())
    {
        if (auto root = XamlRoot())
        {
            if (auto focused = winrt::FocusManager::GetFocusedElement(root).try_as<winrt::DependencyObject>())
            {
                winrt::DependencyObject node = focused;
                while (node)
                {
                    // Both data rows and group headers are elements of m_rowsRepeater, so either is a
                    // valid focus anchor for arrow navigation. A header MUST be recognized here: if
                    // it reports -1, relative navigation falls back to row 0 / the selected row and
                    // fights the framework's built-in focus move (skipped rows, focus bouncing back
                    // to the previous header, and selection landing on the wrong row).
                    if (node.try_as<winrt::TableViewRow>() || node.try_as<winrt::TableViewGroupHeader>())
                    {
                        if (auto const element = node.try_as<winrt::UIElement>())
                        {
                            const auto idx = repeater.GetElementIndex(element);
                            if (idx >= 0)
                            {
                                return idx;
                            }
                        }
                    }
                    node = winrt::VisualTreeHelper::GetParent(node);
                }
            }
        }
    }
    return -1;
}

int32_t TableView::GetEstimatedRowsPerPage()
{
    if (auto sv = m_bodyScroller.get())
    {
        const auto vh = sv.ViewportHeight();

        double rowH = GetDensityRowMinHeight(); // Density is the best fallback before realized rows can be sampled.
        if (auto repeater = m_rowsRepeater.get())
        {
            // A grouped projection realizes group headers alongside data rows, and a header is
            // typically taller than a row. Paging moves the focused *data* row, so measure a data
            // row; only fall back to any realized element when no row has been realized yet.
            double anyChildH = 0.0;
            const int32_t childCount = winrt::VisualTreeHelper::GetChildrenCount(repeater);
            for (int32_t i = 0; i < childCount; i++)
            {
                auto const child = winrt::VisualTreeHelper::GetChild(repeater, i);
                auto const el = child.try_as<winrt::FrameworkElement>();
                if (!el)
                {
                    continue;
                }

                const auto h = el.ActualHeight();
                if (h <= 0)
                {
                    continue;
                }

                if (child.try_as<winrt::TableViewRow>())
                {
                    anyChildH = h;
                    break;
                }

                if (anyChildH <= 0)
                {
                    anyChildH = h;
                }
            }

            if (anyChildH > 0)
            {
                rowH = anyChildH;
            }
        }

        if (vh > 0 && rowH > 0)
        {
            const auto pageRows = static_cast<int32_t>(vh / rowH);
            return std::max(1, pageRows);
        }
    }
    return 10; // sensible fallback when template isn't realized yet.
}
