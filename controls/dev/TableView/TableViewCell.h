// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TableViewCellAutomationPeer.h"
#include "TableViewRow.h"

struct __declspec(uuid("843c3f59-aab0-4bcb-a8a4-12327286a29f")) __declspec(novtable)
    ITableViewCellAutomationPeerAccess : ::IUnknown
{
    virtual HRESULT __stdcall GetExistingPeer(_Outptr_result_maybenull_ ::IInspectable** peer) noexcept = 0;
};

struct __declspec(uuid("6b0d94f1-3c27-4e58-8a6d-71f2c9b40e35")) __declspec(novtable)
    ITableViewCellNavigationAccess : ::IUnknown
{
    virtual HRESULT __stdcall MoveCellCursor(
        int32_t virtualKey, boolean control, _Out_ boolean* handled) noexcept = 0;

    // Delivers the key with an explicit pre-key cursor anchor, exactly as the control's
    // PreviewKeyDown -> KeyDown pair does. This is what lets a test reproduce "the framework's
    // built-in directional navigation already advanced focus one cell", which is the condition that
    // made a single arrow press step two columns.
    virtual HRESULT __stdcall MoveCellCursorFromAnchor(
        int32_t virtualKey, boolean control, int32_t anchorRow, int32_t anchorColumn,
        _Out_ boolean* handled) noexcept = 0;
};

// Border is sealed. A single-child Grid preserves cell chrome while allowing
// normal framework peer discovery to return the same peer as Grid.GetItem.
//
// The cell is also the keyboard focus target. A TableView is navigated cell by cell (Left/Right
// within a row, Up/Down across rows preserving the column), so the element UIA reports as focused
// has to be the cell and not the row - that is what lets Narrator announce a single
// "{column}, {value}" instead of re-reading every column of the row on each arrow press.
class TableViewCell :
    public ReferenceTracker<TableViewCell, winrt::Microsoft::UI::Xaml::Controls::GridT,
        winrt::composable, ITableViewCellAutomationPeerAccess, ITableViewCellNavigationAccess>
{
public:
    TableViewCell(winrt::TableViewRow const& row, winrt::TableViewColumn const& column, int32_t columnIndex)
        : m_row(winrt::make_weak(row)), m_column(winrt::make_weak(column)), m_columnIndex(columnIndex)
    {
    }

    static winrt::Grid Create(
        winrt::TableViewRow const& row, winrt::TableViewColumn const& column, int32_t columnIndex)
    {
        auto const wrapper = winrt::make<TableViewCell>(row, column, columnIndex).as<winrt::Grid>();

        // Focusable, so a cell can be the UIA FocusedElement and SetFocus() on its peer succeeds
        // instead of throwing "Target element cannot receive focus". CUIElement::IsFocusable gates
        // on IsTabStop, so this is what makes the cell reachable at all.
        //
        // Every cell being a tab stop does NOT make the table a tab trap: the rows repeater uses
        // TabFocusNavigation="Once", so the whole body stays a single tab stop and Tab leaves.
        wrapper.IsTabStop(true);

        // The framework's own focus rectangle, not a hand-rolled one. UseSystemFocusVisuals is a
        // UIElement property (KnownPropertyIndex::UIElement_UseSystemFocusVisuals), and the focus
        // rect manager honours it for any focusable UIElement - the Control-only branch it takes is
        // just the FocusTargetDescendant redirection, which a plain cell does not need. This is the
        // same call ItemsControl makes on the containers it generates, and it gets the
        // FocusVisualPrimaryBrush / FocusVisualSecondaryBrush pair, High Contrast adaptation and
        // "keyboard focus only, never pointer" behaviour for free.
        wrapper.UseSystemFocusVisuals(true);

        return wrapper;
    }

    winrt::hstring GetRuntimeClassName() const
    {
        return winrt::hstring_name_of<winrt::Grid>();
    }

    winrt::AutomationPeer OnCreateAutomationPeer()
    {
        if (auto const peer = m_automationPeer.get())
        {
            return peer;
        }
        winrt::AutomationPeer const peer = winrt::make<TableViewCellAutomationPeer>(
            get_strong().as<winrt::FrameworkElement>(), m_row.get(), m_column.get(), m_columnIndex);
        m_automationPeer = winrt::make_weak(peer);
        return peer;
    }

    HRESULT __stdcall GetExistingPeer(::IInspectable** result) noexcept
    {
        if (!result)
        {
            return E_POINTER;
        }
        *result = nullptr;
        try
        {
            CheckThread();
            auto peer = m_automationPeer.get();
            *result = reinterpret_cast<::IInspectable*>(winrt::detach_abi(peer));
            return S_OK;
        }
        catch (...)
        {
            return winrt::to_hresult();
        }
    }

    HRESULT __stdcall MoveCellCursor(int32_t virtualKey, boolean control, boolean* handled) noexcept
    {
        return MoveCellCursorFromAnchor(virtualKey, control, -1, -1, handled);
    }

    HRESULT __stdcall MoveCellCursorFromAnchor(
        int32_t virtualKey, boolean control, int32_t anchorRow, int32_t anchorColumn,
        boolean* handled) noexcept
    {
        if (!handled)
        {
            return E_POINTER;
        }
        *handled = false;
        try
        {
            CheckThread();
            if (auto const row = m_row.get())
            {
                if (auto const owner = winrt::get_self<TableViewRow>(row)->GetOwningTableView())
                {
                    auto const ownerImpl = winrt::get_self<TableView>(owner);
                    auto const key = static_cast<winrt::Windows::System::VirtualKey>(virtualKey);
                    *handled = static_cast<boolean>(anchorRow < 0 || anchorColumn < 0
                        ? ownerImpl->TryMoveCellCursor(key, control != false)
                        : ownerImpl->TryMoveCellCursorFromAnchor(
                            key, control != false, anchorRow, anchorColumn));
                }
            }
            return S_OK;
        }
        catch (...)
        {
            return winrt::to_hresult();
        }
    }

    static winrt::AutomationPeer TryGetExistingPeer(winrt::UIElement const& cell)
    {
        winrt::AutomationPeer peer{ nullptr };
        if (auto const access = cell.try_as<ITableViewCellAutomationPeerAccess>())
        {
            winrt::check_hresult(access->GetExistingPeer(
                reinterpret_cast<::IInspectable**>(winrt::put_abi(peer))));
        }
        return peer;
    }

    static winrt::UIElement Child(winrt::Grid const& cell)
    {
        auto const children = cell.Children();
        return children.Size() ? children.GetAt(0) : nullptr;
    }

    static void Child(winrt::Grid const& cell, winrt::UIElement const& content)
    {
        auto const children = cell.Children();
        children.Clear();
        if (content)
        {
            children.Append(content);
        }
    }

private:
    winrt::weak_ref<winrt::TableViewRow> m_row;
    winrt::weak_ref<winrt::TableViewColumn> m_column;
    winrt::weak_ref<winrt::AutomationPeer> m_automationPeer{ nullptr };
    int32_t m_columnIndex;
};
