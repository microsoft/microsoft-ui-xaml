// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CSolidColorBrush;

class HighlightRegion
{
public:
    // The start and end indexes of a HighlightRegion take a position,
    // which may include both visible characters and hidden offsets,
    // as opposed to a character index, which would only count visible characters.
    HighlightRegion(
        int startIndexInit,
        int endIndexInit,
        _In_ CSolidColorBrush* foregroundBrushInit,
        _In_ CSolidColorBrush* backgroundBrushInit
    )
        : startIndex(startIndexInit)
        , endIndex(endIndexInit)
        , foregroundBrush(foregroundBrushInit)
        , backgroundBrush(backgroundBrushInit)
    {}

    int startIndex = 0;
    int endIndex = 0;
    CSolidColorBrush* foregroundBrush = nullptr;
    CSolidColorBrush* backgroundBrush = nullptr;
private:
};
