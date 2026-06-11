// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

enum class CellUnitTypes : uint8_t
{
    None = 0x00,
    Auto = 0x01,
    Star = 0x02,
    Pixel = 0x04,
};
DEFINE_ENUM_FLAG_OPERATORS(CellUnitTypes);

struct CellCache
{
    CUIElement* m_child;

    // Index of the next cell in the group.
    unsigned int m_next;

    // Union of the different height unit types across the row
    // definitions within the row span of this cell.
    CellUnitTypes m_rowHeightTypes;

    // Union of the different width unit types across the column
    // definitions within the column span of this cell.
    CellUnitTypes m_columnWidthTypes;

    static bool IsStar(CellUnitTypes unitTypes)
    {
        return (unitTypes & CellUnitTypes::Star) == CellUnitTypes::Star;
    }

    static bool IsAuto(CellUnitTypes unitTypes)
    {
        return (unitTypes & CellUnitTypes::Auto) == CellUnitTypes::Auto;
    }
};

// Grid classifies cells into four groups based on their column/row
// type. The following diagram depicts all the possible combinations
// and their corresponding cell group:
// 
//                  Px      Auto     Star
//              +--------+--------+--------+
//              |        |        |        |
//           Px |    1   |    1   |    3   |
//              |        |        |        |
//              +--------+--------+--------+
//              |        |        |        |
//         Auto |    1   |    1   |    3   |
//              |        |        |        |
//              +--------+--------+--------+
//              |        |        |        |
//         Star |    4   |    2   |    4   |
//              |        |        |        |
//              +--------+--------+--------+
//
struct CellGroups
{
    unsigned int group1;
    unsigned int group2;
    unsigned int group3;
    unsigned int group4;
};