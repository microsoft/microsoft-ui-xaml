// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewRow.h"

#include <utility>
#include <vector>

// Row selection: single-item, row-scoped.
//
// SelectionModel owns the selected index and reconciles it across collection changes - the same
// component ItemsView uses. This file is the layer around it: the DP projections, the row chrome,
// the gestures and the automation events. SelectedItem and SelectedIndex are read-only to apps,
// so this control is their only writer.

namespace
{
    // Restores the previous value rather than clearing, so nesting cannot unlatch an outer scope.
    struct ScopedFlag
    {
        ScopedFlag(bool& flag, bool value) noexcept : m_flag(flag), m_previous(flag) { m_flag = value; }
        ~ScopedFlag() { m_flag = m_previous; }

        ScopedFlag(ScopedFlag const&) = delete;
        ScopedFlag& operator=(ScopedFlag const&) = delete;

    private:
        bool& m_flag;
        bool m_previous;
    };
}

bool TableView::CanSelectRows()
{
    return SelectionMode() != winrt::TableViewSelectionMode::None;
}

bool TableView::HasRowsSource() const
{
    if (auto const repeater = m_rowsRepeater.get())
    {
        return repeater.ItemsSourceView() != nullptr;
    }

    return false;
}

// ----- SelectionModel plumbing -----

void TableView::EnsureSelectionModel()
{
    if (m_selectionModel)
    {
        return;
    }

    m_selectionModel = winrt::SelectionModel{};
    m_selectionModel.SingleSelect(true);

    auto weakThis = get_weak();
    m_selectionModelChangedRevoker = m_selectionModel.SelectionChanged(
        winrt::auto_revoke,
        [weakThis](winrt::SelectionModel const& sender, winrt::SelectionModelSelectionChangedEventArgs const& args)
        {
            if (auto strongThis = weakThis.get())
            {
                strongThis->OnSelectionModelSelectionChanged(sender, args);
            }
        });
}

void TableView::UpdateSelectionModelSource()
{
    if (!m_selectionModel)
    {
        return;
    }

    winrt::IInspectable rowsSource{ nullptr };
    if (auto const repeater = m_rowsRepeater.get())
    {
        // The repeater's view, not the raw source: SelectionNode reuses an ItemsSourceView it is
        // handed, so the model and the repeater observe one shared view in one subscription order
        // instead of racing two independent views over the same collection.
        rowsSource = repeater.ItemsSourceView();
    }

    // Source has no identity short-circuit: assigning the same source again clears the selection.
    if (m_selectionModel.Source() != rowsSource)
    {
        m_selectionModel.Source(rowsSource);
    }
}

int32_t TableView::SelectedIndexInternal() const
{
    if (m_selectionModel)
    {
        // Flat source, so the path is one deep when there is a selection.
        if (auto const path = m_selectionModel.SelectedIndex(); path && path.GetSize() > 0)
        {
            return path.GetAt(0);
        }
    }

    return -1;
}

winrt::IInspectable TableView::SelectedItemInternal() const
{
    return SelectedItemForIndex(SelectedIndexInternal());
}

winrt::IInspectable TableView::SelectedItemForIndex(int32_t index) const
{
    // SelectionModel::SelectedItem indexes its source without a bounds check, and the model can be
    // momentarily ahead of or behind the collection while a change is being dispatched.
    if (!m_selectionModel || index < 0 || index >= GetItemsSourceCount())
    {
        return nullptr;
    }

    return m_selectionModel.SelectedItem();
}

// ----- Deferred reload request -----
//
// Only one thing defers now: unload drains the repeater's source, and re-sourcing on load hands
// SelectionModel a new view, which clears it. The selected item is held across that round trip.
// ItemsView needs no equivalent because it never drains its repeater on unload.

bool TableView::ShouldDeferSelectionRequest()
{
    return !CanSelectRows() || !HasRowsSource();
}

void TableView::ClearPendingSelection()
{
    m_pendingSelectedItem.set(nullptr);
}

bool TableView::DrainPendingSelection()
{
    if (ShouldDeferSelectionRequest())
    {
        return false;
    }

    if (auto const pendingItem = m_pendingSelectedItem.get())
    {
        ClearPendingSelection();
        ApplySelection(IndexOfItem(pendingItem));
        return true;
    }

    return false;
}

int32_t TableView::IndexOfItem(winrt::IInspectable const& item) const
{
    if (!item)
    {
        return -1;
    }

    auto const repeater = m_rowsRepeater.get();
    if (!repeater)
    {
        return -1;
    }

    auto const view = repeater.ItemsSourceView();
    if (!view)
    {
        return -1;
    }

    // SelectionModel indexes but does not look items up, so resolving a held item on reload still
    // needs this. Linear scan: it runs at most once per reload.
    auto const target = UnwrapEditingDataItem(item);
    const int32_t count = view.Count();
    for (int32_t index = 0; index < count; ++index)
    {
        // Identity, not ABI-pointer equality: the same object reached through a different
        // interface (boxed values, projected interfaces) must still match.
        if (auto const candidate = UnwrapEditingDataItem(view.GetAt(index));
            candidate && SameInspectableIdentity(candidate, target))
        {
            return index;
        }
    }

    return -1;
}

// ----- The single writer -----

void TableView::ApplySelection(int32_t index)
{
    if (!CanSelectRows())
    {
        index = -1;
    }

    if (index >= 0 && index >= GetItemsSourceCount())
    {
        index = -1;
    }

    if (index < 0 && !m_selectionModel)
    {
        // Nothing selected and no model yet - nothing to clear, but publish so the projections
        // start out agreeing with the model.
        PushSelectionProperties();
        return;
    }

    EnsureSelectionModel();

    if (index < 0)
    {
        m_selectionModel.ClearSelection();
    }
    else
    {
        m_selectionModel.Select(index);
    }

    // The model raises SelectionChanged only when the selection actually moved; publish here too so
    // a rejected write still leaves the DPs agreeing with the model.
    PushSelectionProperties();
}

void TableView::OnSelectionModelSelectionChanged(
    const winrt::SelectionModel& /*sender*/,
    const winrt::SelectionModelSelectionChangedEventArgs& /*args*/)
{
    const int32_t newIndex = SelectedIndexInternal();
    auto const newItem = SelectedItemInternal();

    auto const deselectedRow = FindRealizedRowForIndex(m_lastPublishedIndex);
    auto const selectedRow = FindRealizedRowForIndex(newIndex);
    m_lastPublishedIndex = newIndex;

    // Arm the guard BEFORE any DP write. Both the row IsSelected pushes below and
    // PushSelectionProperties notify synchronously, so an observer can select something else from
    // inside either one. When that happens the nested pass has already published and raised for the
    // newer selection; finishing this one would overwrite it and raise a bogus delta.
    const uint32_t version = ++m_selectionVersion;

    // Restamp before notifying, so a handler that walks the rows sees settled chrome.
    if (deselectedRow)
    {
        winrt::get_self<TableViewRow>(deselectedRow)->SetIsSelectedInternal(false);
    }

    if (selectedRow)
    {
        winrt::get_self<TableViewRow>(selectedRow)->SetIsSelectedInternal(true);
    }

    if (m_selectionVersion != version)
    {
        return;
    }

    // While a reload is being restored, SelectionModel::Source clears before the stashed item is
    // re-selected. Chrome is already settled above; suppressing the rest keeps that round trip from
    // publishing a transient null and raising a clear-then-reselect pair for a selection that never
    // actually changed. ResolveSelectionAfterSourceChange publishes once when it finishes.
    if (m_isRestoringSelection)
    {
        return;
    }

    PushSelectionProperties();
    if (m_selectionVersion != version)
    {
        return;
    }

    RaiseSelectionAutomationEvents(deselectedRow, selectedRow);
    RaiseSelectionChanged(newItem);
}

void TableView::PushSelectionProperties()
{
    // The properties are read-only to apps, so this is the only writer and there is no echo to
    // suppress and nothing to re-assert against an observer writing back.
    // Read the index once and derive the item from it: each SelectedIndexInternal() call builds an
    // IndexPath, and SelectedItemInternal() would read it again.
    const int32_t index = SelectedIndexInternal();
    auto const item = SelectedItemForIndex(index);

    if (SelectedIndex() != index)
    {
        SelectedIndex(index);
    }

    if (!SameInspectableIdentity(SelectedItem(), item))
    {
        SelectedItem(item);
    }
}

winrt::TableViewRow TableView::FindRealizedRowForIndex(int32_t index)
{
    if (index < 0)
    {
        return nullptr;
    }

    if (auto const repeater = m_rowsRepeater.get())
    {
        // TryGetElement only returns containers the repeater currently considers realized, so this
        // cannot hand back a pooled ghost.
        return repeater.TryGetElement(index).try_as<winrt::TableViewRow>();
    }

    return nullptr;
}

void TableView::RaiseSelectionChanged(winrt::IInspectable const& addedItem)
{
    // `removed` is the last item actually REPORTED, so a superseded transition's removal still
    // surfaces exactly once rather than being lost with the suppressed event.
    //
    // Single-selection only: the delta is derived from one cached item. Under Multiple this
    // short-circuit would SILENTLY suppress a real change whose first item happened not to move -
    // selecting a second row while the first stays selected. Multiple must diff the selected sets
    // instead, the way Selector does; SelectionModelSelectionChangedEventArgs carries no delta.
    auto const removedItem = m_lastRaisedSelectedItem.get();
    if (SameInspectableIdentity(removedItem, addedItem))
    {
        return;
    }

    m_lastRaisedSelectedItem.set(addedItem);

    std::vector<winrt::IInspectable> added;
    if (addedItem)
    {
        added.push_back(addedItem);
    }

    std::vector<winrt::IInspectable> removed;
    if (removedItem)
    {
        removed.push_back(removedItem);
    }

    // The platform args type, not a bespoke one, so a handler can be shared with ListView.
    // Constructor order is (removedItems, addedItems).
    auto const args = winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs{
        winrt::single_threaded_vector<winrt::IInspectable>(std::move(removed)),
        winrt::single_threaded_vector<winrt::IInspectable>(std::move(added)) };

    m_selectionChangedEventSource(*this, args);
}

void TableView::RaiseSelectionAutomationEvents(
    winrt::TableViewRow const& deselectedRow,
    winrt::TableViewRow const& selectedRow)
{
    // Container-level first: it is the only signal available when the selected row is unrealized
    // and there is no row peer to raise a per-element event on.
    if (winrt::AutomationPeer::ListenerExists(winrt::AutomationEvents::SelectionPatternOnInvalidated))
    {
        if (auto const peer = winrt::FrameworkElementAutomationPeer::FromElement(*this))
        {
            peer.RaiseAutomationEvent(winrt::AutomationEvents::SelectionPatternOnInvalidated);
        }
    }

    // FromElement returns an existing peer or null - it never forces one into existence.
    if (selectedRow &&
        winrt::AutomationPeer::ListenerExists(winrt::AutomationEvents::SelectionItemPatternOnElementSelected))
    {
        if (auto const peer = winrt::FrameworkElementAutomationPeer::FromElement(selectedRow))
        {
            peer.RaiseAutomationEvent(winrt::AutomationEvents::SelectionItemPatternOnElementSelected);
        }
    }

    if (deselectedRow &&
        winrt::AutomationPeer::ListenerExists(winrt::AutomationEvents::SelectionItemPatternOnElementRemovedFromSelection))
    {
        if (auto const peer = winrt::FrameworkElementAutomationPeer::FromElement(deselectedRow))
        {
            peer.RaiseAutomationEvent(winrt::AutomationEvents::SelectionItemPatternOnElementRemovedFromSelection);
        }
    }

    // Narrator keys off the IsSelected property change, not only the pattern events above;
    // TreeViewItem raises both for the same reason.
    if (winrt::AutomationPeer::ListenerExists(winrt::AutomationEvents::PropertyChanged))
    {
        if (selectedRow)
        {
            if (auto const peer = winrt::FrameworkElementAutomationPeer::FromElement(selectedRow))
            {
                peer.RaisePropertyChangedEvent(
                    winrt::SelectionItemPatternIdentifiers::IsSelectedProperty(),
                    box_value(false),
                    box_value(true));
            }
        }

        if (deselectedRow)
        {
            if (auto const peer = winrt::FrameworkElementAutomationPeer::FromElement(deselectedRow))
            {
                peer.RaisePropertyChangedEvent(
                    winrt::SelectionItemPatternIdentifiers::IsSelectedProperty(),
                    box_value(true),
                    box_value(false));
            }
        }
    }
}

// ----- Property-changed callbacks -----

void TableView::OnSelectionModePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& /*args*/)
{
    if (!CanSelectRows())
    {
        // Turning selection off is an explicit app action: clear, and drop anything held for a
        // reload so it cannot resurrect after the app asked for nothing to be selected.
        ClearPendingSelection();
        ApplySelection(-1);
    }
}


// ----- Source changes -----

void TableView::UpdateSelectionCollectionChangedSubscription()
{
    // auto_revoke drops the prior source's subscription.
    m_selectionCollectionChangedRevoker = {};

    if (auto const repeater = m_rowsRepeater.get())
    {
        if (auto const view = repeater.ItemsSourceView())
        {
            m_selectionCollectionChangedRevoker = view.CollectionChanged(
                winrt::auto_revoke, { this, &TableView::OnSelectionItemsSourceCollectionChanged });
        }
    }
}

void TableView::OnSelectionItemsSourceCollectionChanged(
    const winrt::IInspectable& /*sender*/,
    const winrt::IInspectable& /*args*/)
{
    // SelectionModel has already reconciled the index - and deliberately does not raise when an
    // insert merely shifts it. All that is left is the chrome: ItemsRepeater re-indexes its
    // realized containers around now, so re-derive every one of them rather than trusting the
    // index any single row was last stamped against.
    m_lastPublishedIndex = SelectedIndexInternal();
    RestampAllRealizedRowSelection(m_lastPublishedIndex);
    PushSelectionProperties();
}

void TableView::RestampAllRealizedRowSelection()
{
    RestampAllRealizedRowSelection(SelectedIndexInternal());
}

void TableView::RestampAllRealizedRowSelection(int32_t selectedIndex)
{
    // The index is passed in because SelectedIndexInternal() builds an IndexPath per call, and this
    // runs once per realized row on every collection notification.
    ForEachRealizedRow([this, selectedIndex](winrt::TableViewRow const& row)
        {
            RefreshRowSelectionState(row, selectedIndex);
        });
}

void TableView::StashSelectionForReload()
{
    if (m_pendingSelectedItem.get())
    {
        return;
    }

    if (auto const item = SelectedItemInternal())
    {
        m_pendingSelectedItem.set(item);
    }
}

void TableView::ResolveSelectionAfterSourceChange()
{
    // Create the model before anything else subscribes. UpdateSelectionModelSource is a no-op while
    // the model is null, so leaving it lazy here would let this control register on the shared view
    // first and restamp rows from a not-yet-reconciled index. ItemsView constructs its model inline
    // for the same reason.
    EnsureSelectionModel();

    const bool hasStashedSelection = m_pendingSelectedItem.get() != nullptr;

    {
        // Setting Source clears, and the drain then re-selects. Publishing between the two would
        // emit a transient null and a clear-then-reselect event pair for a selection that did not
        // actually change, so the whole restore is published once, at the end.
        ScopedFlag const restoring{ m_isRestoringSelection, hasStashedSelection };

        // Model first: it must be subscribed to the shared view ahead of this control, so that by
        // the time OnSelectionItemsSourceCollectionChanged restamps rows the index is reconciled.
        UpdateSelectionModelSource();
        UpdateSelectionCollectionChangedSubscription();

        // A selection held across a reload outranks the live one: the live value was resolved
        // against the source being replaced, whereas the held item is what was selected before.
        DrainPendingSelection();
    }

    m_lastPublishedIndex = SelectedIndexInternal();
    PushSelectionProperties();
    RestampAllRealizedRowSelection(m_lastPublishedIndex);

    // Only raises when the item actually differs from the last one reported, so a round trip that
    // restores the same selection stays silent.
    RaiseSelectionChanged(SelectedItemInternal());
}

// ----- Row plumbing -----

void TableView::RefreshRowSelectionState(winrt::TableViewRow const& row)
{
    RefreshRowSelectionState(row, SelectedIndexInternal());
}

void TableView::RefreshRowSelectionState(winrt::TableViewRow const& row, int32_t selectedIndex)
{
    if (!row)
    {
        return;
    }

    bool isSelected = false;
    if (selectedIndex >= 0)
    {
        if (auto const repeater = m_rowsRepeater.get())
        {
            // By INDEX, not item identity: a collection may hold the same object twice, and an
            // identity match would light up every row showing it.
            isSelected = repeater.GetElementIndex(row) == selectedIndex;
        }
    }

    winrt::get_self<TableViewRow>(row)->SetIsSelectedInternal(isSelected);
}

void TableView::OnRowPointerSelect(winrt::TableViewRow const& row)
{
    if (!row)
    {
        return;
    }

    if (auto const repeater = m_rowsRepeater.get())
    {
        const auto index = repeater.GetElementIndex(row);
        if (index >= 0)
        {
            // SelectRowIndexFromInteraction gates on SelectionMode.
            SelectRowIndexFromInteraction(index);
        }
    }
}

void TableView::SelectRowIndexFromInteraction(int32_t index)
{
    // Ctrl toggles, matching SingleSelector::OnInteractedAction and ListViewBase. Without it there
    // is no pointer or keyboard gesture that can clear a selection once one is made - the app would
    // have to call DeselectAll. Read live rather than off the args, as ItemsView's interaction
    // layer does, so the row's pointer handlers do not have to carry modifier state.
    const bool isControlDown =
        (winrt::InputKeyboardSource::GetKeyStateForCurrentThread(winrt::VirtualKey::Control) &
            winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down;

    SelectRowIndexFromInteraction(index, isControlDown);
}

void TableView::SelectRowIndexFromInteraction(int32_t index, bool toggle)
{
    if (!CanSelectRows())
    {
        // A user gesture while selection is off is a no-op.
        return;
    }

    // Group headers share the flat projection's index space with data rows but are not selectable.
    // Keyboard navigation legitimately lands focus on a header (to expand/collapse it); when it
    // does, leave the existing data selection untouched rather than moving it to - or clearing it
    // against - a header index. Selection-follows-focus resumes on the next data row.
    if (index >= 0 && IsGroupHeaderRow(index))
    {
        return;
    }

    // An explicit gesture settles the question - drop anything held for a reload.
    ClearPendingSelection();

    if (toggle && IsSelected(index))
    {
        ApplySelection(-1);
        return;
    }

    ApplySelection(index);
}

// ----- Public API -----

void TableView::Select(int32_t index)
{
    if (index < 0)
    {
        // Explicit "select nothing".
        DeselectAll();
        return;
    }

    // Reject rather than coerce. ApplySelection turns an unresolvable index into "clear", which is
    // right for a coercion path but wrong here: Select(999) must not wipe an existing selection.
    // ItemsView::Select is a straight pass-through to SelectionModel and never clears either.
    if (!CanSelectRows() || index >= GetItemsSourceCount())
    {
        return;
    }

    ApplySelection(index);
}

void TableView::Deselect(int32_t index)
{
    // Only clears when `index` IS the selection, so a stale call cannot clobber a newer one.
    if (IsSelected(index))
    {
        ApplySelection(-1);
    }
}

bool TableView::IsSelected(int32_t index)
{
    // Ask the model rather than comparing against the single selected index, so this stays correct
    // when Multiple lands. The guard is required: SelectionModel::IsSelected asserts on index < 0.
    if (index < 0 || !m_selectionModel || index >= GetItemsSourceCount())
    {
        return false;
    }

    auto const selected = m_selectionModel.IsSelected(index);
    return selected && selected.Value();
}

void TableView::DeselectAll()
{
    ClearPendingSelection();
    ApplySelection(-1);
}
