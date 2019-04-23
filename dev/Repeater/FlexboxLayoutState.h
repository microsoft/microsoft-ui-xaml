// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class FlexboxLayoutState :
    public winrt::implements<FlexboxLayoutState, winrt::IInspectable>
{
public:
    struct RowMeasureInfo
    {
    public:
        float MainAxis;
        float CrossAxis;
        unsigned int Count;
        float Grow;
    };

    std::vector<RowMeasureInfo> Rows;
};
