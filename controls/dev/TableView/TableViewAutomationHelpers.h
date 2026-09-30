// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <optional>
#include <string_view>
#include "TableView.h"
#include "TableViewCellsPanel.h"
#include "TableViewColumn.h"
#include "ResourceAccessor.h"

// Shared helpers for the TableView automation peers, so the visible-column and column-header-string
// logic lives in one place instead of being copy-pasted across the peer translation units. Assumes
// pch.h (winrt type aliases) is included first, per the TableView header convention.

// Resource lookups feeding UIA are supplementary: degrade instead of letting a missing PRI (a host
// app that does not merge the control's resources) escape into a UIA call.
inline winrt::hstring TryGetLocalizedString(const std::wstring_view& resourceName)
{
    try
    {
        return ResourceAccessor::GetLocalizedStringResource(resourceName);
    }
    catch (...)
    {
        return {};
    }
}

inline bool IsVisibleColumn(winrt::TableViewColumn const& column)
{
    return column && column.Visibility() == winrt::Visibility::Visible;
}

inline winrt::AutomationPeer GetRealizedColumnHeaderPeer(
    winrt::TableView const& table,
    winrt::TableViewColumn const& column)
{
    if (!table || !IsVisibleColumn(column) ||
        winrt::get_self<TableViewColumn>(column)->GetOwningTableView() != table)
    {
        return nullptr;
    }

    if (auto const host = winrt::get_self<TableView>(table)->GetHeaderHostInternal())
    {
        if (auto const header = TableViewCellsPanel::CellForColumn(host, column))
        {
            return winrt::FrameworkElementAutomationPeer::CreatePeerForElement(header);
        }
    }
    return nullptr;
}

// Stringifies a data item for UIA. Shared by the TableView peer's item search and the row peer's
// name fallback, so both describe the same item the same way.
inline winrt::hstring ItemToName(winrt::IInspectable const& item)
{
    // Boxed WinRT primitives surface as IPropertyValue, not IStringable.
    if (auto const propValue = item.try_as<winrt::IPropertyValue>())
    {
        switch (propValue.Type())
        {
        case winrt::PropertyType::String:  return propValue.GetString();
        case winrt::PropertyType::Boolean: return propValue.GetBoolean() ? winrt::hstring{ L"True" } : winrt::hstring{ L"False" };
        case winrt::PropertyType::Int16:   return winrt::to_hstring(static_cast<int32_t>(propValue.GetInt16()));
        case winrt::PropertyType::Int32:   return winrt::to_hstring(propValue.GetInt32());
        case winrt::PropertyType::Int64:   return winrt::to_hstring(propValue.GetInt64());
        case winrt::PropertyType::UInt8:   return winrt::to_hstring(static_cast<uint32_t>(propValue.GetUInt8()));
        case winrt::PropertyType::UInt16:  return winrt::to_hstring(static_cast<uint32_t>(propValue.GetUInt16()));
        case winrt::PropertyType::UInt32:  return winrt::to_hstring(propValue.GetUInt32());
        case winrt::PropertyType::UInt64:  return winrt::to_hstring(propValue.GetUInt64());
        case winrt::PropertyType::Single:  return winrt::to_hstring(propValue.GetSingle());
        case winrt::PropertyType::Double:  return winrt::to_hstring(propValue.GetDouble());
        default: break;
        }
    }

    if (auto const stringable = item.try_as<winrt::IStringable>())
    {
        return stringable.ToString();
    }

    return {};
}

inline winrt::hstring GroupInfoToName(winrt::TableViewGroupInfo const& info)
{
    const auto keyText = info.KeyText();
    const auto countText = info.ItemCountText();
    if (!keyText.empty() && !countText.empty())
    {
        return winrt::hstring{ std::wstring{ keyText } + L" " + std::wstring{ countText } };
    }
    return keyText;
}

inline int32_t CountVisibleColumns(winrt::IVector<winrt::TableViewColumn> const& columns)
{
    int32_t count = 0;
    for (auto const& column : columns)
    {
        if (IsVisibleColumn(column))
        {
            ++count;
        }
    }
    return count;
}

// Returns the text a cell displays: the column-generated TextBlock's text, else the content's own
// computed UIA name. Shared so a cell's name and the row name composed from its cells agree.
//
// allowPeerCreation gates the fallback: CreatePeerForElement does not just read a name, it creates
// and permanently attaches a peer. Worth it for a cell naming itself; not for the row name, which
// walks every cell on every name query, so it passes false and skips template content.
inline winrt::hstring GetCellContentName(
    winrt::FrameworkElement const& content, bool allowPeerCreation, uint32_t depth, uint32_t& remaining)
{
    if (!content || content.Visibility() != winrt::Visibility::Visible || depth == 0 || remaining == 0)
    {
        return {};
    }
    --remaining;

    if (auto const name = winrt::AutomationProperties::GetName(content); !name.empty())
    {
        return name;
    }
    if (auto const label = winrt::AutomationProperties::GetLabeledBy(content))
    {
        auto const peer = allowPeerCreation
            ? winrt::FrameworkElementAutomationPeer::CreatePeerForElement(label)
            : winrt::FrameworkElementAutomationPeer::FromElement(label);
        if (peer)
        {
            if (auto const name = peer.GetName(); !name.empty())
            {
                return name;
            }
        }
    }
    if (auto const textBlock = content.try_as<winrt::TextBlock>())
    {
        return textBlock.Text();
    }

    // Templated presenter peers can stringify the data object instead of the visible template.
    auto const presenter = content.try_as<winrt::ContentPresenter>();
    if (!presenter || !presenter.ContentTemplate())
    {
        auto const peer = allowPeerCreation
            ? winrt::FrameworkElementAutomationPeer::CreatePeerForElement(content)
            : winrt::FrameworkElementAutomationPeer::FromElement(content);
        if (peer)
        {
            if (auto const name = peer.GetName(); !name.empty())
            {
                return name;
            }
        }
    }

    // A named control describes its own content; do not repeat its inner interactive labels.
    // Only traverse layout wrappers, and bound the work of each cell-name query.
    if (presenter || content.try_as<winrt::Panel>() || content.try_as<winrt::Border>())
    {
        const auto count = winrt::VisualTreeHelper::GetChildrenCount(content);
        for (int32_t i = 0; i < count && remaining > 0; ++i)
        {
            if (auto const child = winrt::VisualTreeHelper::GetChild(content, i).try_as<winrt::FrameworkElement>())
            {
                if (auto const name = GetCellContentName(child, allowPeerCreation, depth - 1, remaining); !name.empty())
                {
                    return name;
                }
            }
        }
    }
    return {};
}

inline winrt::FrameworkElement GetCellContentElement(winrt::FrameworkElement const& cell)
{
    if (!cell)
    {
        return nullptr;
    }
    if (auto const border = cell.try_as<winrt::Border>())
    {
        return border.Child().try_as<winrt::FrameworkElement>();
    }
    if (auto const grid = cell.try_as<winrt::Grid>(); grid && grid.Tag().try_as<winrt::TableViewColumn>())
    {
        auto const children = grid.Children();
        return children.Size() ? children.GetAt(0).try_as<winrt::FrameworkElement>() : nullptr;
    }
    return cell;
}

inline winrt::hstring GetCellDisplayText(winrt::FrameworkElement const& cell, bool allowPeerCreation = true)
{
    auto const content = GetCellContentElement(cell);
    uint32_t remaining = 32;
    return GetCellContentName(content, allowPeerCreation, 8, remaining);
}

// Returns the column Header's string form (an IStringable, or a String-typed IPropertyValue), or
// nullopt when the header is not a string (or the column is null). Returning nullopt rather than an
// empty string lets callers distinguish "no string header" from "an explicitly empty string header".
inline std::optional<winrt::hstring> TryGetColumnHeaderString(winrt::TableViewColumn const& column)
{
    if (column)
    {
        auto const header = column.Header();
        if (auto const stringable = header.try_as<winrt::IStringable>())
        {
            return stringable.ToString();
        }
        if (auto const propValue = header.try_as<winrt::IPropertyValue>())
        {
            if (propValue.Type() == winrt::PropertyType::String)
            {
                return propValue.GetString();
            }
        }
    }
    return std::nullopt;
}
