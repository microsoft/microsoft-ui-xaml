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

#include <string>
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
    GetRowIndex();
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
        return *this;
    }

    if (patternInterface == winrt::PatternInterface::VirtualizedItem && IsVirtualized())
    {
        return *this;
    }

    return __super::GetPatternCore(patternInterface);
}

hstring TableViewCellAutomationPeer::GetClassNameCore()
{
    // Keep the logical cell class independent of its implementation-only visual.
    return L"TableViewCell";
}

winrt::AutomationControlType TableViewCellAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::Custom;
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

hstring TableViewCellAutomationPeer::GetNameCore()
{
    auto const row = m_row.get();
    UpdateNameItem(row && winrt::get_self<TableViewRow>(row)->GetOwningTableView()
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
    UpdateNameItem(row && winrt::get_self<TableViewRow>(row)->GetOwningTableView()
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
    cell.InvalidateMeasure();
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
    // Compose "{column header}, {cell value}", falling back to either part alone.
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

    std::wstring composed = std::wstring{ headerText.c_str() } + L", " + valueText.c_str();
    return winrt::hstring{ composed };
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
        // TableView exposes no public row-index API, so resolve it from ItemsRepeater.
        if (auto const tableView = winrt::get_self<TableViewRow>(row)->GetOwningTableView())
        {
            m_lastOwningTable = winrt::make_weak(tableView);
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
    if (m_lastKnownRowIndex >= tableImpl->GetRowCountInternal())
    {
        return false;
    }

    auto const repeater = tableImpl->GetRowsRepeaterInternal();
    return repeater && !repeater.TryGetElement(m_lastKnownRowIndex);
}

void TableViewCellAutomationPeer::Realize()
{
    if (!IsVirtualized())
    {
        return;
    }

    const int32_t rowIndex = m_lastKnownRowIndex;
    if (auto const queue = DispatcherQueue())
    {
        auto const weakThis = get_weak();
        if (queue.TryEnqueue(winrt::DispatcherQueuePriority::Normal, [weakThis, rowIndex]()
        {
            if (auto const peer = weakThis.get())
            {
                peer->RealizeCore(rowIndex);
            }
        }))
        {
            return;
        }
    }

    RealizeCore(rowIndex);
}

void TableViewCellAutomationPeer::RealizeCore(int32_t rowIndex)
{
    auto const tableView = m_lastOwningTable.get();
    if (!tableView || rowIndex < 0)
    {
        return;
    }

    auto const tableImpl = winrt::get_self<TableView>(tableView);
    if (rowIndex >= tableImpl->GetRowCountInternal())
    {
        return;
    }

    auto const repeater = tableImpl->GetRowsRepeaterInternal();
    if (!repeater)
    {
        return;
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
        return;
    }

    if (auto const row = element.try_as<winrt::TableViewRow>())
    {
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
    // Returns -1 only while the row has no resolvable repeater index.
    return GetRowIndex();
}

int32_t TableViewCellAutomationPeer::Column()
{
    // Computed live, mirroring Row(): a cached index goes stale as soon as a column is hidden or
    // shown underneath a client holding this provider.
    if (auto const row = m_row.get())
    {
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
        if (auto const owner = winrt::get_self<TableViewRow>(row)->GetOwningTableView())
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
            if (auto const owner = winrt::get_self<TableViewRow>(row)->GetOwningTableView())
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
    if (!row || !column)
    {
        throw winrt::hresult_error(E_FAIL, L"The cell is no longer realized.");
    }

    auto const owner = winrt::get_self<TableViewColumn>(column)->GetOwningTableView();
    if (!owner)
    {
        throw winrt::hresult_error(E_FAIL, L"The cell has no owning TableView.");
    }

    auto const item = row.DataContext();
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
