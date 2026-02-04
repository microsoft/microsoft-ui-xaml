// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TestThemingInterop.h"
#include "Theme.h"

TestThemingInterop::TestThemingInterop()
    : m_systemTheme(Theming::Theme::Dark)
    , m_highContrastTheme(Theming::Theme::HighContrastNone)
    , m_accentColor(0xFF000000)
{
}

void TestThemingInterop::SetSystemTheme(Theming::Theme theme)
{
    m_systemTheme = theme;
}

void TestThemingInterop::SetSystemHighContrastTheme(Theming::Theme theme)
{
    m_highContrastTheme = theme;
}

void TestThemingInterop::SetSystemAccentColor(unsigned int color)
{
    m_accentColor = color;
}

Theming::Theme TestThemingInterop::GetSystemTheme()
{
    return m_systemTheme;
}

Theming::Theme TestThemingInterop::GetSystemHighContrastTheme()
{
    return m_highContrastTheme;
}

unsigned int TestThemingInterop::GetSystemColor(int colorId)
{
    return static_cast<unsigned int>(colorId);
}

unsigned int TestThemingInterop::GetSystemAccentColor()
{
    return m_accentColor;
}

unsigned int TestThemingInterop::GetSystemVariantAccentColor(VariantAccentColors colorIndex)
{
    return m_accentColor + static_cast<unsigned int>(colorIndex);
}
