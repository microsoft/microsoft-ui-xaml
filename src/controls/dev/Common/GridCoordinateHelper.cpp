// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "GridCoordinateHelper.h"
#include <limits>

GridCoordinateHelper::GridCoordinateHelper(int32_t rowCount, int32_t columnCount)
    : m_rowCount(rowCount < 0 ? 0 : rowCount)
    , m_columnCount(columnCount < 0 ? 0 : columnCount)
{
}

void GridCoordinateHelper::Resize(int32_t rowCount, int32_t columnCount) noexcept
{
    m_rowCount = rowCount < 0 ? 0 : rowCount;
    m_columnCount = columnCount < 0 ? 0 : columnCount;
}

bool GridCoordinateHelper::IsValidCellAddress(int32_t row, int32_t column) const noexcept
{
    return row >= 0 && row < m_rowCount && column >= 0 && column < m_columnCount;
}

int32_t GridCoordinateHelper::GetFlatIndex(int32_t row, int32_t column) const noexcept
{
    if (!IsValidCellAddress(row, column))
    {
        return -1;
    }
    // Use int64_t to avoid overflow on very large grids.
    const int64_t flat = static_cast<int64_t>(row) * static_cast<int64_t>(m_columnCount)
        + static_cast<int64_t>(column);
    return flat > std::numeric_limits<int32_t>::max() ? -1 : static_cast<int32_t>(flat);
}

bool GridCoordinateHelper::TryGetCellAddress(int32_t flatIndex, int32_t& row, int32_t& column) const noexcept
{
    // Use int64_t so large grids cannot wrap the bounds check.
    const int64_t total = static_cast<int64_t>(m_rowCount) * static_cast<int64_t>(m_columnCount);
    if (m_columnCount <= 0 || m_rowCount <= 0 ||
        flatIndex < 0 || static_cast<int64_t>(flatIndex) >= total)
    {
        row = -1;
        column = -1;
        return false;
    }

    row = flatIndex / m_columnCount;
    column = flatIndex % m_columnCount;
    return true;
}

bool GridCoordinateHelper::TryGetNextFocusableCell(
    int32_t currentRow,
    int32_t currentColumn,
    winrt::FocusNavigationDirection direction,
    bool wrap,
    int32_t& nextRow,
    int32_t& nextColumn) const noexcept
{
    nextRow = -1;
    nextColumn = -1;

    if (m_rowCount <= 0 || m_columnCount <= 0)
    {
        return false;
    }

    // Clamp the starting cell into the grid so callers can pass any "current focus" hint.
    const int32_t startRow = std::clamp(currentRow, 0, m_rowCount - 1);
    const int32_t startColumn = std::clamp(currentColumn, 0, m_columnCount - 1);

    int32_t row = startRow;
    int32_t column = startColumn;

    switch (direction)
    {
    case winrt::FocusNavigationDirection::Left:
    {
        if (column > 0)
        {
            column -= 1;
        }
        else if (wrap)
        {
            // Wrap to the previous row, or the last cell from row 0.
            if (row > 0)
            {
                row -= 1;
            }
            else
            {
                row = m_rowCount - 1;
            }
            column = m_columnCount - 1;
        }
        else
        {
            return false;
        }
        break;
    }
    case winrt::FocusNavigationDirection::Right:
    {
        if (column < m_columnCount - 1)
        {
            column += 1;
        }
        else if (wrap)
        {
            // Wrap to the next row, or (0,0) from the last row.
            if (row < m_rowCount - 1)
            {
                row += 1;
            }
            else
            {
                row = 0;
            }
            column = 0;
        }
        else
        {
            return false;
        }
        break;
    }
    case winrt::FocusNavigationDirection::Up:
    {
        if (row > 0)
        {
            row -= 1;
        }
        else if (wrap)
        {
            row = m_rowCount - 1;
        }
        else
        {
            return false;
        }
        break;
    }
    case winrt::FocusNavigationDirection::Down:
    {
        if (row < m_rowCount - 1)
        {
            row += 1;
        }
        else if (wrap)
        {
            row = 0;
        }
        else
        {
            return false;
        }
        break;
    }
    case winrt::FocusNavigationDirection::Next:
    {
        // Reading-order tab with int64_t math to avoid large-grid overflow.
        const int64_t flat = static_cast<int64_t>(row) * m_columnCount + column + 1;
        const int64_t total = static_cast<int64_t>(m_rowCount) * m_columnCount;
        if (flat >= total)
        {
            if (!wrap)
            {
                return false;
            }
            row = 0;
            column = 0;
        }
        else
        {
            row = static_cast<int32_t>(flat / m_columnCount);
            column = static_cast<int32_t>(flat % m_columnCount);
        }
        break;
    }
    case winrt::FocusNavigationDirection::Previous:
    {
        // Reverse reading-order tab. int64 intermediate as above.
        const int64_t flat = static_cast<int64_t>(row) * m_columnCount + column - 1;
        if (flat < 0)
        {
            if (!wrap)
            {
                return false;
            }
            row = m_rowCount - 1;
            column = m_columnCount - 1;
        }
        else
        {
            row = static_cast<int32_t>(flat / m_columnCount);
            column = static_cast<int32_t>(flat % m_columnCount);
        }
        break;
    }
    default:
        // None / other directions are not defined for grid navigation.
        return false;
    }

    nextRow = row;
    nextColumn = column;
    return true;
}
