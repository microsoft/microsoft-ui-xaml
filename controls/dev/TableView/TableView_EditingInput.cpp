// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewColumn.h"
#include "TableViewRow.h"
#include "TableViewFocusHelpers.h"
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

    // The open popup whose content holds `element`. Flyouts that are not reachable from the editor's
    // properties (a DatePicker's picker, a Button.Flyout, an attached flyout) surface only here.
    winrt::Popup FindOpenPopupContaining(winrt::IInspectable const& element, winrt::XamlRoot const& root)
    {
        if (!element || !root)
        {
            return nullptr;
        }

        for (auto const& popup : winrt::VisualTreeHelper::GetOpenPopupsForXamlRoot(root))
        {
            if (auto const child = popup.Child().try_as<winrt::FrameworkElement>(); child && IsWithinElement(element, child))
            {
                return popup;
            }
        }

        return nullptr;
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
        // and roll back the whole cell edit. The same goes for the editor's own menu: Escape
        // light-dismisses it and focus is back in the editor by the time the key routes there.
        if (!args.Handled() && IsEditing() && !m_editorFlyoutClosing)
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
                    strongThis->m_editorFlyoutClosingRevoker.revoke();
                    strongThis->MarkEditorFlyoutClosing();
                }
            });

        m_editorFlyoutClosedRevoker = flyout.Closed(winrt::auto_revoke,
            [weakThis = get_weak()](auto&&, auto&&)
            {
                if (auto strongThis = weakThis.get(); strongThis && !strongThis->m_focusLossCommitQueued)
                {
                    strongThis->QueueFocusLossCommit();
                }
            });
        return;
    }

    // Focus went into some other popup - a picker or a button flyout inside the editor. A popup has
    // no link back to its owner, but table menus stay closed while editing, so the popup that took
    // focus from the editor is the editor's. Decide again once it closes.
    if (auto const popup = FindOpenPopupContaining(focused, root))
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
