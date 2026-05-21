// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ThemingInterop.h"

class TestThemingInterop : public IThemingInterop
{
public:
    TestThemingInterop();

    void SetSystemTheme(Theming::Theme theme);
    void SetSystemHighContrastTheme(Theming::Theme theme);
    void SetSystemAccentColor(unsigned int color);

    //
    // IThemingInterop
    //
    Theming::Theme GetSystemTheme() override;
    Theming::Theme GetSystemHighContrastTheme() override;
    unsigned int GetSystemColor(int colorId) override;
    unsigned int GetSystemAccentColor() override;
    unsigned int GetSystemVariantAccentColor(VariantAccentColors colorIndex) override;

private:
    Theming::Theme m_systemTheme;
    Theming::Theme m_highContrastTheme;
    unsigned int m_accentColor;

}; // class SystemThemingInterop
