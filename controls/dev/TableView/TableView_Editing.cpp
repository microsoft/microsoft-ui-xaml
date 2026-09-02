// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewColumn.h"
#include "TableViewTextColumn.h"
#include "TableViewRow.h"
#include "TableViewBeginningEditEventArgs.h"
#include "TableViewCellEditEndingEventArgs.h"
#include "TVDiag.h"

#include <atomic>
#include <string>
#include <string_view>

namespace
{
    // The data field a column edits. Derived from the column's Binding rather than authored
    // separately: a column already knows what it is bound to, and the public "which field is this
    // column about?" concept is the base column's SortMemberPath - editing must not grow a second
    // one. Empty means "no single source property", and the control falls back to object-level
    // validation and to scraping the editor's bindings for a cancel snapshot.
    winrt::hstring ResolveEditingPropertyPath(winrt::TableViewColumn const& column)
    {
        if (auto const textColumn = column.try_as<winrt::TableViewTextColumn>())
        {
            try
            {
                return winrt::get_self<TableViewTextColumn>(textColumn)->GetEditingPropertyPath();
            }
            catch (...)
            {
                return {};
            }
        }

        return {};
    }

    bool IsColumnInColumns(winrt::TableView const& owner, winrt::TableViewColumn const& column)
    {
        if (!owner || !column)
        {
            return false;
        }

        if (auto columns = owner.Columns())
        {
            for (auto const& candidate : columns)
            {
                if (candidate == column)
                {
                    return true;
                }
            }
        }

        return false;
    }

    // First column a keyboard user could edit. Used only as the fallback when no current column
    // has ever been established (see TryResolveFocusedCell).
    winrt::TableViewColumn FirstEditableColumn(winrt::TableView const& owner)
    {
        if (!owner)
        {
            return nullptr;
        }

        if (auto columns = owner.Columns())
        {
            for (auto const& candidate : columns)
            {
                if (candidate &&
                    !candidate.IsReadOnly() &&
                    candidate.Visibility() == winrt::Visibility::Visible)
                {
                    return candidate;
                }
            }
        }

        return nullptr;
    }
    winrt::IInspectable GetBindingSnapshotSource(
        winrt::Binding const& binding,
        winrt::IInspectable const& fallbackDataItem)
    {
        if (binding)
        {
            if (auto source = binding.Source())
            {
                return source;
            }
        }
        return fallbackDataItem;
    }

    winrt::hstring GetBindingPath(winrt::Binding const& binding)
    {
        if (binding)
        {
            if (auto path = binding.Path())
            {
                return path.Path();
            }
        }
        return {};
    }
    void LogConsumerHandlerThrow(wchar_t const* eventName)
    {
        // Retail-visible on purpose. A consumer handler that throws leaves the edit continuing
        // as though the handler had agreed, which is indistinguishable at runtime from a handler
        // that simply did nothing. Logging only under DBG meant a shipping app had no way to
        // diagnose it.
        const auto hr = static_cast<unsigned int>(winrt::to_hresult());
        TVDiag::LogRetailF(
            L"[TableView] Consumer %ls handler threw (HRESULT 0x%08X); edit continues as un-cancelled.",
            eventName,
            hr);
    }

    // A cancel that cannot restore the pre-edit value is a silent data-integrity failure from the
    // user's point of view: they pressed Esc and their edit stayed. Report it.
    void LogRevertUnavailable(wchar_t const* reason)
    {
        TVDiag::LogRetailF(
            L"[TableView] Cancel cannot revert this edit (%ls). Implement ITableViewEditableItem "
            L"on the data item to make rollback reliable.",
            reason);
    }
}

// ----- Editing lifecycle (opt-in via IsReadOnly=false) -----

// COM identity, not ABI-pointer equality: IInspectable's operator== compares raw abi pointers, so
// the same object reached through a different interface - boxed values, projected interfaces, a
// future wrapping layer - would not match. A member rather than a file-local helper so every
// comparison of data items in the control uses the same one.
bool TableView::SameInspectableIdentity(winrt::IInspectable const& lhs, winrt::IInspectable const& rhs)
{
    if (lhs == rhs)
    {
        return true;
    }
    if (!lhs || !rhs)
    {
        return false;
    }

    try
    {
        return lhs.as<winrt::Windows::Foundation::IUnknown>() ==
            rhs.as<winrt::Windows::Foundation::IUnknown>();
    }
    catch (...)
    {
        return false;
    }
}

// Single choke point for turning a row's container-level item into the object an edit writes to.
// Identity today; it exists as a seam for a layer that wraps items (grouping), where the unwrap and
// the nullptr "not editable" answer belong here rather than at every edit entry point.
winrt::IInspectable TableView::UnwrapEditingDataItem(winrt::IInspectable const& item) const
{
    return item;
}

winrt::IInspectable TableView::CurrentItem()
{
    // The current cell tracks focus at all times, and deliberately does NOT switch to reporting the
    // edit target while an edit is open: a property whose meaning depends on hidden state would
    // stop being valid the moment the edit closes. BeginEdit moves it onto the edited cell anyway.
    return m_currentItem.get();
}

winrt::TableViewColumn TableView::CurrentColumn()
{
    auto const column = m_currentColumn.get();
    if (IsColumnInColumns(*this, column))
    {
        return column;
    }
    return nullptr;
}

void TableView::SetCurrentItem(winrt::IInspectable const& item)
{
    m_currentItem.set(item);
}

void TableView::UpdateCurrentColumn(winrt::TableViewColumn const& column)
{
    if (IsColumnInColumns(*this, column))
    {
        m_currentColumn.set(column);
    }
    else
    {
        m_currentColumn.set(nullptr);
    }
}

void TableView::SetCurrentCell(winrt::IInspectable const& item, winrt::TableViewColumn const& column)
{
    SetCurrentItem(item);
    UpdateCurrentColumn(column);
}

bool TableView::RaiseBeginningEdit(winrt::IInspectable const& item, winrt::TableViewColumn const& column)
{
    if (!m_beginningEditEventSource)
    {
        return true;
    }
    auto args = winrt::make_self<TableViewBeginningEditEventArgs>(item, column);
    try
    {
        m_beginningEditEventSource(*this, *args);
    }
    catch (...)
    {
        LogConsumerHandlerThrow(L"BeginningEdit");
    }
    return !args->Cancel();
}

TableView::EditEndingResult TableView::RaiseEditEnding(
    EditingUnit unit,
    winrt::TableViewEditAction action,
    bool honorCancel)
{
    const auto item = m_currentEditItem.get();
    const auto column = m_currentEditColumn.get();

    // Raised synchronously. There is no deferral in this release, so a handler decides before it
    // returns and the veto is simply read afterwards - no sink, no completion marshalling, and no
    // way for an unclosed deferral to wedge the control at IsEditing == true.
    if (m_cellEditEndingEventSource)
    {
        auto cellArgs = winrt::make_self<TableViewCellEditEndingEventArgs>(item, column, action);
        auto const self = get_strong();
        PostEditNotification([self, cellArgs]()
        {
            try
            {
                self->m_cellEditEndingEventSource(*self, *cellArgs);
            }
            catch (...)
            {
                LogConsumerHandlerThrow(L"CellEditEnding");
            }
        });

        // honorCancel == false is a forced close - the caller is tearing the edit down now and a
        // handler must not be able to keep it open. That is also the only case in which the raise
        // above was deferred onto the dispatcher, so Cancel would not have been written yet anyway.
        if (honorCancel && cellArgs->Cancel())
        {
            return EditEndingResult::Vetoed;
        }
    }

    return EditEndingResult::Completed;
}

bool TableView::TryResolveFocusedCell(winrt::IInspectable& item, winrt::TableViewColumn& column)
{
    item = nullptr;
    column = nullptr;

    auto repeater = m_rowsRepeater.get();
    if (!repeater)
    {
        return false;
    }

    winrt::TableViewRow focusedRow{ nullptr };
    winrt::TableViewColumn focusedColumn{ nullptr };

    if (auto root = XamlRoot())
    {
        if (auto focused = winrt::FocusManager::GetFocusedElement(root).try_as<winrt::DependencyObject>())
        {
            winrt::DependencyObject node = focused;
            while (node)
            {
                if (!focusedColumn)
                {
                    if (auto fe = node.try_as<winrt::FrameworkElement>())
                    {
                        focusedColumn = fe.Tag().try_as<winrt::TableViewColumn>();
                    }
                }

                if (!focusedRow)
                {
                    focusedRow = node.try_as<winrt::TableViewRow>();
                }

                if (focusedRow && focusedColumn)
                {
                    break;
                }

                node = winrt::VisualTreeHelper::GetParent(node);
            }
        }
    }

    if (!focusedRow)
    {
        return false;
    }

    const auto rowIndex = repeater.GetElementIndex(focusedRow);
    if (!TryGetItemAtRowIndex(rowIndex, item))
    {
        return false;
    }

    column = focusedColumn ? focusedColumn : m_currentColumn.get();

    // Keyboard-only fallback. Row navigation focuses the row, not a column-tagged cell, so a user
    // who has never used the pointer has no current column and F2 would silently do nothing.
    // Defaulting to the first visible editable column makes the keyboard path reachable.
    if (!column)
    {
        column = FirstEditableColumn(*this);
    }

    if (!item || !column || !IsColumnInColumns(*this, column))
    {
        item = nullptr;
        column = nullptr;
        return false;
    }

    return true;
}

bool TableView::TryResolveCurrentCell(winrt::IInspectable& item, winrt::TableViewColumn& column)
{
    // Focus wins over the cached current cell when they disagree. Row navigation moves focus but not
    // the current cell (there is no cell-level keyboard navigation yet), so without this, clicking
    // row 1 then arrowing to row 40 and pressing F2 would edit row 1. Preferring the focused row
    // while keeping the cached column gives the user the cell they are looking at.
    winrt::IInspectable focusedItem{ nullptr };
    winrt::TableViewColumn focusedColumn{ nullptr };
    if (TryResolveFocusedCell(focusedItem, focusedColumn))
    {
        item = focusedItem;
        column = focusedColumn;
        SetCurrentCell(item, column);
        return true;
    }

    item = m_currentItem.get();
    column = m_currentColumn.get();

    if (item && column && IsColumnInColumns(*this, column))
    {
        return true;
    }

    item = nullptr;
    column = nullptr;
    return false;
}

bool TableView::TryResolveCurrentCellForEdit(winrt::IInspectable& item, winrt::TableViewColumn& column)
{
    if (!TryResolveCurrentCell(item, column))
    {
        return false;
    }
    // Editing additionally requires the resolved column to be editable; a read-only
    // column is a valid current cell for navigation but cannot begin an edit.
    if (column.IsReadOnly())
    {
        item = nullptr;
        column = nullptr;
        return false;
    }
    return true;
}

bool TableView::TryGetItemAtRowIndex(int32_t rowIndex, winrt::IInspectable& item) const
{
    item = nullptr;
    if (rowIndex < 0)
    {
        return false;
    }

    if (auto repeater = m_rowsRepeater.get())
    {
        if (auto row = repeater.TryGetElement(rowIndex).try_as<winrt::TableViewRow>())
        {
            if (auto dataItem = UnwrapEditingDataItem(row.DataContext()))
            {
                item = dataItem;
                return true;
            }
        }

        if (auto view = repeater.ItemsSourceView())
        {
            if (rowIndex < view.Count())
            {
                if (auto dataItem = UnwrapEditingDataItem(view.GetAt(rowIndex)))
                {
                    item = dataItem;
                    return true;
                }
            }
        }
    }

    return false;
}

winrt::TableViewRow TableView::FindRealizedRowForItem(winrt::IInspectable const& item)
{
    if (!item)
    {
        return nullptr;
    }

    if (auto repeater = m_rowsRepeater.get())
    {
        const auto childrenCount = winrt::VisualTreeHelper::GetChildrenCount(repeater);
        for (int32_t childIndex = 0; childIndex < childrenCount; ++childIndex)
        {
            if (auto row = winrt::VisualTreeHelper::GetChild(repeater, childIndex).try_as<winrt::TableViewRow>())
            {
                // ItemsRepeater keeps recycled containers parented in its pool, and a pooled row
                // still carries the DataContext of the item it last showed. Without this filter a
                // scrolled-away ghost can win the identity match below over the genuinely realized
                // row - so the visible row never gets its state, intermittently and only after
                // scrolling. GetElementIndex is >= 0 only for containers the repeater currently
                // considers realized.
                if (repeater.GetElementIndex(row) < 0)
                {
                    continue;
                }

                // COM identity, not ABI-pointer equality: IInspectable's operator== compares the
                // raw abi pointers, so the same object reached through a different interface
                // (boxed values, GroupedEntry unwrapping, projected interfaces) would not match
                // and BeginEdit would fail with no diagnostic.
                if (auto dataItem = UnwrapEditingDataItem(row.DataContext());
                    dataItem && SameInspectableIdentity(dataItem, item))
                {
                    return row;
                }
            }
        }
    }

    return nullptr;
}

bool TableView::TryBeginEditVisual(winrt::IInspectable const& item, winrt::TableViewColumn const& column)
{
    if (auto row = FindRealizedRowForItem(item))
    {
        if (winrt::get_self<TableViewRow>(row)->BeginCellEdit(column, item))
        {
            m_currentEditRow.set(row);
            return true;
        }
    }

    return false;
}

// The editor currently in the tree, or null once the edit has been torn down. Surfaced on the
// EditEnding args so a handler can read the pending value before it is written to the source.
winrt::FrameworkElement TableView::CurrentEditingElement() const
{
    if (auto const row = m_currentEditRow.get())
    {
        return winrt::get_self<TableViewRow>(row)->GetEditingElement();
    }
    return nullptr;
}

void TableView::EndEditVisual(winrt::TableViewEditAction action)
{
    if (auto row = m_currentEditRow.get())
    {
        if (m_suppressEditVisualRestore)
        {
            // Running inside a layout pass: release the row's edit state without touching the
            // tree. The row's cells are being restamped or rebuilt, so the display visual does
            // not need restoring - and doing it here would re-enter layout.
            winrt::get_self<TableViewRow>(row)->AbandonCellEdit();
        }
        else
        {
            winrt::get_self<TableViewRow>(row)->EndCellEdit(action);
        }
    }
    m_currentEditRow.set(nullptr);
}

bool TableView::TerminateEditWithoutVisualRestore(bool insideLayoutPass)
{
    if (m_editState == EditState::None)
    {
        return true;
    }

    m_suppressEditVisualRestore = true;
    m_insideLayoutPass = m_insideLayoutPass || insideLayoutPass;
    const bool clearLayoutFlag = insideLayoutPass;
    auto restore = wil::scope_exit([this, clearLayoutFlag]() noexcept
    {
        m_suppressEditVisualRestore = false;
        if (clearLayoutFlag)
        {
            m_insideLayoutPass = false;
        }
    });

    return TerminateEditForReset(true /* force */);
}

// App code must not run inside ItemsRepeater's measure. A CellEditEnding / EditEnded handler is
// ordinary app code, and the obvious things it does - touch the tree, invalidate layout, show a
// dialog - re-enter the pass we are standing in and trip the XAML re-entrancy fail-fast.
//
// Deferring them is legal precisely here: a teardown from a layout pass is always FORCED, so
// honorCancel is false, no handler can veto it, and RaiseEditEnding never waits on a deferral in
// that case. The edit is over either way - the notification is purely informational, so delivering
// it one tick later changes nothing an app can observe except that it no longer crashes.
//
// The item write is deliberately NOT deferred: it stays synchronous so a forced close cannot lose
// the user's value if the control goes away before the dispatcher runs.
void TableView::PostEditNotification(std::function<void()> notify)
{
    if (!notify)
    {
        return;
    }

    if (!m_insideLayoutPass)
    {
        notify();
        return;
    }

    auto weakThis = get_weak();
    auto queue = DispatcherQueue();
    if (queue && queue.TryEnqueue([weakThis, notify]()
        {
            if (auto strongThis = weakThis.get())
            {
                notify();
            }
        }))
    {
        return;
    }

    // No dispatcher, or it is shutting down. Losing the notification silently would be worse than
    // delivering it inline: an app that tracks edit state would be wedged with no way to recover.
    notify();
}

bool TableView::HasBlockingValidationErrors(
    winrt::IInspectable const& item,
    winrt::TableViewColumn const& column) const
{
    if (!item)
    {
        return false;
    }

    auto errorInfo = item.try_as<winrt::Microsoft::UI::Xaml::Data::INotifyDataErrorInfo>();
    if (!errorInfo)
    {
        return false;
    }

    // Scope to the property this column actually writes. Object-level HasErrors lets an
    // unrelated, pre-existing error on a different property block this cell permanently -
    // the user could never leave the edit.
    const auto editingPath = column ? ResolveEditingPropertyPath(column) : winrt::hstring{};

    if (!editingPath.empty())
    {
        // GetErrors takes a property name, not a path; use the leaf segment.
        const auto lastDot = std::wstring_view{ editingPath }.find_last_of(L'.');
        const auto propertyName = (lastDot == std::wstring_view::npos)
            ? editingPath
            : winrt::hstring{ std::wstring_view{ editingPath }.substr(lastDot + 1) };
        try
        {
            if (auto errors = errorInfo.GetErrors(propertyName))
            {
                for (auto const& error : errors)
                {
                    UNREFERENCED_PARAMETER(error);
                    return true;
                }
            }
            return false;
        }
        catch (...)
        {
            return false;
        }
    }

    try
    {
        return errorInfo.HasErrors();
    }
    catch (...)
    {
        return false;
    }
}

bool TableView::FinishEditTeardown(EditingUnit unit, winrt::TableViewEditAction action, bool honorCancel)
{
    const auto item = m_currentEditItem.get();
    const auto column = m_currentEditColumn.get();

    const auto editingElement = CurrentEditingElement();

    if (action == winrt::TableViewEditAction::Commit)
    {
        // A forced close already transferred the value (see TerminateEditForReset), so writing
        // again here would invoke the app's setters twice for one commit.
        const bool alreadyTransferred = !honorCancel;
        bool wrote = alreadyTransferred;

        if (!wrote && column && editingElement)
        {
            try
            {
                wrote = winrt::get_self<TableViewColumn>(column)->CommitCellEdit(editingElement);
                m_editSourceWritten = m_editSourceWritten || wrote;
            }
            catch (...)
            {
                LogConsumerHandlerThrow(L"CommitCellEdit");
                wrote = false;
            }
        }

        if (!wrote)
        {
            // Nothing reached the item - a setter threw, or no writable binding could be resolved.
            // Reporting success here is silent data loss, so the edit stays open instead.
            //
            // Logged because "the editor will not close" is otherwise indistinguishable from a
            // hang. The common cause is an editor the base cannot see: GetBindingExpression only
            // finds classic {Binding}, so a CellEditingTemplate using {x:Bind}, or any editor
            // outside the built-in property list, must override CommitCellEditCore.
            TVDiag::LogRetailF(
                L"[TableView] Commit wrote nothing, so the edit stays open. If this column uses a "
                L"CellEditingTemplate, override CommitCellEditCore - compiled {x:Bind} bindings and "
                L"custom editors are not discoverable by the base implementation.");
            return false;
        }

        // Validate AFTER the write - the pending value lives in the editor until the column
        // commits, so validating before it would only ever re-check the pre-edit value.
        if (honorCancel && HasBlockingValidationErrors(item, column))
        {
            if (column && editingElement)
            {
                // Undo the write. CancelCellEdit restores the EDITOR, and because the editing
                // binding is Explicit that alone never reaches the item - so commit the restored
                // value to push the pre-edit value back to the source. Without this second write
                // the item keeps the value the control just rejected, and the user's typed text is
                // replaced in the editor as well: invalid data on the item, nothing on screen.
                winrt::get_self<TableViewColumn>(column)->CancelCellEdit(editingElement, m_editUneditedValue.get());

                if (!winrt::get_self<TableViewColumn>(column)->CommitCellEdit(editingElement))
                {
                    TVDiag::LogRetailF(
                        L"[TableView] A rejected value could not be rolled back; the item still holds it.");
                }
                m_editSourceWritten = false;
            }
            return false;
        }
    }
    else
    {
        // Cancel. The column reverts just this editor, leaving cells already committed in
        // the row alone.
        if (column && editingElement)
        {
            // Only touch the editor when the source actually needs repairing.
            //
            // A plain cancel never reached the item - the editing binding is Explicit - and the
            // editor is torn out of the tree a few lines below, so restoring its text achieves
            // nothing visible. It is also actively harmful: mutating an in-tree editor's Text
            // changes the cell's desired width, which re-enters column width resolution during
            // teardown and fail-fasts with 0xc0000420 in an Auto-width column.
            //
            // The one case that DOES need it: a commit already wrote to the item and validation
            // then rejected the value, leaving the edit open. There the editor has to be restored
            // and re-committed so the item stops holding a value the control rejected.
            if (m_editSourceWritten)
            {
                try
                {
                    winrt::get_self<TableViewColumn>(column)->CancelCellEdit(editingElement, m_editUneditedValue.get());
                    winrt::get_self<TableViewColumn>(column)->CommitCellEdit(editingElement);
                }
                catch (...)
                {
                    LogConsumerHandlerThrow(L"CancelCellEdit");
                }

                m_editSourceWritten = false;
            }
        }
    }

    EndEditVisual(action);
    m_editUneditedValue.set(nullptr);

    m_currentEditItem.set(nullptr);
    m_currentEditColumn.set(nullptr);
    return true;
}

bool TableView::CompleteEditEnd(EditingUnit unit, winrt::TableViewEditAction action, bool honorCancel, bool vetoed)
{
    if (m_editState != EditState::Ending)
    {
        return false;
    }

    if (vetoed)
    {
        // Ending -> Editing: a handler kept the edit open.
        m_editState = EditState::Editing;
        ClearCoalescedEditReshape();
        return false;
    }

    auto const generation = m_editGeneration;

    const bool closed = FinishEditTeardown(unit, action, honorCancel);

    // A rollback write inside FinishEditTeardown raises PropertyChanged, which can re-enter layout
    // and drive a forced teardown (row rebuild / recycle) underneath this frame. That teardown has
    // already closed the edit and torn the editor out of the tree, so resurrecting the state here
    // would leave the control reporting IsEditing with no editor.
    if (m_editGeneration != generation)
    {
        return false;
    }

    m_editState = closed ? EditState::None : EditState::Editing;

    if (closed)
    {
        DrainCoalescedEditReshape();
    }
    else
    {
        ClearCoalescedEditReshape();
    }

    return closed;
}

bool TableView::EndCurrentEdit(EditingUnit unit, winrt::TableViewEditAction action, bool honorCancel)
{
    // Only an open, interactive edit can be closed: Beginning and Ending are both rejected here, so
    // a consumer callback cannot re-enter teardown.
    //
    // The exception is a forced close (honorCancel == false) while a previous close waits on a
    // deferral. That edit is already logically over and cannot be vetoed, so the caller must be able
    // to finish it now - otherwise an app that never closes its deferral wedges the control at
    // IsEditing == true and a recycled row keeps a live editor over a different item.
    //
    // The ending events are NOT raised again for it: the app has already seen CellEditEnding for
    // this edit, and a second one - potentially reporting a different action - would look like two
    // separate edits closing. The pending deferral is abandoned via the generation bump and the
    // teardown runs directly.
    if (m_editState == EditState::Ending && !honorCancel)
    {
        // Any late deferral completion must not run teardown a second time. The state is already
        // Ending, which is what CompleteEditEnd requires, so run the teardown directly.
        ++m_editGeneration;

        const bool closed = CompleteEditEnd(unit, action, honorCancel, false /* vetoed */);
        if (closed)
        {
            DrainCoalescedEditReshape();
        }
        return closed;
    }

    if (m_editState != EditState::Editing)
    {
        return false;
    }

    m_editState = EditState::Ending;

    const auto result = RaiseEditEnding(unit, action, honorCancel);

    return CompleteEditEnd(unit, action, honorCancel, result == EditEndingResult::Vetoed);
}

bool TableView::TerminateEditForReset(bool force)
{
    if (m_editState == EditState::None)
    {
        return true;
    }

    // Forced teardown (source-driven reset / ItemsSource replacement / unload) cannot be
    // vetoed: commit the pending value if it is valid, otherwise report Cancel and restore
    // the edit snapshot. Control-initiated (non-forced) reshapes stay cancelable.
    auto action = winrt::TableViewEditAction::Commit;
    if (force)
    {
        // Transfer the pending editor value to the source BEFORE validating. The editing
        // bindings use UpdateSourceTrigger::Explicit, so until this runs the typed value is
        // still sitting in the editor and validation would only ever re-check the pre-edit
        // value - and because a forced close passes honorCancel=false, the post-write
        // validation in FinishEditTeardown is skipped. Without this order a forced teardown
        // silently commits a value the control itself considers invalid.
        //
        // FinishEditTeardown must not repeat this write; it detects the forced case via
        // honorCancel == false.
        bool wrote = false;
        if (auto const column = m_currentEditColumn.get())
        {
            if (auto const editingElement = CurrentEditingElement())
            {
                try
                {
                    wrote = winrt::get_self<TableViewColumn>(column)->CommitCellEdit(editingElement);
                }
                catch (...)
                {
                    LogConsumerHandlerThrow(L"CommitCellEdit");
                }
            }
        }

        if (!wrote || HasBlockingValidationErrors(m_currentEditItem.get(), m_currentEditColumn.get()))
        {
            // Cancel: FinishEditTeardown's non-commit path reverts the editor, undoing the
            // transfer above.
            action = winrt::TableViewEditAction::Cancel;
        }
    }

    return EndCurrentEdit(
        EditingUnit::Row,
        action,
        !force);
}

bool TableView::TryTerminateEditForControlInitiatedReshape()
{
    if (m_editState == EditState::None)
    {
        return true;
    }

    // Already inside a transition (a consumer callback, or a deferral in flight): the reshape
    // cannot proceed now. The caller queues it and it is replayed once the edit closes.
    if (m_editState != EditState::Editing)
    {
        return false;
    }

    const bool terminated = TerminateEditForReset(false /* force */);
    if (!terminated)
    {
        ClearCoalescedEditReshape();
        return false;
    }

    // Reentrant reshape requests raised by edit-ending handlers are queued and drained by the
    // outer operation after it applies. This preserves the original outer intent (A) and then
    // replays reentrant intents (B, C...) in arrival order.
    return terminated;
}

void TableView::QueueCoalescedEditReshape(std::function<void()> operation)
{
    if (operation)
    {
        m_pendingEditReshapes.push_back(std::move(operation));
    }
}

void TableView::ClearCoalescedEditReshape()
{
    m_pendingEditReshapes.clear();
}

void TableView::DrainCoalescedEditReshape()
{
    // Never replay inside a layout pass. The queued operations are app-supplied callables that
    // reshape the source; running one from ItemsRepeater's element-clearing/measure callback
    // re-enters the repeater mid-pass. m_suppressEditVisualRestore is exactly the "we are inside
    // layout" signal, so it gates this too.
    if (m_suppressEditVisualRestore)
    {
        return;
    }

    if (m_isApplyingCoalescedEditReshape || m_pendingEditReshapes.empty())
    {
        return;
    }

    m_isApplyingCoalescedEditReshape = true;
    auto applyGuard = wil::scope_exit([this]() noexcept { m_isApplyingCoalescedEditReshape = false; });

    // Operations are opaque callables, so editing has no compile-time knowledge of sorting,
    // grouping or row expansion; the caller that deferred the work supplies the intent.
    while (!m_pendingEditReshapes.empty())
    {
        auto operation = std::move(m_pendingEditReshapes.front());
        m_pendingEditReshapes.pop_front();
        if (operation)
        {
            operation();
        }
    }
}

bool TableView::BeginEdit()
{
    winrt::IInspectable item{ nullptr };
    winrt::TableViewColumn column{ nullptr };
    if (!TryResolveCurrentCellForEdit(item, column))
    {
        return false;
    }

    return BeginEdit(item, column);
}

bool TableView::BeginEdit(winrt::IInspectable const& item, winrt::TableViewColumn const& column)
{
    if (IsReadOnly() || !item || !column || column.IsReadOnly() || !IsColumnInColumns(*this, column))
    {
        return false;
    }

    // Reject reentrancy from inside a consumer callback, and while a deferred close is in flight.
    if (m_editState == EditState::Beginning || m_editState == EditState::Ending)
    {
        return false;
    }

    if (m_editState == EditState::Editing)
    {
        // Moving commits the open edit. WPF parity: leaving the *row* is what ends the row-level
        // transaction, so moving to a different item commits with unit Row - which is what raises
        // RowEditEnding and calls ITableViewEditableItem.EndEdit at row scope. Moving between cells
        // of the same row stays a Cell-unit commit.
        //
        // A veto, a validation failure or a deferral all leave the existing edit open, so the move
        // must not proceed.
        const bool movingToAnotherItem =
            !SameInspectableIdentity(m_currentEditItem.get(), item);

        const auto unit = movingToAnotherItem
            ? EditingUnit::Row
            : EditingUnit::Cell;

        if (!CommitEditInternal(unit))
        {
            return false;
        }
    }

    m_currentEditItem.set(item);
    m_currentEditColumn.set(column);
    m_editState = EditState::Beginning;
    m_abandonPendingBeginEdit = false;
    m_editSourceWritten = false;

    // Everything from here to the promotion below runs app code - a BeginningEdit handler, the
    // editing binding's getter, ITableViewEditableItem.BeginEdit - any of which can mutate the
    // collection and have ItemsRepeater recycle the row underneath us. A forced teardown cannot
    // close an edit that is still Beginning, so OnRowElementClearing raises this flag instead and
    // the checks below unwind rather than promoting to Editing over a recycled row.
    const bool allowed = RaiseBeginningEdit(item, column);
    if (!allowed || m_abandonPendingBeginEdit || !TryBeginEditVisual(item, column))
    {
        // Beginning -> None. Do not leave stale trackers behind, and reset the state before
        // draining so a queued reshape can itself start an edit.
        //
        // The item transaction is deliberately NOT touched: a cell commit keeps the row
        // transaction open, so anything held here belongs to a previous cell in the same row and
        // is still legitimately live. Clearing it would leave the item with an unpaired BeginEdit.
        m_currentEditItem.set(nullptr);
        m_currentEditColumn.set(nullptr);
        m_editUneditedValue.set(nullptr);
        m_editState = EditState::None;
        m_abandonPendingBeginEdit = false;
        DrainCoalescedEditReshape();
        return false;
    }

    // The column primes its own editor and hands back whatever it needs to revert. Done here rather
    // than in the row so the returned value has an owner for the lifetime of the edit.
    if (auto const row = m_currentEditRow.get())
    {
        if (auto const editingElement = winrt::get_self<TableViewRow>(row)->GetEditingElement())
        {
            try
            {
                m_editUneditedValue.set(winrt::get_self<TableViewColumn>(column)->PrepareCellForEdit(editingElement, nullptr));
            }
            catch (...)
            {
                LogConsumerHandlerThrow(L"PrepareCellForEdit");
            }
        }
    }

    // Last chance to unwind: PrepareCellForEdit calls app code, so
    // the row can have been recycled between the check above and here. Promoting to Editing over a
    // recycled row is what lets a later commit write into a different item.
    if (m_abandonPendingBeginEdit || !m_currentEditRow.get())
    {
        m_abandonPendingBeginEdit = false;
        m_currentEditItem.set(nullptr);
        m_currentEditColumn.set(nullptr);
        m_editUneditedValue.set(nullptr);
        m_editState = EditState::None;
        DrainCoalescedEditReshape();
        return false;
    }

    SetCurrentCell(item, column);
    m_editState = EditState::Editing;
    DrainCoalescedEditReshape();
    return true;
}

bool TableView::CommitEdit()
{
    return CommitEditInternal(EditingUnit::Cell);
}

bool TableView::CommitEditInternal(EditingUnit unit)
{
    return EndCurrentEdit(unit, winrt::TableViewEditAction::Commit, true /* honorCancel */);
}

bool TableView::CancelEdit()
{
    return CancelEditInternal(EditingUnit::Cell);
}

bool TableView::CancelEditInternal(EditingUnit unit)
{
    // Normal cancel is cancelable per spec (only FORCED teardown is non-cancelable):
    // a CellEditEnding handler may veto the cancel and keep the edit open.
    return EndCurrentEdit(unit, winrt::TableViewEditAction::Cancel, true /* honorCancel */);
}
