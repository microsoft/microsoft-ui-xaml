// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SystemThemingInterop.h"
#include <Windows.UI.ViewManagement.h>

#include <xamlbehaviormode.h>

#include <colorutil.h>
#include <FxCallbacks.h>
#include "theming\inc\Theme.h"

bool SystemThemingInterop::s_hasSystemThemeOverride = false;
Theming::Theme SystemThemingInterop::s_systemThemeOverride = Theming::Theme::None;

bool SystemThemingInterop::s_hasHighContrastOverride = false;
std::shared_ptr<std::list<std::pair<int, unsigned int>>> SystemThemingInterop::s_sysColorPaletteOverride;

bool SystemThemingInterop::s_hasAccentColorOverride = false;
unsigned int SystemThemingInterop::s_accentColorOverride = 0;

using namespace Theming;

// Light/dark detection (based on text color)
inline bool ColorCheckHelper_IsLightThemeColor(wu::Color color)
{
    // It's possible to get a little more accurate than this via (30*r) + (59*g) + (11*b) >= 12750,
    // but there's a lot of code around windows that uses this simplified check so we're going to stick with that.
    return ((5 * color.G + 2 * color.R + color.B) > 8 * 128);
}

Theme SystemThemingInterop::GetSystemTheme()
{
    auto systemTheme = Theme::Dark;

    // Return the override value if one has been set, such as during test runs.
    if (s_hasSystemThemeOverride)
    {
        return s_systemThemeOverride;
    }

    // Scope using UISettings to Windows.Core platform only since this is where we need theme state to be
    // based on the color changes. The long term ideal is to expand this to be the default way to check light/dark on all SKUs.
    ULONG platformId = 0;

    ::RtlGetDeviceFamilyInfoEnum(nullptr, &platformId, nullptr);

    if (platformId == DEVICEFAMILYINFOENUM_WINDOWS_CORE)
    {
        // IUISettings GetColorValue for UIColorType_Background will retrieve the current background color.
        // Ideally we should have a helper API which should tell us if we are in light mode or dark mode, but for
        // now we are using ColorCheckHelper::IsLightColor's implementation to determine what mode we are in.
        wrl::ComPtr<wuv::IUISettings3> spUISettings;

        if (SUCCEEDED(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Windows_UI_ViewManagement_UISettings).Get(), &spUISettings)))
        {
            wu::Color color = { };

            if (SUCCEEDED(spUISettings->GetColorValue(wuv::UIColorType_Background, &color)))
            {
                return ColorCheckHelper_IsLightThemeColor(color) ? Theme::Light : Theme::Dark;
            }
        }
    }

    //On certain platforms (for e.g. Hololens), Shell API to query for the default theme doesn't exist.
    //We (XAML & MRT), for consistency, have agreed to query the reg key: SOFTWARE\Microsoft\Windows\CurrentVersion\Themes\Personalize\AppsUseLightTheme and SystemUsesLightTheme
    //until we have a (Shell) converged way to query the theme value on all platforms
    DWORD reg_useLightThemeValue = 0;
    DWORD size = sizeof(reg_useLightThemeValue);
    DWORD dataType = 0;
    const wchar_t* const registryKey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";

    auto returnVal = RegGetValueW(
        HKEY_CURRENT_USER,
        registryKey,
        L"AppsUseLightTheme",
        RRF_RT_REG_DWORD,
        &dataType,
        &reg_useLightThemeValue,
        &size);

    if (returnVal == ERROR_SUCCESS)
    {
        systemTheme = reg_useLightThemeValue == 0 ? Theme::Dark : Theme::Light;
    }
    else
    {
        // If the user-specific setting has not been set, we try to check if system-wide setting is set
        size = sizeof(reg_useLightThemeValue);
        returnVal = RegGetValueW(
            HKEY_LOCAL_MACHINE,
            registryKey,
            L"SystemUsesLightTheme",
            RRF_RT_REG_DWORD,
            &dataType,
            &reg_useLightThemeValue,
            &size);

        if (returnVal == ERROR_SUCCESS)
        {
            systemTheme = reg_useLightThemeValue == 0 ? Theme::Dark : Theme::Light;
        }
        else
        {
            wrl::ComPtr<wuv::IUISettings3> uiSettings;
            if (SUCCEEDED(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Windows_UI_ViewManagement_UISettings).Get(), &uiSettings)))
            {
                wu::Color value = {};
                VERIFYHR(uiSettings->GetColorValue(wuv::UIColorType::UIColorType_Background, &value));
                unsigned int color = ConvertColorToIntValue(value);
                systemTheme = color == 0xFFFFFFFF ? Theme::Light : Theme::Dark;
            }
        }
    }

    return systemTheme;
}

Theme SystemThemingInterop::GetSystemHighContrastTheme()
{
    auto highContrastTheme = Theme::HighContrastNone;

    // Return the override value if one has been set, such as during test runs.
    if (s_hasHighContrastOverride)
    {
        unsigned int crForeground = GetSystemColor(COLOR_WINDOWTEXT);
        unsigned int crBackground = GetSystemColor(COLOR_WINDOW);

        if (0xFFFFFFFF == crBackground && 0xFF000000 == crForeground)
        {
            highContrastTheme = Theme::HighContrastWhite;
        }
        else if (0xFF000000 == crBackground && 0xFFFFFFFF == crForeground)
        {
            highContrastTheme = Theme::HighContrastBlack;
        }
        else if (0xFFEEEEEE == crBackground && 0xFF111111 == crForeground)
        {
            highContrastTheme = Theme::HighContrastCustom;
        }
        else
        {
            highContrastTheme = Theme::HighContrast;
        }

        return highContrastTheme;
    }

    HIGHCONTRASTW highContrast = {};
    highContrast.cbSize = sizeof(HIGHCONTRASTW);

    // check if high contrast is on
    if (SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(HIGHCONTRASTW), &highContrast, 0)
        && (HCF_HIGHCONTRASTON & highContrast.dwFlags))
    {
        unsigned int crForeground = (unsigned int)GetSysColor(COLOR_WINDOWTEXT);
        unsigned int crBackground = (unsigned int)GetSysColor(COLOR_WINDOW);

        // Redstone Bug #6417331: Xbox uses video-safe black (0x101010) and white (0xEBEBEB) when in High Contrast
        if ((0x00FFFFFF == crBackground && 0x0 == crForeground) || (0x00EBEBEB == crBackground && 0x00101010 == crForeground))
        {
            highContrastTheme = Theme::HighContrastWhite;
        }
        else if ((0x0 == crBackground && 0x00FFFFFF == crForeground) || (0x00101010 == crBackground && 0x00EBEBEB == crForeground))
        {
            highContrastTheme = Theme::HighContrastBlack;
        }
        else
        {
            highContrastTheme = Theme::HighContrastCustom;
        }
    }

    return highContrastTheme;
}

unsigned int SystemThemingInterop::GetSystemColor(int colorId)
{
    // Return the override value if one has been set, such as during test runs.
    if (s_hasHighContrastOverride)
    {
        // Default to an easily findable color value for entries not in
        // our override color palette.  This should let us find instances
        // where brushes are referencing system colors that don't
        // correspond to the user-configurable colors.
        unsigned int color = 0xFFAABBCC;

        for (const auto& idColorPair : *s_sysColorPaletteOverride)
        {
            if (idColorPair.first == colorId)
            {
                return idColorPair.second;
                break;
            }
        }

        return color;
    }

    return ConvertFromABGRToARGB(GetSysColor(colorId) | 0xFF000000);
}

unsigned int SystemThemingInterop::GetSystemAccentColor()
{
    // Return the override value if one has been set, such as during test runs.
    if (s_hasAccentColorOverride)
    {
        return s_accentColorOverride;
    }

    return GetSystemVariantAccentColor(VariantAccentColors::SystemAccentColor);
}

unsigned int SystemThemingInterop::GetSystemVariantAccentColor(VariantAccentColors colorIndex)
{
    // Return the override value if one has been set, such as during test runs.
    if (s_hasAccentColorOverride)
    {
        // For now when there's an override just return hard-coded colors for the variants. The important factor
        // is that these colors are stable across different themes/platforms. If we wanted to get fancy we could
        // do alpha blend logic of the override color over white/black to get these variants.
        switch (colorIndex)
        {
        case VariantAccentColors::SystemAccentColorDark1:
            return 0xFF005A9E;
        case VariantAccentColors::SystemAccentColorDark2:
            return 0xFF004275;
        case VariantAccentColors::SystemAccentColorDark3:
            return 0xFF002642;
        case VariantAccentColors::SystemAccentColorLight1:
            return 0xFF429CE3;
        case VariantAccentColors::SystemAccentColorLight2:
            return 0xFF76B9ED;
        case VariantAccentColors::SystemAccentColorLight3:
            return 0xFFA6D8FF;
        case VariantAccentColors::SystemAccentColor:
        default:
            return s_accentColorOverride;
        }
    }

    // We expect that every SKU should support IUISettings3->GetColorValue(UIColorType_Accent), since that's a public API that 3rd party apps can
    // also use to query the accent color.

    if (!m_initializedUISettings3)
    {
        ctl::ComPtr<wuv::IUISettings> spUISettings;

        if (SUCCEEDED(FxCallbacks::DXamlCore_GetUISettings(spUISettings)))
        {
            IGNOREHR(spUISettings.As(&m_spUISettings3));
        }

        m_initializedUISettings3 = true;
    }

    if (m_spUISettings3)
    {
        wu::Color accentColor = { 0x00, 0x00, 0x00, 0x00 };
        wuv::UIColorType uiColorType = wuv::UIColorType_Accent;

        switch (colorIndex)
        {
            case VariantAccentColors::SystemAccentColorDark1:
                uiColorType = wuv::UIColorType_AccentDark1;
                break;

            case VariantAccentColors::SystemAccentColorDark2:
                uiColorType = wuv::UIColorType_AccentDark2;
                break;

            case VariantAccentColors::SystemAccentColorDark3:
                uiColorType = wuv::UIColorType_AccentDark3;
                break;

            case VariantAccentColors::SystemAccentColorLight1:
                uiColorType = wuv::UIColorType_AccentLight1;
                break;

            case VariantAccentColors::SystemAccentColorLight2:
                uiColorType = wuv::UIColorType_AccentLight2;
                break;

            case VariantAccentColors::SystemAccentColorLight3:
                uiColorType = wuv::UIColorType_AccentLight3;
                break;

            default:
                uiColorType = wuv::UIColorType_Accent;
                break;
        }

        if (SUCCEEDED(m_spUISettings3->GetColorValue(uiColorType, &accentColor)))
        {
            return ConvertColorToIntValue(accentColor);
        }
    }

    // If IUISettings3 is not available, we have a default color of fuchsia to make it hopefully-obvious that we need to fix something.
    return 0xFF000000 | RGB(255, 0, 255);
}

void SystemThemingInterop::OverrideSystemTheme(Theme theme)
{
    s_systemThemeOverride = theme;
    s_hasSystemThemeOverride = true;
}

void SystemThemingInterop::OverrideHighContrast(std::shared_ptr<std::list<std::pair<int, unsigned int>>> sysColorPalette)
{
    s_hasHighContrastOverride = static_cast<bool>(sysColorPalette);
    s_sysColorPaletteOverride = sysColorPalette;
}

void SystemThemingInterop::OverrideAccentColor(unsigned int accentColor)
{
    s_accentColorOverride = accentColor;
    s_hasAccentColorOverride = true;
}

void SystemThemingInterop::RemoveThemingOverrides()
{
    s_hasSystemThemeOverride = false;
    s_hasHighContrastOverride = false;
    s_hasAccentColorOverride = false;
}

unsigned int SystemThemingInterop::ConvertColorToIntValue(_In_ wu::Color& color)
{
    return color.A << 24 | color.R << 16 | color.G << 8 | color.B;
}
