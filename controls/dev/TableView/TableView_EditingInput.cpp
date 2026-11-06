// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewColumn.h"
#include "TableViewRow.h"
#include "TableViewFocusHelpers.h"
#include "TableViewAutomationHelpers.h"
#include "TableViewSource.h"
#include "TVDiag.h"

#include <algorithm>
#include <cmath>
#include <vector>

// Input gestures that drive editing.
//
// Kept separate from TableView_Editing.cpp: that file owns the edit state machine, reachable
// entirely through BeginEdit/CommitEdit/CancelEdit; this one owns the policy of which gestures map
// onto that API. The split lets a future keyboard or selection layer re-route gestures without
// reopening the state machine, and keeps the state machine testable without synthesizing input.

namespace
{
    // True when `candidate` is `ancestor` or sits underneath it, including the ancestor's own
    // popups (a ComboBox editor's dropdown items).
    bool IsWithinElement(winrt::IInspectable const& candidate, winrt::FrameworkElement const& ancestor)
    {
        if (!ancestor)
        {
            return false;
        }

        return TableViewDetails::IsWithinElementPopupAware(candidate, ancestor, ancestor.XamlRoot());
    }

    // An open flyout owned by the editor or one of its descendants: a ContextFlyout, or a text
    // control's SelectionFlyout. Its popup is not in the visual tree, so focus inside it is not
    // within the editor.
    winrt::FlyoutBase FindOpenEditorFlyout(winrt::DependencyObject const& element)
    {
        if (auto const uiElement = element.try_as<winrt::UIElement>())
        {
            if (auto const flyout = uiElement.ContextFlyout(); flyout && flyout.IsOpen())
            {
                return flyout;
            }
        }

        winrt::FlyoutBase selectionFlyout{ nullptr };
        if (auto const textBox = element.try_as<winrt::TextBox>())
        {
            selectionFlyout = textBox.SelectionFlyout();
        }
        else if (auto const richEditBox = element.try_as<winrt::RichEditBox>())
        {
            selectionFlyout = richEditBox.SelectionFlyout();
        }
        if (selectionFlyout && selectionFlyout.IsOpen())
        {
            return selectionFlyout;
        }

        const int32_t count = winrt::VisualTreeHelper::GetChildrenCount(element);
        for (int32_t i = 0; i < count; ++i)
        {
            if (auto const flyout = FindOpenEditorFlyout(winrt::VisualTreeHelper::GetChild(element, i)))
            {
                return flyout;
            }
        }

        return nullptr;
    }

    // The open popup that is or holds `element`. Flyouts that are not reachable from the editor's
    // properties (a DatePicker's picker, a Button.Flyout, an attached flyout) surface only here.
    // While a flyout opens, focus can rest on its Popup before reaching the content.
    winrt::Popup FindOpenPopupContaining(winrt::IInspectable const& element, winrt::XamlRoot const& root)
    {
        if (!element || !root)
        {
            return nullptr;
        }

        auto const elementAsPopup = element.try_as<winrt::Popup>();
        for (auto const& popup : winrt::VisualTreeHelper::GetOpenPopupsForXamlRoot(root))
        {
            if (popup == elementAsPopup)
            {
                return popup;
            }
            if (auto const child = popup.Child().try_as<winrt::FrameworkElement>(); child && IsWithinElement(element, child))
            {
                return popup;
            }
        }

        return nullptr;
    }

    // An open Button.Flyout, SplitButton.Flyout or attached flyout under the editor (a
    // CalendarDatePicker's calendar is attached in its template).
    bool HasOpenOwnedFlyout(winrt::DependencyObject const& element)
    {
        auto const isOpen = [](winrt::FlyoutBase const& flyout) { return flyout && flyout.IsOpen(); };

        if (auto const fe = element.try_as<winrt::FrameworkElement>(); fe && isOpen(winrt::FlyoutBase::GetAttachedFlyout(fe)))
        {
            return true;
        }
        if (auto const button = element.try_as<winrt::Button>(); button && isOpen(button.Flyout()))
        {
            return true;
        }
        if (auto const splitButton = element.try_as<winrt::SplitButton>(); splitButton && isOpen(splitButton.Flyout()))
        {
            return true;
        }

        const int32_t count = winrt::VisualTreeHelper::GetChildrenCount(element);
        for (int32_t i = 0; i < count; ++i)
        {
            if (HasOpenOwnedFlyout(winrt::VisualTreeHelper::GetChild(element, i)))
            {
                return true;
            }
        }
        return false;
    }

    template <typename T>
    bool ContainsElementOfType(winrt::DependencyObject const& element)
    {
        if (element.try_as<T>())
        {
            return true;
        }

        const int32_t count = winrt::VisualTreeHelper::GetChildrenCount(element);
        for (int32_t i = 0; i < count; ++i)
        {
            if (ContainsElementOfType<T>(winrt::VisualTreeHelper::GetChild(element, i)))
            {
                return true;
            }
        }
        return false;
    }

    // True when an open popup outside the editor's tree was opened by the editor. A popup has no
    // public link to its owner, so only the editor's own flyouts count; a ContentDialog, a
    // TeachingTip or another table's menu does not.
    bool IsEditorOwnedPopup(winrt::Popup const& popup, winrt::FrameworkElement const& editor)
    {
        // Only one top-level flyout is open per root, so an open flyout under the editor is this one.
        if (HasOpenOwnedFlyout(editor))
        {
            return true;
        }

        // A DatePicker/TimePicker shows its picker flyout from code, out of reach of the walk above.
        auto const child = popup.Child();
        if (child.try_as<winrt::DatePickerFlyoutPresenter>())
        {
            return ContainsElementOfType<winrt::DatePicker>(editor);
        }
        if (child.try_as<winrt::TimePickerFlyoutPresenter>())
        {
            return ContainsElementOfType<winrt::TimePicker>(editor);
        }
        return false;
    }

    bool IsModifierKey(winrt::VirtualKey key)
    {
        switch (key)
        {
        case winrt::VirtualKey::Shift:
        case winrt::VirtualKey::LeftShift:
        case winrt::VirtualKey::RightShift:
        case winrt::VirtualKey::Control:
        case winrt::VirtualKey::LeftControl:
        case winrt::VirtualKey::RightControl:
        case winrt::VirtualKey::Menu:
        case winrt::VirtualKey::LeftMenu:
        case winrt::VirtualKey::RightMenu:
        case winrt::VirtualKey::LeftWindows:
        case winrt::VirtualKey::RightWindows:
            return true;
        default:
            return false;
        }
    }

    bool IsKeyDown(winrt::VirtualKey key)
    {
        return (winrt::InputKeyboardSource::GetKeyStateForCurrentThread(key) &
            winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down;
    }

    // Visual-tree only, unlike IsWithinElement: focus in a popup the editor opened (a ComboBox
    // dropdown) must not count, so Tab there stays with the popup.
    bool IsVisualDescendant(winrt::IInspectable const& candidate, winrt::DependencyObject const& ancestor)
    {
        for (auto node = candidate.try_as<winrt::DependencyObject>(); node; node = winrt::VisualTreeHelper::GetParent(node))
        {
            if (node == ancestor)
            {
                return true;
            }
        }
        return false;
    }

    winrt::TableViewRow OwningRow(winrt::IInspectable const& element)
    {
        for (auto node = element.try_as<winrt::DependencyObject>(); node; node = winrt::VisualTreeHelper::GetParent(node))
        {
            if (auto const row = node.try_as<winrt::TableViewRow>())
            {
                return row;
            }
        }
        return nullptr;
    }

    // Tab continues from the focused cell; let it leave the row as a Tab from a focused cell does.
    void PrepareTabWalkFromFocusedRow(winrt::XamlRoot const& root)
    {
        if (auto const row = root ? OwningRow(winrt::FocusManager::GetFocusedElement(root)) : nullptr)
        {
            winrt::get_self<TableViewRow>(row)->PrepareTabWalkFromCellInternal();
        }
    }
}

// F2 / Enter / Escape / Tab. Registered WITH handledEventsToo, because a single-line TextBox reports Enter
// as handled and the commit must still run. Each case therefore owns its own Handled policy
// explicitly: F2 and Escape defer to an editor that genuinely consumed the key, Enter does not.
void TableView::OnKeyDownForEditing(
    const winrt::IInspectable& /*sender*/,
    const winrt::KeyRoutedEventArgs& args)
{
    switch (args.Key())
    {
    case winrt::Windows::System::VirtualKey::F2:
        // Keyboard equivalent of double-click, on the cell the user navigated to. Only when the
        // key is still unhandled: F2 belongs to whatever focused control claimed it first.
        if (!args.Handled() && !IsEditing() && BeginEdit())
        {
            args.Handled(true);
        }
        break;

    case winrt::Windows::System::VirtualKey::Enter:
        if (IsEditing())
        {
            // Acted on even if the editor marked it handled: a single-line TextBox reports Enter as
            // handled, which would leave the editor with no way to close from the keyboard.
            // Handled regardless of the result - a veto or pending deferral still owns the key, and
            // letting it bubble would scroll the table under an open editor.
            CommitEdit();
            args.Handled(true);
        }
        break;

    case winrt::Windows::System::VirtualKey::Escape:
        // Like F2, this defers to an editor that genuinely consumed the key - a ComboBox in a
        // CellEditingTemplate swallows Escape to close its popup, and that must not also cancel
        // and roll back the whole cell edit. The same goes for the editor's own menu: Escape
        // light-dismisses it and focus is back in the editor by the time the key routes there.
        if (!args.Handled() && IsEditing() && !m_editorFlyoutClosing)
        {
            CancelEdit();
            args.Handled(true);
        }
        break;

    case winrt::Windows::System::VirtualKey::Tab:
        // An editor that consumes Tab marks it handled first.
        if (!args.Handled() &&
            !IsKeyDown(winrt::Windows::System::VirtualKey::Control) &&
            !IsKeyDown(winrt::Windows::System::VirtualKey::Menu))
        {
            const bool forward = !IsKeyDown(winrt::Windows::System::VirtualKey::Shift);
            if (m_pendingTabEditActive && !IsEditing())
            {
                // A Tab pressed before the wrapped row is ready moves on from that target.
                if (AdvancePendingTabEdit(forward))
                {
                    args.Handled(true);
                }
            }
            else if (IsEditing() && TryMoveEditToAdjacentCell(forward))
            {
                args.Handled(true);
            }
        }
        break;

    default:
        break;
    }
}

int32_t TableView::GetVisibleColumnIndex(winrt::TableViewColumn const& target)
{
    int32_t visible = 0;
    if (auto const columns = Columns())
    {
        for (auto const& column : columns)
        {
            if (!IsVisibleColumn(column))
            {
                continue;
            }
            if (column == target)
            {
                return visible;
            }
            ++visible;
        }
    }
    return -1;
}

// The next (previous) editable cell in logical column order, wrapping to the adjacent data row.
std::optional<TableView::TabEditTarget> TableView::FindAdjacentTabEditTarget(int32_t rowIndex, int32_t visibleColumn, bool forward)
{
    std::vector<TabEditTarget> editable;
    int32_t visible = 0;
    if (auto const columns = Columns())
    {
        for (auto const& column : columns)
        {
            if (!IsVisibleColumn(column))
            {
                continue;
            }
            if (IsColumnCellEditableInternal(column))
            {
                editable.push_back({ rowIndex, visible, column });
            }
            ++visible;
        }
    }
    if (editable.empty() || rowIndex < 0)
    {
        return std::nullopt;
    }

    if (forward)
    {
        for (auto const& entry : editable)
        {
            if (entry.VisibleColumn > visibleColumn)
            {
                return entry;
            }
        }
    }
    else
    {
        for (auto it = editable.rbegin(); it != editable.rend(); ++it)
        {
            if (it->VisibleColumn < visibleColumn)
            {
                return *it;
            }
        }
    }

    const int32_t rowCount = GetItemsSourceCount();
    int32_t adjacentRow = rowIndex;
    do
    {
        adjacentRow += forward ? 1 : -1;
    } while (adjacentRow >= 0 && adjacentRow < rowCount && IsGroupHeaderRow(adjacentRow));

    if (adjacentRow < 0 || adjacentRow >= rowCount)
    {
        return std::nullopt;
    }

    auto target = forward ? editable.front() : editable.back();
    target.Row = adjacentRow;
    return target;
}

// False leaves Tab to XAML: no editable cell follows, or focus moves within a multi-control editor.
bool TableView::TryMoveEditToAdjacentCell(bool forward)
{
    auto const repeater = m_rowsRepeater.get();
    auto const editRow = m_currentEditRow.get();
    auto const editItem = m_currentEditItem.get();
    auto const editColumn = m_currentEditColumn.get();
    auto const root = XamlRoot();
    if (m_editState != EditState::Editing || !repeater || !editRow || !editItem || !editColumn || !root)
    {
        return false;
    }

    auto const editingElement = winrt::get_self<TableViewRow>(editRow)->GetEditingElement();
    if (!editingElement)
    {
        return false;
    }

    // Tab inside an open dropdown or picker belongs to that popup.
    if (!IsVisualDescendant(winrt::FocusManager::GetFocusedElement(root), editingElement))
    {
        return false;
    }

    // A CellEditingTemplate with several controls keeps Tab until its last (first) one.
    if (HasTabStopAfterFocusInEditor(root, editingElement, forward))
    {
        return false;
    }

    const int32_t sourceRow = repeater.GetElementIndex(editRow);
    const int32_t sourceColumn = GetVisibleColumnIndex(editColumn);
    if (sourceRow < 0 || sourceColumn < 0)
    {
        return false;
    }

    // Past the last (first) cell the commit still runs here: Shift+Tab from the editor lands on its own
    // cell, which hands focus straight back to the editor, and a failed commit must keep the editor.
    auto const target = FindAdjacentTabEditTarget(sourceRow, sourceColumn, forward);
    if (!CommitEditInternal(target && target->Row != sourceRow ? EditingUnit::Row : EditingUnit::Cell))
    {
        return true;
    }

    // CellEditEnding handlers can move focus, reshape the rows or change the columns. Only focus still on
    // the committed record moves on; a re-shape restore still carrying focus to it counts.
    auto const focused = winrt::FocusManager::GetFocusedElement(root);
    auto const focusedRow = OwningRow(focused);
    const bool focusOnRecord =
        (focusedRow && SameInspectableIdentity(UnwrapEditingDataItem(focusedRow.DataContext()), editItem)) ||
        (m_pendingReshapeFocusRow >= 0 && (!focused || IsElementInBody(focused.try_as<winrt::DependencyObject>())));
    if (!focusOnRecord)
    {
        return true;
    }

    const int32_t rowIndex = FindEntryIndexForDataItem(editItem);
    const int32_t columnIndex = GetVisibleColumnIndex(editColumn);
    auto const next = FindAdjacentTabEditTarget(rowIndex, columnIndex >= 0 ? columnIndex : sourceColumn, forward);
    if (!next)
    {
        PrepareTabWalkFromFocusedRow(root);
        return false;
    }

    MoveEditToTarget(*next, rowIndex);
    return true;
}

void TableView::MoveEditToTarget(TabEditTarget target, int32_t sourceRow)
{
    auto const repeater = m_rowsRepeater.get();
    auto const item = UnwrapEditingDataItem(GetRowsSourceItemAt(target.Row));
    if (!repeater || !item)
    {
        return;
    }

    if (target.Row != sourceRow)
    {
        // SelectionChanged handlers can reshape the rows.
        SelectRowIndexFromKeyboardFocus(target.Row);
        target.Row = FindEntryIndexForDataItem(item);
        if (target.Row < 0)
        {
            return;
        }
    }

    auto const row = FindRealizedRowForItem(item);
    if (row && repeater.GetElementIndex(row) == target.Row && IsRowElementArranged(row) && IsRowElementInViewport(row))
    {
        OpenTabEditTarget(target.Row, item, target);
        return;
    }

    FocusCell(target.Row, target.VisibleColumn);
    RequestPendingTabEdit(target, item);
}

// BeginEdit focuses the editor itself; the cell is focused only when the edit is refused.
void TableView::OpenTabEditTarget(int32_t rowIndex, winrt::IInspectable const& item, TabEditTarget const& target)
{
    SetColumnCursorInternal(target.VisibleColumn);
    if (!BeginEdit(item, target.Column))
    {
        FocusCell(rowIndex, target.VisibleColumn);
    }
}

// False when nothing follows the pending target: the pending edit ends and Tab leaves normally.
bool TableView::AdvancePendingTabEdit(bool forward)
{
    auto const item = m_pendingTabEditItem.get();
    int32_t rowIndex = m_pendingTabEditRow;
    if (!item || !SameInspectableIdentity(UnwrapEditingDataItem(GetRowsSourceItemAt(rowIndex)), item))
    {
        rowIndex = item ? FindEntryIndexForDataItem(item) : -1;
    }

    const int32_t column = GetVisibleColumnIndex(m_pendingTabEditColumn.get());
    if (auto const next = FindAdjacentTabEditTarget(rowIndex, column >= 0 ? column : m_pendingTabEditVisibleColumn, forward))
    {
        MoveEditToTarget(*next, rowIndex);
        return true;
    }

    CancelPendingTabEdit();
    PrepareTabWalkFromFocusedRow(XamlRoot());
    return false;
}

bool TableView::HasTabStopAfterFocusInEditor(winrt::XamlRoot const& root, winrt::FrameworkElement const& editingElement, bool forward)
{
    // Searched from XamlRoot.Content, not the editor, so a search that wraps cannot keep Tab in the editor.
    // FocusManager allows Next/Previous on desktop only because a SearchRoot is given.
    try
    {
        winrt::FindNextElementOptions options;
        options.SearchRoot(root.Content());
        auto const next = winrt::FocusManager::FindNextElement(
            forward ? winrt::FocusNavigationDirection::Next : winrt::FocusNavigationDirection::Previous, options);
        return next && IsVisualDescendant(next, editingElement);
    }
    catch (winrt::hresult_error const&)
    {
        auto const edge = forward
            ? winrt::FocusManager::FindLastFocusableElement(editingElement)
            : winrt::FocusManager::FindFirstFocusableElement(editingElement);
        auto const focused = winrt::FocusManager::GetFocusedElement(root);
        return edge && focused && edge != focused.try_as<winrt::DependencyObject>();
    }
}

void TableView::RequestPendingTabEdit(TabEditTarget const& target, winrt::IInspectable const& item)
{
    CancelPendingTabEdit();

    auto const root = XamlRoot();
    auto const queue = DispatcherQueue();
    if (target.Row < 0 || !item || !target.Column || !root || !winrt::FocusManager::GetFocusedElement(root) || !queue)
    {
        return;
    }

    const uint32_t generation = ++m_pendingTabEditGeneration;
    m_pendingTabEditActive = true;
    m_pendingTabEditRow = target.Row;
    m_pendingTabEditVisibleColumn = target.VisibleColumn;
    m_pendingTabEditAttemptsLeft = s_keyboardFocusAttempts;
    m_pendingTabEditItem.set(item);
    m_pendingTabEditColumn.set(target.Column);

    // Each layout only queues a dispatcher turn: a template editor realizes via UpdateLayout, which is a
    // no-op inside LayoutUpdated.
    m_pendingTabEditLayoutRevoker = LayoutUpdated(winrt::auto_revoke,
        [weakThis = get_weak(), generation](winrt::IInspectable const&, winrt::IInspectable const&)
        {
            if (auto strongThis = weakThis.get(); strongThis && generation == strongThis->m_pendingTabEditGeneration)
            {
                strongThis->QueuePendingTabEditTurn();
            }
        });

    QueuePendingTabEditTurn();
}

// Retries re-arm at Low priority so layout runs first; the body focus retry loop re-arms when it ends.
void TableView::QueuePendingTabEditTurn(winrt::DispatcherQueuePriority priority)
{
    if (!m_pendingTabEditActive || m_pendingTabEditTurnQueued)
    {
        return;
    }

    auto const queue = DispatcherQueue();
    const uint32_t generation = m_pendingTabEditGeneration;
    m_pendingTabEditTurnQueued = true;
    if (!queue || !queue.TryEnqueue(priority,
        [weakThis = get_weak(), generation]()
        {
            if (auto strongThis = weakThis.get())
            {
                if (generation == strongThis->m_pendingTabEditGeneration)
                {
                    strongThis->m_pendingTabEditTurnQueued = false;
                }
                strongThis->TryCompletePendingTabEdit(generation);
            }
        }))
    {
        CancelPendingTabEdit();
    }
}

void TableView::TryCompletePendingTabEdit(uint32_t generation)
{
    if (!m_pendingTabEditActive || generation != m_pendingTabEditGeneration)
    {
        return;
    }

    auto const item = m_pendingTabEditItem.get();
    auto const column = m_pendingTabEditColumn.get();
    auto const repeater = m_rowsRepeater.get();
    auto const root = XamlRoot();
    auto const focused = root ? winrt::FocusManager::GetFocusedElement(root) : nullptr;
    winrt::DependencyObject const self = *this;
    const bool bodyFocusPending = IsKeyboardBodyFocusPending();
    if (IsEditing() || !item || !column || !repeater || !focused || !IsVisualDescendant(focused, self) ||
        (!bodyFocusPending && m_pendingTabEditAttemptsLeft <= 0))
    {
        CancelPendingTabEdit();
        return;
    }
    if (!bodyFocusPending)
    {
        --m_pendingTabEditAttemptsLeft;
    }

    // Rows only shift under the target on an Add/Remove above it; scan just then.
    int32_t rowIndex = m_pendingTabEditRow;
    if (!SameInspectableIdentity(UnwrapEditingDataItem(GetRowsSourceItemAt(rowIndex)), item))
    {
        rowIndex = FindEntryIndexForDataItem(item);
        if (rowIndex < 0)
        {
            CancelPendingTabEdit();
            return;
        }
        m_pendingTabEditRow = rowIndex;
    }

    auto const row = repeater.TryGetElement(rowIndex).try_as<winrt::TableViewRow>();
    if (!row || !SameInspectableIdentity(UnwrapEditingDataItem(row.DataContext()), item) ||
        !IsRowElementArranged(row) || !IsRowElementInViewport(row))
    {
        if (!bodyFocusPending)
        {
            QueuePendingTabEditTurn(winrt::DispatcherQueuePriority::Low);
        }
        return;
    }

    const TabEditTarget target{ rowIndex, GetVisibleColumnIndex(column), column };
    CancelPendingTabEdit();
    if (target.VisibleColumn < 0)
    {
        return;
    }
    OpenTabEditTarget(rowIndex, item, target);
}

void TableView::CancelPendingTabEdit()
{
    ++m_pendingTabEditGeneration;
    if (!m_pendingTabEditActive)
    {
        return;
    }

    m_pendingTabEditActive = false;
    m_pendingTabEditTurnQueued = false;
    m_pendingTabEditRow = -1;
    m_pendingTabEditVisibleColumn = -1;
    m_pendingTabEditAttemptsLeft = 0;
    m_pendingTabEditItem.set(nullptr);
    m_pendingTabEditColumn.set(nullptr);
    m_pendingTabEditLayoutRevoker.revoke();

    // The live re-shape held for this pending edit (see SetLiveShapingHold) can run now.
    if (!IsEditing())
    {
        if (auto const source = m_activeSource.get())
        {
            winrt::get_self<::TableViewSource>(source)->ResumeHeldLiveShaping();
        }
    }
}

// Tab advances a pending edit (see OnKeyDownForEditing); modifiers alone are not a newer intent.
void TableView::CancelPendingTabEditForKey(winrt::VirtualKey key)
{
    if (key != winrt::VirtualKey::Tab && !IsModifierKey(key))
    {
        CancelPendingTabEdit();
    }
}

// Any other focus move is a newer intent, except the repeater moving focus off a row it clears.
void TableView::CancelPendingTabEditForFocusMove(const winrt::Microsoft::UI::Xaml::Input::LosingFocusEventArgs& args)
{
    if (!m_pendingTabEditActive)
    {
        return;
    }

    auto const repeater = m_rowsRepeater.get();
    if (auto const oldRow = OwningRow(args.OldFocusedElement()); oldRow && repeater && repeater.GetElementIndex(oldRow) < 0)
    {
        return;
    }

    if (auto const newRow = OwningRow(args.NewFocusedElement());
        newRow && SameInspectableIdentity(UnwrapEditingDataItem(newRow.DataContext()), m_pendingTabEditItem.get()))
    {
        return;
    }

    CancelPendingTabEdit();
}

// Click-away / tab-away commit. Without it the editor stays open when focus leaves, and a second
// edit elsewhere leaves two editors on screen. Committing matches the platform grid convention.
//
// The decision is deliberately NOT made here. Opening an editor produces a burst of focus traffic,
// so a synchronous commit on the first LosingFocus closes the edit in the gesture that opened it.
// The check is posted and re-evaluated once focus has settled.
void TableView::OnLosingFocusForEditing(
    const winrt::IInspectable& /*sender*/,
    const winrt::Microsoft::UI::Xaml::Input::LosingFocusEventArgs& args)
{
    CancelPendingTabEditForFocusMove(args);

    if (!IsEditing())
    {
        return;
    }

    auto const row = m_currentEditRow.get();
    if (!row)
    {
        return;
    }

    auto const editingElement = winrt::get_self<TableViewRow>(row)->GetEditingElement();
    if (!editingElement)
    {
        return;
    }

    // Focus must actually be leaving the editor, and not merely moving within it (a ComboBox
    // opening its popup, or a template column containing several controls).
    if (!IsWithinElement(args.OldFocusedElement(), editingElement) ||
        IsWithinElement(args.NewFocusedElement(), editingElement))
    {
        return;
    }

    if (m_focusLossCommitQueued)
    {
        return;
    }

    QueueFocusLossCommit();
}

void TableView::QueueFocusLossCommit()
{
    m_focusLossCommitQueued = true;

    auto weakThis = get_weak();
    bool queued = false;
    if (auto const dispatcher = DispatcherQueue())
    {
        queued = dispatcher.TryEnqueue([weakThis]()
        {
            if (auto strongThis = weakThis.get())
            {
                strongThis->CompleteFocusLossCommit();
            }
        });
    }

    if (!queued)
    {
        // Nothing will clear the flag for us. Leaving it set would silently disable focus-loss
        // commit for the rest of the control's life.
        m_focusLossCommitQueued = false;
    }
}

void TableView::CompleteFocusLossCommit()
{
    m_focusLossCommitQueued = false;
    m_editorFlyoutClosedRevoker.revoke();
    m_editorFlyoutClosingRevoker.revoke();
    m_editorPopupClosedRevoker.revoke();

    if (!IsEditing())
    {
        return;
    }

    auto const row = m_currentEditRow.get();
    if (!row)
    {
        return;
    }

    auto const editingElement = winrt::get_self<TableViewRow>(row)->GetEditingElement();
    if (!editingElement)
    {
        return;
    }

    // Focus may have come back to the editor while this was queued - which is exactly what the
    // open-an-editor gesture itself does. Only a genuine move away closes the edit.
    auto const root = XamlRoot();
    winrt::IInspectable focused{ nullptr };
    if (root)
    {
        focused = winrt::FocusManager::GetFocusedElement(root);

        if (IsWithinElement(focused, editingElement))
        {
            return;
        }

        // Refocus the editor only when focus lands on the row/cell host from the opening gesture;
        // hosted controls in other cells remain valid focus targets.
        auto const editingCell = winrt::get_self<TableViewRow>(row)->GetEditingCellWrapper();
        if (focused == row.try_as<winrt::DependencyObject>() ||
            (editingCell && focused == editingCell.try_as<winrt::DependencyObject>()))
        {
            editingElement.Focus(winrt::FocusState::Programmatic);
            return;
        }
    }

    // Focus went into the editor's own menu (a TextBox's copy/paste flyout): the user is still
    // working in the editor. Decide again once the menu closes - it hands focus back.
    if (auto const flyout = FindOpenEditorFlyout(editingElement))
    {
        // Escape light-dismisses the menu before the key is routed, so the editor then receives
        // that same Escape. Mark the close so the key handler leaves the edit alone.
        m_editorFlyoutClosingRevoker = flyout.Closing(winrt::auto_revoke,
            [weakThis = get_weak()](auto&&, auto&&)
            {
                if (auto strongThis = weakThis.get())
                {
                    strongThis->MarkEditorFlyoutClosing();
                }
            });

        m_editorFlyoutClosedRevoker = flyout.Closed(winrt::auto_revoke,
            [weakThis = get_weak()](auto&&, auto&&)
            {
                if (auto strongThis = weakThis.get())
                {
                    strongThis->m_editorFlyoutClosingRevoker.revoke();
                    strongThis->m_editorFlyoutClosedRevoker.revoke();
                    if (!strongThis->m_focusLossCommitQueued)
                    {
                        strongThis->QueueFocusLossCommit();
                    }
                }
            });
        return;
    }

    // Focus went into a popup the editor opened - a picker or a button flyout inside it. Decide again
    // once it closes. Focus in any other popup is a genuine move away and commits now.
    if (auto const popup = FindOpenPopupContaining(focused, root); popup && IsEditorOwnedPopup(popup, editingElement))
    {
        m_editorPopupClosedRevoker = popup.Closed(winrt::auto_revoke,
            [weakThis = get_weak()](auto&&, auto&&)
            {
                if (auto strongThis = weakThis.get())
                {
                    strongThis->m_editorPopupClosedRevoker.revoke();
                    strongThis->MarkEditorFlyoutClosing();
                    if (!strongThis->m_focusLossCommitQueued)
                    {
                        strongThis->QueueFocusLossCommit();
                    }
                }
            });
        return;
    }

    CommitEdit();
}

void TableView::MarkEditorFlyoutClosing()
{
    // Lives only until the current input has been dispatched.
    m_editorFlyoutClosing = true;
    auto const dispatcher = DispatcherQueue();
    if (!dispatcher || !dispatcher.TryEnqueue([weakThis = get_weak()]()
        {
            if (auto strongThis = weakThis.get())
            {
                strongThis->m_editorFlyoutClosing = false;
            }
        }))
    {
        m_editorFlyoutClosing = false;
    }
}
