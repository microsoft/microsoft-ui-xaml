// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewColumn.h"
#include "TableViewRow.h"
#include "TVDiag.h"

#include <algorithm>
#include <cmath>


namespace
{
    bool IsWithinElement(winrt::IInspectable const& candidate, winrt::FrameworkElement const& ancestor)
    {
        if (!ancestor)
        {
            return false;
        }

        auto current = candidate.try_as<winrt::DependencyObject>();
        while (current)
        {
            if (current == ancestor)
            {
                return true;
            }
            current = winrt::VisualTreeHelper::GetParent(current);
        }

        return false;
    }

}

// F2 / Enter / Escape. Registered WITH handledEventsToo, because a single-line TextBox reports Enter
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
        // and roll back the whole cell edit.
        if (!args.Handled() && IsEditing())
        {
            CancelEdit();
            args.Handled(true);
        }
        break;

    default:
        break;
    }
}

void TableView::OnLosingFocusForEditing(
    const winrt::IInspectable& /*sender*/,
    const winrt::Microsoft::UI::Xaml::Input::LosingFocusEventArgs& args)
{
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
        m_focusLossCommitQueued = false;
    }
}

void TableView::CompleteFocusLossCommit()
{
    m_focusLossCommitQueued = false;

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

    if (auto const root = XamlRoot())
    {
        auto const focused = winrt::FocusManager::GetFocusedElement(root);

        if (IsWithinElement(focused, editingElement))
        {
            return;
        }

        // Focus settling on the ROW CONTAINER itself - or on the CELL that hosts the editor - is
        // the tail of the gesture that opened the editor: the row takes pointer focus (now onto the
        // pressed cell) before the editor does. Re-focus the editor rather than closing an edit the
        // user has only just started.
        //
        // Deliberately those two elements only, not the whole subtree: a Button, ComboBox or
        // hyperlink in another cell of the same row is a genuine focus target, and stealing focus
        // back from it would make those controls unusable while an edit is open.
        auto const editingCell = winrt::get_self<TableViewRow>(row)->GetEditingCellWrapper();
        if (focused == row.try_as<winrt::DependencyObject>() ||
            (editingCell && focused == editingCell.try_as<winrt::DependencyObject>()))
        {
            editingElement.Focus(winrt::FocusState::Programmatic);
            return;
        }
    }

    CommitEdit();
}
