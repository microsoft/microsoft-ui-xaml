// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "InkToolbarHighlighterButton.g.h"

#include "InkToolbarPenButton.h"
#include "ResourceAccessor.h"

class InkToolbarHighlighterButton :
    public winrt::implementation::InkToolbarHighlighterButtonT<InkToolbarHighlighterButton, InkToolbarPenButton>
{
public:
    ForwardRefToBaseReferenceTracker(InkToolbarPenButton)

    InkToolbarHighlighterButton()
    {
        SetToolKind(winrt::InkToolbarTool::Highlighter);
        SetDefaultStyleKey(this);

        // A highlighter must look like a highlighter: bold color (Yellow, index 0) and a wide tip,
        // so its ink is clearly distinct from the thin ballpoint pen. DrawAsHighlighter (set by the
        // toolbar) then renders it translucent.
        SelectedBrushIndex(0);
        SelectedStrokeWidth(16.0);
    }

    winrt::hstring GetLocalizedToolName() override
    {
        return ResourceAccessor::GetLocalizedStringResource(SR_InkToolbarHighlighterButtonName);
    }

    // UWP InkToolbarHighlighterButton::GetColors: WinUI 2 highlighter palette (6 colors, ARGB).
    std::vector<winrt::Windows::UI::Color> GetColors() override
    {
        return {
            { 0xFF, 0xFF, 0xE6, 0x00 }, // Yellow
            { 0xFF, 0x26, 0xE6, 0x00 }, // Green
            { 0xFF, 0x44, 0xC8, 0xF5 }, // Light blue
            { 0xFF, 0xEC, 0x00, 0x8C }, // Pink
            { 0xFF, 0xFF, 0x55, 0x00 }, // Orange
            { 0xFF, 0x66, 0x00, 0xCC }  // Purple
        };
    }

    // UWP InkToolbarHighlighterButton::CreateInkDrawingAttributes: wide rectangle tip, translucent.
    winrt::InkDrawingAttributes CreateInkDrawingAttributes() override
    {
        auto attrs = winrt::InkDrawingAttributes();
        if (auto solid = SelectedBrush().try_as<winrt::SolidColorBrush>())
        {
            attrs.Color(solid.Color());
        }
        auto self = this->try_as<winrt::InkToolbarPenButton>();
        float w = InkToolbarPenButton::DetermineStrokeWidth(self);
        // UWP highlighter uses a chisel nib: Width = strokeWidth / 3, Height = strokeWidth.
        attrs.Size({ w / 3.0f, w });
        attrs.PenTip(winrt::Windows::UI::Input::Inking::PenTipShape::Rectangle);
        attrs.DrawAsHighlighter(true);
        return attrs;
    }
};

