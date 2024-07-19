// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

enum ElementBrushProperty
{
    Fill,
    Stroke
};

class CUIElement;

// Parameters for getting a WUC brush for the render walk
struct BrushParams
{
    // Identifies SolidColorBrush usage. Used to pick the correct transitioning brush.
    const CUIElement* m_element = nullptr;
    ElementBrushProperty m_brushProperty;
};
