// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "InkToolBarPenButton.g.h"
#include "InkToolBarPenButton.properties.h"

#include "InkToolBarToolButton.h"

class InkToolBarPenButton :
    public winrt::implementation::InkToolBarPenButtonT<InkToolBarPenButton, InkToolBarToolButton, winrt::composable>, 
    public InkToolBarPenButtonProperties
{
public:
    ForwardRefToBaseReferenceTracker(InkToolBarToolButton)

    InkToolBarPenButton()
    {
        // Initialize default palette with common colors
        m_palette = winrt::single_threaded_vector<winrt::Brush>();
        InitializeDefaultPalette();
    }

    // These functions are ambiguous with InkToolBarToolButton, disambiguate
    using InkToolBarPenButtonProperties::EnsureProperties;
    using InkToolBarPenButtonProperties::ClearProperties;

    winrt::IVector<winrt::Brush> Palette() { return m_palette; }
    void Palette(winrt::IVector<winrt::Brush> value) { m_palette = value; }
    
    double MinStrokeWidth() { return m_minStrokeWidth; }
    void MinStrokeWidth(double value) { m_minStrokeWidth = value; }
    
    double MaxStrokeWidth() { return m_maxStrokeWidth; }
    void MaxStrokeWidth(double value) { m_maxStrokeWidth = value; }
    
    winrt::Brush SelectedBrush() 
    { 
        if (m_palette && m_selectedBrushIndex >= 0 && static_cast<uint32_t>(m_selectedBrushIndex) < m_palette.Size())
        {
            return m_palette.GetAt(m_selectedBrushIndex);
        }
        return nullptr;
    }
    
    int32_t SelectedBrushIndex() { return m_selectedBrushIndex; }
    void SelectedBrushIndex(int32_t value) { m_selectedBrushIndex = value; }
    
    double SelectedStrokeWidth() { return m_selectedStrokeWidth; }
    void SelectedStrokeWidth(double value) { m_selectedStrokeWidth = value; }

protected:
    // Replace the palette with an explicit color set. Used by subclasses (e.g. the
    // highlighter) to install their WinUI 2 parity palette.
    void SetPaletteColors(winrt::Windows::UI::Color const* colors, uint32_t count)
    {
        m_palette.Clear();
        for (uint32_t i = 0; i < count; i++)
        {
            m_palette.Append(winrt::SolidColorBrush(colors[i]));
        }
    }

private:
    void InitializeDefaultPalette()
    {
        // WinUI 2 InkToolbarBallpointPenButton / InkToolbarPencilButton default
        // palette (30 colors, ARGB) ported verbatim from the OS inking controls so
        // the lifted InkToolBar matches WinUI 2 exactly. Color layout is { A, R, G, B }.
        static const winrt::Windows::UI::Color s_defaultColors[] =
        {
            { 0xFF, 0x00, 0x00, 0x00 }, { 0xFF, 0xFF, 0xFF, 0xFF }, { 0xFF, 0xD1, 0xD3, 0xD4 },
            { 0xFF, 0xA7, 0xA9, 0xAC }, { 0xFF, 0x80, 0x82, 0x85 }, { 0xFF, 0x58, 0x59, 0x5B },
            { 0xFF, 0xB3, 0x15, 0x64 }, { 0xFF, 0xE6, 0x1B, 0x1B }, { 0xFF, 0xFF, 0x55, 0x00 },
            { 0xFF, 0xFF, 0xAA, 0x00 }, { 0xFF, 0xFF, 0xCE, 0x00 }, { 0xFF, 0xFF, 0xE6, 0x00 },
            { 0xFF, 0xA2, 0xE6, 0x1B }, { 0xFF, 0x26, 0xE6, 0x00 }, { 0xFF, 0x00, 0x80, 0x55 },
            { 0xFF, 0x00, 0xAA, 0xCC }, { 0xFF, 0x00, 0x4D, 0xE6 }, { 0xFF, 0x3D, 0x00, 0xB8 },
            { 0xFF, 0x66, 0x00, 0xCC }, { 0xFF, 0x60, 0x00, 0x80 }, { 0xFF, 0xF7, 0xD7, 0xC4 },
            { 0xFF, 0xBB, 0x91, 0x67 }, { 0xFF, 0x8E, 0x56, 0x2E }, { 0xFF, 0x61, 0x3D, 0x30 },
            { 0xFF, 0xFF, 0x80, 0xFF }, { 0xFF, 0xFF, 0xC6, 0x80 }, { 0xFF, 0xFF, 0xFF, 0x80 },
            { 0xFF, 0x80, 0xFF, 0x9E }, { 0xFF, 0x80, 0xD6, 0xFF }, { 0xFF, 0xBC, 0xB3, 0xFF }
        };
        SetPaletteColors(s_defaultColors, static_cast<uint32_t>(ARRAYSIZE(s_defaultColors)));
    }

    winrt::IVector<winrt::Brush> m_palette{ nullptr };
    double m_minStrokeWidth{ 1.0 };
    double m_maxStrokeWidth{ 24.0 };
    int32_t m_selectedBrushIndex{ 0 };
    double m_selectedStrokeWidth{ 2.0 };
};

