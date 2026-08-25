// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "InkToolbarPenButton.g.h"
#include "InkToolbarPenButton.properties.h"

#include "InkToolbarToolButton.h"
#include "ColorNames.h"

#include <winrt/Windows.UI.ViewManagement.h>

// Faithful C++/WinRT port of onecoreuap\...\inkcontrols\lib\InkToolbarPenButton_Partial.{h,cpp}.
// Palette / SelectedBrush / SelectedBrushIndex / Min|MaxStrokeWidth / SelectedStrokeWidth are the
// generated DPs (InkToolbarPenButtonProperties). UWP's IPenButtonDerived is expressed as virtual
// methods overridden by the concrete pen buttons.
class InkToolbarPenButton :
    public winrt::implementation::InkToolbarPenButtonT<InkToolbarPenButton, InkToolbarToolButton, winrt::composable>,
    public InkToolbarPenButtonProperties
{
public:
    ForwardRefToBaseReferenceTracker(InkToolbarToolButton)

    InkToolbarPenButton();

    // Disambiguate the generated statics that exist on both bases.
    using InkToolbarPenButtonProperties::EnsureProperties;
    using InkToolbarPenButtonProperties::ClearProperties;

    // Populate the default palette when the template is applied. Invoked by
    // InkToolbarToolButton::OnApplyTemplate via the OnApplyTemplateCore hook.
    void OnApplyTemplateCore() override;

    // DP change routing.
    void OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);

    // UWP static: stroke width in DIPs from the pen button's SelectedStrokeWidth + Min/Max range.
    static float DetermineStrokeWidth(winrt::InkToolbarPenButton const& penButton);

    // UWP IPenButtonDerived (implemented by concrete leaf pen buttons).
    virtual bool SupportsDynamicStrokeWidthChange() { return true; }
    virtual winrt::InkDrawingAttributes CreateInkDrawingAttributes() { return nullptr; }
    virtual std::vector<winrt::Windows::UI::Color> GetColors();

    // High-contrast pen-flyout palette: the normal palette filtered to colors with >=7.0 WCAG contrast
    // vs the system background, plus the system color. Replaces UWP's internal GetHighContrastAdjustedInkColor.
    winrt::IVector<winrt::Brush> GetHighContrastPalette(int highContrastAdjustment);
    bool IsHighContrast();

protected:
    ~InkToolbarPenButton() {}

private:
    // Sets SelectedBrush from the active (normal or high-contrast) palette at the given index (UWP).
    void SetSelectedBrushByIndex(int index);
    // Pushes the selected color name into AutomationProperties.HelpText (UWP UpdatePenButtonHelpText).
    void UpdatePenButtonHelpText();
    static double RelativeLuminance(winrt::Windows::UI::Color const& c);
    static double ContrastRatio(winrt::Windows::UI::Color const& a, winrt::Windows::UI::Color const& b);

    ColorNames m_colorNames;
    int m_normalModeBrushIndex{ -1 };
    int m_highContrastModeBrushIndex{ -1 };
    // High-contrast palette (UWP m_highContrastPalette): the contrast-filtered colors shown in HC mode.
    winrt::IVector<winrt::Brush> m_highContrastPalette{ nullptr };
    winrt::Windows::UI::ViewManagement::AccessibilitySettings m_accessibilitySettings{ nullptr };
};

