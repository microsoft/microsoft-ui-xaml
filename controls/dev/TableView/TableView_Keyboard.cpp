// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewRow.h"
#include "GridCoordinateHelper.h"

#include <algorithm>

// Row keyboard navigation and its focus/measurement helpers live here.

void TableView::OnPreviewKeyDownForNavigation(
    const winrt::IInspectable& /*sender*/,
    const winrt::KeyRoutedEventArgs& args)
{
    // Runs on the tunneling pass, before the framework's built-in focus navigation moves focus for
    // this key. Record the focused row now so OnKeyDownForNavigation (bubbling, possibly after the
    // built-in move) advances from the pre-move row. -1 when focus isn't on one of our rows.
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
            if (auto const focusedRow = winrt::FocusManager::GetFocusedElement(root).try_as<winrt::TableViewRow>())
            {
                if (auto const repeater = m_rowsRepeater.get())
                {
                    focusOnOurRow = repeater.GetElementIndex(focusedRow) >= 0;
                }
            }
        }
        if (!focusOnOurRow)
        {
            return;
        }
    }

    const auto key = args.Key();

    // GridCoordinateHelper keeps row-navigation clamp semantics in one place.
    const int32_t rowCount = GetItemsSourceCount();
    if (rowCount <= 0)
    {
        return;
    }

    // Space selects the focused row without moving it. Only when the ROW ITSELF has focus: a Space
    // pressed inside a cell's interactive content (a CheckBox in a template column) belongs to that
    // control, and swallowing it here would break it.
    if (key == winrt::Windows::System::VirtualKey::Space && CanSelectRows())
    {
        if (auto const root = XamlRoot())
        {
            if (auto const focusedRow = winrt::FocusManager::GetFocusedElement(root).try_as<winrt::TableViewRow>())
            {
                if (auto const repeater = m_rowsRepeater.get())
                {
                    const auto focusedIndex = repeater.GetElementIndex(focusedRow);
                    if (focusedIndex >= 0)
                    {
                        SelectRowIndexFromInteraction(focusedIndex);
                        args.Handled(true);
                    }
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
    const bool isControlDown =
        (winrt::InputKeyboardSource::GetKeyStateForCurrentThread(winrt::VirtualKey::Control) &
            winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down;

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
                SelectRowIndexFromInteraction(initialRow);
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
        if (newRow != currentRow)
        {
            // The framework's built-in navigation may have already advanced focus to newRow; if so
            // don't move again (that doubling is the alternate-row skip) — just consume the key.
            if (GetFocusedRowIndex() == newRow || FocusRow(newRow))
            {
                if (!isControlDown)
                {
                    SelectRowIndexFromInteraction(newRow);
                }
                args.Handled(true);
            }
        }
        else if (absoluteRowNav)
        {
            // Consume boundary no-ops so PART_BodyScroller does not scroll.
            args.Handled(true);
        }
    }
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
                    if (auto row = node.try_as<winrt::TableViewRow>())
                    {
                        const auto idx = repeater.GetElementIndex(row);
                        if (idx >= 0)
                        {
                            return idx;
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
            const int32_t childCount = winrt::VisualTreeHelper::GetChildrenCount(repeater);
            for (int32_t i = 0; i < childCount; i++)
            {
                if (auto el = winrt::VisualTreeHelper::GetChild(repeater, i).try_as<winrt::FrameworkElement>())
                {
                    const auto h = el.ActualHeight();
                    if (h > 0)
                    {
                        rowH = h;
                        break;
                    }
                }
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

bool TableView::FocusRow(int32_t index)
{
    auto repeater = m_rowsRepeater.get();
    if (!repeater)
    {
        return false;
    }

    const auto rowCount = GetItemsSourceCount();
    if (index < 0 || index >= rowCount)
    {
        return false;
    }

    // A new focus request supersedes any earlier pending deferred (off-screen) focus, so an
    // older callback can't later yank focus back to a stale index over this newer one.
    if (m_pendingFocusLayoutToken.value)
    {
        LayoutUpdated(m_pendingFocusLayoutToken);
        m_pendingFocusLayoutToken = {};
    }

    // Materialize virtualized rows before focusing and scrolling them into view.
    auto element = repeater.GetOrCreateElement(index);
    if (!element)
    {
        return false;
    }

    auto frameworkElement = element.try_as<winrt::FrameworkElement>();
    if (frameworkElement)
    {
        // Scroll through PART_BodyScroller without animation when focus moves.
        frameworkElement.StartBringIntoView();
    }

    if (auto control = element.try_as<winrt::Control>())
    {
        if (frameworkElement &&
            (!frameworkElement.IsLoaded() ||
                frameworkElement.ActualHeight() <= 0.0 ||
                !winrt::VisualTreeHelper::GetParent(frameworkElement)))
        {
            // Keep only one pending deferred focus callback (any prior one was cleared above).
            auto weakThis = get_weak();
            m_pendingFocusLayoutToken = LayoutUpdated(
                [weakThis, index](winrt::IInspectable const&, winrt::IInspectable const&)
                {
                    if (auto strongThis = weakThis.get())
                    {
                        if (strongThis->m_pendingFocusLayoutToken.value)
                        {
                            strongThis->LayoutUpdated(strongThis->m_pendingFocusLayoutToken);
                            strongThis->m_pendingFocusLayoutToken = {};
                        }

                        if (index < 0 || index >= strongThis->GetItemsSourceCount())
                        {
                            return;
                        }

                        if (auto repeater = strongThis->m_rowsRepeater.get())
                        {
                            if (auto element = repeater.GetOrCreateElement(index))
                            {
                                if (auto control = element.try_as<winrt::Control>())
                                {
                                    control.Focus(winrt::FocusState::Keyboard);
                                }
                            }
                        }
                    }
                });
            return true;
        }

        return control.Focus(winrt::FocusState::Keyboard);
    }
    return false;
}
