// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Faithful C++/WinRT port of onecoreuap\...\inkcontrols\lib\InkToolbarPenButton_Partial.cpp.
// OS-dxaml scaffolding (composable ToolButton factory, QI override, HRESULT/boxing) dropped.
// High-contrast palette switching is wired via AccessibilitySettings; the internal per-color adjust
// (IInkPresenterInternal2::GetHighContrastAdjustedInkColor) is replaced by a WCAG >=7.0 contrast filter.

#include "pch.h"
#include "common.h"
#include "InkToolbarPenButton.h"
#include <winrt/Windows.UI.ViewManagement.h>
#include "InkToolbarTrace.h"

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

// UWP SetSelectedBrushByIndex: index into the active palette (high-contrast when HC is on, else normal).
void InkToolbarPenButton::SetSelectedBrushByIndex(int index)
{
    if (index < 0)
    {
        return;
    }

    bool highContrast = IsHighContrast();
    auto palette = (highContrast && m_highContrastPalette) ? m_highContrastPalette : Palette();
    if (palette && static_cast<uint32_t>(index) < palette.Size())
    {
        SelectedBrush(palette.GetAt(index));
        if (highContrast) { m_highContrastModeBrushIndex = index; }
        else { m_normalModeBrushIndex = index; }
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

bool InkToolbarPenButton::IsHighContrast()
{
    try
    {
        if (!m_accessibilitySettings)
        {
            m_accessibilitySettings = winrt::Windows::UI::ViewManagement::AccessibilitySettings();
        }
        return m_accessibilitySettings.HighContrast();
    }
    catch (winrt::hresult_error const& e)
    {
        InkToolbarLogHResult(e.code(), L"high-contrast state query");
        return false;
    }
}

// Relative luminance of an sRGB color (0..1), WCAG 2.1 definition. This is a faithful reimplementation
// of the OS ink engine's own high-contrast math in
// onecoreuap/windows/AdvCore/WinRT/DirectInk/Helpers/HighContrastHelper.cpp (GetLuminance /
// ClampLuminanceValue / FindContrastRatio, MinContrastRatio = 7.0f). UWP reached it through the internal
// IInkPresenterInternal2::GetHighContrastAdjustedInkColor, which is not projected to lifted, so we
// compute the identical formula and 7.0 threshold here.
double InkToolbarPenButton::RelativeLuminance(winrt::Windows::UI::Color const& c)
{
    auto channel = [](double v)
    {
        v /= 255.0;
        return v <= 0.03928 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * channel(c.R) + 0.7152 * channel(c.G) + 0.0722 * channel(c.B);
}

// WCAG contrast ratio between two colors (1..21).
double InkToolbarPenButton::ContrastRatio(winrt::Windows::UI::Color const& a, winrt::Windows::UI::Color const& b)
{
    double la = RelativeLuminance(a);
    double lb = RelativeLuminance(b);
    double hi = (std::max)(la, lb);
    double lo = (std::min)(la, lb);
    return (hi + 0.05) / (lo + 0.05);
}

// Build the high-contrast pen-flyout palette, matching the OS ink engine's HighContrastHelper /
// GetHighContrastAdjustedInkColor (onecoreuap/windows/AdvCore/WinRT/DirectInk/Helpers/HighContrastHelper.cpp).
// Branch on InkPresenter.HighContrastAdjustment as UWP does: UseOriginalColors keeps every color,
// UseSystemColors keeps none, and UseSystemColorsWhenNecessary keeps colors whose contrast against the
// system Background is >= 7.0 (MinContrastRatio); the system color is appended when not already present.
// UWP reached the per-color check through IInkPresenterInternal2 (not in lifted); we compute the same
// WCAG contrast and 7.0 threshold inline.
winrt::IVector<winrt::Brush> InkToolbarPenButton::GetHighContrastPalette(int highContrastAdjustment)
{
    auto hcPalette = winrt::single_threaded_vector<winrt::Brush>();
    try
    {
        winrt::Windows::UI::ViewManagement::UISettings settings;
        // UWP resolves the system color by tool type: Highlight for the highlighter, WindowText for pens.
        auto systemColor = try_as<winrt::InkToolbarHighlighterButton>()
            ? settings.UIElementColor(winrt::Windows::UI::ViewManagement::UIElementType::Highlight)
            : settings.UIElementColor(winrt::Windows::UI::ViewManagement::UIElementType::WindowText);
        // Match the OS reference background: HighContrastHelper uses UIElementType_Background (PageBackground fallback).
        winrt::Windows::UI::Color background{};
        try { background = settings.UIElementColor(winrt::Windows::UI::ViewManagement::UIElementType::Background); }
        catch (winrt::hresult_error const&) { background = settings.UIElementColor(winrt::Windows::UI::ViewManagement::UIElementType::PageBackground); }

        bool systemPresent = false;
        if (auto palette = Palette())
        {
            for (auto const& brush : palette)
            {
                if (auto solid = brush.try_as<winrt::SolidColorBrush>())
                {
                    auto color = solid.Color();
                    bool keep = false;
                    switch (highContrastAdjustment)
                    {
                    case 2: keep = true; break;                                    // UseOriginalColors
                    case 1: keep = false; break;                                   // UseSystemColors
                    default: keep = ContrastRatio(color, background) >= 7.0; break; // UseSystemColorsWhenNecessary
                    }
                    if (keep)
                    {
                        hcPalette.Append(brush);
                        if (color.R == systemColor.R && color.G == systemColor.G && color.B == systemColor.B && color.A == systemColor.A)
                        {
                            systemPresent = true;
                        }
                    }
                }
            }
        }
        if (!systemPresent)
        {
            hcPalette.Append(winrt::SolidColorBrush(systemColor));
        }
    }
    catch (winrt::hresult_error const& e)
    {
        InkToolbarLogHResult(e.code(), L"high-contrast palette build");
    }
    m_highContrastPalette = hcPalette;
    return hcPalette;
}
