// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "InkToolBarHighlighterButton.g.h"

#include "InkToolBarPenButton.h"

class InkToolBarHighlighterButton :
    public winrt::implementation::InkToolBarHighlighterButtonT<InkToolBarHighlighterButton, InkToolBarPenButton>
{
public:
    ForwardRefToBaseReferenceTracker(InkToolBarPenButton)

    InkToolBarHighlighterButton()
    {
        SetToolKind(winrt::InkToolBarTool::Highlighter);

        // WinUI 2 InkToolbarHighlighterButton default palette (6 colors, ARGB),
        // ported verbatim from the OS inking controls. Color layout is { A, R, G, B }.
        static const winrt::Windows::UI::Color s_highlighterColors[] =
        {
            { 0xFF, 0xFF, 0xE6, 0x00 }, // Yellow
            { 0xFF, 0x26, 0xE6, 0x00 }, // Green
            { 0xFF, 0x44, 0xC8, 0xF5 }, // Light blue
            { 0xFF, 0xEC, 0x00, 0x8C }, // Pink
            { 0xFF, 0xFF, 0x55, 0x00 }, // Orange
            { 0xFF, 0x66, 0x00, 0xCC }  // Purple
        };
        SetPaletteColors(s_highlighterColors, static_cast<uint32_t>(ARRAYSIZE(s_highlighterColors)));

        // A highlighter must look like a highlighter: bold color and a wide tip.
        // Select Yellow (index 0) and a wide stroke so highlighter ink is clearly
        // distinct from the thin black ballpoint pen. DrawAsHighlighter (set by the
        // toolbar) then renders it translucent.
        SelectedBrushIndex(0);
        SelectedStrokeWidth(16.0);
    }
};

