// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Faithful C++/WinRT port of onecoreuap\...\inkcontrols\lib\InkToolbarPenButton_Partial.cpp.
// OS-dxaml scaffolding (composable ToolButton factory, QI override, HRESULT/boxing) dropped.
// High-contrast palette switching (AccessibilitySettings + IInkPresenterInternal2 color adjust)
// is a lifted-platform delta and is not wired yet; the normal palette is used.

#include "pch.h"
#include "common.h"
#include "InkToolbarPenButton.h"

InkToolbarPenButton::InkToolbarPenButton()
{
}

// UWP GetColors default (Ballpoint/Pencil share this 30-color WinUI2 palette; Highlighter overrides).
std::vector<winrt::Windows::UI::Color> InkToolbarPenButton::GetColors()
{
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
    return std::vector<winrt::Windows::UI::Color>(std::begin(s_defaultColors), std::end(s_defaultColors));
}

// UWP OnApplyTemplateImpl: if the app didn't supply a Palette via markup, populate the default one
// from GetColors(); then sync SelectedBrush to SelectedBrushIndex. Runs from the base
// InkToolbarToolButton::OnApplyTemplate via the OnApplyTemplateCore hook.
void InkToolbarPenButton::OnApplyTemplateCore()
{
    auto palette = Palette();
    m_normalModeBrushIndex = SelectedBrushIndex();

    if (!palette || palette.Size() == 0)
    {
        auto colors = GetColors();
        auto brushes = winrt::single_threaded_vector<winrt::Brush>();
        for (auto const& color : colors)
        {
            brushes.Append(winrt::SolidColorBrush(color));
        }
        Palette(brushes);
    }

    SetSelectedBrushByIndex(SelectedBrushIndex());
    UpdatePenButtonHelpText();
}

void InkToolbarPenButton::OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    auto property = args.Property();
    if (property == winrt::InkToolbarPenButton::SelectedBrushIndexProperty())
    {
        SetSelectedBrushByIndex(winrt::unbox_value<int>(args.NewValue()));
        UpdatePenButtonHelpText();
    }
    else if (property == winrt::InkToolbarPenButton::PaletteProperty())
    {
        SetSelectedBrushByIndex(SelectedBrushIndex());
        UpdatePenButtonHelpText();
    }
    else
    {
        InkToolbarToolButton::OnPropertyChanged(args);
    }
}

// UWP SetSelectedBrushByIndex (normal palette; high-contrast palette is the delta noted above).
void InkToolbarPenButton::SetSelectedBrushByIndex(int index)
{
    if (index < 0)
    {
        return;
    }

    auto palette = Palette();
    if (palette && static_cast<uint32_t>(index) < palette.Size())
    {
        SelectedBrush(palette.GetAt(index));
        m_normalModeBrushIndex = index;
    }
}

// UWP UpdatePenButtonHelpText: expose the selected color's name via AutomationProperties.HelpText.
void InkToolbarPenButton::UpdatePenButtonHelpText()
{
    auto selectedBrush = SelectedBrush();
    if (auto solid = selectedBrush.try_as<winrt::SolidColorBrush>())
    {
        bool isGenericFormat = false;
        auto name = m_colorNames.GetColorName(solid.Color(), isGenericFormat);
        // UWP skips HelpText for generic (unnamed) colors; the lift only has generic names for now.
        if (!isGenericFormat)
        {
            winrt::AutomationProperties::SetHelpText(*this, name);
        }
    }
}

// UWP DetermineStrokeWidth: the config slider's SelectedStrokeWidth clamped to [Min, Max].
float InkToolbarPenButton::DetermineStrokeWidth(winrt::InkToolbarPenButton const& penButton)
{
    double width = penButton.SelectedStrokeWidth();
    double minWidth = penButton.MinStrokeWidth();
    double maxWidth = penButton.MaxStrokeWidth();
    if (width < minWidth) { width = minWidth; }
    if (maxWidth > 0 && width > maxWidth) { width = maxWidth; }
    return static_cast<float>(width);
}
