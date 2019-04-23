// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class GridLayoutState :
    public winrt::implements<GridLayoutState, winrt::IInspectable>
{
public:
    GridLayoutState()
    {
        Columns.reset(new std::map<int, MeasuredGridTrackInfo>());
        Rows.reset(new std::map<int, MeasuredGridTrackInfo>());
    }

    // Calculated info on one of the grid tracks, used to carry over calculations from Measure to Arrange
    // PORT_TODO: Should be a struct
    class MeasuredGridTrackInfo
    {
    public:
        float Size{};
        float Start{};
    };

    std::unique_ptr<std::map<int, MeasuredGridTrackInfo>> Columns;
    std::unique_ptr<std::map<int, MeasuredGridTrackInfo>> Rows;
};
