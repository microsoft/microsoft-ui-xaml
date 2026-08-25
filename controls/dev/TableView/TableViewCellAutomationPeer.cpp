// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewTextColumn.h"
#include "TableViewRow.h"
#include "TableViewColumnHeaderAutomationPeer.h"
#include "TableViewCellAutomationPeer.h"
#include "TableViewAutomationHelpers.h"
#include "TableViewCellAutomationPeer.properties.cpp"

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

    return __super::GetPatternCore(patternInterface);
}

hstring TableViewCellAutomationPeer::GetClassNameCore()
{
    // The cell is a Border, so report a stable TableView cell class name.
    return L"TableViewCell";
}

winrt::AutomationControlType TableViewCellAutomationPeer::GetAutomationControlTypeCore()
{
    // DataItem lets Narrator read the composed cell name instead of a generic container.
    return winrt::AutomationControlType::DataItem;
}

hstring TableViewCellAutomationPeer::GetNameCore()
{
    // Compose "{column header}, {cell value}", falling back to either part alone.
    const auto headerText = GetColumnHeaderText();
    const auto valueText = GetCellValueText();

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
    auto const cell = Owner().try_as<winrt::FrameworkElement>();
    if (!cell)
    {
        return {};
    }

    // The cell wrapper's child is the column-generated content.
    winrt::FrameworkElement content{ nullptr };
    if (auto const border = cell.try_as<winrt::Border>())
    {
        content = border.Child().try_as<winrt::FrameworkElement>();
    }
    if (!content)
    {
        content = cell;
    }

    // Common text-column case: read the generated TextBlock.
    if (auto const textBlock = content.try_as<winrt::TextBlock>())
    {
        return textBlock.Text();
    }

    // Template content uses the standard UIA name computation.
    if (auto const peer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(content))
    {
        return peer.GetName();
    }

    return {};
}

int32_t TableViewCellAutomationPeer::GetRowIndex()
{
    if (auto const row = m_row.get())
    {
        // TableView exposes no public row-index API, so resolve it from ItemsRepeater.
        winrt::DependencyObject parent = winrt::VisualTreeHelper::GetParent(row);
        while (parent)
        {
            if (auto repeater = parent.try_as<winrt::ItemsRepeater>())
            {
                return repeater.GetElementIndex(row);
            }
            parent = winrt::VisualTreeHelper::GetParent(parent);
        }
    }

    return -1;
}

int32_t TableViewCellAutomationPeer::Row()
{
    // Returns -1 only while the row has no resolvable repeater index.
    return GetRowIndex();
}

int32_t TableViewCellAutomationPeer::Column()
{
    // Matches TableViewAutomationPeer::GetItem's cell-host child index.
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
                auto const headerPeer = winrt::make<TableViewColumnHeaderAutomationPeer>(owner, column);
                headers.push_back(ProviderFromPeer(headerPeer));
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
