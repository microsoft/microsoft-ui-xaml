// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewRow.h"
#include "ResizeGripper.h"
#include "TableViewGroupHeader.h"
#include "TableViewColumn.h"
#include "TableViewColumnHeader.h"
#include "TableViewToolTipHelpers.h"
#include "GridCoordinateHelper.h"
#include "ResourceAccessor.h"
#include "TableViewAutomationHelpers.h"
#include "TableViewFocusHelpers.h"
#include "Utils.h"
#include "SharedHelpers.h"
#include "TVDiag.h"

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

    // Header and row cells share visible-column coordinates, letting Tab preserve the current
    // column across bands.
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

    winrt::UIElement FindFirstFocusableDescendant(const winrt::UIElement& element)
    {
        if (!element)
        {
            return nullptr;
        }

        const int32_t childCount = winrt::VisualTreeHelper::GetChildrenCount(element);
        for (int32_t i = 0; i < childCount; ++i)
        {
            if (auto const child = winrt::VisualTreeHelper::GetChild(element, i).try_as<winrt::DependencyObject>())
            {
                if (auto const target = winrt::FocusManager::FindFirstFocusableElement(child))
                {
                    if (auto const targetElement = target.try_as<winrt::UIElement>())
                    {
                        return targetElement;
                    }
                }
            }
        }

        return nullptr;
    }

    winrt::Microsoft::UI::Xaml::Documents::Hyperlink FindFirstHyperlinkDescendant(const winrt::DependencyObject& element, int32_t depth)
    {
        const int32_t childCount = depth > 0 ? winrt::VisualTreeHelper::GetChildrenCount(element) : 0;
        for (int32_t i = 0; i < childCount; ++i)
        {
            auto const child = winrt::VisualTreeHelper::GetChild(element, i);
            if (auto const childElement = child.try_as<winrt::UIElement>(); childElement && childElement.Visibility() == winrt::Visibility::Visible)
            {
                if (auto const link = FindHyperlink(childElement))
                {
                    return link;
                }
                if (auto const link = FindFirstHyperlinkDescendant(child, depth - 1))
                {
                    return link;
                }
            }
        }
        return nullptr;
    }

    winrt::DependencyObject GetFocusedVisual(const winrt::XamlRoot& root)
    {
        auto const focused = winrt::FocusManager::GetFocusedElement(root).try_as<winrt::DependencyObject>();
        if (auto const textElement = focused.try_as<winrt::Microsoft::UI::Xaml::Documents::TextElement>())
        {
            return textElement.ContentStart().VisualParent();
        }
        return focused;
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
            // Best-effort UIA announcement; missing PRI/UIA must not cancel resize.
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

// Alt+Left/Right resizes the focused header using WPF DataGrid's binding; Shift is large-step.
// Alt is the only resize modifier (dev-spec Keyboard); the gripper keeps pointer and keyboard on one clamp path.
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
// The table is one tab stop on the header band (the body when no header is actionable); arrows
// move between the bands, and both share visible-column coordinates.

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

// step=0 tries the exact header; +/-1 walks to the next focusable visible header.
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
        if (IsHeaderCellActionableInternal(cell))
        {
            // Drops a stamp made before the header's template content loaded.
            ClearHeaderTabStopStampInternal(cell);
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

// Before any real column cursor exists, enter the first focusable header; afterward, honor the
// shared body/header column cursor. m_columnCursorEstablished distinguishes column 0 from "none".
int32_t TableView::ResolveHeaderEntryIndex(const std::vector<winrt::FrameworkElement>& cells)
{
    const int32_t count = static_cast<int32_t>(cells.size());
    if (count <= 0)
    {
        return -1;
    }

    const int32_t preferred = m_columnCursorEstablished
        ? std::clamp(m_currentCellColumn, 0, count - 1)
        : 0;
    const int32_t after = FindActionableHeaderIndex(cells, preferred, 1);
    return after >= 0 ? after : FindActionableHeaderIndex(cells, preferred - 1, -1);
}

int32_t TableView::FindActionableHeaderIndex(
    const std::vector<winrt::FrameworkElement>& cells, int32_t visibleIndex, int32_t step)
{
    for (int32_t i = visibleIndex; i >= 0 && i < static_cast<int32_t>(cells.size()); i += step)
    {
        if (IsHeaderCellActionableInternal(cells[static_cast<size_t>(i)]))
        {
            return i;
        }
    }
    return -1;
}

// Sortable (static configuration), resizable, has a context flyout or a tooltip, or hosts
// focusable content. A style-set IsTabStop="False" still wins.
bool TableView::IsHeaderCellActionableInternal(const winrt::FrameworkElement& header)
{
    if (!header || header.Visibility() != winrt::Visibility::Visible || !ShouldShowColumnHeaders())
    {
        return false;
    }

    auto const column = header.Tag().try_as<winrt::TableViewColumn>();
    auto const control = header.try_as<winrt::Control>();
    // Our own IsTabStop stamp is not the app's opt-out.
    if (!IsVisibleColumn(column) || (control && (!control.IsEnabled() ||
        (!control.IsTabStop() && !HasHeaderTabStopStampInternal(header)))))
    {
        return false;
    }

    // Resizable only while the gripper is available: a pinned column cannot be resized.
    if (IsColumnUserSortableInternal(column) || (IsColumnResizeAllowed(column) && !IsColumnResizePinned(column)) ||
        column.HeaderContextFlyout() || ContextFlyout() || header.ContextFlyout() ||
        TableViewDetails::HasEffectiveToolTip(header))
    {
        return true;
    }


    // Only the content region: the sort glyph and resize gripper are chrome, not content.
    winrt::DependencyObject content = header;
    if (auto const columnHeader = header.try_as<winrt::TableViewColumnHeader>())
    {
        if (auto const presenter = winrt::get_self<TableViewColumnHeader>(columnHeader)->GetContentPresenterInternal())
        {
            content = presenter;
        }
    }
    int32_t budget = 64;
    return HasActionableContent(content, 8, budget);
}


winrt::FrameworkElement TableView::FindOwnHeaderCell(const winrt::DependencyObject& element) const
{
    auto const host = m_headerHost.get();
    auto const cell = element.try_as<winrt::FrameworkElement>();
    if (!host || !cell || winrt::VisualTreeHelper::GetParent(cell) != host.try_as<winrt::DependencyObject>())
    {
        return nullptr;
    }

    auto const column = cell.Tag().try_as<winrt::TableViewColumn>();
    return column && winrt::get_self<TableViewColumn>(column)->GetOwningTableView() == *this ? cell : nullptr;
}

// The table is one tab stop. Tab and Shift+Tab entry from outside land on the header row, else the
// body. Non-actionable headers are not tab stops (RefreshHeaderTabStopsInternal).
void TableView::OnTableGettingFocus(
    const winrt::IInspectable& /*sender*/,
    const winrt::Microsoft::UI::Xaml::Input::GettingFocusEventArgs& args)
{
    auto const newFocus = args.NewFocusedElement();
    if (!newFocus || args.Cancel())
    {
        return;
    }

    if (args.FocusState() != winrt::FocusState::Keyboard)
    {
        m_cellIntentColumn = -1;
    }

    auto const direction = args.Direction();
    const bool isTab =
        direction == winrt::FocusNavigationDirection::Next ||
        direction == winrt::FocusNavigationDirection::Previous;
    if (!isTab && direction != winrt::FocusNavigationDirection::None && IsInUnrealizedBodyContainer(newFocus))
    {
        args.TryCancel();
        return;
    }
    // Popup-aware so an editor's dropdown counts as inside; a focused Hyperlink stands in as its text
    // host. Focus that was in this table and has left the live tree is XAML's rescue after a reset or
    // editor teardown.
    winrt::DependencyObject const selfObject = *this;
    auto oldFocus = args.OldFocusedElement();
    if (auto const textElement = oldFocus.try_as<winrt::Microsoft::UI::Xaml::Documents::TextElement>())
    {
        oldFocus = textElement.ContentStart().VisualParent();
    }
    auto const oldElement = oldFocus.try_as<winrt::FrameworkElement>();
    const bool oldRemoved = oldElement && (!oldElement.XamlRoot() || !oldElement.IsLoaded()) &&
        oldFocus == m_lastFocusedWithin.get();
    const bool cameFromWithin = oldFocus &&
        (oldRemoved || TableViewDetails::IsWithinElementPopupAware(oldFocus, selfObject, XamlRoot()));

    // A nested TableView resolves its own moves; entry from outside still lands on this table's target.
    for (auto node = newFocus; cameFromWithin && node && node != selfObject; node = winrt::VisualTreeHelper::GetParent(node))
    {
        if (node.try_as<winrt::TableView>())
        {
            return;
        }
    }

    if (!cameFromWithin)
    {
        RefreshHeaderTabStopsInternal();
    }

    if (oldRemoved && isTab)
    {
        if (!TrySetBodyEntryTarget(args))
        {
            QueueFocusTableEntry(newFocus, true /* bodyOnly */);
        }
        return;
    }

    // Tab entry, or app/window focus aimed at a header that is not (or no longer) a target.
    auto const header = FindOwnHeaderCell(newFocus);
    const bool headerInactive = header && !IsHeaderCellActionableInternal(header);
    const bool isEntry = !cameFromWithin &&
        (isTab || (direction == winrt::FocusNavigationDirection::None && args.FocusState() != winrt::FocusState::Pointer && headerInactive));
    if (isEntry && !HasRedirectedFocusEntry(args.CorrelationId()) && !TryRedirectTableEntry(args))
    {
        // Window Tab-wrap and island entry cannot be redirected; focus moves on afterward.
        QueueFocusTableEntry(newFocus, false /* bodyOnly */);
    }
    else if (!isEntry && headerInactive && !args.TryCancel())
    {
        QueueFocusTableEntry(newFocus, false /* bodyOnly */);
    }
}

// Corrects an entry XAML would not let us redirect, unless focus has moved on since.
void TableView::QueueFocusTableEntry(winrt::DependencyObject const& expected, bool bodyOnly)
{
    if (auto const queue = DispatcherQueue())
    {
        queue.TryEnqueue([weakThis = get_weak(), weakExpected = winrt::make_weak(expected), bodyOnly]()
            {
                auto strongThis = weakThis.get();
                auto const root = strongThis ? strongThis->XamlRoot() : nullptr;
                if (!root)
                {
                    return;
                }
                try
                {
                    auto const focused = winrt::FocusManager::GetFocusedElement(root).try_as<winrt::DependencyObject>();
                    if (focused && focused != weakExpected.get())
                    {
                        return;
                    }
                    auto const cells = GetVisibleHeaderCells(strongThis->m_headerHost.get());
                    const int32_t index = bodyOnly || !strongThis->ShouldShowColumnHeaders() ? -1 : strongThis->ResolveHeaderEntryIndex(cells);
                    if (index < 0 || strongThis->FocusVisibleHeaderFrom(index, 0) < 0)
                    {
                        strongThis->FocusBodyEntryTarget(false /* select */);
                    }
                }
                catch (...)
                {
                    TVDiag::LogRetailF(L"[TableView] Moving focus to the table entry failed.");
                }
            });
    }
}

// Hidden headers hand focus to the body: XAML's rescue off the header is redirected, and a queued move
// covers XAML not moving focus at all.
void TableView::MoveFocusOffCollapsingHeaders()
{
    auto const root = XamlRoot();
    auto const queue = DispatcherQueue();
    if (!root || !queue)
    {
        return;
    }

    // Focus may be on content inside the header (a Button, CheckBox or inline Hyperlink in its template).
    winrt::FrameworkElement headerCell{ nullptr };
    if (!ResolveFocusedHeaderColumn(GetFocusedVisual(root), m_headerHost.get(), headerCell) || !headerCell)
    {
        return;
    }

    winrt::DependencyObject const header = headerCell;
    auto const focused = winrt::FocusManager::GetFocusedElement(root).try_as<winrt::UIElement>();
    auto const focusState = focused ? focused.FocusState() : winrt::FocusState::Programmatic;
    auto const isWithinHeader = [](winrt::DependencyObject element, winrt::DependencyObject const& originHeader)
    {
        if (auto const textElement = element.try_as<winrt::Microsoft::UI::Xaml::Documents::TextElement>())
        {
            element = textElement.ContentStart().VisualParent();
        }
        return element && originHeader &&
            (element == originHeader || SharedHelpers::IsAncestor(element, originHeader, false /* checkVisibility */));
    };
    m_headerCollapseFocusRevoker = winrt::FocusManager::GettingFocus(winrt::auto_revoke,
        [weakThis = get_weak(), weakHeader = winrt::make_weak(header), isWithinHeader](winrt::IInspectable const&, winrt::Microsoft::UI::Xaml::Input::GettingFocusEventArgs const& args)
        {
            // One-shot: only the first move off the hidden header is XAML's rescue.
            if (auto const strongThis = weakThis.get())
            {
                auto const originHeader = weakHeader.get();
                strongThis->m_headerCollapseFocusRevoker.revoke();
                try
                {
                    auto const oldFocus = args.OldFocusedElement();
                    if ((!oldFocus || isWithinHeader(oldFocus, originHeader)) && !strongThis->IsElementInBody(args.NewFocusedElement()))
                    {
                        strongThis->TrySetBodyEntryTarget(args);
                    }
                }
                catch (...)
                {
                    TVDiag::LogRetailF(L"[TableView] Redirecting focus off hidden headers failed.");
                }
            }
        });
    queue.TryEnqueue([weakThis = get_weak(), weakHeader = winrt::make_weak(header), focusState, isWithinHeader]()
        {
            auto const strongThis = weakThis.get();
            if (!strongThis)
            {
                return;
            }
            strongThis->m_headerCollapseFocusRevoker.revoke();
            auto const root = strongThis->XamlRoot();
            if (!root)
            {
                return;
            }
            try
            {
                auto const now = winrt::FocusManager::GetFocusedElement(root).try_as<winrt::DependencyObject>();
                if (!strongThis->ShouldShowColumnHeaders() && (!now || isWithinHeader(now, weakHeader.get())))
                {
                    strongThis->FocusBodyEntryTarget(false /* select */, focusState);
                }
            }
            catch (...)
            {
                TVDiag::LogRetailF(L"[TableView] Moving focus off hidden headers failed.");
            }
        });
}
// Our stamp is a local false; any other local value or binding there now is the app's.
bool TableView::HasHeaderTabStopStampInternal(const winrt::FrameworkElement& header)
{
    auto const columnHeader = header ? header.try_as<winrt::TableViewColumnHeader>() : nullptr;
    auto const headerImpl = columnHeader ? winrt::get_self<TableViewColumnHeader>(columnHeader) : nullptr;
    if (!headerImpl || !headerImpl->OwnsTabStopStampInternal())
    {
        return false;
    }

    auto const local = header.ReadLocalValue(winrt::UIElement::IsTabStopProperty()).try_as<winrt::IReference<bool>>();
    if (local && !local.Value())
    {
        return true;
    }
    headerImpl->SetOwnsTabStopStampInternal(false);
    return false;
}

void TableView::ClearHeaderTabStopStampInternal(const winrt::FrameworkElement& header)
{
    if (HasHeaderTabStopStampInternal(header))
    {
        header.ClearValue(winrt::UIElement::IsTabStopProperty());
        winrt::get_self<TableViewColumnHeader>(header.as<winrt::TableViewColumnHeader>())->SetOwnsTabStopStampInternal(false);
    }
}

// Inactive headers are taken out of the tab order like inactive cells. Only our own stamp is cleared,
// so a style-set or app-local IsTabStop is kept.
void TableView::RefreshHeaderTabStopsInternal()
{
    winrt::DependencyObject focusedInactive{ nullptr };
    for (auto const& cell : GetVisibleHeaderCells(m_headerHost.get()))
    {
        auto const header = cell.try_as<winrt::TableViewColumnHeader>();
        if (!header)
        {
            continue;
        }
        const bool stamped = HasHeaderTabStopStampInternal(cell);
        const bool actionable = IsHeaderCellActionableInternal(cell);
        if (actionable && stamped)
        {
            ClearHeaderTabStopStampInternal(cell);
        }
        else if (!actionable && !stamped && header.IsTabStop() &&
            header.ReadLocalValue(winrt::UIElement::IsTabStopProperty()) == winrt::DependencyProperty::UnsetValue())
        {
            header.IsTabStop(false);
            winrt::get_self<TableViewColumnHeader>(header)->SetOwnsTabStopStampInternal(true);
        }
        if (!actionable && header.FocusState() != winrt::FocusState::Unfocused)
        {
            focusedInactive = header;
        }
    }

    // A focused header that stopped being actionable hands focus to the nearest actionable one, else the body.
    if (auto const queue = focusedInactive ? DispatcherQueue() : nullptr)
    {
        queue.TryEnqueue([weakThis = get_weak(), weakHeader = winrt::make_weak(focusedInactive)]()
            {
                auto const strongThis = weakThis.get();
                auto const root = strongThis ? strongThis->XamlRoot() : nullptr;
                auto const header = weakHeader.get();
                try
                {
                    if (!root || !header || winrt::FocusManager::GetFocusedElement(root) != header)
                    {
                        return;
                    }
                    auto const cells = GetVisibleHeaderCells(strongThis->m_headerHost.get());
                    const int32_t index = IndexOfHeaderCell(cells, header.as<winrt::FrameworkElement>());
                    if (index < 0 || (strongThis->FocusVisibleHeaderFrom(index, 1) < 0 && strongThis->FocusVisibleHeaderFrom(index, -1) < 0))
                    {
                        strongThis->FocusBodyEntryTarget(false /* select */);
                    }
                }
                catch (...)
                {
                    TVDiag::LogRetailF(L"[TableView] Moving focus off an inactive header failed.");
                }
            });
    }
}
bool TableView::TryRedirectTableEntry(const winrt::Microsoft::UI::Xaml::Input::GettingFocusEventArgs& args)
{
    if (ShouldShowColumnHeaders())
    {
        auto const cells = GetVisibleHeaderCells(m_headerHost.get());
        if (const int32_t index = ResolveHeaderEntryIndex(cells); index >= 0)
        {
            winrt::DependencyObject const target = cells[static_cast<size_t>(index)];
            if (target == args.NewFocusedElement() || args.TrySetNewFocusedElement(target))
            {
                NoteFocusEntryRedirect(args.CorrelationId());
                return true;
            }
            return false;
        }
    }

    return TrySetBodyEntryTarget(args);
}

bool TableView::TrySetBodyEntryTarget(const winrt::Microsoft::UI::Xaml::Input::GettingFocusEventArgs& args)
{
    winrt::DependencyObject const selfObject = *this;
    auto const trySet = [&](FocusEntryTarget const& entry)
    {
        if (!entry.Element)
        {
            return false;
        }

        // Only one level of a row is focusable at a time; arm the target's level first. A cell-level target
        // also takes ownership of the armed row so later row-level cleanup disarms the right row.
        auto const rowImpl = entry.Row ? winrt::get_self<TableViewRow>(entry.Row) : nullptr;
        const bool wasCellLevel = rowImpl && rowImpl->IsCellLevelInternal();
        auto const previousOwner = m_cellLevelRow.get();
        const bool previousCursorActive = m_cellCursorActive;
        if (rowImpl)
        {
            if (entry.CellLevel)
            {
                ArmCellLevelRowInternal(entry.Row);
            }
            else
            {
                rowImpl->SetCellLevelInternal(false);
            }
        }

        winrt::DependencyObject const target = entry.Element;
        if (target == args.NewFocusedElement() ||
            (IsEntryRedirectTargetFocusable(entry.Element, selfObject) && args.TrySetNewFocusedElement(target)))
        {
            NoteFocusEntryRedirect(args.CorrelationId());
            if (!entry.CellLevel)
            {
                SetCellCursorActiveInternal(false);
            }
            return true;
        }

        if (rowImpl)
        {
            rowImpl->SetCellLevelInternal(wasCellLevel);
            if (entry.CellLevel)
            {
                if (previousOwner && previousOwner != entry.Row)
                {
                    winrt::get_self<TableViewRow>(previousOwner)->SetCellLevelInternal(true);
                }
                m_cellLevelRow = previousOwner ? winrt::make_weak(previousOwner) : winrt::weak_ref<winrt::TableViewRow>{};
                m_cellCursorActive = previousCursorActive;
            }
        }
        return false;
    };

    const bool previous = args.Direction() == winrt::FocusNavigationDirection::Previous;
    if (trySet(ResolveBodyEntryTarget(previous)))
    {
        return true;
    }

    FocusEntryTarget edge{};
    edge.Element = FindViewportEdgeElementInternal(previous, true /* includeGroupHeaders */, true /* allowNoViewport */);
    edge.Row = edge.Element.try_as<winrt::TableViewRow>();
    return trySet(edge);
}

// The empty-state content when it is shown; else the selected row in view if it was selected after
// the remembered row was recorded (by the app or a right-click); else the remembered row (or its
// actionable remembered cell) in view; else the selected row in view; else the first (last) row or
// group header in view.
TableView::FocusEntryTarget TableView::ResolveBodyEntryTarget(bool last)
{
    FocusEntryTarget result{};
    if (auto const emptyState = m_emptyStatePresenter.get(); emptyState && emptyState.Visibility() == winrt::Visibility::Visible)
    {
        result.Element = last ? winrt::FocusManager::FindLastFocusableElement(emptyState)
                              : winrt::FocusManager::FindFirstFocusableElement(emptyState);
        return result;
    }

    auto const usable = [this](winrt::TableViewRow const& row)
    {
        return row && row.IsEnabled() && IsRowElementInViewport(row);
    };

    winrt::TableViewRow remembered{ nullptr };
    if (auto const rememberedItem = m_currentCellRow >= 0 ? m_currentItem.get() : nullptr)
    {
        winrt::IInspectable itemAtRow{ nullptr };
        remembered =
            TryGetItemAtRowIndex(m_currentCellRow, itemAtRow) && SameInspectableIdentity(itemAtRow, rememberedItem)
            ? GetRealizedRowAt(m_currentCellRow)
            : FindRealizedRowForItem(rememberedItem);
    }

    auto const selected = GetRealizedRowAt(SelectedIndexInternal());
    const bool selectionIsNewer = m_selectionSequence != m_rememberedSelectionSequence;
    result.Row = selectionIsNewer && usable(selected) ? selected : usable(remembered) ? remembered : usable(selected) ? selected : nullptr;
    result.Remembered = result.Row && result.Row == remembered;

    if (result.Row)
    {
        auto const rowImpl = winrt::get_self<TableViewRow>(result.Row);
        const int32_t cellCount = rowImpl->GetVisibleCellCountInternal();
        auto const cell = m_cellCursorActive && cellCount > 0
            ? rowImpl->GetVisibleCellInternal(std::clamp(m_currentCellColumn, 0, cellCount - 1))
            : nullptr;
        result.CellLevel = cell && rowImpl->IsCellActionableInternal(cell);
        result.Element = result.CellLevel ? cell : result.Row;
        return result;
    }

    // A group header first (last) in view is an entry target like a row.
    result.Element = FindViewportEdgeElementInternal(last, true /* includeGroupHeaders */, true /* allowNoViewport */);
    result.Row = result.Element.try_as<winrt::TableViewRow>();
    return result;
}

// Down from the header row (selection follows focus unless Ctrl is held), or a deferred entry.
bool TableView::FocusBodyEntryTarget(bool select, winrt::FocusState focusState)
{
    // A newer selection scrolled out of view is brought back rather than replaced.
    if (const int32_t selectedIndex = SelectedIndexInternal();
        select && selectedIndex >= 0 && m_selectionSequence != m_rememberedSelectionSequence)
    {
        if (auto const selected = GetRealizedRowAt(selectedIndex); !selected || !IsRowElementInViewport(selected))
        {
            auto const root = XamlRoot();
            auto const origin = root ? winrt::FocusManager::GetFocusedElement(root).try_as<winrt::DependencyObject>() : nullptr;
            const bool moved = FocusRow(selectedIndex);
            if (moved && origin && m_pendingReshapeFocusRow >= 0)
            {
                m_pendingBodyFocusOrigin = winrt::make_weak(origin);
            }
            return moved;
        }
    }

    auto const entry = ResolveBodyEntryTarget(false);
    if (!entry.Element)
    {
        return false;
    }

    if (!entry.Row)
    {
        if (auto const link = entry.Element.try_as<winrt::Microsoft::UI::Xaml::Documents::Hyperlink>())
        {
            return link.Focus(focusState);
        }
        auto const element = entry.Element.try_as<winrt::UIElement>();
        return element && (element.try_as<winrt::TableViewGroupHeader>()
            ? FocusRowContainerInternal(element, focusState)
            : element.Focus(focusState));
    }

    // An edge row standing in at cell level takes the actionable cell nearest the column.
    const bool edgeAtCell = m_cellCursorActive && !entry.Remembered && entry.Row != GetRealizedRowAt(SelectedIndexInternal());
    if (!(entry.CellLevel || edgeAtCell ? FocusRealizedRowCell(entry.Row, m_currentCellColumn, focusState) : FocusRowContainerInternal(entry.Row, focusState)))
    {
        return false;
    }

    // An edge row stands in for a selection scrolled out of view; it does not take the selection.
    if (auto const repeater = m_rowsRepeater.get(); repeater && select && (entry.Remembered || SelectedIndexInternal() < 0))
    {
        SelectRowIndexFromKeyboardFocus(repeater.GetElementIndex(entry.Row));
    }
    return true;
}
// Editable means F2 opens an editor: a text column needs a Binding to generate one.
bool TableView::IsColumnCellEditableInternal(const winrt::TableViewColumn& column)
{
    return column && !IsReadOnly() && !column.IsReadOnly() &&
        (column.CellEditingTemplate() || (column.try_as<winrt::TableViewTextColumn>() && column.as<winrt::TableViewTextColumn>().Binding()));
}

// Records the focused header column into the shared cursor for body entry.
void TableView::OnHeaderHostGotFocus(
    const winrt::IInspectable& /*sender*/,
    const winrt::RoutedEventArgs& /*args*/)
{
    if (const int32_t index = GetFocusedVisibleHeaderIndex(); index >= 0)
    {
        // From here on, band entry follows the shared cursor rather than restarting at column 0.
        SetColumnCursorInternal(index);
    }
}

// Bare Left/Right re-asserts from the pre-key header anchor; live focus may already have moved, so
// stepping again would skip a column.
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
    const bool isHome = key == winrt::Windows::System::VirtualKey::Home;
    const bool isEnd = key == winrt::Windows::System::VirtualKey::End;
    if (!isLeft && !isRight && !isHome && !isEnd)
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

    // Prefer the pre-key header anchor; fall back to live focus if the snapshot is stale.
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

    // Home/End reach the first/last actionable header and the scroll edge, never row navigation.
    if (isHome || isEnd)
    {
        const int32_t target = FindActionableHeaderIndex(cells, isHome ? 0 : count - 1, isHome ? 1 : -1);
        if (target >= 0 && target != liveIndex)
        {
            FocusVisibleHeaderFrom(target, 0);
        }
        ScrollBodyHorizontallyInternal(isEnd, true /* toEdge */, target >= 0 ? cells[static_cast<size_t>(target)] : nullptr);
        args.Handled(true);
        return true;
    }

    // RTL mirrors the cell move: Right means "towards the row end" in reading order.
    const bool isRtl = FlowDirection() == winrt::FlowDirection::RightToLeft;
    const bool forward = isRight != isRtl;
    const int32_t step = forward ? 1 : -1;
    const int32_t targetIndex = std::clamp(currentIndex + step, 0, count - 1);

    // Past the last (first) actionable header the row scrolls on, so the remaining columns stay reachable.
    if ((liveIndex == targetIndex || FocusVisibleHeaderFrom(targetIndex, targetIndex == currentIndex ? 0 : step) < 0) &&
        liveIndex == currentIndex)
    {
        ScrollBodyHorizontallyInternal(forward);
    }

    // Consume bounds/refusals so directional focus navigation cannot walk out of the table.
    args.Handled(true);
    return true;
}

// Up stays in the header band; Down leaves it for the body. Either way the key is consumed, so
// focus navigation cannot escape the band sideways or land mid-table.
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

    // The bands share one column cursor, so the body is entered in the header's column.
    if (key == winrt::Windows::System::VirtualKey::Down)
    {
        SetColumnCursorInternal(headerIndex);
        FocusBodyEntryTarget(!IsKeyDown(winrt::VirtualKey::Control));
    }

    args.Handled(true);
    return true;
}

bool TableView::TryMoveFocusToHeaderBandFromBody(int32_t visibleColumnIndex)
{
    auto const host = m_headerHost.get();
    if (!host)
    {
        return false;
    }

    auto const cells = GetVisibleHeaderCells(host);
    if (cells.empty())
    {
        return false;
    }

    // Publish first so ResolveHeaderEntryIndex aims at it; negative (row level) keeps the old cursor.
    SetColumnCursorInternal(visibleColumnIndex);

    const int32_t target = ResolveHeaderEntryIndex(cells);
    if (target < 0)
    {
        return false;
    }

    return FocusVisibleHeaderFrom(target, 0) >= 0;
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

    // Require the header chrome itself; an unhandled Enter from a TextBox in a header template must
    // not toggle sort.
    if (args.OriginalSource().try_as<winrt::DependencyObject>() != headerCell)
    {
        return nullptr;
    }

    return column;
}

// Enter activates on KeyDown; Space follows XAML activation semantics: arm on KeyDown, fire on
// unhandled KeyUp.
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
    m_navKeyHandledEarly = false;
    using winrt::Windows::System::VirtualKey;
    const auto pressed = args.Key();
    const bool isModifier = pressed == VirtualKey::Shift || pressed == VirtualKey::Control || pressed == VirtualKey::Menu ||
        pressed == VirtualKey::LeftWindows || pressed == VirtualKey::RightWindows ||
        pressed == VirtualKey::LeftShift || pressed == VirtualKey::RightShift ||
        pressed == VirtualKey::LeftControl || pressed == VirtualKey::RightControl ||
        pressed == VirtualKey::LeftMenu || pressed == VirtualKey::RightMenu;
    if (!isModifier && pressed != VirtualKey::Up && pressed != VirtualKey::Down)
    {
        m_cellIntentColumn = -1;
    }
    if (!isModifier && pressed != VirtualKey::Left && pressed != VirtualKey::Right)
    {
        m_drillInOffset = -1.0;
    }
    if (!isModifier)
    {
        m_drillRestoreOffset = -1.0;
    }

    // A keyboard move still scrolling to its row is the newest intent, not a stale restore: a
    // repeated key pages on from that row, and any other key leaves the scroll to finish.
    int32_t pendingKeyboardRow = -1;
    if (IsKeyboardBodyFocusPending())
    {
        pendingKeyboardRow = m_pendingReshapeFocusRow;
        if (m_pendingBodyFocusItem && !IsPendingBodyFocusItemAt(pendingKeyboardRow))
        {
            pendingKeyboardRow = FindEntryIndexForDataItem(m_pendingBodyFocusItem);
        }
    }

    switch (args.Key())
    {
    case winrt::Windows::System::VirtualKey::Shift:
    case winrt::Windows::System::VirtualKey::Control:
    case winrt::Windows::System::VirtualKey::Menu:
    case winrt::Windows::System::VirtualKey::LeftWindows:
    case winrt::Windows::System::VirtualKey::RightWindows:
        break;
    default:
        if (!IsKeyboardBodyFocusPending())
        {
            CancelReshapeFocusRestoreForUserInput();
        }
        break;
    }

    // Snapshot before XAML focus navigation; Left/Right only need the cell anchor.
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
        // Record the pre-key body level: cell, row container, or group header.
        m_navAnchorRowContainer = m_navAnchorCellColumn >= 0 ? -1 : GetFocusedRowContainerIndex();
        // Group headers own Left/Right for collapse/expand, not cell drill-in.
        m_navAnchorGroupHeader =
            (m_navAnchorCellColumn >= 0 || m_navAnchorRowContainer >= 0) ? -1 : GetFocusedGroupHeaderIndex();
        // Header navigation needs the same pre-key anchor.
        m_navAnchorHeaderColumn = GetFocusedVisibleHeaderIndex();
        break;
    default:
        m_navAnchorHeaderColumn = -1;
        m_navAnchorRowContainer = -1;
        m_navAnchorGroupHeader = -1;
        break;
    }

    // Live focus can still be on the row a pending keyboard move is leaving; anchor at its target.
    switch (args.Key())
    {
    case winrt::Windows::System::VirtualKey::Up:
    case winrt::Windows::System::VirtualKey::Down:
    case winrt::Windows::System::VirtualKey::Home:
    case winrt::Windows::System::VirtualKey::End:
    case winrt::Windows::System::VirtualKey::PageUp:
    case winrt::Windows::System::VirtualKey::PageDown:
        if (pendingKeyboardRow >= 0 && !IsBodyFocusAtRow(pendingKeyboardRow))
        {
            m_navAnchorRow = pendingKeyboardRow;
            m_navAnchorCellRow = m_pendingReshapeFocusCellLevel ? pendingKeyboardRow : -1;
            m_navAnchorCellColumn = m_pendingReshapeFocusCellLevel ? m_pendingReshapeFocusColumn : -1;
            m_navAnchorRowContainer = -1;
            m_navAnchorGroupHeader = -1;
        }
        break;
    default:
        break;
    }

    // PART_BodyScroller would otherwise page on its own before our handledEventsToo KeyDown runs.
    if ((args.Key() == winrt::Windows::System::VirtualKey::PageUp ||
            args.Key() == winrt::Windows::System::VirtualKey::PageDown) &&
        !args.Handled() &&
        !IsFocusInsideEditingElement() &&
        IsFocusExactlyOnBodyItem())
    {
        args.Handled(true);
    }
}

// Shared by the Preview and bubbling navigation handlers so both agree on who owns a paging key.
bool TableView::IsFocusInsideEditingElement() const
{
    if (!IsEditing())
    {
        return false;
    }

    auto const editingElement = CurrentEditingElement();
    auto const root = XamlRoot();
    if (!editingElement || !root)
    {
        return false;
    }

    // Popup-aware: an open ComboBox editor's dropdown items are not visual descendants of the editor.
    return TableViewDetails::IsWithinElementPopupAware(winrt::FocusManager::GetFocusedElement(root), editingElement, root);
}

void TableView::OnKeyDownForNavigation(
    const winrt::IInspectable& /*sender*/,
    const winrt::KeyRoutedEventArgs& args)
{
    // While editing, let the editor own keys; otherwise PageUp/PageDown can recycle the edited row
    // mid-edit. WPF DataGrid does the same.
    if (IsFocusInsideEditingElement())
    {
        return;
    }

    // After the editing guard so an open editor keeps its arrow keys; Alt+Arrow cannot shadow the
    // bare navigation arrows below.
    if (TryHandleHeaderColumnResizeKey(args))
    {
        return;
    }

    // Header arrows run before sort and row navigation so focus stays inside the header band.
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

    // handledEventsToo recovers keys swallowed by PART_BodyScroller, but not keys consumed by
    // hosted controls.
    if (args.Handled() && !IsFocusExactlyOnBodyItem())
    {
        return;
    }

    const auto key = args.Key();

    // Escape runs after the handled guard: hosted controls such as ComboBox keep their first
    // Escape, and the unhandled second Escape restores grid navigation. Header resize ran above.
    if (TryHandleCellInteractionEscapeKey(args))
    {
        return;
    }

    if (TryHandleCellInteractionNavigationKey(args))
    {
        return;
    }

    // Claim group-header expand/collapse before row drill-in, then cell cursor movement; each owns
    // a different level of the treegrid.
    if (TryHandleGroupHeaderExpandCollapseKey(args))
    {
        return;
    }

    if (TryHandleRowLevelDrillKey(args))
    {
        return;
    }

    if (TryHandleCellInteractionEnterKey(args))
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

    const bool isRowSelectionSpace =
        key == winrt::Windows::System::VirtualKey::Space &&
        !IsKeyDown(winrt::VirtualKey::Menu);
    if (isRowSelectionSpace && CanSelectRows())
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

    // Use the pre-key row anchor; live focus may already have advanced.
    int32_t currentRow = m_navAnchorRow;

    // Ctrl+Arrow moves focus without selection/UIA churn, matching ListViewBase.
    const bool isControlDown = IsKeyDown(winrt::VirtualKey::Control);

    // If relative navigation re-enters without a row anchor, resume from selection instead of
    // yanking focus to row 0. Absolute keys keep their own entry points below.
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
        std::optional<double> initialAlign{};
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
            initialRow = GetPageTargetRow(0, true /* pageDown */, rowCount);
            initialAlign = 1.0;
            break;
        default:
            break;
        }
        if (initialRow >= 0 && FocusRowContainer(initialRow, initialAlign))
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
    // Absolute/page keys are consumed at boundaries; cell-level Up/Down may escape the table.
    bool absoluteRowNav = false;
    std::optional<double> alignRatio{};

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
        newRow = GetPageTargetRow(currentRow, false /* pageDown */, rowCount);
        alignRatio = 0.0;
        absoluteRowNav = true;
        break;
    case winrt::Windows::System::VirtualKey::PageDown:
        newRow = GetPageTargetRow(currentRow, true /* pageDown */, rowCount);
        alignRatio = 1.0;
        absoluteRowNav = true;
        break;
    default:
        consumeKey = false;
        break;
    }

    if (consumeKey)
    {
        // A cell-level Up/Down that landed on a neighbor cell or on the row keeps its column.
        const bool hasIntent = m_cellIntentColumn >= 0 && m_cellIntentRow == currentRow &&
            (m_navAnchorCellRow == m_navAnchorRow || m_navAnchorRowContainer == m_navAnchorRow);
        const bool hasCellAnchor = hasIntent ||
            (m_navAnchorRow >= 0 &&
            m_navAnchorCellRow == m_navAnchorRow &&
            m_navAnchorCellColumn >= 0);
        const int32_t anchorColumn = hasIntent ? m_cellIntentColumn : hasCellAnchor ? m_navAnchorCellColumn : m_currentCellColumn;
        m_cellIntentColumn = -1;
        if (newRow != currentRow)
        {
            // Up/Down preserve the starting level per ARIA treegrid: row-to-row or same-column
            // cell-to-cell.
            bool moved = hasCellAnchor
                ? FocusCell(newRow, anchorColumn, alignRatio)
                : FocusRowContainer(newRow, alignRatio);
            // Row-level Up/Down steps over rows that refuse focus (disabled), as ListView does.
            if (!moved && !hasCellAnchor && !absoluteRowNav)
            {
                const int32_t step = newRow > currentRow ? 1 : -1;
                for (int32_t next = newRow + step; next >= 0 && next < rowCount; next += step)
                {
                    if (FocusRowContainer(next, alignRatio))
                    {
                        newRow = next;
                        moved = true;
                        break;
                    }
                }
            }
            if (moved)
            {
                if (hasCellAnchor && (key == winrt::Windows::System::VirtualKey::Up || key == winrt::Windows::System::VirtualKey::Down))
                {
                    m_cellIntentColumn = anchorColumn;
                    m_cellIntentRow = newRow;
                    SetColumnCursorInternal(anchorColumn);
                }
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
        else if (key == winrt::Windows::System::VirtualKey::Up &&
            currentRow == 0 &&
            TryMoveFocusToHeaderBandFromBody(hasCellAnchor ? anchorColumn : -1))
        {
            args.Handled(true);
        }

        // Unhandled, the body scroller would focus a control inside this row's cells.
        if (!args.Handled() && m_navKeyHandledEarly && !hasCellAnchor &&
            (key == winrt::Windows::System::VirtualKey::Up || key == winrt::Windows::System::VirtualKey::Down))
        {
            args.Handled(true);
        }
    }
}

// Rows forward Left/Right (and row-level Up/Down) before PART_BodyScroller acts on them; the
// handledEventsToo KeyDown then skips the key, since hierarchy Left/Right is not idempotent.
void TableView::HandleBodyKeyFromRowInternal(const winrt::KeyRoutedEventArgs& args)
{
    m_navKeyHandledEarly = true;
    OnKeyDownForNavigation(nullptr, args);
}

bool TableView::FocusRow(int32_t index)
{
    // Follow the current cursor level: row-level callers land on rows; drilled-in callers stay on cells.
    return m_cellCursorActive
        ? FocusCell(index, m_currentCellColumn)
        : FocusRowContainer(index);
}

// ----- Re-shape focus restore (dev-spec Keyboard, "Re-shape while focused") -----
//
// A sort or filter keeps body focus at the same projected row index, whichever record now occupies
// it. This deliberately differs from Tab re-entry (ResolveBodyEntryTarget), which follows the item.
// Restore only if focus was in the body or dropped to null; focus elsewhere is never taken back.
void TableView::CaptureBodyFocusForReshape()
{
    if (m_reshapeFocusValid)
    {
        // Both entry points fire inside one re-shape; the first capture wins.
        return;
    }

    m_reshapeFocusRow = -1;
    m_reshapeFocusColumn = m_currentCellColumn;
    m_reshapeFocusCellLevel = false;

    auto const root = XamlRoot();
    if (!root)
    {
        return;
    }

    auto const focusedElement = winrt::FocusManager::GetFocusedElement(root);
    auto const focused = focusedElement.try_as<winrt::DependencyObject>();

    int32_t row = -1;
    int32_t column = m_currentCellColumn;
    bool cellLevel = false;
    winrt::FocusState focusState = winrt::FocusState::Keyboard;

    if (TryResolveLiveBodyFocusPosition(row, column, cellLevel))
    {
        focusState = GetRestoreFocusStateFor(focusedElement);
    }
    else
    {
        // Focus orphaned in a recycled body container counts as dropped and uses the remembered
        // position.
        if (focused && !IsElementInBody(focused))
        {
            return;
        }

        if (!m_lastBodyFocusValid || m_lastBodyFocusRow < 0)
        {
            return;
        }

        row = m_lastBodyFocusRow;
        column = m_lastBodyFocusColumn;
        cellLevel = m_lastBodyFocusCellLevel;
        focusState = m_lastBodyFocusState;
    }

    m_reshapeFocusRow = row;
    m_reshapeFocusColumn = column;
    m_reshapeFocusCellLevel = cellLevel;
    m_reshapeFocusState = focusState;
    m_reshapeFocusValid = true;

    QueueReshapeFocusCaptureDisarm();
}

bool TableView::IsElementInBody(const winrt::DependencyObject& element) const
{
        if (!element)
        {
            return false;
        }

        auto const repeater = m_rowsRepeater.get();
        if (!repeater)
        {
            return false;
        }

        auto const repeaterObject = repeater.try_as<winrt::DependencyObject>();
        return element == repeaterObject ||
            SharedHelpers::IsAncestor(element, repeaterObject, false /* checkVisibility */);
}

// Null focus is deliberately false: that is what a re-shape leaves behind, not a user move.
bool TableView::IsFocusOnNonBodyElement() const
{
        auto const root = XamlRoot();
        if (!root)
        {
            return false;
        }

        auto const focused = winrt::FocusManager::GetFocusedElement(root).try_as<winrt::DependencyObject>();
        return focused && !IsElementInBody(focused);
}

void TableView::ForgetBodyFocusPosition()
{
        m_lastBodyFocusValid = false;
        m_lastBodyFocusRow = -1;
        m_lastBodyFocusState = winrt::FocusState::Keyboard;
}

// Pointer/app-placed focus is replayed as Programmatic so a restore never paints keyboard visuals.
winrt::FocusState TableView::GetRestoreFocusStateFor(const winrt::IInspectable& focusedElement)
{
        if (auto const control = focusedElement.try_as<winrt::Control>())
        {
            switch (control.FocusState())
            {
            case winrt::FocusState::Pointer:
            case winrt::FocusState::Programmatic:
                return winrt::FocusState::Programmatic;
            default:
                break;
            }
        }

        return winrt::FocusState::Keyboard;
}

bool TableView::TryResolveLiveBodyFocusPosition(int32_t& row, int32_t& column, bool& cellLevel) const
{
    row = -1;
    column = m_currentCellColumn;
    cellLevel = false;

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

    auto const repeaterObject = repeater.try_as<winrt::DependencyObject>();
    if (focused != repeaterObject &&
        !SharedHelpers::IsAncestor(focused, repeaterObject, false /* checkVisibility */))
    {
        return false;
    }

    int32_t cellRow = -1;
    int32_t cellColumn = -1;
    // Non-exact: focus may be on hosted content inside the cell, which is still cell level.
    const bool onCell = TryGetFocusedCell(cellRow, cellColumn, false /* requireExactCell */);

    // GetFocusedRowIndex also recognizes group headers, which occupy projected positions too.
    const int32_t resolvedRow = onCell ? cellRow : GetFocusedRowIndex();
    if (resolvedRow < 0)
    {
        return false;
    }

    row = resolvedRow;
    column = (onCell && cellColumn >= 0) ? cellColumn : m_currentCellColumn;
    cellLevel = onCell;
    return true;
}

// Tracked continuously because ItemsRepeater can recycle the focused row before any re-shape
// notification reaches us.
void TableView::TrackBodyFocusPosition()
{
    if (m_suppressBodyFocusTracking)
    {
        // Inside a re-shape: ItemsRepeater's focus rescue (MoveFocusFromClearedIndex) must not
        // overwrite the user's position.
        return;
    }

    int32_t row = -1;
    int32_t column = m_currentCellColumn;
    bool cellLevel = false;

    if (TryResolveLiveBodyFocusPosition(row, column, cellLevel))
    {
        m_lastBodyFocusValid = true;
        m_lastBodyFocusRow = row;
        m_lastBodyFocusColumn = column;
        m_lastBodyFocusCellLevel = cellLevel;
        if (auto const root = XamlRoot())
        {
            m_lastBodyFocusState = GetRestoreFocusStateFor(winrt::FocusManager::GetFocusedElement(root));
        }
        return;
    }

    if (IsFocusOnNonBodyElement())
    {
        ForgetBodyFocusPosition();
    }
}

void TableView::OnTableViewGotFocusForBodyTracking(
    const winrt::IInspectable& /*sender*/,
    const winrt::RoutedEventArgs& args)
{
    if (auto const source = args.OriginalSource().try_as<winrt::DependencyObject>())
    {
        m_lastFocusedWithin = winrt::make_weak(source);
    }
    TrackBodyFocusPosition();
}

// The remembered body position survives only focus being dropped to null; focus landing on any
// element outside the body discards it, however it got there.
void TableView::OnTableViewLosingFocusForBodyTracking(
    const winrt::IInspectable& /*sender*/,
    const winrt::Microsoft::UI::Xaml::Input::LosingFocusEventArgs& args)
{
    if (m_suppressBodyFocusTracking)
    {
        // The re-shape's own focus rescue; see TrackBodyFocusPosition.
        return;
    }

    auto const newFocus = args.NewFocusedElement();

    if (!newFocus)
    {
        return;
    }

    if (IsElementInBody(newFocus))
    {
        // GotFocus records the new position.
        return;
    }

    ForgetBodyFocusPosition();
}

void TableView::SuppressBodyFocusTrackingForThisTurn()
{
    if (m_suppressBodyFocusTracking)
    {
        return;
    }

    m_suppressBodyFocusTracking = true;

    auto const queue = DispatcherQueue();
    if (!queue)
    {
        m_suppressBodyFocusTracking = false;
        return;
    }

    auto weakThis = get_weak();
    if (!queue.TryEnqueue([weakThis]()
        {
            if (auto strongThis = weakThis.get())
            {
                strongThis->m_suppressBodyFocusTracking = false;
            }
        }))
    {
        m_suppressBodyFocusTracking = false;
    }
}

// An app can raise Reset on its own collection, so a capture not claimed by a restore in the same
// turn is dropped rather than left for an unrelated later re-shape to replay.
void TableView::QueueReshapeFocusCaptureDisarm()
{
    if (m_reshapeFocusDisarmQueued)
    {
        return;
    }

    auto const queue = DispatcherQueue();
    if (!queue)
    {
        return;
    }

    m_reshapeFocusDisarmQueued = true;
    auto weakThis = get_weak();
    if (!queue.TryEnqueue([weakThis]()
        {
            if (auto strongThis = weakThis.get())
            {
                strongThis->m_reshapeFocusDisarmQueued = false;
                strongThis->m_reshapeFocusValid = false;
                strongThis->m_reshapeFocusRow = -1;
            }
        }))
    {
        m_reshapeFocusDisarmQueued = false;
    }
}

void TableView::RestoreBodyFocusAfterReshape()
{
    if (!m_reshapeFocusValid)
    {
        return;
    }

    // Consume synchronously so the queued disarm cannot cancel a claimed restore.
    const int32_t row = m_reshapeFocusRow;
    m_pendingReshapeFocusColumn = m_reshapeFocusColumn;
    m_pendingReshapeFocusCellLevel = m_reshapeFocusCellLevel;
    m_pendingReshapeFocusState = m_reshapeFocusState;

    m_reshapeFocusValid = false;
    m_reshapeFocusRow = -1;

    if (row < 0)
    {
        return;
    }

    m_pendingReshapeFocusRow = row;
    m_pendingReshapeFocusAlign.reset();
    m_pendingBodyFocusItem = nullptr;
    m_pendingBodyFocusFromKeyboard = false;
    m_pendingBodyFocusOrigin = nullptr;

    m_reshapeFocusArmedThisTurn = true;
    if (auto const queue = DispatcherQueue())
    {
        auto weakThis = get_weak();
        if (!queue.TryEnqueue([weakThis]()
            {
                if (auto strongThis = weakThis.get())
                {
                    strongThis->m_reshapeFocusArmedThisTurn = false;
                }
            }))
        {
            m_reshapeFocusArmedThisTurn = false;
        }
    }
    else
    {
        m_reshapeFocusArmedThisTurn = false;
    }
    if (m_pendingReshapeFocusCellLevel)
    {
        SetColumnCursorInternal(m_pendingReshapeFocusColumn);
    }

    // Budgeted retry: ItemsRepeater is still unwinding its Reset, so nothing is realized at the
    // index yet, and deferred re-shape work can drop focus again after a single attempt.
    m_reshapeFocusAttemptsLeft = s_reshapeFocusRestoreAttempts;
    ArmReshapeFocusAttempt();
}

// Attempts run per layout pass AND per dispatcher turn: layout changes whether the target is
// arranged, but only a turn outside layout may force realization (ItemsRepeater refuses
// GetOrCreateElement during layout). Both spend the same budget.
void TableView::ArmReshapeFocusAttempt()
{
    if (m_reshapeFocusLayoutToken.value)
    {
        LayoutUpdated(m_reshapeFocusLayoutToken);
        m_reshapeFocusLayoutToken = {};
    }

    if (m_pendingReshapeFocusRow < 0)
    {
        return;
    }

    auto weakThis = get_weak();
    m_reshapeFocusLayoutToken = LayoutUpdated(
        [weakThis](winrt::IInspectable const&, winrt::IInspectable const&)
        {
            if (auto strongThis = weakThis.get())
            {
                strongThis->OnReshapeFocusAttempt();
            }
        });

    // A body scroll in flight lands through layout; spinning dispatcher turns would only burn the
    // attempt budget before it applies.
    if (m_reshapeFocusTurnQueued || m_reshapeFocusWaitForLayout)
    {
        return;
    }

    if (auto const queue = DispatcherQueue())
    {
        m_reshapeFocusTurnQueued = true;
        if (!queue.TryEnqueue([weakThis]()
            {
                if (auto strongThis = weakThis.get())
                {
                    strongThis->m_reshapeFocusTurnQueued = false;
                    try
                    {
                        strongThis->OnReshapeFocusAttempt();
                    }
                    catch (...)
                    {
                        // Best-effort; never fail-fast the dispatcher.
                    }
                }
            }))
        {
            m_reshapeFocusTurnQueued = false;
        }
    }
}

void TableView::CancelReshapeFocusRestore()
{
    m_pendingReshapeFocusRow = -1;
    m_pendingBodyFocusItem = nullptr;
    m_pendingBodyFocusFromKeyboard = false;
    m_pendingBodyFocusOrigin = nullptr;
    m_requestedBodyVerticalOffset.reset();
    m_reshapeFocusWaitForLayout = false;
    m_reshapeFocusAttemptsLeft = 0;
    if (m_reshapeFocusLayoutToken.value)
    {
        LayoutUpdated(m_reshapeFocusLayoutToken);
        m_reshapeFocusLayoutToken = {};
    }
}

// A restore re-aims at its row until it confirms, which is right against the repeater's own focus
// rescue but wrong against the user: a key or pointer press during that window is a newer intent,
// even when it lands on another body row (which the non-body check in OnReshapeFocusAttempt misses).
// Input in the turn that armed the restore is exempt: that input is what triggered the re-shape
// (a page PreviewKeyDown tunnels ahead of ours; a press-mode control inside the table bubbles first).
void TableView::CancelReshapeFocusRestoreForUserInput()
{
    if (m_pendingReshapeFocusRow >= 0 && !m_reshapeFocusArmedThisTurn)
    {
        CancelReshapeFocusRestore();
    }
}

void TableView::OnReshapeFocusAttempt()
{
    if (m_reshapeFocusLayoutToken.value)
    {
        LayoutUpdated(m_reshapeFocusLayoutToken);
        m_reshapeFocusLayoutToken = {};
    }

    if (m_pendingReshapeFocusRow < 0)
    {
        return;
    }

    if (m_reshapeFocusAttemptsLeft <= 0)
    {
        TVDiag::DbgLogF(
            L"TableView: re-shape focus restore gave up at row %d.", m_pendingReshapeFocusRow);
        CancelReshapeFocusRestore();
        return;
    }
    --m_reshapeFocusAttemptsLeft;

    // Checked before every aim: focus the user or app moved outside the body is never pulled back.
    auto const origin = m_pendingBodyFocusOrigin.get();
    auto const root = origin ? XamlRoot() : nullptr;
    const bool onOrigin = root && winrt::FocusManager::GetFocusedElement(root).try_as<winrt::DependencyObject>() == origin;
    if (!onOrigin)
    {
        // One-way: once focus has left the origin, returning to it is a new user move.
        m_pendingBodyFocusOrigin = nullptr;
    }
    if (!onOrigin && IsFocusOnNonBodyElement())
    {
        CancelReshapeFocusRestore();
        return;
    }

    bool landed = false;
    try
    {
        // Layout callbacks fail-fast on escaping exceptions.
        landed = TryLandReshapeFocus();
    }
    catch (...)
    {
        // Best-effort: the projection can move under a settling restore.
    }

    if (landed)
    {
        CancelReshapeFocusRestore();
        return;
    }

    ArmReshapeFocusAttempt();
}

// Success (focus at the row and the row in view) is confirmed on the next attempt, not inferred
// from Focus(), so a landing that later re-shape work wipes or scrolls away is re-aimed.
bool TableView::TryLandReshapeFocus()
{
    const int32_t rowCount = GetItemsSourceCount();
    if (rowCount <= 0)
    {
        return true;
    }

    auto const repeater = m_rowsRepeater.get();
    if (!repeater)
    {
        return true;
    }

    if (m_pendingBodyFocusItem)
    {
        // Add/Remove/Move above the target shift indices without a Reset; follow the item, and
        // give up once it leaves the projection.
        if (!IsPendingBodyFocusItemAt(m_pendingReshapeFocusRow))
        {
            const int32_t resolved = FindEntryIndexForDataItem(m_pendingBodyFocusItem);
            if (resolved < 0)
            {
                return true;
            }
            m_pendingReshapeFocusRow = resolved;
        }
    }

    // A filter can shorten the projection past the captured position; clamp to stay in the body.
    const int32_t target = std::clamp(m_pendingReshapeFocusRow, 0, rowCount - 1);

    // TryGetElement first: GetOrCreateElement can recycle the container the repeater's rescue just
    // focused, and it throws while a layout pass is in flight.
    auto element = repeater.TryGetElement(target);
    m_reshapeFocusWaitForLayout = false;
    if (!element && m_pendingBodyFocusFromKeyboard)
    {
        // A keyboard move scrolls until the repeater realizes its row; see FocusRowElementInternal.
        if (IsBodyScrollInFlight() || ScrollBodyToIndex(target, m_pendingReshapeFocusAlign))
        {
            m_reshapeFocusWaitForLayout = true;
            return false;
        }
    }

    if (!element)
    {
        try
        {
            element = repeater.GetOrCreateElement(target);
        }
        catch (...)
        {
            return false;
        }

        if (!element)
        {
            return false;
        }
    }

    if (!IsRowElementArranged(element))
    {
        // Realized but not arranged yet; Focus() would be refused.
        m_reshapeFocusWaitForLayout = m_pendingBodyFocusFromKeyboard;
        return false;
    }

    // Compare containers, not indices: a container focused at an estimated position can still report
    // the target index after the repeater realizes another container for it in the viewport.
    const bool focused = IsBodyFocusInContainer(element);
    if (focused && IsRowElementInViewport(element))
    {
        RevealFocusedBodyElement(element);
        return true;
    }

    if (!focused)
    {
        if (m_pendingReshapeFocusCellLevel)
        {
            FocusRealizedRowCell(element, m_pendingReshapeFocusColumn, m_pendingReshapeFocusState);
        }
        else
        {
            FocusRowContainerInternal(element, m_pendingReshapeFocusState);
        }
    }

    if (!IsRowElementInViewport(element))
    {
        const bool scrolling = IsBodyScrollInFlight() || ScrollBodyToRowElement(element, m_pendingReshapeFocusAlign);
        m_reshapeFocusWaitForLayout = scrolling || m_pendingBodyFocusFromKeyboard;
    }
    return false;
}

void TableView::BeginPendingBodyFocus(
    int32_t row, int32_t column, bool cellLevel, std::optional<double> alignRatio, winrt::IInspectable const& item)
{
    m_pendingReshapeFocusRow = row;
    m_pendingReshapeFocusColumn = column;
    m_pendingReshapeFocusCellLevel = cellLevel;
    m_pendingReshapeFocusState = winrt::FocusState::Keyboard;
    m_pendingReshapeFocusAlign = alignRatio;
    // A caller that ran layout passes the item it captured before; the row may now hold another.
    m_pendingBodyFocusItem = item ? item : GetRowsSourceItemAt(row);
    m_pendingBodyFocusFromKeyboard = true;
    m_pendingBodyFocusOrigin = nullptr;
    m_reshapeFocusAttemptsLeft = s_keyboardFocusAttempts;
    ArmReshapeFocusAttempt();
}

bool TableView::IsPendingBodyFocusItemAt(int32_t row) const
{
    if (!m_rowsItemsSourceView || row < 0 || row >= m_rowsItemsSourceView.Count())
    {
        return false;
    }

    auto const target = m_pendingBodyFocusItem.try_as<::IUnknown>();
    auto const candidate = m_rowsItemsSourceView.GetAt(row).try_as<::IUnknown>();
    return target && candidate && target.get() == candidate.get();
}

winrt::IInspectable TableView::GetRowsSourceItemAt(int32_t index) const
{
    if (!m_rowsItemsSourceView || index < 0 || index >= m_rowsItemsSourceView.Count())
    {
        return nullptr;
    }
    return m_rowsItemsSourceView.GetAt(index);
}

// A synchronous layout runs app code (SizeChanged, bindings) that can edit the source or recycle
// the container; false means the element no longer shows expectedItem at index.
bool TableView::ArrangeAndRevalidate(
    winrt::UIElement const& element, int32_t index, winrt::IInspectable const& expectedItem)
{
    auto const repeater = m_rowsRepeater.get();
    auto const view = m_rowsItemsSourceView;
    if (!element || !repeater || index < 0)
    {
        return false;
    }

    try
    {
        element.UpdateLayout();
    }
    catch (...)
    {
        return false;
    }

    if (m_rowsRepeater.get() != repeater || m_rowsItemsSourceView != view ||
        repeater.GetElementIndex(element) != index)
    {
        return false;
    }

    auto const expected = expectedItem.try_as<::IUnknown>();
    auto const current = GetRowsSourceItemAt(index).try_as<::IUnknown>();
    return expected.get() == current.get();
}

bool TableView::IsRowElementArranged(winrt::UIElement const& element)
{
    auto const frameworkElement = element.try_as<winrt::FrameworkElement>();
    if (!frameworkElement ||
        !frameworkElement.IsLoaded() ||
        !(frameworkElement.ActualHeight() > 0.0) ||
        !winrt::VisualTreeHelper::GetParent(frameworkElement))
    {
        return false;
    }

    // ItemsRepeater parks cleared containers at (-10000, -10000); one it re-realizes keeps that
    // offset until its next arrange, and focusing it there strands focus off-screen.
    const auto offset = frameworkElement.ActualOffset();
    return !(offset.x <= -9999.0f && offset.y <= -9999.0f);
}

bool TableView::IsRowElementInViewport(winrt::UIElement const& element) const
{
    auto const frameworkElement = element.try_as<winrt::FrameworkElement>();
    auto const scroller = m_bodyScroller.get();
    if (!frameworkElement)
    {
        return false;
    }
    if (!scroller)
    {
        // No body scroller: let focus's own bring-into-view reach the app's scroller.
        return true;
    }

    const double viewportHeight = scroller.ViewportHeight();
    if (!(viewportHeight > 0.0))
    {
        return false;
    }

    double top = 0.0;
    try
    {
        top = frameworkElement.TransformToVisual(scroller).TransformPoint({ 0.0f, 0.0f }).Y;
    }
    catch (winrt::hresult_error const&)
    {
        return false;
    }

    // Same slack as FindViewportEdgeElementInternal; a row taller than the viewport counts once top-aligned
    // or once it covers the whole viewport (a bottom-aligned PageDown target leaves its top negative).
    constexpr double slack = 0.5;
    const double bottom = top + frameworkElement.ActualHeight();
    return (top >= -slack && (bottom <= viewportHeight + slack || top <= slack)) ||
        (top <= slack && bottom >= viewportHeight - slack);
}

bool TableView::BringRowElementIntoView(winrt::UIElement const& element, std::optional<double> alignRatio)
{
    auto const frameworkElement = element.try_as<winrt::FrameworkElement>();
    if (!frameworkElement)
    {
        return false;
    }

    if (!IsRowElementArranged(element))
    {
        auto const repeater = m_rowsRepeater.get();
        const int32_t index = repeater ? repeater.GetElementIndex(element) : -1;
        if (index < 0 ||
            !ArrangeAndRevalidate(element, index, GetRowsSourceItemAt(index)) ||
            !IsRowElementArranged(element))
        {
            return false;
        }
    }

    if (IsRowElementInViewport(element))
    {
        return true;
    }

    // Scroll PART_BodyScroller directly. A far target sits at the repeater's estimated position,
    // and a bubbling StartBringIntoView hands whatever the body cannot reach yet to the page's
    // ScrollViewer, scrolling the table out of view. The attempt loop re-aims as estimates settle;
    // RevealFocusedBodyElement bubbles once the row is really in view.
    ScrollBodyToRowElement(element, alignRatio);
    return true;
}

bool TableView::ScrollBodyToRowElement(winrt::UIElement const& element, std::optional<double> alignRatio)
{
    auto const frameworkElement = element.try_as<winrt::FrameworkElement>();
    auto const scroller = m_bodyScroller.get();
    auto const repeater = m_rowsRepeater.get();
    if (!frameworkElement || !scroller || !repeater)
    {
        return false;
    }

    const double viewportHeight = scroller.ViewportHeight();
    if (!(viewportHeight > 0.0))
    {
        return false;
    }

    const double offset = scroller.VerticalOffset();
    const double scrollable = std::max(0.0, scroller.ScrollableHeight());
    const int32_t index = repeater.GetElementIndex(element);
    const int32_t count = GetItemsSourceCount();

    double target = offset;
    if (index == 0)
    {
        // The extent edges are exact even while the rows between them are still estimated.
        target = 0.0;
    }
    else if (index > 0 && index == count - 1)
    {
        target = scrollable;
    }
    else
    {
        double top = 0.0;
        try
        {
            top = frameworkElement.TransformToVisual(scroller).TransformPoint({ 0.0f, 0.0f }).Y;
        }
        catch (winrt::hresult_error const&)
        {
            return false;
        }

        const double height = frameworkElement.ActualHeight();
        double delta = 0.0;
        if (height > viewportHeight)
        {
            delta = top;
        }
        else if (alignRatio)
        {
            delta = top - *alignRatio * (viewportHeight - height);
        }
        else if (top < 0.0)
        {
            delta = top;
        }
        else if (top + height > viewportHeight)
        {
            delta = top + height - viewportHeight;
        }
        target = offset + delta;
    }

    target = std::clamp(target, 0.0, scrollable);
    if (std::abs(target - offset) < 0.5)
    {
        return false;
    }

    RequestBodyVerticalOffset(target);
    return true;
}

void TableView::RequestBodyVerticalOffset(double offset)
{
    if (auto const scroller = m_bodyScroller.get())
    {
        m_requestedBodyVerticalOffset = offset;
        scroller.ChangeView(nullptr, offset, nullptr, true /* disableAnimation */);
    }
}

// ChangeView applies on a later tick (cleared by OnBodyScrollerViewChanged); until then positions
// still describe the old offset.
bool TableView::IsBodyScrollInFlight()
{
    auto const scroller = m_bodyScroller.get();
    if (!scroller || !m_requestedBodyVerticalOffset)
    {
        return false;
    }

    if (std::abs(scroller.VerticalOffset() - *m_requestedBodyVerticalOffset) < 0.5)
    {
        m_requestedBodyVerticalOffset.reset();
        return false;
    }
    return true;
}

// For a row the repeater has not realized: estimate from the realized container nearest to it, so
// the repeater realizes the row where the viewport lands. Re-run until the row itself is realized,
// at which point ScrollBodyToRowElement is exact.
bool TableView::ScrollBodyToIndex(int32_t index, std::optional<double> alignRatio)
{
    auto const repeater = m_rowsRepeater.get();
    auto const scroller = m_bodyScroller.get();
    if (!repeater || !scroller)
    {
        return false;
    }

    if (auto const element = repeater.TryGetElement(index); element && IsRowElementArranged(element))
    {
        return ScrollBodyToRowElement(element, alignRatio);
    }

    const double viewportHeight = scroller.ViewportHeight();
    if (!(viewportHeight > 0.0))
    {
        return false;
    }

    const double offset = scroller.VerticalOffset();
    const double scrollable = std::max(0.0, scroller.ScrollableHeight());
    const int32_t count = GetItemsSourceCount();

    double target = offset;
    if (index <= 0)
    {
        target = 0.0;
    }
    else if (index >= count - 1)
    {
        target = scrollable;
    }
    else
    {
        int32_t nearestIndex = -1;
        double nearestTop = 0.0;
        double nearestHeight = 0.0;
        double totalHeight = 0.0;
        int32_t measured = 0;
        const int32_t childCount = winrt::VisualTreeHelper::GetChildrenCount(repeater);
        for (int32_t i = 0; i < childCount; ++i)
        {
            auto const child = winrt::VisualTreeHelper::GetChild(repeater, i).try_as<winrt::FrameworkElement>();
            if (!child)
            {
                continue;
            }

            const int32_t childIndex = repeater.GetElementIndex(child);
            const double height = child.ActualHeight();
            if (childIndex < 0 || !(height > 0.0))
            {
                continue;
            }

            double top = 0.0;
            try
            {
                top = child.TransformToVisual(scroller).TransformPoint({ 0.0f, 0.0f }).Y;
            }
            catch (winrt::hresult_error const&)
            {
                continue;
            }

            totalHeight += height;
            ++measured;
            if (nearestIndex < 0 || std::abs(childIndex - index) < std::abs(nearestIndex - index))
            {
                nearestIndex = childIndex;
                nearestTop = top;
                nearestHeight = height;
            }
        }

        if (measured == 0)
        {
            return false;
        }

        const double averageHeight = totalHeight / measured;
        const double top = index > nearestIndex
            ? nearestTop + nearestHeight + (index - nearestIndex - 1) * averageHeight
            : nearestTop - (nearestIndex - index) * averageHeight;
        double delta = 0.0;
        if (alignRatio && averageHeight <= viewportHeight)
        {
            delta = top - *alignRatio * (viewportHeight - averageHeight);
        }
        else if (top < 0.0 || averageHeight > viewportHeight)
        {
            delta = top;
        }
        else if (top + averageHeight > viewportHeight)
        {
            delta = top + averageHeight - viewportHeight;
        }
        target = offset + delta;
    }

    target = std::clamp(target, 0.0, scrollable);
    if (std::abs(target - offset) < 0.5)
    {
        return false;
    }

    RequestBodyVerticalOffset(target);
    return true;
}

// Once the body shows the row, reveal it in any outer scroller (ListView does the same on focus).
// The request starts at the TableView, not the row: routed through PART_BodyScroller and the
// repeater it would re-anchor the virtualized body.
void TableView::RevealFocusedBodyElement(winrt::UIElement const& element)
{
    auto const root = XamlRoot();
    auto const scroller = m_bodyScroller.get();
    if (!element || !root || !scroller)
    {
        return;
    }

    winrt::FrameworkElement target = element.try_as<winrt::FrameworkElement>();
    if (auto const focused = winrt::FocusManager::GetFocusedElement(root).try_as<winrt::FrameworkElement>();
        focused && SharedHelpers::IsAncestor(focused, element, false /* checkVisibility */))
    {
        target = focused;
    }
    if (!target)
    {
        return;
    }

    try
    {
        // A focus move suppressed while off-viewport skipped the horizontal reveal of its cell.
        const auto inBody = target.TransformToVisual(scroller).TransformPoint({ 0.0f, 0.0f });
        const double width = target.ActualWidth();
        const double viewportWidth = scroller.ViewportWidth();
        if (target != element && viewportWidth > 0.0 && width <= viewportWidth)
        {
            double delta = 0.0;
            if (inBody.X < 0.0)
            {
                delta = inBody.X;
            }
            else if (inBody.X + width > viewportWidth)
            {
                delta = inBody.X + width - viewportWidth;
            }
            if (std::abs(delta) >= 0.5)
            {
                scroller.ChangeView(scroller.HorizontalOffset() + delta, nullptr, nullptr, true /* disableAnimation */);
            }
        }

        const auto origin = target.TransformToVisual(*this).TransformPoint({ 0.0f, 0.0f });
        const double tableHeight = ActualHeight();
        const double top = std::clamp<double>(origin.Y, 0.0, tableHeight);
        const double bottom = std::clamp<double>(origin.Y + target.ActualHeight(), 0.0, tableHeight);
        if (!(bottom > top))
        {
            return;
        }

        const double tableWidth = ActualWidth();
        const double left = std::clamp<double>(origin.X, 0.0, tableWidth);
        const double right = std::clamp<double>(origin.X + target.ActualWidth(), 0.0, tableWidth);
        winrt::BringIntoViewOptions options;
        options.AnimationDesired(false);
        options.TargetRect(winrt::Rect{
            static_cast<float>(left), static_cast<float>(top),
            static_cast<float>(std::max(0.0, right - left)), static_cast<float>(bottom - top) });
        StartBringIntoView(options);
    }
    catch (winrt::hresult_error const&)
    {
    }
}

// GetOrCreateElement makes its element the repeater's layout anchor, and only the repeater's own
// bring-into-view handling releases it (no public API cancels it). Raise a request for the repeater
// through one of its children, and swallow it once the repeater has seen it, so nothing scrolls.
void TableView::ReleaseRowsRepeaterAnchor(winrt::UIElement const& anchor)
{
    auto const repeater = m_rowsRepeater.get();
    if (!repeater || winrt::VisualTreeHelper::GetChildrenCount(repeater) == 0)
    {
        return;
    }

    auto target = anchor;
    if (!target || winrt::VisualTreeHelper::GetParent(target) != repeater)
    {
        target = winrt::VisualTreeHelper::GetChild(repeater, 0).try_as<winrt::UIElement>();
    }
    if (!target)
    {
        return;
    }

    std::erase_if(m_anchorReleaseTargets, [](auto const& pending) { return !pending.get(); });
    m_anchorReleaseTargets.push_back(winrt::make_weak(target));
    winrt::BringIntoViewOptions options;
    options.AnimationDesired(false);
    options.TargetRect(winrt::Rect{});
    target.StartBringIntoView(options);
}

// XAML pads a keyboard-focus bring-into-view by 20 DIPs (or the focus visual margin), so focusing a
// row that is already fully in view would still nudge the body, e.g. a first PageDown would scroll.
// Drop only the vertical padding; the horizontal request still reveals a focused cell.
void TableView::OnBodyBringIntoViewRequested(const winrt::BringIntoViewRequestedEventArgs& args)
{
    // Routed asynchronously, so matched by its target and empty rect.
    if (args.TargetRect().Width == 0.0f && args.TargetRect().Height == 0.0f)
    {
        auto const releaseTarget = args.TargetElement();
        if (auto const it = std::find_if(m_anchorReleaseTargets.begin(), m_anchorReleaseTargets.end(),
                [&releaseTarget](auto const& pending) { return pending.get() == releaseTarget; });
            it != m_anchorReleaseTargets.end())
        {
            m_anchorReleaseTargets.erase(it);
            args.Handled(true);
            return;
        }
    }

    auto const repeater = m_rowsRepeater.get();
    auto const target = args.TargetElement().try_as<winrt::FrameworkElement>();
    if (!repeater || !target)
    {
        return;
    }

    // Our own reveal after a drill-in: a scroll it causes is recorded, not taken as the user's.
    auto noteDrillReveal = wil::scope_exit([this, &args, &target]() noexcept
    {
        auto const scroller = m_bodyScroller.get();
        if (m_drillInOffset < 0.0 || args.Handled() || !scroller)
        {
            return;
        }
        try
        {
            // Only a reveal the scroller can still perform scrolls (padding at an edge does not).
            auto const bounds = target.TransformToVisual(scroller).TransformBounds(args.TargetRect());
            const double offset = scroller.HorizontalOffset();
            m_drillRevealPending = m_drillRevealPending ||
                (bounds.X < -0.5f && offset > 0.5) ||
                (bounds.X + bounds.Width > static_cast<float>(scroller.ViewportWidth()) + 0.5f && offset < scroller.ScrollableWidth() - 0.5);
        }
        catch (...)
        {
            m_drillRevealPending = true;
        }
    });

    winrt::UIElement bodyItem{ nullptr };
    for (winrt::DependencyObject node = target; node; node = winrt::VisualTreeHelper::GetParent(node))
    {
        if (node.try_as<winrt::TableViewRow>() || node.try_as<winrt::TableViewGroupHeader>())
        {
            if (auto const element = node.try_as<winrt::UIElement>();
                winrt::VisualTreeHelper::GetParent(element) == repeater)
            {
                bodyItem = element;
                break;
            }
        }
    }

    if (bodyItem && m_drillRestoreOffset >= 0.0 && target == bodyItem && bodyItem == m_drillRestoreRow.get())
    {
        const double offset = std::exchange(m_drillRestoreOffset, -1.0);
        args.Handled(true);
        if (auto const scroller = m_bodyScroller.get())
        {
            scroller.ChangeView(winrt::IReference<double>{ offset }, nullptr, nullptr, true /* disableAnimation */);
        }
        return;
    }

    if (bodyItem)
    {
        // A group toggle's rescued focus may sit on a cached row or header; revealing it would jump the body.
        if (m_pendingGroupFocusLayoutToken.value &&
            IsBodyFocusInContainer(bodyItem))
        {
            args.Handled(true);
            return;
        }

        // Focus raises its padded request asynchronously. When keys move focus faster than that,
        // the request arrives for a container focus has already left, whose position may now be
        // a recycled or estimated one; honoring it scrolled the body back to the top.
        const auto requested = args.TargetRect();
        const bool focusPadded = requested.Y < 0.0f || requested.Y + requested.Height > target.ActualHeight();
        if (focusPadded && !IsBodyFocusInContainer(bodyItem))
        {
            args.Handled(true);
            return;
        }

        // While the table drives the body to a row it has not reached yet, the request focus raises
        // would bubble the unreachable part of an estimated position to outer scrollers.
        if (IsKeyboardBodyFocusPending() && !IsRowElementInViewport(bodyItem))
        {
            args.Handled(true);
            return;
        }
    }

    if (!std::isnan(args.VerticalAlignmentRatio()) || args.VerticalOffset() != 0.0)
    {
        return;
    }

    auto const root = XamlRoot();
    if (!root || winrt::FocusManager::GetFocusedElement(root) != target)
    {
        return;
    }

    // Only the containers and cells the table focuses itself; hosted content keeps XAML's behavior.
    winrt::UIElement container{ nullptr };
    if (target.try_as<winrt::TableViewRow>() || target.try_as<winrt::TableViewGroupHeader>())
    {
        container = target;
    }
    else if (winrt::VisualTreeHelper::GetParent(target).try_as<winrt::TableViewCellsPanel>())
    {
        for (auto node = winrt::VisualTreeHelper::GetParent(target); node; node = winrt::VisualTreeHelper::GetParent(node))
        {
            if (auto const row = node.try_as<winrt::TableViewRow>())
            {
                container = row;
                break;
            }
        }
    }
    if (!container || winrt::VisualTreeHelper::GetParent(container) != repeater || !IsRowElementInViewport(container))
    {
        return;
    }

    auto rect = args.TargetRect();
    const float height = static_cast<float>(target.ActualHeight());
    if (!(rect.Height > 0.0f) || !(height > 0.0f))
    {
        return;
    }

    const float top = std::max(rect.Y, 0.0f);
    const float bottom = std::min(rect.Y + rect.Height, height);
    if (bottom > top && (top != rect.Y || bottom != rect.Y + rect.Height))
    {
        rect.Y = top;
        rect.Height = bottom - top;
        args.TargetRect(rect);
    }
}

bool TableView::IsInUnrealizedBodyContainer(winrt::DependencyObject const& element) const
{
    auto const repeater = m_rowsRepeater.get();
    winrt::DependencyObject const repeaterObject = repeater;
    winrt::DependencyObject node = element;
    if (auto const textElement = element.try_as<winrt::Microsoft::UI::Xaml::Documents::TextElement>())
    {
        node = textElement.ContentStart().VisualParent();
    }
    while (repeater && node)
    {
        auto const parent = winrt::VisualTreeHelper::GetParent(node);
        if (parent == repeaterObject)
        {
            return repeater.GetElementIndex(node.as<winrt::UIElement>()) < 0;
        }
        node = parent;
    }
    return false;
}

bool TableView::IsBodyFocusInContainer(winrt::UIElement const& container) const
{
    auto const repeater = m_rowsRepeater.get();
    auto const root = XamlRoot();
    if (!container || !repeater || !root)
    {
        return false;
    }

    for (auto node = winrt::FocusManager::GetFocusedElement(root).try_as<winrt::DependencyObject>();
         node;
         node = winrt::VisualTreeHelper::GetParent(node))
    {
        if (node == container)
        {
            return true;
        }
        if (node == repeater)
        {
            return false;
        }
    }
    return false;
}

bool TableView::IsBodyFocusAtRow(int32_t rowIndex) const
{
    if (rowIndex < 0)
    {
        return false;
    }

    int32_t cellRow = -1;
    int32_t cellColumn = -1;
    if (TryGetFocusedCell(cellRow, cellColumn, false /* requireExactCell */))
    {
        return cellRow == rowIndex;
    }

    return GetFocusedRowIndex() == rowIndex;
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

// A refused target can be substituted by one that lands back on a row and re-enters the redirect.
bool TableView::IsEntryRedirectTargetFocusable(winrt::DependencyObject const& targetObject, winrt::DependencyObject const& owner)
{
    // A Hyperlink is checked through its host text element.
    auto const link = targetObject.try_as<winrt::Microsoft::UI::Xaml::Documents::Hyperlink>();
    auto const target = link ? link.ContentStart().VisualParent().try_as<winrt::UIElement>() : targetObject.try_as<winrt::UIElement>();
    if (!target || !target.XamlRoot() || target.Visibility() != winrt::Visibility::Visible || !(link ? link.IsTabStop() : target.IsTabStop()))
    {
        return false;
    }

    if (!SharedHelpers::IsAncestor(target, owner, true /* checkVisibility */))
    {
        return false;
    }

    // Cells are Grids, not Controls, so a disabled ancestor row does not show in the target's own state.
    winrt::DependencyObject current = target;
    while (current)
    {
        if (auto const control = current.try_as<winrt::Control>(); control && !control.IsEnabled())
        {
            return false;
        }
        if (current == owner)
        {
            break;
        }
        current = winrt::VisualTreeHelper::GetParent(current);
    }

    return true;
}

winrt::UIElement TableView::FindViewportEdgeElementInternal(bool last, bool includeGroupHeaders, bool allowNoViewport) const
{
    auto const repeater = m_rowsRepeater.get();
    auto const scroller = m_bodyScroller.get();
    if (!repeater || (!scroller && !allowNoViewport))
    {
        return nullptr;
    }

    const double viewportHeight = scroller ? scroller.ViewportHeight() : 0.0;
    if (scroller && !(viewportHeight > 0.0))
    {
        return nullptr;
    }

    winrt::UIElement fullyVisible{ nullptr };
    int32_t fullyVisibleIndex = -1;
    winrt::UIElement partiallyVisible{ nullptr };
    int32_t partiallyVisibleIndex = -1;
    auto const better = [last](int32_t candidate, int32_t current)
    {
        return current < 0 || (last ? candidate > current : candidate < current);
    };

    const int32_t childCount = winrt::VisualTreeHelper::GetChildrenCount(repeater);
    for (int32_t i = 0; i < childCount; ++i)
    {
        auto const element = winrt::VisualTreeHelper::GetChild(repeater, i).try_as<winrt::Control>();
        const bool isCandidate = element &&
            (element.try_as<winrt::TableViewRow>() || (includeGroupHeaders && element.try_as<winrt::TableViewGroupHeader>()));
        if (!isCandidate || element.Visibility() != winrt::Visibility::Visible || !element.IsEnabled())
        {
            continue;
        }

        // Recycled containers stay parented with index -1.
        const int32_t index = repeater.GetElementIndex(element);
        const double height = element.ActualHeight();
        if (index < 0 || !(height > 0.0))
        {
            continue;
        }

        if (!scroller)
        {
            if (better(index, fullyVisibleIndex))
            {
                fullyVisible = element;
                fullyVisibleIndex = index;
            }
            continue;
        }

        double top = 0.0;
        try
        {
            top = element.TransformToVisual(scroller).TransformPoint({ 0.0f, 0.0f }).Y;
        }
        catch (winrt::hresult_error const&)
        {
            continue;
        }

        // Half a pixel of slack absorbs layout rounding at the viewport edges.
        constexpr double slack = 0.5;
        const double bottom = top + height;
        if (bottom <= slack || top >= viewportHeight - slack)
        {
            continue;
        }

        if (top >= -slack && bottom <= viewportHeight + slack)
        {
            if (better(index, fullyVisibleIndex))
            {
                fullyVisible = element;
                fullyVisibleIndex = index;
            }
        }
        else if (better(index, partiallyVisibleIndex))
        {
            partiallyVisible = element;
            partiallyVisibleIndex = index;
        }
    }

    return fullyVisible ? fullyVisible : partiallyVisible;
}

int32_t TableView::GetPageTargetRow(int32_t currentRow, bool pageDown, int32_t rowCount)
{
    // Flattened indices: a group header at the viewport edge is a page target like a row, so a
    // viewport holding one data row between headers still pages by the viewport.
    int32_t first = -1;
    int32_t last = -1;
    if (auto const repeater = m_rowsRepeater.get())
    {
        if (auto const element = FindViewportEdgeElementInternal(false /* last */, true /* includeGroupHeaders */))
        {
            first = repeater.GetElementIndex(element);
        }
        if (auto const element = FindViewportEdgeElementInternal(true /* last */, true /* includeGroupHeaders */))
        {
            last = repeater.GetElementIndex(element);
        }
    }

    const int32_t step = (first >= 0 && last >= 0)
        ? std::max(1, last - first)
        : std::max(1, GetEstimatedRowsPerPage() - 1);
    if (pageDown)
    {
        return (last >= 0 && currentRow < last) ? last : std::clamp(currentRow + step, 0, rowCount - 1);
    }
    return (first >= 0 && currentRow > first) ? first : std::clamp(currentRow - step, 0, rowCount - 1);
}

// Excludes nested TableViews and hosted content: hosted TextBox/ComboBox controls keep the keys they claim.
bool TableView::IsFocusExactlyOnBodyItem() const
{
    auto const root = XamlRoot();
    if (!root)
    {
        return false;
    }

    auto const focused = winrt::FocusManager::GetFocusedElement(root).try_as<winrt::UIElement>();
    if (!focused)
    {
        return false;
    }

    if (focused.try_as<winrt::TableViewRow>() || focused.try_as<winrt::TableViewGroupHeader>())
    {
        auto const repeater = m_rowsRepeater.get();
        return repeater && repeater.GetElementIndex(focused) >= 0;
    }

    int32_t focusedRow = -1;
    int32_t focusedColumn = -1;
    return TryGetFocusedCell(focusedRow, focusedColumn, true /* requireExactCell */);
}

void TableView::ArmCellLevelRowInternal(winrt::TableViewRow const& row)
{
    // Release any other drilled-in row first so only one row ever has its cells armed.
    if (!row)
    {
        return;
    }

    if (auto const previous = m_cellLevelRow.get(); previous && previous != row)
    {
        winrt::get_self<TableViewRow>(previous)->SetCellLevelInternal(false);
    }
    if (auto const rowImpl = winrt::get_self<TableViewRow>(row); !rowImpl->IsCellLevelInternal())
    {
        rowImpl->SetCellLevelInternal(true);
    }
    m_cellLevelRow = winrt::make_weak(row);
    m_cellCursorActive = true;
}

// Popping out of cell level must restore the drilled row to row level, or that row disappears from
// Tab navigation.
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
    if (rowIndex != m_cellIntentRow)
    {
        m_cellIntentColumn = -1;
    }

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
            m_rememberedSelectionSequence = m_selectionSequence;
            winrt::IInspectable item{ nullptr };
            SetCurrentItem(TryGetItemAtRowIndex(rowIndex, item) ? item : nullptr);
        }
        SetCellCursorActiveInternal(false);
        return;
    }

    const int32_t columnIndex = rowImpl->GetVisibleCellIndexInternal(focusedCell);
    if (m_cellIntentColumn >= 0 && rowIndex == m_cellIntentRow)
    {
        SetColumnCursorInternal(m_cellIntentColumn);
    }
    else if (columnIndex >= 0)
    {
        // A cell actually took focus, so the user is in a column. Establishes the shared cursor for
        // the header band too - Shift+Tab back up should land on the column being worked in.
        SetColumnCursorInternal(columnIndex);
    }

    if (rowIndex >= 0)
    {
        m_currentCellRow = rowIndex;
        m_rememberedSelectionSequence = m_selectionSequence;
    }

    ArmCellLevelRowInternal(row);

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

    auto const focused = GetFocusedVisual(root);
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

bool TableView::FocusCell(int32_t rowIndex, int32_t visibleColumnIndex, std::optional<double> alignRatio)
{
    if (visibleColumnIndex >= 0)
    {
        // An explicit column names the column the caller wants the cursor on, so it establishes it.
        SetColumnCursorInternal(visibleColumnIndex);
    }

    return FocusRowElementInternal(rowIndex, m_currentCellColumn, true /* cellLevel */, alignRatio);
}

// Single writer for the shared column cursor; setting it also marks that a real column was chosen.
void TableView::SetColumnCursorInternal(int32_t visibleColumnIndex)
{
    if (visibleColumnIndex < 0)
    {
        return;
    }

    m_currentCellColumn = visibleColumnIndex;
    m_columnCursorEstablished = true;
}

// Reset to "no column chosen" so the next band entry starts at the first focusable header.
void TableView::ResetColumnCursorInternal()
{
    m_currentCellColumn = 0;
    m_columnCursorEstablished = false;
}

bool TableView::FocusRowContainer(int32_t rowIndex, std::optional<double> alignRatio)
{
    // Row level keeps the remembered column untouched for the next drill-in.
    return FocusRowElementInternal(rowIndex, m_currentCellColumn, false /* cellLevel */, alignRatio);
}

// Shared realization path; a row that cannot be arranged yet finishes in the re-shape attempt loop.
bool TableView::FocusRowElementInternal(
    int32_t rowIndex, int32_t targetColumn, bool cellLevel, std::optional<double> alignRatio)
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

    // This request is newer intent than any keyboard or re-shape focus still settling.
    CancelReshapeFocusRestore();

    // A row the repeater has not realized is scrolled to first and focused once realized.
    // GetOrCreateElement would make it the repeater's anchor at an estimated position, and repeated
    // far jumps (key-repeat PageDown) left focus on such a container after the repeater re-indexed
    // it off-screen.
    auto element = repeater.TryGetElement(rowIndex);
    if (!element)
    {
        if (ScrollBodyToIndex(rowIndex, alignRatio))
        {
            BeginPendingBodyFocus(rowIndex, targetColumn, cellLevel, alignRatio);
            return true;
        }
        element = repeater.GetOrCreateElement(rowIndex);
    }
    if (!element || !element.try_as<winrt::Control>())
    {
        return false;
    }

    if (!IsRowElementArranged(element))
    {
        auto const expectedItem = GetRowsSourceItemAt(rowIndex);
        if (!ArrangeAndRevalidate(element, rowIndex, expectedItem))
        {
            BeginPendingBodyFocus(rowIndex, targetColumn, cellLevel, alignRatio, expectedItem);
            return true;
        }
    }

    if (IsRowElementArranged(element))
    {
        const bool focused = cellLevel
            ? FocusRealizedRowCell(element, targetColumn)
            : FocusRowContainerInternal(element);
        if (!focused)
        {
            return false;
        }

        if (IsRowElementInViewport(element))
        {
            return true;
        }
        BringRowElementIntoView(element, alignRatio);
    }

    BeginPendingBodyFocus(rowIndex, targetColumn, cellLevel, alignRatio);
    return true;
}

// Focuses an already-realized container at row level; group headers have no cell level.
bool TableView::FocusRowContainerInternal(winrt::UIElement const& element, winrt::FocusState focusState)
{
    auto const row = element.try_as<winrt::TableViewRow>();
    if (row)
    {
        // Make the row focusable before clearing the focused cell from tab order.
        winrt::get_self<TableViewRow>(row)->EnableRowFocusInternal();
    }

    auto const control = element.try_as<winrt::Control>();
    if (!control || !control.Focus(focusState))
    {
        return false;
    }

    // Focus has landed on the row; now cells can leave the tab order.
    if (row)
    {
        winrt::get_self<TableViewRow>(row)->SetCellLevelInternal(false);
    }
    SetCellCursorActiveInternal(false);
    return true;
}

// Focuses a visible cell in an already-realized row; group headers stay at row level.
bool TableView::FocusRealizedRowCell(
    winrt::UIElement const& element,
    int32_t visibleColumnIndex,
    winrt::FocusState focusState)
{
    if (auto const row = element.try_as<winrt::TableViewRow>())
    {
        auto const rowImpl = winrt::get_self<TableViewRow>(row);
        const int32_t cellCount = rowImpl->GetVisibleCellCountInternal();
        if (cellCount > 0)
        {
            // The nearest actionable cell to the requested column; ties go forward.
            const int32_t requested = std::clamp(visibleColumnIndex, 0, cellCount - 1);
            const int32_t after = rowImpl->FindActionableCellInternal(requested, 1);
            const int32_t before = rowImpl->FindActionableCellInternal(requested - 1, -1);
            const int32_t column = (after >= 0 && (before < 0 || after - requested <= requested - before)) ? after : before;
            if (column >= 0 && rowImpl->FocusVisibleCellInternal(column, focusState))
            {
                return true;
            }
        }

        // The row has no focusable cell (no columns, or every column collapsed). Row level is the
        // only level left, and it is a legitimate resting place in the two-level model.
        return FocusRowContainerInternal(element, focusState);
    }

    // A group header: never a cell-level target.
    return FocusRowContainerInternal(element, focusState);
}

// Group headers use treegrid Left/Right expand/collapse, not row drill-in. This handledEventsToo
// backstop is safe because directional expansion is idempotent.
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

    // Use the pre-key group-header anchor; live focus may already have moved.
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

    // Consume even when nothing changes so focus navigation cannot walk sideways out of the body.
    args.Handled(true);

    if (!header.IsExpandable())
    {
        return true;
    }

    // RTL mirrors the arrows the same way every other band does: "Right" is towards the row end in
    // reading order, and expanding is the reading-order-forward direction.
    const bool isRtl = FlowDirection() == winrt::FlowDirection::RightToLeft;
    const bool expand = isRight != isRtl;

    // Avoid queuing a reshape/focus-restore round trip for repeated no-op arrows.
    if (header.IsExpanded() == expand)
    {
        return true;
    }

    winrt::get_self<TableViewGroupHeader>(header)->SetGestureFocusStateInternal(winrt::FocusState::Keyboard);
    SetGroupExpansion(header, expand);
    return true;
}

// Exact focused group-header index; hosted focusable content keeps the arrow keys it claims.
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

bool TableView::TryHandleCellInteractionEnterKey(const winrt::KeyRoutedEventArgs& args)
{
    if (args.Handled() ||
        args.Key() != winrt::Windows::System::VirtualKey::Enter ||
        IsEditing() ||
        IsKeyDown(winrt::VirtualKey::Menu) ||
        IsKeyDown(winrt::VirtualKey::Control) ||
        IsKeyDown(winrt::VirtualKey::Shift))
    {
        return false;
    }

    int32_t rowIndex = -1;
    int32_t columnIndex = -1;
    if (!TryGetFocusedCell(rowIndex, columnIndex, true /* requireExactCell */))
    {
        return false;
    }

    auto const row = GetRealizedRowAt(rowIndex);
    if (!row)
    {
        return false;
    }

    // Enter reaches focusable display content, not the editor: F2 owns editing, while template
    // columns need Enter to reach controls such as ComboBox/CheckBox inside the single tab stop.
    auto const cell = winrt::get_self<TableViewRow>(row)->GetVisibleCellInternal(columnIndex);
    auto const cellElement = cell.try_as<winrt::FrameworkElement>();
    if (!cellElement)
    {
        return false;
    }

    auto const target = FindFirstFocusableDescendant(cell);
    auto const link = target ? nullptr : FindFirstHyperlinkDescendant(cell, 8);
    if (target ? !target.Focus(winrt::FocusState::Keyboard) : !(link && link.Focus(winrt::FocusState::Keyboard)))
    {
        return false;
    }

    SetColumnCursorInternal(columnIndex);
    SetCellCursorActiveInternal(true);
    m_cellInteractionActive = true;
    m_cellInteractionCell = winrt::make_weak(cellElement);
    args.Handled(true);
    return true;
}

bool TableView::TryHandleCellInteractionNavigationKey(const winrt::KeyRoutedEventArgs& args)
{
    if (args.Handled() ||
        !m_cellInteractionActive ||
        IsEditing() ||
        IsKeyDown(winrt::VirtualKey::Menu) ||
        IsKeyDown(winrt::VirtualKey::Control))
    {
        return false;
    }

    switch (args.Key())
    {
    case winrt::Windows::System::VirtualKey::Left:
    case winrt::Windows::System::VirtualKey::Right:
    case winrt::Windows::System::VirtualKey::Up:
    case winrt::Windows::System::VirtualKey::Down:
    case winrt::Windows::System::VirtualKey::Home:
    case winrt::Windows::System::VirtualKey::End:
    case winrt::Windows::System::VirtualKey::PageUp:
    case winrt::Windows::System::VirtualKey::PageDown:
        break;
    default:
        return false;
    }

    auto const enteredCell = m_cellInteractionCell.get();
    auto const root = XamlRoot();
    auto const focused = root ? GetFocusedVisual(root).try_as<winrt::UIElement>() : nullptr;
    if (!enteredCell ||
        !focused ||
        (focused != enteredCell &&
            !SharedHelpers::IsAncestor(focused, enteredCell, false /* checkVisibility */)))
    {
        m_cellInteractionActive = false;
        m_cellInteractionCell = nullptr;
        return false;
    }

    int32_t rowIndex = -1;
    int32_t columnIndex = -1;
    if (!TryGetFocusedCell(rowIndex, columnIndex, false /* requireExactCell */))
    {
        m_cellInteractionActive = false;
        m_cellInteractionCell = nullptr;
        return false;
    }

    int32_t exactRow = -1;
    int32_t exactColumn = -1;
    if (TryGetFocusedCell(exactRow, exactColumn, true /* requireExactCell */))
    {
        m_cellInteractionActive = false;
        m_cellInteractionCell = nullptr;
        return false;
    }

    // The hosted control did not claim this key - args.Handled() is still false - so without this
    // XAML's directional navigation takes it and moves focus to whatever is geometrically nearest.
    // That walks OUT of the cell and lands unpredictably: from a CheckBox in column 7, Right
    // skipped the ComboBox in column 8 entirely and landed on the date editor in column 9, because
    // the search is by position, not by column. Interaction mode means the cell's content owns the
    // arrows; Escape is the way back to grid navigation, exactly as the ARIA grid pattern requires.
    args.Handled(true);
    return true;
}

bool TableView::TryHandleCellInteractionEscapeKey(const winrt::KeyRoutedEventArgs& args)
{
    if (args.Handled() ||
        args.Key() != winrt::Windows::System::VirtualKey::Escape ||
        !m_cellInteractionActive ||
        IsEditing())
    {
        return false;
    }

    auto const enteredCell = m_cellInteractionCell.get();
    auto const root = XamlRoot();
    auto const focused = root ? GetFocusedVisual(root).try_as<winrt::UIElement>() : nullptr;
    if (!enteredCell ||
        !focused ||
        (focused != enteredCell &&
            !SharedHelpers::IsAncestor(focused, enteredCell, false /* checkVisibility */)))
    {
        m_cellInteractionActive = false;
        m_cellInteractionCell = nullptr;
        return false;
    }

    int32_t rowIndex = -1;
    int32_t columnIndex = -1;
    if (!TryGetFocusedCell(rowIndex, columnIndex, false /* requireExactCell */))
    {
        m_cellInteractionActive = false;
        m_cellInteractionCell = nullptr;
        return false;
    }

    int32_t exactRow = -1;
    int32_t exactColumn = -1;
    if (TryGetFocusedCell(exactRow, exactColumn, true /* requireExactCell */))
    {
        m_cellInteractionActive = false;
        m_cellInteractionCell = nullptr;
        return false;
    }

    SetColumnCursorInternal(columnIndex);
    if (!FocusCell(rowIndex, columnIndex))
    {
        return false;
    }

    int32_t focusedRow = -1;
    int32_t focusedColumn = -1;
    if (!TryGetFocusedCell(focusedRow, focusedColumn, true /* requireExactCell */) ||
        focusedRow != rowIndex ||
        focusedColumn != columnIndex)
    {
        return false;
    }

    SetCellCursorActiveInternal(true);
    m_cellInteractionActive = false;
    m_cellInteractionCell = nullptr;
    args.Handled(true);
    return true;
}

// In the ARIA treegrid model, Right drills from row to first cell and Left from first cell pops
// back to row; group headers own expand/collapse above.
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

    // Use the pre-key row anchor; live focus may already have moved out of the row.

    // Focus was on a row container.
    if (m_navAnchorRowContainer >= 0)
    {
        auto const row = GetRealizedRowAt(m_navAnchorRowContainer);
        if (!row)
        {
            // Consume Left so directional focus navigation cannot walk out of the table.
            if (!drillIn)
            {
                args.Handled(true);
                return true;
            }
            return false;
        }

        // Treegrid level: forward expands a collapsed node else drills in; backward collapses an
        // expanded node else moves to the parent. Shift chords keep the plain drill.
        if (row.Level() > 0 && !IsKeyDown(winrt::VirtualKey::Shift))
        {
            if (drillIn && row.IsExpandable() && !row.IsExpanded())
            {
                SetGroupExpansion(row, true);
                args.Handled(true);
                return true;
            }

            if (!drillIn)
            {
                if (row.IsExpandable() && row.IsExpanded())
                {
                    SetGroupExpansion(row, false);
                }
                else if (!TryFocusParentRow(row))
                {
                    ScrollBodyHorizontallyInternal(false);
                }

                // Consumed even at a root, for the same reason as the flat row below.
                args.Handled(true);
                return true;
            }
        }

        if (!drillIn)
        {
            // Nothing further out at row level. Consumed anyway: an unconsumed Left here reaches
            // directional focus navigation, which would walk focus sideways out of the table.
            ScrollBodyHorizontallyInternal(false);
            args.Handled(true);
            return true;
        }

        auto const rowImpl = winrt::get_self<TableViewRow>(row);
        const int32_t cellCount = rowImpl->GetVisibleCellCountInternal();
        if (cellCount <= 0)
        {
            // A row with no visible columns has no cell level to drill into.
            args.Handled(true);
            return true;
        }

        // Enter at the first actionable cell, not the remembered column, so Right and Left stay
        // inverses: Left on the first actionable cell pops back to the row. Bringing it into view
        // undoes any horizontal scroll. Without one, Right scrolls on.
        if (const int32_t first = rowImpl->FindActionableCellInternal(0, 1); first >= 0)
        {
            // Remembered so Left can undo the bring-into-view in one step.
            auto const scroller = m_bodyScroller.get();
            m_drillInOffset = scroller ? scroller.HorizontalOffset() : -1.0;
            m_drillSettledOffset = m_drillInOffset;
            m_drillRevealPending = false;
            m_drillInRow = m_navAnchorRowContainer;
            rowImpl->FocusVisibleCellInternal(first, winrt::FocusState::Keyboard);
        }
        else
        {
            ScrollBodyHorizontallyInternal(true);
        }
        args.Handled(true);
        return true;
    }

    // Focus was on a cell: only the first actionable cell pops out; other cells leave the key to
    // cell navigation.
    auto const anchorRow = GetRealizedRowAt(m_navAnchorCellRow);
    if (drillIn || !anchorRow || m_navAnchorCellColumn < 0 ||
        winrt::get_self<TableViewRow>(anchorRow)->FindActionableCellInternal(m_navAnchorCellColumn - 1, -1) >= 0)
    {
        return false;
    }

    // Right and Left stay inverses across a horizontal scroll: the drill-in's bring-into-view is undone
    // in one step, else the body steps back while the cell stays in view.
    auto const scroller = m_bodyScroller.get();
    // A scroll since our own reveals settled was the user's (or the app's): step back through it instead.
    const bool scrolledSinceDrill = scroller && !m_drillRevealPending &&
        std::abs(scroller.HorizontalOffset() - m_drillSettledOffset) > 8.0;
    const double restoreOffset = m_drillInRow == m_navAnchorCellRow && !scrolledSinceDrill ? m_drillInOffset : -1.0;
    if (restoreOffset < 0.0 &&
        ScrollBodyHorizontallyInternal(false, false, winrt::get_self<TableViewRow>(anchorRow)->GetVisibleCellInternal(m_navAnchorCellColumn)))
    {
        args.Handled(true);
        return true;
    }
    m_drillInOffset = -1.0;
    if (scroller && restoreOffset >= 0.0)
    {
        m_drillRestoreRow = winrt::make_weak(anchorRow.as<winrt::UIElement>());
        m_drillRestoreOffset = restoreOffset;
    }
    if (!FocusRowContainer(m_navAnchorCellRow))
    {
        m_drillRestoreOffset = -1.0;
        // The row refused focus; let the cell cursor have the key rather than swallowing it.
        return false;
    }

    // Applied again in place of the row's focus bring-into-view (OnBodyBringIntoViewRequested).
    if (scroller && restoreOffset >= 0.0)
    {
        scroller.ChangeView(winrt::IReference<double>{ restoreOffset }, nullptr, nullptr, true /* disableAnimation */);
    }
    args.Handled(true);
    return true;
}

// Reading-order forward or back by one step (or to the edge), clamped to the scrollable range. Not
// animated, so auto-repeat steps from the offset the previous key reached.
bool TableView::ScrollBodyHorizontallyInternal(bool forward, bool toEdge, winrt::UIElement const& keepInView)
{
    auto const scroller = m_bodyScroller.get();
    if (!scroller || (forward ? scroller.HorizontalOffset() >= scroller.ScrollableWidth() - 0.5 : scroller.HorizontalOffset() <= 0.5))
    {
        return false;
    }

    // A few pixels left by a focus bring-into-view count as the start: settle there, but report no
    // scroll so Left still returns to the row.
    constexpr double slack = 8.0;
    if (!forward && !toEdge && scroller.HorizontalOffset() <= slack)
    {
        scroller.ChangeView(winrt::IReference<double>{ 0.0 }, nullptr, nullptr, true /* disableAnimation */);
        return false;
    }

    constexpr double step = 48.0;
    const double offset = scroller.HorizontalOffset();
    double target = toEdge ? (forward ? scroller.ScrollableWidth() : 0.0)
        : std::clamp(offset + (forward ? step : -step), 0.0, scroller.ScrollableWidth());

    // A focused non-frozen element stays in view: an edge scroll stops short, and a step is refused.
    auto const element = keepInView.try_as<winrt::FrameworkElement>();
    auto const column = element ? element.Tag().try_as<winrt::TableViewColumn>() : nullptr;
    if (element && !(column && column.FrozenEdge() != winrt::TableViewFrozenEdge::None))
    {
        auto const bounds = element.TransformToVisual(scroller).TransformBounds({ 0.0f, 0.0f, static_cast<float>(element.ActualWidth()), 0.0f });
        const double left = offset + bounds.X;
        // The leading frozen band covers the start of the viewport.
        const double inView = forward ? std::min(target, std::max(0.0, left - ComputeLeadingFrozenWidth()))
            : std::max(target, left + bounds.Width - scroller.ViewportWidth());
        if (!toEdge && inView != target)
        {
            return false;
        }
        target = inView;
    }
    m_drillInOffset = -1.0;
    return scroller.ChangeView(winrt::IReference<double>{ target }, nullptr, nullptr, true /* disableAnimation */);
}

// Exact focused row-container index; group headers and cells own different navigation levels.
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

// Cell-level navigation follows WPF/ListView: Left/Right/Home/End do not wrap, Ctrl+Home/End jump
// grid-wide, and selection stays row-level. Re-assert from the pre-key anchor because XAML focus
// navigation may already have moved live focus.
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
        // Ctrl moves the cursor without dragging selection/UIA events through every row.
        FocusCell(targetRow, isHome ? 0 : c_lastColumnSentinel);
        return true;
    }

    // Only actionable cells take focus, so moves skip inactive ones and stop at the last actionable.
    auto const rowImpl = winrt::get_self<TableViewRow>(row);
    const bool isRtl = FlowDirection() == winrt::FlowDirection::RightToLeft;
    const int32_t step = (isRight != isRtl) ? 1 : -1;
    int32_t targetColumn =
        isHome ? rowImpl->FindActionableCellInternal(0, 1) :
        isEnd ? rowImpl->FindActionableCellInternal(cellCount - 1, -1) :
        rowImpl->FindActionableCellInternal(currentColumn + step, step);
    if (targetColumn < 0)
    {
        targetColumn = currentColumn;
    }

    const bool atBound = targetColumn == currentColumn;
    const bool alreadyOnTarget = onLiveCell && liveRow == currentRow && liveColumn == targetColumn;

    bool moved = alreadyOnTarget;
    if (!moved)
    {
        winrt::get_self<TableViewRow>(row)->FocusVisibleCellInternal(
            targetColumn, winrt::FocusState::Keyboard);

        // FocusVisibleCellInternal can fall back to the row, so read focus back; non-exact keeps
        // hosted cell content as a hit.
        int32_t movedRow = -1;
        int32_t movedColumn = -1;
        moved =
            TryGetFocusedCell(movedRow, movedColumn, false /* requireExactCell */) &&
            movedRow == currentRow && movedColumn == targetColumn;
    }

    if (moved)
    {
        SetColumnCursorInternal(targetColumn);
        // Home/End reach the edge, and past the last (first) actionable cell the body scrolls on.
        if (isHome || isEnd || atBound)
        {
            ScrollBodyHorizontallyInternal(isHome ? false : isEnd ? true : step > 0, isHome || isEnd,
                isHome || isEnd ? rowImpl->GetVisibleCellInternal(targetColumn) : nullptr);
        }
        return true;
    }

    // If focus refuses the target, keep the cursor on real focus. Still consume bounds and Home/End
    // so focus navigation or row navigation cannot reinterpret them.
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
