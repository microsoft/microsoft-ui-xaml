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
