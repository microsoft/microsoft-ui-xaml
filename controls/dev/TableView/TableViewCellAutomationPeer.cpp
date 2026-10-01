// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewTextColumn.h"
#include "TableViewRow.h"
#include "TableViewAutomationPeer.h"
#include "TableViewColumnHeaderAutomationPeer.h"
#include "TableViewCellAutomationPeer.h"
#include "TableViewAutomationHelpers.h"
#include "TableViewToolTipHelpers.h"
#include "ResourceAccessor.h"
#include "TableViewCellAutomationPeer.properties.cpp"
#include "TVDiag.h"

#include <UIAutomationCore.h>
#include <UIAutomationCoreApi.h>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

TableViewCellAutomationPeer::TableViewCellAutomationPeer(
    winrt::FrameworkElement const& cell,
    winrt::TableViewRow const& row,
    winrt::TableViewColumn const& column,
    int32_t columnIndex)
    : ReferenceTracker(cell)
    , m_columnIndex(columnIndex)
{
    // The cell owner supplies bounds; weak refs avoid extending row/column lifetimes.
    if (row)
    {
        m_row = winrt::make_weak(row);
    }
    if (column)
    {
        m_column = winrt::make_weak(column);
    }
    if (row)
    {
        if (auto const tableView = winrt::get_self<TableViewRow>(row)->GetOwningTableView())
        {
            TrackRowItem(row, tableView);
        }
    }
    // Primes m_lastOwningTable / m_lastKnownRowIndex for IsVirtualized(). It has to happen while
    // the row is still in the tree: once the row is recycled the index is unresolvable, and a
    // client may not query VirtualizedItem until after that point.
    std::ignore = GetRowIndex();
}

winrt::IInspectable TableViewCellAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    // GridItem + TableItem are structural and always meaningful for realized cells.
    if (patternInterface == winrt::PatternInterface::GridItem ||
        patternInterface == winrt::PatternInterface::TableItem)
    {
        return *this;
    }

    // Offered only where SetValue can honour it: the cell must be editable and its column must
    // produce a TextBox editor. Advertising it elsewhere tells assistive technology it can set a
    // value, then fails after opening an edit.
    if (patternInterface == winrt::PatternInterface::Value && SupportsValuePattern())
    {
        if (auto const row = m_row.get(); row && GetTrackedTableForRow(row))
        {
            return *this;
        }
    }

    if (patternInterface == winrt::PatternInterface::VirtualizedItem && IsVirtualized())
    {
        return *this;
    }

    return __super::GetPatternCore(patternInterface);
}

hstring TableViewCellAutomationPeer::GetClassNameCore()
{
    return L"TableViewCell";
}

winrt::AutomationControlType TableViewCellAutomationPeer::GetAutomationControlTypeCore()
{
    // DataItem lets Narrator read the composed cell name instead of a generic container.
    return winrt::AutomationControlType::DataItem;
}

hstring TableViewCellAutomationPeer::GetLocalizedControlTypeCore()
{
    // A host app may not merge the control's PRI; degrade to the framework default rather than
    // throwing into UIA.
    if (auto const localized = TryGetLocalizedString(SR_TableViewCellLocalizedControlType); !localized.empty())
    {
        return localized;
    }

    return __super::GetLocalizedControlTypeCore();
}

// UIA focusability is not Tab reachability: row-level cells still need SetFocus and
// IsKeyboardFocusable to succeed when enabled and visible.
bool TableViewCellAutomationPeer::IsKeyboardFocusableCore()
{
    auto const cell = Owner().try_as<winrt::FrameworkElement>();
    if (!cell || cell.Visibility() != winrt::Visibility::Visible || IsVirtualized())
    {
        return false;
    }

    // The cell wrapper is a Grid, so IsEnabled lives on the owning row, not on the cell.
    auto const row = m_row.get();
    if (row && !GetTrackedTableForRow(row))
    {
        return false;
    }
    return !row || row.IsEnabled();
}

void TableViewCellAutomationPeer::SetFocusCore()
{
    auto const row = m_row.get();
    if (!row || !GetTrackedTableForRow(row))
    {
        ThrowElementNotAvailable();
    }

    auto const cell = row ? GetRealizedCellFromRow(row) : nullptr;

    if (row && cell)
    {
        // Drill the row in first, exactly as Right does: the cell is not focusable at all while the
        // row is at row level, so a bare Focus() here would silently do nothing.
        winrt::get_self<TableViewRow>(row)->SetCellLevelInternal(true);
        if (cell.Focus(winrt::FocusState::Programmatic))
        {
            return;
        }
    }

    __super::SetFocusCore();
}

hstring TableViewCellAutomationPeer::GetNameCore()
{
    auto const row = m_row.get();
    auto const tableView = row ? GetTrackedTableForRow(row) : nullptr;
    if (row && !tableView)
    {
        ThrowElementNotAvailable();
    }

    UpdateNameItem(row && tableView
        ? row.DataContext() : nullptr);
    if (m_editName)
    {
        return *m_editName;
    }
    if (row)
    {
        if (auto const display = winrt::get_self<TableViewRow>(row)->GetDisplayElementForAutomation(Owner()))
        {
            auto const name = ReadDisplayName(display.try_as<winrt::FrameworkElement>());
            m_lastName = name;
            return name;
        }
    }
    auto const name = ReadDisplayName();
    m_lastName = name;
    return name;
}

winrt::hstring TableViewCellAutomationPeer::ReadNameForEdit()
{
    auto const row = m_row.get();
    auto const tableView = row ? GetTrackedTableForRow(row) : nullptr;
    if (row && !tableView)
    {
        ThrowElementNotAvailable();
    }

    UpdateNameItem(row && tableView
        ? row.DataContext() : nullptr);
    auto const name = ReadDisplayName();
    m_lastName = name;
    return name;
}

void TableViewCellAutomationPeer::UpdateNameItem(winrt::IInspectable const& item)
{
    if (!TableView::SameInspectableIdentity(m_nameItem.get(), item))
    {
        ResetEditName();
        m_nameItem.set(item);
    }
}

void TableViewCellAutomationPeer::BeginEditName()
{
    ++m_nameGeneration;
    m_nameLayoutUpdatedRevoker.revoke();
    m_editName = m_lastName;
}

void TableViewCellAutomationPeer::ResetEditName()
{
    ++m_nameGeneration;
    m_nameLayoutUpdatedRevoker.revoke();
    m_editName.reset();
    m_lastName.reset();
    m_nameItem.set(nullptr);
}

void TableViewCellAutomationPeer::EndEditName()
{
    m_nameLayoutUpdatedRevoker.revoke();
    auto const generation = m_nameGeneration;
    auto const weakThis = get_weak();
    auto const cell = Owner().try_as<winrt::FrameworkElement>();
    if (!cell)
    {
        m_editName.reset();
        return;
    }
    auto cleanupOnFailure = wil::scope_exit([this]() noexcept
    {
        m_nameLayoutUpdatedRevoker.revoke();
        m_editName.reset();
    });

    // Released by whichever of the three paths below resolves first. Owned by the backstop's
    // lambda, so the subscription cannot outlive one frame.
    auto const unloadRevoker = std::make_shared<winrt::FrameworkElement::Unloaded_revoker>();

    // LayoutUpdated precedes the framework's automatic-property pass. Queue
    // release from that event, leaving the held old Name intact for the pass.
    m_nameLayoutUpdatedRevoker = cell.LayoutUpdated(winrt::auto_revoke,
        [weakThis, generation](auto const&, auto const&)
        {
            if (auto const peer = weakThis.get(); peer && peer->m_nameGeneration == generation)
            {
                peer->m_nameLayoutUpdatedRevoker.revoke();
                try
                {
                    peer->QueueFinalName(generation);
                }
                catch (...)
                {
                    peer->m_editName.reset();
                    TVDiag::LogRetailF(L"[TableView] Optional post-layout name publication could not be queued.");
                }
            }
        });

    // An unloaded, collapsed, or hidden-column cell never sees another LayoutUpdated, so the pinned
    // pre-edit name would otherwise be returned by GetNameCore forever.
    *unloadRevoker = cell.Unloaded(winrt::auto_revoke,
        [weakThis, generation](auto const&, auto const&)
        {
            if (auto const peer = weakThis.get(); peer && peer->m_nameGeneration == generation)
            {
                peer->m_nameLayoutUpdatedRevoker.revoke();
                peer->m_editName.reset();
                try
                {
                    peer->InvalidatePeer();
                }
                catch (...)
                {
                    TVDiag::LogRetailF(L"[TableView] Optional cell-name invalidation on unload failed.");
                }
            }
        });

    cell.InvalidateMeasure();

    // Backstop: LayoutUpdated is not guaranteed to run for this cell at all. An armed layout
    // revoker at end of frame means it did not, so release rather than stay pinned.
    auto const queue = DispatcherQueue();
    if (!queue || !queue.TryEnqueue(winrt::DispatcherQueuePriority::Low,
        [weakThis, generation, unloadRevoker]()
        {
            unloadRevoker->revoke();
            if (auto const peer = weakThis.get();
                peer && peer->m_nameGeneration == generation && peer->m_nameLayoutUpdatedRevoker)
            {
                peer->m_nameLayoutUpdatedRevoker.revoke();
                peer->m_editName.reset();
                try
                {
                    peer->InvalidatePeer();
                }
                catch (...)
                {
                    TVDiag::LogRetailF(L"[TableView] Optional cell-name backstop invalidation failed.");
                }
            }
        }))
    {
        // Without the backstop the one-frame bound cannot be honoured, so do not pin at all.
        TVDiag::LogRetailF(L"[TableView] Optional cell-name release backstop could not be queued.");
        return;
    }

    cleanupOnFailure.release();
}

void TableViewCellAutomationPeer::QueueFinalName(uint64_t generation)
{
    auto const weakThis = get_weak();
    auto const queue = DispatcherQueue();
    if (queue && queue.TryEnqueue(winrt::DispatcherQueuePriority::Low, [weakThis, generation]()
    {
        if (auto const peer = weakThis.get(); peer && peer->m_nameGeneration == generation)
        {
            try
            {
                auto const row = peer->m_row.get();
                auto const cell = peer->Owner();
                if (!row || !winrt::get_self<TableViewRow>(row)->GetOwningTableView() ||
                    !TableView::SameInspectableIdentity(row.DataContext(), peer->m_nameItem.get()) ||
                    winrt::VisualTreeHelper::GetParent(cell).try_as<winrt::Panel>() !=
                        winrt::get_self<TableViewRow>(row)->GetCellsHostPanelInternal())
                {
                    peer->ResetEditName();
                    return;
                }
                peer->m_editName.reset();
                peer->InvalidatePeer();
            }
            catch (...)
            {
                TVDiag::LogRetailF(L"[TableView] Optional final cell-name invalidation failed.");
            }
        }
    }))
    {
        return;
    }
    m_editName.reset();
    TVDiag::LogRetailF(L"[TableView] Optional final cell-name invalidation could not be queued.");
}

winrt::hstring TableViewCellAutomationPeer::ReadDisplayName(winrt::FrameworkElement const& display)
{
    const auto headerText = GetColumnHeaderText();
    uint32_t remaining = 32;
    const auto valueText = display
        ? GetCellContentName(display, true, 8, remaining)
        : GetCellDisplayText(Owner().try_as<winrt::FrameworkElement>());

    if (headerText.empty())
    {
        return valueText;
    }
    if (valueText.empty())
    {
        return headerText;
    }

    return FormatLocalizedOrFallback(SR_TableViewCellNameFormat, L"%1!s!, %2!s!", headerText.c_str(), valueText.c_str(), L", ");
}

winrt::hstring TableViewCellAutomationPeer::GetColumnHeaderText()
{
    // Non-string headers have no simple textual prefix, so let the value stand alone.
    if (auto const headerString = TryGetColumnHeaderString(m_column.get()))
    {
        return *headerString;
    }

    return {};
}

winrt::hstring TableViewCellAutomationPeer::GetCellValueText()
{
    auto const row = m_row.get();
    if (row && !GetTrackedTableForRow(row))
    {
        ThrowElementNotAvailable();
    }

    auto const content = GetCellContentElement(Owner().try_as<winrt::FrameworkElement>());
    // ValuePattern describes editable text, not the accessibility label naming that text.
    if (auto const text = content.try_as<winrt::TextBlock>())
    {
        return text.Text();
    }
    if (auto const editor = content.try_as<winrt::TextBox>())
    {
        return editor.Text();
    }
    return GetCellDisplayText(Owner().try_as<winrt::FrameworkElement>());
}

hstring TableViewCellAutomationPeer::GetHelpTextCore()
{
    auto const row = m_row.get();
    if (row && !GetTrackedTableForRow(row))
    {
        ThrowElementNotAvailable();
    }

    auto const helpText = __super::GetHelpTextCore();
    if (helpText.empty())
    {
        return helpText;
    }

    auto const record = TableViewDetails::GetRecord(Owner().try_as<winrt::FrameworkElement>());

    // Resolved here, not at attach, where the cell's binding may not have produced a value yet.
    // Gated on the record so text the app set is never dropped.
    if (record && !record->PublishedHelpText.empty() &&
        helpText == record->PublishedHelpText &&
        helpText == GetCellValueText())
    {
        return {};
    }

    return helpText;
}

int32_t TableViewCellAutomationPeer::GetRowIndex()
{
    if (auto const row = m_row.get())
    {
        if (auto const tableView = GetTrackedTableForRow(row))
        {
            m_lastOwningTable = winrt::make_weak(tableView);

            // Use the row peer's coordinate basis and avoid a visual-tree walk per realized cell.
            if (auto const repeater = winrt::get_self<TableView>(tableView)->GetRowsRepeaterInternal())
            {
                if (const auto rowIndex = repeater.GetElementIndex(row); rowIndex >= 0)
                {
                    m_lastKnownRowIndex = rowIndex;
                    return rowIndex;
                }
            }
        }

        // TableView exposes no public row-index API, so fall back to the hosting ItemsRepeater for
        // a row whose owner is not resolvable yet.
        auto const item = GetTrackedItem();
        auto const rowItem = row.DataContext();
        if (!item || !rowItem || !TableView::SameInspectableIdentity(rowItem, item))
        {
            return -1;
        }

        winrt::DependencyObject parent = winrt::VisualTreeHelper::GetParent(row);
        while (parent)
        {
            if (auto repeater = parent.try_as<winrt::ItemsRepeater>())
            {
                if (const auto rowIndex = repeater.GetElementIndex(row); rowIndex >= 0)
                {
                    m_lastKnownRowIndex = rowIndex;
                    return rowIndex;
                }
            }
            parent = winrt::VisualTreeHelper::GetParent(parent);
        }
    }

    return -1;
}

bool TableViewCellAutomationPeer::IsVirtualized()
{
    if (GetRowIndex() >= 0)
    {
        return false;
    }

    auto const tableView = m_lastOwningTable.get();
    if (!tableView || m_lastKnownRowIndex < 0)
    {
        return false;
    }

    auto const tableImpl = winrt::get_self<TableView>(tableView);
    const auto rowIndex = GetTrackedItemIndex(tableView);
    if (rowIndex < 0 || rowIndex >= tableImpl->GetRowCountInternal())
    {
        return false;
    }
    m_lastKnownRowIndex = rowIndex;

    auto const repeater = tableImpl->GetRowsRepeaterInternal();
    return repeater && !repeater.TryGetElement(rowIndex);
}

void TableViewCellAutomationPeer::Realize()
{
    if (!IsVirtualized())
    {
        return;
    }

    // IVirtualizedItemProvider::Realize is synchronous: on return the client immediately re-queries
    // this provider and expects the element realized, with VirtualizedItem no longer supported.
    // Deferring to the dispatcher hands the client a still-virtualized provider.
    RealizeCore();
}

void TableViewCellAutomationPeer::RealizeCore()
{
    auto const tableView = m_lastOwningTable.get();
    const auto rowIndex = tableView ? GetTrackedItemIndex(tableView) : -1;
    if (!tableView || rowIndex < 0)
    {
        ThrowElementNotAvailable();
    }

    auto const tableImpl = winrt::get_self<TableView>(tableView);
    if (rowIndex >= tableImpl->GetRowCountInternal())
    {
        ThrowElementNotAvailable();
    }

    auto const repeater = tableImpl->GetRowsRepeaterInternal();
    if (!repeater)
    {
        ThrowElementNotAvailable();
    }

    winrt::UIElement element{ nullptr };
    try
    {
        element = repeater.TryGetElement(rowIndex);
        if (!element)
        {
            element = repeater.GetOrCreateElement(rowIndex);
        }
    }
    catch (...)
    {
        ThrowElementNotAvailable();
    }

    if (auto const row = element.try_as<winrt::TableViewRow>())
    {
        if (!IsTrackedRow(row, tableView))
        {
            ThrowElementNotAvailable();
        }

        if (auto const cell = GetRealizedCellFromRow(row))
        {
            if (auto const cellElement = cell.try_as<winrt::FrameworkElement>())
            {
                cellElement.StartBringIntoView();
                return;
            }
        }
    }

    if (auto const frameworkElement = element.try_as<winrt::FrameworkElement>())
    {
        frameworkElement.StartBringIntoView();
    }
}

winrt::TableView TableViewCellAutomationPeer::GetTrackedTableForRow(winrt::TableViewRow const& row)
{
    if (!row)
    {
        return nullptr;
    }

    auto const tableView = winrt::get_self<TableViewRow>(row)->GetOwningTableView();
    if (tableView && IsTrackedRow(row, tableView))
    {
        m_lastOwningTable = winrt::make_weak(tableView);
        return tableView;
    }

    return nullptr;
}

winrt::IInspectable TableViewCellAutomationPeer::GetTrackedItem() const
{
    return m_item.get();
}

int32_t TableViewCellAutomationPeer::GetTrackedItemIndex(winrt::TableView const& tableView)
{
    auto const item = GetTrackedItem();
    if (!tableView || !item)
    {
        return -1;
    }

    auto const tableImpl = winrt::get_self<TableView>(tableView);
    auto const repeater = tableImpl->GetRowsRepeaterInternal();
    auto const view = repeater ? repeater.ItemsSourceView() : nullptr;
    if (!view)
    {
        return -1;
    }

    const int32_t count = view.Count();
    for (int32_t index = 0; index < count; ++index)
    {
        if (auto const candidate = tableImpl->UnwrapEditingDataItem(view.GetAt(index));
            candidate && TableView::SameInspectableIdentity(candidate, item))
        {
            return index;
        }
    }

    return -1;
}

bool TableViewCellAutomationPeer::IsTrackedRow(winrt::TableViewRow const& row, winrt::TableView const& tableView)
{
    auto const item = GetTrackedItem();
    if (!row || !tableView || !item)
    {
        return false;
    }

    auto const rowItem = winrt::get_self<TableView>(tableView)->UnwrapEditingDataItem(row.DataContext());
    const bool sameItem = rowItem && TableView::SameInspectableIdentity(rowItem, item);
    if (sameItem)
    {
        m_item = winrt::make_weak(rowItem);
    }
    return sameItem;
}

void TableViewCellAutomationPeer::TrackRowItem(winrt::TableViewRow const& row, winrt::TableView const& tableView)
{
    if (row && tableView)
    {
        if (auto const item = winrt::get_self<TableView>(tableView)->UnwrapEditingDataItem(row.DataContext()))
        {
            m_item = winrt::make_weak(item);
        }
    }
}

[[noreturn]] void TableViewCellAutomationPeer::ThrowElementNotAvailable()
{
    throw winrt::hresult_error(UIA_E_ELEMENTNOTAVAILABLE);
}

winrt::UIElement TableViewCellAutomationPeer::GetRealizedCellFromRow(winrt::TableViewRow const& row)
{
    if (!row)
    {
        return nullptr;
    }

    auto const rowImpl = winrt::get_self<TableViewRow>(row);
    auto const cellsHost = rowImpl ? rowImpl->GetCellsHostPanelInternal() : nullptr;
    if (!cellsHost)
    {
        return nullptr;
    }

    if (auto const column = m_column.get())
    {
        for (auto const& child : cellsHost.Children())
        {
            auto const cell = child.try_as<winrt::UIElement>();
            if (cell && rowImpl->GetCellOwningColumn(cell) == column)
            {
                return cell;
            }
        }
    }

    return rowImpl->GetVisibleCellInternal(m_columnIndex);
}

int32_t TableViewCellAutomationPeer::Row()
{
    return GetRowIndex();
}

int32_t TableViewCellAutomationPeer::Column()
{
    // Computed live, mirroring Row(): a cached index goes stale as soon as a column is hidden or
    // shown underneath a client holding this provider.
    if (auto const row = m_row.get())
    {
        if (!GetTrackedTableForRow(row))
        {
            ThrowElementNotAvailable();
        }

        if (auto const rowImpl = winrt::get_self<TableViewRow>(row))
        {
            if (auto const cellsHost = rowImpl->GetCellsHostPanelInternal())
            {
                auto const cell = Owner().try_as<winrt::UIElement>();
                auto const cellChildren = cellsHost.Children();
                const auto count = cellChildren.Size();
                int32_t visibleColumnIndex = 0;

                // Same walk and predicate as TableViewAutomationPeer::VisibleColumnToChildIndex and
                // the row peer's GetChildrenCore, so all three agree on the coordinate.
                for (uint32_t i = 0; i < count; ++i)
                {
                    auto const child = cellChildren.GetAt(i).try_as<winrt::UIElement>();
                    if (!child || !IsVisibleColumn(rowImpl->GetCellOwningColumn(child)))
                    {
                        continue;
                    }

                    if (child == cell)
                    {
                        return visibleColumnIndex;
                    }
                    ++visibleColumnIndex;
                }
            }
        }
    }

    return m_columnIndex;
}

int32_t TableViewCellAutomationPeer::RowSpan()
{
    return 1;
}

int32_t TableViewCellAutomationPeer::ColumnSpan()
{
    return 1;
}

winrt::IRawElementProviderSimple TableViewCellAutomationPeer::ContainingGrid()
{
    // The containing grid is the owning TableView's automation peer.
    if (auto const row = m_row.get())
    {
        if (auto const owner = GetTrackedTableForRow(row))
        {
            if (auto const peer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(owner))
            {
                return ProviderFromPeer(peer);
            }
        }
    }

    return nullptr;
}

winrt::com_array<winrt::IRawElementProviderSimple> TableViewCellAutomationPeer::GetRowHeaderItems()
{
    // TableView has no row-header concept.
    return {};
}

winrt::com_array<winrt::IRawElementProviderSimple> TableViewCellAutomationPeer::GetColumnHeaderItems()
{
    // Return the corresponding column header provider using the same peer construction path
    // as the table-level header enumeration.
    std::vector<winrt::IRawElementProviderSimple> headers;

    if (auto const column = m_column.get())
    {
        if (auto const row = m_row.get())
        {
            if (auto const owner = GetTrackedTableForRow(row))
            {
                // Resolve the visual's peer even when the app supplies a custom table peer.
                auto const headerPeer = GetRealizedColumnHeaderPeer(owner, column);

                // A provider array must not contain nulls - UIA marshals every element. An empty
                // array correctly reports "this cell has no reachable column header".
                if (headerPeer)
                {
                    if (auto const provider = ProviderFromPeer(headerPeer))
                    {
                        headers.push_back(provider);
                    }
                }
            }
        }
    }

    return winrt::com_array(headers);
}

// ----- IValueProvider -----

winrt::hstring TableViewCellAutomationPeer::Value()
{
    return GetCellValueText();
}

bool TableViewCellAutomationPeer::IsReadOnly()
{
    return !SupportsValuePattern();
}

void TableViewCellAutomationPeer::SetValue(winrt::hstring const& value)
{
    if (IsReadOnly())
    {
        throw winrt::hresult_error(E_NOTIMPL, L"This cell is read-only.");
    }

    auto const row = m_row.get();
    auto const column = m_column.get();
    if (!row || !column || !GetTrackedTableForRow(row))
    {
        throw winrt::hresult_error(E_FAIL, L"The cell is no longer realized.");
    }

    auto const owner = winrt::get_self<TableViewColumn>(column)->GetOwningTableView();
    if (!owner)
    {
        throw winrt::hresult_error(E_FAIL, L"The cell has no owning TableView.");
    }

    auto const item = GetTrackedItem();
    if (!item)
    {
        ThrowElementNotAvailable();
    }
    auto ownerImpl = winrt::get_self<TableView>(owner);

    // Drive the real edit lifecycle rather than writing the source directly, so a BeginningEdit
    // handler can still veto and CellEditEnding/validation still run - a programmatic set must not
    // be able to do what a user cannot.
    if (!ownerImpl->BeginEdit(item, column))
    {
        throw winrt::hresult_error(E_FAIL, L"The cell could not be opened for editing.");
    }

    bool wrote = false;
    if (auto const editingElement = ownerImpl->CurrentEditingElement())
    {
        if (auto const textBox = editingElement.try_as<winrt::TextBox>())
        {
            textBox.Text(value);
            wrote = true;
        }
    }

    if (!wrote)
    {
        // Nothing we can type into - do not leave the editor open.
        ownerImpl->CancelEdit();
        throw winrt::hresult_error(E_NOTIMPL, L"This cell's editor does not support setting a text value.");
    }

    if (!ownerImpl->CommitEdit())
    {
        // Vetoed, rejected by validation, or waiting on a deferral; the edit stays open and the
        // caller must not be told the value was applied.
        throw winrt::hresult_error(E_FAIL, L"The value was not accepted.");
    }
}

bool TableViewCellAutomationPeer::SupportsValuePattern()
{
    auto const column = m_column.get();
    if (!column || column.IsReadOnly())
    {
        return false;
    }

    auto const owner = winrt::get_self<TableViewColumn>(column)->GetOwningTableView();
    if (!owner || owner.IsReadOnly())
    {
        return false;
    }

    // SetValue writes text, so the column must produce a TextBox. A text column no longer implies
    // one: CellEditingTemplate lives on the base column now, so an app can replace any column's
    // editor with an arbitrary template. Advertising the pattern then tells assistive technology it
    // can set a value, and the attempt fails only after an edit has been opened on screen.
    auto const textColumn = column.try_as<winrt::TableViewTextColumn>();
    return textColumn != nullptr && column.CellEditingTemplate() == nullptr;
}
