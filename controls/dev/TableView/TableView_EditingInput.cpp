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

// Input gestures that drive editing.
//
// Kept separate from TableView_Editing.cpp: that file owns the edit state machine, reachable
// entirely through BeginEdit/CommitEdit/CancelEdit; this one owns the policy of which gestures map
// onto that API. The split lets a future keyboard or selection layer re-route gestures without
// reopening the state machine, and keeps the state machine testable without synthesizing input.

namespace
{
    // True when `candidate` is `ancestor` or sits underneath it.
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
        // Nothing will clear the flag for us. Leaving it set would silently disable focus-loss
        // commit for the rest of the control's life.
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

    // Focus may have come back to the editor while this was queued - which is exactly what the
    // open-an-editor gesture itself does. Only a genuine move away closes the edit.
    if (auto const root = XamlRoot())
    {
        auto const focused = winrt::FocusManager::GetFocusedElement(root);

        if (IsWithinElement(focused, editingElement))
        {
            return;
        }

        // Focus settling on the ROW CONTAINER itself is the tail of the gesture that opened the
        // editor - the row takes pointer focus before the editor does - so re-focus the editor
        // rather than closing an edit the user just started.
        //
        // Deliberately the container only, not its subtree: a Button, ComboBox or hyperlink in
        // another cell of the same row is a genuine focus target, and stealing focus back from it
        // would make those controls unusable while an edit is open.
        if (focused == row.try_as<winrt::DependencyObject>())
        {
            editingElement.Focus(winrt::FocusState::Programmatic);
            return;
        }
    }

    CommitEdit();
}
