// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ThemingInterop.h"

class SystemThemingInterop final : public IThemingInterop
{
public:
    // IThemingInterop
    Theming::Theme GetSystemTheme() override;
    Theming::Theme GetSystemHighContrastTheme() override;
    unsigned int GetSystemColor(int colorId) override;
    unsigned int GetSystemAccentColor() override;
    unsigned int GetSystemVariantAccentColor(IThemingInterop::VariantAccentColors colorIndex) override;

    // Test Hooks
    static void OverrideSystemTheme(Theming::Theme theme);
    static void OverrideHighContrast(std::shared_ptr<std::list<std::pair<int, unsigned int>>> sysColorPalette);
    static void OverrideAccentColor(unsigned int accentColor);
    static void RemoveThemingOverrides();
    static void OverrideVariantAccentColor(std::shared_ptr<std::list<std::pair<IThemingInterop::VariantAccentColors, unsigned int>>> variantAccentColor);

private:
    bool m_initializedUISettings3 = false;
    ctl::ComPtr<wuv::IUISettings3> m_spUISettings3;

    static bool                                                     s_hasSystemThemeOverride;
    static Theming::Theme                                           s_systemThemeOverride;

    static bool                                                     s_hasHighContrastOverride;
    static std::shared_ptr<std::list<std::pair<int, unsigned int>>> s_sysColorPaletteOverride;

    static bool                                                     s_hasAccentColorOverride;
    static unsigned int                                             s_accentColorOverride;

    unsigned int ConvertColorToIntValue(_In_ wu::Color& color);

}; // class SystemThemingInterop
