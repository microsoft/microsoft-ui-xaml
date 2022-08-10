// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class FlexboxLayoutState :
    public winrt::implements<FlexboxLayoutState, winrt::IInspectable>
{
public:
    struct RowMeasureInfo
    {
        float MainAxis;
        float CrossAxis;
        int Count;
        float Grow;
        float Shrink;
        float Basis;
    };

    std::vector<RowMeasureInfo> Rows;
};
