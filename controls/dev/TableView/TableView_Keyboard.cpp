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
#include "TableViewAutomationHelpers.h"
#include "Utils.h"
#include "SharedHelpers.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

// Row keyboard navigation and its focus/measurement helpers live here.

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

    // The header band in visible-column order. The predicate is the same one TableViewRow uses to
    // enumerate its visible cells (IsVisibleColumn on the owning column), so a header's index here
    // and a row's visible-column index are ONE coordinate space - which is what lets Tab hand the
    // column the user was on in one band to the other band instead of guessing at it.
    std::vector<winrt::FrameworkElement> GetVisibleHeaderCells(const winrt::Panel& headerHost)
    {
        std::vector<winrt::FrameworkElement> cells;
        if (!headerHost)
        {
            return cells;
        }

        auto const children = headerHost.Children();
        const uint32_t size = children.Size();
        cells.reserve(size);
        for (uint32_t i = 0; i < size; ++i)
        {
            auto const cell = children.GetAt(i).try_as<winrt::FrameworkElement>();
            if (!cell)
            {
                continue;
            }
            if (!IsVisibleColumn(cell.Tag().try_as<winrt::TableViewColumn>()))
            {
                continue;
            }
            cells.push_back(cell);
        }
        return cells;
    }

    int32_t IndexOfHeaderCell(
        const std::vector<winrt::FrameworkElement>& cells, const winrt::FrameworkElement& cell)
    {
        if (!cell)
        {
            return -1;
        }
        for (size_t i = 0; i < cells.size(); ++i)
        {
            if (cells[i] == cell)
            {
                return static_cast<int32_t>(i);
            }
        }
        return -1;
    }

    // "Actionable" (sortable or resizable) is what RebuildHeaders stamps onto IsTabStop. Now that
    // the band is a single tab stop, that flag no longer buys a header its own Tab press - it
    // decides whether the header is focusable at all, and therefore whether ARROW navigation can
    // land on it. A header with nothing to activate is still skipped, exactly as before.
    bool IsFocusableHeaderCell(const winrt::FrameworkElement& cell)
    {
        auto const element = cell.try_as<winrt::UIElement>();
        return element && element.IsTabStop() && element.Visibility() == winrt::Visibility::Visible;
    }

    // The focused header is the element AT is on, so attribute the announcement to its peer.
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

        // Whole pixels: sub-pixel precision is noise in an announcement.
        auto const width = TableViewDetails::FormatIntegerForCurrentCulture(static_cast<int32_t>(std::lround(column.ActualWidth())));

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
            // The host app may not merge the control's PRI; a missing string must not break resize.
            // Best-effort UIA announcement; missing PRI/UIA must not cancel the resize.
        }
    }
}

// Both input paths end in DragCompleted, so the announcement lives there rather than in the key
// handler: a pointer resize was otherwise completely silent to assistive technology.
void TableView::AnnounceColumnWidth(const winrt::IInspectable& announcer, const winrt::TableViewColumn& column)
{
    // FromElement never creates a peer. During a keyboard resize the header cell is focused so one
    // exists, but a pointer drag needs no focus, so fall back to the table's own peer.
    if (auto const element = announcer.try_as<winrt::UIElement>();
        element && winrt::FrameworkElementAutomationPeer::FromElement(element))
    {
        AnnounceColumnWidthOn(announcer, column);
    }
    else
    {
        AnnounceColumnWidthOn(*this, column);
    }
}

// Alt+Left / Alt+Right resizes the column whose header has focus; Shift takes the large step, and
// Ctrl is accepted as an alias. This is the WPF DataGrid binding ("Default Keyboard and Mouse
// Behavior in the DataGrid Control"), which leaves the bare arrows free to NAVIGATE between
// headers inside the band. Driving the gripper's own Begin/Try/End keeps pointer and keyboard on
// one clamping path.
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

    // Alt is the resize modifier, not a disqualifier. Alt+Arrow is an ordinary accelerator chord:
    // the window menu opens on SC_KEYMENU, which the OS only synthesizes when Alt is pressed and
    // released with NO other key in the chord, so an arrow in the chord already suppresses it.
    // Marking the key handled below additionally stops the chord reaching the access-key manager.
    if (!IsKeyDown(winrt::VirtualKey::Menu))
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

// ----- Header-band keyboard navigation -----
//
// Model: two bands, each ONE tab stop, arrows confined to the band they start in. This is what
// Explorer's Details view does, and what WinUI's own ListView / GridView / ItemsView do. Tab and
// Shift+Tab are the ONLY way between the header band and the body; no arrow key crosses between
// them in either direction.
//
// Within the band the arrows work in the same visible-column coordinate space as the cell cursor,
// so the column a user leaves the band on is the column Tab hands to the body.

int32_t TableView::GetFocusedVisibleHeaderIndex() const
{
    auto const host = m_headerHost.get();
    auto const root = XamlRoot();
    if (!host || !root)
    {
        return -1;
    }

    auto const focused = winrt::FocusManager::GetFocusedElement(root);
    if (!focused)
    {
        return -1;
    }

    winrt::FrameworkElement headerCell{ nullptr };
    if (!ResolveFocusedHeaderColumn(focused, host, headerCell) || !headerCell)
    {
        return -1;
    }

    return IndexOfHeaderCell(GetVisibleHeaderCells(host), headerCell);
}

// `step` of 0 tries only `visibleIndex`; +1/-1 keeps walking in that direction while the header
// refuses focus. A header is focusable only while it is a tab stop (IsTabStop is set from
// "resizable or sortable"), so a decorative header transparently hands the move on to the next
// actionable one instead of swallowing the key. This is what keeps EVERY actionable column's sort
// and resize reachable now that the band is a single tab stop.
int32_t TableView::FocusVisibleHeaderFrom(int32_t visibleIndex, int32_t step)
{
    auto const host = m_headerHost.get();
    if (!host)
    {
        return -1;
    }

    auto const cells = GetVisibleHeaderCells(host);
    const int32_t count = static_cast<int32_t>(cells.size());
    if (count <= 0 || visibleIndex < 0 || visibleIndex >= count)
    {
        return -1;
    }

    for (int32_t i = visibleIndex; i >= 0 && i < count; i += step)
    {
        auto const& cell = cells[static_cast<size_t>(i)];
        if (IsFocusableHeaderCell(cell))
        {
            if (auto const element = cell.try_as<winrt::UIElement>();
                element && element.Focus(winrt::FocusState::Keyboard))
            {
                return i;
            }
        }
        if (step == 0)
        {
            break;
        }
    }

    return -1;
}

// The header the band should be entered on.
//
// FIRST entry - nothing has moved the shared column cursor yet - is always the band's FIRST
// focusable header. The remembered column is honoured only once the cursor has actually been
// established by a user move (m_columnCursorEstablished).
//
// This distinction has to be explicit because m_currentCellColumn is an int that starts at 0, and
// 0 is indistinguishable from "never set". Any write to the shared cursor before the band is first
// entered therefore silently relocated the entry point, and the band looked like it had an
// unreachable first column even though Left could still get there.
//
// The "remembered column" is m_currentCellColumn - the body's existing cell cursor - not a second
// header-only cursor. One cursor is what makes Tab in either direction between the two bands agree
// on which column the user is in, once there IS a column the user is in.
int32_t TableView::ResolveHeaderEntryIndex(const std::vector<winrt::FrameworkElement>& cells) const
{
    const int32_t count = static_cast<int32_t>(cells.size());
    if (count <= 0)
    {
        return -1;
    }

    const int32_t preferred = m_columnCursorEstablished
        ? std::clamp(m_currentCellColumn, 0, count - 1)
        : 0;
    for (int32_t i = preferred; i < count; ++i)
    {
        if (IsFocusableHeaderCell(cells[static_cast<size_t>(i)]))
        {
            return i;
        }
    }
    for (int32_t i = preferred - 1; i >= 0; --i)
    {
        if (IsFocusableHeaderCell(cells[static_cast<size_t>(i)]))
        {
            return i;
        }
    }
    return -1;
}

// Tab into the band lands on its first focusable header, because the band is one tab stop. Redirect
// that to the remembered column. Modelled on TableViewRow::OnRowGettingFocus, including its
// escape-hatch guard: focus LEAVING the band is never pulled back, or Tab could not get out.
void TableView::OnHeaderHostGettingFocus(
    const winrt::IInspectable& /*sender*/,
    const winrt::Microsoft::UI::Xaml::Input::GettingFocusEventArgs& args)
{
    auto const host = m_headerHost.get();
    if (!host)
    {
        return;
    }

    // Tab / Shift+Tab only. A programmatic Focus() (an arrow step within the band, a test hook)
    // reports Direction None and already names the header it means; redirecting it would break the
    // column-exact contract those paths depend on.
    auto const direction = args.Direction();
    if (direction != winrt::FocusNavigationDirection::Next &&
        direction != winrt::FocusNavigationDirection::Previous)
    {
        return;
    }

    auto const hostObject = host.try_as<winrt::DependencyObject>();
    auto const oldFocus = args.OldFocusedElement();
    if (oldFocus &&
        (oldFocus == hostObject || SharedHelpers::IsAncestor(oldFocus, hostObject, false /* checkVisibility */)))
    {
        // Tabbing OUT of the band. Leave it alone.
        return;
    }

    auto const cells = GetVisibleHeaderCells(host);
    const int32_t target = ResolveHeaderEntryIndex(cells);
    if (target < 0)
    {
        // No actionable header: the band has no tab stop at all and XAML's own target stands.
        return;
    }

    auto const element = cells[static_cast<size_t>(target)].try_as<winrt::DependencyObject>();
    if (!element || element == args.NewFocusedElement())
    {
        return;
    }

    // Refused during some focus operations; failing just leaves focus on the band's first header,
    // which is still inside the band.
    args.TrySetNewFocusedElement(element);
}

// Records the column of whatever header actually took focus - Tab entry, arrow step or pointer
// press - into the shared cursor, so Tab onward into the body enters at that column instead of
// snapping back to column 0.
void TableView::OnHeaderHostGotFocus(
    const winrt::IInspectable& /*sender*/,
    const winrt::RoutedEventArgs& /*args*/)
{
    if (const int32_t index = GetFocusedVisibleHeaderIndex(); index >= 0)
    {
        // A header actually took focus, so the user IS in a column now. From here on the band's
        // entry point follows the cursor rather than restarting at the first header.
        SetColumnCursorInternal(index);
    }
}

// Bare Left/Right on a focused header steps one header, clamping at the band's ends.
//
// Like the cell move, this is an idempotent RE-ASSERT computed from the PRE-KEY anchor captured in
// PreviewKeyDown, not a blind step from live focus: XAML's built-in directional navigation may
// already have moved header focus before this bubbling handler runs, and stepping again from there
// would skip a column. Applied only when focus is not already on the target.
bool TableView::TryHandleHeaderNavigationKey(const winrt::KeyRoutedEventArgs& args)
{
    if (args.Handled())
    {
        return false;
    }

    // Alt is the resize chord and Ctrl is not a header gesture; neither navigates.
    if (IsKeyDown(winrt::VirtualKey::Menu) || IsKeyDown(winrt::VirtualKey::Control))
    {
        return false;
    }

    const auto key = args.Key();
    const bool isLeft = key == winrt::Windows::System::VirtualKey::Left;
    const bool isRight = key == winrt::Windows::System::VirtualKey::Right;
    if (!isLeft && !isRight)
    {
        return false;
    }

    auto const host = m_headerHost.get();
    if (!host)
    {
        return false;
    }

    auto const cells = GetVisibleHeaderCells(host);
    const int32_t count = static_cast<int32_t>(cells.size());
    if (count <= 0)
    {
        return false;
    }

    const int32_t liveIndex = GetFocusedVisibleHeaderIndex();

    // Anchor on the header focus was on BEFORE the key; fall back to live focus when the snapshot
    // is stale (no routed snapshot ran, or the band was rebuilt under it).
    int32_t currentIndex = m_navAnchorHeaderColumn;
    if (currentIndex < 0 || currentIndex >= count)
    {
        currentIndex = liveIndex;
    }
    if (currentIndex < 0)
    {
        // Focus is not on the header band; this is not a header gesture.
        return false;
    }

    // RTL mirrors the arrows exactly as the cell move does: Right means "towards the row end" in
    // reading order. Same expression, so the two bands can never disagree.
    const bool isRtl = FlowDirection() == winrt::FlowDirection::RightToLeft;
    const bool forward = isRight != isRtl;
    const int32_t step = forward ? 1 : -1;
    const int32_t targetIndex = std::clamp(currentIndex + step, 0, count - 1);

    if (liveIndex != targetIndex)
    {
        FocusVisibleHeaderFrom(targetIndex, targetIndex == currentIndex ? 0 : step);
    }

    // Consumed even at a bound and even when focus refused: letting Left on the first header or
    // Right on the last fall through to directional focus navigation would walk focus straight out
    // of the table, which is the escape the cell cursor already blocks at its own bounds.
    args.Handled(true);
    return true;
}

// Up/Down on a focused header are clamped inside the band: the header band and the body are
// separate tab stops, and Tab is the only way between them.
//
// Consumed rather than left to fall through, following the same precedent the cell cursor uses at
// its horizontal bounds: an unconsumed arrow reaches directional focus navigation, which would
// walk focus out of the band - and, before the band had its own vertical handling, reached ROW
// navigation, which read "focus is not on a row" as "enter the table" and jumped into row 0.
bool TableView::TryHandleHeaderVerticalKey(const winrt::KeyRoutedEventArgs& args)
{
    if (args.Handled())
    {
        return false;
    }

    if (IsKeyDown(winrt::VirtualKey::Menu))
    {
        return false;
    }

    const auto key = args.Key();
    if (key != winrt::Windows::System::VirtualKey::Down &&
        key != winrt::Windows::System::VirtualKey::Up)
    {
        return false;
    }

    int32_t headerIndex = m_navAnchorHeaderColumn;
    if (headerIndex < 0)
    {
        headerIndex = GetFocusedVisibleHeaderIndex();
    }
    if (headerIndex < 0)
    {
        // Focus is not on the header band; this is not a header gesture.
        return false;
    }

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
    // Snapshot before XAML's built-in focus navigation so bubbling handlers move from the pre-key
    // cell. Left/Right deliberately snapshot only the CELL anchor: row navigation never handles
    // them, and overwriting m_navAnchorRow here would discard the row anchor instead of leaving it
    // for the next vertical key.
    switch (args.Key())
    {
    case winrt::Windows::System::VirtualKey::Up:
    case winrt::Windows::System::VirtualKey::Down:
    case winrt::Windows::System::VirtualKey::Home:
    case winrt::Windows::System::VirtualKey::End:
    case winrt::Windows::System::VirtualKey::PageUp:
    case winrt::Windows::System::VirtualKey::PageDown:
        m_navAnchorRow = GetFocusedRowIndex();
        [[fallthrough]];
    case winrt::Windows::System::VirtualKey::Left:
    case winrt::Windows::System::VirtualKey::Right:
        m_navAnchorCellRow = -1;
        m_navAnchorCellColumn = -1;
        TryGetFocusedCell(m_navAnchorCellRow, m_navAnchorCellColumn, true /* requireExactCell */);
        // Body navigation is two-level, so the pre-key snapshot has to record WHICH level focus was
        // on. A cell anchor above means cell level; this is the row-level counterpart, and it is
        // what lets Right on a row drill in even after built-in directional navigation has already
        // moved focus off the row.
        m_navAnchorRowContainer = m_navAnchorCellColumn >= 0 ? -1 : GetFocusedRowContainerIndex();
        // A group header is the third possibility, and it is mutually exclusive with the other two:
        // it is neither a TableViewRow nor a cell. Left/Right there collapse/expand.
        m_navAnchorGroupHeader =
            (m_navAnchorCellColumn >= 0 || m_navAnchorRowContainer >= 0) ? -1 : GetFocusedGroupHeaderIndex();
        // The header band needs the same pre-key snapshot as the cell grid: the arrows now
        // navigate it, and built-in directional navigation can move header focus first.
        m_navAnchorHeaderColumn = GetFocusedVisibleHeaderIndex();
        break;
    default:
        m_navAnchorHeaderColumn = -1;
        m_navAnchorRowContainer = -1;
        m_navAnchorGroupHeader = -1;
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
    // arrow keys. Alt+Left/Alt+Right only, so it cannot shadow the bare arrows below.
    if (TryHandleHeaderColumnResizeKey(args))
    {
        return;
    }

    // The header band is a band of its own: bare Left/Right step between headers, Up/Down are
    // clamped inside it. Before sort, so an arrow cannot reach the sort gesture, and before row
    // navigation, whose row cursor has no meaning while focus is on a header.
    if (TryHandleHeaderNavigationKey(args))
    {
        return;
    }

    if (TryHandleHeaderVerticalKey(args))
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

    // Two-level body navigation. Group headers first: they have no cells, so Left/Right there mean
    // collapse/expand, and the row-level drill must never claim them. Then the row/cell drill:
    // Right on a ROW enters its first cell, Left on a row's FIRST cell pops back out to the row.
    // Both run before the cell cursor, which owns Left/Right everywhere else in the row and would
    // otherwise clamp at column 0 instead of letting Left escape row-ward. Both run after the
    // header handlers, so a focused column header keeps its own Left/Right.
    if (TryHandleGroupHeaderExpandCollapseKey(args))
    {
        return;
    }

    if (TryHandleRowLevelDrillKey(args))
    {
        return;
    }

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
        if (initialRow >= 0 && FocusRowContainer(initialRow))
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
            // Vertical navigation keeps the LEVEL it started on, which is what the ARIA `treegrid`
            // pattern asks for: Up/Down on a row move between rows, Up/Down on a cell move between
            // cells in the same column. The cell anchor is the authority on which level that is -
            // it is set only when a cell actually held focus before the key.
            const bool moved = hasCellAnchor
                ? FocusCell(newRow, anchorColumn)
                : FocusRowContainer(newRow);
            if (moved)
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
    // Follows the cursor's current LEVEL: row-level callers (body entry, the first navigation key)
    // land on the row container, and a cursor already drilled into cells stays on cells.
    return m_cellCursorActive
        ? FocusCell(index, m_currentCellColumn)
        : FocusRowContainer(index);
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

winrt::TableViewRow TableView::ResolveFocusEntryRow(
    winrt::TableViewRow const& row, winrt::DependencyObject const& oldFocusedElement)
{
    if (!row)
    {
        return nullptr;
    }

    winrt::DependencyObject const selfObject = *this;
    const bool focusCameFromWithin =
        oldFocusedElement == selfObject ||
        SharedHelpers::IsAncestor(oldFocusedElement, selfObject, false /* checkVisibility */);

    if (m_currentCellRow < 0 || focusCameFromWithin)
    {
        return row;
    }

    // m_currentCellRow is a bare index, and nothing renumbers it when the source reshapes
    // (sort, filter, group expand/collapse, insert, remove), so index 2 can name a different
    // record by the time focus comes back. Follow the remembered ITEM instead: m_currentItem is
    // written by the same OnRowCellFocusChanged funnel that writes m_currentCellRow, and it is
    // cleared with SetCurrentCell(nullptr, nullptr) when the source is replaced.
    auto const rememberedItem = m_currentItem.get();
    winrt::IInspectable itemAtRememberedRow{ nullptr };
    auto const stillTheSameRecord =
        rememberedItem &&
        TryGetItemAtRowIndex(m_currentCellRow, itemAtRememberedRow) &&
        SameInspectableIdentity(itemAtRememberedRow, rememberedItem);

    // A reshape that only moved the item is recoverable: FindRealizedRowForItem searches
    // realized rows only, so this never forces a realization or a surprise scroll. When the
    // item is gone or off-screen, focus stays on the row the framework aimed at.
    if (auto const remembered = stillTheSameRecord
            ? GetRealizedRowAt(m_currentCellRow)
            : FindRealizedRowForItem(rememberedItem))
    {
        return remembered;
    }

    return row;
}

// Switches the two-level cursor. Popping OUT of cell level has to put the row that was drilled in
// back to row level, or that row is left as the one row in the body that Tab cannot reach.
void TableView::SetCellCursorActiveInternal(bool active)
{
    if (!active)
    {
        if (auto const drilled = m_cellLevelRow.get())
        {
            winrt::get_self<TableViewRow>(drilled)->SetCellLevelInternal(false);
        }
        m_cellLevelRow = nullptr;
    }

    m_cellCursorActive = active;
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
    const int32_t rowIndex = repeater.GetElementIndex(row);

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
        // Focus is on the ROW container itself - the body's entry level. Record the row so a later
        // re-entry comes back here, and make sure the two-level cursor agrees that no cell is
        // current, including releasing whichever row was previously drilled in.
        if (rowIndex >= 0)
        {
            m_currentCellRow = rowIndex;
        }
        SetCellCursorActiveInternal(false);
        return;
    }

    const int32_t columnIndex = rowImpl->GetVisibleCellIndexInternal(focusedCell);
    if (columnIndex >= 0)
    {
        // A cell actually took focus, so the user is in a column. Establishes the shared cursor for
        // the header band too - Shift+Tab back up should land on the column being worked in.
        SetColumnCursorInternal(columnIndex);
    }

    if (rowIndex >= 0)
    {
        m_currentCellRow = rowIndex;
    }

    // A cell holds focus, so the cursor is at cell level on THIS row. Release any other row that
    // was still drilled in before claiming this one, so only one row ever has its cells armed.
    if (auto const previous = m_cellLevelRow.get(); previous && previous != row)
    {
        winrt::get_self<TableViewRow>(previous)->SetCellLevelInternal(false);
    }
    rowImpl->SetCellLevelInternal(true);
    m_cellLevelRow = winrt::make_weak(row);
    m_cellCursorActive = true;

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
    if (visibleColumnIndex >= 0)
    {
        // An explicit column names the column the caller wants the cursor on, so it establishes it.
        SetColumnCursorInternal(visibleColumnIndex);
    }

    return FocusRowElementInternal(rowIndex, m_currentCellColumn, true /* cellLevel */);
}

// Single writer for the shared column cursor. Writing it at all means the user (or a caller acting
// for them) has named a column, which is exactly the condition that lets the header band stop
// entering at its first header and start honouring the remembered one. Keeping the flag and the
// value on one setter is what stops the two drifting apart.
void TableView::SetColumnCursorInternal(int32_t visibleColumnIndex)
{
    if (visibleColumnIndex < 0)
    {
        return;
    }

    m_currentCellColumn = visibleColumnIndex;
    m_columnCursorEstablished = true;
}

// Puts the cursor back to "never moved", so the next band entry starts at the first focusable
// header again. Called when the data set is replaced, alongside the CurrentItem/CurrentCell reset.
void TableView::ResetColumnCursorInternal()
{
    m_currentCellColumn = 0;
    m_columnCursorEstablished = false;
}

bool TableView::FocusRowContainer(int32_t rowIndex)
{
    // Row level keeps the remembered column untouched: it is what the next Right will drill into.
    return FocusRowElementInternal(rowIndex, m_currentCellColumn, false /* cellLevel */);
}

// Shared realization + deferred-focus path for both levels. Realizing a row can require a layout
// pass before the container is focusable, so the "not laid out yet" case parks a one-shot
// LayoutUpdated and finishes there; `cellLevel` rides along so the deferred half lands on the same
// level the caller asked for.
bool TableView::FocusRowElementInternal(int32_t rowIndex, int32_t targetColumn, bool cellLevel)
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
                [weakThis, rowIndex, targetColumn, cellLevel](winrt::IInspectable const&, winrt::IInspectable const&)
                {
                    auto strongThis = weakThis.get();
                    if (!strongThis)
                    {
                        return;
                    }

                    if (strongThis->m_pendingFocusLayoutToken.value)
                    {
                        strongThis->LayoutUpdated(strongThis->m_pendingFocusLayoutToken);
                        strongThis->m_pendingFocusLayoutToken = {};
                    }

                    // This runs from a LAYOUT callback. An exception escaping here does not reach
                    // any app handler - it unwinds through XAML's layout pass and fails the
                    // process fast - so the realization and focus work is contained.
                    try
                    {
                        if (rowIndex < 0 || rowIndex >= strongThis->GetItemsSourceCount())
                        {
                            return;
                        }

                        if (auto repeater = strongThis->m_rowsRepeater.get())
                        {
                            if (auto element = repeater.GetOrCreateElement(rowIndex))
                            {
                                if (cellLevel)
                                {
                                    strongThis->FocusRealizedRowCell(element, targetColumn);
                                }
                                else
                                {
                                    strongThis->FocusRowContainerInternal(element);
                                }
                            }
                        }
                    }
                    catch (...)
                    {
                        // Best-effort deferred focus: the row can be recycled or the source
                        // reshaped between the request and this callback.
                    }
                });
            return true;
        }

        return cellLevel
            ? FocusRealizedRowCell(element, targetColumn)
            : FocusRowContainerInternal(element);
    }
    return false;
}

// Focuses an already-realized container AT ROW LEVEL. Group headers are elements of the same
// repeater; they have no cells and are always a row-level target.
bool TableView::FocusRowContainerInternal(winrt::UIElement const& element)
{
    auto const row = element.try_as<winrt::TableViewRow>();
    if (row)
    {
        // Make the row a tab stop BEFORE moving focus, but leave its cells armed for the moment: a
        // row still drilled in is not focusable at all, and clearing IsTabStop on the cell that is
        // holding focus right now would be the wrong order.
        winrt::get_self<TableViewRow>(row)->EnableRowFocusInternal();
    }

    auto const control = element.try_as<winrt::Control>();
    if (!control || !control.Focus(winrt::FocusState::Keyboard))
    {
        return false;
    }

    // Focus has landed on the row. Now it is safe to take cells out of the tab order - both this
    // row's and those of whichever other row was still drilled in.
    if (row)
    {
        winrt::get_self<TableViewRow>(row)->SetCellLevelInternal(false);
    }
    SetCellCursorActiveInternal(false);
    return true;
}

// Focuses the cell at `visibleColumnIndex` inside an already-realized container, drilling the
// cursor to CELL level. Group headers are elements of the same repeater and have no cells, so they
// stay at row level and keep taking container focus.
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

        // The row has no focusable cell (no columns, or every column collapsed). Row level is the
        // only level left, and it is a legitimate resting place in the two-level model.
        return FocusRowContainerInternal(element);
    }

    // A group header: never a cell-level target.
    return FocusRowContainerInternal(element);
}

// Left / Right on a focused GROUP HEADER expand and collapse it. This is the `treegrid` rule for a
// parent row, and it is the explicit counterpart to TryHandleRowLevelDrillKey below:
//
//   * on a GROUP HEADER, Right means EXPAND and Left means COLLAPSE. A group header has no cells,
//     so there is no cell level to drill into and the row-level drill must never claim these keys.
//   * on a DATA ROW, Right means GO TO THE FIRST CELL and Left (at the first cell) means come back.
//
// TableViewGroupHeader::OnKeyDown already binds the same two keys. This handler does not replace
// it, it backstops it: it is registered handledEventsToo, so it still runs when the header's own
// handler has already marked the key Handled, and it also runs when that handler never got the key
// at all (something upstream consumed it, or focus was on a child of the header rather than the
// header itself). Double application is safe by construction - the request carries a DIRECTION,
// and ApplyGroupExpansionByIdentity documents the directional form as an idempotent set, so two
// identical "expand" requests expand once.
bool TableView::TryHandleGroupHeaderExpandCollapseKey(const winrt::KeyRoutedEventArgs& args)
{
    if (IsKeyDown(winrt::VirtualKey::Menu) || IsKeyDown(winrt::VirtualKey::Control))
    {
        return false;
    }

    const auto key = args.Key();
    const bool isLeft = key == winrt::Windows::System::VirtualKey::Left;
    const bool isRight = key == winrt::Windows::System::VirtualKey::Right;
    if (!isLeft && !isRight)
    {
        return false;
    }

    // Pre-key anchor, for the same reason the cell cursor and the header band use one: built-in
    // directional navigation can move focus off the header before this bubbling handler runs.
    if (m_navAnchorGroupHeader < 0)
    {
        return false;
    }

    auto const repeater = m_rowsRepeater.get();
    if (!repeater)
    {
        return false;
    }

    auto const header = repeater.TryGetElement(m_navAnchorGroupHeader).try_as<winrt::TableViewGroupHeader>();
    if (!header)
    {
        return false;
    }

    // Consumed whether or not the group can actually move: an unconsumed arrow on a header reaches
    // directional focus navigation, which walks focus sideways out of the body.
    args.Handled(true);

    if (!header.IsExpandable())
    {
        return true;
    }

    // RTL mirrors the arrows the same way every other band does: "Right" is towards the row end in
    // reading order, and expanding is the reading-order-forward direction.
    const bool isRtl = FlowDirection() == winrt::FlowDirection::RightToLeft;
    const bool expand = isRight != isRtl;

    // Already in the requested state: nothing to do, but the key stays consumed. Checked here
    // rather than relying on the provider's no-op so a redundant Right does not queue a reshape
    // and a focus-restore round trip on every repeat.
    if (header.IsExpanded() == expand)
    {
        return true;
    }

    SetGroupExpansion(header, expand);
    return true;
}

// The repeater index of the focused GROUP HEADER, else -1. Deliberately EXACT, mirroring
// GetFocusedRowContainerIndex: if an app's group-header template hosts its own focusable content,
// that content keeps the arrow keys it claims rather than having them read as collapse/expand.
int32_t TableView::GetFocusedGroupHeaderIndex() const
{
    auto const repeater = m_rowsRepeater.get();
    auto const root = XamlRoot();
    if (!repeater || !root)
    {
        return -1;
    }

    auto const focused = winrt::FocusManager::GetFocusedElement(root).try_as<winrt::UIElement>();
    if (!focused)
    {
        return -1;
    }

    auto const header = focused.try_as<winrt::TableViewGroupHeader>();
    if (!header)
    {
        return -1;
    }

    // Also rejects a nested TableView's header, which is not an element of our repeater.
    return repeater.GetElementIndex(header);
}

// Right on a focused ROW drills into that row's first cell; Left on the FIRST cell pops back out to
// the ROW. This is the W3C ARIA APG `treegrid` pattern verbatim, which is the right standard here
// because the body can contain expandable group rows.
//
// Deliberately does NOT touch group headers: they have no cells, and Left/Right there mean
// collapse/expand. TryHandleGroupHeaderExpandCollapseKey above claims those first, and the anchor
// this handler reads (m_navAnchorRowContainer) is only ever set for a TableViewRow, so the two can
// never both fire for one key.
bool TableView::TryHandleRowLevelDrillKey(const winrt::KeyRoutedEventArgs& args)
{
    if (IsKeyDown(winrt::VirtualKey::Menu) || IsKeyDown(winrt::VirtualKey::Control))
    {
        return false;
    }

    const auto key = args.Key();
    const bool isLeft = key == winrt::Windows::System::VirtualKey::Left;
    const bool isRight = key == winrt::Windows::System::VirtualKey::Right;
    if (!isLeft && !isRight)
    {
        return false;
    }

    // RTL mirrors the arrows exactly as the cell and header moves do: "Right" means "towards the
    // row end" in reading order, so drilling in is the reading-order-forward key.
    const bool isRtl = FlowDirection() == winrt::FlowDirection::RightToLeft;
    const bool drillIn = isRight != isRtl;

    // Anchored on the PRE-KEY focus captured in PreviewKeyDown, not on live focus: XAML's built-in
    // directional navigation can move focus out of a row before this bubbling handler runs, and
    // reading live focus would then mistake the move for a cell-level one (or for no row at all).
    // Same discipline as the cell cursor and the header band.

    // Case 1: focus was on a ROW container.
    if (m_navAnchorRowContainer >= 0)
    {
        if (!drillIn)
        {
            // Nothing further out at row level. Consumed anyway: an unconsumed Left here reaches
            // directional focus navigation, which would walk focus sideways out of the table.
            args.Handled(true);
            return true;
        }

        auto const row = GetRealizedRowAt(m_navAnchorRowContainer);
        if (!row)
        {
            return false;
        }

        auto const rowImpl = winrt::get_self<TableViewRow>(row);
        const int32_t cellCount = rowImpl->GetVisibleCellCountInternal();
        if (cellCount <= 0)
        {
            // A row with no visible columns has no cell level to drill into.
            args.Handled(true);
            return true;
        }

        // Enter at the remembered column, clamped - the same cursor the header band shares, so
        // drilling in lands where the user last was rather than resetting to column 0.
        rowImpl->FocusVisibleCellInternal(
            std::clamp(m_currentCellColumn, 0, cellCount - 1), winrt::FocusState::Keyboard);
        args.Handled(true);
        return true;
    }

    // Case 2: focus was on a CELL. Only the FIRST cell pops back out; every other cell leaves the
    // key to the cell cursor, which steps columns and clamps.
    if (drillIn || m_navAnchorCellRow < 0 || m_navAnchorCellColumn != 0)
    {
        return false;
    }

    if (!FocusRowContainer(m_navAnchorCellRow))
    {
        // The row refused focus; let the cell cursor have the key rather than swallowing it.
        return false;
    }

    args.Handled(true);
    return true;
}

// The repeater index of the focused element when that element is one of OUR row CONTAINERS, else
// -1. Deliberately exact and deliberately row-only: a group header is a Control of the same
// repeater but owns Left/Right for expand/collapse, and a focused cell is the other level.
int32_t TableView::GetFocusedRowContainerIndex() const
{
    auto const repeater = m_rowsRepeater.get();
    auto const root = XamlRoot();
    if (!repeater || !root)
    {
        return -1;
    }

    auto const focused = winrt::FocusManager::GetFocusedElement(root).try_as<winrt::UIElement>();
    if (!focused)
    {
        return -1;
    }

    auto const row = focused.try_as<winrt::TableViewRow>();
    if (!row)
    {
        return -1;
    }

    // Also rejects a nested TableView's row, which is not an element of our repeater.
    return repeater.GetElementIndex(row);
}

// Left / Right / Home / End (and their Ctrl forms) move the cursor within the grid of cells. This
// is the CELL level of the two-level model; TryHandleRowLevelDrillKey above has already claimed
// the two keys that cross between levels (Right on a row, Left on a row's first cell).
//
// Modelled on WPF's DataGrid and on ListView/GridView keyboarding: Left/Right step one cell and
// stop at the row's first/last visible column without wrapping, Home/End jump to the ends of the
// CURRENT row, and Ctrl+Home/Ctrl+End jump to the first cell of the first row / the last cell of
// the last row.
//
// Moving within a row never touches selection: selection stays row-level, so there is nothing to
// change and nothing to announce.
//
// The horizontal move is an idempotent RE-ASSERT, not a blind step. XAML's built-in directional
// navigation may already have moved focus one cell before this bubbling handler runs, so the move
// is computed from the PRE-KEY anchor and applied only when focus is not already on the target -
// stepping again from live focus skipped a column, and skipped the boundary cell entirely.
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

    int32_t liveRow = -1;
    int32_t liveColumn = -1;
    const bool onLiveCell = TryGetFocusedCell(liveRow, liveColumn, true /* requireExactCell */);

    int32_t currentRow = anchorRow;
    int32_t currentColumn = anchorColumn;
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
        // Consumed even when focus refuses: an unconsumed Home/End reaches the row-level handler,
        // which re-reads it as a jump to the first/last ROW.
        FocusCell(targetRow, isHome ? 0 : c_lastColumnSentinel);
        return true;
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
        // Ctrl moves the cursor without dragging selection/UIA events through every row, and is
        // consumed on failure for the same reason plain Home/End is.
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

    const bool atBound = targetColumn == currentColumn;
    const bool alreadyOnTarget = onLiveCell && liveRow == currentRow && liveColumn == targetColumn;

    bool moved = alreadyOnTarget;
    if (!moved)
    {
        winrt::get_self<TableViewRow>(row)->FocusVisibleCellInternal(
            targetColumn, winrt::FocusState::Keyboard);

        // FocusVisibleCellInternal falls back to focusing the ROW when the cell refuses, so its
        // result means "something took focus", not "the target cell did". Read focus back instead.
        // Non-exact, because a cell whose content owns focus (an open editor) is still a hit.
        int32_t movedRow = -1;
        int32_t movedColumn = -1;
        moved =
            TryGetFocusedCell(movedRow, movedColumn, false /* requireExactCell */) &&
            movedRow == currentRow && movedColumn == targetColumn;
    }

    if (moved)
    {
        SetColumnCursorInternal(targetColumn);
        return true;
    }

    // Focus refused the target (a collapsed or zero-width column, a disabled subtree, or
    // TrySetNewFocusedElement refusing mid-focus-operation). Leave m_currentCellColumn on the
    // column focus is really on, so the next Up/Down still anchors where the user is.
    //
    // The key is still consumed in exactly two cases: at a bound (Left on the first column, Right
    // on the last), where letting it fall through to focus navigation would walk focus out of the
    // table; and Home/End, which the row-level handler below would otherwise re-read as "jump to
    // the first/last ROW". A failed relative step that was not at a bound claims nothing.
    return atBound || isHome || isEnd;
}

int32_t TableView::GetFocusedRowIndex() const
{
    if (auto repeater = m_rowsRepeater.get())
    {
        if (auto root = XamlRoot())
        {
            if (auto focused = winrt::FocusManager::GetFocusedElement(root).try_as<winrt::DependencyObject>())
            {
                // Walk up in case focus is on a row descendant.
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
                        // Nested TableViews can contribute inner rows; keep walking for ours.
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

        // Sample realized visual children; item-index sampling fails after virtualization.
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
