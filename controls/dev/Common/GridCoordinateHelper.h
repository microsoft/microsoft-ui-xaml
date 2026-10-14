// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Internal C++ helper: row/column <-> flat-index + focus-navigation math used by
// TableView's keyboard navigation. Not projected (no public runtimeclass).
class GridCoordinateHelper
{
public:
    GridCoordinateHelper() = default;
    GridCoordinateHelper(int32_t rowCount, int32_t columnCount);

    int32_t RowCount() const noexcept { return m_rowCount; }
    int32_t ColumnCount() const noexcept { return m_columnCount; }

    void Resize(int32_t rowCount, int32_t columnCount) noexcept;

    bool IsValidCellAddress(int32_t row, int32_t column) const noexcept;

    int32_t GetFlatIndex(int32_t row, int32_t column) const noexcept;

    bool TryGetCellAddress(int32_t flatIndex, int32_t& row, int32_t& column) const noexcept;

    bool TryGetNextFocusableCell(
        int32_t currentRow,
        int32_t currentColumn,
        winrt::FocusNavigationDirection direction,
        bool wrap,
        int32_t& nextRow,
        int32_t& nextColumn) const noexcept;

private:
    int32_t m_rowCount{};
    int32_t m_columnCount{};
};
