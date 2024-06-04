// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Theming {
    enum class Theme : uint8_t;
}

struct IThemingInterop
{
    enum class VariantAccentColors
    {
        SystemAccentColor,
        SystemAccentColorDark1,
        SystemAccentColorDark2,
        SystemAccentColorDark3,
        SystemAccentColorLight1,
        SystemAccentColorLight2,
        SystemAccentColorLight3
    };

    // Returns either ThemeLight or ThemeDark.
    virtual Theming::Theme GetSystemTheme() = 0;

    // Returns ThemeHighContrastNone if off, correct high-contrast
    // theme otherwise.
    virtual Theming::Theme GetSystemHighContrastTheme() = 0;

    virtual unsigned int GetSystemColor(int colorId) = 0;

    virtual unsigned int GetSystemAccentColor() = 0;

    virtual unsigned int GetSystemVariantAccentColor(VariantAccentColors colorIndex) = 0;

}; // class IThemingInterop
