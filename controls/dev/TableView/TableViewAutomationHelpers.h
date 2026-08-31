// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <optional>
#include "TableViewColumn.h"

// Shared helpers for the TableView automation peers, so the visible-column and column-header-string
// logic lives in one place instead of being copy-pasted across the peer translation units. Assumes
// pch.h (winrt type aliases) is included first, per the TableView header convention.

inline bool IsVisibleColumn(winrt::TableViewColumn const& column)
{
    return column && column.Visibility() == winrt::Visibility::Visible;
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

// The text a realized cell reports to UIA: the generated TextBlock for a text column, otherwise the
// standard UIA name of the column-generated content. Shared by TableViewCellAutomationPeer's name
// and help-text composition, so the two cannot drift. Only called from the peer, so the automation
// peer it may allocate is created when UIA is actually asking.
inline winrt::hstring GetCellValueTextFromWrapper(winrt::FrameworkElement const& cellWrapper)
{
    if (!cellWrapper)
    {
        return {};
    }

    winrt::FrameworkElement content{ nullptr };
    if (auto const border = cellWrapper.try_as<winrt::Border>())
    {
        content = border.Child().try_as<winrt::FrameworkElement>();
    }
    if (!content)
    {
        content = cellWrapper;
    }

    if (auto const textBlock = content.try_as<winrt::TextBlock>())
    {
        return textBlock.Text();
    }

    if (auto const peer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(content))
    {
        return peer.GetName();
    }

    return {};
}
