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

// Border is sealed; use a single-child Grid so framework peer discovery and Grid.GetItem return
// the same cell peer. The cell is the keyboard/UIA focus target, letting Narrator announce one
// "{column}, {value}" instead of the whole row.
class TableViewCell :
    public ReferenceTracker<TableViewCell, winrt::Microsoft::UI::Xaml::Controls::GridT,
        winrt::composable, ITableViewCellAutomationPeerAccess>
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

        // Cells need IsTabStop for UIA SetFocus to succeed; row policy later gates which cells are
        // in the tab order.
        wrapper.IsTabStop(true);

        // The focus rect manager honours this on any focusable UIElement, not just on a Control.
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
