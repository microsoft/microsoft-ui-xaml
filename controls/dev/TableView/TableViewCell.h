// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TableViewCellAutomationPeer.h"

// Implementation-only access: unlike this framework's FromElement, this never
// creates a peer. It is not part of the projected TableView API.
struct __declspec(uuid("843c3f59-aab0-4bcb-a8a4-12327286a29f")) __declspec(novtable)
    ITableViewCellAutomationPeerAccess : ::IUnknown
{
    virtual HRESULT __stdcall GetExistingPeer(_Outptr_result_maybenull_ ::IInspectable** peer) noexcept = 0;
};

// Border is sealed. A single-child Grid preserves cell chrome while allowing
// normal framework peer discovery to return the same peer as Grid.GetItem.
class TableViewCell :
    public ReferenceTracker<TableViewCell, winrt::Microsoft::UI::Xaml::Controls::GridT,
        winrt::composable, ITableViewCellAutomationPeerAccess>
{
public:
    TableViewCell(winrt::TableViewRow const& row, winrt::TableViewColumn const& column, int32_t columnIndex)
        : m_row(winrt::make_weak(row)), m_column(winrt::make_weak(column)), m_columnIndex(columnIndex)
    {
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
