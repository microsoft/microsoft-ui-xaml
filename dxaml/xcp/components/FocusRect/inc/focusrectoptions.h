// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

struct FocusRectangleOptions
{
    FocusRectangleOptions() = default;
    // Used to set the colors of the two alternating brushes
    // When isContinuous==true, the second rect is drawn inside the first
    // When isContinuous==false, the first and second are drawn together as a dotted line
    xref_ptr<CSolidColorBrush> firstBrush;
    xref_ptr<CSolidColorBrush> secondBrush;

    // Used to disable/enable the drawing of the two different lines
    bool drawFirst = false;
    bool drawSecond = false;
    bool drawReveal = false;

    // Used to indicate whether this is a continuous focus rectangle (not dashed)
    // When true, the second rect is drawn inside the first
    // When false, the rects are drawn as alternating dotted lines
    bool isContinuous = false;

    // Used to indicate if we are drawing a borderless reveal glow
    bool isRevealBorderless = false;

    uint32_t revealColor = 0;

    // Bounds of the focus rect.  If uninitialized, we'll just use element's bounds
    XRECTF bounds{};
    XCORNERRADIUS cornerRadius{};
 
    // Previous bounds, used for animating reveal focus visual
    XRECTF previousBounds{};
    
    // Determines the thickness of the focus rectangle
    // When !isContinuous, only fistThickness is honored
    XTHICKNESS firstThickness{};
    XTHICKNESS secondThickness{};

    void UseElementBounds(_In_ CUIElement* element);
};
